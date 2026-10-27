/* SYS_03 阶段 3 · A 组：TCP 是字节流
 *
 * 六个用例共用一个程序，因为它们要的不是六种"功能"，而是六条"语义边界"：
 *   echo       三条 write 合成一次 read；长度前缀才能恢复消息边界
 *   halfclose  shutdown(SHUT_WR) 之后仍能读；对端读到 0 不等于连接已不可写
 *   reset      SO_LINGER{on,0} 触发 RST，服务端 recv 得到 ECONNRESET
 *   sigpipe    对端已关闭时 send 会先收到 SIGPIPE；默认动作是终止进程
 *   timeout    SO_RCVTIMEO 让 recv 在指定时间后返回 EAGAIN
 *   shortwrite 发送缓冲满了就是短写，必须循环到写完
 *
 * 所有连接都在 127.0.0.1 上，监听端口用 bind(0) 让内核分配，
 * 因此可以反复运行、并发运行，不依赖固定端口。
 */
#define _GNU_SOURCE
#include "net_common.h"

#include <signal.h>
#include <stdlib.h>

#define MSG_COUNT 3
#define MSG_LEN 1000
#define FRAME_BUF 8192

/* ---------- 长度前缀分帧 ---------- */
/* 字节流没有边界，要恢复消息边界只有两条路：长度前缀或分隔符。
 * 这里用 4 字节大端长度前缀，`frames` 计数就是"恢复出来的消息条数"。 */
struct framer {
    unsigned char buf[FRAME_BUF];
    size_t have;
    int frames;
};

static void framer_feed(struct framer *f, const void *data, size_t n)
{
    if (f->have + n > sizeof f->buf)
        return; /* 取证数据量远小于缓冲；溢出说明用例本身写错了 */
    memcpy(f->buf + f->have, data, n);
    f->have += n;
    for (;;) {
        uint32_t mlen;

        if (f->have < 4)
            break;
        memcpy(&mlen, f->buf, 4);
        mlen = ntohl(mlen);
        if (mlen > sizeof f->buf - 4)
            break;
        if (f->have < 4 + (size_t)mlen)
            break;
        f->frames++;
        f->have -= 4 + (size_t)mlen;
        memmove(f->buf, f->buf + 4 + (size_t)mlen, f->have);
    }
}

/* ---------- 建一对回环连接 ---------- */
static int make_pair(int *a_out, int *b_out)
{
    int port = 0;
    int lfd = listen_loopback(&port);
    int a, b;

    if (lfd < 0)
        return -1;
    a = connect_loopback(port);
    if (a < 0) {
        close(lfd);
        return -1;
    }
    b = accept(lfd, NULL, NULL);
    close(lfd);
    if (b < 0) {
        close(a);
        return -1;
    }
    *a_out = a;
    *b_out = b;
    return 0;
}

/* ---------- echo：合并读 + 分帧 + EOF ---------- */
static void case_echo(void)
{
    char payload[MSG_LEN];
    char in[65536];
    unsigned char *stream;
    size_t stream_len = 0;
    size_t first_read = 0;
    size_t got = 0;
    struct framer sf, cf;
    int port = 0, lfd = -1, cfd = -1, sfd = -1;
    int client_frames = 0, eof = 0;
    ssize_t n;

    memset(payload, 'A', sizeof payload);
    memset(&sf, 0, sizeof sf);
    memset(&cf, 0, sizeof cf);

    stream = malloc((size_t)MSG_COUNT * (4 + MSG_LEN));
    if (!stream)
        return;
    for (int i = 0; i < MSG_COUNT; i++) {
        uint32_t be = htonl(MSG_LEN);

        memcpy(stream + stream_len, &be, 4);
        stream_len += 4;
        memcpy(stream + stream_len, payload, MSG_LEN);
        stream_len += MSG_LEN;
    }

    lfd = listen_loopback(&port);
    cfd = connect_loopback(port);
    if (lfd < 0 || cfd < 0) {
        pr("SYS03|tcp|echo|setup_failed|errno=%d\n", errno);
        goto out;
    }

    /* 三次独立 write，模拟"三条消息" */
    for (int i = 0; i < MSG_COUNT; i++) {
        if (write_all(cfd, payload, sizeof payload) < 0) {
            pr("SYS03|tcp|echo|write_failed|errno=%d\n", errno);
            goto out;
        }
    }

    /* 故意晚 100 ms 再收：让 3000 字节全部落进接收缓冲，
     * 这样 first_read 反映的是"字节流会合并"，而不是"调度恰好切在这里"。
     * 与 SYS_02 的 first_read 同一个手法。 */
    usleep(100 * 1000);
    sfd = accept(lfd, NULL, NULL);
    if (sfd < 0) {
        pr("SYS03|tcp|echo|accept_failed|errno=%d\n", errno);
        goto out;
    }

    n = recv_retry(sfd, in, sizeof in);
    if (n > 0)
        first_read = (size_t)n;

    /* 再用长度前缀发一次同样的三条消息，服务端解析后原样回显 */
    if (write_all(cfd, stream, stream_len) < 0)
        goto out;
    while (got < stream_len) {
        n = recv_retry(sfd, in + got, sizeof in - got);
        if (n <= 0)
            break;
        got += (size_t)n;
    }
    framer_feed(&sf, in, got);
    if (got > 0 && write_all(sfd, in, got) < 0)
        goto out;

    while (cf.frames < MSG_COUNT) {
        n = recv_retry(cfd, in, sizeof in);
        if (n <= 0)
            break;
        framer_feed(&cf, in, (size_t)n);
    }
    client_frames = cf.frames;

    /* 客户端关掉写方向，服务端应当读到 0 字节 */
    shutdown(cfd, SHUT_WR);
    n = recv_retry(sfd, in, sizeof in);
    eof = (n == 0) ? 1 : 0;

    pr("SYS03|tcp|echo|sent=%d|first_read=%zu|boundary=%s|framed=%d|client_frames=%d|eof=%d\n",
       MSG_COUNT * MSG_LEN, first_read,
       (first_read == (size_t)(MSG_COUNT * MSG_LEN)) ? "merged" : "split", sf.frames,
       client_frames, eof);

out:
    free(stream);
    if (sfd >= 0)
        close(sfd);
    if (cfd >= 0)
        close(cfd);
    if (lfd >= 0)
        close(lfd);
}

/* ---------- halfclose：关写方向不等于不能读 ---------- */
static void case_halfclose(void)
{
    char buf[64];
    int a = -1, b = -1;
    int got_ping = 0, eof_seen = 0, sent_after_eof = 0, got_pong = 0;
    ssize_t n;

    if (make_pair(&a, &b) < 0) {
        pr("SYS03|tcp|halfclose|setup_failed|errno=%d\n", errno);
        return;
    }

    if (write_all(a, "PING", 4) < 0)
        goto out;
    shutdown(a, SHUT_WR); /* 客户端：我不再发数据了，但我还在等你的回复 */

    n = recv_retry(b, buf, sizeof buf);
    got_ping = (n == 4 && memcmp(buf, "PING", 4) == 0) ? 1 : 0;

    n = recv_retry(b, buf, sizeof buf);
    eof_seen = (n == 0) ? 1 : 0; /* 服务端：读到 0，说明对端写方向已关 */

    /* 关键点：此时服务端的写方向仍然有效 */
    if (write_all(b, "PONG", 4) == 0)
        sent_after_eof = 1;
    shutdown(b, SHUT_WR);

    n = recv_retry(a, buf, sizeof buf);
    got_pong = (n == 4 && memcmp(buf, "PONG", 4) == 0) ? 1 : 0;

    pr("SYS03|tcp|halfclose|got_ping=%d|eof_seen=%d|sent_after_eof=%d|client_got=%s|"
       "read_after_shutdown=%d\n",
       got_ping, eof_seen, sent_after_eof, got_pong ? "PONG" : "none",
       (eof_seen && got_pong) ? 1 : 0);

out:
    close(a);
    close(b);
}

/* ---------- reset：SO_LINGER{on,0} 把正常关闭变成 RST ---------- */
static void case_reset(void)
{
    struct linger lg = { .l_onoff = 1, .l_linger = 0 };
    char buf[64];
    int a = -1, b = -1;
    int err = 0;
    ssize_t n;

    if (make_pair(&a, &b) < 0) {
        pr("SYS03|tcp|reset|setup_failed|errno=%d\n", errno);
        return;
    }

    if (setsockopt(a, SOL_SOCKET, SO_LINGER, &lg, sizeof lg) < 0) {
        pr("SYS03|tcp|reset|setsockopt_failed|errno=%d\n", errno);
        close(a);
        close(b);
        return;
    }
    close(a); /* 带 linger 0 关闭 = 直接发 RST，不发 FIN */
    a = -1;
    usleep(50 * 1000); /* 让 RST 到达对端 */

    n = recv_retry(b, buf, sizeof buf);
    if (n < 0)
        err = errno;

    pr("SYS03|tcp|reset|recv_rc=%d|errno=%d|detected=%d\n", (int)n, err,
       (n < 0 && err == ECONNRESET) ? 1 : 0);

    close(b);
}

/* ---------- sigpipe：写已关闭的对端先收信号，再看 errno ----------
 *
 * 这里必须用**正常关闭（FIN）**，不能用 SO_LINGER{on,0} 的 RST：
 * 首版照搬 reset 用例的写法，两次都得到 ECONNRESET(104)，SIGPIPE 一次没来。
 * 原因是 RST 会让 socket 直接进入错误态，"对端已关闭"以 ECONNRESET 的形式暴露；
 * 只有收到 FIN、本地仍然 write 时，内核才走 SIGPIPE/EPIPE 这条路径。
 * 这条差异本身就是本用例的结论，所以 reset 用例保留不合并。
 */
static volatile sig_atomic_t g_sigpipe_hits;

static void on_sigpipe(int sig)
{
    (void)sig;
    g_sigpipe_hits++;
}

struct wpair {
    int writes; /* 出错前成功的写次数 */
    int err;
};

static struct wpair write_until_error(int fd, int flags)
{
    struct wpair r = { 0, 0 };

    for (int k = 0; k < 8; k++) {
        ssize_t n = send(fd, "X", 1, flags);

        if (n <= 0) {
            r.err = errno;
            return r;
        }
        r.writes++;
        /* 给对端的 RST 一点时间回来：否则第二写可能还排在缓冲区里 */
        usleep(20 * 1000);
    }
    return r;
}

static void case_sigpipe(void)
{
    struct sigaction sa;
    struct wpair ign, hnd;
    int a = -1, b = -1;
    int hits;

    /* 第一对：忽略 SIGPIPE，看 write 返回什么 */
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        pr("SYS03|tcp|sigpipe|signal_failed|errno=%d\n", errno);
        return;
    }
    if (make_pair(&a, &b) < 0) {
        pr("SYS03|tcp|sigpipe|setup_failed|errno=%d\n", errno);
        return;
    }
    close(b); /* 正常关闭：发 FIN，不发 RST */
    b = -1;
    usleep(50 * 1000);
    ign = write_until_error(a, MSG_NOSIGNAL);
    close(a);
    a = -1;

    /* 第二对：装一个计数处理函数，证明信号确实送达、errno 仍是 EPIPE */
    g_sigpipe_hits = 0;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_sigpipe;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGPIPE, &sa, NULL) < 0) {
        pr("SYS03|tcp|sigpipe|sigaction_failed|errno=%d\n", errno);
        return;
    }
    if (make_pair(&a, &b) < 0) {
        pr("SYS03|tcp|sigpipe|setup_failed|errno=%d\n", errno);
        return;
    }
    close(b);
    b = -1;
    usleep(50 * 1000);
    hnd = write_until_error(a, 0); /* 不带 MSG_NOSIGNAL：信号会先到 */
    hits = (int)g_sigpipe_hits;
    close(a);

    pr("SYS03|tcp|sigpipe|default_would_kill=1|ignore_writes=%d|ignore_errno=%d|"
       "handler_writes=%d|handler_errno=%d|handler_hits=%d\n",
       ign.writes, ign.err, hnd.writes, hnd.err, hits);
}

/* ---------- timeout：接收超时是 EAGAIN，不是 EOF ---------- */
static void case_timeout(void)
{
    struct timeval tv = { .tv_sec = 0, .tv_usec = 200 * 1000 };
    char buf[64];
    int a = -1, b = -1;
    long long t0, dt;
    int err = 0;
    ssize_t n;

    if (make_pair(&a, &b) < 0) {
        pr("SYS03|tcp|timeout|setup_failed|errno=%d\n", errno);
        return;
    }
    if (setsockopt(b, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv) < 0) {
        pr("SYS03|tcp|timeout|setsockopt_failed|errno=%d\n", errno);
        close(a);
        close(b);
        return;
    }

    t0 = now_ms();
    n = recv(b, buf, sizeof buf, 0); /* 对端不发数据，只等超时 */
    dt = now_ms() - t0;
    if (n < 0)
        err = errno;

    pr("SYS03|tcp|timeout|rcvtimeo_ms=200|recv_rc=%d|errno=%d|elapsed_ms=%lld|timed_out=%d\n",
       (int)n, err, dt, (n < 0 && err == EAGAIN) ? 1 : 0);

    close(a);
    close(b);
}

/* ---------- shortwrite：发送缓冲满了就是短写 ---------- */
static void case_shortwrite(void)
{
    const size_t payload = 1024u * 1024u;
    char *data;
    char drain[16384];
    int sndbuf = 4096;
    int a = -1, b = -1;
    size_t sent = 0, recvd = 0;
    int short_writes = 0, mismatch = 0;
    long long t0;
    int ok = 0;

    data = malloc(payload);
    if (!data)
        return;
    for (size_t i = 0; i < payload; i++)
        data[i] = (char)(i & 0xff);

    if (make_pair(&a, &b) < 0) {
        pr("SYS03|tcp|shortwrite|setup_failed|errno=%d\n", errno);
        free(data);
        return;
    }
    /* 把发送缓冲压到 4 KB：1 MiB 的写入不可能一次装完，短写必现 */
    if (setsockopt(a, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof sndbuf) < 0)
        pr("SYS03|tcp|shortwrite|sndbuf_note|errno=%d\n", errno);
    set_nonblock(a);
    set_nonblock(b);

    t0 = now_ms();
    while (sent < payload || recvd < payload) {
        if (now_ms() - t0 > 10000)
            break; /* 上界：宁可判失败也不挂住 */

        if (sent < payload) {
            size_t want = payload - sent;
            ssize_t n = send(a, data + sent, want, MSG_NOSIGNAL);

            if (n > 0) {
                if ((size_t)n < want)
                    short_writes++;
                sent += (size_t)n;
            } else if (n < 0 && errno != EAGAIN && errno != EINTR) {
                break;
            }
        }
        for (;;) {
            ssize_t n = recv(b, drain, sizeof drain, 0);

            if (n > 0) {
                if (recvd + (size_t)n > payload) {
                    mismatch++; /* 收到超出约定的字节：判失败，不读越界 */
                    break;
                }
                if (memcmp(drain, data + recvd, (size_t)n) != 0)
                    mismatch++;
                recvd += (size_t)n;
                continue;
            }
            break;
        }
        {
            struct pollfd p[2] = { { a, POLLOUT, 0 }, { b, POLLIN, 0 } };

            poll(p, 2, 500);
        }
    }

    ok = (sent == payload && recvd == payload && mismatch == 0) ? 1 : 0;
    pr("SYS03|tcp|shortwrite|payload=%zu|sent=%zu|recv=%zu|short_writes=%d|mismatch=%d|ok=%d\n",
       payload, sent, recvd, short_writes, mismatch, ok);

    close(a);
    close(b);
    free(data);
}

int main(int argc, char **argv)
{
    const char *mode = argc > 1 ? argv[1] : "echo";

    if (strcmp(mode, "echo") == 0)
        case_echo();
    else if (strcmp(mode, "halfclose") == 0)
        case_halfclose();
    else if (strcmp(mode, "reset") == 0)
        case_reset();
    else if (strcmp(mode, "sigpipe") == 0)
        case_sigpipe();
    else if (strcmp(mode, "timeout") == 0)
        case_timeout();
    else if (strcmp(mode, "shortwrite") == 0)
        case_shortwrite();
    else {
        pr("用法: tcp_basic <echo|halfclose|reset|sigpipe|timeout|shortwrite>\n");
        return 2;
    }
    pr("SYS03|tcp|done|mode=%s\n", mode);
    return 0;
}
