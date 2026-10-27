/*
 * F103_13：FreeRTOS 软件定时器（单次 / 周期 / 到期漂移 / 回调不得阻塞）
 *
 * 目标：把"定时器回调在哪个上下文里跑、为什么它不能慢"变成可读取的数字。
 *
 * 三个任务：
 *   app     （优先级 2）创建并操作定时器，打印每次到期的实际 tick 与漂移
 *   monitor （优先级 1）每 500 ms 打印堆余量与三个任务（含守护任务）的栈水位
 *   守护任务（优先级 4，优先于 app）由 configUSE_TIMERS = 1 在启动调度器时创建，
 *           所有定时器命令和到期回调都在它的上下文里执行
 *
 * 三个定时器：
 *   once      300 ms 单次：演示 xTimerCreate 之后不会自动启动，以及 xTimerReset 复用
 *   periodic  200 ms 周期：演示自动重装与"回调阻塞不改变自己的相位"
 *   fast       50 ms 周期：轻回调，用来观测"守护任务被别人的回调占住"的代价
 *
 * 两处刻意设计：
 *   1. 定时器创建后是"休眠"状态（xTimerCreate 不自动启动），先等一个周期再 start，
 *      否则"创建即启动"与"手工 start"两种语义分不清。
 *   2. 在第 5 次周期到期后注入 60 tick 的忙等，模拟"回调里做了重活"。
 *      60 > 50 是刻意的：必须让 fast 的某个到期点落在阻塞窗口内，
 *      否则这段注入不会产生任何可读的延迟（第一次实测就是这样失效的）。
 *
 * 时间单位：本文件里的 ms 都是"固件毫秒"（1 tick = 1 ms 假设）；
 * QEMU 的 SYSCLK 是 24 MHz 而固件按 8 MHz 计算，实测固件时间比真实时间快 2.90 倍，
 * 所以本实验的绝对值只用于比较比例，不能当实时性结论。
 */
#include <stdint.h>
#include <stdio.h>

#include "stm32f1xx.h"
#include "f103_uart.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"

#define UART_BAUD            115200U
#define ONCE_PERIOD_MS       300U
#define PERIOD_PERIOD_MS     200U
#define FAST_PERIOD_MS       50U      /* 第二个周期定时器：用来量化"守护任务被占住"的代价 */
#define MONITOR_PERIOD_MS    500U
#define STALL_TICKS          60U     /* 注入的回调占用：必须 > FAST_PERIOD_MS，
                                      * 才能保证 fast 的某个到期点落在阻塞窗口内 */
#define FIRE_LOG_MAX         24U     /* 记录前 N 次 200 ms 到期，够覆盖全部演示段 */
#define FAST_LOG_MAX         48U     /* 50 ms 定时器在 2 s 内约 40 次，留余量 */

static SemaphoreHandle_t s_log_mutex;
static TimerHandle_t     s_once;
static TimerHandle_t     s_periodic;
static TimerHandle_t     s_fast;
static TaskHandle_t      s_h_app;
static TaskHandle_t      s_h_monitor;

/* 回调里只做计数与取 tick：任何 printf、阻塞、动态分配都不允许出现在回调里。
 * 用 volatile 是因为这些变量由守护任务写、由 app 任务读。 */
static volatile uint32_t s_once_cnt;
static volatile uint32_t s_once_act;      /* 单次定时器到期的实际 tick */
static volatile uint32_t s_in_daemon;     /* 回调里自证上下文：1 = 跑在守护任务里 */

static volatile uint32_t s_per_cnt;
static volatile uint32_t s_per_act[ FIRE_LOG_MAX ];   /* 每次周期到期的实际 tick */
static volatile uint32_t s_per_dur[ FIRE_LOG_MAX ];   /* 回调自身耗时（tick） */
static volatile uint32_t s_stall;         /* >0 时周期回调按 tick 忙等，模拟超时回调 */

/* 50 ms 定时器：它的到期点会落在"周期回调阻塞"的窗口内，
 * 于是"守护任务被占住 → 别的定时器被推迟"第一次变成可读的数字。 */
static volatile uint32_t s_fast_cnt;
static volatile uint32_t s_fast_act[ FAST_LOG_MAX ];

#define LOG( ... )                                                       \
    do {                                                                 \
        if( xSemaphoreTake( s_log_mutex, portMAX_DELAY ) == pdTRUE ) {    \
            printf( __VA_ARGS__ );                                       \
            xSemaphoreGive( s_log_mutex );                               \
        }                                                                \
    } while( 0 )

/* ---- 回调：只记账，不做别的 ---------------------------------------------- */

static void cb_once( TimerHandle_t t )
{
    ( void ) t;

    s_once_act = ( uint32_t ) xTaskGetTickCount();
    s_once_cnt++;

    /* 定时器回调一定跑在守护任务里——这里用句柄比对证明，而不是照抄文档。
     * xTimerGetTimerDaemonTaskHandle() 只在 configUSE_TIMERS == 1 时存在。 */
    if( xTaskGetCurrentTaskHandle() == xTimerGetTimerDaemonTaskHandle() )
    {
        s_in_daemon = 1U;
    }
}

static void cb_periodic( TimerHandle_t t )
{
    ( void ) t;

    uint32_t idx  = s_per_cnt % FIRE_LOG_MAX;
    uint32_t t0   = ( uint32_t ) xTaskGetTickCount();
    uint32_t spin = s_stall;

    /* 故障注入：占住守护任务 60 tick。这会让**其它**定时器的到期处理被推迟，
     * 但不会推迟本定时器的下一次到期（重装已在本回调之前完成）。
     * **生产代码里绝不能在回调里这样等。** */
    if( spin > 0U )
    {
        while( ( uint32_t ) ( xTaskGetTickCount() - t0 ) < spin )
        {
        }
    }

    s_per_act[ idx ] = t0;
    s_per_dur[ idx ] = ( uint32_t ) ( xTaskGetTickCount() - t0 );
    s_per_cnt++;
}

/* 轻回调：只记 tick。它的漂移反映"守护任务有没有被别人的回调占住"。
 *
 * 实测语义（两个都是反直觉的，不要凭直觉断言）：
 *   1. 忙等发生在回调里，但 200 ms 定时器自己的下一次到期仍然准时——因为 FreeRTOS
 *      在**调用回调之前**就完成了自动重装，所以回调阻塞不改变本定时器的相位。
 *   2. fast 定时器在阻塞窗口内到期的两拍，会被压到同一 tick 连续执行（实测 #20/#21
 *      均为 1067），随后因为重装基准是**到期时刻**而不是执行时刻，下一拍就回到
 *      理想网格（#22 drift 从 61 回到 1）。所以代价是"丢节拍"，不是"永久偏移"。
 */
static void cb_fast( TimerHandle_t t )
{
    ( void ) t;

    if( s_fast_cnt < FAST_LOG_MAX )
    {
        s_fast_act[ s_fast_cnt ] = ( uint32_t ) xTaskGetTickCount();
    }

    s_fast_cnt++;
}

/* ---- 工具 ---------------------------------------------------------------- */

/* 等某个计数达到 target；超时返回 pdFALSE，避免演示脚本卡死。 */
static BaseType_t wait_cnt( volatile uint32_t *pcnt, uint32_t target, uint32_t timeout_ms )
{
    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS( timeout_ms );

    while( *pcnt < target )
    {
        if( xTaskGetTickCount() >= deadline )
        {
            return pdFALSE;
        }

        vTaskDelay( pdMS_TO_TICKS( 10U ) );
    }

    return pdTRUE;
}

/* 打印周期定时器每一期的漂移：实际 tick − 理想 tick（base + n × period）。
 * 漂移为正说明回调被推迟执行；注意"被推迟"不等于"永久偏移"，
 * 因为自动重装以到期时刻为基准，阻塞结束后会自己追平（见 cb_fast 的注释）。 */
static void report_periodic( const char *tag, uint32_t base, uint32_t from, uint32_t to )
{
    for( uint32_t k = from; k < to && k < FIRE_LOG_MAX; k++ )
    {
        uint32_t ideal = base + ( k + 1U ) * PERIOD_PERIOD_MS;
        LOG( "[tmr] %s period#%lu act=%lu ideal=%lu drift=%ld dur=%lu\n",
             tag,
             ( unsigned long ) ( k + 1U ),
             ( unsigned long ) s_per_act[ k ],
             ( unsigned long ) ideal,
             ( long ) ( s_per_act[ k ] - ideal ),
             ( unsigned long ) s_per_dur[ k ] );
    }
}

/* 只打印"漂移发生变化"的那些点：44 次到期里绝大多数漂移恒定，
 * 逐个打印会淹没真正有信息的两三行。 */
static void report_fast( const char *tag, uint32_t base )
{
    uint32_t n = ( s_fast_cnt < FAST_LOG_MAX ) ? s_fast_cnt : FAST_LOG_MAX;
    long     prev = 0;
    long     max  = 0;

    for( uint32_t k = 0; k < n; k++ )
    {
        uint32_t ideal = base + ( k + 1U ) * FAST_PERIOD_MS;
        long     drift = ( long ) ( s_fast_act[ k ] - ideal );

        if( drift > max )
        {
            max = drift;
        }

        if( drift != prev )
        {
            LOG( "[tmr] %s fast#%lu act=%lu ideal=%lu drift=%ld\n",
                 tag, ( unsigned long ) ( k + 1U ),
                 ( unsigned long ) s_fast_act[ k ], ( unsigned long ) ideal, drift );
            prev = drift;
        }
    }

    LOG( "[tmr] %s fast_total=%lu max_drift=%ld\n",
         tag, ( unsigned long ) s_fast_cnt, max );
}

/* ---- 任务 ---------------------------------------------------------------- */

static void app_task( void *arg )
{
    ( void ) arg;

    /* 1) 创建：xTimerCreate 之后定时器处于休眠状态，不会自己到期 */
    unsigned heap_before = ( unsigned ) xPortGetFreeHeapSize();

    s_once     = xTimerCreate( "once", pdMS_TO_TICKS( ONCE_PERIOD_MS ), pdFALSE, NULL, cb_once );
    s_periodic = xTimerCreate( "periodic", pdMS_TO_TICKS( PERIOD_PERIOD_MS ), pdTRUE, NULL, cb_periodic );
    s_fast     = xTimerCreate( "fast", pdMS_TO_TICKS( FAST_PERIOD_MS ), pdTRUE, NULL, cb_fast );

    if( ( s_once == NULL ) || ( s_periodic == NULL ) || ( s_fast == NULL ) )
    {
        LOG( "[tmr] 创建定时器失败：堆不足\n" );
        for( ;; )
        {
            vTaskDelay( pdMS_TO_TICKS( 1000U ) );
        }
    }

    /* 三个 Timer_t 的实际堆开销：这是唯一能直接测到的对象成本，
     * 守护任务与命令队列的成本在调度器启动时才产生（见 main() 打印的启动前余量）。 */
    LOG( "[tmr] 三个 Timer_t 用掉 %u B（堆余量 %u → %u）\n",
         heap_before - ( unsigned ) xPortGetFreeHeapSize(),
         heap_before, ( unsigned ) xPortGetFreeHeapSize() );

    LOG( "[tmr] 三个定时器已创建（休眠状态）：once=%u ms 单次，periodic=%u ms，fast=%u ms 周期，daemon 优先级=%u\n",
         ( unsigned ) ONCE_PERIOD_MS, ( unsigned ) PERIOD_PERIOD_MS,
         ( unsigned ) FAST_PERIOD_MS, ( unsigned ) configTIMER_TASK_PRIORITY );

    /* 2) 启动：命令先进队列、由守护任务取出执行，所以"启动时刻"取调用前的 tick */
    uint32_t base_per  = ( uint32_t ) xTaskGetTickCount();
    uint32_t base_once = base_per;
    uint32_t base_fast = base_per;

    if( xTimerStart( s_once, pdMS_TO_TICKS( 100U ) ) != pdPASS )
    {
        LOG( "[tmr] xTimerStart(once) 失败：命令队列满\n" );
    }

    if( xTimerStart( s_periodic, pdMS_TO_TICKS( 100U ) ) != pdPASS )
    {
        LOG( "[tmr] xTimerStart(periodic) 失败：命令队列满\n" );
    }

    if( xTimerStart( s_fast, pdMS_TO_TICKS( 100U ) ) != pdPASS )
    {
        LOG( "[tmr] xTimerStart(fast) 失败：命令队列满\n" );
    }

    if( wait_cnt( &s_once_cnt, 1U, 1500U ) == pdTRUE )
    {
        LOG( "[tmr] once fire#1 act=%lu ideal=%lu drift=%ld in_daemon=%lu\n",
             ( unsigned long ) s_once_act,
             ( unsigned long ) ( base_once + ONCE_PERIOD_MS ),
             ( long ) ( s_once_act - ( base_once + ONCE_PERIOD_MS ) ),
             ( unsigned long ) s_in_daemon );
    }
    else
    {
        LOG( "[tmr] once 未在 1500 ms 内到期\n" );
    }

    /* 3) 单次定时器到期后自动处于休眠状态；xTimerReset 让它重新计时。
     *    这是"单次"与"周期"最实用的差别：单次要靠软件再次触发。 */
    base_once = ( uint32_t ) xTaskGetTickCount();

    if( xTimerReset( s_once, pdMS_TO_TICKS( 100U ) ) != pdPASS )
    {
        LOG( "[tmr] xTimerReset(once) 失败\n" );
    }

    if( wait_cnt( &s_once_cnt, 2U, 1500U ) == pdTRUE )
    {
        LOG( "[tmr] once fire#2 act=%lu ideal=%lu drift=%ld（reset 生效，单次定时器可复用）\n",
             ( unsigned long ) s_once_act,
             ( unsigned long ) ( base_once + ONCE_PERIOD_MS ),
             ( long ) ( s_once_act - ( base_once + ONCE_PERIOD_MS ) ) );
    }

    /* 4) 前 4 期：回调只记账，漂移应接近 0 */
    if( wait_cnt( &s_per_cnt, 4U, 2000U ) == pdTRUE )
    {
        report_periodic( "normal", base_per, 0U, 4U );
    }

    report_fast( "normal", base_fast );

    /* 5) 注入 60 tick 的回调占用，观察"守护任务被占住"的代价。
     *    STALL_TICKS > FAST_PERIOD_MS 是关键：必须让 fast 的某个到期点落在阻塞窗口内，
     *    否则整段注入不会产生任何可见延迟。 */
    s_stall = STALL_TICKS;
    LOG( "[tmr] 注入：周期回调忙等 %u tick（模拟回调里做重活）\n", ( unsigned ) STALL_TICKS );

    if( wait_cnt( &s_per_cnt, 8U, 3000U ) == pdTRUE )
    {
        report_periodic( "stalled", base_per, 4U, 8U );
    }

    s_stall = 0U;
    LOG( "[tmr] 注入结束：回调恢复只记账\n" );

    if( wait_cnt( &s_per_cnt, 10U, 2000U ) == pdTRUE )
    {
        report_periodic( "after", base_per, 8U, 10U );
    }

    report_fast( "after", base_fast );

    /* 先停 fast：后面 600 ms 的"停止验证"期间它还会继续到期，
     * 不先停住的话 fast_total 会多出十几次，干扰对漂移的读取。 */
    if( xTimerStop( s_fast, pdMS_TO_TICKS( 100U ) ) != pdPASS )
    {
        LOG( "[tmr] xTimerStop(fast) 失败\n" );
    }

    /* 6) 停止：停止后计数必须不再增长，这是"停住了"的唯一证据 */
    uint32_t frozen         = s_per_cnt;
    uint32_t frozen_fast    = s_fast_cnt;

    if( xTimerStop( s_periodic, pdMS_TO_TICKS( 100U ) ) != pdPASS )
    {
        LOG( "[tmr] xTimerStop 失败\n" );
    }

    vTaskDelay( pdMS_TO_TICKS( 600U ) );
    LOG( "[tmr] 已停止：periodic 前=%lu 后=%lu 差=%ld；fast 前=%lu 后=%lu 差=%ld（都应为 0）\n",
         ( unsigned long ) frozen, ( unsigned long ) s_per_cnt,
         ( long ) ( s_per_cnt - frozen ),
         ( unsigned long ) frozen_fast, ( unsigned long ) s_fast_cnt,
         ( long ) ( s_fast_cnt - frozen_fast ) );

    LOG( "[tmr] done heap_free=%u B\n", ( unsigned ) xPortGetFreeHeapSize() );

    for( ;; )
    {
        vTaskDelay( pdMS_TO_TICKS( 1000U ) );
    }
}

static void monitor_task( void *arg )
{
    ( void ) arg;

    for( ;; )
    {
        vTaskDelay( pdMS_TO_TICKS( MONITOR_PERIOD_MS ) );
        /* 守护任务的句柄由内核在 vTaskStartScheduler 里创建，这里每次现取；
         * 它承担了全部回调执行，栈不够时表现为 configASSERT 或 Hook，不是静默错误。 */
        LOG( "[res] heap_free=%u B, stack_hwm app=%u monitor=%u daemon=%u words\n",
             ( unsigned ) xPortGetFreeHeapSize(),
             ( unsigned ) uxTaskGetStackHighWaterMark( s_h_app ),
             ( unsigned ) uxTaskGetStackHighWaterMark( s_h_monitor ),
             ( unsigned ) uxTaskGetStackHighWaterMark( xTimerGetTimerDaemonTaskHandle() ) );
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

/* ---- 入口 ---------------------------------------------------------------- */

int main( void )
{
    SystemCoreClockUpdate();
    f103_uart_init( UART_BAUD );

    printf( "\r\n[F103_13] FreeRTOS %s software timers, HCLK=%lu Hz, tick=%lu Hz\r\n",
            tskKERNEL_VERSION_NUMBER,
            ( unsigned long ) SystemCoreClock,
            ( unsigned long ) configTICK_RATE_HZ );
    printf( "[F103_13] configUSE_TIMERS=%d daemon prio=%u queue=%u stack=%u words\r\n",
            ( int ) configUSE_TIMERS,
            ( unsigned ) configTIMER_TASK_PRIORITY,
            ( unsigned ) configTIMER_QUEUE_LENGTH,
            ( unsigned ) configTIMER_TASK_STACK_DEPTH );

    s_log_mutex = xSemaphoreCreateMutex();
    if( s_log_mutex == NULL )
    {
        printf( "创建互斥量失败：heap 不足\r\n" );
        for( ;; )
        {
        }
    }

    printf( "[F103_13] 互斥量建好后堆余量 %u B\r\n", ( unsigned ) xPortGetFreeHeapSize() );

    xTaskCreate( app_task,     "app",     192, NULL, 2, &s_h_app );
    /* monitor 用 160 字而非 128：实测 128 字时水位只剩 26 字，低于"绝对 ≥32 字"的判据。
     * 它的栈消耗主要来自 printf 的整型格式化路径，与实验 12 的 102 字一致。 */
    xTaskCreate( monitor_task, "monitor", 160, NULL, 1, &s_h_monitor );

    printf( "[F103_13] 两个任务建好后堆余量 %u B，启动调度器"
            "（守护任务、定时器命令队列与 idle 由内核创建）\r\n",
            ( unsigned ) xPortGetFreeHeapSize() );
    vTaskStartScheduler();

    printf( "调度器未能启动（堆不足？）\r\n" );
    for( ;; )
    {
    }
}
