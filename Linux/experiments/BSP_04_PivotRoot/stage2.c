/* BSP_04 第二阶段：真实根上的 init（PID 仍是 1）
 *
 * 职责：从**新根内部**回答"旧根还在不在"。核心手段是把 /proc/self/mounts 整表 dump 出来——
 * 不预设"chroot 之后旧根会怎样"，而是让内核自己说；判据从实测读数里取。
 *
 * 为什么 dump 全表而不是只查 /：
 *   只 chroot 时，进程的路径解析起点变了，但**挂载表是命名空间级的**，
 *   旧 rootfs 仍然在表里（类型字段写作 "rootfs"）。这一行在不在，就是
 *   "chroot 没释放旧根 / pivot_root + umount 释放了"的直接证据。
 *
 * 其余读数与 BSP_03 同口径：
 *   root_after / dev_type / proc_type 的 f_type、mounts_root / mounts_dev 的类型串、
 *   marker、/dev/vda 是否存在、/init 是否还能看到。
 */
#define _GNU_SOURCE

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "proot.h"

#define MARKER  "/marker"
#define DEVNODE "/dev/vda"
#define MAXBUF  512
#define MAXMNT  8192
#define MAXINFO 16384
#define MAXROWS 32

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

/* 类型字符串（第三列）。f_type 分不清 devtmpfs 与 tmpfs（同魔数 0x01021994），
 * 类型串是内核自己写的，能分清。 */
static void show_mount_type( const char *mnt, const char *tag )
{
    static char buf[MAXMNT];
    int         n = read_all( "/proc/self/mounts", buf, sizeof buf );

    if( n <= 0 )
    {
        pr_out( "PROOT|%s|absent\n", tag );
        return;
    }

    char *line = strtok( buf, "\n" );

    while( line != NULL )
    {
        char dev[128], mntp[256], type[64];

        if( sscanf( line, "%127s %255s %63s", dev, mntp, type ) == 3 && strcmp( mntp, mnt ) == 0 )
        {
            pr_out( "PROOT|%s|%s\n", tag, type );
            return;
        }

        line = strtok( NULL, "\n" );
    }

    pr_out( "PROOT|%s|not_mounted\n", tag );
}

/* 文件里是否含有某个子串。只用于读 /proc/cmdline 判断本组模式。 */
static int file_contains( const char *path, const char *needle )
{
    static char buf[MAXMNT];
    int         n = read_all( path, buf, sizeof buf );

    return n > 0 && strstr( buf, needle ) != NULL;
}

/* 整表 dump：每行一条 PROOT|mounts|，再加一行统计。
 * has_rootfs 是"挂载表里还有没有 rootfs 类型的挂载"——pivot + umount 之后应当没有，
 * 只 chroot 时应当还在。判据只读这一行，不依赖我猜的挂载点名字。 */
static void dump_mounts( void )
{
    static char buf[MAXMNT];
    int         n = read_all( "/proc/self/mounts", buf, sizeof buf );

    if( n <= 0 )
    {
        pr_out( "PROOT|mounts|absent\n" );
        return;
    }

    int  rows  = 0;
    int  hasrf = 0;
    char *line = strtok( buf, "\n" );

    while( line != NULL && rows < MAXROWS )
    {
        char dev[128], mntp[256], type[64];

        if( sscanf( line, "%127s %255s %63s", dev, mntp, type ) == 3 )
        {
            pr_out( "PROOT|mounts|%d|%s|%s\n", rows, mntp, type );
            rows++;

            if( strcmp( type, "rootfs" ) == 0 )
            {
                hasrf++;
            }
        }

        line = strtok( NULL, "\n" );
    }

    pr_out( "PROOT|mounts_count|%d\n", rows );
    pr_out( "PROOT|mounts_rootfs|%d\n", hasrf );
}

/* mountinfo 比 mounts 多出前两列：本挂载的 id 与父挂载的 id。
 * 严格 umount 的 EBUSY 只能来自两处（do_umount 末尾的 propagate_mount_busy）：
 *   ① 本挂载有子挂载（mnt_mounts 非空）
 *   ② 引用计数超出"树内一份 + 本次调用一份"
 * 有了父 id，就能直接从表里数出 ① 有没有，不必猜。 */
static void dump_mountinfo( void )
{
    static char buf[MAXINFO];
    int         n = read_all( "/proc/self/mountinfo", buf, sizeof buf );

    if( n <= 0 )
    {
        pr_out( "PROOT|mountinfo|absent\n" );
        return;
    }

    char *line = strtok( buf, "\n" );

    while( line != NULL )
    {
        int  id, parent;
        char mntp[256];

        if( sscanf( line, "%d %d %*s %*s %255s", &id, &parent, mntp ) == 3 )
        {
            pr_out( "PROOT|mountinfo|%d|%d|%s\n", id, parent, mntp );
        }

        line = strtok( NULL, "\n" );
    }
}

/* 打开的文件本身就钉住挂载。把 /proc/self/fd 全读出来，
 * 才能排除"某个 fd 让旧根不可卸载"这一可能。 */
static void dump_fds( void )
{
    DIR *d = opendir( "/proc/self/fd" );
    int  rows = 0;

    if( d == NULL )
    {
        pr_out( "PROOT|fds|absent\n" );
        return;
    }

    struct dirent *de;

    while( ( de = readdir( d ) ) != NULL )
    {
        /* d_name 最长 255 字节，路径缓冲按 15 + 255 的上界给，别让 -Werror 的
         * format-truncation 有话说。 */
        char path[320];
        char tgt[256];

        if( de->d_name[0] == '.' )
        {
            continue;
        }

        snprintf( path, sizeof path, "/proc/self/fd/%s", de->d_name );

        ssize_t n = readlink( path, tgt, sizeof tgt - 1U );

        if( n < 0 )
        {
            continue;
        }

        tgt[n] = '\0';
        pr_out( "PROOT|fd|%s|%s\n", de->d_name, tgt );
        rows++;
    }

    closedir( d );
    pr_out( "PROOT|fds_count|%d\n", rows );
}

/* 对照组：在**新根内部**现挂一个同类型文件系统，再严格卸载它。
 *   umount_ok   → 本命名空间里严格 umount 本身可用，旧根的 EBUSY 属于那个实例
 *   umount_busy → 连新挂的实例都卸不掉，问题在用法或环境，与旧根无关
 * 先 tmpfs（普通实例）再 rootfs（与 initramfs 同类型），
 * 用来区分"整个类型被钉住"还是"只有内核自建的那一个实例被钉住"。 */
static void ctrl_umount( const char *fstype )
{
    char dir[64];

    snprintf( dir, sizeof dir, "/ctrl-%s", fstype );

    if( mkdir( dir, 0755 ) != 0 && errno != EEXIST )
    {
        pr_out( "PROOT|ctrl|%s|mkdir_fail|errno=%d\n", fstype, errno );
        return;
    }

    if( mount( fstype, dir, fstype, 0, "" ) != 0 )
    {
        pr_out( "PROOT|ctrl|%s|mount_fail|errno=%d\n", fstype, errno );
        return;
    }

    if( umount2( dir, 0 ) == 0 )
    {
        pr_out( "PROOT|ctrl|%s|umount_ok\n", fstype );
        return;
    }

    pr_out( "PROOT|ctrl|%s|umount_busy|errno=%d\n", fstype, errno );
}

/* 把 0/1/2 重新绑到**新根**的 /dev/console。
 * 这是 pivot-late 组的唯一变量：内核把 /init 当作 PID 1 时，0/1/2 指向的
 * /dev/console 属于 initramfs 的 rootfs；exec 换不掉这三个 fd，
 * 于是旧根被自己的 stdio 钉住。重绑之后旧的 struct file 被放掉。 */
static void rebind_stdio( void )
{
    int fd = open( "/dev/console", O_RDWR );

    if( fd < 0 )
    {
        pr_out( "PROOT|stdio_rebind|fail|errno=%d\n", errno );
        return;
    }

    if( dup2( fd, 0 ) < 0 || dup2( fd, 1 ) < 0 || dup2( fd, 2 ) < 0 )
    {
        pr_out( "PROOT|stdio_rebind|dup2_fail|errno=%d\n", errno );
    }
    else
    {
        pr_out( "PROOT|stdio_rebind|ok\n" );
    }

    if( fd > 2 )
    {
        close( fd );
    }
}

int main( void )
{
    struct stat     st;
    struct timespec ts = { 0, 800 * 1000 * 1000 };

    pr_out( "PROOT|stage|2|pid=%d\n", ( int ) getpid() );

    /* 等 800 ms 再取证：stage1 里 fork 的观察者在 100 ms 时打印它看到的 "/"，
     * 那是 chroot 与 pivot_root 唯一可观测的差别（见 stage1 的 spawn_watcher）。
     * 不等就断电会把那一行截掉。 */
    pr_out( "PROOT|stage2|wait|800ms\n" );
    nanosleep( &ts, NULL );

    /* pivot-late：旧根留给这一阶段（已在新根上、映像也在新根上）卸载。
     *
     * 这一组本来验证"stage1 的可执行映射钉住旧根"——第一次实测**否证**了它：
     * exec 之后严格 umount 仍 EBUSY(16)。于是先取证再判定，EBUSY 的两个来源逐一排除：
     *   mountinfo 的父 id → 旧根有没有子挂载（实测：没有）
     *   /proc/self/fd    → 有没有打开的文件钉着旧根（实测：0/1/2 指向 /oldroot/dev/console）
     *   ctrl_umount       → 严格 umount 在本命名空间能否用（实测：新挂的 tmpfs 可严格卸载）
     * 结论若成立，因果实验必须能把它推翻或坐实：同一次运行里只改 0/1/2 的绑定，再卸一次。
     * 第一次仍失败、第二次成功，才说明"是 stdio 钉住了旧根"，而不是别的什么。 */
    if( file_contains( "/proc/cmdline", "proot=pivot-late" ) )
    {
        dump_mountinfo();
        dump_fds();
        dump_mounts();      /* 卸载前的表：旧根可达时应以 rootfs 类型出现在 /oldroot */
        ctrl_umount( "tmpfs" );
        ctrl_umount( "rootfs" );

        if( umount2( "/oldroot", 0 ) == 0 )
        {
            pr_out( "PROOT|late_umount_keep_stdio|ok\n" );
        }
        else
        {
            pr_out( "PROOT|late_umount_keep_stdio|busy|errno=%d\n", errno );
        }

        rebind_stdio();
        dump_fds();

        if( umount2( "/oldroot", 0 ) == 0 )
        {
            pr_out( "PROOT|late_umount_after_rebind|ok\n" );
        }
        else
        {
            pr_out( "PROOT|late_umount_after_rebind|busy|errno=%d\n", errno );
        }

        /* 兜底：严格仍失败时用 MNT_DETACH 摘掉，别让挂载表被旧根占着。 */
        if( umount2( "/oldroot", MNT_DETACH ) == 0 )
        {
            pr_out( "PROOT|late_umount_detach|ok\n" );
        }
        else
        {
            pr_out( "PROOT|late_umount_detach|fail|errno=%d\n", errno );
        }
    }

    pr_report_fs( "root_after", "/" );
    pr_report_fs( "dev_type", "/dev" );
    pr_report_fs( "proc_type", "/proc" );

    show_mount_type( "/", "mounts_root" );
    show_mount_type( "/dev", "mounts_dev" );

    dump_mounts();

    show_file( "marker", MARKER );
    show_file( "cmdline_after", "/proc/cmdline" );

    if( stat( "/init", &st ) == 0 )
    {
        pr_out( "PROOT|old_init|visible\n" );
    }
    else
    {
        pr_out( "PROOT|old_init|gone\n" );
    }

    if( stat( DEVNODE, &st ) == 0 )
    {
        pr_out( "PROOT|dev_vda|present\n" );
    }
    else
    {
        pr_out( "PROOT|dev_vda|absent\n" );
    }

    pr_out( "PROOT|stage2|done\n" );

    sync();
    reboot( RB_POWER_OFF );

    for( ;; )
    {
        pause();
    }

    return 0;
}
