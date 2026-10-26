/**
 * 实验 02：按键读入与消抖（PB14 按键翻转 PC13 LED）
 *
 * 相比实验 01 新增三件事：
 *   1. 输入路径：CRH 的 CNF/MODE 配置 + IDR 读取，而不是 BSRR 输出。
 *   2. 消抖：机械按键的触点在 5~20 ms 内反复通断，直接读 IDR 会得到多次边沿。
 *   3. 边沿检测：按一次翻转一次，而不是「按住才亮」。
 *
 * 硬件（DShanMCU-103）：
 *   LED  PC13  低电平点亮
 *   KEY  PB14  外接上拉到 3.3 V，按下接地；因此松开为 1，按下为 0。
 *              课程 0506/0507 两个示例对 PB14 都用 GPIO_NOPULL（浮空输入），
 *              说明上拉电阻在板上。
 */

#include <stdint.h>
#include "stm32f1xx.h"
#include "f103_tick.h"

#define LED_PORT GPIOC
#define KEY_PORT GPIOB

#define KEY_IDLE_LEVEL   1U    /* 外部上拉，松开为高 */
#define KEY_DEBOUNCE_MS  20U   /* 抖动窗口：大于典型抖动时长，小于人的按键操作 */

static void led_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

    /* PC13：推挽输出、2 MHz。CRH 管 Pin8~15，每脚 4 bit：CNF[1:0] | MODE[1:0] */
    LED_PORT->CRH &= ~(GPIO_CRH_CNF13 | GPIO_CRH_MODE13);
    LED_PORT->CRH |= GPIO_CRH_MODE13_1;

    LED_PORT->BSRR = GPIO_BSRR_BS13;   /* 初始熄灭 */
}

static void key_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;

    /* PB14：浮空输入 —— CNF=01、MODE=00。
     * 之所以不用「上拉输入」，是因为板上已有外部上拉；两者叠加只是多加一个
     * 40 K 左右的并联电阻，不影响电平，但会让「上拉到底由谁提供」这件事变模糊。
     * 换到没有外部上拉的板子时，改成 CNF=10 并把 ODR 的 bit14 置 1
     * （STM32F1 的输入上下拉方向由 ODR 位选择，这一点与 F4 不同）。 */
    KEY_PORT->CRH &= ~(GPIO_CRH_CNF14 | GPIO_CRH_MODE14);
    KEY_PORT->CRH |= GPIO_CRH_CNF14_0;      /* CNF = 01：浮空输入 */
}

static void led_toggle(void)
{
    /* 读 ODR 当前电平再写 BSRR 完成翻转。只需主循环一处写该引脚，
     * 因此不必为「读-判-写」加临界区。 */
    if ((LED_PORT->ODR & GPIO_ODR_ODR13) != 0U) {
        LED_PORT->BSRR = GPIO_BSRR_BR13;    /* 当前高 → 拉低（点亮） */
    } else {
        LED_PORT->BSRR = GPIO_BSRR_BS13;    /* 当前低 → 拉高（熄灭） */
    }
}

/*
 * 每 1 ms 调用一次。返回 1 表示出现一次「已稳定按下」的边沿事件。
 *
 * 做法：连续 KEY_DEBOUNCE_MS 次采样一致才承认电平真的变了。
 * 不采用 delay(20) 死等，因为那会在抖动窗口内阻塞主循环，
 * 一旦后面还有别的周期任务（串口收包、显示刷新），就会被按键拖住。
 */
static uint8_t key_poll_pressed_event(void)
{
    static uint8_t  stable   = KEY_IDLE_LEVEL;   /* 已确认的稳定电平 */
    static uint8_t  last_raw = KEY_IDLE_LEVEL;   /* 上一毫秒的原始采样 */
    static uint16_t same_ms  = 0U;               /* 原始电平连续不变的毫秒数 */

    uint8_t raw = ((KEY_PORT->IDR & GPIO_IDR_IDR14) != 0U) ? 1U : 0U;

    if (raw != last_raw) {
        last_raw = raw;
        same_ms  = 0U;          /* 刚发生跳变，抖动用时清零 */
        return 0U;
    }

    if (same_ms < KEY_DEBOUNCE_MS) {
        same_ms++;
    }

    if ((same_ms >= KEY_DEBOUNCE_MS) && (raw != stable)) {
        stable  = raw;
        same_ms = 0U;
        return (stable == 0U) ? 1U : 0U;   /* 稳定为低 = 按下 */
    }

    return 0U;
}

int main(void)
{
    led_init();
    key_init();
    f103_tick_init();

    for (;;) {
        f103_tick_wait_1ms();
        if (key_poll_pressed_event() != 0U) {
            led_toggle();
        }
    }
}
