/* SYS_03 阶段 3 · C 组：select / poll / epoll 三套多路复用
 *
 * 同一套协议、同一个事件循环骨架，只换"谁告诉我哪个 fd 就绪"：
 *   select   位图 + FD_SETSIZE 上界，每次都要重新填三个集合
 *   poll     数组 + 每个 fd 的 revents，没有上界，但仍是 O(n) 扫描
 *   epoll    Red-Black 树 + 就绪链表，注册一次，返回的只有就绪项
 *   epoll-et 边沿触发：**必须读到 EAGAIN**，否则剩余数据不会再有事件
 *
 * 为了把 ET 的这条约束变成可观测的事实，读取块故意只有 16 字节：
 * 一条 100 字节的消息需要 7 次 read。正确实现循环读到 EAGAIN；
 * `epoll-et-broken` 只读一次，于是 84 字节留在内核缓冲里不再产生事件，用例停住。
 * 这不是"模拟"，是 ET 的真实失效方式。
 *
 * 全部 fd 都在回环地址上，客户端同时也是本进程的 fd，
 * 所以不需要线程，也不会因为端口固定而互相干扰。
 */
#define _GNU_SOURCE
#include "net_common.h"

#include <sys/epoll.h>
#include <sys/select.h>

#define MAX_CONN 8
#define MSG_BYTES 100
#define READ_CHUNK 16 /* 故意小：让"必须读到 EAGAIN"成为必现条件 */
#define DEADLINE_MS 2000

struct side {
    int fd;
    size_t got;
    int finished;
};

struct ctx {
    struct side srv[MAX_CONN]; /* 服务端侧（accept 得到的连接） */
    struct side cli[MAX_CONN]; /* 客户端侧 */
    char payload[MAX_CONN][MSG_BYTES];
    int n;
    int echoed;
    int done;
    int mismatch;
    int et_once; /* 1 = 故意只读一次，用来证伪 ET 的实现要求 */
    int epfd;
    int events;
};

/* 非阻塞写干净：回显只有 100 字节，正常不会 EAGAIN，
 * 但"正常不会"不是理由——EAGAIN 必须被处理，否则就是隐藏的丢字节。 */
static int write_nb_all(int fd, const void *buf, size_t len)
{
    const char *p = buf;
    size_t left = len;

    while (left > 0) {
        ssize_t n = send(fd, p, left, MSG_NOSIGNAL);

        if (n > 0) {
            p += (size_t)n;
            left -= (size_t)n;
            continue;
        }
        if (n < 0 && errno == EINTR)
            continue;
        if (n < 0 && errno == EAGAIN) {
            struct pollfd pfd = { fd, POLLOUT, 0 };

            if (poll(&pfd, 1, 500) <= 0)
                return -1;
            continue;
        }
        return -1;
    }
    return 0;
}

/* 服务端侧可读：攒够一条完整消息再回显 */
static void handle_srv(struct ctx *x, int i)
{
    struct side *s = &x->srv[i];
    char buf[READ_CHUNK];

    for (;;) {
        ssize_t n = recv(s->fd, buf, sizeof buf, 0);

        if (n > 0) {
            s->got += (size_t)n;
            if (x->et_once)
                break; /* 破坏版：只处理一个块，剩下的留在内核缓冲里 */
            continue; /* 正确版：一直读到 EAGAIN */
        }
        break;
    }
    if (s->got >= MSG_BYTES && !s->finished) {
        s->finished = 1;
        x->echoed++;
        if (write_nb_all(s->fd, x->payload[i], MSG_BYTES) < 0)
            x->mismatch++;
    }
}

/* 客户端侧可读：逐字节核对回显内容 */
static void handle_cli(struct ctx *x, int i)
{
    struct side *c = &x->cli[i];
    char buf[READ_CHUNK * 4];

    for (;;) {
        ssize_t n = recv(c->fd, buf, sizeof buf, 0);

        if (n > 0) {
            if (c->got + (size_t)n > MSG_BYTES) {
                x->mismatch++;
                break;
            }
            if (memcmp(buf, x->payload[i] + c->got, (size_t)n) != 0)
                x->mismatch++;
            c->got += (size_t)n;
            if (c->got >= MSG_BYTES) {
                if (!c->finished) {
                    c->finished = 1;
                    x->done++;
                }
                break;
            }
            continue;
        }
        break;
    }
}

static void pump_ready(struct ctx *x, int *srv_ready, int *cli_ready)
{
    for (int i = 0; i < x->n; i++)
        if (srv_ready[i])
            handle_srv(x, i);
    for (int i = 0; i < x->n; i++)
        if (cli_ready[i])
            handle_cli(x, i);
}

static int pump_select(struct ctx *x)
{
    int srv_ready[MAX_CONN] = { 0 };
    int cli_ready[MAX_CONN] = { 0 };
    struct timeval tv = { .tv_sec = 0, .tv_usec = 100 * 1000 };
    fd_set rf;
    int maxfd = -1, rc;

    FD_ZERO(&rf);
    for (int i = 0; i < x->n; i++) {
        FD_SET(x->srv[i].fd, &rf);
        FD_SET(x->cli[i].fd, &rf);
        if (x->srv[i].fd > maxfd)
            maxfd = x->srv[i].fd;
        if (x->cli[i].fd > maxfd)
            maxfd = x->cli[i].fd;
    }
    rc = select(maxfd + 1, &rf, NULL, NULL, &tv);
    if (rc <= 0)
        return rc; /* 0 = 超时，-1 = 出错（EINTR 由调用方继续） */
    for (int i = 0; i < x->n; i++) {
        if (FD_ISSET(x->srv[i].fd, &rf))
            srv_ready[i] = 1;
        if (FD_ISSET(x->cli[i].fd, &rf))
            cli_ready[i] = 1;
    }
    pump_ready(x, srv_ready, cli_ready);
    return rc;
}

static int pump_poll(struct ctx *x)
{
    struct pollfd pfd[MAX_CONN * 2];
    int srv_ready[MAX_CONN] = { 0 };
    int cli_ready[MAX_CONN] = { 0 };
    int rc;

    for (int i = 0; i < x->n; i++) {
        pfd[i * 2].fd = x->srv[i].fd;
        pfd[i * 2].events = POLLIN;
        pfd[i * 2].revents = 0;
        pfd[i * 2 + 1].fd = x->cli[i].fd;
        pfd[i * 2 + 1].events = POLLIN;
        pfd[i * 2 + 1].revents = 0;
    }
    rc = poll(pfd, (nfds_t)(x->n * 2), 100);
    if (rc <= 0)
        return rc;
    for (int i = 0; i < x->n; i++) {
        if (pfd[i * 2].revents & POLLIN)
            srv_ready[i] = 1;
        if (pfd[i * 2 + 1].revents & POLLIN)
            cli_ready[i] = 1;
    }
    pump_ready(x, srv_ready, cli_ready);
    return rc;
}

static int pump_epoll(struct ctx *x)
{
    struct epoll_event ev[2 * MAX_CONN];
    int srv_ready[MAX_CONN] = { 0 };
    int cli_ready[MAX_CONN] = { 0 };
    int rc = epoll_wait(x->epfd, ev, 2 * MAX_CONN, 100);

    if (rc <= 0)
        return rc;
    for (int k = 0; k < rc; k++) {
        /* data.fd 编码：偶数 = 服务端侧下标，奇数 = 客户端侧下标 */
        int code = (int)ev[k].data.u32;

        if (code % 2 == 0)
            srv_ready[code / 2] = 1;
        else
            cli_ready[code / 2] = 1;
    }
    pump_ready(x, srv_ready, cli_ready);
    return rc;
}

static void case_mux(const char *mode, int backend, int et, int et_once)
{
    struct ctx x;
    int port = 0;
    int lfd;
    uint32_t evmask = EPOLLIN | (et ? EPOLLET : 0u);
    long long t0, elapsed;
    int ok;

    memset(&x, 0, sizeof x);
    x.et_once = et_once;
    x.epfd = -1;

    lfd = listen_loopback(&port);
    if (lfd < 0) {
        pr("SYS03|mux|mode=%s|setup_failed|errno=%d\n", mode, errno);
        return;
    }

    /* 先把 8 个客户端全部连上并各发一条消息，消息会躺在接收缓冲里等 accept */
    for (int i = 0; i < MAX_CONN; i++) {
        for (int k = 0; k < MSG_BYTES; k++)
            x.payload[i][k] = (char)('a' + i);
        x.cli[i].fd = connect_loopback(port);
        if (x.cli[i].fd < 0) {
            pr("SYS03|mux|mode=%s|connect_failed|errno=%d\n", mode, errno);
            close(lfd);
            return;
        }
        if (write_all(x.cli[i].fd, x.payload[i], MSG_BYTES) < 0) {
            pr("SYS03|mux|mode=%s|client_write_failed|errno=%d\n", mode, errno);
            close(lfd);
            return;
        }
        set_nonblock(x.cli[i].fd);
    }

    for (int i = 0; i < MAX_CONN; i++) {
        int fd = accept(lfd, NULL, NULL);

        if (fd < 0) {
            pr("SYS03|mux|mode=%s|accept_failed|i=%d|errno=%d\n", mode, i, errno);
            break;
        }
        set_nonblock(fd);
        x.srv[i].fd = fd;
        x.n++;
    }
    close(lfd);

    if (backend == 2) {
        struct epoll_event ev;

        x.epfd = epoll_create1(0);
        if (x.epfd < 0) {
            pr("SYS03|mux|mode=%s|epoll_create_failed|errno=%d\n", mode, errno);
            goto out;
        }
        for (int i = 0; i < x.n; i++) {
            memset(&ev, 0, sizeof ev);
            ev.events = evmask;
            ev.data.u32 = (uint32_t)(i * 2);
            if (epoll_ctl(x.epfd, EPOLL_CTL_ADD, x.srv[i].fd, &ev) < 0)
                pr("SYS03|mux|mode=%s|epoll_ctl_srv_failed|errno=%d\n", mode, errno);
            ev.data.u32 = (uint32_t)(i * 2 + 1);
            if (epoll_ctl(x.epfd, EPOLL_CTL_ADD, x.cli[i].fd, &ev) < 0)
                pr("SYS03|mux|mode=%s|epoll_ctl_cli_failed|errno=%d\n", mode, errno);
        }
    }

    t0 = now_ms();
    while (x.done < x.n) {
        int rc;

        if (now_ms() - t0 > DEADLINE_MS)
            break;
        if (backend == 0)
            rc = pump_select(&x);
        else if (backend == 1)
            rc = pump_poll(&x);
        else
            rc = pump_epoll(&x);
        if (rc < 0 && errno != EINTR)
            break;
        if (rc > 0)
            x.events += rc;
    }
    elapsed = now_ms() - t0;

    ok = (x.n == MAX_CONN && x.done == MAX_CONN && x.mismatch == 0) ? 1 : 0;
    pr("SYS03|mux|mode=%s|clients=%d|accepted=%d|echoed=%d|done=%d|mismatch=%d|events=%d|"
       "elapsed_ms=%lld|stalled=%d|ok=%d\n",
       mode, MAX_CONN, x.n, x.echoed, x.done, x.mismatch, x.events, elapsed,
       (x.done < x.n) ? 1 : 0, ok);

out:
    if (x.epfd >= 0)
        close(x.epfd);
    for (int i = 0; i < MAX_CONN; i++) {
        if (x.srv[i].fd > 0)
            close(x.srv[i].fd);
        if (x.cli[i].fd > 0)
            close(x.cli[i].fd);
    }
}

int main(int argc, char **argv)
{
    const char *mode = argc > 1 ? argv[1] : "epoll";

    if (strcmp(mode, "select") == 0)
        case_mux(mode, 0, 0, 0);
    else if (strcmp(mode, "poll") == 0)
        case_mux(mode, 1, 0, 0);
    else if (strcmp(mode, "epoll") == 0)
        case_mux(mode, 2, 0, 0);
    else if (strcmp(mode, "epoll-et") == 0)
        case_mux(mode, 2, 1, 0);
    else if (strcmp(mode, "epoll-et-broken") == 0)
        case_mux(mode, 2, 1, 1);
    else {
        pr("用法: mux <select|poll|epoll|epoll-et|epoll-et-broken>\n");
        return 2;
    }
    pr("SYS03|mux|done|mode=%s\n", mode);
    return 0;
}
