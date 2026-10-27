#!/usr/bin/env bash
# SYS_02 阶段 2：编译 → 六组取证 → 机器判定。
#
# 用法：
#   ./run.sh                                 全部跑一遍并判定，全通过时退出码 0
#   ./run.sh ipc                             只跑一组（ipc|cond|race|deadlock|pool|tsan）
#   ./run.sh --judge <组> <日志>             只判定不跑（0 通过 / 1 失败 / 2 日志不存在）
#
# 每个用例都套 timeout：并发程序写错时宁可失败也不要挂住整个脚本。
set -uo pipefail

HERE=$(cd "$(dirname "$0")" && pwd)
cd "$HERE" || exit 1
LOGS="$HERE/logs"
mkdir -p "$LOGS"

TIMEOUT_S=120

if [ "${1:-}" = "--judge" ]; then
    if [ $# -lt 3 ]; then
        echo "用法: $0 --judge <ipc|cond|race|deadlock|pool|tsan> <日志>" >&2
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

run_ipc() {
    : > "$LOGS/ipc.log"
    for m in pipe fifo shm uds-stream uds-dgram; do
        timeout "$TIMEOUT_S" ./ipc_basic "$m" >> "$LOGS/ipc.log" 2>&1
        echo "ipc/$m rc=$?"
    done
}

run_cond() {
    : > "$LOGS/cond.log"
    for m in spurious-if spurious-while queue; do
        timeout "$TIMEOUT_S" ./condvar "$m" >> "$LOGS/cond.log" 2>&1
        echo "cond/$m rc=$?"
    done
}

run_race() {
    : > "$LOGS/race.log"
    for m in plain mutex atomic rwlock rwlock-broken; do
        timeout "$TIMEOUT_S" ./race_detect "$m" >> "$LOGS/race.log" 2>&1
        echo "race/$m rc=$?"
    done
}

run_deadlock() {
    : > "$LOGS/deadlock.log"
    for m in deadlock ordered; do
        timeout "$TIMEOUT_S" ./deadlock "$m" >> "$LOGS/deadlock.log" 2>&1
        echo "deadlock/$m rc=$?"
    done
}

run_pool() {
    : > "$LOGS/pool.log"
    for m in graceful immediate; do
        timeout "$TIMEOUT_S" ./threadpool "$m" >> "$LOGS/pool.log" 2>&1
        echo "pool/$m rc=$?"
    done
}

# TSan 的原始报告很长，这里只把"每个模式报了几次 data race"落成一行，
# 原始 stderr 仍各存一份，便于人工核对栈帧。
run_tsan() {
    if ! make -s tsan; then
        echo "TSan 编译失败" >&2
        return 1
    fi
    : > "$LOGS/tsan.log"
    for m in plain mutex atomic rwlock rwlock-broken; do
        local raw="$LOGS/tsan_${m}.raw"
        local reports rc

        timeout "$TIMEOUT_S" ./race_detect_tsan "$m" --iters 500 > "$raw" 2>&1
        rc=$?
        reports=$(grep -c "WARNING: ThreadSanitizer: data race" "$raw" || true)
        printf 'TSAN|mode=%s|reports=%s|rc=%s\n' "$m" "$reports" "$rc" >> "$LOGS/tsan.log"
        echo "tsan/$m reports=$reports rc=$rc"
    done
}

if [ -z "$ONLY" ] || [ "$ONLY" = "ipc" ]; then run_ipc; fi
if [ -z "$ONLY" ] || [ "$ONLY" = "cond" ]; then run_cond; fi
if [ -z "$ONLY" ] || [ "$ONLY" = "race" ]; then run_race; fi
if [ -z "$ONLY" ] || [ "$ONLY" = "deadlock" ]; then run_deadlock; fi
if [ -z "$ONLY" ] || [ "$ONLY" = "pool" ]; then run_pool; fi
if [ -z "$ONLY" ] || [ "$ONLY" = "tsan" ]; then run_tsan; fi

RC=0
want() { [ -z "$ONLY" ] || [ "$ONLY" = "$1" ]; }

if want ipc; then python3 judge.py ipc "$LOGS/ipc.log" || RC=1; fi
if want cond; then python3 judge.py cond "$LOGS/cond.log" || RC=1; fi
if want race; then python3 judge.py race "$LOGS/race.log" || RC=1; fi
if want deadlock; then python3 judge.py deadlock "$LOGS/deadlock.log" || RC=1; fi
if want pool; then python3 judge.py pool "$LOGS/pool.log" || RC=1; fi
if want tsan; then python3 judge.py tsan "$LOGS/tsan.log" || RC=1; fi

if [ "$RC" -eq 0 ]; then
    echo "全部通过"
else
    echo "有失败项" >&2
fi
exit "$RC"
