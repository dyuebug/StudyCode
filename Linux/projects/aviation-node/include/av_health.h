/**
 * 健康管理与故障标志
 *
 * 故障标志是**整个系统的公共语言**：rx 任务写，ctrl 任务读（驱动模式状态机），
 * health 任务读（喂狗门控与统计）。因此这里只做两件事：
 *
 *   1. 记账（帧数、CRC 错、序号缺口、各类拒收）
 *   2. 把"当前有哪些故障"换算成一个位图，且全部是**纯函数**——谁来读都不改状态
 *
 * 三组标志的语义刻意分开：
 *
 *   AV_F_TIMEOUT  链路层：300 ms 内没有任何 CRC 正确的帧 → 线断了/主机停了
 *   AV_F_STALE    数据层：300 ms 内没有任何**通过全部校验**的样本 → 收到也没用
 *   AV_F_CRC      CRC 错连续 3 帧 → 物理层质量差
 *   AV_F_RANGE/RATE/SOURCE  由样本层判定，最后一次出现后保持 AV_FAULT_HOLD_MS
 *
 * 为什么 TIMEOUT 与 STALE 要分开：若只留一个"超时"标志，那么"帧一直来但全部
 * 越界"和"一个字节都没来"在日志里无法区分，而这两种故障的处置动作完全不同
 * （前者要切备份传感器，后者要查链路）。
 *
 * 为什么要"保持"：RANGE 这类标志由单帧事件产生，若下一帧正常就立刻清除，模式
 * 状态机会在边界值上来回跳。保持时间相当于一个去抖窗口，取值必须大于采样周期
 * 且远小于模式切换的时间常数。
 */
#ifndef AV_HEALTH_H
#define AV_HEALTH_H

#include <stdint.h>

#include "FreeRTOS.h"
#include "av_sensor.h"

#define AV_F_TIMEOUT          ( 1UL << 0 )
#define AV_F_STALE            ( 1UL << 1 )
#define AV_F_CRC              ( 1UL << 2 )
#define AV_F_RANGE            ( 1UL << 3 )
#define AV_F_RATE             ( 1UL << 4 )
#define AV_F_SOURCE           ( 1UL << 5 )

#define AV_LINK_TIMEOUT_MS    300U
#define AV_DATA_TIMEOUT_MS    300U
#define AV_CRC_STREAK_MAX     3U
#define AV_FAULT_HOLD_MS      1000U      /* 单帧故障保持窗口 */

typedef struct {
    uint32_t frames_ok;      /* CRC 正确且长度正确的 SENSOR 帧 */
    uint32_t crc_errors;
    uint32_t len_errors;
    uint32_t type_errors;
    uint32_t seq_gaps;       /* 按序号推算的丢失帧数 */
    uint32_t duplicates;     /* 重复序号 */
    uint32_t kicks;          /* rx 任务心跳次数 */
    uint8_t  last_seq;
    int      have_seq;
} av_health_stats_t;

void av_health_init( void );

/* ---- 由 rx 任务调用 ---------------------------------------------------------- */

/** rx 任务每轮主循环调用一次（无论是否收到字节），用于证明"任务还在跑"。 */
void av_health_kick( void );

void av_health_frame_ok( uint8_t seq );
void av_health_crc_error( void );
void av_health_len_error( void );
void av_health_type_error( void );

/** 样本层拒收：flag 取 AV_F_RANGE / AV_F_RATE / AV_F_SOURCE。 */
void av_health_mark( uint32_t flag );

/* ---- 由 ctrl / health 任一线程调用（纯函数） -------------------------------- */

uint32_t av_health_flags( const av_sensor_t *sensor, TickType_t now );

/** 距 rx 任务最近一次心跳的毫秒数。 */
uint32_t av_health_rx_age( TickType_t now );

const av_health_stats_t *av_health_stats( void );

#endif /* AV_HEALTH_H */
