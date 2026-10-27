#!/usr/bin/env python3
"""SYS_04 判定器：只读日志，不重跑程序。

判的是三类东西：
  1. 功能不变量：正确形态必须一帧不差（verified == expect、frames == expect）
  2. 破坏版必须真的坏：et-broken / oneshot-broken 的 verified 必须 < expect
  3. 时间对比：pool 与 master 明显快于 single，且 single 够慢（证明串行是真的）

时间只判倍数与下限，不判绝对值——`total_ms` 每次都会漂。

退出码：0 = 全部通过，1 = 有失败，2 = 日志不存在或用例缺失。
"""
import os
import sys

PASS, FAIL = "PASS", "FAIL"


def parse_kv(line):
    kv = {}
    for part in line.split("|"):
        if "=" in part:
            k, v = part.split("=", 1)
            kv[k] = v
    return kv


def as_int(s):
    try:
        return int(s)
    except (TypeError, ValueError):
        return None


def load(path):
    if not os.path.exists(path):
        print(f"日志不存在：{path}")
        sys.exit(2)
    cases, cur = {}, None
    with open(path, encoding="utf-8", errors="replace") as f:
        for raw in f:
            ln = raw.rstrip("\n")
            if ln.startswith("SYS04|case|"):
                cur = parse_kv(ln).get("name")
                cases.setdefault(cur, {})
                cases[cur]["meta"] = parse_kv(ln)
            elif ln.startswith("SYS04|srv|done") and cur:
                cases[cur]["srv"] = parse_kv(ln)
            elif ln.startswith("SYS04|gen|done") and cur:
                cases[cur]["gen"] = parse_kv(ln)
    return cases


def num(d, k):
    return as_int((d or {}).get(k))


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else "logs/reactor.log"
    cases = load(path)
    out = []

    def need(name):
        c = cases.get(name)
        if not c or "gen" not in c or "srv" not in c:
            out.append((FAIL, f"{name}/present", "缺少该用例的 srv 或 gen 读数"))
            return None
        return c

    def eq(name, section, key, want, tag=None):
        c = cases.get(name) or {}
        got = num(c.get(section), key)
        ok = got == want
        out.append((PASS if ok else FAIL, tag or f"{name}/{section}.{key}",
                    f"{key}={got}（期望 {want}）"))
        return ok

    def lt(name, section, key, other, tag):
        c = cases.get(name) or {}
        got = num(c.get(section), key)
        ok = got is not None and other is not None and got < other
        out.append((PASS if ok else FAIL, tag, f"{key}={got} 对照={other}"))

    single = need("single")
    pool4 = need("pool4")
    master2 = need("master2")
    et = need("et")
    oneshot = need("oneshot")
    etb = need("et-broken")
    osb = need("oneshot-broken")

    expect = None
    if single:
        g = single["gen"]
        expect = num(g, "expect")
        eq("single", "gen", "verified", expect, "single/一帧不差")
        eq("single", "gen", "mismatch", 0, "single/无内容错")
        eq("single", "gen", "timeouts", 0, "single/无超时")
        eq("single", "srv", "frames", expect, "single/服务端帧数一致")

    for name, c in (("et", et), ("oneshot", oneshot)):
        if not c or expect is None:
            continue
        eq(name, "gen", "verified", expect, f"{name}/一帧不差")
        eq(name, "gen", "mismatch", 0, f"{name}/无内容错")
        eq(name, "srv", "frames", expect, f"{name}/服务端帧数一致")

    if pool4 and single:
        g = pool4["gen"]
        eq("pool4", "gen", "verified", expect, "pool4/一帧不差")
        eq("pool4", "gen", "mismatch", 0, "pool4/无内容错")
        eq("pool4", "srv", "frames", expect, "pool4/服务端帧数一致")
        eq("pool4", "srv", "q_drops", 0, "pool4/队列未丢任务")
        s_ms = num(single["gen"], "total_ms")
        p_ms = num(g, "total_ms")
        ok = s_ms and p_ms and p_ms * 10 <= s_ms * 6
        out.append((PASS if ok else FAIL, "pool4/比单线程快",
                    f"pool={p_ms}ms single={s_ms}ms（要求 ≤60%）"))

    if master2 and single:
        eq("master2", "gen", "verified", expect, "master2/一帧不差")
        eq("master2", "gen", "mismatch", 0, "master2/无内容错")
        eq("master2", "srv", "subs", 2, "master2/两个 sub")
        # 主 reactor 只转交不服务，自己的 accepted 恒为 0，所以单独看 handoff
        eq("master2", "srv", "handoff", num(master2["gen"], "clients"), "master2/连接全部转交")
        s0 = num(master2["srv"], "sub0_frames")
        s1 = num(master2["srv"], "sub1_frames")
        ok = s0 is not None and s1 is not None and s0 > 0 and s1 > 0
        out.append((PASS if ok else FAIL, "master2/连接真的分到两个 sub",
                    f"sub0={s0} sub1={s1}"))
        s_ms = num(single["gen"], "total_ms")
        m_ms = num(master2["gen"], "total_ms")
        ok = s_ms and m_ms and m_ms * 100 <= s_ms * 85
        out.append((PASS if ok else FAIL, "master2/比单线程快",
                    f"master={m_ms}ms single={s_ms}ms（要求 ≤85%）"))

    # single 必须真的"慢"：串行 8×20×2ms≈320ms，取一半作为下限
    if single:
        total = num(single["gen"], "total_ms")
        delay_us = num(single["srv"], "delay_us") or 0
        floor_ms = (expect or 0) * delay_us // 1000 // 2
        ok = total is not None and total >= floor_ms
        out.append((PASS if ok else FAIL, "single/串行确实发生",
                    f"total={total}ms 下限={floor_ms}ms"))

    # 破坏版必须坏：客户端拿到的帧数少于应有的，服务端处理的帧数也少
    if etb and expect is not None:
        v = num(etb["gen"], "verified")
        sf = num(etb["srv"], "frames")
        out.append((PASS if v is not None and v < expect else FAIL,
                    "et-broken/客户端未收齐", f"verified={v}/{expect}"))
        out.append((PASS if sf is not None and sf < expect else FAIL,
                    "et-broken/服务端确实停在半路", f"frames={sf}/{expect}"))

    if osb and expect is not None:
        v = num(osb["gen"], "verified")
        sf = num(osb["srv"], "frames")
        out.append((PASS if v is not None and v < expect else FAIL,
                    "oneshot-broken/客户端未收齐", f"verified={v}/{expect}"))
        out.append((PASS if sf is not None and sf < expect else FAIL,
                    "oneshot-broken/连接停在第一帧", f"frames={sf}/{expect}"))

    bad = 0
    for status, name, detail in out:
        print(f"[{status}] {name}  {detail}")
        if status == FAIL:
            bad += 1
    print(f"-- {len(out) - bad}/{len(out)} 通过")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
