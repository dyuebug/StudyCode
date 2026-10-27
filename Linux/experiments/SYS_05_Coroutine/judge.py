#!/usr/bin/env python3
"""SYS_05 判定器：从日志里抽读数，只断言"读数之间的关系"或内核固定常量。

用法: judge.py <log> [case ...]

日志格式（由 run.sh / coro_basics / coro_sched / client 产出）：
  S05|case|name=<用例>[|...]        用例分隔行
  S05|basics|k=v|k=v ...            基础用例读数
  S05|srv|done|k=v|k=v ...          服务端读数
  S05|gen|done|k=v|k=v ...          客户端读数

不成立的断言打印 FAIL 并以非零码退出；不做"自动修正"。
"""
import sys

PREFIX = "S05|"


def parse(path):
    """→ {case: {"meta": {}, "basics": {}, "srv": {}, "gen": {}}}，保持日志顺序"""
    cases = {}
    order = []
    cur = None

    with open(path, "r", encoding="utf-8", errors="replace") as f:
        for raw in f:
            line = raw.strip()
            if not line.startswith(PREFIX):
                continue
            parts = line.split("|")
            if len(parts) < 2:
                continue
            kind = parts[1]
            if kind == "case":
                fields = kv(parts[2:])
                # 基础用例的 rc 行与用例体同名：不带 name 的字段并进同一用例
                name = fields.get("name")
                if name is None:
                    continue
                cur = name
                if name not in cases:
                    cases[name] = {"meta": {}, "basics": {}, "srv": {}, "gen": {}, "order": []}
                    order.append(name)
                cases[name]["meta"].update(fields)
                continue
            if cur is None:
                continue
            if kind == "basics":
                bucket = "basics"
            elif kind == "srv":
                if parts[2] != "done":
                    continue
                bucket = "srv"
            elif kind == "gen":
                if parts[2] != "done":
                    continue
                bucket = "gen"
            else:
                continue
            data = kv(parts[3:] if kind != "basics" else parts[2:])
            cases[cur][bucket].update(data)
    return cases


def kv(parts):
    out = {}
    for p in parts:
        if "=" in p:
            k, v = p.split("=", 1)
            out[k] = v
    return out


def num(d, k):
    v = d.get(k)
    if v is None:
        return None
    try:
        return int(v)
    except ValueError:
        return None


def main():
    if len(sys.argv) < 2:
        print("用法: judge.py <log> [case ...]")
        return 2
    path = sys.argv[1]
    only = set(sys.argv[2:])

    try:
        cases = parse(path)
    except OSError:
        print("S05|judge|error=log_missing|path=%s" % path)
        return 2

    # (用例, 桶, 字段, 关系, 期望值)  —— 只列必要不变量
    RULES = [
        # ----- Generator：惰性、单次遍历、析构释放 -----
        ("gen", "basics", "cnt", "==", 5),
        ("gen", "basics", "sum", "==", 30),
        ("gen", "basics", "first", "==", 0),
        ("gen", "basics", "alive_after_iter", "==", 1),   # 遍历完仍持有帧
        ("gen", "basics", "alive_after_break", "==", 0),  # 提前 break 也要释放

        # ----- Task：一次 resume 靠对称转移跑完三层 -----
        ("task", "basics", "value", "==", 42),
        ("task", "basics", "resumes", "==", 1),
        ("task", "basics", "done", "==", 1),
        ("task", "basics", "alive_before_destroy", "==", 1),
        ("task", "basics", "alive_after", "==", 0),

        # ----- 异常：unhandled_exception 存 exception_ptr，await_resume 重抛 -----
        ("except", "basics", "caught", "str:", "boom"),
        ("except", "basics", "outer_value", "==", 1),
        ("except", "basics", "alive_after", "==", 0),

        # ----- 生命周期：final_suspend=suspend_always 时跑完仍占帧 -----
        ("life", "basics", "done_before", "==", 0),
        ("life", "basics", "alive_after_run", "==", 1),
        ("life", "basics", "done_after", "==", 1),
        ("life", "basics", "value", "==", 5),
        ("life", "basics", "alive_after_destroy", "==", 0),

        # ----- 破坏版：done() 之后必须拦住，不能 resume -----
        ("resume-twice", "basics", "blocked", "==", 1),
        ("resume-twice", "basics", "actual_resume_after_done", "==", 0),

        # ----- 泄漏：正确销毁归零；不销毁停在 100（计数即判据） -----
        ("leak", "basics", "alive_before_cleanup", "==", 100),
        ("leak", "basics", "alive_after", "==", 0),
        ("leak-broken", "basics", "do_destroy", "==", 0),
        ("leak-broken", "basics", "alive_after", "==", 100),

        # ----- 就绪队列：高优先级整体在前，且每个 fd 每轮至多一次 -----
        ("sched-order", "basics", "resumed", "==", 5),
        ("sched-order", "basics", "hi_first", "==", 1),
        ("sched-order", "basics", "lo_after", "==", 1),
        ("sched-order", "basics", "alive", "==", 0),

        # ----- 贪心：一次 resume 处理多帧 → 同连接连续段可以 >1 -----
        ("coro-greedy", "srv", "frames", "==", 160),
        ("coro-greedy", "srv", "max_run", ">=", 2),
        ("coro-greedy", "srv", "alive_frames", "==", 0),
        ("coro-greedy", "gen", "verified", "==", 160),
        ("coro-greedy", "gen", "mismatch", "==", 0),

        # ----- 配额 1：同连接连续段恒为 1（公平性判据） -----
        ("coro-fair", "srv", "frames", "==", 160),
        ("coro-fair", "srv", "max_run", "==", 1),
        ("coro-fair", "srv", "alive_frames", "==", 0),
        ("coro-fair", "gen", "verified", "==", 160),

        # ----- 请求-响应：正确性基线 -----
        ("coro-rr", "srv", "frames", "==", 160),
        ("coro-rr", "gen", "verified", "==", 160),
        ("coro-rr", "gen", "mismatch", "==", 0),

        # ----- 空闲超时：cancel(ETIMEDOUT) 之后连接被关闭 -----
        ("coro-cancel", "srv", "cancelled", ">=", 2),
        ("coro-cancel", "srv", "alive_frames", "==", 0),
        ("coro-cancel", "gen", "eof_seen", ">=", 2),

        # ----- RST：ECONNRESET 被单独记录，且不留帧 -----
        ("coro-rst", "srv", "resets", ">=", 1),
        ("coro-rst", "srv", "alive_frames", "==", 0),
        ("coro-rst", "gen", "verified", "==", 20),

        # ----- 半关闭：读到 EOF 而不是当错误 -----
        ("coro-halfclose", "srv", "eof", ">=", 1),
        ("coro-halfclose", "gen", "verified", "==", 20),
        ("coro-halfclose", "gen", "eof_seen", ">=", 1),

        # ----- 线程对照：每连接一个线程，结果与协程版一致 -----
        ("thread", "srv", "threads", "==", 8),
        ("thread", "srv", "frames", "==", 160),
        ("thread", "gen", "verified", "==", 160),

        # ----- 静默后才发数据：正常版会等（基线） -----
        ("coro-idle", "srv", "frames", "==", 2),
        ("coro-idle", "gen", "verified", "==", 2),

        # ----- 破坏版：EAGAIN 当 EOF → 客户端还没发就被挂断 -----
        ("eagain-as-eof", "srv", "frames", "==", 0),
        ("eagain-as-eof", "gen", "verified", "==", 0),

        # ----- 破坏版：销毁后不 detach → 事件打到已销毁槽位（计数，不踩 UB） -----
        ("cancel-free", "srv", "use_after_destroy", ">=", 1),
    ]

    fails = []
    checks = 0
    for case, bucket, key, rel, want in RULES:
        if only and case not in only:
            continue
        checks += 1
        if case not in cases:
            fails.append((case, bucket, key, "case_missing", 0))
            continue
        data = cases[case][bucket]
        if rel == "str:":
            got = data.get(key)
            ok = got == want
        else:
            got = num(data, key)
            if got is None:
                fails.append((case, bucket, key, "missing", 0))
                continue
            ok = got == want if rel == "==" else got >= want
        if not ok:
            fails.append((case, bucket, key, got, want))

    # 跨用例：线程版的 VmSize 必须明显高于协程版（每连接 8MB 栈 vs 一次 mmap 都不到）
    if not only and "thread" in cases and "coro-greedy" in cases:
        checks += 1
        tv = num(cases["thread"]["srv"], "vm_kb")
        cv = num(cases["coro-greedy"]["srv"], "vm_kb")
        if tv is None or cv is None:
            fails.append(("thread", "srv", "vm_kb", "missing", 0))
        elif tv - cv < 20000:
            fails.append(("thread", "srv", "vm_kb-coro", tv - cv, ">=20000"))

    for case, bucket, key, got, want in fails:
        print("S05|judge|case=%s|bucket=%s|check=%s|actual=%s|expect=%s|FAIL"
              % (case, bucket, key, got, want))
    print("S05|judge|summary|checks=%d|pass=%d|fail=%d"
          % (checks, checks - len(fails), len(fails)))
    return 0 if not fails else 1


if __name__ == "__main__":
    sys.exit(main())
