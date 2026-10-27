cmake_minimum_required(VERSION 3.20)
include("${CMAKE_CURRENT_SOURCE_DIR}/../F103_common/cmake/f103.cmake")

project(f103_16_freertos_priority_inversion LANGUAGES C ASM)

f103_add_firmware(f103_16_freertos_priority_inversion NO_TICK SOURCES main.c)
f103_add_freertos(f103_16_freertos_priority_inversion)
f103_add_flash_target(f103_16_freertos_priority_inversion)
