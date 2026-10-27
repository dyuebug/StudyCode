#!/usr/bin/env python3
"""PC 侧传感器帧注入器（航空主题地面仿真节点）

把二进制帧写到 stdout，由 tests/qemu-stm32/run.sh 管道喂给 QEMU 的串口。
不依赖 pyserial：QEMU 的 `-serial stdio` 就是一个字节管道，用管道比开虚拟串口
更容易在 CI/WSL 里复现，也不需要额外权限。

用法：
    python3 inject.py            # 默认剧本（约 14.3 s 真实时间）
    python3 inject.py --list     # 只看剧本，不发数据

剧本按"真实秒"排期，帧率固定 50 Hz。注意固件侧的毫秒是 QEMU 时基下的
"固件毫秒"，实测比真实时间快约 2.90 倍（tests/qemu-stm32/RESULT.md §3.3），
所以本文件里的 0.02 s ≈ 58 固件毫秒，而固件里的 300 ms 超时 ≈ 0.10 s 真实时间。
判据一律写在固件里（以固件毫秒为单位），本文件只负责按时间投放。

两条上行流各自独立：
  * SENSOR（0x01）50 Hz —— 传感器数据，载荷 8 字节；
  * STATUS（0x02）10 Hz —— 地面站心跳，载荷 4 字节（地面站本地 uptime_ms）。
STATUS 的序号与 SENSOR 分开计数，混用会让两侧的"缺口"统计都失真。

帧格式（小端，CRC 覆盖 TYPE..PAYLOAD）：
    AA 55 | TYPE | SEQ | LEN | PAYLOAD | CRC16-LE
SENSOR 载荷：airspeed(int16, 0.1 m/s) pitch(int16, 0.01°) roll(int16, 0.01°) flags(u16)
"""

import argparse
import sys
import time

SOF0, SOF1 = 0xAA, 0x55
TYPE_SENSOR = 0x01
TYPE_STATUS = 0x02
PERIOD_S = 0.02          # 50 Hz
PERIOD_MS = int(PERIOD_S * 1000)
START_S = 0.50           # 留出固件启动与串口初始化时间
NOMINAL = (150, 100, 0)  # 15.0 m/s, +1.00°, 0.00°
STATUS_EVERY = 5         # 每 5 个 50 Hz 时隙 = 100 ms 一帧心跳


def crc16_ccitt_false(data: bytes) -> int:
    """多项式 0x1021，初值 0xFFFF，不反射，不异或输出。"""
    crc = 0xFFFF
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if (crc & 0x8000) else (crc << 1) & 0xFFFF
    return crc


def build_frame(seq: int, payload: bytes, corrupt_crc: bool = False,
                ftype: int = TYPE_SENSOR) -> bytes:
    body = bytes([ftype, seq & 0xFF, len(payload)]) + payload
    crc = crc16_ccitt_false(body)
    if corrupt_crc:
        crc ^= 0xFFFF          # 只翻 CRC 字段，模拟"数据完好但校验失败"
    return bytes([SOF0, SOF1]) + body + crc.to_bytes(2, "little")


def build_status(seq: int, uptime_ms: int) -> bytes:
    """地面站心跳：载荷是地面站自己的 uptime_ms。

    不用序号表达"地面站重启"：uint8 序号回绕到 0 与重启后从 0 开始无法区分，
    而时间戳能区分"继续数"和"重新数"。"""
    return build_frame(seq, (uptime_ms & 0xFFFFFFFF).to_bytes(4, "little"), ftype=TYPE_STATUS)


def s16(value: int) -> bytes:
    """有符号 16 位小端。flags 走无符号分支，所以单独用 u16。"""
    return (value & 0xFFFF).to_bytes(2, "little")


def payload_of(air: int, pitch: int, roll: int, flags: int = 0) -> bytes:
    return s16(air) + s16(pitch) + s16(roll) + (flags & 0xFFFF).to_bytes(2, "little")


# (名称, 起始秒, 结束秒, 说明) —— 每个阶段只改一个变量，便于串口日志归因
#
# 3.60–4.30 这段"恢复"窗口不能太窄：RNG/RATE/SRC 的保持期是 1000 ms **固件**时间
# （≈345 ms 真实时间），从最后一个越界帧开始计时。原来窗口只到 4.00，扣掉保持期后
# 真正能用来恢复的余量只有 ~55 ms 真实时间——是否在突变段之前回到 NORMAL 全看抖动，
# 多一条串口通道（下行二进制）就会把结果翻过去。窗口拉到 700 ms 后，
# "越界→恢复→突变再次降级"这三步才是确定性的。
PHASES = [
    ("valid",  0.50, 2.00, "正常帧：INIT → NORMAL"),
    ("badcrc", 2.00, 2.60, "CRC 错帧：CRC 标志 + 链路/数据超时"),
    ("valid",  2.60, 3.20, "恢复：连续有效帧后回 NORMAL"),
    ("range",  3.20, 3.60, "越界：pitch = 90.00°（> 60°）"),
    ("valid",  3.60, 4.30, "恢复：留出 ≥ 保持期的余量，使降级→恢复可确定"),
    ("rate",   4.30, 4.55, "突变：pitch 1.00° → 45.00°（> 20°）"),
    ("valid",  4.55, 5.00, "恢复（突变帧全部被拒，回到原值才被接受）"),
    ("silent", 5.00, 7.00, "静默：无任何字节 → TIMEOUT/STALE → DEGRADED → SAFE"),
    ("valid",  7.00, 7.60, "恢复：SAFE → DEGRADED → NORMAL"),
    ("drop",   7.60, 7.66, "丢帧：连续 3 帧不发（seq 仍递增）→ 只增 gap，不降级"),
    ("valid",  7.66, 8.20, "恢复稳态：确认丢帧没有触发降级"),
    ("hb_only", 8.20, 11.80, "只发地面站心跳、停发 SENSOR → STALE 单独置位，T 不置位"),
    ("hb_lost", 11.80, 14.30, "只发 SENSOR、停发地面站心跳 → GND 置位，模式不变"),
]

# 不发 SENSOR 字节的阶段。三者都跳过 SENSOR，但含义不同：
#   silent  —— 模拟链路断开，长度超过 300 ms 超时门限，应当降级；
#   drop    —— 模拟单帧丢失，长度短于超时门限，应当只增 gap；
#   hb_only —— 地面站不发数据但仍在发心跳：链路层活着（STATUS 刷新 T 的计时），
#              数据层断了（没有样本 → STALE）。这一段就是把 T 与 STALE 拆开看的证据。
#              时长按"段内至少 2 条 [stat]"定（固件 10.6 s > 2 × 5000 ms）——
#              只有 1 条时无法判"相邻两条之间 SENSOR 帧数不变"。
# 3 个时隙 = 60 ms 真实 ≈ 174 固件毫秒，距离 300 ms 超时门限还有 126 tick 余量。
SILENT_PHASES = ("silent", "drop", "hb_only")

# 连地面站心跳也不发的阶段：只有"整条链路断开"与"地面站自己失联"两种。
# silent 必须在内——否则链路层（T）永远不会置位，剧本就测不到它了。
NO_STATUS_PHASES = ("silent", "hb_lost")


def phase_at(t: float):
    for name, start, end, _desc in PHASES:
        if start <= t < end:
            return name
    return None


def frame_for(phase: str, index: int) -> bytes:
    if phase == "badcrc":
        return build_frame(index, payload_of(*NOMINAL), corrupt_crc=True)
    if phase == "range":
        return build_frame(index, payload_of(150, 9000, 0))
    if phase == "rate":
        return build_frame(index, payload_of(150, 4500, 0))
    return build_frame(index, payload_of(*NOMINAL))


def main() -> int:
    ap = argparse.ArgumentParser(description="航空节点上行帧注入器")
    ap.add_argument("--list", action="store_true", help="只打印剧本，不发送")
    ap.add_argument("--start", type=float, default=START_S, help="首帧时刻（真实秒）")
    args = ap.parse_args()

    if args.list:
        for name, start, end, desc in PHASES:
            print(f"{start:5.2f}–{end:5.2f}s  {name:8s} {desc}")
        return 0

    end_s = PHASES[-1][2]
    index = 0
    status_seq = 0
    status_sent = 0
    dropped = 0
    clock0 = time.monotonic()
    out = sys.stdout.buffer
    slot = 0

    while True:
        # 用"起点 + 序号×周期"而不是浮点累加：累加误差虽小，但阶段边界恰好落在网格点上，
        # 差一个 ulp 就会让某个时隙归属相邻阶段（丢帧段多一帧或少一帧）。
        t = args.start + slot * PERIOD_S
        if t >= end_s:
            break
        slot += 1

        # 绝对截止时间：避免 sleep 误差累积成帧率漂移
        delay = (clock0 + t) - time.monotonic()
        if delay > 0:
            time.sleep(delay)

        phase = phase_at(t)
        if phase is None:
            continue

        # 序号按 50 Hz 网格递增，即使不发也同样 +1：
        # 这样静默段结束后接收侧会看到 seq 跳变，"丢帧统计"才有可核对的对象。
        index += 1

        wrote = False

        # 地面站心跳（10 Hz）与 SENSOR 独立调度：hb_lost 段只停心跳，其余段照常发。
        # 用 slot 网格算整数毫秒，不用浮点累加（理由同上）。
        if phase not in NO_STATUS_PHASES and (slot - 1) % STATUS_EVERY == 0:
            status_seq = (status_seq + 1) & 0xFF
            out.write(build_status(status_seq, slot * PERIOD_MS))
            status_sent += 1
            wrote = True

        if phase in SILENT_PHASES:
            if phase == "drop":
                dropped += 1
        else:
            out.write(frame_for(phase, index))
            wrote = True

        if wrote:
            out.flush()

    print(f"[inject] 剧本结束：{index} 个 50 Hz 时隙（丢帧 {dropped} 个）、"
          f"{status_sent} 帧地面站心跳，{end_s - args.start:.1f} s",
          file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
