cmake_minimum_required(VERSION 3.20)

# 复用 F103_common 的启动文件、CMSIS 子集、SysTick/UART 驱动与编译选项；
# -DF103_QEMU=ON 时链接脚本切换为 STM32F100RB（QEMU stm32vldiscovery）布局。
include("${CMAKE_CURRENT_SOURCE_DIR}/../../../experiments/F103_common/cmake/f103.cmake")

project(qemu_stm32_probe LANGUAGES C ASM)

f103_add_firmware(qemu_stm32_probe
  SOURCES main.c)
