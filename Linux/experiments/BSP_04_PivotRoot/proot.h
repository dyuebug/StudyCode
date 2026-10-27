/* BSP_04 共用的小工具：日志输出与 f_type 名字映射
 *
 * 两个阶段都以 PID 1 运行，没有库可依赖：不能用 perror（它读 locale），
 * 也不能假设 stderr 一定连着串口——统一 stdout + 显式 fflush，
 * 保证每一行在 exec 换掉进程映像之前已经落到 tty。
 *
 * 输出约定：全 ASCII，前缀 "PROOT|"，字段用 '|' 分隔，便于 grep 与机器判定。
 *
 * 与 BSP_03 的差别只在命名（避免两个实验的符号混在一起读）；
 * f_type 的两条"与教科书不符"的实测结论保留在这里，因为 BSP_04 仍要用它判"根换了没有"：
 *   1. rootfs 后端可能是 shmem：init/do_mounts.c 的 init_rootfs() 在命令行没有 root=/
 *      rootfstype= 且 CONFIG_TMPFS=y 时把 rootfs 建在 shmem 上，f_type 报 0x01021994；
 *      给了 root= 才回落到 RAMFS_MAGIC(0x858458f6)。两种情况 /proc/mounts 都写作 "rootfs"。
 *   2. devtmpfs 以 shmem/ramfs 为后端，f_type 同样是 0x01021994——
 *      f_type 区分不了 "/dev 挂了 devtmpfs" 和 "/dev 是个 tmpfs"，得读挂载表的类型字段。
 */
#ifndef BSP04_PROOT_H
#define BSP04_PROOT_H

#include <stdarg.h>
#include <stdio.h>
#include <sys/vfs.h>

static inline const char *pr_fs_name( unsigned long t )
{
    switch( t )
    {
        case 0x858458F6UL: return "ramfs";
        case 0xEF53UL:     return "ext4";
        case 0x1373UL:     return "devtmpfs";
        case 0x9FA0UL:     return "proc";
        case 0x62656572UL: return "sysfs";
        case 0x01021994UL: return "tmpfs";
        default:           return "other";
    }
}

/* 打印某路径上的 f_type。路径不存在也照样留痕（absent 本身是证据）。
 * 返回 0 成功，-1 失败。 */
static inline int pr_report_fs( const char *tag, const char *path )
{
    struct statfs st;

    if( statfs( path, &st ) != 0 )
    {
        printf( "PROOT|%s|absent\n", tag );
        fflush( stdout );
        return -1;
    }

    printf( "PROOT|%s|type=%s|magic=0x%lx\n",
            tag, pr_fs_name( ( unsigned long ) st.f_type ), ( unsigned long ) st.f_type );
    fflush( stdout );
    return 0;
}

/* 变参输出 + 立即 flush：exec 之后进程内存整体换掉，未 flush 的缓冲会丢。 */
static inline void pr_out( const char *fmt, ... )
{
    va_list ap;

    va_start( ap, fmt );
    vprintf( fmt, ap );
    va_end( ap );
    fflush( stdout );
}

#endif /* BSP04_PROOT_H */
