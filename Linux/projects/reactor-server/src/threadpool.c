/*
 * threadpool.c —— 有界任务队列 + 固定 worker 线程
 *
 * 关闭协议（重要）：
 *   tp_shutdown(drain=1) 设置 shutting_down 后广播条件变量，worker 在队列空时退出；
 *   队列非空则继续跑完。这保证"在飞任务不会丢"，也保证 shutdown 一定能返回——
 *   若 worker 里有阻塞调用，shutdown 会永久挂住，因此 job_fn 内不得做阻塞 I/O。
 *
 * 计数口径：
 *   pending = 还在队列里的 + 正在跑的。调用方用它判断"是否可以安全关闭连接表"。
 */
#include "reactor.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define QUEUE_CAP 1024

typedef struct job_node {
    job_fn            fn;
    payload_t        *job;
    struct job_node  *next;
} job_node;

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  g_not_empty = PTHREAD_COND_INITIALIZER;
static pthread_cond_t  g_not_full  = PTHREAD_COND_INITIALIZER;

static job_node        *g_head;
static job_node        *g_tail;
static int              g_queued;
static int              g_running;
static int              g_shutting_down;
static int              g_nthreads;
static pthread_t       *g_tids;

static void *worker_main(void *arg)
{
    (void)arg;
    for (;;) {
        pthread_mutex_lock(&g_lock);
        while (g_head == NULL && !g_shutting_down) {
            pthread_cond_wait(&g_not_empty, &g_lock);
        }
        if (g_head == NULL && g_shutting_down) {
            pthread_mutex_unlock(&g_lock);
            return NULL;
        }
        job_node *node = g_head;
        g_head = node->next;
        if (g_head == NULL) {
            g_tail = NULL;
        }
        g_queued--;
        g_running++;
        pthread_cond_signal(&g_not_full);
        pthread_mutex_unlock(&g_lock);

        node->fn(node->job);
        free(node->job);
        free(node);

        pthread_mutex_lock(&g_lock);
        g_running--;
        if (g_queued == 0 && g_running == 0) {
            pthread_cond_broadcast(&g_not_empty); /* 让 shutdown 的等待者醒 */
        }
        pthread_mutex_unlock(&g_lock);
    }
}

int tp_start(int nthreads)
{
    if (nthreads < 1) {
        nthreads = 1;
    }
    if (nthreads > 64) {
        nthreads = 64;
    }
    if (g_tids != NULL) {
        return -1;
    }
    g_tids = calloc((size_t)nthreads, sizeof(*g_tids));
    if (g_tids == NULL) {
        return -1;
    }
    for (int i = 0; i < nthreads; i++) {
        if (pthread_create(&g_tids[i], NULL, worker_main, NULL) != 0) {
            /* 已启动的线程照常跑，shutting_down 后自然退出 */
            g_nthreads = i;
            fprintf(stderr, "SRV|error|what=tp_start|detail=thread_create_failed|n=%d\n", i);
            return -1;
        }
        g_nthreads++;
    }
    return 0;
}

int tp_submit(job_fn fn, payload_t *job)
{
    pthread_mutex_lock(&g_lock);
    if (g_shutting_down || g_queued >= QUEUE_CAP) {
        pthread_mutex_unlock(&g_lock);
        return -1;
    }
    job_node *node = malloc(sizeof(*node));
    if (node == NULL) {
        pthread_mutex_unlock(&g_lock);
        return -1;
    }
    node->fn = fn;
    node->job = job;
    node->next = NULL;
    if (g_tail != NULL) {
        g_tail->next = node;
    } else {
        g_head = node;
    }
    g_tail = node;
    g_queued++;
    pthread_cond_signal(&g_not_empty);
    pthread_mutex_unlock(&g_lock);
    return 0;
}

void tp_shutdown(int drain)
{
    pthread_mutex_lock(&g_lock);
    if (!drain) {
        /* 丢弃队列中的 job：调用方已放弃它们，释放以免泄漏 */
        job_node *node = g_head;
        while (node != NULL) {
            job_node *next = node->next;
            free(node->job);
            free(node);
            node = next;
        }
        g_head = g_tail = NULL;
        g_queued = 0;
    }
    g_shutting_down = 1;
    pthread_cond_broadcast(&g_not_empty);
    pthread_cond_broadcast(&g_not_full);
    pthread_mutex_unlock(&g_lock);

    for (int i = 0; i < g_nthreads; i++) {
        pthread_join(g_tids[i], NULL);
    }
    free(g_tids);
    g_tids = NULL;
    g_nthreads = 0;
}

int tp_pending(void)
{
    pthread_mutex_lock(&g_lock);
    int n = g_queued + g_running;
    pthread_mutex_unlock(&g_lock);
    return n;
}

int tp_threads(void)
{
    return g_nthreads;
}
