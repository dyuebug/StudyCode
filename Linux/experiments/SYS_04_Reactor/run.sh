#!/usr/bin/env bash
# SYS_04 取证脚本：每个用例 = 起一个服务端 + 跑一次压测客户端 + 收一份日志
#
# 用法：
#   ./run.sh              跑全部用例并判定
#   ./run.sh --judge      只判定已有日志（不重跑）
#   ./run.sh single pool4 只跑指定用例
#
# 端口一律由内核分配（bind 端口 0），日志里第一行 ready 带 port，
# 客户端从日志里取值——因此用例可以并发、可以重跑而不会撞 EADDRINUSE。
set -u

cd "$(dirname "$0")" || exit 1
mkdir -p logs

OUT=${LOG:-logs/reactor.log}
TIMEOUT_S=${TIMEOUT_S:-60}
CLIENTS=${CLIENTS:-8}
MSGS=${MSGS:-20}
SIZE=${SIZE:-200}
DELAY=${DELAY:-2000}

if [ "${1:-}" = "--judge" ]; then
    shift
    exec python3 -I judge.py "$OUT" "$@"
fi

if [ ! -x ./reactor_srv ] || [ ! -x ./loadgen ]; then
    make || exit 1
fi

: > "$OUT"
SRV_PID=

start_srv() { # $1=name, 之后是服务端参数
    local name=$1
    shift
    : > "logs/$name.srv.log"
    ./reactor_srv "$@" > "logs/$name.srv.log" 2>&1 &
    SRV_PID=$!
    for _ in $(seq 1 300); do
        if grep -q 'SYS04|srv|ready' "logs/$name.srv.log" 2>/dev/null; then
            return 0
        fi
        if ! kill -0 "$SRV_PID" 2>/dev/null; then
            return 1
        fi
        sleep 0.02
    done
    return 1
}

run_case() { # $1=name $2=gap_ms 之后是服务端参数
    local name=$1 gap=$2
    shift 2

    if ! start_srv "$name" "$@"; then
        echo "SYS04|case|name=$name|error=server_not_ready" >> "$OUT"
        return 1
    fi

    local port
    port=$(sed -n 's/.*|port=\([0-9][0-9]*\)|.*/\1/p' "logs/$name.srv.log" | head -n 1)
    if [ -z "$port" ]; then
        echo "SYS04|case|name=$name|error=no_port" >> "$OUT"
        wait "$SRV_PID" 2>/dev/null
        return 1
    fi

    echo "SYS04|case|name=$name|port=$port|args=$*" >> "$OUT"
    timeout "$TIMEOUT_S" ./loadgen "$port" "$CLIENTS" "$MSGS" "$SIZE" 2000 "$gap" \
        > "logs/$name.gen.log" 2>&1
    local rc=$?
    wait "$SRV_PID" 2>/dev/null
    SRV_PID=

    cat "logs/$name.srv.log" >> "$OUT"
    cat "logs/$name.gen.log" >> "$OUT"
    echo "SYS04|case|name=$name|gen_rc=$rc" >> "$OUT"
}

want() {
    if [ -z "$ONLY" ]; then
        return 0 # 没给用例名：全跑
    fi
    for w in "$@"; do
        [ "$w" = "$ONLY" ] && return 0
    done
    return 1
}

ONLY=${1:-}
[ $# -gt 0 ] && shift

# 1) 单 Reactor 单线程：业务耗时全压在事件循环里 → 8×20×2ms 串行
want single && run_case single 0 --mode single --delay-us "$DELAY"

# 2) Reactor + 线程池：同样的业务耗时移到 4 个 worker
want pool4 && run_case pool4 0 --mode pool --delay-us "$DELAY" --workers 4

# 3) 主从 Reactor：main 只 accept，两个 sub 各带一个 epoll
want master2 && run_case master2 0 --mode master --delay-us "$DELAY" --subs 2

# 4) 正确 ET：读到 EAGAIN
want et && run_case et 0 --mode single --et --delay-us "$DELAY"

# 5) EPOLLONESHOT：每次事件后必须重挂（gap 让"一次事件 = 一次到达"成立）
want oneshot && run_case oneshot 2 --mode single --oneshot --delay-us 500

# 6) 破坏版：ET 只读一次 → 剩余字节不再产生事件
#    （停住的连接连 FIN 都收不到，服务端只能靠 max-run 退出，所以调小它）
want et-broken && run_case et-broken 0 --mode single --et-broken --max-run 6000

# 7) 破坏版：ONESHOT 不重挂 → 第一帧之后连接静止（客户端必须等到超时）
want oneshot-broken && run_case oneshot-broken 5 --mode single --oneshot-broken --max-run 6000

exec python3 -I judge.py "$OUT"
