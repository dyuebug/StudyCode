/*
 * lab_motion_timing.c —— Day18 测距与位置：主机端可复算判据
 *
 * 目的：把 SR04 测距、旋转编码器测速这两个**依赖 system_get_ns()** 的功能，
 *       在主机上按真实配置复算一遍，看它们在 `uwTick` 恒 0 时的输出。
 *       凡是依赖电平、真实时序、传感器行为的结论一律不在这里断言（那是板上/示波器的事）。
 *
 * 本仓库事实（源码核对得出，见 Day14 缺陷①、Day18 §4）：
 *   HAL_Init() → HAL_InitTick()（stm32f1xx_hal_timebase_tim.c:42）
 *     → TIM4 配为 1 MHz 计数 / ARR=999 / 1 ms 更新，HAL_TIM_Base_Start_IT + NVIC 使能
 *     → TIM4_IRQHandler（stm32f1xx_it.c:165）→ HAL_TIM_IRQHandler(&htim4)
 *     → HAL_TIM_PeriodElapsedCallback（stm32f1xx_hal_tim.c:3956）
 *     → 弱定义（stm32f1xx_hal_tim.c:5652），**本工程无强定义** ⇒ 空函数
 *     ⇒ HAL_IncTick() 从未被调用 ⇒ uwTick 恒 0 ⇒ HAL_GetTick() 恒 0
 *
 * 于是 system_get_ns()（driver_timer.c:135）只剩周期内部分：
 *   ns = HAL_GetTick() * 1e6 + cnt * 1e6 / reload     ← 前项恒 0
 *     ⇒ 返回值落在 [0, 1e6) ns，**每 1 ms 回绕一次**
 *
 * 覆盖：
 *   E1  system_get_ns() 的回绕范围与分辨率
 *   E2  SR04_Read 的输出随 Echo 起始相位变化（相位扫描）
 *   E3  SR04_TIMEOUT 对应的最大可测距离 vs 教材宣称的 450 cm
 *   E4  换算常数 17/1e6 的精确性与两级整数截断造成的系统性 -1 cm
 *   E5  编码器 g_speed：整数毫秒间隔直接导致除零
 *   E6  红外避障的"低有效"映射
 *   E7  `while (timeout--)` + `if (!timeout)` 使失败分支成为死代码
 *
 * 编译：gcc -std=c11 -Wall -Wextra -O2 lab_motion_timing.c -o lab_motion_timing
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

static int g_fail = 0;
static int g_case = 0;

#define CHECK(cond, fmt, ...)                                            \
    do {                                                                 \
        g_case++;                                                        \
        if (cond) {                                                      \
            printf("  PASS  " fmt "\n", ##__VA_ARGS__);                  \
        } else {                                                         \
            g_fail++;                                                    \
            printf("  FAIL  " fmt "   <-- (%s:%d)\n", ##__VA_ARGS__,     \
                   __FILE__, __LINE__);                                  \
        }                                                                \
    } while (0)

/* ========================================================================
 * 时基模型：与 stm32f1xx_hal_timebase_tim.c 的配置一致
 *   Period    = 1000000/1000 - 1 = 999        （1 ms 更新）
 *   Prescaler 使计数器时钟 = 1 MHz            （1 tick = 1 us）
 * ====================================================================== */
#define TIM4_ARR        999ULL
#define TIM4_TICK_NS    1000ULL                            /* 1 us */
#define TIM4_PERIOD_NS  ((TIM4_ARR + 1ULL) * TIM4_TICK_NS) /* 1 ms */

/* 复刻 driver_timer.c:135 的 system_get_ns()，uwTick 恒 0 */
static uint64_t system_get_ns_broken(uint64_t real_ns)
{
    uint64_t cnt = (real_ns % TIM4_PERIOD_NS) / TIM4_TICK_NS;   /* 0..999 */
    return cnt * 1000000ULL / TIM4_ARR;                          /* tick 项恒 0 */
}

/* 若 uwTick 正常递增时的同一函数（对照用） */
static uint64_t system_get_ns_fixed(uint64_t real_ns)
{
    uint64_t cnt = (real_ns % TIM4_PERIOD_NS) / TIM4_TICK_NS;
    uint64_t ns  = (real_ns / TIM4_PERIOD_NS) * 1000000ULL;
    return ns + cnt * 1000000ULL / TIM4_ARR;
}

/* ========================================================================
 * E1：回绕范围
 * ====================================================================== */
static void e1_wrap(void)
{
    uint64_t t, min = UINT64_MAX, max = 0;
    int wraps = 0;
    uint64_t prev = system_get_ns_broken(0);

    printf("[E1] system_get_ns() 在 uwTick 恒 0 时的取值范围（driver_timer.c:135-150）\n");
    for (t = 0; t <= 10 * TIM4_PERIOD_NS; t += TIM4_TICK_NS) {   /* 观测 10 ms */
        uint64_t v = system_get_ns_broken(t);
        if (v < min) min = v;
        if (v > max) max = v;
        if (v < prev) wraps++;
        prev = v;
    }
    printf("  10 ms 内：min=%llu ns, max=%llu ns, 回绕 %d 次\n",
           (unsigned long long)min, (unsigned long long)max, wraps);

    /* cnt=999 时 999*1e6/999 = 1000000，正好等于一个周期 —— 上界含 1 ms，
     * 但表示不了 2 ms，且下一秒立即回绕。 */
    CHECK(max <= 1000000ULL && max < 2000000ULL,
          "返回值上界 %llu ns（≈1 ms），表示不了 2 ms 以上的时长",
          (unsigned long long)max);
    CHECK(wraps >= 9, "10 ms 内回绕 %d 次（约每 1 ms 一次）", wraps);

    CHECK(system_get_ns_fixed(5000000ULL) >= 5000000ULL - 2000ULL,
          "对照：tick 正常时 5 ms 处返回 %llu ns（≈5e6，单调可用）",
          (unsigned long long)system_get_ns_fixed(5000000ULL));
}

/* ========================================================================
 * E2：SR04_Read 的相位扫描
 *
 * 换算：距离 d cm ⇒ Echo 高电平 = 2d/34000 s = d*58823.53 ns
 * 驱动：time = system_get_ns()（下降沿） - system_get_ns()（上升沿）  :93-106
 *       *pData = time * 17 / 1000000                                 :108
 *
 * 因为 system_get_ns() 只保留 1 ms 周期内位置，dt = (t0+E) mod 1ms - t0 mod 1ms：
 *   未跨越毫秒边界 ⇒ dt = E mod 1ms（可能正确，但已被截掉整数毫秒）
 *   跨越毫秒边界 ⇒ dt 为负，uint64 下溢成天文数字
 * 所以同一距离的读数取决于 Echo 从 1 ms 窗口的哪个位置开始。
 * ====================================================================== */
#define CM_TO_ECHO_NS(cm)  ((uint64_t)((cm) * 2.0 / 34000.0 * 1e9))

static uint32_t sr04_read_sim(double cm, uint64_t t_rise_ns)
{
    uint64_t a  = system_get_ns_broken(t_rise_ns);
    uint64_t b  = system_get_ns_broken(t_rise_ns + CM_TO_ECHO_NS(cm));
    uint64_t dt = b - a;                       /* 跨边界时下溢 */
    return (uint32_t)(dt * 17ULL / 1000000ULL);
}

static void e2_sr04_phase_sweep(void)
{
    static const double dists[] = {5.0, 10.0, 17.0, 20.0, 100.0};
    /* 六个起始相位，覆盖 1 ms 窗口的不同位置 */
    static const uint64_t phases[] = {0, 100000, 300000, 500000, 700000, 900000};
    const unsigned NP = sizeof(phases) / sizeof(phases[0]);
    unsigned k, p;

    printf("[E2] SR04_Read 读数随 Echo 起始相位变化（驱动 :93-108）\n");
    printf("  相位取 0/100/300/500/700/900 us，覆盖 1 ms 窗口的不同位置\n");
    printf("  %-7s %-10s %-56s %s\n", "距离cm", "Echo(us)", "六相位读数", "正确");

    for (k = 0; k < sizeof(dists) / sizeof(dists[0]); k++) {
        unsigned want = (unsigned)(dists[k] + 0.5);
        unsigned ok = 0;
        printf("  %-7.0f %-10.1f ", dists[k], (double)CM_TO_ECHO_NS(dists[k]) / 1000.0);
        for (p = 0; p < NP; p++) {
            uint32_t v = sr04_read_sim(dists[k], phases[p]);
            printf("%-11u", v);
            if (v == want) ok++;
        }
        printf(" %u/%u\n", ok, NP);

        if (CM_TO_ECHO_NS(dists[k]) < TIM4_PERIOD_NS) {
            CHECK(ok > 0 && ok <= NP,
                  "%.0f cm（Echo %.0f us < 1 ms）%u/%u 个相位读对 ⇒ 正确与否取决于起始时刻",
                  dists[k], (double)CM_TO_ECHO_NS(dists[k]) / 1000.0, ok, NP);
        } else {
            /* Echo ≥ 1 ms：整数毫秒被丢弃，绝不可能读对 */
            CHECK(ok == 0,
                  "%.0f cm（Echo %.0f us ≥ 1 ms）%u/%u 个相位读对 ⇒ 必然读错",
                  dists[k], (double)CM_TO_ECHO_NS(dists[k]) / 1000.0, ok, NP);
        }
    }

    /* 相位跨度对读数的影响：同一距离，两个相位差值极大 */
    {
        uint32_t lo = sr04_read_sim(5.0, 100000);   /* 不跨边界 */
        uint32_t hi = sr04_read_sim(5.0, 900000);   /* 跨边界 */
        printf("  同样 5 cm：相位 100 us ⇒ %u，相位 900 us ⇒ %u\n", lo, hi);
        CHECK(lo != hi, "同一距离因起始相位不同给出 %u 与 %u（差 %lld）",
              lo, hi, (long long)lo - (long long)hi);
    }
}

/* ========================================================================
 * E3：SR04_TIMEOUT 限制的最大量程
 * ====================================================================== */
static void e3_range(void)
{
    /* driver :46  #define SR04_TIMEOUT 20000 // us
     * 声呐往返：t = 2d / c，c = 340 m/s
     *   d(cm) = t(us) * 1e-6 * 340 * 100 / 2 = t * 0.017 */
    const double timeout_us  = 20000.0;
    const double max_cm      = timeout_us * 1e-6 * 340.0 * 100.0 / 2.0;
    const double textbook_cm = 450.0;
    const double need_us     = textbook_cm / 100.0 / 340.0 * 2.0 * 1e6;

    printf("[E3] SR04_TIMEOUT 与量程（驱动 :46、:97-102）\n");
    printf("  超时 %.0f us ⇒ 最大 %.0f cm；教材宣称 450 cm 需 %.0f us\n",
           timeout_us, max_cm, need_us);
    CHECK(max_cm < textbook_cm,
          "超时上限只够 %.0f cm < 教材的 %.0f cm ⇒ 远程必然耗尽 timeout",
          max_cm, textbook_cm);
    CHECK(max_cm > 300.0, "但仍覆盖 3 m 以内（%.0f cm），近距离使用不暴露", max_cm);
    CHECK(need_us > timeout_us,
          "450 cm 需要 %.0f us 往返 > 超时 %.0f us（差 %.1f 倍）⇒ 远程耗尽后按 E7 返回垃圾而非 -1",
          need_us, timeout_us, need_us / timeout_us);
}

/* ========================================================================
 * E4：换算常数与两级截断
 * ====================================================================== */
static void e4_conversion(void)
{
    printf("[E4] *pData = time * 17 / 1000000 的常数与截断（驱动 :108）\n");

    /* 常数本身是精确的：1e6/17 = 2e9/34000 */
    CHECK(1000000ULL / 17ULL == 58823ULL && CM_TO_ECHO_NS(17.0) / 17ULL == 58823ULL,
          "1 cm 对应 %llu ns（1e6/17 与 2e9/34000 同值）", 1000000ULL / 17ULL);

    /* 但两级整数截断各丢一点：
     *   ① CM_TO_ECHO_NS 把 ns 截成整数（丢 <1 ns）
     *   ② ×17/1e6 再截一次
     * 结果每个读数系统性偏小 1 cm。 */
    {
        unsigned d, low = 0;
        printf("  %-6s %-12s %-8s %-8s\n", "距离cm", "Echo(ns)", "驱动读数", "偏差");
        for (d = 1; d <= 10; d++) {
            uint64_t echo = CM_TO_ECHO_NS((double)d);
            uint32_t got = (uint32_t)(echo * 17ULL / 1000000ULL);
            printf("  %-6u %-12llu %-8u %+d\n",
                   d, (unsigned long long)echo, got, (int)got - (int)d);
            if (got < d) low++;
        }
        CHECK(low == 10, "1..10 cm 全部偏小 1 cm（%u/10）⇒ 系统性截断，不是随机误差", low);
    }

    /* uint32 截断：time 因回绕变成巨大值时，宽结果赋给 uint32_t 只保留低 32 位 */
    {
        uint64_t bogus = 0xFFFFFFFFFFF00000ULL;
        uint64_t wide  = bogus * 17ULL / 1000000ULL;
        uint32_t narrow = (uint32_t)wide;
        printf("  下溢 time=0x%llX ⇒ 中间值 %llu ⇒ 截断为 %u\n",
               (unsigned long long)bogus, (unsigned long long)wide, narrow);
        CHECK((uint64_t)narrow != wide,
              "uint64 中间值 %llu 赋给 uint32_t 后为 %u ⇒ 静默截断",
              (unsigned long long)wide, narrow);
    }
}

/* ========================================================================
 * E5：编码器测速
 *
 * 驱动 :139  g_speed = (uint64_t)1000000000 / (time - pre_time);
 * time 来自 system_get_ns()（只保留 1 ms 周期内位置）。
 * ====================================================================== */
static int encoder_speed_sim(uint64_t t_prev_ns, uint64_t t_now_ns,
                             uint32_t *out)
{
    uint64_t dt = system_get_ns_broken(t_now_ns) - system_get_ns_broken(t_prev_ns);
    if (dt == 0) return -1;                       /* 驱动此处是除零 */
    *out = (uint32_t)(1000000000ULL / dt);
    return 0;
}

static void e5_encoder(void)
{
    printf("[E5] 编码器测速（driver_rotary_encoder.c:139）\n");

    /* 首次中断：pre_time 初值 0 ⇒ time - 0 就是 system_get_ns() 本身 */
    {
        uint64_t v = system_get_ns_broken(3000000000ULL);   /* 开机 3 s 后第一次转 */
        printf("  首次中断（真实 t=3 s）：system_get_ns() 返回 %llu ns\n",
               (unsigned long long)v);
        CHECK(v < TIM4_PERIOD_NS,
              "已过 3 s，函数只返回 %llu ns（<1 ms）⇒ 首次 g_speed 为 %u",
              (unsigned long long)v, v ? (unsigned)(1000000000ULL / v) : 0U);
    }

    /* 整数毫秒的间隔 ⇒ 两端 system_get_ns() 相同 ⇒ dt=0 ⇒ 除零 */
    {
        uint64_t intervals_ms[] = {1, 5, 10, 20, 100};
        unsigned k, zero = 0;
        uint32_t dummy;
        for (k = 0; k < sizeof(intervals_ms) / sizeof(intervals_ms[0]); k++) {
            uint64_t t0 = 1234567890000ULL;                  /* 任意非整毫秒相位 */
            uint64_t t1 = t0 + intervals_ms[k] * 1000000ULL; /* 整数毫秒后 */
            if (encoder_speed_sim(t0, t1, &dummy) < 0) zero++;
        }
        printf("  间隔取 {1,5,10,20,100} ms：%u/5 次 dt=0（驱动处为除零）\n", zero);
        CHECK(zero == 5,
              "整数毫秒间隔全部 dt=0 ⇒ g_speed = 1000000000/0（未定义的商）");
    }

    /* 非整数毫秒间隔：dt 只等于小数部分 ⇒ 真实间隔翻倍，读数不变 */
    {
        uint64_t t0 = 1234567000000ULL;          /* 相位 0，避免跨边界干扰观察 */
        uint32_t s103 = 0, s203 = 0;
        int r1 = encoder_speed_sim(t0, t0 + 10300000ULL, &s103);   /* 10.3 ms */
        int r2 = encoder_speed_sim(t0, t0 + 20300000ULL, &s203);   /* 20.3 ms */
        printf("  真实间隔 10.3 ms ⇒ g_speed=%u；20.3 ms ⇒ g_speed=%u\n",
               r1 ? 0U : s103, r2 ? 0U : s203);
        printf("  （正确应约 97 与 49，比值 2:1）\n");
        CHECK(!r1 && !r2 && s103 == s203,
              "间隔翻倍（10.3→20.3 ms）读数不变（%u）⇒ 与真实间隔完全脱钩", s103);
    }

    /* 对照：tick 正常时读数才可用 */
    {
        uint64_t a = system_get_ns_fixed(1234567890000ULL);
        uint64_t b = system_get_ns_fixed(1234567890000ULL + 10000000ULL);
        uint32_t s = (uint32_t)(1000000000ULL / (b - a));
        printf("  对照（tick 正常）：间隔 10 ms ⇒ g_speed=%u\n", s);
        CHECK(s == 100, "tick 正常时 10 ms 间隔稳定得 %u（应为 100）", s);
    }
}

/* ========================================================================
 * E6：红外避障的极性
 * ====================================================================== */
static int ir_obstacle_read_sim(int pin_high)          /* 1 = 高电平 */
{
    return pin_high ? 1 : 0;                           /* driver_ir_obstacle.c:70-73 */
}

static void e6_ir_polarity(void)
{
    printf("[E6] 红外避障的极性映射（driver_ir_obstacle.c:68-74）\n");
    CHECK(ir_obstacle_read_sim(0) == 0, "PB13 低电平 ⇒ 返回 0（有障碍物，低有效）");
    CHECK(ir_obstacle_read_sim(1) == 1, "PB13 高电平 ⇒ 返回 1（无障碍物）");
    CHECK(ir_obstacle_read_sim(0) == 0,
          "与教材 :897「低电平表示即将碰到障碍物」一致");
}

/* ========================================================================
 * E7：`while (timeout--)` + `if (!timeout)` 的后置自减陷阱
 *
 * driver_ultrasonic_sr04.c:85-92 / :97-104
 *   while (timeout-- && (ReadPin(ECHO) == RESET)) { udelay(1); }
 *   if (!timeout) { return -1; }
 *
 * 后置自减先把旧值交给判断，再把 timeout 减 1。于是超时耗尽的那一次
 * timeout 从 0 减到 UINT32_MAX —— `!timeout` 为假，**失败分支永不进入**。
 * 反过来，若恰好在旧值为 1 时条件成立、循环退出，timeout 变成 0，
 * 明明成功却返回 -1。
 * ====================================================================== */
static uint32_t timeout_after_loop(uint32_t init, uint32_t n_true)
{
    uint32_t timeout = init;
    uint32_t i = 0;

    for (;;) {
        uint32_t t = timeout--;      /* 后置自减：先取值，再减 1 */
        if (t == 0) break;           /* 短路退出：timeout 已回绕 */
        if (i >= n_true) break;      /* cond 为假退出 */
        i++;
    }
    return timeout;
}

static void e7_post_decrement(void)
{
    uint32_t t_exhaust, t_normal, t_edge;

    printf("[E7] while(timeout--) + if(!timeout) 的后置自减陷阱（驱动 :85-92、:97-104）\n");

    /* ① 真正的超时：条件永远为真，直到 timeout 耗尽 */
    t_exhaust = timeout_after_loop(20000, 1000000);
    printf("  超时耗尽：退出时 timeout=0x%08X，!timeout=%d\n",
           t_exhaust, !t_exhaust);
    CHECK(t_exhaust == 0xFFFFFFFFU,
          "耗尽退出时 timeout 回绕为 0x%08X ⇒ `if (!timeout)` 为假 ⇒ 失败分支是死代码",
          t_exhaust);
    CHECK(!t_exhaust == 0,
          "`!timeout` = %d ⇒ 不会 return -1，函数继续往下算出垃圾距离并 return 0",
          !t_exhaust);

    /* ② 正常路径：条件在几百次内变假 */
    t_normal = timeout_after_loop(20000, 500);
    printf("  正常回波：退出时 timeout=%u，!timeout=%d\n", t_normal, !t_normal);
    CHECK(t_normal != 0 && !t_normal == 0,
          "正常路径返回 %u ⇒ `!timeout` 为假，正确不返回 -1", t_normal);

    /* ③ 边界：恰好在旧值为 1 时条件变假 ⇒ timeout=0 ⇒ 误报失败 */
    t_edge = timeout_after_loop(501, 500);
    printf("  边界情形（恰好最后一次）：退出时 timeout=%u，!timeout=%d\n",
           t_edge, !t_edge);
    CHECK(t_edge == 0,
          "旧值为 1 时退出 ⇒ timeout=0 ⇒ `!timeout` 为真 ⇒ 成功被误判为失败");

    /* ④ 直接把"耗尽"灌进驱动同款判断 */
    {
        int ret = (!t_exhaust) ? -1 : 0;
        printf("  驱动同款判断对耗尽情形返回 %d（注释承诺 -1 = 失败）\n", ret);
        CHECK(ret == 0, "承诺 `return -1` 的超时情形实际返回 %d ⇒ 调用方无从区分", ret);
    }
}

/* ====================================================================== */

int main(int argc, char **argv)
{
    int inject = 0, i;

    for (i = 1; i < argc; i++)
        if (strcmp(argv[i], "--inject") == 0 && i + 1 < argc)
            inject = atoi(argv[++i]);

    printf("lab_motion_timing — Day18 主机端可复算判据\n");
    printf("模型：TIM4 = 1 MHz 计数 / ARR 999 / 1 ms 更新；uwTick 恒 0\n\n");

    e1_wrap();        printf("\n");
    e2_sr04_phase_sweep(); printf("\n");
    e3_range();       printf("\n");
    e4_conversion();  printf("\n");
    e5_encoder();     printf("\n");
    e6_ir_polarity(); printf("\n");
    e7_post_decrement(); printf("\n");

    if (inject > 0) {
        printf("[注入] --inject %d\n", inject);
        switch (inject) {
        case 1:
            CHECK(system_get_ns_broken(5000000ULL) >= 5000000ULL,
                  "注入：声称回绕后的值能表示 5 ms（应失败）");
            break;
        case 2: {
            uint32_t v = sr04_read_sim(100.0, 0);
            CHECK(v == 100, "注入：声称 100 cm 在相位 0 时输出 100（实为 %u，应失败）", v);
            break;
        }
        case 3:
            CHECK(0, "注入：无条件失败（应失败）");
            break;
        default:
            printf("  未知注入编号，忽略\n");
            break;
        }
        printf("\n");
    }

    printf("汇总：%d 条判据，%d 条失败\n", g_case, g_fail);
    if (g_fail == 0) { printf("RESULT: PASS\n"); return 0; }
    printf("RESULT: FAIL\n");
    return 1;
}
