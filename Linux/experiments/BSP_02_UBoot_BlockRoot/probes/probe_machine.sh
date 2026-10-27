#!/usr/bin/env bash
# 判定这份 u-boot 到底认哪台机器：virt 与 vexpress-a9 各试一次，看谁出 prompt。
set -u
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
UB="$HOME/bsp02-tools/root/usr/lib/u-boot/qemu_arm/u-boot.bin"
LOG_DIR="$SCRIPT_DIR/logs"; mkdir -p "$LOG_DIR"

probe() {
    local tag="$1" machine="$2"; shift 2
    local log="$LOG_DIR/probe_machine_$tag.log"
    echo "==> [$tag] -M $machine"
    timeout 25 qemu-system-arm -M "$machine" -m 512M -smp 1 -bios "$UB" "$@" \
        -display none -monitor none -serial stdio -no-reboot \
        > "$log" 2>&1 <<'CMDS'
version
bdinfo
CMDS
    echo "    有效输出："
    grep -avE '^(ALSA|alsa|\[W\]|\[E\]|audio:|pulseaudio:|qemu-system-arm:)' "$log" \
        | sed 's/^/      /' | head -30
    echo
}

probe virt virt
probe vexpress vexpress-a9
