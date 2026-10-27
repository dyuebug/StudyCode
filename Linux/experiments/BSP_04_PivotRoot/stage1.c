/* BSP_04 第一阶段：initramfs 里的 init（PID 1）
 *
 * 目的：把 `pivot_root` 与 `chroot + exec` 放在**同一段代码**里做同口径对照，
 * 只由内核命令行的一个参数决定走哪条路，免得"两条不同的脚本"这种不成立的对照。
 *
 *   公共前半段（两组完全一致）：
 *     ① mount devtmpfs /proc /sys      ← DEVTMPFS_MOUNT 未设，必须自己挂
 *     ② mount("/dev/vda", "/mnt/newroot", "ext4")
 *     ③ 用 MS_MOVE 把 ① 的三个挂载点搬进新根
 *
 *   分叉（命令行 proot=<mode>）：
 *     chroot        chroot(NEWROOT) → chdir("/") → exec /stage2
 *     pivot         pivot_root(NEWROOT, NEWROOT"/oldroot") → chdir("/")
 *                   → 试严格 umount 旧根，失败再 MNT_DETACH → exec /stage2
 *     pivot-nomove  同 pivot，但跳过 ③：证明 pivot_root **不**搬挂载点
 *     pivot-ondir   对新根下的**普通目录** pivot：证明 new_root 必须是挂载点（预期 EINVAL）
 *
 * 三个要回答的问题：
 *   1. 只 chroot 时旧根是不是还在？ → stage2 把 /proc/self/mounts 全表 dump 出来，不靠猜。
 *   2. pivot_root 之后能不能真正卸载旧根？ → 严格 umount 与 MNT_DETACH 各试一次，
 *      两者结果都由日志给出；"要不要先 exec" 这个结论从这里来。
 *   3. pivot_root 需不需要提前搬 /dev /proc /sys？ → pivot-nomove 组。
 *
 * 输出约定见 proot.h。
 */
#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "proot.h"

#define NEWROOT "/mnt/newroot"
#define PUTOLD  "/oldroot"      /* 相对新根 */
#define SUBDIR  "/sub"          /* pivot-ondir 用：ext4 上的普通目录，不是挂载点 */
#define STAGE2  "/stage2"
#define MARKER  "/marker"
#define DEVNODE "/dev/vda"
#define MAXBUF  512

static int read_all( const char *path, char *buf, size_t cap )
{
    int fd = open( path, O_RDONLY );

    if( fd < 0 )
    {
        return -1;
    }

    ssize_t n = read( fd, buf, cap - 1U );

    close( fd );

    if( n < 0 )
    {
        return -1;
    }

    buf[n] = '\0';
    return ( int ) n;
}

/* 取 key=<value> 形式的参数值（key 含 '='）。没有就返回 fallback。
 * 只认整词：避免 "xproot=pivot" 被误当成本实验的参数。 */
static const char *cmdline_value( const char *key, const char *fallback )
{
    static char  buf[MAXBUF];
    static char  val[64];
    const size_t klen = strlen( key );
    int          n    = read_all( "/proc/cmdline", buf, sizeof buf );

    if( n <= 0 )
    {
        return fallback;
    }

    char *p = buf;

    while( *p != '\0' )
    {
        while( *p == ' ' || *p == '\t' || *p == '\n' )
        {
            p++;
        }

        char *tok = p;

        while( *p != '\0' && *p != ' ' && *p != '\t' && *p != '\n' )
        {
            p++;
        }

        const size_t len = ( size_t ) ( p - tok );

        if( len > klen && strncmp( tok, key, klen ) == 0 )
        {
            size_t vlen = len - klen;

            if( vlen >= sizeof val )
            {
                vlen = sizeof val - 1U;
            }

            memcpy( val, tok + klen, vlen );
            val[vlen] = '\0';
            return val;
        }
    }

    return fallback;
}

static void show_file( const char *tag, const char *path )
{
    static char buf[MAXBUF];
    int         n = read_all( path, buf, sizeof buf );

    if( n < 0 )
    {
        pr_out( "PROOT|%s|absent\n", tag );
        return;
    }

    while( n > 0 && ( buf[n - 1] == '\n' || buf[n - 1] == '\r' ) )
    {
        buf[--n] = '\0';
    }

    pr_out( "PROOT|%s|%s\n", tag, buf );
}

/* 失败收尾：先留痕再断电。PID 1 return 会 panic 出一大段无关栈，把证据淹掉。
 * 断电前等 400 ms：观察者（见 spawn_watcher）在 100 ms 时打印，
 * 不留给它窗口的话，pivot-ondir 这种"立刻失败"的组会丢一行动证据。 */
static void fail_and_poweroff( const char *where, int err )
{
    struct timespec ts = { 0, 400 * 1000 * 1000 };

    pr_out( "PROOT|fail|%s|errno=%d\n", where, err );
    pr_out( "PROOT|stage2|skipped\n" );

    nanosleep( &ts, NULL );

    sync();
    reboot( RB_POWER_OFF );

    for( ;; )
    {
        pause();
    }
}

static void ensure_dir( const char *path )
{
    if( mkdir( path, 0755 ) != 0 && errno != EEXIST )
    {
        pr_out( "PROOT|mkdir|%s|fail|errno=%d\n", path, errno );
    }
}

/* MS_MOVE 是"搬"（旧位置消失、挂载实例不变），不是"再挂一次"。 */
static void move_mountpoint( const char *sub )
{
    char dst[MAXBUF];

    snprintf( dst, sizeof dst, "%s%s", NEWROOT, sub );
    ensure_dir( dst );

    if( mount( sub, dst, NULL, MS_MOVE, NULL ) != 0 )
    {
        const int err = errno;

        pr_out( "PROOT|move|%s|fail|errno=%d\n", sub, err );
        return;
    }

    pr_out( "PROOT|move|%s|ok\n", sub );
}

/* pivot_root 在 glibc 里没有包装函数（它不是 POSIX），只能走 syscall()。
 * 返回值语义：0 成功；失败时把 errno 原样带出来给上层判断。 */
static int do_pivot( const char *newroot, const char *putold )
{
    if( syscall( SYS_pivot_root, newroot, putold ) != 0 )
    {
        const int err = errno;

        pr_out( "PROOT|pivot_root|fail|new=%s|put=%s|errno=%d\n", newroot, putold, err );
        return err;
    }

    pr_out( "PROOT|pivot_root|ok|new=%s|put=%s\n", newroot, putold );
    return 0;
}

/* 卸载旧根：先严格 umount，失败（EBUSY=16）再 MNT_DETACH。
 * 为什么两种都要试：严格失败本身是证据 —— 旧根仍被引用。本阶段至少有两处引用：
 *   ① 本进程的 mm->exe_file（/init 就在旧 rootfs 上）
 *   ② 内核交给 PID 1 的 0/1/2，指向旧 rootfs 里的 /dev/console，exec 换不掉
 * ② 是必要条件，由 pivot-late 组单独证实（只重绑 stdio，同一挂载就能严格卸载）；
 * ① 不是必要条件（exec 之后 exe_file 已换掉，EBUSY 依旧）。
 * MNT_DETACH 成功则说明"从命名空间摘掉"与"真正释放"是两件事。
 * 调用前必须已 chdir("/")，否则 cwd 落在旧根上也会钉住它。 */
static void umount_old( const char *putold )
{
    if( umount2( putold, 0 ) == 0 )
    {
        pr_out( "PROOT|umount_oldroot|ok\n" );
        return;
    }

    {
        const int err = errno;

        pr_out( "PROOT|umount_oldroot|busy|errno=%d\n", err );

        if( umount2( putold, MNT_DETACH ) == 0 )
        {
            pr_out( "PROOT|umount_oldroot_detach|ok\n" );
        }
        else
        {
            pr_out( "PROOT|umount_oldroot_detach|fail|errno=%d\n", errno );
        }
    }
}

/* 命名空间内的观察者：证明 chroot 与 pivot_root 的差别**在进程之外**。
 *
 * 为什么必须有它：chroot 与 pivot_root 换完之后，第二阶段的读数完全一样
 * （/ 是 ext4、/dev 是 devtmpfs、设备节点在、旧 /init 看不到），连挂载表都一样
 * ——从 chroot 内部读 /proc/self/mounts 时，旧根那一行被内核隐藏了，看不见。
 * 二者真正的差别是：chroot 只改**本进程**的 fs 根，命名空间与其它进程仍以旧根为 /；
 * pivot_root 会把命名空间里所有"根指向旧根"的进程一起改到新根。
 *
 * 所以在切换前 fork 一个子进程，让它等 100 ms（此时父进程已经切完）再看自己的 "/"：
 *   chroot 组 → 子进程的 / 仍是 tmpfs（旧根还在，只是父进程看不见它）
 *   pivot  组 → 子进程的 / 变为 ext4（命名空间一起换根）
 * 观察者不自称"证明"，只报自己的读数；结论由两组对照得出。 */
static void spawn_watcher( void )
{
    pid_t pid = fork();

    if( pid < 0 )
    {
        pr_out( "PROOT|watcher|fork_fail|errno=%d\n", errno );
        return;
    }

    if( pid != 0 )
    {
        pr_out( "PROOT|watcher|pid=%d\n", ( int ) pid );
        return;
    }

    struct timespec ts = { 0, 100 * 1000 * 1000 };
    struct stat     st;

    nanosleep( &ts, NULL );

    pr_report_fs( "watcher_root", "/" );

    if( stat( "/init", &st ) == 0 )
    {
        pr_out( "PROOT|watcher_init|visible\n" );
    }
    else
    {
        pr_out( "PROOT|watcher_init|gone\n" );
    }

    for( ;; )
    {
        pause();
    }
}

static void exec_stage2( void )
{
    char *const argv[] = { ( char * ) STAGE2, NULL };
    char *const envp[] = { NULL };

    execve( STAGE2, argv, envp );

    pr_out( "PROOT|exec|fail|path=%s|errno=%d\n", STAGE2, errno );
    fail_and_poweroff( "exec_stage2", errno );
}

int main( void )
{
    const char *mode;

    pr_out( "PROOT|stage|1|pid=%d\n", ( int ) getpid() );

    /* 探针：进 init 时旧根里到底有没有 /dev/console。
     * 节点由 rootfs_initcall(default_rootfs) 在 rootfs 里创建（init/noinitramfs.c:18，
     * 该文件是 obj-y，恒编译）；内核在 exec init 之前调用 console_on_rootfs()
     * 打开它并 dup 到 0/1/2（init/main.c:1603），这三个 fd 之后跟着进程走、exec 也换不掉。
     * pivot-late 的因果结论就架在这条链上，所以让读数自己说话：
     * 预期 present，且 mode 必须是 020600（字符设备 + 0600）。 */
    {
        struct stat st;

        if( stat( "/dev/console", &st ) == 0 )
        {
            pr_out( "PROOT|boot_console|present|mode=0%o\n", ( unsigned ) st.st_mode );
        }
        else
        {
            pr_out( "PROOT|boot_console|absent|errno=%d\n", errno );
        }
    }

    ensure_dir( "/dev" );
    ensure_dir( "/proc" );
    ensure_dir( "/sys" );

    if( mount( "devtmpfs", "/dev", "devtmpfs", 0, "" ) != 0 )
    {
        pr_out( "PROOT|mount|devtmpfs|fail|errno=%d\n", errno );
    }
    else
    {
        pr_out( "PROOT|mount|devtmpfs|ok\n" );
    }

    if( mount( "proc", "/proc", "proc", 0, "" ) != 0 )
    {
        pr_out( "PROOT|mount|proc|fail|errno=%d\n", errno );
    }
    else
    {
        pr_out( "PROOT|mount|proc|ok\n" );
    }

    if( mount( "sysfs", "/sys", "sysfs", 0, "" ) != 0 )
    {
        pr_out( "PROOT|mount|sysfs|fail|errno=%d\n", errno );
    }
    else
    {
        pr_out( "PROOT|mount|sysfs|ok\n" );
    }

    /* 切根前的基线 + 本组走哪条路。命令行要等 /proc 挂上才读得到。 */
    pr_report_fs( "root_before", "/" );
    show_file( "cmdline", "/proc/cmdline" );

    mode = cmdline_value( "proot=", "pivot" );
    pr_out( "PROOT|mode|%s\n", mode );

    ensure_dir( NEWROOT );

    if( mount( DEVNODE, NEWROOT, "ext4", 0, "" ) != 0 )
    {
        const int err = errno;

        pr_out( "PROOT|mount|newroot|fail|errno=%d\n", err );
        fail_and_poweroff( "mount_newroot", err );
    }

    pr_out( "PROOT|mount|newroot|ok\n" );
    show_file( "newroot_marker", NEWROOT MARKER );

    /* ③ 搬挂载点。pivot-nomove 故意跳过：pivot_root 换的是"根"，不是"挂载点名册"。 */
    if( strcmp( mode, "pivot-nomove" ) == 0 )
    {
        pr_out( "PROOT|move|/dev|skipped\n" );
        pr_out( "PROOT|move|/proc|skipped\n" );
        pr_out( "PROOT|move|/sys|skipped\n" );
    }
    else
    {
        move_mountpoint( "/dev" );
        move_mountpoint( "/proc" );
        move_mountpoint( "/sys" );
    }

    /* 切换之前先放一个观察者：chroot 与 pivot_root 的差别只有它能看见。
     * pivot-late 组不放：那组要测"旧根被谁钉住"，观察者自己就带着
     * 可执行映射与继承来的 0/1/2 两个引用，会让这一组的结论失真，所以按模式关掉。 */
    if( strcmp( mode, "pivot-late" ) != 0 )
    {
        spawn_watcher();
    }

    if( strcmp( mode, "chroot" ) == 0 )
    {
        if( chroot( NEWROOT ) != 0 )
        {
            const int err = errno;

            pr_out( "PROOT|chroot|fail|errno=%d\n", err );
            fail_and_poweroff( "chroot", err );
        }

        if( chdir( "/" ) != 0 )
        {
            fail_and_poweroff( "chdir", errno );
        }

        pr_out( "PROOT|chroot|ok\n" );
        exec_stage2();
    }

    if( strcmp( mode, "pivot-ondir" ) == 0 )
    {
        /* 反例：/sub 是 ext4 上的普通目录，不是任何挂载点。
         * pivot_root 要求 new_root 必须是挂载点，这里预期 EINVAL(22)。
         * put_old 也必须在 new_root 之下，所以先建 /sub/oldroot。 */
        ensure_dir( NEWROOT SUBDIR );
        ensure_dir( NEWROOT SUBDIR PUTOLD );

        const int err = do_pivot( NEWROOT SUBDIR, NEWROOT SUBDIR PUTOLD );

        /* 预期是 EINVAL(22)。真成功反而说明前提条件不成立，必须显式记下来，
         * 不能让它悄悄走进后面的流程。 */
        if( err == 0 )
        {
            pr_out( "PROOT|pivot_ondir|unexpected_ok\n" );
        }

        fail_and_poweroff( "pivot_root_not_a_mountpoint", err );
    }

    /* pivot / pivot-nomove：put_old 必须是新根下的一个空目录。 */
    ensure_dir( NEWROOT PUTOLD );

    {
        const int err = do_pivot( NEWROOT, NEWROOT PUTOLD );

        if( err != 0 )
        {
            fail_and_poweroff( "pivot_root", err );
        }
    }

    /* 换根之后 cwd 仍是旧根的目录（内核拒绝 pivot 时要求如此），
     * 不 chdir 到新根就把旧根钉住了 —— 严格 umount 必然 EBUSY。 */
    if( chdir( "/" ) != 0 )
    {
        fail_and_poweroff( "chdir_after_pivot", errno );
    }

    pr_out( "PROOT|pivot_chdir|ok\n" );

    /* pivot-late：故意不在这里卸载，交给 exec 之后的 stage2 去做。
     * 用来回答"严格 umount 为什么 EBUSY"——如果 exec 之后就能卸掉，
     * 说明钉住旧根的是 stage1 自己的可执行映射，而不是 chdir 或残留的子挂载。 */
    if( strcmp( mode, "pivot-late" ) == 0 )
    {
        pr_out( "PROOT|umount_oldroot|deferred|to=stage2\n" );
        exec_stage2();
    }

    umount_old( PUTOLD );

    exec_stage2();

    return 0;
}
