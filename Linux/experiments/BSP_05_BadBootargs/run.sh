#!/usr/bin/env bash
# BSP_05 坏 bootargs：区分"bootloader 参数错"与"内核配置错"（在 WSL / Linux 下运行）
#
# 用法：
#   ./run.sh              跑全部六组，逐组机器判定
#   ./run.sh ok           只跑一组
#   ./run.sh --judge [<用例> [日志]]   只判定不跑；不带用例名时判定 logs/ 下全部已有日志
#
# 前置：
#   - qemu-system-arm >= 10.x
#   - arm-linux-gnueabihf-gcc、mke2fs、file
#   - 已构建的 ARMv7 内核树（默认见 KERNEL_TREE）
#
# 为什么需要六组：
#   只跑 "ok" 证明不了任何事——内核配置对的时候它当然能起来。
#   要判断"起不来"是参数错还是内核配置错，必须让**同一份 zImage、同一份镜像**
#   在只改命令行的情况下分别失败和成功。这就是本实验的全部设计：
#
#     bad-root / bad-type / bad-init  → 三种致命参数错，日志里各点名一处
#     bad-console                     → 一个**坏但非致命**的参数，内核照常引导
#     ok-fixed                        → 与 bad-root 只差设备名一个字符，必须恢复
#
#   最后两组缺一不可：没有 bad-console，"内核能起来"可能只是运气；
#   没有 ok-fixed，"参数错"就无法与"这个内核根本不支持 virtio/ext4"分开。

set -uo pipefail

SCRIPT_DIR="$( cd "$( dirname "$0" )" && pwd )"
WORK="$SCRIPT_DIR/build"
LOGS="$SCRIPT_DIR/logs"

KERNEL_TREE="${KERNEL_TREE:-$HOME/kbuild/linux-source-7.0.0}"
ZIMAGE="$KERNEL_TREE/arch/arm/boot/zImage"
QEMU_BIN="${QEMU_BIN:-qemu-system-arm}"
CROSS_CC="${CROSS_CC:-arm-linux-gnueabihf-gcc}"
BOOT_TIMEOUT="${BOOT_TIMEOUT:-60}"

MARKER_TEXT="bsp05-marker"
DISK_IMG="root_ok.ext4"

die()
{
    printf '!! %s\n' "$*" >&2
    exit 1
}

check_env()
{
    local miss=0 c

    for c in "$QEMU_BIN" "$CROSS_CC" mke2fs file; do
        command -v "$c" >/dev/null 2>&1 || { printf '   缺: %s\n' "$c" >&2; miss=1; }
    done

    [ -f "$ZIMAGE" ] || { printf '   缺: %s（用 KERNEL_TREE 指定）\n' "$ZIMAGE" >&2; miss=1; }
    [ "$miss" -eq 0 ] || die "环境不完整"

    mkdir -p "$WORK" "$LOGS"
}

# ---------- 产物 ----------

build_bins()
{
    make -s -C "$SCRIPT_DIR" CC="$CROSS_CC" || die "编译失败"
    file "$SCRIPT_DIR/init" | grep -q 'ARM' \
        || die "init 不是 ARM 二进制：$(file -b "$SCRIPT_DIR/init")"
}

# 真实根：只放 init 与标记文件。**没有 initramfs**——这是本实验与前两个 BSP 的关键差别。
build_rootfs()
{
    local root="$WORK/rootfs"

    rm -rf "$root"
    mkdir -p "$root/sbin" "$root/dev" "$root/proc" "$root/sys"
    cp "$SCRIPT_DIR/init" "$root/sbin/init"
    chmod 0755 "$root/sbin/init"
    printf '%s\n' "$MARKER_TEXT" > "$root/marker"

    rm -f "$WORK/$DISK_IMG"
    truncate -s 8M "$WORK/$DISK_IMG"
    mke2fs -q -t ext4 -F -d "$root" "$WORK/$DISK_IMG" || die "mke2fs 失败"
}

# ---------- 运行 ----------

CONSOLE="console=ttyAMA0"
DISK_ARGS=( -drive "file=$WORK/$DISK_IMG,format=raw,if=none,id=root0"
            -device virtio-blk-device,drive=root0 )

# $1=用例名  $2=内核命令行  $3=1 挂盘 / 0 不挂  $4=超时秒数（panic 用例不需要等满 60 s）
run_case()
{
    local name="$1" append="$2" with_disk="$3" tmo="${4:-$BOOT_TIMEOUT}"
    local log="$LOGS/$name.log"
    local args=(-M virt -m 512M -smp 1 -display none -monitor none -serial stdio -no-reboot
                -kernel "$ZIMAGE" -append "$append")

    if [ "$with_disk" = "1" ]; then
        args+=( "${DISK_ARGS[@]}" )
    fi

    printf '\n===== %s =====\n' "$name"
    printf '   append: %s\n' "$append"

    timeout "$tmo" "$QEMU_BIN" "${args[@]}" > "$log" 2>&1
    printf '   exit=%s  log=%s\n' "$?" "${log#"$SCRIPT_DIR/"}"

    if grep -aq '^BADARGS|' "$log"; then
        grep -a '^BADARGS|' "$log" | sed 's/^/   /'
    else
        printf '   （没有 BADARGS| 行：init 没跑起来）\n'
    fi
}

# ---------- 判定 ----------

has()  { grep -aqF "$2" "$1"; }
lacks() { ! grep -aqF "$2" "$1"; }

expect()
{
    local log="$1"; shift
    local ok=0 pat

    for pat in "$@"; do
        if ! has "$log" "$pat"; then
            printf '   缺失：%s\n' "$pat" >&2
            ok=1
        fi
    done

    return "$ok"
}

refute()
{
    local log="$1"; shift
    local ok=0 pat

    for pat in "$@"; do
        if has "$log" "$pat"; then
            printf '   不该出现：%s\n' "$pat" >&2
            ok=1
        fi
    done

    return "$ok"
}

# "内核没挂到根"的统一形状：panic 行是 `VFS: Unable to mount root fs on ...`。
# 注意不能只用 `VFS: Cannot open root device` —— 那是 bad-root（设备名不存在）独有的；
# bad-type 的设备能找到，失败在超级块，panic 措辞不同。两者共用的只有 panic 这一行。
judge_not_mounted()
{
    expect "$1" 'VFS: Unable to mount root fs' \
    && refute "$1" 'BADARGS|init|started'
}

# 内核自己列出的"可挂载文件系统"清单。它证明 ext4 驱动**在**内核里，
# 所以 bad-root/bad-type 的失败只能是参数/镜像问题，不是内核配置缺项。
# 这是本实验区分"bootloader 参数错"与"内核配置错"的直接证据。
expect_kernel_has_ext4()
{
    expect "$1" 'List of all bdev filesystems:' 'ext4'
}

judge_ok()
{
    expect "$1" \
        'BADARGS|init|started' \
        'BADARGS|root_fs|magic=0xef53' \
        "BADARGS|marker|$MARKER_TEXT" \
        'BADARGS|init|done' \
    && expect "$1" 'BADARGS|cmdline|' 'console=ttyAMA0'
}

# bad-root：root 设备名不存在。内核找不到设备，init 不可能跑。
judge_bad_root()
{
    judge_not_mounted "$1" \
    && expect_kernel_has_ext4 "$1" \
    && expect "$1" 'VFS: Cannot open root device "/dev/vdz"' \
                'Please append a correct "root=" boot option'
}

# bad-type：设备在、但按 ext2 去读 ext4 镜像（ext4 特性位 22c4 在 ext2 里不支持）。
# 判据必须点名 ext2 路径，而不是笼统的"挂载失败"——否则与 bad-root 无法区分。
judge_bad_type()
{
    judge_not_mounted "$1" \
    && expect "$1" \
        'EXT2-fs (vda): error: couldn'"'"'t mount because of unsupported optional features' \
        'No filesystem could mount root, tried:' \
        'ext2'
}

# bad-init：root 挂载**成功**（EXT4-fs ... mounted + VFS: Mounted root），
# 错只在 init= 这一个参数上。这一组把 `root=` 与 `init=` 分开。
judge_bad_init()
{
    expect "$1" \
        'EXT4-fs (vda): mounted filesystem' \
        'VFS: Mounted root (ext4 filesystem)' \
        'Run /nope as init process' \
        'Requested init /nope failed' \
    && refute "$1" 'BADARGS|init|started'
}

# bad-console：一个不存在的 console 参数 + 一个有效 console。
# 内核忽略坏的那个、照常用好的那个 → 引导完全正常。
# 这条证明"参数里有坏值"不等于"内核坏了"。
# 关键读数：printk 只对 ttyAMA0 打 enabled，ttyS9 被静默忽略。
judge_bad_console()
{
    judge_ok "$1" \
    && expect "$1" 'printk: console [ttyAMA0] enabled'
}

# ok-fixed：与 bad-root 只差设备名。同一镜像、同一内核。
judge_ok_fixed()
{
    judge_ok "$1"
}

# 六组必须跑在**同一份内核**上，否则"改参数就恢复"退化成"换了个内核"。
# 判据：六份日志的 `Linux version` 行完全一致（含构建时间与 #N）。
judge_same_kernel()
{
    local base rc=0 c

    base=$( grep -ahm1 '^Linux version' "$LOGS/ok.log" | tr -d '\r' )

    if [ -z "$base" ]; then
        printf '   [FAIL] 取不到 ok.log 的内核版本行\n' >&2
        return 1
    fi

    for c in "${CASES[@]}"; do
        if ! grep -aqF "$base" "$LOGS/$c.log"; then
            printf '   [FAIL] %s 的内核不是同一份\n' "$c" >&2
            rc=1
        fi
    done

    if [ "$rc" -eq 0 ]; then
        printf '   [PASS] 六组同一内核：%s\n' "${base#Linux version }"
    fi

    return "$rc"
}

run_judge()
{
    local name="$1" log="$2" rc=0

    case "$name" in
    ok)            judge_ok "$log" || rc=1 ;;
    bad-root)      judge_bad_root "$log" || rc=1 ;;
    bad-type)      judge_bad_type "$log" || rc=1 ;;
    bad-init)      judge_bad_init "$log" || rc=1 ;;
    bad-console)   judge_bad_console "$log" || rc=1 ;;
    ok-fixed)      judge_ok_fixed "$log" || rc=1 ;;
    *) printf '   未知用例：%s\n' "$name" >&2; return 2 ;;
    esac

    if [ "$rc" -eq 0 ]; then
        printf '   [PASS] %s\n' "$name"
    else
        printf '   [FAIL] %s\n' "$name" >&2
    fi

    return "$rc"
}

# ---------- 用例定义 ----------

CASES=(ok bad-root bad-type bad-init bad-console ok-fixed)

declare -A APPEND DISK
APPEND[ok]="$CONSOLE root=/dev/vda rootfstype=ext4 rw"
APPEND[bad-root]="$CONSOLE root=/dev/vdz rootfstype=ext4 rw"
APPEND[bad-type]="$CONSOLE root=/dev/vda rootfstype=ext2 rw"
APPEND[bad-init]="$CONSOLE root=/dev/vda rootfstype=ext4 rw init=/nope"
APPEND[bad-console]="console=ttyS9 $CONSOLE root=/dev/vda rootfstype=ext4 rw"
APPEND[ok-fixed]="$CONSOLE root=/dev/vda rootfstype=ext4 rw"

DISK[ok]=1
DISK[bad-root]=1
DISK[bad-type]=1
DISK[bad-init]=1
DISK[bad-console]=1
DISK[ok-fixed]=1

# panic 用例不会自己退出（-no-reboot 只禁止重启），等满 60 s 纯属浪费。
# 20 s 远大于实测"启动到 panic"的耗时，同时保留"日志被截断"这种失败形态。
case_timeout()
{
    case "$1" in
    bad-root | bad-type | bad-init) printf '20' ;;
    *)                              printf '%s' "$BOOT_TIMEOUT" ;;
    esac
}

# ---------- 入口 ----------

check_env

if [ "${1:-}" = "--judge" ]; then
    shift
    if [ $# -eq 0 ]; then
        rc=0
        for c in "${CASES[@]}"; do
            run_judge "$c" "$LOGS/$c.log" || rc=1
        done
        exit "$rc"
    fi
    [ $# -ge 1 ] || die "用法: $0 --judge <用例> [日志]"
    run_judge "$1" "${2:-$LOGS/$1.log}"
    exit $?
fi

build_bins
build_rootfs

if [ $# -gt 0 ]; then
    case " ${CASES[*]} " in
    *" $1 "*) run_case "$1" "${APPEND[$1]}" "${DISK[$1]}" "$( case_timeout "$1" )" ;;
    *) die "未知用例：$1；可用：${CASES[*]}" ;;
    esac
    exit 0
fi

FAIL=0
for c in "${CASES[@]}"; do
    run_case "$c" "${APPEND[$c]}" "${DISK[$c]}" "$( case_timeout "$c" )"
    run_judge "$c" "$LOGS/$c.log" || FAIL=1
done

judge_same_kernel || FAIL=1

if [ "$FAIL" -eq 0 ]; then
    printf '\n全部通过\n'
else
    printf '\n有失败项\n' >&2
fi

exit "$FAIL"
