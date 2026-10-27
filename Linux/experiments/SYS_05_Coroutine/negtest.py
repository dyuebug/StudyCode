#!/usr/bin/env python3
"""SYS_05 判定器负向验证：证明 judge.py 的每条断言真的在看那一行。

做法：拿一份已有日志，按 (用例, 行前缀, 原文, 替换) 做**用例内**变异，
         每条变异都必须让 judge.py 失败；再验证原日志通过、缺日志返回 2。

两条纪律：
  1. 变异限定在用例块内。`frames=160` 在日志里出现多次，全局替换等于同时
     改坏多个用例，验不出"某条断言真的在看这一行"。
  2. 依赖时序的读数（max_run / resets / cancelled / vm_kb ...）从日志里
     现取原值再替换，不写死——写死会在跨运行漂移时变成 mutate-not-found。

用法: negtest.py [log]        默认 logs/coro.log
输出: logs/negtest.log，末行 pass/fail/total；任一不符合预期即非零退出。
"""
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
JUDGE = os.path.join(HERE, "judge.py")
OUT = os.path.join(HERE, "logs", "negtest.log")


def fields_from(lines):
    """→ {(用例, 字段名): 值}。只收 basics / srv|done 两类读数行。"""
    out, cur = {}, None

    for ln in lines:
        s = ln.strip()
        if s.startswith("S05|case|name="):
            cur = s.split("name=", 1)[1].split("|", 1)[0]
            continue
        if cur is None:
            continue
        parts = s.split("|")
        if len(parts) < 2:
            continue
        if parts[1] == "basics":
            kv = parts[2:]
        elif parts[1] == "srv" and len(parts) > 2 and parts[2] == "done":
            kv = parts[3:]
        else:
            continue
        for p in kv:
            if "=" in p:
                k, v = p.split("=", 1)
                out[(cur, k)] = v
    return out


def build_mutations(f):
    """f[(用例,字段)] → [(说明, 用例, 行前缀, 原文, 替换)]"""

    def g(case, key):
        return f[(case, key)]

    return [
        # ----- 基础用例：读数完全确定，直接写死 -----
        ("gen 少一个值", "gen", "S05|basics|", "|cnt=5|", "|cnt=4|"),
        ("gen 遍历后不持有帧", "gen", "S05|basics|", "|alive_after_iter=1|", "|alive_after_iter=0|"),
        ("task 多次 resume", "task", "S05|basics|", "|resumes=1|", "|resumes=2|"),
        # 行尾字段没有右竖线：alive_after 是 task 行的最后一个 k=v
        ("task 帧未释放", "task", "S05|basics|", "alive_after=0", "alive_after=1"),
        ("except 异常没传到外层", "except", "S05|basics|", "caught=boom", "caught=nope"),
        ("life 跑完不占帧", "life", "S05|basics|", "|alive_after_run=1|", "|alive_after_run=0|"),
        ("resume-twice 没拦住", "resume-twice", "S05|basics|", "|blocked=1|", "|blocked=0|"),
        ("leak-broken 泄漏看不见", "leak-broken", "S05|basics|", "|alive_after=100|", "|alive_after=0|"),
        ("sched-order 高优先级没先跑", "sched-order", "S05|basics|", "|hi_first=1|", "|hi_first=0|"),

        # ----- 服务端用例：原值从日志取 -----
        ("greedy 退化成公平", "coro-greedy", "S05|srv|done|",
         "|max_run=%s|" % g("coro-greedy", "max_run"), "|max_run=1|"),
        ("greedy 少收 20 帧", "coro-greedy", "S05|srv|done|",
         "|frames=%s|" % g("coro-greedy", "frames"),
         "|frames=%d|" % (int(g("coro-greedy", "frames")) - 20)),
        ("fair 没有轮转", "coro-fair", "S05|srv|done|",
         "|max_run=%s|" % g("coro-fair", "max_run"), "|max_run=2|"),
        ("cancel 没超时", "coro-cancel", "S05|srv|done|",
         "|cancelled=%s|" % g("coro-cancel", "cancelled"), "|cancelled=0|"),
        ("rst 没记到复位", "coro-rst", "S05|srv|done|",
         "|resets=%s|" % g("coro-rst", "resets"), "|resets=0|"),
        ("halfclose 没读到 EOF", "coro-halfclose", "S05|srv|done|",
         "|eof=%s|" % g("coro-halfclose", "eof"), "|eof=0|"),
        ("thread 没起线程", "thread", "S05|srv|done|",
         "|threads=%s|" % g("thread", "threads"), "|threads=0|"),
        ("线程版 VmSize 没差别", "thread", "S05|srv|done|",
         "|vm_kb=%s|" % g("thread", "vm_kb"),
         "|vm_kb=%s|" % g("coro-greedy", "vm_kb")),
        ("coro-idle 丢了一帧", "coro-idle", "S05|srv|done|",
         "|frames=%s|" % g("coro-idle", "frames"), "|frames=1|"),
        ("eagain-as-eof 没挂断", "eagain-as-eof", "S05|srv|done|",
         "|frames=%s|" % g("eagain-as-eof", "frames"), "|frames=2|"),
        ("cancel-free 不记 use-after-destroy", "cancel-free", "S05|srv|done|",
         "use_after_destroy=%s" % g("cancel-free", "use_after_destroy"),
         "use_after_destroy=0"),
    ]


def mutate(lines, case, prefix, old, new):
    """在 case 块内、prefix 行上把第一个 old 换成 new；返回 (新行列表, 是否改到)"""
    cur, out, done = None, [], False

    for ln in lines:
        if ln.startswith("S05|case|name="):
            # 必须 strip：行尾带 "\n"，不 strip 会把 "gen\n" 当成用例名，
            # 于是所有 basics 变异都匹配不到（第一版就这么翻车的）。
            cur = ln.strip().split("name=", 1)[1].split("|", 1)[0]
        if not done and cur == case and ln.startswith(prefix) and old in ln:
            ln = ln.replace(old, new, 1)
            done = True
        out.append(ln)
    return out, done


def run_judge(path):
    p = subprocess.run([sys.executable, "-I", JUDGE, path],
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    return p.returncode, p.stdout.strip().splitlines()


def main():
    log = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, "logs", "coro.log")
    if not os.path.exists(log):
        print("S05|negtest|error=log_missing|path=%s" % log)
        return 2
    with open(log, "r", encoding="utf-8", errors="replace") as f:
        lines = f.read().splitlines(True)

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    rep = []
    ok = bad = 0

    # 1) 原日志必须通过
    rc, out = run_judge(log)
    if rc == 0:
        ok += 1
        rep.append("PASS baseline rc=0")
    else:
        bad += 1
        rep.append("FAIL baseline rc=%d %s" % (rc, out[-1] if out else ""))

    # 2) 每条变异都必须让判定器失败
    try:
        mutations = build_mutations(fields_from(lines))
    except KeyError as e:
        print("S05|negtest|error=field_missing|field=%s" % (e,))
        return 2

    for desc, case, prefix, old, new in mutations:
        mutated, hit = mutate(lines, case, prefix, old, new)
        if not hit:
            bad += 1
            rep.append("FAIL mutate-not-found case=%s old=%s" % (case, old))
            continue
        fd, tmp = tempfile.mkstemp(suffix=".log")
        try:
            with os.fdopen(fd, "w", encoding="utf-8") as f:
                f.writelines(mutated)
            rc, out = run_judge(tmp)
            if rc != 0:
                ok += 1
                rep.append("PASS caught: %s → %s" % (desc, out[0] if out else ""))
            else:
                bad += 1
                rep.append("FAIL missed: %s (rc=0)" % desc)
        finally:
            os.unlink(tmp)

    # 3) 缺日志必须是 2（不是 1，也不是 0）
    rc, _ = run_judge(os.path.join(HERE, "logs", "__no_such_log__"))
    if rc == 2:
        ok += 1
        rep.append("PASS missing-log rc=2")
    else:
        bad += 1
        rep.append("FAIL missing-log rc=%d (expect 2)" % rc)

    with open(OUT, "w", encoding="utf-8") as f:
        f.write("\n".join(rep) + "\n")
        f.write("S05|negtest|summary|pass=%d|fail=%d|total=%d\n" % (ok, bad, ok + bad))
    print("\n".join(rep))
    print("S05|negtest|summary|pass=%d|fail=%d|total=%d" % (ok, bad, ok + bad))
    return 0 if bad == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
