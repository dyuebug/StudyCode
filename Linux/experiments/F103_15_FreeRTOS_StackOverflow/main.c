/*
 * F103_15：FreeRTOS 栈溢出检出（让 vApplicationStackOverflowHook 真的触发一次）
 *
 * 目标：把"栈溢出能被发现"从"配置里写了 configCHECK_FOR_STACK_OVERFLOW = 2"
 * 变成"注入一次溢出，钩子确实报出正确的任务名"。分三件可分别判定的事：
 *   1) 对照：安全深度下钩子不触发，且 uxTaskGetStackHighWaterMark 给出可信余量；
 *   2) 标定：用两次探测的最低栈地址差算出每帧字节数，避免"猜一个深度"；
 *   3) 注入：逐级加深递归，以 uxTaskGetStackHighWaterMark 归零作为"栈底 16 字节
 *      填充已被写穿"的判据（不按地址反推——那样会差几个字节，首轮实测因此漏检），
 *      之后钩子在下一次任务切换时报出任务名并停机。
 *
 * 只触发检出的 paint 那一半：任务返回后栈指针本身合法，被破坏的是栈块底部
 * 16 字节的 0xA5 填充（configCHECK_FOR_STACK_OVERFLOW = 2 的第一个检查）。
 * SP 越界那一半要求栈指针真的越过栈块，会先撞坏相邻的堆块，本实验不做——
 * 那属于"溢出未被及时检出"的故障，不是这篇要证的东西。
 *
 * 平台：STM32F103（Cortex-M3，栈向下增长），FreeRTOS heap_4。
 * 仿真边界：本实验只依赖调度器、tick 与 UART 打印，QEMU 可实测；
 * 结论不涉及任何绝对时间，因此不受 2.90 倍时基影响。
 */
#include <stdint.h>
#include <stdio.h>

#include "stm32f1xx.h"
#include "f103_uart.h"

#include "FreeRTOS.h"
#include "task.h"

#define UART_BAUD            115200U

/* 256 字 = 1 KB：够放下探测递归，又能让由余量反推的深度落在两位数，
 * 日志里可以直接对着数，不必相信我给的常数。 */
#define PROBE_STACK_WORDS    256U
#define SAFE_DEPTH           12U    /* 对照深度：远不到栈底 */
#define MEASURE_STEP         4U     /* 第二个采样点比对照深几帧，用来算每帧字节数 */
#define PROBE_PRIO           2U

/* 栈底 16 字节（4 个字）是 paint 区：configCHECK_FOR_STACK_OVERFLOW == 2 的第一半
 * 判据就比较这 4 个字是否仍为 0xA5A5A5A5。本实验用"余量归零"间接证明这些字节
 * 已被写穿，不再单独为 paint 定义常量。 */

/* 探测递归。
 *
 * volatile 数组让每帧真的占内存；用返回值参与运算、并标 noinline，是为了阻止
 * 编译器把递归变成尾调用或循环——帧消失后"每帧字节数"会算成 0，
 * 那说明的是优化生效，不是栈够用。 */
__attribute__( ( noinline ) ) static uint32_t descend( uint32_t depth, uintptr_t *lowest )
{
    volatile uint32_t pad[ 2 ];
    pad[ 0 ] = depth;
    pad[ 1 ] = depth ^ 0xA5A5A5A5U;

    uintptr_t here = ( uintptr_t ) &pad[ 0 ];
    if( here < *lowest )
    {
        *lowest = here;
    }

    if( depth == 0U )
    {
        return pad[ 1 ];
    }

    uint32_t inner = descend( depth - 1U, lowest );

    return inner ^ pad[ 0 ];
}

/* 一次探测：返回最低栈地址，depth 由调用者给。 */
static uintptr_t probe( uint32_t depth )
{
    uintptr_t lowest = UINTPTR_MAX;

    ( void ) descend( depth, &lowest );

    return lowest;
}

/* 为什么用"余量归零"而不是"按地址反推栈底"：
 *
 * uxTaskGetStackHighWaterMark 的实现是从 pxStack 往高地址逐字节扫 0xa5
 * （tasks.c 的 prvTaskCheckFreeStackSpace），扫到第一个非 0xa5 字节即停，
 * 剩余字节数 / 4 就是返回值。于是"余量 == 0"直接等价于"pxStack 处已被写过"，
 * 也就是检出宏里 pulStack[0] != 0xa5a5a5a5 那一半必然成立。
 *
 * 反过来，用"最低局部变量地址 - 余量×4"反推 pxStack 会差几个字节：帧里除了
 * 局部数组还有保存的寄存器，最低被写字节并不等于最低局部变量地址。首版就是
 * 这么算的，注入点落在 paint 边界外一个字，钩子没有触发——那次失败保留了
 * 结论：判据要用"余量"，不要用地址算术。
 */
static void probe_task( void *arg )
{
    ( void ) arg;

    printf( "\r\n[so] 任务栈 %lu 字（%lu B）\r\n",
            ( unsigned long ) PROBE_STACK_WORDS,
            ( unsigned long ) ( PROBE_STACK_WORDS * 4U ) );

    /* ---- 对照：安全深度，钩子此时不该触发 ---- */
    uintptr_t low_safe = probe( SAFE_DEPTH );
    uint32_t  hwm_safe = uxTaskGetStackHighWaterMark( NULL );

    printf( "[so] 对照 depth=%lu → 最低地址 0x%08lX，余量 %lu 字（判据 ≥32）\r\n",
            ( unsigned long ) SAFE_DEPTH,
            ( unsigned long ) low_safe,
            ( unsigned long ) hwm_safe );

    /* ---- 标定每帧字节数 ---- */
    uintptr_t low_deep = probe( SAFE_DEPTH + MEASURE_STEP );
    uint32_t  frame_bytes = 0U;

    if( low_deep < low_safe )
    {
        frame_bytes = ( uint32_t ) ( low_safe - low_deep ) / MEASURE_STEP;
    }

    printf( "[so] 标定 depth=%lu → 最低地址 0x%08lX，每帧 %lu B\r\n",
            ( unsigned long ) ( SAFE_DEPTH + MEASURE_STEP ),
            ( unsigned long ) low_deep,
            ( unsigned long ) frame_bytes );

    if( frame_bytes == 0U )
    {
        /* 帧被优化掉时不能继续：继续下去等于用一个假的字节数算深度。 */
        printf( "[so] FAIL 每帧字节数为 0，递归可能被优化成循环/尾调用，本实验结论无效\r\n" );
        for( ;; )
        {
            vTaskDelay( 1000 );
        }
    }

    /* 粗估（只用于和实测对照，不参与判定）：
     * "当前余量 ÷ 标定帧长 = 还能深几帧"。它必然会偏小，实测偏了 8 帧，原因清楚：
     * depth 12 / 16 / 20 三档的余量实测都是 160 字——这 8 帧的写入落在已经被
     * 上层调用写过的区域里，不消耗余量。估算把它们算成了消耗，于是归零点被提前。
     * 另一条次要原因：余量以 4 字节为粒度，帧与 paint 边界的相位每帧都在变。
     * 结论：估算只能给量级，判据必须用余量归零本身。 */
    uint32_t predicted = SAFE_DEPTH + ( hwm_safe * 4U ) / frame_bytes;

    /* ---- 逐级加深，直到 paint 被写穿 ----
     * 粗调每步 4 帧，余量小于 8 字后改为每步 1 帧：越界量越小，越不容易
     * 撞坏栈块下方的堆控制块。
     *
     * 每步都打印读数，用来核对"余量是否按标定帧长线性递减"。这一步是必要的：
     * 归零点总比"按余量粗估"晚几帧（当前版本 8 帧），只看终点无法判断是帧长
     * 标定错了、还是余量本身不是线性的。printf 的帧位于 probe_task 当前栈顶附近，
     * 深度区域在它下面几百字节处，不会被它提前写脏——所以打印不会污染判据。 */
    uint32_t depth       = SAFE_DEPTH + MEASURE_STEP;
    uint32_t hwm         = uxTaskGetStackHighWaterMark( NULL );
    uint32_t depth_low32 = 0U;
    uint32_t depth_zero  = 0U;

    printf( "[so] depth=%lu 余量 %lu 字\r\n", ( unsigned long ) depth, ( unsigned long ) hwm );

    while( ( hwm > 0U ) && ( depth < PROBE_STACK_WORDS ) )
    {
        depth += ( hwm > 8U ) ? MEASURE_STEP : 1U;

        ( void ) probe( depth );
        hwm = uxTaskGetStackHighWaterMark( NULL );

        printf( "[so] depth=%lu 余量 %lu 字\r\n", ( unsigned long ) depth, ( unsigned long ) hwm );

        if( ( depth_low32 == 0U ) && ( hwm < 32U ) )
        {
            depth_low32 = depth;
        }

        if( hwm == 0U )
        {
            depth_zero = depth;
        }
    }

    printf( "[so] 逐级加深：首次余量 <32 字在 depth=%lu，余量归零在 depth=%lu"
            "（按余量预测 %lu）\r\n",
            ( unsigned long ) depth_low32,
            ( unsigned long ) depth_zero,
            ( unsigned long ) predicted );

    if( depth_zero == 0U )
    {
        printf( "[so] FAIL 加深到 %lu 帧仍未写穿 paint，判据或栈大小有问题\r\n",
                ( unsigned long ) depth );
        for( ;; )
        {
            vTaskDelay( 1000 );
        }
    }

    printf( "[so] 注入完成：depth=%lu 时 hwm=0，即栈底 16 B 的 0xA5 已被写穿；"
            "越界量不超过一帧（%lu B）\r\n",
            ( unsigned long ) depth_zero, ( unsigned long ) frame_bytes );

    /* 检出发生在下一次任务切换：检查宏测的是"要换出去的那个任务"的 paint，
     * probe_task 返回后栈指针合法，只有切换时才看得到被破坏的填充。
     * 这里必须真的阻塞一次——忙等触发不了切换，钩子也就没机会运行。 */
    printf( "[so] 返回后栈指针合法；vTaskDelay(1) 等待切换时的 paint 检查\r\n" );
    vTaskDelay( 1 );

    /* 走到这里说明检出失效。不能"安静地正常"结束——那会让日志看起来像通过。 */
    printf( "[so] FAIL 切换后钩子未触发：configCHECK_FOR_STACK_OVERFLOW 未生效，"
            "或注入未真正写穿 paint\r\n" );

    for( ;; )
    {
        vTaskDelay( 1000 );
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
    printf( "\r\n[STACK OVERFLOW] 任务 %s 的栈被写穿，钩子已触发\r\n", pcTaskName );
    for( ;; )
    {
    }
}

/* ---- 入口 ---------------------------------------------------------------- */

int main( void )
{
    SystemCoreClockUpdate();
    f103_uart_init( UART_BAUD );

    printf( "\r\n[F103_15] FreeRTOS %s stack-overflow detection, HCLK=%lu Hz, tick=%lu Hz\r\n",
            tskKERNEL_VERSION_NUMBER,
            ( unsigned long ) SystemCoreClock,
            ( unsigned long ) configTICK_RATE_HZ );
    printf( "[F103_15] configCHECK_FOR_STACK_OVERFLOW=%d（2 = paint + SP 双重检查），"
            "任务栈 %u 字\r\n",
            ( int ) configCHECK_FOR_STACK_OVERFLOW,
            ( unsigned ) PROBE_STACK_WORDS );

    if( xTaskCreate( probe_task, "probe", PROBE_STACK_WORDS, NULL, PROBE_PRIO, NULL ) != pdPASS )
    {
        printf( "[F103_15] 任务创建失败：堆不足\r\n" );
        for( ;; )
        {
        }
    }

    vTaskStartScheduler();

    printf( "[F103_15] 调度器未能启动（堆不足？）\r\n" );
    for( ;; )
    {
    }
}
