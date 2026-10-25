# F103_common：STM32F103 裸机实验公共设施

面向 `STM32F103C8T6`（Cortex-M3，64 KB Flash / 20 KB SRAM，DShan 出品的 DShanMCU-103 板）的构建公共层。
每个 `F103_NN_xxx` 实验只需提供自己的 `main.c` 和 `CMakeLists.txt`。

## 目录

| 路径 | 作用 |
| --- | --- |
| `cmake/arm-none-eabi.cmake` | 交叉编译工具链文件（`CMAKE_TOOLCHAIN_FILE`） |
| `cmake/f103.cmake` | `f103_add_firmware()` / `f103_add_flash_target()` 两个函数 |
| `ld/STM32F103C8Tx.ld` | 链接脚本。课程素材只有 Keil 的 `.sct`，此文件为自撰 |
| `vendor/startup/startup_stm32f103xb.s` | CMSIS GCC 启动文件（向量表、`.data` 拷贝、`.bss` 清零） |
| `vendor/cmsis/` | 裁剪后的 CMSIS Core + 设备头文件 |
| `vendor/src/system_stm32f1xx.c` | `SystemInit()` / `SystemCoreClockUpdate()` |
| `src/crt_stubs.c` | 补 `_init` / `_fini`，配合 `-nostartfiles` |
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

## 设计取舍

- **自带启动文件 + `-nostartfiles`**：默认链接会额外引入 `crt0.o`，它为 `exit()` 拉入 newlib 的 `stdio`，进而引用 `_write/_read/_lseek/_close`，使每次构建出现四条 libnosys "未实现"告警，并多占 Flash。本仓库已有完整启动文件，crt0 属重复。代价是 crti 提供的 `_init/_fini` 消失，由 `src/crt_stubs.c` 补空实现。
- **`-Wl,--no-warn-rwx-segments`**：`.init_array`/`.fini_array` 物理在 Flash、带可写标志，ld 据此报 RWX 段。裸机没有 MMU，段权限不产生保护语义，属工具链提示。
- **`._user_heap_stack` 预留 1.5 KB**：把堆栈预留量真实占进 RAM，`data + bss` 吃穿预留区时在链接期报 `region RAM overflowed`，而不是运行期踩栈表现成随机死机。因此 `size` 报告的 `bss` 含这 1536 B，不代表真实变量占用。
- **裁剪 vendor 而非引用课程目录**：`Linux/Linux_WEIDS/DShanMCU-F103/` 被 `.gitignore` 排除，直接引用会导致克隆后无法构建。裁剪后仅约 1 MB，见 `vendor/README.md`。

## 已知限制

- 只覆盖 STM32F103xB 系列；换芯片需同时替换启动文件、链接脚本和 `STM32F103xB` 宏。
- 链接脚本假设 Flash 64 KB / SRAM 20 KB（C8T6 实测规格）。C6T6 等同系列小容量型号不可直接复用。
- 未接入 HAL。需要 HAL 外设时再向 `vendor/` 增补对应模块，不预先引入。
- `flash` 目标的 OpenOCD 配置按 ST-Link V2（`hla_swd`）编写；V3 或 J-Link 需另写配置。
