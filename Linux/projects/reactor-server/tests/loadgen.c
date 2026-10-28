/*
 * loadgen.c —— 压测与协议校验客户端
 *
 * 协议：2 字节大端长度 + payload；服务端原样回显同一帧。
 * 客户端把每个字节做成确定性图案（conn*31 + frame + offset），回显比对就能抓出
 * "顺序错、截断、串包"三类错误，而不只是统计字节数。
 *
 * 用法：
 *   ./loadgen --port N [--clients 8] [--frames 20] [--size 200]
 *             [--hold-ms 0] [--half-close] [--idle-after-ms 0] [--pipeline]
 *             [--read-delay-ms 0]
 *
 * 退出码：0 = 全部连接无错；1 = 有连接失败或内容不一致。
 */
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

typedef struct {
    int port;
    int clients;
    int frames;
    int size;
    int hold_ms;
    int idle_after_ms;
    int half_close;
    int pipeline;     /* 先发完再读：用于制造服务端写缓冲积压 */
    int read_delay_ms;/* pipeline 下读完之前的停顿：让服务端写缓冲必然堆满 */
} opts_t;

typedef struct {
    opts_t *o;
    int     idx;
    long    ok;
    size_t  sent;
    size_t  recv;
    int     mismatch;
    int     err;      /* 0=ok 1=connect 2=io 3=mismatch 4=idle_closed 5=timeout */
} arg_t;

static uint64_t now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000u + (uint64_t)(ts.tv_nsec / 1000000);
}

static ssize_t write_all(int fd, const void *buf, size_t len)
{
    const uint8_t *p = buf;
    size_t off = 0;
    while (off < len) {
        ssize_t n = write(fd, p + off, len - off);
        if (n > 0) {
            off += (size_t)n;
            continue;
        }
        if (n < 0 && errno == EINTR) {
            continue;
        }
        return -1;
    }
    return (ssize_t)off;
}

static ssize_t read_all(int fd, void *buf, size_t len)
{
    uint8_t *p = buf;
    size_t off = 0;
    while (off < len) {
        ssize_t n = read(fd, p + off, len - off);
        if (n > 0) {
            off += (size_t)n;
            continue;
        }
        if (n == 0) {
            return off == 0 ? 0 : -1;
        }
        if (errno == EINTR) {
            continue;
        }
        return -1;
    }
    return (ssize_t)off;
}

static void make_payload(uint8_t *dst, int size, int conn, int frame)
{
    for (int i = 0; i < size; i++) {
        dst[i] = (uint8_t)((conn * 31 + frame + i) & 0xFF);
    }
}

/* 读一帧并逐字节比对。返回 0 = 一致；-1 = 失败（a->err 已置位）。 */
static int read_frame(int fd, uint8_t *rbuf, uint8_t *expect, arg_t *a, int frame)
{
    int size = a->o->size;
    ssize_t n = read_all(fd, rbuf, 2);
    if (n != 2) {
        a->err = 2;
        return -1;
    }
    uint16_t got = (uint16_t)((rbuf[0] << 8) | rbuf[1]);
    if (got != (uint16_t)size) {
        a->err = 3;
        a->mismatch++;
        return -1;
    }
    n = read_all(fd, rbuf, (size_t)size);
    if (n != (ssize_t)size) {
        a->err = 2;
        return -1;
    }
    a->recv += (size_t)size;
    make_payload(expect, size, a->idx, frame);
    if (memcmp(expect, rbuf, (size_t)size) != 0) {
        a->err = 3;
        a->mismatch++;
        return -1;
    }
    return 0;
}

static void *client_main(void *p)
{
    arg_t *a = p;
    opts_t *o = a->o;

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        a->err = 1;
        return NULL;
    }
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons((uint16_t)o->port);
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    struct timeval tv = { 10, 0 };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

    if (connect(fd, (struct sockaddr *)&sa, sizeof(sa)) != 0) {
        a->err = 1;
        close(fd);
        return NULL;
    }
    if (o->hold_ms > 0) {
        struct timespec ts = { o->hold_ms / 1000, (o->hold_ms % 1000) * 1000000L };
        nanosleep(&ts, NULL);
    }

    uint8_t *hdr = malloc((size_t)o->size + 2);
    uint8_t *rbuf = malloc((size_t)o->size + 2);
    if (hdr == NULL || rbuf == NULL) {
        free(hdr);
        free(rbuf);
        a->err = 2;
        close(fd);
        return NULL;
    }

    uint8_t expect[8192];

    /* 交替模式：发一帧读一帧，单连接在飞 job 恒为 1，不会给服务端制造写缓冲压力。
     * pipeline 模式：先发完全部帧再统一读回，服务端回显积压 → 触发 EPOLLOUT 与写缓冲路径，
     * 用来验证 conn_flush 的 EAGAIN 分支和 wbuf_full 背压。 */
    for (int f = 0; f < o->frames; f++) {
        uint16_t be = htons((uint16_t)o->size);
        memcpy(hdr, &be, 2);
        make_payload(hdr + 2, o->size, a->idx, f);
        if (write_all(fd, hdr, (size_t)o->size + 2) < 0) {
            a->err = 2;
            break;
        }
        a->sent += (size_t)o->size;
        if (!o->pipeline && read_frame(fd, rbuf, expect, a, f) != 0) {
            break;
        }
    }
    if (a->err == 0 && o->pipeline) {
        /* 故意先不读：服务端回显只能堆在内核发送队列 + 自己的写缓冲里，
         * 写缓冲一旦放不下就按背压策略断开——这是 wbuf_full 的确定性入口。 */
        if (o->read_delay_ms > 0) {
            struct timespec ts = { o->read_delay_ms / 1000,
                                   (o->read_delay_ms % 1000) * 1000000L };
            nanosleep(&ts, NULL);
        }
        for (int f = 0; f < o->frames; f++) {
            if (read_frame(fd, rbuf, expect, a, f) != 0) {
                break;
            }
        }
    }

    if (a->err == 0 && o->half_close) {
        shutdown(fd, SHUT_WR);
        /* 半关闭后仍应收到剩余回显：再读一轮，读到 EOF 为止 */
        for (;;) {
            ssize_t n = read(fd, rbuf, (size_t)o->size + 2);
            if (n > 0) {
                a->recv += (size_t)n;
                continue;
            }
            break;
        }
    }

    if (a->err == 0 && o->idle_after_ms > 0) {
        struct timespec ts = { o->idle_after_ms / 1000, (o->idle_after_ms % 1000) * 1000000L };
        nanosleep(&ts, NULL);
        uint8_t probe;
        ssize_t n = read(fd, &probe, 1);
        if (n == 0) {
            a->err = 4; /* 服务端主动关闭：超时用例的预期结果 */
        } else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            a->err = 5; /* 仍开着：超时未生效 */
        } else {
            a->err = 2;
        }
    }

    free(hdr);
    free(rbuf);
    close(fd);
    return NULL;
}

int main(int argc, char **argv)
{
    opts_t o = { 0, 8, 20, 200, 0, 0, 0, 0, 0 };
    for (int i = 1; i < argc; i++) {
        const char *k = argv[i];
        const char *v = (i + 1 < argc) ? argv[i + 1] : NULL;
        if (strcmp(k, "--port") == 0 && v) { o.port = atoi(v); i++; }
        else if (strcmp(k, "--clients") == 0 && v) { o.clients = atoi(v); i++; }
        else if (strcmp(k, "--frames") == 0 && v) { o.frames = atoi(v); i++; }
        else if (strcmp(k, "--size") == 0 && v) { o.size = atoi(v); i++; }
        else if (strcmp(k, "--hold-ms") == 0 && v) { o.hold_ms = atoi(v); i++; }
        else if (strcmp(k, "--idle-after-ms") == 0 && v) { o.idle_after_ms = atoi(v); i++; }
        else if (strcmp(k, "--half-close") == 0) { o.half_close = 1; }
        else if (strcmp(k, "--pipeline") == 0) { o.pipeline = 1; }
        else if (strcmp(k, "--read-delay-ms") == 0 && v) { o.read_delay_ms = atoi(v); i++; }
        else {
            fprintf(stderr, "未知参数: %s\n", k);
            return 2;
        }
    }
    if (o.port <= 0 || o.clients <= 0 || o.size <= 0 || o.size > 8192) {
        fprintf(stderr, "参数非法\n");
        return 2;
    }

    setvbuf(stdout, NULL, _IOLBF, 0);
    uint64_t t0 = now_ms();

    pthread_t *tids = calloc((size_t)o.clients, sizeof(*tids));
    arg_t *args = calloc((size_t)o.clients, sizeof(*args));
    if (tids == NULL || args == NULL) {
        return 2;
    }
    for (int i = 0; i < o.clients; i++) {
        args[i].o = &o;
        args[i].idx = i;
        if (pthread_create(&tids[i], NULL, client_main, &args[i]) != 0) {
            args[i].err = 1;
        }
    }
    for (int i = 0; i < o.clients; i++) {
        pthread_join(tids[i], NULL);
    }

    size_t sent = 0, recv = 0;
    int fail = 0, mismatch = 0;
    int idle_closed = 0, timeout_missed = 0;
    for (int i = 0; i < o.clients; i++) {
        sent += args[i].sent;
        recv += args[i].recv;
        mismatch += args[i].mismatch;
        if (args[i].err == 4) {
            idle_closed++;
        } else if (args[i].err == 5) {
            timeout_missed++;
        } else if (args[i].err != 0) {
            fail++;
        }
        fprintf(stdout, "CLI|conn|idx=%d|state=%d|sent=%zu|recv=%zu|mismatch=%d\n",
                i, args[i].err, args[i].sent, args[i].recv, args[i].mismatch);
    }
    fprintf(stdout,
            "CLI|done|clients=%d|frames=%d|size=%d|sent=%zu|recv=%zu|fail=%d|"
            "mismatch=%d|idle_closed=%d|timeout_missed=%d|ms=%llu\n",
            o.clients, o.frames, o.size, sent, recv, fail, mismatch,
            idle_closed, timeout_missed, (unsigned long long)(now_ms() - t0));

    free(tids);
    free(args);
    return (fail == 0 && mismatch == 0 && timeout_missed == 0) ? 0 : 1;
}
