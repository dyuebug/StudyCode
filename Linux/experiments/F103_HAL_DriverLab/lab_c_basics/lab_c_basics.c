/*
 * lab_c_basics：主机侧验证教材 5.6 节「C 语言基础补充」的四条前提。
 *
 * 为什么放在主机侧：这四条结论（指针大小、结构体偏移、位运算优先级、字节序）
 * 与目标芯片无关，用主机 gcc 就能得到读数，不需要 DShanMCU-103 板。
 * 唯一与目标相关的是"32 位 vs 64 位"——本程序同时打印本机实况，
 * 说明为什么在 PC 上读到的指针大小不能直接搬到 STM32 上。
 *
 * 构建与运行（本机实测 Windows 11 + Git Bash + MinGW gcc 15.1.0）：
 *     gcc -std=c11 -Wall -Wextra -O0 lab_c_basics.c -o lab_c_basics
 *     ./lab_c_basics
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* ---- 1. 结构体偏移：头部只有 5 个字段，顺序必须等于寄存器偏移顺序 ---- */
typedef struct {
    uint32_t CRL;   /* 0x00 */
    uint32_t CRH;   /* 0x04 */
    uint32_t IDR;   /* 0x08 */
    uint32_t ODR;   /* 0x0C */
    uint32_t BSRR;  /* 0x10 ← 反汇编里 [r0, #16] 就是它 */
    uint32_t BRR;   /* 0x14 */
    uint32_t LCKR;  /* 0x18 */
} GPIO_REGS_DEMO;

/* ---- 3. 复刻 driver_ds18b20.c:463-467 的三条条件 ---- */
static const char *accuracy_buggy(uint8_t cfg)
{
    if (cfg & (3 << 5) == 0)          return "9bit";   /* 本行永远不会命中 */
    else if (cfg & (3 << 5) == (1 << 5)) return "10bit";
    else if (cfg & (3 << 5) == (2 << 5)) return "11bit";
    else                              return "12bit";
}

static const char *accuracy_fixed(uint8_t cfg)
{
    if ((cfg & (3 << 5)) == 0)          return "9bit";
    else if ((cfg & (3 << 5)) == (1 << 5)) return "10bit";
    else if ((cfg & (3 << 5)) == (2 << 5)) return "11bit";
    else                               return "12bit";
}

int main(void)
{
    /* ---- 1. 各种指针与整型在「本机」的大小 ---- */
    printf("[1] sizeof: void*=%zu  int*=%zu  short*=%zu  long*=%zu  char*=%zu\n",
           sizeof(void *), sizeof(int *), sizeof(short *), sizeof(long *),
           sizeof(char *));
    printf("[1] sizeof: uint8_t=%zu  uint16_t=%zu  uint32_t=%zu  uint64_t=%zu  long=%zu\n",
           sizeof(uint8_t), sizeof(uint16_t), sizeof(uint32_t), sizeof(uint64_t),
           sizeof(long));

    /* ---- 2. 结构体偏移与总大小：有没有填充 ---- */
    printf("[2] offsetof: CRL=%zu CRH=%zu IDR=%zu ODR=%zu BSRR=%zu BRR=%zu LCKR=%zu\n",
           offsetof(GPIO_REGS_DEMO, CRL), offsetof(GPIO_REGS_DEMO, CRH),
           offsetof(GPIO_REGS_DEMO, IDR), offsetof(GPIO_REGS_DEMO, ODR),
           offsetof(GPIO_REGS_DEMO, BSRR), offsetof(GPIO_REGS_DEMO, BRR),
           offsetof(GPIO_REGS_DEMO, LCKR));
    printf("[2] sizeof(GPIO_REGS_DEMO)=%zu\n", sizeof(GPIO_REGS_DEMO));

    /* ---- 3. 优先级：& 与 == 谁先结合 ---- */
    printf("[3] 表达式自检: (3<<5)=%d  ((3<<5)==0)=%d  (0x60 & ((3<<5)==0))=%d\n",
           3 << 5, (3 << 5) == 0, 0x60 & ((3 << 5) == 0));

    const uint8_t cfgs[] = {0x00, 0x20, 0x40, 0x60};
    for (size_t i = 0; i < sizeof(cfgs) / sizeof(cfgs[0]); i++) {
        printf("[3] cfg=0x%02X  buggy=%s  fixed=%s\n",
               cfgs[i], accuracy_buggy(cfgs[i]), accuracy_fixed(cfgs[i]));
    }

    /* ---- 4. 位操作的三种常用形态 ---- */
    uint32_t a = 0x123;
    printf("[4] set   bit7,bit8 : 0x%03X -> 0x%03X\n",
           a, a | (1u << 7) | (1u << 8));
    printf("[4] clear bit7,bit8 : 0x%03X -> 0x%03X\n",
           a, a & ~((1u << 7) | (1u << 8)));
    uint32_t val = 3;
    uint32_t b = a & ~(3u << 7);
    b = b | (val << 7);
    printf("[4] set field[8:7]=%u: 0x%03X -> 0x%03X\n", val, a, b);

    /* ---- 5. 字节序：本机实况 + STM32 侧由链接脚本决定的结论 ---- */
    uint32_t v = 0x12345678u;
    uint8_t bytes[4];
    memcpy(bytes, &v, sizeof(bytes));
    printf("[5] 0x12345678 在内存中的低->高地址字节: %02X %02X %02X %02X (%s)\n",
           bytes[0], bytes[1], bytes[2], bytes[3],
           bytes[0] == 0x78 ? "little endian" : "big endian");

    return 0;
}
