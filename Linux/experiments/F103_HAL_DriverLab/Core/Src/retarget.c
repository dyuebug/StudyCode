/**
 * printf 重定向到 USART1（PA9/PA10，115200 8N1）
 *
 * 为什么放在独立文件而不是 crt_stubs.c：
 * crt_stubs.c 里的桩与硬件无关，可原样复用于任何 F103 工程；
 * 这个 _write 依赖 huart1，只能和 usart.c 同时存在。分开后
 * "换一个串口"只改本文件一行，"关掉 stdio"只需不编译本文件。
 *
 * 为什么用阻塞发送而不是中断/DMA：
 * 课程驱动的测试函数（Led_Test 等）都在裸机上下文按顺序打印，
 * 没有并发写者，阻塞发送的语义最简单。若将来在中断里调用 printf，
 * 必须换成环形缓冲 + DMA，否则会长时间占用中断上下文。
 */

#include "usart.h"

#include <errno.h>
#include <stdint.h>

/**
 * newlib 的 sys_write 一次给一段缓冲区，可能含多个字节，也可能含 '\0'，
 * 因此按长度发送而不是当字符串处理。
 *
 * HAL_UART_STATE_RESET 表示 MX_USART1_UART_Init() 还没执行；此时返回 -1，
 * 让 printf 的调用方能看出"输出没发出去"，而不是静默丢弃。
 * 用 Instance 判空不可靠：CubeMX 生成的句柄是全局零初始化，
 * 但 HAL_UART_Init 成功前也可能已经赋过 Instance。
 */
int _write(int fd, const char *buf, int len)
{
    (void)fd;

    if (len <= 0) {
        return 0;
    }
    if (huart1.gState == HAL_UART_STATE_RESET) {
        errno = EIO;
        return -1;
    }

    /* 超时按波特率估算偏保守：115200 8N1 约 87 us/字节，给足 10 倍余量 */
    if (HAL_UART_Transmit(&huart1, (uint8_t *)buf, (uint16_t)len,
                          (uint32_t)len * 10U) != HAL_OK) {
        errno = EIO;
        return -1;
    }
    return len;
}
