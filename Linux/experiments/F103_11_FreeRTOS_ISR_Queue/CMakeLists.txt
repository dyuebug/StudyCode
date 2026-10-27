cmake_minimum_required(VERSION 3.20)

# NO_TICK：SysTick_Handler 归 FreeRTOS，公共 f103_tick.c 不能同时链接。
include("${CMAKE_CURRENT_SOURCE_DIR}/../F103_common/cmake/f103.cmake")

project(f103_11_freertos_isr_queue LANGUAGES C ASM)

# SVC/PendSV/SysTick 的尾跳转路由由 f103_add_freertos() 统一加入
# （F103_common/src/f103_freertos_vectors.s），本实验不重复实现。
f103_add_firmware(f103_11_freertos_isr_queue
  NO_TICK
  SOURCES
    main.c)

f103_add_freertos(f103_11_freertos_isr_queue)
f103_add_flash_target(f103_11_freertos_isr_queue)
