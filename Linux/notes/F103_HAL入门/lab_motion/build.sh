#!/usr/bin/env bash
# build.sh — 构建并运行 Day18 主机端判据，含负向验证
#
# 用法：
#   ./build.sh          # 构建 + 运行 + 负向验证
#   ./build.sh run      # 只运行（需先构建）
#
# 依赖：gcc（C11）。不需要交叉工具链，不需要开发板。

set -u
cd "$(dirname "$0")"

SRC=lab_motion_timing.c
BIN=lab_motion_timing.exe
LOG_DIR=logs
mkdir -p "$LOG_DIR"

build() {
    echo "== 构建 =="
    gcc -std=c11 -Wall -Wextra -O2 "$SRC" -o "$BIN"
    local rc=$?
    if [ $rc -ne 0 ]; then
        echo "构建失败 rc=$rc"
        exit $rc
    fi
    echo "构建成功（0 警告）"
}

negative() {
    echo
    echo "== 负向验证：注入必须判 FAIL（rc=1）=="
    local n ok=0 bad=0
    for n in 1 2 3; do
        ./"$BIN" --inject "$n" > "$LOG_DIR/neg_inject_$n.log" 2>&1
        local rc=$?
        if [ $rc -eq 1 ]; then
            echo "  inject=$n rc=1  OK（按预期判失败）"
            ok=$((ok + 1))
        else
            echo "  inject=$n rc=$rc  BAD（应判失败）"
            bad=$((bad + 1))
        fi
    done
    ./"$BIN" --inject 99 > "$LOG_DIR/neg_inject_99.log" 2>&1
    local rc99=$?
    if [ $rc99 -eq 0 ]; then
        echo "  inject=99 rc=0  OK（对照项，未知编号被忽略）"
        ok=$((ok + 1))
    else
        echo "  inject=99 rc=$rc99  BAD（对照项应仍通过）"
        bad=$((bad + 1))
    fi
    echo "负向验证：$ok 项符合预期，$bad 项不符"
    [ $bad -eq 0 ]
}

case "${1:-all}" in
    run)
        ./"$BIN"; exit $?
        ;;
    all)
        build || exit 1
        echo
        echo "== 判据运行 =="
        ./"$BIN" > "$LOG_DIR/lab_motion_timing.log" 2>&1
        rc=$?
        cat "$LOG_DIR/lab_motion_timing.log"
        echo "rc=$rc（原始输出已存 $LOG_DIR/lab_motion_timing.log）"
        [ $rc -eq 0 ] || exit $rc
        negative || exit 1
        ;;
    *)
        echo "用法：./build.sh [run|all]"
        exit 2
        ;;
esac
