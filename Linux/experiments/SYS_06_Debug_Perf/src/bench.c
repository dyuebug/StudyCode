/* bench.c —— 没有 perf 时能测什么、测不了什么
 *
 *   vdso        clock_gettime(CLOCK_MONOTONIC) —— 走 vDSO，不进内核
 *   syscall     syscall(SYS_getpid) —— 真进内核，与上一条构成配对
 *   pagefault   每页写一个字节，用 getrusage 的 ru_minflt 数缺页；第二遍应为 0
 *   ctxswitch   socketpair 上的 ping-pong，测一次上下文切换的往返代价
 *
 * 全部用 CLOCK_MONOTONIC 取值、取三轮中位数。
 * 本文件不产生"绝对性能结论"，只产生同机同进程内的相对关系（README §10）。
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static long long now_ns(void)
{
        struct timespec t;
        clock_gettime(CLOCK_MONOTONIC, &t);
        return (long long)t.tv_sec * 1000000000LL + (long long)t.tv_nsec;
}

static int cmp_ll(const void *a, const void *b)
{
        long long x = *(const long long *)a, y = *(const long long *)b;
        return (x > y) - (x < y);
}

static void bench_vdso(int n)
{
        long long runs[3];
        struct timespec ts;
        for (int r = 0; r < 3; r++) {
                long long t0 = now_ns();
                long acc = 0;
                for (int i = 0; i < n; i++) { clock_gettime(CLOCK_MONOTONIC, &ts); acc += ts.tv_nsec & 1; }
                long long t1 = now_ns();
                runs[r] = t1 - t0;
                printf("S06|bench|case=vdso|run=%d|ns_per_op=%.2f|acc=%ld\n",
                       r, (double)(t1 - t0) / n, acc);
        }
        qsort(runs, 3, sizeof runs[0], cmp_ll);
        printf("S06|bench|case=vdso|median_ns=%.2f|ops=%d\n", (double)runs[1] / n, n);
}

static void bench_syscall(int n)
{
        long long runs[3];
        for (int r = 0; r < 3; r++) {
                long long t0 = now_ns();
                long acc = 0;
                for (int i = 0; i < n; i++) acc += syscall(SYS_getpid);
                long long t1 = now_ns();
                runs[r] = t1 - t0;
                printf("S06|bench|case=syscall|run=%d|ns_per_op=%.2f|acc=%ld\n",
                       r, (double)(t1 - t0) / n, acc & 1);
        }
        qsort(runs, 3, sizeof runs[0], cmp_ll);
        printf("S06|bench|case=syscall|median_ns=%.2f|ops=%d\n", (double)runs[1] / n, n);
}

static void bench_pagefault(long pages)
{
        size_t len = (size_t)pages * 4096;
        for (int r = 0; r < 3; r++) {
                char *p = mmap(NULL, len, PROT_READ | PROT_WRITE,
                               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                if (p == MAP_FAILED) { perror("mmap"); return; }
                /* 关掉 THP：否则 2 MiB 大页会让"每页一次缺页"这个前提直接不成立 */
                madvise(p, len, MADV_NOHUGEPAGE);

                struct rusage a, b, c;
                getrusage(RUSAGE_SELF, &a);
                long long t0 = now_ns();
                for (long i = 0; i < pages; i++) p[i * 4096] = 1;
                long long t1 = now_ns();
                getrusage(RUSAGE_SELF, &b);
                for (long i = 0; i < pages; i++) p[i * 4096] = (char)(p[i * 4096] + 1);
                long long t2 = now_ns();
                getrusage(RUSAGE_SELF, &c);

                printf("S06|bench|case=pagefault|run=%d|pages=%ld|first_minflt=%ld"
                       "|second_minflt=%ld|first_ns_per_page=%.1f|second_ns_per_page=%.1f\n",
                       r, pages,
                       b.ru_minflt - a.ru_minflt, c.ru_minflt - b.ru_minflt,
                       (double)(t1 - t0) / pages, (double)(t2 - t1) / pages);
                munmap(p, len);
        }
}

static void read_ctxswitches(long *vol, long *invol)
{
        *vol = *invol = -1;
        FILE *f = fopen("/proc/self/status", "r");
        if (!f) return;
        char line[256];
        while (fgets(line, sizeof line, f)) {
                long v;
                if (sscanf(line, "voluntary_ctxt_switches: %ld", &v) == 1) *vol = v;
                else if (sscanf(line, "nonvoluntary_ctxt_switches: %ld", &v) == 1) *invol = v;
        }
        fclose(f);
}

static void bench_ctxswitch(int n)
{
        int sv[2];
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) { perror("socketpair"); return; }

        fflush(NULL);
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); return; }

        if (pid == 0) {
                close(sv[0]);
                char c;
                while (read(sv[1], &c, 1) == 1)
                        if (write(sv[1], &c, 1) != 1) break;
                close(sv[1]);
                _exit(0);
        }

        close(sv[1]);
        long v0, i0, v1, i1;
        read_ctxswitches(&v0, &i0);

        char c = 'x';
        long long t0 = now_ns();
        int ok = 1;
        for (int i = 0; i < n; i++) {
                if (write(sv[0], &c, 1) != 1) { ok = 0; break; }
                if (read(sv[0], &c, 1) != 1) { ok = 0; break; }
        }
        long long t1 = now_ns();
        read_ctxswitches(&v1, &i1);

        close(sv[0]);
        waitpid(pid, NULL, 0);

        printf("S06|bench|case=ctxswitch|roundtrips=%d|ok=%d|ns_per_rt=%.1f"
               "|ns_per_switch=%.1f|voluntary=%ld|nonvoluntary=%ld\n",
               n, ok, (double)(t1 - t0) / n, (double)(t1 - t0) / (2.0 * n),
               v1 - v0, i1 - i0);
}

int main(int argc, char **argv)
{
        const char *what = argc > 1 ? argv[1] : "vdso";

        if (!strcmp(what, "vdso"))            bench_vdso(200000);
        else if (!strcmp(what, "syscall"))    bench_syscall(200000);
        else if (!strcmp(what, "pagefault"))  bench_pagefault(16384);
        else if (!strcmp(what, "ctxswitch"))  bench_ctxswitch(20000);
        else { fprintf(stderr, "unknown mode: %s\n", what); return 2; }
        return 0;
}
