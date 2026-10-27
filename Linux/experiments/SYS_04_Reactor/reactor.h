/* SYS_04 · Reactor 内核
 *
 * 这一层只做三件事，别的都不做：
 *   1. 维护 epoll 注册（LT / ET / EPOLLONESHOT 三种模式由开关控制）
 *   2. 维护两条缓冲区（读、写）并把非阻塞 I/O 的短读短写挡在业务之外
 *   3. 在状态变化时调用回调，绝不替业务决定"怎么处理数据"
 *
 * 关键约束：**一个 conn 只归一个线程所有**。跨线程的数据必须通过
 * reactor 提供的唤醒通道交回来，不能在别的线程里直接改 conn 的缓冲区。
 * 这条约束在 server.c 的线程池模式里被真实使用。
 */
#ifndef REACTOR_H
#define REACTOR_H

#include <stddef.h>
#include <stdint.h>

/* 读写缓冲都是定长的：实验里单帧很小，定长让"溢出"成为显式判据，
 * 而不是悄悄 realloc 掩盖内存增长。 */
#define RBUF_SIZE 8192
#define WBUF_SIZE 65536

struct conn {
    int fd;
    int id;
    uint32_t reg;          /* 已注册的事件位（只会有 EPOLLIN / EPOLLOUT） */
    char rbuf[RBUF_SIZE];
    size_t rlen;
    char wbuf[WBUF_SIZE];
    size_t wlen;
    int closing;           /* 读到 EOF：剩余写数据发完就关 */
    int busy;              /* 业务层正在处理（线程池模式下置位） */
    void *ctx;
};

struct reactor;

/* 返回 0 继续；-1 关闭连接 */
typedef int (*frame_cb)(struct reactor *r, struct conn *c);
typedef void (*close_cb)(struct reactor *r, struct conn *c);
typedef int (*accept_cb)(struct reactor *r, int fd);

enum reactor_flags {
    R_ET = 1 << 0,          /* 边沿触发 */
    R_ONESHOT = 1 << 1,     /* EPOLLONESHOT */
    R_ONESHOT_BROKEN = 1 << 2, /* 一次性但**不重注册**：用来证伪 */
    R_ET_BROKEN = 1 << 3,   /* ET 但每次只读一次：用来证伪 */
};

struct reactor *reactor_new(int flags, frame_cb on_frame, close_cb on_close);
void reactor_free(struct reactor *r);

/* 注册监听 fd；`on_accept` 拿到裸 fd，是否收下由业务决定 */
int reactor_add_listen(struct reactor *r, int lfd, accept_cb on_accept);

/* 注册"唤醒 fd"（eventfd / pipe 读端）：内核说它可读时调用 `on_notify`。
 * 线程池与主从模式都靠它把别的线程的结果交回 reactor 线程——
 * 跨线程只允许动这个 fd，不允许动 conn 的缓冲区。 */
typedef void (*notify_cb)(struct reactor *r);
int reactor_add_notify(struct reactor *r, int fd, notify_cb on_notify);

/* 把一个已经就绪的 fd 交给 reactor 管理（所有权随之转移） */
int reactor_add_conn(struct reactor *r, struct conn *c);

/* 事件循环：处理一轮后返回处理的事件数；`timeout_ms < 0` 表示一直阻塞 */
int reactor_run_once(struct reactor *r, int timeout_ms);
void reactor_stop(struct reactor *r);

/* 追加待发数据并尽量立刻发出；返回 -1 表示连接已不可用 */
int conn_send(struct reactor *r, struct conn *c, const void *data, size_t n);

/* 声明"不再读，写完剩余数据后关闭" */
void conn_shutdown_after_write(struct reactor *r, struct conn *c);

/* 立刻关闭并回收 */
void conn_close(struct reactor *r, struct conn *c);

/* 业务处理完后主动再跑一次分帧（线程池模式在 worker 交回结果后调用） */
int reactor_kick(struct reactor *r, struct conn *c);

/* 读侧：把内核缓冲里的数据读空。
 * LT 模式读一次就够（没读完下次还会通知）；
 * ET 模式必须循环到 EAGAIN（`et_broken` 故意只读一次）。 */
int conn_drain(struct reactor *r, struct conn *c);

/* 统计：全部由 reactor 自己记，业务代码不需要关心 */
struct reactor_stats {
    long accepted;      /* 累计 accept 的连接数 */
    long closed;        /* 累计关闭的连接数 */
    long frames;        /* 业务回调报告完成的帧数（由业务累加） */
    long iters;         /* epoll_wait 被调用且返回 >0 的次数 */
    long buf_drops;     /* 写缓冲不足导致丢弃的次数 */
    int active;         /* 当前活跃连接 */
    int peak;           /* 峰值并发 */
};

const struct reactor_stats *reactor_stats(const struct reactor *r);
void reactor_note_frame(struct reactor *r);

#endif /* REACTOR_H */
