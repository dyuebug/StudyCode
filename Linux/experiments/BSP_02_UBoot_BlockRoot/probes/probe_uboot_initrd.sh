#!/usr/bin/env bash
# 试探：mkimage -T ramdisk 把 initramfs 包成 uImage 后，bootz 是否接受并传给内核。
set -u
SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"   # probes/ 的上一层即实验目录
WORK="$SCRIPT_DIR/build"; LOGD="$SCRIPT_DIR/probes/logs"; mkdir -p "$WORK" "$LOGD"
KERNEL_TREE="${KERNEL_TREE:-$HOME/kbuild/linux-source-7.0.0}"
ZIMAGE="$KERNEL_TREE/arch/arm/boot/zImage"
UB="$HOME/bsp02-tools/root/usr/lib/u-boot/qemu_arm/u-boot.bin"
MKI="$HOME/bsp02-tools/root/usr/bin/mkimage"
LOG="$LOGD/probe_uboot_initrd.log"

[ -x "$MKI" ] || { echo "!! 缺 mkimage（先跑 get_uboot.sh）" >&2; exit 1; }

make -s -C "$SCRIPT_DIR" || exit 1

# 1) initramfs（gzip cpio）
R="$WORK/root_initrd"; rm -rf "$R"; mkdir -p "$R"; cp "$SCRIPT_DIR/init" "$R/init"
( cd "$R" && find . -print0 | cpio --null -o -H newc 2>/dev/null | gzip -9 ) > "$WORK/initramfs.cpio.gz"
echo "  initramfs.cpio.gz: $(stat -c %s "$WORK/initramfs.cpio.gz") B"

# 2) mkimage 包成 uImage
"$MKI" -A arm -O linux -T ramdisk -C gzip -n "bsp02-initramfs" \
    -d "$WORK/initramfs.cpio.gz" "$WORK/initrd.uimg"
echo "  initrd.uimg: $(stat -c %s "$WORK/initrd.uimg") B"
"$MKI" -l "$WORK/initrd.uimg" | sed 's/^/    /'

# 3) 放进 ext4 盘
D="$WORK/root_uboot"; mkdir -p "$D/boot"
cp "$SCRIPT_DIR/init" "$D/init"; cp "$ZIMAGE" "$D/boot/zimage"; cp "$WORK/initrd.uimg" "$D/boot/initrd.uimg"
IMG="$WORK/disk_uboot.ext4"; rm -f "$IMG"; truncate -s 28M "$IMG"
mke2fs -q -t ext4 -F -d "$D" "$IMG"

echo "==> u-boot: ext4load 内核与 uImage ramdisk"
timeout 60 qemu-system-arm -M virt -m 512M -smp 1 -bios "$UB" \
    -drive file="$IMG",format=raw,if=none,id=root0 \
    -device virtio-blk-device,drive=root0 \
    -display none -monitor none -serial stdio -no-reboot \
    > "$LOG" 2>&1 <<'CMDS'
#
ls virtio 0 /boot
ext4load virtio 0 0x40400000 /boot/zimage
ext4load virtio 0 0x48000000 /boot/initrd.uimg
setenv bootargs console=ttyAMA0 rdinit=/init panic=1
bootz 0x40400000 0x48000000 0x40000000
CMDS
echo "  exit=$?"
grep -avE '^(ALSA|alsa|\[W\]|\[E\]|audio:|pulseaudio:|qemu-system-arm:)' "$LOG" \
    | grep -aE '=> |bytes read|Wrong Ramdisk|Booting using|Starting kernel|Unpacking initramfs|Kernel command line|BOOT\||panic' \
    | head -25
