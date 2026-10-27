#!/usr/bin/env python3
"""SYS_01 判定器：只读日志，不重跑程序。

每组的判据都写成"读数之间的关系"，不写死会变的绝对值
（进程号、字节数、损坏行数每次运行都不同）。

退出码：0 = 全部通过，1 = 有失败，2 = 日志不存在。
"""
import os
import re
import sys

PASS, FAIL = "PASS", "FAIL"


def read_lines(path):
    if not os.path.exists(path):
        print(f"日志不存在：{path}")
        sys.exit(2)
    with open(path, encoding="utf-8", errors="replace") as f:
        return [ln.rstrip("\n") for ln in f]


def field(line, key):
    m = re.search(r"\|" + re.escape(key) + r"=([^|]+)", line)
    return m.group(1) if m else None


def find(lines, *needles):
    """返回第一条同时包含全部 needle 的行。"""
    for ln in lines:
        if all(n in ln for n in needles):
            return ln
    return None


def check_fileio(lines):
    out = []

    # A1：O_APPEND 的原子追加是必然成立的，所以可以断言"一行不少"。
    ln = find(lines, "SYS01|append|", "mode=O_APPEND+yield")
    if not ln:
        out.append((FAIL, "append-atomic", "缺少 O_APPEND+yield 读数"))
    else:
        good, expected = field(ln, "good"), field(ln, "expected")
        ok = good is not None and expected is not None and good == expected and field(ln, "bad") == "0"
        out.append((PASS if ok else FAIL, "append-atomic",
                    f"good={good}/{expected} bad={field(ln, 'bad')}"))

    # A2：lseek 后再写，在注入 yield 之后必然丢行。丢的是整行（被覆盖），
    # 所以 bad 仍是 0——判据只能看 good 是否少于 expected，不能看 bad。
    ln = find(lines, "SYS01|append|", "mode=lseek+yield")
    if not ln:
        out.append((FAIL, "lseek-race-injected", "缺少 lseek+yield 读数"))
    else:
        good, expected = field(ln, "good"), field(ln, "expected")
        try:
            ok = int(good) < int(expected)
        except (TypeError, ValueError):
            ok = False
        out.append((PASS if ok else FAIL, "lseek-race-injected", f"good={good}/{expected}"))

    # A3：自然模式只记录不断言。它是概率性的——实测 3 次里只有 1 次丢行，
    # 把这个数字当判据会造出随机失败的测试。
    ln = find(lines, "SYS01|append|", "mode=lseek+natural")
    out.append((PASS, "lseek-natural(observed)",
                "未断言（概率性）: good=" + str(field(ln, "good")) if ln else "缺少读数"))

    # C：O_CLOEXEC 的 fd 在 exec 后必须消失，另一个必须还在。
    ln = find(lines, "SYS01|cloexec|child|")
    if not ln:
        out.append((FAIL, "cloexec", "缺少子进程读数"))
    else:
        ok = field(ln, "keep_alive") == "1" and field(ln, "cloexec_alive") == "0"
        out.append((PASS if ok else FAIL, "cloexec",
                    f"keep_alive={field(ln, 'keep_alive')} cloexec_alive={field(ln, 'cloexec_alive')}"))
    return out


def check_proc(lines):
    out = []

    ln = find(lines, "SYS01|zombie|before_wait|")
    ok = ln is not None and field(ln, "state") == "Z"
    out.append((PASS if ok else FAIL, "zombie-detect", f"state={field(ln, 'state') if ln else None}"))

    # 回收之后 /proc/<pid>/stat 必须消失。只判"gone"，
    # 不判"状态变成 X"——那说明目录项还在，性质完全不同。
    ok = find(lines, "SYS01|zombie|after_wait|gone") is not None
    out.append((PASS if ok else FAIL, "zombie-reaped", "after_wait"))

    # B1：不设 subreaper 时，孤儿的父进程必须**不是**本进程。
    # 不假设它一定变成 1：WSL / 容器里可能是别的 subreaper（实测就是 497）。
    ln = find(lines, "SYS01|orphan|grand|", "subreaper=0")
    if not ln:
        out.append((FAIL, "orphan-reparented", "缺少 subreaper=0 读数"))
    else:
        ppid, gp = field(ln, "ppid"), field(ln, "grandparent_pid")
        ok = ppid is not None and gp is not None and ppid != gp
        out.append((PASS if ok else FAIL, "orphan-reparented", f"ppid={ppid} grandparent={gp}"))

    # B2：设了 subreaper 之后，同一个孤儿必须落到本进程名下。
    ln = find(lines, "SYS01|orphan|grand|", "subreaper=1")
    if not ln:
        out.append((FAIL, "subreaper-adopts", "缺少 subreaper=1 读数"))
    else:
        ppid, gp = field(ln, "ppid"), field(ln, "grandparent_pid")
        ok = ppid is not None and ppid == gp
        out.append((PASS if ok else FAIL, "subreaper-adopts", f"ppid={ppid} grandparent={gp}"))

    ln = find(lines, "SYS01|wait|normal_exit|")
    ok = ln is not None and field(ln, "exited") == "1" and field(ln, "code") == "42"
    out.append((PASS if ok else FAIL, "wait-normal", f"code={field(ln, 'code') if ln else None}"))

    # 信号终止必须与"退出码 143"区分开：WIFSIGNALED 为真、WTERMSIG 为 15。
    ln = find(lines, "SYS01|wait|signal_exit|")
    ok = ln is not None and field(ln, "signaled") == "1" and field(ln, "sig") == "15"
    out.append((PASS if ok else FAIL, "wait-signal",
                f"signaled={field(ln, 'signaled') if ln else None} sig={field(ln, 'sig') if ln else None}"))
    return out


def check_pipe(lines):
    out = []

    ln = find(lines, "SYS01|pipe|short|")
    if not ln:
        out.append((FAIL, "short-read", "缺少读数"))
    else:
        total, reads, last = field(ln, "total"), field(ln, "reads"), field(ln, "last_chunk")
        # 1000 字节按 64 字节缓冲：15 次满额 + 1 次 40 字节，共 16 次。
        ok = total == "1000" and reads == "16" and last == "40"
        out.append((PASS if ok else FAIL, "short-read", f"total={total} reads={reads} last={last}"))

    ln = find(lines, "SYS01|pipe|eintr|first_read|")
    ok = ln is not None and field(ln, "ret") == "-1" and field(ln, "errno") == "4"
    out.append((PASS if ok else FAIL, "eintr",
                f"ret={field(ln, 'ret') if ln else None} errno={field(ln, 'errno') if ln else None}"))

    # 重试必须拿到数据：ret=2 且 errno 已被显式清零（不能是上一次的 4）。
    ln = find(lines, "SYS01|pipe|eintr|retry_read|")
    ok = ln is not None and field(ln, "ret") == "2" and field(ln, "errno") == "0"
    out.append((PASS if ok else FAIL, "eintr-retry",
                f"ret={field(ln, 'ret') if ln else None} errno={field(ln, 'errno') if ln else None}"))
    return out


def check_shell(ev, stdout_path):
    out = []

    ln = find(ev, "SYS01|sh|pipe|left=echo|right=wc|")
    ok = ln is not None and field(ln, "st_left") == "0" and field(ln, "st_right") == "0"
    out.append((PASS if ok else FAIL, "pipe-builtin", f"st_right={field(ln, 'st_right') if ln else None}"))

    # cd 必须在父进程生效：两条 pwd 的 path 必须不同，后一条必须是 "/"。
    pwds = [ln for ln in ev if "SYS01|sh|builtin|pwd|path=" in ln]
    ok = len(pwds) >= 2 and field(pwds[0], "path") != field(pwds[-1], "path") \
        and field(pwds[-1], "path") == "/"
    out.append((PASS if ok else FAIL, "builtin-cd-persists",
                f"n={len(pwds)} last={field(pwds[-1], 'path') if pwds else None}"))

    # 不存在的命令：exec 失败要写 errno=2，且 shell 用 127 报告，不应崩溃。
    ln = find(ev, "SYS01|sh|exec_fail|cmd=nonexistent-xyz|")
    ok = ln is not None and field(ln, "errno") == "2"
    out.append((PASS if ok else FAIL, "exec-fail-reported", f"errno={field(ln, 'errno') if ln else None}"))

    ln = find(ev, "SYS01|sh|exec|cmd=nonexistent-xyz|")
    ok = ln is not None and field(ln, "status") == "127"
    out.append((PASS if ok else FAIL, "exec-fail-127", f"status={field(ln, 'status') if ln else None}"))

    # 命令输出必须落在 stdout，不能混进取证流。
    if os.path.exists(stdout_path):
        with open(stdout_path, encoding="utf-8", errors="replace") as f:
            so = f.read()
        ok = "hello-minish" in so and "\n4\n" in "\n" + so + "\n" and "one\ntwo\n" in so
        out.append((PASS if ok else FAIL, "stdout-content",
                    f"hello={'hello-minish' in so} wc={'4' in so.split()} redirect={'one' in so and 'two' in so}"))
    else:
        out.append((FAIL, "stdout-content", "缺少 stdout 文件"))

    ln = find(ev, "SYS01|sh|exit|")
    ok = ln is not None and field(ln, "code") == "0"
    out.append((PASS if ok else FAIL, "shell-exit", f"code={field(ln, 'code') if ln else None}"))
    return out


def main():
    if len(sys.argv) < 3:
        print("用法: judge.py <fileio|proc|pipe|shell> <日志路径> [stdout 路径]")
        return 2
    case, path = sys.argv[1], sys.argv[2]
    lines = read_lines(path)

    if case == "fileio":
        results = check_fileio(lines)
    elif case == "proc":
        results = check_proc(lines)
    elif case == "pipe":
        results = check_pipe(lines)
    elif case == "shell":
        results = check_shell(lines, sys.argv[3] if len(sys.argv) > 3 else
                              os.path.join(os.path.dirname(path), "minish.stdout"))
    else:
        print(f"未知用例：{case}")
        return 2

    bad = 0
    for status, name, detail in results:
        print(f"[{status}] {case}/{name}  {detail}")
        if status == FAIL:
            bad += 1
    print(f"-- {case}: {len(results) - bad}/{len(results)} 通过")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
