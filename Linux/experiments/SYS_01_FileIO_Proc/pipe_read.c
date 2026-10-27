/* SYS_01 阶段 1：pipe 上的短读与 EINTR
 *
 * 两个结论都必须能被读数否证，而不是靠"我读过 man 手册"：
 *   A  一次 write(1000) 之后，用 64 字节缓冲读，read 不会一次给满——
 *      实测读数应当正好是 15 次 64 字节 + 1 次 40 字节。
 *   B  阻塞在空 pipe 上的 read 被信号打断时返回 -1/EINTR，
 *      如果没处理 EINTR 而直接当成"读失败"，数据就丢了。
 *
 * B 组刻意不用 SA_RESTART：默认行为就是"可重启的系统调用被中断后返回 EINTR"，
 * 加了 SA_RESTART 就测不到这条路径了。
 */
#define _GNU_SOURCE
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define TOTAL 1000
#define CHUNK 64

static volatile sig_atomic_t usr1_hits;

static void on_usr1(int sig)
{
    (void)sig;
    usr1_hits++; /* 异步信号里只做这一件事：改 sig_atomic_t */
}

static void ev(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
static void ev(const char *fmt, ...)
{
    char buf[512];
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (n > 0)
        (void)!write(STDOUT_FILENO, buf, (size_t)n < sizeof buf ? (size_t)n : sizeof buf - 1);
}

static int write_all(int fd, const char *buf, size_t len)
{
    size_t done = 0;

    while (done < len) {
        ssize_t n = write(fd, buf + done, len - done);

        if (n < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        done += (size_t)n;
    }
    return 0;
}

/* A 组：写端一次给满 1000 字节，读端用 64 字节缓冲循环读。 */
static void exp_short_read(void)
{
    int fds[2];
    pid_t pid;

    if (pipe(fds) < 0) {
        ev("SYS01|pipe|short|pipe_fail|errno=%d\n", errno);
        return;
    }

    pid = fork();
    if (pid == 0) {
        close(fds[1]);
        char buf[CHUNK];
        long total = 0;
        int reads = 0;
        int last = 0;

        for (;;) {
            ssize_t n = read(fds[0], buf, sizeof buf);

            if (n < 0) {
                if (errno == EINTR)
                    continue;
                ev("SYS01|pipe|short|read_fail|errno=%d\n", errno);
                _exit(1);
            }
            if (n == 0)
                break;
            total += n;
            reads++;
            last = (int)n;
        }
        ev("SYS01|pipe|short|total=%ld|reads=%d|last_chunk=%d|chunk_max=%d\n", total, reads, last,
           CHUNK);
        close(fds[0]);
        _exit(0);
    }

    close(fds[0]);
    {
        char payload[TOTAL];

        memset(payload, 'x', sizeof payload);
        if (write_all(fds[1], payload, sizeof payload) < 0)
            ev("SYS01|pipe|short|write_fail|errno=%d\n", errno);
    }
    close(fds[1]); /* 不关写端，子进程就永远等不到 EOF */
    {
        int st = 0;

        if (waitpid(pid, &st, 0) < 0)
            ev("SYS01|pipe|short|wait_fail|errno=%d\n", errno);
    }
}

/* B 组：子进程阻塞在 read 上，父进程发信号。 */
static void exp_eintr(void)
{
    int data[2];
    int sync[2];
    pid_t pid;

    if (pipe(data) < 0 || pipe(sync) < 0) {
        ev("SYS01|pipe|eintr|pipe_fail|errno=%d\n", errno);
        return;
    }

    pid = fork();
    if (pid == 0) {
        struct sigaction sa;
        char buf[64];
        char sig_ready = 'R';

        memset(&sa, 0, sizeof sa);
        sa.sa_handler = on_usr1;
        sa.sa_flags = 0; /* 不带 SA_RESTART，read 才会返回 EINTR */
        sigaction(SIGUSR1, &sa, NULL);

        close(data[1]);
        close(sync[0]);

        /* 先告诉父进程"我要读了"，避免信号打在 read 之前（那会变成"没被中断"） */
        if (write_all(sync[1], &sig_ready, 1) < 0)
            _exit(1);
        close(sync[1]);

        errno = 0;
        {
            ssize_t n = read(data[0], buf, sizeof buf);

            ev("SYS01|pipe|eintr|first_read|ret=%d|errno=%d|usr1_hits=%d\n", (int)n, errno,
               (int)usr1_hits);
        }

        /* 重试：这一步才是"处理 EINTR"与"放弃读取"的分界。
         * 成功时把 errno 打成 0——errno 只在上一次失败后才有意义，
         * 直接把旧值打出来会让人误以为重试也失败了（首版就是这样）。 */
        {
            ssize_t n;

            errno = 0;
            n = read(data[0], buf, sizeof buf);
            ev("SYS01|pipe|eintr|retry_read|ret=%d|errno=%d\n", (int)n, n < 0 ? errno : 0);
        }
        close(data[0]);
        _exit(0);
    }

    close(data[0]);
    close(sync[1]);
    {
        char ch;

        if (read(sync[0], &ch, 1) != 1)
            ev("SYS01|pipe|eintr|sync_fail\n");
    }
    close(sync[0]);

    usleep(50000); /* 让子进程确实进到 read 里面 */
    kill(pid, SIGUSR1);
    usleep(50000);

    if (write_all(data[1], "OK", 2) < 0)
        ev("SYS01|pipe|eintr|write_fail|errno=%d\n", errno);
    close(data[1]);

    {
        int st = 0;

        if (waitpid(pid, &st, 0) < 0)
            ev("SYS01|pipe|eintr|wait_fail|errno=%d\n", errno);
    }
}

int main(void)
{
    exp_short_read();
    exp_eintr();
    ev("SYS01|pipe|done\n");
    return 0;
}
