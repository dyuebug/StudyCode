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

    /* ---- 11. IWDG（独立看门狗）----
     * 顺序是"先写后读"，读回值决定能否在仿真里验证看门狗。
     * 注意：KR 写 0xCCCC 在真芯片上会真正启动看门狗，所以探针只允许在 QEMU 里跑；
     * 真板上不要在"没有喂狗循环"的代码里启动 IWDG。写完立刻喂一次，缩短超时窗口。 */
    IWDG->KR  = 0x5555U;                 /* 解除 PR/RLR 写保护 */
    IWDG->PR  = 0x04U;                   /* 预分频 ÷64 */
    IWDG->RLR = 0x0FFFU;                 /* 重装值 */
    uint32_t iwdg_pr  = IWDG->PR;
    uint32_t iwdg_rlr = IWDG->RLR;
    uint32_t iwdg_sr  = IWDG->SR;
    IWDG->KR = 0xCCCCU;                  /* 启动 */
    IWDG->KR = 0xAAAAU;                  /* 立刻喂一次 */
    printf("[11] IWDG：写 PR=4 RLR=0x0FFF，读回 PR=%lu RLR=%lu SR=0x%08lX"
           "（读回 0 说明未建模，看门狗计时与复位不可验证）\r\n",
           (unsigned long)iwdg_pr, (unsigned long)iwdg_rlr, (unsigned long)iwdg_sr);

    /* ---- 12. 复位原因寄存器 ---- */
    uint32_t csr = RCC->CSR;
    printf("[12] RCC->CSR = 0x%08lX：IWDGRSTF=%lu PORRSTF=%lu PINRSTF=%lu SFTRSTF=%lu\r\n",
           (unsigned long)csr,
           (unsigned long)((csr & RCC_CSR_IWDGRSTF) ? 1U : 0U),
           (unsigned long)((csr & RCC_CSR_PORRSTF) ? 1U : 0U),
           (unsigned long)((csr & RCC_CSR_PINRSTF) ? 1U : 0U),
           (unsigned long)((csr & RCC_CSR_SFTRSTF) ? 1U : 0U));

    /* ---- 13. USART2：是否存在可用的第二个串口（二进制下行通道的前提） ----
     * 只写不读：把 8 字节特征串推给 USART2，由 run.sh 用
     * `-serial file:logs/probe_usart2.bin` 收集宿主侧是否收到。
     * 文件非空 → QEMU 把 USART2 接到了第二个串口后端，二进制下行可以走独立通道，
     *   不必与 USART1 的 ASCII 日志争同一条流；
     * 文件为空 → USART2 没有宿主出口，二进制下行只能复用 USART1（会破坏日志可解析性）。
     * 真板上这一段只是让 UART 空转，不启动任何有副作用的设备。 */
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    USART2->CR1 = 0U;                    /* 配置期间先关 UE */
    USART2->CR2 = 0U;
    USART2->CR3 = 0U;
    USART2->BRR = 0x45U;                 /* 仅给合法值；QEMU 不按 baud 排字节 */
    USART2->CR1 = USART_CR1_UE | USART_CR1_TE;

    {
        static const uint8_t pattern[8] = { 0xAAU, 0x55U, 0x13U, 0x00U,
                                            0x04U, 0xDEU, 0xADU, 0xBEU };
        uint32_t written = 0U;

        for (uint32_t i = 0U; i < sizeof(pattern); i++) {
            uint32_t guard = 0U;

            while (((USART2->SR & USART_SR_TXE) == 0U) && (guard < 100000U)) {
                guard++;
            }
            if (guard >= 100000U) {
                break;                   /* TXE 永不来：直接记录，不挂死 */
            }
            USART2->DR = pattern[i];
            written++;
        }

        printf("[13] USART2：接受 %lu/8 字节待发，SR=0x%08lX"
               "（宿主 logs/probe_usart2.bin 非空即说明第二串口可用）\r\n",
               (unsigned long)written, (unsigned long)USART2->SR);
    }

    printf("=== 探针结束 ===\r\n");

    for (;;) {
        SPIN(1000000U);
    }
}
