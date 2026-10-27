#include <string.h>

#include "av_sensor.h"
#include "av_tick.h"       /* av_tick_elapsed：跨任务的 tick 差值 */

static int16_t rd_i16( const uint8_t *p )
{
    return ( int16_t ) ( ( uint16_t ) p[ 0 ] | ( ( uint16_t ) p[ 1 ] << 8 ) );
}

static uint16_t rd_u16( const uint8_t *p )
{
    return ( uint16_t ) ( ( uint16_t ) p[ 0 ] | ( ( uint16_t ) p[ 1 ] << 8 ) );
}

static int32_t iabs32( int32_t v )
{
    return ( v < 0 ) ? -v : v;
}

void av_sensor_init( av_sensor_t *s )
{
    memset( s, 0, sizeof( *s ) );
}

int av_sensor_decode( const av_frame_t *f, av_sensor_sample_t *out )
{
    if( f->len != AV_SENSOR_PAYLOAD_LEN )
    {
        return -1;
    }

    out->airspeed_dmps = rd_i16( &f->payload[ 0 ] );
    out->pitch_cdeg    = rd_i16( &f->payload[ 2 ] );
    out->roll_cdeg     = rd_i16( &f->payload[ 4 ] );
    out->flags         = rd_u16( &f->payload[ 6 ] );
    return 0;
}

static int out_of_range( const av_sensor_sample_t *raw )
{
    if( ( raw->airspeed_dmps < AV_AIRSPEED_MIN_DMPS ) || ( raw->airspeed_dmps > AV_AIRSPEED_MAX_DMPS ) )
    {
        return 1;
    }

    if( ( raw->pitch_cdeg < -AV_ATTITUDE_MAX_CDEG ) || ( raw->pitch_cdeg > AV_ATTITUDE_MAX_CDEG ) )
    {
        return 1;
    }

    if( ( raw->roll_cdeg < -AV_ATTITUDE_MAX_CDEG ) || ( raw->roll_cdeg > AV_ATTITUDE_MAX_CDEG ) )
    {
        return 1;
    }

    return 0;
}

av_verdict_t av_sensor_accept( av_sensor_t *s, const av_sensor_sample_t *raw, TickType_t now )
{
    TickType_t prev_seen = s->last_seen_tick;

    s->last_seen_tick = now;

    if( ( raw->flags & 0x0001U ) != 0U )
    {
        s->rej_source++;
        return AV_VERDICT_SOURCE;
    }

    if( out_of_range( raw ) != 0 )
    {
        s->rej_range++;
        return AV_VERDICT_RANGE;
    }

    /* 变化率：判据用"距上一**收到**的样本"的时间，不是"距上一**有效**样本"。
     *
     * 这个区别是首轮实测逼出来的：用上次有效样本做参考时，一旦连续拒收，
     * 参考时间戳就不再前进，200 ms 后窗口判定失效，突变值反而被接受——等于
     * 突变保护在持续突变下自动关闭。改用上次收到的样本后，只要帧还在来，
     * 变化率检查就一直生效；只有真正的链路空档（> 200 ms 没有帧）才跳过。 */
    if( ( s->have_good != 0 ) && ( ( now - prev_seen ) <= pdMS_TO_TICKS( AV_RATE_WINDOW_MS ) ) )
    {
        int32_t dv  = iabs32( ( int32_t ) raw->airspeed_dmps - ( int32_t ) s->last_good.airspeed_dmps );
        int32_t dp  = iabs32( ( int32_t ) raw->pitch_cdeg - ( int32_t ) s->last_good.pitch_cdeg );
        int32_t dr  = iabs32( ( int32_t ) raw->roll_cdeg - ( int32_t ) s->last_good.roll_cdeg );

        if( ( dv > AV_RATE_DV_DMPS ) || ( dp > AV_RATE_DPITCH_CDEG ) || ( dr > AV_RATE_DROLL_CDEG ) )
        {
            s->rej_rate++;
            return AV_VERDICT_RATE;
        }
    }

    s->last_good      = *raw;
    s->last_good_tick = now;
    s->have_good      = 1;
    s->accepted++;
    return AV_VERDICT_OK;
}

uint32_t av_sensor_age( const av_sensor_t *s, TickType_t now )
{
    if( s->have_good == 0 )
    {
        return UINT32_MAX;
    }

    /* tick 约 49.7 天回绕一次，必须用差值而不是 now > last；而 last_good_tick 由
     * rx 任务写入，可能比调用方（health）的快照晚 1～2 tick，所以差值要走
     * av_tick_elapsed 把下溢压成 0（否则日志里的 age 会打印 4294967292）。 */
    return av_tick_elapsed( now, s->last_good_tick );
}
