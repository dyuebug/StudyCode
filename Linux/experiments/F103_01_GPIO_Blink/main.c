/**
 * 实验 01：寄存器级 LED 闪烁（STM32F103C8T6 / DShanMCU-F103 板）
 *
 * 目标：用最少的代码验证四条链路 —— 交叉编译、烧写、系统时钟、GPIO 输出。
 * 不依赖 HAL，不使用中断，不配置 PLL（保持复位后的 HSI）。
 *
 * LED 接法：PC13，低电平点亮（阴极经限流电阻接 PC13）。
 *
 * 注意 SystemCoreClock：
 *   CubeMX 版本的 system_stm32f1xx.c 里 SystemInit() 只处理向量表，
 *   并不调用 SystemCoreClockUpdate()，该变量停在初值 16000000。
 *   所以这里主动调用一次，按 RCC->CFGR 反推真实频率（复位后为 HSI 8 MHz），
 *   否则 1 ms 的 SysTick 会被算成 2 ms，闪烁周期整体慢一倍。
 */

#include <stdint.h>
#include "stm32f1xx.h"

#define LED_PORT GPIOC

static void led_init(void)
{
    /* 1. 使能 GPIOC 时钟（APB2ENR 的 IOPCEN，bit4）。
     *    外设寄存器在时钟关闭时不响应写入，这一步漏掉的现象是「代码对、灯不亮」。 */
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

    /* 2. PC13 配置为「推挽输出、2 MHz」。
     *    CRH 管理 Pin8~Pin15，每引脚 4 bit：CNF[1:0] | MODE[1:0]。
     *    推挽输出 CNF=00；输出模式 MODE=10 表示 2 MHz。
     *    先清零再置位：复位默认 0x4444_4444（浮空输入），不清零会残留 CNF 位。 */
    LED_PORT->CRH &= ~(GPIO_CRH_CNF13 | GPIO_CRH_MODE13);
    LED_PORT->CRH |= GPIO_CRH_MODE13_1;

    /* 3. 上电初始状态熄灭。用 BSRR 而非 ODR 读改写：BSRR 置位/复位是原子的，
     *    不会读回再写回 ODR，也就不会误改其他引脚。 */
    LED_PORT->BSRR = GPIO_BSRR_BS13;
}

static void systick_init_1ms(void)
{
    SystemCoreClockUpdate();

    /* LOAD = 周期 - 1：递减计数器从 LOAD 数到 0 共 LOAD+1 个时钟 */
    SysTick->LOAD = (SystemCoreClock / 1000U) - 1U;
    SysTick->VAL  = 0U;  /* 写任意值都会清零当前计数并清 COUNTFLAG */
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk   /* 时钟源 = HCLK，而非 HCLK/8 */
                  | SysTick_CTRL_ENABLE_Msk;
}

static void delay_ms(uint32_t ms)
{
    while (ms-- > 0U) {
        /* COUNTFLAG 在计数器回到 0 时置位，读 CTRL 即清零。
         * 这里轮询而不是开中断：实验 01 不引入中断路径，变量最少。 */
        while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0U) {
        }
    }
}

int main(void)
{
    led_init();
    systick_init_1ms();

    for (;;) {
        LED_PORT->BSRR = GPIO_BSRR_BR13;  /* 低电平：点亮 */
        delay_ms(500);
        LED_PORT->BSRR = GPIO_BSRR_BS13;  /* 高电平：熄灭 */
        delay_ms(500);
    }
}
