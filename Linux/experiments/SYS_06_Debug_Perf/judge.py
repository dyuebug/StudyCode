#!/usr/bin/env python3
"""阶段 6 判定器：只断言"读数之间的关系"与"工具输出的原话"。

判据选取原则（与前面各阶段一致）：
  * 不写死时间/地址这类每次运行都会漂的值，只比较同一份日志内两个读数的关系；
  * sanitizer 的期望值取自**工具自己打印的类别名**，不取自我们注入时用的用例名；
  * 环境一旦变化（比如某天 strace 装上了），相关断言应当失败并提醒复核文档，
    这是特性不是缺陷。

退出码：0 全通过；1 有 FAIL；2 日志缺失或不可解析。
"""
import sys

LOG = sys.argv[1] if len(sys.argv) > 1 else "logs/s06.log"


def field(line, key):
    for part in line.split("|"):
        if part.startswith(key + "="):
            return part[len(key) + 1:]
    return None


def load(path):
    with open(path, encoding="utf-8", errors="replace") as f:
        lines = f.read().splitlines()
    segs, syms, case = {}, {}, None
    for ln in lines:
        if ln.startswith("S06|case|name="):
            case = field(ln, "name")
            # begin 与 rc 两行都属于该用例，必须一起入段，否则 case_rc() 取不到
            segs.setdefault(case, []).append(ln)
            continue
        if ln.startswith("S06|sani|case="):
            case = field(ln, "case")
            segs.setdefault(case, []).append(ln)
            continue
        if ln.startswith("S06|crash|sym|") or ln.startswith("S06|crash|abs|"):
            syms.setdefault(field(ln, "case"), []).append(ln)
            continue
        if case is not None:
            segs[case].append(ln)
    return lines, segs, syms


LINES, SEGS, SYMS = [], {}, {}
# run.sh 的 run_case 与 sani_case 各写一行 rc，同值重复；下面的一致性判据盯着它们不能分叉
SANI_CASES = [f"{s}-{m}" for s in ("asan", "ubsan", "tsan")
              for m in ("ok", "heap-overflow", "uaf", "stack-overflow", "leak",
                        "shift", "ovf", "div", "null", "race", "atomic", "mutex")]

results = []


def emit(case, check, actual, expect, ok):
    results.append(ok)
    tag = "PASS" if ok else "FAIL"
    print(f"S06|judge|case={case}|check={check}|actual={actual}|expect={expect}|{tag}")


def inform(case, check, actual):
    print(f"S06|judge|info|case={case}|check={check}|actual={actual}")


def values(case, module, key):
    out = []
    for ln in SEGS.get(case, []):
        if ln.startswith(f"S06|{module}|"):
            v = field(ln, key)
            if v is not None:
                out.append(v)
    return out


def num(case, module, key, occ=0):
    vs = values(case, module, key)
    if not vs:
        return None
    try:
        return float(vs[occ if occ >= 0 else len(vs) + occ])
    except ValueError:
        return None


def text(case, module, key, occ=0):
    vs = values(case, module, key)
    return vs[occ] if len(vs) > occ else None


def case_rc(name):
    vs = values(name, "case", "rc")
    return int(vs[-1]) if vs else None


def check(name, check_id, actual, expect, ok):
    emit(name, check_id, "?" if actual is None else actual, expect, bool(ok))


def check_num(name, module, key, want, rel="==", occ=0):
    v = num(name, module, key, occ)
    if v is None:
        check(name, key, None, f"{rel}{want}", False)
        return
    ok = {"==": v == want, ">=": v >= want, "<=": v <= want}.get(rel, False)
    emit(name, f"{module}.{key}", v, f"{rel} {want}", ok)


def check_str(name, module, key, want, occ=0):
    v = text(name, module, key, occ)
    emit(name, f"{module}.{key}", v, f"str:{want}", v == want)


def main():
    global LINES, SEGS, SYMS
    LINES, SEGS, SYMS = load(LOG)

    # ---------- 构建与环境 ----------
    build_rc = None
    for ln in LINES:
        if ln.startswith("S06|build|rc="):
            build_rc = field(ln, "rc")
    check("build", "rc", build_rc, "str:0", build_rc == "0")
    linked = [ln for ln in LINES if ln.startswith("S06|build|sanitizer|") and field(ln, "linked") == "1"]
    emit("build", "sanitizer.linked", len(linked), "== 3", len(linked) == 3)

    def tool(name):
        for ln in LINES:
            if ln.startswith("S06|env|tool=") and field(ln, "tool") == name:
                return field(ln, "present")
        return None

    for t, expect in (("addr2line", "1"), ("nm", "1"), ("strace", "0"),
                      ("gdb", "0"), ("perf", "0"), ("valgrind", "0")):
        got = tool(t)
        emit("env", f"tool.{t}", got, f"str:{expect}", got == expect)

    # ---------- 1. ptrace 版 strace ----------
    check_num("trace-fdok", "trace", "net_fd", 0)
    check_num("trace-fdleak", "trace", "net_fd", 2)
    o = num("trace-fdleak", "trace", "opens")
    c = num("trace-fdleak", "trace", "closes")
    emit("trace-fdleak", "trace.opens_minus_closes",
         None if o is None or c is None else int(o - c), "== 2",
         o is not None and c is not None and o - c == 2)

    check_num("trace-vdso", "trace", "clock_gettime", 900, ">=")
    check_num("trace-vdso", "trace", "clock_gettime", 1100, "<=")
    check_num("trace-vdso", "trace", "syscalls", 1500, "<=")
    raw = num("trace-vdso", "victim", "raw_calls")
    seen = num("trace-vdso", "trace", "clock_gettime")
    emit("trace-vdso", "trace.clock_le_raw",
         None if seen is None or raw is None else int(seen), f"<= raw_calls={raw}",
         seen is not None and raw is not None and seen <= raw)
    emit("trace-vdso", "trace.clock_ge_90pct_raw",
         None if seen is None or raw is None else round(seen / raw, 3), ">= 0.9",
         seen is not None and raw is not None and raw > 0 and seen / raw >= 0.9)
    # 交叉核对：总 syscall 数减去被强制的 1000 次，剩下的只应是加载器与写日志那几十次。
    # 若 1000 次 vDSO 调用也进了内核，这个差值会跳到 1000 以上（syscalls≈2034）。
    total_sc = num("trace-vdso", "trace", "syscalls")
    overhead = None if total_sc is None or seen is None else int(total_sc - seen)
    emit("trace-vdso", "trace.non_clock_syscalls", overhead, "0 <= n <= 100",
         overhead is not None and 0 <= overhead <= 100)

    for name in ("trace-fdok", "trace-fdleak", "trace-vdso"):
        check_num(name, "trace", "execve", 1)
        check_num(name, "trace", "child_rc", 0)
        check(name, "case.rc", case_rc(name), "str:0", case_rc(name) == 0)

    # ---------- 2. 崩溃回溯 ----------
    check_num("crash-null", "crash", "sig", 11)
    check_str("crash-null", "crash", "addr", "(nil)")
    check_num("crash-null", "crash", "frames", 6, ">=")
    check_num("crash-deep", "crash", "frames", 60, ">=")
    check("crash-ok", "crash.siginfo", len(values("crash-ok", "crash", "sig")), "== 0",
          len(values("crash-ok", "crash", "sig")) == 0)
    check_str("crash-ok", "crash", "frames", "0")
    for name, want in (("crash-null", 3), ("crash-deep", 3), ("crash-ok", 0)):
        emit(name, "case.rc", case_rc(name), f"== {want}", case_rc(name) == want)

    # SYMS 里同时存了 sym 行与 abs 反例行，取函数名时必须排除 abs，否则 "??" 会混进调用链
    def funcs_of(case, kind="sym"):
        return [field(ln, "func") for ln in SYMS.get(case, [])
                if ln.startswith(f"S06|crash|{kind}|")]

    sym_funcs = funcs_of("crash-null")
    for want in ("on_sig", "f3", "f2", "f1", "main"):
        emit("crash-null", f"sym.has_{want}", ",".join(sym_funcs), f"contains {want}",
             want in sym_funcs)
    order = [f for f in sym_funcs if f in ("f3", "f2", "f1", "main")]
    emit("crash-null", "sym.call_order", ">".join(order), "f3>f2>f1>main",
         order == ["f3", "f2", "f1", "main"])

    deep_funcs = funcs_of("crash-deep")
    emit("crash-deep", "sym.deep_frames", deep_funcs.count("deep"), ">= 50",
         deep_funcs.count("deep") >= 50)

    abs_funcs = funcs_of("crash-null", "abs") + funcs_of("crash-deep", "abs")
    emit("crash-null", "sym.absolute_lookup", ",".join(abs_funcs), "all ?? (反例)",
         bool(abs_funcs) and all(f == "??" for f in abs_funcs))

    # ---------- 3. sanitizer ----------
    def sani(name, key):
        vs = values(name, "sani", key)
        return vs[-1] if vs else None

    check_str("asan-ok", "sani", "kind", "none")
    check("asan-ok", "case.rc", case_rc("asan-ok"), "str:0", case_rc("asan-ok") == 0)
    for name, kind in (("asan-heap-overflow", "heap-buffer-overflow"),
                       ("asan-uaf", "heap-use-after-free"),
                       ("asan-stack-overflow", "stack-overflow"),
                       ("asan-leak", "leak")):
        emit(name, "sani.kind", sani(name, "kind"), f"str:{kind}", sani(name, "kind") == kind)
        emit(name, "case.rc", case_rc(name), "!= 0", case_rc(name) not in (0, None))

    check_str("ubsan-ok", "sani", "msg", "none")
    check("ubsan-ok", "case.rc", case_rc("ubsan-ok"), "str:0", case_rc("ubsan-ok") == 0)
    for name, phrase in (("ubsan-shift", "shift exponent"),
                         ("ubsan-ovf", "signed integer overflow"),
                         ("ubsan-div", "division by zero"),
                         ("ubsan-null", "load of null pointer")):
        msg = sani(name, "msg") or ""
        emit(name, "sani.msg", msg, f"contains {phrase!r}", phrase in msg)
        emit(name, "case.rc", case_rc(name), "!= 0", case_rc(name) not in (0, None))

    check_num("tsan-race", "sani", "race", 1)
    emit("tsan-race", "case.rc", case_rc("tsan-race"), "!= 0", case_rc("tsan-race") not in (0, None))
    for name in ("tsan-atomic", "tsan-mutex"):
        check_num(name, "sani", "race", 0)
        emit(name, "case.rc", case_rc(name), "str:0", case_rc(name) == 0)
        # 正确同步下计数不能丢；这条是"同步原语真的起作用"的硬判据
        res = num(name, "tsan", "result")
        exp = num(name, "tsan", "expect")
        emit(name, "tsan.result_equals_expect", f"{res}/{exp}", "equal",
             res is not None and exp is not None and res == exp)
    # 记录项：本例连跑 20 次都没丢更新，"没丢"不能当"没竞争"的证据（README §7.5）
    inform("tsan-race", "counters", f"result={num('tsan-race', 'tsan', 'result')}"
                                   f",expect={num('tsan-race', 'tsan', 'expect')}"
                                   f",lost={num('tsan-race', 'tsan', 'lost')}")

    # rc 有两处记录（run_case 一行、sani 汇总一行）。两处必须一致：
    # "工具报了竞争但退出码是 0"正是最危险的形态，任何一边被改成 0 都要被这条抓住。
    for name in SANI_CASES:
        if name not in SEGS:
            continue
        got = text(name, "sani", "rc")
        want = case_rc(name)
        emit(name, "sani.rc_matches_case", f"sani={got},case={want}", "equal",
             got is not None and want is not None and int(got) == want)

    # ---------- 4. 基准 ----------
    vdso = num("bench-vdso", "bench", "median_ns")
    sysc = num("bench-syscall", "bench", "median_ns")
    check("bench-vdso", "bench.median_ns", vdso, "> 0", vdso is not None and vdso > 0)
    check("bench-syscall", "bench.median_ns", sysc, "> 0", sysc is not None and sysc > 0)
    emit("bench-syscall", "bench.ratio_over_vdso",
         None if not vdso or sysc is None else round(sysc / vdso, 2), ">= 1.5",
         bool(vdso) and sysc is not None and sysc / vdso >= 1.5)
    check_num("bench-syscall", "bench", "median_ns", 500, "<=")

    first = [float(v) for v in values("bench-pagefault", "bench", "first_minflt")]
    second = [float(v) for v in values("bench-pagefault", "bench", "second_minflt")]
    emit("bench-pagefault", "bench.first_minflt_min", min(first) if first else None, ">= 15000",
         bool(first) and min(first) >= 15000)
    emit("bench-pagefault", "bench.second_minflt_max", max(second) if second else None, "<= 100",
         bool(second) and max(second) <= 100)
    emit("bench-pagefault", "bench.second_vs_first",
         None if not first or not second else round(second[0] / first[0], 4), "< 0.01",
         bool(first) and bool(second) and first[0] > 0 and second[0] / first[0] < 0.01)

    check_num("bench-ctxswitch", "bench", "ok", 1)
    rt = num("bench-ctxswitch", "bench", "ns_per_rt")
    emit("bench-ctxswitch", "bench.rt_over_syscall",
         None if rt is None or not sysc else round(rt / sysc, 2), ">= 10",
         rt is not None and bool(sysc) and rt / sysc >= 10)
    check_num("bench-ctxswitch", "bench", "ns_per_rt", 1000000, "<=")
    vol = num("bench-ctxswitch", "bench", "voluntary")
    emit("bench-ctxswitch", "bench.voluntary_ge_half",
         vol, ">= 10000", vol is not None and vol >= 10000)

    # ---------- 5. perf 权限边界 ----------
    self_fd = num("perf", "perf", "self_fd")
    emit("perf", "perf.self_fd", self_fd, ">= 0", self_fd is not None and self_fd >= 0)
    check_num("perf", "perf", "sys_errno", 13)
    check_num("perf", "perf", "other_errno", 13)
    paranoid = num("perf", "perf", "paranoid")
    # 条件断言：paranoid>=1 时才要求 systemwide 被拒；否则弃权（不记 PASS 也不记 FAIL）
    if paranoid is not None and paranoid >= 1:
        syserr = num("perf", "perf", "sys_errno")
        emit("perf", "perf.paranoid_implies_eacces",
             f"paranoid={int(paranoid)},errno={syserr}", "errno==13",
             syserr == 13)
    else:
        inform("perf", "perf.paranoid_implies_eacces", "弃权：paranoid<1")
    check_num("perf", "perf", "cycles", 1, ">=")
    ipc = num("perf", "perf", "ipc")
    emit("perf", "perf.ipc", ipc, "0.2 <= ipc <= 4.0",
         ipc is not None and 0.2 <= ipc <= 4.0)

    total = len(results)
    passed = sum(1 for r in results if r)
    print(f"S06|judge|summary|checks={total}|pass={passed}|fail={total - passed}")
    return 0 if passed == total else 1


if __name__ == "__main__":
    try:
        sys.exit(main())
    except FileNotFoundError:
        print(f"S06|judge|error|log_missing={LOG}")
        sys.exit(2)
