cmake_minimum_required(VERSION 3.20)

include("${CMAKE_CURRENT_SOURCE_DIR}/../F103_common/cmake/f103.cmake")

project(f103_01_gpio_blink LANGUAGES C ASM)

f103_add_firmware(f103_01_gpio_blink
  SOURCES main.c)

f103_add_flash_target(f103_01_gpio_blink)
