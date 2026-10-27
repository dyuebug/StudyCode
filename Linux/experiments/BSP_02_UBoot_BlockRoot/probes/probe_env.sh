#!/usr/bin/env bash
# 环境探针：u-boot 阶段 + 块设备根文件系统这一课需要哪些东西在本机已具备。
# 只读探测，不安装、不下载、不改配置。
set -u

echo "=== 1. 工具链与工具 ==="
for c in qemu-system-arm arm-linux-gnueabihf-gcc mke2fs mkfs.ext4 sfdisk \
         fdisk debugfs dumpe2fs e2fsck cpio gzip dd truncate genext2fs; do
    p="$(command -v "$c" 2>/dev/null || true)"
    printf '  %-28s %s\n' "$c" "${p:-MISSING}"
done

echo
echo "=== 2. QEMU 版本 ==="
qemu-system-arm --version 2>&1 | head -2

echo
echo "=== 3. u-boot 候选（本机已存在的） ==="
if ls -l /usr/lib/u-boot/*/u-boot.bin 2>/dev/null; then :; else echo "  /usr/lib/u-boot/*/u-boot.bin : 无"; fi
echo "  apt 可安装包查询（不安装）:"
apt-cache policy u-boot-qemu 2>/dev/null | sed 's/^/    /' || echo "    apt-cache 不可用"
echo "  ~/kbuild 下的内容:"
ls -1 ~/kbuild 2>/dev/null | sed 's/^/    /' || echo "    ~/kbuild 不存在"
echo "  家目录下的 u-boot 产物（深度 4）:"
find "$HOME" -maxdepth 4 -type f -name 'u-boot*.bin' 2>/dev/null | sed 's/^/    /' | head -20
echo "  以上搜索结束"

echo
echo "=== 4. 内核树与块设备支持 ==="
K="$HOME/kbuild/linux-source-7.0.0"
echo "  zImage: $(ls -l "$K/arch/arm/boot/zImage" 2>/dev/null || echo MISSING)"
if [ -f "$K/.config" ]; then
    echo "  关键 CONFIG:"
    grep -E '^CONFIG_(EXT4_FS|EXT2_FS|VIRTIO|VIRTIO_BLK|VIRTIO_MMIO|BLK_DEV_SD|MMC|MMC_VEXPRESS|DEVTMPFS|DEVTMPFS_MOUNT|BLK_DEV_INITRD|RD_GZIP|SCSI|BLK_DEV_LOOP|SMP|ARCH_VEXPRESS|VEXPRESS_CONFIG)=' "$K/.config" \
        | sed 's/^/    /' || echo "    （无匹配）"
    echo "  未启用但相关的项（# ... is not set）:"
    grep -E '^# CONFIG_(EXT4_FS|VIRTIO_BLK|VIRTIO_MMIO|BLK_DEV_SD|MMC|DEVTMPFS_MOUNT) is not set' "$K/.config" \
        | sed 's/^/    /' || echo "    （无）"
    echo "  内核版本: $(make -s -C "$K" kernelversion 2>/dev/null || echo 未取到)"
else
    echo "  $K/.config MISSING"
fi

echo
echo "=== 5. QEMU vexpress-a9 可用设备（与启动/块设备相关） ==="
qemu-system-arm -M vexpress-a9 -device help 2>&1 \
    | grep -iE 'virtio-blk|virtio-mmio|sd-card|mmc|smc91c111|usb-storage' \
    | sed 's/^/  /' | head -20

echo
echo "=== 6. 课程资料里是否已有 u-boot 源码/镜像 ==="
ls -d /mnt/e/Code/ProjectsCode/WorkSpace/ClaudeCode/StudyCode/Linux/Linux_WEIDS/*/ 2>/dev/null \
    | sed 's#.*/Linux_WEIDS/##' | head -40
