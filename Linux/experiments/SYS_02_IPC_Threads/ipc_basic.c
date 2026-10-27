/* SYS_02 阶段 2 · A 组：四种 IPC 的同一套收发对照
 *
 * 四种通道发同一批数据（3 条 × 1000 字节），只看两个读数：
 *   first_read  读端用 4096 字节缓冲**只读一次**拿到了多少
 *   boundary    消息边界还在不在（merged / preserved）
 *
 * 结论摘要：pipe / fifo / UDS-SOCK_STREAM 是字节流，边界被合并；
 *          UDS-SOCK_DGRAM 保留边界；共享内存两端看到同一份字节但没有同步语义，
 *          同步必须另配（本实验用一条匿名管道）。
 *
 * 每种用例都让读端先睡 100 ms，保证数据在读取之前已经全部到达内核缓冲，
 * 否则 first_read 会随调度变化，读数的差别会被误读成语义差别。
 *
 * 一个必须写对的细节：读端在读完数据之后要读到 EOF，而 EOF 只在**所有**写端都关闭后出现。
 * fork 出来的子进程天然持有父进程写端的副本，如果不在子进程里 close，
 * 读循环永远等不到 EOF——这不是语义问题，是资源所有权问题。
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MSG_SIZE 1000
#define MSG_COUNT 3
#define READ_BUF 4096
#define TMP_FIFO "/tmp/sys02_ipc.fifo"
#define SHM_NAME "/sys02_shm"
#define SHM_SIZE 4096

enum chan { CH_PIPE, CH_FIFO, CH_UDS_STREAM };

static void pr(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

static void pr(const char *fmt, ...)
{
    char buf[512];
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (n > 0) {
        size_t len = (size_t)n < sizeof buf ? (size_t)n : sizeof buf - 1;

        /* 必须消费返回值：glibc 把 write 标成 warn_unused_result，
         * 用 (void) 丢弃在 -Werror 下仍然报错。 */
        if (write(STDOUT_FILENO, buf, len) < 0) {
            /* 取证行都写不出去时没有更合适的补救，保持静默 */
        }
    }
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

/* 消息内容带序号，便于人工核对；长度固定，避免读端按长度区分边界。 */
static void fill_msg(char *buf, int idx)
{
    memset(buf, 'A' + idx, MSG_SIZE);
    (void)snprintf(buf, MSG_SIZE - 1, "MSG-%d:", idx);
}

static const char *chan_name(int kind)
{
    switch (kind) {
    case CH_FIFO:
        return "fifo";
    case CH_UDS_STREAM:
        return "uds-stream";
    default:
        return "pipe";
    }
}

/* 字节流通道：写 3 条，读端只读一次，再读到 EOF 核对总字节。
 * fds 由调用方创建：pipe() 或 socketpair()；FIFO 组不用 fds，按名字打开。 */
static void run_byte_stream(int kind, int fds[2])
{
    const char *tag = chan_name(kind);
    char *buf;
    pid_t pid;

    pid = fork();
    if (pid < 0) {
        pr("SYS02|ipc|%s|fork_fail|errno=%d\n", tag, errno);
        return;
    }

    if (pid == 0) {
        int rd;
        ssize_t first;
        int reads = 0;
        size_t total = 0;

        if (kind == CH_FIFO) {
            rd = open(TMP_FIFO, O_RDONLY); /* 阻塞到写端打开为止 */
            if (rd < 0)
                _exit(1);
        } else {
            rd = fds[0];
            close(fds[1]); /* 关键：不关写端副本，读循环永远等不到 EOF */
        }

        buf = malloc(READ_BUF);
        if (!buf)
            _exit(2);
        usleep(100 * 1000); /* 先睡再读：让三条消息都落进内核缓冲 */
        first = read(rd, buf, READ_BUF);
        total = first > 0 ? (size_t)first : 0;
        reads = 1;
        for (;;) {
            ssize_t n = read(rd, buf, READ_BUF);

            if (n <= 0)
                break;
            total += (size_t)n;
            reads++;
        }
        pr("SYS02|ipc|%s|sent=%d|first_read=%zd|reads=%d|total=%zu|boundary=%s\n", tag,
           MSG_SIZE * MSG_COUNT, first, reads, total,
           (first > MSG_SIZE) ? "merged" : "single");
        free(buf);
        close(rd);
        _exit(0);
    }

    {
        int wr;
        char msg[MSG_SIZE];

        if (kind == CH_FIFO) {
            wr = open(TMP_FIFO, O_WRONLY);
            if (wr < 0) {
                pr("SYS02|ipc|%s|open_w_fail|errno=%d\n", tag, errno);
                (void)waitpid(pid, NULL, 0);
                return;
            }
            pr("SYS02|ipc|fifo|created=1\n");
        } else {
            wr = fds[1];
            close(fds[0]); /* 父进程不读，关掉读端副本 */
        }

        for (int i = 0; i < MSG_COUNT; i++) {
            fill_msg(msg, i);
            if (write_all(wr, msg, MSG_SIZE) < 0)
                pr("SYS02|ipc|%s|write_fail|errno=%d\n", tag, errno);
        }
        close(wr); /* 关掉最后一个写端，读端才会见到 EOF */
        (void)waitpid(pid, NULL, 0);
    }
}

static void case_pipe(void)
{
    int fds[2];

    if (pipe(fds) < 0) {
        pr("SYS02|ipc|pipe|create_fail|errno=%d\n", errno);
        return;
    }
    run_byte_stream(CH_PIPE, fds);
}

static void case_fifo(void)
{
    (void)unlink(TMP_FIFO);
    if (mkfifo(TMP_FIFO, 0644) < 0) {
        pr("SYS02|ipc|fifo|mkfifo_fail|errno=%d\n", errno);
        return;
    }
    run_byte_stream(CH_FIFO, NULL);
    (void)unlink(TMP_FIFO); /* 命名管道是文件系统对象，不删就一直留在 /tmp */
}

static void case_uds_stream(void)
{
    int sv[2];

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) {
        pr("SYS02|ipc|uds-stream|socketpair_fail|errno=%d\n", errno);
        return;
    }
    run_byte_stream(CH_UDS_STREAM, sv);
}

static void case_uds_dgram(void)
{
    int sv[2];
    pid_t pid;

    if (socketpair(AF_UNIX, SOCK_DGRAM, 0, sv) < 0) {
        pr("SYS02|ipc|uds-dgram|socketpair_fail|errno=%d\n", errno);
        return;
    }
    pid = fork();
    if (pid < 0) {
        pr("SYS02|ipc|uds-dgram|fork_fail|errno=%d\n", errno);
        close(sv[0]);
        close(sv[1]);
        return;
    }
    if (pid == 0) {
        char *buf = malloc(READ_BUF);
        int msgs = 0;
        size_t first_len = 0;
        char lens[128] = "";
        size_t used = 0;

        close(sv[0]);
        if (!buf)
            _exit(1);
        usleep(100 * 1000);
        for (;;) {
            ssize_t n = recv(sv[1], buf, READ_BUF, 0);

            if (n <= 0)
                break;
            /* SOCK_DGRAM 没有 EOF 语义：对端 close 之后 recv 仍然阻塞，
             * 所以结束条件必须由应用层给出。这里用 1 字节的哨兵消息。 */
            if (n == 1 && buf[0] == '\0')
                break;
            if (msgs == 0)
                first_len = (size_t)n;
            msgs++;
            if (used < sizeof lens)
                used += (size_t)snprintf(lens + used, sizeof lens - used, "%s%zd",
                                         used ? "," : "", n);
        }
        pr("SYS02|ipc|uds-dgram|sent=%d|msgs=%d|lens=%s|first_len=%zu|boundary=%s|"
           "eof_via=sentinel\n",
           MSG_SIZE * MSG_COUNT, msgs, lens, first_len,
           (msgs == MSG_COUNT && first_len == MSG_SIZE) ? "preserved" : "broken");
        free(buf);
        close(sv[1]);
        _exit(0);
    }

    {
        char msg[MSG_SIZE];

        close(sv[1]);
        for (int i = 0; i < MSG_COUNT; i++) {
            fill_msg(msg, i);
            if (send(sv[0], msg, MSG_SIZE, 0) < 0)
                pr("SYS02|ipc|uds-dgram|send_fail|errno=%d\n", errno);
        }
        if (send(sv[0], "", 1, 0) < 0) /* 哨兵：结束条件由应用层给出 */
            pr("SYS02|ipc|uds-dgram|sentinel_fail|errno=%d\n", errno);
        close(sv[0]);
        (void)waitpid(pid, NULL, 0);
    }
}

static void case_shm(void)
{
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0644);
    int sync_pipe[2];
    void *p;
    pid_t pid;

    if (fd < 0) {
        pr("SYS02|ipc|shm|open_fail|errno=%d\n", errno);
        return;
    }
    (void)shm_unlink(SHM_NAME); /* 先解除名字，fd 仍有效；进程退出后自动回收 */
    if (ftruncate(fd, SHM_SIZE) < 0) {
        pr("SYS02|ipc|shm|truncate_fail|errno=%d\n", errno);
        close(fd);
        return;
    }
    p = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) {
        pr("SYS02|ipc|shm|mmap_fail|errno=%d\n", errno);
        close(fd);
        return;
    }
    if (pipe(sync_pipe) < 0) {
        pr("SYS02|ipc|shm|pipe_fail|errno=%d\n", errno);
        munmap(p, SHM_SIZE);
        close(fd);
        return;
    }

    /* 共享内存本身没有同步语义：写完之后要靠别的机制通知对方。
     * 这里用一条匿名管道只是为了让读数可复现，不属于共享内存提供的能力。 */
    *(volatile int *)p = 12345;

    pid = fork();
    if (pid < 0) {
        pr("SYS02|ipc|shm|fork_fail|errno=%d\n", errno);
        return;
    }
    if (pid == 0) {
        char c = 0;
        int child_value;

        close(sync_pipe[1]);
        if (read(sync_pipe[0], &c, 1) < 0)
            _exit(1);
        child_value = *(volatile int *)p;
        pr("SYS02|ipc|shm|child_value=%d|same=%d|sync_via=pipe\n", child_value,
           child_value == 12345);
        munmap(p, SHM_SIZE);
        close(fd);
        close(sync_pipe[0]);
        _exit(0);
    }
    close(sync_pipe[0]);
    if (write(sync_pipe[1], "x", 1) < 0)
        pr("SYS02|ipc|shm|sync_fail|errno=%d\n", errno);
    close(sync_pipe[1]);
    (void)waitpid(pid, NULL, 0);

    /* 名字已经被 unlink，再按名字打开必须失败——这是"命名 IPC 的生命周期"判据 */
    munmap(p, SHM_SIZE);
    close(fd);
    {
        int again = shm_open(SHM_NAME, O_RDONLY, 0);

        pr("SYS02|ipc|shm|after_unlink=%s|errno=%d\n", again < 0 ? "gone" : "alive",
           again < 0 ? errno : 0);
        if (again >= 0)
            close(again);
    }
}

int main(int argc, char **argv)
{
    const char *mode = argc > 1 ? argv[1] : "pipe";

    if (strcmp(mode, "pipe") == 0)
        case_pipe();
    else if (strcmp(mode, "fifo") == 0)
        case_fifo();
    else if (strcmp(mode, "shm") == 0)
        case_shm();
    else if (strcmp(mode, "uds-stream") == 0)
        case_uds_stream();
    else if (strcmp(mode, "uds-dgram") == 0)
        case_uds_dgram();
    else {
        pr("用法: ipc_basic <pipe|fifo|shm|uds-stream|uds-dgram>\n");
        return 2;
    }
    pr("SYS02|ipc|done|mode=%s\n", mode);
    return 0;
}
