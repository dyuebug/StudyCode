/**
 * lab_la_proto.c —— la_core 的主机端判定程序（不接板、不链接 HAL）
 *
 * 为什么这个程序是本实验的主要证据：
 *   QEMU 的 stm32vldiscovery 没有建模 DWT（`neg_f103_09_delay_precision.log` 里
 *   CYCCNTENA 读回 0）、也没有可用的 GPIO 输入（`neg_f103_02_gpio_input.log` 是空日志），
 *   所以"定时采样 + 读引脚"这条硬件路径在仿真里跑不出数据。
 *   能在一台没有板的机器上判定的，只有与硬件无关的那部分：打包位序、缓冲区满时策略、
 *   协议字段、UART 波形还原。这些一旦错了，板上只会表现为"波形看起来不对"，
 *   很难反过来定位。所以先在这里把它们钉死。
 *
 * 编译运行（在 experiments/F103_20_LogicAnalyzer/ 下）：
 *   gcc -std=c11 -Wall -Wextra -O2 -I. -o host/lab_la_proto host/lab_la_proto.c la_core.c
 *   ./host/lab_la_proto        # 期望末行 PASS，退出码 0
 *
 * 与固件的关系：本程序链接的 la_core.c 就是编进固件的那一份，不是"再写一遍"。
 */
#include <stdio.h>
#include <string.h>

#include "la_core.h"

static int g_pass;
static int g_fail;

/* 断言：失败也要继续跑完，一次运行拿到全部结论，而不是改一个错重跑一次 */
#define CHECK(name, cond)                                                      \
    do {                                                                       \
        if (cond) {                                                            \
            g_pass++;                                                          \
            printf("  [ok]   %s\n", (name));                                   \
        } else {                                                               \
            g_fail++;                                                          \
            printf("  [FAIL] %s  (line %d)\n", (name), __LINE__);              \
        }                                                                      \
    } while (0)

/* ---- 波形构造 ---------------------------------------------------------- */

/*
 * 把一个 UART 字节按 8N1 展开成采样序列。
 * 只驱动 ch 这一根通道，其余通道写 0 —— 单通道测试够用，
 * 而且写 0 而不是"保持原值"能顺带暴露"忘了清位"的实现错误。
 */
static void put_bit(uint8_t *buf, uint16_t *pos, uint8_t ch, uint16_t spp, uint8_t v)
{
    for (uint16_t k = 0; k < spp; k++) {
        buf[(*pos)++] = v ? (uint8_t)(1u << ch) : (uint8_t)0;
    }
}

static void put_byte(uint8_t *buf, uint16_t *pos, uint8_t ch, uint16_t spp,
                     uint8_t value, uint8_t stop)
{
    put_bit(buf, pos, ch, spp, 0);              /* 起始位 */
    for (uint8_t b = 0; b < 8u; b++) {
        put_bit(buf, pos, ch, spp, (uint8_t)((value >> b) & 1u)); /* LSB first */
    }
    put_bit(buf, pos, ch, spp, stop);           /* 停止位（传 0 即人为制造帧错误） */
}

/* ---- 各组用例 ---------------------------------------------------------- */

static void test_pack(void)
{
    printf("\n[pack] 引脚 → 字节的位序\n");

    /* mask 就取 PA0..PA7，位序应当是恒等映射：这是最容易一眼验证的对照 */
    CHECK("mask=0x00FF idr=0x5A -> 0x5A",
          la_pack(0x0000005AUL, 0x000000FFUL) == 0x5Au);
    CHECK("mask=0x00FF idr=0xFFFFFF00 -> 0x00（高 24 位不在 mask 里）",
          la_pack(0xFFFFFF00UL, 0x000000FFUL) == 0x00u);

    /* 非连续 mask：第 1 个被选的位（bit8）落到结果 bit0，第 4 个（bit11）落到 bit3。
     * 0x0A00 = bit9 | bit11 → 结果 bit1 | bit3 = 0x0A */
    CHECK("mask=0x0F00 idr=0x0A00 -> 0x0A（按引脚升序压实）",
          la_pack(0x00000A00UL, 0x00000F00UL) == 0x0Au);

    /* 超过 8 个被选位时只取前 8 个：bit8 是第 9 个，必须被丢掉 */
    CHECK("mask=0x01FF idr=0x0100 -> 0x00（第 9 个位超出 8 通道被丢弃）",
          la_pack(0x00000100UL, 0x000001FFUL) == 0x00u);
    CHECK("mask=0x01FF idr=0x00FF -> 0xFF（低 8 个位全亮）",
          la_pack(0x000000FFUL, 0x000001FFUL) == 0xFFu);

    CHECK("mask=0 -> 0（没选通道不该读出垃圾）",
          la_pack(0xFFFFFFFFUL, 0x00000000UL) == 0x00u);

    /* 位序错误的后果：若实现改成"从高引脚往低填"，上面 0x0A00 那条会得 0x50。
     * 主机端解码器按 bit0..bit7 当通道 0..7，所以位序错了整份波形会错位。 */
    printf("  说明：位序错了不会崩，只会让解码出的通道互相对调——\n");
    printf("        这类错误在板上表现为\"波形看着有、就是内容不对\"。\n");
}

static void test_ring(void)
{
    uint8_t buf[4];
    la_ring r;
    uint8_t v;

    printf("\n[ring] 满时策略与读写位置\n");

    /* 容量 4，推 6 个：应保留最早 4 个，丢弃后 2 个并计数 */
    la_ring_init(&r, buf, 4);
    for (uint8_t i = 1; i <= 6u; i++) {
        (void)la_ring_push(&r, i);
    }
    CHECK("push 6/cap 4 -> len=4", la_ring_len(&r) == 4u);
    CHECK("push 6/cap 4 -> dropped=2", r.dropped == 2u);
    CHECK("push 6/cap 4 -> total=6", r.total == 6u);

    int order_ok = 1;
    for (uint8_t i = 1; i <= 4u; i++) {
        if (la_ring_pop(&r, &v) != 1 || v != i) {
            order_ok = 0;
        }
    }
    CHECK("取出顺序 1,2,3,4（丢的是新样本，不是旧样本）", order_ok);

    /* 空缓冲 pop 必须返回 0 且不写 *out —— 这条防的是"空队列读到上一次的残值" */
    v = 0xEE;
    CHECK("空缓冲 pop 返回 0", la_ring_pop(&r, &v) == 0);
    CHECK("空缓冲 pop 不写 out（仍是 0xEE）", v == 0xEEu);

    /* 回卷：写满 → 取走 2 个 → 再写 2 个，此时 head 已绕过末尾。
     * 尾位置由 (head + cap - count) % cap 算出，这条用例专门打这个算式。 */
    la_ring_init(&r, buf, 4);
    for (uint8_t i = 1; i <= 4u; i++) {
        (void)la_ring_push(&r, i);
    }
    (void)la_ring_pop(&r, &v);              /* 1 */
    (void)la_ring_pop(&r, &v);              /* 2 */
    (void)la_ring_push(&r, 5u);
    (void)la_ring_push(&r, 6u);
    CHECK("回卷后 len=4", la_ring_len(&r) == 4u);
    CHECK("回卷后无丢弃", r.dropped == 0u);

    order_ok = 1;
    const uint8_t expect[4] = {3u, 4u, 5u, 6u};
    for (uint8_t i = 0; i < 4u; i++) {
        if (la_ring_pop(&r, &v) != 1 || v != expect[i]) {
            order_ok = 0;
        }
    }
    CHECK("回卷后顺序 3,4,5,6", order_ok);

    /* 容量 1 是退化的边界：第二次 push 必然丢弃 */
    la_ring_init(&r, buf, 1);
    CHECK("cap=1 首次 push 成功", la_ring_push(&r, 0xA5u) == 1);
    CHECK("cap=1 第二次 push 被丢弃", la_ring_push(&r, 0x5Au) == 0);
    CHECK("cap=1 dropped=1", r.dropped == 1u);
    CHECK("cap=1 保留的是先到的 0xA5",
          la_ring_pop(&r, &v) == 1 && v == 0xA5u);
}

static void test_sump(void)
{
    printf("\n[sump] 深度字段 '1'..'4'\n");
    CHECK("'1' -> 256",     la_sump_depth('1') == 256UL);
    CHECK("'2' -> 65536",   la_sump_depth('2') == 65536UL);
    CHECK("'3' -> 16777216", la_sump_depth('3') == 16777216UL);
    /* 2^32 在 uint32 里无法表示。返回上限而不是 0，是为了让"溢出"与"未知字段"可区分 */
    CHECK("'4' -> 0xFFFFFFFF（饱和，不伪装成可寻址）",
          la_sump_depth('4') == 0xFFFFFFFFUL);
    CHECK("'0' -> 0（非法字段）",  la_sump_depth('0') == 0UL);
    CHECK("'x' -> 0（非法字段）",  la_sump_depth('x') == 0UL);
}

static void test_fmt(void)
{
    char out[16];
    int n;

    printf("\n[fmt] 样本行的 ASCII 编码\n");

    n = la_fmt_sample(out, 0u, 0x3Cu);
    CHECK("idx=0 v=0x3C -> \"0,3C\"", strcmp(out, "0,3C") == 0);
    CHECK("返回值 = 长度 4", n == 4);

    n = la_fmt_sample(out, 1234u, 0xABu);
    CHECK("idx=1234 v=0xAB -> \"1234,AB\"", strcmp(out, "1234,AB") == 0);
    CHECK("返回值 = 长度 7", n == 7);

    n = la_fmt_sample(out, 4294967295u, 0x00u);
    CHECK("idx=UINT32_MAX -> \"4294967295,00\"（10 位十进制不丢位）",
          strcmp(out, "4294967295,00") == 0);
    CHECK("返回值 = 长度 13", n == 13);

    /* 十六进制必须补零：0x05 写成 "5" 会让按逗号切分的解析器把它当索引读 */
    n = la_fmt_sample(out, 7u, 0x05u);
    CHECK("v=0x05 -> \"7,05\"（十六进制补零）", strcmp(out, "7,05") == 0);
}

static void test_uart(void)
{
    uint8_t  wave[512];
    uint8_t  got[16];
    uint16_t pos;
    const uint16_t spp = 8u;
    const uint8_t  ch  = 0u;

    printf("\n[uart] 8N1 波形还原\n");

    /* 正常帧：前导空闲 1 bit，三字节，尾部留够停止位。
     * 0x00 与 0xFF 是两端极值，中间放 0x41（'A' = 0100 0001）打乱位序。 */
    pos = 0;
    put_bit(wave, &pos, ch, spp, 1);        /* 空闲高 */
    put_byte(wave, &pos, ch, spp, 0x41u, 1);
    put_byte(wave, &pos, ch, spp, 0x00u, 1);
    put_byte(wave, &pos, ch, spp, 0xFFu, 1);
    put_bit(wave, &pos, ch, spp, 1);        /* 尾部空闲 */

    memset(got, 0, sizeof(got));
    int n = la_uart_decode(wave, pos, ch, spp, got, (uint16_t)sizeof(got));
    CHECK("三字节解码数量 = 3", n == 3);
    CHECK("第 1 字节 = 0x41", n >= 1 && got[0] == 0x41u);
    CHECK("第 2 字节 = 0x00", n >= 2 && got[1] == 0x00u);
    CHECK("第 3 字节 = 0xFF", n >= 3 && got[2] == 0xFFu);

    /* 位序反了会得到 0x82（0x41 的位序镜像）。单独断言这一点，
     * 让"LSB first"这条约定在失败时能被指名，而不是笼统地说"解码不对"。 */
    CHECK("0x41 的位序镜像是 0x82，解码结果不是它",
          n >= 1 && got[0] != 0x82u);

    /* 帧错误：停止位人为写成 0，且数据全 0 —— 整帧连成一段低电平，
     * 之后回到高电平。解码器应丢弃整帧，不写 out、不计数。 */
    pos = 0;
    put_bit(wave, &pos, ch, spp, 1);
    put_byte(wave, &pos, ch, spp, 0x00u, 0);   /* 停止位 = 0 → 帧错误 */
    put_bit(wave, &pos, ch, spp, 1);           /* 回到空闲高 */
    put_byte(wave, &pos, ch, spp, 0x41u, 1);   /* 后面跟一帧正常的 */

    memset(got, 0, sizeof(got));
    n = la_uart_decode(wave, pos, ch, spp, got, (uint16_t)sizeof(got));
    CHECK("帧错误被整帧丢弃，只剩 1 个字节", n == 1);
    CHECK("丢弃后解出的是后面那个 0x41", n >= 1 && got[0] == 0x41u);

    /* 尾部不足一帧：必须丢弃而不是越界读。这是最容易写成"多读一个字节"的地方。 */
    pos = 0;
    put_bit(wave, &pos, ch, spp, 1);
    put_bit(wave, &pos, ch, spp, 0);           /* 只有一个起始位就没了 */
    memset(got, 0, sizeof(got));
    n = la_uart_decode(wave, pos, ch, spp, got, (uint16_t)sizeof(got));
    CHECK("尾部不足一帧 -> 0 字节", n == 0);

    /* 全高电平（线上没数据）：下降沿都找不到，结果必须是 0 而不是随机值 */
    pos = 0;
    for (uint16_t k = 0; k < 64u; k++) {
        put_bit(wave, &pos, ch, spp, 1);
    }
    memset(got, 0, sizeof(got));
    n = la_uart_decode(wave, pos, ch, spp, got, (uint16_t)sizeof(got));
    CHECK("全空闲高 -> 0 字节", n == 0);

    /* 参数校验：非法参数必须返回 -1，不能返回 0。
     * 返回 0 会让"参数写错"和"线上确实没数据"混在一起。 */
    CHECK("samples=NULL -> -1", la_uart_decode(0, 0, ch, spp, got, 4) == -1);
    CHECK("out=NULL -> -1",     la_uart_decode(wave, pos, ch, spp, 0, 4) == -1);
    CHECK("ch=8 越界 -> -1",    la_uart_decode(wave, pos, 8u, spp, got, 4) == -1);
    CHECK("spp=1 无法取中点 -> -1", la_uart_decode(wave, pos, ch, 1u, got, 4) == -1);

    /* 输出缓冲只有 1 格：解出 3 个字节时返回值仍是 3（真实产量），
     * 但只写了 1 个 —— 让调用者能发现"被截断了"，而不是静默少几个字节 */
    memset(got, 0, sizeof(got));
    pos = 0;
    put_bit(wave, &pos, ch, spp, 1);
    put_byte(wave, &pos, ch, spp, 0x11u, 1);
    put_byte(wave, &pos, ch, spp, 0x22u, 1);
    put_bit(wave, &pos, ch, spp, 1);
    n = la_uart_decode(wave, pos, ch, spp, got, 1);
    CHECK("outcap=1 时返回值仍报真实产量 2", n == 2);
    CHECK("outcap=1 时只写入首字节 0x11", got[0] == 0x11u && got[1] == 0x00u);
}

static void test_arith(void)
{
    printf("\n[arith] 容量与时间换算\n");
    /* F103C8T6：20 KB SRAM；本工程预留 4 KB 给栈与其它变量 */
    CHECK("20480-4096 -> 16384 样本", la_max_samples(20480UL, 4096UL) == 16384UL);
    CHECK("预留 >= SRAM 时不返回负数（饱和到 0）",
          la_max_samples(4096UL, 4096UL) == 0UL);
    CHECK("预留 > SRAM 时饱和到 0，不回绕",
          la_max_samples(1024UL, 4096UL) == 0UL);

    /* 先乘后除：16384 * 1e6 = 1.6e10，超出 uint32 一倍多。
     * 若实现写成 (nsamples * 1000000) / fs，这里会得到一个荒谬的小数。 */
    CHECK("16384 样本 @1 MSa/s = 16384 µs",
          la_capture_us(16384UL, 1000000UL) == 16384UL);
    /* 注意单位：函数返回的是**微秒**。11520 Sa/s 下 16384 个样本 = 1 422 222 µs，
     * 也就是 1422 ms。把 ms 直接写进断言是本程序第一版的真实错误。 */
    CHECK("16384 样本 @11.52 kSa/s = 1 422 222 µs（= 1422 ms）",
          la_capture_us(16384UL, 11520UL) == 1422222UL);
    CHECK("fs=0 -> 0（不除零）", la_capture_us(1000UL, 0UL) == 0UL);
    /* 饱和而不是回绕：16384 @1 Sa/s = 1.6e10 µs > uint32 */
    CHECK("结果超出 uint32 时饱和到 0xFFFFFFFF",
          la_capture_us(16384UL, 1UL) == 0xFFFFFFFFUL);
}

int main(void)
{
    printf("== la_core 主机端判定（同一份源码编进固件）==\n");

    test_pack();
    test_ring();
    test_sump();
    test_fmt();
    test_uart();
    test_arith();

    printf("\n---- 合计 %d 项：通过 %d，失败 %d ----\n", g_pass + g_fail, g_pass, g_fail);
    printf("%s\n", g_fail == 0 ? "PASS" : "FAIL");
    return g_fail == 0 ? 0 : 1;
}
