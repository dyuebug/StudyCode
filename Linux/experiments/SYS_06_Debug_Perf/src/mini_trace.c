/* mini_trace.c —— 用 ptrace 实现一个最小 strace
 *
 * 本机没有 strace 二进制（也不允许 sudo 安装），所以这里自己做一个：
 * 跟踪 syscall 进出、解码少数参数（字符串）、做 fd 收支统计。
 *
 * 明确不做的（README §7 有完整边界）：
 *   - 不跟随子进程/线程（没有 PTRACE_O_TRACEFORK/CLONE）
 *   - 不解析返回值指向的结构体（只给 ret 数值）
 *   - 只解码 open/openat/execve/access/newfstatat 的第一个字符串参数
 *   - 除 SIGTRAP 外的信号一律原样放行
 *
 * 关键点：entry/exit 成对识别靠内核保证的 syscall-stop（SIGTRAP|0x80），
 * 不用寄存器快照去猜——否则 vDSO/信号打断了配对就全乱了。
 *
 * 退出码：0 = 跟踪正常结束；2 = 用法错误；3 = ptrace 本身失败
 */
#define _GNU_SOURCE
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <unistd.h>

/* 内核 ABI：这三个常量与结构体定义抄自 <linux/ptrace.h>，
 * 不直接 include 它是因为它和 <sys/ptrace.h> 在若干结构体/宏上冲突。 */
#define PTRACE_GET_SYSCALL_INFO_NR 0x420e
#define INFO_OP_ENTRY  1
#define INFO_OP_EXIT   2

struct syscall_info {
        unsigned char op;
        unsigned char pad[3];
        unsigned int arch;
        unsigned long long ip;
        unsigned long long sp;
        union {
                struct { unsigned long long nr; unsigned long long args[6]; } entry;
                struct { long long rval; unsigned char is_error; } exit;
                struct { unsigned long long nr; unsigned long long args[6];
                         unsigned int ret_data; } seccomp;
        };
};

struct name_ent { long nr; const char *name; };

static const struct name_ent NAMES[] = {
        { SYS_read, "read" },           { SYS_write, "write" },
        { SYS_open, "open" },           { SYS_openat, "openat" },
        { SYS_close, "close" },         { SYS_lseek, "lseek" },
        { SYS_mmap, "mmap" },           { SYS_munmap, "munmap" },
        { SYS_mprotect, "mprotect" },   { SYS_brk, "brk" },
        { SYS_execve, "execve" },       { SYS_exit_group, "exit_group" },
        { SYS_clock_gettime, "clock_gettime" },
        { SYS_getpid, "getpid" },       { SYS_futex, "futex" },
        { SYS_newfstatat, "newfstatat" }, { SYS_access, "access" },
        { SYS_readlinkat, "readlinkat" }, { SYS_pread64, "pread64" },
        { SYS_arch_prctl, "arch_prctl" }, { SYS_set_tid_address, "set_tid_address" },
        { SYS_set_robust_list, "set_robust_list" }, { SYS_rseq, "rseq" },
        { SYS_prlimit64, "prlimit64" }, { SYS_ioctl, "ioctl" },
        { SYS_fcntl, "fcntl" },         { SYS_socket, "socket" },
        { SYS_connect, "connect" },     { SYS_sendto, "sendto" },
        { SYS_recvfrom, "recvfrom" },   { SYS_poll, "poll" },
        { SYS_ppoll, "ppoll" },         { SYS_epoll_create1, "epoll_create1" },
        { SYS_epoll_ctl, "epoll_ctl" }, { SYS_epoll_wait, "epoll_wait" },
        { SYS_clone, "clone" },         { SYS_clone3, "clone3" },
        { SYS_getrandom, "getrandom" }, { SYS_uname, "uname" },
        { SYS_nanosleep, "nanosleep" },  { SYS_fstat, "fstat" },
};

static const char *name_of(long nr)
{
        for (size_t i = 0; i < sizeof NAMES / sizeof NAMES[0]; i++)
                if (NAMES[i].nr == nr)
                        return NAMES[i].name;
        return "?";
}

/* 从被跟踪进程读 C 字符串。失败时留空串而不是报错：
 * 参数指针在 execve 之后会失效，读不到是常态，不是致命错误。 */
static void read_cstr(pid_t pid, unsigned long long addr, char *out, size_t n)
{
        size_t got = 0;
        out[0] = '\0';
        if (addr == 0) { snprintf(out, n, "NULL"); return; }
        while (got < n - 1) {
                size_t chunk = n - 1 - got;
                if (chunk > 64) chunk = 64;
                struct iovec local  = { out + got, chunk };
                struct iovec remote = { (void *)(uintptr_t)(addr + got), chunk };
                ssize_t k = process_vm_readv(pid, &local, 1, &remote, 1, 0);
                if (k <= 0) break;
                if (memchr(out + got, '\0', (size_t)k) != NULL) return;
                got += (size_t)k;
                if ((size_t)k < chunk) break;
        }
        out[got] = '\0';
}

/* 哪个参数是"有意义的字符串"：不同 syscall 位置不同 */
static int str_arg_index(const char *name)
{
        if (!strcmp(name, "open") || !strcmp(name, "execve") ||
            !strcmp(name, "access") || !strcmp(name, "unlink"))
                return 0;
        if (!strcmp(name, "openat") || !strcmp(name, "newfstatat") ||
            !strcmp(name, "readlinkat"))
                return 1;
        return -1;
}

struct stats {
        long total;
        long opens;
        long closes;
        long clock_gettime;
        long execve;
};

int main(int argc, char **argv)
{
        int verbose = 0;
        int argi = 1;
        if (argi < argc && !strcmp(argv[argi], "--all")) { verbose = 1; argi++; }
        if (argi >= argc) {
                fprintf(stderr, "usage: %s [--all] program [args...]\n", argv[0]);
                return 2;
        }

        fflush(NULL);
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); return 3; }

        if (pid == 0) {
                if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) < 0) { perror("TRACEME"); _exit(127); }
                /* 先停一下，等父进程设置完选项再 exec，避免漏掉 loader 的早期 syscall */
                raise(SIGSTOP);
                execv(argv[argi], &argv[argi]);
                perror("execv");
                _exit(127);
        }

        int st;
        if (waitpid(pid, &st, 0) < 0) { perror("waitpid"); return 3; }
        if (ptrace(PTRACE_SETOPTIONS, pid, NULL,
                   (void *)(uintptr_t)(PTRACE_O_TRACESYSGOOD | PTRACE_O_EXITKILL)) < 0) {
                perror("SETOPTIONS");
                return 3;
        }

        struct stats S = { 0, 0, 0, 0, 0 };
        struct syscall_info ci;
        int have_entry = 0;
        long cur_nr = -1;
        unsigned long long cur_args[6] = { 0 };
        int printed = 0;
        int child_rc = -1;

        for (;;) {
                if (ptrace(PTRACE_SYSCALL, pid, NULL, NULL) < 0) { perror("PTRACE_SYSCALL"); return 3; }
                if (waitpid(pid, &st, 0) < 0) { perror("waitpid"); return 3; }

                if (WIFEXITED(st))   { child_rc = WEXITSTATUS(st); break; }
                if (WIFSIGNALED(st)) { child_rc = 128 + WTERMSIG(st); break; }
                if (!WIFSTOPPED(st)) continue;

                int sig = WSTOPSIG(st);
                if (sig != (SIGTRAP | 0x80)) {
                        /* 非 syscall-stop：吞掉 ptrace 自己的 SIGTRAP，其余原样放行 */
                        if (sig != SIGTRAP && sig != SIGSTOP)
                                ptrace(PTRACE_SYSCALL, pid, NULL, (void *)(uintptr_t)sig);
                        continue;
                }

                memset(&ci, 0, sizeof ci);
                if (ptrace(PTRACE_GET_SYSCALL_INFO_NR, pid, (void *)(uintptr_t)sizeof ci, &ci) < 0) {
                        perror("GET_SYSCALL_INFO");
                        return 3;
                }

                if (ci.op == INFO_OP_ENTRY) {
                        have_entry = 1;
                        cur_nr = (long)ci.entry.nr;
                        for (int i = 0; i < 6; i++) cur_args[i] = ci.entry.args[i];
                        continue;
                }

                if (ci.op != INFO_OP_EXIT || !have_entry) {
                        have_entry = 0;
                        continue;
                }
                have_entry = 0;

                const char *nm = name_of(cur_nr);
                long long rv = ci.exit.rval;
                S.total++;
                if (!strcmp(nm, "clock_gettime")) S.clock_gettime++;
                if (!strcmp(nm, "execve"))        S.execve++;
                if (rv >= 0 && (!strncmp(nm, "open", 4))) S.opens++;
                if (!strcmp(nm, "close") && rv == 0)      S.closes++;

                if (verbose || printed < 32) {
                        char path[128] = "";
                        int idx = str_arg_index(nm);
                        if (idx >= 0) read_cstr(pid, cur_args[idx], path, sizeof path);

                        if (path[0])
                                printf("S06|trace|call|name=%s|arg=%s|ret=%lld\n", nm, path, rv);
                        else if (!strcmp(nm, "?"))
                                printf("S06|trace|call|name=?|nr=%ld|ret=%lld\n", cur_nr, rv);
                        else
                                printf("S06|trace|call|name=%s|ret=%lld\n", nm, rv);
                        printed++;
                        if (!verbose && printed == 32)
                                printf("S06|trace|note|truncated=1|limit=32\n");
                }
        }

        printf("S06|trace|summary|syscalls=%ld|opens=%ld|closes=%ld|net_fd=%ld"
               "|clock_gettime=%ld|execve=%ld|child_rc=%d\n",
               S.total, S.opens, S.closes, S.opens - S.closes,
               S.clock_gettime, S.execve, child_rc);
        (void)have_entry;
        return 0;
}
