/**
 * F103_06：TIM2 三通道 PWM 驱动 RGB 呼吸灯
 *
 * 硬件（DShanMCU-103 板载共阳 RGB LED，课程 1003_pwm_color_led）：
 *   R = TIM2_CH3 = PA2
 *   G = TIM2_CH1 = PA15
 *   B = TIM2_CH2 = PB3
 * 共阳极接 3.3 V，引脚拉低才点亮 —— 因此输出极性必须取反（CCxP = 1）。
 *
 * 时钟：TIM2 挂在 APB1。PPRE1 = 1 时 TIM2CLK = PCLK1 = 8 MHz，
 *       f_pwm = 8e6 / ((PSC+1) * (ARR+1)) = 8e6 / (18 * 2000) ≈ 222 Hz。
 *
 * 关键前置：PA15 = JTDI、PB3 = JTDO，复位后属于 JTAG。
 * 必须把 AFIO->MAPR 的 SWJ_CFG 改为 010（关 JTAG、留 SWD），
 * 否则 PA15/PB3 上的 PWM 波形永远到不了引脚，现象是"只有红色通道亮"。
 */
#include <stdint.h>

#include "stm32f1xx.h"
#include "f103_tick.h"

#define PWM_PSC        17U     /* 分频后 8 MHz / 18 ≈ 444 kHz 计数频率 */
#define PWM_ARR        1999U   /* 2000 个计数一周期 -> 222 Hz，占空比 2000 级 */
#define BREATH_STEP_MS 20U

static void color_led_gpio_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;

    /*
     * 复用推挽输出 50 MHz = CNF 10 + MODE 11 = 0xB。
     * PA2 在 CRL 的 bit[11:8]，PA15 在 CRH 的 bit[31:28]，PB3 在 CRL 的 bit[15:12]。
     */
    GPIOA->CRL = (GPIOA->CRL & ~(0xFU << 8))  | (0xBU << 8);
    GPIOA->CRH = (GPIOA->CRH & ~(0xFU << 28)) | (0xBU << 28);
    GPIOB->CRL = (GPIOB->CRL & ~(0xFU << 12)) | (0xBU << 12);

    /* 释放 JTAG：SWJ_CFG = 010，JTAG-DP 关闭、SW-DP 保留，PA13/PA14 仍可调试 */
    AFIO->MAPR = (AFIO->MAPR & ~AFIO_MAPR_SWJ_CFG) | AFIO_MAPR_SWJ_CFG_JTAGDISABLE;
}

static void tim2_pwm_init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    TIM2->PSC = PWM_PSC;
    TIM2->ARR = PWM_ARR;

    /*
     * PWM1 模式：CNT < CCR 时参考信号有效（配合 CCxP=1，引脚为低 = 点亮）。
     * OCxPE 使能 CCR 预装载，新占空比在下一个更新事件整体生效，避免半周期改值造成毛刺。
     */
    TIM2->CCMR1 = TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1PE
                | TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2PE;
    TIM2->CCMR2 = TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3PE;

    /* 共阳 LED：三个通道都取反极性并使能输出 */
    TIM2->CCER = TIM_CCER_CC1E | TIM_CCER_CC1P
               | TIM_CCER_CC2E | TIM_CCER_CC2P
               | TIM_CCER_CC3E | TIM_CCER_CC3P;

    TIM2->CR1 = TIM_CR1_ARPE;   /* ARR 也预装载 */
    TIM2->EGR = TIM_EGR_UG;     /* 立即把 PSC/ARR 装入影子寄存器，并从 0 开始计数 */
    TIM2->CR1 |= TIM_CR1_CEN;
}

/* 0..255 的亮度映射到三个通道；通道与引脚对应关系见文件头 */
static void color_led_set(uint8_t r, uint8_t g, uint8_t b)
{
    TIM2->CCR3 = ((uint32_t)r * PWM_ARR) / 255U;   /* R = CH3 = PA2 */
    TIM2->CCR1 = ((uint32_t)g * PWM_ARR) / 255U;   /* G = CH1 = PA15 */
    TIM2->CCR2 = ((uint32_t)b * PWM_ARR) / 255U;   /* B = CH2 = PB3 */
}

int main(void)
{
    f103_tick_init();
    color_led_gpio_init();
    tim2_pwm_init();

    for (;;) {
        for (uint32_t i = 0U; i <= 255U; i++) {
            color_led_set((uint8_t)i, (uint8_t)i, (uint8_t)i);
            f103_tick_delay_ms(BREATH_STEP_MS);
        }
        for (uint32_t i = 255U; i > 0U; i--) {
            color_led_set((uint8_t)i, (uint8_t)i, (uint8_t)i);
            f103_tick_delay_ms(BREATH_STEP_MS);
        }
    }
}
