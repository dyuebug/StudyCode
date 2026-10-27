/**
 * 传感器样本的有效性判定
 *
 * 三层判据，刻意分开，因为它们的失败含义不同：
 *
 *   1. 帧层（av_frame.c）：SOF / LEN / CRC —— "这串字节是不是一帧完整的帧"
 *   2. 样本层（本文件）：范围 / 变化率 / 数据源标志 —— "这帧内容能不能用"
 *   3. 时间层（av_health.c）：超时 / 陈旧 —— "还有没有新的东西"
 *
 * 为什么范围检查必须在 CRC 之后：CRC 只证明比特没翻，不证明传感器没坏。
 * 越界值配着正确的 CRC 是真实故障里最常见的形态。
 *
 * 为什么变化率要检查"距上一帧的时间"：若两次采样相隔很久（例如链路中断后
 * 恢复），正常的量值变化会被误判成突变。所以只有间隔在 AV_RATE_WINDOW_MS
 * 以内才做变化率判定——这条规则是从"误报"反推出来的，不是教科书抄的。
 *
 * 但这个间隔必须按"上一**收到**的样本"计算，不能按"上一**有效**样本"：
 * 后者在连续拒收时不再前进，窗口很快失效，突变保护会自己关掉自己。
 * 首轮实测就是因为这一点漏掉了 RATE 阶段的 STALE，见 av_sensor.c 的注释。
 */
#ifndef AV_SENSOR_H
#define AV_SENSOR_H

#include <stdint.h>

#include "FreeRTOS.h"
#include "av_frame.h"

/* 量纲：空速 0.1 m/s，姿态 0.01°（整数传输，避免在 MCU 上引浮点） */
#define AV_AIRSPEED_MIN_DMPS   0
#define AV_AIRSPEED_MAX_DMPS   400       /* 40.0 m/s */
#define AV_ATTITUDE_MAX_CDEG   6000      /* ±60.00° */

#define AV_RATE_WINDOW_MS      200U      /* 相邻帧间隔超过此值不做变化率判定 */
#define AV_RATE_DV_DMPS        100       /* 空速跳变 > 10.0 m/s */
#define AV_RATE_DPITCH_CDEG    2000      /* 俯仰跳变 > 20.00° */
#define AV_RATE_DROLL_CDEG     3000      /* 滚转跳变 > 30.00° */

typedef struct {
    int16_t  airspeed_dmps;
    int16_t  pitch_cdeg;
    int16_t  roll_cdeg;
    uint16_t flags;                      /* bit0 = 数据源自报无效 */
} av_sensor_sample_t;

typedef enum {
    AV_VERDICT_OK = 0,
    AV_VERDICT_RANGE,                    /* 越界 */
    AV_VERDICT_RATE,                     /* 相对上一有效帧变化率过大 */
    AV_VERDICT_SOURCE                    /* 数据源自己标记无效 */
} av_verdict_t;

typedef struct {
    av_sensor_sample_t last_good;
    TickType_t         last_good_tick;
    TickType_t         last_seen_tick;    /* 最近一次**收到**样本（无论是否有效） */
    int                have_good;
    uint32_t           accepted;
    uint32_t           rej_range;
    uint32_t           rej_rate;
    uint32_t           rej_source;
} av_sensor_t;

void av_sensor_init( av_sensor_t *s );

/** 把 SENSOR 帧载荷解码成样本。返回 0 表示长度正确。 */
int av_sensor_decode( const av_frame_t *f, av_sensor_sample_t *out );

/**
 * 提交一个样本。返回判定结果；只有 AV_VERDICT_OK 时才刷新 last_good，
 * 因此"收到但无效"与"没收到"在时间层上会表现为同一种陈旧，而 RANGE/RATE
 * 标志负责区分原因。
 */
av_verdict_t av_sensor_accept( av_sensor_t *s, const av_sensor_sample_t *raw, TickType_t now );

/** 距最近一次有效样本的毫秒数；从未收到过有效样本时返回 UINT32_MAX。 */
uint32_t av_sensor_age( const av_sensor_t *s, TickType_t now );

#endif /* AV_SENSOR_H */
