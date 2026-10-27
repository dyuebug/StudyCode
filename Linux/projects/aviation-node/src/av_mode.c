#include "av_mode.h"

void av_mode_init( av_mode_ctx_t *ctx, TickType_t now )
{
    ctx->mode        = AV_MODE_INIT;
    ctx->enter_tick  = now;
    ctx->good_streak = 0U;
}

const char *av_mode_name( av_mode_t mode )
{
    switch( mode )
    {
        case AV_MODE_INIT:     return "INIT";
        case AV_MODE_NORMAL:   return "NORMAL";
        case AV_MODE_DEGRADED: return "DEGRADED";
        case AV_MODE_SAFE:     return "SAFE";
        default:               return "?";
    }
}

av_mode_t av_mode_step( av_mode_ctx_t *ctx, uint32_t flags, TickType_t now )
{
    uint32_t  dwell = ( uint32_t ) ( now - ctx->enter_tick );
    int       fault = ( flags != 0U ) ? 1 : 0;
    av_mode_t next  = ctx->mode;

    /* 判据统一用"无符号差值 + 阈值"，与其他超时判定保持同一套写法 */
    if( fault != 0 )
    {
        ctx->good_streak = 0U;
    }

    switch( ctx->mode )
    {
        case AV_MODE_INIT:
            if( ( fault == 0 ) && ( ctx->good_streak >= AV_GOOD_TO_NORMAL ) )
            {
                next = AV_MODE_NORMAL;
            }
            else if( dwell >= pdMS_TO_TICKS( AV_INIT_TIMEOUT_MS ) )
            {
                /* 上电后一直没数据：不必等故障标志，直接判"数据源不可用" */
                next = AV_MODE_SAFE;
            }
            break;

        case AV_MODE_NORMAL:
            if( fault != 0 )
            {
                next = AV_MODE_DEGRADED;
            }
            break;

        case AV_MODE_DEGRADED:
            if( ( fault == 0 ) && ( ctx->good_streak >= AV_GOOD_TO_NORMAL ) )
            {
                next = AV_MODE_NORMAL;
            }
            else if( ( fault != 0 ) && ( dwell >= pdMS_TO_TICKS( AV_DEGRADED_TIMEOUT_MS ) ) )
            {
                next = AV_MODE_SAFE;
            }
            break;

        case AV_MODE_SAFE:
            if( ( fault == 0 ) && ( ctx->good_streak >= AV_SAFE_RECOVER_GOOD ) )
            {
                next = AV_MODE_DEGRADED;   /* 两级恢复，不直接回 NORMAL */
            }
            break;

        default:
            next = AV_MODE_INIT;
            break;
    }

    if( next != ctx->mode )
    {
        ctx->mode        = next;
        ctx->enter_tick  = now;
        ctx->good_streak = 0U;
    }

    return next;
}
