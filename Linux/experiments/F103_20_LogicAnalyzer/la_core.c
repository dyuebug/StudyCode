/**
 * la_core.c —— 逻辑分析仪可移植核心的实现
 *
 * 约束：只用 C99 标准库（<stdint.h> 都不用别的），
 * 保证同一份源码既能被 arm-none-eabi-gcc 编进固件，也能被宿主 gcc 编进测试程序。
 */
#include "la_core.h"

uint8_t la_pack(uint32_t idr, uint32_t mask)
{
    uint8_t out = 0;
    uint8_t bit = 0;

    for (uint32_t i = 0; i < 32u; i++) {
        if ((mask & (1UL << i)) == 0u) {
            continue;
        }
        if (bit >= LA_CHANNELS) {
            break;                      /* 超过 8 位：只取低引脚的那 8 个 */
        }
        if ((idr & (1UL << i)) != 0u) {
            out = (uint8_t)(out | (uint8_t)(1u << bit));
        }
        bit++;
    }
    return out;
}

/* ---- 环形缓冲 ---------------------------------------------------------- */

void la_ring_init(la_ring *r, uint8_t *buf, uint16_t cap)
{
    r->buf = buf;
    r->cap = cap;
    r->head = 0;
    r->count = 0;
    r->total = 0;
    r->dropped = 0;
}

uint16_t la_ring_len(const la_ring *r)
{
    return r->count;
}

int la_ring_push(la_ring *r, uint8_t v)
{
    r->total++;
    if (r->count >= r->cap) {
        r->dropped++;                   /* 满：丢新的，不覆盖旧的 */
        return 0;
    }
    r->buf[r->head] = v;
    r->head = (uint16_t)((r->head + 1u) % r->cap);
    r->count++;
    return 1;
}

int la_ring_pop(la_ring *r, uint8_t *out)
{
    uint16_t tail;

    if (r->count == 0u) {
        return 0;
    }
    /* 尾位置不能单独存一个变量：它由 head 与 count 唯一确定，
     * 多存一个变量就多一处可以不同步的状态。 */
    tail = (uint16_t)((r->head + r->cap - r->count) % r->cap);
    *out = r->buf[tail];
    r->count--;
    return 1;
}

/* ---- 帧格式 ------------------------------------------------------------ */

uint32_t la_sump_depth(char tag)
{
    switch (tag) {
    case '1': return 1UL << 8;          /* 256 样本 */
    case '2': return 1UL << 16;         /* 65 536 样本 */
    case '3': return 1UL << 24;         /* 16 777 216 样本 */
    case '4': return 0xFFFFFFFFUL;      /* 名义 2^32：超出 uint32，按上限返回 */
    default:  return 0;
    }
}

int la_fmt_sample(char *out, uint32_t idx, uint8_t v)
{
    static const char hex[] = "0123456789ABCDEF";
    int n = 0;

    /* 手写而不是 snprintf：固件侧 newlib-nano 的 snprintf 会带进格式化引擎，
     * 而且这里的格式只有十进制定宽和两位十六进制，没必要付那个代价。 */
    char tmp[10];
    int t = 0;

    if (idx == 0u) {
        tmp[t++] = '0';
    } else {
        uint32_t x = idx;
        while (x > 0u && t < (int)sizeof(tmp)) {
            tmp[t++] = (char)('0' + (x % 10u));
            x /= 10u;
        }
    }
    while (t > 0) {
        out[n++] = tmp[--t];
    }
    out[n++] = ',';
    out[n++] = hex[(v >> 4) & 0x0Fu];
    out[n++] = hex[v & 0x0Fu];
    out[n] = '\0';
    return n;
}

/* ---- UART 波形解码 ----------------------------------------------------- */

int la_uart_decode(const uint8_t *samples, uint16_t n, uint8_t ch,
                   uint16_t spp, uint8_t *out, uint16_t outcap)
{
    uint16_t i = 0;
    int produced = 0;

    if (samples == 0 || out == 0 || ch >= LA_CHANNELS || spp < 2u) {
        return -1;
    }

    while (i < n) {
        /* 找下降沿：前一点是高、当前点是低 */
        uint8_t prev = (uint8_t)((samples[i] >> ch) & 1u);
        uint16_t s;

        if (prev == 0u) {
            i++;
            continue;                   /* 还在低电平里（上一帧的停止位之后），跳过 */
        }
        s = (uint16_t)(i + 1u);
        if (s >= n) {
            break;
        }
        if (((samples[s] >> ch) & 1u) != 0u) {
            i = s;                      /* 不是下降沿 */
            continue;
        }

        /* s 是起始位的第一个采样点；整帧应占 (1 起始 + 8 数据 + 1 停止) = 10 bit */
        if ((uint32_t)s + (uint32_t)spp * 10u > n) {
            break;                      /* 尾部不足一帧，丢弃 */
        }

        /* 采样点必须落在每个 bit 的**中点**，而不是边沿之后第一个点。
         * 索引关系（以 s 为起始位第一个采样点，spp 为每 bit 采样数）：
         *   起始位   [s,               s+spp)
         *   数据位 b [s+(1+b)*spp,     s+(2+b)*spp)   b = 0..7
         *   停止位   [s+9*spp,         s+10*spp)
         * 所以数据位 b 的中点是 s + spp + spp/2 + b*spp。
         * 首版漏掉那个 +spp，整帧偏移 1 bit：第 8 个数据位被当成停止位校验，
         * 于是任何含 0 数据位的帧都被判成帧错误。这个错误由 host/lab_la_proto.c
         * 的 "三字节解码数量 = 3" 一行抓出——没有板时能抓到的正是这类错。 */
        uint8_t value = 0;
        for (uint8_t b = 0; b < 8u; b++) {
            uint16_t at = (uint16_t)(s + spp + spp / 2u + (uint16_t)b * spp);
            if (((samples[at] >> ch) & 1u) != 0u) {
                value = (uint8_t)(value | (uint8_t)(1u << b));   /* LSB first */
            }
        }
        uint16_t stop_at = (uint16_t)(s + 9u * spp + spp / 2u);
        if (((samples[stop_at] >> ch) & 1u) == 0u) {
            i = (uint16_t)(s + spp);    /* 停止位不是高：帧错误，整帧丢弃 */
            continue;
        }
        if (produced < (int)outcap) {
            out[produced] = value;
        }
        produced++;
        /* 下一轮的起点放在**停止位**（高电平）上，而不是放在下一帧的起始位。
         * 边沿检测的前提是 samples[i] 为高、samples[i+1] 为低；
         * 若直接跳到 s+spp*10，那里正是下一帧起始位的低电平，prev 一开始就是 0，
         * 会被"还在低电平里"的分支一路跳过，结果每段波形只解出第一帧。 */
        i = (uint16_t)(s + 9u * spp);
    }
    return produced;
}

/* ---- 容量算术 ---------------------------------------------------------- */

uint32_t la_max_samples(uint32_t sram_bytes, uint32_t overhead)
{
    if (sram_bytes <= overhead) {
        return 0;
    }
    return sram_bytes - overhead;
}

uint32_t la_capture_us(uint32_t nsamples, uint32_t fs_hz)
{
    uint64_t us;

    if (fs_hz == 0u) {
        return 0;
    }
    /* 先乘后除：nsamples 最大 2^24 时，乘 1e6 仍在 uint64 内。
     * 用 32 位乘会溢出，这是"看起来对的算式其实在边界上错"的典型位置。 */
    us = ((uint64_t)nsamples * 1000000ULL) / (uint64_t)fs_hz;
    /* 结果超出 uint32（约 4295 秒）时饱和，而不是回绕成一个看起来正常的数。
     * 静默回绕会让"缓冲能存多久"这个结论在一端变成负数级别的胡说。 */
    return (us > 0xFFFFFFFFULL) ? 0xFFFFFFFFUL : (uint32_t)us;
}
