/* SYS_02 阶段 2 · B 组：条件变量
 *
 * 两件事放在一个程序里，因为它们共用同一套 mutex + cond：
 *   spurious-if / spurious-while  虚假唤醒：`if` 检查 vs `while` 检查
 *   queue                          有界队列的生产者消费者，核对不变量
 *
 * 虚假唤醒不是"理论上的可能"，可以人为构造：在等待者已经阻塞时广播一次
 * 而不改变条件，看它醒来之后做什么。`if` 版本会直接往下走，
 * `while` 版本会重新检查并继续等待。
 *
 * 怎么保证"广播时对方真的在等"：
 * 主线程持锁时无法区分"等待者正在 cond_wait（已释放锁）"与"等待者还没走到 cond_wait
 * （正卡在抢锁）"——两种情况下 waiters 都等于 1。所以不用一次性握手，
 * 而是循环广播，直到等待者自己记下"我被无理由唤醒过一次"（spurious_hits）为止。
 * 这个循环有上界，不会挂死。
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

#define QUEUE_CAP 8
#define PRODUCERS 2
#define CONSUMERS 2
#define ITEMS_PER_PRODUCER 500
#define TOTAL_ITEMS (PRODUCERS * ITEMS_PER_PRODUCER)

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

/* ---------- 虚假唤醒 ---------- */

struct spurious_ctx {
    pthread_mutex_t mu;
    pthread_cond_t cv;
    int flag;          /* 真正的条件 */
    int waiters;
    int spurious_hits; /* 被"无理由"唤醒的次数 */
    int flag_at_exit;  /* 离开等待循环时 flag 的值 */
    int use_while;     /* 1 = while 检查，0 = if 检查 */
};

static void *spurious_waiter(void *arg)
{
    struct spurious_ctx *c = arg;

    pthread_mutex_lock(&c->mu);
    c->waiters++;
    if (c->use_while) {
        while (!c->flag) {
            pthread_cond_wait(&c->cv, &c->mu);
            if (!c->flag)
                c->spurious_hits++; /* 醒了但条件不成立 = 虚假唤醒 */
        }
    } else {
        if (!c->flag)
            pthread_cond_wait(&c->cv, &c->mu);
        if (!c->flag)
            c->spurious_hits++; /* if 版本不会重新等，直接往下走 */
    }
    c->flag_at_exit = c->flag;
    c->waiters--;
    pthread_mutex_unlock(&c->mu);
    return NULL;
}

static void case_spurious(int use_while)
{
    struct spurious_ctx c;
    pthread_t th;
    int rounds = 0;
    const int max_rounds = 100000;

    memset(&c, 0, sizeof c);
    c.use_while = use_while;
    pthread_mutex_init(&c.mu, NULL);
    pthread_cond_init(&c.cv, NULL);

    pthread_create(&th, NULL, spurious_waiter, &c);

    /* 反复广播，直到等待者报告"被无理由唤醒过一次" */
    for (rounds = 0; rounds < max_rounds; rounds++) {
        pthread_mutex_lock(&c.mu);
        if (c.spurious_hits >= 1) {
            pthread_mutex_unlock(&c.mu);
            break;
        }
        pthread_cond_broadcast(&c.cv);
        pthread_mutex_unlock(&c.mu);
        sched_yield();
    }

    /* 再给一个真实的唤醒，保证两种版本都能退出 */
    pthread_mutex_lock(&c.mu);
    c.flag = 1;
    pthread_cond_signal(&c.cv);
    pthread_mutex_unlock(&c.mu);

    pthread_join(th, NULL);

    pr("SYS02|cond|spurious|check=%s|rounds=%d|hits=%d|flag_at_exit=%d|safe=%d\n",
       use_while ? "while" : "if", rounds, c.spurious_hits, c.flag_at_exit,
       (c.flag_at_exit == 1) ? 1 : 0);

    pthread_cond_destroy(&c.cv);
    pthread_mutex_destroy(&c.mu);
}

/* ---------- 有界队列 ---------- */

struct queue_ctx {
    pthread_mutex_t mu;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
    int buf[QUEUE_CAP];
    int head, tail, count;
    int max_depth;
    int shutdown;
    long produced;
    long consumed;
    int executed[TOTAL_ITEMS]; /* 每个任务被执行了几次，用来查重复 */
    long stale_wake;           /* 醒来时条件已不成立的次数（不是虚假唤醒，见 queue_pop） */
};

static void queue_push(struct queue_ctx *q, int v)
{
    pthread_mutex_lock(&q->mu);
    while (q->count == QUEUE_CAP)
        pthread_cond_wait(&q->not_full, &q->mu);
    q->buf[q->tail] = v;
    q->tail = (q->tail + 1) % QUEUE_CAP;
    q->count++;
    if (q->count > q->max_depth)
        q->max_depth = q->count;
    q->produced++;
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->mu);
}

/* 返回 0 取到、-1 关闭且队列已空 */
static int queue_pop(struct queue_ctx *q, int *out)
{
    int rc;

    pthread_mutex_lock(&q->mu);
    while (q->count == 0) {
        if (q->shutdown) {
            pthread_mutex_unlock(&q->mu);
            return -1;
        }
        pthread_cond_wait(&q->not_empty, &q->mu);
        /* 醒来后条件可能已经被另一个消费者改掉——这不是虚假唤醒，
         * 而是"被唤醒"与"拿到锁"之间别人先一步取走了唯一一件货。
         * 只要用 while 重新检查就安全；把它记下来是为了说明
         * **即使没有任何虚假唤醒，也必须用 while**。 */
        if (q->count == 0)
            q->stale_wake++;
    }
    *out = q->buf[q->head];
    q->head = (q->head + 1) % QUEUE_CAP;
    q->count--;
    q->consumed++;
    rc = 0;
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->mu);
    return rc;
}

struct worker_arg {
    struct queue_ctx *q;
    int id;
};

static void *producer(void *arg)
{
    struct worker_arg *w = arg;

    for (int i = 0; i < ITEMS_PER_PRODUCER; i++)
        queue_push(w->q, w->id * ITEMS_PER_PRODUCER + i);
    return NULL;
}

static void *consumer(void *arg)
{
    struct worker_arg *w = arg;
    int v;

    while (queue_pop(w->q, &v) == 0) {
        pthread_mutex_lock(&w->q->mu);
        w->q->executed[v]++;
        pthread_mutex_unlock(&w->q->mu);
    }
    return NULL;
}

static void case_queue(void)
{
    struct queue_ctx q;
    pthread_t prod[PRODUCERS], cons[CONSUMERS];
    struct worker_arg parg[PRODUCERS], carg[CONSUMERS];
    int dup = 0, missing = 0;

    memset(&q, 0, sizeof q);
    pthread_mutex_init(&q.mu, NULL);
    pthread_cond_init(&q.not_full, NULL);
    pthread_cond_init(&q.not_empty, NULL);

    for (int i = 0; i < CONSUMERS; i++) {
        carg[i].q = &q;
        carg[i].id = i;
        pthread_create(&cons[i], NULL, consumer, &carg[i]);
    }
    for (int i = 0; i < PRODUCERS; i++) {
        parg[i].q = &q;
        parg[i].id = i;
        pthread_create(&prod[i], NULL, producer, &parg[i]);
    }
    for (int i = 0; i < PRODUCERS; i++)
        pthread_join(prod[i], NULL);

    pthread_mutex_lock(&q.mu);
    q.shutdown = 1;
    pthread_cond_broadcast(&q.not_empty);
    pthread_mutex_unlock(&q.mu);

    for (int i = 0; i < CONSUMERS; i++)
        pthread_join(cons[i], NULL);

    for (int i = 0; i < TOTAL_ITEMS; i++) {
        if (q.executed[i] == 0)
            missing++;
        else if (q.executed[i] > 1)
            dup++;
    }

    pr("SYS02|cond|queue|producers=%d|consumers=%d|cap=%d|produced=%ld|consumed=%ld|"
       "max_depth=%d|stale_wake=%ld|missing=%d|dup=%d|ok=%d\n",
       PRODUCERS, CONSUMERS, QUEUE_CAP, q.produced, q.consumed, q.max_depth, q.stale_wake,
       missing, dup,
       (q.produced == TOTAL_ITEMS && q.consumed == TOTAL_ITEMS && q.max_depth <= QUEUE_CAP &&
        missing == 0 && dup == 0)
           ? 1
           : 0);

    pthread_cond_destroy(&q.not_full);
    pthread_cond_destroy(&q.not_empty);
    pthread_mutex_destroy(&q.mu);
}

int main(int argc, char **argv)
{
    const char *mode = argc > 1 ? argv[1] : "queue";

    if (strcmp(mode, "spurious-if") == 0)
        case_spurious(0);
    else if (strcmp(mode, "spurious-while") == 0)
        case_spurious(1);
    else if (strcmp(mode, "queue") == 0)
        case_queue();
    else {
        pr("用法: condvar <queue|spurious-if|spurious-while>\n");
        return 2;
    }
    pr("SYS02|cond|done|mode=%s\n", mode);
    return 0;
}
