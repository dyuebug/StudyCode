/* BSP_03 第二阶段：真实根文件系统上的 init（PID 仍是 1）
 *
 * 它的唯一职责是**从新根内部**回答"根到底换了没有"，并且给出机器可判的读数：
 *   root_after  / 的 f_type          → ext4 才算换根成功；还是 rootfs 就是没换
 *   dev_type    /dev 的 f_type       → devtmpfs 说明挂载实例被 MS_MOVE 搬过来了；
 *                                      还是 ext4 说明 /dev 只是新根上的一个空目录
 *   proc_type   /proc 的 f_type      → 同上，proc 的魔数是 0x9fa0
 *   marker      新根上的标记文件内容  → 证明"挂上来的是我做的那个镜像"
 *   dev_vda     设备节点是否存在      → 直接决定第二阶段之后能不能读盘
 *
 * 为什么不能只报"根目录里有文件"：initramfs 里本来就拷了 stage1，
 * 只数文件会被"我把东西放进去了"骗过去；f_type 是内核层的属性，改不了。
 */
#define _GNU_SOURCE

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "swroot.h"

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

/* 从 /proc/self/mounts 里取某个挂载点的**类型字符串**（第三列）。
 * 为什么不能只看 statfs：本内核的 devtmpfs 以 shmem 为后端，f_type 与 tmpfs 相同（0x01021994），
 * 魔数区分不了 "/dev 挂了 devtmpfs" 和 "/dev 只是个 tmpfs 目录"。
 * 类型字符串是内核自己写的，这才是能区分两者的证据。
 * /proc 没挂上时读不到，报 absent —— 这本身也是 no-move 组的预期现象。 */
static void show_mount_type( const char *mnt, const char *tag )
{
    static char buf[8192];
    int         n = read_all( "/proc/self/mounts", buf, sizeof buf );

    if( n <= 0 )
    {
        sw_out( "SWROOT|%s|absent\n", tag );
        return;
    }

    char *line = strtok( buf, "\n" );

    while( line != NULL )
    {
        char dev[128], mntp[256], type[64];

        if( sscanf( line, "%127s %255s %63s", dev, mntp, type ) == 3 && strcmp( mntp, mnt ) == 0 )
        {
            sw_out( "SWROOT|%s|%s\n", tag, type );
            return;
        }

        line = strtok( NULL, "\n" );
    }

    sw_out( "SWROOT|%s|not_mounted\n", tag );
}

int main( void )
{
    struct stat st;

    /* 同一行里带上旧根是否还可达：/init 是 stage1，只存在于 initramfs 的 rootfs 上。
     * 切根成功后它在新根视角下应当不存在。 */
    sw_out( "SWROOT|stage|2|pid=%d\n", ( int ) getpid() );

    sw_report_fs( "root_after", "/" );
    sw_report_fs( "dev_type", "/dev" );
    sw_report_fs( "proc_type", "/proc" );
    sw_report_fs( "sys_type", "/sys" );

    /* 类型字符串补上 f_type 分辨不了的那一半（devtmpfs 与 tmpfs 同魔数）。 */
    show_mount_type( "/", "mounts_root" );
    show_mount_type( "/dev", "mounts_dev" );

    show_file( "marker", MARKER );
    show_file( "cmdline_after", "/proc/cmdline" );

    if( stat( "/init", &st ) == 0 )
    {
        sw_out( "SWROOT|old_init|visible\n" );
    }
    else
    {
        sw_out( "SWROOT|old_init|gone\n" );
    }

    if( stat( DEVNODE, &st ) == 0 )
    {
        sw_out( "SWROOT|dev_vda|present\n" );
    }
    else
    {
        sw_out( "SWROOT|dev_vda|absent\n" );
    }

    sw_out( "SWROOT|stage2|done\n" );

    /* PID 1 退出会让内核 panic，所以这里主动断电：QEMU 配合 -no-reboot 干净退出，
     * 日志末尾不会多出一段与实验无关的 panic 栈。 */
    sync();
    reboot( RB_POWER_OFF );

    for( ;; )
    {
        pause();
    }

    return 0;
}
