/*
 * reactor.h —— epoll ET + 线程池的最小可运行服务器：公共定义
 *
 * 平台：Linux（epoll / accept4 / eventfd），C11，gcc 15.2.0 实测。
 * 约束：单线程做全部 I/O，线程池只做业务计算，结果经完成队列回投主线程。
 *       这样连接结构只被主线程访问，不存在跨线程共享可变状态。
 */
#ifndef REACTOR_H
#define REACTOR_H

#include <stddef.h>
#include <stdint.h>

#define MAX_CONNS   1024      /* 同时在册连接上限，id 即槽位号 + 世代 */
#define MAX_FRAME   8192      /* 单帧 payload 上限，超出按协议错误断开 */

/* ---------- 任务队列（job） ---------- */

/* job 与 result 用同一种载荷：连接 id + 一段字节。
 * worker 不持有连接指针——连接可能已经被主线程关闭，用 id 匹配才能丢掉过期结果。 */
typedef struct {
    uint64_t conn_id;
    uint32_t len;
    uint8_t  data[MAX_FRAME];
} payload_t;

typedef void (*job_fn)(payload_t *job);

/* ---------- 线程池 ---------- */

/* 返回 0 成功；-1 失败（线程创建失败或已经启动）。 */
int  tp_start(int nthreads);
/* 投递一个 job。返回 0 成功；-1 表示队列满或已关闭（调用方负责释放 job）。 */
int  tp_submit(job_fn fn, payload_t *job);
/* drain=1：停止接受新 job 并把已有 job 跑完；drain=0：尽量快退出。 */
void tp_shutdown(int drain);
int  tp_pending(void);
int  tp_threads(void);

/* ---------- 服务端（server.c 提供给 main.c） ---------- */

struct srv_opts {
    uint16_t port;        /* 0 = 由内核分配，实际端口打进 SRV|listen */
    int      idle_ms;     /* 连接空闲超时，0 = 不超时 */
    int      threads;     /* worker 线程数 */
    int      max_conns;   /* 上限于 MAX_CONNS */
};

/* 返回进程退出码。内部完成：建 epoll / listen / 信号自管道 / 优雅退出。 */
int srv_run(const struct srv_opts *opts);

#endif /* REACTOR_H */
