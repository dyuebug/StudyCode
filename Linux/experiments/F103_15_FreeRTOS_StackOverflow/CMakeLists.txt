cmake_minimum_required(VERSION 3.20)

# NO_TICK：SysTick_Handler 归 FreeRTOS，公共 f103_tick.c 不能同时链接。
include("${CMAKE_CURRENT_SOURCE_DIR}/../F103_common/cmake/f103.cmake")

project(f103_15_freertos_stackoverflow LANGUAGES C ASM)

# 本实验只用任务与 tick，不需要队列/信号量/定时器模块。
f103_add_firmware(f103_15_freertos_stackoverflow
  NO_TICK
  SOURCES
    main.c)

f103_add_freertos(f103_15_freertos_stackoverflow)
f103_add_flash_target(f103_15_freertos_stackoverflow)
