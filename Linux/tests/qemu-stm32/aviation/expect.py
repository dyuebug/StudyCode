#!/usr/bin/env python3
"""aviation 用例的机器判定（第 9–10 周"可机器判定"的第一步）

从 `logs/aviation_node.log` 里抽取**不变量**并判定，不比对具体时间戳——
QEMU 虚拟时钟相对注入器墙钟的比例随主机负载变化，同一事件跨运行会漂移且越靠后越大
（两次运行逐事件对照：`t≈1100` 差 41、`t≈13000` 差 75、`t≈27000` 差 154），
硬编码时刻的测试必然假失败。

判定项分两类：
  * 通用不变量（C1–C11、C15）：与剧本的具体数值无关，改剧本不受影响。
  * 剧本相关（C7 的时隙数、C12 的投放值、C13 的降级次数）：改 inject.py 时必须同步，
    否则会失败。

用法：
    python3 expect.py logs/aviation_node.log
    python3 expect.py --silent-slots 100 --dropped-frames 3 --expected-down 5 logs/aviation_node.log

退出码：0 全通过；1 有失败项；2 日志不可读或关键行缺失到无法判定。
"""

import re
import sys

# --- 故障位定义，与 include/av_health.h 保持一致 -------------------------
BIT_TIMEOUT = 1 << 0
BIT_STALE = 1 << 1
BIT_CRC = 1 << 2
BIT_RANGE = 1 << 3
BIT_RATE = 1 << 4
BIT_SOURCE = 1 << 5

# 与 main.c 的 RX_HB_TIMEOUT_MS 保持一致：心跳超时本身就是"停喂"的合法原因，
# 而且这种情况下 flags 可能全是 0（任务卡住，数据本身没坏）。
RX_HB_TIMEOUT_MS = 400

RE_MODE = re.compile(r"^\[mode\] t=(\d+) (\w+) -> (\w+) flags=0x([0-9A-Fa-f]+)")
RE_FAULT = re.compile(r"^\[fault\] t=(\d+) set=0x([0-9A-Fa-f]+) clear=0x([0-9A-Fa-f]+)")
RE_STAT = re.compile(
    r"^\[stat\] t=(\d+) mode=(\w+) cmd=(-?\d+) flags=0x([0-9A-Fa-f]+)"
    r".*age=(\d+) frames=(\d+) crc=(\d+) len=(\d+) type=(\d+) gap=(\d+) dup=(\d+) hb=(\d+)"
    r".*feed=(ON|OFF)\((\d+)/(\d+)\) heap=(\d+) hwm=(\d+)/(\d+)/(\d+)"
)


def parse_stat(line: str):
    """把一条 [stat] 解析成命名字典；匹配不上返回 None。

    为什么单独抽出来：分组序号只在这里出现一次。link.py 也要读 [stat] 做双通道
    交叉核对，如果它自己再写一套 group(N)，往格式里加一个字段就得改两处，
    而且改漏了不会报错，只会静默比较错了列——这正是最难查的那类测试缺陷。
    """
    m = RE_STAT.match(line)
    if not m:
        return None
    return {
        "t": int(m.group(1)), "mode": m.group(2), "cmd": int(m.group(3)),
        "flags": int(m.group(4), 16), "age": int(m.group(5)),
        "frames": int(m.group(6)), "crc": int(m.group(7)), "len": int(m.group(8)),
        "type": int(m.group(9)), "gap": int(m.group(10)), "dup": int(m.group(11)),
        "hb": int(m.group(12)),
        "feed": m.group(13), "feeds": int(m.group(14)), "skips": int(m.group(15)),
        "heap": int(m.group(16)),
        "hwm": (int(m.group(17)), int(m.group(18)), int(m.group(19))),
    }
# 阻塞注入（编译期，见 main.c 的 AV_DEBUG_BLOCK_RX_MS）与停喂门控的边沿日志
RE_DEBUG_START = re.compile(r"^\[debug\] t=(\d+) rx 阻塞开始 plant=(\d+) ms")
RE_DEBUG_END = re.compile(r"^\[debug\] t=(\d+) rx 阻塞结束 dur=(\d+) ms")
RE_GATE = re.compile(r"^\[gate\] t=(\d+) feed=(ON|OFF) reason=([A-Z+]+) hb=(\d+)")

STACKS = (("rx", 176), ("ctrl", 224), ("health", 224))  # 与 main.c 的 xTaskCreate 一致
RX_HB_TIMEOUT_MS = 400                                  # 与 main.c 的 RX_HB_TIMEOUT_MS 一致


class Log:
    def __init__(self, text: str):
        self.text = text
        self.modes = []   # (t, src, dst, flags)
        self.faults = []  # (t, set_mask, clear_mask)
        self.stats = []   # dict
        self.gates = []   # (t, feed, reason, hb_age)
        self.blocks = []  # (start_t, end_t, plant_ms)
        blk_start = None
        for line in text.splitlines():
            m = RE_MODE.match(line)
            if m:
                self.modes.append((int(m.group(1)), m.group(2), m.group(3), int(m.group(4), 16)))
                continue
            m = RE_FAULT.match(line)
            if m:
                self.faults.append((int(m.group(1)), int(m.group(2), 16), int(m.group(3), 16)))
                continue
            m = RE_GATE.match(line)
            if m:
                self.gates.append((int(m.group(1)), m.group(2), m.group(3), int(m.group(4))))
                continue
            m = RE_DEBUG_START.match(line)
            if m:
                blk_start = (int(m.group(1)), int(m.group(2)))
                continue
            m = RE_DEBUG_END.match(line)
            if m and blk_start is not None:
                self.blocks.append((blk_start[0], int(m.group(1)), blk_start[1]))
                blk_start = None
                continue
            m = RE_STAT.match(line)
            if m:
                self.stats.append(parse_stat(line))


def judge(log: Log, silent_slots: int, dropped_frames: int, expected_down: int):
    """返回 [(编号, 描述, 是否通过, 展示文案)]。文案按通过/失败分别给，避免 PASS 行
    打印失败原因——那会让输出自相矛盾，读日志的人要先分辨哪句才是结论。"""
    res = []

    def add(no, desc, ok, fail_detail="", pass_detail=""):
        res.append((no, desc, bool(ok), pass_detail if ok else fail_detail))

    add("C1", "启动 CRC 自检通过",
        'CRC16-CCITT-FALSE("123456789") = 0x29B1 PASS' in log.text,
        "未找到自检 PASS 行；协议两端会全帧互斥", "0x29B1，与 PC 注入器一致")

    add("C2", "至少 3 条 [stat]（覆盖到稳态）", len(log.stats) >= 3,
        f"实际只有 {len(log.stats)} 条", f"实际 {len(log.stats)} 条")

    add("C3", "INIT → NORMAL 发生",
        any(d == "NORMAL" and s == "INIT" for _, s, d, _ in log.modes),
        "启动后从未靠数据进入 NORMAL", "由 3 个有效帧触发")

    seen = 0
    for _, set_mask, _ in log.faults:
        seen |= set_mask
    for bit, name in ((BIT_TIMEOUT, "T"), (BIT_STALE, "STALE"), (BIT_CRC, "CRC"),
                      (BIT_RANGE, "RNG"), (BIT_RATE, "RATE")):
        add("C4", f"故障位 {name} 至少置位一次", (seen & bit) != 0,
            "该位从未置位：剧本未覆盖，或判定逻辑未触发", "已观测")

    down = sum(1 for _, s, d, _ in log.modes if s == "NORMAL" and d == "DEGRADED")
    up = sum(1 for _, s, d, _ in log.modes if s == "DEGRADED" and d == "NORMAL")
    add("C5", "多次降级与恢复（不可只降不恢复）", down >= 4 and up >= 3,
        f"降级 {down} 次、恢复 {up} 次，次数不足", f"降级 {down} 次、恢复 {up} 次")

    to_safe = [t for t, s, d, _ in log.modes if s == "DEGRADED" and d == "SAFE"]
    from_safe = [t for t, s, d, _ in log.modes if s == "SAFE" and d == "DEGRADED"]
    add("C6a", "长时间故障推进到 SAFE", len(to_safe) >= 1,
        "从未进入 SAFE", f"{len(to_safe)} 次")
    add("C6b", "SAFE 经 DEGRADED 两级恢复", len(from_safe) >= 1,
        "从未从 SAFE 恢复", f"{len(from_safe)} 次")

    # 3000 ms 是**DEGRADED 停留门限**：从进入 DEGRADED 到 DEGRADED→SAFE。
    # 不是 SAFE 的停留时长——后者由数据何时恢复决定（本例 2948 ms 只是剧本巧合）。
    # 容差 ±150 ms：20 ms 检查颗粒 + tick 边界对齐。
    dwells, deg_entry = [], None
    for t, s, d, _ in log.modes:
        if d == "DEGRADED":
            deg_entry = t
        elif s == "DEGRADED" and d == "SAFE" and deg_entry is not None:
            dwells.append(t - deg_entry)
            deg_entry = None
    dwell_bad = [x for x in dwells if not (2850 <= x <= 3150)]
    add("C6c", "DEGRADED 停留到 3000 ms 门限才进 SAFE", dwells and not dwell_bad,
        ("、".join(f"{x} ms" for x in dwells) or "无完整 DEGRADED 段") + "（超出 2850–3150 容差）",
        "、".join(f"{x} ms" for x in dwells) + f"（{len(dwells)} 段）")

    if log.stats:
        last = log.stats[-1]
        # 三个来源相加：crc 错帧（序号没被"看到"）、静默时隙、剧本里主动丢掉的帧。
        # 漏掉任何一项都会让这条不变量变成噪声。
        expect_gap = last["crc"] + silent_slots + dropped_frames
        add("C7", "序号缺口自洽（gap = crc + 静默时隙 + 丢帧）",
            last["gap"] == expect_gap,
            f"gap={last['gap']}，期望 {last['crc']} + {silent_slots} + {dropped_frames} = {expect_gap}",
            f"gap={last['gap']} = crc {last['crc']} + 静默 {silent_slots} + 丢帧 {dropped_frames}")

    heaps = {s["heap"] for s in log.stats}
    add("C8", "空闲堆在稳态下恒定（无泄漏）", len(heaps) == 1,
        f"读数不唯一 {sorted(heaps)}", f"读数恒定 {sorted(heaps)}")

    if log.stats:
        worst = [min(s["hwm"][i] for s in log.stats) for i in range(3)]
        ok, detail = True, []
        for (name, total), free in zip(STACKS, worst):
            enough = free >= 32 and free * 100 >= total * 25
            ok = ok and enough
            detail.append(f"{name} {free}/{total}")
        add("C9", "栈水位满足「绝对 ≥32 字 / 相对 ≥25%」", ok,
            "、".join(detail) + "（有任务不达标）", "、".join(detail))

    # 门控是"与"语义，但只对阻塞性原因成立：feed_ok = rx_live && !(T|STALE)。
    # CRC/RNG/RATE/SRC 不参与门控（单帧坏数据不该让整机停喂），所以"feed=ON 且 flags≠0"
    # 是合法状态。之前那条 "feed=ON ⇒ flags==0" 的判据会把它读成矛盾——判定器错，
    # 固件没错；这类误判只在 stat 恰好落进 RNG/RATE 窗口时暴露，属于偶发失败。
    bad_feed = []
    for s in log.stats:
        block_flags = (s["flags"] & (BIT_TIMEOUT | BIT_STALE)) != 0
        block_hb = s["hb"] > RX_HB_TIMEOUT_MS
        if s["feed"] == "ON" and (block_flags or block_hb):
            bad_feed.append(s)                  # 有阻塞原因却仍在喂狗
        elif s["feed"] == "OFF" and not (block_flags or block_hb):
            bad_feed.append(s)                  # 两条原因都不成立却停喂
    add("C10", "喂狗门控与故障标志一致（与语义）", not bad_feed,
        f"{len(bad_feed)} 条 stat 的 feed 与 flags/hb 矛盾", "全部一致")

    bad_pair = [s for s in log.stats if (s["flags"] & BIT_RANGE) and (s["flags"] & BIT_RATE)]
    add("C11", "RNG 与 RATE 不同时置位（剧本各段只改一个变量）", not bad_pair,
        f"{len(bad_pair)} 条同时置位", "无同时置位")

    # 剧本相关：NORMAL 由控制律给出 cmd，SAFE 必须回中位
    cmd_bad = [s for s in log.stats
               if (s["mode"] == "SAFE" and s["cmd"] != 0)
               or (s["mode"] == "NORMAL" and s["cmd"] != -50)
               or (s["mode"] == "DEGRADED" and s["cmd"] not in (0, -50))]
    add("C12", "SAFE 回中位、NORMAL 由控制律给出指令（剧本相关）", not cmd_bad,
        f"{len(cmd_bad)} 条不符（改过剧本则本条需同步）", "符合")

    # 剧本相关：丢帧段（3 个时隙 ≈ 174 固件毫秒）短于 300 ms 超时门限，不应产生新的
    # NORMAL→DEGRADED。降级次数严格相等而不是 ≥，正是"没有多降一次"的判据；
    # 把丢帧段拉长到超过门限，本条会失败——这是它的用途，不是它的脆弱。
    #
    # 期望的 5 次分别是：badcrc、range、rate、silent，以及**剧本放完后**的静默
    # （注入器 8.2 s 退出、QEMU 继续跑到超时，链路无字节 → T/STALE → 降级）。
    # 最后一次不是剧本里的故障段，而是收尾静默；把预期写成 4 会让"丢帧多降一次"
    # 与"少一次收尾降级"互相抵消，判据就失去意义——所以宁可显式列出五项。
    # 每段之间必须先恢复成 NORMAL 才会产生新的一次，这正是 inject.py 里那几段
    # "valid" 窗口的宽度要求（见其 PHASES 注释）。
    add("C13", "短时丢帧未触发额外降级（剧本相关）", down == expected_down,
        f"NORMAL→DEGRADED 出现 {down} 次，期望 {expected_down} 次"
        f"（badcrc/range/rate/silent/收尾静默各 1 次）：丢帧段可能超过了超时门限",
        f"NORMAL→DEGRADED {down} 次，与剧本一致")

    # 任务阻塞：只在编译期开了注入的构建里判定（CMakeLists 在 F103_QEMU 下默认 800 ms）。
    # 没开注入时报"不适用"并保持通过——若报 SKIP 而不计入总数，两个构建的通过项数目就
    # 不可比；文案里写明未启用，避免把"没跑"读成"跑过"。
    #
    # 三条子判据缺一不可：
    #   1. 实际阻塞时长与注入计划一致（否则"阻塞"可能没生效，日志只是走了个过场）；
    #   2. 阻塞前最后一次门控采样里心跳**未**超时（排除"阻塞前就已经 HB"的伪因果）；
    #   3. 阻塞窗口内出现过 reason 含 HB 且 hb > 门限 的门控行（这才是因果本身）。
    if not log.blocks:
        add("C14", "任务阻塞：心跳超时触发停喂（HB 原因）", True,
            "", "未启用调试注入（本条不适用）")
    else:
        bad = []
        for start, end, plant in log.blocks:
            dur = end - start
            before = [g for g in log.gates if g[0] < start]
            in_win = [g for g in log.gates if start < g[0] <= end]
            if abs(dur - plant) > 5:
                bad.append(f"阻塞 {dur} ms ≠ 计划 {plant} ms")
            if before and before[-1][3] > RX_HB_TIMEOUT_MS:
                bad.append(f"阻塞前心跳已 {before[-1][3]} ms（HB 原因不能归给注入）")
            if not any("HB" in g[2] and g[3] > RX_HB_TIMEOUT_MS for g in in_win):
                bad.append(f"窗口内无 HB 门控行（窗口内 {len(in_win)} 条）")
        add("C14", "任务阻塞：心跳超时触发停喂（HB 原因）", not bad,
            "；".join(bad), "阻塞期间 hb 超门限且门控原因含 HB")

    # 时间戳年龄必须在合理区间：age/hb 都是"now - 某个时间戳"。时间戳由更高优先级的
    # rx 任务写入，可能比 health 的快照晚 1～2 tick，无符号减法会下溢成 ~4.29e9。
    # 实测抓到过一次 age=4294967292（由 av_tick_elapsed 修掉）；hb 下溢的后果更重——
    # 门控会读成"心跳早断了"并误停一次喂狗。这里用 < 2^31 作为"没有下溢"的判据。
    overflow = [s for s in log.stats if s["age"] >= 1 << 31 or s["hb"] >= 1 << 31]
    add("C15", "时间戳年龄无下溢（age/hb < 2^31）", not overflow,
        f"{len(overflow)} 条 stat 的 age/hb 为下溢值："
        + "；".join(f"t={s['t']} age={s['age']} hb={s['hb']}" for s in overflow),
        "全部在合理区间")

    return res


def report_timings(log: Log):
    """把检测与恢复延迟从事件序列里算出来，供人读。

    只做相邻事件差值，不新增判据——判据在 judge() 里，这里不产生 PASS/FAIL，
    免得把"统计"和"判定"混成一件事。同 tick 时 fault 先于 mode 处理：
    `[fault] set` 与 `[mode] 降级` 常写在同一 tick，反过来算会得出错误的延迟。
    """
    events = [(t, 1, "mode", (s, d)) for t, s, d, _ in log.modes]
    events += [(t, 0, "fault", (set_m, clear_m)) for t, set_m, clear_m in log.faults]
    events.sort(key=lambda e: (e[0], e[1]))

    fade, rec, dwell, safe_out, safe_first = [], [], [], [], []
    last_set = last_clear = None
    cur, cur_t, cur_from = "INIT", None, None

    for t, _, kind, payload in events:
        if kind == "fault":
            set_m, clear_m = payload
            if set_m:
                last_set = t
            if clear_m:
                last_clear = t
            continue
        src, dst = payload
        if src == "SAFE" and dst == "DEGRADED":
            if cur == "SAFE" and cur_t is not None:
                safe_out.append(t - cur_t)      # 进入 SAFE → 退出 SAFE，受剧本控制
            if last_clear is not None:
                safe_first.append(t - last_clear)  # 故障清除 → 第一级恢复完成
        if dst == "DEGRADED":
            # SAFE→DEGRADED 是恢复的第一级，其耗时归 safe_first / safe_out，不算"故障置位→降级"
            if src != "SAFE" and last_set is not None:
                fade.append(t - last_set)
        elif dst == "SAFE" and cur == "DEGRADED" and cur_t is not None:
            dwell.append(t - cur_t)
        elif dst == "NORMAL" and src == "DEGRADED":
            # SAFE 之后那一跳由 5 帧+3 帧决定，与"故障清除"不对应，单列
            if cur_from != "SAFE" and last_clear is not None:
                rec.append(t - last_clear)
        cur, cur_t, cur_from = dst, t, src

    def fmt(vals, unit, note=""):
        if not vals:
            return f"无{note}"
        return "、".join(f"{v} {unit}" for v in vals) + f"（{len(vals)} 次{note}）"

    # 阻塞注入 → 首次被判心跳超时。没有注入时列表为空，fmt() 会写"无"。
    blk_hb = []
    for start, end, _ in log.blocks:
        first = [g[0] - start for g in log.gates if start < g[0] <= end and "HB" in g[2]]
        if first:
            blk_hb.append(first[0])

    return [
        "故障置位 → 降级：" + fmt(fade, "tick"),
        "故障清除 → 恢复 NORMAL：" + fmt(rec, "tick", "；不含 SAFE 之后"),
        "故障清除 → SAFE→DEGRADED：" + fmt(safe_first, "tick", "；两级恢复的第一级"),
        "DEGRADED 进入 → SAFE 进入：" + fmt(dwell, "ms", "；门限 3000"),
        "SAFE 进入 → SAFE→DEGRADED：" + fmt(safe_out, "ms", "；受剧本恢复时刻控制"),
        "阻塞开始 → 心跳判超时：" + (
            fmt(blk_hb, "tick", f"；门限 {RX_HB_TIMEOUT_MS}") if blk_hb
            else f"无（未启用注入，或窗口内未判到 HB；门限 {RX_HB_TIMEOUT_MS}）"),
    ]


def main() -> int:
    args = sys.argv[1:]
    silent_slots, dropped_frames, expected_down = 100, 3, 5
    for flag, cast in (("--silent-slots", int), ("--dropped-frames", int), ("--expected-down", int)):
        if flag in args:
            i = args.index(flag)
            try:
                value = cast(args[i + 1])
            except (IndexError, ValueError):
                print(f"{flag} 需要一个整数参数", file=sys.stderr)
                return 2
            if flag == "--silent-slots":
                silent_slots = value
            elif flag == "--dropped-frames":
                dropped_frames = value
            else:
                expected_down = value
            del args[i:i + 2]
    if len(args) != 1:
        print("用法：expect.py [--silent-slots N] [--dropped-frames N] [--expected-down N] <日志路径>",
              file=sys.stderr)
        return 2

    try:
        with open(args[0], "r", encoding="utf-8", errors="replace") as f:
            text = f.read()
    except OSError as exc:
        print(f"[expect] 读不到日志：{exc}", file=sys.stderr)
        return 2

    log = Log(text)
    if not log.stats:
        print("[expect] 日志里没有任何 [stat] 行，无法判定（固件没跑到稳态？）", file=sys.stderr)
        return 2

    results = judge(log, silent_slots, dropped_frames, expected_down)
    failed = [r for r in results if not r[2]]
    print(f"[expect] {args[0]}")
    for no, desc, ok, detail in results:
        print(f"  {'PASS' if ok else 'FAIL'} {no:4s} {desc}" + (f" —— {detail}" if detail else ""))
    print("[timing] 延迟汇总（统计，不参与判定）：")
    for line in report_timings(log):
        print(f"  {line}")
    print(f"[expect] {len(results) - len(failed)}/{len(results)} 通过"
          + ("" if not failed else f"，失败：{', '.join(r[0] for r in failed)}"))
    return 0 if not failed else 1


if __name__ == "__main__":
    sys.exit(main())
