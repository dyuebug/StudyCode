#!/usr/bin/env bash
# 用内核树中的 cpp + dtc 编译自定义 vexpress 设备树
set -euo pipefail
cd "$(dirname "$0")"

KDIR="${KDIR:-$HOME/kbuild/linux-source-7.0.0}"
DTS_DIR="$KDIR/arch/arm/boot/dts/arm"

if [ ! -x "$KDIR/scripts/dtc/dtc" ]; then
	echo "dtc not found in kernel tree: $KDIR/scripts/dtc/dtc" >&2
	exit 1
fi

cpp -nostdinc -undef -x assembler-with-cpp \
	-I "$KDIR/include" \
	-I "$KDIR/arch/arm/boot/dts" \
	-I "$DTS_DIR" \
	100ask-vexpress.dts -o .100ask-vexpress.pre.dts

"$KDIR/scripts/dtc/dtc" -I dts -O dtb \
	-o 100ask-vexpress.dtb .100ask-vexpress.pre.dts

echo "built: 100ask-vexpress.dtb"
