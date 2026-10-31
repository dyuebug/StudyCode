/*
 * lab_timer_calc.c —— 定时器配置与计时函数的主机端校验（教材第 10 章）
 *
 * 不接板也能查清七件事：
 *   1. 本工程各定时器的 PSC/ARR 各自变成多少频率、多少占空比（含驱动对 CubeMX 值的覆盖）；
 *   2. APB1 分频器不为 1 时，定时器时钟为什么是 PCLK1 的两倍（HAL_InitTick 的算术）；
 *   3. udelay/mdelay 里的 "reload 对应 1ms" 差一位，累计误差有多大；
 *   4. "占空比"在 OCPolarity=LOW 下有两个口径（引脚高电平 vs 负载导通）；
 *   5. system_get_ns 先读 tick 再读 CNT，跨越计数器回卷时误差多大、怎么修；
 *   6. PA8 / TIM1_CH1 被无源蜂鸣器和红外发射器抢用；
 *   7. CubeMX 把定时器时钟使能分散在 Base_MspInit 与 PWM_MspInit，驱动漏调谁就不亮。
 *
 * 数值全部抄自仓库内源码，不凭记忆：
 *   Core/Src/tim.c（TIM1/2/3 的 MX_TIMx_Init 与 HAL_TIM_*_MspInit）
 *   Core/Src/stm32f1xx_hal_timebase_tim.c（TIM4 时基）
 *   Drivers/DshanMCU-F103/driver_timer.c（udelay/mdelay/system_get_ns）
 *   Drivers/DshanMCU-F103/driver_passive_buzzer.c、driver_ir_sender.c、
 *   driver_color_led.c、driver_motor.c
 *
 * 构建（宿主 gcc，不链接 HAL）：
 *   gcc -std=c11 -Wall -Wextra -o lab_timer_calc lab_timer_calc.c
 *   ./lab_timer_calc
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#define HCLK_HZ     72000000u
#define PCLK1_HZ    36000000u   /* APB1 = HCLK/2（main.c: RCC_HCLK_DIV2） */
#define PCLK2_HZ    72000000u   /* APB2 = HCLK/1 */

/* ============ 1/2. 定时器时钟与分频 ============ */

/* 复刻 stm32f1xx_hal_timebase_tim.c:57-70 的规则 */
static uint32_t tim_clock_for_apb1(uint32_t pclk1, int apb1_div_is_1)
{
    return apb1_div_is_1 ? pclk1 : 2u * pclk1;
}

typedef struct {
    const char *who;          /* 谁写的这组参数 */
    uint32_t    timclk;       /* 定时器输入时钟 CK_PSC */
    uint32_t    psc;
    uint32_t    arr;
    uint32_t    pulse;
    int         polarity_high;
    const char *note;
} timer_cfg_t;

/*
 * PWM1 模式下"CNT < CCR 期间通道有效"，有效时的引脚电平由 OCPolarity 决定：
 *   TIM_OCPOLARITY_HIGH（CCxP=0）→ 有效 = 高电平
 *   TIM_OCPOLARITY_LOW （CCxP=1）→ 有效 = 低电平
 * 所以 pulse/(ARR+1) 是"有效区占比"，不是"引脚高电平占比"。
 */
static void report(const timer_cfg_t *t)
{
    uint32_t ck_cnt = t->timclk / (t->psc + 1u);
    uint32_t ticks  = t->arr + 1u;                 /* ARR 是"从 0 数到 ARR"，共 ARR+1 个计数 */
    double   freq   = (double)ck_cnt / (double)ticks;
    double   active = 100.0 * (double)t->pulse / (double)ticks;
    double   pin_hi = t->polarity_high ? active : 100.0 - active;

    printf("  %-28s PSC=%-4u ARR=%-5u Pulse=%-5u %-4s f=%9.2f Hz  有效区=%6.2f%%  引脚高=%6.2f%%  引脚低=%6.2f%%\n",
           t->who, t->psc, t->arr, t->pulse,
           t->polarity_high ? "HIGH" : "LOW", freq, active, pin_hi, 100.0 - pin_hi);
    if (t->note)
        printf("      %s\n", t->note);
}

/* ============ 3. udelay 的 tick 算术 ============ */

/*
 * driver_timer.c:81-83：
 *     uint32_t reload = __HAL_TIM_GET_AUTORELOAD(hHalTim);   // = ARR = 999
 *     ticks = us * reload / (1000);                          // 假设 reload 对应 1ms
 * 但 CNT 从 0 数到 ARR 共 ARR+1 个计数，所以"1 ms"应该是 ARR+1。
 */
static uint32_t ticks_as_written(int us, uint32_t arr)
{
    return (uint32_t)us * arr / 1000u;
}

static uint32_t ticks_if_fixed(int us, uint32_t arr)
{
    return (uint32_t)us * (arr + 1u) / 1000u;
}

/* ============ 5. system_get_ns 的两次读 ============ */

/*
 * driver_timer.c:141-150（原文件 138-151 行）：
 *     uint64_t ns = HAL_GetTick();            // ① 先读 ms
 *     cnt    = __HAL_TIM_GET_COUNTER(hHalTim); // ② 再读 CNT
 *     reload = __HAL_TIM_GET_AUTORELOAD(hHalTim);
 *     ns = ns*1000000 + cnt*1000000/reload;
 * ①② 之间若发生更新事件（uwTick+1、CNT 归 0），两者就属于不同的毫秒。
 */
typedef struct {
    uint32_t tick;   /* uwTick */
    uint32_t cnt;    /* TIM4 CNT，0..ARR */
} snapshot_t;

static uint64_t ns_from(snapshot_t s, uint32_t arr)
{
    uint64_t ns = (uint64_t)s.tick * 1000000u;
    ns += (uint64_t)s.cnt * 1000000u / arr;   /* 驱动就是用 ARR 做分母，不是 ARR+1 */
    return ns;
}

int main(void)
{
    const uint32_t tim_apb2 = PCLK2_HZ;                        /* TIM1 */
    const uint32_t tim_apb1 = tim_clock_for_apb1(PCLK1_HZ, 0); /* TIM2/3/4 */

    printf("== 1. 各定时器的实际配置（后写者覆盖先写者）==\n");
    {
        timer_cfg_t t[] = {
            /* TIM1：CubeMX 初值，随后被两个驱动分别改写 */
            {"tim.c MX_TIM1_Init",       tim_apb2, 71,  999,  499, 1, NULL},
            {"PassiveBuzzer_Init",       tim_apb2, 71,  999,  499, 0,
             "极性由 HIGH 改为 LOW；1 kHz 只是初值，Set_Freq_Duty 会改 ARR"},
            {"IRSender_Init",            tim_apb2,  0, 1894,  473, 0,
             "PSC/ARR 全改；38 kHz，与蜂鸣器互斥（同一个 htim1）"},
            /* TIM2：彩色 LED，ARR=1999 即 2000 个计数 */
            {"tim.c MX_TIM2_Init",       tim_apb1, 359, 1999,    0, 1, NULL},
            {"ColorLED_Set(c=0xFF)",     tim_apb1, 359, 1999, 2000, 0,
             "Pulse = c*2000/255；c=255 时 CCR=2000=ARR+1 ⇒ 恒有效 ⇒ 有效区 100%"},
            /* TIM3：电机 B */
            {"tim.c MX_TIM3_Init",       tim_apb1,  7,  999,  900, 1,
             "Pulse = speed*10，speed 0..99 ⇒ 0..99% 占空比（9 kHz）"},
            /* TIM4：HAL 时基 */
            {"stm32f1xx_hal_timebase_tim", tim_apb1, 71, 999, 0, 1,
             "1 MHz 计数 / 1 ms 更新中断，uwTick 的来源"},
        };
        unsigned i;

        for (i = 0; i < sizeof t / sizeof t[0]; i++)
            report(&t[i]);
    }

    printf("\n== 2. APB1 分频器不是 1 时，定时器时钟要乘 2 ==\n");
    {
        uint32_t with_div1 = tim_clock_for_apb1(PCLK1_HZ, 1);
        uint32_t with_div2 = tim_clock_for_apb1(PCLK1_HZ, 0);
        uint32_t psc_div2  = (with_div2 / 1000000u) - 1u;
        uint32_t psc_wrong = (PCLK1_HZ / 1000000u) - 1u;

        printf("  APB1CLKDivider = DIV1 : TIMxCLK = PCLK1      = %u Hz\n", with_div1);
        printf("  APB1CLKDivider = DIV2 : TIMxCLK = 2 x PCLK1  = %u Hz   <- 本工程\n", with_div2);
        printf("  要得到 1 MHz 计数时钟：PSC = %u/1000000 - 1 = %u\n", with_div2, psc_div2);
        printf("  若误按 PCLK1=36MHz 算：PSC = %u，CK_CNT = %u Hz，时基慢一倍\n",
               psc_wrong, with_div2 / (psc_wrong + 1u));

        assert(with_div2 == 72000000u);
        assert(psc_div2 == 71u);          /* 与 htim4.Init.Prescaler 一致 */
    }

    printf("\n== 3. udelay 的 \"reload 对应 1ms\" 差一位 ==\n");
    {
        uint32_t arr = 999;               /* TIM4：Period = 1000-1 */
        uint32_t w1  = ticks_as_written(1000, arr);
        uint32_t r1  = ticks_if_fixed(1000, arr);
        uint64_t us_as_written = 0, us_fixed = 0;
        int i;

        printf("  udelay(1000)：写成 ticks = 1000*%u/1000 = %u 个 tick\n", arr, w1);
        printf("                应为 ticks = 1000*%u/1000 = %u 个 tick\n", arr + 1u, r1);

        /*
         * 模拟 mdelay(1000) = 1000 次 udelay(1000)。CK_CNT = 1 MHz ⇒ 1 tick = 1 us。
         * 首版这里写成 `w1 * 1000u / 1000000u ? w1 : 0`：整数除法先得 0，三目永远取 0，
         * 输出 "0 us / 少 100%"——和本节要讲的 us*reload/1000 截断是同一类错误。
         */
        for (i = 0; i < 1000; i++) {
            us_as_written += w1;
            us_fixed      += r1;
        }
        printf("  mdelay(1000) 累计：写成版 = %llu us，应为 %llu us\n",
               (unsigned long long)us_as_written, (unsigned long long)us_fixed);
        printf("  每秒少 %llu us（%.3f%%），跑 1 小时累计少 %.1f s\n",
               (unsigned long long)(us_fixed - us_as_written),
               100.0 * (double)(us_fixed - us_as_written) / (double)us_fixed,
               (double)(us_fixed - us_as_written) * 3600.0 / 1e6);

        assert(w1 == 999u);
        assert(r1 == 1000u);
    }

    printf("\n== 4. 占空比的两个口径：引脚电平 vs 有效区 ==\n");
    {
        struct {
            const char *name;
            uint32_t    arr, pulse;
            int         polarity_high;
        } cases[] = {
            { "红外发射 38kHz", 1894,  473, 0 },
            { "无源蜂鸣器 1kHz",  999,  499, 0 },
            { "彩色 LED c=0xFF",  1999, 2000, 0 },
            { "电机 B speed=99",   999,  990, 1 },
        };
        unsigned i;

        for (i = 0; i < sizeof cases / sizeof cases[0]; i++) {
            uint32_t ticks  = cases[i].arr + 1u;
            double   active = 100.0 * cases[i].pulse / (double)ticks;
            double   pin_hi = cases[i].polarity_high ? active : 100.0 - active;

            printf("  %-18s CCR/(ARR+1)=%4u/%-4u 有效区=%6.2f%%  极性=%-4s 引脚高=%6.2f%% 引脚低=%6.2f%%\n",
                   cases[i].name, cases[i].pulse, ticks, active,
                   cases[i].polarity_high ? "HIGH" : "LOW", pin_hi, 100.0 - pin_hi);
        }
        printf("  极性：前三个 LOW，电机 HIGH（见 §1）。\n");
        printf("  => 同一个 473/1895，「占空比」说 24.96%%（有效区/引脚低）还是 75.04%%（引脚高），\n");
        printf("     取决于标准；驱动注释写 75%%，与引脚高一致，与 CCR/(ARR+1) 不一致。\n");
        printf("  => 想回答「红外 LED 到底亮了多久」，必须先有原理图（低电平点亮还是高电平点亮）。\n");
        printf("     本仓库没有该模块的原理图，所以这里只能给两种口径，不能断言哪一个。\n");
        printf("  => 另一个易错点：tim.c 三路全是 OCPolarity=HIGH，是驱动在 Init 里改成 LOW 的\n");
        printf("     （LED/蜂鸣器/红外），电机保持 HIGH。真正生效的极性由驱动决定。\n");
    }

    printf("\n== 5. system_get_ns 跨回卷时的两次读 ==\n");
    {
        uint32_t arr = 999;
        /* 场景：读 tick 时 CNT=995；读 CNT 之前发生更新事件，CNT 归 0 后走到 3 */
        snapshot_t before = { 100, 995 };
        snapshot_t after  = { 101,   3 };
        snapshot_t code_order;    /* 先 tick（旧）后 cnt（新） */
        snapshot_t reverse;       /* 先 cnt（旧）后 tick（新） */
        uint64_t ns_truth = ns_from(after, arr);

        code_order.tick = before.tick;
        code_order.cnt  = after.cnt;
        reverse.tick    = after.tick;
        reverse.cnt     = before.cnt;

        printf("  回卷前 tick=%u CNT=%u；回卷后 tick=%u CNT=%u（真实 %.3f ms）\n",
               before.tick, before.cnt, after.tick, after.cnt, (double)ns_truth / 1e6);
        printf("  代码顺序（先 tick 后 CNT）：%.3f ms，偏小 %.3f ms\n",
               (double)ns_from(code_order, arr) / 1e6,
               (double)(ns_truth - ns_from(code_order, arr)) / 1e6);
        printf("  反序（先 CNT 后 tick）：    %.3f ms，偏大 %.3f ms\n",
               (double)ns_from(reverse, arr) / 1e6,
               (double)(ns_from(reverse, arr) - ns_truth) / 1e6);
        printf("  => 两种顺序都不精确，误差量级都是 1 ms（= 一个更新周期）；\n");
        printf("     修法是 tick-CNT-tick 读两次比对，前后不一致就重读。\n");

        assert(ns_from(code_order, arr) <= ns_truth);
        assert(ns_from(reverse, arr)  >  ns_truth);
        printf("  (assert 已锁定两条误差方向)\n");
    }

    printf("\n== 6. PA8 / TIM1_CH1 被两个驱动抢用 ==\n");
    {
        uint32_t ck_buzzer = tim_apb2 / (71u + 1u);
        uint32_t ck_ir     = tim_apb2 / (0u + 1u);
        double   f_buzzer  = (double)ck_buzzer / (999u + 1u);
        double   f_ir      = (double)ck_ir / (1894u + 1u);

        printf("  蜂鸣器：CK_CNT=%9u Hz, ARR=999  -> %8.2f Hz\n", ck_buzzer, f_buzzer);
        printf("  红外  ：CK_CNT=%9u Hz, ARR=1894 -> %8.2f Hz\n", ck_ir, f_ir);
        printf("  两者写的是同一个 htim1 的 PSC/ARR，后调用的 Init 覆盖前者。\n");
        printf("  若红外在蜂鸣器之后初始化，蜂鸣器会变成 %.2f Hz：超出人耳响应，等于哑了。\n", f_ir);
        printf("  本工程用 LAB_TEST=5 / 6 互斥规避（不可能同时编译），这是\"配置层互斥\"而非修复。\n");
    }

    printf("\n== 7. 定时器时钟使能被分散在两个 MspInit 里 ==\n");
    {
        /*
         * tim.c 的事实（逐行核对）：
         *   HAL_TIM_Base_MspInit   : 只处理 TIM1、TIM2 —— 使能它们的外设时钟
         *   HAL_TIM_PWM_MspInit    : 只处理 TIM3        —— 使能 TIM3 的时钟
         *   HAL_TIM_MspPostInit    : 只配 PA8/PA15/PB3/PA2/PB4 的复用推挽，不开时钟
         */
        static const char *rows[] = {
            "MX_TIM1_Init        : Base_Init + PWM_Init    -> Base_MspInit(TIM1)  -> 时钟开",
            "MX_TIM2_Init        : Base_Init + PWM_Init    -> Base_MspInit(TIM2)  -> 时钟开",
            "MX_TIM3_Init        : 只有 PWM_Init           -> PWM_MspInit(TIM3)   -> 时钟开",
            "PassiveBuzzer_Init  : 只 TIM_Base_SetConfig   -> 不触发任何 MspInit  -> 时钟不开",
            "IRSender_Init       : Base_DeInit + Base_Init -> Base_MspInit(TIM1)  -> 时钟开",
        };
        unsigned i;

        for (i = 0; i < sizeof rows / sizeof rows[0]; i++)
            printf("  %s\n", rows[i]);

        printf("\n  => PassiveBuzzer_Init 不开时钟，它依赖 main.c 先调过 MX_TIM1_Init()。\n");
        printf("     单独把 driver_passive_buzzer.c 拷进新工程，蜂鸣器不会响，\n");
        printf("     而 HAL 不会返回任何错误——它只是把寄存器写进了一个没有时钟的外设。\n");
        printf("     与 Day11 的\"CS 没配但 HAL_SPI_* 返回 HAL_OK\"是同一类失败：\n");
        printf("     外设不检查前置条件，前置条件只能由调用方保证。\n");
    }

    printf("\n== 8. 主频变了，时基的 PSC 必须重算 ==\n");
    {
        /*
         * main.c 的顺序是 HAL_Init() -> SystemClock_Config()：
         *   HAL_Init 时主频还是复位值 HSI 8 MHz，APB1 分频为复位值 DIV1；
         *   SystemClock_Config 切到 72 MHz 并把 APB1 改成 DIV2。
         * 两个时刻 TIM4CLK 都是 8 MHz 与 72 MHz，PSC 分别是 7 与 71。
         * 若 HAL_RCC_ClockConfig() 不重调 HAL_InitTick()，时基就会快 9 倍。
         */
        uint32_t hsi_hz    = 8000000u;
        uint32_t psc_at_8  = (hsi_hz / 1000000u) - 1u;
        uint32_t psc_at_72 = (tim_apb1 / 1000000u) - 1u;
        double   ck_stale  = (double)tim_apb1 / (psc_at_8 + 1u);
        /* 一次更新 = ARR+1 = 1000 个 tick；tick 率是 ck_stale -> 时长(us) = 1000/ck_stale*1e6 */
        double   us_stale  = 1000.0 / ck_stale * 1e6;

        printf("  HSI  8 MHz (APB1=DIV1) -> TIM4CLK=8 MHz  -> PSC=%u -> CK_CNT=1 MHz\n", psc_at_8);
        printf("  72 MHz      (APB1=DIV2) -> TIM4CLK=72 MHz -> PSC=%u -> CK_CNT=1 MHz\n", psc_at_72);
        printf("  若沿用 PSC=%u 跑到 72 MHz：CK_CNT=%.0f Hz，一次更新只花 %.1f us\n",
               psc_at_8, ck_stale, us_stale);
        printf("  => 名义 1 ms 的 tick 实际 111 us，uwTick 快 9 倍：HAL_Delay(1000) 只等 111 ms。\n");
        printf("     这就是 HAL_InitTick 的注释写 \"or ... by HAL_RCC_ClockConfig()\" 的原因。\n");

        assert(psc_at_8 == 7u);
        assert(psc_at_72 == 71u);
    }

    printf("\n== 9. uwTick 没有递增者时，HAL_Delay 的条件恒真 ==\n");
    {
        /*
         * 事实（全仓 grep）：
         *   HAL_IncTick() 只有 stm32f1xx_hal.c:293 的 __weak 定义，没有任何调用点；
         *   HAL_TIM_PeriodElapsedCallback 只有头文件声明，没有实现。
         * 所以 TIM4 的更新中断虽然每秒来 1000 次，HAL_TIM_IRQHandler 调用的却是
         * 空的弱回调，uwTick 永远停在 0。
         *
         * HAL_Delay 的循环条件：(HAL_GetTick() - tickstart) < wait
         *   tickstart = HAL_GetTick() = 0，之后每次读仍是 0 ⇒ 0 < wait 恒真。
         * 这里用有界轮询模拟，不真的死循环。
         */
        const uint32_t poll_limit = 1000000u;
        uint32_t tick, tickstart, polls;
        uint32_t wait = 200u + 1u;        /* HAL_Delay(200)：wait += uwTickFreq */

        /* 情形 A：现状，uwTick 恒为 0 */
        tick = 0;
        tickstart = tick;
        for (polls = 0; polls < poll_limit; polls++) {
            if (!((tick - tickstart) < wait))
                break;
        }
        printf("  现状（uwTick 恒 0）：轮询 %u 次仍未退出（上限 %u）⇒ 无限循环\n",
               polls, poll_limit);
        assert(polls == poll_limit);

        /* 情形 B：补上回调后，每 1000 次轮询 uwTick 加 1（1 ms 一个 tick 的粗略模型） */
        tick = 0;
        tickstart = 0;
        for (polls = 0; polls < poll_limit; polls++) {
            if ((polls % 1000u) == 0u && polls != 0u)
                tick++;
            if (!((tick - tickstart) < wait))
                break;
        }
        printf("  补上回调后：第 %u 次轮询退出，对应 %u 个 tick（wait=%u）\n",
               polls, tick, wait);
        assert(polls < poll_limit);

        printf("  => 后果不止 HAL_Delay：所有用 HAL_GetTick() 的超时判据都永不超时\n");
        printf("     （HAL_SPI_Transmit 的 Timeout、HAL_UART 的等待等），外设不响应时直接卡死。\n");
        printf("  => system_get_ns() 也只剩 cnt 部分：每 1 ms 从 0 重来，不是单调递增的系统时间。\n");
    }

    return 0;
}
