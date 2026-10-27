/* SYS_02 阶段 2 · D 组：死锁与加锁顺序
 *
 * 同一个骨架、同一对互斥量，唯一差别是加锁顺序：
 *   deadlock  T1 先 A 后 B，T2 先 B 后 A → 互相等对方手里的锁，两个都超时
 *   ordered   两个线程都先 A 后 B       → 串行通过，没有超时
 *
 * 用 pthread_mutex_timedlock 而不是 pthread_mutex_lock：
 * 真的死锁会让进程挂住，既不能写进日志也无法机器判定。
 * 超时把"死锁"从一个"卡死现象"变成"可观测的 ETIMEDOUT(110)"。
 *
 * barrier 用来把交错固定下来：两个线程都拿到各自的第一个锁之后才继续抢第二个，
 * 否则可能一个线程跑完了另一个还没开始，测不出死锁。
 * ordered 模式下 T2 会阻塞在第一个锁上，所以那一组不使用 barrier
 * （barrier 需要两个线程都到达，T2 到不了）。
 */
#define _GNU_SOURCE
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define HOLD_MS 2000

static void pr(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

static void pr(const char *fmt, ...)
{
    char buf[512];
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (n > 0) {
        size_t len = (size_t)n < sizeof buf ? (size_t)n : sizeof buf - 1;

        /* 必须消费返回值：glibc 把 write 标成 warn_unused_result */
        if (write(STDOUT_FILENO, buf, len) < 0) {
            /* 取证行都写不出去时没有更合适的补救 */
        }
    }
}

struct ctx {
    pthread_mutex_t a;
    pthread_mutex_t b;
    pthread_barrier_t bar;
    int ordered;
    int rc1;
    int rc2;
    int t1_progress; /* 1 = T1 越过第一次加锁 */
    int t2_progress;
};

static void deadline(struct timespec *ts, int ms)
{
    clock_gettime(CLOCK_REALTIME, ts);
    ts->tv_sec += ms / 1000;
    ts->tv_nsec += (long)(ms % 1000) * 1000000L;
    if (ts->tv_nsec >= 1000000000L) {
        ts->tv_sec++;
        ts->tv_nsec -= 1000000000L;
    }
}

/* T1：A → B */
static void *t1(void *p)
{
    struct ctx *c = p;
    struct timespec ts;
    int rc;

    pthread_mutex_lock(&c->a);
    c->t1_progress = 1;
    if (!c->ordered)
        pthread_barrier_wait(&c->bar);
    deadline(&ts, HOLD_MS);
    rc = pthread_mutex_timedlock(&c->b, &ts);
    c->rc1 = rc;
    if (rc == 0)
        pthread_mutex_unlock(&c->b);
    pthread_mutex_unlock(&c->a);
    return NULL;
}

/* T2：死锁组先 B 后 A；有序组先 A 后 B */
static void *t2(void *p)
{
    struct ctx *c = p;
    struct timespec ts;
    int rc;

    if (c->ordered) {
        pthread_mutex_lock(&c->a);
        c->t2_progress = 1;
        deadline(&ts, HOLD_MS);
        rc = pthread_mutex_timedlock(&c->b, &ts);
        c->rc2 = rc;
        if (rc == 0)
            pthread_mutex_unlock(&c->b);
        pthread_mutex_unlock(&c->a);
        return NULL;
    }

    pthread_mutex_lock(&c->b);
    c->t2_progress = 1;
    pthread_barrier_wait(&c->bar);
    deadline(&ts, HOLD_MS);
    rc = pthread_mutex_timedlock(&c->a, &ts);
    c->rc2 = rc;
    if (rc == 0)
        pthread_mutex_unlock(&c->a);
    pthread_mutex_unlock(&c->b);
    return NULL;
}

static void run(int ordered)
{
    struct ctx c;
    pthread_t th1, th2;

    memset(&c, 0, sizeof c);
    c.ordered = ordered;
    pthread_mutex_init(&c.a, NULL);
    pthread_mutex_init(&c.b, NULL);
    if (!ordered)
        pthread_barrier_init(&c.bar, NULL, 2);

    pthread_create(&th1, NULL, t1, &c);
    pthread_create(&th2, NULL, t2, &c);
    pthread_join(th1, NULL);
    pthread_join(th2, NULL);

    pr("SYS02|deadlock|mode=%s|t1_rc=%d|t2_rc=%d|t1_progress=%d|t2_progress=%d|"
       "detected=%d|safe=%d\n",
       ordered ? "ordered" : "deadlock", c.rc1, c.rc2, c.t1_progress, c.t2_progress,
       (c.rc1 == ETIMEDOUT && c.rc2 == ETIMEDOUT) ? 1 : 0,
       (c.rc1 == 0 && c.rc2 == 0) ? 1 : 0);

    if (!ordered)
        pthread_barrier_destroy(&c.bar);
    pthread_mutex_destroy(&c.b);
    pthread_mutex_destroy(&c.a);
}

int main(int argc, char **argv)
{
    const char *mode = argc > 1 ? argv[1] : "deadlock";

    if (strcmp(mode, "deadlock") == 0)
        run(0);
    else if (strcmp(mode, "ordered") == 0)
        run(1);
    else {
        pr("用法: deadlock <deadlock|ordered>\n");
        return 2;
    }
    pr("SYS02|deadlock|done|mode=%s\n", mode);
    return 0;
}
