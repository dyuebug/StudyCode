#!/usr/bin/env python3
"""下行二进制状态帧的解码与自检（USART2 通道）

固件在 health 任务里每 100 ms 发一帧 STATUS（格式见
`projects/aviation-node/include/av_downlink.h`）：

    AA 55 | TYPE(0x81) | SEQ | LEN(18) | PAYLOAD | CRC16-LE

CRC16-CCITT-FALSE 覆盖 TYPE..PAYLOAD（与上行同规则），所以这个脚本用注入器的
同一份实现来验证——两边算得不一样，校验必然全错，这本身就是一次交叉核对。

判据（L1–L7）判的是不变量，不比对绝对时刻（与 expect.py 同样的理由：QEMU 时基
2.90 倍且跨运行漂移）：

    L1 至少解出 1 帧
    L2 所有完整帧 CRC 正确
    L3 SEQ 逐帧 +1（允许 255→0 回绕），无跳号、无重复
    L4 TYPE 恒为 0x81、LEN 恒为 18
    L5 uptime 严格递增
    L6 模式序列里出现过 NORMAL，且不只有 NORMAL（剧本注入了故障）
    L7（可选 --ascii）与上行日志交叉核对：用二进制帧的 uptime 找最近一条
       `[stat]`，两者的 mode/frames/crc/gap 必须一致

用法：
    python3 link.py logs/aviation_downlink.bin
    python3 link.py logs/aviation_downlink.bin --ascii logs/aviation_node.log

退出码：0 全通过；1 有失败项；2 文件不可读或没有可解帧。
"""

import argparse
import struct
import sys

from expect import RE_MODE, parse_stat
from inject import crc16_ccitt_false

SOF = b"\xAA\x55"
TYPE_STATUS = 0x81
LEN_STATUS = 18
PAYLOAD_FMT = "<IBBhhhHHH"           # uptime, mode, flags, cmd, air, pitch, frames, crc, gap
MODE_NAMES = {0: "INIT", 1: "NORMAL", 2: "DEGRADED", 3: "SAFE"}


class Frame:
    def __init__(self, seq, uptime, mode, flags, cmd, air, pitch, frames, crc_err, gap):
        self.seq = seq
        self.uptime = uptime
        self.mode = mode
        self.flags = flags
        self.cmd = cmd
        self.air = air
        self.pitch = pitch
        self.frames = frames
        self.crc_err = crc_err
        self.gap = gap

    def mode_name(self):
        return MODE_NAMES.get(self.mode, f"?{self.mode}")


def parse(data):
    """按 SOF 定位解帧。返回 (frames, bad_crc, bad_len, bad_type, skipped_bytes)。"""
    frames, bad_crc, bad_len, bad_type, skipped = [], 0, 0, 0, 0
    i = 0
    n = len(data)
    while i + 5 <= n:
        if data[i:i + 2] != SOF:
            i += 1
            skipped += 1
            continue
        ftype = data[i + 2]
        seq = data[i + 3]
        length = data[i + 4]
        end = i + 5 + length + 2
        if end > n:
            break                       # 尾部被截断（QEMU 退出时正常现象）
        payload = data[i + 5:i + 5 + length]
        crc_rx = data[i + 5 + length] | (data[i + 5 + length + 1] << 8)
        crc_calc = crc16_ccitt_false(data[i + 2:i + 5 + length])
        if crc_calc != crc_rx:
            bad_crc += 1
            i += 2
            continue
        if ftype != TYPE_STATUS:
            bad_type += 1
        if length != LEN_STATUS:
            bad_len += 1
        else:
            uptime, mode, flags, cmd, air, pitch, fr, ce, gap = struct.unpack(
                PAYLOAD_FMT, payload)
            frames.append(Frame(seq, uptime, mode, flags, cmd, air, pitch, fr, ce, gap))
        i = end
    return frames, bad_crc, bad_len, bad_type, skipped


def checks(frames, bad_crc, bad_len, bad_type):
    res = []

    def add(tag, desc, ok, note=""):
        res.append((tag, desc, bool(ok), note))

    add("L1", "至少解出 1 帧", frames, f"{len(frames)} 帧")
    add("L2", "所有完整帧 CRC 正确", bad_crc == 0, f"坏帧 {bad_crc} 个")
    # 只有 0/1 帧时"逐帧比较"为空——空条件按真处理（没有可违反的相邻对），
    # 单帧样本的强弱由 L1/L6 反映，不在这里伪造一个失败。
    add("L3", "SEQ 逐帧 +1（允许 255→0 回绕）",
        all((frames[k].seq - frames[k - 1].seq) % 256 == 1 for k in range(1, len(frames))),
        f"首末 SEQ {frames[0].seq} → {frames[-1].seq}" if frames else "无帧")
    add("L4", "TYPE 恒为 0x81、LEN 恒为 18", bad_type == 0 and bad_len == 0,
        f"类型错 {bad_type}、长度错 {bad_len}")
    add("L5", "uptime 严格递增",
        all(frames[k].uptime > frames[k - 1].uptime for k in range(1, len(frames))),
        f"{frames[0].uptime} → {frames[-1].uptime} ms" if frames else "无帧")
    modes = [f.mode_name() for f in frames]
    add("L6", "模式覆盖 NORMAL 且不只有 NORMAL",
        ("NORMAL" in modes) and (len(set(modes)) > 1), " ".join(sorted(set(modes))))
    return res


MAX_COUNTER_LAG = 3   # 两次读之间允许 rx 抢跑几帧（见下），50 Hz 下 3 帧 = 60 ms


def cross_check(frames, ascii_path):
    """用 uptime 找"同一次 health 迭代"的两个视角。

    main.c 里下行帧与 `[stat]` 在同一个 100 ms 周期、用同一个 `now` 输出，所以
    两者 t 相等的记录属于同一时刻。但两者不是原子的：health 的优先级是 2，
    rx 是 3，`av_downlink_status()` 的 TXE 轮询期间 rx 会抢占并再接受若干帧——
    于是 `[stat]` 的 frames 可能比同一 t 的下行帧大 1～2。这类差异是抢占，
    不是不一致，所以计数器按"只增 + 增量有上界"核对；逐字相等会把抢占判成缺陷。

    mode/flags 则必须相等：flags 是同一个局部变量传下去的，只有 ctrl（优先级 4）
    抢在两次读之间改了模式才会不同——那种情况一定伴随一条同 t 的 `[mode]` 行，
    因此把"有迁移行"作为 mode 差异的合法解释。
    """
    try:
        with open(ascii_path, "r", encoding="utf-8", errors="replace") as fh:
            stats, moved = {}, set()
            for line in fh:
                line = line.strip()
                row = parse_stat(line)
                if row:
                    stats[row["t"]] = row
                    continue
                m = RE_MODE.match(line)
                if m:
                    moved.add(int(m.group(1)))
    except OSError as exc:
        return ("L7", "与上行日志交叉核对", False, f"读不到 {ascii_path}: {exc}")

    by_uptime = {f.uptime: f for f in frames}
    pairs = [(t, stats[t]) for t in sorted(stats) if t in by_uptime]
    if len(pairs) < 2:
        return ("L7", "与上行日志交叉核对", False,
                f"只有 {len(pairs)} 组 t 相等的记录，样本不足（需要 ≥2）")

    bad, late = [], []
    for t, s in pairs:
        f = by_uptime[t]
        diffs = []
        if s["mode"] != f.mode_name() and t not in moved:
            diffs.append(f"mode {s['mode']}≠{f.mode_name()}（且无同 t 的 [mode] 行）")
        if s["flags"] & 0xFF != f.flags:
            diffs.append(f"flags {s['flags'] & 0xFF:02X}≠{f.flags:02X}")
        for label, expect, got in (("frames", s["frames"], f.frames),
                                   ("crc", s["crc"], f.crc_err),
                                   ("gap", s["gap"], f.gap)):
            if expect < got or expect - got > MAX_COUNTER_LAG:
                diffs.append(f"{label} {expect} vs {got}")
            elif expect != got:
                late.append(f"{label}+{expect - got}")
        if diffs:
            bad.append(f"t={t}: " + "、".join(diffs))

    note = f"{len(pairs)} 组同一时刻记录"
    if late:
        note += f"（{len(late)} 项计数器因 rx 抢占前进 1～{MAX_COUNTER_LAG}）"
    note += ("；不一致 → " + " | ".join(bad)) if bad else "；全部一致"
    return ("L7", "与上行日志交叉核对（mode/flags/frames/crc/gap）", not bad, note)


def main() -> int:
    ap = argparse.ArgumentParser(description="解析下行二进制状态帧并自检")
    ap.add_argument("bin_path", help="USART2 抓到的二进制流（logs/aviation_downlink.bin）")
    ap.add_argument("--ascii", dest="ascii_path", default=None,
                    help="上行 ASCII 日志，用于 L7 交叉核对")
    args = ap.parse_args()

    try:
        with open(args.bin_path, "rb") as fh:
            data = fh.read()
    except OSError as exc:
        print(f"读不到 {args.bin_path}: {exc}", file=sys.stderr)
        return 2

    frames, bad_crc, bad_len, bad_type, skipped = parse(data)
    print(f"输入 {len(data)} 字节，解出 {len(frames)} 帧，"
          f"CRC 错 {bad_crc}、长度错 {bad_len}、类型错 {bad_type}，跳过 {skipped} 字节")
    if not frames:
        print("没有可解帧：检查 USART2 是否接到了第二个串口后端", file=sys.stderr)
        return 2

    # 帧间隔只报告不判定：QEMU 时基是固件的 2.90 倍，绝对值无意义
    gaps = [frames[k].uptime - frames[k - 1].uptime for k in range(1, len(frames))]
    if gaps:
        gaps_sorted = sorted(gaps)
        print(f"帧间隔（固件 ms）：中位 {gaps_sorted[len(gaps_sorted) // 2]}，"
              f"最小 {min(gaps)}，最大 {max(gaps)}（设计 100）")

    print("\n首帧：" + f"t={frames[0].uptime} {frames[0].mode_name()} "
          f"flags=0x{frames[0].flags:02X} cmd={frames[0].cmd} "
          f"air={frames[0].air} pitch={frames[0].pitch} "
          f"frames={frames[0].frames} crc={frames[0].crc_err} gap={frames[0].gap}")
    print("末帧：" + f"t={frames[-1].uptime} {frames[-1].mode_name()} "
          f"flags=0x{frames[-1].flags:02X} cmd={frames[-1].cmd} "
          f"air={frames[-1].air} pitch={frames[-1].pitch} "
          f"frames={frames[-1].frames} crc={frames[-1].crc_err} gap={frames[-1].gap}")

    transitions = []
    for k in range(1, len(frames)):
        if frames[k].mode != frames[k - 1].mode:
            transitions.append(f"t={frames[k].uptime} {frames[k - 1].mode_name()}"
                               f"→{frames[k].mode_name()}")
    print("模式迁移：" + ("；".join(transitions) if transitions else "无"))

    res = checks(frames, bad_crc, bad_len, bad_type)
    if args.ascii_path:
        res.append(cross_check(frames, args.ascii_path))

    print()
    failed = 0
    for tag, desc, ok, note in res:
        print(f"{'PASS' if ok else 'FAIL'} {tag} {desc} —— {note}")
        if not ok:
            failed += 1
    print(f"\n{len(res) - failed}/{len(res)} 通过")
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
