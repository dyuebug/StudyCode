#!/usr/bin/env bash
# u-boot 装载路径验证（定稿方向）：
#   ext4 盘里放 /boot/zimage 与 /init
#   u-boot: ext4load 内核 → setenv bootargs → bootz
#   内核:   用同一块盘做根 (root=/dev/vda)
#
# 不传 initrd：实测 u-boot 的 bootz 不认裸 gzip 压缩的 initramfs
# （"Wrong Ramdisk Image Format"），要用 mkimage 包成 uImage 或做 FIT。
set -u
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
WORK="$SCRIPT_DIR/build"; mkdir -p "$WORK" "$SCRIPT_DIR/logs"
KERNEL_TREE="${KERNEL_TREE:-$HOME/kbuild/linux-source-7.0.0}"
UB="$HOME/bsp02-tools/root/usr/lib/u-boot/qemu_arm/u-boot.bin"
ZIMAGE="$KERNEL_TREE/arch/arm/boot/zImage"
LOG="$SCRIPT_DIR/logs/probe_uboot_ext4.log"

make -s -C "$SCRIPT_DIR" || { echo "!! init 编译失败" >&2; exit 1; }
file "$SCRIPT_DIR/init" | grep -q ARM || { echo "!! init 不是 ARM" >&2; exit 1; }

R="$WORK/rootfs_uboot"; rm -rf "$R"; mkdir -p "$R/boot"
cp "$SCRIPT_DIR/init" "$R/init"
cp "$ZIMAGE" "$R/boot/zimage"

IMG="$WORK/uboot_disk.ext4"; rm -f "$IMG"
truncate -s 24M "$IMG"
mke2fs -q -t ext4 -F -d "$R" "$IMG"
echo "==> 盘: $IMG ($(stat -c %s "$IMG") B)"
dumpe2fs -h "$IMG" 2>/dev/null | grep -E 'Filesystem features|Block size|Inode size' | sed 's/^/  /'

echo "==> 启动 u-boot"
timeout 90 qemu-system-arm -M virt -m 512M -smp 1 -bios "$UB" \
    -drive file="$IMG",format=raw,if=none,id=root0 \
    -device virtio-blk-device,drive=root0 \
    -display none -monitor none -serial stdio -no-reboot \
    > "$LOG" 2>&1 <<'CMDS'

ls virtio 0 /
ls virtio 0 /boot
ext4load virtio 0 0x40400000 /boot/zimage
setenv bootargs console=ttyAMA0 root=/dev/vda init=/init panic=1
printenv bootargs
bootz 0x40400000
CMDS
echo "  exit=$?"
echo "  --- 关键行 ---"
grep -avE '^(ALSA|alsa|\[W\]|\[E\]|audio:|pulseaudio:|qemu-system-arm:)' "$LOG" \
    | grep -aE 'U-Boot 20|=> |ext4load|bytes read|bootargs=|Starting kernel|BOOT\||EXT4-fs|VFS|panic|Bad Linux' \
    | head -40
