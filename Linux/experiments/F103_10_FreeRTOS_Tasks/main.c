/*
 * F103_10：FreeRTOS 最小可运行系统（任务 / 队列 / 互斥量）
 *
 * 目标：把"调度器真的在切换任务"变成可观察的串口输出，而不是靠灯闪猜。
 *
 * 三个任务：
 *   producer（优先级 2）每 200 ms 造一条样本，投进深度 8 的队列
 *   consumer（优先级 3）阻塞等队列，收到就打印
 *   monitor （优先级 1）每 1000 ms 打印一次计数与队列水位
 *
 * 两个刻意的设计：
 *   1. consumer 优先级高于 producer —— 一旦 producer 投递成功，
 *      consumer 立刻抢占执行，这条抢占关系可以用时间戳核对。
 *   2. 两个任务都要 printf，所以用互斥量把整行输出包起来；
 *      否则抢占会把一行拆成两半。互斥量还顺带演示优先级继承。
 *
 * SysTick / SVCall / PendSV 由 FreeRTOS 接管，见 freertos_vectors.s。
 */
#include <stdint.h>
#include <stdio.h>

#include "stm32f1xx.h"
#include "f103_uart.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#define UART_BAUD        115200U
#define SAMPLE_PERIOD_MS 200U
#define REPORT_PERIOD_MS 1000U
#define QUEUE_DEPTH      8U

typedef struct {
    uint32_t seq;
    TickType_t tick;
} sample_t;

static QueueHandle_t     s_queue;
static SemaphoreHandle_t s_log_mutex;
static TaskHandle_t      s_h_consumer;
static TaskHandle_t      s_h_producer;
static TaskHandle_t      s_h_monitor;

static volatile uint32_t s_produced;
static volatile uint32_t s_consumed;
static volatile uint32_t s_dropped;

/* 整行输出必须串起来：printf 本身不是原子的，抢占会把它拆成两行碎片 */
#define LOG( ... )                                                       \
    do {                                                                 \
        if( xSemaphoreTake( s_log_mutex, portMAX_DELAY ) == pdTRUE ) {    \
            printf( __VA_ARGS__ );                                       \
            xSemaphoreGive( s_log_mutex );                               \
        }                                                                \
    } while( 0 )

static void producer_task( void *arg )
{
    ( void ) arg;
    uint32_t seq = 0U;

    for( ;; )
    {
        sample_t s;

        s.seq  = ++seq;
        s.tick = xTaskGetTickCount();

        /* 队列满时最多等 50 ms：宁可丢一条并计数，也不让采集任务无休止地堵住 */
        if( xQueueSend( s_queue, &s, pdMS_TO_TICKS( 50U ) ) != pdPASS )
        {
            s_dropped++;
        }
        else
        {
            s_produced++;
        }

        vTaskDelay( pdMS_TO_TICKS( SAMPLE_PERIOD_MS ) );
    }
}

static void consumer_task( void *arg )
{
    ( void ) arg;
    sample_t s;

    for( ;; )
    {
        if( xQueueReceive( s_queue, &s, portMAX_DELAY ) == pdPASS )
        {
            s_consumed++;
            LOG( "[consume] seq=%lu tick=%lu\n",
                 ( unsigned long ) s.seq, ( unsigned long ) s.tick );
        }
    }
}

static void monitor_task( void *arg )
{
    ( void ) arg;

    for( ;; )
    {
        vTaskDelay( pdMS_TO_TICKS( REPORT_PERIOD_MS ) );
        LOG( "[monitor] tick=%lu produced=%lu consumed=%lu dropped=%lu queued=%lu\n",
             ( unsigned long ) xTaskGetTickCount(),
             ( unsigned long ) s_produced,
             ( unsigned long ) s_consumed,
             ( unsigned long ) s_dropped,
             ( unsigned long ) uxQueueMessagesWaiting( s_queue ) );

        /* 剩余栈（单位：字，不是字节）与空闲堆：判断 RAM 预算够不够的唯一凭据，
         * 只看 RAM 占用率看不出"哪个任务快把栈用穿了"。 */
        LOG( "[res] heap_free=%u B, stack_hwm consume=%u produce=%u monitor=%u words\n",
             ( unsigned ) xPortGetFreeHeapSize(),
             ( unsigned ) uxTaskGetStackHighWaterMark( s_h_consumer ),
             ( unsigned ) uxTaskGetStackHighWaterMark( s_h_producer ),
             ( unsigned ) uxTaskGetStackHighWaterMark( s_h_monitor ) );
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

    printf( "\r\n[F103_10] FreeRTOS %s on STM32F103, HCLK=%lu Hz, tick=%lu Hz\r\n",
            tskKERNEL_VERSION_NUMBER,
            ( unsigned long ) SystemCoreClock,
            ( unsigned long ) configTICK_RATE_HZ );

    s_queue     = xQueueCreate( QUEUE_DEPTH, sizeof( sample_t ) );
    s_log_mutex = xSemaphoreCreateMutex();

    if( ( s_queue == NULL ) || ( s_log_mutex == NULL ) )
    {
        printf( "创建队列/互斥量失败：heap 不足\r\n" );
        for( ;; )
        {
        }
    }

    /* 先建 consumer，让它进入阻塞态；producer 一投递就会触发抢占 */
    xTaskCreate( consumer_task, "consume", 256, NULL, 3, &s_h_consumer );
    xTaskCreate( producer_task, "produce", 160, NULL, 2, &s_h_producer );
    xTaskCreate( monitor_task,  "monitor", 192, NULL, 1, &s_h_monitor );

    printf( "任务已创建，启动调度器（初始空闲堆 %u B）\r\n",
            ( unsigned ) xPortGetFreeHeapSize() );

    vTaskStartScheduler();

    /* 正常情况下不会执行到这里：调度器启动失败才会返回 */
    printf( "调度器未能启动（堆不足？）\r\n" );
    for( ;; )
    {
    }
}
