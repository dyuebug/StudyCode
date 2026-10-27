/**
 * 模式状态机（INIT → NORMAL → DEGRADED → SAFE）
 *
 * 设计要点：
 *
 * 1. 状态机只吃"故障标志的白名单子集是否为 0"，不关心具体是哪一位。原因位是
 *    健康管理层（av_health.c）的职责；状态机关心的是"系统还能不能用这套数据"。
 *    子集由 av_health.h 的 AV_F_MODE_MASK 给出：新增一位时默认**不参与**判定，
 *    要参与必须显式加进掩码。地面站心跳（AV_F_GROUND）就是不参与的例子——
 *    它在同一条链路上，但不属于"数据能不能用"。
 * 2. 恢复必须比降级更难：降级只要一次故障，恢复要求"连续 N 个有效帧且
 *    无任何故障"。否则边界数据会让模式来回抖动，输出跟着抖。
 * 3. DEGRADED 有停留上限：故障持续超过 AV_DEGRADED_TIMEOUT_MS 就进 SAFE。
 *    降级只是缓冲，不是终点——长期带着坏数据运行比明确停用更危险。
 * 4. SAFE 不直接回 NORMAL，先回 DEGRADED 并停留观察。这条"两级恢复"是
 *    飞控/航电里常见的处理：跳过中间态意味着一次跳回正常。
 */
#ifndef AV_MODE_H
#define AV_MODE_H

#include <stdint.h>

#include "FreeRTOS.h"

#define AV_GOOD_TO_NORMAL        3U      /* INIT/DEGRADED → NORMAL 要求的连续有效帧数 */
#define AV_SAFE_RECOVER_GOOD     5U      /* SAFE → DEGRADED 要求的连续有效帧数 */
#define AV_INIT_TIMEOUT_MS       3000U   /* INIT 里始终无有效数据则直接进 SAFE */

/* DEGRADED 停留门限必须**明显大于** av_health.h 的 AV_FAULT_HOLD_MS。
 * 原因：故障标志在最后一次出现后还会保持 HOLD_MS，若两者接近，一次很短的
 * 越界事件就能占满停留计时并推进到 SAFE——这是 2026-10-26 首轮实测发现的
 * （400 ms 的越界把系统推到了 SAFE），不是理论担忧。取 3 倍保持窗口后，
 * "停留 2000 ms 以上"才真正等价于"故障持续 2000 ms 以上"。 */
#define AV_DEGRADED_TIMEOUT_MS   3000U

typedef enum {
    AV_MODE_INIT = 0,
    AV_MODE_NORMAL,
    AV_MODE_DEGRADED,
    AV_MODE_SAFE
} av_mode_t;

typedef struct {
    av_mode_t  mode;
    TickType_t enter_tick;     /* 进入当前模式的时刻，用于"停留时长"判据 */
    uint8_t    good_streak;    /* 连续有效帧数，由采样侧递增、状态机清零 */
} av_mode_ctx_t;

void av_mode_init( av_mode_ctx_t *ctx, TickType_t now );

const char *av_mode_name( av_mode_t mode );

/**
 * 推进状态机。flags 为 0 表示健康。
 * 返回推进后的模式；发生切换时 ctx->enter_tick 被刷新。
 */
av_mode_t av_mode_step( av_mode_ctx_t *ctx, uint32_t flags, TickType_t now );

#endif /* AV_MODE_H */
