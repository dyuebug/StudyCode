#include <string.h>

#include "av_frame.h"

static uint16_t crc_update( uint16_t crc, uint8_t byte )
{
    crc ^= ( uint16_t ) ( ( uint16_t ) byte << 8 );

    for( uint32_t i = 0U; i < 8U; i++ )
    {
        if( ( crc & 0x8000U ) != 0U )
        {
            crc = ( uint16_t ) ( ( crc << 1 ) ^ 0x1021U );
        }
        else
        {
            crc = ( uint16_t ) ( crc << 1 );
        }
    }

    return crc;
}

uint16_t av_crc16_ccitt_false( const uint8_t *data, uint32_t len )
{
    uint16_t crc = 0xFFFFU;

    for( uint32_t i = 0U; i < len; i++ )
    {
        crc = crc_update( crc, data[ i ] );
    }

    return crc;
}

void av_parser_init( av_parser_t *p )
{
    memset( p, 0, sizeof( *p ) );
    p->state = AV_RX_SOF0;
}

av_frame_result_t av_parser_push( av_parser_t *p, uint8_t byte, av_frame_t *out )
{
    switch( p->state )
    {
        case AV_RX_SOF0:
            if( byte == AV_SOF0 )
            {
                p->state = AV_RX_SOF1;
            }
            return AV_FRAME_NONE;

        case AV_RX_SOF1:
            if( byte == AV_SOF1 )
            {
                p->state = AV_RX_TYPE;
            }
            else if( byte == AV_SOF0 )
            {
                /* 0xAA 0xAA 0x55：把第二个 0xAA 当成新的帧头，否则会漏掉这一帧。
                 * 这类"重复前缀"是字节流同步里最常见的细节，不是随手写的防御。 */
                p->state = AV_RX_SOF1;
            }
            else
            {
                p->state = AV_RX_SOF0;
            }
            return AV_FRAME_NONE;

        case AV_RX_TYPE:
            p->frame.type = byte;
            p->crc_calc   = crc_update( 0xFFFFU, byte );
            p->state      = AV_RX_SEQ;
            return AV_FRAME_NONE;

        case AV_RX_SEQ:
            p->frame.seq = byte;
            p->crc_calc  = crc_update( p->crc_calc, byte );
            p->state     = AV_RX_LEN;
            return AV_FRAME_NONE;

        case AV_RX_LEN:
            p->frame.len = byte;
            p->crc_calc  = crc_update( p->crc_calc, byte );

            if( byte > AV_PAYLOAD_MAX )
            {
                /* LEN 非法时立即放弃本帧。注意：该帧剩余的载荷字节会被当作
                 * "可能的帧头"重新搜索，最坏的代价是多丢一帧——比越界写缓冲安全。 */
                av_parser_init( p );
                return AV_FRAME_BAD_LEN;
            }

            p->idx   = 0U;
            p->state = ( byte == 0U ) ? AV_RX_CRC_LO : AV_RX_PAYLOAD;
            return AV_FRAME_NONE;

        case AV_RX_PAYLOAD:
            p->frame.payload[ p->idx++ ] = byte;
            p->crc_calc                  = crc_update( p->crc_calc, byte );

            if( p->idx >= ( uint32_t ) p->frame.len )
            {
                p->state = AV_RX_CRC_LO;
            }
            return AV_FRAME_NONE;

        case AV_RX_CRC_LO:
            p->crc_recv = byte;
            p->state    = AV_RX_CRC_HI;
            return AV_FRAME_NONE;

        case AV_RX_CRC_HI:
        {
            p->crc_recv |= ( uint16_t ) ( ( uint16_t ) byte << 8 );

            int ok = ( p->crc_recv == p->crc_calc );

            if( ok != 0 )
            {
                *out = p->frame;
            }

            av_parser_init( p );
            return ok ? AV_FRAME_OK : AV_FRAME_BAD_CRC;
        }

        default:
            av_parser_init( p );
            return AV_FRAME_NONE;
    }
}
