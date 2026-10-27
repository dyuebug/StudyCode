#!/usr/bin/env bash
# BSP_03 两阶段启动与切根：四组对照（在 WSL / Linux 下运行）
#
# 用法：
#   ./run.sh              # 跑全部五组，逐组机器判定
#   ./run.sh ok           # 切根成功
#   ./run.sh rootfs-ramfs # 同一套东西，命令行加 root= → rootfs 后端从 tmpfs 变 ramfs
#   ./run.sh no-move      # 跳过 MS_MOVE（chroot 但仍换根），证明搬运不能省
#   ./run.sh noroot       # 不给块设备：新根挂载失败
#   ./run.sh no-stage2    # 新根上没有 /stage2：exec 失败
#
#   ./run.sh --judge ok [日志]   # 只判定不跑 QEMU，用于对日志做变异验证判据
#
# 前置：
#   - qemu-system-arm >= 10.x
#   - arm-linux-gnueabihf-gcc、mke2fs、cpio、gzip
#   - 已构建的 ARMv7 内核树（默认见 KERNEL_TREE）
#
# 为什么五组而不是一组：正向用例只能证明"这条路能走通"。切根链路上有两个
# 容易混淆的失败点——新根没挂上、第二阶段的程序不存在——两者的日志完全不同；
# no-move 组是唯一能证明 MS_MOVE 必要性的证据（只 chroot 也能换根，但会丢设备节点）；
# rootfs-ramfs 组用同一份 initramfs 证明"切根前 / 的 f_type"取决于内核命令行，
# 免得把 tmpfs 魔数当成放之四海皆准的结论。

set -uo pipefail

SCRIPT_DIR="$( cd "$( dirname "$0" )" && pwd )"
WORK="$SCRIPT_DIR/build"
LOGS="$SCRIPT_DIR/logs"

KERNEL_TREE="${KERNEL_TREE:-$HOME/kbuild/linux-source-7.0.0}"
ZIMAGE="$KERNEL_TREE/arch/arm/boot/zImage"
QEMU_BIN="${QEMU_BIN:-qemu-system-arm}"
CROSS_CC="${CROSS_CC:-arm-linux-gnueabihf-gcc}"
BOOT_TIMEOUT="${BOOT_TIMEOUT:-60}"

MARKER_TEXT="bsp03-rootfs-marker"

die()
{
    printf '!! %s\n' "$*" >&2
    exit 1
}

check_env()
{
    local miss=0 c

    for c in "$QEMU_BIN" "$CROSS_CC" mke2fs cpio gzip; do
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

    local f
    for f in stage1 stage2; do
        file "$SCRIPT_DIR/$f" | grep -q 'ARM' \
            || die "$f 不是 ARM 二进制：$(file -b "$SCRIPT_DIR/$f")"
    done
}

# initramfs：只放 /init（=stage1）与 /mnt（留给新根的挂载点）
build_initramfs()
{
    local root="$WORK/root_initrd"

    rm -rf "$root"
    mkdir -p "$root/mnt"
    cp "$SCRIPT_DIR/stage1" "$root/init"
    chmod 0755 "$root/init"

    ( cd "$root" && find . -print0 | cpio --null -o -H newc 2>/dev/null | gzip -9 ) \
        > "$WORK/initramfs.cpio.gz" || die "initramfs 打包失败"
    [ -s "$WORK/initramfs.cpio.gz" ] || die "initramfs 为空"
}

# 真实根：放 stage2、标记文件，以及三个挂载点目录（MS_MOVE 的目标必须已存在）。
# $1 = 输出镜像名  $2 = 是否放 stage2（1/0）
build_rootfs()
{
    local img="$WORK/$1" with_stage2="$2"
    local root="$WORK/root_$( basename "$img" .ext4 )"

    rm -rf "$root"
    mkdir -p "$root/dev" "$root/proc" "$root/sys" "$root/mnt"

    [ "$with_stage2" = "1" ] && { cp "$SCRIPT_DIR/stage2" "$root/stage2"; chmod 0755 "$root/stage2"; }

    printf '%s\n' "$MARKER_TEXT" > "$root/marker"

    rm -f "$img"
    truncate -s 8M "$img"
    mke2fs -q -t ext4 -F -d "$root" "$img" || die "mke2fs 失败: $img"
}

# ---------- 运行 ----------

# $1=用例名  $2=内核命令行  $3=是否挂块设备（1/0）  $4=块设备镜像名（可空）
run_case()
{
    local name="$1" append="$2" with_disk="$3" img="${4:-root_ok.ext4}"
    local log="$LOGS/$name.log"
    local args=(-M virt -m 512M -smp 1 -display none -monitor none -serial stdio -no-reboot
                -kernel "$ZIMAGE" -initrd "$WORK/initramfs.cpio.gz" -append "$append")

    if [ "$with_disk" = "1" ]; then
        args+=(-drive "file=$WORK/$img,format=raw,if=none,id=root0"
               -device virtio-blk-device,drive=root0)
    fi

    printf '\n===== %s =====\n' "$name"
    printf '   append: %s\n' "$append"
    [ "$with_disk" = "1" ] && printf '   disk:   %s\n' "$img"

    timeout "$BOOT_TIMEOUT" "$QEMU_BIN" "${args[@]}" > "$log" 2>&1
    printf '   exit=%s  log=%s\n' "$?" "${log#"$SCRIPT_DIR/"}"

    if grep -aq '^SWROOT|' "$log"; then
        grep -a '^SWROOT|' "$log" | sed 's/^/   /'
    else
        printf '   （没有 SWROOT| 行）\n'
    fi
}

# ---------- 判定 ----------

has()  { grep -aqF "$2" "$1"; }
lacks() { ! grep -aqF "$2" "$1"; }

# 逐条检查，任一不满足即失败，并把缺失项打印出来（不靠人眼读整份日志）
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

judge_ok()
{
    # 判据全部来自实测，不用教科书的旧结论：
    #   root_before 是 tmpfs 而不是 ramfs —— 本内核在"命令行没有 root="且 CONFIG_TMPFS=y"
    #     时把 rootfs 建在 shmem 上（init/do_mounts.c:init_rootfs），f_type 报 TMPFS_MAGIC。
    #   dev_type 也是 tmpfs —— devtmpfs 以内核 shmem 为后端，statfs 继承后端实现。
    #     所以"搬运成功"不能靠 dev_type，要靠 /proc/self/mounts 的类型串与 /dev/vda 是否存在。
    expect "$1" \
        'SWROOT|stage|1|pid=1' \
        'SWROOT|mount|devtmpfs|ok' \
        'SWROOT|cmdline|' \
        'SWROOT|root_before|type=tmpfs|magic=0x1021994' \
        'SWROOT|mount|newroot|ok' \
        'SWROOT|newroot_marker|bsp03-rootfs-marker' \
        'SWROOT|move|/dev|ok' \
        'SWROOT|chroot|ok' \
        'SWROOT|stage|2|' \
        'SWROOT|root_after|type=ext4|magic=0xef53' \
        'SWROOT|mounts_root|ext4' \
        'SWROOT|mounts_dev|devtmpfs' \
        'SWROOT|marker|bsp03-rootfs-marker' \
        'SWROOT|old_init|gone' \
        'SWROOT|dev_vda|present' \
        'SWROOT|stage2|done' \
    && refute "$1" 'SWROOT|fail|'
}

# 同一份 initramfs、同一份镜像，只在命令行加 root=/dev/vda：
# init_rootfs() 因此不再选 shmem，rootfs 回落到 ramfs，f_type 从 0x1021994 变成 0x858458f6。
# 这一组存在的意义：证明上面那条"root_before=tmpfs"是内核行为，不是我的观察错误。
judge_rootfs_ramfs()
{
    expect "$1" \
        'SWROOT|root_before|type=ramfs|magic=0x858458f6' \
        'SWROOT|mount|newroot|ok' \
        'SWROOT|root_after|type=ext4|magic=0xef53' \
        'SWROOT|stage2|done'
}

judge_no_move()
{
    # 只 chroot 也能换根（root_after 仍是 ext4），但 /dev 只剩新根上的空目录：
    # 判定点从"根换了没有"变成"搬运做了没有"。
    # mounts_dev 读不到是必然的——/proc 没搬过去，/proc/self/mounts 不存在。
    expect "$1" \
        'SWROOT|move|/dev|skipped' \
        'SWROOT|chroot|ok' \
        'SWROOT|stage|2|' \
        'SWROOT|root_after|type=ext4|magic=0xef53' \
        'SWROOT|dev_type|type=ext4|magic=0xef53' \
        'SWROOT|mounts_dev|absent' \
        'SWROOT|dev_vda|absent' \
        'SWROOT|stage2|done'
}

judge_noroot()
{
    expect "$1" \
        'SWROOT|mount|newroot|fail|errno=2' \
        'SWROOT|stage2|skipped' \
    && refute "$1" 'SWROOT|stage|2|'
}

judge_no_stage2()
{
    expect "$1" \
        'SWROOT|mount|newroot|ok' \
        'SWROOT|chroot|ok' \
        'SWROOT|exec|fail|path=/stage2|errno=2' \
        'SWROOT|stage2|skipped' \
    && refute "$1" 'SWROOT|stage|2|'
}

# ---------- 用例表 ----------

declare -A APPEND=(
    [ok]='console=ttyAMA0 rdinit=/init panic=1'
    [rootfs-ramfs]='console=ttyAMA0 rdinit=/init root=/dev/vda rootfstype=ext4 panic=1'
    [no-move]='console=ttyAMA0 rdinit=/init panic=1 swroot=nomove'
    [noroot]='console=ttyAMA0 rdinit=/init panic=1'
    [no-stage2]='console=ttyAMA0 rdinit=/init panic=1'
)
declare -A DISK=(
    [ok]=1
    [rootfs-ramfs]=1
    [no-move]=1
    [noroot]=0
    [no-stage2]=1
)
# no-stage2 用一份"少了 /stage2"的镜像，其余用例共用完整镜像
declare -A IMG=(
    [ok]=root_ok.ext4
    [rootfs-ramfs]=root_ok.ext4
    [no-move]=root_ok.ext4
    [noroot]=
    [no-stage2]=root_no_stage2.ext4
)
declare -A JUDGE=(
    [ok]=judge_ok
    [rootfs-ramfs]=judge_rootfs_ramfs
    [no-move]=judge_no_move
    [noroot]=judge_noroot
    [no-stage2]=judge_no_stage2
)

CASES=(ok rootfs-ramfs no-move noroot no-stage2)

main()
{
    local c rc failed=0

    # 只判定不跑 QEMU：对已有日志做变异，确认判据真的会失败（不重跑、不改仓库文件）。
    # 退出码沿用判定结果：0 通过 / 1 失败 / 2 日志不存在
    if [ "${1:-}" = "--judge" ]; then
        c="${2:?用法: ./run.sh --judge <用例> [日志]}"
        local log="${3:-$LOGS/$c.log}"

        case " ${CASES[*]} " in
            *" $c "*) ;;
            *) die "未知用例: $c（可选: ${CASES[*]}）" ;;
        esac

        [ -f "$log" ] || { printf '!! 日志不存在: %s\n' "$log" >&2; exit 2; }

        if "${JUDGE[$c]}" "$log"; then
            printf '[PASS] %s <- %s\n' "$c" "$log"
            exit 0
        fi

        printf '[FAIL] %s <- %s\n' "$c" "$log"
        exit 1
    fi

    check_env
    build_bins
    build_initramfs
    build_rootfs root_ok.ext4 1
    build_rootfs root_no_stage2.ext4 0

    printf '== 产物 ==\n'
    printf '   stage1          %s B\n' "$( stat -c %s "$SCRIPT_DIR/stage1" )"
    printf '   stage2          %s B\n' "$( stat -c %s "$SCRIPT_DIR/stage2" )"
    printf '   initramfs.cpio.gz %s B\n' "$( stat -c %s "$WORK/initramfs.cpio.gz" )"
    printf '   root_ok.ext4    %s B\n' "$( stat -c %s "$WORK/root_ok.ext4" )"

    [ $# -eq 0 ] && set -- "${CASES[@]}"

    for c in "$@"; do
        case " ${CASES[*]} " in
            *" $c "*) ;;
            *) die "未知用例: $c（可选: ${CASES[*]}）" ;;
        esac

        run_case "$c" "${APPEND[$c]}" "${DISK[$c]}" "${IMG[$c]}"

        printf '   ---- 判定 %s ----\n' "$c"
        if "${JUDGE[$c]}" "$LOGS/$c.log"; then
            printf '   [PASS] %s\n' "$c"
        else
            printf '   [FAIL] %s\n' "$c"
            failed=$(( failed + 1 ))
        fi
    done

    printf '\n合计：%d 组通过，%d 组失败\n' "$(( $# - failed ))" "$failed"
    printf '日志目录: %s\n' "$LOGS"

    [ "$failed" -eq 0 ]
}

main "$@"
