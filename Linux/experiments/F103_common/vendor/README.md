# vendor：裁剪后的 CMSIS 子集

## 来源

裁剪自课程素材 `Linux_WEIDS/DShanMCU-F103/5_程序源码/01_单片机程序/02_视频配套的源码/[05]_GPIO操作/0501_led/Drivers/`
（CubeMX 生成的 STM32F1 工程，HAL/LL 全套 + CMSIS 全系列）。

复制时未做内容修改。`vendor/` 下全部文件均为原样拷贝，本仓库的改动只发生在 `../cmake/`、`../ld/`、`../src/`。

## 为什么只取子集

课程目录 `Linux/Linux_WEIDS/DShanMCU-F103/` 被根 `.gitignore` 排除，直接引用会导致克隆后无法构建。
完整 `Drivers/` 约 17 MB（含全部 STM32F1 型号头、HAL/LL 全模块、DSP/NN 库），对本阶段实验是冗余。
这里只保留 **STM32F103xB + GCC + 裸机** 路径实际需要的文件，约 1 MB。

## 文件清单

| 文件 | 用途 |
| --- | --- |
| `cmsis/core/core_cm3.h` | Cortex-M3 内核寄存器、`SysTick`、`NVIC`、`SCB` 定义 |
| `cmsis/core/cmsis_gcc.h` | GCC 内联汇编与 `__NOP`/屏障等内置函数 |
| `cmsis/core/cmsis_compiler.h` | 编译器选择层（只为引入 `cmsis_gcc.h`） |
| `cmsis/core/cmsis_version.h` | CMSIS 版本号 |
| `cmsis/core/mpu_armv7.h` | `core_cm3.h` 无条件包含，即便不用 MPU |
| `cmsis/device/stm32f1xx.h` | STM32F1 家族总入口，按 `STM32F103xB` 选择具体型号头 |
| `cmsis/device/stm32f103xb.h` | STM32F103x8/xB 的寄存器与位定义（843 KB，本体就是数据手册的寄存器映射） |
| `cmsis/device/system_stm32f1xx.h` | `SystemInit` / `SystemCoreClockUpdate` 声明 |
| `src/system_stm32f1xx.c` | 上述两函数实现 |
| `startup/startup_stm32f103xb.s` | GCC 启动文件：向量表、复位入口、`.data` 拷贝、`.bss` 清零、`bl main` |
| `LICENSE-CMSIS.txt` | 原目录 `CMSIS/LICENSE.txt` |

## 与 HAL 的边界

`stm32f1xx.h` 末尾有：

```c
#if defined (USE_HAL_DRIVER)
 #include "stm32f1xx_hal.h"
#endif
```

因此**不定义 `USE_HAL_DRIVER` 时完全不需要 HAL 头文件**。实验改用 HAL 时，需再复制
`Drivers/STM32F1xx_HAL_Driver/Inc` 与所需模块的 `Src`（`stm32f1xx_hal.c`、`_cortex.c`、`_rcc.c`、
`_rcc_ex.c`、`_gpio.c`、`_gpio_ex.c` 起步），并在 `Core/Inc/stm32f1xx_hal_conf.h` 中裁剪模块开关。

## 许可

CMSIS 为 Apache-2.0，见 `LICENSE-CMSIS.txt`。HAL 为 BSD-3-Clause（当前未引入）。
两份许可都允许随工程分发；保留许可文件即可。
