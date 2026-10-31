/**
 * lab_la_arith.c —— 逻辑分析仪的容量与带宽算术（主机端，不接板）
 *
 * 教材第 4 章给了两种方案（buffer 模式 / stream 模式）和"用 STM32F103 做 demo"的选择，
 * 但没给任何数字。没有数字就无法判断这个选择在什么条件下成立、在什么条件下不成立。
 * 本程序把那些隐含的约束算出来，每一条都用 assert 钉住，改错会当场失败。
 *
 * 编译运行（在 experiments/F103_20_LogicAnalyzer/ 下）：
 *   gcc -std=c11 -Wall -Wextra -O2 -I. -o host/lab_la_arith host/lab_la_arith.c la_core.c
 *   ./host/lab_la_arith          # 期望 rc=0
 *
 * 全部数字都是**理论估算**（按 STM32F103C8T6 的 72 MHz / 20 KB SRAM 与 8N1 帧格式算），
 * 唯一例外是用 la_core.c 的同一个函数算出来的容量值——那是软件事实，不是估算。
 * 任何"实测"字样都不会出现在本程序的输出里：本机没有板，也没有逻辑分析仪。
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "la_core.h"

/* ---- 平台常量（STM32F103C8T6 / 本工程配置） ---------------------------- */
#define CPU_HZ          72000000UL   /* HCLK = 72 MHz */
#define SRAM_BYTES      20480UL      /* 20 KB */
#define STACK_RESERVE   4096UL       /* 本工程留给栈与非采样变量的余量（估算） */
#define BYTES_PER_SAMPLE_PACKED   1UL   /* 8 通道打包成 1 字节 */
#define BYTES_PER_SAMPLE_IDR      4UL   /* 图省事直接存整个 IDR：uint32 */

/* 8N1 = 1 起始 + 8 数据 + 1 停止 = 10 bit/字节 */
#define UART_BITS_PER_BYTE  10UL

/* 一次采样中断的开销（周期数，按 Cortex-M3 的保守估计）：
 *   入栈/出栈 12+12、读 IDR 与打包 5、写环形缓冲 8、循环判断 3 ≈ 40
 * 取 32 是乐观值、40 是保守值，两个都算，不挑好看的那个。 */
#define ISR_CYCLES_OPT    32UL
#define ISR_CYCLES_PESS   40UL

/* ---- 输出辅助 ---------------------------------------------------------- */

static void hr(const char *title)
{
    printf("\n== %s ==\n", title);
}

/* ---- 1. buffer 模式：SRAM 能换到多长的观测窗口 ------------------------ */

static void part_capacity(void)
{
    const uint32_t samples = la_max_samples(SRAM_BYTES, STACK_RESERVE);
    static const uint32_t fs_list[] = {10000UL, 100000UL, 500000UL, 1000000UL, 2250000UL};

    hr("1. buffer 模式：20 KB SRAM 的观测窗口");
    printf("SRAM %lu B − 预留 %lu B = %lu 个打包样本（每样本 1 B / 8 通道）\n",
           (unsigned long)SRAM_BYTES, (unsigned long)STACK_RESERVE,
           (unsigned long)samples);
    /* 逐行陈述而不是列表格：中文表头的显示宽度与字节数不同，列一定对不齐，
     * 一旦对不齐，读者就会开始怀疑数字本身。 */
    for (unsigned int i = 0; i < sizeof(fs_list) / sizeof(fs_list[0]); i++) {
        uint32_t us = la_capture_us(samples, fs_list[i]);
        printf("  %-8lu Sa/s → 窗口 %.1f ms（%lu µs）\n",
               (unsigned long)fs_list[i],
               (double)us / 1000.0, (unsigned long)us);
    }

    /* 钉住两个端点：1 MSa/s 只有 16 ms，2.25 MSa/s 只有 7 ms。
     * 这两个数字决定了"buffer 模式够不够用"这个问题的答案。 */
    assert(la_capture_us(samples, 1000000UL) == 16384UL);   /* 16.384 ms */
    assert(la_capture_us(samples, 2250000UL) == 7281UL);    /* 7.281 ms  */

    printf("\n结论：在 1 MSa/s 下，整块 SRAM 只够 16.4 ms。\n");
    printf("      教材说 FPGA 方案靠外挂 DRAM 拿长窗口——这就是那句话的算术依据：\n");
    printf("      16 KB 与 1 GB 差 65 536 倍，采样率不变时窗口也差 65 536 倍。\n");
}

/* ---- 2. 存储格式：打包不打包差几倍 ------------------------------------ */

static void part_packing(void)
{
    const uint32_t samples_packed = la_max_samples(SRAM_BYTES, STACK_RESERVE);
    const uint32_t samples_idr = samples_packed / BYTES_PER_SAMPLE_IDR;

    hr("2. 存储格式的代价：存整个 IDR 还是打包成 1 字节");
    printf("  打包（1 B/样本）：%lu 样本\n", (unsigned long)samples_packed);
    printf("  存 IDR（4 B/样本）：%lu 样本\n", (unsigned long)samples_idr);
    printf("  比值 %.2f×\n",
           (double)samples_packed / (double)samples_idr);

    assert(samples_idr == 4096UL);

    /* 时间窗同比例缩水 */
    assert(la_capture_us(samples_packed, 1000000UL) == 16384UL);
    assert(la_capture_us(samples_idr,    1000000UL) == 4096UL);

    printf("\n结论：同一块 SRAM，存整个 IDR 只有 1/4 的窗口。\n");
    printf("      「反正 IDR 一次就读到」是省 5 行代码、丢 3/4 采样深度的典型交换。\n");
    printf("      而且打包还顺手统一了通道顺序，解码器不必再知道引脚号。\n");
}

/* ---- 3. stream 模式：串口把采样率压到多少 ---------------------------- */

static void part_uart_ceiling(void)
{
    const uint32_t baud = 115200UL;
    const uint32_t bytes_per_s = baud / UART_BITS_PER_BYTE;
    const uint32_t samples = la_max_samples(SRAM_BYTES, STACK_RESERVE);
    const uint32_t dump_us = la_capture_us(samples, bytes_per_s);
    const uint32_t fill_us = la_capture_us(samples, 1000000UL);

    hr("3. stream 模式：115200 串口的上限");
    printf("  %lu bps ÷ %lu bit/B = %lu B/s\n",
           (unsigned long)baud, (unsigned long)UART_BITS_PER_BYTE,
           (unsigned long)bytes_per_s);
    printf("  1 字节/样本 → 上限 %lu Sa/s\n", (unsigned long)bytes_per_s);
    printf("  填满 %lu 样本需 %lu µs（@1 MSa/s），上传需 %lu µs（%lu ms）\n",
           (unsigned long)samples, (unsigned long)fill_us,
           (unsigned long)dump_us, (unsigned long)(dump_us / 1000UL));
    printf("  上传/采集 = %.1f 倍\n", (double)dump_us / (double)fill_us);

    assert(bytes_per_s == 11520UL);
    /* 单位是微秒：16384 × 1e6 ÷ 11520 = 1 422 222 µs。写成 1422 是拿 ms 当 µs 用。 */
    assert(dump_us == 1422222UL);
    assert(fill_us == 16384UL);

    printf("\n结论：115200 下 stream 模式只有 11.5 kSa/s，比 1 MSa/s 慢 87 倍。\n");
    printf("      所以教材的顺序（先串口、再 USB CDC）不是偏好，是被带宽逼出来的：\n");
    printf("      USB FS 的 bulk 实测常在 1 MB/s 量级，才勉强追上 1 MSa/s 的 8 通道打包流。\n");
    printf("      本工程因此只做 buffer 模式 + 事后上传，不在采样期间转发。\n");
}

/* ---- 4. 量化误差：多快的采样率才够用 --------------------------------- */

static void part_quantization(void)
{
    /* 采样周期 Ts，测一个宽度 W 的脉冲：量化误差 ±Ts，相对误差 Ts/W。
     * 要求相对误差 ≤ 10% ⇒ Ts ≤ W/10 ⇒ fs ≥ 10/W。 */
    static const uint32_t width_us[] = {1UL, 10UL, 100UL, 1000UL};
    const uint32_t k = 10UL;   /* 目标：相对误差 ≤ 1/k */

    hr("4. 量化误差：要测多窄的脉冲，就得多快的采样率");
    printf("  判据：相对误差 = Ts/W ≤ 1/%lu ⇒ fs ≥ %lu/W\n\n",
           (unsigned long)k, (unsigned long)k);
    for (unsigned int i = 0; i < sizeof(width_us) / sizeof(width_us[0]); i++) {
        uint32_t need = k * 1000000UL / width_us[i];     /* fs ≥ k/W */
        /* 1 MSa/s 下 Ts = 1 µs，相对误差 = 1 µs / W。
         * 用浮点打印而不是 100/W：整数除法会把 0.1% 印成 0%，读数失真。 */
        int ok = (1000000UL >= need);
        printf("  W=%-6lu µs → 需要 %-9lu Hz，1 MSa/s 下误差 %.1f%% → %s\n",
               (unsigned long)width_us[i], (unsigned long)need,
               100.0 / (double)width_us[i], ok ? "够" : "不够");
    }

    assert(k * 1000000UL / 10UL == 1000000UL);    /* 10 µs 需要 1 MHz */
    assert(k * 1000000UL / 1UL == 10000000UL);    /* 1 µs 需要 10 MHz */

    printf("\n结论：1 MSa/s 只能把 10 µs 以上的脉冲测到 10%% 误差以内。\n");
    printf("      测 1 µs 需要 10 MSa/s——F103 做不到（见下一节）。\n");
    printf("      这也是为什么调 I2C/SPI 时序要看边沿位置，而不是脉冲宽度：\n");
    printf("      边沿可以靠 EXTI 打时间戳，精度不取决于采样率。\n");
}

/* ---- 5. 采样率上限：中断开销决定天花板 ------------------------------- */

static void part_isr_ceiling(void)
{
    const double t_opt_ns  = (double)ISR_CYCLES_OPT  * 1e9 / (double)CPU_HZ;
    const double t_pess_ns = (double)ISR_CYCLES_PESS * 1e9 / (double)CPU_HZ;

    hr("5. 采样率天花板：一次采样中断要花多少时间");
    printf("  CPU %lu MHz → 1 周期 = %.2f ns\n",
           (unsigned long)(CPU_HZ / 1000000UL), 1e9 / (double)CPU_HZ);
    printf("  乐观 %lu 周期 = %.0f ns → 上限 %.2f MSa/s（100%% CPU）\n",
           (unsigned long)ISR_CYCLES_OPT, t_opt_ns,
           (double)CPU_HZ / (double)ISR_CYCLES_OPT / 1e6);
    printf("  保守 %lu 周期 = %.0f ns → 上限 %.2f MSa/s（100%% CPU）\n",
           (unsigned long)ISR_CYCLES_PESS, t_pess_ns,
           (double)CPU_HZ / (double)ISR_CYCLES_PESS / 1e6);
    printf("  留 50%% CPU 给主循环 → 保守估计 %.2f MSa/s\n",
           (double)CPU_HZ / (double)ISR_CYCLES_PESS / 1e6 / 2.0);

    /* 上面 part_capacity 用的是 2.25 MSa/s 这个"乐观上限"，两处必须一致：
     * 否则表格看起来能跑，代码里却设了一个跑不到的采样率。 */
    assert(CPU_HZ / ISR_CYCLES_OPT == 2250000UL);
    assert(CPU_HZ / ISR_CYCLES_PESS == 1800000UL);

    printf("\n结论：定时器采样在 F103 上的实际上限在 1.8–2.25 MSa/s 之间，\n");
    printf("      取决于编译器优化与 ISR 里的实际指令数（本机没有板，无法实测）。\n");
    printf("      工程上取 1 MSa/s 留余量；再高就要考虑 DMA 搬 GPIO（F1 没有此项能力）。\n");

    printf("\n抖动来源（比平均开销更难缠）：\n");
    printf("  · 若 UART RX 中断优先级高于采样中断，每个字节会插队 87 µs 一次\n");
    printf("  · SysTick 1 ms 一次，开销相对可忽略\n");
    printf("  · 后果不是\"少采几个点\"，而是时间轴不均——FFT 与协议解码都会因此判错\n");
    printf("  对策：采样期间关全局中断（会丢串口字节）或把采样交给硬件触发（F1 无 DMA-on-GPIO）\n");
}

/* ---- 6. 时间戳方案：精度与用途 --------------------------------------- */

static void part_timestamp(void)
{
    const double cycle_ns = 1e9 / (double)CPU_HZ;

    hr("6. 对照：EXTI + DWT 时间戳方案");
    printf("  DWT->CYCCNT 每周期 +1，@%lu MHz → 分辨率 %.2f ns（一次采样 1 µs 的 1/72）\n",
           (unsigned long)(CPU_HZ / 1000000UL), cycle_ns);
    printf("  时间戳能测到边沿的 1/72 µs，但**不能**定速采样：它只记事件发生的时刻。\n");

    printf("\n  本机限制：QEMU 的 stm32vldiscovery 未建模 DWT——\n");
    printf("  Linux/tests/qemu-stm32/logs/neg_f103_09_delay_precision.log 里\n");
    printf("  CYCCNTENA 读回 0，周期列恒为 0。所以本实验的固件在 QEMU 里只能证明\"编进去了\"，\n");
    printf("  证明不了\"采到了\"。这也是本实验把可判定部分全部搬到主机端的原因。\n");
}

int main(void)
{
    printf("== F103_20 逻辑分析仪：容量与带宽算术（理论估算，非实测）==\n");
    printf("平台假设：STM32F103C8T6，HCLK 72 MHz，SRAM 20 KB，8 通道打包 1 B/样本\n");

    part_capacity();
    part_packing();
    part_uart_ceiling();
    part_quantization();
    part_isr_ceiling();
    part_timestamp();

    printf("\n---- 全部 assert 通过 ----\n");
    return 0;
}
