#!/usr/bin/env bash
# SYS_05 取证脚本：基础用例跑一次进程，服务端用例各起一次服务端 + 客户端
#
# 用法：
#   ./run.sh                 跑全部用例并判定
#   ./run.sh --judge         只判定已有日志（不重跑）
#   ./run.sh gen coro-greedy 只跑指定用例
#
# 端口一律由内核分配（bind 端口 0），日志里 ready 行带 port，客户端从该行取值；
# 每个用例的服务端日志先清空再启动，避免读到上一轮的端口。
set -u

cd "$(dirname "$0")" || exit 1
mkdir -p logs

OUT=${LOG:-logs/coro.log}
TIMEOUT_S=${TIMEOUT_S:-60}
CLIENTS=${CLIENTS:-8}
FRAMES=${FRAMES:-20}
SIZE=${SIZE:-200}

if [ "${1:-}" = "--judge" ]; then
    shift
    exec python3 -I judge.py "$OUT" "$@"
fi

if [ ! -x ./coro_basics ] || [ ! -x ./coro_sched ] || [ ! -x ./client ]; then
    make || exit 1
fi

: > "$OUT"
SRV_PID=

# ---------- 基础用例（coro_basics 自己打印 case 头） ----------
run_basics() { # $1=name
    local name=$1

    timeout "$TIMEOUT_S" ./coro_basics "$name" >> "$OUT" 2>&1
    echo "S05|case|name=$name|rc=$?" >> "$OUT"
}

# ---------- 服务端用例 ----------
start_srv() { # $1=name, 之后是服务端参数
    local name=$1
    shift
    : > "logs/$name.srv.log"
    ./coro_sched "$@" > "logs/$name.srv.log" 2>&1 &
    SRV_PID=$!
    for _ in $(seq 1 300); do
        if grep -q 'S05|srv|ready' "logs/$name.srv.log" 2>/dev/null; then
            return 0
        fi
        if ! kill -0 "$SRV_PID" 2>/dev/null; then
            return 1
        fi
        sleep 0.02
    done
    return 1
}

run_case() { # $1=name $2=client_mode $3=conns $4=frames $5=size $6=extra -- 服务端参数
    local name=$1 cmode=$2 conns=$3 frames=$4 size=$5 extra=$6
    shift 6
    [ "${1:-}" = "--" ] && shift

    if ! start_srv "$name" "$@"; then
        echo "S05|case|name=$name|error=server_not_ready" >> "$OUT"
        SRV_PID=
        return 1
    fi

    local port
    port=$(sed -n 's/.*|port=\([0-9][0-9]*\)|.*/\1/p' "logs/$name.srv.log" | head -n 1)
    if [ -z "$port" ]; then
        echo "S05|case|name=$name|error=no_port" >> "$OUT"
        wait "$SRV_PID" 2>/dev/null
        SRV_PID=
        return 1
    fi

    echo "S05|case|name=$name|port=$port|client=$cmode/$conns/$frames/$size/$extra|srv=$*" >> "$OUT"
    timeout "$TIMEOUT_S" ./client "$port" "$cmode" "$conns" "$frames" "$size" "$extra" \
        > "logs/$name.gen.log" 2>&1
    local rc=$?
    wait "$SRV_PID" 2>/dev/null
    SRV_PID=

    cat "logs/$name.srv.log" >> "$OUT"
    cat "logs/$name.gen.log" >> "$OUT"
    echo "S05|case|name=$name|client_rc=$rc" >> "$OUT"
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

# ===== 基础：Generator / Task / 生命周期 / 就绪队列 =====
want gen && run_basics gen
want task && run_basics task
want except && run_basics except
want life && run_basics life
want resume-twice && run_basics resume-twice
want leak && run_basics leak
want leak-broken && run_basics leak-broken
want sched-order && run_basics sched-order

# ===== 服务端：协程版 echo =====
# 1) 贪心：一次 resume 处理完缓冲里的所有帧 → 同连接连续段可以很长
want coro-greedy && run_case coro-greedy pipeline "$CLIENTS" "$FRAMES" "$SIZE" 0 -- \
    --mode coro --batch 0

# 2) 配额 1 帧：每条连接之间必须轮转 → 同连接连续段恒为 1
want coro-fair && run_case coro-fair pipeline "$CLIENTS" "$FRAMES" "$SIZE" 0 -- \
    --mode coro --batch 1

# 3) 请求-响应：正确性基线（每帧都回、字节一致）
want coro-rr && run_case coro-rr rr "$CLIENTS" "$FRAMES" "$SIZE" 0 -- --mode coro

# 4) 空闲超时：客户端只连不发，服务端 200ms 后 cancel(ETIMEDOUT) 并 close
want coro-cancel && run_case coro-cancel silent 2 1 1 600 -- --mode coro --idle-ms 200

# 5) RST：客户端 SO_LINGER{1,0} 关闭，服务端读到 ECONNRESET
want coro-rst && run_case coro-rst rst 4 5 "$SIZE" 0 -- --mode coro

# 6) 半关闭：客户端 shutdown(SHUT_WR)，服务端读到 EOF
want coro-halfclose && run_case coro-halfclose halfclose 4 5 "$SIZE" 0 -- --mode coro

# 7) 对照：每连接一个阻塞线程
want thread && run_case thread pipeline "$CLIENTS" "$FRAMES" "$SIZE" 0 -- --mode thread

# 8) 静默后才发数据：正常版会等（基线），与 9 只差一个开关
want coro-idle && run_case coro-idle idle-then-send 2 1 "$SIZE" 400 -- --mode coro

# 9) 破坏版：把 EAGAIN 当 EOF → 客户端还没发就已经被挂断
want eagain-as-eof && run_case eagain-as-eof idle-then-send 2 1 "$SIZE" 400 -- \
    --mode coro --eagain-as-eof

# 10) 破坏版：超时后 destroy 但不 detach、不 close → 事件打到已销毁的槽位
#     （存活槽位永不归零，只能靠 --max-run 退出）
want cancel-free && run_case cancel-free idle-then-send 2 1 "$SIZE" 400 -- \
    --mode coro --idle-ms 200 --cancel-free --max-run 4000

exec python3 -I judge.py "$OUT"
