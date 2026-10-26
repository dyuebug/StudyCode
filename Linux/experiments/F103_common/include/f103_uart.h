/**
 * USART1 最小驱动（轮询收发）
 *
 * 适用：PA9 = TX（复用推挽），PA10 = RX（浮空输入），8N1，无硬件流控。
 * 时钟假设：PCLK2 = HCLK = SystemCoreClock（PPRE2 未分频）。
 *
 * 调用顺序要求：必须先调用 f103_tick_init() 或 f103_tick_init_irq()，
 * 它们内部的 SystemCoreClockUpdate() 负责把 SystemCoreClock 从错误的
 * 16 MHz 初值刷新为真实的 8 MHz；否则 f103_uart_init() 算出的 BRR 会偏一倍。
 */
#ifndef F103_UART_H
#define F103_UART_H

#include <stdint.h>

/* 轮询结果：区分"没有数据"与"数据被丢弃"，避免溢出被静默吞掉 */
enum {
    F103_UART_EMPTY = 0,
    F103_UART_BYTE  = 1,
    F103_UART_OVER  = 2
};

void f103_uart_init(uint32_t baud);
void f103_uart_putc(char c);

/**
 * 非阻塞取一个字节。
 * 返回 F103_UART_BYTE 时 *out 有效；F103_UART_OVER 表示发生溢出，
 * 该字节已被丢弃（读 DR 只为清除 ORE）。
 */
uint8_t f103_uart_poll(char *out);

#endif /* F103_UART_H */
