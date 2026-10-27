#!/usr/bin/env bash
# 试探 u-boot 在 vexpress-a9 上的装载方式：-bios 与 -kernel 两种，取能出 prompt 的那个。
set -u
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
UB="$HOME/bsp02-tools/root/usr/lib/u-boot/qemu_arm/u-boot.bin"
LOG_DIR="$SCRIPT_DIR/logs"; mkdir -p "$LOG_DIR"

run() {
    local tag="$1"; shift
    local log="$LOG_DIR/probe_load_$tag.log"
    echo "==> [$tag] $*"
    timeout 20 qemu-system-arm -M vexpress-a9 -m 512M -smp 1 "$@" \
        -display none -monitor none -serial stdio -no-reboot \
        > "$log" 2>&1 <<'CMDS'
version
CMDS
    echo "    退出码 $? ；有效行（过滤 ALSA/qemu-system 噪音）:"
    grep -avE '^(ALSA|alsa|\[W\]|\[E\]|audio:|qemu-system-arm:)' "$log" | sed 's/^/      /' | head -25
    echo
}

run bios -bios "$UB"
run kernel -kernel "$UB"
