/* SYS_02 阶段 2 · C 组：数据竞争与读写锁
 *
 * 三种模式的对照（同一个计数器，唯一差别是保护方式）：
 *   plain   无保护：读-改-写之间插 sched_yield，丢更新必然发生
 *   mutex   互斥锁：结果正确
 *   atomic  _Atomic long + fetch_add：结果正确
 *
 * 另两种模式的对照（同一个"两个字段必须相等"的不变量）：
 *   rwlock        写者持写锁更新两个字段、读者持读锁检查 → 不变量恒成立
 *   rwlock-broken 写者不持锁，在两个字段之间 yield → 读者必然看到撕裂
 *
 * sched_yield 在这里是**故障注入**：把"读到旧值 → 写回新值"之间的窗口放大到必现，
 * 否则丢更新是概率性的（阶段 1 的结论同样适用）。
 *
 * TSan 检出用法见 Makefile 的 `tsan` 目标：同一份源码加 -fsanitize=thread 重编。
 */
#define _GNU_SOURCE
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define THREADS 4
#define DEFAULT_ITERS 20000

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

/* ---------- 计数器：plain / mutex / atomic ---------- */

static long counter_plain;      /* 无保护 */
static long counter_mutex;      /* 由 counter_mu 保护 */
static pthread_mutex_t counter_mu = PTHREAD_MUTEX_INITIALIZER;
static _Atomic long counter_atomic;

struct plain_arg {
    int iters;
};

static void *plain_worker(void *arg)
{
    struct plain_arg *a = arg;

    for (int i = 0; i < a->iters; i++) {
        long v = counter_plain; /* load */

        sched_yield(); /* 放大窗口：别的线程也读到同一个旧值 */
        counter_plain = v + 1; /* store */
    }
    return NULL;
}

static void *mutex_worker(void *arg)
{
    struct plain_arg *a = arg;

    for (int i = 0; i < a->iters; i++) {
        pthread_mutex_lock(&counter_mu);
        counter_mutex++;
        pthread_mutex_unlock(&counter_mu);
    }
    return NULL;
}

static void *atomic_worker(void *arg)
{
    struct plain_arg *a = arg;

    for (int i = 0; i < a->iters; i++)
        atomic_fetch_add_explicit(&counter_atomic, 1, memory_order_relaxed);
    return NULL;
}

static void case_counter(const char *mode, int iters)
{
    pthread_t th[THREADS];
    struct plain_arg a = {iters};
    long final;
    long expected = (long)THREADS * iters;

    if (strcmp(mode, "plain") == 0) {
        counter_plain = 0;
        for (int i = 0; i < THREADS; i++)
            pthread_create(&th[i], NULL, plain_worker, &a);
        for (int i = 0; i < THREADS; i++)
            pthread_join(th[i], NULL);
        final = counter_plain;
    } else if (strcmp(mode, "mutex") == 0) {
        counter_mutex = 0;
        for (int i = 0; i < THREADS; i++)
            pthread_create(&th[i], NULL, mutex_worker, &a);
        for (int i = 0; i < THREADS; i++)
            pthread_join(th[i], NULL);
        final = counter_mutex;
    } else {
        atomic_store_explicit(&counter_atomic, 0, memory_order_relaxed);
        for (int i = 0; i < THREADS; i++)
            pthread_create(&th[i], NULL, atomic_worker, &a);
        for (int i = 0; i < THREADS; i++)
            pthread_join(th[i], NULL);
        final = atomic_load_explicit(&counter_atomic, memory_order_relaxed);
    }

    pr("SYS02|race|mode=%s|threads=%d|iters=%d|expected=%ld|final=%ld|lost=%ld\n", mode,
       THREADS, iters, expected, final, expected - final);
}

/* ---------- 不变量：a 必须恒等于 b ---------- */

struct pair_state {
    long a;
    long b;
};

static struct pair_state pair_state;
static pthread_rwlock_t pair_lock = PTHREAD_RWLOCK_INITIALIZER;
static _Atomic int torn;
static _Atomic int stop_readers;

static void *pair_writer(void *arg)
{
    int broken = *(int *)arg;

    for (long k = 1; k <= 20000; k++) {
        if (broken) {
            pair_state.a = k;
            sched_yield(); /* 撕裂窗口：读者可能正好落在这里 */
            pair_state.b = k;
        } else {
            pthread_rwlock_wrlock(&pair_lock);
            pair_state.a = k;
            pair_state.b = k;
            pthread_rwlock_unlock(&pair_lock);
        }
    }
    atomic_store(&stop_readers, 1);
    return NULL;
}

static void *pair_reader(void *arg)
{
    (void)arg;
    while (!atomic_load(&stop_readers)) {
        long a, b;

        if (atomic_load(&stop_readers))
            break;
        pthread_rwlock_rdlock(&pair_lock);
        a = pair_state.a;
        b = pair_state.b;
        pthread_rwlock_unlock(&pair_lock);
        if (a != b)
            atomic_fetch_add(&torn, 1);
    }
    return NULL;
}

static void case_pair(int broken)
{
    pthread_t wr, rd[3];
    int b = broken;

    pair_state.a = pair_state.b = 0;
    atomic_store(&torn, 0);
    atomic_store(&stop_readers, 0);

    for (int i = 0; i < 3; i++)
        pthread_create(&rd[i], NULL, pair_reader, NULL);
    pthread_create(&wr, NULL, pair_writer, &b);
    pthread_join(wr, NULL);
    for (int i = 0; i < 3; i++)
        pthread_join(rd[i], NULL);

    pr("SYS02|rwlock|mode=%s|readers=3|writer_updates=20000|torn=%d|safe=%d\n",
       broken ? "rwlock-broken" : "rwlock", atomic_load(&torn),
       atomic_load(&torn) == 0 ? 1 : 0);
}

int main(int argc, char **argv)
{
    const char *mode = argc > 1 ? argv[1] : "plain";
    int iters = DEFAULT_ITERS;

    for (int i = 2; i + 1 < argc; i += 2)
        if (strcmp(argv[i], "--iters") == 0)
            iters = atoi(argv[i + 1]);

    if (strcmp(mode, "plain") == 0 || strcmp(mode, "mutex") == 0 ||
        strcmp(mode, "atomic") == 0) {
        case_counter(mode, iters);
    } else if (strcmp(mode, "rwlock") == 0) {
        case_pair(0);
    } else if (strcmp(mode, "rwlock-broken") == 0) {
        case_pair(1);
    } else {
        pr("用法: race_detect <plain|mutex|atomic|rwlock|rwlock-broken> [--iters N]\n");
        return 2;
    }
    pr("SYS02|race|done|mode=%s\n", mode);
    return 0;
}
