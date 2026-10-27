/*
 * F103_12：FreeRTOS 同步原语（二值信号量 / 计数信号量 / 事件组 / 中断→信号量）
 *
 * 目标：把"任务之间除了队列还能怎么同步"变成可核对的串口输出。
 * 实验 10 用的是队列（传数据），实验 11 是队列 + ISR（传数据的发起者是中断）。
 * 本实验只传"状态"，不传数据，因此换用三种原语，并逐个给出判据。
 *
 * 结构：
 *   core    （优先级 3）依次跑四个演示，每个演示打印自己的判据
 *   feeder  （优先级 2）按固定节拍 give / set 位，跑完自删
 *   monitor （优先级 1）每 500 ms 打印堆余量、栈水位与对象计数
 *   USART1_IRQHandler    收字节 → xSemaphoreGiveFromISR → portYIELD_FROM_ISR
 *
 * 四个演示与"看得见的结果"：
 *   A 二值信号量   core 阻塞等，feeder 800 ms 后 give → 打印实际等待 tick 数
 *   B 计数信号量   初始 1 个可用；取光后 0 超时拿不到；feeder 归还 2 个后能拿到
 *   C 事件组       等 (BIT_ARITH|BIT_SENSOR) 全置位 → 成功；等无人置位的 BIT 300 ms → 超时
 *   D 中断→信号量  串口每来一个字节 give 一次，但二值信号量装满后就不再接受，
 *                  因此"字节数 > 成功 give 数"，差额即被合并的事件数
 *
 * 为什么 D 是关键：队列会逐条排队，二值信号量只保留"有没有发生过"。
 * 这个差别在真实系统里决定"中断风暴会不会把任务淹没"。
 *
 * 本文件不预设合并次数：所有计数都原样打印，具体数值见实验 README 的实测记录。
 *
 * SysTick / SVCall / PendSV 由 FreeRTOS 接管，见 f103_freertos_vectors.s。
 */
#include <stdint.h>
#include <stdio.h>

#include "stm32f1xx.h"
#include "f103_uart.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "event_groups.h"

#define UART_BAUD         115200U
#define REPORT_PERIOD_MS  500U
#define MONITOR_PERIOD_MS 1000U       /* 资源报告周期：太密会淹没演示输出 */
#define USART1_IRQ_PRIO   5U          /* 数值越大优先级越低；必须 >= configMAX_SYSCALL_INTERRUPT_PRIORITY */

#define POOL_SIZE         3U          /* 资源池总量 */
#define POOL_INIT         1U          /* 初始可用数：故意只给 1 个，才能演示"取光" */

#define EV_BIT_ARITH      ( ( EventBits_t ) 0x01U )
#define EV_BIT_SENSOR     ( ( EventBits_t ) 0x02U )
#define EV_BIT_NEVER      ( ( EventBits_t ) 0x04U )   /* 没有任何任务置位，只用于超时路径 */

static SemaphoreHandle_t  s_bin;      /* 二值信号量：一次事件通知 */
static SemaphoreHandle_t  s_pool;     /* 计数信号量：资源槽位 */
static SemaphoreHandle_t  s_uart;     /* ISR 侧二值信号量 */
static SemaphoreHandle_t  s_log_mutex;
static EventGroupHandle_t s_events;

/* ISR 与任务共享：volatile 只保证不被优化掉，真正的一致性靠信号量本身 */
static volatile uint32_t s_uart_bytes;
static volatile uint32_t s_uart_wakes;   /* 成功的 give 次数 */

static TaskHandle_t s_h_core;
static TaskHandle_t s_h_feeder;
static TaskHandle_t s_h_monitor;

/* 整行输出必须串起来：printf 不是原子的，抢占会把它拆成两行碎片 */
#define LOG( ... )                                                       \
    do {                                                                 \
        if( xSemaphoreTake( s_log_mutex, portMAX_DELAY ) == pdTRUE ) {    \
            printf( __VA_ARGS__ );                                       \
            xSemaphoreGive( s_log_mutex );                               \
        }                                                                \
    } while( 0 )

/* ---- 演示 A：二值信号量 --------------------------------------------------- */

static void demo_binary( void )
{
    TickType_t before = xTaskGetTickCount();

    /* 二值信号量只有"有/无"两态。create 之后是空的，所以这里必然阻塞，
     * 直到 feeder 在 800 ms 后 give —— 这就是它与队列最根本的区别：
     * 队列能存 N 份数据，二值信号量最多记 1 次"发生过"。 */
    if( xSemaphoreTake( s_bin, portMAX_DELAY ) == pdTRUE )
    {
        uint32_t waited = ( uint32_t ) ( xTaskGetTickCount() - before );
        LOG( "[sync] bin waited=%lu ticks, take ok\n", ( unsigned long ) waited );
    }
    else
    {
        LOG( "[sync] bin take failed\n" );
    }
}

/* ---- 演示 B：计数信号量 --------------------------------------------------- */

static void demo_counting( void )
{
    /* 初始只有 POOL_INIT 个：先取光，才能观察到"再取失败" */
    if( xSemaphoreTake( s_pool, 0 ) == pdTRUE )
    {
        LOG( "[sync] pool take1 ok, count=%u\n", ( unsigned ) uxSemaphoreGetCount( s_pool ) );
    }

    /* 0 超时 = 不阻塞的尝试。拿不到要能区分"没有资源"和"调用失败"，
     * 所以返回值和计数一起打印。 */
    BaseType_t r = xSemaphoreTake( s_pool, 0 );
    LOG( "[sync] pool take2(0ms) result=%d, count=%u\n",
         ( int ) r, ( unsigned ) uxSemaphoreGetCount( s_pool ) );

    /* feeder 会归还 2 个。这里用 1000 ms 阻塞等待，而不是轮询：
     * 计数信号量的典型用法就是"资源不可用时挂起，而不是空转"。 */
    for( unsigned i = 0; i < 2U; i++ )
    {
        if( xSemaphoreTake( s_pool, pdMS_TO_TICKS( 1000U ) ) == pdTRUE )
        {
            LOG( "[sync] pool refill take ok, count=%u\n",
                 ( unsigned ) uxSemaphoreGetCount( s_pool ) );
        }
        else
        {
            LOG( "[sync] pool refill timeout\n" );
        }
    }
}

/* ---- 演示 C：事件组 ------------------------------------------------------- */

static void demo_events( void )
{
    TickType_t before = xTaskGetTickCount();

    /* xWaitForAllBits = pdTRUE：两个位都置位才返回，返回的是当时的全部位。
     * xClearOnExit = pdTRUE：返回时把这些位清掉，下一次等待从零开始。
     * 这两个开关的组合决定了"全部条件满足"还是"任一条件满足"，事件组的语义全靠它们。 */
    EventBits_t got = xEventGroupWaitBits( s_events,
                                           EV_BIT_ARITH | EV_BIT_SENSOR,
                                           pdTRUE,      /* clear on exit */
                                           pdTRUE,      /* wait for all */
                                           pdMS_TO_TICKS( 2000U ) );
    LOG( "[sync] event wait_all got=0x%02lx waited=%lu ticks\n",
         ( unsigned long ) got,
         ( unsigned long ) ( xTaskGetTickCount() - before ) );

    /* 超时路径：没有任何任务置 EV_BIT_NEVER，必须返回 0 而不是挂死。
     * 300 ms 是"固件毫秒"，QEMU 的时基比真实时间快约 3 倍（见 tests/qemu-stm32 的 tickrate 用例）。 */
    before = xTaskGetTickCount();
    got    = xEventGroupWaitBits( s_events, EV_BIT_NEVER,
                                  pdTRUE, pdTRUE, pdMS_TO_TICKS( 300U ) );
    LOG( "[sync] event timeout got=0x%02lx waited=%lu ticks\n",
         ( unsigned long ) got,
         ( unsigned long ) ( xTaskGetTickCount() - before ) );
}

/* ---- 演示 D：中断 → 二值信号量 ------------------------------------------- */

static void demo_isr_sem( void )
{
    unsigned takes = 0;

    /* 第一段：阻塞等第一行数据的第一个字节。
     * 超时给到 3000 tick：数据由宿主机 feeder 在真实 1 s 左右投放，
     * 按 2.90 倍时基折算约 2900 tick，留出余量。 */
    if( xSemaphoreTake( s_uart, pdMS_TO_TICKS( 3000U ) ) != pdTRUE )
    {
        LOG( "[sync] isr 3 s 内未收到任何字节\n" );
        return;
    }
    takes++;
    LOG( "[sync] isr wake#%u bytes=%lu gives=%lu\n",
         takes, ( unsigned long ) s_uart_bytes, ( unsigned long ) s_uart_wakes );

    /* 第二段：消费者故意慢一拍。间隔 300 tick 足以让整行的其余字节全部到齐，
     * 而这期间二值信号量装不下第二个事件 → 成功的 give 数会明显小于字节数。
     * 换成实验 11 的队列就不会合并：N 个字节排 N 项（受队列深度限制）。
     * 这里只打印原始计数，合并量 = bytes - gives 由日志读出。 */
    for( unsigned i = 0; i < 3U; i++ )
    {
        vTaskDelay( pdMS_TO_TICKS( 300U ) );

        if( xSemaphoreTake( s_uart, 0 ) != pdTRUE )
        {
            LOG( "[sync] isr 空闲 300 tick 后无新事件，结束采集\n" );
            break;
        }

        takes++;
        LOG( "[sync] isr wake#%u bytes=%lu gives=%lu\n",
             takes, ( unsigned long ) s_uart_bytes, ( unsigned long ) s_uart_wakes );
    }

    LOG( "[sync] isr summary bytes=%lu gives=%lu takes=%u coalesced=%lu\n",
         ( unsigned long ) s_uart_bytes,
         ( unsigned long ) s_uart_wakes,
         takes,
         ( unsigned long ) ( ( s_uart_bytes > s_uart_wakes ) ?
                             ( s_uart_bytes - s_uart_wakes ) : 0U ) );
}

/* ---- 任务 ---------------------------------------------------------------- */

static void core_task( void *arg )
{
    ( void ) arg;

    demo_binary();
    demo_counting();
    demo_events();
    demo_isr_sem();

    LOG( "[sync] done heap_free=%u B\n", ( unsigned ) xPortGetFreeHeapSize() );

    for( ;; )
    {
        vTaskDelay( pdMS_TO_TICKS( REPORT_PERIOD_MS ) );
    }
}

static void feeder_task( void *arg )
{
    ( void ) arg;

    /* 800 ms 后才 give：让 core 的"等待"是真实阻塞，而不是立即返回 */
    vTaskDelay( pdMS_TO_TICKS( 800U ) );
    xSemaphoreGive( s_bin );

    /* 归还两个资源：core 此时正阻塞在带超时的 take 上 */
    vTaskDelay( pdMS_TO_TICKS( 200U ) );
    xSemaphoreGive( s_pool );
    xSemaphoreGive( s_pool );

    /* 先置一个位，200 ms 后再置另一个：core 等的是"两个都置位"，
     * 所以第一次置位不应该唤醒它 —— 这一点体现在 waited 的数值上。 */
    xEventGroupSetBits( s_events, EV_BIT_ARITH );
    vTaskDelay( pdMS_TO_TICKS( 200U ) );
    xEventGroupSetBits( s_events, EV_BIT_SENSOR );

    /* 不调用 vTaskDelete(NULL)：monitor 要持续读 feeder 的栈水位，
     * 任务被删除后其 TCB 与栈已归还堆，再用句柄读水位就是访问已释放内存。 */
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
        LOG( "[res] heap_free=%u B, stack_hwm core=%u feeder=%u monitor=%u words\n",
             ( unsigned ) xPortGetFreeHeapSize(),
             ( unsigned ) uxTaskGetStackHighWaterMark( s_h_core ),
             ( unsigned ) uxTaskGetStackHighWaterMark( s_h_feeder ),
             ( unsigned ) uxTaskGetStackHighWaterMark( s_h_monitor ) );
    }
}

/* ---- 中断 ---------------------------------------------------------------- */

/*
 * 与实验 11 的对应关系：同样的 USART1 中断，那里投的是队列（逐字节排队），
 * 这里投的是二值信号量（只记"发生过"）。选择依据是接收方要"数据"还是"状态"。
 *
 * 中断里仍然只能调 FromISR 系列：xSemaphoreGiveFromISR 在 ISR 里不会阻塞，
 * 返回 pdTRUE 表示"有一个更高优先级的任务被唤醒"，此时才需要 portYIELD_FROM_ISR。
 */
void USART1_IRQHandler( void )
{
    BaseType_t woken = pdFALSE;
    uint32_t   sr    = USART1->SR;

    if( ( sr & ( USART_SR_RXNE | USART_SR_ORE ) ) == 0U )
    {
        return;
    }

    uint8_t ch = ( uint8_t ) USART1->DR;   /* 读 DR 同时清 RXNE 与 ORE */
    ( void ) ch;

    if( ( sr & USART_SR_ORE ) == 0U )
    {
        s_uart_bytes++;
        /* 拿不到（信号量仍为"有"）就什么都不做：这正是合并语义，不是丢失错误 */
        if( xSemaphoreGiveFromISR( s_uart, &woken ) == pdTRUE )
        {
            s_uart_wakes++;
        }
    }

    portYIELD_FROM_ISR( woken );
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

    /* 打开 RX 中断并设优先级：数值 5 恰等于 configMAX_SYSCALL_INTERRUPT_PRIORITY，
     * 再小（数值更小 = 优先级更高）就会落在 FreeRTOS 临界区屏蔽之外，
     * 中断里的 FromISR 调用会破坏内核数据结构。 */
    USART1->CR1 |= USART_CR1_RXNEIE;
    NVIC_SetPriority( USART1_IRQn, USART1_IRQ_PRIO );
    NVIC_EnableIRQ( USART1_IRQn );

    printf( "\r\n[F103_12] FreeRTOS %s sync primitives, HCLK=%lu Hz, tick=%lu Hz\r\n",
            tskKERNEL_VERSION_NUMBER,
            ( unsigned long ) SystemCoreClock,
            ( unsigned long ) configTICK_RATE_HZ );
    printf( "[F103_12] pool=%u (init %u), USART1_IRQ prio=%u\r\n",
            ( unsigned ) POOL_SIZE, ( unsigned ) POOL_INIT, ( unsigned ) USART1_IRQ_PRIO );

    /* 同步对象和任务栈都从同一个 4 KB 堆里切，分开记账才知道哪部分吃得多。
     * 注意：heap_4 的 xFreeBytesRemaining 在第一次 pvPortMalloc 时才初始化，
     * 所以"建对象前"这个值恒为 0，只能建完对象后用 总量 - 余量 反算。 */
    s_log_mutex = xSemaphoreCreateMutex();
    s_bin       = xSemaphoreCreateBinary();
    s_pool      = xSemaphoreCreateCounting( POOL_SIZE, POOL_INIT );
    s_uart      = xSemaphoreCreateBinary();
    s_events    = xEventGroupCreate();

    if( ( s_log_mutex == NULL ) || ( s_bin == NULL ) || ( s_pool == NULL ) ||
        ( s_uart == NULL ) || ( s_events == NULL ) )
    {
        printf( "创建同步对象失败：heap 不足\r\n" );
        for( ;; )
        {
        }
    }

    printf( "[F103_12] 五个同步对象（4 个信号量 + 1 个事件组）用掉 %u B\n",
            ( unsigned ) ( configTOTAL_HEAP_SIZE - xPortGetFreeHeapSize() ) );

    /* 二值信号量创建后是"空"的。显式确认一次，避免把"能立刻拿到"误当成正常行为。 */
    if( xSemaphoreTake( s_bin, 0 ) == pdTRUE )
    {
        printf( "[F103_12] 警告：二值信号量初值非空，后续 waited 数值不可信\r\n" );
        xSemaphoreGive( s_bin );
    }
    else
    {
        printf( "[F103_12] 二值信号量初值为空（符合预期）\r\n" );
    }

    xTaskCreate( core_task,    "core",    192, NULL, 3, &s_h_core );
    xTaskCreate( feeder_task,  "feeder",  128, NULL, 2, &s_h_feeder );
    xTaskCreate( monitor_task, "monitor", 160, NULL, 1, &s_h_monitor );

    printf( "[F103_12] 初始空闲堆 %u B，启动调度器\r\n",
            ( unsigned ) xPortGetFreeHeapSize() );

    vTaskStartScheduler();

    printf( "调度器未能启动（堆不足？）\r\n" );
    for( ;; )
    {
    }
}
