cmake_minimum_required(VERSION 3.20)

# 本项目复用 experiments/F103_common 的构建规则（启动文件、链接脚本、Cortex-M3 选项、
# FreeRTOS 裁剪子集）。仓库约定见 Linux/CLAUDE.md §5：公共设施放 experiments/，
# 可复现综合项目放 projects/，因此这里的 include 路径要跨目录。
include("${CMAKE_CURRENT_SOURCE_DIR}/../../experiments/F103_common/cmake/f103.cmake")

project(aviation_node LANGUAGES C ASM)

# 故障注入（第 9–10 周"任务阻塞"剧本）：rx 任务在 AV_DEBUG_BLOCK_AT_MS 处空转
# AV_DEBUG_BLOCK_RX_MS 毫秒，用于验证"任务卡住 → 心跳超时 → 停喂"这条链。
#
# 默认只在 QEMU 构建里打开：这条注入没有任何运行时开关，编进去就会改变固件行为，
# 所以不能让真机/生产构建默认带上它。真板构建（不带 F103_QEMU）保持 0，
# 注入代码在 #if 之后整段不参与编译。
# 时刻 16500 落在注入剧本的静默段（固件时间约 14.7–17.7 s）内：队列为空，
# 阻塞不会丢字节，也就不会污染 crc/gap 的计数自洽性。
if(F103_QEMU)
  set(_av_block_default 800)
else()
  set(_av_block_default 0)
endif()

set(AV_DEBUG_BLOCK_RX_MS ${_av_block_default} CACHE STRING
    "rx 任务阻塞注入时长（ms，0 = 关闭）")
set(AV_DEBUG_BLOCK_AT_MS 16500 CACHE STRING
    "rx 任务阻塞注入时刻（固件 ms）")

# NO_TICK：时基交给 FreeRTOS 的 SysTick，避免 f103_tick.c 重复定义 SysTick_Handler
f103_add_firmware(aviation_node
  NO_TICK
  SOURCES
    src/main.c
    src/av_frame.c
    src/av_sensor.c
    src/av_mode.c
    src/av_health.c
    src/av_downlink.c
  INCLUDES include)

target_compile_definitions(aviation_node PRIVATE
  AV_DEBUG_BLOCK_RX_MS=${AV_DEBUG_BLOCK_RX_MS}
  AV_DEBUG_BLOCK_AT_MS=${AV_DEBUG_BLOCK_AT_MS})

# 不链接额外的内核模块：本节点只用任务/队列/互斥量
f103_add_freertos(aviation_node)

f103_add_flash_target(aviation_node)
