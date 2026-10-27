/* perf_probe.c —— "perf 用不了"到底卡在哪一层
 *
 * 三件事，全部输出实测值：
 *   1. /proc/sys/kernel/perf_event_paranoid 的当前值
 *   2. 三种 perf_event_open 调用的返回值与 errno
 *        self（pid=0, exclude_kernel=1）—— 允许则 fd>=0
 *        systemwide（pid=-1, cpu=0）    —— paranoid>=1 时应 EACCES
 *        other pid（pid=1）             —— 同上
 *   3. 如果 self 可用，就真的数一次 cycles/instructions，并算 IPC
 *
 * 结论落点：缺的不是"能力"，而是"权限 + 二进制"两层，两层的证据要分开。
 *
 * 首版手抄过一个精简的 perf_event_attr，结果三种调用全部 E2BIG(7)：
 * 内核按 size 字段判断结构体版本，抄漏一个字段就整条路走不通。
 * 这里改用内核头文件里的真结构体——省下的那点依赖不值得。
 */
#define _GNU_SOURCE
#include <errno.h>
#include <linux/perf_event.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>

static int open_ev(unsigned long long config, int pid, int cpu, int excl_kernel)
{
        struct perf_event_attr a;
        memset(&a, 0, sizeof a);
        a.type = PERF_TYPE_HARDWARE;
        a.size = sizeof a;
        a.config = config;
        a.disabled = 1;
        a.exclude_kernel = excl_kernel;
        a.exclude_hv = 1;
        return (int)syscall(SYS_perf_event_open, &a, pid, cpu, -1, 0);
}

static long long read_count(int fd)
{
        long long v = -1;
        ssize_t n = read(fd, &v, sizeof v);
        return n == (ssize_t)sizeof v ? v : -1;
}

static long read_paranoid(void)
{
        FILE *f = fopen("/proc/sys/kernel/perf_event_paranoid", "r");
        if (!f) return -999;
        long v = -998;
        if (fscanf(f, "%ld", &v) != 1) v = -997;
        fclose(f);
        return v;
}

/* 一段不会被优化掉的用户态工作 */
__attribute__((noinline)) static unsigned long work(unsigned long n)
{
        unsigned long acc = 0;
        for (unsigned long i = 0; i < n; i++)
                acc = acc * 1103515245UL + i + 12345UL;
        return acc;
}

int main(void)
{
        long paranoid = read_paranoid();

        int fd_self = open_ev(PERF_COUNT_HW_CPU_CYCLES, 0, -1, 1);
        int self_errno = fd_self < 0 ? errno : 0;
        int fd_sys = open_ev(PERF_COUNT_HW_CPU_CYCLES, -1, 0, 0);
        int sys_errno = fd_sys < 0 ? errno : 0;
        int fd_other = open_ev(PERF_COUNT_HW_CPU_CYCLES, 1, -1, 0);
        int other_errno = fd_other < 0 ? errno : 0;

        printf("S06|perf|paranoid=%ld|self_fd=%d|self_errno=%d"
               "|sys_fd=%d|sys_errno=%d|other_fd=%d|other_errno=%d\n",
               paranoid, fd_self, self_errno, fd_sys, sys_errno, fd_other, other_errno);

        if (fd_self >= 0) {
                int fd_insn = open_ev(PERF_COUNT_HW_INSTRUCTIONS, 0, -1, 1);
                ioctl(fd_self, PERF_EVENT_IOC_RESET, 0);
                if (fd_insn >= 0) ioctl(fd_insn, PERF_EVENT_IOC_RESET, 0);
                ioctl(fd_self, PERF_EVENT_IOC_ENABLE, 0);
                if (fd_insn >= 0) ioctl(fd_insn, PERF_EVENT_IOC_ENABLE, 0);

                volatile unsigned long sink = work(20000000UL);

                ioctl(fd_self, PERF_EVENT_IOC_DISABLE, 0);
                if (fd_insn >= 0) ioctl(fd_insn, PERF_EVENT_IOC_DISABLE, 0);
                long long cycles = read_count(fd_self);
                long long insns = fd_insn >= 0 ? read_count(fd_insn) : -1;
                double ipc = (cycles > 0 && insns > 0) ? (double)insns / (double)cycles : -1.0;
                printf("S06|perf|counters|cycles=%lld|instructions=%lld|ipc=%.2f|insn_fd=%d|sink=%lu\n",
                       cycles, insns, ipc, fd_insn, sink);
                if (fd_insn >= 0) close(fd_insn);
                close(fd_self);
        }
        if (fd_sys >= 0) close(fd_sys);
        if (fd_other >= 0) close(fd_other);
        return 0;
}
