#!/usr/bin/env bash
# 复现尝试：`SIGINT` 撞上"在飞 job"时，服务能否在时限内优雅退出。
#
# 背景（README §6.2）：旧版代码上出现过 `kill -INT` 后进程 600 s 不退出。
# 改掉读缓冲缺陷后未再复现，但**根因未定位**——"未复现 ≠ 已修复"。
# 这个脚本把"未复现"从一次手测变成可重复的尝试：每轮都在 job 平均 20 ms、
# 4 条连接满载时发信号，退出时限 10 s，超时就抓 /proc 现场再 SIGKILL。
#
# 用法：
#   ./sigint_stress.sh [轮数]        默认 20 轮，退出码 0 = 全部在时限内退出
#   SIG=TERM ./sigint_stress.sh 10   改测 SIGTERM
#   JOB_US=200000 SIG_DELAY_MS=50 ./sigint_stress.sh 8
#                                    让每 job 200 ms、信号在 50 ms 时落下，
#                                    逼出"在飞 job 排空"那条路径（默认 20 ms 时
#                                    多数 job 已跑完，实测 6–8 ms 就退出了）
#
# 产出：tests/logs/sigint_stress.log（只有 JSON 式的单行记录，便于 grep）
#
# 为什么不用 `timeout` 包住服务端：`timeout` 自己会给子进程发信号，
# 那正好污染被测对象——这里要的是"我们发的信号"与"进程何时退"两件事分开观测。
set -uo pipefail

HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/.." && pwd)
BUILD="$ROOT/${BUILD_DIR:-build}"
LOG_DIR="$HERE/logs"
LOG="$LOG_DIR/sigint_stress.log"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

ROUNDS=${1:-20}
SIG=${SIG:-INT}
JOB_US=${JOB_US:-20000}
SIG_DELAY_MS=${SIG_DELAY_MS:-200}
READY_TIMEOUT_MS=5000
EXIT_TIMEOUT_MS=10000

[ -x "$BUILD/reactor_srv" ] || { echo "先 make：$BUILD/reactor_srv 不存在" >&2; exit 2; }
[ -x "$BUILD/loadgen" ] || { echo "先 make：$BUILD/loadgen 不存在" >&2; exit 2; }
mkdir -p "$LOG_DIR"
# 追加而不是截断：同一份日志要能同时容纳"默认 20 ms"与"200 ms 在飞"两种配置，
# 否则跑完第二种就把第一种的证据覆盖掉了。末尾只回显本次新增的行，
# 否则 `make stress` 跑第二遍会把第一遍的记录再打一次，读数看起来像重复执行。
skip=$(wc -l < "$LOG" 2>/dev/null || echo 0)
echo "RUN|start=$(date -Is)|rounds=$ROUNDS|signal=$SIG|job_us=$JOB_US|sig_delay_ms=$SIG_DELAY_MS" >> "$LOG"

# 毫秒时间戳。不用 `date +%s%3N`：本机 GNU date 的 `%3N` 会输出完整纳秒
# （实测 19 位数字），把它当毫秒相加会让"等到 5 s"立刻超时——首版就这么空转了 10 轮。
now_ms() { echo $(( $(date +%s%N) / 1000000 )); }

# 等文件里出现 SRV|listen|port=，最多 READY_TIMEOUT_MS
wait_port() {
    local file="$1" deadline=$(( $(now_ms) + READY_TIMEOUT_MS ))
    while [ "$(now_ms)" -lt "$deadline" ]; do
        local p
        p=$(sed -n 's/^SRV|listen|port=\([0-9][0-9]*\).*/\1/p' "$file" | head -1)
        if [ -n "$p" ]; then printf '%s' "$p"; return 0; fi
        sleep 0.01
    done
    return 1
}

# 抓进程现场：每个线程的 state / wchan / syscall。
# wchan 是"睡在哪个内核函数"，syscall 是"正在阻塞于哪个系统调用"——
# 有 gdb 时用 bt 更好；本机没有 gdb，这两个文件是唯一能拿到的栈替代品。
# 服务端在退出路径的五个阶段各打一条 SRV|shutdown|phase=，
# 把它们一并抓进来，就能把"卡住"定位到具体阶段，而不只是"进程还在"。
dump_hung() {
    local pid="$1" round="$2" srvlog="${3:-}"
    echo "HUNG|round=$round|pid=$pid|threads=$(ls -1 "/proc/$pid/task" 2>/dev/null | wc -l)" >> "$LOG"
    if [ -n "$srvlog" ] && [ -f "$srvlog" ]; then
        local phases
        phases=$(grep '^SRV|shutdown|' "$srvlog" | tr '\n' ';')
        echo "HUNG|round=$round|shutdown_phases=${phases:-none}" >> "$LOG"
    fi
    local tid
    for tid in /proc/"$pid"/task/*; do
        [ -d "$tid" ] || continue
        local id state wchan syscall comm
        id=$(basename "$tid")
        comm=$(cat "$tid/comm" 2>/dev/null || echo '?')
        state=$(sed -n 's/^[0-9]* (.*) \([A-Z]\).*/\1/p' "$tid/stat" 2>/dev/null | head -1)
        wchan=$(cat "$tid/wchan" 2>/dev/null || echo '?')
        syscall=$(cut -d' ' -f1-3 "$tid/syscall" 2>/dev/null || echo '?')
        echo "HUNG|round=$round|tid=$id|comm=$comm|state=${state:-?}|wchan=${wchan:-?}|syscall=${syscall:-?}" >> "$LOG"
    done
}

ok=0
hung=0
silent=0
failed_start=0
round=1
while [ "$round" -le "$ROUNDS" ]; do
    srv_log="$TMP/srv.$round.log"
    SRV_JOB_US=$JOB_US "$BUILD/reactor_srv" --port 0 --threads 4 --idle-ms 10000 \
        > "$srv_log" 2>&1 &
    pid=$!

    if ! port=$(wait_port "$srv_log"); then
        echo "ROUND|$round|verdict=no_listen|pid=$pid" >> "$LOG"
        kill -9 "$pid" 2>/dev/null
        wait "$pid" 2>/dev/null
        failed_start=$((failed_start + 1))
        round=$((round + 1))
        continue
    fi

    # 客户端满载：先发完再统一读回，让服务端同时在写缓冲上忙，
    # 这样信号落下时主线程与 worker 都在活动路径上，而不是空等。
    "$BUILD/loadgen" --port "$port" --clients 4 --frames 40 --size 200 --pipeline \
        > "$TMP/cli.$round.log" 2>&1 &
    cpid=$!

    sleep "$(awk -v ms="$SIG_DELAY_MS" 'BEGIN{printf "%.3f", ms/1000}')"   # 让首轮 job 真正在飞
    kill -s "$SIG" "$pid" 2>/dev/null
    t0=$(now_ms)

    exited=0
    while [ $(( $(now_ms) - t0 )) -lt "$EXIT_TIMEOUT_MS" ]; do
        if ! kill -0 "$pid" 2>/dev/null; then exited=1; break; fi
        sleep 0.01
    done

    elapsed=$(( $(now_ms) - t0 ))
    if [ "$exited" -eq 1 ]; then
        wait "$pid" 2>/dev/null
        rc=$?
        summary=$(grep -m1 '^SRV|summary|' "$srv_log" || true)
        # 退出快不等于退得对：2026-10-27 实测发现"排空完成队列"漏判会让
        # 在飞 job 的回显静默消失（tx=0、dropped=0、leftover_jobs=0）。
        # 这里用最有辨识度的一条不变量把它变成可判：真有 job 完成，就必然有字节写回。
        jobs=$(printf '%s' "$summary" | sed -n 's/.*|jobs=\([0-9][0-9]*\)|.*/\1/p')
        tx=$(printf '%s' "$summary" | sed -n 's/.*|tx=\([0-9][0-9]*\)|.*/\1/p')
        verdict=ok
        if [ -n "${jobs:-}" ] && [ "${jobs:-0}" -gt 0 ] && [ "${tx:-0}" -eq 0 ]; then
            verdict=silent_drop
            silent=$((silent + 1))
        else
            ok=$((ok + 1))
        fi
        echo "ROUND|$round|signal=$SIG|verdict=$verdict|exit_rc=$rc|elapsed_ms=$elapsed|$summary" >> "$LOG"
    else
        dump_hung "$pid" "$round" "$srv_log"
        kill -9 "$pid" 2>/dev/null
        wait "$pid" 2>/dev/null
        echo "ROUND|$round|signal=$SIG|verdict=hung|elapsed_ms=$elapsed|limit_ms=$EXIT_TIMEOUT_MS" >> "$LOG"
        hung=$((hung + 1))
    fi

    kill -9 "$cpid" 2>/dev/null
    wait "$cpid" 2>/dev/null
    round=$((round + 1))
done

echo "SUMMARY|rounds=$ROUNDS|signal=$SIG|job_us=$JOB_US|sig_delay_ms=$SIG_DELAY_MS|ok=$ok|silent_drop=$silent|hung=$hung|no_listen=$failed_start" >> "$LOG"
tail -n +$((skip + 1)) "$LOG"
[ "$hung" -eq 0 ] && [ "$failed_start" -eq 0 ] && [ "$silent" -eq 0 ]
