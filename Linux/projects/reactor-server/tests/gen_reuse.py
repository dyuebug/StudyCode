#!/usr/bin/env python3
"""gen_reuse.py —— 「槽位复用 + 迟到结果」分支的确定性客户端。

背景：`dropped` 用例只覆盖"结果到达时连接已关闭"。世代号真正要防的是
**同一个槽位换了人**：旧 job 的结果若按槽位投递，就会打进新连接。
这条分支此前只有设计、没有用例（README §8 如实记作"仅设计未验证"）。

时序（每一步都由可观测事件推进，不靠 sleep 猜）：

  1. 连接 A 发 1 帧 400 B，然后 `shutdown(SHUT_WR)` 半关闭。
     服务端读到 0 会**主动 close**，A 因此读到 EOF——这就是"槽位已释放"的证据。
  2. A 的 job 仍在跑（`--job-ms`，默认 600 ms），结果稍后才到。
  3. 立刻连接 B：唯一的空闲槽位就是 A 用过的那个，服务端给它新世代号
     （id 高 32 位相同、低 32 位递增）。
  4. B 发 1 帧 8 B，然后只读。空闲门限设得远大于 job（5000 ms），
     所以 B 不需要保活帧。
  5. A 的迟到结果到达时，槽位里是 B —— 必须按世代丢弃（`stale_gen`），
     且 B **不能**收到 A 的 0x41 载荷；B 自己的 0x42 回显应当正常返回。

为什么不用保活帧：服务端在一个 job 在飞期间**不再读该连接的 socket**，
保活数据会留在内核缓冲里、`last_active` 不刷新，连接照样被空闲清扫关掉
（首版就是这么失败的：B 只送出 1 帧就被判 idle_timeout）。
半关闭 + 大空闲门限可以完全绕开这个耦合。
"""

import argparse
import socket
import sys
import time

PAYLOAD_A = b"A" * 400
PAYLOAD_B = b"B" * 8


def send_frame(sock, payload):
    sock.sendall(len(payload).to_bytes(2, "big") + payload)


def wait_eof(sock, deadline):
    """等服务端 close；返回 1 表示真的读到了 EOF。"""
    while time.monotonic() < deadline:
        try:
            if sock.recv(4096) == b"":
                return 1
        except socket.timeout:
            continue
        except OSError:
            return 0
    return 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", type=int, required=True)
    ap.add_argument("--job-ms", type=int, default=600)
    args = ap.parse_args()

    t_start = time.monotonic()

    # ---- A：发一帧后半关闭，等服务端主动 close ----
    a = socket.create_connection(("127.0.0.1", args.port), timeout=5)
    a.settimeout(0.05)
    send_frame(a, PAYLOAD_A)
    a.shutdown(socket.SHUT_WR)
    eof_a = wait_eof(a, time.monotonic() + 2.0)
    a.close()

    # ---- B：紧接着连接，应当复用 A 的槽位 ----
    b = socket.create_connection(("127.0.0.1", args.port), timeout=5)
    b.settimeout(0.05)
    send_frame(b, PAYLOAD_B)

    got = bytearray()
    # 覆盖 A 的结果（自 A 发送起 job_ms 后到达）与 B 自己的回显，各留余量。
    end = time.monotonic() + args.job_ms / 1000.0 + 0.6
    while time.monotonic() < end:
        try:
            d = b.recv(4096)
        except socket.timeout:
            continue
        except OSError:
            break
        if d == b"":
            break
        got += d
    b.close()

    stale_received = 1 if PAYLOAD_A[:8] in got else 0
    echo_b = got.count(PAYLOAD_B)
    ms = int((time.monotonic() - t_start) * 1000)
    print("CLI|done|case=gen_reuse|eof_a=%d|stale_received=%d|echo_b=%d|bytes=%d|ms=%d"
          % (eof_a, stale_received, echo_b, len(got), ms))
    return 0


if __name__ == "__main__":
    sys.exit(main())
