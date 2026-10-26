/**
 * F103_04：USART1 轮询收发 + printf 重定向
 *
 * 硬件：PA9 = USART1_TX，PA10 = USART1_RX，115200 8N1，无硬件流控
 * 时钟：未配置 PLL，HCLK = PCLK2 = 8 MHz（HSI）
 *
 * USART1 的寄存器配置在 F103_common/src/f103_uart.c 中，本实验关注的是
 * 轮询接收模型与 newlib stdio 的接线：_write -> f103_uart_putc，
 * _isatty / _sbrk / 四个 syscall 桩在 F103_common/src/crt_stubs.c。
 */
#include <stdint.h>
#include <stdio.h>

#include "stm32f1xx.h"
#include "f103_tick.h"
#include "f103_uart.h"

#define UART_BAUD      115200U
#define HEARTBEAT_MS   1000U

int main(void)
{
    /* 先刷新 SystemCoreClock（SystemInit 不设置它），f103_uart_init 的 BRR 依赖该值 */
    f103_tick_init_irq();
    f103_uart_init(UART_BAUD);

    printf("\r\n[F103_04] USART1 echo, PCLK2=%lu Hz, %lu 8N1\r\n",
           (unsigned long)SystemCoreClock,
           (unsigned long)UART_BAUD);

    uint32_t next_beat = HEARTBEAT_MS;
    uint32_t rx_bytes  = 0U;
    uint32_t rx_over   = 0U;

    for (;;) {
        char c;
        uint8_t r = f103_uart_poll(&c);

        if (r == F103_UART_BYTE) {
            rx_bytes++;
            f103_uart_putc(c);
            if (c == '\r') {
                f103_uart_putc('\n');   /* 终端回车只回显 \r 不会换行 */
            }
        } else if (r == F103_UART_OVER) {
            rx_over++;
        }

        uint32_t now = f103_tick_now_ms();
        if ((int32_t)(now - next_beat) >= 0) {
            next_beat += HEARTBEAT_MS;
            printf("[%lu ms] alive, rx=%lu, over=%lu\r\n",
                   (unsigned long)now,
                   (unsigned long)rx_bytes,
                   (unsigned long)rx_over);
        }
    }
}
