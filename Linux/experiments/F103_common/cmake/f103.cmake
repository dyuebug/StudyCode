# F103 裸机工程公共构建规则
#
# 在实验的 CMakeLists.txt 中这样使用：
#   cmake_minimum_required(VERSION 3.20)
#   include("${CMAKE_CURRENT_SOURCE_DIR}/../F103_common/cmake/f103.cmake")
#   project(f103_01_gpio_blink C ASM)
#   f103_add_firmware(f103_01_gpio_blink SOURCES main.c)
#
# 该函数负责：启动文件、系统初始化、链接脚本、Cortex-M3 编译选项、
# hex/bin 产物转换和体积报告。实验只需提供自己的源文件。

set(F103_COMMON_ROOT "${CMAKE_CURRENT_LIST_DIR}/.."
    CACHE INTERNAL "F103_common 目录")

set(F103_LDSCRIPT "${F103_COMMON_ROOT}/ld/STM32F103C8Tx.ld")

# QEMU 的 stm32vldiscovery 建模的是 STM32F100RB（128 KB Flash / 8 KB SRAM），
# 沿用 F103C8T6 的 _estack = 0x20005000 会在第一次压栈时 BusFault。
# 该选项只切换链接脚本，便于做"能跑起来"的仿真验证；
# 仿真能力边界见 Linux/tests/qemu-stm32/README.md。
option(F103_QEMU "按 STM32F100RB（QEMU stm32vldiscovery）的内存布局构建" OFF)
if(F103_QEMU)
  set(F103_LDSCRIPT "${F103_COMMON_ROOT}/ld/STM32F100RB_qemu.ld")
endif()
set(F103_STARTUP  "${F103_COMMON_ROOT}/vendor/startup/startup_stm32f103xb.s")
set(F103_SYSTEM   "${F103_COMMON_ROOT}/vendor/src/system_stm32f1xx.c")
set(F103_CRTSTUB  "${F103_COMMON_ROOT}/src/crt_stubs.c")
set(F103_TICK     "${F103_COMMON_ROOT}/src/f103_tick.c")
set(F103_UART     "${F103_COMMON_ROOT}/src/f103_uart.c")

# Cortex-M3 无 FPU：浮点一律走软件库，避免链接期找不到硬件浮点指令
set(F103_CPU_FLAGS -mcpu=cortex-m3 -mthumb -mfloat-abi=soft)

function(f103_add_firmware TARGET)
  cmake_parse_arguments(ARG "" "LDSCRIPT" "SOURCES;INCLUDES;DEFINES" ${ARGN})

  if(NOT ARG_SOURCES)
    message(FATAL_ERROR "f103_add_firmware(${TARGET}) 需要 SOURCES 参数")
  endif()

  if(ARG_LDSCRIPT)
    set(_f103_ldscript "${ARG_LDSCRIPT}")
  else()
    set(_f103_ldscript "${F103_LDSCRIPT}")
  endif()

  add_executable(${TARGET}
    ${ARG_SOURCES}
    ${F103_STARTUP}
    ${F103_SYSTEM}
    ${F103_CRTSTUB}
    ${F103_TICK}
    ${F103_UART})

  target_include_directories(${TARGET} PRIVATE
    "${F103_COMMON_ROOT}/include"
    "${F103_COMMON_ROOT}/vendor/cmsis/core"
    "${F103_COMMON_ROOT}/vendor/cmsis/device"
    ${ARG_INCLUDES})

  # STM32F103xB 是 CMSIS 设备头的型号选择宏，必须与启动文件/链接脚本一致
  target_compile_definitions(${TARGET} PRIVATE
    STM32F103xB
    ${ARG_DEFINES})

  target_compile_options(${TARGET} PRIVATE
    ${F103_CPU_FLAGS}
    -ffunction-sections -fdata-sections
    -Wall -Wextra
    -g3 -Og)

  target_link_options(${TARGET} PRIVATE
    ${F103_CPU_FLAGS}
    -T${_f103_ldscript}
    # 自带启动文件，不需要 crt0/crti；同时避免 crt0 拉入 stdio 与 syscall stub
    -nostartfiles
    --specs=nano.specs
    --specs=nosys.specs
    -Wl,--gc-sections
    # .init_array/.fini_array 物理位于 Flash 且带可写标志，ld 会据此报 RWX 段。
    # 裸机没有 MMU，段权限不产生保护语义，属工具链提示而非缺陷。
    -Wl,--no-warn-rwx-segments
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.map,--cref
    -Wl,--print-memory-usage)

  # 不生成 lib<target>.a 之类中间库，也不给产物加平台后缀
  set_target_properties(${TARGET} PROPERTIES SUFFIX ".elf")

  add_custom_command(TARGET ${TARGET} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:${TARGET}> ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${TARGET}> ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${TARGET}>
    COMMENT "生成 ${TARGET}.hex / ${TARGET}.bin")
endfunction()

# ---- 烧写（可选）------------------------------------------------------------
# openocd 定位顺序：-DF103_OPENOCD=<路径> → PATH → WinGet 的 xpack 安装目录。
# 找不到时不注册 flash 目标，构建流程不受影响。
if(NOT F103_OPENOCD)
  find_program(F103_OPENOCD openocd)
endif()
if(NOT F103_OPENOCD AND DEFINED ENV{LOCALAPPDATA})
  file(GLOB _f103_openocd_candidates
    "$ENV{LOCALAPPDATA}/Microsoft/WinGet/Packages/xpack-dev-tools.openocd-xpack_*/xpack-openocd-*/bin/openocd.exe")
  if(_f103_openocd_candidates)
    list(GET _f103_openocd_candidates 0 F103_OPENOCD)
  endif()
endif()

function(f103_add_flash_target TARGET)
  if(NOT F103_OPENOCD)
    message(STATUS "未找到 openocd，跳过 ${TARGET} 的 flash 目标"
                   "（可传 -DF103_OPENOCD=<openocd 路径>）")
    return()
  endif()

  add_custom_target(flash
    COMMAND ${F103_OPENOCD}
            -f "${F103_COMMON_ROOT}/openocd/stlink_f103.cfg"
            -c "program $<TARGET_FILE:${TARGET}> verify reset exit"
    DEPENDS ${TARGET}
    COMMENT "通过 ST-Link 烧写 ${TARGET}"
    VERBATIM)
endfunction()
