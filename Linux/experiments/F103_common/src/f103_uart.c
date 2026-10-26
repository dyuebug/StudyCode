/**
 * USART1 最小驱动实现
 *
 * 寄存器路径：
 *   RCC->APB2ENR 开 GPIOA / USART1 时钟
 *   GPIOA->CRH   把 PA9 置为复用推挽，PA10 置为浮空输入
 *   USART1->BRR  按 PCLK2 / baud 计算波特率
 *   USART1->CR1  使能 UE / TE / RE
 *   USART1->SR   轮询 TXE（可写）、RXNE（可读）、ORE（溢出）
 */
#include "f103_uart.h"

#include "stm32f1xx.h"

/*
 * PA9 在 CRH 的 bit[7:4]，PA10 在 bit[11:8]。
 * 复用推挽输出 50 MHz = CNF 10 + MODE 11 = 0xB；浮空输入 = CNF 01 + MODE 00 = 0x4。
 */
#define UART_CRH_MASK   0x00000FF0U
#define UART_CRH_VALUE  0x000004B0U

static void uart_gpio_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;

    /* 先清两个引脚的 4 bit 配置，再整体置位；CRH 复位值为 0x4444_4444 */
    GPIOA->CRH = (GPIOA->CRH & ~UART_CRH_MASK) | UART_CRH_VALUE;
}

void f103_uart_init(uint32_t baud)
{
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    uart_gpio_init();

    USART1->CR1 = 0U;   /* 配置期间先关 UE，避免半配置状态上线 */
    USART1->CR2 = 0U;   /* 1 个停止位 */
    USART1->CR3 = 0U;   /* 无硬件流控，无 DMA */

    /*
     * BRR = round(PCLK2 / baud)。PCLK2 = HCLK = 8 MHz（PPRE2 分频为 1），
     * 8e6 / 115200 = 69.44 → 69 = 0x45，实际 115942 baud，误差 +0.64%。
     * 若日后配置 PLL 到 72 MHz，必须重新调用本函数，否则波特率整体偏移。
     */
    USART1->BRR = (SystemCoreClock + (baud / 2U)) / baud;

    USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

void f103_uart_putc(char c)
{
    while ((USART1->SR & USART_SR_TXE) == 0U) {
        /* 等待发送数据寄存器空 */
    }
    USART1->DR = (uint8_t)c;
}

/**
 * F1 的 ORE 必须靠"先读 SR、再读 DR"清除；溢出时读出的数据不可信，
 * 直接丢弃并上报，而不是把一个可能错位的字节交给调用者。
 */
uint8_t f103_uart_poll(char *out)
{
    uint32_t sr = USART1->SR;

    if ((sr & (USART_SR_RXNE | USART_SR_ORE)) == 0U) {
        return F103_UART_EMPTY;
    }

    char c = (char)USART1->DR;   /* 读 DR 同时清 RXNE 与 ORE */

    if ((sr & USART_SR_ORE) != 0U) {
        return F103_UART_OVER;
    }
    *out = c;
    return F103_UART_BYTE;
}

/* newlib 的 printf 最终调用 _write；返回写入的字节数 */
int _write(int fd, char *ptr, int len)
{
    (void)fd;
    for (int i = 0; i < len; i++) {
        f103_uart_putc(ptr[i]);
    }
    /* 等最后一字节真正移出移位寄存器，避免复位时截断 */
    while ((USART1->SR & USART_SR_TC) == 0U) {
    }
    return len;
}
