#!/usr/bin/env bash
# 阶段实验的统一入口：把"构建 + 运行 + 判定"从各阶段的 run.sh 收口到一处。
#
#   ./scripts/run_stage.sh list          列出已有阶段与对应目录
#   ./scripts/run_stage.sh 6             跑阶段 6 的完整流程（构建+运行+判定）
#   ./scripts/run_stage.sh 6 --judge     只判定阶段 6 已有日志
#   ./scripts/run_stage.sh 5 --negtest   只做阶段 5 的负向验证
#
# 依赖：bash 4+、各阶段自带的 run.sh。不支持 Windows cmd/PowerShell（需在 WSL 内运行）。
# 不修改除各阶段 logs/ 之外的任何文件；失败时透传对应 run.sh 的退出码。
set -u
cd "$(dirname "$0")/.." || exit 2

usage() {
        sed -n '2,12p' "$0" | sed 's/^# \{0,1\}//'
        exit "${1:-0}"
}

list_stages() {
        local d
        for d in experiments/SYS_0*_*/; do
                [ -d "$d" ] || continue
                printf '%s\n' "$(basename "$d")"
        done
}

case "${1:-}" in
"" | -h | --help)
        usage 0
        ;;
list)
        list_stages
        ;;
*)
        stage=$1
        shift
        dir=$(list_stages | grep -E "^SYS_0${stage}_" | head -1)
        if [ -z "$dir" ]; then
                echo "没有编号为 $stage 的阶段实验；可用：$(list_stages | tr '\n' ' ')" >&2
                exit 2
        fi
        if [ ! -x "experiments/$dir/run.sh" ]; then
                echo "experiments/$dir/run.sh 不存在或不可执行" >&2
                exit 2
        fi
        exec bash "experiments/$dir/run.sh" "$@"
        ;;
esac
