/*
 * FreeRTOSConfig.h —— STM32F103C8T6 / Cortex-M3 裸机实验用配置
 *
 * 内核版本：FreeRTOS-Kernel V11.3.0（来源与裁剪范围见 ../../README.md）
 * 移植层：  portable/GCC/ARM_CM3
 *
 * 取舍原则：只打开本阶段实验（任务、队列、信号量）需要的功能，
 * 关掉软件定时器、事件组、流缓冲和静态分配，避免把未使用的代码链进 64 KB Flash。
 */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ---- 调度器 ------------------------------------------------------------- */
#define configUSE_PREEMPTION                    1
#define configUSE_TIME_SLICING                  1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1   /* CM3 有 CLZ，要求 configMAX_PRIORITIES <= 32 */
#define configMAX_PRIORITIES                    5
#define configNUMBER_OF_CORES                   1

/* ---- 时基 --------------------------------------------------------------- */
/*
 * 8 MHz：HSI 直连，不配 PLL，与本仓库其余实验一致。
 * 注意 QEMU 的 stm32vldiscovery 以 24 MHz 驱动 SYSCLK，仿真里节拍会比真实时间快约 3 倍，
 * 相关实测与限制见 Linux/tests/qemu-stm32/RESULT.md。
 */
#define configCPU_CLOCK_HZ                      8000000UL
#define configTICK_RATE_HZ                      1000U
#define configTICK_TYPE_WIDTH_IN_BITS           TICK_TYPE_WIDTH_32_BITS
#define configUSE_TICKLESS_IDLE                 0

/* ---- 内存 --------------------------------------------------------------- */
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configSUPPORT_STATIC_ALLOCATION         0
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) ( 4U * 1024U ) )
#define configAPPLICATION_ALLOCATED_HEAP        0
#define configMINIMAL_STACK_SIZE                ( ( uint16_t ) 128U )
#define configSTACK_DEPTH_TYPE                  uint16_t

/* ---- 任务 / 队列 / 同步 -------------------------------------------------- */
#define configUSE_TASK_NOTIFICATIONS            1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES   1
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configQUEUE_REGISTRY_SIZE               0
#define configUSE_QUEUE_SETS                    0
#define configMESSAGE_BUFFER_LENGTH_TYPE        size_t
#define configUSE_STREAM_BUFFERS                0

/* 软件定时器由目标按需开启：只有定义了 F103_FREERTOS_USE_TIMERS=1 的固件才链接 timers.c。
 * 这样实验 10–12 的体积与行为保持不变，实验 13 用 -DF103_FREERTOS_USE_TIMERS=1 单独打开。 */
#ifndef F103_FREERTOS_USE_TIMERS
#define F103_FREERTOS_USE_TIMERS            0
#endif
#define configUSE_TIMERS                    ( F103_FREERTOS_USE_TIMERS )

/* 以下三项只在 configUSE_TIMERS == 1 时被引用；先给出取值，避免开关切换时再改一遍。
 * 守护任务优先级取最高的 4：定时器命令与到期回调都由它执行，优先级低了会让到期时间漂移。
 * 队列长度 5：同一 tick 内最多缓存 5 条命令（启动/停止/复位/改周期），溢出时 API 返回 pdFAIL。
 * 守护任务栈 160 字：实测 128 字时水位只剩 28 字，低于"绝对 ≥32 字"的栈判据（见实验 13）。 */
#define configTIMER_TASK_PRIORITY           ( configMAX_PRIORITIES - 1 )
#define configTIMER_QUEUE_LENGTH            5
#define configTIMER_TASK_STACK_DEPTH        ( 160 )

/* ---- 可选功能（本阶段一律关闭，减少体积） -------------------------------- */
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configUSE_PASSIVE_IDLE_HOOK             0
#define configUSE_MALLOC_FAILED_HOOK            1
#define configCHECK_FOR_STACK_OVERFLOW          2
#define configUSE_TRACE_FACILITY                0
#define configUSE_STATS_FORMATTING_FUNCTIONS    0
#define configGENERATE_RUN_TIME_STATS           0
#define configUSE_NEWLIB_REENTRANT              0
#define configENABLE_BACKWARD_COMPATIBILITY     0

/* ---- 按需打开的 API ------------------------------------------------------ */
/* V11 的 INCLUDE_* 默认全是 0：不开就会出现 "undefined reference to vTaskDelay" */
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskDelayUntil                 1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     1

/* ---- 中断优先级 ---------------------------------------------------------- */
/*
 * STM32F1 的 NVIC 实现 4 位优先级（RM0008 6.1.2），优先级数值越小越高。
 * configMAX_SYSCALL_INTERRUPT_PRIORITY = 5 << 4：数值 0~4 的中断不受临界区屏蔽，
 * 也因此不允许调用任何 FreeRTOS API。串口中断按 5 配置即可调用 ...FromISR()。
 */
#define configPRIO_BITS                         4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY     15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5
#define configKERNEL_INTERRUPT_PRIORITY \
    ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )

/* ---- 断言与句柄安装检查 -------------------------------------------------- */
/*
 * configCHECK_HANDLER_INSTALLATION = 0：本仓库采用**间接路由**。
 * 启动文件里向量表固定写 SVC_Handler / PendSV_Handler / SysTick_Handler，
 * 由 freertos_vectors.s 用 `b` 尾跳转到 vPortSVCHandler / xPortPendSVHandler /
 * xPortSysTickHandler。此时向量表里的地址与移植层函数地址不同，
 * port.c 的"直接路由"校验必然失败，故按官方说明关闭。
 * 若哪天把启动文件的向量表直接指向移植层函数，应改回 1。
 */
#define configCHECK_HANDLER_INSTALLATION        0

/* 断言失败：关中断并停在可被调试器看到的位置；串口不可用时不依赖 printf */
void vAssertCalled( const char *pcFile, unsigned long ulLine );
#define configASSERT( x )                                                    \
    do {                                                                     \
        if( ( x ) == 0 ) {                                                   \
            vAssertCalled( __FILE__, __LINE__ );                             \
        }                                                                    \
    } while( 0 )

#endif /* FREERTOS_CONFIG_H */
