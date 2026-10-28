/*
 * server.c —— epoll ET + EPOLLONESHOT + 线程池的最小可运行服务器
 *
 * 线程模型（为什么这样切）：
 *   主线程独占全部 socket I/O；worker 只做业务计算，结果经"完成队列 + eventfd"回投。
 *   结果是：连接结构体只被主线程访问，没有锁、没有数据竞争；代价是 payload 要拷两次
 *   （进 job 一次、回结果一次），换来的是可推理的生命周期。
 *
 * ET 的三条硬约束（本文件处处体现）：
 *   1. accept 与 read 必须循环到 EAGAIN，否则边沿会丢；
 *   2. 每次 EPOLLONESHOT 事件处理后必须显式重挂（epoll_ctl MOD），否则连接永久沉默；
 *   3. 在飞 job 期间不重挂 EPOLLIN——否则主线程会和 worker 同时处理同一条连接。
 *
 * 背压策略：写缓冲放不下新回显时直接断开（reason=wbuf_full），不无限排队。
 *   理由是"宁可让对端知道被拒，也不要让单连接吃掉全部内存"。
 */
#include "reactor.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define RBUF_CAP   65536
#define WBUF_CAP   65536
#define EV_BATCH   128
#define WAIT_MS    100

typedef struct {
    int      used;
    int      fd;
    int      in_flight;    /* 有 job 在跑：不重挂 EPOLLIN，也不再解析新帧 */
    int      peer_eof;     /* 对端已半关闭：写完剩余数据就关 */
    uint64_t id;           /* (slot << 32) | generation */
    uint64_t last_active;  /* ms，单调时钟 */
    size_t   rlen;
    size_t   roff;
    size_t   wlen;
    size_t   woff;
    uint64_t rx_bytes;
    uint64_t tx_bytes;
    uint8_t  rbuf[RBUF_CAP];
    uint8_t  wbuf[WBUF_CAP];
} conn_t;

static conn_t   g_conns[MAX_CONNS];
static int      g_epfd = -1;
static int      g_listen_fd = -1;
static int      g_res_efd = -1;
static uint32_t g_gen;
static int      g_active;

static uint64_t st_accepted, st_closed, st_rx, st_tx, st_jobs, st_dropped;
/* st_stale 是 st_dropped 的子集：迟到结果的槽位**已被新连接复用**。
 * 两者的区别是这个项目唯一无法从外部观测到的分支——
 * "连接已关闭"与"槽位已换人"在 dropped 计数上完全一样，
 * 但后者一旦判断出错，旧数据会被投给新连接。分开计数才能分别断言。 */
static uint64_t st_stale;
static uint64_t st_timeouts, st_errors;

/* 退出中标志：置位后 conn_service 不再提交新 job，只把已完成的结果刷出去。
 * 没有它，"等 job 排空"会被持续到达的输入无限续命——实测 4 条管道化连接
 * 每 200 ms 完成一批、立刻又提交下一批，排空条件永远不满足，
 * 2000 ms 门限一到就带着 leftover_jobs=4 退出。 */
static int g_draining;

/* ---------- 完成队列（worker → 主线程） ---------- */

typedef struct res_node {
    payload_t       *res;
    struct res_node *next;
} res_node;

static pthread_mutex_t g_res_lock = PTHREAD_MUTEX_INITIALIZER;
static res_node       *g_res_head;
static res_node       *g_res_tail;
static int             g_res_queued;

static uint64_t now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000u + (uint64_t)(ts.tv_nsec / 1000000);
}

static void post_result(const payload_t *src)
{
    payload_t *copy = malloc(sizeof(*copy));
    if (copy == NULL) {
        return; /* 丢掉结果 -> 该连接会因超时/无响应被断开，由日志暴露 */
    }
    *copy = *src;
    res_node *node = malloc(sizeof(*node));
    if (node == NULL) {
        free(copy);
        return;
    }
    node->res = copy;
    node->next = NULL;

    pthread_mutex_lock(&g_res_lock);
    if (g_res_tail != NULL) {
        g_res_tail->next = node;
    } else {
        g_res_head = node;
    }
    g_res_tail = node;
    g_res_queued++;
    pthread_mutex_unlock(&g_res_lock);

    uint64_t one = 1;
    ssize_t n = write(g_res_efd, &one, sizeof(one));
    (void)n; /* eventfd 计数已饱和时 write 也不会失败；返回值不影响正确性 */
}

/* g_res_queued 的跨线程读取。
 * 这个计数由 worker 在 post_result() 里加、由主线程在 drain_results() 里清零，
 * 两处都在锁内；但主线程还有两处"要不要排空"的判断以前直接读全局变量，
 * 等于无锁读一个被其他线程写的 int——TSan 会判数据竞争，真实后果是
 * "看到 0 就跳过排空"（结果留到下一轮，或退出时被判为遗留）。
 * 所有读取统一走这个访问器。 */
static int res_pending(void)
{
    pthread_mutex_lock(&g_res_lock);
    int n = g_res_queued;
    pthread_mutex_unlock(&g_res_lock);
    return n;
}

/* 业务：把 payload 原样回显。延迟由环境变量模拟，用于观察背压而不改协议。 */
static void job_echo(payload_t *job)
{
    const char *d = getenv("SRV_JOB_US");
    if (d != NULL) {
        long us = strtol(d, NULL, 10);
        if (us > 0) {
            struct timespec ts = { us / 1000000, (us % 1000000) * 1000 };
            nanosleep(&ts, NULL);
        }
    }
    post_result(job);
}

/* ---------- 连接生命周期 ---------- */

static conn_t *conn_by_id(uint64_t id)
{
    uint32_t slot = (uint32_t)(id >> 32);
    if (slot >= MAX_CONNS) {
        return NULL;
    }
    conn_t *c = &g_conns[slot];
    if (!c->used || c->id != id) {
        return NULL; /* 世代不匹配：这是上一条连接的迟到结果 */
    }
    return c;
}

/* 槽位是否已经被**另一代**连接占用。
 * conn_by_id() 返回 NULL 有两种成因：槽位空着（连接已关闭），或者槽位被复用（世代号不匹配）。
 * 只有后者才证明世代号真的在起作用——生成用例时必须能把两者区分开。 */
static int conn_slot_reused(uint64_t id)
{
    uint32_t slot = (uint32_t)(id >> 32);
    if (slot >= MAX_CONNS) {
        return 0;
    }
    const conn_t *c = &g_conns[slot];
    return c->used && c->id != id;
}

static void conn_close(conn_t *c, const char *reason)
{
    if (!c->used) {
        return;
    }
    fprintf(stdout, "SRV|close|id=%llu|reason=%s|rx=%llu|tx=%llu\n",
            (unsigned long long)c->id, reason,
            (unsigned long long)c->rx_bytes, (unsigned long long)c->tx_bytes);
    epoll_ctl(g_epfd, EPOLL_CTL_DEL, c->fd, NULL);
    close(c->fd);
    st_closed++;
    g_active--;
    c->used = 0;
    c->fd = -1;
    c->in_flight = 0;
    c->rlen = c->roff = c->wlen = c->woff = 0;
}

static int conn_arm(conn_t *c, uint32_t events)
{
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = events;
    ev.data.u64 = c->id;
    return epoll_ctl(g_epfd, EPOLL_CTL_MOD, c->fd, &ev);
}

/* ET：注册时检查当前状态即可补上"未注册期间到达的数据" */
static void conn_rearm(conn_t *c)
{
    uint32_t ev = EPOLLONESHOT | EPOLLRDHUP;
    if (c->in_flight) {
        return; /* 在飞期间不挂，结果回来后由 pump 重挂 */
    }
    if (!c->peer_eof) {
        ev |= EPOLLIN;
    }
    if (c->wlen > c->woff) {
        ev |= EPOLLOUT;
    } else if (c->peer_eof) {
        conn_close(c, "peer_eof");
        return;
    }
    if (conn_arm(c, ev) != 0) {
        conn_close(c, "arm_failed");
    }
}

/* 尝试把写缓冲刷出去。
 * 返回 0 = 已排空，1 = EAGAIN（等 EPOLLOUT 再写），-1 = 硬错误（EPIPE/ECONNRESET 等）。
 * 三种情况必须分开：把 EAGAIN 当错误会让每条大响应都断连。 */
static int conn_flush(conn_t *c)
{
    while (c->woff < c->wlen) {
        ssize_t n = write(c->fd, c->wbuf + c->woff, c->wlen - c->woff);
        if (n > 0) {
            c->woff += (size_t)n;
            c->tx_bytes += (uint64_t)n;
            st_tx += (uint64_t)n;
            c->last_active = now_ms();
            continue;
        }
        if (n < 0 && errno == EINTR) {
            continue;
        }
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return 1;
        }
        return -1;
    }
    c->woff = c->wlen = 0;
    return 0;
}

/* 把一条完整帧取出来投递给线程池；返回 1 = 已投递，0 = 无完整帧，-1 = 协议错误 */
static int conn_pump(conn_t *c)
{
    if (c->in_flight) {
        return 0;
    }
    for (;;) {
        size_t avail = c->rlen - c->roff;
        if (avail < 2) {
            break;
        }
        const uint8_t *p = c->rbuf + c->roff;
        uint32_t flen = ((uint32_t)p[0] << 8) | (uint32_t)p[1];
        if (flen == 0 || flen > MAX_FRAME) {
            return -1;
        }
        if (avail < 2 + (size_t)flen) {
            break;
        }
        payload_t *job = malloc(sizeof(*job));
        if (job == NULL) {
            return -1;
        }
        job->conn_id = c->id;
        job->len = flen;
        memcpy(job->data, p + 2, flen);
        c->roff += 2 + (size_t)flen;

        if (tp_submit(job_echo, job) != 0) {
            free(job);
            return -1; /* 队列满：按背压处理 */
        }
        st_jobs++;
        c->in_flight = 1;
        break; /* 一连接同时只允许一个在飞 job，保证回显顺序 */
    }
    /* 压缩读缓冲：roff 超过一半才搬，避免每帧 memmove */
    if (c->roff > 0 && c->roff * 2 >= c->rlen) {
        memmove(c->rbuf, c->rbuf + c->roff, c->rlen - c->roff);
        c->rlen -= c->roff;
        c->roff = 0;
    }
    return 0;
}

/* ET 读循环：必须读到 EAGAIN。
 * 读缓冲满不是错误——它只表示"先消费再读"，由 conn_service 负责续读；
 * 把满缓冲当错误会让"管道化灌入 > RBUF_CAP"的正常连接被误杀。 */
static int conn_read(conn_t *c, int *full)
{
    *full = 0;
    for (;;) {
        if (c->rlen >= RBUF_CAP) {
            *full = 1;
            return 0;
        }
        ssize_t n = read(c->fd, c->rbuf + c->rlen, RBUF_CAP - c->rlen);
        if (n > 0) {
            c->rlen += (size_t)n;
            c->rx_bytes += (uint64_t)n;
            st_rx += (uint64_t)n;
            c->last_active = now_ms();
            continue;
        }
        if (n == 0) {
            c->peer_eof = 1;
            return 0;
        }
        if (errno == EINTR) {
            continue;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }
        return -1;
    }
}

/* 连接的单次驱动：读 → 解析 → 刷写，直到无事可做或需要等待。
 * ET 下"缓冲满"必须续读：边沿已经消费掉，不主动再读就再也收不到通知。
 * 返回时若连接仍存活，本函数负责重挂 EPOLLONESHOT。 */
static void conn_service(conn_t *c)
{
    for (;;) {
        int full = 0;
        if (!c->peer_eof && conn_read(c, &full) != 0) {
            conn_close(c, "read_err");
            return;
        }
        if (g_draining) {
            /* 退出中：不再解析/提交新 job，只把在飞结果的回显刷出去 */
            if (conn_flush(c) < 0) {
                conn_close(c, "write_err");
                return;
            }
            break;
        }
        size_t before = c->rlen - c->roff;
        int rc = conn_pump(c);
        if (rc < 0) {
            conn_close(c, "protocol");
            return;
        }
        if (rc > 0) {
            return; /* 有 job 在飞：结果回来后由 drain_results 再驱动 */
        }
        size_t after = c->rlen - c->roff;
        int fr = conn_flush(c);
        if (fr < 0) {
            conn_close(c, "write_err");
            return;
        }
        if (fr == 0 && c->peer_eof && after == 0) {
            conn_close(c, "peer_eof");
            return;
        }
        if (!full || after == before) {
            break; /* 没读满，或本轮无进展（帧不完整）：等下一次事件 */
        }
    }
    conn_rearm(c);
}

static void conn_handle(conn_t *c, uint32_t events)
{
    if (events & (EPOLLHUP | EPOLLERR)) {
        conn_close(c, "epoll_err");
        return;
    }
    /* EPOLLIN / EPOLLOUT / EPOLLRDHUP 都走同一条驱动路径：
     * 先读（幂等，读到 EAGAIN 为止），再解析，最后刷写。 */
    conn_service(c);
}

static void accept_new(void)
{
    for (;;) {
        struct sockaddr_in sa;
        socklen_t sl = sizeof(sa);
        int fd = accept4(g_listen_fd, (struct sockaddr *)&sa, &sl,
                         SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (fd < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return;
            }
            st_errors++;
            fprintf(stdout, "SRV|error|what=accept|errno=%d\n", errno);
            return;
        }

        int slot = -1;
        for (int i = 0; i < MAX_CONNS; i++) {
            if (!g_conns[i].used) {
                slot = i;
                break;
            }
        }
        if (slot < 0) {
            close(fd);
            st_errors++;
            fprintf(stdout, "SRV|error|what=accept|detail=max_conns\n");
            continue;
        }

        conn_t *c = &g_conns[slot];
        memset(c, 0, sizeof(*c));
        c->used = 1;
        c->fd = fd;
        c->id = ((uint64_t)(uint32_t)slot << 32) | (uint64_t)(++g_gen);
        c->last_active = now_ms();
        g_active++;
        st_accepted++;

        int one = 1;
        setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

        struct epoll_event ev;
        memset(&ev, 0, sizeof(ev));
        ev.events = EPOLLIN | EPOLLONESHOT | EPOLLRDHUP;
        ev.data.u64 = c->id;
        if (epoll_ctl(g_epfd, EPOLL_CTL_ADD, fd, &ev) != 0) {
            conn_close(c, "epoll_add_failed");
            st_errors++;
            continue;
        }
        fprintf(stdout, "SRV|accept|id=%llu|active=%d\n",
                (unsigned long long)c->id, g_active);
    }
}

static void drain_results(void)
{
    pthread_mutex_lock(&g_res_lock);
    res_node *node = g_res_head;
    g_res_head = g_res_tail = NULL;
    g_res_queued = 0;
    pthread_mutex_unlock(&g_res_lock);

    while (node != NULL) {
        res_node *next = node->next;
        payload_t *r = node->res;
        conn_t *c = conn_by_id(r->conn_id);
        if (c == NULL) {
            st_dropped++; /* 迟到的结果：连接已关闭或槽位已换人 */
            if (conn_slot_reused(r->conn_id)) {
                st_stale++;
                fprintf(stdout, "SRV|stale|id=%llu\n",
                        (unsigned long long)r->conn_id);
            }
        } else {
            c->in_flight = 0;
            size_t need = 2 + (size_t)r->len;
            if (c->wlen + need > WBUF_CAP) {
                conn_close(c, "wbuf_full");
            } else {
                c->wbuf[c->wlen++] = (uint8_t)((r->len >> 8) & 0xFF);
                c->wbuf[c->wlen++] = (uint8_t)(r->len & 0xFF);
                memcpy(c->wbuf + c->wlen, r->data, r->len);
                c->wlen += r->len;
                c->last_active = now_ms();
                conn_service(c); /* 刷写 + 续解析 + 重挂；失败时内部已关闭 */
            }
        }
        free(r);
        free(node);
        node = next;
    }
}

static void sweep_idle(int idle_ms, uint64_t now)
{
    if (idle_ms <= 0) {
        return;
    }
    for (int i = 0; i < MAX_CONNS; i++) {
        conn_t *c = &g_conns[i];
        if (!c->used) {
            continue;
        }
        if (now - c->last_active >= (uint64_t)idle_ms) {
            st_timeouts++;
            conn_close(c, "idle_timeout");
        }
    }
}

int srv_run(const struct srv_opts *opts)
{
    setvbuf(stdout, NULL, _IOLBF, 0);

    /* 信号必须在创建 worker 之前屏蔽：worker 继承创建时刻的掩码，
     * 若先起线程再屏蔽，SIGINT 会被内核投递给某个 worker 并按默认动作终止进程，
     * signalfd 的优雅退出路径就永远走不到。 */
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);
    signal(SIGPIPE, SIG_IGN); /* 对端已关闭时让 write 返回 EPIPE，走错误分支而不是杀进程 */

    if (tp_start(opts->threads) != 0) {
        return 1;
    }

    g_epfd = epoll_create1(EPOLL_CLOEXEC);
    g_res_efd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (g_epfd < 0 || g_res_efd < 0) {
        perror("epoll_create1/eventfd");
        return 1;
    }

    int sigfd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
    if (sigfd < 0) {
        perror("signalfd");
        return 1;
    }

    g_listen_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (g_listen_fd < 0) {
        perror("socket");
        return 1;
    }
    int one = 1;
    setsockopt(g_listen_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(opts->port);
    if (bind(g_listen_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        perror("bind");
        return 1;
    }
    if (listen(g_listen_fd, 512) != 0) {
        perror("listen");
        return 1;
    }

    socklen_t alen = sizeof(addr);
    getsockname(g_listen_fd, (struct sockaddr *)&addr, &alen);
    fprintf(stdout, "SRV|listen|port=%u|threads=%d|idle_ms=%d\n",
            (unsigned)ntohs(addr.sin_port), tp_threads(), opts->idle_ms);
    fprintf(stdout, "SRV|ready|pid=%ld\n", (long)getpid());

    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.u64 = UINT64_MAX; /* 监听 fd 专用标记 */
    epoll_ctl(g_epfd, EPOLL_CTL_ADD, g_listen_fd, &ev);

    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.u64 = UINT64_MAX - 1;
    epoll_ctl(g_epfd, EPOLL_CTL_ADD, g_res_efd, &ev);

    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.u64 = UINT64_MAX - 2;
    epoll_ctl(g_epfd, EPOLL_CTL_ADD, sigfd, &ev);

    struct epoll_event events[EV_BATCH];
    int running = 1;
    int exit_code = 0;

    while (running) {
        int n = epoll_wait(g_epfd, events, EV_BATCH, WAIT_MS);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("epoll_wait");
            exit_code = 1;
            break;
        }
        for (int i = 0; i < n; i++) {
            uint64_t key = events[i].data.u64;
            if (key == UINT64_MAX) {
                accept_new();
            } else if (key == UINT64_MAX - 1) {
                uint64_t v;
                ssize_t r = read(g_res_efd, &v, sizeof(v));
                (void)r;
                drain_results();
            } else if (key == UINT64_MAX - 2) {
                struct signalfd_siginfo si;
                ssize_t r = read(sigfd, &si, sizeof(si));
                (void)r;
                fprintf(stdout, "SRV|signal|signo=%u\n", si.ssi_signo);
                running = 0;
            } else {
                conn_t *c = conn_by_id(key);
                if (c != NULL) {
                    conn_handle(c, events[i].events);
                }
            }
        }
        /* 结果可能在处理事件时才产生，这里再排一次，避免等下一个 eventfd */
        if (res_pending() > 0) {
            drain_results();
        }
        sweep_idle(opts->idle_ms, now_ms());
    }

    /* 优雅退出：停止接受 -> 等在飞 job 排空 -> 关连接 -> 停线程池
     *
     * 排空条件必须同时看两处：`tp_pending()`（队列里 + 正在跑）和 `g_res_queued`
     * （worker 已完成、结果还没被主线程取走）。只看前者会漏掉最后一批结果——
     * 实测 200 ms job 在信号后完成时，`tp_pending()` 先归零、结果仍躺完成队列里，
     * 循环直接退出，连接被关闭后那批回显再没人取（`tx=0`、`dropped=0`，
     * 计数上完全看不出来）。 */
    epoll_ctl(g_epfd, EPOLL_CTL_DEL, g_listen_fd, NULL);
    close(g_listen_fd);
    g_listen_fd = -1;
    g_draining = 1; /* 先停"产生新 job"，再等"已有 job 收尾" */

    /* 退出路径分五个阶段各打一条带耗时的时间戳。
     * 起因：旧版曾在 `kill -INT` 后 600 s 不退出，而本机没有 gdb，无法在挂起时取栈。
     * 有了这五行，"卡在哪一步"至少能由日志判定（是排空没等到、join 没返回，
     * 还是 flush 卡住），不必再靠复现。这只提高可诊断性，不修复任何已知根因。 */
    uint64_t t_shutdown = now_ms();
#define SHUTDOWN_PHASE(name) \
    fprintf(stdout, "SRV|shutdown|phase=%s|at=%llu\n", (name), \
            (unsigned long long)(now_ms() - t_shutdown))
    SHUTDOWN_PHASE("begin");

    uint64_t deadline = now_ms() + 2000;
    while ((tp_pending() > 0 || res_pending() > 0) && now_ms() < deadline) {
        drain_results();
        struct timespec ts = { 0, 5 * 1000 * 1000 };
        nanosleep(&ts, NULL);
    }
    drain_results(); /* 循环最后一次 sleep 期间入队的那批 */
    int leftover = tp_pending();
    SHUTDOWN_PHASE("drained");

    for (int i = 0; i < MAX_CONNS; i++) {
        conn_t *c = &g_conns[i];
        if (!c->used) {
            continue;
        }
        conn_flush(c);
        conn_close(c, "shutdown");
    }
    SHUTDOWN_PHASE("conns_closed");

    tp_shutdown(1);
    SHUTDOWN_PHASE("tp_joined");
    /* 超过 2 s 门限仍在跑的 job，其结果是"连接已关闭之后到达"的，
     * 走 conn_by_id()==NULL 的路径计入 dropped，而不是静默丢弃。 */
    drain_results();
    SHUTDOWN_PHASE("final_drain");
#undef SHUTDOWN_PHASE

    fprintf(stdout,
            "SRV|summary|accepted=%llu|closed=%llu|active=%d|rx=%llu|tx=%llu|"
            "jobs=%llu|timeouts=%llu|errors=%llu|dropped=%llu|stale_gen=%llu|"
            "leftover_jobs=%d\n",
            (unsigned long long)st_accepted, (unsigned long long)st_closed, g_active,
            (unsigned long long)st_rx, (unsigned long long)st_tx,
            (unsigned long long)st_jobs, (unsigned long long)st_timeouts,
            (unsigned long long)st_errors, (unsigned long long)st_dropped,
            (unsigned long long)st_stale, leftover);
    fprintf(stdout, "SRV|exit|code=%d\n", exit_code);

    close(sigfd);
    close(g_res_efd);
    close(g_epfd);
    return exit_code;
}
