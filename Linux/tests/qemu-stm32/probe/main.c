/**
 * QEMU STM32 外设建模能力探针
 *
 * 目的：不靠读 QEMU 源码猜，而是让固件在 -M stm32vldiscovery 上真跑一遍，
 * 逐个外设写入后读回，把"模型到底实现了什么"变成可复现的输出。
 *
 * 断言方式：
 *   - 写一个特征值再读回，读回值相同 → 该寄存器有状态（可能已建模）
 *   - 读回恒为 0 或写入被忽略     → 未建模（QEMU 的 unimplemented_device）
 *
 * 所有轮询都带迭代上限：未建模的外设不会给状态位，没有上限就会挂死。
 */
#include <stdint.h>
#include <stdio.h>

#include "stm32f1xx.h"
#include "f103_tick.h"
#include "f103_uart.h"

#define UART_BAUD 115200U
#define SPIN(n)   do { for (volatile uint32_t _i = 0U; _i < (n); _i++) { } } while (0)

static void report_reg(const char *name, uint32_t wrote, uint32_t read)
{
    printf("%-14s 写入 0x%08lX  读回 0x%08lX  %s\r\n",
           name, (unsigned long)wrote, (unsigned long)read,
           (read == wrote) ? "有状态" : "未建模/只读");
}

int main(void)
{
    SystemCoreClockUpdate();
    f103_uart_init(UART_BAUD);

    printf("\r\n=== QEMU STM32 建模探针 ===\r\n");
    printf("[1] UART：你看到这一行，说明 USART1 的 TX 通路已建模\r\n");
    printf("    SystemCoreClock = %lu Hz（RCC 读回 0 时走 HSI 8 MHz 分支）\r\n",
           (unsigned long)SystemCoreClock);

    /* ---- 2. Flash 与 SRAM ---- */
    volatile uint32_t *img = (volatile uint32_t *)0x08000000U;
    printf("[2] Flash[0x08000000] = 0x%08lX（应为初始 MSP，证明 Flash 区域有效）\r\n",
           (unsigned long)img[0]);

    /*
     * 只测 8 KB 之内。超出范围的访问不是"读回 0"：QEMU 会把它变成 BusFault，
     * 没有 fault handler 时升级成 HardFault，默认 Handler 是死循环。
     * 上次用 F103C8T6 的链接脚本（_estack = 0x20005000）跑，第一次压栈就是这样挂掉的。
     */
    volatile uint32_t *ram_ok = (volatile uint32_t *)0x20001FF0U;
    *ram_ok = 0xA5A55A5AU;
    printf("    RAM[0x20001FF0] 读回 0x%08lX（写入 0xA5A55A5A，8 KB 之内）\r\n",
           (unsigned long)*ram_ok);

    /* ---- 3. RCC ---- */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    report_reg("RCC->APB2ENR", RCC_APB2ENR_IOPAEN, RCC->APB2ENR & RCC_APB2ENR_IOPAEN);
    printf("    RCC->CFGR = 0x%08lX\r\n", (unsigned long)RCC->CFGR);

    /* ---- 4. GPIO ---- */
    GPIOA->CRL = 0x44444444U;
    GPIOA->ODR = 0x0020U;
    report_reg("GPIOA->ODR", 0x00000020U, GPIOA->ODR);
    report_reg("GPIOA->CRL", 0x44444444U, GPIOA->CRL);
    printf("    GPIOA->IDR = 0x%08lX\r\n", (unsigned long)GPIOA->IDR);

    /* ---- 5. TIM4 ---- */
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
    TIM4->PSC = 0U;
    TIM4->ARR = 0xFFFFU;
    TIM4->CR1 = TIM_CR1_CEN;
    uint32_t cnt0 = TIM4->CNT;
    SPIN(1000U);
    uint32_t cnt1 = TIM4->CNT;
    printf("[5] TIM4：CNT 两次读数为 %lu -> %lu（不变说明定时器未建模）\r\n",
           (unsigned long)cnt0, (unsigned long)cnt1);
    report_reg("TIM4->CR1", TIM_CR1_CEN, TIM4->CR1 & TIM_CR1_CEN);

    /* ---- 6. I2C1 ---- */
    I2C1->CR1 |= I2C_CR1_PE;
    report_reg("I2C1->CR1", I2C_CR1_PE, I2C1->CR1 & I2C_CR1_PE);
    printf("    I2C1->SR1 = 0x%08lX\r\n", (unsigned long)I2C1->SR1);

    /* ---- 7. SPI1 ---- */
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR_0 | SPI_CR1_SPE;
    report_reg("SPI1->CR1", SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR_0 | SPI_CR1_SPE,
               SPI1->CR1);
    SPI1->DR = 0x9FU;                    /* 读写一次，看 RXNE 会不会置位 */
    uint32_t rxne = 0U;
    for (uint32_t i = 0U; i < 1000U; i++) {
        if ((SPI1->SR & SPI_SR_RXNE) != 0U) {
            rxne = 1U;
            break;
        }
    }
    printf("    SPI1：写 DR 后 RXNE %s，SR = 0x%08lX，DR = 0x%08lX\r\n",
           rxne ? "置位" : "未置位", (unsigned long)SPI1->SR,
           (unsigned long)SPI1->DR);

    /* ---- 8. DWT ---- */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    uint32_t dwt_ctrl = DWT->CTRL;
    uint32_t dwt_a = DWT->CYCCNT;
    SPIN(1000U);
    uint32_t dwt_b = DWT->CYCCNT;
    printf("[8] DWT：CTRL 读回 0x%08lX，CYCCNT %lu -> %lu（差 %lu）\r\n",
           (unsigned long)dwt_ctrl, (unsigned long)dwt_a, (unsigned long)dwt_b,
           (unsigned long)(dwt_b - dwt_a));

    /* ---- 9. SysTick 轮询 ---- */
    SysTick->LOAD = (SystemCoreClock / 1000U) - 1U;
    SysTick->VAL = 0U;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
    uint32_t wraps = 0U;
    for (uint32_t i = 0U; (i < 20000000U) && (wraps < 5U); i++) {
        if ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) != 0U) {
            wraps++;
        }
    }
    printf("[9] SysTick 轮询：捕获 %lu 次 COUNTFLAG（5 次即视为工作）\r\n",
           (unsigned long)wraps);

    /* ---- 10. SysTick 中断 + NVIC（复用 f103_tick 的中断模式） ---- */
    f103_tick_init_irq();          /* 该模块自带 SysTick_Handler，累加毫秒计数 */
    for (uint32_t i = 0U; (i < 20000000U) && (f103_tick_now_ms() < 5U); i++) {
        __asm volatile("nop");
    }
    printf("[10] SysTick 中断：5 ms 计数到 %lu（≥5 说明中断通路正常）\r\n",
           (unsigned long)f103_tick_now_ms());

    printf("=== 探针结束 ===\r\n");

    for (;;) {
        SPIN(1000000U);
    }
}
