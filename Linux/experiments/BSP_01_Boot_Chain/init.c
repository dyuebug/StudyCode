/* BSP_01 启动链取证装置（PID 1）
 *
 * 目的：不加载任何驱动，只回答一个问题——内核从拿到 zImage/DTB/initramfs
 * 到执行 /init 这一段，到底把哪些东西变成了可见的接口。
 *
 * 取证点与它对应的启动链环节：
 *   /proc/cmdline                      bootloader/QEMU 传给内核的参数
 *   /sys/firmware/devicetree/base/     DTB 被内核展开成目录树（不是原样放着）
 *   /sys/firmware/fdt                  内核保留的原始 DTB 二进制副本
 *   /sys/firmware/devicetree/base/model 机器型号，来自 DTB 根节点
 *   /proc/mounts 里挂到 / 的那行        initramfs 展开成的 rootfs
 *   rootfs 里的文件数                   initramfs 确实被解包，而不是"挂上了但空的"
 *
 * 输出约定：全部 ASCII，前缀 "BOOT|"，便于在串口日志里 grep。
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
#include <unistd.h>

#define DT_BASE "/sys/firmware/devicetree/base"
#define MAXBUF  1024

/* 读整文件；返回字节数（0 表示空文件），失败 -1。 */
static int read_all( const char *path, char *buf, size_t cap )
{
    int fd = open( path, O_RDONLY );

    if( fd < 0 )
        return -1;

    ssize_t n = read( fd, buf, cap - 1U );
    close( fd );

    if( n < 0 )
        return -1;

    buf[n] = '\0';
    return ( int ) n;
}

/* 设备树里的字符串以 NUL 结尾，日志里要清理掉。
 * strip_tail 只去尾部的 NUL / 空白 / 分隔逗号：正文里的字符要保留。 */
static void strip_tail( char *s, int n )
{
    while( n > 0 )
    {
        char c = s[n - 1];

        if( c == '\0' || c == '\n' || c == '\r' || c == '\t' || c == ' ' || c == ',' )
            n--;
        else
            break;
    }

    s[n] = '\0';
}

/* 判断一段内容是不是"纯文本 + NUL 分隔"。设备树的字符串属性常常是
 * 多值（如 compatible），用 NUL 分隔，不是二进制。 */
static int is_text_blob( const char *b, int n )
{
    for( int i = 0; i < n; i++ )
    {
        unsigned char c = ( unsigned char ) b[i];

        if( c == '\0' || c == '\n' || c == '\r' || c == '\t' )
            continue;

        if( c < 0x20 || c > 0x7e )
            return 0;
    }

    return 1;
}

/* 打印一个设备树属性或纯文本文件的原文；不存在时打印 absent。
 * "不存在"本身也是证据：说明这一层没被建立起来。 */
static void print_prop( const char *tag, const char *path )
{
    static char buf[MAXBUF];
    int         n = read_all( path, buf, sizeof buf );

    if( n < 0 )
    {
        printf( "BOOT|%s|absent\n", tag );
        return;
    }

    if( !is_text_blob( buf, n ) )
    {
        printf( "BOOT|%s|binary|len=%d\n", tag, n );
        return;
    }

    /* NUL 分隔的多值属性换成逗号，一行打印完。 */
    int last = n;

    for( int i = 0; i < n; i++ )
    {
        if( buf[i] == '\n' || buf[i] == '\r' || buf[i] == '\t' )
            buf[i] = ' ';
        else if( buf[i] == '\0' )
            buf[i] = ',';
    }

    strip_tail( buf, last );
    printf( "BOOT|%s|%s\n", tag, buf );
}

/* 读设备树里的第一个 memory@ 节点，打印它的 reg（两个大端 u32：基址、大小）。
 * vexpress 的 DTS 声明 1 GB，QEMU 会按 -m 改写成实际值——
 * 所以这里打印出来的数字，是判断"内核看到的 DT 和你传的文件是否一致"的关键。 */
static void print_dt_memory( void )
{
    DIR           *d = opendir( DT_BASE );
    struct dirent *e;
    char           path[512];

    if( d == NULL )
    {
        printf( "BOOT|dt_memory|absent\n" );
        return;
    }

    while( ( e = readdir( d ) ) != NULL )
    {
        if( strncmp( e->d_name, "memory@", 7 ) != 0 )
            continue;

        snprintf( path, sizeof path, "%s/%s/reg", DT_BASE, e->d_name );

        static char buf[16];
        int         n = read_all( path, buf, sizeof buf );

        if( n >= 8 )
        {
            unsigned long base = ( ( unsigned long ) ( unsigned char ) buf[0] << 24 )
                                 | ( ( unsigned long ) ( unsigned char ) buf[1] << 16 )
                                 | ( ( unsigned long ) ( unsigned char ) buf[2] << 8 )
                                 | ( unsigned long ) ( unsigned char ) buf[3];
            unsigned long size = ( ( unsigned long ) ( unsigned char ) buf[4] << 24 )
                                 | ( ( unsigned long ) ( unsigned char ) buf[5] << 16 )
                                 | ( ( unsigned long ) ( unsigned char ) buf[6] << 8 )
                                 | ( unsigned long ) ( unsigned char ) buf[7];

            printf( "BOOT|dt_memory|node=%s|base=0x%08lx|size=0x%08lx|mib=%lu\n",
                    e->d_name, base, size, size >> 20 );
        }
        else
        {
            printf( "BOOT|dt_memory|node=%s|reg_unreadable\n", e->d_name );
        }

        closedir( d );
        return;
    }

    closedir( d );
    printf( "BOOT|dt_memory|absent\n" );
}

/* 统计 rootfs 里的普通文件数（深度受限）。
 * 用途只有一个：证明 initramfs 被解包成了真实的文件树。 */
static int count_files( const char *dir, int depth )
{
    DIR           *d;
    struct dirent *e;
    struct stat    st;
    char           path[512];
    int            n = 0;

    if( depth <= 0 )
        return 0;

    d = opendir( dir );

    if( d == NULL )
        return 0;

    while( ( e = readdir( d ) ) != NULL )
    {
        if( strcmp( e->d_name, "." ) == 0 || strcmp( e->d_name, ".." ) == 0 )
            continue;

        snprintf( path, sizeof path, "%s/%s", dir, e->d_name );

        /* 用 lstat：不追符号链接，避免 /proc 之类的外部树把计数搅乱。 */
        if( lstat( path, &st ) != 0 )
            continue;

        if( S_ISREG( st.st_mode ) )
            n++;
        else if( S_ISDIR( st.st_mode ) && strcmp( e->d_name, "proc" ) != 0
                 && strcmp( e->d_name, "sys" ) != 0 && strcmp( e->d_name, "dev" ) != 0 )
            n += count_files( path, depth - 1 );
    }

    closedir( d );
    return n;
}

/* 从 /proc/mounts 找出挂在 / 的那一行，打印文件系统类型。
 * initramfs 的 rootfs 类型字面就是 "rootfs"，它是判断"根是内存盘"的直接证据。 */
static void print_root_fstype( void )
{
    static char buf[4096];
    int         n = read_all( "/proc/mounts", buf, sizeof buf );

    if( n <= 0 )
    {
        printf( "BOOT|root_fstype|absent\n" );
        return;
    }

    char *line = strtok( buf, "\n" );

    while( line != NULL )
    {
        char dev[128], mnt[128], type[64];

        if( sscanf( line, "%127s %127s %63s", dev, mnt, type ) == 3 && strcmp( mnt, "/" ) == 0 )
        {
            printf( "BOOT|root_fstype|%s\n", type );
            return;
        }

        line = strtok( NULL, "\n" );
    }

    printf( "BOOT|root_fstype|not_found\n" );
}

int main( void )
{
    struct stat st;

    /* 只挂两棵树：proc 给 cmdline/mounts，sysfs 给 firmware 下的设备树视图。
     * devtmpfs 本实验不需要——不加载驱动、不建节点。 */
    mkdir( "/proc", 0755 );
    mkdir( "/sys", 0755 );

    if( mount( "proc", "/proc", "proc", 0, "" ) != 0 )
        printf( "BOOT|mount|proc|fail|errno=%d\n", errno );

    if( mount( "sysfs", "/sys", "sysfs", 0, "" ) != 0 )
        printf( "BOOT|mount|sysfs|fail|errno=%d\n", errno );

    printf( "BOOT|init|pid=%d\n", ( int ) getpid() );

    print_prop( "cmdline", "/proc/cmdline" );
    print_prop( "version", "/proc/version" );

    /* DTB → 目录树。根节点 model 是"这份 DT 是哪一个板子"的唯一标识。 */
    print_prop( "dt_model", DT_BASE "/model" );
    print_prop( "dt_compatible", DT_BASE "/compatible" );

    /* 内核保留的原始 DTB 副本：只报大小，说明"DTB 不只被解析，还被留存"。 */
    if( stat( "/sys/firmware/fdt", &st ) == 0 )
        printf( "BOOT|fdt_raw|size=%ld\n", ( long ) st.st_size );
    else
        printf( "BOOT|fdt_raw|absent\n" );

    /* 自定义节点是否存在，用来确认"跑的是我传的那份 DTB"，而不是 QEMU 自带的。 */
    if( stat( DT_BASE "/myled_for_test", &st ) == 0 )
        printf( "BOOT|dt_custom_node|present\n" );
    else
        printf( "BOOT|dt_custom_node|absent\n" );

    /* 内存节点：DTS 写的是 1 GB，QEMU -m 512M 会改写它。
     * 这两个数字对不上，就是"内核看到的 DT 不是你传的文件原文"的直接证据。 */
    print_dt_memory();

    printf( "BOOT|rootfs_files|%d\n", count_files( "/", 4 ) );
    print_root_fstype();

    printf( "BOOT|init|done\n" );
    fflush( stdout );

    sync();
    reboot( RB_POWER_OFF );

    for( ;; )
        pause();

    return 0;
}
