/* sani_tsan.c —— ThreadSanitizer：同一份代码，只换同步方式
 *
 *   race   两个线程对同一全局变量做非原子 ++（数据竞争）
 *   atomic 换成 atomic_fetch_add（配对修正）
 *   mutex  换成互斥锁（第二种配对修正）
 *
 * 三个用例的线程数、迭代次数、读写位置完全相同，差别只在同步手段，
 * 否则"报了/没报"就无法归因到竞争本身。
 */
#define _GNU_SOURCE
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ITERS 200000

static long g_plain;
static atomic_long g_atomic;
static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;
static long g_mutex;

static void *worker_plain(void *arg)
{
        (void)arg;
        for (int i = 0; i < ITERS; i++) g_plain++;
        return NULL;
}

static void *worker_atomic(void *arg)
{
        (void)arg;
        for (int i = 0; i < ITERS; i++) atomic_fetch_add(&g_atomic, 1);
        return NULL;
}

static void *worker_mutex(void *arg)
{
        (void)arg;
        for (int i = 0; i < ITERS; i++) {
                pthread_mutex_lock(&g_mu);
                g_mutex++;
                pthread_mutex_unlock(&g_mu);
        }
        return NULL;
}

int main(int argc, char **argv)
{
        const char *what = argc > 1 ? argv[1] : "race";
        void *(*fn)(void *) = worker_plain;
        long result = 0;
        long expect = 2L * ITERS;

        if (!strcmp(what, "race"))        fn = worker_plain;
        else if (!strcmp(what, "atomic")) fn = worker_atomic;
        else if (!strcmp(what, "mutex"))  fn = worker_mutex;
        else { fprintf(stderr, "unknown mode: %s\n", what); return 2; }

        pthread_t a, b;
        if (pthread_create(&a, NULL, fn, NULL) != 0) return 3;
        if (pthread_create(&b, NULL, fn, NULL) != 0) return 3;
        pthread_join(a, NULL);
        pthread_join(b, NULL);

        if (!strcmp(what, "race"))        result = g_plain;
        else if (!strcmp(what, "atomic")) result = atomic_load(&g_atomic);
        else                              result = g_mutex;

        printf("S06|tsan|case=%s|result=%ld|expect=%ld|lost=%ld\n",
               what, result, expect, expect - result);
        return 0;
}
