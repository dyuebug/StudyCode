/* crash.c —— 没有 gdb 时怎么拿到崩溃现场
 *
 * 三个子命令：
 *   null  f1 -> f2 -> f3 解引用空指针，停在打印回溯
 *   deep  无限递归耗尽栈，验证信号处理器在 altstack 上仍能跑
 *   ok    不触发任何信号（配对基线：frames=0）
 *
 * 输出里的 off= 是"运行时地址 − 该模块装载基址"，
 * 只有它才能直接喂给 addr2line（PIE 下绝对地址不行，README §8.3 有实测）。
 *
 * 已知代价（如实标注，不假装这是生产级故障采集）：
 *   backtrace()/dladdr()/snprintf() 都不是 async-signal-safe，处理器里仍在用它们；
 *   本实验只把它们当作"能跑通的最小路径"，真生产环境应改用预先写好的回溯表。
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>
#include <unistd.h>

#define MAXFRAMES 64
static char altstack_mem[64 * 1024] __attribute__((aligned(16)));

static void emit(const char *buf, int len)
{
        if (len > 0) {
                ssize_t r = write(STDOUT_FILENO, buf, (size_t)len);
                (void)r;
        }
}

static void on_sig(int sig, siginfo_t *si, void *uctx)
{
        char line[256];
        void *bt[MAXFRAMES];

        /* 回溯数组里混着两种地址：
         *   - 故障 PC（出错那一条指令的地址，来自 ucontext）
         *   - 返回地址（调用点的下一条指令）
         * 前者不能减 1，后者必须减 1 才能落回调用行。
         * 这里把 PC 单独打出来，判定器据此决定要不要减（README §8.3）。 */
        unsigned long fault_pc = 0;
        if (uctx != NULL) {
                const ucontext_t *uc = (const ucontext_t *)uctx;
                fault_pc = (unsigned long)uc->uc_mcontext.gregs[REG_RIP];
        }

        int n = backtrace(bt, MAXFRAMES);
        emit(line, snprintf(line, sizeof line,
                            "S06|crash|siginfo|sig=%d|addr=%p|pc=0x%lx|frames=%d\n",
                            sig, si->si_addr, fault_pc, n));

        for (int i = 0; i < n; i++) {
                Dl_info info;
                int is_pc = ((unsigned long)bt[i] == fault_pc);
                if (dladdr(bt[i], &info) != 0 && info.dli_fbase != NULL) {
                        unsigned long off =
                            (unsigned long)((const char *)bt[i] - (const char *)info.dli_fbase);
                        int self = info.dli_fname && strstr(info.dli_fname, "crash") != NULL;
                        emit(line, snprintf(line, sizeof line,
                                            "S06|crash|bt|i=%d|addr=0x%lx|off=0x%lx|mod=%s|is_pc=%d\n",
                                            i, (unsigned long)bt[i], off,
                                            self ? "self" : "other", is_pc));
                } else {
                        emit(line, snprintf(line, sizeof line,
                                            "S06|crash|bt|i=%d|addr=0x%lx|mod=?|is_pc=%d\n",
                                            i, (unsigned long)bt[i], is_pc));
                }
        }
        _exit(3);
}

__attribute__((noinline)) static int f3(void)
{
        volatile int *p = NULL;
        return *p;
}

__attribute__((noinline)) static int f2(int x)
{
        return f3() + x;
}

__attribute__((noinline)) static int f1(void)
{
        return f2(1);
}

/* 用 volatile 保证 pad 真占栈；返回值相加保证不是尾调用（尾调用会被优化成循环）。 */
__attribute__((noinline)) static int deep(int n)
{
        volatile char pad[4096];
        pad[0] = (char)n;
        if (n <= 0) return pad[0];
        return deep(n - 1) + pad[0];
}

int main(int argc, char **argv)
{
        const char *what = argc > 1 ? argv[1] : "ok";

        stack_t ss;
        memset(&ss, 0, sizeof ss);
        ss.ss_sp = altstack_mem;
        ss.ss_size = sizeof altstack_mem;
        ss.ss_flags = 0;
        if (sigaltstack(&ss, NULL) != 0) { perror("sigaltstack"); return 3; }

        struct sigaction sa;
        memset(&sa, 0, sizeof sa);
        sa.sa_sigaction = on_sig;
        sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
        sigemptyset(&sa.sa_mask);
        if (sigaction(SIGSEGV, &sa, NULL) != 0 || sigaction(SIGBUS, &sa, NULL) != 0) {
                perror("sigaction");
                return 3;
        }

        if (!strcmp(what, "null")) {
                printf("S06|crash|start|mode=null\n");
                fflush(NULL);
                return f1();
        }
        if (!strcmp(what, "deep")) {
                printf("S06|crash|start|mode=deep\n");
                fflush(NULL);
                return deep(100000);
        }
        if (!strcmp(what, "ok")) {
                printf("S06|crash|ok|frames=0\n");
                return 0;
        }
        fprintf(stderr, "unknown mode: %s\n", what);
        return 2;
}
