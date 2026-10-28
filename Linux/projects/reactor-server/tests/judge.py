#!/usr/bin/env python3
"""judge.py —— reactor-server 用例日志的机器判据。

用法：python3 tests/judge.py tests/logs

判据分两层：
  公共层（C1–C9）：每个用例都必须成立——服务起得来、跑完、计数自洽、干净退出。
  用例层（C10+）：按用例语义追加，例如空闲超时必须真的由服务端断开。

退出码：0 = 全部通过；1 = 有判据失败；2 = 日志缺失或结构不可解析。
判的是不变量，不是具体数值：连接数、线程数、机型都可以变，计数关系不能变。
"""

import os
import re
import sys

CASE_FRAMES = {
    "echo": 20,
    "pipeline": 50,
    "half_close": 10,
    "idle_timeout": 1,
    "backpressure": 1000,
    "dropped": 1,
    "gen_reuse": 1,
    "graceful": 10,
}

KV = re.compile(r"([A-Za-z_][A-Za-z0-9_]*)=([^\s|]+)")


def load(path):
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as f:
            return [ln.rstrip("\n") for ln in f]
    except OSError:
        return None


def kv(line):
    return {m.group(1): m.group(2) for m in KV.finditer(line)}


def pick(lines, prefix):
    """返回第一条匹配 prefix 的行解析结果；没有则 None。"""
    for ln in lines:
        if ln.startswith(prefix):
            d = kv(ln)
            d["_line"] = ln
            return d
    return None


def pick_all(lines, prefix):
    return [d for d in (kv(ln) for ln in lines if ln.startswith(prefix)) if d]


def num(d, key, default=None):
    try:
        return int(d[key])
    except (KeyError, TypeError, ValueError):
        return default


class Report:
    def __init__(self):
        self.rows = []

    def chk(self, case, cid, ok, detail):
        self.rows.append((case, cid, bool(ok), detail))

    def case_failed(self, case):
        return any(c == case and not ok for c, _, ok, _ in self.rows)


def judge_case(rep, name, logdir):
    srv = load(os.path.join(logdir, name + ".srv.log"))
    cli = load(os.path.join(logdir, name + ".cli.log"))
    meta_lines = load(os.path.join(logdir, name + ".meta")) or []
    if srv is None or cli is None:
        rep.chk(name, "C0", False, "日志缺失（srv 或 cli 不存在）")
        return
    meta = kv("|".join(meta_lines))

    listen = pick(srv, "SRV|listen|")
    ready = pick(srv, "SRV|ready|")
    summ = pick(srv, "SRV|summary|")
    exit_ = pick(srv, "SRV|exit|")
    done = pick(cli, "CLI|done|")
    closes = pick_all(srv, "SRV|close|")
    reasons = {}
    for c in closes:
        r = c.get("reason", "?")
        reasons[r] = reasons.get(r, 0) + 1

    rep.chk(name, "C1", listen is not None, "SRV|listen 存在（拿到端口）")
    rep.chk(name, "C2", ready is not None, "SRV|ready 存在（线程池已起）")
    rep.chk(name, "C3", summ is not None, "SRV|summary 存在（走完退出路径）")
    rep.chk(name, "C4", exit_ is not None and num(exit_, "code") == 0,
            "SRV|exit|code=0")
    rep.chk(name, "C5", meta.get("srv_rc") == "0",
            "服务进程退出码 0（实际 %s）" % meta.get("srv_rc", "缺失"))
    rep.chk(name, "C6", done is not None, "CLI|done 存在（客户端跑完）")
    if summ is None:
        return

    acc = num(summ, "accepted")
    clo = num(summ, "closed")
    act = num(summ, "active")
    err = num(summ, "errors")
    rep.chk(name, "C7", act == 0, "summary active=0（实际 %s）" % act)
    rep.chk(name, "C8", err == 0, "summary errors=0（实际 %s）" % err)
    rep.chk(name, "C9", acc is not None and acc == clo,
            "accepted == closed（%s vs %s）" % (acc, clo))

    if name in ("echo", "pipeline", "half_close"):
        frames = CASE_FRAMES[name]
        jobs = num(summ, "jobs")
        rep.chk(name, "C10", done is not None and num(done, "fail") == 0
                and num(done, "mismatch") == 0,
                "客户端 fail=0 且 mismatch=0")
        rep.chk(name, "C11", num(summ, "rx") == num(summ, "tx"),
                "rx == tx（%s vs %s）" % (summ.get("rx"), summ.get("tx")))
        rep.chk(name, "C12", jobs == acc * frames,
                "jobs == accepted×frames（%s vs %s×%d）" % (jobs, acc, frames))
        rep.chk(name, "C13", set(reasons) <= {"peer_eof"},
                "关闭原因只有 peer_eof（实际 %s）" % reasons)

    elif name == "idle_timeout":
        rep.chk(name, "C10", num(summ, "timeouts", 0) >= num(summ, "accepted", 1),
                "timeouts >= accepted（%s vs %s）"
                % (summ.get("timeouts"), summ.get("accepted")))
        rep.chk(name, "C11", set(reasons) <= {"idle_timeout"},
                "关闭原因只有 idle_timeout（实际 %s）" % reasons)
        rep.chk(name, "C12", done is not None
                and num(done, "idle_closed") == num(done, "clients"),
                "客户端每条连接都观察到服务端关闭")

    elif name == "backpressure":
        rep.chk(name, "C10", set(reasons) <= {"peer_eof", "wbuf_full"},
                "关闭原因只有 peer_eof / wbuf_full（实际 %s）" % reasons)
        full = reasons.get("wbuf_full", 0)
        fail = num(done, "fail", 0) if done else 0
        rep.chk(name, "C11", (fail > 0) == (full > 0),
                "有 wbuf_full 时客户端必须可见失败（wbuf_full=%d fail=%d）"
                % (full, fail))
        rep.chk(name, "C12", num(summ, "rx") >= num(summ, "tx"),
                "rx >= tx（被背压丢弃的写不会再计数，%s vs %s）"
                % (summ.get("rx"), summ.get("tx")))

    elif name == "dropped":
        # 迟到的结果必须被记账。少了这几条，dropped=0 与"确实没有迟到结果"
        # 在日志上完全一样——这正是这个用例存在的理由。
        jobs = num(summ, "jobs")
        rep.chk(name, "C10", num(summ, "dropped", 0) >= 1,
                "dropped >= 1（在飞结果到达时连接已关闭，实际 %s）" % summ.get("dropped"))
        rep.chk(name, "C11", jobs is not None and num(summ, "dropped") == jobs,
                "dropped == jobs（每个被丢弃的结果都记了一次，%s vs %s）"
                % (summ.get("dropped"), jobs))
        rep.chk(name, "C12", num(summ, "tx", -1) == 0,
                "tx == 0（没有任何回显写回，实际 %s）" % summ.get("tx"))
        rep.chk(name, "C13", set(reasons) <= {"idle_timeout", "shutdown"},
                "关闭原因只有 idle_timeout / shutdown（实际 %s）" % reasons)
        rep.chk(name, "C14", num(summ, "timeouts", 0) >= num(summ, "accepted", 1),
                "timeouts >= accepted（%s vs %s）"
                % (summ.get("timeouts"), summ.get("accepted")))

    elif name == "gen_reuse":
        # 世代号分支：前提（槽位真的被复用）与结果（旧载荷没投给新连接）都要判。
        # 只判 stale_gen>=1 不够——用例若因竞态没复用槽位，stale_gen 会是 0，
        # 那时必须区分"实现错"与"前提没成立"。
        accs = pick_all(srv, "SRV|accept|")
        ids = []
        for a in accs:
            v = num(a, "id")
            if v is not None:
                ids.append(v)
        same_slot = len(ids) >= 2 and (ids[0] >> 32) == (ids[1] >> 32)
        gen_up = len(ids) >= 2 and (ids[1] & 0xFFFFFFFF) > (ids[0] & 0xFFFFFFFF)
        rep.chk(name, "C10", same_slot,
                "前提：两条连接落在同一槽位（ids=%s）" % ids)
        rep.chk(name, "C11", gen_up,
                "前提：槽位复用时世代号递增（ids=%s）" % ids)
        rep.chk(name, "C12", done is not None and num(done, "eof_a") == 1,
                "前提：A 已被服务端关闭（客户端读到 EOF，实际 %s）"
                % (done.get("eof_a") if done else "无 done 行"))
        rep.chk(name, "C13", num(summ, "stale_gen", 0) >= 1,
                "stale_gen >= 1（迟到结果被按世代丢弃，实际 %s）"
                % summ.get("stale_gen"))
        sg = num(summ, "stale_gen")
        rep.chk(name, "C14", sg is not None and num(summ, "dropped", 0) >= sg,
                "dropped 是 stale_gen 的超集（%s vs %s）"
                % (summ.get("dropped"), summ.get("stale_gen")))
        rep.chk(name, "C15", done is not None and num(done, "stale_received") == 0,
                "新连接没有收到旧连接的载荷（stale_received=%s）"
                % (done.get("stale_received") if done else "无 done 行"))
        rep.chk(name, "C16", done is not None and num(done, "echo_b", 0) >= 1,
                "槽位复用后的连接仍能收到自己的回显（echo_b=%s）"
                % (done.get("echo_b") if done else "无 done 行"))

    elif name == "graceful":
        sig = pick(srv, "SRV|signal|")
        rep.chk(name, "C10", sig is not None, "收到并处理了退出信号")
        rep.chk(name, "C11", num(summ, "leftover_jobs", -1) == 0,
                "退出时无遗留 job（%s）" % summ.get("leftover_jobs"))
        rep.chk(name, "C12", set(reasons) <= {"shutdown", "peer_eof"},
                "关闭原因只有 shutdown / peer_eof（实际 %s）" % reasons)


def main(argv):
    logdir = argv[1] if len(argv) > 1 else "tests/logs"
    if not os.path.isdir(logdir):
        print("日志目录不存在: %s" % logdir, file=sys.stderr)
        return 2

    rep = Report()
    names = sorted(f[:-8] for f in os.listdir(logdir) if f.endswith(".srv.log"))
    if not names:
        print("没有任何 *.srv.log，用例未执行", file=sys.stderr)
        return 2

    for name in names:
        judge_case(rep, name, logdir)

    total = len(rep.rows)
    failed = [r for r in rep.rows if not r[2]]
    for case in names:
        rows = [r for r in rep.rows if r[0] == case]
        print("=== %s ===" % case)
        for _, cid, ok, detail in rows:
            print("  [%s] %-4s %s" % ("PASS" if ok else "FAIL", cid, detail))
    print("---")
    print("judge|total=%d|passed=%d|failed=%d" % (total, total - len(failed), len(failed)))
    for case, cid, _, detail in failed:
        print("  FAIL %s/%s: %s" % (case, cid, detail))
    return 0 if not failed else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
