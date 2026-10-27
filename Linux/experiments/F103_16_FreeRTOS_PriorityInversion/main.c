/*
 * F103_16：优先级反转与优先级继承
 *
 * 目标：把"低优先级任务持锁、中优先级任务插队、高优先级任务被拖住"这条因果链
 * 变成可核对的 tick 读数，并用互斥量（优先级继承）做对照。
 *
 * 三个任务 + 一个协调者，优先级分别是 1 / 2 / 3 / 4（configMAX_PRIORITIES = 5）：
 *   low   (1) 持锁做两段耗时动作，每段 50 tick，中间必须让出 CPU
 *   mid   (2) 纯忙等 300 tick —— 不 delay、不 yield、不等任何对象
 *   high  (3) 尝试取锁，测量等了多少 tick
 *   coord (4) 保证"L 先持锁、再放出 M 和 H"，并在最后汇总
 *
 * 两轮场景只改一个变量：锁对象的类型。
 *   A 二值信号量  创建后先 give 一次当"开锁"；没有持有者概念，也不做继承
 *   B 互斥量      有持有者，等待者优先级高于持有者时临时提升持有者
 *
 * 判据（全部是 tick 读数，不比对绝对秒数）：
 *   1 H 等锁时长：A 由 M 的忙等决定（约 300 + 持锁剩余），B 回落到持锁时长量级
 *   2 L 的阶段一完成时刻：A 被推迟到 M 忙等之后，B 按 50 tick 正常到点
 *   3 H 阻塞期间 L 的实时优先级：A 仍为 1，B 为 3
 *   4 L 释放锁之前读到的优先级：B 为 3；give 之后为 1（恢复发生在 give 内部）
 *
 * 为什么 M 必须是纯忙等：只要它让出 CPU，L 就能推进，反转立刻消失。
 * 这也说明了软件侧心跳/超时在被监督任务忙等时看不见（见实验 14 与项目 README §6.6）。
 *
 * 边界：tick 是虚拟时间。QEMU 时基相对墙钟约 2.90 倍（tests/qemu-stm32/RESULT.md），
 * 本实验只比较 tick 之间的比例，不报绝对秒数；真机上的中断延迟与临界区时长不在测量范围内。
 *
 * SysTick / SVCall / PendSV 由 FreeRTOS 接管，见 F103_common/src/f103_freertos_vectors.s。
 */
#include <stdint.h>
#include <stdio.h>

#include "stm32f1xx.h"
#include "f103_uart.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define UART_BAUD       115200U

#define PRIO_COORD      4U            /* 必须最高：它决定 M 与 H 的放行时刻 */
#define PRIO_HIGH       3U
#define PRIO_MID        2U
#define PRIO_LOW        1U

#define LOW_HOLD_MS     50U           /* 每段持锁动作；两段，中间让出 CPU */
#define LOW_HOLD_STAGES 2U
#define MID_BUSY_MS     300U          /* 纯忙等长度：要明显长于 L 的持锁总时长 */
#define H_WAIT_MS       3000U         /* H 等的上限：超时只可能是真挂死，不是正常路径 */
#define WAIT_DONE_MS    5000U
#define DONE_COUNT      3U            /* L / M / H 各上报一次完成 */

#define STACK_COORD     192U
#define STACK_LOW       160U          /* 64 字余量 / 160 = 40%：首版 144 字时实测 42，相对余量 29% 太贴线 */
#define STACK_MID       160U          /* 首版 144 字时实测余量 40 字、相对 27.8%：过线但太贴，提到 160 */
#define STACK_HIGH      160U          /* 同上：首版 144 字时余量 38 字，相对 26%，提到 160 后 34% */

/* 场景运行期的全部读数。字段由各任务写入，coordinator 在三个完成事件齐了之后读，
 * 所以不需要额外加锁——写入与读取被信号量建立了先后关系。
 *
 * 放在静态区而不是栈上：它约 120 B，占 coordinator 栈峰值的近四分之一，
 * 而 4 KB 堆既要放四个任务的栈，也要放它，重复计算会让 `xTaskCreate` 直接失败（首版实测）。 */
typedef struct
{
    const char *      name;
    uint8_t           use_mutex;

    SemaphoreHandle_t lock;       /* 被争用的锁：互斥量或二值信号量 */
    SemaphoreHandle_t locked;     /* L 取锁后通知 coordinator */
    SemaphoreHandle_t done;       /* 计数信号量：L / M / H 各 give 一次 */
    TaskHandle_t      low;

    TickType_t        t_hold;     /* L 取到锁 */
    TickType_t        t_stage1;   /* L 完成第一段持锁动作 */
    TickType_t        t_release;  /* L 释放锁 */
    TickType_t        t_m_start;  /* M 开始忙等 */
    TickType_t        t_m_end;    /* M 忙等结束 */
    TickType_t        t_h_start;  /* H 开始等锁 */
    TickType_t        t_h_got;    /* H 拿到锁（或超时返回） */
    BaseType_t        h_result;

    UBaseType_t       prio_seen;        /* H 阻塞期间 coordinator 读到 L 的优先级 */
    UBaseType_t       prio_stage1;      /* L 自己读到的（继承是否已生效） */
    UBaseType_t       prio_at_release;  /* give 之前 */
    UBaseType_t       prio_after;       /* give 之后 */

    uint16_t          hwm_low;
    uint16_t          hwm_mid;
    uint16_t          hwm_high;
    uint16_t          hwm_coord;
} inv_ctx_t;

static SemaphoreHandle_t s_log_mutex;

/* 两轮场景的汇总副本。只放"要对比的那几个数"，避免把整份 ctx 留在 RAM 里。 */
typedef struct
{
    const char * name;
    uint8_t      use_mutex;
    TickType_t   h_wait;
    TickType_t   l_hold;
    TickType_t   l_stage1;
    TickType_t   m_start;
    TickType_t   m_dur;
    UBaseType_t  prio_seen;
    UBaseType_t  prio_at_release;
    BaseType_t   h_result;
} inv_summary_t;

static inv_summary_t s_summary[2];
static inv_ctx_t     s_ctx;

/* 整行输出必须串起来：printf 不是原子的，抢占会把它拆成两行碎片。
 * 注意本实验的 LOG 也会短暂持有一个互斥量——它同样是可继承的锁，
 * 但因为持锁时间只有一次格式化输出，对场景读数的扰动可以忽略（实测两轮数值稳定）。 */
#define LOG( ... )                                                        \
    do {                                                                  \
        if( xSemaphoreTake( s_log_mutex, portMAX_DELAY ) == pdTRUE ) {    \
            printf( __VA_ARGS__ );                                        \
            xSemaphoreGive( s_log_mutex );                                \
        }                                                                 \
    } while( 0 )

/* ---- 任务 ---------------------------------------------------------------- */

/*
 * L：先取锁，再通知 coordinator。它被抢占时还没有执行到第一段 delay，
 * 所以两段 delay 都从"coordinator 放出 M 和 H 之后"才开始计时。
 */
static void low_task( void *arg )
{
    inv_ctx_t *c = arg;

    if( xSemaphoreTake( c->lock, portMAX_DELAY ) != pdTRUE )
    {
        LOG( "[inv] %s L: 取锁失败\n", c->name );
        xSemaphoreGive( c->done );
        vTaskDelete( NULL );
        return;
    }

    c->t_hold = xTaskGetTickCount();
    LOG( "[inv] %s L: 取锁 prio=%u t=%lu\n",
         c->name,
         ( unsigned ) uxTaskPriorityGet( NULL ),
         ( unsigned long ) c->t_hold );

    /* 交接点：coordinator（4）会在这里抢占，然后才放出 M（2）与 H（3）。 */
    xSemaphoreGive( c->locked );

    for( unsigned i = 0; i < LOW_HOLD_STAGES; i++ )
    {
        vTaskDelay( pdMS_TO_TICKS( LOW_HOLD_MS ) );

        if( i == 0U )
        {
            c->t_stage1    = xTaskGetTickCount();
            c->prio_stage1 = uxTaskPriorityGet( NULL );
            LOG( "[inv] %s L: 阶段一完成 prio=%u 距取锁 %lu tick\n",
                 c->name,
                 ( unsigned ) c->prio_stage1,
                 ( unsigned long ) ( c->t_stage1 - c->t_hold ) );
        }
    }

    c->t_release       = xTaskGetTickCount();
    c->prio_at_release = uxTaskPriorityGet( NULL );

    /* 这一行在 give 之前打印：有继承时读到的仍是"被提升后"的 3，
     * 恢复发生在 xSemaphoreGive() 内部，所以下一个读数才是 1。 */
    LOG( "[inv] %s L: 释放前 prio=%u 持锁 %lu tick\n",
         c->name,
         ( unsigned ) c->prio_at_release,
         ( unsigned long ) ( c->t_release - c->t_hold ) );

    xSemaphoreGive( c->lock );

    /* give 内部可能立刻切换到 H，所以这两行赋值晚于 H 的输出，但读到的值仍是恢复后的。 */
    c->prio_after = uxTaskPriorityGet( NULL );
    c->hwm_low    = uxTaskGetStackHighWaterMark( NULL );

    xSemaphoreGive( c->done );
    vTaskDelete( NULL );
}

/*
 * M：纯忙等。不调用任何会让出 CPU 的 API —— 这是反转成立的前提。
 * 用 tick 差而不是循环次数做边界：QEMU 时基不可信，但 tick 之间的比例仍可比。
 */
static void mid_task( void *arg )
{
    inv_ctx_t *c = arg;
    TickType_t t0 = xTaskGetTickCount();

    c->t_m_start = t0;

    while( ( TickType_t ) ( xTaskGetTickCount() - t0 ) <
           ( TickType_t ) pdMS_TO_TICKS( MID_BUSY_MS ) )
    {
    }

    c->t_m_end = xTaskGetTickCount();
    LOG( "[inv] %s M: 忙等结束 t=%lu 距取锁 %lu tick 时长 %lu tick\n",
         c->name,
         ( unsigned long ) c->t_m_end,
         ( unsigned long ) ( c->t_m_end - c->t_hold ),
         ( unsigned long ) ( c->t_m_end - t0 ) );

    c->hwm_mid = uxTaskGetStackHighWaterMark( NULL );

    xSemaphoreGive( c->done );
    vTaskDelete( NULL );
}

/*
 * H：只做一件事——取锁并测量等了多少 tick。
 * 带超时而不是 portMAX_DELAY：真挂死时日志要能收尾，不能让整份输出停在这一行。
 */
static void high_task( void *arg )
{
    inv_ctx_t *c = arg;

    c->t_h_start = xTaskGetTickCount();

    c->h_result = xSemaphoreTake( c->lock, pdMS_TO_TICKS( H_WAIT_MS ) );

    c->t_h_got = xTaskGetTickCount();
    LOG( "[inv] %s H: 等锁 %lu tick result=%d prio=%u\n",
         c->name,
         ( unsigned long ) ( c->t_h_got - c->t_h_start ),
         ( int ) c->h_result,
         ( unsigned ) uxTaskPriorityGet( NULL ) );

    if( c->h_result == pdTRUE )
    {
        xSemaphoreGive( c->lock );   /* 拿到即放：本实验只测等待时长，不测持锁 */
    }

    c->hwm_high = uxTaskGetStackHighWaterMark( NULL );

    xSemaphoreGive( c->done );
    vTaskDelete( NULL );
}

/* ---- 协调者 -------------------------------------------------------------- */

/* 一个计数信号量收三个完成事件：三个二值信号量要多占约 176 B 堆，
 * 而 4 KB 堆在"四任务 + 三同步对象"之后已经见底（首版就是在这里 MALLOC FAILED）。
 * 计数信号量只保留"完成了几次"，而这个实验恰好不需要区分是谁完成的。 */
static void wait_done( const inv_ctx_t *c, unsigned index )
{
    if( xSemaphoreTake( c->done, pdMS_TO_TICKS( WAIT_DONE_MS ) ) != pdTRUE )
    {
        LOG( "[inv] %s 超时：第 %u 个完成事件未到（后续读数不可信）\n",
             c->name, index );
    }
}

/*
 * ctx 不清零：每个字段要么在任务启动前由协调者赋值，要么由任务写入后
 * 被完成信号量建立"先后关系"再读取。（清单式依赖比 memset 更值得写清——
 * 漏掉一次 memset 的后果，和漏掉一次初值完全一样。）
 */
static void run_scenario( inv_ctx_t *c, const char *name, uint8_t use_mutex )
{
    c->name      = name;
    c->use_mutex = use_mutex;

    c->lock   = use_mutex ? xSemaphoreCreateMutex() : xSemaphoreCreateBinary();
    c->locked = xSemaphoreCreateBinary();
    c->done   = xSemaphoreCreateCounting( DONE_COUNT, 0 );

    if( ( c->lock == NULL ) || ( c->locked == NULL ) || ( c->done == NULL ) )
    {
        LOG( "[inv] %s 创建同步对象失败：heap 不足\n", name );
        return;
    }

    if( use_mutex == 0U )
    {
        /* 二值信号量创建后是"空"的，先 give 一次模拟"锁可用"。
         * 与互斥量的差别从这一步就开始了：谁都可以给，没有持有者，也没有继承。 */
        xSemaphoreGive( c->lock );
    }

    LOG( "\r\n[inv] ===== 场景 %s：%s =====\n",
         name,
         use_mutex ? "互斥量（优先级继承）" : "二值信号量当锁（无继承）" );

    /* 任务创建失败必须当场报出：首版漏了返回值检查，堆不足时钩子直接把现场停掉，
     * 日志只停在"L: 取锁"一行，看起来像调度器卡死。 */
    if( xTaskCreate( low_task, "low", STACK_LOW, c, PRIO_LOW, &c->low ) != pdPASS )
    {
        LOG( "[inv] %s 创建 low 失败：heap 不足（%u B 可用）\n",
             name, ( unsigned ) xPortGetFreeHeapSize() );
        return;
    }

    /* 必须确认 L 已持锁再往下走，否则 H 可能先拿到锁，整段对照就失去意义。 */
    if( xSemaphoreTake( c->locked, pdMS_TO_TICKS( 1000 ) ) != pdTRUE )
    {
        LOG( "[inv] %s L 未在 1000 tick 内取到锁，本轮作废\n", name );
        return;
    }

    /* 此刻 L(1) 持锁。放出 M(2) 与 H(3) 后 coordinator 立即让出 CPU：
     * H 先运行并阻塞在锁上，M 再运行开始忙等，L 在两者占着 CPU 时抢不回来。 */
    if( ( xTaskCreate( mid_task,  "mid",  STACK_MID,  c, PRIO_MID,  NULL ) != pdPASS ) ||
        ( xTaskCreate( high_task, "high", STACK_HIGH, c, PRIO_HIGH, NULL ) != pdPASS ) )
    {
        LOG( "[inv] %s 创建 mid/high 失败：heap 不足（%u B 可用）\n",
             name, ( unsigned ) xPortGetFreeHeapSize() );
        return;
    }

    vTaskDelay( pdMS_TO_TICKS( 10 ) );

    /* H 已阻塞 10 tick：继承若生效，这一刻必然已经发生。 */
    c->prio_seen = uxTaskPriorityGet( c->low );
    LOG( "[inv] %s 观测：H 阻塞期间 L 的实时优先级 = %u\n",
         c->name, ( unsigned ) c->prio_seen );

    /* 三个完成事件都要等：有继承时 M 被推迟到 H 之后才跑完，
     * 只等 H 会读到 M 尚未记录的水位（0）。 */
    for( unsigned i = 1; i <= DONE_COUNT; i++ )
    {
        wait_done( c, i );
    }

    /* give 之后到 vTaskDelete 之间还有几行，让它们跑完再读 ctx */
    vTaskDelay( pdMS_TO_TICKS( 5 ) );

    c->hwm_coord = uxTaskGetStackHighWaterMark( NULL );

    LOG( "[inv] %s 结果：H 等锁=%lu  L 持锁=%lu  L 阶段一=%lu  M 开始=%lu  L prio(阻塞期)=%u -> (释放后)=%u\n",
         c->name,
         ( unsigned long ) ( c->t_h_got - c->t_h_start ),
         ( unsigned long ) ( c->t_release - c->t_hold ),
         ( unsigned long ) ( c->t_stage1 - c->t_hold ),
         ( unsigned long ) ( c->t_m_start - c->t_hold ),
         ( unsigned ) c->prio_seen,
         ( unsigned ) c->prio_after );

    LOG( "[inv] %s 栈水位（字）：coord=%u low=%u mid=%u high=%u\n",
         c->name,
         ( unsigned ) c->hwm_coord,
         ( unsigned ) c->hwm_low,
         ( unsigned ) c->hwm_mid,
         ( unsigned ) c->hwm_high );

    /* 汇总副本按 use_mutex 索引存放：两轮复用同一个 ctx，不能靠指针区分是哪一轮 */
    {
        inv_summary_t *s = &s_summary[ use_mutex ? 1U : 0U ];

        s->name            = name;
        s->use_mutex       = use_mutex;
        s->h_wait          = c->t_h_got - c->t_h_start;
        s->l_hold          = c->t_release - c->t_hold;
        s->l_stage1        = c->t_stage1 - c->t_hold;
        s->m_start         = c->t_m_start - c->t_hold;
        s->m_dur           = c->t_m_end - c->t_m_start;
        s->prio_seen       = c->prio_seen;
        s->prio_at_release = c->prio_at_release;
        s->h_result        = c->h_result;
    }

    vSemaphoreDelete( c->lock );
    vSemaphoreDelete( c->locked );
    vSemaphoreDelete( c->done );
}

static void print_summary( void )
{
    LOG( "\r\n[inv] ===== 两轮对照（单位：tick）=====\n" );
    LOG( "[inv] 锁类型          H等锁  L持锁  L阶段一  M开始  M时长  L阻塞期prio  L释放前prio\n" );

    for( unsigned i = 0; i < 2U; i++ )
    {
        const inv_summary_t *s = &s_summary[i];

        if( s->name == NULL )
        {
            continue;
        }

        LOG( "[inv] %-14s %6lu %6lu %8lu %6lu %6lu %12u %11u\n",
             s->use_mutex ? "mutex" : "binary-sem",
             ( unsigned long ) s->h_wait,
             ( unsigned long ) s->l_hold,
             ( unsigned long ) s->l_stage1,
             ( unsigned long ) s->m_start,
             ( unsigned long ) s->m_dur,
             ( unsigned ) s->prio_seen,
             ( unsigned ) s->prio_at_release );
    }

    LOG( "[inv] 判读：binary-sem 轮里 M 一直占着 CPU，L 的阶段一被推迟到 M 忙等之后，\n" );
    LOG( "[inv]       H 的等锁时长 ≈ M 时长 + 持锁剩余；mutex 轮里 L 被临时提到 H 的优先级，\n" );
    LOG( "[inv]       M 抢不到 CPU，等待回落到持锁时长量级。继承只压缩\"被插队\"的部分，\n" );
    LOG( "[inv]       不能让持锁者自己的 delay/IO 变短 —— 这就是等待时间不为 0 的原因。\n" );
}

static void coord_task( void *arg )
{
    ( void ) arg;

    /* 两个场景串行复用同一个 ctx。每个字段要么在任务启动前由协调者赋值，
     * 要么由任务写入后被完成信号量建立"先后关系"再读取，所以不清零；
     * 上一轮的读数已复制进 s_summary，不依赖残留值。 */
    run_scenario( &s_ctx, "A-bin",   0U );
    run_scenario( &s_ctx, "B-mutex", 1U );

    print_summary();

    LOG( "[inv] done heap_free=%u B\n", ( unsigned ) xPortGetFreeHeapSize() );

    for( ;; )
    {
        vTaskDelay( pdMS_TO_TICKS( 1000 ) );
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

    printf( "\r\n[F103_16] FreeRTOS %s 优先级反转 / 优先级继承, tick=%lu Hz\r\n",
            tskKERNEL_VERSION_NUMBER,
            ( unsigned long ) configTICK_RATE_HZ );
    printf( "[F103_16] prio: coord=%u high=%u mid=%u low=%u; low hold=%u ms x %u; mid busy=%u ms\r\n",
            ( unsigned ) PRIO_COORD, ( unsigned ) PRIO_HIGH, ( unsigned ) PRIO_MID,
            ( unsigned ) PRIO_LOW, ( unsigned ) LOW_HOLD_MS,
            ( unsigned ) LOW_HOLD_STAGES, ( unsigned ) MID_BUSY_MS );

    s_log_mutex = xSemaphoreCreateMutex();
    if( s_log_mutex == NULL )
    {
        printf( "创建日志互斥量失败：heap 不足\r\n" );
        for( ;; )
        {
        }
    }

    printf( "[F103_16] 初始空闲堆 %u B，启动调度器\r\n",
            ( unsigned ) xPortGetFreeHeapSize() );

    /* 优先级 4 = configMAX_PRIORITIES - 1：协调者必须在任何被测任务之上，
     * 否则它自己也会被 M 的忙等挡住，放行时刻就不受控了。 */
    xTaskCreate( coord_task, "coord", STACK_COORD, NULL, PRIO_COORD, NULL );

    vTaskStartScheduler();

    printf( "调度器未能启动（堆不足？）\r\n" );
    for( ;; )
    {
    }
}
