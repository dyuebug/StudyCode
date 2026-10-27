/**
 * F103_19：DMA1 通道 1 内存到内存搬运
 *
 * 为什么选 mem2mem：
 *   这是唯一不依赖任何外设的 DMA 用法（CCR.MEM2MEM=1 时，"外设端"直接是内存），
 *   所以搬运能不能成功只取决于寄存器序列，不取决于有没有接外设、外设是否建模。
 *   反过来，接 USART/ADC 的 DMA 一旦外设自己没工作，失败原因就分不清是 DMA 还是外设。
 *
 * 三个用例对应三种常见用法与同一个陷阱：
 *   1. 轮询 TCIF    —— 最直接，但 CPU 在等
 *   2. TC 中断      —— CPU 不忙等，代价是中断上下文
 *   3. 不清标志直接复用 —— 演示"假完成"：TCIF 上一次留下的 1 会被读成这次的完成
 *
 * 关键寄存器（RM0008 §13）：
 *   CCR    : EN / TCIE / DIR(1=读内存) / CIRC / PINC / MINC / PSIZE / MSIZE / MEM2MEM
 *   CNDTR  : 剩余传输个数，递减到 0 即完成。**EN=1 时不可写**，必须关 EN 才能重装
 *   CPAR   : mem2mem 下是"源"地址，靠 PINC 递增
 *   CMAR   : 目的地址，靠 MINC 递增
 *   ISR/IFCR: 标志只读，清零只能写 IFCR，写 ISR 无效
 *
 * 所有等待都带迭代上限：QEMU 的 stm32vldiscovery 没有 DMA 模型，
 * TCIF 永远不会置位；没有上限就挂死，日志停在半截也看不出原因。
 */
#include <stdint.h>
#include <stdio.h>

#include "stm32f1xx.h"
#include "f103_tick.h"
#include "f103_uart.h"

#define UART_BAUD   115200U
#define BUF_WORDS   256U        /* 1 KB：32 位宽，PSIZE/MSIZE = 10 */
#define POLL_LIMIT  200000U     /* 轮询上限，非时间单位 */

static uint32_t s_src[BUF_WORDS];
static uint32_t s_dst[BUF_WORDS];
static volatile uint32_t s_irq_count;

/* 中断里只清标志、只计数：不做 IO，也不碰任何 printf */
void DMA1_Channel1_IRQHandler(void)
{
    if ((DMA1->ISR & DMA_ISR_TCIF1) != 0U) {
        DMA1->IFCR = DMA_IFCR_CTCIF1;
        s_irq_count++;
    }
}

/* 关通道并等 EN 真正读回 0。
 * 手册要求修改 CNDTR 前先关 EN；硬件是在当前这次传输结束后才清 EN，所以不能写完就走。 */
static uint32_t dma1_ch1_disable(void)
{
    DMA1_Channel1->CCR &= ~DMA_CCR_EN;

    for (uint32_t i = 0U; i < POLL_LIMIT; i++) {
        if ((DMA1_Channel1->CCR & DMA_CCR_EN) == 0U) {
            return 1U;
        }
    }
    return 0U;
}

/* 装地址与长度并启动。PINC/MINC 必须同时置位：
 * mem2mem 下"外设端"被当成源，不置 PINC 就是同一个源字写 256 次，
 * 结果 dst 全是第一个字，看起来"跑完了"但数据是错的。 */
static void dma1_ch1_start(const uint32_t *src, uint32_t *dst, uint32_t words, uint32_t extra)
{
    DMA1_Channel1->CPAR  = (uint32_t)(uintptr_t)src;
    DMA1_Channel1->CMAR  = (uint32_t)(uintptr_t)dst;
    DMA1_Channel1->CNDTR = words;
    DMA1_Channel1->CCR   = DMA_CCR_MEM2MEM | DMA_CCR_PINC | DMA_CCR_MINC |
                           DMA_CCR_DIR | DMA_CCR_PSIZE_1 | DMA_CCR_MSIZE_1 | extra;
    DMA1_Channel1->CCR  |= DMA_CCR_EN;
}

static void fill_src(void)
{
    for (uint32_t i = 0U; i < BUF_WORDS; i++) {
        s_src[i] = 0xA5A50000U + i;
        s_dst[i] = 0U;
    }
}

/* 返回第一个不匹配的下标，全部匹配返回 BUF_WORDS */
static uint32_t first_mismatch(void)
{
    for (uint32_t i = 0U; i < BUF_WORDS; i++) {
        if (s_dst[i] != s_src[i]) {
            return i;
        }
    }
    return BUF_WORDS;
}

static void report_copy(const char *tag, uint32_t elapsed_ms, uint32_t tcif, uint32_t cndtr)
{
    const uint32_t bad = first_mismatch();
    printf("%s：TCIF1=%lu CNDTR=%lu 用时 %lu ms，拷贝 %s\r\n",
           tag, (unsigned long)tcif, (unsigned long)cndtr, (unsigned long)elapsed_ms,
           (bad == BUF_WORDS) ? "全对" : "有错");
    if (bad != BUF_WORDS) {
        printf("      首个不一致下标 %lu：src=0x%08lX dst=0x%08lX\r\n",
               (unsigned long)bad, (unsigned long)s_src[bad], (unsigned long)s_dst[bad]);
    }
}

int main(void)
{
    f103_tick_init_irq();              /* 用毫秒计数给每次搬运计时 */
    f103_uart_init(UART_BAUD);

    printf("\r\n=== F103_19 DMA1 ch1 内存到内存 ===\r\n");

    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1->IFCR = 0x0000000FU;          /* 清通道 1 的全部标志，避免上一轮残留 */

    uint32_t ccr_readback = 0U;
    uint32_t cndtr_readback = 0U;

    /* ---- 用例 1：轮询 TCIF ---- */
    {
        fill_src();
        const uint32_t t0 = f103_tick_now_ms();
        dma1_ch1_start(s_src, s_dst, BUF_WORDS, 0U);

        /* 启动后立刻读一次 CCR/CNDTR：能读回配置值才说明通道真的被写进去了。
         * 恒读 0 就是未建模——此时后面的 TCIF 等待必然超时，不必再猜。 */
        ccr_readback   = DMA1_Channel1->CCR;
        cndtr_readback = DMA1_Channel1->CNDTR;

        uint32_t tcif = 0U;
        for (uint32_t i = 0U; i < POLL_LIMIT; i++) {
            if ((DMA1->ISR & DMA_ISR_TCIF1) != 0U) {
                tcif = 1U;
                break;
            }
        }
        const uint32_t t1 = f103_tick_now_ms();

        report_copy("用例1 轮询", t1 - t0, tcif, DMA1_Channel1->CNDTR);
        printf("      CCR 读回 0x%08lX（期望含 EN|DIR|PINC|MINC|PSIZE|MSIZE|MEM2MEM），"
               "CNDTR 读回 %lu\r\n",
               (unsigned long)ccr_readback, (unsigned long)cndtr_readback);
        if (ccr_readback == 0U) {
            printf("      CCR 恒为 0：本机未建模 DMA1，下面两个用例也只能看超时分支\r\n");
        }
        if (tcif != 0U) {
            DMA1->IFCR = DMA_IFCR_CTCIF1;
        }
        (void)dma1_ch1_disable();
    }

    /* ---- 用例 2：TC 中断 ---- */
    {
        fill_src();
        s_irq_count = 0U;
        NVIC_SetPriority(DMA1_Channel1_IRQn, 1U);   /* 比 SysTick 低，不抢占时基 */
        NVIC_EnableIRQ(DMA1_Channel1_IRQn);

        DMA1->IFCR = DMA_IFCR_CTCIF1;
        const uint32_t t0 = f103_tick_now_ms();
        dma1_ch1_start(s_src, s_dst, BUF_WORDS, DMA_CCR_TCIE);

        for (uint32_t i = 0U; (i < POLL_LIMIT) && (s_irq_count == 0U); i++) {
            /* 空转等中断。真板上这段几乎瞬间返回，仿真里会走满上限 */
        }
        const uint32_t t1 = f103_tick_now_ms();

        NVIC_DisableIRQ(DMA1_Channel1_IRQn);
        report_copy("用例2 中断", t1 - t0, s_irq_count, DMA1_Channel1->CNDTR);
        printf("      中断进入 %lu 次（应为 1；>1 说明标志没清干净导致重复进中断）\r\n",
               (unsigned long)s_irq_count);
        (void)dma1_ch1_disable();
    }

    /* ---- 用例 3：不清 TCIF 直接复用 → 假完成 ----
     * 用例 1 结束时 TCIF 本应被清掉。这里故意保留它，重装长度后立刻读：
     * 标志仍是上次的 1，CNDTR 还是全量。任何"while(!TCIF);"的写法都会立刻通过。 */
    {
        fill_src();
        (void)dma1_ch1_disable();
        /* 先做一次完整搬运并把 TCIF 留在置位状态 */
        dma1_ch1_start(s_src, s_dst, BUF_WORDS, 0U);
        for (uint32_t i = 0U; i < POLL_LIMIT; i++) {
            if ((DMA1->ISR & DMA_ISR_TCIF1) != 0U) {
                break;
            }
        }
        (void)dma1_ch1_disable();
        const uint32_t stale = (DMA1->ISR & DMA_ISR_TCIF1) ? 1U : 0U;

        /* 不清标志，直接重装并启动 */
        dma1_ch1_start(s_src, s_dst, BUF_WORDS, 0U);
        const uint32_t tcif_now  = (DMA1->ISR & DMA_ISR_TCIF1) ? 1U : 0U;
        const uint32_t cndtr_now = DMA1_Channel1->CNDTR;
        printf("用例3 假完成：复用前 TCIF1=%lu，启动后立刻读 TCIF1=%lu CNDTR=%lu\r\n",
               (unsigned long)stale, (unsigned long)tcif_now, (unsigned long)cndtr_now);
        /* 判据只依赖 stale 与 tcif_now：CNDTR 在真板上可能已经开始递减甚至归零，
         * 它只是辅助读数，不适合当断言。 */
        if ((stale != 0U) && (tcif_now != 0U)) {
            printf("      如预期：启动瞬间标志就是 1，等 TCIF 的循环会立刻返回，"
                   "与这一帧是否搬完无关\r\n");
        } else if (stale == 0U) {
            printf("      本机 TCIF 从未置位，这一条无从对照（未建模）\r\n");
        }

        /* 正确写法：关 EN → 清标志 → 重装 → 启动 */
        (void)dma1_ch1_disable();
        DMA1->IFCR = DMA_IFCR_CTCIF1;
        dma1_ch1_start(s_src, s_dst, BUF_WORDS, 0U);
        printf("      清标志后重启：TCIF1=%lu CNDTR=%lu（应为 0 与 %u）\r\n",
               (unsigned long)((DMA1->ISR & DMA_ISR_TCIF1) ? 1U : 0U),
               (unsigned long)DMA1_Channel1->CNDTR, (unsigned)BUF_WORDS);
        (void)dma1_ch1_disable();
    }

    printf("=== 结束（QEMU 未建模 DMA 时三条都是超时/原值；真板需重跑本节）===\r\n");
    for (;;) {
        f103_tick_delay_ms(1000U);
    }
}
