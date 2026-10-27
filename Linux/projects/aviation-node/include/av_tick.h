/**
 * tick 差值的统一算法（供 av_sensor / av_health 共用）
 *
 * FreeRTOS 的 TickType_t 是无符号的，`now - then` 只在 now ≥ then 时等于"经过的
 * 毫秒数"。本工程里 now 一般是调用方在循环开始时取的快照，而优先级更高的 rx 任务
 * 会在这之后更新心跳、故障保持、最近有效样本等时间戳——这些时间戳因此可能比 now
 * 大 1～2 tick，无符号减法会下溢成 ~4.29e9：
 *
 *   - 心跳年龄 → 读成"链路早断了"，门控误停一次喂狗；
 *   - 故障保持期 → 读成"保持已过"，故障位瞬断一周期；
 *   - 日志里的 age → 打印 4294967292 这种明显不合理的值。
 *
 * 用法：一律把"可能比快照晚"的时间戳交给这里，负数压成 0，取"刚发生"这一侧。
 * 同一个任务的 `now` 与其自身时间戳比较（如 av_sensor_accept 内部的变化率窗口）
 * 不经过这里：那里两边同源，没有跨任务顺序问题。
 */
#ifndef AV_TICK_H
#define AV_TICK_H

#include <stdint.h>

#include "FreeRTOS.h"

static inline uint32_t av_tick_elapsed( TickType_t now, TickType_t then )
{
    if( ( int32_t ) ( now - then ) < 0 )
    {
        return 0U;
    }

    return ( uint32_t ) ( now - then );
}

#endif /* AV_TICK_H */
