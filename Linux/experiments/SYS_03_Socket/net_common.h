/* SYS_03 阶段 3 · 公共设施
 *
 * 只放"和实验结论无关"的东西：日志输出、非阻塞开关、回环地址上的监听/连接。
 * 每个用例自己的判据必须写在各自的 .c 里，不能藏进公共头文件。
 *
 * 全部函数声明为 `static inline`：单文件没用到时不会触发 -Wunused-function。
 */
#ifndef NET_COMMON_H
#define NET_COMMON_H

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/* 单调时钟，只用于"等了多久"，不受系统时间调整影响 */
static inline long long now_ms(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static inline void pr(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

static inline void pr(const char *fmt, ...)
{
    char buf[1024];
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (n > 0) {
        size_t len = (size_t)n < sizeof buf ? (size_t)n : sizeof buf - 1;

        /* 必须消费返回值：glibc 把 write 标成 warn_unused_result */
        if (write(STDOUT_FILENO, buf, len) < 0) {
            /* 取证行都写不出去时没有更合适的补救 */
        }
    }
}

static inline int set_nonblock(int fd)
{
    int fl = fcntl(fd, F_GETFL, 0);

    if (fl < 0)
        return -1;
    return fcntl(fd, F_SETFL, fl | O_NONBLOCK);
}

/* 监听 127.0.0.1 的临时端口（bind 端口 0），返回监听 fd 并回填端口 */
static inline int listen_loopback(int *port_out)
{
    struct sockaddr_in addr;
    socklen_t alen = sizeof addr;
    int one = 1;
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd < 0)
        return -1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one) < 0)
        goto fail;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    if (bind(fd, (struct sockaddr *)&addr, sizeof addr) < 0)
        goto fail;
    if (listen(fd, 16) < 0)
        goto fail;
    if (getsockname(fd, (struct sockaddr *)&addr, &alen) < 0)
        goto fail;
    if (port_out)
        *port_out = ntohs(addr.sin_port);
    return fd;

fail:
    close(fd);
    return -1;
}

static inline int connect_loopback(int port)
{
    struct sockaddr_in addr;
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd < 0)
        return -1;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons((unsigned short)port);
    if (connect(fd, (struct sockaddr *)&addr, sizeof addr) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

/* 阻塞地把 len 字节写干净；被信号打断就重试 */
static inline int write_all(int fd, const void *buf, size_t len)
{
    const char *p = buf;
    size_t left = len;

    while (left > 0) {
        ssize_t n = write(fd, p, left);

        if (n < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (n == 0)
            return -1;
        p += (size_t)n;
        left -= (size_t)n;
    }
    return 0;
}

/* 只读一次；EINTR 重试，其余错误原样返回给调用方判断 */
static inline ssize_t recv_retry(int fd, void *buf, size_t len)
{
    for (;;) {
        ssize_t n = recv(fd, buf, len, 0);

        if (n < 0 && errno == EINTR)
            continue;
        return n;
    }
}

#endif /* NET_COMMON_H */
