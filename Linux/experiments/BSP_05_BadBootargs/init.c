/* BSP_05 坏 bootargs 用例的 init：只在"内核成功挂到根并执行了它"时出现。
 *
 * 它的存在本身就是判据：跑起来 = 内核的 virtio-blk / ext4 / init 执行链都正常，
 * 因此能把两种"起不来"分开：
 *
 *   参数错     → 同一份 zImage 改对 `root=`/`init=`/`console=` 后它就跑起来；
 *   内核配置错 → 参数怎么改它都不会出现（缺驱动、缺文件系统、缺 init 支持）。
 *
 * 为什么必须由它自己报 mark，而不是靠"日志里有没有内核启动行"：
 * 内核在挂载 root 之前就已经打印了成百行。那些行只证明"内核起来了"，
 * 不证明"找到了根、执行了用户态"。mark 出现在用户态，是这条链走到头的证据。
 */
#define _GNU_SOURCE

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <unistd.h>

#define MAXBUF 512

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
        printf( "BADARGS|%s|absent\n", tag );
        return;
    }

    while( n > 0 && ( buf[n - 1] == '\n' || buf[n - 1] == '\r' ) )
    {
        buf[--n] = '\0';
    }

    printf( "BADARGS|%s|%s\n", tag, buf );
}

/* 报 / 的文件系统魔数：ext4 = 0xef53。
 * 这一行回答"init 是从哪个根跑起来的"，补上 cmdline 里写了 root= 但可能没生效的缺口。 */
static void show_root_fs( void )
{
    struct statfs st;

    if( statfs( "/", &st ) == 0 )
    {
        printf( "BADARGS|root_fs|magic=0x%lx\n", ( unsigned long ) st.f_type );
    }
    else
    {
        printf( "BADARGS|root_fs|absent\n" );
    }
}

int main( void )
{
    /* 本实验没有 initramfs，内核不会替我们挂 /proc（那是 init 的职责）。
     * 不挂就读不到 /proc/cmdline —— 第一版这里报了 cmdline|absent，
     * 看起来像"命令行丢了"，实际是"没有人去读它"。 */
    mkdir( "/proc", 0755 );
    mount( "proc", "/proc", "proc", 0, NULL );

    printf( "BADARGS|init|started|pid=%d\n", ( int ) getpid() );

    show_file( "cmdline", "/proc/cmdline" );
    show_root_fs();
    show_file( "marker", "/marker" );

    printf( "BADARGS|init|done\n" );

    /* PID 1 退出会让内核 panic，多出一段与实验无关的栈。主动断电，
     * 配合 QEMU 的 -no-reboot 干净退出（照搬 BSP_03/04 的做法）。 */
    sync();
    reboot( RB_POWER_OFF );

    for( ;; )
    {
        pause();
    }

    return 0;
}
