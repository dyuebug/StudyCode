#!/usr/bin/env bash
# 阶段 6 · 调试、性能与工程化：24 个用例
#
#   ./run.sh              构建 + 跑全部用例 + 判定
#   ./run.sh --judge      只判定已有 logs/s06.log
#   ./run.sh --negtest    只做判定器的负向验证（需先有一份通过的日志）
#   ./run.sh <case-name>  只跑一个用例（不判定）
#
# 每个用例的 stderr 单独落在 logs/<case>.err（原始证据），
# 主日志 logs/s06.log 只保留一行可判定的读数。
set -u
cd "$(dirname "$0")" || exit 2

LOG=logs/s06.log
TMO=180
mkdir -p logs

field() { printf '%s\n' "$1" | sed -n "s/^.*|$2=\([^|]*\).*$/\1/p"; }

env_probe() {
        for t in strace ltrace gdb valgrind perf cmake make addr2line nm objdump readelf python3; do
                if command -v "$t" >/dev/null 2>&1; then p=1; else p=0; fi
                echo "S06|env|tool=$t|present=$p" >> "$LOG"
        done
        echo "S06|env|kernel=$(uname -r)|nproc=$(nproc)|gcc=$(gcc -dumpfullversion)" >> "$LOG"
        echo "S06|env|paranoid=$(cat /proc/sys/kernel/perf_event_paranoid 2>/dev/null || echo n/a)" >> "$LOG"
}

run_case() {
        local name=$1; shift
        echo "S06|case|name=$name|begin" >> "$LOG"
        : > "logs/$name.err"
        timeout "$TMO" "$@" >> "$LOG" 2>> "logs/$name.err"
        local rc=$?
        echo "S06|case|name=$name|rc=$rc" >> "$LOG"
        return 0
}

# 三个 sanitizer 的报表格式各不相同，这里把"工具说了什么"抽成一行，
# 注意抽取的是工具输出里的原话，不是我们注入时用的名字（否则就是自证）。
sani_case() {
        local san=$1 mode=$2 name="$1-$2"
        run_case "$name" "./build/sani_$san" "$mode"
        local rc kind=none msg=none race=0
        rc=$(field "$(grep -o "|name=$name|rc=[0-9]*" "$LOG" | tail -1)" rc)
        case $san in
        asan)
                kind=$(grep -m1 -oE "ERROR: AddressSanitizer: [a-z-]+" "logs/$name.err" 2>/dev/null | sed 's/.*: //')
                if [ -z "$kind" ] && grep -q "ERROR: LeakSanitizer: detected memory leaks" "logs/$name.err" 2>/dev/null; then
                        kind=leak
                fi
                kind=${kind:-none}
                ;;
        ubsan)
                msg=$(grep -m1 -oE "runtime error: [a-z0-9 '\"-]+" "logs/$name.err" 2>/dev/null \
                      | sed 's/runtime error: //' | cut -c1-48)
                msg=${msg:-none}
                ;;
        tsan)
                race=$(grep -c "WARNING: ThreadSanitizer: data race" "logs/$name.err" 2>/dev/null || true)
                ;;
        esac
        echo "S06|sani|case=$name|rc=${rc:-?}|kind=$kind|msg=$msg|race=$race" >> "$LOG"
}

main() {
        : > "$LOG"

        if ! make -s > logs/build.log 2>&1; then
                echo "S06|build|rc=1|log=logs/build.log" >> "$LOG"
                echo "构建失败，见 logs/build.log" >&2
                exit 2
        fi
        echo "S06|build|rc=0|log=logs/build.log" >> "$LOG"
        make -s check >> "$LOG" 2>&1 || echo "S06|build|sanitizer|linked=0" >> "$LOG"

        env_probe

        # ---- 1. ptrace 版 strace：fd 收支 + vDSO 盲区 ----
        run_case trace-fdok   ./build/mini_trace ./build/victim fdok
        run_case trace-fdleak ./build/mini_trace ./build/victim fdleak
        run_case trace-vdso   ./build/mini_trace ./build/victim vdso

        # ---- 2. 崩溃回溯：SIGSEGV -> backtrace -> addr2line ----
        run_case crash-null ./build/crash null
        run_case crash-deep ./build/crash deep
        run_case crash-ok   ./build/crash ok
        python3 -I symbolize.py "$LOG" ./build/crash >> "$LOG" 2>> logs/symbolize.err

        # ---- 3. 三组 sanitizer ----
        sani_case asan  ok
        sani_case asan  heap-overflow
        sani_case asan  uaf
        sani_case asan  stack-overflow
        sani_case asan  leak
        sani_case ubsan ok
        sani_case ubsan shift
        sani_case ubsan ovf
        sani_case ubsan div
        sani_case ubsan null
        sani_case tsan  race
        sani_case tsan  atomic
        sani_case tsan  mutex

        # ---- 4. 没有 perf 时的基准 ----
        run_case bench-vdso       ./build/bench vdso
        run_case bench-syscall    ./build/bench syscall
        run_case bench-pagefault  ./build/bench pagefault
        run_case bench-ctxswitch  ./build/bench ctxswitch

        # ---- 5. perf_event_open 的权限边界 ----
        run_case perf ./build/perf_probe
}

build_only() {
        : > "$LOG"
        if make -s > logs/build.log 2>&1; then
                echo "S06|build|rc=0|log=logs/build.log" >> "$LOG"
        else
                echo "S06|build|rc=1|log=logs/build.log" >> "$LOG"
                exit 2
        fi
        make -s check >> "$LOG" 2>&1 || echo "S06|build|sanitizer|linked=0" >> "$LOG"
}

case "${1:-}" in
--judge)
        python3 -I judge.py "$LOG"
        exit $?
        ;;
--negtest)
        python3 -I negtest.py
        exit $?
        ;;
"")
        main
        python3 -I judge.py "$LOG"
        exit $?
        ;;
*)
        build_only
        name=$1; shift
        run_case "$name" "$@"
        echo "已运行 $name，日志见 $LOG"
        ;;
esac
