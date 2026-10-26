# QEMU 仿真实测记录（2026-10-26）

本文件只记录**实际跑出来的输出**。凡是推算、预期、未验证的内容，都会明确标注。

## 1. 环境

| 项 | 值 |
| --- | --- |
| 主机 | Windows 11 Pro 10.0.26100（仓库在 `E:` 盘） |
| 仿真器 | `qemu-system-arm` 10.2.1（Debian `1:10.2.1+ds-1ubuntu3.3`），运行于 WSL2 Ubuntu |
| 机器模型 | `-M stm32vldiscovery`（STM32F100RB） |
| 固件构建 | `arm-none-eabi-gcc 14.2.1` + CMake 4.0.3 + Ninja 1.13.2，`-DF103_QEMU=ON` |
| 串口 | `-serial stdio`，输入通过管道喂入，输出经 `tr -d '\r'` 后落盘到 `logs/` |
| 终止方式 | `timeout` 发 SIGTERM，因此每个日志末尾都有 `qemu-system-arm: terminating on signal 15` |

## 2. 探针：QEMU 到底建模了什么

探针源码 `probe/main.c`，日志 `logs/probe.log`。原始输出：

```text
=== QEMU STM32 建模探针 ===
[1] UART：你看到这一行，说明 USART1 的 TX 通路已建模
    SystemCoreClock = 8000000 Hz（RCC 读回 0 时走 HSI 8 MHz 分支）
[2] Flash[0x08000000] = 0x20002000（应为初始 MSP，证明 Flash 区域有效）
    RAM[0x20001FF0] 读回 0xA5A55A5A（写入 0xA5A55A5A，8 KB 之内）
RCC->APB2ENR   写入 0x00000004  读回 0x00000000  未建模/只读
    RCC->CFGR = 0x00000000
GPIOA->ODR     写入 0x00000020  读回 0x00000000  未建模/只读
GPIOA->CRL     写入 0x44444444  读回 0x00000000  未建模/只读
    GPIOA->IDR = 0x00000000
[5] TIM4：CNT 两次读数为 0 -> 0（不变说明定时器未建模）
TIM4->CR1      写入 0x00000001  读回 0x00000000  未建模/只读
I2C1->CR1      写入 0x00000001  读回 0x00000000  未建模/只读
    I2C1->SR1 = 0x00000000
SPI1->CR1      写入 0x0000034C  读回 0x0000034C  有状态
    SPI1：写 DR 后 RXNE 置位，SR = 0x0000000B，DR = 0x00000000
[8] DWT：CTRL 读回 0x00000000，CYCCNT 0 -> 0（差 0）
[9] SysTick 轮询：捕获 5 次 COUNTFLAG（5 次即视为工作）
[10] SysTick 中断：5 ms 计数到 5（≥5 说明中断通路正常）
=== 探针结束 ===
```

矩阵汇总：

| 外设 / 资源 | 观测 | 结论 |
| --- | --- | --- |
| USART1 TX | 全部打印正常 | 已建模 |
| USART1 RX + RXNE 中断 | `F103_04` / `F103_05` 收发成功（见 §3） | 已建模 |
| SysTick（轮询 COUNTFLAG） | 捕获 5 次 | 已建模 |
| SysTick（中断 + NVIC） | 5 ms 内计数到 5 | 已建模 |
| Flash @0x08000000 | 读出 `0x20002000`，正是 `STM32F100RB_qemu.ld` 的 `_estack` | 有效，布局按链接脚本生效 |
| SRAM | 0x20001FF0 写入回读一致 | 8 KB 有效（越界会 BusFault，见 §6） |
| RCC | 写入丢弃，读回 0 | 未建模 |
| GPIOA | ODR/CRL/IDR 全 0 | 未建模 |
| TIM4 | PSC/ARR/CR1 读回 0，CNT 恒 0 | 未建模 |
| I2C1 | CR1/SR1 全 0 | 未建模 |
| SPI1 | CR1 写读一致；写 DR 后 RXNE 置位、SR=0x0B、读 DR 得 0 | 半建模（无片选、无真实移位） |
| DWT | CTRL 恒 0，CYCCNT 恒 0 | 未建模 |

`SystemCoreClock = 8000000` 不是 QEMU 的答案：QEMU 的 RCC 读回 0，`SystemCoreClockUpdate()` 走了"HSE 未就绪 → 回落 HSI 8 MHz"分支。QEMU 实际的 SYSCLK 是 24 MHz，差值在 §3.3 量化。

## 3. 正向用例

### 3.1 F103_04 UART 轮询回显（`logs/exp04_echo.log`）

输入：启动后 1 s 发送 `hello\r\n`，再 1 s 发送 `xyz\r\n`。

```text
[F103_04] USART1 echo, PCLK2=8000000 Hz, 115200 8N1
[1000 ms] alive, rx=0, over=0
[2000 ms] alive, rx=0, over=0
hello

[3000 ms] alive, rx=7, over=0
[4000 ms] alive, rx=7, over=0
[5000 ms] alive, rx=7, over=0
xyz

[6000 ms] alive, rx=12, over=0
...
```

结论：`rx` 由 0 → 7 → 12，与输入字节数一致（`hello\r\n` = 7，`xyz\r\n` = 5）；回显内容逐字节正确；`over` 始终 0。

### 3.2 F103_05 UART 中断 + 环形缓冲（`logs/exp05_irq_ring.log`）

```text
[F103_05] USART1 RX IRQ + ring buffer (128 B, usable 127 B)
[1000 ms] alive, rx=0, drop=0, ore=0
alpha

[3000 ms] alive, rx=7, drop=0, ore=0
beta

[6000 ms] alive, rx=13, drop=0, ore=0
```

主循环只从环形缓冲取字节、不碰 `USART1->DR`，因此 `rx` 增长证明 **RXNE 中断确实被触发并执行了 ISR**。这是 QEMU 能验证中断通路的直接证据。

### 3.3 时基校准（`logs/exp04_tick_rate.log`）

真实运行 10 s，固件 `f103_tick_now_ms()` 走到 29000：

```text
[29000 ms] alive, rx=0, over=0
固件 29000 ms / 真实 10000 ms ≈ 2.90 倍（24 MHz ÷ 8 MHz 应为 3.00）
```

含义：固件按 8 MHz 计算 `SysTick->LOAD`，QEMU 以 24 MHz 驱动 SYSCLK，于是固件的"毫秒"比真实时间快约 2.9 倍。差值不到 3.00 的部分来自 QEMU 启动与串口输出开销，未进一步分解。

**所以 QEMU 里的任何延时、超时、吞吐数字都不能当结论**；`F103_09` 的误差表也只能在真机上看。

### 3.4 300 字节突发（`logs/exp05_burst300.log`）

```text
[F103_05] USART1 RX IRQ + ring buffer (128 B, usable 127 B)
[1000 ms] alive, rx=0, drop=0, ore=0
[2000 ms] alive, rx=0, drop=0, ore=0
AAAAAAAA...(连续输出)...
AAAAAAAA...(续)... 

[4000 ms] alive, rx=302, drop=0, ore=0
```

302 字节全部收到，`drop=0`、`ore=0`。多次运行中有时会在突发中间夹进一行心跳（例如 `rx=241`），
那是主循环与 ISR 交错执行的时刻不同导致的，属正常抖动，不影响"全部收到"这一结论。

但这条记录**不能证明环形缓冲的溢出处理正确**：QEMU 在 `RDR` 未读时通过 `can_receive` 背压住后端，根本不会产生真 UART 的 ORE 溢出。溢出路径只能上板验证。

## 4. 反向用例（预期失败，`run_negative.sh`）

| 实验 | QEMU 表现（`logs/neg_*.log`） | 判断 |
| --- | --- | --- |
| `F103_01_GPIO_Blink` | 4 s 内除 QEMU 自身的终止提示外无任何输出 | 该实验不初始化串口；GPIO 未建模，灯不会闪，无任何可观测证据 |
| `F103_02_GPIO_Input` | 无输出 | 同上，读按键恒 0 |
| `F103_03_EXTI_Key` | 无输出 | EXTI 未建模，按键中断永不触发 |
| `F103_06_TIM_PWM_RGB` | 无输出 | TIM 未建模，PWM 无从产生 |
| `F103_08_SPI_Flash` | 见下 | 能跑，但结论全错 |
| `F103_09_Delay_Precision` | 见下 | 挂死 |

`F103_08` 的实际输出：

```text
[F103_08] SPI1 master, mode 0, 8-bit, SCLK = 2000000 Hz, CR1 = 0x034C
JEDEC ID = 00 00 00
非 Winbond 厂商码，按原始值记录，不猜型号

--- 自检：擦除 → 写入 → 回读（会清空 4 KB 扇区 0）---
扇区擦除 : ok         (1 ms)
页编程 55 B: ok         (0 ms)
回读内容 : 
原始字节 : 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
结果     : FAIL（内容不一致）
```

注意"擦除 ok / 编程 ok"是假象：QEMU 的 `SPI1->DR` 读回恒 0，忙等状态位立刻满足；真正的数据通路并不存在。**这种"全成功但结果为假"的仿真比直接报错更危险**，看到 ok 不能当作验证通过。

`F103_09` 的实际输出：

```text
[F103_09] HCLK = 8000000 Hz，1 us = 8 cycles
SysTick : LOAD = 7999（1 ms），CTRL = 0x10005
TIM4    : PSC = 0，ARR = 0（1 MHz，65.536 ms 回卷）
DWT     : CYCCNTENA 读回 = 0
DWT 未使能，下表的周期列将恒为 0，请先排查 DEMCR/CTRL 写入

请求us    方法        DWT周期     实测us   误差us
----------------------------------------------------
     0  SysTick            0      0.0  +0.0
...
     0  TIM4               0      0.0  +0.0
```

随后停在此处直到超时：`delay_tim_us(1)` 等 `TIM4->CNT` 变化，而 CNT 恒 0，循环永不退出。

顺带确认了固件里的自检是有效的：`DWT 未使能` 这一行正是 `dwt_init()` 读回 0 时触发的告警，没有把失败伪装成"延时为 0"。

## 5. 构建体积（QEMU 链接脚本，128 KB / 8 KB）

| ELF | text | data | bss | 说明 |
| --- | --- | --- | --- | --- |
| `qemu_stm32_probe.elf` | 6468 | 96 | 1880 | 探针 |
| `f103_04_uart_echo.elf` | 4852 | 96 | 1880 | 正向 |
| `f103_05_uart_irq_ring.elf` | 5072 | 96 | 2024 | 正向 |
| `f103_01_gpio_blink.elf` | 768 | 4 | 1540 | 反向 |
| `f103_06_tim_pwm_rgb.elf` | 948 | 4 | 1540 | 反向 |
| `f103_08_spi_flash.elf` | 7072 | 96 | 1880 | 反向 |
| `f103_09_delay_precision.elf` | — | — | — | FLASH 6536 B / RAM 1984 B（CMake 报告） |

`bss` 里约 1.5 KB 是链接脚本 `._user_heap_stack` 的哨兵预留（512 B 堆 + 1 KB 栈），不是实际占用。

## 6. 失败案例：用 F103C8T6 链接脚本跑 QEMU

现象（未换链接脚本时，`F103_04` 的第一次尝试）：

```text
Loaded reset SP 0x20005000
qemu-system-arm: Data Abort ... 0x20004ff0 ...
BusFault with BFSR.STKERR
```

原因：`STM32F103C8Tx.ld` 的 `_estack = 0x20005000`（20 KB SRAM 末尾），QEMU 的 SRAM 只到 0x20002000。第一次中断/函数调用压栈就落在不存在的地址上，升级为 HardFault，默认 Handler 是死循环，于是串口一个字节都没有。

修复：`-DF103_QEMU=ON` 切到 `STM32F100RB_qemu.ld`（`_estack = 0x20002000`）。修复后探针读到的初始 MSP 正是 `0x20002000`。

这条记录的价值在于：**"串口完全没输出"在裸机上不一定是驱动问题，先确认栈指针落在有效 RAM 内。**

## 7. 结论

可以下的结论：

1. QEMU 的 `stm32vldiscovery` 能真实仿真 USART1–3 与 SysTick（含 NVIC 中断），因此"串口 + 时基 + 中断"这条主干可以在无板环境下验证。
2. GPIO、EXTI、RCC、TIM、I2C、DWT 均未建模，SPI 只实现到寄存器层面。依赖它们的实验在 QEMU 里得不到有效证据。
3. QEMU 的 SYSCLK 是 24 MHz，与固件的 8 MHz 假设相差约 2.9 倍，时间相关结论一律不得来自仿真。
4. QEMU 用背压代替 UART 溢出，环形缓冲的丢弃路径无法在仿真中触发。

不能下的结论：

- 不能说 `F103_05` 的环形缓冲"溢出处理正确"——只验证了正常路径与中断通路。
- 不能说 `F103_04` 的"115200 8N1"配置正确——QEMU 不做比特级移位，`BRR` 配错也照样收发。
- 不能用 §3.3 的比例反推真机时基——真机是 8 MHz HSI，QEMU 是 24 MHz，两者没有可比性。

## 8. 复现方式

```bash
# 1. 构建（Windows 侧，每个实验各做一次）
cd Linux/experiments/F103_04_UART_Echo
cmake -S . -B build-qemu -G Ninja -DF103_QEMU=ON \
  -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake \
  -DCMAKE_MAKE_PROGRAM="<ninja 路径>"
cmake --build build-qemu

# 2. 运行（WSL 侧）
cd Linux/tests/qemu-stm32
bash run.sh            # 正向：probe + exp04 + exp05 + tickrate
bash run_negative.sh   # 反向：01/02/03/06/08/09
```
