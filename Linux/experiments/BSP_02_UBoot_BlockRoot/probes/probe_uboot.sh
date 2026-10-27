#!/usr/bin/env bash
# u-boot 能力探针：真的启动它，在 prompt 里问它自己有什么命令、环境变量长什么样。
# 结论只认这份日志，不靠 strings 猜。
set -eu

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
UB="$HOME/bsp02-tools/root/usr/lib/u-boot/qemu_arm/u-boot.bin"
LOG_DIR="$SCRIPT_DIR/logs"
mkdir -p "$LOG_DIR"
LOG="$LOG_DIR/probe_uboot.log"

[ -f "$UB" ] || { echo "!! 缺少 u-boot: $UB（先跑 get_uboot.sh）" >&2; exit 1; }

echo "==> 启动 u-boot，从 stdin 喂命令"
timeout 30 qemu-system-arm \
    -M vexpress-a9 -m 512M -smp 1 \
    -bios "$UB" \
    -display none -monitor none -serial stdio -no-reboot \
    > "$LOG" 2>&1 <<'CMDS'
version
bdinfo
mmc list
mmc dev 0
printenv bootcmd
printenv bootargs
help bootz
help fatload
help mmc
reset
CMDS

echo "==> 日志: $LOG ($(wc -l < "$LOG") 行)"
echo "----- 前 40 行 -----"
sed -n '1,40p' "$LOG"
