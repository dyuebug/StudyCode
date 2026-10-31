/**
 * F103_20：逻辑分析仪（8 通道、buffer 模式）
 *
 * 教材第 4 章（hal.txt:223-262）给了方案与取舍，没给代码。本文件是把它落到
 * F103C8T6 上的实现，并把"哪里能被判定、哪里不能"分开：
 *
 *   · 采样源、时钟、环形缓冲、帧格式、UART 还原都在 la_core.c —— 与硬件无关，
 *     由 host/lab_la_proto.c 在没有板的机器上逐条判定（54 项断言）。
 *   · 本文件只负责三件与硬件有关的事：定时器节拍、读 IDR、把样本流打出去。
 *
 * 采样率算术（详见 host/lab_la_arith.c 的实测输出）：
 *   TIM2 时钟 = 72 MHz（APB1 = 36 MHz，分频 2 时 TIMxCLK = 2 × PCLK1）
 *   PSC = 71 → 计数频率 1 MHz
 *   ARR = 9  → 更新频率 1 MHz / 10 = 100 kSa/s（每 10 µs 一个样本）
 *   512 个样本 → 5.12 ms 窗口
 *   为什么不是 1 MSa/s：一次采样中断约 32–40 周期，100% CPU 也只能到
 *   1.8–2.25 MSa/s；给主循环留出余量后取 100 kSa/s 是保守值。
 *
 * 本机限制（实测，见 Linux/tests/qemu-stm32/logs/）：
 *   neg_f103_02_gpio_input.log 是空日志、neg_f103_09_delay_precision.log 里
 *   DWT->CYCCNTENA 读回 0 —— QEMU 的 stm32vldiscovery 既不可用 GPIO 输入，
 *   也不建模 DWT，定时器计数同样不推进。所以：
 *     1. 采样源在 LA_SYNTHETIC=1 时改成按时间合成的电平序列（CMake 在 F103_QEMU 下自动置 1），
 *        否则仿真里永远采到全 0，"跑通"是假的；
 *     2. 启动先做一次 TIM2 计数探针，明确打印"在计数"还是"卡住"——
 *        F103_17 的教训是：未建模外设会让"等待清零/等待计数"类判据假通过，
 *        判据必须能区分"没响应"与"做完了"；
 *     3. 定时器卡住时降级为主循环节拍采样（mode=sw），把整条数据链路仍走一遍并留下读数。
 *
 * 未实现（如实记录，不写成已完成）：
 *   · 外部触发（EXTI 上升沿启动采集）—— 需要真板才能判"触发时刻对不对"
 *   · 采样期间关中断以保证时间轴均匀 —— 会影响串口收字节，需真板权衡
 *   · SUMP 二进制协议与 PulseView 对接 —— 本工程用 ASCII 流，两者不能互相解析
 *   · 任何板上现象 —— 本机无板、无示波器、无逻辑分析仪
 */
#include <stdint.h>
#include <stdio.h>

#include "stm32f1xx.h"
#include "f103_tick.h"
#include "f103_uart.h"
#include "la_core.h"

/* CMake 在 F103_QEMU 下传 -DLA_SYNTHETIC=1；真板默认 0（读真实引脚） */
#ifndef LA_SYNTHETIC
#  define LA_SYNTHETIC 0
#endif

#define UART_BAUD   115200U
#define LA_MASK     0x00FFUL     /* PA0..PA7，8 个通道 */

/* 缓冲 512 个样本：QEMU 的链接脚本只有 8 KB SRAM，512 × 2（环形 + 线性副本）= 1 KB
 * 留足余量。真板 20 KB 可以放到 16 KB，窗口随之 ×32（算术见 lab_la_arith 第 1 节）。 */
#define BUF_SAMPLES 512u

/* 合成源参数：'L','A','!' 三个字节的 8N1 波形，每 bit 8 个采样点。
 * 240 + 16 个空闲样本 = 256，正好装进 512 的缓冲。
 * 注意这是"给解码器用的波形"，不是真实波特率：8 采样/bit × 10 µs = 80 µs/bit = 12.5 kBd。
 * 真实引脚路径下不存在这个参数：100 kSa/s 采样 115200 bps 时 1 bit 只有 0.87 个采样点，
 * 根本还原不了——这也是真板上必须提高采样率或用 EXTI 打时间戳的原因。 */
#if LA_SYNTHETIC
#  define SYNTH_SPP   8u
#  define SYNTH_IDLE  16u
static const uint8_t k_msg[3] = {'L', 'A', '!'};
#endif

static uint8_t  g_buf[BUF_SAMPLES];
static uint8_t  g_flat[BUF_SAMPLES];
static la_ring  g_ring;
static uint32_t g_synth_idx;

/* ---- 故障钩子：未建模地址的访问会升级成 HardFault ---------------------- */
static void fault_report(const char *name)
{
    printf("\r\n[FAULT] %s: CFSR=0x%08lX HFSR=0x%08lX BFAR=0x%08lX\r\n",
           name, (unsigned long)SCB->CFSR, (unsigned long)SCB->HFSR,
           (unsigned long)SCB->BFAR);
    printf("[FAULT] 本实验在 QEMU 里若走到这里，说明访问了未映射的外设段；\r\n");
    printf("[FAULT] 这不代表真板行为，只说明该段没有模型。\r\n");
    for (;;) {
        __asm volatile("nop");
    }
}

void BusFault_Handler(void)   { fault_report("BusFault"); }
void HardFault_Handler(void)  { fault_report("HardFault"); }

/* ---- 采样源 ------------------------------------------------------------ */

/* 合成的 8N1 电平序列：通道 0 是波形，通道 1..7 固定为 0x2A。
 * 固定图案的用处：解码只取通道 0，但如果打包把未驱动的通道写成别的值，
 * 主机端能立刻从样本行里看出来（0x2A | level 的关系被破坏）。 */
#if LA_SYNTHETIC
static uint8_t synth_level(uint32_t idx)
{
    const uint32_t frame_len = 10u * SYNTH_SPP;
    const uint32_t total = 3u * frame_len;
    int32_t rel = (int32_t)idx - (int32_t)SYNTH_IDLE;
    uint8_t lvl;

    if (rel < 0 || (uint32_t)rel >= total) {
        return (uint8_t)(0x2Au | 0x01u);          /* 空闲高 */
    }
    {
        uint32_t bit_i = ((uint32_t)rel % frame_len) / SYNTH_SPP;
        uint32_t byte_i = (uint32_t)rel / frame_len;

        if (bit_i == 0u) {
            lvl = 0u;                             /* 起始位 */
        } else if (bit_i == 9u) {
            lvl = 1u;                             /* 停止位 */
        } else {
            lvl = (uint8_t)((k_msg[byte_i] >> (bit_i - 1u)) & 1u);  /* LSB first */
        }
    }
    return (uint8_t)(0x2Au | lvl);
}
#endif /* LA_SYNTHETIC */

/* 一次采样：返回打包后的 1 字节。
 * 两种源走同一个函数，是为了让"链路其余部分"在两种情况下完全相同。 */
static uint8_t la_sample_once(void)
{
#if LA_SYNTHETIC
    return synth_level(g_synth_idx++);
#else
    return la_pack(GPIOA->IDR, LA_MASK);
#endif
}

/* ---- 定时器 ------------------------------------------------------------ */

static volatile uint32_t g_isr_calls;

static void la_timer_init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    TIM2->CR1  = 0u;                 /* 先停：改 PSC/ARR 时不要它跑 */
    TIM2->PSC  = 72u - 1u;           /* 72 MHz / 72 = 1 MHz */
    TIM2->ARR  = 10u - 1u;           /* 1 MHz / 10 = 100 kHz */
    TIM2->EGR  = TIM_EGR_UG;         /* 立即把 PSC 装进计数器，否则第一周期长度不对 */
    TIM2->SR   = 0u;                 /* UG 会置 UIF，清掉 */
    TIM2->DIER = TIM_DIER_UIE;
    TIM2->CR1  = TIM_CR1_CEN;
}

static void la_gpio_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    /* PA0..PA7 = 浮空输入（CRL 每位 4 bit：CNF=01 MODE=00 → 0x4）。
     * 逐位写而不是整个 CRL = 0x44444444，避免把 PA8..PA15 一起改掉。 */
    GPIOA->CRL = 0x44444444UL;
}

void TIM2_IRQHandler(void)
{
    if ((TIM2->SR & TIM_SR_UIF) != 0u) {
        TIM2->SR = (uint32_t)~TIM_SR_UIF;        /* rc_w0：写 0 清除 */
        g_isr_calls++;
        (void)la_ring_push(&g_ring, la_sample_once());
    }
}

/* ---- 探针：定时器到底在不在计数 ---------------------------------------- */

/* 返回 1 = 计数在推进，0 = 卡住。
 * 只比较两次读数而不看"是否到达某个值"，是因为没到达的原因有很多
 * （时钟没开、PSC 写错、计数值被清零），而"两次读一样"只有一个解释。
 *
 * 实测（QEMU stm32vldiscovery，见 tests/qemu-stm32/logs/exp20_logicanalyzer.log）：
 *   TIM2 PSC=0 ARR=0 CR1=0x0000 DIER=0x0000
 * 写进去的值**一个都没读回来**。所以这台机器上不是"计数器不跑"，
 * 而是整个 TIM2 段没有模型、读任何寄存器都返回 0。
 * 这两件事对判据的影响完全不同：前者可以等，后者永远等不到——
 * 所以探针必须先打印寄存器回读值，再打印 CNT 变化，两行合起来才可判。 */
static int probe_timer(void)
{
    uint32_t a, b;

    printf("[probe] TIM2 PSC=%lu ARR=%lu CR1=0x%04lX DIER=0x%04lX\r\n",
           (unsigned long)TIM2->PSC, (unsigned long)TIM2->ARR,
           (unsigned long)TIM2->CR1, (unsigned long)TIM2->DIER);

    a = TIM2->CNT;
    f103_tick_delay_ms(2);           /* 2 ms 内 100 kHz 应走 ~200 个数 */
    b = TIM2->CNT;

    if (a == b) {
        printf("[probe] CNT|stuck|a=%lu b=%lu|定时器未推进\r\n",
               (unsigned long)a, (unsigned long)b);
        return 0;
    }
    printf("[probe] CNT|counting|a=%lu b=%lu\r\n",
           (unsigned long)a, (unsigned long)b);
    return 1;
}

/* ---- 采集 -------------------------------------------------------------- */

/* isr=1 走定时器中断；isr=0 降级为主循环节拍（1 ms 一个样本） */
static uint32_t capture(int use_isr, uint32_t need)
{
    uint32_t i;

    la_ring_init(&g_ring, g_buf, (uint16_t)BUF_SAMPLES);
    g_synth_idx = 0;
    g_isr_calls = 0;

    if (use_isr) {
        TIM2->CNT = 0u;
        TIM2->SR  = 0u;
        NVIC_SetPriority(TIM2_IRQn, 1u);
        NVIC_EnableIRQ(TIM2_IRQn);
        TIM2->CR1 |= TIM_CR1_CEN;

        /* 等够 need 个样本，最多等 need*2 ms（100 kSa/s 下 need/100 ms 就够） */
        for (i = 0; i < need * 20u; i++) {
            if (la_ring_len(&g_ring) >= (uint16_t)need) {
                break;
            }
            f103_tick_delay_ms(1);
        }
        TIM2->CR1 &= ~TIM_CR1_CEN;
        NVIC_DisableIRQ(TIM2_IRQn);
    } else {
        /* 降级路径：不做定时，用毫秒节拍推进。
         * 它证明不了时序精度，但能证明"打包 → 缓冲 → 帧格式 → 解码"整条链路。 */
        for (i = 0; i < need; i++) {
            (void)la_ring_push(&g_ring, la_sample_once());
            f103_tick_delay_ms(1);
        }
    }
    return la_ring_len(&g_ring);
}

/* ---- 输出 -------------------------------------------------------------- */

static void dump_samples(uint16_t n)
{
    char line[16];
    uint16_t i;

    for (i = 0; i < n; i++) {
        (void)la_fmt_sample(line, i, g_flat[i]);
        printf("[la] s=%s\r\n", line);
    }
}

int main(void)
{
    uint32_t got;
    uint16_t i;
    int counting;
#if LA_SYNTHETIC
    int nbytes;
#endif

    f103_tick_init();                /* 轮询时基：不占 SysTick 中断，采样期间不会插队 */
    f103_uart_init(UART_BAUD);
    la_ring_init(&g_ring, g_buf, (uint16_t)BUF_SAMPLES);

    printf("\r\n[la] F103_20 logic analyzer, ch=8, mask=0x%04lX\r\n",
           (unsigned long)LA_MASK);
#if LA_SYNTHETIC
    printf("[la] src=synthetic (LA_SYNTHETIC=1)\r\n");
#else
    printf("[la] src=gpio PA0..PA7\r\n");
#endif

    la_gpio_init();
    la_timer_init();
    counting = probe_timer();

    got = capture(counting, 256u);
    printf("[la] mode=%s need=256 got=%lu dropped=%lu isr=%lu\r\n",
           counting ? "isr" : "sw", (unsigned long)got,
           (unsigned long)g_ring.dropped, (unsigned long)g_isr_calls);

    /* 把环形缓冲拉平成数组：后面的解码与输出都用同一个数组，
     * 免得"解码用环形顺序、打印用线性顺序"这种两套顺序对不上。 */
    if (got > BUF_SAMPLES) {
        got = BUF_SAMPLES;
    }
    for (i = 0; i < (uint16_t)got; i++) {
        (void)la_ring_pop(&g_ring, &g_flat[i]);
    }

    /* UART 还原只在合成源下有意义：真实引脚路径 100 kSa/s 采 115200 bps，
     * 1 bit 只有 0.87 个采样点，采样定理都不满足，解出来的东西没有信息量。
     * 与其输出一个看似成功的乱码，不如明确跳过并说明原因。 */
#if LA_SYNTHETIC
    nbytes = la_uart_decode(g_flat, (uint16_t)got, 0u, SYNTH_SPP,
                            g_buf, 8u);              /* 借用 g_buf 前 8 字节做输出 */
    printf("[la] decode n=%d bytes=", nbytes);
    for (i = 0; i < (uint16_t)(nbytes > 0 ? nbytes : 0); i++) {
        printf("%02X ", g_buf[i]);
    }
    printf("\r\n");

    /* 期望 'L','A','!' = 4C 41 21。判据写在这里而不是"看着像就行"：
     * 解码器错一位就会得到别的字节，而主机端的 lab_la_proto 已经证明
     * 同一份解码实现在合成波形上是正确的——所以这里失败只可能是链路问题。 */
    if (nbytes == 3 && g_buf[0] == 'L' && g_buf[1] == 'A' && g_buf[2] == '!') {
        printf("[la] verdict=PASS\r\n");
    } else {
        printf("[la] verdict=FAIL\r\n");
    }
#else
    printf("[la] decode skipped: spp<1 at 100 kSa/s vs 115200 bps\r\n");
    printf("[la] verdict=NA\r\n");
#endif

    /* 打 32 行而不是 8 行：8 行只覆盖到空闲段，看不出波形形状；
     * 32 行能覆盖第一帧的起始位与若干数据位，日志本身就是证据。 */
    dump_samples((uint16_t)(got < 32u ? got : 32u));

    printf("[la] done\r\n");
    for (;;) {
        __asm volatile("nop");
    }
}
