#!/usr/bin/env bash
# 定位 u-boot 传递 DT 的正确姿势：
#   R1 先看环境里的 fdt 相关变量，并验证 0x40000000 处是不是一份可用 DT
#   R2 bootz 时显式给出 fdt 地址
#   R3 bootz 显式给出 "-"（表示无 initrd）并用 $fdtcontroladdr
set -u
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
WORK="$SCRIPT_DIR/build"; LOGD="$SCRIPT_DIR/logs"; mkdir -p "$LOGD"
UB="$HOME/bsp02-tools/root/usr/lib/u-boot/qemu_arm/u-boot.bin"
IMG="$WORK/uboot_disk.ext4"
[ -f "$IMG" ] || { echo "!! 先跑 probe_uboot_ext4.sh 生成 $IMG" >&2; exit 1; }

run() {
    local tag="$1"; shift
    local log="$LOGD/probe_fdt_$tag.log"
    echo "==> [$tag]"
    timeout 45 qemu-system-arm -M virt -m 512M -smp 1 -bios "$UB" \
        -drive file="$IMG",format=raw,if=none,id=root0 \
        -device virtio-blk-device,drive=root0 \
        -display none -monitor none -serial stdio -no-reboot \
        > "$log" 2>&1
    grep -avE '^(ALSA|alsa|\[W\]|\[E\]|audio:|pulseaudio:|qemu-system-arm:)' "$log" \
        | grep -aE '=> |fdt|FDT|bootargs=|Starting kernel|BOOT\||EXT4-fs|VFS|panic|Bad Linux|Machine model|resetting' \
        | head -30
    echo
}

# R1：只做检查，不引导
printf '%s\n' '#' 'printenv fdtcontroladdr fdt_addr loadaddr' 'fdt addr 0x40000000' \
    'fdt header' 'fdt print /model' \
    | run r1_check

# R2：显式 fdt
printf '%s\n' '#' 'ext4load virtio 0 0x40400000 /boot/zimage' \
    'setenv bootargs console=ttyAMA0 root=/dev/vda init=/init panic=1' \
    'bootz 0x40400000 - 0x40000000' \
    | run r2_explicit -
