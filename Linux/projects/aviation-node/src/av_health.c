#include <string.h>

#include "FreeRTOS.h"
#include "task.h"          /* xTaskGetTickCount */

#include "av_health.h"
#include "av_tick.h"       /* av_tick_elapsed：跨任务的 tick 差值 */

enum {
    AV_HOLD_RANGE = 0,
    AV_HOLD_RATE,
    AV_HOLD_SOURCE,
    AV_HOLD_COUNT
};

static av_health_stats_t s_stats;

static TickType_t s_last_frame_tick;     /* 最近一个 CRC 正确的帧 */
static TickType_t s_rx_kick_tick;        /* rx 任务最近一次心跳 */
static uint32_t   s_crc_streak;          /* 连续 CRC 错帧数 */

static TickType_t s_hold_tick[ AV_HOLD_COUNT ];
static uint32_t   s_hold_mask;           /* 哪些槽位已经被写过（避免 0 值误判为"刚刚写过"） */

static int hold_slot( uint32_t flag )
{
    switch( flag )
    {
        case AV_F_RANGE:  return AV_HOLD_RANGE;
        case AV_F_RATE:   return AV_HOLD_RATE;
        case AV_F_SOURCE: return AV_HOLD_SOURCE;
        default:          return -1;
    }
}

void av_health_init( void )
{
    memset( &s_stats, 0, sizeof( s_stats ) );
    memset( s_hold_tick, 0, sizeof( s_hold_tick ) );
    s_last_frame_tick = 0U;
    s_rx_kick_tick    = 0U;
    s_crc_streak      = 0U;
    s_hold_mask       = 0U;
}

void av_health_kick( void )
{
    s_rx_kick_tick = xTaskGetTickCount();
    s_stats.kicks++;
}

void av_health_frame_ok( uint8_t seq )
{
    s_last_frame_tick = xTaskGetTickCount();
    s_crc_streak      = 0U;
    s_stats.frames_ok++;

    if( s_stats.have_seq != 0 )
    {
        /* 序号是 uint8，按无符号差值算前进距离：0 表示重复，n 表示丢了 n-1 帧 */
        uint8_t d = ( uint8_t ) ( seq - s_stats.last_seq );

        if( d == 0U )
        {
            s_stats.duplicates++;
        }
        else
        {
            s_stats.seq_gaps += ( uint32_t ) ( d - 1U );
        }
    }

    s_stats.last_seq = seq;
    s_stats.have_seq = 1;
}

void av_health_crc_error( void )
{
    s_stats.crc_errors++;

    if( s_crc_streak < 0xFFFFFFFFU )
    {
        s_crc_streak++;
    }
}

void av_health_len_error( void )
{
    s_stats.len_errors++;
    s_crc_streak = 0U;     /* 长度错误属于结构错误，不应积累 CRC 连续计数 */
}

void av_health_type_error( void )
{
    s_stats.type_errors++;
}

void av_health_mark( uint32_t flag )
{
    int slot = hold_slot( flag );

    if( slot < 0 )
    {
        return;
    }

    s_hold_tick[ slot ] = xTaskGetTickCount();
    s_hold_mask |= ( 1UL << ( uint32_t ) slot );
}

uint32_t av_health_flags( const av_sensor_t *sensor, TickType_t now )
{
    uint32_t flags = 0U;

    if( av_tick_elapsed( now, s_last_frame_tick ) > pdMS_TO_TICKS( AV_LINK_TIMEOUT_MS ) )
    {
        flags |= AV_F_TIMEOUT;
    }

    /* 从未收到有效样本时不算"陈旧"：上电初期没有数据是正常的，
     * 由 INIT 的停留超时负责，不在这里重复报同一个故障。 */
    if( ( sensor->have_good != 0 ) && ( av_sensor_age( sensor, now ) > pdMS_TO_TICKS( AV_DATA_TIMEOUT_MS ) ) )
    {
        flags |= AV_F_STALE;
    }

    if( s_crc_streak >= AV_CRC_STREAK_MAX )
    {
        flags |= AV_F_CRC;
    }

    for( int i = 0; i < AV_HOLD_COUNT; i++ )
    {
        if( ( s_hold_mask & ( 1UL << ( uint32_t ) i ) ) == 0U )
        {
            continue;
        }

        if( av_tick_elapsed( now, s_hold_tick[ i ] ) <= pdMS_TO_TICKS( AV_FAULT_HOLD_MS ) )
        {
            flags |= ( 1UL << ( uint32_t ) ( i + 3 ) );   /* AV_HOLD_* 与 AV_F_* 的位序一一对应 */
        }
    }

    return flags;
}

uint32_t av_health_rx_age( TickType_t now )
{
    return av_tick_elapsed( now, s_rx_kick_tick );
}

const av_health_stats_t *av_health_stats( void )
{
    return &s_stats;
}
