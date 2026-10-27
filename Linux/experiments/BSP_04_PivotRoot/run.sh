#!/usr/bin/env bash
# BSP_04 pivot_root 与 chroot+exec 的同口径对照（在 WSL / Linux 下运行）
#
# 用法：
#   ./run.sh                 # 跑全部五组，逐组机器判定
#   ./run.sh chroot          # 基线：只 chroot + exec（BSP_03 的做法）
#   ./run.sh pivot           # pivot_root + chdir + 卸载旧根（stage1 里卸）
#   ./run.sh pivot-late      # pivot_root + chdir；在 exec 之后做"旧根被谁钉住"的因果实验
#   ./run.sh pivot-nomove    # 跳过 MS_MOVE 再 pivot：证明 pivot_root 不搬挂载点
#   ./run.sh pivot-ondir     # 对普通目录 pivot：证明 new_root 必须是挂载点（预期 EINVAL）
#
#   ./run.sh --judge <用例> [日志]   # 只判定不跑 QEMU，用于对日志做变异验证判据
#
# 前置：qemu-system-arm >= 10.x、arm-linux-gnueabihf-gcc、mke2fs、cpio、gzip，
#       以及已构建的 ARMv7 内核树（默认 ~/kbuild/linux-source-7.0.0）。
#
# 四组的差别被压到最小：同一份 stage1/stage2、同一份 initramfs、同一份 ext4 镜像，
# 只有命令行里的 proot=<mode> 不同。这样才能说"差异来自 pivot_root 本身"，
# 而不是"两条不同的脚本各自跑出来的结果"。

set -uo pipefail

SCRIPT_DIR="$( cd "$( dirname "$0" )" && pwd )"
WORK="$SCRIPT_DIR/build"
LOGS="$SCRIPT_DIR/logs"

KERNEL_TREE="${KERNEL_TREE:-$HOME/kbuild/linux-source-7.0.0}"
ZIMAGE="$KERNEL_TREE/arch/arm/boot/zImage"
QEMU_BIN="${QEMU_BIN:-qemu-system-arm}"
CROSS_CC="${CROSS_CC:-arm-linux-gnueabihf-gcc}"
BOOT_TIMEOUT="${BOOT_TIMEOUT:-60}"

MARKER_TEXT="bsp04-pivot-marker"

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

# 真实根：stage2、标记文件、三个挂载点目录（MS_MOVE 目标必须已存在），
# 以及 pivot-ondir 用到的普通子目录 /sub。
build_rootfs()
{
    local root="$WORK/root_ext4"

    rm -rf "$root"
    mkdir -p "$root/dev" "$root/proc" "$root/sys" "$root/mnt" "$root/sub"

    cp "$SCRIPT_DIR/stage2" "$root/stage2"
    chmod 0755 "$root/stage2"

    printf '%s\n' "$MARKER_TEXT" > "$root/marker"

    rm -f "$WORK/root_ext4.img"
    truncate -s 8M "$WORK/root_ext4.img"
    mke2fs -q -t ext4 -F -d "$root" "$WORK/root_ext4.img" || die "mke2fs 失败"
}

# ---------- 运行 ----------

run_case()
{
    local name="$1" append="$2"
    local log="$LOGS/$name.log"
    local args=(-M virt -m 512M -smp 1 -display none -monitor none -serial stdio -no-reboot
                -kernel "$ZIMAGE" -initrd "$WORK/initramfs.cpio.gz" -append "$append"
                -drive "file=$WORK/root_ext4.img,format=raw,if=none,id=root0"
                -device virtio-blk-device,drive=root0)

    printf '\n===== %s =====\n' "$name"
    printf '   append: %s\n' "$append"

    timeout "$BOOT_TIMEOUT" "$QEMU_BIN" "${args[@]}" > "$log" 2>&1
    printf '   exit=%s  log=%s\n' "$?" "${log#"$SCRIPT_DIR/"}"
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

# chroot 基线：本进程的 / 已是 ext4，但**命名空间**仍以旧根为 /。
# 判据的核心是观察者那一对读数，不是本进程的读数：
#   watcher_root=tmpfs + watcher_init=visible  → 旧根还在，其它进程仍从它出发
# 另外 pin 一条实测出来的反直觉事实：从 chroot 内部读 /proc/self/mounts 时，
# 旧根那一行被内核隐藏（mounts_rootfs=0），所以挂载表**不能**用来判断
# "chroot 有没有释放旧根"。这条写成断言是为了防止有人日后拿它当证据。
judge_chroot()
{
    expect "$1" \
        'PROOT|mode|chroot' \
        'PROOT|root_before|type=tmpfs|magic=0x1021994' \
        'PROOT|mount|newroot|ok' \
        'PROOT|move|/dev|ok' \
        'PROOT|chroot|ok' \
        'PROOT|stage|2|' \
        'PROOT|root_after|type=ext4|magic=0xef53' \
        'PROOT|mounts_dev|devtmpfs' \
        'PROOT|mounts_rootfs|0' \
        'PROOT|watcher_root|type=tmpfs|magic=0x1021994' \
        'PROOT|watcher_init|visible' \
        'PROOT|dev_vda|present' \
        'PROOT|stage2|done' \
    && refute "$1" 'PROOT|fail|'
}

# pivot：换根成功，且换根对**整个命名空间**生效。
#   watcher_root=ext4 + watcher_init=gone → 观察者的 / 也被改到新根
# umount_oldroot 的 ok/busy 两种结果都接受（只要求出现过），但"最后必须真摘掉"由
# umount_oldroot_detach|ok 保证：严格 umount 在本实验里实测 EBUSY(16)，
# 原因是旧根仍被引用（stage1 的 exe_file 与继承来的 0/1/2），
# 直接原因是后者 —— pivot-late 组把它单独证实了。故实际切换要用 MNT_DETACH。
judge_pivot()
{
    expect "$1" \
        'PROOT|mode|pivot' \
        'PROOT|mount|newroot|ok' \
        'PROOT|pivot_root|ok' \
        'PROOT|pivot_chdir|ok' \
        'PROOT|umount_oldroot|' \
        'PROOT|umount_oldroot_detach|ok' \
        'PROOT|stage|2|' \
        'PROOT|root_after|type=ext4|magic=0xef53' \
        'PROOT|mounts_root|ext4' \
        'PROOT|mounts_dev|devtmpfs' \
        'PROOT|watcher_root|type=ext4|magic=0xef53' \
        'PROOT|watcher_init|gone' \
        'PROOT|dev_vda|present' \
        'PROOT|stage2|done' \
    && refute "$1" 'PROOT|fail|' 'PROOT|pivot_root|fail'
}

# 跳过 MS_MOVE 再 pivot：根与命名空间都换到 ext4，但 /dev 里没有任何设备节点。
# 与 BSP_03 的 no-move 组同一条结论：pivot_root 换的是"根"，不是"挂载点名册"。
judge_pivot_nomove()
{
    expect "$1" \
        'PROOT|mode|pivot-nomove' \
        'PROOT|move|/dev|skipped' \
        'PROOT|pivot_root|ok' \
        'PROOT|watcher_root|type=ext4|magic=0xef53' \
        'PROOT|stage|2|' \
        'PROOT|root_after|type=ext4|magic=0xef53' \
        'PROOT|dev_type|type=ext4|magic=0xef53' \
        'PROOT|mounts|absent' \
        'PROOT|dev_vda|absent' \
        'PROOT|stage2|done'
}

# 对普通目录 pivot：new_root 不是挂载点 → EINVAL(22)，第二阶段不会启动。
# 观察者读数同时证明"失败没有改变任何东西"：它仍看到 tmpfs 与 /init。
judge_pivot_ondir()
{
    expect "$1" \
        'PROOT|mode|pivot-ondir' \
        'PROOT|pivot_root|fail|new=/mnt/newroot/sub|put=/mnt/newroot/sub/oldroot|errno=22' \
        'PROOT|watcher_root|type=tmpfs|magic=0x1021994' \
        'PROOT|watcher_init|visible' \
        'PROOT|stage2|skipped' \
    && refute "$1" 'PROOT|pivot_root|ok' 'PROOT|stage|2|' 'PROOT|pivot_ondir|unexpected_ok'
}

# pivot-late：切根后不立刻卸载，交给已在新根上运行的 stage2 去卸。
#
# 原先的假设是"stage1 的可执行映像钉住旧根，所以 exec 之后就能严格卸载"。
# 实测**否证**：exec 之后仍 EBUSY(16)。于是这一组改成真正的因果实验：
#   ① 继承来的 0/1/2 指向 /oldroot/dev/console（旧 rootfs 里的 console）
#   ② 带着这三个 fd 严格卸载 → 必 EBUSY
#   ③ 只把 0/1/2 重绑到新根的 /dev/console，别的什么都不改 → 严格卸载应当成功
# ②③ 同进程、同挂载、相隔几毫秒，差异只有 stdio 绑定，因果才成立。
# 同时 pin 两个排除项：
#   ctrl|tmpfs|umount_ok      —— 严格 umount 在本命名空间本身可用
#   mountinfo 里没有以旧根 id 为父的挂载 —— 旧根没有子挂载（子挂载是 EBUSY 的另一来源）
judge_pivot_late()
{
    expect "$1" \
        'PROOT|mode|pivot-late' \
        'PROOT|boot_console|present|mode=020600' \
        'PROOT|pivot_root|ok' \
        'PROOT|pivot_chdir|ok' \
        'PROOT|umount_oldroot|deferred|to=stage2' \
        'PROOT|fd|1|/oldroot/dev/console' \
        'PROOT|late_umount_keep_stdio|busy|errno=16' \
        'PROOT|ctrl|tmpfs|umount_ok' \
        'PROOT|mounts|' \
        'PROOT|mounts_rootfs|1' \
        'PROOT|stdio_rebind|ok' \
        'PROOT|fd|1|/dev/console' \
        'PROOT|late_umount_after_rebind|ok' \
        'PROOT|stage|2|' \
        'PROOT|root_after|type=ext4|magic=0xef53' \
        'PROOT|mounts_rootfs|0' \
        'PROOT|dev_vda|present' \
        'PROOT|stage2|done' \
    && refute "$1" 'PROOT|fail|' 'PROOT|late_umount_after_rebind|busy'
}

# ---------- 用例表 ----------

declare -A APPEND=(
    [chroot]='console=ttyAMA0 rdinit=/init panic=1 proot=chroot'
    [pivot]='console=ttyAMA0 rdinit=/init panic=1 proot=pivot'
    [pivot-late]='console=ttyAMA0 rdinit=/init panic=1 proot=pivot-late'
    [pivot-nomove]='console=ttyAMA0 rdinit=/init panic=1 proot=pivot-nomove'
    [pivot-ondir]='console=ttyAMA0 rdinit=/init panic=1 proot=pivot-ondir'
)
declare -A JUDGE=(
    [chroot]=judge_chroot
    [pivot]=judge_pivot
    [pivot-late]=judge_pivot_late
    [pivot-nomove]=judge_pivot_nomove
    [pivot-ondir]=judge_pivot_ondir
)

CASES=(chroot pivot pivot-late pivot-nomove pivot-ondir)

main()
{
    local c rc failed=0

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
    build_rootfs

    printf '== 产物 ==\n'
    printf '   stage1            %s B\n' "$( stat -c %s "$SCRIPT_DIR/stage1" )"
    printf '   stage2            %s B\n' "$( stat -c %s "$SCRIPT_DIR/stage2" )"
    printf '   initramfs.cpio.gz %s B\n' "$( stat -c %s "$WORK/initramfs.cpio.gz" )"
    printf '   root_ext4.img     %s B\n' "$( stat -c %s "$WORK/root_ext4.img" )"

    [ $# -eq 0 ] && set -- "${CASES[@]}"

    for c in "$@"; do
        case " ${CASES[*]} " in
            *" $c "*) ;;
            *) die "未知用例: $c（可选: ${CASES[*]}）" ;;
        esac

        run_case "$c" "${APPEND[$c]}"

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
