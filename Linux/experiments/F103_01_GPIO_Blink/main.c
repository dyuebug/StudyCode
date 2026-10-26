/**
 * 实验 01：寄存器级 LED 闪烁（STM32F103C8T6 / DShanMCU-F103 板）
 *
 * 目标：用最少的代码验证四条链路 —— 交叉编译、烧写、系统时钟、GPIO 输出。
 * 不依赖 HAL，不使用中断，不配置 PLL（保持复位后的 HSI 8 MHz）。
 *
 * LED 接法：PC13，低电平点亮（阴极经 1 K 电阻接 PC13，阳极接 3.3 V）。
 * 依据：课程手册《HAL 快速入门与项目实战（基于 DshanMCU-103）》§5.3.1。
 *
 * 1 ms 时基由 F103_common 的 f103_tick 提供，其中已处理
 * 「SystemInit() 不设置 SystemCoreClock」这一陷阱。
 */

#include <stdint.h>
#include "stm32f1xx.h"
#include "f103_tick.h"

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

int main(void)
{
    led_init();
    f103_tick_init();

    for (;;) {
        LED_PORT->BSRR = GPIO_BSRR_BR13;  /* 低电平：点亮 */
        f103_tick_delay_ms(500);
        LED_PORT->BSRR = GPIO_BSRR_BS13;  /* 高电平：熄灭 */
        f103_tick_delay_ms(500);
    }
}
