#!/usr/bin/env python3
"""阶段 6 判定器的负向验证：证明它真的会红。

做法：读一份**已经通过**的 logs/s06.log，逐条改成"有缺陷"的样子（只改文本，
不重跑程序），再跑一次 judge.py，要求

  * rc == 1，且
  * 失败项正好落在预期的那条判据上（只红不看位置，等于没验）。

两条硬规矩：

1. **变异必须必然失效**。锚点找不到（日志格式变了、判据早空了）计 FAIL，
   不能当"跳过"——"没找到"和"通过了"是两件事。
2. **变异只能落在目标用例块内**，且数值锚点必须用正则。
   首版把 `syscalls=1034`、`median_ns=41.64`、`ns_per_rt=42354.8` 当字面量，
   换一次全量运行就有 3 条锚点丢失（读数本来就每次不同）——这条教训写进 README §7.7。

退出码：0 全部按预期被抓；1 有变异没被抓或锚点丢失。
"""
import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
LOG = os.path.join(HERE, "logs", "s06.log")
JUDGE = os.path.join(HERE, "judge.py")

BLOCK_START = re.compile(r"^S06\|case\|name=([^|]+)\|begin$", re.M)

# (名称, 目标块, 正则, 替换, 预期失败的 (case, check))
#   目标块 = None 表示日志开头的公共段（build / env 那几行不在任何用例块内）
MUTATIONS = [
    ("build.rc_nonzero", None,
     r"^S06\|build\|rc=0", "S06|build|rc=1", ("build", "rc")),
    ("env.strace_present_changed", None,
     r"^S06\|env\|tool=strace\|present=0", "S06|env|tool=strace|present=1", ("env", "tool.strace")),

    ("trace.fd_leak_masked", "trace-fdleak",
     r"net_fd=2", "net_fd=0", ("trace-fdleak", "trace.net_fd")),
    ("trace.forced_calls_inflated", "trace-vdso",
     r"clock_gettime=\d+l?\|execve=1", "clock_gettime=2000|execve=1", ("trace-vdso", "trace.clock_le_raw")),
    ("trace.syscall_total_faked", "trace-vdso",
     r"syscalls=\d+", "syscalls=3000", ("trace-vdso", "trace.non_clock_syscalls")),

    # 符号行由 symbolize.py 统一追加在日志末尾，物理上全在 crash-ok 块里；
    # judge 按行内 case= 归档，所以"改哪块"与"该哪条判据红"经常不是同一个名字。
    ("crash.frame_dropped", "crash-ok",
     r"func=f2\|", "func=f2X|", ("crash-null", "sym.has_f2")),
    ("crash.call_order_broken", "crash-ok",
     r"func=f1\|", "func=f2|", ("crash-null", "sym.call_order")),
    ("crash.ok_gained_signal", "crash-ok",
     r"(^S06\|case\|name=crash-ok\|begin$)",
     r"\1\nS06|crash|siginfo|sig=11|addr=(nil)|pc=0x1|frames=3", ("crash-ok", "crash.siginfo")),
    # abs 反例行由 symbolize.py 追加在最后一个用例块末尾，物理上属于 crash-ok 块，
    # 但 judge 按行内的 case= 归档，所以"改哪块"和"该哪条判据红"不是同一个名字。
    ("crash.abs_counterexample_gone", "crash-ok",
     r"S06\|crash\|abs\|", "S06|crash|absX|", ("crash-null", "sym.absolute_lookup")),

    ("asan.leak_kind_swapped", "asan-leak",
     r"kind=leak", "kind=heap-buffer-overflow", ("asan-leak", "sani.kind")),
    ("asan.ok_false_positive", "asan-ok",
     r"kind=none", "kind=heap-buffer-overflow", ("asan-ok", "sani.kind")),
    ("ubsan.message_altered", "ubsan-div",
     r"msg=division by zero", "msg=divide error", ("ubsan-div", "sani.msg")),
    ("tsan.atomic_gained_race", "tsan-atomic",
     r"race=0", "race=1", ("tsan-atomic", "sani.race")),
    ("tsan.race_rc_zeroed", "tsan-race",
     r"^S06\|case\|name=tsan-race\|rc=66", "S06|case|name=tsan-race|rc=0", ("tsan-race", "case.rc")),
    ("tsan.sani_rc_split", "tsan-race",
     r"^S06\|sani\|case=tsan-race\|rc=66", "S06|sani|case=tsan-race|rc=0",
     ("tsan-race", "sani.rc_matches_case")),
    ("tsan.atomic_lost_update", "tsan-atomic",
     r"S06\|tsan\|case=atomic\|result=\d+", "S06|tsan|case=atomic|result=1",
     ("tsan-atomic", "tsan.result_equals_expect")),

    ("bench.syscall_too_slow", "bench-syscall",
     r"median_ns=[\d.]+", "median_ns=2000.0", ("bench-syscall", "bench.median_ns")),
    ("bench.syscall_cheaper_than_vdso", "bench-syscall",
     r"median_ns=[\d.]+", "median_ns=0.1", ("bench-syscall", "bench.ratio_over_vdso")),
    ("bench.pagefault_missed_second_pass", "bench-pagefault",
     r"second_minflt=0", "second_minflt=5000", ("bench-pagefault", "bench.second_minflt_max")),
    ("bench.ctxswitch_too_cheap", "bench-ctxswitch",
     r"ns_per_rt=[\d.]+", "ns_per_rt=100.0", ("bench-ctxswitch", "bench.rt_over_syscall")),

    ("perf.systemwide_allowed", "perf",
     r"sys_errno=13", "sys_errno=0", ("perf", "perf.sys_errno")),
]

results = []


def report(name, mutated, rc, caught, expect, ok):
    results.append(ok)
    print(f"S06|neg|name={name}|mutated={mutated}|rc={rc}|caught={caught}"
          f"|expect={expect}|{'PASS' if ok else 'FAIL'}")


def run_judge(log_path):
    proc = subprocess.run([sys.executable, "-I", JUDGE, log_path],
                          capture_output=True, text=True, check=False)
    return proc.returncode, proc.stdout


def split_blocks(text):
    """按 `S06|case|name=X|begin` 切块，保持物理顺序。

    返回 [[name, chunk], ...]；第一项的 name 为 None，装 build/env 这些公共行。
    """
    marks = [(m.start(), m.group(1)) for m in BLOCK_START.finditer(text)]
    if not marks:
        return [[None, text]]
    blocks = [[None, text[:marks[0][0]]]]
    for i, (pos, name) in enumerate(marks):
        end = marks[i + 1][0] if i + 1 < len(marks) else len(text)
        blocks.append([name, text[pos:end]])
    return blocks


def main():
    try:
        with open(LOG, encoding="utf-8", errors="replace") as f:
            base = f.read()
    except OSError as exc:
        print(f"S06|neg|error|base_missing={exc}")
        return 2

    # 基线：原日志必须通过，否则下面所有变异都没有意义
    rc, out = run_judge(LOG)
    summary = next((ln for ln in out.splitlines() if ln.startswith("S06|judge|summary")), "?")
    report("baseline.clean_log", 1, rc, summary, "rc==0", rc == 0)
    if rc != 0:
        print("S06|neg|abort=baseline_failed")
        return 1

    # 缺日志必须返回 2（判定器不能把"没有证据"当"通过"）
    rc_missing, out_missing = run_judge(os.path.join(HERE, "logs", "__no_such_log__"))
    report("baseline.missing_log", 1, rc_missing,
           "log_missing" if "log_missing" in out_missing else "?",
           "rc==2", rc_missing == 2)

    blocks = split_blocks(base)
    index = {name: i for i, (name, _) in enumerate(blocks)}
    tmpdir = os.path.join(HERE, "logs")

    for name, block, pattern, repl, (case, check) in MUTATIONS:
        if block not in index:
            report(name, 0, "-", "-", f"{case}/{check}", False)
            continue
        i = index[block]
        mutated_chunk, n = re.subn(pattern, repl, blocks[i][1], flags=re.M)
        if n == 0:
            report(name, 0, "-", "-", f"{case}/{check}", False)
            continue
        parts = [chunk if j != i else mutated_chunk for j, (_, chunk) in enumerate(blocks)]
        mutated = "".join(parts)

        fd, path = tempfile.mkstemp(prefix="negtest_", suffix=".log", dir=tmpdir)
        try:
            with os.fdopen(fd, "w", encoding="utf-8") as f:
                f.write(mutated)
            rc, out = run_judge(path)
        finally:
            os.unlink(path)
        hits = [ln for ln in out.splitlines()
                if f"|case={case}|check={check}|" in ln and ln.endswith("|FAIL")]
        report(name, n, rc, len(hits), f"{case}/{check}", rc == 1 and bool(hits))

    total = len(results)
    passed = sum(1 for r in results if r)
    print(f"S06|neg|summary|cases={total}|pass={passed}|fail={total - passed}")
    return 0 if passed == total else 1


if __name__ == "__main__":
    sys.exit(main())
