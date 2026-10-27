/* SYS_04 · 三种 Reactor 形态
 *
 *   single  单 Reactor 单线程：accept、读、业务、写全在一个线程里
 *   pool    Reactor + 线程池：业务处理交给 worker，结果经 eventfd 交回
 *   master  主从 Reactor：main 只 accept，把 fd 交给 sub 线程（各带一个 epoll）
 *
 * 三种形态共用同一个 frame 解析函数与同一个 reactor 内核，
 * 差别只有一条：**业务处理在哪个线程里执行**。
 *
 * 破坏版（用来证伪，不是功能）：
 *   --et-broken        ET 但每次只读一次，剩余字节不再产生事件
 *   --oneshot-broken   注册 EPOLLONESHOT 但处理完不重挂，连接停在第一帧
 */
#define _GNU_SOURCE
#include "reactor.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define MAX_CONNS 64
#define FRAME_MAX 4096
#define Q_SIZE 1024
#define SUB_MAX 8

enum mode { M_SINGLE, M_POOL, M_MASTER };

static struct cfg {
    enum mode mode;
    const char *mode_name;
    const char *flags_name;
    int delay_us;
    int workers;
    int subs;
    int idle_exit_ms;
    int max_run_ms;
    int et;
} g = { M_SINGLE, "single", "lt", 0, 4, 2, 400, 20000, 0 };

/* ---------- 公共工具 ---------- */
static long long now_ms(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

static void pr(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    fflush(stdout);
}

static int set_nonblock(int fd)
{
    int fl = fcntl(fd, F_GETFL, 0);

    if (fl < 0)
        return -1;
    return fcntl(fd, F_SETFL, fl | O_NONBLOCK);
}

static void work_delay(void)
{
    struct timespec ts;

    if (g.delay_us <= 0)
        return;
    ts.tv_sec = g.delay_us / 1000000;
    ts.tv_nsec = (long)(g.delay_us % 1000000) * 1000L;
    nanosleep(&ts, NULL);
}

/* 唤醒 reactor 线程：eventfd 只负责"有东西了"，具体内容走队列。
 * 写失败（EAGAIN）不需要处理——计数器已非零，下一次读仍会把它取走。 */
static void wake(int fd)
{
    uint64_t one = 1;
    ssize_t r = write(fd, &one, sizeof one);

    if (r < 0) {
        /* 忽略：eventfd 满意味着计数未清零，读端照样会被唤醒 */
    }
}

static void drain_wake(int fd)
{
    uint64_t v;
    ssize_t r = read(fd, &v, sizeof v);

    if (r < 0) {
        /* 忽略：EAGAIN 说明计数已被别处读掉，队列仍由调用方排空 */
    }
}

/* ---------- 连接池：id → conn*，跨线程回包靠 id 找回连接 ---------- */
static struct conn g_conns[MAX_CONNS];
static struct conn *g_by_id[MAX_CONNS];
static pthread_mutex_t g_id_mu = PTHREAD_MUTEX_INITIALIZER;

static struct conn *conn_alloc(int fd)
{
    struct conn *c = NULL;

    pthread_mutex_lock(&g_id_mu);
    for (int i = 0; i < MAX_CONNS; i++) {
        if (!g_by_id[i]) {
            c = &g_conns[i];
            memset(c, 0, sizeof *c);
            c->id = i;
            c->fd = fd;
            g_by_id[i] = c;
            break;
        }
    }
    pthread_mutex_unlock(&g_id_mu);
    return c;
}

static struct conn *conn_by_id(int id)
{
    struct conn *c;

    if (id < 0 || id >= MAX_CONNS)
        return NULL;
    pthread_mutex_lock(&g_id_mu);
    c = g_by_id[id];
    pthread_mutex_unlock(&g_id_mu);
    return c;
}

static void on_close(struct reactor *r, struct conn *c)
{
    (void)r;
    pthread_mutex_lock(&g_id_mu);
    if (c->id >= 0 && c->id < MAX_CONNS)
        g_by_id[c->id] = NULL;
    pthread_mutex_unlock(&g_id_mu);
}

/* ---------- 线程池队列 ---------- */
struct job {
    int cid;
    int len;
    char *data;
};

static struct job g_jobs[Q_SIZE];
static int g_jh, g_jn;
static struct job g_done[Q_SIZE];
static int g_dh, g_dn;
static long g_q_drops;
static pthread_mutex_t g_q_mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_q_cv = PTHREAD_COND_INITIALIZER;
static volatile int g_quit;
static int g_efd = -1;

static int enqueue_job(int cid, const char *buf, size_t n)
{
    struct job j;
    int ok = 0;

    j.data = malloc(n);
    if (!j.data)
        return -1;
    memcpy(j.data, buf, n);
    j.cid = cid;
    j.len = (int)n;

    pthread_mutex_lock(&g_q_mu);
    if (g_jn < Q_SIZE) {
        g_jobs[(g_jh + g_jn) % Q_SIZE] = j;
        g_jn++;
        ok = 1;
        pthread_cond_signal(&g_q_cv);
    } else {
        g_q_drops++; /* 队列满：显式计数，不悄悄阻塞 reactor 线程 */
    }
    pthread_mutex_unlock(&g_q_mu);
    if (!ok)
        free(j.data);
    return ok ? 0 : -1;
}

static void *worker_main(void *arg)
{
    (void)arg;

    for (;;) {
        struct job j;

        pthread_mutex_lock(&g_q_mu);
        while (g_jn == 0 && !g_quit)
            pthread_cond_wait(&g_q_cv, &g_q_mu);
        if (g_jn == 0) { /* 只有 quit 能让这里成立 */
            pthread_mutex_unlock(&g_q_mu);
            return NULL;
        }
        j = g_jobs[g_jh];
        g_jh = (g_jh + 1) % Q_SIZE;
        g_jn--;
        pthread_mutex_unlock(&g_q_mu);

        work_delay(); /* 业务耗时：故意放在 reactor 线程之外 */

        pthread_mutex_lock(&g_q_mu);
        if (g_dn < Q_SIZE) {
            g_done[(g_dh + g_dn) % Q_SIZE] = j;
            g_dn++;
        } else {
            g_q_drops++;
            free(j.data);
        }
        pthread_mutex_unlock(&g_q_mu);

        wake(g_efd); /* 唤醒 reactor 线程来取结果 */
    }
}

/* ---------- 业务：长度前缀分帧 + 原样回显 ---------- */
static int deploy(struct reactor *r, struct conn *c, const char *buf, size_t n)
{
    if (g.mode == M_POOL) {
        if (enqueue_job(c->id, buf, n) < 0)
            return -1;
        c->busy = 1; /* 每条连接同时只有一个在飞的 job：保证回包顺序 */
        return 0;
    }
    work_delay();
    if (conn_send(r, c, buf, n) < 0)
        return -1;
    reactor_note_frame(r);
    return 0;
}

static int handle_frames(struct reactor *r, struct conn *c)
{
    size_t off = 0;

    while (c->rlen - off >= 4) {
        uint32_t len;

        if (g.mode == M_POOL && c->busy)
            break; /* 在飞未回：剩下的帧留在 rbuf，等回包后再处理 */
        memcpy(&len, c->rbuf + off, 4);
        len = ntohl(len);
        if (len == 0 || len > FRAME_MAX)
            return -1; /* 非法长度：直接断开，不猜 */
        if (c->rlen - off < 4 + (size_t)len)
            break; /* 半帧：等下一次读 */
        if (deploy(r, c, c->rbuf + off, 4 + (size_t)len) < 0)
            return -1;
        off += 4 + (size_t)len;
    }
    if (off > 0) {
        memmove(c->rbuf, c->rbuf + off, c->rlen - off);
        c->rlen -= off;
    }
    return 0;
}

/* ---------- 线程池模式的回包通道 ---------- */
static void pool_on_notify(struct reactor *r)
{
    drain_wake(g_efd); /* EAGAIN 也算正常：只管把队列排空 */
    for (;;) {
        struct job j;
        struct conn *c;

        pthread_mutex_lock(&g_q_mu);
        if (g_dn == 0) {
            pthread_mutex_unlock(&g_q_mu);
            break;
        }
        j = g_done[g_dh];
        g_dh = (g_dh + 1) % Q_SIZE;
        g_dn--;
        pthread_mutex_unlock(&g_q_mu);

        c = conn_by_id(j.cid);
        if (c && c->fd >= 0) {
            if (conn_send(r, c, j.data, (size_t)j.len) == 0) {
                reactor_note_frame(r);
                c->busy = 0;
                if (reactor_kick(r, c) < 0)
                    conn_close(r, c);
            } else {
                c->busy = 0;
                conn_close(r, c);
            }
        }
        free(j.data);
    }
}

/* ---------- 主从模式：sub reactor ---------- */
struct sub {
    struct reactor *r;
    int efd;
    int pending[MAX_CONNS];
    int npending;
    pthread_mutex_t mu;
    pthread_t th;
    long served;
    volatile int stop;
};

static struct sub g_subs[SUB_MAX];
static int g_nsubs;
/* 主从模式下连接被"转交"，主 reactor 自己的 accepted 恒为 0：
 * 单独记一个 handoff 计数，否则日志会读成"一条都没接"。 */
static long g_handoff;

static void sub_handoff(struct sub *s, int fd)
{
    pthread_mutex_lock(&s->mu);
    if (s->npending < MAX_CONNS)
        s->pending[s->npending++] = fd;
    else
        close(fd);
    pthread_mutex_unlock(&s->mu);
    wake(s->efd);
}

static void sub_on_notify(struct reactor *r)
{
    uint64_t v;
    int fds[MAX_CONNS];
    int n = 0;

    /* 找到是哪个 sub：notify 回调不带 cookie，靠 fd 反查 */
    struct sub *s = NULL;

    for (int i = 0; i < g_nsubs; i++) {
        if (g_subs[i].r == r) {
            s = &g_subs[i];
            break;
        }
    }
    if (!s)
        return;
    if (read(s->efd, &v, sizeof v) < 0)
        return;
    pthread_mutex_lock(&s->mu);
    for (int i = 0; i < s->npending; i++)
        fds[n++] = s->pending[i];
    s->npending = 0;
    pthread_mutex_unlock(&s->mu);

    for (int i = 0; i < n; i++) {
        struct conn *c;

        (void)set_nonblock(fds[i]);
        c = conn_alloc(fds[i]);
        if (!c) {
            close(fds[i]);
            continue;
        }
        if (reactor_add_conn(r, c) < 0) {
            int bad = c->fd;

            c->fd = -1;
            on_close(r, c); /* 归还 id 槽位，否则 64 个连接后永久耗尽 */
            close(bad);
        }
    }
}

static void *sub_main(void *arg)
{
    struct sub *s = arg;

    while (!s->stop)
        (void)reactor_run_once(s->r, 50);
    return NULL;
}

/* ---------- accept ---------- */
static int on_accept_single(struct reactor *r, int fd)
{
    struct conn *c;

    (void)set_nonblock(fd);
    c = conn_alloc(fd);
    if (!c)
        return -1;
    if (reactor_add_conn(r, c) < 0) {
        c->fd = -1;
        return -1;
    }
    return 0;
}

static int on_accept_master(struct reactor *r, int fd)
{
    int pick = 0;

    (void)r;
    for (int i = 1; i < g_nsubs; i++)
        if (g_subs[i].served < g_subs[pick].served)
            pick = i;
    g_subs[pick].served++;
    g_handoff++;
    sub_handoff(&g_subs[pick], fd);
    return 0;
}

/* ---------- 装配 ---------- */
static int make_listen(int *port_out)
{
    struct sockaddr_in a;
    socklen_t alen = sizeof a;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int one = 1;

    if (fd < 0)
        return -1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    a.sin_port = 0; /* 端口交给内核：可并发运行，不撞 EADDRINUSE */
    if (bind(fd, (struct sockaddr *)&a, sizeof a) < 0)
        goto fail;
    if (listen(fd, 128) < 0)
        goto fail;
    /* 监听 fd 必须非阻塞：accept 循环靠 EAGAIN 退出，
     * 否则第二个 accept 会把整个事件循环永久挂住（首版就挂在这里）。 */
    if (set_nonblock(fd) < 0)
        goto fail;
    if (getsockname(fd, (struct sockaddr *)&a, &alen) < 0)
        goto fail;
    *port_out = ntohs(a.sin_port);
    return fd;
fail:
    close(fd);
    return -1;
}

static int reactor_flags(void)
{
    int f = 0;

    if (g.et)
        f |= R_ET;
    if (strcmp(g.flags_name, "oneshot") == 0)
        f |= R_ONESHOT;
    if (strcmp(g.flags_name, "oneshot-broken") == 0)
        f |= R_ONESHOT_BROKEN;
    if (strcmp(g.flags_name, "et-broken") == 0)
        f |= R_ET_BROKEN;
    return f;
}

static void usage(void)
{
    pr("用法: reactor_srv [--mode single|pool|master] [--delay-us N] [--workers N]\n"
       "                   [--subs N] [--et] [--oneshot|--oneshot-broken|--et-broken]\n"
       "                   [--idle-exit MS] [--max-run MS]\n");
}

static int parse_args(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];

        if (strcmp(a, "--mode") == 0 && i + 1 < argc) {
            const char *m = argv[++i];

            if (strcmp(m, "single") == 0) {
                g.mode = M_SINGLE;
            } else if (strcmp(m, "pool") == 0) {
                g.mode = M_POOL;
            } else if (strcmp(m, "master") == 0) {
                g.mode = M_MASTER;
            } else {
                return -1;
            }
            g.mode_name = m;
        } else if (strcmp(a, "--delay-us") == 0 && i + 1 < argc) {
            g.delay_us = atoi(argv[++i]);
        } else if (strcmp(a, "--workers") == 0 && i + 1 < argc) {
            g.workers = atoi(argv[++i]);
        } else if (strcmp(a, "--subs") == 0 && i + 1 < argc) {
            g.subs = atoi(argv[++i]);
        } else if (strcmp(a, "--idle-exit") == 0 && i + 1 < argc) {
            g.idle_exit_ms = atoi(argv[++i]);
        } else if (strcmp(a, "--max-run") == 0 && i + 1 < argc) {
            g.max_run_ms = atoi(argv[++i]);
        } else if (strcmp(a, "--et") == 0) {
            g.et = 1;
            g.flags_name = "et";
        } else if (strcmp(a, "--oneshot") == 0) {
            g.flags_name = "oneshot";
        } else if (strcmp(a, "--oneshot-broken") == 0) {
            g.flags_name = "oneshot-broken";
        } else if (strcmp(a, "--et-broken") == 0) {
            g.flags_name = "et-broken";
            g.et = 1;
        } else {
            return -1;
        }
    }
    if (g.workers < 1 || g.workers > 32 || g.subs < 1 || g.subs > SUB_MAX)
        return -1;
    return 0;
}

int main(int argc, char **argv)
{
    struct reactor *main_r;
    pthread_t workers[32];
    int lfd, port = 0, nw = 0;
    long long t0;
    int served_any = 0;
    long long last_busy = 0;

    signal(SIGPIPE, SIG_IGN);
    if (parse_args(argc, argv) < 0) {
        usage();
        return 2;
    }

    lfd = make_listen(&port);
    if (lfd < 0) {
        pr("SYS04|srv|error|listen_failed|errno=%d\n", errno);
        return 1;
    }
    pr("SYS04|srv|ready|port=%d|mode=%s|flags=%s|delay_us=%d|workers=%d|subs=%d\n",
       port, g.mode_name, g.flags_name, g.delay_us, g.workers,
       g.mode == M_MASTER ? g.subs : 0);

    main_r = reactor_new(reactor_flags(), handle_frames, on_close);
    if (!main_r) {
        pr("SYS04|srv|error|reactor_new_failed\n");
        return 1;
    }

    if (g.mode == M_POOL) {
        g_efd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (g_efd < 0 || reactor_add_notify(main_r, g_efd, pool_on_notify) < 0) {
            pr("SYS04|srv|error|eventfd_failed|errno=%d\n", errno);
            return 1;
        }
        for (int i = 0; i < g.workers; i++)
            pthread_create(&workers[nw++], NULL, worker_main, NULL);
    }

    if (g.mode == M_MASTER) {
        g_nsubs = g.subs;
        for (int i = 0; i < g_nsubs; i++) {
            struct sub *s = &g_subs[i];

            memset(s, 0, sizeof *s);
            pthread_mutex_init(&s->mu, NULL);
            s->efd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
            s->r = reactor_new(reactor_flags(), handle_frames, on_close);
            if (s->efd < 0 || !s->r ||
                reactor_add_notify(s->r, s->efd, sub_on_notify) < 0) {
                pr("SYS04|srv|error|sub_setup_failed|errno=%d\n", errno);
                return 1;
            }
            pthread_create(&s->th, NULL, sub_main, s);
        }
    }

    if (reactor_add_listen(main_r, lfd, g.mode == M_MASTER ? on_accept_master
                                                           : on_accept_single) < 0) {
        pr("SYS04|srv|error|add_listen_failed|errno=%d\n", errno);
        return 1;
    }

    t0 = now_ms();
    last_busy = t0;
    for (;;) {
        const struct reactor_stats *st;
        long accepted_total;
        int active;
        int pending = 0;

        (void)reactor_run_once(main_r, 50);
        st = reactor_stats(main_r);
        active = st->active;
        accepted_total = st->accepted;
        for (int i = 0; i < g_nsubs; i++) {
            const struct reactor_stats *ss = reactor_stats(g_subs[i].r);

            active += ss->active;
            accepted_total += ss->accepted;
            pending += g_subs[i].npending; /* 已 accept 但还没登记到 sub */
        }
        if (accepted_total > 0)
            served_any = 1;
        if (active > 0 || pending > 0)
            last_busy = now_ms();
        if (served_any && active == 0 && pending == 0 && now_ms() - last_busy >= g.idle_exit_ms)
            break;
        if (now_ms() - t0 > g.max_run_ms)
            break;
    }

    /* 收尾：先停 sub 线程，再释放 reactor */
    for (int i = 0; i < g_nsubs; i++) {
        g_subs[i].stop = 1;
        wake(g_subs[i].efd);
    }
    for (int i = 0; i < g_nsubs; i++)
        pthread_join(g_subs[i].th, NULL);
    if (g.mode == M_POOL) {
        pthread_mutex_lock(&g_q_mu);
        g_quit = 1;
        pthread_cond_broadcast(&g_q_cv);
        pthread_mutex_unlock(&g_q_mu);
        for (int i = 0; i < nw; i++)
            pthread_join(workers[i], NULL);
        close(g_efd);
    }

    {
        const struct reactor_stats *st = reactor_stats(main_r);
        long frames = st->frames;

        for (int i = 0; i < g_nsubs; i++)
            frames += reactor_stats(g_subs[i].r)->frames;
        pr("SYS04|srv|done|mode=%s|flags=%s|delay_us=%d|accepted=%ld|handoff=%ld|frames=%ld|"
           "iters=%ld|peak=%d|buf_drops=%ld|q_drops=%ld|serve_ms=%lld",
           g.mode_name, g.flags_name, g.delay_us, st->accepted, g_handoff, frames, st->iters,
           st->peak, st->buf_drops, g_q_drops, now_ms() - t0);
        for (int i = 0; i < g_nsubs; i++)
            pr("|sub%d_frames=%ld", i, reactor_stats(g_subs[i].r)->frames);
        pr("|subs=%d\n", g_nsubs);
    }

    for (int i = 0; i < g_nsubs; i++)
        reactor_free(g_subs[i].r);

    reactor_free(main_r);
    close(lfd);
    return 0;
}
