#!/usr/bin/env bash
# build.sh — 构建并运行 Day17 主机端判据，含负向验证
#
# 用法：
#   ./build.sh          # 构建 + 运行 + 负向验证
#   ./build.sh run      # 只运行（需先构建）
#
# 依赖：gcc（C11）、libm。不需要交叉工具链，不需要开发板。

set -u
cd "$(dirname "$0")"

SRC=lab_sensor_decode.c
BIN=lab_sensor_decode.exe
LOG_DIR=logs
mkdir -p "$LOG_DIR"

build() {
    echo "== 构建 =="
    gcc -std=c11 -Wall -Wextra -O2 "$SRC" -o "$BIN" -lm
    local rc=$?
    if [ $rc -ne 0 ]; then
        echo "构建失败 rc=$rc"
        exit $rc
    fi
    echo "构建成功（0 警告）"
}

run() {
    echo
    echo "== 判据运行 =="
    ./"$BIN"
    local rc=$?
    echo "rc=$rc"
    return $rc
}

negative() {
    echo
    echo "== 负向验证：注入必须判 FAIL（rc=1）=="
    local n fail_ok=0 fail_bad=0
    for n in 1 2 3; do
        ./"$BIN" --inject "$n" > "$LOG_DIR/neg_inject_$n.log" 2>&1
        local rc=$?
        if [ $rc -eq 1 ]; then
            echo "  inject=$n rc=1  OK（按预期判失败）"
            fail_ok=$((fail_ok + 1))
        else
            echo "  inject=$n rc=$rc  BAD（应判失败）"
            fail_bad=$((fail_bad + 1))
        fi
    done
    # 对照：未知注入编号不改变结果，必须仍 rc=0
    ./"$BIN" --inject 99 > "$LOG_DIR/neg_inject_99.log" 2>&1
    local rc99=$?
    if [ $rc99 -eq 0 ]; then
        echo "  inject=99 rc=0  OK（对照项，未知编号被忽略）"
        fail_ok=$((fail_ok + 1))
    else
        echo "  inject=99 rc=$rc99  BAD（对照项应仍通过）"
        fail_bad=$((fail_bad + 1))
    fi
    echo "负向验证：$fail_ok 项符合预期，$fail_bad 项不符"
    [ $fail_bad -eq 0 ]
}

case "${1:-all}" in
    run)
        run; exit $?
        ;;
    all)
        build || exit 1
        run > "$LOG_DIR/lab_sensor_decode.log" 2>&1
        rc=$?
        cat "$LOG_DIR/lab_sensor_decode.log"
        echo "rc=$rc（原始输出已存 $LOG_DIR/lab_sensor_decode.log）"
        [ $rc -eq 0 ] || exit $rc
        negative || exit 1
        ;;
    *)
        echo "用法：./build.sh [run|all]"
        exit 2
        ;;
esac
