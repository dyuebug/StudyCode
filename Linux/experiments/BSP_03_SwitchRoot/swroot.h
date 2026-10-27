/* BSP_03 共用的小工具：日志输出与 f_type 名字映射
 *
 * 两个阶段都跑在 PID 1 的位置上，没有任何库可依赖：不能用 perror（它读 locale），
 * 也不能依赖 stderr 一定连着串口——所以统一用 stdout + 显式 fflush，
 * 保证每一行在进程被切换/关掉之前已经落到 tty。
 *
 * 输出约定：全部 ASCII，前缀 "SWROOT|"，字段用 '|' 分隔，便于在串口日志里 grep。
 */
#ifndef BSP03_SWROOT_H
#define BSP03_SWROOT_H

#include <stdarg.h>
#include <stdio.h>
#include <sys/vfs.h>

/* f_type 是内核内部的魔数，不是文件名。判"根到底换了没有"只能看它：
 * 在 initramfs 下 / 的魔数是 tmpfs 或 ramfs（见下），切根之后是 ext4（0xef53）。
 * 目录里有没有文件会被"我本来就把文件拷进去了"骗过去，魔数不会。
 *
 * 两个与"教科书说法"不符、本内核实测的细节（别照背旧结论）：
 *   1. rootfs 的后端可以是 shmem 也可以是 ramfs。init/do_mounts.c 的 init_rootfs()
 *      在命令行既没有 root= 也没有 rootfstype= 且 CONFIG_TMPFS=y 时设 is_tmpfs，
 *      于是 f_type 报 TMPFS_MAGIC(0x01021994)；给了 root= 才回落到 RAMFS_MAGIC(0x858458f6)。
 *      /proc/mounts 里两种情况都写作 "rootfs"，只有 f_type 能区分。
 *   2. devtmpfs 以内核里的 shmem/ramfs 作为后端（drivers/base/devtmpfs.c），
 *      它的 statfs 继承后端实现，因此 f_type 也是 0x01021994——
 *      **本内核上 f_type 无法区分 /dev 是 devtmpfs 还是 tmpfs**。要区分得读 /proc/self/mounts 的
 *      类型字段（stage2 里就是这么做的）。
 */
static inline const char *sw_fs_name( unsigned long t )
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

/* 打印某个路径上的 f_type。路径不存在时也照样留痕（absent 本身就是证据）。
 * 返回 0 成功，-1 失败（errno 已打印）。 */
static inline int sw_report_fs( const char *tag, const char *path )
{
    struct statfs st;

    if( statfs( path, &st ) != 0 )
    {
        printf( "SWROOT|%s|absent\n", tag );
        fflush( stdout );
        return -1;
    }

    printf( "SWROOT|%s|type=%s|magic=0x%lx\n",
            tag, sw_fs_name( ( unsigned long ) st.f_type ), ( unsigned long ) st.f_type );
    fflush( stdout );
    return 0;
}

/* 变参输出 + 立即 flush。用 vprintf 而不是自己拼缓冲区：
 * 阶段切换点之后进程内存会被 exec 换掉，任何未 flush 的缓冲都会丢。 */
static inline void sw_out( const char *fmt, ... )
{
    va_list ap;

    va_start( ap, fmt );
    vprintf( fmt, ap );
    va_end( ap );
    fflush( stdout );
}

#endif /* BSP03_SWROOT_H */
