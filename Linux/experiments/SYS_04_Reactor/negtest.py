#!/usr/bin/env python3
"""SYS_04 判定器的负向验证：不重跑 QEMU/服务端，只变异已有日志。

一个判定器"通过"不代表它能抓错。这里对 logs/reactor.log 的副本做 12 处
单点变异，每一处都**必须**让 judge.py 返回 1；再加上原日志（应返回 0）
与缺失日志（应返回 2），共 14 次调用。

变异只在指定用例的块内生效：日志里 `verified=160` 出现 7 次，
全局替换等于同时改坏多个用例，验不出"某条断言真的在看这一行"。

用法: python3 -I negtest.py [日志路径]
输出: 每行一处结论 + 末行 pass=N fail=M total=N
退出码: 0 全部符合预期，1 有不符合预期
"""
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
JUDGE = os.path.join(HERE, "judge.py")

# (说明, 用例名, 行前缀, 旧串, 新串)
MUTATIONS = [
    ("single 少收一帧", "single", "SYS04|gen|done", "verified=160", "verified=159"),
    ("single 内容错一帧", "single", "SYS04|gen|done", "mismatch=0", "mismatch=1"),
    ("single 出现超时", "single", "SYS04|gen|done", "timeouts=0", "timeouts=1"),
    ("single 服务端帧数不符", "single", "SYS04|srv|done", "frames=160", "frames=159"),
    ("pool4 队列丢任务", "pool4", "SYS04|srv|done", "q_drops=0", "q_drops=3"),
    ("pool4 没有变快", "pool4", "SYS04|gen|done", "total_ms=85", "total_ms=300"),
    ("master2 少一个 sub", "master2", "SYS04|srv|done", "subs=2", "subs=1"),
    ("master2 有连接没转交", "master2", "SYS04|srv|done", "handoff=8", "handoff=7"),
    ("master2 连接全压在 sub0", "master2", "SYS04|srv|done", "sub1_frames=80", "sub1_frames=0"),
    ("single 其实没串行", "single", "SYS04|gen|done", "total_ms=339", "total_ms=100"),
    ("et-broken 居然收齐了", "et-broken", "SYS04|gen|done", "verified=0", "verified=160"),
    ("oneshot-broken 服务端没停", "oneshot-broken", "SYS04|srv|done", "frames=8", "frames=160"),
]


def mutate(lines, case, prefix, old, new):
    """在 case 块内、prefix 行上把第一个 old 换成 new；返回是否改到。"""
    cur, out, done = None, [], False
    for ln in lines:
        if ln.startswith("SYS04|case|name="):
            cur = ln.split("name=", 1)[1].split("|", 1)[0]
        if not done and cur == case and ln.startswith(prefix) and old in ln:
            ln = ln.replace(old, new, 1)
            done = True
        out.append(ln)
    return out, done


def run_judge(path):
    p = subprocess.run([sys.executable, "-I", JUDGE, path],
                       capture_output=True, text=True)
    return p.returncode, p.stdout


def main():
    src = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, "logs/reactor.log")
    if not os.path.exists(src):
        print(f"日志不存在：{src}")
        return 2
    with open(src, encoding="utf-8", errors="replace") as f:
        orig = f.readlines()

    results = []

    def check(label, want, path):
        rc, out = run_judge(path)
        ok = rc == want
        detail = ""
        if ok and want == 1:
            # 顺便看清是哪条断言抓到的，避免"别的断言替它背锅"
            fails = [l for l in out.splitlines() if l.startswith("[FAIL]")]
            detail = f"抓到 {len(fails)} 条：" + (fails[0] if fails else "?")
        elif not ok:
            detail = f"rc={rc}（期望 {want}）"
        results.append((ok, label, detail))

    check("原日志应全通过", 0, src)

    tmp = tempfile.mkdtemp(prefix="sys04_neg_")
    for i, (label, case, prefix, old, new) in enumerate(MUTATIONS, 1):
        lines, hit = mutate(orig, case, prefix, old, new)
        path = os.path.join(tmp, f"mut{i:02d}.log")
        with open(path, "w", encoding="utf-8") as f:
            f.writelines(lines)
        if not hit:
            results.append((False, label, "变异没落到任何行上（用例名或读数已漂移）"))
            continue
        check(label, 1, path)

    check("缺失日志应返回 2", 2, os.path.join(tmp, "no_such.log"))

    bad = 0
    for ok, label, detail in results:
        print(f"[{'PASS' if ok else 'FAIL'}] {label}  {detail}")
        if not ok:
            bad += 1
    print(f"-- pass={len(results) - bad} fail={bad} total={len(results)}")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
