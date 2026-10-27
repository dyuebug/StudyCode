/* SYS_05 · 客户端（每连接一线程）
 *
 * 用法: client <port> <mode> <conns> <frames> <size> [extra]
 *   rr              请求-响应：发一帧、收一帧
 *   pipeline        流水线：先把 frames 帧全发出去，再统一收
 *   silent          不发数据、只等（hold_ms = extra）：用于触发服务端空闲超时
 *   idle-then-send  先静默 wait_ms 再发一帧（extra）：用于触发"取消后又来事件"
 *   rst             收发完毕后用 SO_LINGER{1,0} 关闭 → 对端收到 RST
 *   halfclose       收发完毕后 shutdown(SHUT_WR) → 对端读到 EOF，自己仍可读
 *
 * 输出: S05|gen|done|mode=..|clients=..|frames=..|size=..|verified=..|expect=..
 *       |mismatch=..|timeouts=..|eof_seen=..|send_fail=..|total_ms=..|max_conn_ms=..
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
    const char *mode;
    int frames;
    int size;
    int extra;
    long verified;
    long mismatch;
    int timeouts;
    int eof_seen;
    int send_fail;
    long long ms;
};

static long long now_ms(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

static void sleep_ms(int ms)
{
    struct timespec ts;

    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
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
        return -1;
    }
    return 0;
}

static void *cli_main(void *arg)
{
    struct cli *c = arg;
    struct sockaddr_in a;
    struct timeval tv = { .tv_sec = 2, .tv_usec = 0 };
    unsigned char *frame = NULL, *back = NULL;
    uint32_t be;
    long long t0 = now_ms();
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int one = 1;

    if (fd < 0)
        return NULL;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv);
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);

    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    a.sin_port = htons((unsigned short)c->port);
    if (connect(fd, (struct sockaddr *)&a, sizeof a) < 0) {
        c->send_fail++;
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
        c->ms = now_ms() - t0;
        return NULL;
    }
    be = htonl((uint32_t)c->size);
    memcpy(frame, &be, 4);
    memset(frame + 4, (unsigned char)('a' + (c->idx % 26)), (size_t)c->size);

    if (strcmp(c->mode, "silent") == 0) {
        /* 只等：服务端空闲超时后应 close，这里读到 0 或超时都算预期行为 */
        char sink[64];
        ssize_t r;

        sleep_ms(c->extra);
        r = recv(fd, sink, sizeof sink, 0);
        if (r == 0)
            c->eof_seen++;
        else if (r < 0)
            c->timeouts++;
    } else if (strcmp(c->mode, "idle-then-send") == 0) {
        char sink[64];

        sleep_ms(c->extra);
        if (write_all(fd, frame, (size_t)c->size + 4) < 0)
            c->send_fail++;
        if (recv(fd, sink, sizeof sink, 0) <= 0)
            c->timeouts++;
        else
            c->verified++;
    } else if (strcmp(c->mode, "pipeline") == 0) {
        int ok = 1;

        for (int i = 0; i < c->frames; i++)
            if (write_all(fd, frame, (size_t)c->size + 4) < 0) {
                c->send_fail++;
                ok = 0;
                break;
            }
        if (ok) {
            for (int i = 0; i < c->frames; i++) {
                if (read_all(fd, back, (size_t)c->size + 4) < 0) {
                    c->timeouts++;
                    break;
                }
                if (memcmp(back, frame, (size_t)c->size + 4) == 0)
                    c->verified++;
                else
                    c->mismatch++;
            }
        }
    } else if (strcmp(c->mode, "rst") == 0) {
        struct linger lg = { 1, 0 };

        for (int i = 0; i < c->frames; i++) {
            if (write_all(fd, frame, (size_t)c->size + 4) < 0) {
                c->send_fail++;
                break;
            }
            if (read_all(fd, back, (size_t)c->size + 4) < 0) {
                c->timeouts++;
                break;
            }
            if (memcmp(back, frame, (size_t)c->size + 4) == 0)
                c->verified++;
            else
                c->mismatch++;
        }
        /* 强制 RST：不发 FIN */
        setsockopt(fd, SOL_SOCKET, SO_LINGER, &lg, sizeof lg);
    } else if (strcmp(c->mode, "halfclose") == 0) {
        for (int i = 0; i < c->frames; i++) {
            if (write_all(fd, frame, (size_t)c->size + 4) < 0) {
                c->send_fail++;
                break;
            }
            if (read_all(fd, back, (size_t)c->size + 4) < 0) {
                c->timeouts++;
                break;
            }
            if (memcmp(back, frame, (size_t)c->size + 4) == 0)
                c->verified++;
            else
                c->mismatch++;
        }
        shutdown(fd, SHUT_WR);
        {
            char sink[64];

            if (recv(fd, sink, sizeof sink, 0) == 0)
                c->eof_seen++;
        }
    } else { /* rr */
        for (int i = 0; i < c->frames; i++) {
            if (write_all(fd, frame, (size_t)c->size + 4) < 0) {
                c->send_fail++;
                break;
            }
            if (read_all(fd, back, (size_t)c->size + 4) < 0) {
                c->timeouts++;
                break;
            }
            if (memcmp(back, frame, (size_t)c->size + 4) == 0)
                c->verified++;
            else
                c->mismatch++;
        }
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
    int ncli, frames, size, extra = 0;
    const char *mode;
    long verified = 0, mismatch = 0, expect;
    int timeouts = 0, eof_seen = 0, send_fail = 0, clients;
    long long t0, t1, max_ms = 0;

    if (argc < 6) {
        printf("用法: client <port> <mode> <conns> <frames> <size> [extra]\n");
        return 2;
    }
    mode = argv[2];
    clients = atoi(argv[3]);
    frames = atoi(argv[4]);
    size = atoi(argv[5]);
    if (argc > 6)
        extra = atoi(argv[6]);
    if (clients < 1 || clients > MAX_CLIENTS || frames < 1 || size < 1 || size > 4090) {
        printf("参数越界：conns 1..%d, frames>=1, size 1..4090\n", MAX_CLIENTS);
        return 2;
    }
    ncli = clients;

    for (int i = 0; i < ncli; i++) {
        memset(&cs[i], 0, sizeof cs[i]);
        cs[i].idx = i;
        cs[i].port = atoi(argv[1]);
        cs[i].mode = mode;
        cs[i].frames = frames;
        cs[i].size = size;
        cs[i].extra = extra;
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
        eof_seen += cs[i].eof_seen;
        send_fail += cs[i].send_fail;
        if (cs[i].ms > max_ms)
            max_ms = cs[i].ms;
    }
    if (strcmp(mode, "pipeline") == 0 || strcmp(mode, "rr") == 0 ||
        strcmp(mode, "rst") == 0 || strcmp(mode, "halfclose") == 0)
        expect = (long)ncli * frames;
    else if (strcmp(mode, "idle-then-send") == 0)
        expect = ncli;
    else
        expect = 0; /* silent：不该有回显 */

    printf("S05|gen|done|mode=%s|clients=%d|frames=%d|size=%d|verified=%ld|expect=%ld|"
           "mismatch=%ld|timeouts=%d|eof_seen=%d|send_fail=%d|total_ms=%lld|max_conn_ms=%lld\n",
           mode, ncli, frames, size, verified, expect, mismatch, timeouts, eof_seen, send_fail,
           t1 - t0, max_ms);
    return (mismatch == 0 && send_fail == 0) ? 0 : 1;
}
