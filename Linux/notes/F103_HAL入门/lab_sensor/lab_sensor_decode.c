/*
 * lab_sensor_decode.c —— Day17 传感器采集：主机端可复算判据
 *
 * 目的：不接板、不烧写，把四个驱动里**纯算法部分**的缺陷变成可复算的读数。
 *       凡是依赖时序、电平、时钟的结论一律不在这里断言（那是板上/示波器的事）。
 *
 * 覆盖：
 *   D1  driver_ds18b20.c 精度判断的三个条件恒为假（运算符优先级）
 *   D2  driver_ds18b20.c 负温解码把补码当"符号-数值"，误差随温度变化
 *   D3  driver_dht11.c   校验和未取低 8 位，和 >255 时判失败
 *   D4  driver_dht11.c   DHT11_ReadByte 返回 -1 存入 unsigned char 后的值
 *   D5  driver_ds18b20.c DS18B20_WaitReady 的 timeout_us 实为 63 us/次
 *   D6  driver_mpu6050.c 逐字节读 12 次 vs burst 读 1 次的时间预算
 *
 * 构建与运行见同目录 build.sh。--inject N 用于负向验证（必须判 FAIL）。
 *
 * 编译：gcc -std=c11 -Wall -Wextra -O2 lab_sensor_decode.c -o lab_sensor_decode -lm
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

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
 * D1：driver_ds18b20.c:463-467 的精度判断
 *
 * 驱动原文（行尾注释见 driver_ds18b20.c:463-467）：
 *   if      (ram[4] & (3<<5) == 0)          -- 注释写 精度: 9bit
 *   else if (ram[4] & (3<<5) == (1<<5))     -- 注释写 精度: 10bit
 *   else if (ram[4] & (3<<5) == (2<<5))     -- 注释写 精度: 11bit
 *   else i = 0;                             -- 注释写 精度是 12 bit
 *
 * C 的优先级：== (8) 高于 & (9)，所以实际语义是
 *   ram[4] & ((3<<5) == 0)   ==   ram[4] & 0   ==   0
 * 三个条件全部恒假，永远走 else ⇒ 恒按 12 bit 解释。
 * ====================================================================== */

/* 原样复刻驱动写法，故意保留以证明警告不是写错 */
static int drv_precision_index(unsigned char ram4)
{
    int i;
    if ((ram4 & (3 << 5)) == 0)          /* 正确写法，用于对照 */
        i = 3;
    else if ((ram4 & (3 << 5)) == (1 << 5))
        i = 2;
    else if ((ram4 & (3 << 5)) == (2 << 5))
        i = 1;
    else
        i = 0;
    return i;
}

/* 以下函数**故意**复刻驱动原文写法，GCC 会报 3 条 -Wparentheses。
 * 这正是 driver_ds18b20.c:463/465/467 那 3 条警告的来源（见本篇验证记录），
 * 不是笔误。局部屏蔽以保持本次构建 0 警告，警告本身由 D1 的输出作为证据。 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wparentheses"
static int drv_precision_index_as_written(unsigned char ram4)
{
    int i;
    if (ram4 & (3 << 5) == 0)
        i = 3;
    else if (ram4 & (3 << 5) == (1 << 5))
        i = 2;
    else if (ram4 & (3 << 5) == (2 << 5))
        i = 1;
    else
        i = 0;
    return i;
}
#pragma GCC diagnostic pop

static void d1_precision_precedence(void)
{
    /* ram[4] bit6:5 编码：00=9bit 01=10bit 10=11bit 11=12bit */
    static const struct { unsigned char cfg_bits; int expect; const char *name; } cases[] = {
        { 0x00, 3, "9bit " },
        { 0x20, 2, "10bit" },
        { 0x40, 1, "11bit" },
        { 0x60, 0, "12bit" },
    };
    unsigned k;

    printf("[D1] 精度判断的运算符优先级（driver_ds18b20.c:463-467）\n");
    for (k = 0; k < sizeof(cases) / sizeof(cases[0]); k++) {
        unsigned char ram4 = cases[k].cfg_bits;
        int good = drv_precision_index(ram4);
        int written = drv_precision_index_as_written(ram4);

        CHECK(good == cases[k].expect,
              "正确写法 ram[4]=0x%02X(%s) ⇒ 起始位索引 %d", ram4, cases[k].name, good);
        CHECK(written == 0,
              "驱动写法 ram[4]=0x%02X(%s) ⇒ 恒得 0（应为 %d）",
              ram4, cases[k].name, cases[k].expect);
    }
    /* 三个条件本身恒假，与 ram[4] 无关 */
    CHECK(drv_precision_index_as_written(0x00) == drv_precision_index_as_written(0xFF),
          "驱动写法的结果与 ram[4] 无关 ⇒ 三个分支均为死代码");
}

/* ========================================================================
 * D2：driver_ds18b20.c:444-492 的温度解码
 *
 * 驱动做法：把 ram[1] bit3 当符号位，bit0-2 与 ram[0] 的 8 位拼成"绝对值"，
 *           最后 sum = 0 - sum。
 * 正确做法：12 位寄存器是**二进制补码**，见到符号位为 1 时应
 *           raw = ((ram[1] & 0x0F) << 8) | ram[0];  if (raw & 0x800) raw -= 4096;
 *           再乘 0.0625。
 *
 * 正温时两者一致（补码与符号-数值同形），负温时驱动错误。
 * ====================================================================== */

static double drv_decode_as_written(const unsigned char ram[9])
{
    static const double val[] = {0.0625, 0.125, 0.25, 0.5, 1, 2, 4, 8, 16, 32, 64};
    double sum = 0;
    int i;

    /* 沿用 D1 的驱动写法，恒取 i = 0 */
    i = drv_precision_index_as_written(ram[4]);

    for (; i < 8; i++)
        if (ram[0] & (1 << i))
            sum += val[i];
    for (i = 0; i < 3; i++)
        if (ram[1] & (1 << i))
            sum += val[8 + i];
    if (ram[1] & (1 << 3))
        sum = 0 - sum;
    return sum;
}

static double correct_decode(const unsigned char ram[9])
{
    int raw = ((ram[1] & 0x0F) << 8) | ram[0];
    if (raw & 0x800)
        raw -= 4096;
    return raw * 0.0625;
}

static void d2_negative_temperature(void)
{
    /* 12 bit 原始值 → 真值（0.0625 °C/LSB，补码） */
    static const int16_t raws[] = {
        0x0191,   /*  25.0625 */
        0x0008,   /*   0.5    */
        0x0000,   /*   0.0    */
        0xFFF8,   /*  -0.5    */
        0xFF5E,   /* -10.125  */
        0xFE6F,   /* -25.0625 */
        0xFE90,   /* -23.0    */
    };
    unsigned k;

    printf("[D2] 负温解码：驱动把补码当符号-数值（driver_ds18b20.c:444-492）\n");
    printf("  %-12s %-10s %-10s %-10s %-10s\n", "raw", "ram[0]", "真值(°C)", "驱动(°C)", "误差");
    for (k = 0; k < sizeof(raws) / sizeof(raws[0]); k++) {
        unsigned char ram[9];
        double want, got;

        memset(ram, 0, sizeof(ram));
        ram[4] = 0x1F;                       /* 出厂默认配置寄存器（12 bit） */
        ram[0] = (unsigned char)(raws[k] & 0xFF);
        ram[1] = (unsigned char)((raws[k] >> 8) & 0xFF);
        want = correct_decode(ram);
        got = drv_decode_as_written(ram);

        printf("  0x%04X      0x%02X       %-10.4f %-10.4f %+.4f\n",
               (unsigned)(uint16_t)raws[k], ram[0], want, got, got - want);

        if (raws[k] >= 0)
            CHECK(fabs(got - want) < 1e-9, "正温 raw=0x%04X 驱动与正确算法一致",
                  (unsigned)(uint16_t)raws[k]);
        else
            CHECK(fabs(got - want) > 1.0, "负温 raw=0x%04X 驱动偏离 %.3f °C",
                  (unsigned)(uint16_t)raws[k], got - want);
    }
    /* 误差不是常数，随数值变化：不能用"减一个固定值"修正 */
    {
        unsigned char a[9], b[9];
        double da, db;
        memset(a, 0, 9); memset(b, 0, 9);
        a[4] = b[4] = 0x1F;
        a[0] = 0xF8; a[1] = 0xFF;             /* -0.5 */
        b[0] = 0x6F; b[1] = 0x0E;             /* -25.0625 */
        da = drv_decode_as_written(a) - correct_decode(a);
        db = drv_decode_as_written(b) - correct_decode(b);
        CHECK(fabs(da - db) > 1.0,
              "两处负温的误差不同（%.3f vs %.3f）⇒ 误差不是常量偏移", da, db);
    }
}

/* ========================================================================
 * D3：driver_dht11.c:285 的校验和
 *
 *   if (hum_m + hum_n + temp_m + temp_n == check)
 *
 * 左边 int（可能 > 255），右边 unsigned char 提升为 int。
 * 协议规定校验和是四个字节之和的**低 8 位**，这里没有取模。
 * ====================================================================== */

static int dht11_checksum_as_written(unsigned char h_i, unsigned char h_d,
                                     unsigned char t_i, unsigned char t_d,
                                     unsigned char check)
{
    return (h_i + h_d + t_i + t_d) == check;
}

static int dht11_checksum_correct(unsigned char h_i, unsigned char h_d,
                                  unsigned char t_i, unsigned char t_d,
                                  unsigned char check)
{
    return ((unsigned char)(h_i + h_d + t_i + t_d)) == check;
}

static void d3_dht11_checksum(void)
{
    printf("[D3] DHT11 校验和未取低 8 位（driver_dht11.c:285）\n");
    /* 正常量程：湿度 20-90，温度 0-50，和 ≤ 140，不溢出 */
    CHECK(dht11_checksum_as_written(65, 0, 25, 0, 90) == 1, "正常量程 65+25=90 ⇒ 通过");
    /* 干扰帧：和 260，低 8 位 = 4 */
    CHECK(dht11_checksum_correct(200, 0, 60, 0, 4) == 1,
          "正确算法：200+60=260，低 8 位 4 ⇒ 通过");
    CHECK(dht11_checksum_as_written(200, 0, 60, 0, 4) == 0,
          "驱动算法：260 != 4 ⇒ 判校验失败（把数据异常报成校验失败）");
    /* 最大正常和 */
    CHECK(dht11_checksum_as_written(90, 0, 50, 0, 140) == 1,
          "量程上限 90+50=140 < 256 ⇒ 正常场景不触发该缺陷");
}

/* ========================================================================
 * D4：driver_dht11.c:252-254 / :278 的错误码窄化
 *
 *   unsigned char hum_m, hum_n, temp_m, temp_n, check;
 *   hum_m = DHT11_ReadByte();      -- ReadByte 失败时返回 -1
 *
 * -1 被存进 unsigned char ⇒ 0xFF。随后这 0xFF 参与校验和。
 * ====================================================================== */

static int dht11_read_byte_stub(int fail) { return fail ? -1 : 0x5A; }

static void d4_error_code_narrowing(void)
{
    unsigned char hum_m;

    printf("[D4] DHT11_ReadByte 的 -1 被 unsigned char 截断（driver_dht11.c:252-278）\n");
    hum_m = (unsigned char)dht11_read_byte_stub(1);
    CHECK(hum_m == 0xFF, "失败返回 -1 ⇒ 存入 unsigned char 后为 0x%02X", hum_m);

    /* 该缺陷要真正造成误判，需要"截断后的 0xFF 恰好让校验通过"。
     * check 字节失败时驱动判 S == 0xFF，即要求前四字节和恰为 255。 */
    CHECK(dht11_checksum_as_written(200, 0, 55, 0, 0xFF) == 1,
          "构造 S=255 的越界数据(湿 200/温 55) ⇒ 驱动判校验通过，返回超量程值");

    /* 但在 DHT11 合法量程内（湿度 20-90、温度 0-50、小数字节恒 0），和最大 140，
     * 永远凑不出 255 ⇒ 正常数据下本缺陷不暴露。 */
    CHECK(90 + 0 + 50 + 0 < 255,
          "合法量程上限 140 < 255 ⇒ 正常数据无法触发，该缺陷被量程掩盖");

    /* 真正的代价不在误判率，而在"错误码从数据路径上消失了"：
     * 数据字节失败时该字节变成 0xFF（合法值域内），后续没有任何显式检测点。 */
    CHECK(hum_m == 0xFF && 0xFF <= 90 + 255,
          "0xFF 落在 8 位数据值域内 ⇒ 无法用取值区分'读到 255'与'读取失败'");
}

/* ========================================================================
 * D5：DS18B20_WaitReady 的时间量纲
 *
 *   static int DS18B20_WaitReady(int timeout_us)
 *   { while (timeout_us--) { if (DS18B20_ReadBit() == 1) return 0; udelay(1); } ... }
 *
 * 每次迭代 = DS18B20_ReadBit() + udelay(1)。
 * ReadBit 内部：PinSetForTime(0, 2) + PinRelease() + udelay(10) + 读 + udelay(50)
 *            ≈ 2 + 10 + 50 = 62 us（不含函数调用与 GPIO 操作开销）。
 * 所以 timeout_us 的单位不是微秒。
 * ====================================================================== */

#define READBIT_US      62      /* 2 + 10 + 50，源码可加和 */
#define LOOP_EXTRA_US   1       /* udelay(1) */
#define CONVERT_T_US    1000000 /* DS18B20_StartConvert 传入的实参 */

static void d5_waitready_scale(void)
{
    double per_iter = READBIT_US + LOOP_EXTRA_US;
    double worst = per_iter * CONVERT_T_US / 1e6;   /* 秒 */
    double tconv_max = 0.750;                        /* 12 bit 最长转换时间 */

    printf("[D5] DS18B20_WaitReady 的 timeout_us 实际量纲（driver_ds18b20.c:315-324）\n");
    printf("  每次迭代 ≈ %.0f us；传入 %d ⇒ 最坏忙等 %.1f s\n",
           per_iter, CONVERT_T_US, worst);
    CHECK(worst > 60.0, "名义 1 s 的超时，实际最坏约 %.1f s（差 %.0f 倍）",
          worst, worst);
    CHECK(tconv_max / (per_iter / 1e6) < 20000,
          "正常转换 750 ms 约需 %.0f 次迭代（远小于 1e6）⇒ 传感器在线时不会走满",
          tconv_max / (per_iter / 1e6));
}

/* ========================================================================
 * D6：MPU6050 逐字节读 vs burst 读
 *
 * driver_mpu6050.c:159-201 对 6 个 16 位量分 12 次单个寄存器读，
 * 每次 HAL_I2C_Mem_Read(..., 1, ..., 1, ...)。
 *
 * 一次单字节 Mem_Read 的帧（标准模式，含 ACK 位）：
 *   S(1) + 从地址(9) + 寄存器(9) + Sr(1) + 从地址(9) + 数据(9) + P(1) = 39 位
 * burst 读 14 字节：
 *   S(1) + 从地址(9) + 寄存器(9) + Sr(1) + 从地址(9) + 14*9 + P(1) = 156 位
 * ====================================================================== */

#define BIT_US_100K     10      /* 100 kHz：一位 10 us */

static void d6_mpu6050_i2c_budget(void)
{
    int per_read_bits = 1 + 9 + 9 + 1 + 9 + 9 + 1;
    int burst_bits    = 1 + 9 + 9 + 1 + 9 + 14 * 9 + 1;
    double bytewise_us = 12.0 * per_read_bits * BIT_US_100K;
    double burst_us    = burst_bits * BIT_US_100K;
    double period_us   = 10000.0;   /* SMPLRT_DIV=9 ⇒ 1 kHz/(1+9) = 100 Hz */

    printf("[D6] MPU6050 读取方式的 I2C 时间预算（driver_mpu6050.c:159-201）\n");
    printf("  逐字节：12 次 x %d 位 = %.0f us\n", per_read_bits, bytewise_us);
    printf("  burst ： 1 次 x %d 位 = %.0f us\n", burst_bits, burst_us);
    printf("  100 Hz 采样周期 = %.0f us\n", period_us);
    printf("  逐字节占周期 %.1f%%，burst 占 %.1f%%\n",
           100.0 * bytewise_us / period_us, 100.0 * burst_us / period_us);

    CHECK(bytewise_us > 4000.0, "逐字节读约 %.0f us", bytewise_us);
    CHECK(bytewise_us / burst_us > 2.5, "逐字节比 burst 慢 %.2f 倍", bytewise_us / burst_us);
    CHECK(bytewise_us / period_us > 0.4, "逐字节读占用 %.0f%% 的 100 Hz 采样周期",
          100.0 * bytewise_us / period_us);
}

/* ====================================================================== */

int main(int argc, char **argv)
{
    int inject = 0;
    int i;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--inject") == 0 && i + 1 < argc) {
            inject = atoi(argv[++i]);
        }
    }

    printf("lab_sensor_decode — Day17 主机端可复算判据\n");
    printf("构建：gcc -std=c11 -Wall -Wextra -O2 lab_sensor_decode.c -o lab_sensor_decode -lm\n\n");

    d1_precision_precedence();
    printf("\n");
    d2_negative_temperature();
    printf("\n");
    d3_dht11_checksum();
    printf("\n");
    d4_error_code_narrowing();
    printf("\n");
    d5_waitready_scale();
    printf("\n");
    d6_mpu6050_i2c_budget();
    printf("\n");

    /* 负向验证：注入一条被判据捕获的错误，用来证明判据不是恒真 */
    if (inject > 0) {
        printf("[注入] --inject %d\n", inject);
        switch (inject) {
        case 1:
            CHECK(drv_precision_index_as_written(0x00) == 3, "注入：声称驱动写法能得 3（应失败）");
            break;
        case 2:
            CHECK(0, "注入：无条件失败（应失败）");
            break;
        case 3:
            CHECK(drv_decode_as_written((const unsigned char[]){0xF8, 0xFF, 0, 0, 0x1F, 0, 0, 0, 0})
                      > -1.0,
                  "注入：声称 -0.5 °C 的驱动输出大于 -1（应失败）");
            break;
        default:
            printf("  未知注入编号，忽略\n");
            break;
        }
        printf("\n");
    }

    printf("汇总：%d 条判据，%d 条失败\n", g_case, g_fail);
    if (g_fail == 0) {
        printf("RESULT: PASS\n");
        return 0;
    }
    printf("RESULT: FAIL\n");
    return 1;
}
