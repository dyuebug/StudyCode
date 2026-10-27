cmake_minimum_required(VERSION 3.20)

# NO_TICK：SysTick_Handler 归 FreeRTOS，公共 f103_tick.c 不能同时链接。
include("${CMAKE_CURRENT_SOURCE_DIR}/../F103_common/cmake/f103.cmake")

project(f103_13_freertos_timers LANGUAGES C ASM)

# F103_FREERTOS_USE_TIMERS=1 是本工程独有的开关：它让 FreeRTOSConfig.h 里
# configUSE_TIMERS 变成 1，内核才会创建守护任务并编译 timers.c 的入口。
# 实验 10–12 不定义该宏，configUSE_TIMERS 仍为 0，体积与行为不受影响。
f103_add_firmware(f103_13_freertos_timers
  NO_TICK
  SOURCES
    main.c
  DEFINES
    F103_FREERTOS_USE_TIMERS=1)

# MODULES timers：软件定时器的实现单独成文件，不显式加入会在链接期报
# undefined reference to xTimerCreate。
f103_add_freertos(f103_13_freertos_timers MODULES timers)
f103_add_flash_target(f103_13_freertos_timers)
