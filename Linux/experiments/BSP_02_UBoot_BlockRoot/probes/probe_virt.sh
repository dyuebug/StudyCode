#!/usr/bin/env bash
# 机制验证（写装置前先把零件试通）：
#   T1 内核直接跑在 -M virt 上（不走 u-boot）
#   T2 用 mke2fs -d 造 ext4 全盘根，root=/dev/vda，不给 initramfs
#
# 两处已踩的坑写在命令里：
#   1. 必须确认 init 是 ARM；"CC ?=" 曾让它编成 x86-64，内核报 ENOEXEC (error -8)
#   2. 必须用 virtio-blk-device(MMIO)；if=virtio 建的是 PCI 设备，
#      本内核 "# CONFIG_PCI is not set"，压根看不见
set -u
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
WORK="$SCRIPT_DIR/build"; mkdir -p "$WORK" "$SCRIPT_DIR/logs"
KERNEL_TREE="${KERNEL_TREE:-$HOME/kbuild/linux-source-7.0.0}"
ZIMAGE="$KERNEL_TREE/arch/arm/boot/zImage"

[ -f "$ZIMAGE" ] || { echo "!! 缺 zImage: $ZIMAGE" >&2; exit 1; }

clean() { grep -avE '^(ALSA|alsa|\[W\]|\[E\]|audio:|pulseaudio:|qemu-system-arm:)' "$1"; }

echo "=== 准备 init 与 initramfs ==="
make -s -C "$SCRIPT_DIR" || { echo "!! init 编译失败" >&2; exit 1; }
if ! file "$SCRIPT_DIR/init" | grep -q 'ARM'; then
    echo "!! init 不是 ARM 二进制：$(file -b "$SCRIPT_DIR/init")" >&2
    exit 1
fi
echo "  init: $(file -b "$SCRIPT_DIR/init" | cut -d, -f1-2)"

R="$WORK/rootfs"; rm -rf "$R"; mkdir -p "$R"
cp "$SCRIPT_DIR/init" "$R/init"
( cd "$R" && find . -print0 | cpio --null -o -H newc 2>/dev/null | gzip -9 > "$WORK/initramfs.cpio.gz" )
echo "  initramfs: $(stat -c %s "$WORK/initramfs.cpio.gz") B"

echo
echo "=== T1 内核直接引导（-M virt，不传 -dtb） ==="
timeout 40 qemu-system-arm -M virt -m 512M -smp 1 -kernel "$ZIMAGE" \
    -initrd "$WORK/initramfs.cpio.gz" \
    -append "console=ttyAMA0 rdinit=/init panic=1" \
    -display none -monitor none -serial stdio -no-reboot \
    > "$SCRIPT_DIR/logs/probe_t1_virt_kernel.log" 2>&1
echo "  exit=$?"
clean "$SCRIPT_DIR/logs/probe_t1_virt_kernel.log" \
    | grep -aE 'BOOT\||Machine model|Unpacking|Run /init|Failed to execute|Kernel panic' | head -20

echo
echo "=== T2 块设备根（ext4 全盘，root=/dev/vda，virtio-blk MMIO） ==="
IMG="$WORK/rootfs_whole.ext4"
rm -f "$IMG"
truncate -s 16M "$IMG"
mke2fs -q -t ext4 -F -d "$R" "$IMG" 2>&1 | sed 's/^/  /'
echo "  镜像: $(stat -c %s "$IMG") B"
timeout 40 qemu-system-arm -M virt -m 512M -smp 1 -kernel "$ZIMAGE" \
    -drive file="$IMG",format=raw,if=none,id=d0 \
    -device virtio-blk-device,drive=d0 \
    -append "console=ttyAMA0 root=/dev/vda rootfstype=ext4 init=/init panic=1" \
    -display none -monitor none -serial stdio -no-reboot \
    > "$SCRIPT_DIR/logs/probe_t2_block_root.log" 2>&1
echo "  exit=$?"
clean "$SCRIPT_DIR/logs/probe_t2_block_root.log" \
    | grep -aE 'BOOT\||virtio_blk|EXT4-fs|VFS|Run /init|Kernel panic' | head -20
