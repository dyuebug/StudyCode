/*
 * lab_ir_w25q64.c —— Day19 红外收发与 SPI FLASH：主机端可复算判据
 *
 * 目的：把 NEC 红外收发的时序记账、以及 W25Q64 驱动的地址/返回值算术，
 *       按驱动的真实配置在主机上复算一遍。凡是依赖电平、真实波形、
 *       芯片行为的结论一律不在这里断言（那是示波器/逻辑分析仪/真板的事）。
 *
 * 本仓库事实（源码核对得出）：
 *   driver_timer.c:53  udelay() 直接轮询 TIM4 计数器（__HAL_TIM_GET_COUNTER），
 *                      不经过 HAL_GetTick()          => 发送方向的延时是准的
 *   driver_timer.c:135 system_get_ns() = HAL_GetTick()*1e6 + cnt*1e6/reload
 *                      HAL_GetTick() 恒 0（见 Day18 §6）=> 返回值落在 [0, 1e6) ns
 *   driver_ir_receiver.c:244  if (time - pre_time > 100000000) cnt = 0;
 *                      两个 <1e6 的值相减，跨毫秒边界时 uint64 下溢成天文数字
 *   driver_spiflash_w25q64.c:360      if ((offset & 4095) && (len & 4095)) return -1;
 *   driver_spiflash_w25q64.c:162-177  while (timeout--) ... if (!timeout) return -1;
 *
 * 覆盖：
 *   A  NEC 边沿记账：68 边沿的构成、解析索引、引导码窗口、位判据
 *   B  中断计数复位逻辑：坏时基下 cnt 能到多大；好时基下解析是否真的正确
 *   C  发送方向的延时为什么不受影响（udelay 的 tick 算术）
 *   D  W25Q64_Erase 的 && / || 判据
 *   E  W25Q64_WaitReady 的返回值与它如何吃掉 Write 的错误检查
 *   F  回读自测的 payload 熵：system_get_ns() 的高位永远是 0
 *   G  W25Q64_Read 的 uint32_t -> uint16_t 截断
 *
 * 编译：gcc -std=c11 -Wall -Wextra -O2 lab_ir_w25q64.c -o lab_ir_w25q64
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
 * 时基模型（与 stm32f1xx_hal_timebase_tim.c 一致：TIM4 = 1 MHz / ARR 999）
 * ====================================================================== */
#define TIM4_ARR       999ULL
#define TIM4_PERIOD_NS 1000000ULL          /* (999+1) * 1 us */

/* driver_timer.c:135，uwTick 恒 0 => 只保留 1 ms 周期内的位置 */
static uint64_t ts_broken(uint64_t real_ns)
{
    uint64_t cnt = (real_ns % TIM4_PERIOD_NS) / 1000ULL;
    return cnt * 1000000ULL / TIM4_ARR;
}

/* 同一个函数在 uwTick 正常递增时的行为（对照） */
static uint64_t ts_fixed(uint64_t real_ns)
{
    uint64_t cnt = (real_ns % TIM4_PERIOD_NS) / 1000ULL;
    return (real_ns / TIM4_PERIOD_NS) * 1000000ULL + cnt * 1000000ULL / TIM4_ARR;
}

/* ========================================================================
 * NEC 时序（driver_ir_sender.c:103-135）
 * ====================================================================== */
#define NEC_LEADER_LOW_NS   9000000ULL
#define NEC_LEADER_HIGH_NS  4500000ULL
#define NEC_BIT_LOW_NS       560000ULL
#define NEC_BIT1_HIGH_NS    1600000ULL
#define NEC_BIT0_HIGH_NS     560000ULL
#define NEC_STOP_LOW_NS      560000ULL

#define NEC_EDGES            68

/* 按发送端时序生成 68 个边沿的真实时刻（低有效接收头：有载波=低） */
static int build_edges(uint8_t device, uint8_t data, uint64_t *edges)
{
    uint8_t datas[4];
    uint64_t t;
    int i, idx = 0;

    datas[0] = device;
    datas[1] = (uint8_t)~device;
    datas[2] = data;
    datas[3] = (uint8_t)~data;

    edges[idx++] = 0;                                  /* 引导码载波起点 */
    edges[idx++] = NEC_LEADER_LOW_NS;                  /* 引导码结束 */
    t = NEC_LEADER_LOW_NS + NEC_LEADER_HIGH_NS;
    edges[idx++] = t;                                  /* bit0 载波起点 */

    for (i = 0; i < 32; i++) {
        int bit = (datas[i / 8] >> (7 - (i % 8))) & 1;
        t += NEC_BIT_LOW_NS;
        edges[idx++] = t;                              /* 载波结束 */
        t += bit ? NEC_BIT1_HIGH_NS : NEC_BIT0_HIGH_NS;
        edges[idx++] = t;                              /* 下一位载波起点 */
    }
    t += NEC_STOP_LOW_NS;
    edges[idx++] = t;                                  /* 停止位载波结束 */
    return idx;
}

/* 复刻 driver_ir_receiver.c:131 的解析（1.0 ms 门限、1/3 字节取反校验） */
static int nec_parse(const uint64_t *tm, uint8_t *pdev, uint8_t *pdata)
{
    uint8_t datas[4] = { 0, 0, 0, 0 };
    uint64_t span;
    int i, m, n, byte = 0, bits = 0;
    uint8_t data = 0;

    span = tm[1] - tm[0];
    if (span < 8000000ULL || span > 10000000ULL) return -1;
    span = tm[2] - tm[1];
    if (span < 3500000ULL || span > 55000000ULL) return -1;   /* 上界多一个零 */

    for (i = 0; i < 32; i++) {
        m = 3 + i * 2;
        n = m + 1;
        span = tm[n] - tm[m];
        data <<= 1;
        bits++;
        if (span > 1000000ULL) data |= 1;
        if (bits == 8) { datas[byte] = data; byte++; data = 0; bits = 0; }
    }
    datas[1] = (uint8_t)~datas[1];
    datas[3] = (uint8_t)~datas[3];
    if (datas[0] != datas[1] || datas[2] != datas[3]) return -1;

    *pdev  = datas[0];
    *pdata = datas[2];
    return 0;
}

/* 复刻 driver_ir_receiver.c:232 的中断回调，返回 cnt 到达过的最大值 */
static int sim_irq(const uint64_t *real, int n, int fixed, int *frames, int *repeat_hits)
{
    uint64_t pre_time = 0;
    int cnt = 0, max = 0, i;

    if (frames) *frames = 0;
    if (repeat_hits) *repeat_hits = 0;

    for (i = 0; i < n; i++) {
        uint64_t time = fixed ? ts_fixed(real[i]) : ts_broken(real[i]);

        if (time - pre_time > 100000000ULL) {   /* 驱动 :244 */
            cnt = 0;
        }
        pre_time = time;

        cnt++;
        if (cnt > max) max = cnt;

        if (cnt == 4 && repeat_hits) (*repeat_hits)++;   /* 重复码分支被走到 */
        if (cnt == NEC_EDGES) {                          /* 解析分支 */
            if (frames) (*frames)++;
            cnt = 0;
        }
    }
    return max;
}

/* ========================================================================
 * A 组：NEC 边沿记账
 * ====================================================================== */
static void a_nec_edges(void)
{
    uint64_t edges[NEC_EDGES + 8];
    int n, i, maxidx = -1;

    printf("[A] NEC 边沿记账（driver_ir_receiver.c:44、:131-200）\n");

    n = build_edges(0x55, 0xa2, edges);
    printf("  按发送端时序生成的边沿数 = %d\n", n);
    CHECK(n == NEC_EDGES,
          "68 = 1（引导起点）+ 2（引导码）+ 64（32 位 x 2）+ 1（停止位）");

    for (i = 0; i < 32; i++) {
        int m = 3 + i * 2, nn = m + 1;
        if (nn > maxidx) maxidx = nn;
    }
    printf("  解析访问的最大下标 = %d，数组容量 = %d\n", maxidx, NEC_EDGES);
    CHECK(maxidx == 66,
          "m=3+2i / n=m+1 覆盖 3..66，下标 67（停止位结束）从不读取");
    CHECK(maxidx == NEC_EDGES - 2,
          "容量 68 只比最大下标多 1 => 紧配，既没有溢出也没有富余");

    /* 引导码高电平窗口：驱动 3.5-55 ms，NEC 名义 4.5 ms */
    {
        double lo = 3.5, hi = 55.0, nom = 4.5;
        printf("  引导码高电平：名义 %.1f ms，窗口 %.1f-%.1f ms\n", nom, lo, hi);
        CHECK(hi / nom > 10.0,
              "上界是名义值的 %.1f 倍 => 源码写成 55000000 而非 5500000（多一个零）",
              hi / nom);
        CHECK(hi / 5.5 == 10.0,
              "若按 5.5 ms 的正常上界写，比值应为 1.22 ==> 这个零是笔误不是设计");
    }

    /* 数据位门限：0.56 ms vs 1.69 ms，判据 1.0 ms */
    {
        double t0 = 0.56, t1 = 1.69, th = 1.0;
        printf("  位判据：'0'=%.2f ms，'1'=%.2f ms，门限=%.2f ms\n", t0, t1, th);
        CHECK(th > t0 * 1.25 && th < t1 * 0.75,
              "门限落在两侧 25%% 容差内（'0' 上限 %.2f、'1' 下限 %.2f）=> 判据本身没问题",
              t0 * 1.25, t1 * 0.75);
    }
}

/* ========================================================================
 * B 组：中断计数复位逻辑
 * ====================================================================== */
static void b_irq_counter(void)
{
    uint64_t edges[NEC_EDGES + 8];
    int n, max_broken, max_fixed, frames, repeat_hits;

    printf("[B] 中断回调的计数复位（driver_ir_receiver.c:232-278）\n");

    n = build_edges(0x55, 0xa2, edges);
    printf("  帧长 %.2f ms，共 %d 个边沿\n", (double)edges[n - 1] / 1e6, n);

    CHECK(ts_broken(5000000ULL) < TIM4_PERIOD_NS,
          "坏时基下 5 ms 处返回 %llu ns（<1 ms）=> 所有时刻被压进同一个毫秒窗口",
          (unsigned long long)ts_broken(5000000ULL));

    max_broken = sim_irq(edges, n, 0, &frames, &repeat_hits);
    printf("  坏时基：cnt 最大 %d，解析触发 %d 次，重复码分支 %d 次\n",
           max_broken, frames, repeat_hits);
    CHECK(max_broken < 4,
          "cnt 最多到 %d => `if (cnt==4)` 与 `if (cnt==68)` 两个分支永不成立",
          max_broken);
    CHECK(frames == 0 && repeat_hits == 0,
          "整帧 %d 个边沿走完，解析 0 次、重复码 0 次 => IRReceiver_Read 永远返回 -1", n);

    max_fixed = sim_irq(edges, n, 1, &frames, &repeat_hits);
    printf("  好时基：cnt 最大 %d，解析触发 %d 次\n", max_fixed, frames);
    CHECK(max_fixed == NEC_EDGES && frames == 1,
          "对照：时基正常时 cnt 恰好到 %d 并触发 %d 次解析 => 断的是时基，不是解析逻辑",
          max_fixed, frames);

    /* 好时基下把解析真的跑一遍，验证索引算术与门限 */
    {
        uint64_t tm[NEC_EDGES];
        uint8_t dev = 0, dat = 0;
        int ok = 0, round = 0;
        static const uint8_t devs[]  = { 0x00, 0x55, 0xff };
        static const uint8_t datas[] = { 0xa2, 0x00, 0xff, 0xe2 };
        size_t x, y;
        int i;

        for (x = 0; x < sizeof(devs); x++) {
            for (y = 0; y < sizeof(datas); y++) {
                build_edges(devs[x], datas[y], edges);
                for (i = 0; i < NEC_EDGES; i++) tm[i] = ts_fixed(edges[i]);
                round++;
                if (nec_parse(tm, &dev, &dat) == 0 &&
                    dev == devs[x] && dat == datas[y]) ok++;
            }
        }
        printf("  解码往返：%d/%d 组 (device,data) 还原正确\n", ok, round);
        CHECK(ok == round,
              "好时基下 %d 组全部还原 => m=3+2i 对齐的是高电平段，1.0 ms 门限也正确",
              round);
    }

    /* isRepeatedKey 在坏时基下恒为假 */
    {
        uint64_t d = edges[1] - edges[0];
        uint64_t broken = ts_broken(edges[1]) - ts_broken(edges[0]);
        printf("  引导码间隔：真实 %llu ns，坏时基算得 %llu ns\n",
               (unsigned long long)d, (unsigned long long)broken);
        CHECK(broken < 8000000ULL && d == NEC_LEADER_LOW_NS,
              "isRepeatedKey 首条判据 `< 8000000` 恒真 => 重复码分支同样进不去");
    }
}

/* ========================================================================
 * C 组：发送方向的延时为什么不受影响
 * ====================================================================== */
static uint32_t udelay_ticks(int us)
{
    /* driver_timer.c:74  ticks = us * reload / 1000;  reload = ARR = 999 */
    return (uint32_t)((uint64_t)us * TIM4_ARR / 1000ULL);
}

static void c_sender_timing(void)
{
    uint64_t edges[NEC_EDGES + 8];
    int n;

    printf("[C] 发送方向的延时（driver_timer.c:53-105、driver_ir_sender.c:93-136）\n");
    printf("  udelay(9000)=%u tick，udelay(1600)=%u，udelay(560)=%u，udelay(1)=%u\n",
           udelay_ticks(9000), udelay_ticks(1600),
           udelay_ticks(560), udelay_ticks(1));

    CHECK(udelay_ticks(9000) == 8991,
          "9000 us 实等 %u 个 1 us tick => 8991 us，误差 -0.1%%，仍在接收端 8-10 ms 窗口内",
          udelay_ticks(9000));
    CHECK(udelay_ticks(1) == 0,
          "udelay(1) 算出 %u 个 tick => 只等到计数器变化一次（0-1 us，均值 0.5 us）",
          udelay_ticks(1));
    CHECK(udelay_ticks(1000) == 999,
          "mdelay(1)=udelay(1000)=%u tick => 0.999 ms，误差 -0.1%%",
          udelay_ticks(1000));

    /* 这一条是对 Day18 §9 的修正：SR04 的轮询超时被 udelay(1) 折半 */
    {
        double real_us = 20000.0 * 0.5;            /* 每轮 udelay(1) 均值 0.5 us */
        double max_cm  = real_us * 1e-6 * 340.0 * 100.0 / 2.0;
        printf("  SR04：20000 轮 udelay(1) 名义 20000 us，按 0 tick 折算约 %.0f us\n", real_us);
        CHECK(max_cm < 340.0,
              "Day18 §9 的 340 cm 是名义上界；udelay(1) 只花一半时间 => 实际约 %.0f cm", max_cm);
    }

    n = build_edges(0x55, 0xa2, edges);
    printf("  (device=0x55,data=0xa2) 帧长 %.2f ms，共 %d 个边沿\n",
           (double)edges[n - 1] / 1e6, n);
    CHECK(edges[n - 1] > 60000000ULL && edges[n - 1] < 70000000ULL,
          "全帧 %.1f ms 落在 NEC 名义 67.5 ms 附近 => 发送端时序自洽",
          (double)edges[n - 1] / 1e6);

    /* 发送端每段载波一次 Start/Stop；接收端每个载波贡献一对边沿 —— 是同一件事的两面 */
    {
        int bursts = 1 + 32 + 1;                 /* 引导 + 32 位 + 停止位 */
        printf("  PWM Start/Stop 各 %d 次 => %d 个载波段 => %d 个边沿\n",
               bursts, bursts, bursts * 2);
        CHECK(bursts * 2 == n,
              "发送端 %d 次 Start/Stop 与接收端 %d 个边沿对得上 => 两端的记账一致",
              bursts, n);
    }
}

/* ========================================================================
 * D 组：W25Q64_Erase 的对齐判据
 * ====================================================================== */
static int gate_and(uint32_t off, uint32_t len) { return (off & 4095U) && (len & 4095U); }
static int gate_or (uint32_t off, uint32_t len) { return (off & 4095U) || (len & 4095U); }

/* 复刻 driver_spiflash_w25q64.c:354-397 的扇区循环。
 * 返回擦除的扇区数；*cmd_addr 为命令里发出的地址；*first_base 为芯片实际擦的扇区基址
 * （W25Q64 的 0x20 命令忽略 A11-A0，芯片自己把地址截断）；*rejected 为是否被判据拒绝。 */
static int erase_sim(uint32_t off, uint32_t len, uint32_t *cmd_addr,
                     uint32_t *first_base, int *rejected)
{
    int sectors = 0;
    uint32_t phy = off;

    *rejected = 0;
    if (gate_and(off, len)) {          /* 驱动 :360 */
        *rejected = 1;
        return 0;
    }
    for (uint32_t s = 0; s < len / 4096U; s++) {
        if (sectors == 0) {
            *cmd_addr   = phy;
            *first_base = phy & ~0xFFFU;   /* 芯片侧截断，不是驱动做的 */
        }
        sectors++;
        phy += 4096U;
    }
    return sectors;
}

static void d_erase_gate(void)
{
    static const struct { uint32_t off, len; const char *note; } cases[] = {
        { 0,    4096, "both aligned" },
        { 4096, 4096, "both aligned" },
        { 0,    100,  "off ok, len bad" },
        { 100,  4096, "off bad, len ok" },
        { 100,  100,  "both bad" },
    };
    size_t i;
    int off_diag = 0;

    printf("[D] W25Q64_Erase 的对齐判据（driver_spiflash_w25q64.c:360）\n");
    printf("  %-6s %-6s %-16s %-8s %-8s\n", "off", "len", "case", "&&ok", "||ok");

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        int a = !gate_and(cases[i].off, cases[i].len);
        int o = !gate_or(cases[i].off, cases[i].len);
        printf("  %-6u %-6u %-16s %-8s %-8s\n",
               cases[i].off, cases[i].len, cases[i].note,
               a ? "accept" : "reject", o ? "accept" : "reject");
        if (a && !o) off_diag++;
    }
    CHECK(off_diag == 2,
          "`&&` 只拒绝「两个都错」的象限，放过 2 个「只错一个」的象限 —— 危险的恰恰是后者");

    /* 情形一：对齐 off + 非对齐 len => 声称擦了 len 字节，循环实际 0 次 */
    {
        uint32_t cmd = 0xdeadbeef, base = 0xdeadbeef;
        int rejected = 0;
        int sectors = erase_sim(0, 100, &cmd, &base, &rejected);
        printf("  Erase(0, 100)：判据通过=%d，扇区循环 %d 次，驱动返回值 = len = 100\n",
               !rejected, sectors);
        CHECK(!rejected && sectors == 0 && 100 / 4096 == 0,
              "返回 100（声称成功），但 100/4096 = 0 => 一个扇区都没擦，返回值在撒谎");
    }

    /* 情形二：非对齐 off + 对齐 len => 擦的是往下取整的扇区 */
    {
        uint32_t cmd = 0xdeadbeef, base = 0xdeadbeef;
        int rejected = 0;
        int sectors = erase_sim(100, 4096, &cmd, &base, &rejected);
        printf("  Erase(100, 4096)：判据通过=%d，擦 %d 个扇区\n", !rejected, sectors);
        printf("    命令里的地址 = 0x%08X，芯片实际擦的扇区基址 = 0x%08X\n", cmd, base);
        CHECK(!rejected && sectors == 1 && cmd == 100 && base == 0,
              "请求从 0x%X 起擦，命令原样发 0x%X，芯片截断后擦 0x%X => 静默越界擦除",
              (unsigned)100, cmd, base);
    }
}

/* ========================================================================
 * E 组：W25Q64_WaitReady 的返回值
 * ====================================================================== */
/* 驱动 :162-177 的循环，返回退出时 timeout 的值 */
static int wait_ready_exit(int init, int ready)
{
    int timeout = init;
    while (timeout--) {                    /* 后置自减：先取值，再减 1 */
        if (ready) return timeout;
    }
    return timeout;                        /* 耗尽退出的值 */
}

/* 驱动 :173-177 的返回值 */
static int wait_ready_ret(int init, int ready)
{
    int timeout = wait_ready_exit(init, ready);
    if (!timeout) return -1;
    return 0;
}

/* Day18 SR04 用的是 uint32_t 版本，耗尽值与上面相反方向地"绕" */
static uint32_t timeout_exit_u32(uint32_t init)
{
    uint32_t t = init;
    while (t--) { }
    return t;
}

static void e_wait_ready(void)
{
    int exit_val = wait_ready_exit(500, 0);
    int ret      = wait_ready_ret(500, 0);

    printf("[E] W25Q64_WaitReady 的返回值（driver_spiflash_w25q64.c:153-178）\n");
    printf("  芯片永不就绪：循环退出时 timeout=%d，!timeout=%d，函数返回 %d\n",
           exit_val, !exit_val, ret);

    CHECK(exit_val == -1,
          "int 型 timeout 耗尽后是 %d（不是 0）=> `if (!timeout) return -1` 不可达", exit_val);
    CHECK(ret == 0,
          "芯片永远不就绪仍返回 %d（0 = 成功）=> 唯一的失败返回是死代码", ret);
    CHECK(!exit_val == 0 && exit_val != 0,
          "耗尽的 int 是 %d，不为 0 => `!timeout` 为假", exit_val);
    CHECK(timeout_exit_u32(5) == 0xFFFFFFFFU && wait_ready_exit(5, 0) == -1,
          "对照 Day18/E7：uint32_t 耗尽得 0x%08X、int 耗尽得 %d，两者都不等于 0 => 两处都是死代码",
          timeout_exit_u32(5), wait_ready_exit(5, 0));
    CHECK(ret == 0 && exit_val == -1,
          "但方向相反：SR04 是「该报错没报」，WaitReady 是「该报错反而报成功」");

    /* 组合缺陷 1：Write 的 err 检查因此失效 */
    {
        int err = wait_ready_ret(500, 0);              /* 驱动 :328 */
        printf("  W25Q64_Write 的 `if (err)`：err=%d => %s\n",
               err, err ? "判失败" : "判成功");
        CHECK(err == 0,
              "WaitReady 永不失败 => Write 的 if (err) 永不成立 => 空芯片也报「写了 4 字节」");
    }

    /* 组合缺陷 2：Erase 里漏写了 err = */
    {
        int err_after_write_enable = 0;                /* WriteEnable 成功后残留 */
        int tx_result = -1;                            /* 本次 Tx 实际失败 */
        int detected = err_after_write_enable;         /* 驱动 :378 没有 err = */
        (void)tx_result;
        printf("  W25Q64_Erase 发送擦除命令后 err 仍是旧值 %d（Tx 实际返回 %d）\n",
               detected, tx_result);
        CHECK(detected == 0 && tx_result == -1,
              "命令发送失败而 err 仍为 0 => 擦除命令发没发出去无人知道");
    }
}

/* ========================================================================
 * F 组：回读自测的 payload 熵
 * ====================================================================== */
static void f_test_payload(void)
{
    uint64_t max_ns = 0, v, t;
    uint32_t max_val;
    unsigned bits_used = 0, i;

    printf("[F] W25Q64_Test 的 payload（driver_spiflash_w25q64.c:445-458）\n");

    for (t = 0; t <= TIM4_PERIOD_NS; t += 1000ULL) {
        v = ts_broken(t);
        if (v > max_ns) max_ns = v;
    }
    max_val = (uint32_t)max_ns;
    printf("  system_get_ns() 的最大值 = %u (0x%08X)\n", max_val, max_val);

    CHECK(max_val <= 1000000U,
          "payload 上界 %u < 2^20 => 32 位里有 12 位从未被写过", max_val);

    printf("  按 4 字节小端写盘：byte3 = 0x%02X，byte2 <= 0x%02X\n",
           (max_val >> 24) & 0xFF, (max_val >> 16) & 0xFF);
    CHECK(((max_val >> 24) & 0xFF) == 0,
          "byte3 恒为 0x00 => 任何「高字节粘 0」的故障都能通过 8 MB 全盘自测");
    CHECK(((max_val >> 16) & 0xFF) <= 0x0F,
          "byte2 只有低 4 位会动（<=0x%02X）=> 高 4 位的故障同样检不出",
          (max_val >> 16) & 0xFF);

    for (i = 0; i < 32; i++)
        if (max_val & (1U << i)) bits_used++;
    printf("  实际会取到 1 的位：%u/32（上界 %u = 0x%08X 的 popcount）\n",
           bits_used, max_val, max_val);
    CHECK(bits_used == 7,
          "32 个位里只有 %u 个可能为 1，其余 25 位恒定 => val1==val2 只覆盖这 7 位的往返",
          bits_used);
    printf("  结论：payload 退化不是自测逻辑的错，而是同一个坏时基的第三个受害者\n");
}

/* ========================================================================
 * G 组：W25Q64_Read 的 uint16_t 截断
 * ====================================================================== */
static void g_size_truncate(void)
{
    uint32_t len = 70000;
    uint16_t size = (uint16_t)len;

    printf("[G] W25Q64_Read 的长度截断（driver_spiflash_w25q64.c:231-261）\n");
    printf("  len=%u 传给 uint16_t Size => %u；函数末尾仍 return len = %u\n",
           len, size, len);
    CHECK(size != len,
          "单次超过 65535 字节的读只搬 %u 字节，却报成功 %u", size, len);
    CHECK(size == (uint16_t)(len & 0xFFFFU),
          "截断是静默的：无警告、无返回码变化 => 调用方无法察觉");
}

/* ====================================================================== */

int main(int argc, char **argv)
{
    int inject = 0, i;

    for (i = 1; i < argc; i++)
        if (strcmp(argv[i], "--inject") == 0 && i + 1 < argc)
            inject = atoi(argv[++i]);

    printf("lab_ir_w25q64 — Day19 主机端可复算判据\n");
    printf("模型：TIM4 = 1 MHz / ARR 999；uwTick 恒 0 => system_get_ns() 属于 [0, 1e6)\n\n");

    a_nec_edges();       printf("\n");
    b_irq_counter();     printf("\n");
    c_sender_timing();   printf("\n");
    d_erase_gate();      printf("\n");
    e_wait_ready();      printf("\n");
    f_test_payload();    printf("\n");
    g_size_truncate();   printf("\n");

    if (inject > 0) {
        printf("[注入] --inject %d\n", inject);
        switch (inject) {
        case 1:
            CHECK(ts_broken(9000000ULL) >= NEC_LEADER_LOW_NS,
                  "注入：声称坏时基能表示 9 ms（应失败）");
            break;
        case 2: {
            uint64_t edges[NEC_EDGES + 8];
            int n = build_edges(0x55, 0xa2, edges);
            int frames = 0;
            sim_irq(edges, n, 0, &frames, NULL);
            CHECK(frames > 0, "注入：声称坏时基下解析触发 %d 次（实为 0，应失败）", frames);
            break;
        }
        case 3: {
            uint32_t cmd = 0, base = 0;
            int rejected = 0;
            erase_sim(0, 100, &cmd, &base, &rejected);
            CHECK(rejected, "注入：声称 Erase(0,100) 被拒绝（实为通过，应失败）");
            break;
        }
        case 4:
            CHECK(wait_ready_ret(500, 0) == -1,
                  "注入：声称 WaitReady 超时返回 -1（实为 0，应失败）");
            break;
        case 5: {
            uint8_t dev = 0, dat = 0;
            uint64_t tm[NEC_EDGES];
            uint64_t edges[NEC_EDGES + 8];
            int k;
            build_edges(0x55, 0xa2, edges);
            for (k = 0; k < NEC_EDGES; k++) tm[k] = ts_broken(edges[k]);
            CHECK(nec_parse(tm, &dev, &dat) == 0,
                  "注入：声称坏时基的时间序列仍能解析（应失败）");
            break;
        }
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
