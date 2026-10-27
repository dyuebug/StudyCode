#!/usr/bin/env python3
"""SYS_03 判定器：只读日志，不重跑程序。

判据一律写成"读数之间的关系"或"内核约定的常量"：
  - `ECONNRESET`=104 / `EPIPE`=32 / `EAGAIN`=11 / `EMSGSIZE`=90 是 Linux 的固定 errno，
    写死是可以的；但 `elapsed_ms` 只判区间，`events` 只判非零
  - 对照组（`epoll-et-broken`、`reset`）必须真的坏，否则说明注入没生效
  - 端口由内核分配，日志里不出现，所以判定不受并发运行影响

退出码：0 = 全部通过，1 = 有失败，2 = 日志不存在或用法错误。
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


def check(lines, needles, key, op, value, name):
    """取一条日志里的字段做比较；行缺失或字段缺失即失败。"""
    ln = find(lines, *needles)
    got = field(ln, key) if ln else None
    n = num(got)
    ok = n is not None and op(n, value)
    detail = f"{key}={got}"
    if not ln:
        detail = f"缺少读数（{' '.join(needles)}）"
    return (PASS if ok else FAIL, name, detail)


def check_str(lines, needles, key, want, name):
    ln = find(lines, *needles)
    got = field(ln, key) if ln else None
    ok = got == want
    return (PASS if ok else FAIL, name,
            f"{key}={got}（期望 {want}）" if ln else f"缺少读数（{' '.join(needles)}）")


def check_tcp(lines):
    out = []

    # echo：三条 1000 字节被合并成一次读，长度前缀恢复出 3 条
    ln = find(lines, "SYS03|tcp|echo|")
    ok = (ln is not None and field(ln, "boundary") == "merged"
          and num(field(ln, "first_read")) == num(field(ln, "sent"))
          and num(field(ln, "framed")) == 3 and num(field(ln, "client_frames")) == 3
          and field(ln, "eof") == "1")
    out.append((PASS if ok else FAIL, "echo-merged-and-framed",
                f"first_read={field(ln, 'first_read') if ln else None} "
                f"framed={field(ln, 'framed') if ln else None} "
                f"client_frames={field(ln, 'client_frames') if ln else None} "
                f"eof={field(ln, 'eof') if ln else None}"))

    for key, name in (("got_ping", "halfclose-ping"),
                      ("eof_seen", "halfclose-eof"),
                      ("sent_after_eof", "halfclose-write-after-eof"),
                      ("read_after_shutdown", "halfclose-read-after-shutdown")):
        out.append(check(lines, ["SYS03|tcp|halfclose|"], key, lambda a, b: a == b, 1, name))

    out.append(check(lines, ["SYS03|tcp|reset|"], "errno", lambda a, b: a == b, 104, "reset-ECONNRESET"))
    out.append(check(lines, ["SYS03|tcp|reset|"], "detected", lambda a, b: a == b, 1, "reset-detected"))

    # SIGPIPE：FIN 之后先成功写一次，第二次才是 EPIPE(32)；处理器必须真的被调用
    out.append(check(lines, ["SYS03|tcp|sigpipe|"], "ignore_errno", lambda a, b: a == b, 32,
                     "sigpipe-EPIPE(ignored)"))
    out.append(check(lines, ["SYS03|tcp|sigpipe|"], "handler_errno", lambda a, b: a == b, 32,
                     "sigpipe-EPIPE(handler)"))
    out.append(check(lines, ["SYS03|tcp|sigpipe|"], "handler_hits", lambda a, b: a >= b, 1,
                     "sigpipe-signal-delivered"))
    out.append(check(lines, ["SYS03|tcp|sigpipe|"], "ignore_writes", lambda a, b: a >= b, 1,
                     "sigpipe-fin-then-epipe"))

    out.append(check(lines, ["SYS03|tcp|timeout|"], "errno", lambda a, b: a == b, 11,
                     "timeout-EAGAIN"))
    out.append(check(lines, ["SYS03|tcp|timeout|"], "timed_out", lambda a, b: a == b, 1,
                     "timeout-detected"))
    # 200 ms 的门限只判"不少于 100 ms、不超过 2 s"：调度慢一点不算错
    out.append(check(lines, ["SYS03|tcp|timeout|"], "elapsed_ms", lambda a, b: a >= b, 100,
                     "timeout-elapsed>=100ms"))
    out.append(check(lines, ["SYS03|tcp|timeout|"], "elapsed_ms", lambda a, b: a <= b, 2000,
                     "timeout-elapsed<=2000ms"))

    ln = find(lines, "SYS03|tcp|shortwrite|")
    ok = (ln is not None and num(field(ln, "sent")) == num(field(ln, "payload"))
          and num(field(ln, "recv")) == num(field(ln, "payload"))
          and num(field(ln, "short_writes")) > 0 and field(ln, "mismatch") == "0"
          and field(ln, "ok") == "1")
    out.append((PASS if ok else FAIL, "shortwrite-loops",
                f"sent={field(ln, 'sent') if ln else None}/{field(ln, 'payload') if ln else None} "
                f"recv={field(ln, 'recv') if ln else None} "
                f"short_writes={field(ln, 'short_writes') if ln else None} "
                f"mismatch={field(ln, 'mismatch') if ln else None}"))
    return out


def check_udp(lines):
    out = []

    ln = find(lines, "SYS03|udp|boundary|")
    ok = (ln is not None and field(ln, "preserved") == "1"
          and num(field(ln, "recv_msgs")) == 3
          and num(field(ln, "sent_msgs")) == 3)
    out.append((PASS if ok else FAIL, "udp-boundary-preserved",
                f"recv_msgs={field(ln, 'recv_msgs') if ln else None} "
                f"recv_lens={field(ln, 'recv_lens') if ln else None}"))

    ln = find(lines, "SYS03|udp|trunc|")
    ok = (ln is not None and field(ln, "truncated") == "1"
          and num(field(ln, "recv_rc")) == num(field(ln, "buf"))
          and num(field(ln, "real_len")) == 2000)
    out.append((PASS if ok else FAIL, "udp-trunc-SMSG_TRUNC",
                f"recv_rc={field(ln, 'recv_rc') if ln else None} "
                f"real_len={field(ln, 'real_len') if ln else None} "
                f"buf={field(ln, 'buf') if ln else None}"))

    out.append(check(lines, ["SYS03|udp|toobig|"], "errno", lambda a, b: a == b, 90,
                     "udp-EMSGSIZE"))
    out.append(check(lines, ["SYS03|udp|toobig|"], "detected", lambda a, b: a == b, 1,
                     "udp-toobig-detected"))
    return out


def check_mux(lines):
    out = []

    # 四种正确后端必须都把 8 条连接服务完
    for mode in ("select", "poll", "epoll", "epoll-et"):
        ln = find(lines, f"SYS03|mux|mode={mode}|")
        if not ln:
            out.append((FAIL, f"mux-{mode}", "缺少读数"))
            continue
        clients = num(field(ln, "clients"))
        ok = (field(ln, "ok") == "1" and field(ln, "stalled") == "0"
              and num(field(ln, "accepted")) == clients
              and num(field(ln, "echoed")) == clients
              and num(field(ln, "done")) == clients
              and field(ln, "mismatch") == "0")
        out.append((PASS if ok else FAIL, f"mux-{mode}",
                    f"done={field(ln, 'done')}/{field(ln, 'clients')} "
                    f"echoed={field(ln, 'echoed')} "
                    f"mismatch={field(ln, 'mismatch')} events={field(ln, 'events')}"))

    # 只读一次的 ET 实现必须停住：它拿不到剩下 84 字节
    ln = find(lines, "SYS03|mux|mode=epoll-et-broken|")
    if not ln:
        out.append((FAIL, "mux-epoll-et-broken", "缺少读数"))
    else:
        done, clients = num(field(ln, "done")), num(field(ln, "clients"))
        ok = (field(ln, "stalled") == "1" and done is not None and clients is not None
              and done < clients)
        out.append((PASS if ok else FAIL, "mux-epoll-et-broken",
                    f"stalled={field(ln, 'stalled')} done={done}/{clients} "
                    f"echoed={field(ln, 'echoed')}"))
    return out


def main():
    if len(sys.argv) < 3:
        print("用法: judge.py <tcp|udp|mux> <日志路径>")
        return 2
    case, path = sys.argv[1], sys.argv[2]
    lines = read_lines(path)

    table = {"tcp": check_tcp, "udp": check_udp, "mux": check_mux}
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
