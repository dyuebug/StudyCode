# F103_20：逻辑分析仪（8 通道、buffer 模式）
#
# 与其他实验的唯一差别：本工程有两个编译单元。
#   la_core.c 与硬件无关，同一份源码也被 host/ 下的两个判定程序链接，
#   所以"板上跑的"与"主机端测过的"不是两套近似实现。
#
# QEMU 下自动打开 LA_SYNTHETIC=1：
#   stm32vldiscovery 不建模 GPIO 输入与 TIM 计数（见 tests/qemu-stm32/logs/
#   neg_f103_02_gpio_input.log、neg_f103_09_delay_precision.log），
#   采样源若仍取真实引脚，仿真里恒为 0，"跑通"就成了假象。
#   真板构建时该宏为 0，读 PA0..PA7。
cmake_minimum_required(VERSION 3.20)
include("${CMAKE_CURRENT_SOURCE_DIR}/../F103_common/cmake/f103.cmake")
project(f103_20_logicanalyzer LANGUAGES C ASM)

set(LA_DEFS)
if(F103_QEMU)
  list(APPEND LA_DEFS LA_SYNTHETIC=1)
endif()

f103_add_firmware(f103_20_logicanalyzer
  SOURCES  main.c la_core.c
  DEFINES  ${LA_DEFS})

f103_add_flash_target(f103_20_logicanalyzer)

# ---- 主机端判定程序不写进本文件 -----------------------------------------
# 本工程用 arm-none-eabi 工具链，add_executable 出来的主机目标也会被交叉编译器
# 编成 ARM 二进制，在 Windows/Linux 上跑不起来。
# 所以 host/ 下的两个程序用宿主 gcc 单独编译（与 F103_HAL_DriverLab 的做法一致）：
#   gcc -std=c11 -Wall -Wextra -O2 -I. -o host/lab_la_proto host/lab_la_proto.c la_core.c
#   gcc -std=c11 -Wall -Wextra -O2 -I. -o host/lab_la_arith host/lab_la_arith.c la_core.c
