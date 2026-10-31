/**
 * la_core.h —— 逻辑分析仪的可移植核心
 *
 * 为什么不放在 main.c 里：教材第 4 章只给方案、不给代码，所以"这个项目到底对不对"
 * 必须能在一台**没有板**的机器上判定。把与硬件无关的三件事抽出来，
 * 就能用宿主 gcc 单独编译、单独测：
 *
 *   ① 采样打包：把任意 8 根引脚压成 1 字节（内存开销 ÷ 8）
 *   ② 环形缓冲：满了之后是"覆盖旧的"还是"丢弃新的"，必须显式选一个并计数
 *   ③ 帧格式与解码：SUMP 的深度字段、样本行的 ASCII 编码、UART 波形还原
 *
 * 本文件不包含任何 HAL / CMSIS 头，固件与主机程序共用同一份实现。
 * 这样"主机端测过的逻辑"就是"板上跑的同一份逻辑"，而不是两套近似代码。
 */
#ifndef LA_CORE_H
#define LA_CORE_H

#include <stdint.h>

/* 本工程固定 8 通道：一个字节装一次采样，正好对齐 UART 的 1 字节粒度 */
#define LA_CHANNELS 8u

/**
 * 把 I/O 输入寄存器的若干位"压实"成 LA_CHANNELS 位。
 *
 * idr   : 端口输入寄存器（如 GPIOA->IDR）
 * mask  : 关心的位（如 0x00FF 取 PA0..PA7）
 *
 * 位序规则：按**引脚号升序**依次填入结果的 bit0..bit7。
 * 也就是说 mask 里第 1 个为 1 的位 → 结果 bit0。
 * 这条规则是解码器的前提：主机端按同一个顺序解释通道号。
 * mask 中置 1 的位超过 8 个时，只取前 8 个（低引脚优先）。
 */
uint8_t la_pack(uint32_t idr, uint32_t mask);

/* ---- 环形缓冲 ---------------------------------------------------------- */

/*
 * 满时策略：**丢弃新样本并计数**，不覆盖旧样本。
 *
 * 选它的理由：逻辑分析仪的价值在"故障发生的那一段"。
 * 覆盖旧的会让首段消失，而丢新的只是少了尾部，且 dropped 会明说少了多少。
 * 代价：故障在缓冲区填满之后才出现时，什么都记不到 —— 所以必须有 dropped 计数，
 * 让"没采到"和"采到全 0"能被区分开（这条教训来自 F103_17 的假阳性判据）。
 */
typedef struct {
    uint8_t *buf;
    uint16_t cap;      /* 容量（样本数） */
    uint16_t head;     /* 下一个写入位置 */
    uint16_t count;    /* 当前已存样本数 */
    uint32_t total;    /* 累计尝试写入次数 */
    uint32_t dropped;  /* 因满而丢弃的次数 */
} la_ring;

void     la_ring_init(la_ring *r, uint8_t *buf, uint16_t cap);
uint16_t la_ring_len(const la_ring *r);
int      la_ring_push(la_ring *r, uint8_t v);   /* 1 = 写入成功，0 = 满（已计 dropped） */
int      la_ring_pop(la_ring *r, uint8_t *out); /* 1 = 取出成功，0 = 空 */

/* ---- 帧格式 ------------------------------------------------------------ */

/*
 * SUMP 协议里 "0x80 + '1'..'4'" 表示采样深度 2^(8N)。
 * 返回实际样本数；'4' 对应 2^32，无法在 uint32_t 里表示，
 * 本实现返回 UINT32_MAX 并视为"超出可寻址范围"，不做截断伪装。
 */
uint32_t la_sump_depth(char tag);

/*
 * 一条采样记录的 ASCII 编码：<idx>,<hex>,<hex>,... 形式（本工程简化流格式）。
 * 返回写入长度（不含结尾 NUL）；out 至少需要 16 字节。
 *
 * 明确它不是 SUMP 二进制协议：SUMP 用的是位打包的 0x01..0x04 采样记录，
 * 这里为了能在串口助手里直接看，故意用 ASCII。两者不能互相解析。
 */
int la_fmt_sample(char *out, uint32_t idx, uint8_t v);

/* ---- UART 波形解码 ----------------------------------------------------- */

/**
 * 从采样流里还原一个 UART 字节序列（8N1，LSB first，空闲高）。
 *
 * samples / n : 采样缓冲（每个元素是一次采样，通道位由 ch 指定）
 * ch          : 通道号 0..7
 * spp         : 每 bit 的采样点数（samples per bit），必须 >= 2
 * out/outcap  : 解码结果
 *
 * 采样点选在每 bit 的**中点**：以起始位第一个采样点 s 为基准，
 * 数据位 b 取 `s + spp + spp/2 + b*spp`，停止位取 `s + 9*spp + spp/2`。
 * 不是"取边沿后第一个点"——后者在 spp 很小、有抖动时必然出错；
 * 也别忘了起始位本身占一个 spp，漏掉它整帧会偏 1 bit（本工程首版就是这样错的）。
 *
 * 返回：成功解码的字节数；参数非法返回 -1。
 * 校验失败（停止位不是高）的帧被整帧丢弃，不写入 out，也不改变返回值计数。
 */
int la_uart_decode(const uint8_t *samples, uint16_t n, uint8_t ch,
                   uint16_t spp, uint8_t *out, uint16_t outcap);

/* ---- 容量算术 ---------------------------------------------------------- */

/* SRAM 里能放多少个 1 字节样本：sram_bytes - overhead，字节为单位 */
uint32_t la_max_samples(uint32_t sram_bytes, uint32_t overhead);

/* 以 fs_hz 采 nsamples 个点，覆盖多长时间（微秒） */
uint32_t la_capture_us(uint32_t nsamples, uint32_t fs_hz);

#endif /* LA_CORE_H */
