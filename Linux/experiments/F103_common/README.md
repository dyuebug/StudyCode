# F103_common：STM32F103 裸机实验公共设施

面向 `STM32F103C8T6`（Cortex-M3，64 KB Flash / 20 KB SRAM，DShan 出品的 DShanMCU-103 板）的构建公共层。
每个 `F103_NN_xxx` 实验只需提供自己的 `main.c` 和 `CMakeLists.txt`。

## 目录

| 路径 | 作用 |
| --- | --- |
| `cmake/arm-none-eabi.cmake` | 交叉编译工具链文件（`CMAKE_TOOLCHAIN_FILE`） |
| `cmake/f103.cmake` | `f103_add_firmware()` / `f103_add_flash_target()` 两个函数 |
| `ld/STM32F103C8Tx.ld` | 链接脚本。课程素材只有 Keil 的 `.sct`，此文件为自撰 |
| `ld/STM32F100RB_qemu.ld` | QEMU `stm32vldiscovery` 用的链接脚本（128 KB Flash / 8 KB SRAM），仅在 `-DF103_QEMU=ON` 时启用 |
| `include/f103_tick.h` | 1 ms 时基接口：轮询模式（`wait_1ms`/`delay_ms`）与中断模式（`init_irq`/`now_ms`） |
| `src/f103_tick.c` | SysTick 实现，集中处理 `SystemCoreClock` 刷新；中断模式下占用 `SysTick_Handler`，见下文「体积开销」 |
| `include/f103_uart.h` | USART1 轮询收发接口（`init`/`putc`/`poll`），含 `F103_UART_OVER` 状态 |
| `src/f103_uart.c` | USART1 寄存器配置、BRR 计算、`_write` 重定向；实验 04 起复用 |
| `vendor/startup/startup_stm32f103xb.s` | CMSIS GCC 启动文件（向量表、`.data` 拷贝、`.bss` 清零） |
| `vendor/cmsis/` | 裁剪后的 CMSIS Core + 设备头文件 |
| `vendor/src/system_stm32f1xx.c` | `SystemInit()` / `SystemCoreClockUpdate()` |
| `src/crt_stubs.c` | 补 `_init` / `_fini`、`_sbrk`（512 B 堆）、`_isatty` 及四个 stdio syscall 桩 |
| `openocd/stlink_f103.cfg` | ST-Link V2 烧写配置 |

## 工具链

实测版本：`arm-none-eabi-gcc 14.2.1 (Arm GNU Toolchain 14.2.Rel1)`、`CMake 4.0.3`、`Ninja 1.13.2`、`xPack OpenOCD 0.12.0`。

工具链文件按以下顺序定位编译器，任一命中即可，无需改仓库文件：

1. `-DARM_TOOLCHAIN_DIR=<bin 目录>`
2. 环境变量 `ARM_TOOLCHAIN_DIR`
3. `PATH` 中的 `arm-none-eabi-gcc`
4. Arm 官方 Windows 安装器的默认目录 `C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/*/bin`

## 构建与烧写

在某个实验目录内执行：

```bash
cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake
cmake --build build            # 产出 .elf / .hex / .bin，并打印 Flash/RAM 占用
cmake --build build --target flash   # 需 ST-Link 已连接，且 openocd 可被找到
```

`flash` 目标要求 `openocd` 在 `PATH` 中，或用 `-DF103_OPENOCD=<路径>` 指定；找不到时该目标自动跳过，构建不受影响。

## QEMU 仿真构建

加 `-DF103_QEMU=ON` 即可产出能跑在 `qemu-system-arm -M stm32vldiscovery` 上的固件：

```bash
cmake -S . -B build-qemu -G Ninja -DF103_QEMU=ON \
  -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake
cmake --build build-qemu
```

该选项只做一件事：把链接脚本换成 `ld/STM32F100RB_qemu.ld`。**为什么必须换脚本**：QEMU 的 `stm32vldiscovery` 建模的是 STM32F100RB（8 KB SRAM），而 `STM32F103C8Tx.ld` 的 `_estack = 0x20005000` 超出其 RAM 范围，第一次压栈就 BusFault，表现为串口一片空白。

产物放在独立的 `build-qemu/`（已加入 `.gitignore`），与真机 `build/` 并存，便于逐字节对照。

**能仿真的范围很窄**：QEMU 只真实建模 USART1–3 与 SysTick；GPIO、EXTI、RCC、TIM、I2C、DWT 全部未建模，SPI 半建模。且 QEMU 的 SYSCLK 是 24 MHz，而固件按 8 MHz 计算时基，实测偏差约 2.9 倍。完整能力矩阵、原始日志与运行脚本见 [`../../tests/qemu-stm32/`](../../tests/qemu-stm32/README.md)。

## 设计取舍

- **自带启动文件 + `-nostartfiles`**：默认链接会额外引入 `crt0.o`，它为 `exit()` 拉入 newlib 的 `stdio`，进而引用 `_write/_read/_lseek/_close`，使每次构建出现四条 libnosys "未实现"告警，并多占 Flash。本仓库已有完整启动文件，crt0 属重复。代价是 crti 提供的 `_init/_fini` 消失，由 `src/crt_stubs.c` 补空实现。
- **`-Wl,--no-warn-rwx-segments`**：`.init_array`/`.fini_array` 物理在 Flash、带可写标志，ld 据此报 RWX 段。裸机没有 MMU，段权限不产生保护语义，属工具链提示。
- **UART 在第二个使用者出现时才提取**：实验 04 的串口寄存器配置原本写在自身 `main.c`，因为该实验的重点就是逐寄存器讲解；实验 05 需要同一套初始化与发送，才收进 `f103_uart.c`。`f103_uart.c` 会被所有目标编译，未引用的函数由 `--gc-sections` 回收（实测实验 01–03 体积不变）。
- **stdio 桩集中在一处**：`_sbrk`、`_isatty`、`_close`、`_fstat`、`_lseek`、`_read` 与硬件无关，统一放在 `crt_stubs.c`；`_write` 绑定具体串口，放在 `f103_uart.c`。两个容易踩的点：`_isatty` 缺失会使 stdout 全缓冲，表现成「printf 不输出」；`_sbrk` 以 `_Min_Heap_Size`（512 B）为上限，与链接脚本的堆预算一致，超出返回 `ENOMEM` 而不是悄悄爬进栈区。
- **`._user_heap_stack` 预留 1.5 KB**：把堆栈预留量真实占进 RAM，`data + bss` 吃穿预留区时在链接期报 `region RAM overflowed`，而不是运行期踩栈表现成随机死机。因此 `size` 报告的 `bss` 含这 1536 B，不代表真实变量占用。
- **裁剪 vendor 而非引用课程目录**：`Linux/Linux_WEIDS/DShanMCU-F103/` 被 `.gitignore` 排除，直接引用会导致克隆后无法构建。裁剪后仅约 1 MB，见 `vendor/README.md`。
- **中断模式的固定体积开销**：`f103_tick.c` 一旦定义 `SysTick_Handler`，向量表（`KEEP`）就会引用它，`--gc-sections` 无法回收。实测代价为 16 B Flash + 4 B bss，**只用轮询模式的实验也要承担**。未调用的 `f103_tick_init_irq`（0x34）与 `f103_tick_now_ms`（0xC）可被正常回收（map 中地址为 0）。若要为极小镜像去掉这 20 B，只能把中断模式拆到独立的 `f103_tick_irq.c`，由实验按需加入；当前三个实验都远未接近 64 KB，不做此拆分。

## 已知限制

- 只覆盖 STM32F103xB 系列；换芯片需同时替换启动文件、链接脚本和 `STM32F103xB` 宏。
- 链接脚本假设 Flash 64 KB / SRAM 20 KB（C8T6 实测规格）。C6T6 等同系列小容量型号不可直接复用。
- 未接入 HAL。需要 HAL 外设时再向 `vendor/` 增补对应模块，不预先引入。
- `flash` 目标的 OpenOCD 配置按 ST-Link V2（`hla_swd`）编写；V3 或 J-Link 需另写配置。
