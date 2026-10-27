#!/usr/bin/env bash
# 取 u-boot 二进制与 mkimage 而不安装系统包：apt-get download + dpkg-deb -x 解到用户目录。
# 下载物放在独立的空目录里，repo 只通过绝对路径引用产物，不执行包内任何脚本。
set -eu

WORK="$HOME/bsp02-tools"
DEB_DIR="$WORK/deb"
OUT="$WORK/root"

mkdir -p "$DEB_DIR" "$OUT"
cd "$DEB_DIR"

# u-boot-qemu 提供各架构的 u-boot.bin；u-boot-tools 提供宿主机上的 mkimage
for pkg in u-boot-qemu u-boot-tools; do
    if ! ls -1 "${pkg}"_*.deb >/dev/null 2>&1; then
        echo "==> apt-get download $pkg"
        apt-get download "$pkg"
    fi
done
ls -l ./*.deb

echo "==> dpkg-deb -x"
for d in ./*.deb; do
    dpkg-deb -x "$d" "$OUT"
done

echo
echo "==> u-boot 产物"
find "$OUT" -type f -name 'u-boot*.bin' | sort | while read -r f; do
    printf '  %-62s %8s B\n' "$f" "$(stat -c %s "$f")"
done

UB="$OUT/usr/lib/u-boot/qemu_arm/u-boot.bin"
MKI="$OUT/usr/bin/mkimage"

echo
if [ -f "$UB" ]; then
    echo "==> qemu_arm/u-boot.bin 就位: $UB ($(stat -c %s "$UB") B)"
    strings "$UB" | grep -m1 -E '^U-Boot 20|^U-Boot 1' | sed 's/^/    /' || true
else
    echo "!! 未找到 qemu_arm/u-boot.bin" >&2
    exit 1
fi

if [ -x "$MKI" ]; then
    echo "==> mkimage 就位: $MKI"
    "$MKI" -V | sed 's/^/    /'
else
    echo "!! 未找到 mkimage（u-boot-tools 解包失败？）" >&2
    exit 1
fi
