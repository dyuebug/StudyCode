/**
 * F103_09：三种微秒级延时实现的对比（SysTick 递减计数 / TIM4 自由运行 / DWT 周期计数）
 *
 * 三者都只是"忙等一个计数器"，所以真正要比较的不是"谁更准"，
 * 而是分辨率、回卷周期、对 HAL/RTOS 时基的占用，以及调用开销：
 *
 *   SysTick  24 位递减，LOAD 由 HAL 的 1 ms 时基决定 → 回卷 1 ms，与 HAL/RTOS 争用
 *   TIM4     16 位递增，预分频到 1 MHz        → 回卷 65.536 ms，独立于 HAL
 *   DWT      32 位递增，直接数 HCLK 周期       → 回卷 536.87 s（8 MHz），不需要外设
 *
 * 裁判方式：用 DWT 量 SysTick 与 TIM4 的延时，再用 TIM4 量 DWT 的延时。
 * 前两行验证"被测方法"，最后一行验证"裁判本身"，避免自己量自己。
 *
 * 探针：PC13（板载 LED，低电平点亮）。每轮基准结束后以 1 ms 低 / 1 ms 高翻转，
 * 逻辑分析仪或示波器接 PC13 与 GND 即可核对真实脉宽。
 */
#include <stdint.h>
#include <stdio.h>

#include "stm32f1xx.h"
#include "f103_uart.h"

#define UART_BAUD   115200U

#define PROBE_PORT  GPIOC
#define PROBE_LOW()  (PROBE_PORT->BSRR = GPIO_BSRR_BR13)
#define PROBE_HIGH() (PROBE_PORT->BSRR = GPIO_BSRR_BS13)

typedef void (*delay_fn)(uint32_t us);

/* HCLK / 1 MHz：8 MHz 时为 8，即 1 us = 8 个核心周期 */
static uint32_t s_cycles_per_us;

/* ---- 初始化 ------------------------------------------------------------- */

static void probe_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

    /* PC13 推挽输出 2 MHz（CRH 的 bit[23:20]），空闲为高（LED 熄灭） */
    PROBE_PORT->CRH &= ~(GPIO_CRH_CNF13 | GPIO_CRH_MODE13);
    PROBE_PORT->CRH |= GPIO_CRH_MODE13_1;
    PROBE_HIGH();
}

/*
 * SysTick 用 HAL 的 1 ms 负载：LOAD = HCLK/1000 - 1，但不开中断。
 * 本实验 HCLK = 8 MHz，LOAD = 7999。
 *
 * 24 位上限约束的不是 LOAD（72 MHz 下 71999 也远小于 2^24），而是"单次计数窗口"：
 * 若把 LOAD 设成 0xFFFFFF 当自由计数器用，8 MHz 下 2^24/8e6 ≈ 2.097 s 就回卷。
 * 反过来说，改 LOAD 会连同 HAL_GetTick / RTOS 时基一起改掉——这正是
 * 下面 TIM4、DWT 两种方案存在的理由。
 */
static void systick_init_ms(void)
{
    SysTick->LOAD = (SystemCoreClock / 1000U) - 1U;
    SysTick->VAL  = 0U;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
}

/*
 * TIM4 时钟：APB1 预分频系数为 1 时 TIMxCLK = PCLK1；不为 1 时是 PCLK1 的 2 倍。
 * 本工程不配 RCC_CFGR，复位值 PPRE1 = 000（不分频），故 TIM4CLK = HCLK = 8 MHz。
 * 预分频 7 → 计数频率 1 MHz，一个计数正好 1 us。
 * ARR 取满 0xFFFF，回卷周期 65536 us；相比课程的 1 ms 回卷，减少一半的跨回卷处理。
 */
static void tim4_init_1mhz(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;

    TIM4->PSC = (uint16_t)(s_cycles_per_us - 1U);
    TIM4->ARR = 0xFFFFU;

    /* UG 把 PSC/ARR 立刻装入影子寄存器，否则要等第一次更新事件；
     * 它会顺带置 UIF，这里不使能中断，清掉即可，避免 SR 残留状态误导排查 */
    TIM4->EGR = TIM_EGR_UG;
    TIM4->SR  = 0U;

    TIM4->CR1 = TIM_CR1_CEN;
}

/*
 * Cortex-M3 的 DWT 周期计数器：需要先打开 DEMCR.TRCENA，再置 CYCCNTENA。
 * 与外设无关，直接数核心周期；32 位，8 MHz 下 2^32 / 8e6 ≈ 536.87 s 才回卷。
 * 返回是否真正使能成功——若读回为 0，后面的测量全都不可信。
 */
static int dwt_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    return (DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) != 0U;
}

/* ---- 三种微秒延时 ------------------------------------------------------- */

/*
 * SysTick 递减计数：told >= tnow 表示这一轮没有跨回卷；
 * 否则跨过 0 点，补上 LOAD+1（= HCLK/1000 个周期）。
 *
 * 课程的写法是 ticks = us * (LOAD+1) / 1000，在 72 MHz 下 LOAD+1 = 72000，
 * us ≥ 59653 时 us * 72000 溢出 32 位，延时直接变成接近 0。
 * 这里换成 us * HCLK/1e6，量纲相同但不会溢出到 536 秒以内。
 * us=0 时目标为 0，函数直接返回——调用开销就是最小开销。
 */
static void delay_systick_us(uint32_t us)
{
    const uint32_t target = us * s_cycles_per_us;
    const uint32_t load   = SysTick->LOAD;
    uint32_t told = SysTick->VAL;
    uint32_t counted = 0U;

    while (counted < target) {
        uint32_t tnow = SysTick->VAL;
        counted += (told >= tnow) ? (told - tnow) : (told + load + 1U - tnow);
        told = tnow;
    }
}

/* TIM4 递增计数，1 个计数 = 1 us，跨回卷时补 65536 */
static void delay_tim_us(uint32_t us)
{
    uint32_t last = TIM4->CNT;
    uint32_t counted = 0U;

    while (counted < us) {
        uint32_t now = TIM4->CNT;
        counted += (now >= last) ? (now - last) : (0x10000U - last + now);
        last = now;
    }
}

/*
 * DWT 周期计数：32 位无符号减法天然处理单次回卷，
 * 前提是两次读取之间回卷不超过一圈（8 MHz 下约 536 s）。
 */
static void delay_dwt_us(uint32_t us)
{
    const uint32_t target = us * s_cycles_per_us;
    const uint32_t start  = DWT->CYCCNT;

    while ((uint32_t)(DWT->CYCCNT - start) < target) {
    }
}

/* ---- 测量 --------------------------------------------------------------- */

/* 用 DWT 作裁判：返回被测延时实际消耗的核心周期 */
static uint32_t measure_by_dwt(delay_fn fn, uint32_t us)
{
    uint32_t start = DWT->CYCCNT;

    fn(us);
    return (uint32_t)(DWT->CYCCNT - start);
}

/* 用 TIM4 作裁判：返回被测延时实际消耗的微秒数（1 us 分辨率，≤65.5 ms 不跨圈） */
static uint32_t measure_by_tim_us(delay_fn fn, uint32_t us)
{
    uint32_t start = TIM4->CNT;

    fn(us);
    {
        uint32_t now = TIM4->CNT;
        return (now >= start) ? (now - start) : (0x10000U - start + now);
    }
}

static void print_row(uint32_t req_us, const char *name, delay_fn fn)
{
    uint32_t cycles = measure_by_dwt(fn, req_us);
    uint32_t us_x10 = (cycles * 10U) / s_cycles_per_us;
    int32_t  err_x10 = (int32_t)us_x10 - (int32_t)(req_us * 10U);
    int32_t  abs_x10 = (err_x10 < 0) ? -err_x10 : err_x10;

    printf("%6lu  %-8s  %10lu  %5lu.%lu  %c%ld.%ld\r\n",
           (unsigned long)req_us, name,
           (unsigned long)cycles,
           (unsigned long)(us_x10 / 10U), (unsigned long)(us_x10 % 10U),
           (err_x10 < 0) ? '-' : '+',
           (long)(abs_x10 / 10), (long)(abs_x10 % 10));
}

static void run_benchmark(void)
{
    static const struct {
        uint32_t    us;
        const char *name;
        delay_fn    fn;
    } cases[] = {
        {     0U, "SysTick", delay_systick_us },
        {     1U, "SysTick", delay_systick_us },
        {    10U, "SysTick", delay_systick_us },
        {   100U, "SysTick", delay_systick_us },
        {  1000U, "SysTick", delay_systick_us },
        { 10000U, "SysTick", delay_systick_us },
        {     0U, "TIM4",    delay_tim_us      },
        {     1U, "TIM4",    delay_tim_us      },
        {    10U, "TIM4",    delay_tim_us      },
        {   100U, "TIM4",    delay_tim_us      },
        {  1000U, "TIM4",    delay_tim_us      },
        { 10000U, "TIM4",    delay_tim_us      },
        {     0U, "DWT",     delay_dwt_us      },
        {     1U, "DWT",     delay_dwt_us      },
        {    10U, "DWT",     delay_dwt_us      },
        {   100U, "DWT",     delay_dwt_us      },
        {  1000U, "DWT",     delay_dwt_us      },
        { 10000U, "DWT",     delay_dwt_us      },
    };

    printf("\r\n请求us    方法        DWT周期     实测us   误差us\r\n");
    printf("----------------------------------------------------\r\n");

    for (uint32_t i = 0U; i < (sizeof(cases) / sizeof(cases[0])); i++) {
        print_row(cases[i].us, cases[i].name, cases[i].fn);
    }

    /*
     * 交叉校验：DWT 延时用 TIM4 量。两套裁判来自同一个 HCLK，
     * 所以这只证明"计数换算一致"，不能替代外部仪器；
     * 真正的绝对精度要用 PC13 脉宽 + 逻辑分析仪确认。
     */
    {
        uint32_t tim_us = measure_by_tim_us(delay_dwt_us, 1000U);
        printf("\r\n交叉校验：delay_dwt_us(1000) 由 TIM4 裁判测得 %lu us\r\n",
               (unsigned long)tim_us);
    }
}

int main(void)
{
    probe_init();

    /*
     * 必须先刷新 SystemCoreClock：CubeMX 的 SystemInit() 不设置它，
     * 变量停在初值 16 MHz，而复位后实际是 HSI 8 MHz。
     * 本实验不调用 f103_tick_init()，缺了这一步 UART 波特率和全部换算都会错一倍。
     */
    SystemCoreClockUpdate();
    s_cycles_per_us = SystemCoreClock / 1000000U;

    f103_uart_init(UART_BAUD);
    systick_init_ms();
    tim4_init_1mhz();
    int dwt_ok = dwt_init();

    printf("\r\n[F103_09] HCLK = %lu Hz，1 us = %lu cycles\r\n",
           (unsigned long)SystemCoreClock, (unsigned long)s_cycles_per_us);
    printf("SysTick : LOAD = %lu（1 ms），CTRL = 0x%lX\r\n",
           (unsigned long)SysTick->LOAD, (unsigned long)SysTick->CTRL);
    printf("TIM4    : PSC = %lu，ARR = %lu（1 MHz，65.536 ms 回卷）\r\n",
           (unsigned long)TIM4->PSC, (unsigned long)TIM4->ARR);
    printf("DWT     : CYCCNTENA 读回 = %d\r\n", dwt_ok ? 1 : 0);

    if (dwt_ok == 0) {
        printf("DWT 未使能，下表的周期列将恒为 0，请先排查 DEMCR/CTRL 写入\r\n");
    }

    run_benchmark();

    printf("\r\n进入探针模式：PC13 = 1 ms 低 / 1 ms 高，每 500 次打印一行\r\n");

    uint32_t pulses = 0U;
    for (;;) {
        PROBE_LOW();
        delay_dwt_us(1000U);
        PROBE_HIGH();
        delay_dwt_us(1000U);

        if (++pulses >= 500U) {
            pulses = 0U;
            printf("[probe] PC13 已翻转 500 次（理论 1000 ms）  DWT = %lu\r\n",
                   (unsigned long)DWT->CYCCNT);
        }
    }
}
