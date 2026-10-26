# QEMU 仿真验证（STM32F103 裸机实验）

本目录回答一个问题：**在没有板子、或者不想反复烧写的时候，哪些实验可以在 QEMU 里真正跑起来并拿到证据，哪些只能"编译通过"。**

答案不是靠读文档猜的，而是用两个手段确认的：

1. **读 QEMU 源码**（v10.2.1）：`-M stm32vldiscovery` 建模的是 STM32F100RB，`hw/arm/stm32f100_soc.c` 里哪些外设是真实模型、哪些是 `create_unimplemented_device`。
2. **跑探针固件**（`probe/`）：逐个外设写入特征值再读回，把"模型到底实现了什么"变成可复现的串口输出。

实测结论见 [RESULT.md](RESULT.md)，原始串口日志在 `logs/`。

## 环境

| 项 | 值 |
| --- | --- |
| QEMU | `qemu-system-arm` 10.2.1（Debian 1:10.2.1+ds-1ubuntu3.3），**装在 WSL Ubuntu 内**，Windows 侧没有 |
| 机器 | `-M stm32vldiscovery`（STM32F100RB：Cortex-M3，24 MHz SYSCLK，128 KB Flash，8 KB SRAM） |
| 固件构建 | 与真机实验同一套 CMake，额外加 `-DF103_QEMU=ON` |

## 目录

```text
tests/qemu-stm32/
├── README.md           # 本文件：怎么用、边界在哪
├── RESULT.md           # 实测记录：探针矩阵、实验输出、时基校准
├── run.sh              # 正向用例：探针 + 04 + 05 + 时基校准，日志落到 logs/
├── run_negative.sh     # 反向用例：01/02/03/06/08/09 在 QEMU 里的真实表现
├── logs/               # 串口原始输出（可回放、可 diff）
└── probe/              # 外设建模能力探针（裸机构建，单独工程）
    ├── CMakeLists.txt
    └── main.c
```

## 构建

关键一步是 `-DF103_QEMU=ON`。QEMU 的 SRAM 只有 8 KB（F103C8T6 是 20 KB），沿用 `STM32F103C8Tx.ld` 的 `_estack = 0x20005000` 会在**第一次压栈**时 BusFault，现象是串口一个字节都不打印。该选项把链接脚本切到 `F103_common/ld/STM32F100RB_qemu.ld`。

```bash
# 以 F103_04 为例（Windows Git Bash + Ninja）
cd Linux/experiments/F103_04_UART_Echo
cmake -S . -B build-qemu -G Ninja -DF103_QEMU=ON \
  -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake \
  -DCMAKE_MAKE_PROGRAM="<ninja 路径>"
cmake --build build-qemu
```

产物与真机构建同目录结构但互不覆盖（`build/` vs `build-qemu/`），两套 ELF 并存，方便对照。

## 运行

```bash
cd Linux/tests/qemu-stm32
bash run.sh            # 全部：probe + exp04 + exp05 + tickrate
bash run.sh probe      # 只有探针
bash run.sh exp04      # UART 轮询回显
bash run.sh exp05      # UART 中断 + 环形缓冲
bash run.sh tickrate   # 量化时基偏差
bash run_negative.sh   # 反向用例：01/02/03/06/07/08/09
```

脚本必须在 **WSL/Linux** 里跑，因为 QEMU 装在那里；它通过 `/mnt/e/...` 访问 Windows 侧仓库，日志写回 `logs/`。

喂数据要延迟 1 秒：QEMU 的串口后端在 `USART_CR1.RE` 置位前会拒绝接收，输入若早早全部到达会滞留在管道里，表现为 `rx=0`——这不是固件的 bug，是仿真时序。

## 能力边界（摘要）

| 外设 | QEMU 模型 | 能验证什么 | 不能验证什么 |
| --- | --- | --- | --- |
| USART1/2/3 | 真实模型 | 收发、RXNE、TXE、中断、轮询 | 波特率精度、真实比特时序、ORE 溢出 |
| SysTick | 真实模型 | 计数、COUNTFLAG、中断、NVIC | 与 8 MHz 假设一致的时间（QEMU 是 24 MHz） |
| Flash / SRAM | 真实 | 链接布局、越界访问触发 BusFault | 擦写寿命、选项字节 |
| DWT | **未建模** | — | `CYCCNT` 恒 0，`F103_09` 会死循环 |
| GPIO / EXTI | **未建模** | — | 读写恒 0，按键、LED、探针脉宽全部无法观测 |
| RCC / PWR / BKP | **未建模** | — | `SystemCoreClockUpdate()` 走 HSI 8 MHz 兜底分支 |
| TIM2–TIM7 | **未建模** | — | PWM、输入捕获、定时器延时全部无效 |
| I2C1/2 | **未建模** | — | — |
| SPI1/2 | 半建模 | `CR1` 有状态，写 `DR` 会置 `RXNE` | 无片选、无真实移位，W25Q 协议不可信 |

由此得出（每条都对应 `run.sh` / `run_negative.sh` 的一次实测，见 RESULT.md）：

- **可以在 QEMU 里验证**：`F103_04_UART_Echo`、`F103_05_UART_IRQ_Ring`，以及任何只用到"串口 + SysTick + NVIC"的代码。
- **能跑起来、但结果不可信**：`F103_07`（I2C 永远无应答，只验证到超时分支；`CCR`/`TRISE` 读回 0 说明配置根本没生效）、`F103_08`（JEDEC ID 读出 `00 00 00`，擦写"成功"、回读全 0，自检停在 FAIL）。
- **跑起来但拿不到任何证据**：`F103_01/02/03/06`——它们只操作 GPIO/TIM，且不初始化串口，QEMU 里既无输出也无波形，等于空转。
- **会挂死**：`F103_09`——DWT 未使能（读回 0）、TIM4 计数恒 0，延时循环永不退出，打印到 TIM4 第一档就停住。
- **共同前提**：所有时间相关结论都不能从 QEMU 得出（时基偏差 2.90 倍，见 RESULT.md）。

## 已知失真

1. **SYSCLK 24 MHz vs 固件假设 8 MHz**：固件按 8 MHz 算 `SysTick->LOAD`，QEMU 按 24 MHz 计数，实测"固件毫秒"是真实时间的 2.90 倍。任何延时、超时、吞吐结论必须回到真机。
2. **串口无速率概念**：QEMU 的 USART 不做比特级移位，不管 `BRR` 配多少都立即收完。因此 `rx` 计数、回显内容可信，**速度不可信**。
3. **背压代替溢出**：QEMU 在 `RDR` 未读时拒绝新字节（`can_receive` 返回 0），不会像真 UART 那样置 ORE。`F103_05` 的 `drop`/`ore` 计数在仿真里恒为 0，突发 300 字节也不丢——这条路径只能上板验证。
4. **没有 GPIO**：所有"看灯闪""量脉宽"的验证方式在 QEMU 里完全失效，只剩串口打印。

## 相关文档

- [RESULT.md](RESULT.md)：本次实测的完整记录与环境版本。
- `Linux/experiments/F103_common/ld/STM32F100RB_qemu.ld`：QEMU 布局链接脚本，注释里写了为什么必须单开一份。
- `Linux/experiments/F103_common/cmake/f103.cmake`：`F103_QEMU` 选项的定义处。
