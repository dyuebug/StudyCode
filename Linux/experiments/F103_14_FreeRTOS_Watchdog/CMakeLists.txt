cmake_minimum_required(VERSION 3.20)

# NO_TICK：SysTick_Handler 归 FreeRTOS，公共 f103_tick.c 不能同时链接。
include("${CMAKE_CURRENT_SOURCE_DIR}/../F103_common/cmake/f103.cmake")

project(f103_14_freertos_watchdog LANGUAGES C ASM)

# 本实验不需要 timers / event_groups：心跳用任务自身 + tick 比较实现，
# 不依赖任何额外内核模块，所以 f103_add_freertos 不带 MODULES。
f103_add_firmware(f103_14_freertos_watchdog
  NO_TICK
  SOURCES
    main.c)

f103_add_freertos(f103_14_freertos_watchdog)
f103_add_flash_target(f103_14_freertos_watchdog)
