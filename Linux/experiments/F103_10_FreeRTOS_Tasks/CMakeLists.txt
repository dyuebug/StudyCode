cmake_minimum_required(VERSION 3.20)

# NO_TICK：不链接 f103_tick.c —— SysTick_Handler 由 FreeRTOS 接管，
# 两者同时存在会符号重定义。毫秒延时改用 vTaskDelay。
include("${CMAKE_CURRENT_SOURCE_DIR}/../F103_common/cmake/f103.cmake")

project(f103_10_freertos_tasks LANGUAGES C ASM)

f103_add_firmware(f103_10_freertos_tasks
  NO_TICK
  SOURCES
    main.c)

# SVC/PendSV/SysTick 的尾跳转路由由 f103_add_freertos() 统一加入
# （F103_common/src/f103_freertos_vectors.s），实验代码不必重复实现。
f103_add_freertos(f103_10_freertos_tasks)
f103_add_flash_target(f103_10_freertos_tasks)
