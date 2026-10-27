/*
 * F103_11：中断 → 任务通信（USART1 RXNE + FreeRTOS 队列）
 *
 * 目标：把"中断里能做什么、不能做什么"变成可核对的输出。
 *
 * 结构：
 *   USART1_IRQHandler (ISR)  读 SR/DR → xQueueSendFromISR → portYIELD_FROM_ISR
 *   consume  (优先级 3)      xQueueReceive(portMAX_DELAY) 阻塞等，按行输出
 *   monitor  (优先级 1)      每 1000 ms 打印统计与"ISR 投递 → 任务取走"的最大延迟
 *
 * 三个必答问题（都在代码注释里给了答案）：
 *   1. 为什么 ISR 里不能直接 printf？          → 见 USART1_IRQHandler 注释
 *   2. 为什么用 xQueueSendFromISR 而非 xQueueSend？→ 见同上
 *   3. 为什么 NVIC 优先级必须是 5 或更大？      → 见 USART1_IRQ_PRIO
 *
 * 与实验 10 的区别：实验 10 的任务间通信由任务自己发起；这里发起者是中断，
 * 涉及"是否需要在中断退出时立刻切换"这个只有 ISR 才知道的决定。
 */
#include <stdint.h>
#include <stdio.h>

#include "stm32f1xx.h"
#include "f103_uart.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#define UART_BAUD         115200U
#define RX_QUEUE_DEPTH    32U
#define LINE_MAX          64U
#define REPORT_PERIOD_MS  1000U

/*
 * 中断优先级：数值 5 == configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY。
 *
 * FreeRTOS 的临界区靠 BASEPRI 屏蔽中断，只屏蔽"优先级数值 ≥ 5"的中断。
 * 若把 USART1 配成 0～4（更高优先级），它不会被临界区屏蔽，于是可能在
 * 内核正在改队列链表的中途抢占进来，再调用 xQueueSendFromISR 破坏数据结构。
 * 规则：**能调用 FreeRTOS API 的中断，优先级数值必须 ≥ configMAX_SYSCALL_INTERRUPT_PRIORITY**。
 * 反过来说，真正要求低延迟、又不调用任何 API 的中断才允许用 0～4。
 */
#define USART1_IRQ_PRIO   5U

typedef struct {
    uint8_t    ch;
    TickType_t tick;   /* ISR 投递时刻，用于量"中断 → 任务"的延迟 */
} rx_item_t;

static QueueHandle_t     s_rx_queue;
static SemaphoreHandle_t s_log_mutex;

/* ISR 侧计数（32 位对齐单次读写，M3 上无需关中断即可被任务读取） */
static volatile uint32_t s_rx_bytes;    /* 收到的字节数 */
static volatile uint32_t s_queue_drop;  /* 队列满，ISR 主动丢弃 */
static volatile uint32_t s_ore;         /* 硬件溢出：DR 被覆盖 */

/* 任务侧计数 */
static volatile uint32_t s_lines;       /* 完整行数 */
static volatile uint32_t s_truncated;   /* 超长行被丢弃 */
static volatile uint32_t s_lat_max;     /* ISR 投递 → 任务取走 的最大 tick 差 */

#define LOG( ... )                                                       \
    do {                                                                 \
        if( xSemaphoreTake( s_log_mutex, portMAX_DELAY ) == pdTRUE ) {    \
            printf( __VA_ARGS__ );                                       \
            xSemaphoreGive( s_log_mutex );                               \
        }                                                                \
    } while( 0 )

/*
 * IRQ 37。ISR 只能做三件事：清硬件标志、把数据搬进队列、决定是否立刻切换。
 *
 * 不能 printf：f103_uart_putc 是轮询发送，一个字符在 115200 下约 87 µs，
 *   一行 40 字符 ≈ 3.5 ms，期间其他中断全被推迟——RTOS 的时间约束立刻崩掉。
 * 不能用 xQueueSend：队列满时它会按 tick 阻塞，而中断上下文没有"任务"可以挂起。
 * 不能用带 INCLUDE 开关之外的任务 API：同理，任何可能导致阻塞/suspend 的调用都非法。
 */
void USART1_IRQHandler( void )
{
    BaseType_t higher_prio_woken = pdFALSE;
    uint32_t   sr                = USART1->SR;

    if( ( sr & ( USART_SR_RXNE | USART_SR_ORE ) ) == 0U )
    {
        return;
    }

    uint8_t ch = ( uint8_t ) USART1->DR;   /* 读 DR 同时清 RXNE 与 ORE */

    if( ( sr & USART_SR_ORE ) != 0U )
    {
        s_ore++;
    }
    else
    {
        rx_item_t item;

        s_rx_bytes++;
        item.ch   = ch;
        item.tick = xTaskGetTickCountFromISR();   /* ISR 里只能读这个版本 */

        if( xQueueSendFromISR( s_rx_queue, &item, &higher_prio_woken ) != pdPASS )
        {
            s_queue_drop++;   /* 队列满：丢弃并计数，绝不阻塞 */
        }
    }

    /*
     * 只有 ISR 知道"这次投递唤醒了谁、它是否比当前任务优先级高"。
     * 传 pdTRUE 时立即置 PendSV，中断退出后直接切到消费任务；
     * 传 pdFALSE 则等下一个 tick 才轮到它——延迟量级差 0～1 ms，本实验用 s_lat_max 观测。
     * 宏在 ARM_CM3 上可能展开为空语句，所以必须带分号。
     */
    portYIELD_FROM_ISR( higher_prio_woken );
}

static void consumer_task( void *arg )
{
    char   line[ LINE_MAX ];
    size_t len = 0U;

    ( void ) arg;

    /*
     * 中断使能放在任务里而不是 main() 里：此刻调度器已在运行。
     * 若在 vTaskStartScheduler() 之前使能，而恰好此时来了一个字节，
     * ISR 里的 portYIELD_FROM_ISR 会挂起 PendSV，但内核还没有当前任务
     * （pxCurrentTCB 为空），切换会失败（分析结论，未在硬件上复现）。
     */
    USART1->CR1 |= USART_CR1_RXNEIE;
    NVIC_SetPriority( USART1_IRQn, USART1_IRQ_PRIO );
    NVIC_EnableIRQ( USART1_IRQn );

    for( ;; )
    {
        rx_item_t item;

        if( xQueueReceive( s_rx_queue, &item, portMAX_DELAY ) != pdPASS )
        {
            continue;
        }

        TickType_t lat = xTaskGetTickCount() - item.tick;

        if( lat > s_lat_max )
        {
            s_lat_max = lat;
        }

        if( item.ch == '\r' )
        {
            continue;
        }

        if( item.ch == '\n' )
        {
            line[ len ] = '\0';
            s_lines++;
            LOG( "[line %lu] \"%s\" len=%u\r\n",
                 ( unsigned long ) s_lines, line, ( unsigned ) len );
            len = 0U;
            continue;
        }

        if( len < ( LINE_MAX - 1U ) )
        {
            line[ len++ ] = ( char ) item.ch;
        }
        else
        {
            /* 超长行：丢弃已累计的部分并重新开始，避免越界 */
            s_truncated++;
            len = 0U;
        }
    }
}

static void monitor_task( void *arg )
{
    ( void ) arg;

    for( ;; )
    {
        vTaskDelay( pdMS_TO_TICKS( REPORT_PERIOD_MS ) );
        LOG( "[stats] tick=%lu rx=%lu lines=%lu qdrop=%lu ore=%lu trunc=%lu "
             "queued=%lu lat_max=%lu heap_free=%u\r\n",
             ( unsigned long ) xTaskGetTickCount(),
             ( unsigned long ) s_rx_bytes,
             ( unsigned long ) s_lines,
             ( unsigned long ) s_queue_drop,
             ( unsigned long ) s_ore,
             ( unsigned long ) s_truncated,
             ( unsigned long ) uxQueueMessagesWaiting( s_rx_queue ),
             ( unsigned long ) s_lat_max,
             ( unsigned ) xPortGetFreeHeapSize() );
    }
}

/* ---- 钩子 ---------------------------------------------------------------- */

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

int main( void )
{
    SystemCoreClockUpdate();
    f103_uart_init( UART_BAUD );

    printf( "\r\n[F103_11] FreeRTOS ISR→task: USART1 RXNE + queue(depth=%u), "
            "NVIC prio=%u (= MAX_SYSCALL)\r\n",
            ( unsigned ) RX_QUEUE_DEPTH, ( unsigned ) USART1_IRQ_PRIO );

    s_rx_queue  = xQueueCreate( RX_QUEUE_DEPTH, sizeof( rx_item_t ) );
    s_log_mutex = xSemaphoreCreateMutex();

    if( ( s_rx_queue == NULL ) || ( s_log_mutex == NULL ) )
    {
        printf( "创建队列/互斥量失败：heap 不足\r\n" );
        for( ;; )
        {
        }
    }

    xTaskCreate( consumer_task, "consume", 256, NULL, 3, NULL );
    xTaskCreate( monitor_task,  "monitor", 192, NULL, 1, NULL );

    printf( "任务已创建，启动调度器（RXNE 中断由 consume 任务使能）\r\n" );

    vTaskStartScheduler();

    printf( "调度器未能启动（堆不足？）\r\n" );
    for( ;; )
    {
    }
}
