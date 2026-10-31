# F103_HAL_DriverLab —— 官方驱动库的 GCC/CMake 构建
#
# 与课程原始工程的区别只体现在构建系统：
#   - 课程：Keil MDK（RVDS 移植层、ARMCC 启动文件、.sct 链接脚本）
#   - 本工程：arm-none-eabi-gcc（GCC 启动文件、.ld 链接脚本、CMake+Ninja）
# 源码层面（Core/ 的 MX_xxx_Init、Drivers/DshanMCU-F103 的 driver_*.c）与课程一致。
#
# 用法：
#   cmake -S . -B build -G Ninja \
#     -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake
#   cmake --build build

cmake_minimum_required(VERSION 3.20)

include("${CMAKE_CURRENT_SOURCE_DIR}/../F103_common/cmake/arm-none-eabi.cmake")

project(f103_hal_driverlab C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(HAL_DIR     "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32F1xx_HAL_Driver")
set(DRV_DIR     "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/DshanMCU-F103")
set(CMSIS_DEV   "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32F1xx/Include")
set(CMSIS_CORE  "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS/Include")

# ---- CubeMX 生成的外设初始化 -------------------------------------------------
# 这些文件由 STM32CubeMX 维护，不要手改；改动会被重新生成覆盖。
set(CORE_SOURCES
  Core/Src/main.c
  Core/Src/gpio.c
  Core/Src/usart.c
  Core/Src/i2c.c
  Core/Src/spi.c
  Core/Src/adc.c
  Core/Src/tim.c
  Core/Src/stm32f1xx_it.c
  Core/Src/stm32f1xx_hal_msp.c
  Core/Src/stm32f1xx_hal_timebase_tim.c
  Core/Src/system_stm32f1xx.c)

# ---- 运行时补丁（非 CubeMX 生成，重新生成不会覆盖）---------------------------
# crt_stubs.c：-nostartfiles 后缺失的 _init/_fini 与 libnosys 的 syscall 桩
# retarget.c ：printf → USART1，依赖 huart1，故与 usart.c 同时加入
# lab_pins.c ：驱动要求、但模板 gpio.c 没配的纯 GPIO（PB0/PB1/PB8/PB9/PB10/PB12/PB13/PA1）
#              与 EXTI15_10 的 NVIC 使能，理由见文件头注释
# hanzi_font.c / lab_oled_hanzi.c：教材 8.5 的 16x16 汉字显示，课程驱动无此功能。
#              hanzi_font.c 不含 HAL，主机端 lab_oled_hanzi/render_hanzi.c 也编译它，
#              以此保证"固件里画的"和"终端里验的"是同一份点阵数据。
set(RUNTIME_SOURCES
  Core/Src/crt_stubs.c
  Core/Src/retarget.c
  Core/Src/lab_pins.c
  Core/Src/hanzi_font.c
  Core/Src/lab_oled_hanzi.c)

# ---- HAL 模块 ---------------------------------------------------------------
# 必须与 Core/Inc/stm32f1xx_hal_conf.h 中启用的 HAL_xxx_MODULE_ENABLED 一致：
# hal_conf 启用了某个模块却没有对应 .c，会在链接期报未定义符号。
set(HAL_SOURCES
  ${HAL_DIR}/Src/stm32f1xx_hal.c
  ${HAL_DIR}/Src/stm32f1xx_hal_cortex.c
  ${HAL_DIR}/Src/stm32f1xx_hal_rcc.c
  ${HAL_DIR}/Src/stm32f1xx_hal_rcc_ex.c
  ${HAL_DIR}/Src/stm32f1xx_hal_gpio.c
  ${HAL_DIR}/Src/stm32f1xx_hal_gpio_ex.c
  ${HAL_DIR}/Src/stm32f1xx_hal_dma.c
  ${HAL_DIR}/Src/stm32f1xx_hal_exti.c
  ${HAL_DIR}/Src/stm32f1xx_hal_pwr.c
  ${HAL_DIR}/Src/stm32f1xx_hal_flash.c
  ${HAL_DIR}/Src/stm32f1xx_hal_flash_ex.c
  ${HAL_DIR}/Src/stm32f1xx_hal_tim.c
  ${HAL_DIR}/Src/stm32f1xx_hal_tim_ex.c
  ${HAL_DIR}/Src/stm32f1xx_hal_uart.c
  ${HAL_DIR}/Src/stm32f1xx_hal_i2c.c
  ${HAL_DIR}/Src/stm32f1xx_hal_spi.c
  ${HAL_DIR}/Src/stm32f1xx_hal_adc.c
  ${HAL_DIR}/Src/stm32f1xx_hal_adc_ex.c)

# ---- 课程官方驱动 -----------------------------------------------------------
# 20 个驱动文件 + 课程 IRQ 汇总，与课程 5_程序源码 中的 Drivers/DshanMCU-F103 同源。
# ascii_font.c 不在此列：它没有头文件，是被 driver_oled.c 用 #include 引入的
# 数据文件。单独编译会得到 multiple definition of `ascii_font`（已实测）。
# 全量编译的理由：每个驱动的 Test 函数是独立符号，未被 main.c 分派到的会由
# --gc-sections 回收；按需增删源文件的代价是每次换模块都要改构建脚本。
set(DRIVER_SOURCES
  ${DRV_DIR}/driver_led.c
  ${DRV_DIR}/driver_timer.c
  ${DRV_DIR}/driver_irq.c
  ${DRV_DIR}/driver_key.c
  ${DRV_DIR}/driver_uart.c
  ${DRV_DIR}/driver_color_led.c
  ${DRV_DIR}/driver_active_buzzer.c
  ${DRV_DIR}/driver_passive_buzzer.c
  ${DRV_DIR}/driver_ir_sender.c
  ${DRV_DIR}/driver_ir_receiver.c
  ${DRV_DIR}/driver_ir_obstacle.c
  ${DRV_DIR}/driver_light_sensor.c
  ${DRV_DIR}/driver_motor.c
  ${DRV_DIR}/driver_rotary_encoder.c
  ${DRV_DIR}/driver_oled.c
  ${DRV_DIR}/driver_lcd.c
  ${DRV_DIR}/driver_ds18b20.c
  ${DRV_DIR}/driver_dht11.c
  ${DRV_DIR}/driver_ultrasonic_sr04.c
  ${DRV_DIR}/driver_spiflash_w25q64.c
  ${DRV_DIR}/driver_mpu6050.c)

add_executable(${PROJECT_NAME}
  ${CORE_SOURCES}
  ${RUNTIME_SOURCES}
  ${HAL_SOURCES}
  ${DRIVER_SOURCES}
  startup/startup_stm32f103xb.s)

set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".elf")

target_include_directories(${PROJECT_NAME} PRIVATE
  Core/Inc
  ${DRV_DIR}
  ${HAL_DIR}/Inc
  ${CMSIS_DEV}
  ${CMSIS_CORE})

# USE_HAL_DRIVER 让 stm32f1xx.h 引入 HAL 头文件；
# STM32F103xB 是器件型号宏，必须与启动文件、链接脚本的内存布局一致。
# 选择要跑的模块自测（见 Core/Src/main.c 的 Lab_Run 分支）；0 = 板载 LED
set(LAB_TEST 0 CACHE STRING "Lab_Run 分支编号，0 = 板载 LED")

target_compile_definitions(${PROJECT_NAME} PRIVATE
  USE_HAL_DRIVER
  STM32F103xB
  LAB_TEST=${LAB_TEST})

# Cortex-M3 无 FPU，浮点一律走软件库
target_compile_options(${PROJECT_NAME} PRIVATE
  -mcpu=cortex-m3 -mthumb -mfloat-abi=soft
  -ffunction-sections -fdata-sections
  -Wall
  -g3 -Og)

target_link_options(${PROJECT_NAME} PRIVATE
  -mcpu=cortex-m3 -mthumb -mfloat-abi=soft
  # 启动文件已提供复位入口与 .data/.bss 初始化，crt0.o 属于重复；
  # 它还会为 exit() 拉入 stdio，连带 libnosys 的四条"未实现"告警。
  # 代价是 _init/_fini 消失，由 Core/Src/crt_stubs.c 补空实现。
  -nostartfiles
  -T${CMAKE_CURRENT_SOURCE_DIR}/ld/STM32F103C8Tx.ld
  -Wl,--gc-sections
  -Wl,--no-warn-rwx-segments
  -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.map
  --specs=nano.specs
  --specs=nosys.specs)

# ---- hex / bin 产物与体积报告 ------------------------------------------------
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:${PROJECT_NAME}> ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.hex
  COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.bin
  COMMAND ${CMAKE_SIZE} -A -x $<TARGET_FILE:${PROJECT_NAME}>
  COMMENT "生成 hex/bin 并打印体积")

# ---- 烧写（可选）-------------------------------------------------------------
# 复用 F103_common 的 ST-Link 配置；找不到 openocd 时不注册该目标。
find_program(F103_OPENOCD openocd)
if(F103_OPENOCD)
  add_custom_target(flash
    COMMAND ${F103_OPENOCD}
            -f "${CMAKE_CURRENT_SOURCE_DIR}/../F103_common/openocd/stlink_f103.cfg"
            -c "program $<TARGET_FILE:${PROJECT_NAME}> verify reset exit"
    DEPENDS ${PROJECT_NAME}
    COMMENT "通过 ST-Link 烧写 ${PROJECT_NAME}"
    VERBATIM)
else()
  message(STATUS "未找到 openocd，跳过 flash 目标")
endif()
