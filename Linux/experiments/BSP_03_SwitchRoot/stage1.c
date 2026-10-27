/* BSP_03 第一阶段：initramfs 里的 init（PID 1）
 *
 * 目的：把 switch_root 的四个动作拆开，每一步都留痕，让"根确实换了"可以被机器判定。
 *
 *   ① 建运行期基础树   mount devtmpfs /proc /sys      ← 内核配置里 DEVTMPFS_MOUNT 未设，必须自己挂
 *   ② 挂新根           mount("/dev/vda", "/mnt/newroot", "ext4", ...)
 *   ③ 搬运挂载点       mount("/dev", "/mnt/newroot/dev", NULL, MS_MOVE, NULL) ×3
 *   ④ 换根 + 换进程    chroot("/mnt/newroot")  →  execv("/stage2")
 *
 * 为什么③不能省：chroot 只改进程的路径解析起点，不搬任何挂载实例。不搬，新根里
 * 的 /dev 只是 ext4 上的一个空目录——第二阶段看不到任何设备节点，也看不到 /proc。
 * 这就是老式 initramfs 里 switch_root 先 mount --move 再 chroot 的原因。
 *
 * 为什么④必须 exec：本进程的可执行文件与数据页都在旧 rootfs 上，不 exec 就永远
 * 持有旧根的引用，内核无法释放那部分内存。switch_root 的最后一步必然是 exec。
 *
 * 输入只有内核命令行：带 "swroot=nomove" 时跳过③，用来证明③的必要性（失败对照组）。
 * 输出约定见 swroot.h。
 */
#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "swroot.h"

#define NEWROOT  "/mnt/newroot"
#define STAGE2   "/stage2"
#define MARKER   "/marker"
#define DEVNODE  "/dev/vda"
#define MAXBUF   512

/* 读整个文件，返回字节数；失败 -1。给 /proc/cmdline 与 /marker 用。 */
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

/* 内核命令行里是否存在某个整词参数。
 * 按空白切词后整词比较，不用 strstr：否则 "xs wroot=nomove2" 这类前缀也会命中。 */
static int cmdline_has( const char *flag )
{
    static char buf[MAXBUF];
    int         n = read_all( "/proc/cmdline", buf, sizeof buf );

    if( n <= 0 )
    {
        return 0;
    }

    const size_t want = strlen( flag );
    char        *p    = buf;

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

        if( len == want && strncmp( tok, flag, want ) == 0 )
        {
            return 1;
        }
    }

    return 0;
}

/* 打印文件内容（去尾换行）。不存在就报 absent —— 与"内容是空的"区分开。 */
static void show_file( const char *tag, const char *path )
{
    static char buf[MAXBUF];
    int         n = read_all( path, buf, sizeof buf );

    if( n < 0 )
    {
        sw_out( "SWROOT|%s|absent\n", tag );
        return;
    }

    while( n > 0 && ( buf[n - 1] == '\n' || buf[n - 1] == '\r' ) )
    {
        buf[--n] = '\0';
    }

    sw_out( "SWROOT|%s|%s\n", tag, buf );
}

/* 失败收尾：先留痕，再断电。
 * 不能让 PID 1 直接 return —— 内核会 panic 并打印一大段与本次实验无关的栈，
 * 把真正的证据淹掉；RB_POWER_OFF 让 QEMU 干净退出。 */
static void fail_and_poweroff( const char *where, int err )
{
    sw_out( "SWROOT|fail|%s|errno=%d\n", where, err );
    sw_out( "SWROOT|stage2|skipped\n" );

    sync();
    reboot( RB_POWER_OFF );

    for( ;; )
    {
        pause();
    }
}

/* 建目录，已存在不算错（镜像里可能已经备好）。 */
static void ensure_dir( const char *path )
{
    if( mkdir( path, 0755 ) != 0 && errno != EEXIST )
    {
        sw_out( "SWROOT|mkdir|%s|fail|errno=%d\n", path, errno );
    }
}

/* ③：把一个已存在的挂载点搬到新根的同名位置。
 * MS_MOVE 是"搬"（旧位置消失、实例不变），不是"再挂一次"：
 * 重新挂载会出现两份独立的 devtmpfs 实例，旧根释放时会把设备节点一起带走。 */
static void move_mountpoint( const char *sub )
{
    char dst[MAXBUF];

    snprintf( dst, sizeof dst, "%s%s", NEWROOT, sub );
    ensure_dir( dst );

    if( mount( sub, dst, NULL, MS_MOVE, NULL ) != 0 )
    {
        sw_out( "SWROOT|move|%s|fail|errno=%d\n", sub, errno );
        return;
    }

    sw_out( "SWROOT|move|%s|ok\n", sub );
}

int main( void )
{
    /* 1. 身份：判"这次跑的是哪一组对照"就看这一行。
     *    命令行要等 /proc 挂上之后才能读，所以放在下面。 */
    sw_out( "SWROOT|stage|1|pid=%d\n", ( int ) getpid() );

    /* 2. 运行期基础树。DEVTMPFS_MOUNT 未设，所以 /dev 不会自动出现——
     *    不自己挂 devtmpfs，后面连 /dev/vda 这个节点都不存在。 */
    ensure_dir( "/dev" );
    ensure_dir( "/proc" );
    ensure_dir( "/sys" );

    if( mount( "devtmpfs", "/dev", "devtmpfs", 0, "" ) != 0 )
    {
        sw_out( "SWROOT|mount|devtmpfs|fail|errno=%d\n", errno );
    }
    else
    {
        sw_out( "SWROOT|mount|devtmpfs|ok\n" );
    }

    if( mount( "proc", "/proc", "proc", 0, "" ) != 0 )
    {
        sw_out( "SWROOT|mount|proc|fail|errno=%d\n", errno );
    }
    else
    {
        sw_out( "SWROOT|mount|proc|ok\n" );
    }

    if( mount( "sysfs", "/sys", "sysfs", 0, "" ) != 0 )
    {
        sw_out( "SWROOT|mount|sysfs|fail|errno=%d\n", errno );
    }
    else
    {
        sw_out( "SWROOT|mount|sysfs|ok\n" );
    }

    /* 切根之前 / 是 initramfs 展开成的 rootfs：这是"还没切"的基线证据。
     * 命令行也在这里读：/proc 刚挂上，早了只会读到 absent。 */
    sw_report_fs( "root_before", "/" );
    show_file( "cmdline", "/proc/cmdline" );

    /* 3. 挂新根。ext4 镜像是 QEMU 的 virtio-blk，设备名 vda（无分区表）。 */
    ensure_dir( NEWROOT );

    if( mount( DEVNODE, NEWROOT, "ext4", 0, "" ) != 0 )
    {
        sw_out( "SWROOT|mount|newroot|fail|errno=%d\n", errno );
        fail_and_poweroff( "mount_newroot", errno );
    }

    sw_out( "SWROOT|mount|newroot|ok\n" );

    /* 新根上的东西先读一次：确认挂上来的是"我做的那个镜像"，不是空文件系统。 */
    show_file( "newroot_marker", NEWROOT MARKER );

    /* 4. 搬运挂载点。nomove 对照组故意跳过，用来证明这一步不能省。 */
    if( cmdline_has( "swroot=nomove" ) )
    {
        sw_out( "SWROOT|move|/dev|skipped\n" );
        sw_out( "SWROOT|move|/proc|skipped\n" );
        sw_out( "SWROOT|move|/sys|skipped\n" );
    }
    else
    {
        move_mountpoint( "/dev" );
        move_mountpoint( "/proc" );
        move_mountpoint( "/sys" );
    }

    /* 5. 换根。chroot 只改路径解析起点，不卸载旧根——旧根仍被本进程的可执行文件
     *    与数据页引用着，所以要靠下一步的 exec 把最后一个引用放掉。 */
    if( chroot( NEWROOT ) != 0 )
    {
        sw_out( "SWROOT|chroot|fail|errno=%d\n", errno );
        fail_and_poweroff( "chroot", errno );
    }

    if( chdir( "/" ) != 0 )
    {
        sw_out( "SWROOT|chdir|fail|errno=%d\n", errno );
        fail_and_poweroff( "chdir", errno );
    }

    sw_out( "SWROOT|chroot|ok\n" );

    /* 6. 换进程。argv[0] 用绝对路径：chroot 之后旧根的 PATH 已不可达。 */
    {
        char *const argv[] = { ( char * ) STAGE2, NULL };
        char *const envp[] = { NULL };

        execve( STAGE2, argv, envp );

        /* 只有 exec 失败才会走到这里（文件不存在 / 不是可执行 / 权限不对）。 */
        sw_out( "SWROOT|exec|fail|path=%s|errno=%d\n", STAGE2, errno );
        fail_and_poweroff( "exec_stage2", errno );
    }

    return 0;
}
