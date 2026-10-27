#!/usr/bin/env bash
# u-boot 装载路径验证：virtio-blk(vvfat) → fatload → bootz
#
# 三个待定问题，只有实测能回答：
#   1. u-boot 能不能看见 vvfat 这块盘（virtio + FAT）
#   2. bootz 的 DT 参数从哪来（不传 -dtb，virt 的 DT 由 QEMU 给 u-boot）
#   3. 自动引导会吃掉 stdin 的第一个字符，需不需要先发一个空行
set -u
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
WORK="$SCRIPT_DIR/build"; mkdir -p "$WORK" "$SCRIPT_DIR/logs"
KERNEL_TREE="${KERNEL_TREE:-$HOME/kbuild/linux-source-7.0.0}"
UB="$HOME/bsp02-tools/root/usr/lib/u-boot/qemu_arm/u-boot.bin"
ZIMAGE="$KERNEL_TREE/arch/arm/boot/zImage"
LOG="$SCRIPT_DIR/logs/probe_uboot_load.log"

make -s -C "$SCRIPT_DIR" || { echo "!! init 编译失败" >&2; exit 1; }

# vvfat 用短文件名，避免 8.3 截断带来意料之外的别名
FAT="$WORK/fat"; rm -rf "$FAT"; mkdir -p "$FAT"
cp "$ZIMAGE" "$FAT/zimage"
cp "$SCRIPT_DIR/init" "$FAT/init"
( cd "$WORK/rootfs" 2>/dev/null || { mkdir -p "$WORK/rootfs"; cp "$SCRIPT_DIR/init" "$WORK/rootfs/init"; }; \
  find . -print0 | cpio --null -o -H newc 2>/dev/null | gzip -9 ) > "$FAT/initrd"
ls -l "$FAT" | sed 's/^/  /'

echo "==> 启动 u-boot 并执行装载序列"
# 用 fat:rw: 而不是 fat:ro:：QEMU 的 virtio-blk 不接受只读块节点，
# 会直接拒绝启动（"Block node is read-only"）。本实验只读不写，rv 权限没有实际影响。
timeout 60 qemu-system-arm -M virt -m 512M -smp 1 -bios "$UB" \
    -drive file=fat:rw:"$FAT",format=raw,if=none,id=fat0 \
    -device virtio-blk-device,drive=fat0 \
    -display none -monitor none -serial stdio -no-reboot \
    > "$LOG" 2>&1 <<'CMDS'

virtio info
ls virtio 0
fatload virtio 0 0x40400000 zimage
fatload virtio 0 0x48000000 initrd
setenv bootargs console=ttyAMA0 rdinit=/init panic=1
printenv bootargs
bootz 0x40400000 0x48000000
CMDS
echo "  exit=$?"
echo "  --- 关键行 ---"
grep -avE '^(ALSA|alsa|\[W\]|\[E\]|audio:|pulseaudio:|qemu-system-arm:)' "$LOG" \
    | grep -aE 'U-Boot 20|=> |virtio|fatload|reading|bootargs|Starting kernel|BOOT\||panic|Bad Linux|Machine model' \
    | head -40
