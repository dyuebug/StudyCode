#!/usr/bin/env bash
# 启动 QEMU vexpress-a9 并执行自动验证
#
# 用法：bash run.sh
# 退出：init 完成后重启，-no-reboot 让 QEMU 退出

set -euo pipefail
cd "$(dirname "$0")"

KDIR="${KDIR:-$HOME/kbuild/linux-source-7.0.0}"
TIMEOUT="${TIMEOUT:-120}"

if [ ! -f "$KDIR/arch/arm/boot/zImage" ]; then
	echo "zImage not found: $KDIR/arch/arm/boot/zImage" >&2
	exit 1
fi

if [ ! -f initramfs.cpio.gz ]; then
	echo "initramfs missing, run: make" >&2
	exit 1
fi

DTB="${DTB:-$PWD/100ask-vexpress.dtb}"
if [ ! -f "$DTB" ]; then
	echo "custom dtb missing, run: make dtb" >&2
	exit 1
fi

exec timeout "$TIMEOUT" qemu-system-arm \
	-M vexpress-a9 \
	-m 512M \
	-nographic \
	-no-reboot \
	-kernel "$KDIR/arch/arm/boot/zImage" \
	-dtb "$DTB" \
	-initrd initramfs.cpio.gz \
	-append "console=ttyAMA0 loglevel=7 rdinit=/init"
