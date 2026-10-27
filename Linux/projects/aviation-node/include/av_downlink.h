/**
 * 下行二进制状态帧（MCU → PC，USART2）
 *
 * 为什么单开一条串口：USART1 上跑的是人读的 ASCII 日志（[stat]/[mode]/[fault]），
 * 它同时还是注入器的输入口。二进制帧一旦混进同一条流，日志就没法用行解析，
 * 判定器（expect.py）也会被任意字节打断。USART2 做纯下行，两条流互不影响。
 * 前提已实测：QEMU 探针 [13] 证明 `-M stm32vldiscovery` 把 USART2 接到了宿主
 * 的第二个串口后端（logs/probe_usart2.bin 收到 8 字节特征串）。
 *
 * 帧格式与上行同构：
 *
 *   AA 55 | TYPE | SEQ | LEN | PAYLOAD(LEN) | CRC16-LE
 *
 * CRC16-CCITT-FALSE 覆盖 TYPE..PAYLOAD，不含 SOF——理由与上行一致（见 av_frame.h）：
 * SOF 只负责定位边界，CRC 只负责内容完整，绑在一起会让"同步错了"和"数据错了"
 * 在日志里长得一样。
 *
 * STATUS 载荷 18 字节（多字节字段小端）：
 *
 *   [0..3]  uptime_ms  u32   固件毫秒（tick）
 *   [4]     mode       u8    av_mode_t
 *   [5]     flags      u8    AV_F_* 位图
 *   [6..7]  cmd_cdeg   i16   控制指令（0.01°）
 *   [8..9]  airspeed   i16   最近有效空速（0.1 m/s）
 *   [10..11] pitch     i16   最近有效俯仰（0.01°）
 *   [12..13] frames    u16   CRC 正确的 SENSOR 帧数（截断到 16 位）
 *   [14..15] crc_err   u16   CRC 错误帧数
 *   [16..17] gap       u16   序号缺口累计
 */
#ifndef AV_DOWNLINK_H
#define AV_DOWNLINK_H

#include <stdint.h>

#include "FreeRTOS.h"
#include "av_health.h"
#include "av_mode.h"
#include "av_sensor.h"

#define AV_DL_TYPE_STATUS   0x81U
#define AV_DL_STATUS_LEN    18U
#define AV_DL_FRAME_MAX     ( 5U + AV_DL_STATUS_LEN + 2U )

/**
 * 初始化 USART2 仅发送通道。
 * pclk1_hz 是 APB1 时钟：本工程未改 RCC->CFGR，PPRE1 复位值 /1，故 PCLK1 = HCLK。
 * 返回 1 = 已配置；0 = 参数非法（baud 为 0）。
 */
int av_downlink_init( uint32_t pclk1_hz, uint32_t baud );

/**
 * 发送一帧 STATUS。在 health 任务里调用，100 ms 一次。
 * 返回 1 = 整帧写完；0 = TXE 等待超时（帧被截断，宿主侧会因 CRC 失败丢弃）。
 * 不阻塞超过 AV_DL_TXE_GUARD 次轮询，避免 USART2 不在时把监督任务挂死。
 */
int av_downlink_status( TickType_t now,
                        av_mode_t mode,
                        uint32_t flags,
                        int32_t cmd_cdeg,
                        const av_sensor_t *sensor,
                        const av_health_stats_t *stats );

#endif /* AV_DOWNLINK_H */
