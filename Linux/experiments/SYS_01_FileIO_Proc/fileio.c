/* SYS_01 阶段 1：文件 I/O 与文件描述符的两组取证
 *
 * 每组只差一个变量，且都有独立的失败判据：
 *   A  O_APPEND 与"lseek + write"的原子性对照：同一份定长行布局、同一批子进程，
 *      唯一差别是父进程打开时有没有 O_APPEND。
 *   C  O_CLOEXEC：同一对 fd，只有打开标志不同，exec 之后一个还在、一个不在。
 *
 * 用共享 open file description（fork 前 open、fork 后共用）而不是各自 open，
 * 是因为各自 open 的 offset 互相独立，写不出竞态——那会得到一个"看起来没问题"的假结论。
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define WORKERS 8
#define LINES 200
#define LINE_MAX 16 /* "P07-0199\n" 是 9 字节，留余量给 -Werror 下的 snprintf 判定 */

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

static void pr(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

static void pr(const char *fmt, ...)
{
    char buf[512];
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (n > 0)
        (void)write_all(STDOUT_FILENO, buf, (size_t)n < sizeof buf ? (size_t)n : sizeof buf - 1);
}

/* 校验：每一行必须正好是 "P<两位数字>-<四位数字>\n"，共 9 字节。
 * 返回合法行数与总行数；不法行数 > 0 就说明写入被别的进程截断了。 */
static void scan_file(const char *path, long *good, long *bad, long *bytes)
{
    int fd = open(path, O_RDONLY);
    char buf[4096];
    ssize_t r;
    static char carry[LINE_MAX * 2];
    size_t clen = 0;

    *good = *bad = *bytes = 0;
    if (fd < 0) {
        pr("SYS01|scan|fail|errno=%d\n", errno);
        return;
    }

    while ((r = read(fd, buf, sizeof buf)) > 0) {
        *bytes += r;
        for (ssize_t i = 0; i < r; i++) {
            if (clen < sizeof carry)
                carry[clen++] = buf[i];
            if (buf[i] == '\n' || clen == sizeof carry) {
                int ok = (clen == 9 && carry[0] == 'P' && carry[3] == '-' &&
                          carry[8] == '\n');
                if (ok)
                    for (size_t k = 1; k <= 2; k++)
                        ok = ok && carry[k] >= '0' && carry[k] <= '9';
                if (ok)
                    for (size_t k = 4; k <= 7; k++)
                        ok = ok && carry[k] >= '0' && carry[k] <= '9';
                if (ok)
                    (*good)++;
                else
                    (*bad)++;
                clen = 0;
            }
        }
    }
    close(fd);
    if (clen > 0)
        (*bad)++; /* 尾部残行同样算不合法 */
}

/* A 组三种模式，都让每个子进程各自 open（各自独立的文件偏移量）：
 *
 *   MODE_APPEND_YIELD  O_APPEND 打开，写之前 sched_yield
 *   MODE_LSEEK_YIELD   lseek 到末尾 → sched_yield → write
 *   MODE_LSEEK_NATURAL lseek 到末尾 → write（不加 yield，记录自然发生概率）
 *
 * 为什么要有 yield：实测不加 yield 时 8 进程 × 200 行连跑 3 次只有 1 次出现损坏，
 * 概率性的东西不能当判据。yield 把"lseek 之后、write 之前"这个窗口人为放大到
 * 必然被别的进程插进来，于是对照变成确定性的——这是故障注入，不是自然现象，
 * 两组都注入，所以对照仍只差 O_APPEND 一个变量。
 *
 * 反例（首版写法）：若沿用父进程 open 的那一个 fd，偏移量在 fork 后是共享的，
 * lseek 的返回值又不被 write 使用，竞态反而消失，两组都会得到 1600 行全合法。
 */
enum { MODE_LSEEK_NATURAL = 0, MODE_APPEND_YIELD = 1, MODE_LSEEK_YIELD = 2 };

static const char *mode_name(int mode)
{
    switch (mode) {
    case MODE_APPEND_YIELD:
        return "O_APPEND+yield";
    case MODE_LSEEK_YIELD:
        return "lseek+yield";
    default:
        return "lseek+natural";
    }
}

static void exp_append(const char *path, int mode)
{
    const int use_append = (mode == MODE_APPEND_YIELD);
    const int use_yield = (mode != MODE_LSEEK_NATURAL);
    pid_t pids[WORKERS];
    long good = 0, bad = 0, bytes = 0;

    /* 先清空：否则上一组的数据会混进来 */
    {
        int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

        if (fd < 0) {
            pr("SYS01|append|create_fail|errno=%d\n", errno);
            return;
        }
        close(fd);
    }

    for (int i = 0; i < WORKERS; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            pr("SYS01|append|fork_fail|errno=%d\n", errno);
            return;
        }
        if (pid == 0) {
            int fd = open(path, O_WRONLY | (use_append ? O_APPEND : 0));
            char line[LINE_MAX];

            if (fd < 0)
                _exit(1);
            for (int k = 0; k < LINES; k++) {
                int n = snprintf(line, sizeof line, "P%02d-%04d\n", i, k);

                if (n < 0 || (size_t)n >= sizeof line)
                    _exit(2);
                if (!use_append) {
                    /* 无 O_APPEND：先定位末尾，write 用的是此刻的偏移量 */
                    if (lseek(fd, 0, SEEK_END) < 0)
                        _exit(3);
                }
                if (use_yield)
                    sched_yield(); /* 扇大窗口：让别的进程也定位到同一处 */
                if (write_all(fd, line, (size_t)n) < 0)
                    _exit(4);
            }
            close(fd);
            _exit(0);
        }
        pids[i] = pid;
    }

    int failed = 0;

    for (int i = 0; i < WORKERS; i++) {
        int st = 0;

        if (waitpid(pids[i], &st, 0) < 0 || !WIFEXITED(st) || WEXITSTATUS(st) != 0)
            failed++;
    }
    scan_file(path, &good, &bad, &bytes);
    pr("SYS01|append|mode=%s|workers=%d|lines=%d|expected=%d|good=%ld|bad=%ld|bytes=%ld|failed_children=%d\n",
       mode_name(mode), WORKERS, LINES, WORKERS * LINES, good, bad, bytes, failed);
}

/* C 组：两个 fd 同时留给子进程，一个带 O_CLOEXEC，一个不带。
 * 子进程不该"猜到"父进程的 fd 号，所以由父进程通过 argv 传进去。 */
static void exp_cloexec(const char *keep, const char *cloexec_path, char **self_argv)
{
    int fd_keep = open(keep, O_RDONLY);
    int fd_clo = open(cloexec_path, O_RDONLY | O_CLOEXEC);
    pid_t pid;
    int st = 0;

    if (fd_keep < 0 || fd_clo < 0) {
        pr("SYS01|cloexec|open_fail|errno=%d\n", errno);
        if (fd_keep >= 0)
            close(fd_keep);
        return;
    }

    pr("SYS01|cloexec|parent|keep_fd=%d|cloexec_fd=%d\n", fd_keep, fd_clo);

    pid = fork();
    if (pid == 0) {
        char a[16], b[16];

        snprintf(a, sizeof a, "%d", fd_keep);
        snprintf(b, sizeof b, "%d", fd_clo);
        execl(self_argv[0], self_argv[0], "--fd-child", a, b, (char *)NULL);
        _exit(127);
    }
    if (waitpid(pid, &st, 0) < 0 || !WIFEXITED(st))
        pr("SYS01|cloexec|child_fail\n");
    close(fd_keep);
    close(fd_clo);
}

int main(int argc, char **argv)
{
    if (argc == 4 && strcmp(argv[1], "--fd-child") == 0) {
        int fd_keep = atoi(argv[2]);
        int fd_clo = atoi(argv[3]);
        int r_keep = fcntl(fd_keep, F_GETFD);
        int r_clo = fcntl(fd_clo, F_GETFD);

        pr("SYS01|cloexec|child|keep_alive=%d|cloexec_alive=%d|errno=%d\n", r_keep >= 0,
           r_clo >= 0, errno);
        return 0;
    }

    exp_append("/tmp/sys01_append.txt", MODE_APPEND_YIELD);
    exp_append("/tmp/sys01_lseek_yield.txt", MODE_LSEEK_YIELD);
    exp_append("/tmp/sys01_lseek_natural.txt", MODE_LSEEK_NATURAL);

    {
        int fd = open("/tmp/sys01_src.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);

        if (fd < 0) {
            pr("SYS01|cloexec|setup_fail|errno=%d\n", errno);
            return 1;
        }
        close(fd);
    }
    exp_cloexec("/tmp/sys01_src.txt", "/tmp/sys01_src.txt", argv);
    pr("SYS01|done\n");
    return 0;
}
