/* SYS_04 · 压测客户端
 *
 * 每个连接一个线程（客户端侧简单优先），协议与 server 一致：
 *   [4 字节大端长度][payload]
 *
 * 每条连接用不同的 payload 填充（= 连接序号），所以"串包"会被 memcmp 抓到，
 * 而不是被"内容一样、顺序不同"掩盖。
 *
 * 用法: loadgen <port> <clients> <msgs> <size> [recv_timeout_ms]
 * 输出: SYS04|gen|done|clients=..|msgs=..|size=..|verified=..|expect=..|mismatch=..|
 *       total_ms=..|max_conn_ms=..|timeouts=..
 */
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define MAX_CLIENTS 64

struct cli {
    int idx;
    int port;
    int msgs;
    int size;
    int timeout_ms;
    int gap_ms; /* 帧之间的间隔：让"一次事件 = 一次到达"成立，便于对照 EPOLLONESHOT */
    long verified;
    long mismatch;
    int timeouts;
    long long ms;
};

static long long now_ms(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

static int write_all(int fd, const void *buf, size_t n)
{
    const char *p = buf;

    while (n > 0) {
        ssize_t w = send(fd, p, n, MSG_NOSIGNAL);

        if (w > 0) {
            p += w;
            n -= (size_t)w;
            continue;
        }
        if (w < 0 && (errno == EINTR || errno == EAGAIN))
            continue;
        return -1;
    }
    return 0;
}

static int read_all(int fd, void *buf, size_t n)
{
    char *p = buf;

    while (n > 0) {
        ssize_t r = recv(fd, p, n, 0);

        if (r > 0) {
            p += r;
            n -= (size_t)r;
            continue;
        }
        if (r < 0 && errno == EINTR)
            continue;
        return -1; /* EOF 或超时 */
    }
    return 0;
}

static void *cli_main(void *arg)
{
    struct cli *c = arg;
    struct sockaddr_in a;
    struct timeval tv = { .tv_sec = c->timeout_ms / 1000,
                          .tv_usec = (c->timeout_ms % 1000) * 1000 };
    unsigned char *frame;
    unsigned char *back;
    uint32_t be;
    long long t0 = now_ms();
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int one = 1;

    if (fd < 0)
        return NULL;
    {
        struct timeval snd = { .tv_sec = 2, .tv_usec = 0 };

        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &snd, sizeof snd);
        setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);
    }
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    a.sin_port = htons((unsigned short)c->port);
    if (connect(fd, (struct sockaddr *)&a, sizeof a) < 0) {
        close(fd);
        c->ms = now_ms() - t0;
        return NULL;
    }

    frame = malloc((size_t)c->size + 4);
    back = malloc((size_t)c->size + 4);
    if (!frame || !back) {
        free(frame);
        free(back);
        close(fd);
        return NULL;
    }
    be = htonl((uint32_t)c->size);
    memcpy(frame, &be, 4);
    memset(frame + 4, (unsigned char)('a' + (c->idx % 26)), (size_t)c->size);

    for (int i = 0; i < c->msgs; i++) {
        if (write_all(fd, frame, (size_t)c->size + 4) < 0)
            break;
        if (read_all(fd, back, (size_t)c->size + 4) < 0) {
            c->timeouts++; /* 超时或 EOF：破坏版服务器会停在这里 */
            break;
        }
        if (memcmp(back, frame, (size_t)c->size + 4) == 0)
            c->verified++;
        else
            c->mismatch++;
        if (c->gap_ms > 0 && i + 1 < c->msgs) {
            struct timespec ts = { 0, (long)c->gap_ms * 1000000L };

            nanosleep(&ts, NULL);
        }
    }

    shutdown(fd, SHUT_WR);
    {
        char sink[256];

        while (recv(fd, sink, sizeof sink, 0) > 0)
            ; /* 读掉服务端剩余的回显，避免 RST 干扰对端统计 */
    }
    close(fd);
    free(frame);
    free(back);
    c->ms = now_ms() - t0;
    return NULL;
}

int main(int argc, char **argv)
{
    struct cli cs[MAX_CLIENTS];
    pthread_t th[MAX_CLIENTS];
    int ncli, msgs, size, timeout_ms = 2000, gap_ms = 0;
    long verified = 0, mismatch = 0, expect;
    int timeouts = 0;
    long long t0, t1, max_ms = 0;

    if (argc < 5) {
        printf("用法: loadgen <port> <clients> <msgs> <size> [timeout_ms] [gap_ms]\n");
        return 2;
    }
    ncli = atoi(argv[2]);
    msgs = atoi(argv[3]);
    size = atoi(argv[4]);
    if (argc > 5)
        timeout_ms = atoi(argv[5]);
    if (argc > 6)
        gap_ms = atoi(argv[6]);
    if (ncli < 1 || ncli > MAX_CLIENTS || msgs < 1 || size < 1 || size > 4090) {
        printf("参数越界：clients 1..%d, msgs>=1, size 1..4090\n", MAX_CLIENTS);
        return 2;
    }

    for (int i = 0; i < ncli; i++) {
        memset(&cs[i], 0, sizeof cs[i]);
        cs[i].idx = i;
        cs[i].port = atoi(argv[1]);
        cs[i].msgs = msgs;
        cs[i].size = size;
        cs[i].timeout_ms = timeout_ms;
        cs[i].gap_ms = gap_ms;
    }

    t0 = now_ms();
    for (int i = 0; i < ncli; i++)
        pthread_create(&th[i], NULL, cli_main, &cs[i]);
    for (int i = 0; i < ncli; i++)
        pthread_join(th[i], NULL);
    t1 = now_ms();

    for (int i = 0; i < ncli; i++) {
        verified += cs[i].verified;
        mismatch += cs[i].mismatch;
        timeouts += cs[i].timeouts;
        if (cs[i].ms > max_ms)
            max_ms = cs[i].ms;
    }
    expect = (long)ncli * msgs;
    printf("SYS04|gen|done|clients=%d|msgs=%d|size=%d|gap_ms=%d|verified=%ld|expect=%ld|mismatch=%ld|"
           "total_ms=%lld|max_conn_ms=%lld|timeouts=%d\n",
           ncli, msgs, size, gap_ms, verified, expect, mismatch, t1 - t0, max_ms, timeouts);
    return (verified == expect && mismatch == 0) ? 0 : 1;
}
