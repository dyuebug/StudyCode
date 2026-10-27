/* victim.c —— 给 mini_trace 与 sanitizer 用的"被测程序"
 *
 * 每个子命令对应一个可被判定的行为，缺陷都是刻意注入的：
 *   fdleak  打开 3 个 fd 只关 1 个 -> mini_trace 的 net_fd=2
 *   fdok    打开 3 个关 3 个      -> net_fd=0（配对基线）
 *   vdso    1000 次 vDSO 调用 + 1000 次真 syscall
 *           -> ptrace 只看得见后者，这是工具的盲区而不是程序少调用了
 */
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char **argv)
{
        const char *what = argc > 1 ? argv[1] : "fdok";

        if (!strcmp(what, "fdleak")) {
                int fd[3];
                for (int i = 0; i < 3; i++)
                        fd[i] = open("/dev/null", O_RDONLY);
                if (fd[0] >= 0) close(fd[0]);   /* 只关一个：另外两个故意泄漏 */
                printf("S06|victim|fdleak|opened=3|closed=1\n");
                return 0;
        }

        if (!strcmp(what, "fdok")) {
                for (int i = 0; i < 3; i++) {
                        int fd = open("/dev/null", O_RDONLY);
                        if (fd >= 0) close(fd);
                }
                printf("S06|victim|fdok|opened=3|closed=3\n");
                return 0;
        }

        if (!strcmp(what, "vdso")) {
                struct timespec ts;
                long sum = 0;
                for (int i = 0; i < 1000; i++) {
                        clock_gettime(CLOCK_MONOTONIC, &ts);   /* glibc -> vDSO，不进内核 */
                        sum += ts.tv_nsec & 1;
                }
                for (int i = 0; i < 1000; i++) {
                        syscall(SYS_clock_gettime, CLOCK_MONOTONIC, &ts); /* 真 syscall */
                        sum += ts.tv_nsec & 1;
                }
                printf("S06|victim|vdso|vdso_calls=1000|raw_calls=1000|sum=%ld\n", sum);
                return 0;
        }

        fprintf(stderr, "unknown mode: %s\n", what);
        return 2;
}
