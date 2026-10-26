#ifndef F103_TICK_H
#define F103_TICK_H

#include <stdint.h>

/*
 * 最小 1 ms 时基：SysTick 自由运行 + 轮询 COUNTFLAG，不使能中断。
 *
 * 与 HAL_Delay 的区别：这里不依赖 HAL，也不改 NVIC；中断版本留给
 * 「定时器 / 中断」实验，届时 SysTick_Handler 里累加毫秒计数。
 */

/* 按当前 HCLK 配置 SysTick 为 1 ms 周期。会先调用 SystemCoreClockUpdate() */
void f103_tick_init(void);

/*
 * 中断模式：SysTick 每 1 ms 触发中断，由本模块的 SysTick_Handler 累加计数。
 *
 * 注意：本模块定义了 SysTick_Handler。若某个实验（例如接入 RTOS）需要自己
 * 接管该向量，必须改用轮询模式或把累加逻辑合并进自己的 Handler，
 * 否则会出现重复符号。
 */
void f103_tick_init_irq(void);

/* 自 f103_tick_init_irq() 起经过的毫秒数；轮询模式下恒为 0 */
uint32_t f103_tick_now_ms(void);

/* 等到下一次 1 ms 溢出。返回时至少跨过一个 1 ms 边界，不保证相位对齐 */
void f103_tick_wait_1ms(void);

/* 忙等指定毫秒数 */
void f103_tick_delay_ms(uint32_t ms);

#endif /* F103_TICK_H */
