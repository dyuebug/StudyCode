#define _GNU_SOURCE
#include "reactor.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_EVENTS 256

/* fd → conn* 的直查表。比在 epoll_event 里塞指针简单，
 * 也让"同一个 fd 被注册两次"这类错误直接暴露（槽位非空即重复）。 */
struct reactor {
    int epfd;
    int flags;
    int stop;
    int maxfd;
    struct epoll_event evs[MAX_EVENTS];
    struct conn **slots;
    unsigned char *is_listen;
    struct notify { int fd; notify_cb cb; } notes[8];
    int nnotes;
    accept_cb on_accept;
    frame_cb on_frame;
    close_cb on_close;
    struct reactor_stats st;
};

static uint32_t ep_mask(const struct reactor *r)
{
    uint32_t m = 0;

    if (r->flags & R_ET)
        m |= EPOLLET;
    if (r->flags & (R_ONESHOT | R_ONESHOT_BROKEN))
        m |= EPOLLONESHOT;
    return m;
}

const struct reactor_stats *reactor_stats(const struct reactor *r)
{
    return &r->st;
}

void reactor_note_frame(struct reactor *r)
{
    r->st.frames++;
}

struct reactor *reactor_new(int flags, frame_cb on_frame, close_cb on_close)
{
    long lim = sysconf(_SC_OPEN_MAX);
    struct reactor *r;

    if (lim <= 0 || lim > 65536)
        lim = 65536;
    r = calloc(1, sizeof *r);
    if (!r)
        return NULL;
    r->slots = calloc((size_t)lim, sizeof *r->slots);
    r->is_listen = calloc((size_t)lim, 1);
    if (!r->slots || !r->is_listen) {
        free(r->slots);
        free(r->is_listen);
        free(r);
        return NULL;
    }
    r->maxfd = (int)lim;
    r->flags = flags;
    r->on_frame = on_frame;
    r->on_close = on_close;
    r->epfd = epoll_create1(EPOLL_CLOEXEC);
    if (r->epfd < 0) {
        free(r->slots);
        free(r->is_listen);
        free(r);
        return NULL;
    }
    return r;
}

void reactor_free(struct reactor *r)
{
    if (!r)
        return;
    for (int fd = 0; fd < r->maxfd; fd++) {
        if (r->slots[fd]) {
            close(fd);
            r->slots[fd] = NULL;
        }
        if (r->is_listen[fd])
            close(fd);
    }
    close(r->epfd);
    free(r->slots);
    free(r->is_listen);
    free(r);
}

static int ctl(struct reactor *r, int op, int fd, uint32_t events, uint32_t mask)
{
    struct epoll_event e;

    memset(&e, 0, sizeof e);
    e.events = events | mask;
    e.data.u32 = (uint32_t)fd;
    return epoll_ctl(r->epfd, op, fd, &e);
}

/* 监听 fd 与唤醒 fd 只能带 ET，**不能带 EPOLLONESHOT**：
 * 内核在投递一次后就不再通知，而监听 fd 没有人替它重挂。
 * 首版把 ep_mask() 无差别用在所有 fd 上，结果 8 条连接只 accept 到 4 条
 * （oneshot 用例的 accepted=4），看起来像客户端的问题。 */
static uint32_t aux_mask(const struct reactor *r)
{
    return (r->flags & R_ET) ? EPOLLET : 0;
}

int reactor_add_listen(struct reactor *r, int lfd, accept_cb on_accept)
{
    if (lfd < 0 || lfd >= r->maxfd)
        return -1;
    r->on_accept = on_accept;
    r->is_listen[lfd] = 1;
    if (ctl(r, EPOLL_CTL_ADD, lfd, EPOLLIN, aux_mask(r)) < 0)
        return -1;
    return 0;
}

int reactor_add_notify(struct reactor *r, int fd, notify_cb on_notify)
{
    if (r->nnotes >= (int)(sizeof r->notes / sizeof r->notes[0]))
        return -1;
    if (ctl(r, EPOLL_CTL_ADD, fd, EPOLLIN, aux_mask(r)) < 0)
        return -1;
    r->notes[r->nnotes].fd = fd;
    r->notes[r->nnotes].cb = on_notify;
    r->nnotes++;
    return 0;
}

int reactor_add_conn(struct reactor *r, struct conn *c)
{
    if (c->fd < 0 || c->fd >= r->maxfd)
        return -1;
    if (r->slots[c->fd])
        return -1; /* 同一 fd 重复登记：调用方的 bug，直接拒绝 */
    r->slots[c->fd] = c;
    r->st.accepted++;
    r->st.active++;
    if (r->st.active > r->st.peak)
        r->st.peak = r->st.active;
    if (ctl(r, EPOLL_CTL_ADD, c->fd, EPOLLIN, ep_mask(r)) < 0) {
        r->slots[c->fd] = NULL;
        r->st.active--;
        return -1;
    }
    c->reg = EPOLLIN;
    return 0;
}

static int apply_events(struct reactor *r, struct conn *c, uint32_t want)
{
    /* EPOLLONESHOT 下内核在投递一次后就把事件摘掉了，所以即使掩码没变、
     * 也必须再 MOD 一次才算"重挂"。普通模式只在掩码变化时改，省系统调用。 */
    if (r->flags & R_ONESHOT_BROKEN)
        return 0; /* 破坏版：故意永远不重挂 */
    if (!(r->flags & R_ONESHOT) && want == c->reg)
        return 0;
    if (ctl(r, EPOLL_CTL_MOD, c->fd, want, ep_mask(r)) < 0)
        return -1;
    c->reg = want;
    return 0;
}

void conn_close(struct reactor *r, struct conn *c)
{
    int fd;

    if (!c)
        return;
    fd = c->fd;
    epoll_ctl(r->epfd, EPOLL_CTL_DEL, fd, NULL);
    if (fd < r->maxfd && r->slots[fd] == c)
        r->slots[fd] = NULL;
    close(fd);
    r->st.active--;
    r->st.closed++;
    if (r->on_close)
        r->on_close(r, c);
}

static int conn_flush(struct reactor *r, struct conn *c)
{
    /* 已经在关的方向不再挂 EPOLLIN：否则对端持续可读会让循环空转 */
    uint32_t want = c->closing ? 0 : EPOLLIN;

    while (c->wlen > 0) {
        ssize_t n = send(c->fd, c->wbuf, c->wlen, MSG_NOSIGNAL);

        if (n > 0) {
            /* 简化点：真实实现用读/写偏移量，这里 memmove 是为了让
             * "缓冲里剩下的字节 = 还没发出去的字节"始终成立。 */
            memmove(c->wbuf, c->wbuf + n, c->wlen - (size_t)n);
            c->wlen -= (size_t)n;
        } else if (n < 0 && (errno == EAGAIN || errno == EINTR)) {
            break;
        } else {
            return -1;
        }
    }
    if (c->wlen > 0)
        want |= EPOLLOUT;
    return apply_events(r, c, want);
}

int conn_send(struct reactor *r, struct conn *c, const void *data, size_t n)
{
    if (c->wlen + n > WBUF_SIZE) {
        r->st.buf_drops++; /* 显式判据：宁可计数后断开，也不悄悄丢字节 */
        return -1;
    }
    memcpy(c->wbuf + c->wlen, data, n);
    c->wlen += n;
    return conn_flush(r, c);
}

void conn_shutdown_after_write(struct reactor *r, struct conn *c)
{
    c->closing = 1;
    if (conn_flush(r, c) < 0 || c->wlen == 0)
        conn_close(r, c);
}

int conn_drain(struct reactor *r, struct conn *c)
{
    int got = 0;

    for (;;) {
        size_t space = RBUF_SIZE - c->rlen;
        ssize_t n;

        if (space == 0)
            break; /* 缓冲满：交给业务先消费 */
        /* 破坏版每次只读 128 字节并立即停下：同一批数据读不完，
         * 剩下的字节留在内核缓冲里，ET 下不再产生边沿。 */
        if ((r->flags & R_ET_BROKEN) && space > 128)
            space = 128;
        n = recv(c->fd, c->rbuf + c->rlen, space, 0);
        if (n > 0) {
            c->rlen += (size_t)n;
            got = 1;
            /* 破坏版：只读一次。缓冲区里剩下的字节不再产生边沿事件。 */
            if (r->flags & R_ET_BROKEN)
                break;
            continue;
        }
        if (n == 0) {
            c->closing = 1;
            break;
        }
        if (errno == EINTR)
            continue;
        if (errno == EAGAIN)
            break;
        return -1;
    }
    if (got && r->on_frame && !c->busy) {
        if (r->on_frame(r, c) < 0)
            return -1;
    }
    return 0;
}

int reactor_kick(struct reactor *r, struct conn *c)
{
    if (!r->on_frame || c->busy)
        return 0;
    return r->on_frame(r, c);
}

int reactor_run_once(struct reactor *r, int timeout_ms)
{
    int n = epoll_wait(r->epfd, r->evs, MAX_EVENTS, timeout_ms);

    if (n < 0) {
        if (errno == EINTR)
            return 0;
        return -1;
    }
    if (n == 0)
        return 0;
    r->st.iters++;

    for (int i = 0; i < n; i++) {
        int fd = (int)r->evs[i].data.u32;
        uint32_t ev = r->evs[i].events;
        struct conn *c;

        if (fd < 0 || fd >= r->maxfd)
            continue;

        {
            int handled = 0;

            for (int k = 0; k < r->nnotes; k++) {
                if (r->notes[k].fd == fd) {
                    r->notes[k].cb(r);
                    handled = 1;
                    break;
                }
            }
            if (handled)
                continue;
        }

        if (r->is_listen[fd]) {
            /* accept 也要循环到 EAGAIN：ET 下漏掉就再也不通知 */
            for (;;) {
                int afd = accept(fd, NULL, NULL);

                if (afd < 0) {
                    if (errno == EINTR)
                        continue;
                    break; /* EAGAIN 或真错误 */
                }
                if (r->on_accept && r->on_accept(r, afd) < 0) {
                    close(afd);
                    continue;
                }
            }
            continue;
        }

        c = r->slots[fd];
        if (!c)
            continue;

        if (ev & (EPOLLERR | EPOLLHUP)) {
            conn_close(r, c);
            continue;
        }
        if (ev & (EPOLLIN)) {
            if (conn_drain(r, c) < 0) {
                conn_close(r, c);
                continue;
            }
            if (c->closing) {
                /* 对端关了写方向：把剩余数据发完就关 */
                if (c->wlen == 0)
                    conn_close(r, c);
                else
                    (void)apply_events(r, c, 0);
                continue;
            }
        }
        if (ev & EPOLLOUT) {
            if (c->busy) {
                /* 线程池模式下 worker 还没交回结果：先摘掉可写关注，
                 * 等结果入队后由 reactor 自己重新挂上。 */
                (void)apply_events(r, c, 0);
                continue;
            }
            if (conn_flush(r, c) < 0 || (c->closing && c->wlen == 0)) {
                conn_close(r, c);
                continue;
            }
        }
        /* EPOLLONESHOT：处理完必须重挂，否则这条连接再也收不到事件。
         * `oneshot-broken` 不设置 R_ONESHOT，所以这段对它不生效。 */
        if (r->flags & R_ONESHOT) {
            uint32_t want = c->closing ? 0 : EPOLLIN;

            if (c->wlen > 0)
                want |= EPOLLOUT;
            (void)apply_events(r, c, want);
        }
    }
    return n;
}

void reactor_stop(struct reactor *r)
{
    r->stop = 1;
}
