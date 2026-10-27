#!/usr/bin/env python3
"""SYS_02 判定器：只读日志，不重跑程序。

判据一律写成"读数之间的关系"：
  - 不写死线程号、pid、耗时
  - 概率性现象（例如"自然发生的虚假唤醒"）不断言
  - 每个"应当失败的模式"（`if` 检查、无锁写、立即关闭）都要真的失败，
    这是本组判据与阶段 1 一致的地方：对照组的读数必须是**坏**的，否则说明注入没生效

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
    for ln in lines:
        if all(n in ln for n in needles):
            return ln
    return None


def num(s):
    try:
        return int(s)
    except (TypeError, ValueError):
        return None


def cmp_field(lines, needles, key, op, value, name, label=None):
    """取一条日志里的字段做比较；缺失即失败。"""
    ln = find(lines, *needles)
    got = field(ln, key) if ln else None
    n = num(got)
    ok = n is not None and op(n, value)
    detail = f"{label or key}={got}"
    if not ln:
        detail = f"缺少读数（{' '.join(needles)}）"
    return (PASS if ok else FAIL, name, detail)


def check_ipc(lines):
    out = []

    # 三种字节流通道：读一次就拿到全部 3000 字节 = 边界被合并
    for tag in ("pipe", "fifo", "uds-stream"):
        ln = find(lines, f"SYS02|ipc|{tag}|", "boundary=")
        if not ln:
            out.append((FAIL, f"{tag}-merged", "缺少读数"))
            continue
        sent, first, total, bnd = (field(ln, "sent"), field(ln, "first_read"),
                                   field(ln, "total"), field(ln, "boundary"))
        ok = bnd == "merged" and num(first) == num(sent) and num(total) == num(sent)
        out.append((PASS if ok else FAIL, f"{tag}-merged",
                    f"first_read={first} total={total} sent={sent} boundary={bnd}"))

    # FIFO 是命名对象，必须真的在文件系统里建出来
    out.append(cmp_field(lines, ["SYS02|ipc|fifo|created="], "created", lambda a, b: a == b, 1,
                         "fifo-created"))

    # SOCK_DGRAM：每条消息单独成包，first_len 必须小于 sent 才说明边界没被合并
    ln = find(lines, "SYS02|ipc|uds-dgram|", "boundary=")
    if not ln:
        out.append((FAIL, "uds-dgram-preserved", "缺少读数"))
    else:
        msgs, first_len, sent = num(field(ln, "msgs")), num(field(ln, "first_len")), num(field(ln, "sent"))
        ok = (field(ln, "boundary") == "preserved" and msgs == 3 and first_len is not None
              and sent is not None and first_len < sent
              and field(ln, "eof_via") == "sentinel")
        out.append((PASS if ok else FAIL, "uds-dgram-preserved",
                    f"msgs={msgs} first_len={first_len} sent={sent} "
                    f"eof_via={field(ln, 'eof_via')}"))

    # 共享内存：两端看同一份字节；unlink 之后按名字打不开
    out.append(cmp_field(lines, ["SYS02|ipc|shm|child_value="], "same",
                         lambda a, b: a == b, 1, "shm-same"))
    ln = find(lines, "SYS02|ipc|shm|after_unlink=")
    ok = ln is not None and field(ln, "after_unlink") == "gone"
    out.append((PASS if ok else FAIL, "shm-unlinked",
                f"after_unlink={field(ln, 'after_unlink') if ln else None}"))
    return out


def check_cond(lines):
    out = []

    # `if` 版本在虚假唤醒之后会把条件当成已满足：flag_at_exit 必须为 0、safe 必须为 0
    out.append(cmp_field(lines, ["SYS02|cond|spurious|check=if|"], "flag_at_exit",
                         lambda a, b: a == b, 0, "spurious-if-broken"))
    out.append(cmp_field(lines, ["SYS02|cond|spurious|check=if|"], "safe",
                         lambda a, b: a == b, 0, "spurious-if-unsafe"))

    # `while` 版本必须重新检查并继续等待，最终由真实条件唤醒
    out.append(cmp_field(lines, ["SYS02|cond|spurious|check=while|"], "flag_at_exit",
                         lambda a, b: a == b, 1, "spurious-while-safe"))
    out.append(cmp_field(lines, ["SYS02|cond|spurious|check=while|"], "hits",
                         lambda a, b: a >= b, 1, "spurious-while-hit"))

    ln = find(lines, "SYS02|cond|queue|")
    if not ln:
        out.append((FAIL, "queue-invariants", "缺少读数"))
    else:
        produced, consumed = num(field(ln, "produced")), num(field(ln, "consumed"))
        depth, cap = num(field(ln, "max_depth")), num(field(ln, "cap"))
        ok = (field(ln, "ok") == "1" and produced == consumed and field(ln, "missing") == "0"
              and field(ln, "dup") == "0" and depth is not None and cap is not None
              and depth <= cap)
        out.append((PASS if ok else FAIL, "queue-invariants",
                    f"produced={produced} consumed={consumed} max_depth={depth}/{cap} "
                    f"missing={field(ln, 'missing')} dup={field(ln, 'dup')}"))
    return out


def check_race(lines):
    out = []

    # 无锁版本必须丢更新；这是注入生效的证据
    out.append(cmp_field(lines, ["SYS02|race|mode=plain|"], "lost", lambda a, b: a > b, 0,
                         "plain-loses-updates"))
    out.append(cmp_field(lines, ["SYS02|race|mode=mutex|"], "lost", lambda a, b: a == b, 0,
                         "mutex-correct"))
    out.append(cmp_field(lines, ["SYS02|race|mode=atomic|"], "lost", lambda a, b: a == b, 0,
                         "atomic-correct"))

    out.append(cmp_field(lines, ["SYS02|rwlock|mode=rwlock|"], "torn", lambda a, b: a == b, 0,
                         "rwlock-consistent"))
    out.append(cmp_field(lines, ["SYS02|rwlock|mode=rwlock-broken|"], "torn",
                         lambda a, b: a > b, 0, "rwlock-broken-tears"))
    return out


def check_deadlock(lines):
    out = []

    ln = find(lines, "SYS02|deadlock|mode=deadlock|")
    if not ln:
        out.append((FAIL, "deadlock-detected", "缺少读数"))
    else:
        ok = (num(field(ln, "t1_rc")) == 110 and num(field(ln, "t2_rc")) == 110
              and field(ln, "detected") == "1")
        out.append((PASS if ok else FAIL, "deadlock-detected",
                    f"t1_rc={field(ln, 't1_rc')} t2_rc={field(ln, 't2_rc')} detected={field(ln, 'detected')}"))

    ln = find(lines, "SYS02|deadlock|mode=ordered|")
    if not ln:
        out.append((FAIL, "ordered-safe", "缺少读数"))
    else:
        ok = (num(field(ln, "t1_rc")) == 0 and num(field(ln, "t2_rc")) == 0
              and field(ln, "safe") == "1" and field(ln, "detected") == "0")
        out.append((PASS if ok else FAIL, "ordered-safe",
                    f"t1_rc={field(ln, 't1_rc')} t2_rc={field(ln, 't2_rc')} safe={field(ln, 'safe')}"))
    return out


def check_pool(lines):
    out = []

    ln = find(lines, "SYS02|pool|mode=graceful|")
    if not ln:
        out.append((FAIL, "graceful-drains", "缺少读数"))
    else:
        ok = (field(ln, "ok") == "1" and num(field(ln, "executed")) == num(field(ln, "submitted"))
              and field(ln, "missing") == "0" and field(ln, "dup") == "0")
        out.append((PASS if ok else FAIL, "graceful-drains",
                    f"executed={field(ln, 'executed')}/{field(ln, 'submitted')} "
                    f"missing={field(ln, 'missing')} dup={field(ln, 'dup')}"))

    ln = find(lines, "SYS02|pool|mode=immediate|")
    if not ln:
        out.append((FAIL, "immediate-drops", "缺少读数"))
    else:
        executed, submitted = num(field(ln, "executed")), num(field(ln, "submitted"))
        ok = (field(ln, "ok") == "1" and executed is not None and submitted is not None
              and executed < submitted and field(ln, "dup") == "0")
        out.append((PASS if ok else FAIL, "immediate-drops",
                    f"executed={executed}/{submitted} dup={field(ln, 'dup')}"))

    # 两种协议都必须把线程和 fd 收干净
    for mode in ("graceful", "immediate"):
        ln = find(lines, f"SYS02|pool|mode={mode}|")
        if not ln:
            out.append((FAIL, f"{mode}-no-leak", "缺少读数"))
            continue
        ok = (field(ln, "fd_leak") == "0" and num(field(ln, "joined")) == num(field(ln, "workers")))
        out.append((PASS if ok else FAIL, f"{mode}-no-leak",
                    f"joined={field(ln, 'joined')}/{field(ln, 'workers')} "
                    f"fd_leak={field(ln, 'fd_leak')}"))
    return out


def check_tsan(lines):
    out = []

    expected = {"plain": "gt", "mutex": "eq0", "atomic": "eq0",
                "rwlock": "eq0", "rwlock-broken": "gt"}
    for mode, rule in expected.items():
        ln = find(lines, f"TSAN|mode={mode}|")
        reports = num(field(ln, "reports")) if ln else None
        if reports is None:
            out.append((FAIL, f"tsan-{mode}", "缺少读数"))
            continue
        ok = reports > 0 if rule == "gt" else reports == 0
        out.append((PASS if ok else FAIL, f"tsan-{mode}",
                    f"reports={reports}（期望 {'>0' if rule == 'gt' else '0'}）"))
    return out


def main():
    if len(sys.argv) < 3:
        print("用法: judge.py <ipc|cond|race|deadlock|pool|tsan> <日志路径>")
        return 2
    case, path = sys.argv[1], sys.argv[2]
    lines = read_lines(path)

    table = {
        "ipc": check_ipc,
        "cond": check_cond,
        "race": check_race,
        "deadlock": check_deadlock,
        "pool": check_pool,
        "tsan": check_tsan,
    }
    if case not in table:
        print(f"未知用例：{case}")
        return 2

    results = table[case](lines)
    bad = 0
    for status, name, detail in results:
        print(f"[{status}] {case}/{name}  {detail}")
        if status == FAIL:
            bad += 1
    print(f"-- {case}: {len(results) - bad}/{len(results)} 通过")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
