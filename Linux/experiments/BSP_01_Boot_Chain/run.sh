#!/usr/bin/env bash
# BSP_01 启动链取证：用四组对照回答"zImage / DTB / initramfs 各自管什么"
# （在 WSL / Linux 下运行；QEMU 与 ARM 内核树都在 WSL 侧）
#
# 用法：
#   ./run.sh             # 跑全部四组
#   ./run.sh normal      # 三件套齐全
#   ./run.sh bad-init    # rdinit 指向不存在的路径
#   ./run.sh no-initrd   # 不给 initramfs
#   ./run.sh qemu-dtb    # 不给 -dtb，用 QEMU 自带设备树
#
# 前置：
#   - qemu-system-arm >= 10.x
#   - arm-linux-gnueabihf-gcc、cpio、gzip
#   - 已构建的 ARMv7 内核树（默认见 KERNEL_TREE）
#   - 自定义 DTB（默认复用 tests/qemu-vexpress/100ask-vexpress.dtb）
#
# 为什么四组：正向用例只能证明"这条路能走通"，证明不了"少一个会怎样"。
# 缺 initramfs、改坏 rdinit、换一份 DTB，三种现象完全不同，
# 组合起来才说明三件套里每一件各自负责什么。

set -u

SCRIPT_DIR="$( cd "$( dirname "$0" )" && pwd )"
LINUX_ROOT="$( cd "$SCRIPT_DIR/../.." && pwd )"
WORK_DIR="$SCRIPT_DIR/build"
LOG_DIR="$SCRIPT_DIR/logs"

KERNEL_TREE="${KERNEL_TREE:-$HOME/kbuild/linux-source-7.0.0}"
ZIMAGE="$KERNEL_TREE/arch/arm/boot/zImage"
DTB="${DTB:-$LINUX_ROOT/tests/qemu-vexpress/100ask-vexpress.dtb}"

QEMU_BIN="${QEMU_BIN:-qemu-system-arm}"
CROSS_CC="${CROSS_CC:-arm-linux-gnueabihf-gcc}"
BOOT_TIMEOUT="${BOOT_TIMEOUT:-90}"

die()
{
    echo "[错误] $*" >&2
    exit 1
}

check_env()
{
    command -v "$QEMU_BIN" >/dev/null 2>&1 || die "找不到 $QEMU_BIN"
    command -v "$CROSS_CC"  >/dev/null 2>&1 || die "找不到 $CROSS_CC"
    command -v cpio         >/dev/null 2>&1 || die "找不到 cpio"
    [ -f "$ZIMAGE" ] || die "找不到内核镜像：$ZIMAGE（用 KERNEL_TREE 指定）"
    [ -f "$DTB"    ] || die "找不到设备树：$DTB（用 DTB 指定）"
}

build_initramfs()
{
    make -C "$SCRIPT_DIR" CC="$CROSS_CC" >/dev/null || die "init 编译失败"

    rm -rf "$WORK_DIR/root"
    mkdir -p "$WORK_DIR/root"
    cp "$SCRIPT_DIR/init" "$WORK_DIR/root/init"
    chmod +x "$WORK_DIR/root/init"

    ( cd "$WORK_DIR/root" && find . | cpio -o -H newc 2>/dev/null | gzip -9 ) \
        > "$WORK_DIR/initramfs.cpio.gz"
    [ -s "$WORK_DIR/initramfs.cpio.gz" ] || die "initramfs 打包失败"
}

# 每组用例：QEMU 额外参数 + 内核命令行
qemu_extra_for()
{
    case "$1" in
        qemu-dtb) echo "" ;;                        # 故意不给 -dtb
        *)        echo "-dtb $DTB" ;;
    esac
}

cmdline_for()
{
    case "$1" in
        bad-init) echo "console=ttyAMA0 rdinit=/nosuch/init panic=1" ;;
        *)        echo "console=ttyAMA0 rdinit=/init panic=1" ;;
    esac
}

# 不给 DTB 时内核在串口初始化之前就停住，一个字符都不会出，
# 等满 90 s 没有意义，单独给一个短超时。
timeout_for()
{
    case "$1" in
        qemu-dtb) echo 25 ;;
        *)        echo "$BOOT_TIMEOUT" ;;
    esac
}

run_case()
{
    local name="$1" extra cmdline log rc wait_s

    extra="$( qemu_extra_for "$name" )"
    cmdline="$( cmdline_for "$name" )"
    wait_s="$( timeout_for "$name" )"
    log="$LOG_DIR/${name}.log"
    mkdir -p "$LOG_DIR"

    echo "=== 用例：$name ==="
    echo "    cmdline：$cmdline"
    echo "    dtb：${extra:-（不给，用 QEMU 自带）}"

    if [ "$name" = "no-initrd" ]; then
        # shellcheck disable=SC2086
        timeout "$wait_s" "$QEMU_BIN" -M vexpress-a9 -m 512M -smp 1 \
            -kernel "$ZIMAGE" $extra \
            -append "$cmdline" \
            -display none -monitor none -serial stdio -no-reboot \
            > "$log" 2>&1
    else
        # shellcheck disable=SC2086
        timeout "$wait_s" "$QEMU_BIN" -M vexpress-a9 -m 512M -smp 1 \
            -kernel "$ZIMAGE" $extra \
            -initrd "$WORK_DIR/initramfs.cpio.gz" \
            -append "$cmdline" \
            -display none -monitor none -serial stdio -no-reboot \
            > "$log" 2>&1
    fi

    rc=$?

    echo "    ---- 装置输出（BOOT|） ----"
    grep -a -E '^BOOT\|' "$log" || echo "    （没有 BOOT| 行）"

    echo "    ---- 内核对三件套的处理 ----"
    grep -a -E 'Unpacking initramfs|Machine model|Run /|Failed to execute|Kernel panic|Unable to mount root|Freeing unused kernel|Booting Linux|DTB|dtb' "$log" \
        | head -n 12 || echo "    （没有匹配行）"

    if [ $rc -eq 0 ]; then
        echo "    [ok] 断电或 panic 后重启，QEMU 正常退出"
    elif [ $rc -eq 124 ]; then
        echo "    [注意] ${wait_s}s 超时：内核没有自我退出（本用例的预期行为，见日志）"
    else
        echo "    [注意] QEMU 退出码 $rc"
    fi

    echo "    完整日志：$log"
    echo
}

main()
{
    local arg="${1:-all}"

    check_env
    build_initramfs

    if [ "$arg" = "all" ]; then
        for c in normal bad-init no-initrd qemu-dtb; do run_case "$c"; done
    else
        run_case "$arg"
    fi
}

main "$@"
