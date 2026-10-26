#include "f103_tick.h"
#include "stm32f1xx.h"

void f103_tick_init(void)
{
    /*
     * 必须先刷新 SystemCoreClock。
     * CubeMX 版本的 system_stm32f1xx.c 中 SystemInit() 只处理向量表重定位，
     * 不设置该变量，它停在初值 16000000，而复位后实际是 HSI 8 MHz。
     * 不刷新会让 1 ms 变成 2 ms，所有基于它的延时整体偏慢一倍。
     * 后续若配置 PLL，改完时钟必须再调一次本函数。
     */
    SystemCoreClockUpdate();

    /* LOAD = 周期 - 1：递减计数器从 LOAD 数到 0 共 LOAD+1 个时钟 */
    SysTick->LOAD = (SystemCoreClock / 1000U) - 1U;
    SysTick->VAL  = 0U;   /* 写任意值都会清零当前计数并清 COUNTFLAG */
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk   /* 时钟源 = HCLK，而非 HCLK/8 */
                  | SysTick_CTRL_ENABLE_Msk;
}

void f103_tick_wait_1ms(void)
{
    /* COUNTFLAG 在计数器回到 0 时置位，读 CTRL 即清零。
     * 若进入时标志已置位，本函数会立刻返回：调用方得到的是
     * 「至少跨过一个 1 ms 边界」，适合做节拍循环，不适合做严格相位计时。 */
    while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0U) {
    }
}

void f103_tick_delay_ms(uint32_t ms)
{
    while (ms-- > 0U) {
        f103_tick_wait_1ms();
    }
}

/* ---- 中断模式 ---------------------------------------------------------- */

static volatile uint32_t s_ms;

/* 覆盖启动文件中 weak 的 SysTick_Handler。中断里只做计数，不做任何 IO */
void SysTick_Handler(void)
{
    s_ms++;
}

void f103_tick_init_irq(void)
{
    SystemCoreClockUpdate();
    s_ms = 0U;

    SysTick->LOAD = (SystemCoreClock / 1000U) - 1U;
    SysTick->VAL  = 0U;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk
                  | SysTick_CTRL_TICKINT_Msk    /* 计到 0 时产生中断 */
                  | SysTick_CTRL_ENABLE_Msk;
}

uint32_t f103_tick_now_ms(void)
{
    return s_ms;
}
