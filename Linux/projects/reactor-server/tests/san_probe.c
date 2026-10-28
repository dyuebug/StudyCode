/*
 * san_probe.c —— sanitizer 生效性探针
 *
 * 存在的理由：`make check-asan` 全绿有两种解释——"代码干净"和"工具没插桩"。
 * 这两者在日志上完全一样，正是本项目反复记录过的假绿形态（见 README §7）。
 * 这个探针故意写一个已知缺陷，目标**必须失败**：
 *   通过 = 工具没生效 = check-asan / check-tsan 的结论无效。
 *
 *   -DPROBE_ASAN   → 释放后读（ASan 必报）
 *   -DPROBE_UBSAN  → 有符号整数溢出（UBSan 必报）
 *   默认（TSan）   → 两个线程无同步地写同一个 int（TSan 必报）
 *
 * 由 `make probe-asan` / `make probe-tsan` / `make probe-ubsan` 编译并断言"非零退出"。
 */
#include <pthread.h>
#include <stdlib.h>

#ifdef PROBE_ASAN

/* 用 volatile 指针**在释放后读**，而不是 double free：
 * 首版写的是 `free(p); free(p);`，-O1 下 gcc 直接把它优化掉
 * （double free 是 UB，编译器可以假定不发生），探针于是"未报错"——
 * 探针本身也得先证明它没被优化掉。volatile 访问删不掉。 */
int main(void)
{
    volatile char *p = malloc(8);
    p[0] = 1;
    free((void *)p);
    return p[0]; /* 已知缺陷：heap-use-after-free */
}

#elif defined(PROBE_UBSAN)

/* 有符号溢出是 UB，-O1 下可能被常量折叠成"不会发生"。
 * 用 volatile 让操作数在运行时才取值，保证缺陷真的执行。
 * 正常运行时返回 1；UBSan（配 -fno-sanitize-recover=all）先报错并中止。 */
int main(void)
{
    volatile int x = 0x7fffffff;
    int y = x + 1; /* 已知缺陷：signed integer overflow */
    return y == 0;
}

#else

static int g_shared;

static void *worker(void *arg)
{
    (void)arg;
    for (int i = 0; i < 100000; i++) {
        g_shared++; /* 已知缺陷：无同步的自增 */
    }
    return NULL;
}

int main(void)
{
    pthread_t a, b;
    pthread_create(&a, NULL, worker, NULL);
    pthread_create(&b, NULL, worker, NULL);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    return 0;
}

#endif
