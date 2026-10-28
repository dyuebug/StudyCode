#!/usr/bin/env bash
# tests/run.sh —— 起服务 → 打负载 → 收尾 → 机器判定
#
# 设计约束：
#   1. 每个用例独立起停一个服务进程，端口用 0 由内核分配，从 SRV|listen 读回，
#      因此用例之间不会撞 EADDRINUSE，也可以并发重跑。
#   2. 所有等待都有上界：服务 5 s 内没 ready 就判失败，进程 10 s 内没退出就 KILL。
#      "判定器挂住"和"用例失败"必须是两种可区分的现象。
#   3. 退出码只用 0/1/2：0 全部通过，1 有判据失败，2 环境或启动问题。
set -u

ROOT=$(cd "$(dirname "$0")/.." && pwd)
# 构建目录可由 BUILD_DIR 覆盖：`make check-asan` 用 build-address 跑同一套用例，
# 不与默认构建混用二进制。
BUILD_DIR=${BUILD_DIR:-build}
SRV="$ROOT/$BUILD_DIR/reactor_srv"
CLI="$ROOT/$BUILD_DIR/loadgen"
LOGDIR="$ROOT/tests/logs"

if [ ! -x "$SRV" ] || [ ! -x "$CLI" ]; then
    echo "缺少可执行文件，请先运行 make" >&2
    exit 2
fi

rm -rf "$LOGDIR"
mkdir -p "$LOGDIR"

SRV_PID=""
PORT=""
SRV_RC=""

start_server() {  # $1 = 用例名，其余 = 服务参数
    local name=$1
    shift
    "$SRV" "$@" > "$LOGDIR/$name.srv.log" 2>&1 &
    SRV_PID=$!
    local i
    for i in $(seq 1 100); do
        grep -q 'SRV|ready' "$LOGDIR/$name.srv.log" 2>/dev/null && break
        sleep 0.05
    done
    PORT=$(grep -o 'SRV|listen|port=[0-9]*' "$LOGDIR/$name.srv.log" | head -1 | cut -d= -f2)
    if [ -z "$PORT" ]; then
        echo "[$name] 服务 5 s 内未就绪" >&2
        SRV_RC="not_ready"
        return 1
    fi
    return 0
}

stop_server() {  # 发 SIGINT 走优雅退出；10 s 不退则 KILL（记 killed，判定器会判失败）
    [ -n "$SRV_PID" ] || { SRV_RC="no_pid"; return; }
    kill -INT "$SRV_PID" 2>/dev/null
    local i
    for i in $(seq 1 200); do
        kill -0 "$SRV_PID" 2>/dev/null || break
        sleep 0.05
    done
    if kill -0 "$SRV_PID" 2>/dev/null; then
        kill -KILL "$SRV_PID" 2>/dev/null
        wait "$SRV_PID" 2>/dev/null
        SRV_RC="killed"
    else
        wait "$SRV_PID" 2>/dev/null
        SRV_RC=$?
    fi
    SRV_PID=""
}

# ---------- 用例 ----------

case_echo() {
    start_server echo --port 0 --idle-ms 0 --threads 4 || return
    timeout 60 "$CLI" --port "$PORT" --clients 8 --frames 20 --size 200 \
        > "$LOGDIR/echo.cli.log" 2>&1
    echo "cli_rc=$?" > "$LOGDIR/echo.meta"
    stop_server
    echo "srv_rc=$SRV_RC" >> "$LOGDIR/echo.meta"
}

# 管道化：先发完全部帧再读，单次灌入量远超 RBUF_CAP（16 帧 × 8000 = 128 KB）。
# 这个用例是 "读缓冲满被误判为 read_err" 那个缺陷的回归判据。
case_pipeline() {
    start_server pipeline --port 0 --idle-ms 0 --threads 4 || return
    timeout 60 "$CLI" --port "$PORT" --clients 8 --frames 50 --size 4000 --pipeline \
        > "$LOGDIR/pipeline.cli.log" 2>&1
    echo "cli_rc=$?" > "$LOGDIR/pipeline.meta"
    stop_server
    echo "srv_rc=$SRV_RC" >> "$LOGDIR/pipeline.meta"
}

case_idle_timeout() {
    start_server idle_timeout --port 0 --idle-ms 500 --threads 2 || return
    timeout 60 "$CLI" --port "$PORT" --clients 4 --frames 1 --size 200 --idle-after-ms 1500 \
        > "$LOGDIR/idle_timeout.cli.log" 2>&1
    echo "cli_rc=$?" > "$LOGDIR/idle_timeout.meta"
    stop_server
    echo "srv_rc=$SRV_RC" >> "$LOGDIR/idle_timeout.meta"
}

case_half_close() {
    start_server half_close --port 0 --idle-ms 0 --threads 2 || return
    timeout 60 "$CLI" --port "$PORT" --clients 4 --frames 10 --size 500 --half-close \
        > "$LOGDIR/half_close.cli.log" 2>&1
    echo "cli_rc=$?" > "$LOGDIR/half_close.meta"
    stop_server
    echo "srv_rc=$SRV_RC" >> "$LOGDIR/half_close.meta"
}

# 背压：客户端先灌满 8 MB 再延迟 400 ms 才开始读，服务端回显必然堆到写缓冲上限。
# 判据不要求"一定断开"，只要求"要么全成功、要么断开且客户端可见失败"——不允许静默丢数据。
case_backpressure() {
    start_server backpressure --port 0 --idle-ms 0 --threads 4 || return
    timeout 90 "$CLI" --port "$PORT" --clients 2 --frames 1000 --size 8000 \
        --pipeline --read-delay-ms 400 \
        > "$LOGDIR/backpressure.cli.log" 2>&1
    echo "cli_rc=$?" > "$LOGDIR/backpressure.meta"
    stop_server
    echo "srv_rc=$SRV_RC" >> "$LOGDIR/backpressure.meta"
}

# 优雅退出：每个 job 20 ms，客户端跑到一半发 SIGINT，
# 此时必然有连接有在飞 job——用来验证 shutdown 不会丢任务、不会留下半开连接。
case_graceful() {
    SRV_JOB_US=20000 start_server graceful --port 0 --idle-ms 0 --threads 2 || return
    ( timeout 60 "$CLI" --port "$PORT" --clients 4 --frames 10 --size 200 \
        > "$LOGDIR/graceful.cli.log" 2>&1
      echo "cli_rc=$?" > "$LOGDIR/graceful.meta" ) &
    local clipid=$!
    sleep 0.15
    stop_server
    wait "$clipid" 2>/dev/null
    echo "srv_rc=$SRV_RC" >> "$LOGDIR/graceful.meta"
}

# dropped：在飞 job 的结果到达时，连接已经被服务端关掉。
#
# 这是"迟到结果"那条分支的唯一确定性入口：空闲超时清扫（sweep_idle）不检查 in_flight，
# 所以只要 job 跑得比空闲门限久，连接必然先关、结果后到。
# 时序：idle=200 ms，job=1000 ms，余量 700 ms；客户端只发 1 帧，
# 因此每个连接恰好产生 1 个必然被丢弃的结果，dropped 可预期等于 jobs。
# 若不记账，这个数会恒为 0，"结果悄悄消失"和"一切正常"就无法区分。
case_dropped() {
    SRV_JOB_US=1000000 start_server dropped --port 0 --idle-ms 200 --threads 2 || return
    timeout 60 "$CLI" --port "$PORT" --clients 2 --frames 1 --size 200 \
        > "$LOGDIR/dropped.cli.log" 2>&1
    echo "cli_rc=$?" > "$LOGDIR/dropped.meta"
    stop_server
    echo "srv_rc=$SRV_RC" >> "$LOGDIR/dropped.meta"
}

# gen_reuse：迟到结果到达时，槽位**已经换了人**。
#
# 与 dropped 的区别：dropped 是"槽位空着"，这里是"槽位被新连接占用"。
# 两者的 dropped 计数完全相同，只有 stale_gen 能区分——若世代号判断缺失，
# 旧连接的载荷会被写进新连接，而 dropped 计数仍然漂亮。
# 时序：A 发 1 帧后半关闭 → 服务端读到 EOF 主动 close（客户端读到 EOF 作为前提证据）
#       → B 立刻连接复用该槽位 → A 的 job（600 ms）结果到达。
# 空闲门限取 5000 ms：B 不需要保活（在飞 job 期间服务端不读 socket，保活无效）。
case_gen_reuse() {
    SRV_JOB_US=600000 start_server gen_reuse --port 0 --idle-ms 5000 --threads 2 || return
    timeout 90 python3 "$ROOT/tests/gen_reuse.py" --port "$PORT" --job-ms 600 \
        > "$LOGDIR/gen_reuse.cli.log" 2>&1
    echo "cli_rc=$?" > "$LOGDIR/gen_reuse.meta"
    stop_server
    echo "srv_rc=$SRV_RC" >> "$LOGDIR/gen_reuse.meta"
}

case_echo
case_pipeline
case_idle_timeout
case_half_close
case_backpressure
case_dropped
case_gen_reuse
case_graceful

python3 "$ROOT/tests/judge.py" "$LOGDIR"
