/* SYS_02 阶段 2 · E 组：线程池与优雅关闭
 *
 * 同一个线程池、同一批任务（200 个，每个耗时 1 ms），唯一差别是关闭协议：
 *   graceful   置 shutdown 后，工作线程把队列里剩下的任务做完再退出
 *   immediate  置 shutdown 后，工作线程立即退出，队列里的任务被丢弃
 *
 * 判据：
 *   graceful  executed == submitted 且 missing == 0 且 dup == 0
 *   immediate executed <  submitted   且 dup == 0（丢弃不等于重复执行）
 *   两者      joined == 4、fd_leak == 0（线程与文件描述符都要收干净）
 *
 * 为什么任务里要 sleep 1 ms：任务太短的话，提交还没结束工作线程就已经做完了，
 * "关闭时队列还有货"这个前提不成立，immediate 模式会退化成 graceful。
 * 让任务有确定的耗时，是让这个对照可复现的手段。
 */
#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define WORKERS 4
#define TASKS 200
#define QUEUE_CAP 256

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

static void sleep_ms(int ms)
{
    struct timespec ts = {ms / 1000, (long)(ms % 1000) * 1000000L};

    while (nanosleep(&ts, &ts) < 0 && errno == EINTR)
        ;
}

struct pool {
    pthread_mutex_t mu;
    pthread_cond_t has_work;
    int tasks[QUEUE_CAP];
    int head, tail, count;
    int shutdown;
    int graceful;
    int max_depth;
    long executed;
    int per_task[TASKS];
};

struct worker_arg {
    struct pool *p;
    int id;
};

static int count_fds(void)
{
    DIR *d = opendir("/proc/self/fd");
    struct dirent *e;
    int n = 0;

    if (!d)
        return -1;
    while ((e = readdir(d)) != NULL)
        if (e->d_name[0] != '.')
            n++;
    closedir(d);
    return n;
}

static void *worker(void *arg)
{
    struct worker_arg *w = arg;
    struct pool *p = w->p;

    for (;;) {
        int task;

        pthread_mutex_lock(&p->mu);
        while (p->count == 0 && !p->shutdown)
            pthread_cond_wait(&p->has_work, &p->mu);

        if (p->shutdown && (!p->graceful || p->count == 0)) {
            pthread_mutex_unlock(&p->mu);
            break; /* 立即关闭：不等队列；优雅关闭：队列空了才走 */
        }
        task = p->tasks[p->head];
        p->head = (p->head + 1) % QUEUE_CAP;
        p->count--;
        pthread_mutex_unlock(&p->mu);

        sleep_ms(1); /* 模拟任务耗时 */
        pthread_mutex_lock(&p->mu);
        p->per_task[task]++;
        p->executed++;
        pthread_mutex_unlock(&p->mu);
    }
    return NULL;
}

static void run(int graceful)
{
    struct pool p;
    pthread_t th[WORKERS];
    struct worker_arg args[WORKERS];
    int fd_before = count_fds();
    int fd_after;
    int missing = 0, dup = 0, joined = 0;

    memset(&p, 0, sizeof p);
    p.graceful = graceful;
    pthread_mutex_init(&p.mu, NULL);
    pthread_cond_init(&p.has_work, NULL);

    for (int i = 0; i < WORKERS; i++) {
        args[i].p = &p;
        args[i].id = i;
        pthread_create(&th[i], NULL, worker, &args[i]);
    }

    pthread_mutex_lock(&p.mu);
    for (int i = 0; i < TASKS; i++) {
        p.tasks[p.tail] = i;
        p.tail = (p.tail + 1) % QUEUE_CAP;
        p.count++;
        if (p.count > p.max_depth)
            p.max_depth = p.count;
    }
    pthread_cond_broadcast(&p.has_work);
    pthread_mutex_unlock(&p.mu);

    /* 立刻关闭：此时队列里还有大量任务，两种协议的差别才会显现 */
    pthread_mutex_lock(&p.mu);
    p.shutdown = 1;
    pthread_cond_broadcast(&p.has_work);
    pthread_mutex_unlock(&p.mu);

    for (int i = 0; i < WORKERS; i++)
        if (pthread_join(th[i], NULL) == 0)
            joined++;

    for (int i = 0; i < TASKS; i++) {
        if (p.per_task[i] == 0)
            missing++;
        else if (p.per_task[i] > 1)
            dup++;
    }
    fd_after = count_fds();

    pr("SYS02|pool|mode=%s|workers=%d|submitted=%d|executed=%ld|missing=%d|dup=%d|"
       "joined=%d|max_depth=%d|fd_before=%d|fd_after=%d|fd_leak=%d|ok=%d\n",
       graceful ? "graceful" : "immediate", WORKERS, TASKS, p.executed, missing, dup, joined,
       p.max_depth, fd_before, fd_after, fd_after - fd_before,
       (joined == WORKERS && dup == 0 && fd_after == fd_before &&
        (graceful ? (p.executed == TASKS && missing == 0) : (p.executed < TASKS)))
           ? 1
           : 0);

    pthread_cond_destroy(&p.has_work);
    pthread_mutex_destroy(&p.mu);
}

int main(int argc, char **argv)
{
    const char *mode = argc > 1 ? argv[1] : "graceful";

    if (strcmp(mode, "graceful") == 0)
        run(1);
    else if (strcmp(mode, "immediate") == 0)
        run(0);
    else {
        pr("用法: threadpool <graceful|immediate>\n");
        return 2;
    }
    pr("SYS02|pool|done|mode=%s\n", mode);
    return 0;
}
