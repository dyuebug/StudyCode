#!/usr/bin/env bash
# SYS_03 阶段 3：编译 → 三组取证 → 机器判定。
#
# 用法：
#   ./run.sh                              全部跑一遍并判定，全通过时退出码 0
#   ./run.sh tcp                          只跑一组（tcp|udp|mux）
#   ./run.sh --judge <组> <日志>          只判定不跑（0 通过 / 1 失败 / 2 日志不存在）
#
# 网络用例写错时的典型表现是"永远等不到"：
# 每个用例都套 timeout，宁可判失败也不让脚本挂住。
set -uo pipefail

HERE=$(cd "$(dirname "$0")" && pwd)
cd "$HERE" || exit 1
LOGS="$HERE/logs"
mkdir -p "$LOGS"

TIMEOUT_S=60

if [ "${1:-}" = "--judge" ]; then
    if [ $# -lt 3 ]; then
        echo "用法: $0 --judge <tcp|udp|mux> <日志>" >&2
        exit 2
    fi
    python3 judge.py "$2" "$3"
    exit $?
fi

ONLY="${1:-}"

if ! make -s; then
    echo "编译失败" >&2
    exit 1
fi

run_tcp() {
    : > "$LOGS/tcp.log"
    for m in echo halfclose reset sigpipe timeout shortwrite; do
        timeout "$TIMEOUT_S" ./tcp_basic "$m" >> "$LOGS/tcp.log" 2>&1
        echo "tcp/$m rc=$?"
    done
}

run_udp() {
    : > "$LOGS/udp.log"
    for m in boundary trunc toobig; do
        timeout "$TIMEOUT_S" ./udp_basic "$m" >> "$LOGS/udp.log" 2>&1
        echo "udp/$m rc=$?"
    done
}

run_mux() {
    : > "$LOGS/mux.log"
    for m in select poll epoll epoll-et epoll-et-broken; do
        timeout "$TIMEOUT_S" ./mux "$m" >> "$LOGS/mux.log" 2>&1
        echo "mux/$m rc=$?"
    done
}

if [ -z "$ONLY" ] || [ "$ONLY" = "tcp" ]; then run_tcp; fi
if [ -z "$ONLY" ] || [ "$ONLY" = "udp" ]; then run_udp; fi
if [ -z "$ONLY" ] || [ "$ONLY" = "mux" ]; then run_mux; fi

RC=0
want() { [ -z "$ONLY" ] || [ "$ONLY" = "$1" ]; }

if want tcp; then python3 judge.py tcp "$LOGS/tcp.log" || RC=1; fi
if want udp; then python3 judge.py udp "$LOGS/udp.log" || RC=1; fi
if want mux; then python3 judge.py mux "$LOGS/mux.log" || RC=1; fi

if [ "$RC" -eq 0 ]; then
    echo "全部通过"
else
    echo "有失败项" >&2
fi
exit "$RC"
