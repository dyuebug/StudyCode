/**
 * 上行帧格式与增量解析（PC → MCU）
 *
 * 布局（多字节字段全部小端）：
 *
 *   [0] 0xAA  [1] 0x55  [2] TYPE  [3] SEQ  [4] LEN  [5 .. 4+LEN] PAYLOAD  [5+LEN .. 6+LEN] CRC16
 *
 * CRC16-CCITT-FALSE：多项式 0x1021，初值 0xFFFF，不反射输入输出，不异或输出。
 * 计算范围是 TYPE..PAYLOAD，**不含**两个 SOF 字节。
 *
 * 为什么 SOF 不参与校验：SOF 只负责"定位帧边界"，CRC 负责"内容是否完整"。
 * 若把恒定的 0xAA55 纳入校验，等于把"找到一个 0xAA55"和"帧内容正确"两件事
 * 绑在同一个结论上，坏帧排查时无法区分是同步错了还是数据错了。
 *
 * 解析器是**按字节推进的状态机**，不是"攒够一帧再解析"：串口是字节流，
 * 没有天然的帧边界，必须逐字节判断；同时这样不需要额外的行缓冲。
 */
#ifndef AV_FRAME_H
#define AV_FRAME_H

#include <stdint.h>

#define AV_SOF0               0xAAU
#define AV_SOF1               0x55U
#define AV_TYPE_SENSOR        0x01U
/* 上行地面站心跳。注意与下行 0x81 同名不同向：两者是**两条独立链路**，
 * 各自维护序号与超时，不互相刷新对方的活性。 */
#define AV_TYPE_STATUS        0x02U

/* 载荷上限：当前类型分别用 8 / 4 字节，留余量给后续类型，同时把
 * 解析缓冲压到可控大小（39 字节，栈上放得下）。 */
#define AV_PAYLOAD_MAX        32U
#define AV_FRAME_MAX          ( 5U + AV_PAYLOAD_MAX + 2U )

/* SENSOR 载荷：airspeed(2) + pitch(2) + roll(2) + flags(2) */
#define AV_SENSOR_PAYLOAD_LEN 8U

/* STATUS 载荷：ground_uptime_ms(4)。用地面站的本地时间戳而不是它的序号：
 * 序号回绕（255→0）与"地面站重启后从 0 开始"在线上长得一样，时间戳能区分
 * "继续数"和"重新数"，也让重启这件事本身可观测。 */
#define AV_STATUS_PAYLOAD_LEN 4U

typedef struct {
    uint8_t type;
    uint8_t seq;
    uint8_t len;
    uint8_t payload[ AV_PAYLOAD_MAX ];
} av_frame_t;

typedef enum {
    AV_FRAME_NONE = 0,   /* 还没凑齐一帧，正常返回值 */
    AV_FRAME_OK,
    AV_FRAME_BAD_CRC,
    AV_FRAME_BAD_LEN     /* LEN 超过 AV_PAYLOAD_MAX，该帧被放弃 */
} av_frame_result_t;

typedef enum {
    AV_RX_SOF0 = 0,
    AV_RX_SOF1,
    AV_RX_TYPE,
    AV_RX_SEQ,
    AV_RX_LEN,
    AV_RX_PAYLOAD,
    AV_RX_CRC_LO,
    AV_RX_CRC_HI
} av_parser_state_t;

typedef struct {
    av_parser_state_t state;
    av_frame_t        frame;      /* 正在接收的帧 */
    uint32_t          idx;        /* 已收载荷字节数 */
    uint16_t          crc_calc;
    uint16_t          crc_recv;
} av_parser_t;

uint16_t av_crc16_ccitt_false( const uint8_t *data, uint32_t len );

void av_parser_init( av_parser_t *p );

/**
 * 推入一个字节。返回 AV_FRAME_NONE 表示还没成帧；返回其它值时 *out 已填充。
 * 出错后解析器自动复位到"找 SOF0"，不会卡在坏帧中间。
 */
av_frame_result_t av_parser_push( av_parser_t *p, uint8_t byte, av_frame_t *out );

#endif /* AV_FRAME_H */
