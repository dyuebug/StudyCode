#!/usr/bin/env bash
# SYS_01 阶段 1：编译 → 四组取证 → 机器判定。
#
# 用法：
#   ./run.sh                          全部跑一遍并判定，全通过时退出码 0
#   ./run.sh fileio                   只跑一组
#   ./run.sh --judge [<组> <日志> [stdout]]  只判定不跑；不带组名时判定 logs/ 下全部已有日志
set -uo pipefail

HERE=$(cd "$(dirname "$0")" && pwd)
cd "$HERE" || exit 1
LOGS="$HERE/logs"
mkdir -p "$LOGS"

judge_case() {
    local case="$1"
    local log="$2"
    local extra="${3:-}"

    if [ -n "$extra" ]; then
        python3 judge.py "$case" "$log" "$extra"
    else
        python3 judge.py "$case" "$log"
    fi
}

if [ "${1:-}" = "--judge" ]; then
    if [ $# -eq 1 ]; then
        # 裸 --judge：只判定已有日志，不重新编译运行。
        # 供 scripts/run_stage.sh 1 --judge 使用，和其他阶段的接口对齐。
        RC=0
        judge_case fileio "$LOGS/fileio.log" || RC=1
        judge_case proc "$LOGS/proc.log" || RC=1
        judge_case pipe "$LOGS/pipe.log" || RC=1
        judge_case shell "$LOGS/shell.log" "$LOGS/minish.stdout" || RC=1
        if [ "$RC" -eq 0 ]; then echo "全部通过"; else echo "有失败项" >&2; fi
        exit "$RC"
    fi
    if [ $# -lt 3 ]; then
        echo "用法: $0 --judge [<fileio|proc|pipe|shell> <日志> [stdout]]" >&2
        exit 2
    fi
    judge_case "$2" "$3" "${4:-}"
    exit $?
fi

ONLY="${1:-}"

if ! make -s; then
    echo "编译失败" >&2
    exit 1
fi

run_case() {
    local case="$1"
    local bin="$2"

    case "$case" in
    shell)
        # stdout 与取证流分开：命令输出进 .stdout，SYS01| 行进 .log。
        # 混在一起会让"pwd 打印的 /"和"内置命令的取证行"互相干扰判定。
        "./$bin" < scripts/minish_input.txt > "$LOGS/minish.stdout" 2> "$LOGS/shell.log"
        ;;
    *)
        "./$bin" > "$LOGS/$case.log" 2>&1
        ;;
    esac
    echo "$case rc=$?"
}

if [ -z "$ONLY" ] || [ "$ONLY" = "fileio" ]; then run_case fileio fileio; fi
if [ -z "$ONLY" ] || [ "$ONLY" = "proc" ]; then run_case proc proc_life; fi
if [ -z "$ONLY" ] || [ "$ONLY" = "pipe" ]; then run_case pipe pipe_read; fi
if [ -z "$ONLY" ] || [ "$ONLY" = "shell" ]; then run_case shell minish; fi

RC=0
if [ -z "$ONLY" ] || [ "$ONLY" = "fileio" ]; then judge_case fileio "$LOGS/fileio.log" || RC=1; fi
if [ -z "$ONLY" ] || [ "$ONLY" = "proc" ]; then judge_case proc "$LOGS/proc.log" || RC=1; fi
if [ -z "$ONLY" ] || [ "$ONLY" = "pipe" ]; then judge_case pipe "$LOGS/pipe.log" || RC=1; fi
if [ -z "$ONLY" ] || [ "$ONLY" = "shell" ]; then
    judge_case shell "$LOGS/shell.log" "$LOGS/minish.stdout" || RC=1
fi

if [ "$RC" -eq 0 ]; then
    echo "全部通过"
else
    echo "有失败项" >&2
fi
exit "$RC"
