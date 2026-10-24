#!/usr/bin/env bash
# Day1 实验一键运行脚本
set -euo pipefail
cd "$(dirname "$0")"

make clean >/dev/null 2>&1 || true
make

echo "--- 无参数 ---"
./hello_arg

echo "--- 一个参数 ---"
./hello_arg Linux

echo "--- 两个参数 ---"
./hello_arg Linux driver
