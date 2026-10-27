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
├── run.sh              # 正向用例：探针 + 04/05 + 10–15 + aviation + 时基校准，日志落到 logs/
├── run_negative.sh     # 反向用例：01/02/03/06/07/08/09 在 QEMU 里的真实表现
├── aviation/           # 航空节点的 PC 侧工具
│   ├── inject.py       # 帧注入器（纯标准库，写 stdout 由管道喂给 QEMU）
│   ├── expect.py       # 上行不变量判定器：读日志 → 24 项 PASS/FAIL → 退出码
│   └── link.py         # 下行判定器：解 USART2 二进制流 → 7 项（L1–L7）→ 退出码
├── logs/               # 串口原始输出（可回放、可 diff）；aviation_downlink.bin 是 USART2 原始字节
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
bash run.sh            # 全部：probe + exp04 + exp05 + exp10 + exp11 + exp12 + exp13 + exp14 + exp15 + exp16 + aviation-link + tickrate
bash run.sh probe      # 只有探针
bash run.sh exp04      # UART 轮询回显
bash run.sh exp05      # UART 中断 + 环形缓冲
bash run.sh exp10      # FreeRTOS 任务/队列/互斥量
bash run.sh exp11      # FreeRTOS 中断 → 任务（RXNE + 队列）
bash run.sh exp12      # FreeRTOS 同步原语（信号量 / 事件组 / 中断合并）
bash run.sh exp13      # FreeRTOS 软件定时器（单次 / 周期 / 回调阻塞代价）
bash run.sh exp14      # FreeRTOS 心跳与看门狗（监督逻辑；IWDG 未建模）
bash run.sh exp15      # FreeRTOS 栈溢出检出（钩子触发即正常终点，以日志判定成败）
bash run.sh exp16      # FreeRTOS 优先级反转与继承（二值信号量 vs 互斥量，两轮读数对照）
bash run.sh exp16-judge [日志]   # 只判定不跑 QEMU：对日志做变异，确认判据会失败（0/1/2）
bash run.sh aviation   # projects/aviation-node：PC 注入二进制帧 → 帧校验 → 模式状态机 → 自动判定（仅上行）
bash run.sh aviation-link  # 同上，另抓 USART2 下行二进制 → link.py 判定 → 两条通道一起判
bash run.sh tickrate   # 量化时基偏差
bash run_negative.sh   # 反向用例：01/02/03/06/07/08/09
```

`aviation` / `aviation-link` 是带**机器判定**的用例：跑完 QEMU 后由 `aviation/expect.py` 读上行日志，
24 项全部通过才返回 0，否则返回 1（日志不可读或无 `[stat]` 返回 2）。`aviation-link` 额外用
`link.py` 判下行 7 项，**两条通道分别判定、任一失败即整体失败**（下行坏了不能因为上行还是绿的
就放过）。其余用例只记录日志，由人对照 RESULT.md 判断。

`aviation-link` 用 `-serial file:logs/aviation_downlink.bin` 接第二路串口，因此同一次运行会同时产出
`logs/aviation_node.log`（USART1 ASCII）与 `logs/aviation_downlink.bin`（USART2 二进制）。多接一个
字符后端会让时序轻微偏移——第一次实现时正是它让 `rate` 段的降级消失（C13 从 5 次降为 4 次），
`inject.py` 的时段宽度因此按"多一路串口也不会翻盘"来定，见其 `PHASES` 注释。

该用例依赖一个**构建侧前提**：`projects/aviation-node` 在 `-DF103_QEMU=ON` 下默认带任务阻塞注入
（`AV_DEBUG_BLOCK_RX_MS=800`），日志里因此有 `[debug]` 行。用 `-DAV_DEBUG_BLOCK_RX_MS=0`
重建的镜像不会有这些行，C14 自动转成"未启用（本条不适用）"，其余项照常判定。

脚本必须在 **WSL/Linux** 里跑，因为 QEMU 装在那里；它通过 `/mnt/e/...` 访问 Windows 侧仓库，日志写回 `logs/`。

喂数据要延迟 1 秒：QEMU 的串口后端在 `USART_CR1.RE` 置位前会拒绝接收，输入若早早全部到达会滞留在管道里，表现为 `rx=0`——这不是固件的 bug，是仿真时序。

## 能力边界（摘要）

| 外设 | QEMU 模型 | 能验证什么 | 不能验证什么 |
| --- | --- | --- | --- |
| USART1/2/3 | 真实模型 | 收发、RXNE、TXE、中断、轮询 | 波特率精度、真实比特时序、ORE 溢出 |
| SysTick | 真实模型 | 计数、COUNTFLAG、中断、NVIC | 与 8 MHz 假设一致的时间（QEMU 是 24 MHz） |
| NVIC / SVCall / PendSV | 真实模型 | 异常优先级、嵌套、PendSV 挂起与上下文切换 | 与真实芯片的周期级差异 |
| Flash / SRAM | 真实 | 链接布局、越界访问触发 BusFault | 擦写寿命、选项字节 |
| DWT | **未建模** | — | `CYCCNT` 恒 0，`F103_09` 会死循环 |
| GPIO / EXTI | **未建模** | — | 读写恒 0，按键、LED、探针脉宽全部无法观测 |
| RCC / PWR / BKP | **未建模** | — | `SystemCoreClockUpdate()` 走 HSI 8 MHz 兜底分支 |
| TIM2–TIM7 | **未建模** | — | PWM、输入捕获、定时器延时全部无效 |
| I2C1/2 | **未建模** | — | — |
| SPI1/2 | 半建模 | `CR1` 有状态，写 `DR` 会置 `RXNE` | 无片选、无真实移位，W25Q 协议不可信 |
| IWDG | **未建模** | 写入不报错、可对照寄存器定义做构建级检查 | `PR`/`RLR`/`SR` 读回 0，不计时、不复位 |
| `RCC->CSR` 复位标志 | **未建模** | — | 各 `*RSTF` 读回 0，复位原因不可读 |

由此得出（每条都对应 `run.sh` / `run_negative.sh` 的一次实测，见 RESULT.md）：

- **可以在 QEMU 里验证**：`F103_04_UART_Echo`、`F103_05_UART_IRQ_Ring`、`F103_10_FreeRTOS_Tasks`、`F103_11_FreeRTOS_ISR_Queue`、`F103_12_FreeRTOS_Sync`、`F103_13_FreeRTOS_Timers`、`F103_14_FreeRTOS_Watchdog`（仅监督逻辑）、`F103_15_FreeRTOS_StackOverflow`（仅 paint 判据）、`F103_16_FreeRTOS_PriorityInversion`（优先级继承与等待时长比例），以及任何只用到"串口 + SysTick + NVIC"的代码。FreeRTOS 的调度器正建立在 SysTick/PendSV/SVCall 之上，中断→任务的交接又只用到 USART1 RXNE，软件定时器只用到 SysTick，心跳监督只用 SysTick + 信号量，栈溢出检出只用内核自己的检查宏与任务切换，所以任务切换、队列交接、`xQueueSendFromISR`/`portYIELD_FROM_ISR` 的效果、信号量/事件组语义、定时器守护任务与到期调度、心跳超时判定与停喂/恢复、堆与栈指标、栈底 paint 被写穿后的钩子调用都可实测——**只有绝对时间、真机中断延迟、`IWDG` 与复位原因不可信**。栈溢出检出另一难点是 QEMU 不会因越界写抛总线错误，所以真板上"先撞 HardFault 还是先到钩子"的顺序在仿真里得不到答案。
- **综合项目同样可以验证**：`projects/aviation-node`（`bash run.sh aviation-link`）把上面这些能力串成一条链——外部注入二进制帧（`SENSOR` 数据 + `STATUS` 地面站心跳）→ RXNE 中断 → 帧解析与 CRC16 → 三层有效性判定 → 模式状态机与降级/恢复 → 停喂门控与统计 → USART2 下行二进制帧。它能验证的是**软件数据链与判定逻辑**（上行 24 项 + 下行 7 项不变量），其中包括"链路层 / 数据层 / 地面站层各自独立计时"这件事——`hb_only` 段（只发心跳）`T=0 STALE=1 GND=0`、`hb_lost` 段（只发数据）`T=0 STALE=0 GND=1` 两个方向都有实测。CAN、ADC、示波器时序、`IWDG` 复位、下行发送抖动仍不可验证（`RESULT.md` §3.10）。
- **能跑起来、但结果不可信**：`F103_07`（I2C 永远无应答，只验证到超时分支；`CCR`/`TRISE` 读回 0 说明配置根本没生效）、`F103_08`（JEDEC ID 读出 `00 00 00`，擦写"成功"、回读全 0，自检停在 FAIL）。
- **跑起来但拿不到任何证据**：`F103_01/02/03/06`——它们只操作 GPIO/TIM，且不初始化串口，QEMU 里既无输出也无波形，等于空转。
- **会挂死**：`F103_09`——DWT 未使能（读回 0）、TIM4 计数恒 0，延时循环永不退出，打印到 TIM4 第一档就停住。
- **共同前提**：所有时间相关结论都不能从 QEMU 得出（时基偏差 2.90 倍，见 RESULT.md）。

## 已知失真

1. **SYSCLK 24 MHz vs 固件假设 8 MHz**：固件按 8 MHz 算 `SysTick->LOAD`，QEMU 按 24 MHz 计数，实测"固件毫秒"是真实时间的 2.90 倍。任何延时、超时、吞吐结论必须回到真机。
2. **串口无速率概念**：QEMU 的 USART 不做比特级移位，不管 `BRR` 配多少都立即收完。因此 `rx` 计数、回显内容可信，**速度不可信**。
3. **背压代替溢出**：QEMU 在 `RDR` 未读时拒绝新字节（`can_receive` 返回 0），不会像真 UART 那样置 ORE。`F103_05` 的 `drop`/`ore` 计数在仿真里恒为 0，突发 300 字节也不丢——这条路径只能上板验证。
4. **没有 GPIO**：所有"看灯闪""量脉宽"的验证方式在 QEMU 里完全失效，只剩串口打印。

## 机器判定（`aviation/expect.py`）

判定器**不比对硬编码时刻**，而是从日志里抽不变量——QEMU 虚拟时钟相对注入器墙钟的比例随主机
负载变化，同一事件跨运行会漂移，且**越靠后越大**（两次运行逐事件对照：`t≈1100` 差 41、
`t≈13000` 差 75、`t≈27000` 差 154，约 0.6% 固件时间）。写死某个 `t=` 断言的测试必然假失败。

| 组 | 判据 | 判的是什么 |
| --- | --- | --- |
| C1 | 启动 CRC 自检输出 `0x29B1` | 两端 CRC 实现对得上 |
| C2/C3 | ≥3 条 `[stat]`；`INIT → NORMAL` 发生 | 跑到了稳态 |
| C4 | `T`/`STALE`/`CRC`/`RNG`/`RATE`/`GND` 各至少置位一次 | 剧本每类故障都触发了判定 |
| C5 | `NORMAL→DEGRADED ≥4` 且 `DEGRADED→NORMAL ≥3` | 不能只降不恢复 |
| C6a/b/c | 进过 `SAFE`；从 `SAFE` 经 `DEGRADED` 恢复；DEGRADED 停留 2850–3150 ms | 3000 ms 门限真的生效 |
| C7 | `gap == crc + 静默时隙 + 丢帧数` | 丢帧与错帧统计自洽 |
| C8 | 各条 `[stat]` 的 `heap` 恒定 | 稳态无泄漏 |
| C9 | 每任务 `余量 ≥32 字` 且 `相对 ≥25%` | 栈判据（与 `F103_12` 同口径） |
| C10 | `feed=OFF` ⇔ 存在阻塞原因（`T`/`STALE` 置位或 `hb > 400`） | 喂狗门控的"与"语义。**不是** `feed=ON ⇔ flags=0`：`CRC/RNG/RATE/SRC` 不参与门控，`feed=ON` 且 flags≠0 是合法状态；早期按"flags 全 0"判会偶发误报 |
| C11 | `RNG` 与 `RATE` 不同时置位 | 剧本各段只改一个变量 |
| C12 | `SAFE⇒cmd 0`、`NORMAL⇒cmd -50`、`DEGRADED∈{0,-50}` | 输出回中位/控制律生效 |
| C13 | `NORMAL→DEGRADED` 次数**恰好**等于剧本预期 | 短时丢帧没有多降一次 |
| C14 | 注入窗口时长与计划一致、阻塞前心跳未超时、窗口内有 `reason` 含 `HB` 且 `hb > 400` 的 `[gate]` | 任务卡住 → 心跳超时 → 停喂 这条因果链成立（未启用注入时本条直接通过并标注"不适用"） |
| C15 | 所有 `[stat]` 的 `age`/`hb` 均 < 2³¹ | 时间戳年龄没有无符号下溢。实测抓到过 `age=4294967292`（已修）；`hb` 走同一路径，下溢会让门控误停喂 |
| C16 | `GND` 至少置位一次；每次上升沿到下一次 `T`/`STALE` 上升沿之间没有 `NORMAL→DEGRADED` 或 `DEGRADED→SAFE`；`GND` 置位期间至少有一条 `stat` 处于 `NORMAL` | 地面站失联不改变飞行模式（`AV_F_GROUND` 不在 `AV_F_MODE_MASK` 里）。窗口右端用 `>=`：`silent` 段里 `GND` 可能与 `T`/`STALE` 同 tick 到期，此时窗口为空——不可归因，判定器主动弃权而不是把那次降级算到 `GND` 头上 |
| C17 | 存在相邻两条 `[stat]`：`frames` 不变、`T=0`、`GND=0`、`gage ≤ 500`，且后一条 `STALE=1` | 链路层与数据层已分离：`hb_only` 段只发心跳，数据层报 `STALE` 而链路层不报 `T`。若"心跳还在"被误当成"数据可用"，这一对就不存在 |

**剧本相关的常量**，改 `inject.py` 时必须同步：

- `--silent-slots 280`：**所有不发 `SENSOR` 的时隙总数**，用于 C7。它是
  `silent`（100 = 2.00 s × 50 Hz）+ `hb_only`（180 = 3.80 s × 50 Hz）之和。
  `hb_only` 段虽然还在发地面站心跳，但 `SENSOR` 序号照旧在推进，缺口必须计入——
  漏掉它 C7 会稳定失败，而且失败原因看起来与心跳无关，容易误导。
- `--dropped-frames 3`：`drop` 段主动丢掉的帧数，用于 C7。
- `--expected-down 6`：剧本应产生的 `NORMAL→DEGRADED` 次数，用于 C13。六次分别是
  `badcrc`、`range`、`rate`、`silent`、`hb_only`、以及**剧本放完后的收尾静默**
  （注入器 14.3 s 退出、QEMU 继续跑到超时，链路无字节 → `T`/`STALE` → 降级）。
  写成 5 会让"丢帧多降一次"与"少一次收尾降级"互相抵消，判据就失去意义。
- `STACKS = (("rx", 224), ("ctrl", 224), ("health", 256))`：与 `main.c` 的 `xTaskCreate`
  逐项对应，C9 用总栈量算相对余量。改栈必须同步这里，否则相对判据会算错。
- C12 的 `-50` 来自控制律 `-1.00° × 0.5` 与参考空速 15.0 m/s，改注入值就会变。
- `RX_HB_TIMEOUT_MS = 400`（脚本常量，与 `main.c` 同值）：C10/C14 用它判"阻塞前心跳是否已超时"
  与"窗口内是否真的越过门限"。注入时长与时刻**不做成脚本常量**——C14 直接读 `[debug]` 行里的
  `plant` 与实测 `dur` 自比，改 `-DAV_DEBUG_BLOCK_RX_MS` 不用同步判定器。

每加一段不发 `SENSOR` 的剧本段，`--silent-slots` 与 `--expected-down` 都可能要跟着改；
这两处是最容易忘、失败信息又最像"固件坏了"的地方。

**不判的东西**（没有证据，判定器也不假装有）：CAN、ADC、示波器/逻辑分析仪时序、`IWDG`
复位与 `RCC->CSR` 复位原因、下行帧的发送抖动（QEMU 串口没有比特级时序），以及长稳/抖动。
判定通过只说明**软件数据链的不变量成立**。

### 延迟汇总

判定之后还会打印六类延迟（纯统计，不影响退出码）。下面是当前 `logs/aviation_node.log` 的输出，
重跑 `run.sh aviation-link` 会连同日志一起被覆盖：

```text
[timing] 延迟汇总（统计，不参与判定）：
  故障置位 → 降级：0 tick、0 tick、0 tick、0 tick、0 tick、0 tick（6 次）
  故障清除 → 恢复 NORMAL：82 tick、123 tick、101 tick（3 次；不含 SAFE 之后）
  故障清除 → SAFE→DEGRADED：224 tick、222 tick（2 次；两级恢复的第一级）
  DEGRADED 进入 → SAFE 进入：3005 ms、3004 ms、3009 ms（3 次；门限 3000）
  SAFE 进入 → SAFE→DEGRADED：2508 ms、6924 ms（2 次；受剧本恢复时刻控制）
  阻塞开始 → 心跳判超时：415 tick（1 次；门限 400）
```

跨运行区间（每项都标了起点，同一状态迁移可以有两个不同读数，取决于从哪儿算）：

| 延迟 | 区间 | 起点 |
| --- | --- | --- |
| 故障清除 → 恢复 NORMAL | 82–163 tick | 最后一类故障标志清除。下界不稳定：`good_streak` 由 rx 自增、ctrl 只在 `fault≠0` 时清零，清除那一拍 streak 可能已累积 1–2 |
| 故障清除 → `SAFE→DEGRADED` | 205–246 tick | 同上（SAFE 之后的第一级恢复） |
| `SAFE→DEGRADED` → NORMAL | 161–185 tick | 上一行迁移完成的时刻 |
| 进入 DEGRADED → 进入 SAFE | 3003–3017 ms | 门限 3000 ms，判据生效 |
| 进入 SAFE → 退出 SAFE | 2508–6924 ms | **不是固件判据**，由剧本何时恢复注入决定 |
| 阻塞开始 → 心跳判超时 | 415–496 tick | `[debug]` 行；其中 400 是门限本身，余量是 health 的 100 ms 采样颗粒 |

**`0 tick` 不能读成"检测零延迟"。** 它量的是"标志位被置上 → 状态机改模式"，两者常落在
同一 tick（rx 设标志、ctrl 同 tick 读到），所以是 0。"故障真正发生 → 被检出"这一段测不了：
故障发生在 PC 侧，固件毫秒与真实秒差 2.90 倍且没有共同时基，跨端对齐只能上真板 + 逻辑分析仪。

单独复跑判定（不重跑 QEMU）：

```bash
python3 aviation/expect.py logs/aviation_node.log        # 退出码 0/1/2
python3 aviation/expect.py --silent-slots 280 --dropped-frames 3 --expected-down 6 logs/aviation_node.log
python3 aviation/link.py logs/aviation_downlink.bin --ascii logs/aviation_node.log
```

### 下行判定（`aviation/link.py`）

`link.py` 解 USART2 的二进制流（`AA 55 | TYPE 0x81 | SEQ | LEN 18 | PAYLOAD | CRC16-LE`），
用**注入器里同一份** `crc16_ccitt_false` 做校验——两边算得不一样就必然全错，这本身就是一次交叉核对。

| 组 | 判据 | 判的是什么 |
| --- | --- | --- |
| L1 | 至少解出 1 帧 | USART2 确实接到了第二个后端 |
| L2 | 所有完整帧 CRC 正确 | 组帧与 CRC 覆盖范围正确 |
| L3 | `SEQ` 逐帧 +1（允许 255→0 回绕） | 没有跳号/重复发送 |
| L4 | `TYPE` 恒 `0x81`、`LEN` 恒 18 | 帧头字段与协议文档一致 |
| L5 | `uptime` 严格递增 | 载荷布局没有错位（错位会读出乱序时间） |
| L6 | 模式序列含 `NORMAL` 且不只有 `NORMAL` | 下行确实反映了故障段，不是一直在报"正常" |
| L7 | 与同 `t` 的 `[stat]` 交叉核对 mode/flags/frames/crc/gap | 两条通道对同一时刻的描述一致 |

L7 的三条容差都有原因，不是宽松：`frames/crc/gap` 按"只增 + 增量 ≤3"判，因为
`av_downlink_status()` 的 `TXE` 轮询期间 rx（优先级 3）会抢占 health（优先级 2）并再接受几帧——
逐字相等会把抢占判成缺陷；`mode` 允许不同，但**必须**伴随一条同 `t` 的 `[mode]` 行（ctrl 抢在
两次读之间改了模式）；`flags` 则要求严格相等（它们来自同一个局部变量）。

当前实测：13600 字节 / 544 帧，CRC 与长度/类型错误各 0，`SEQ` 0→31、`uptime` 101→54767 ms，
帧间隔中位 100 ms、最大 117 ms，L1–L7 全通过。帧间隔只报告不判定——QEMU 时基是固件的 2.90 倍，
绝对间隔没有校准意义。

**已知盲区**：L1–L7 不检查流的末尾是否完整。把 bin 尾部截掉 10 字节（小于一帧）仍然 7/7 通过，
因为 QEMU 被 `timeout` 杀掉时本来就会在帧中间截断，两者无法区分（`link.py` 的 `parse()` 在
`end > n` 时直接 `break`）。尾部丢失 ≤1 帧不可检出；要检出需要真板上的帧计数与地面站期望值对账。

## 相关文档

- [RESULT.md](RESULT.md)：本次实测的完整记录与环境版本。
- [`projects/aviation-node/README.md`](../../projects/aviation-node/README.md)：航空节点的构建、运行、实测时间线与硬件依赖项。
- [`projects/aviation-node/docs/protocol.md`](../../projects/aviation-node/docs/protocol.md)：帧格式、CRC 参数、故障位与模式迁移规范。
- [`projects/aviation-node/docs/test-report.md`](../../projects/aviation-node/docs/test-report.md)：故障注入测试报告（九类用例 + 判定 + 负向验证）。
- [`projects/aviation-node/docs/task-timing.md`](../../projects/aviation-node/docs/task-timing.md)：任务时序、优先级取舍与实时性边界。
- [`projects/aviation-node/docs/architecture.md`](../../projects/aviation-node/docs/architecture.md)：分层视图、模块依赖、运行时交互与资源预算。
- `Linux/experiments/F103_common/ld/STM32F100RB_qemu.ld`：QEMU 布局链接脚本，注释里写了为什么必须单开一份。
- `Linux/experiments/F103_common/cmake/f103.cmake`：`F103_QEMU` 选项的定义处。
