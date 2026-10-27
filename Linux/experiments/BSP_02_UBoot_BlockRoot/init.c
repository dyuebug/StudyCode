/* BSP_02 启动链取证装置（PID 1）：u-boot 参数传递 + 块设备根
 *
 * 与 BSP_01 的区别：BSP_01 只关心"三件套分工"，根一律是 initramfs。
 * 本装置多取证两件事：
 *
 *   /proc/mounts 里挂到 / 的那一行的**设备字段**  根到底是 rootfs 还是 /dev/vda*
 *   /proc/partitions                              分区表生效与否，直接改变设备名
 *   /dev/vda* 是否存在                            devtmpfs 是否把这些节点建了出来
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

/* 设备树里的字符串以 NUL 结尾，日志里要清理掉。 */
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

/* 判断一段内容是不是"纯文本 + NUL 分隔"：多值属性用 NUL 分隔，不是二进制。 */
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

/* 打印一个设备树属性或纯文本文件的原文；不存在时打印 absent。 */
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

/* 打出挂在 / 的那一行的设备与文件系统类型。
 * 只报类型不够：initramfs 与块设备根的差别在**设备字段**上（rootfs vs /dev/vda*）。 */
static void print_root_mount( void )
{
    static char buf[4096];
    int         n = read_all( "/proc/mounts", buf, sizeof buf );

    if( n <= 0 )
    {
        printf( "BOOT|root_mount|absent\n" );
        return;
    }

    char *line = strtok( buf, "\n" );

    while( line != NULL )
    {
        char dev[128], mnt[128], type[64];

        if( sscanf( line, "%127s %127s %63s", dev, mnt, type ) == 3 && strcmp( mnt, "/" ) == 0 )
        {
            printf( "BOOT|root_mount|dev=%s|fstype=%s\n", dev, type );
            return;
        }

        line = strtok( NULL, "\n" );
    }

    printf( "BOOT|root_mount|not_found\n" );
}

/* 把 /proc/partitions 压成一行：major:minor:blocks:name。
 * 有分区表时这里会同时出现 vda 与 vda1；没有分区表就只有 vda。
 * 这是"分区表是否生效"最直接的证据，不用猜设备路径。 */
static void print_partitions( void )
{
    static char buf[4096];
    int         n = read_all( "/proc/partitions", buf, sizeof buf );

    if( n <= 0 )
    {
        printf( "BOOT|partitions|absent\n" );
        return;
    }

    printf( "BOOT|partitions|" );

    int         first = 1;
    char       *line  = strtok( buf, "\n" );

    while( line != NULL )
    {
        unsigned major, minor;
        unsigned long blocks;
        char     name[64];
        char     extra[64] = "";

        /* 头部 "major minor #blocks name" 解析不成数字，自动被跳过。 */
        if( sscanf( line, "%u %u %lu %63s %63s", &major, &minor, &blocks, name, extra ) >= 4 )
        {
            printf( "%s%u:%u:%lu:%s", first ? "" : ",", major, minor, blocks, name );
            first = 0;
        }

        line = strtok( NULL, "\n" );
    }

    if( first )
        printf( "empty" );

    printf( "\n" );
}

/* 统计根里的普通文件数（深度受限，跳过外部挂载点）。 */
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

int main( void )
{
    struct stat st;

    /* 块设备根与 initramfs 的第一处行为差异：内核把块设备根挂成 **只读**，
     * 而 initramfs 的 rootfs(ramfs) 天生可写。所以真实 init 系统的第一件事
     * 就是把 / 重新挂成可写——不 remount 就连 mkdir /proc 都会失败，
     * 后面的 mount 全部 ENOENT。initramfs 环境下这一步会返回失败，忽略即可。 */
    if( mount( NULL, "/", NULL, MS_REMOUNT, NULL ) == 0 )
        printf( "BOOT|remount_root|ok\n" );
    else
        printf( "BOOT|remount_root|fail|errno=%d\n", errno );

    /* proc 给 cmdline/mounts/partitions；sysfs 给设备树视图；
     * devtmpfs 给设备节点——块设备根这一课要证明节点真的被建出来了。 */
    const char *dirs[] = { "/proc", "/sys", "/dev" };

    for( unsigned i = 0; i < sizeof dirs / sizeof dirs[0]; i++ )
    {
        if( mkdir( dirs[i], 0755 ) != 0 && errno != EEXIST )
            printf( "BOOT|mkdir|%s|fail|errno=%d\n", dirs[i], errno );
    }

    if( mount( "proc", "/proc", "proc", 0, "" ) != 0 )
        printf( "BOOT|mount|proc|fail|errno=%d\n", errno );

    if( mount( "sysfs", "/sys", "sysfs", 0, "" ) != 0 )
        printf( "BOOT|mount|sysfs|fail|errno=%d\n", errno );

    if( mount( "devtmpfs", "/dev", "devtmpfs", 0, "" ) != 0 )
        printf( "BOOT|mount|devtmpfs|fail|errno=%d\n", errno );

    printf( "BOOT|init|pid=%d\n", ( int ) getpid() );

    print_prop( "cmdline", "/proc/cmdline" );
    print_prop( "dt_model", DT_BASE "/model" );

    if( stat( "/sys/firmware/fdt", &st ) == 0 )
        printf( "BOOT|fdt_raw|size=%ld\n", ( long ) st.st_size );
    else
        printf( "BOOT|fdt_raw|absent\n" );

    print_root_mount();
    print_partitions();

    if( stat( "/dev/vda", &st ) == 0 )
        printf( "BOOT|dev_vda|present\n" );
    else
        printf( "BOOT|dev_vda|absent\n" );

    printf( "BOOT|rootfs_files|%d\n", count_files( "/", 3 ) );

    printf( "BOOT|init|done\n" );
    fflush( stdout );

    sync();
    reboot( RB_POWER_OFF );

    for( ;; )
        pause();

    return 0;
}
