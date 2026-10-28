#!/usr/bin/env bash
# tests/bench.sh —— 吞吐观测（不是"吞吐基线"，理由见 benchmarks 文档）
#
# 与 tests/run.sh 的分工：
#   run.sh   判"对不对"——每个用例独立起停，判据失败即非零退出；
#   bench.sh 测"多快"——同一构建、固定 CPU 亲和性、多次运行取分位，
#            只报读数，不设性能门槛（本机没有可比的基线）。
#
# 固定亲和性是为了排除调度抖动。**注意本机是 SMT**：
# /sys/devices/system/cpu/cpu{0,2}/topology/thread_siblings_list 分别是 0-1 与 2-3，
# 即 cpu0 和 cpu1 是同一个物理核的两个超线程，不是两个核。
# 所以默认取"服务端 = 一个物理核（0-1）、客户端 = 另一个物理核的一个线程（4）"，
# 而不是看起来更顺眼的 0/1 配对。**这只降低方差，不产生"跨机器可比"的结论**。
#
# 输出：
#   tests/bench_logs/<TAG>/raw.tsv   每次运行的原始读数（入库前可复核）
#   标准输出                         Markdown 表格（粘进 benchmarks/ 文档）
#
# 为什么要与 tests/logs 分开：tests/run.sh 每次 `make check` 都会 `rm -rf tests/logs`，
# 首版把 bench 日志放进去，结果"再跑一次 check 就把性能证据清空了"。
# tests/bench_logs 只有 `make clean` 会删；按 TAG 分子目录，多组对照可以并存。
#
# 退出码：0 = 所有运行功能正确；1 = 有运行出现 mismatch / 遗留 job / 无背压依据的失败。
set -u

ROOT=$(cd "$(dirname "$0")/.." && pwd)
BUILD_DIR=${BUILD_DIR:-build}
SRV="$ROOT/$BUILD_DIR/reactor_srv"
CLI="$ROOT/$BUILD_DIR/loadgen"
LOGDIR="$ROOT/tests/bench_logs/${TAG:-default}"

CPU_SRV=${CPU_SRV:-0-1}
CPU_CLI=${CPU_CLI:-4}
RUNS=${RUNS:-5}
WARMUP=${WARMUP:-1}
THREADS=${THREADS:-4}

if [ ! -x "$SRV" ] || [ ! -x "$CLI" ]; then
    echo "缺少可执行文件，请先运行 make" >&2
    exit 2
fi
if ! command -v taskset >/dev/null 2>&1; then
    echo "缺少 taskset，无法固定亲和性（本脚本拒绝在不固定的情况下出数）" >&2
    exit 2
fi

rm -rf "$LOGDIR"
mkdir -p "$LOGDIR"
: > "$LOGDIR/raw.tsv"

SRV_PID=""
PORT=""
bad=0

start_server() {  # $1 = 日志前缀（<name>.run<r>）
    taskset -c "$CPU_SRV" "$SRV" --port 0 --idle-ms 0 --threads "$THREADS" \
        > "$LOGDIR/$1.srv.log" 2>&1 &
    SRV_PID=$!
    local i
    for i in $(seq 1 100); do
        grep -q 'SRV|ready' "$LOGDIR/$1.srv.log" 2>/dev/null && break
        sleep 0.05
    done
    PORT=$(grep -o 'SRV|listen|port=[0-9]*' "$LOGDIR/$1.srv.log" | head -1 | cut -d= -f2)
    if [ -z "$PORT" ]; then
        echo "[$1] 服务 5 s 内未就绪" >&2
        bad=1
        return 1
    fi
    return 0
}

stop_server() {
    [ -n "$SRV_PID" ] || return
    kill -INT "$SRV_PID" 2>/dev/null
    local i
    for i in $(seq 1 200); do
        kill -0 "$SRV_PID" 2>/dev/null || break
        sleep 0.05
    done
    if kill -0 "$SRV_PID" 2>/dev/null; then
        kill -KILL "$SRV_PID" 2>/dev/null
        wait "$SRV_PID" 2>/dev/null
    else
        wait "$SRV_PID" 2>/dev/null
    fi
    SRV_PID=""
}

# 一个配置跑 WARMUP + RUNS 次；每次独立起停服务端（不共享连接状态）。
# 参数：标签 客户端数 帧数 载荷字节 附加参数（可为空）
run_config() {
    local name=$1 clients=$2 frames=$3 size=$4 extra=${5:-}
    local total_times=0
    local r tag
    for ((r = 1; r <= WARMUP + RUNS; r++)); do
        # 每轮独立日志：首版用 <name>.srv.log，失败那轮的证据会被下一轮覆盖——
        # 实测中 THREADS=1 的 pipeline 第 1 轮就断连，服务端日志当时已经没了。
        tag="$name.run$r"
        start_server "$tag" || return
        # shellcheck disable=SC2086
        taskset -c "$CPU_CLI" "$CLI" --port "$PORT" --clients "$clients" \
            --frames "$frames" --size "$size" $extra \
            > "$LOGDIR/$tag.cli.log" 2>&1
        local cli_rc=$?
        stop_server

        local done_line srv_line
        done_line=$(grep -h 'CLI|done' "$LOGDIR/$tag.cli.log" | head -1)
        srv_line=$(grep -h 'SRV|summary' "$LOGDIR/$tag.srv.log" | head -1)
        local ms fail mismatch jobs leftover wbuf
        ms=$(printf '%s\n' "$done_line" | grep -o 'ms=[0-9]*' | cut -d= -f2)
        fail=$(printf '%s\n' "$done_line" | grep -o 'fail=[0-9]*' | cut -d= -f2)
        mismatch=$(printf '%s\n' "$done_line" | grep -o 'mismatch=[0-9]*' | cut -d= -f2)
        # 必须用 '|jobs=' 锚定：summary 里还有 leftover_jobs=，
        # 裸 'jobs=[0-9]*' 会同时匹配到两处，得出 "4000\n0"，比较直接报 integer expected。
        jobs=$(printf '%s\n' "$srv_line" | grep -o '|jobs=[0-9]*' | cut -d= -f2)
        leftover=$(printf '%s\n' "$srv_line" | grep -o 'leftover_jobs=[0-9]*' | cut -d= -f2)
        wbuf=$(grep -c 'reason=wbuf_full' "$LOGDIR/$tag.srv.log")
        local expect=$((clients * frames))

        # 数据完整性判据沿用 judge.py 的口径：mismatch 必须为 0（回显逐字节正确），
        # leftover_jobs 必须为 0（退出时没有遗留）。
        # 失败数则分两种合法情况：
        #   fail==0 —— 全部帧走完，jobs 必须等于 clients×frames；
        #   fail>0  —— 只有服务端确实打过 wbuf_full 才算"背压断连"这一设计内结果
        #              （README §5.4），否则就是真缺陷。
        # 这条规则不是为 bench 放宽的：judge.py 的 C11 就是 (fail>0)==(wbuf_full>0)。
        local why=""
        if [ "${mismatch:-1}" -ne 0 ]; then why="mismatch=${mismatch:-?}"; fi
        if [ "${leftover:-1}" -ne 0 ]; then why="$why leftover=${leftover:-?}"; fi
        if [ "${fail:-0}" -eq 0 ]; then
            [ "${jobs:-0}" -ne "$expect" ] && why="$why jobs=${jobs:-?}≠$expect"
        else
            [ "$wbuf" -le 0 ] && why="$why fail=${fail:-?} 但无 wbuf_full"
        fi
        if [ "$cli_rc" -ne 0 ] && [ "${fail:-0}" -eq 0 ]; then
            why="$why rc=$cli_rc 但 fail=0"
        fi
        if [ -n "$why" ]; then
            echo "[$tag] 功能不自洽:$why" >&2
            bad=1
        fi
        if [ "$r" -le "$WARMUP" ]; then
            continue
        fi
        printf '%s\t%d\t%d\t%d\t%s\t%s\t%s\t%s\t%s\n' \
            "$name" "$clients" "$frames" "$size" "${ms:-0}" "${jobs:-0}" \
            "${fail:-?}" "${mismatch:-?}" "${wbuf:-?}" \
            >> "$LOGDIR/raw.tsv"
        total_times=$((total_times + 1))
    done
    if [ "$total_times" -eq 0 ]; then
        echo "[$name] 没有有效运行" >&2
        bad=1
    fi
}

# 三个形态的目的不同：
#   roundtrip  —— 单连接同时只有一个在飞 job，吞吐受 RTT 与 job 调度限制；
#   pipeline   —— 客户端先灌满再读，走的是服务端"读→解析→排队"的主路径；
#   single-conn—— 只有一条连接，用于观察单连接串行化的上限。
#
# 负载量取"单次运行 ≥ 200 ms"：跑 3 轮总耗时仍在 4 s 量级，但每次运行的
# 方差从 16%（63 ms 级的 pipeline）降到个位数百分比，中位数才有意义。
run_config roundtrip   8 2000 256  ""
run_config pipeline    8 1000 4096 "--pipeline"
run_config single-conn 1 8000 256  ""

if [ "$bad" -ne 0 ]; then
    echo "存在功能不自洽的运行，见 ${LOGDIR#$ROOT/} 下的日志" >&2
    exit 1
fi

echo
echo "| 形态 | 连接×帧×字节 | 运行数 | 中位 ms | 最小–最大 ms | fail 合计 | wbuf_full 合计 | 中位 帧/s | 中位 MiB/s |"
echo "| --- | --- | --- | --- | --- | --- | --- | --- | --- |"
awk -F'\t' '
{
    key = $1
    n[key]++
    ms[key, n[key]] = $5
    cfg[key] = $2 "×" $3 "×" $4
    tfail[key] += $7
    twbuf[key] += $9
}
END {
    order[1] = "roundtrip"; order[2] = "pipeline"; order[3] = "single-conn"
    for (k = 1; k <= 3; k++) {
        name = order[k]
        if (n[name] == 0) continue
        # 插入排序（运行数很小）
        for (i = 1; i <= n[name]; i++) {
            v = ms[name, i]
            j = i - 1
            while (j >= 1 && ms[name, j] > v) {
                ms[name, j + 1] = ms[name, j]
                j--
            }
            ms[name, j + 1] = v
        }
        mid = int((n[name] + 1) / 2)
        med = ms[name, mid]
        lo = ms[name, 1]
        hi = ms[name, n[name]]
        split(cfg[name], p, "×")
        frames = p[1] * p[2]
        size = p[3]
        fps = (med > 0) ? frames * 1000.0 / med : 0
        mib = (med > 0) ? frames * size / 1048576.0 * 1000.0 / med : 0
        printf "| %s | %s | %d | %.1f | %.1f–%.1f | %d | %d | %.0f | %.2f |\n",
               name, cfg[name], n[name], med, lo, hi, tfail[name], twbuf[name], fps, mib
    }
}' "$LOGDIR/raw.tsv"

echo
echo "原始读数：${LOGDIR#$ROOT/}/raw.tsv（每行一次运行；预热 ${WARMUP} 次未计入）"
echo "口径：客户端自报 ms（含建连与线程创建），帧数 = 连接数×帧数，MiB 只算 payload，不含 2 B 长度头。"
echo "fail / wbuf_full 非 0 的轮次是服务端按背压策略主动断连（README §5.4），该轮时长不可与全成功轮并排比较。"
