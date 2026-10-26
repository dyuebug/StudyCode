/**
 * F103_05：USART1 中断接收 + 无锁环形缓冲
 *
 * 动机：实验 04 的轮询回显在主循环被 printf 阻塞约 3 ms 期间会丢字节。
 * 本实验把"收"搬进中断：ISR 只做三件事——读 SR/DR、写环形缓冲、计数，
 * 主循环负责回显，两边通过 head/tail 两个索引通信，不加锁。
 *
 * 硬件：PA9 = TX，PA10 = RX，115200 8N1；USART1 走 IRQ 37。
 * 优先级：SysTick = 0（默认），USART1 = 1，计时不被串口打断。
 */
#include <stdint.h>
#include <stdio.h>

#include "stm32f1xx.h"
#include "f103_tick.h"
#include "f103_uart.h"

#define UART_BAUD      115200U
#define HEARTBEAT_MS   1000U

/* 必须是 2 的幂：索引回绕用掩码而不是取模。可用容量 = RX_RING_SIZE - 1 */
#define RX_RING_SIZE   128U
#define RX_IRQ_PRIO    1U

static volatile uint8_t  s_ring[RX_RING_SIZE];
static volatile uint16_t s_head;   /* 只由 ISR 推进 */
static volatile uint16_t s_tail;   /* 只由主循环推进 */
static volatile uint32_t s_ore;    /* 硬件溢出：DR 被新字节覆盖 */
static volatile uint32_t s_drop;   /* 缓冲满：软件主动丢弃 */

static void rx_ring_reset(void)
{
    s_head = 0U;
    s_tail = 0U;
}

/**
 * 单生产者（ISR）单消费者（主循环）：
 *   写者只改 head，读者只改 tail，因此不需要关中断。
 *   代价是必须留一个空位区分"满"和"空"——head == tail 为满或空，
 *   让 head 永远不追上 tail 即可用 head == tail 表示空。
 */
static void rx_ring_put(uint8_t b)
{
    uint16_t next = (uint16_t)((s_head + 1U) & (RX_RING_SIZE - 1U));

    if (next == s_tail) {
        s_drop++;
        return;
    }
    s_ring[s_head] = b;
    s_head = next;
}

static uint8_t rx_ring_get(uint8_t *b)
{
    if (s_head == s_tail) {
        return 0U;
    }
    *b = s_ring[s_tail];
    s_tail = (uint16_t)((s_tail + 1U) & (RX_RING_SIZE - 1U));
    return 1U;
}

/**
 * IRQ 37。进来后先确认是不是本外设的事件（虽然该向量只属于 USART1，
 * 但 RXNE 与 ORE 共用这一条中断，必须分别处理）。
 * 读 DR 一次同时清 RXNE 与 ORE；ORE 说明前一个字节已被覆盖，只能丢弃。
 */
void USART1_IRQHandler(void)
{
    uint32_t sr = USART1->SR;

    if ((sr & (USART_SR_RXNE | USART_SR_ORE)) == 0U) {
        return;
    }

    uint8_t b = (uint8_t)USART1->DR;

    if ((sr & USART_SR_ORE) != 0U) {
        s_ore++;
        return;
    }
    rx_ring_put(b);
}

static void rx_irq_init(void)
{
    rx_ring_reset();

    /* RXNEIE 同时放行 RXNE 与 ORE 中断 */
    USART1->CR1 |= USART_CR1_RXNEIE;

    NVIC_SetPriority(USART1_IRQn, RX_IRQ_PRIO);
    NVIC_EnableIRQ(USART1_IRQn);
}

int main(void)
{
    f103_tick_init_irq();
    f103_uart_init(UART_BAUD);
    rx_irq_init();

    printf("\r\n[F103_05] USART1 RX IRQ + ring buffer (%u B, usable %u B)\r\n",
           (unsigned)RX_RING_SIZE,
           (unsigned)(RX_RING_SIZE - 1U));

    uint32_t next_beat = HEARTBEAT_MS;
    uint32_t rx_bytes  = 0U;

    for (;;) {
        uint8_t b;

        /* 一次取空：主循环无论被 printf 阻塞多久，数据都在 ISR 里排队 */
        while (rx_ring_get(&b) != 0U) {
            rx_bytes++;
            f103_uart_putc((char)b);
            if ((char)b == '\r') {
                f103_uart_putc('\n');
            }
        }

        uint32_t now = f103_tick_now_ms();
        if ((int32_t)(now - next_beat) >= 0) {
            next_beat += HEARTBEAT_MS;
            /*
             * s_drop / s_ore 由 ISR 修改、主循环读取，各自是 32 位对齐访问，
             * 在 M3 上是单条指令，无需关中断；只是三个数不是同一瞬间的快照。
             */
            printf("[%lu ms] alive, rx=%lu, drop=%lu, ore=%lu\r\n",
                   (unsigned long)now,
                   (unsigned long)rx_bytes,
                   (unsigned long)s_drop,
                   (unsigned long)s_ore);
        }
    }
}
