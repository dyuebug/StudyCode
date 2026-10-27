/* sani_asan.c —— AddressSanitizer 四种缺陷 + 一个干净基线
 *
 * 每个缺陷都必须真的被 ASan 报出来，否则本实验失败。
 * 注意 volatile：首版写成 `p[8] = 42;` 且之后再没读过那块内存，
 * -O1 把它当死存储删掉了，ASan 看到的是一个没有越界的程序（rc=0）。
 * 这条教训写进 README §8.1——"工具没报"和"缺陷不存在"是两件事。
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

__attribute__((noinline)) static int deep(int n)
{
        volatile char pad[512];
        pad[0] = (char)n;
        if (n <= 0) return pad[0];
        return deep(n - 1) + pad[0];
}

/* 泄漏用例不能写成 `void *p = malloc(64); (void)p;`：
 * -O1 能把"分配后既不读也不 free"整对消掉（实测反汇编只剩一条 ret），LSan 什么都看不到。
 * 用 volatile 函数指针调用，编译器就失去 malloc 的内建知识、无法消除；
 * 再把指针短暂存进全局后清空（残留指针留在栈上同样会让 LSan 认为"还活着"）。
 * 这两步缺一不可。 */
static void *(*volatile g_alloc)(size_t) = malloc;
static void *g_last;

__attribute__((noinline)) static void leak_alloc(void)
{
        char *p = g_alloc(64);
        p[0] = 1;        /* 真写一下，证明这块内存确实被拿到过 */
        g_last = p;
        g_last = NULL;   /* 不可达：LSan 只认"退出时还能不能找到" */
}

/* 本函数故意制造 UAF。GCC 15 的 -Wuse-after-free 会把它当编译错误拦下，
 * 而这里"编译期就发现"恰恰说明不打 sanitizer 也未必真跑得到——
 * 所以只在最小范围内关掉该警告，并把这件事写进 README §8.1。 */
__attribute__((noinline)) static int use_after_free(void)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuse-after-free"
        char *p = malloc(16);
        free(p);
        volatile char *vp = p;
        int v = vp[0];
#pragma GCC diagnostic pop
        return v;
}

int main(int argc, char **argv)
{
        const char *what = argc > 1 ? argv[1] : "ok";

        if (!strcmp(what, "heap-overflow")) {
                char *p = malloc(8);
                volatile char *vp = p;
                vp[8] = 42;                     /* 红区越界 1 字节 */
                free(p);
        } else if (!strcmp(what, "uaf")) {
                printf("S06|asan|uaf|read=%d\n", use_after_free());   /* 释放后读 */
        } else if (!strcmp(what, "stack-overflow")) {
                printf("S06|asan|deep=%d\n", deep(100000));
        } else if (!strcmp(what, "leak")) {
                leak_alloc();                   /* 故意不 free，交给 LSan */
        } else if (strcmp(what, "ok") != 0) {
                fprintf(stderr, "unknown mode: %s\n", what);
                return 2;
        }

        printf("S06|asan|end|mode=%s\n", what);
        return 0;
}
