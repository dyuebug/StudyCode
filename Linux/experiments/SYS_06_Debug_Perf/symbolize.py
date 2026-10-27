#!/usr/bin/env python3
"""把 crash 用例里的回溯地址翻译成"函数 + 源码行"。

这是"没有 gdb 怎么定位崩溃"的最后一段：crash.c 给出运行时地址与模块基址之差，
这里把差值喂给 addr2line。两处必须做对：

  1. 只有 **返回地址** 要减 1；**故障 PC** 是出错那条指令自己的地址，减 1 会掉进前一个符号
     （首版就因此把 f3 解成了 frame_dummy）。is_pc=1 的帧原样查。
  2. PIE 下不能拿运行时绝对地址去查符号表——那不是文件里的地址，只会得到 ??。
     这条作为反例一起记录下来，省得下一个人再踩。

用法：symbolize.py <log> <binary>，结果写到 stdout（由 run.sh 追加进主日志）。
"""
import os
import subprocess
import sys

TARGET_CASES = {"crash-null", "crash-deep"}


def field(line, key):
    for part in line.split("|"):
        if part.startswith(key + "="):
            return part[len(key) + 1:]
    return None


def addr2line(binary, addr):
    out = subprocess.run(
        ["addr2line", "-f", "-C", "-e", binary, hex(addr)],
        capture_output=True, text=True, check=False,
    ).stdout.splitlines()
    if len(out) < 2:
        return "?", "?"
    func = out[0].strip()
    loc = out[1].strip()
    if loc not in ("?", "??:0"):
        path, _, line = loc.rpartition(":")
        loc = f"{os.path.basename(path)}:{line}"
    return func, loc


def main():
    if len(sys.argv) != 3:
        print("usage: symbolize.py <log> <binary>", file=sys.stderr)
        return 2
    log_path, binary = sys.argv[1], sys.argv[2]
    try:
        with open(log_path, encoding="utf-8", errors="replace") as f:
            lines = f.read().splitlines()
    except OSError as exc:
        print(f"symbolize: 读不到 {log_path}: {exc}", file=sys.stderr)
        return 2

    case = None
    autos = 0
    for ln in lines:
        if ln.startswith("S06|case|name="):
            case = field(ln, "name")
            continue
        if case not in TARGET_CASES or not ln.startswith("S06|crash|bt|"):
            continue
        mod = field(ln, "mod")
        off = field(ln, "off")
        idx = field(ln, "i")
        is_pc = field(ln, "is_pc") == "1"
        if mod != "self" or off is None:
            continue
        off_val = int(off, 16)
        query = off_val if is_pc else max(off_val - 1, 0)
        func, loc = addr2line(binary, query)
        print(f"S06|crash|sym|case={case}|i={idx}|func={func}|loc={loc}|q=0x{query:x}")

        # 反例：用运行时绝对地址去查符号表
        if is_pc:
            abs_addr = int(field(ln, "addr") or "0", 16)
            afunc, aloc = addr2line(binary, abs_addr)
            print(f"S06|crash|abs|case={case}|i={idx}|func={afunc}|loc={aloc}")
        autos += 1

    if autos == 0:
        print("S06|crash|sym|none=1")
    return 0


if __name__ == "__main__":
    sys.exit(main())
