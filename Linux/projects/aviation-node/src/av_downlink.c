#include "av_downlink.h"

#include "stm32f1xx.h"

#include "av_frame.h"

/*
 * TXE 等待上限：25 字节在 115200 下约 2.2 ms，轮询次数留两个数量级余量。
 * 设上限的目的不是防"发送慢"，而是防 USART2 根本不存在时把 health 任务挂死——
 * 监督任务卡住比丢一帧严重得多。
 */
#define AV_DL_TXE_GUARD   200000U

static uint8_t s_seq;

static void put_u16( uint8_t *p, uint16_t v )
{
    p[0] = ( uint8_t ) ( v & 0xFFU );
    p[1] = ( uint8_t ) ( ( v >> 8 ) & 0xFFU );
}

static void put_u32( uint8_t *p, uint32_t v )
{
    p[0] = ( uint8_t ) ( v & 0xFFU );
    p[1] = ( uint8_t ) ( ( v >> 8 ) & 0xFFU );
    p[2] = ( uint8_t ) ( ( v >> 16 ) & 0xFFU );
    p[3] = ( uint8_t ) ( ( v >> 24 ) & 0xFFU );
}

static int dl_put( uint8_t b )
{
    uint32_t guard = 0U;

    while( ( ( USART2->SR & USART_SR_TXE ) == 0U ) && ( guard < AV_DL_TXE_GUARD ) )
    {
        guard++;
    }

    if( guard >= AV_DL_TXE_GUARD )
    {
        return 0;
    }

    USART2->DR = b;
    return 1;
}

int av_downlink_init( uint32_t pclk1_hz, uint32_t baud )
{
    if( baud == 0U )
    {
        return 0;
    }

    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    /* PA2 = USART2_TX：CRL 的 bit[11:8]，复用推挽输出 50 MHz = CNF 10 + MODE 11 = 0xB。
     * QEMU 不建模 GPIO，这几位在仿真里没有效果；真板上没有它 USART2 就发不出去，
     * 因此保留——但"真板上是否真的发出波形"属于未验证项（需逻辑分析仪）。 */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    GPIOA->CRL = ( GPIOA->CRL & ~0x00000F00U ) | 0x00000B00U;

    USART2->CR1 = 0U;   /* 配置期间先关 UE，避免半配置状态上线 */
    USART2->CR2 = 0U;
    USART2->CR3 = 0U;

    /* 与 f103_uart 相同的取整方式：BRR = round(PCLK1 / baud)。
     * PCLK1 = HCLK = 8 MHz 时 8e6/115200 = 69.44 → 69（0x45），误差 +0.64%。 */
    USART2->BRR = ( pclk1_hz + ( baud / 2U ) ) / baud;

    USART2->CR1 = USART_CR1_UE | USART_CR1_TE;

    s_seq = 0U;
    return 1;
}

int av_downlink_status( TickType_t now,
                        av_mode_t mode,
                        uint32_t flags,
                        int32_t cmd_cdeg,
                        const av_sensor_t *sensor,
                        const av_health_stats_t *stats )
{
    uint8_t  buf[AV_DL_FRAME_MAX];
    uint16_t crc;
    uint32_t i;

    /* 头 5 字节：SOF + TYPE + SEQ + LEN */
    buf[0] = 0xAAU;
    buf[1] = 0x55U;
    buf[2] = AV_DL_TYPE_STATUS;
    buf[3] = s_seq++;
    buf[4] = AV_DL_STATUS_LEN;

    /* 载荷 18 字节，起点 5 */
    put_u32( &buf[5], ( uint32_t ) now );
    buf[9]  = ( uint8_t ) mode;
    buf[10] = ( uint8_t ) ( flags & 0xFFU );
    put_u16( &buf[11], ( uint16_t ) cmd_cdeg );
    put_u16( &buf[13], ( uint16_t ) sensor->last_good.airspeed_dmps );
    put_u16( &buf[15], ( uint16_t ) sensor->last_good.pitch_cdeg );
    put_u16( &buf[17], ( uint16_t ) stats->frames_ok );
    put_u16( &buf[19], ( uint16_t ) stats->crc_errors );
    put_u16( &buf[21], ( uint16_t ) stats->seq_gaps );

    /* CRC 覆盖 TYPE..PAYLOAD：从 buf[2] 起 3 + LEN 字节 */
    crc = av_crc16_ccitt_false( &buf[2], 3U + AV_DL_STATUS_LEN );
    put_u16( &buf[23], crc );

    for( i = 0U; i < AV_DL_FRAME_MAX; i++ )
    {
        if( dl_put( buf[i] ) == 0 )
        {
            return 0;   /* 截断帧由宿主侧 CRC 丢弃，不在这里重试 */
        }
    }

    return 1;
}
