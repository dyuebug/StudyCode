# FreeRTOS 内核（已裁剪，仅够 STM32F103 教学实验）

## 来源

| 项目 | 值 |
| --- | --- |
| 内核 | FreeRTOS-Kernel **V11.3.0**（`tskKERNEL_VERSION_NUMBER`） |
| 许可证 | MIT，见同目录 [`LICENSE.md`](LICENSE.md)，未修改任何版权声明 |
| 上游路径 | `Linux/Linux_WEIDS/rtos_doc_source/RTOSCode/FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel/` |
| 上游谱系 | FreeRTOS-LTS 仓库中的 FreeRTOS-Kernel V11.3.0（含 SPDX 2.3 SBOM） |

上游目录是多平台、全功能的内核仓库（含 POSIX、MPU、ARM_CM0/CM4F/CM33、RISC-V 等十几套移植层）。
本仓库只保留 F103（Cortex-M3）+ GCC + 动态分配这一条最小路径，**避免把上千个用不到的文件拖进学习仓库**。

## 收录清单

```text
vendor/freertos/
├── LICENSE.md
├── include/FreeRTOSConfig.h        # 自撰（本仓库特有，非上游文件）
└── Source/
    ├── list.c tasks.c queue.c                        # 内核最小可运行集
    ├── event_groups.c timers.c                       # 按需加入（不默认链接，见下）
    ├── include/*.h                                   # 21 个头文件（见下）
    ├── portable/GCC/ARM_CM3/{port.c,portmacro.h}     # CM3 移植层
    └── portable/MemMang/heap_4.c                     # 带合并的分配器
```

`event_groups.c` / `timers.c` 与上游 V11.3.0 **逐字节一致**（`md5sum` 核对），
是 2026-10-26 为实验 12/13 补入的：两者的实现各自独立成文件，不链接会在链接期报
`undefined reference to xEventGroupCreate` / `xTimerCreate`。它们**不默认参与构建**——
`f103_add_freertos(<target> MODULES event_groups timers)` 显式加入才链接，
避免不开对应配置项时白白增加体积（`timers.c` 还需要 `configUSE_TIMERS = 1` 才有意义）。

**未收录**：`croutine.c`、`stream_buffer.c`、`portable/MemMang/heap_1..3,5.c`、
`portable/*` 的其它架构、`mpu_*`、`examples/`、测试与 SBOM 文件。

头文件保留了 `timers.h`、`event_groups.h`、`stream_buffer.h`、`croutine.h`、`message_buffer.h`。
其中 `event_groups.c` 用于实验 12，`timers.c` 用于实验 13（该目标定义
`F103_FREERTOS_USE_TIMERS=1`，使 `configUSE_TIMERS` 为 1；其余目标仍为 0）。
实验 14（心跳与看门狗）不链接任何额外模块：心跳用任务自身打点 + tick 差值实现，
不依赖 `timers.c` / `event_groups.c`。
`stream_buffer.c` / `croutine.c` 仍只有头文件，将来需要时按同样方式从上游复制。
`newlib-freertos.h` / `picolibc-freertos.h` 由 `FreeRTOS.h` 在 `configUSE_NEWLIB_REENTRANT` /
`configUSE_PICOLIBC_REENTRANT` 为 1 时条件包含，当前配置为 0。

## 改造与配置

- 唯一自撰文件是 `include/FreeRTOSConfig.h`，每个非平凡选项都写了取舍理由（见文件内注释）。
  关键项：8 MHz HSI、1000 Hz tick、4 KB heap_4、`configMAX_PRIORITIES = 5`、
  `configCHECK_HANDLER_INSTALLATION = 0`（间接路由，见 `F103_common/src/f103_freertos_vectors.s`）。
  软件定时器开关为按目标宏 `F103_FREERTOS_USE_TIMERS`（默认 0），守护任务栈 160 字
  （128 字实测只剩 28 字余量，见实验 13 README）。
- 构建接入：`../cmake/f103.cmake` 的 `f103_add_freertos(<target>)`，只加三个 `.c` + 移植层 + heap_4，
  头文件路径只有本目录的两个 `include`。

## 升级方法

替换 `Source/` 与 `Source/include/` 后重新编译，重点核对：

1. `FreeRTOSConfig.h` 是否新增必填宏（V11 引入 `configTICK_TYPE_WIDTH_IN_BITS`、`configNUMBER_OF_CORES`）。
2. `INCLUDE_*` 默认值是否变化（V11 起全部默认 0，缺少会导致 `undefined reference`）。
3. `portable/GCC/ARM_CM3` 的汇编入口名是否仍是 `vPortSVCHandler` / `xPortPendSVHandler` / `xPortSysTickHandler`。
4. 构建体积变化（见实验 README 的实测表），F103 只有 64 KB Flash。

## 边界

- 这是**裁剪后的教学子集**，不是完整发行版；不要用它作为"FreeRTOS 支持某某特性"的依据。
- 只验证过 Cortex-M3 / GCC / `heap_4` / 动态分配这一组合。
- 上游许可证为 MIT，可自由分发；若新增文件请从上游原样复制，不要手改内核源码。
