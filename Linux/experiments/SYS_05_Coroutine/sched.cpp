/* SYS_05 · 协程版 echo 服务器（epoll + coroutine）
 *
 * 每个连接一个协程，单线程内并发：
 *   recv → （EAGAIN 就 co_await 可读）→ echo → （EAGAIN 就 co_await 可写）
 *
 * 形态：
 *   --mode coro      协程（默认）      --mode thread   每连接一个阻塞线程（对照）
 *   --batch N        每次 resume 最多处理 N 帧再让出（0 = 读到 EAGAIN）
 *   --idle-ms N      空闲超时，超时后 cancel(fd, ETIMEDOUT)
 *   --eagain-as-eof  破坏版：把 EAGAIN 当 EOF → 每条连接只回一帧
 *   --cancel-free    破坏版：超时后 destroy 但不 detach、不 close → 事件打到已销毁的槽位
 *
 * 输出以 `S05|srv|` 开头；`done` 行给出统计，由 judge.py 判定。
 */
#include "coro.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <mutex>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#define MAX_CONNS 64
#define FRAME_MAX 4096

struct opts {
    const char *mode = "coro";
    int batch = 0;
    int idle_ms = 0;
    int delay_us = 0;
    int idle_exit_ms = 400;
    int max_run_ms = 20000;
    int eagain_as_eof = 0;
    int cancel_free = 0;
};

static struct opts g;

/* ---------- 连接统计 ---------- */
struct cstat {
    int id = -1;
    long frames = 0;
    long eof = 0;
    long resets = 0;
    long cancelled = 0;
    long err = 0;
    int last_errno = 0;
};

static cstat g_stats[MAX_CONNS];
static int g_nconns = 0;

/* 帧完成顺序（连接 id 序列）：用来量"一次 resume 里连做了几帧" */
static std::vector<int> g_order;
static std::mutex g_order_mu;

static void note_step(int id)
{
    std::lock_guard<std::mutex> lk(g_order_mu);

    g_order.push_back(id);
}

static int max_run(void)
{
    int best = 0, cur = 0, prev = -1;

    for (int id : g_order) {
        if (id == prev) {
            cur++;
        } else {
            prev = id;
            cur = 1;
        }
        if (cur > best)
            best = cur;
    }
    return best;
}

/* ---------- 工具 ---------- */
static long long now_ms(void)
{
    return coro::now_ms();
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
    if (g.delay_us <= 0)
        return;
    struct timespec ts;

    ts.tv_sec = g.delay_us / 1000000;
    ts.tv_nsec = (long)(g.delay_us % 1000000) * 1000L;
    nanosleep(&ts, NULL);
}

static void read_vm(long *vm_kb, long *rss_kb)
{
    FILE *f = fopen("/proc/self/status", "r");
    char line[256];

    *vm_kb = *rss_kb = -1;
    if (!f)
        return;
    while (fgets(line, sizeof line, f)) {
        long v;

        if (sscanf(line, "VmSize: %ld kB", &v) == 1)
            *vm_kb = v;
        else if (sscanf(line, "VmRSS: %ld kB", &v) == 1)
            *rss_kb = v;
    }
    fclose(f);
}

/* ---------- 分帧 ---------- */
/* 从缓冲头部解析一帧，返回帧总长（4 字节大端长度 + payload）；
 * 0 表示数据还不够，-1 表示长度非法。两种连接实现共用这一份判据。 */
static long frame_total(const char *buf, size_t need)
{
    const unsigned char *p = (const unsigned char *)buf;
    size_t len;

    if (need < 4)
        return 0;
    len = ((size_t)p[0] << 24) | ((size_t)p[1] << 16) | ((size_t)p[2] << 8) | p[3];
    if (len == 0 || len > FRAME_MAX)
        return -1;
    if (need < 4 + len)
        return 0;
    return (long)(4 + len);
}

/* ---------- 协程版连接 ---------- */
static coro::Task<int> conn_task(coro::Sched *s, int fd, cstat *st)
{
    char buf[FRAME_MAX + 4];
    size_t need = 0;
    int step = 0;

    for (;;) {
        /* 1) 先把用户缓冲里能切出的帧全部处理完。
         *    这一层不能"处理一帧就回去 recv"：数据可能已经全在自己缓冲里，
         *    此时内核没有可读事件，等 EPOLLIN 会一直等到客户端超时。 */
        for (;;) {
            long total = frame_total(buf, need);

            if (total < 0) {
                st->err++;
                co_return 0;
            }
            if (total == 0)
                break; /* 半帧：等下一次读 */
            st->frames++;
            note_step(st->id);
            work_delay();
            {
                size_t off = 0;

                while (off < (size_t)total) {
                    ssize_t w = send(fd, buf + off, (size_t)total - off, MSG_NOSIGNAL);

                    if (w > 0) {
                        off += (size_t)w;
                        continue;
                    }
                    if (w < 0 && errno == EINTR)
                        continue;
                    if (w < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                        int r = co_await s->wait(fd, EPOLLOUT);

                        if (r < 0) {
                            st->err++;
                            st->last_errno = -r;
                            co_return 0;
                        }
                        continue;
                    }
                    st->err++;
                    st->last_errno = errno;
                    co_return 0;
                }
            }
            memmove(buf, buf + total, need - (size_t)total);
            need -= (size_t)total;
            /* 2) 公平性：配额用完就主动让出一轮。
             *    走 resume_later 而不是等 epoll——剩下的帧就在自己缓冲里，
             *    LT+ONESHOT 只会在**内核**还有数据时重报，靠它是等不到的。 */
            if (g.batch > 0 && ++step >= g.batch) {
                step = 0;
                co_await s->yield_here(fd);
            }
        }

        /* 3) 缓冲里切不出完整帧了，才回内核读 */
        {
            ssize_t n = recv(fd, buf + need, sizeof buf - need, 0);

            if (n > 0) {
                need += (size_t)n;
                continue;
            }
            if (n == 0) {
                st->eof++;
                break;
            }
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if (g.eagain_as_eof) {
                    /* 破坏版：把"暂时没数据"当成"对端结束"。
                     * 请求-响应式客户端此时还没发下一帧，于是每条连接只回一帧。 */
                    break;
                }
                {
                    int r = co_await s->wait(fd, EPOLLIN);

                    if (r < 0) {
                        st->cancelled++;
                        st->last_errno = -r;
                        break;
                    }
                }
                continue;
            }
            if (errno == ECONNRESET) {
                st->resets++;
                break;
            }
            st->err++;
            st->last_errno = errno;
            break;
        }
    }
    co_return 0;
}

/* ---------- 线程版连接（对照） ---------- */
struct thread_arg {
    int fd;
    int id;
};

static void *thread_conn(void *arg)
{
    struct thread_arg *ta = (struct thread_arg *)arg;
    int fd = ta->fd;
    char buf[FRAME_MAX + 4];
    size_t need = 0;
    cstat *st = &g_stats[ta->id];

    for (;;) {
        ssize_t n = recv(fd, buf + need, sizeof buf - need, 0);

        if (n <= 0) {
            if (n == 0)
                st->eof++;
            else if (errno == ECONNRESET)
                st->resets++;
            else if (errno != EINTR)
                st->err++;
            break;
        }
        need += (size_t)n;
        for (;;) {
            long total = frame_total(buf, need);

            if (total < 0) {
                st->err++;
                goto out;
            }
            if (total == 0)
                break;
            work_delay();
            {
                size_t off = 0;

                while (off < (size_t)total) {
                    ssize_t w = send(fd, buf + off, (size_t)total - off, MSG_NOSIGNAL);

                    if (w > 0) {
                        off += (size_t)w;
                        continue;
                    }
                    if (w < 0 && errno == EINTR)
                        continue;
                    st->err++;
                    goto out;
                }
            }
            st->frames++;
            note_step(st->id);
            memmove(buf, buf + total, need - (size_t)total);
            need -= (size_t)total;
        }
    }
out:
    close(fd);
    return NULL;
}

/* ---------- accept ---------- */
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
    a.sin_port = 0;
    if (bind(fd, (struct sockaddr *)&a, sizeof a) < 0)
        goto fail;
    if (listen(fd, 128) < 0)
        goto fail;
    /* 监听 fd 必须非阻塞：accept 循环靠 EAGAIN 退出（阶段 4 的教训） */
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

int main(int argc, char **argv)
{
    coro::Sched s;
    pthread_t th[MAX_CONNS];
    struct thread_arg targs[MAX_CONNS];
    int lfd, port = 0, nthreads = 0;
    long long t0, last_activity;
    long accepted_total = 0;
    int iters = 0;
    long vm0 = 0, rss0 = 0, vm1 = 0, rss1 = 0;
    int use_coro;

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];

        if (strcmp(a, "--mode") == 0 && i + 1 < argc) {
            g.mode = argv[++i];
        } else if (strcmp(a, "--batch") == 0 && i + 1 < argc) {
            g.batch = atoi(argv[++i]);
        } else if (strcmp(a, "--idle-ms") == 0 && i + 1 < argc) {
            g.idle_ms = atoi(argv[++i]);
        } else if (strcmp(a, "--delay-us") == 0 && i + 1 < argc) {
            g.delay_us = atoi(argv[++i]);
        } else if (strcmp(a, "--idle-exit") == 0 && i + 1 < argc) {
            g.idle_exit_ms = atoi(argv[++i]);
        } else if (strcmp(a, "--max-run") == 0 && i + 1 < argc) {
            g.max_run_ms = atoi(argv[++i]);
        } else if (strcmp(a, "--eagain-as-eof") == 0) {
            g.eagain_as_eof = 1;
        } else if (strcmp(a, "--cancel-free") == 0) {
            g.cancel_free = 1;
        } else {
            pr("S05|srv|error=bad_args|arg=%s\n", a);
            return 2;
        }
    }
    use_coro = strcmp(g.mode, "coro") == 0;
    if (!use_coro && strcmp(g.mode, "thread") != 0) {
        pr("S05|srv|error=bad_mode\n");
        return 2;
    }

    read_vm(&vm0, &rss0);
    lfd = make_listen(&port);
    if (lfd < 0) {
        pr("S05|srv|error=listen_failed|errno=%d\n", errno);
        return 1;
    }
    pr("S05|srv|ready|port=%d|mode=%s|batch=%d|idle_ms=%d|delay_us=%d|eagain_as_eof=%d|"
       "cancel_free=%d\n",
       port, g.mode, g.batch, g.idle_ms, g.delay_us, g.eagain_as_eof, g.cancel_free);

    if (use_coro) {
        s.set_idle_ms(g.idle_ms);
        s.on_done = [&s](int fd, std::coroutine_handle<> h) {
            if (g.cancel_free) {
                /* 破坏版：销毁帧，但不 detach、不 close → 槽位仍指向已释放的帧。
                 * 下一次事件只能靠 destroyed 标记拦下来（真实代码这里是 UB）。 */
                s.note_destroyed(fd);
                h.destroy();
                return;
            }
            s.detach(fd);
            h.destroy();
            close(fd);
        };
    }

    t0 = now_ms();
    last_activity = t0;

    for (;;) {
        /* 收下所有排队的连接 */
        for (;;) {
            int cfd = accept(lfd, NULL, NULL);

            if (cfd < 0) {
                if (errno == EINTR)
                    continue;
                break;
            }
            if (g_nconns >= MAX_CONNS) {
                close(cfd);
                continue;
            }
            (void)set_nonblock(cfd);
            {
                int id = g_nconns++;

                g_stats[id].id = id;
                accepted_total++;
                last_activity = now_ms();
                if (use_coro) {
                    auto t = conn_task(&s, cfd, &g_stats[id]);
                    auto h = t.detach();

                    s.resume_later(cfd, h);
                } else {
                    /* 线程版用阻塞 fd：先清掉 O_NONBLOCK */
                    int fl = fcntl(cfd, F_GETFL, 0);

                    if (fl >= 0)
                        (void)fcntl(cfd, F_SETFL, fl & ~O_NONBLOCK);
                    targs[id].fd = cfd;
                    targs[id].id = id;
                    pthread_create(&th[nthreads++], NULL, thread_conn, &targs[id]);
                }
            }
        }

        if (use_coro) {
            int r = s.run_once(20);

            if (r < 0)
                break;
            iters++;
            if (r > 0)
                last_activity = now_ms();
            if (accepted_total > 0 && s.alive_slots() == 0 &&
                now_ms() - last_activity >= g.idle_exit_ms)
                break;
        } else {
            /* 线程版：等"多久没有新连接"就进入收尾 */
            struct timespec ts = { 0, 20 * 1000000L };

            nanosleep(&ts, NULL);
            if (accepted_total > 0 && now_ms() - last_activity >= g.idle_exit_ms)
                break;
        }
        if (now_ms() - t0 > g.max_run_ms)
            break;
    }

    /* 先量再收：线程版要在线程栈还在时读 VmSize，
     * join 之后 glibc 会把栈缓存起来，读数就不再代表"每连接一栈"的代价。 */
    read_vm(&vm1, &rss1);

    /* 收尾：不关监听 fd 之前的连接由各路径自行关闭 */
    for (int i = 0; i < nthreads; i++)
        pthread_join(th[i], NULL);

    {
        long frames = 0, eof = 0, resets = 0, cancelled = 0, errs = 0;

        for (int i = 0; i < g_nconns; i++) {
            frames += g_stats[i].frames;
            eof += g_stats[i].eof;
            resets += g_stats[i].resets;
            cancelled += g_stats[i].cancelled;
            errs += g_stats[i].err;
        }
        pr("S05|srv|done|mode=%s|batch=%d|accepted=%ld|conns=%d|frames=%ld|eof=%ld|resets=%ld|"
           "cancelled=%ld|errs=%ld|use_after_destroy=%ld|max_run=%d|order=%zu|iters=%d|"
           "threads=%d|vm0_kb=%ld|vm_kb=%ld|rss0_kb=%ld|rss_kb=%ld|alive_frames=%ld|serve_ms=%lld\n",
           g.mode, g.batch, accepted_total, g_nconns, frames, eof, resets, cancelled, errs,
           use_coro ? s.use_after_destroy() : 0L, max_run(), g_order.size(), iters, nthreads,
           vm0, vm1, rss0, rss1, coro::g_alive, now_ms() - t0);
    }
    close(lfd);
    return 0;
}
