/**
 * 最小运行时补丁：承接 crt0 / crti / libnosys 的义务
 *
 * 背景：链接时使用了 -nostartfiles。原因是 vendor/startup/startup_stm32f103xb.s
 * 已经完成复位入口、.data 拷贝、.bss 清零，再链接默认的 crt0.o 属于重复；
 * 而且 crt0.o 会为了 exit() 拉入 newlib 的 stdio，进而引用
 * _write/_read/_lseek/_close，使每次构建都出现 libnosys 的"未实现"告警。
 *
 * 代价：crti.o 提供的 _init / _fini 不再存在，而 newlib 的
 * __libc_init_array() 会调用 _init()，所以这里补空实现。
 * C++ 全局构造不受影响：__libc_init_array() 仍然遍历 .init_array。
 *
 * 本文件另外提供与硬件无关的 libnosys 替代桩：
 *   _sbrk   —— newlib 分配 FILE 结构等少量堆内存时需要；libnosys 的版本恒返回 -1
 *   _isatty —— 决定 stdout 的缓冲策略；libnosys 恒返回 0（非终端）会使 printf 全缓冲
 *   _close / _fstat / _lseek / _read —— printf 初始化 stdio 时会被引用；
 *             不提供则每次链接出现四条 "is not implemented and will always fail" 告警
 *   _write  —— 由各实验自行实现（目标串口不同），不在此处定义
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>

/* 链接脚本导出的符号：预留区起点（堆底）与堆预算 */
extern char end;
extern char _Min_Heap_Size;

static char *s_heap_cur = NULL;
static char *s_heap_end = NULL;

/**
 * 堆区 = [end, end + _Min_Heap_Size)，即链接脚本 ._user_heap_stack 的前 512 B。
 * 之所以不用 [_estack - _Min_Stack_Size) 作为上界：预留区被链接脚本放在 end 处，
 * 而运行期栈从 _estack（RAM 末尾）向下增长，两者相差整个剩余 RAM。
 * 以预留区为界，heap 与 stack 的运行期预算才与链接期检查一致。
 */
void *_sbrk(ptrdiff_t incr)
{
    if (s_heap_cur == NULL) {
        s_heap_cur = &end;
        s_heap_end = &end + (uintptr_t)&_Min_Heap_Size;
    }
    if ((incr > 0) && ((s_heap_cur + incr) > s_heap_end)) {
        errno = ENOMEM;
        return (void *)-1;
    }
    char *prev = s_heap_cur;
    s_heap_cur += incr;
    return prev;
}

/**
 * newlib 在初始化 stdout 时调用。返回 0（非终端）会使 stdout 变成全缓冲：
 * 不满一行或缓冲区未满就不调用 _write，表现是"printf 不输出"。
 * 这里声称所有描述符都是终端，使 stdout 采用行缓冲（遇到 '\n' 刷新）。
 */
int _isatty(int fd)
{
    (void)fd;
    return 1;
}

/**
 * 下面四个桩只为消除 libnosys 的链接告警，语义都是"没有对应能力"。
 * _fstat 声明为字符设备，与 _isatty 的结论保持一致。
 */
int _close(int fd)
{
    (void)fd;
    return -1;
}

int _fstat(int fd, struct stat *st)
{
    (void)fd;
    st->st_mode = S_IFCHR;
    return 0;
}

int _lseek(int fd, int offset, int whence)
{
    (void)fd;
    (void)offset;
    (void)whence;
    return 0;
}

/* 串口输入由各实验的轮询/中断路径处理，不走 stdin */
int _read(int fd, char *ptr, int len)
{
    (void)fd;
    (void)ptr;
    (void)len;
    return 0;
}

void _init(void) { }
void _fini(void) { }
