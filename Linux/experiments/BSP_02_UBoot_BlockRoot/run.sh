#!/usr/bin/env bash
# BSP_02：u-boot 阶段 + 块设备根 + initramfs，八组对照
#
# 用法:
#   bash run.sh                # 全部八组
#   bash run.sh uboot-boot     # 单组
#   UB=/path/u-boot.bin bash run.sh
#
# 环境前提（缺任何一项直接报错退出，不猜路径）：
#   qemu-system-arm、arm-linux-gnueabihf-gcc、mke2fs、sfdisk、dd、cpio、
#   内核树里的 zImage、一份 qemu_arm 的 u-boot.bin、u-boot-tools 的 mkimage
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
WORK="$SCRIPT_DIR/build"
LOGS="$SCRIPT_DIR/logs"
KERNEL_TREE="${KERNEL_TREE:-$HOME/kbuild/linux-source-7.0.0}"
ZIMAGE="$KERNEL_TREE/arch/arm/boot/zImage"
UB="${UB:-$HOME/bsp02-tools/root/usr/lib/u-boot/qemu_arm/u-boot.bin}"
MKI="${MKI:-$HOME/bsp02-tools/root/usr/bin/mkimage}"
BOOT_TIMEOUT=60

CASES=(uboot-boot uboot-nofdt uboot-badmagic uboot-initrd uboot-initrd-vs-root
       kernel-whole kernel-part kernel-nopart)

die() { printf '!! %s\n' "$*" >&2; exit 1; }

check_env() {
    local miss=0 c
    for c in qemu-system-arm arm-linux-gnueabihf-gcc mke2fs sfdisk dd cpio; do
        command -v "$c" >/dev/null 2>&1 || { printf '   缺: %s\n' "$c" >&2; miss=1; }
    done
    [ -f "$ZIMAGE" ] || { printf '   缺: %s\n' "$ZIMAGE" >&2; miss=1; }
    [ -f "$UB" ]     || { printf '   缺: %s（跑 get_uboot.sh 或设 UB=）\n' "$UB" >&2; miss=1; }
    [ -x "$MKI" ]    || { printf '   缺: %s（跑 get_uboot.sh 或设 MKI=）\n' "$MKI" >&2; miss=1; }
    [ "$miss" -eq 0 ] || die "环境不完整"
    mkdir -p "$WORK" "$LOGS"
}

# ---------- 产物构建 ----------

build_init() {
    make -s -C "$SCRIPT_DIR" || die "init 编译失败"
    file "$SCRIPT_DIR/init" | grep -q 'ARM' \
        || die "init 不是 ARM 二进制：$(file -b "$SCRIPT_DIR/init")"
}

# $1=根目录 $2=输出镜像 $3=分区字节数(为空表示整盘)
make_ext4() {
    local root="$1" img="$2" bytes="${3:-}"
    rm -f "$img"
    [ -n "$bytes" ] && truncate -s "$bytes" "$img"
    mke2fs -q -t ext4 -F -d "$root" "$img" || die "mke2fs 失败: $img"
}

# 根内容：只放 /init（取证程序）
prep_rootfs() {
    local dst="$1"
    rm -rf "$dst"; mkdir -p "$dst"
    cp "$SCRIPT_DIR/init" "$dst/init"
}

# initramfs → mkimage uImage
# bootz 的 ramdisk 校验只认 uImage/FIT，裸 gzip cpio 会被判 Wrong Ramdisk Image Format
prep_initramfs() {
    local root="$WORK/root_initrd" img="$WORK/initrd.uimg"
    rm -rf "$root"; mkdir -p "$root"
    cp "$SCRIPT_DIR/init" "$root/init"
    ( cd "$root" && find . -print0 | cpio --null -o -H newc 2>/dev/null | gzip -9 ) \
        > "$WORK/initramfs.cpio.gz" || die "initramfs 打包失败"
    "$MKI" -A arm -O linux -T ramdisk -C gzip -n "bsp02-initramfs" \
        -d "$WORK/initramfs.cpio.gz" "$img" >/dev/null || die "mkimage 失败"
    printf '%s\n' "$img"
}

# u-boot 用的盘：/init + /boot/zimage + /boot/initrd.uimg，24 MiB
prep_uboot_disk() {
    local root="$WORK/root_uboot" img="$WORK/disk_uboot.ext4"
    prep_rootfs "$root"
    mkdir -p "$root/boot"
    cp "$ZIMAGE" "$root/boot/zimage"
    cp "$WORK/initrd.uimg" "$root/boot/initrd.uimg"
    rm -f "$img"; truncate -s 24M "$img"
    mke2fs -q -t ext4 -F -d "$root" "$img" || die "mke2fs 失败: $img"
    printf '%s\n' "$img"
}

# 带 MBR 分区表的盘：分区 2048 起、20 MiB，ext4 写在分区里
prep_part_disk() {
    local root="$WORK/root_part" img="$WORK/disk_part.img" fs="$WORK/part_fs.ext4"
    prep_rootfs "$root"
    make_ext4 "$root" "$fs" 20M
    rm -f "$img"; truncate -s 32M "$img"
    sfdisk "$img" >/dev/null <<'EOF' || die "sfdisk 写分区表失败"
label: dos
start=2048, size=40960, type=83
EOF
    dd if="$fs" of="$img" bs=512 seek=2048 conv=notrunc status=none || die "dd 写入分区失败"
    printf '%s\n' "$img"
}

# ---------- 运行 ----------

qemu_common=(-M virt -m 512M -smp 1 -display none -monitor none -serial stdio -no-reboot)

# u-boot 用例：$1=tag $2=命令串（\n 分隔）
run_uboot() {
    local tag="$1"
    local cmds="$2"
    local log="$LOGS/$tag.log"
    { printf '#\n'; printf '%s\n' "$cmds"; } \
        | timeout "$BOOT_TIMEOUT" qemu-system-arm "${qemu_common[@]}" -bios "$UB" \
              -drive file="$WORK/disk_uboot.ext4",format=raw,if=none,id=root0 \
              -device virtio-blk-device,drive=root0 \
              > "$log" 2>&1
    printf '   exit=%s  log=%s\n' "$?" "${log#"$SCRIPT_DIR/"}"
}

# 内核用例：$1=tag $2=append $3=镜像
run_kernel() {
    local tag="$1"
    local append="$2"
    local img="$3"
    local log="$LOGS/$tag.log"
    timeout "$BOOT_TIMEOUT" qemu-system-arm "${qemu_common[@]}" -kernel "$ZIMAGE" \
        -drive file="$img",format=raw,if=none,id=root0 \
        -device virtio-blk-device,drive=root0 \
        -append "$append" \
        > "$log" 2>&1
    printf '   exit=%s  log=%s\n' "$?" "${log#"$SCRIPT_DIR/"}"
}

# 从日志里取证据行；没有就明确说没有，不猜
ev() {
    local log="$1"
    local pat="$2"
    grep -avE '^(ALSA|alsa|\[W\]|\[E\]|audio:|pulseaudio:|qemu-system-arm:)' "$log" \
        | grep -aE "$pat" | head -12 | sed 's/^/   /'
}

# ---------- 用例 ----------

case_uboot_boot() {
    run_uboot uboot-boot '
ext4load virtio 0 0x40400000 /boot/zimage
setenv bootargs console=ttyAMA0 root=/dev/vda init=/init panic=1
bootz 0x40400000 - 0x40000000'
    ev "$LOGS/uboot-boot.log" 'U-Boot 20|bytes read|Booting using the fdt|Starting kernel|Kernel command line|EXT4-fs \(vda\)|BOOT\|cmdline|BOOT\|root_mount|BOOT\|partitions'
}

case_uboot_nofdt() {
    run_uboot uboot-nofdt '
ext4load virtio 0 0x40400000 /boot/zimage
setenv bootargs console=ttyAMA0 root=/dev/vda init=/init panic=1
bootz 0x40400000'
    ev "$LOGS/uboot-nofdt.log" 'Working FDT|FDT and ATAGS|resetting|Starting kernel|BOOT\|'
}

case_uboot_badmagic() {
    run_uboot uboot-badmagic '
bootz 0x40400000 - 0x40000000'
    ev "$LOGS/uboot-badmagic.log" 'Bad Linux|magic|Wrong Image|resetting|FDT'
}

case_uboot_initrd() {
    run_uboot uboot-initrd '
ext4load virtio 0 0x40400000 /boot/zimage
ext4load virtio 0 0x48000000 /boot/initrd.uimg
setenv bootargs console=ttyAMA0 rdinit=/init panic=1
bootz 0x40400000 0x48000000 0x40000000'
    ev "$LOGS/uboot-initrd.log" 'bytes read|Loading init Ramdisk|Image Type|Booting using the fdt|Kernel command line|Unpacking initramfs|Freeing initrd|BOOT\|cmdline|BOOT\|root_mount|BOOT\|partitions|BOOT\|dev_vda'
}

# 同一个 initrd，但命令行里同时写 root=/dev/vda：看根到底是谁
case_uboot_initrd_vs_root() {
    run_uboot uboot-initrd-vs-root '
ext4load virtio 0 0x40400000 /boot/zimage
ext4load virtio 0 0x48000000 /boot/initrd.uimg
setenv bootargs console=ttyAMA0 root=/dev/vda rootfstype=ext4 init=/init panic=1
bootz 0x40400000 0x48000000 0x40000000'
    ev "$LOGS/uboot-initrd-vs-root.log" 'Kernel command line|Unpacking initramfs|EXT4-fs \(vda\)|VFS: Mounted root|BOOT\|cmdline|BOOT\|root_mount|BOOT\|remount_root'
}

case_kernel_whole() {    run_kernel kernel-whole \
        'console=ttyAMA0 root=/dev/vda rootfstype=ext4 init=/init panic=1' \
        "$WORK/disk_whole.ext4"
    ev "$LOGS/kernel-whole.log" 'virtio_blk|EXT4-fs \(vda\)|VFS: Mounted root|BOOT\|remount|BOOT\|root_mount|BOOT\|partitions|BOOT\|cmdline'
}

case_kernel_part() {
    run_kernel kernel-part \
        'console=ttyAMA0 root=/dev/vda1 rootfstype=ext4 init=/init panic=1' \
        "$WORK/disk_part.img"
    ev "$LOGS/kernel-part.log" 'vda1|EXT4-fs \(vda1\)|VFS: Mounted root|BOOT\|partitions|BOOT\|root_mount'
}

case_kernel_nopart() {
    run_kernel kernel-nopart \
        'console=ttyAMA0 root=/dev/vda1 rootfstype=ext4 init=/init panic=1' \
        "$WORK/disk_whole.ext4"
    ev "$LOGS/kernel-nopart.log" 'VFS: Cannot open root|panic|BOOT\|partitions'
}

run_case() {
    printf '\n===== %s =====\n' "$1"
    "case_${1//-/_}"
}

main() {
    check_env
    build_init

    printf '== 构建镜像 ==\n'
    printf '   initrd.uimg     %s B\n' "$(stat -c %s "$(prep_initramfs)")"
    printf '   initramfs.cpio.gz %s B\n' "$(stat -c %s "$WORK/initramfs.cpio.gz")"
    prep_rootfs "$WORK/root_whole"
    make_ext4 "$WORK/root_whole" "$WORK/disk_whole.ext4" 16M
    printf '   disk_whole.ext4 %s B\n' "$(stat -c %s "$WORK/disk_whole.ext4")"
    printf '   disk_uboot.ext4 %s B\n' "$(stat -c %s "$(prep_uboot_disk)")"
    printf '   disk_part.img   %s B\n'   "$(stat -c %s "$(prep_part_disk)")"

    if [ $# -eq 0 ]; then
        for c in "${CASES[@]}"; do run_case "$c"; done
    else
        for c in "$@"; do
            case " ${CASES[*]} " in
                *" $c "*) run_case "$c" ;;
                *) die "未知用例: $c（可选: ${CASES[*]}）" ;;
            esac
        done
    fi
    printf '\n日志目录: %s\n' "$LOGS"
}

main "$@"
