/*
 * F103_14：FreeRTOS 心跳与看门狗（任务存活监督 + 喂狗条件 + 阻塞检出与恢复）
 *
 * 目标：把"看门狗会不会救我"拆成两件可以分别验证的事：
 *   1) 监督逻辑：谁在心跳、心跳断了多久算异常、异常时停止喂狗、异常解除后恢复喂狗；
 *   2) 硬件动作：IWDG 计时到期 → 复位芯片 → 复位原因可读。
 * 第 1 件只依赖 SysTick 与调度器，QEMU 可实测；第 2 件依赖 IWDG 与 RCC 复位标志，
 * 探针实测两者在本仿真环境读回全 0（未建模），因此第 2 件只能上板闭环。
 * 本次日志能证明的是第 1 件，以及"第 2 件的寄存器写入路径已编译通过"。
 *
 * 结构：
 *   sensor  （优先级 3，160 字）每 100 ms 踢一次心跳；故障注入期间改为阻塞在 s_gate 上
 *   io      （优先级 2，128 字）每 150 ms 踢一次心跳，始终健康（用来证明"与"语义）
 *   health  （优先级 1，224 字）每 200 ms 检查所有通道：
 *             - 有任一通道 age > HB_TIMEOUT_MS → 判 STALE、停止喂狗
 *             - 连续 3 次 STALE → 执行恢复动作（解除注入、放行 sensor）
 *             - 全部恢复 → 恢复喂狗，并打印饥饿窗口长度
 *
 * 为什么用"与"语义：只要有一个任务不健康就停喂狗。若每个任务各喂各的，
 * 卡住的那个任务根本没机会喂，看门狗永远等不到它的心跳——反而失去意义。
 *
 * 时间单位：固件毫秒（1 tick = 1 ms）。QEMU 的时基是真实时间的 2.90 倍，
 * 所以本文件的毫秒只用于比较比例，不能当实时性结论。
 */
#include <stdint.h>
#include <stdio.h>

#include "stm32f1xx.h"
#include "f103_uart.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define UART_BAUD            115200U

#define HB_PERIOD_SENSOR_MS  100U      /* sensor 踢狗周期 */
#define HB_PERIOD_IO_MS      150U      /* io 踢狗周期 */
#define HB_TIMEOUT_MS        400U      /* age 超过它即判 STALE：约为两个最大周期的 2 倍余量 */
#define CHECK_PERIOD_MS      200U      /* health 检查周期（必须小于 HB_TIMEOUT_MS） */
#define SUMMARY_AT_CHECK     26U       /* 第几次检查时停止刷屏并打印汇总 */
#define STATUS_EVERY_N       20U       /* 稳定后每 N 次检查打一行状态 */

/* IWDG 名义超时，用来和"软件停止喂狗的窗口"直接比较。
 * PR=÷64、RLR=624、LSI 名义 40 kHz → 998 ms；实机 LSI 30~60 kHz 时约 665~1330 ms。 */
#define IWDG_NOMINAL_MS      998U

/* 两轮故障注入，差别只有一个参数：连续多少次 STALE 才执行恢复动作。
 * 目的不是"演示能恢复"，而是把软件恢复窗口量出来，并让它一轮小于、一轮大于
 * IWDG 名义超时——这样"真板上这一轮会不会被复位"就是可计算的，而不是靠猜。 */
static const struct
{
    uint32_t    inject_at;      /* 第几次检查时注入阻塞 */
    uint32_t    streak_limit;   /* 连续 STALE 达到多少次才执行恢复动作 */
    const char *tag;
} k_rounds[] = {
    { 4U,  3U, "R1 快恢复" },
    { 14U, 6U, "R2 慢恢复" },
};
#define ROUND_COUNT ( ( uint32_t ) ( sizeof( k_rounds ) / sizeof( k_rounds[ 0 ] ) ) )

/* 看门狗硬件路径的开关。QEMU 未建模 IWDG（探针 [11] 读回 PR/RLR/SR 全 0），
 * 因此仿真构建为 0：只编译寄存器写入函数，不调用它。
 * 真机把这里改成 1，并确认喂狗条件与 IWDG 超时窗口匹配后再烧写。 */
#define F103_WATCHDOG_ENABLE 0

/* IWDG 参数（RM0008 19 章）：LSI 40 kHz 名义值，PR=÷64 → 625 Hz。
 * RLR=624 → 名义 998 ms。注意 F1 的 LSI 容差很大（30~60 kHz），
 * 实际超时约在 665 ms ~ 1330 ms 之间，所以喂狗周期必须远小于名义值。 */
#define IWDG_KR_UNLOCK       0x5555U   /* 解除 PR/RLR 写保护 */
#define IWDG_KR_START        0xCCCCU   /* 启动（一旦启动只能等复位） */
#define IWDG_KR_FEED         0xAAAAU   /* 喂狗：重装计数器 */
#define IWDG_PR_DIV64        0x04U
#define IWDG_RLR_NOMINAL_1S  624U

#define HB_CHANNELS          2U
#define HB_SENSOR            0U
#define HB_IO                1U

static SemaphoreHandle_t s_log_mutex;
static SemaphoreHandle_t s_gate;          /* 故障注入用的"卡住点"，正常时无人取用 */

/* 心跳表：last = 最后一次踢狗的时刻，count = 累计踢狗次数。
 * health 读、其它任务写，volatile 只保证可见性；这里不需要原子性，
 * 因为读到的都是单调递增的 uint32，个别字节竞争不影响"是否超时"的判断。 */
static volatile uint32_t s_hb_last[ HB_CHANNELS ];
static volatile uint32_t s_hb_count[ HB_CHANNELS ];

static const char *const k_hb_name[ HB_CHANNELS ] = { "sensor", "io" };

static volatile uint32_t s_inject;        /* 1 = sensor 自锁在 s_gate 上 */
static volatile uint32_t s_feed_count;    /* 逻辑喂狗次数（含被门控掉的检查） */
static volatile uint32_t s_feed_blocked;  /* 因不健康而未喂狗的次数 */
static volatile uint32_t s_last_feed_tick;
static volatile uint32_t s_stale_total;   /* 累计判 STALE 的次数 */

static TaskHandle_t s_h_sensor;
static TaskHandle_t s_h_io;
static TaskHandle_t s_h_health;

#define LOG( ... )                                                       \
    do {                                                                 \
        if( xSemaphoreTake( s_log_mutex, portMAX_DELAY ) == pdTRUE ) {    \
            printf( __VA_ARGS__ );                                       \
            xSemaphoreGive( s_log_mutex );                               \
        }                                                                \
    } while( 0 )

/* ---- 看门狗硬件路径（真板用；QEMU 构建下只编译不调用） ------------------- */

/* 前置声明：health_task 在定义之前调用它，GCC 14 把隐式函数声明当错误。 */
void watchdog_feed_if_enabled( void );

void f103_watchdog_init( void )
{
    IWDG->KR  = IWDG_KR_UNLOCK;
    IWDG->PR  = IWDG_PR_DIV64;
    IWDG->RLR = IWDG_RLR_NOMINAL_1S;
    IWDG->KR  = IWDG_KR_START;
    IWDG->KR  = IWDG_KR_FEED;
}

void f103_watchdog_feed( void )
{
    IWDG->KR = IWDG_KR_FEED;
}

/* 复位原因：真板上这里能看到 IWDGRSTF，读完要写 RMVF 清标志，
 * 否则下一次复位后就分不清是看门狗还是上电。QEMU 读回全 0，见探针 [12]。 */
static uint32_t read_reset_flags( void )
{
    return RCC->CSR;
}

static void clear_reset_flags( void )
{
    RCC->CSR |= RCC_CSR_RMVF;
}

/* ---- 心跳 ---------------------------------------------------------------- */

/* 年龄用无符号差值：tick 是 32 位、约 49.7 天回绕一次，
 * 直接比较 last 与 now 的大小在回绕点会得到荒谬的负值；
 * ( now - last ) 在无符号语义下跨回绕仍然正确。 */
static uint32_t hb_age( uint32_t now, uint32_t idx )
{
    return now - s_hb_last[ idx ];
}

static void hb_kick( uint32_t idx )
{
    s_hb_last[ idx ] = ( uint32_t ) xTaskGetTickCount();
    s_hb_count[ idx ]++;
}

/* ---- 任务 ---------------------------------------------------------------- */

static void sensor_task( void *arg )
{
    ( void ) arg;

    for( ;; )
    {
        vTaskDelay( pdMS_TO_TICKS( HB_PERIOD_SENSOR_MS ) );

        if( s_inject != 0U )
        {
            /* 故障注入：把自己"卡死"在一个没人给的信号量上，模拟任务阻塞/死循环。
             * 关键点是本轮**不踢心跳**——看门狗要监控的正是"任务有没有走到心跳点"，
             * 而不是"任务对象还在不在"。 */
            ( void ) xSemaphoreTake( s_gate, pdMS_TO_TICKS( 1500U ) );
            continue;
        }

        hb_kick( HB_SENSOR );
    }
}

static void io_task( void *arg )
{
    ( void ) arg;

    for( ;; )
    {
        vTaskDelay( pdMS_TO_TICKS( HB_PERIOD_IO_MS ) );
        hb_kick( HB_IO );
    }
}

static void health_task( void *arg )
{
    ( void ) arg;

    uint32_t checks         = 0;
    uint32_t next_round     = 0;      /* 下一个待注入的轮次 */
    uint32_t stale_streak   = 0;      /* 本轮连续 STALE 次数 */
    uint32_t stalled        = 0;      /* 1 = 已注入、尚未执行恢复动作 */
    uint32_t degraded       = 0;      /* 1 = 当前处于停喂状态，只在恢复瞬间打印一次 */
    uint32_t recovered_at[ ROUND_COUNT ] = { 0U };

    for( ;; )
    {
        vTaskDelay( pdMS_TO_TICKS( CHECK_PERIOD_MS ) );
        checks++;

        uint32_t now      = ( uint32_t ) xTaskGetTickCount();
        uint32_t age_s    = hb_age( now, HB_SENSOR );
        uint32_t age_io   = hb_age( now, HB_IO );
        int      stale    = ( ( age_s > HB_TIMEOUT_MS ) || ( age_io > HB_TIMEOUT_MS ) ) ? 1 : 0;
        int      verbose  = ( checks <= SUMMARY_AT_CHECK ) ||
                            ( ( checks % STATUS_EVERY_N ) == 0U );

        if( stale == 0 )
        {
            /* 间隔必须在覆盖 s_last_feed_tick 之前算：
             * 先算 after-set 会得到恒为 0 的假数据。 */
            uint32_t gap = now - s_last_feed_tick;

            /* 全部健康才喂狗："与"语义。sensor 卡住时 io 仍然健康，
             * 但只要有一个不健康，本次就整体停喂——这是本实验的核心判据。 */
            s_feed_count++;
            s_last_feed_tick = now;
            watchdog_feed_if_enabled();

            if( degraded != 0U )
            {
                /* 从"最后一次喂狗"到"恢复喂狗"之间的窗口，就是真板上看门狗
                 * 会不会已经复位的判据：大于 IWDG 名义超时即意味着真板会先复位。 */
                degraded = 0U;
                recovered_at[ next_round - 1U ] = now;

                LOG( "[hb] %s 恢复：t=%lu 重新喂狗，停止喂狗窗口 %lu tick"
                     "（IWDG 名义 %u ms → %s）\n",
                     k_rounds[ next_round - 1U ].tag, ( unsigned long ) now,
                     ( unsigned long ) gap, ( unsigned ) IWDG_NOMINAL_MS,
                     ( gap < IWDG_NOMINAL_MS ) ? "软件自愈先发生" : "真板会先复位" );
            }

            stale_streak = 0U;

            if( verbose != 0 )
            {
                LOG( "[hb] t=%lu sensor age=%lu io age=%lu OK feed=ON feeds=%lu\n",
                     ( unsigned long ) now, ( unsigned long ) age_s, ( unsigned long ) age_io,
                     ( unsigned long ) s_feed_count );
            }
        }
        else
        {
            stale_streak++;
            s_stale_total++;
            s_feed_blocked++;
            degraded = 1U;

            if( verbose != 0 )
            {
                LOG( "[hb] t=%lu sensor age=%lu%s io age=%lu%s STALE feed=OFF streak=%lu\n",
                     ( unsigned long ) now,
                     ( unsigned long ) age_s, ( age_s > HB_TIMEOUT_MS ) ? "!" : "",
                     ( unsigned long ) age_io, ( age_io > HB_TIMEOUT_MS ) ? "!" : "",
                     ( unsigned long ) stale_streak );
            }

            /* 恢复动作：连续 STALE 才动手，避免一次抖动就触发降级。
             * 真实系统里这一步通常是"重启子系统 / 切备份通道 / 进入安全模式"，
             * 本实验用"解除注入并放行被阻塞的任务"代替。
             * streak_limit 来自本轮配置：轮次不同，恢复快慢不同，停喂窗口因此不同。 */
            if( ( stalled != 0U ) &&
                ( stale_streak >= k_rounds[ next_round - 1U ].streak_limit ) )
            {
                LOG( "[hb] %s 恢复动作：连续 %lu 次 STALE（门限 %lu），解除 sensor 阻塞\n",
                     k_rounds[ next_round - 1U ].tag,
                     ( unsigned long ) stale_streak,
                     ( unsigned long ) k_rounds[ next_round - 1U ].streak_limit );
                s_inject = 0U;
                ( void ) xSemaphoreGive( s_gate );
                stalled      = 0U;
                stale_streak = 0U;
            }
        }

        /* 注入编排：放在年龄判定之后，这样"注入时刻"与"首次 STALE 时刻"都能在日志里对齐。 */
        if( ( next_round < ROUND_COUNT ) &&
            ( checks == k_rounds[ next_round ].inject_at ) )
        {
            stalled  = 1U;
            s_inject = 1U;
            LOG( "[hb] %s 注入：t=%lu sensor 阻塞在 s_gate 上，恢复门限 = 连续 %lu 次 STALE\n",
                 k_rounds[ next_round ].tag, ( unsigned long ) now,
                 ( unsigned long ) k_rounds[ next_round ].streak_limit );
            next_round++;
        }

        /* 收尾报告：一次实验只总结一次，之后转为稀疏状态行 */
        if( checks == SUMMARY_AT_CHECK )
        {
            LOG( "[hb] 汇总：checks=%lu stale_total=%lu blocked=%lu feeds=%lu"
                 " 心跳次数 sensor=%lu io=%lu\n",
                 ( unsigned long ) checks,
                 ( unsigned long ) s_stale_total,
                 ( unsigned long ) s_feed_blocked,
                 ( unsigned long ) s_feed_count,
                 ( unsigned long ) s_hb_count[ HB_SENSOR ],
                 ( unsigned long ) s_hb_count[ HB_IO ] );

            for( uint32_t i = 0U; i < ROUND_COUNT; i++ )
            {
                LOG( "[hb] %s 恢复时刻 = %lu tick（0 表示未恢复）\n",
                     k_rounds[ i ].tag, ( unsigned long ) recovered_at[ i ] );
            }

            /* 栈水位读数只有"所有代码路径都跑过"才有意义，所以这里放在
             * 注入—检出—恢复全流程走完之后，而不是启动阶段。 */
            LOG( "[hb] 栈水位（余量/总量，字）：sensor=%lu/160 io=%lu/128 health=%lu/224"
                 "（判据：绝对 ≥32、相对 ≥25%%）\n",
                 ( unsigned long ) uxTaskGetStackHighWaterMark( s_h_sensor ),
                 ( unsigned long ) uxTaskGetStackHighWaterMark( s_h_io ),
                 ( unsigned long ) uxTaskGetStackHighWaterMark( s_h_health ) );
        }
    }
}

/* 把硬件喂狗包一层：QEMU 构建下不调用，但函数体始终参与编译，
 * 于是 IWDG 寄存器写入的位定义与类型错误能在构建阶段暴露。 */
void watchdog_feed_if_enabled( void )
{
#if ( F103_WATCHDOG_ENABLE != 0 )
    f103_watchdog_feed();
#endif
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

    uint32_t reset_flags = read_reset_flags();

    printf( "\r\n[F103_14] FreeRTOS %s heartbeat + watchdog, HCLK=%lu Hz, tick=%lu Hz\r\n",
            tskKERNEL_VERSION_NUMBER,
            ( unsigned long ) SystemCoreClock,
            ( unsigned long ) configTICK_RATE_HZ );
    printf( "[F103_14] HB: sensor=%u ms io=%u ms timeout=%u ms check=%u ms，"
            "注入轮次 %u（恢复门限 %u / %u 次 STALE）\r\n",
            ( unsigned ) HB_PERIOD_SENSOR_MS, ( unsigned ) HB_PERIOD_IO_MS,
            ( unsigned ) HB_TIMEOUT_MS, ( unsigned ) CHECK_PERIOD_MS,
            ( unsigned ) ROUND_COUNT,
            ( unsigned ) k_rounds[ 0 ].streak_limit,
            ( unsigned ) k_rounds[ ROUND_COUNT - 1U ].streak_limit );
    printf( "[F103_14] IWDG 路径 %s；RCC->CSR=0x%08lX（IWDGRSTF=%lu）\r\n",
            ( F103_WATCHDOG_ENABLE != 0 ) ? "已启用" : "未启用（QEMU 未建模 IWDG）",
            ( unsigned long ) reset_flags,
            ( unsigned long ) ( ( reset_flags & RCC_CSR_IWDGRSTF ) ? 1U : 0U ) );

    /* 只"编译进 .o"不足以证明硬件路径存在：-ffunction-sections + --gc-sections
     * 会在链接期把无人调用的函数整段删掉（本实验实测过：nm 在 .o 里能看到
     * f103_watchdog_*，在 ELF 里为空）。这里取地址打印，函数因此被真正引用，
     * 同时这条输出本身就是"寄存器写入已编入镜像"的可核对证据——
     * arm-none-eabi-objdump -d 里能查到 0x40003000 处的 IWDG 写入。 */
    printf( "[F103_14] IWDG 代码地址 init=0x%08lX feed=0x%08lX（本构建不执行）\r\n",
            ( unsigned long ) f103_watchdog_init,
            ( unsigned long ) f103_watchdog_feed );

    printf( "[F103_14] 监控通道 %u 个：", ( unsigned ) HB_CHANNELS );
    for( uint32_t i = 0U; i < HB_CHANNELS; i++ )
    {
        printf( "%s%s", ( i > 0U ) ? ", " : "", k_hb_name[ i ] );
    }
    printf( "（任一通道不健康即停喂狗）\r\n" );

    if( ( reset_flags & RCC_CSR_IWDGRSTF ) != 0U )
    {
        /* 真板路径：读完清标志，否则下次复位无法区分原因 */
        printf( "[F103_14] 上次复位原因 = 独立看门狗（IWDGRSTF）\n" );
        clear_reset_flags();
    }

    if( F103_WATCHDOG_ENABLE != 0 )
    {
        f103_watchdog_init();
        printf( "[F103_14] IWDG 已启动：PR=÷64 RLR=%u，名义超时约 998 ms\n",
                ( unsigned ) IWDG_RLR_NOMINAL_1S );
    }

    s_log_mutex = xSemaphoreCreateMutex();
    s_gate      = xSemaphoreCreateBinary();

    if( ( s_log_mutex == NULL ) || ( s_gate == NULL ) )
    {
        printf( "创建同步对象失败：heap 不足\r\n" );
        for( ;; )
        {
        }
    }

    for( uint32_t i = 0U; i < HB_CHANNELS; i++ )
    {
        s_hb_last[ i ] = ( uint32_t ) xTaskGetTickCount();
    }

    xTaskCreate( sensor_task, "sensor", 160, NULL, 3, &s_h_sensor );
    xTaskCreate( io_task,     "io",     128, NULL, 2, &s_h_io );
    /* health 用 224 字：它每行最多打印 5 个数字，printf 的整型格式化路径较深，
     * 实验 12/13 实测同类打印任务用掉约 102~110 字。 */
    xTaskCreate( health_task, "health", 224, NULL, 1, &s_h_health );

    printf( "[F103_14] 三个任务已创建，堆余量 %u B，启动调度器\r\n",
            ( unsigned ) xPortGetFreeHeapSize() );

    vTaskStartScheduler();

    printf( "调度器未能启动（堆不足？）\r\n" );
    for( ;; )
    {
    }
}
