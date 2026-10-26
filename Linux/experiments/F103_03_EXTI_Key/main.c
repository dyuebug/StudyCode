/**
 * 实验 03：外部中断读按键（EXTI14 + NVIC）
 *
 * 与实验 02 是同一个按键、同一个 LED，区别在「怎么发现按下」：
 *   实验 02：主循环每 1 ms 采样一次 IDR，靠连续一致来消抖（轮询）
 *   实验 03：CPU 在硬件层面被 EXTI 唤醒，ISR 里只记时间戳与事件数（中断）
 *
 * 本实验刻意只在中断里记事件、由主循环点灯：
 * 中断服务程序越短，被拉长的临界区越少，也更不容易与其他上下文争共享资源。
 *
 * 硬件：LED = PC13（低电平点亮）；KEY = PB14（外部上拉，按下接地）
 *
 * 与课程 0601_key_isr 的差异：
 *   课程用 GPIO_MODE_IT_RISING_FALLING（双边沿），在回调里读 IDR 分辨按下/松开，
 *   并在回调里直接写 LED。本实验只开下降沿，用毫秒时间戳做 20 ms 去抖，
 *   点灯动作延迟到主循环。
 */

#include <stdint.h>
#include "stm32f1xx.h"
#include "f103_tick.h"

#define LED_PORT GPIOC
#define KEY_PORT GPIOB

#define KEY_DEBOUNCE_MS 20U

/* 中断与主循环共享，必须 volatile：否则编译器可能把它缓存在寄存器里 */
static volatile uint32_t s_key_events;     /* 已去抖的按下事件计数 */
static volatile uint32_t s_last_event_ms;  /* 上一次被采纳的时刻 */

static void led_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

    LED_PORT->CRH &= ~(GPIO_CRH_CNF13 | GPIO_CRH_MODE13);
    LED_PORT->CRH |= GPIO_CRH_MODE13_1;     /* 推挽输出，2 MHz */

    LED_PORT->BSRR = GPIO_BSRR_BS13;        /* 初始熄灭 */
}

static void led_toggle(void)
{
    if ((LED_PORT->ODR & GPIO_ODR_ODR13) != 0U) {
        LED_PORT->BSRR = GPIO_BSRR_BR13;
    } else {
        LED_PORT->BSRR = GPIO_BSRR_BS13;
    }
}

static void key_exti_init(void)
{
    /* AFIO 也必须开时钟：EXTI 与引脚的对应关系由 AFIO 的 EXTICR 决定，
     * 只开 GPIOB 时钟而漏掉 AFIOEN，是本配置最常见的「中断不进」原因。 */
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;

    /* 引脚本身仍按输入配置：EXTI 只是把该引脚的电平变化接到中断线上，
     * 不改变引脚的输入/输出属性。 */
    KEY_PORT->CRH &= ~(GPIO_CRH_CNF14 | GPIO_CRH_MODE14);
    KEY_PORT->CRH |= GPIO_CRH_CNF14_0;      /* CNF=01 浮空输入 */

    /* 把 EXTI14 的输入源选为 Port B。
     * EXTICR[3] 对应 EXTI12~15，每线 4 bit，EXTI14 占 bit[11:8]；取值 0001 = PB */
    AFIO->EXTICR[3] &= ~AFIO_EXTICR4_EXTI14;
    AFIO->EXTICR[3] |=  AFIO_EXTICR4_EXTI14_PB;

    /* 配置期间先屏蔽，避免清挂起位之前就进中断 */
    EXTI->IMR  &= ~EXTI_IMR_MR14;
    EXTI->FTSR |=  EXTI_FTSR_TR14;          /* 下降沿触发 = 按下的那一刻 */
    EXTI->RTSR &= ~EXTI_RTSR_TR14;          /* 松开不触发 */
    EXTI->PR    =  EXTI_PR_PR14;            /* 写 1 清除可能的残留挂起位 */
    EXTI->IMR  |=  EXTI_IMR_MR14;

    NVIC_SetPriority(EXTI15_10_IRQn, 2U);
    NVIC_EnableIRQ(EXTI15_10_IRQn);
}

/*
 * EXTI10~EXTI15 共用一个中断向量，进入后必须先确认是自己的线。
 * 忘了清 PR 的表现是：一进中断就再也出不来（电平型挂起一直有效）。
 */
void EXTI15_10_IRQHandler(void)
{
    if ((EXTI->PR & EXTI_PR_PR14) == 0U) {
        return;
    }
    EXTI->PR = EXTI_PR_PR14;                /* 写 1 清除，写 0 无效 */

    /* 机械抖动会在几毫秒内产生多个下降沿。
     * 用「距上次采纳是否满 20 ms」做时间窗过滤，比在中断里 delay 更安全。
     * 无符号相减天然支持计数器回绕。 */
    uint32_t now = f103_tick_now_ms();
    if ((now - s_last_event_ms) >= KEY_DEBOUNCE_MS) {
        s_last_event_ms = now;
        s_key_events++;
    }
}

int main(void)
{
    led_init();
    key_exti_init();
    f103_tick_init_irq();     /* 中断模式：为去抖提供毫秒时间戳 */

    for (;;) {
        uint32_t pending;

        /* 读-清 事件计数不能与中断里的 ++ 交错，否则会丢事件。
         * 临界区只有三条指令，代价远小于在中断里做 IO。 */
        __disable_irq();
        pending = s_key_events;
        s_key_events = 0U;
        __enable_irq();

        while (pending-- > 0U) {
            led_toggle();
        }
    }
}
