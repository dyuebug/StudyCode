/*
 * 航空主题地面仿真节点 —— 可仿真部分
 *
 * 数据链路（对应 Linux/CLAUDE.md §12 P1）：
 *
 *   PC 注入帧 ──UART──> USART1 RXNE ISR ──队列──> rx 任务（解析 + 校验）
 *                                                     │
 *                                       有效样本 ─────┤──> ctrl 任务（20 ms）
 *                                       故障标志 ─────┘        │
 *                                                              ├─> 模式状态机
 *   health 任务（100 ms）<── 故障标志/统计                       └─> 控制输出（限幅）
 *        │
 *        └─> 喂狗门控（"与"语义）+ 5 s 状态上报
 *
 * 三条任务线刻意用不同周期，因为它们的时限来源不同：
 *   rx      事件驱动，只受"字节到达"约束
 *   ctrl    20 ms（50 Hz），受被控对象时间常数约束
 *   health  100 ms，受"故障多快必须被报出来"约束
 *
 * 本目标不可仿真的部分（CAN、ADC、示波器/逻辑分析仪、IWDG 计时与复位）见
 * README「硬件依赖项（待真板）」一节，不在这里假装实现。
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "stm32f1xx.h"
#include "f103_uart.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "av_frame.h"
#include "av_sensor.h"
#include "av_mode.h"
#include "av_health.h"
#include "av_downlink.h"

#define UART_BAUD             115200U
#define RX_QUEUE_DEPTH        64U

/* 能调用 FreeRTOS API 的中断，优先级数值必须 >= configMAX_SYSCALL_INTERRUPT_PRIORITY(5)，
 * 否则 BASEPRI 屏蔽不住它，可能在中断里破坏内核链表。理由详见 F103_11 实验。 */
#define USART1_IRQ_PRIO       5U

#define CTRL_PERIOD_MS        20U       /* 50 Hz */
#define HEALTH_PERIOD_MS      100U
#define STAT_PERIOD_MS        5000U
#define RX_HB_TIMEOUT_MS      400U      /* rx 心跳超时（≥4 个检查周期） */

/* 停喂原因位：只为了让日志能区分"没数据"和"任务卡住"，不参与任何判定。
 * 没有它时，两种停喂在 [stat] 里长得一样——只能靠人去看 hb。 */
#define GATE_REASON_HB        0x1U
#define GATE_REASON_FLAGS     0x2U

/* 故障注入（第 9–10 周"任务阻塞"剧本）：到 AV_DEBUG_BLOCK_AT_MS 时让 rx 任务空转
 * AV_DEBUG_BLOCK_RX_MS 毫秒。时刻落在剧本静默段内（队列为空），所以不丢字节、
 * 不扰动解析器状态；注入只是"任务不推进"，不是"链路断开"。
 * 默认为 0 = 不编译这段代码，由 CMakeLists 在 QEMU 构建里打开。 */
#ifndef AV_DEBUG_BLOCK_RX_MS
#define AV_DEBUG_BLOCK_RX_MS  0U
#endif
#ifndef AV_DEBUG_BLOCK_AT_MS
#define AV_DEBUG_BLOCK_AT_MS  16500U
#endif

/* 教学级控制律参数：不对应任何真实机型，只用于验证"限幅/保持/中立位"三档行为 */
#define AIRSPEED_REF_DMPS     150       /* 15.0 m/s 空速保持 */
#define CMD_LIMIT_CDEG        2500      /* 指令限幅 ±25.00° */
#define CMD_STEP_CDEG         100       /* 每 20 ms 最多变化 1.00° */

/* QEMU 未建模 IWDG，真板才置 1。置 1 之前必须确认喂狗条件已由 health 任务独占。 */
#define F103_WATCHDOG_ENABLE  0

static QueueHandle_t     s_rx_queue;
static SemaphoreHandle_t s_log_mutex;

static av_sensor_t   s_sensor;
static av_mode_ctx_t s_mode;

static TaskHandle_t s_h_rx;
static TaskHandle_t s_h_ctrl;
static TaskHandle_t s_h_health;

static int32_t  s_cmd;                 /* 限幅后的升降舵指令，单位 0.01° */
static uint32_t s_prev_flags;           /* 上一拍的故障位图，用于边沿上报 */
static uint32_t s_feeds;
static uint32_t s_feed_blocked;

static volatile uint32_t s_rx_bytes;
static volatile uint32_t s_rx_drops;    /* 队列满，ISR 主动丢弃 */

#define LOG( ... )                                                       \
    do {                                                                 \
        if( xSemaphoreTake( s_log_mutex, portMAX_DELAY ) == pdTRUE ) {    \
            printf( __VA_ARGS__ );                                       \
            xSemaphoreGive( s_log_mutex );                               \
        }                                                                \
    } while( 0 )

/* ---- 硬件看门狗路径（真板） -------------------------------------------------
 *
 * 与 F103_14 相同的写法：函数体里写 IWDG 寄存器，boot 时打印它们的地址。
 * 打印地址不是为了调试，是因为 -ffunction-sections + --gc-sections 会把
 * "没有被引用"的函数整段删掉——__attribute__((used)) 只约束编译器，不约束链接器。
 */
void f103_watchdog_init( void );
void f103_watchdog_feed( void );

void f103_watchdog_init( void )
{
    IWDG->KR  = 0x5555U;   /* 解锁 PR/RLR */
    IWDG->PR  = 0x04U;     /* ÷64，LSI 40 kHz 下名义 998 ms */
    IWDG->RLR = 624U;
    IWDG->KR  = 0xCCCCU;   /* 启动后无法关闭 */
    IWDG->KR  = 0xAAAAU;
}

void f103_watchdog_feed( void )
{
    IWDG->KR = 0xAAAAU;
}

static void watchdog_feed_if_enabled( void )
{
#if ( F103_WATCHDOG_ENABLE != 0 )
    f103_watchdog_feed();
#endif
}

/* ---- 中断 ------------------------------------------------------------------ */

void USART1_IRQHandler( void )
{
    BaseType_t woken = pdFALSE;
    uint32_t   sr    = USART1->SR;

    if( ( sr & ( USART_SR_RXNE | USART_SR_ORE ) ) == 0U )
    {
        return;
    }

    uint8_t ch = ( uint8_t ) USART1->DR;   /* 读 DR 同时清 RXNE 与 ORE */

    if( ( sr & USART_SR_ORE ) != 0U )
    {
        /* 硬件溢出：这个字节已经丢了。教学节点只计数，不上报（等待真板验证 ORE 触发条件）。 */
        return;
    }

    s_rx_bytes++;

    if( xQueueSendFromISR( s_rx_queue, &ch, &woken ) != pdPASS )
    {
        s_rx_drops++;    /* 队列满：丢弃并计数，中断里绝不阻塞 */
    }

    portYIELD_FROM_ISR( woken );
}

/* ---- rx 任务：解析 + 三层校验 ---------------------------------------------- */

static void rx_task( void *arg )
{
    av_parser_t parser;
    uint8_t     ch;
    TickType_t  last_kick = 0U;

    ( void ) arg;

    av_parser_init( &parser );

    /* 中断使能放在调度器启动之后：见 F103_11 注释（此时内核已有当前任务） */
    USART1->CR1 |= USART_CR1_RXNEIE;
    NVIC_SetPriority( USART1_IRQn, USART1_IRQ_PRIO );
    NVIC_EnableIRQ( USART1_IRQn );

    for( ;; )
    {
#if ( AV_DEBUG_BLOCK_RX_MS > 0 )
        /* 一次性故障注入：让 rx 任务"卡住"，其余任务继续跑。
         *
         * 为什么用 taskYIELD() 而不是死循环：rx 优先级 3 高于 health 的 2，硬空转会
         * 把 health 饿死，于是"心跳超时 → 停喂"这条链根本来不及被观察到——注入就
         * 只证明了"调度器还活着"。主动让出 CPU 后，health 每 100 ms 照常运行，
         * 它看到的 rx 心跳年龄就真的在变大。
         *
         * 为什么不在阻塞期间调 av_health_kick()：任务"卡住"的定义就是不推进工作，
         * 顺手刷心跳等于自己报告自己健康，注入就失效了。
         *
         * 为什么是 vTaskDelay(1) 而不是 taskYIELD()/死循环：health 优先级 2 低于 rx 的 3，
         * 忙等时它即使延时到期也抢不到 CPU——监督者被饿死，停喂判定只能等阻塞结束后
         * 才发生（本轮首版实测就是这样：800 ms 内 health 一次都没跑，t=17419 才读到
         * hb=804）。让它每 tick 让出一次，监督链才可观测。代价是注入模型变成"任务不
         * 推进"而非"CPU 忙等"；后者的可见性问题是真实约束，见 README §6.6。
         *
         * 阻塞时长 800 ms > RX_HB_TIMEOUT_MS(400)，保证 health 至少有一次采样落到
         * 超时之后。 */
        {
            static int s_dbg_block_done = 0;

            if( ( s_dbg_block_done == 0 ) &&
                ( ( int32_t ) ( xTaskGetTickCount() - pdMS_TO_TICKS( AV_DEBUG_BLOCK_AT_MS ) ) >= 0 ) )
            {
                TickType_t blk_start = xTaskGetTickCount();

                s_dbg_block_done = 1;
                LOG( "[debug] t=%lu rx 阻塞开始 plant=%u ms（其他任务继续运行）\n",
                     ( unsigned long ) blk_start, ( unsigned ) AV_DEBUG_BLOCK_RX_MS );

                while( ( int32_t ) ( xTaskGetTickCount() - blk_start ) <
                       ( int32_t ) pdMS_TO_TICKS( AV_DEBUG_BLOCK_RX_MS ) )
                {
                    vTaskDelay( 1 );
                }

                LOG( "[debug] t=%lu rx 阻塞结束 dur=%lu ms\n",
                     ( unsigned long ) xTaskGetTickCount(),
                     ( unsigned long ) ( xTaskGetTickCount() - blk_start ) );
            }
        }
#endif

        /* 200 ms 超时只是为了让心跳保持"活着的证据"节奏；无字节时也不阻塞无限久 */
        if( xQueueReceive( s_rx_queue, &ch, pdMS_TO_TICKS( 200 ) ) == pdPASS )
        {
            av_frame_t        f;
            av_frame_result_t r = av_parser_push( &parser, ch, &f );

            if( r == AV_FRAME_BAD_CRC )
            {
                av_health_crc_error();
            }
            else if( r == AV_FRAME_BAD_LEN )
            {
                av_health_len_error();
            }
            else if( r == AV_FRAME_OK )
            {
                if( f.type != AV_TYPE_SENSOR )
                {
                    av_health_type_error();
                }
                else
                {
                    av_sensor_sample_t smp;

                    av_health_frame_ok( f.seq );   /* CRC 正确即算链路可用，与内容是否可用分开 */

                    if( av_sensor_decode( &f, &smp ) != 0 )
                    {
                        av_health_len_error();
                    }
                    else
                    {
                        av_verdict_t v = av_sensor_accept( &s_sensor, &smp, xTaskGetTickCount() );

                        if( v == AV_VERDICT_OK )
                        {
                            /* 连续有效帧计数由采样侧维护，状态机只读并清零。
                             * 跨任务的"读-改-写"必须进临界区：单核也不能假设 uint8 自增是原子的，
                             * 任务可能在取指与写回之间被抢占。 */
                            taskENTER_CRITICAL();
                            if( s_mode.good_streak < 0xFFU )
                            {
                                s_mode.good_streak++;
                            }
                            taskEXIT_CRITICAL();
                        }
                        else if( v == AV_VERDICT_RANGE )
                        {
                            av_health_mark( AV_F_RANGE );
                        }
                        else if( v == AV_VERDICT_RATE )
                        {
                            av_health_mark( AV_F_RATE );
                        }
                        else
                        {
                            av_health_mark( AV_F_SOURCE );
                        }
                    }
                }
            }
        }

        /* 心跳按"每轮循环"打点，而不是"每收到字节"：卡在队列上正是要被发现的故障 */
        if( ( xTaskGetTickCount() - last_kick ) >= pdMS_TO_TICKS( 50 ) )
        {
            av_health_kick();
            last_kick = xTaskGetTickCount();
        }
    }
}

/* ---- ctrl 任务：模式状态机 + 控制输出 -------------------------------------- */

static int32_t clamp_cmd( int32_t v )
{
    if( v > CMD_LIMIT_CDEG )
    {
        return CMD_LIMIT_CDEG;
    }

    if( v < -CMD_LIMIT_CDEG )
    {
        return -CMD_LIMIT_CDEG;
    }

    return v;
}

static void ctrl_task( void *arg )
{
    ( void ) arg;

    for( ;; )
    {
        vTaskDelay( pdMS_TO_TICKS( CTRL_PERIOD_MS ) );

        TickType_t now   = xTaskGetTickCount();
        uint32_t   flags = av_health_flags( &s_sensor, now );
        av_mode_t  before;

        taskENTER_CRITICAL();
        before = s_mode.mode;
        ( void ) av_mode_step( &s_mode, flags, now );
        taskEXIT_CRITICAL();

        if( s_mode.mode != before )
        {
            LOG( "[mode] t=%lu %s -> %s flags=0x%02lX cmd=%d air=%d pitch=%d\n",
                 ( unsigned long ) now,
                 av_mode_name( before ),
                 av_mode_name( s_mode.mode ),
                 ( unsigned long ) flags,
                 ( int ) s_cmd,
                 ( int ) s_sensor.last_good.airspeed_dmps,
                 ( int ) s_sensor.last_good.pitch_cdeg );
        }

        /* 故障边沿上报放在 ctrl（20 ms 分辨率）而不是 health（100 ms）：
         * 检测延迟的证据粒度不能比判定周期还粗。 */
        uint32_t rising  = flags & ~s_prev_flags;
        uint32_t falling = s_prev_flags & ~flags;

        if( rising != 0U || falling != 0U )
        {
            LOG( "[fault] t=%lu set=0x%02lX clear=0x%02lX (T=%d STALE=%d CRC=%d RNG=%d RATE=%d SRC=%d)\n",
                 ( unsigned long ) now,
                 ( unsigned long ) rising,
                 ( unsigned long ) falling,
                 ( int ) ( ( flags & AV_F_TIMEOUT ) != 0U ),
                 ( int ) ( ( flags & AV_F_STALE ) != 0U ),
                 ( int ) ( ( flags & AV_F_CRC ) != 0U ),
                 ( int ) ( ( flags & AV_F_RANGE ) != 0U ),
                 ( int ) ( ( flags & AV_F_RATE ) != 0U ),
                 ( int ) ( ( flags & AV_F_SOURCE ) != 0U ) );
            s_prev_flags = flags;
        }

        /* ---- 控制输出 ----
         * NORMAL：俯仰阻尼 + 空速保持（教学级系数，量纲均为 0.01°）
         * DEGRADED：保持上一拍指令（不跟随已判为不可信的数据）
         * SAFE：回中立位
         * 速率限幅对所有模式生效：模式切换本身不能成为一次阶跃。 */
        int32_t target;

        if( s_mode.mode == AV_MODE_NORMAL )
        {
            int32_t err = AIRSPEED_REF_DMPS - ( int32_t ) s_sensor.last_good.airspeed_dmps;
            target      = -( int32_t ) s_sensor.last_good.pitch_cdeg / 2 + ( err * 12 ) / 10;
            target      = clamp_cmd( target );
        }
        else if( s_mode.mode == AV_MODE_DEGRADED )
        {
            target = s_cmd;
        }
        else
        {
            target = 0;
        }

        int32_t delta = target - s_cmd;

        if( delta > CMD_STEP_CDEG )
        {
            delta = CMD_STEP_CDEG;
        }
        else if( delta < -CMD_STEP_CDEG )
        {
            delta = -CMD_STEP_CDEG;
        }

        s_cmd += delta;
    }
}

/* ---- health 任务：喂狗门控 + 上报 ------------------------------------------ */

/* 停喂原因是"两位独立条件"的组合，不能压成一个 bool：hb 超时（任务卡住）与
 * TIMEOUT/STALE（数据坏了）在 [stat] 里长得一样，只有分开才查得下去。 */
static const char *gate_reason_name( uint32_t reason )
{
    switch( reason )
    {
        case 0U:
            return "OK";
        case GATE_REASON_HB:
            return "HB";
        case GATE_REASON_FLAGS:
            return "FLAGS";
        default:
            return "HB+FLAGS";
    }
}

static void health_task( void *arg )
{
    TickType_t last_stat = 0U;
    const TickType_t health_period = pdMS_TO_TICKS( HEALTH_PERIOD_MS );

    /* -1 表示"还没打印过"，保证第一拍一定落一条 [gate]，便于事后核对初始态 */
    int      last_feed_ok = -1;
    uint32_t last_reason  = 0xFFFFFFFFU;

    ( void ) arg;

    for( ;; )
    {
        vTaskDelay( health_period );

        TickType_t now     = xTaskGetTickCount();
        uint32_t   flags   = av_health_flags( &s_sensor, now );
        uint32_t   rx_age  = av_health_rx_age( now );
        int        rx_live = ( rx_age <= pdMS_TO_TICKS( RX_HB_TIMEOUT_MS ) ) ? 1 : 0;

        /* 喂狗条件必须是"与"：任一通道不健康就整体停喂。
         * 若各任务各喂各的，卡死的任务就不会喂，但别的任务照样把狗喂饱——
         * 看门狗对卡死完全失效（见 F103_14）。 */
        int      feed_ok = ( rx_live != 0 ) && ( ( flags & ( AV_F_TIMEOUT | AV_F_STALE ) ) == 0U );
        uint32_t reason  = 0U;

        if( rx_live == 0 )
        {
            reason |= GATE_REASON_HB;
        }

        if( ( flags & ( AV_F_TIMEOUT | AV_F_STALE ) ) != 0U )
        {
            reason |= GATE_REASON_FLAGS;
        }

        if( feed_ok != 0 )
        {
            s_feeds++;
            watchdog_feed_if_enabled();
        }
        else
        {
            s_feed_blocked++;
        }

        /* 只在"门控状态或原因"变化时打印：100 ms 周期全打印会把日志淹掉，
         * 而边沿已足够还原"什么时候因为什么停喂"。 */
        if( ( feed_ok != last_feed_ok ) || ( reason != last_reason ) )
        {
            last_feed_ok = feed_ok;
            last_reason  = reason;
            LOG( "[gate] t=%lu feed=%s reason=%s hb=%lu flags=0x%02lX\n",
                 ( unsigned long ) now,
                 ( feed_ok != 0 ) ? "ON" : "OFF",
                 gate_reason_name( reason ),
                 ( unsigned long ) rx_age,
                 ( unsigned long ) flags );
        }

        /* 下行二进制状态帧：每轮 health（100 ms）发一帧，与 5000 ms 的 [stat] 分开——
         * 地面站要近实时状态，日志要低频快照。发送失败只在第一次报一行：
         * TXE 超时通常意味着 USART2 不通，刷屏只会把日志淹掉。 */
        {
            static int s_dl_warned = 0;

            if( av_downlink_status( now, s_mode.mode, flags, s_cmd,
                                    &s_sensor, av_health_stats() ) == 0 )
            {
                if( s_dl_warned == 0 )
                {
                    s_dl_warned = 1;
                    LOG( "[link] t=%lu USART2 下行 TXE 超时，帧被截断\n", ( unsigned long ) now );
                }
            }
        }

        if( ( now - last_stat ) >= pdMS_TO_TICKS( STAT_PERIOD_MS ) )
        {
            const av_health_stats_t *st = av_health_stats();

            last_stat = now;

            LOG( "[stat] t=%lu mode=%s cmd=%d flags=0x%02lX[T=%d STALE=%d CRC=%d RNG=%d RATE=%d SRC=%d] "
                 "air=%d pitch=%d age=%lu frames=%lu crc=%lu len=%lu type=%lu gap=%lu dup=%lu "
                 "hb=%lu feed=%s(%lu/%lu) heap=%u hwm=%u/%u/%u\n",
                 ( unsigned long ) now,
                 av_mode_name( s_mode.mode ),
                 ( int ) s_cmd,
                 ( unsigned long ) flags,
                 ( int ) ( ( flags & AV_F_TIMEOUT ) != 0U ),
                 ( int ) ( ( flags & AV_F_STALE ) != 0U ),
                 ( int ) ( ( flags & AV_F_CRC ) != 0U ),
                 ( int ) ( ( flags & AV_F_RANGE ) != 0U ),
                 ( int ) ( ( flags & AV_F_RATE ) != 0U ),
                 ( int ) ( ( flags & AV_F_SOURCE ) != 0U ),
                 ( int ) s_sensor.last_good.airspeed_dmps,
                 ( int ) s_sensor.last_good.pitch_cdeg,
                 ( unsigned long ) av_sensor_age( &s_sensor, now ),
                 ( unsigned long ) st->frames_ok,
                 ( unsigned long ) st->crc_errors,
                 ( unsigned long ) st->len_errors,
                 ( unsigned long ) st->type_errors,
                 ( unsigned long ) st->seq_gaps,
                 ( unsigned long ) st->duplicates,
                 ( unsigned long ) rx_age,
                 ( feed_ok != 0 ) ? "ON" : "OFF",
                 ( unsigned long ) s_feeds,
                 ( unsigned long ) s_feed_blocked,
                 ( unsigned ) xPortGetFreeHeapSize(),
                 ( unsigned ) uxTaskGetStackHighWaterMark( s_h_rx ),
                 ( unsigned ) uxTaskGetStackHighWaterMark( s_h_ctrl ),
                 ( unsigned ) uxTaskGetStackHighWaterMark( s_h_health ) );
        }
    }
}

/* ---- 钩子 ------------------------------------------------------------------ */

void vAssertCalled( const char *pcFile, unsigned long ulLine )
{
    taskDISABLE_INTERRUPTS();
    printf( "\r\n[ASSERT] %s:%lu\r\n", pcFile, ulLine );
    for( ;; )
    {
    }
}

void vApplicationMallocFailedHook( void )
{
    taskDISABLE_INTERRUPTS();
    printf( "\r\n[MALLOC FAILED] 堆耗尽，检查 configTOTAL_HEAP_SIZE\r\n" );
    for( ;; )
    {
    }
}

void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName )
{
    ( void ) xTask;
    taskDISABLE_INTERRUPTS();
    printf( "\r\n[STACK OVERFLOW] %s\r\n", pcTaskName );
    for( ;; )
    {
    }
}

/* ---- 启动 ------------------------------------------------------------------ */

int main( void )
{
    SystemCoreClockUpdate();
    f103_uart_init( UART_BAUD );

    /* CRC 自检：0x29B1 是 CRC16-CCITT-FALSE 对 "123456789" 的标准校验值。
     * 协议层算错 CRC 时，PC 与 MCU 会全帧互斥——这是那种"看起来什么都没发生"的故障，
     * 所以必须在启动时用一个已知答案把它钉住。 */
    static const uint8_t k_crc_probe[ 9 ] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
    uint16_t crc = av_crc16_ccitt_false( k_crc_probe, 9U );

    printf( "\r\n[av-node] 航空主题地面仿真节点（教学，非飞控）\r\n" );
    printf( "[av-node] CRC16-CCITT-FALSE(\"123456789\") = 0x%04X %s\n",
            ( unsigned ) crc, ( crc == 0x29B1U ) ? "PASS" : "FAIL" );

    if( crc != 0x29B1U )
    {
        printf( "[av-node] 协议层自检失败，拒绝启动调度器\n" );
        for( ;; )
        {
        }
    }

    printf( "[av-node] 帧: AA55 TYPE SEQ LEN PAYLOAD CRC16（小端，CRC 覆盖 TYPE..PAYLOAD）\n" );
    printf( "[av-node] 判据: 链路超时 %u ms / 数据陈旧 %u ms / CRC 连续 %u 帧 / 故障保持 %u ms\n",
            ( unsigned ) AV_LINK_TIMEOUT_MS, ( unsigned ) AV_DATA_TIMEOUT_MS,
            ( unsigned ) AV_CRC_STREAK_MAX, ( unsigned ) AV_FAULT_HOLD_MS );
    printf( "[av-node] 模式: INIT→NORMAL(%u 帧) / 故障→DEGRADED / 持续 %u ms→SAFE / SAFE→DEGRADED(%u 帧)\n",
            ( unsigned ) AV_GOOD_TO_NORMAL, ( unsigned ) AV_DEGRADED_TIMEOUT_MS,
            ( unsigned ) AV_SAFE_RECOVER_GOOD );
    printf( "[av-node] IWDG 路径 未启用（QEMU 未建模）；代码地址 init=0x%08lX feed=0x%08lX\n",
            ( unsigned long ) f103_watchdog_init, ( unsigned long ) f103_watchdog_feed );

    /* 下行单独走 USART2（PA2）：USART1 上的 ASCII 日志同时是注入输入口，
     * 二进制帧混进去会破坏行解析与 expect.py 的判定。PCLK1 = HCLK（PPRE1 未分频）。 */
    if( av_downlink_init( SystemCoreClock, UART_BAUD ) != 0 )
    {
        printf( "[av-node] 下行 USART2 已启用：TYPE=0x%02X LEN=%u baud=%lu（PCLK1=%lu Hz 假设）\n",
                ( unsigned ) AV_DL_TYPE_STATUS, ( unsigned ) AV_DL_STATUS_LEN,
                ( unsigned long ) UART_BAUD, ( unsigned long ) SystemCoreClock );
    }
    else
    {
        printf( "[av-node] 下行 USART2 初始化失败：baud=0\n" );
    }

    s_rx_queue  = xQueueCreate( RX_QUEUE_DEPTH, sizeof( uint8_t ) );
    s_log_mutex = xSemaphoreCreateMutex();

    if( ( s_rx_queue == NULL ) || ( s_log_mutex == NULL ) )
    {
        printf( "[av-node] 创建队列/互斥量失败：heap 不足\n" );
        for( ;; )
        {
        }
    }

    av_sensor_init( &s_sensor );
    av_health_init();
    av_mode_init( &s_mode, xTaskGetTickCount() );

    xTaskCreate( rx_task,     "rx",     176, NULL, 3, &s_h_rx );
    xTaskCreate( ctrl_task,   "ctrl",   224, NULL, 4, &s_h_ctrl );
    xTaskCreate( health_task, "health", 224, NULL, 2, &s_h_health );

    printf( "[av-node] 启动调度器，初始空闲堆 %u B\n", ( unsigned ) xPortGetFreeHeapSize() );

    vTaskStartScheduler();

    printf( "[av-node] 调度器未能启动（堆不足？）\n" );
    for( ;; )
    {
    }
}
