/* sani_ubsan.c —— UBSan：未定义行为在"还没崩"的时候就能报出来
 *
 * 编译带 -fno-sanitize-recover=all：默认 UBSan 只打印继续跑，
 * 那样缺陷会被后续输出掩盖，判定器也无从区分 rc。
 */
#define _GNU_SOURCE
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
        const char *what = argc > 1 ? argv[1] : "ok";

        /* volatile 让每一条都真的执行：否则 -O1 能把常量表达式在编译期算掉 */
        volatile int shift = 40;
        volatile int big = INT_MAX;
        volatile int zero = 0;
        volatile int *nullp = NULL;

        if (!strcmp(what, "shift")) {
                volatile int x = 1;
                printf("S06|ubsan|x=%d\n", x << shift);         /* 移位超出位宽 */
        } else if (!strcmp(what, "ovf")) {
                printf("S06|ubsan|a=%d\n", big + 1);            /* 有符号溢出 */
        } else if (!strcmp(what, "div")) {
                printf("S06|ubsan|d=%d\n", 10 / zero);          /* 除零 */
        } else if (!strcmp(what, "null")) {
                printf("S06|ubsan|n=%d\n", *nullp);             /* 空指针解引用 */
        } else if (strcmp(what, "ok") != 0) {
                fprintf(stderr, "unknown mode: %s\n", what);
                return 2;
        }

        printf("S06|ubsan|end|mode=%s\n", what);
        return 0;
}
