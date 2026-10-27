/*
 * FreeRTOS 异常处理入口（间接路由）—— 公共文件，由 f103_add_freertos() 自动加入
 *
 * 为什么需要这个文件：
 *   启动文件 startup_stm32f103xb.s 里向量表写死了 SVC_Handler / PendSV_Handler /
 *   SysTick_Handler 三个名字，而 FreeRTOS 的 CM3 移植层提供的是
 *   vPortSVCHandler / xPortPendSVHandler / xPortSysTickHandler。
 *   两者名字不同，必须由固件把前者接到后者上。
 *
 * 为什么用汇编 `b`（尾跳转）而不是 C 包装函数：
 *   vPortSVCHandler 与 xPortPendSVHandler 是 naked 函数，结尾用
 *   `bx r14`（r14 里是 EXC_RETURN）直接回到任务上下文，永远不会 `pop` 返回。
 *   如果写成 `void SVC_Handler(void) { vPortSVCHandler(); }`，编译器会为包装函数
 *   压栈，然后这份栈帧再也没有机会弹出——PendSV 每个 tick 都进一次，MSP 会一路下移，
 *   最终踩穿栈底。`b` 不产生栈帧，等价于"向量表直接指向移植层函数"。
 *
 * 代价：port.c 的 configCHECK_HANDLER_INSTALLATION 校验会认为向量表没直接指向
 * vPortSVCHandler，所以 FreeRTOSConfig.h 里把它设为 0（即官方文档说的间接路由）。
 *
 * 使用方：F103_10_FreeRTOS_Tasks、F103_11_FreeRTOS_ISR_Queue（无需在实验里重复实现）。
 */
    .syntax unified
    .cpu cortex-m3
    .thumb

    .section .text.SVC_Handler,"ax",%progbits
    .global SVC_Handler
    .type SVC_Handler, %function
SVC_Handler:
    b vPortSVCHandler
    .size SVC_Handler, . - SVC_Handler

    .section .text.PendSV_Handler,"ax",%progbits
    .global PendSV_Handler
    .type PendSV_Handler, %function
PendSV_Handler:
    b xPortPendSVHandler
    .size PendSV_Handler, . - PendSV_Handler

    .section .text.SysTick_Handler,"ax",%progbits
    .global SysTick_Handler
    .type SysTick_Handler, %function
SysTick_Handler:
    b xPortSysTickHandler
    .size SysTick_Handler, . - SysTick_Handler
