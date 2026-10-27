cmake_minimum_required(VERSION 3.20)

# NO_TICK：SysTick_Handler 归 FreeRTOS，公共 f103_tick.c 不能同时链接。
include("${CMAKE_CURRENT_SOURCE_DIR}/../F103_common/cmake/f103.cmake")

project(f103_12_freertos_sync LANGUAGES C ASM)

f103_add_firmware(f103_12_freertos_sync
  NO_TICK
  SOURCES
    main.c)

# MODULES event_groups：f103_add_freertos() 默认只链接 list/queue/tasks，
# 事件组的实现单独成文件，必须显式加入，否则 xEventGroupCreate 报未定义引用。
f103_add_freertos(f103_12_freertos_sync MODULES event_groups)
f103_add_flash_target(f103_12_freertos_sync)
