# QEMU 仿真实测记录（2026-10-26 ~ 2026-10-27）

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
[11] IWDG：写 PR=4 RLR=0x0FFF，读回 PR=0 RLR=0 SR=0x00000000（读回 0 说明未建模，看门狗计时与复位不可验证）
[12] RCC->CSR = 0x00000000：IWDGRSTF=0 PORRSTF=0 PINRSTF=0 SFTRSTF=0
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
| IWDG | `KR`（含 `0xCCCC` 启动）写入不报错，`PR`/`RLR`/`SR` 读回 0 | 未建模：不启动计时，不会复位 |
| `RCC->CSR` 复位标志 | `IWDGRSTF`/`PORRSTF`/`PINRSTF`/`SFTRSTF` 全 0 | 未建模：复位原因不可读 |

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

### 3.5 F103_10 FreeRTOS 任务/队列/互斥量（`logs/exp10_freertos.log`）

构建体积（`-Og -g3`，gcc 14.2.1，零告警）：FLASH 13280 B / 128 KB（10.13%），RAM 6400 B / 8 KB（78.13%），其中 `ucHeap` 固定 4096 B。

6 秒真实时间（17 次 monitor 采样）的原始片段：

```text
[F103_10] FreeRTOS V11.3.0 on STM32F103, HCLK=8000000 Hz, tick=1000 Hz
任务已创建，启动调度器（初始空闲堆 1112 B）
[consume] seq=1 tick=1
[consume] seq=2 tick=206
[consume] seq=3 tick=409
...
[monitor] tick=1002 produced=5 consumed=5 dropped=0 queued=0
[res] heap_free=496 B, stack_hwm consume=154 produce=126 monitor=98 words
...
[monitor] tick=17095 produced=85 consumed=85 dropped=0 queued=0
[res] heap_free=496 B, stack_hwm consume=154 produce=126 monitor=90 words
[consume] seq=88 tick=17584
```

| 指标 | 实测 |
| --- | --- |
| `consume` 条数 / 末 tick | 88 / 17584 |
| 生产间隔（tick 差） | 201×15、202×49、203×22、204×1（众数 202，期望 200） |
| 上报间隔（tick 差） | 1003–1010（众数 ≈1006，期望 1000） |
| produced / consumed / dropped / queued | 85 / 85 / 0 / 0 |
| 空闲堆 | 创建任务后 1112 B → 调度器启动后 496 B，17 次采样**恒定** |
| 栈高水位（剩余字） | consume 154/256、produce 126/160、monitor 98→90/192 |

读法：

- **比例可信**：`consume : monitor = 5 : 5`（每 1000 tick 恰好 5 条），与 200 ms : 1000 ms 的设计比例严格一致，说明两个任务的延时都跟着同一个 SysTick 走。
- **绝对时间不可信**：末 tick 17584 / 6000 ms ≈ 2.93 倍，与 §3.3 的裸机时基偏差同源。
- **间隔略大于设计值是预期行为**：`vTaskDelay` 是相对延时，实际周期 = 设计周期 + 执行时间（队列投递、切换、`printf` 的忙等发送）。monitor 每次打两行约 150 B，115200 下约 13 ms 的发送时间直接叠加进周期。
- **`heap_free` 恒定**说明没有持续分配；**`stack_hwm` 恒定**说明未覆盖到更深的调用路径。
- **带 `printf` 的任务栈用量是纯计算任务的 3 倍**（≈408 B vs 136 B），这是给栈深度定值的实际依据。

不能由此得出的结论：真机上的 200 ms 周期精度、`dropped` 路径（从未触发）、优先级继承（互斥量竞争从未发生）、三个钩子函数（均未触发）。

### 3.6 F103_11 中断 → 任务（`logs/exp11_isr_queue.log`、`logs/exp11_burst.log`）

构建体积：FLASH 13616 B / 128 KB（10.39%），RAM 6400 B / 8 KB（78.13%），零告警。
堆内余量 1048 B（比 §3.5 多 552 B：本实验只有两个任务）。

用例 1，两次输入 `hello\r\n` / `world\r\n`（7 秒真实时间，20 次采样）：

```text
[F103_11] FreeRTOS ISR→task: USART1 RXNE + queue(depth=32), NVIC prio=5 (= MAX_SYSCALL)
任务已创建，启动调度器（RXNE 中断由 consume 任务使能）
[stats] tick=1001 rx=0 lines=0 qdrop=0 ore=0 trunc=0 queued=0 lat_max=0 heap_free=1048
[line 1] "hello" len=5
[stats] tick=3008 rx=7 lines=1 qdrop=0 ore=0 trunc=0 queued=0 lat_max=2 heap_free=1048
[line 2] "world" len=5
[stats] tick=6020 rx=14 lines=2 qdrop=0 ore=0 trunc=0 queued=0 lat_max=2 heap_free=1048
```

用例 2，300 字节无换行突发（6 秒真实时间）：

```text
[line 1] "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" len=44
[stats] tick=3005 rx=302 lines=1 qdrop=0 ore=0 trunc=4 queued=0 lat_max=2 heap_free=1048
```

读法：

- **字节数自洽**：每次 `hello\r\n` 使 `rx` +7、`lines` +1；`\r` 被任务忽略，`len=5`。
- **截断路径自洽**：每轮截断存 63 字节、丢 1 字节（共 64），`300 = 4×64 + 44` → `trunc=4`、末行 `len=44`。ISR 投递、任务累积、计数三处没有错位。
- **`lat_max=2` tick 不能当实时性指标**：QEMU 无中断延迟模型，时基又是真实的 2.93 倍；只能说明"ISR 投递后任务在数 tick 内取走"这一因果关系成立。
- **`qdrop=0` 说明队列从未满**（消费速度远高于 115200 输入），因此 ISR 侧丢弃分支未被覆盖；`ore=0` 同理。

### 3.7 F103_12 同步原语（`logs/exp12_sync.log`）

构建体积：FLASH 15988 B / 128 KB（12.20%），RAM 6408 B / 8 KB（78.22%），零告警；
`text 15888 / data 100 / bss 6308`。比 §3.6 多 2372 B Flash（`event_groups.c` + 四段演示），RAM 多 8 B。

内核堆（4 KB）分段记账，四行相加正好 4096 B：

| 阶段 | 占用 |
| --- | --- |
| 4 个信号量 + 1 个事件组 | 368 B |
| 三个任务（栈 480 字 + 3 个 TCB） | 2232 B |
| idle 任务（128 字栈 + TCB） | 616 B |
| 稳态余量（20 次采样恒定） | 880 B |

用例：一次 7 字节输入（`hello\r\n`），7 秒真实时间。完整输出（38 行，含 20 行 `[res]` 周期采样）：

```text
[F103_12] FreeRTOS V11.3.0 sync primitives, HCLK=8000000 Hz, tick=1000 Hz
[F103_12] pool=3 (init 1), USART1_IRQ prio=5
[F103_12] 五个同步对象（4 个信号量 + 1 个事件组）用掉 368 B
[F103_12] 二值信号量初值为空（符合预期）
[F103_12] 初始空闲堆 1496 B，启动调度器
[sync] bin waited=802 ticks, take ok
[sync] pool take1 ok, count=0
[sync] pool take2(0ms) result=0, count=0
[sync] pool refill take ok, count=0
[sync] pool refill take ok, count=0
[sync] event wait_all got=0x03 waited=200 ticks
[sync] event timeout got=0x00 waited=300 ticks
[res] heap_free=880 B, stack_hwm core=86 feeder=96 monitor=58 words
[sync] isr wake#1 bytes=7 gives=2
[sync] isr wake#2 bytes=7 gives=2
[sync] isr 空闲 300 tick 后无新事件，结束采集
[sync] isr summary bytes=7 gives=2 takes=2 coalesced=5
[sync] done heap_free=880 B
```

读法：

- **二值信号量**：`waited=802` 对应 feeder 的 800 tick 延时，差值来自 tick 边界对齐；两次独立运行分别得到 801 / 802。
- **计数信号量**：初始 1 个，`take1` 后 `count=0`；`take2(0ms) result=0`（`pdFALSE`）证明"取光后不阻塞立即失败"；feeder 归还 2 个后两次 `refill take ok`。
- **事件组**：`wait_all got=0x03 waited=200` 说明"两位都置位才返回"成立（第二位晚 200 tick 置位）；`event timeout got=0x00 waited=300` 说明无人置位的位会精确按请求超时。
- **事件合并**：7 个字节只形成 2 次成功 give（`bytes=7 gives=2`），`coalesced=5` 是合并量而非丢数据；若用 §3.6 的深度 32 队列，同样输入会形成 7 项。
- **栈水位**：core 86/192 字、feeder 96/128 字、monitor 58/160 字；按"绝对 ≥32 字、相对 ≥25%、全路径跑完再读"三条判据全部通过，但 monitor 最接近红线（36%）。
- **堆恒定**：20 次采样 `heap_free=880 B` 无变化，说明四段演示没有动态分配。

不可外推的部分：

- `gives=2` 由 QEMU 的串口投递时序决定（USART1 IRQ 优先级 5 高于 PendSV，剩余字节的 ISR 先于任务切换跑完），真机字节间隔不同，合并次数会变。可迁移的只有"二值信号量会合并事件"这条定性结论。
- 等待时间的 tick 数同样受 2.90 倍时基影响，不能换算成真机毫秒。

### 3.8 F103_13 软件定时器（`logs/exp13_timers.log`）

构建体积：FLASH 18392 B / 128 KB（14.03%），RAM 6864 B / 8 KB（83.79%），零告警；
`text 18292 / data 100 / bss 6764`。比 §3.7 多 2404 B Flash（`timers.c` + 第三个定时器与报告代码），RAM 多 456 B。

内核堆（4 KB）的五段记账，每段都是相邻两次读数的差，五行相加正好 4096 B：

| 阶段 | 占用 | 读数来源 |
| --- | --- | --- |
| 互斥量 | 96 B | `4096 − 4000` |
| 两个任务（192 + 160 字栈 + TCB） | 1616 B | `4000 − 2384` |
| 守护任务 + 命令队列(5) + idle | 1504 B | `2384 − 880`（全部由 `vTaskStartScheduler` 创建） |
| 三个 `Timer_t` | 144 B | `880 − 736`，每个 48 B |
| 稳态余量（20 次采样恒定） | 736 B | `[res]` 行 |

用例：无串口输入，7 s 真实时间（含 20 行 `[res]` 采样）。原始输出的关键片段：

```text
[F103_13] FreeRTOS V11.3.0 software timers, HCLK=8000000 Hz, tick=1000 Hz
[F103_13] configUSE_TIMERS=1 daemon prio=4 queue=5 stack=160 words
[F103_13] 互斥量建好后堆余量 4000 B
[F103_13] 两个任务建好后堆余量 2384 B，启动调度器（守护任务、定时器命令队列与 idle 由内核创建）
[tmr] 三个 Timer_t 用掉 144 B（堆余量 880 → 736）
[tmr] once fire#1 act=310 ideal=310 drift=0 in_daemon=1
[tmr] once fire#2 act=615 ideal=615 drift=0（reset 生效，单次定时器可复用）
[tmr] normal period#1 act=211 ideal=210 drift=1 dur=0
[tmr] normal fast#1 act=61 ideal=60 drift=1
[tmr] normal fast_total=16 max_drift=1
[tmr] 注入：周期回调忙等 60 tick（模拟回调里做重活）
[tmr] stalled period#5 act=1011 ideal=1010 drift=1 dur=60
[tmr] stalled period#8 act=1611 ideal=1610 drift=1 dur=60
[tmr] 注入结束：回调恢复只记账
[tmr] after fast#20 act=1071 ideal=1010 drift=61
[tmr] after fast#21 act=1071 ideal=1060 drift=11
[tmr] after fast#22 act=1111 ideal=1110 drift=1
[tmr] after fast_total=40 max_drift=61
[tmr] 已停止：periodic 前=10 后=10 差=0；fast 前=40 后=40 差=0（都应为 0）
[tmr] done heap_free=736 B
```

读法：

- **回调上下文**：`in_daemon=1` 由 `xTaskGetCurrentTaskHandle() == xTimerGetTimerDaemonTaskHandle()`
  在回调内比对得出，不是照抄文档。
- **单次 / 周期语义**：`once` 到期后休眠，`xTimerReset` 后重新跑满 300 tick（`drift=0`）；
  周期定时器每 200 tick 自动重装（`drift=1`）。
- **慢回调不影响自身相位**：`stalled` 段 `dur=60` 但 `drift` 仍是 1。原因是
  `prvProcessExpiredTimer()` 在调用回调**之前**完成重装（`timers.c:729-740`），
  重装基准是到期时刻而非执行时刻。
- **代价是丢节拍**：守护任务被占住期间到期的 fast 定时器，`#20`（ideal 1010）与
  `#21`（ideal 1060）都在 tick 1071 执行，随后 `#22` 回到 `drift=1`。
  对应 `prvReloadTimer()` 的 `while` 追赶循环（`timers.c:702-710`）：错过的一拍被补发一次，
  不会无限累积，也不会永久偏移。
- **`xTimerStop` 的判据**：停后静置 600 ms，两个计数差值都是 0（总采样 10/40 不再增长）。
- **堆恒定**：20 次采样 `heap_free=736 B` 无变化。
- **栈水位**：app 74/192、monitor 58/160、守护任务 132/160 字。首次构建时 monitor 给 128 字
  只剩 26 字、守护任务（`configTIMER_TASK_STACK_DEPTH=128`）只剩 28 字，均低于"绝对 ≥32 字"判据，
  已分别改为 160 字——这条修正来自实测，不是事后推测。

不可外推的部分：

- `drift` 的单位是固件 tick，受 2.90 倍时基影响；`drift=61` ≈ 21 ms 真实时间，不能当真机数据。
- `drift=1` 的成因（tick 边界对齐 vs 回调执行耗时）未分解，需要 DWT 或更高分辨率时基。
- 守护任务与命令队列吃掉的 1504 B 是**本配置**（栈 160 字、队列 5 项、优先级 4）下的数值，
  改任一参数都要重测。

### 3.9 F103_14 心跳与看门狗（`logs/exp14_watchdog.log`）

构建体积：FLASH 14944 B / 128 KB（11.40%），RAM 6424 B / 8 KB（78.42%），零告警；
`text 14844 / data 100 / bss 6324`。比 §3.8 少 3448 B Flash——该目标不链接 `timers.c`，
也不需要守护任务与命令队列，这与 `f103_add_freertos(<target>)` 不带 `MODULES` 的配置一致。
内核堆：三个任务 + 互斥量 + 二值信号量占用 2536 B，余量 1560 B。

用例：无串口输入，5 s 真实时间；两轮故障注入只差"连续几次 STALE 才恢复"（3 与 6）。
原始输出的关键片段：

```text
[F103_14] HB: sensor=100 ms io=150 ms timeout=400 ms check=200 ms，注入轮次 2（恢复门限 3 / 6 次 STALE）
[F103_14] IWDG 路径 未启用（QEMU 未建模 IWDG）；RCC->CSR=0x00000000（IWDGRSTF=0）
[F103_14] IWDG 代码地址 init=0x0800010D feed=0x08000131（本构建不执行）
[hb] t=808 sensor age=9 io age=59 OK feed=ON feeds=4
[hb] R1 快恢复 注入：t=808 sensor 阻塞在 s_gate 上，恢复门限 = 连续 3 次 STALE
[hb] t=1217 sensor age=417! io age=16 STALE feed=OFF streak=1
[hb] t=1422 sensor age=622! io age=71 STALE feed=OFF streak=2
[hb] t=1623 sensor age=823! io age=122 STALE feed=OFF streak=3
[hb] R1 快恢复 恢复动作：连续 3 次 STALE（门限 3），解除 sensor 阻塞
[hb] R1 快恢复 恢复：t=1834 重新喂狗，停止喂狗窗口 819 tick（IWDG 名义 998 ms → 软件自愈先发生）
[hb] R2 慢恢复 注入：t=2847 sensor 阻塞在 s_gate 上，恢复门限 = 连续 6 次 STALE
[hb] t=4267 sensor age=1433! io age=66 STALE feed=OFF streak=6
[hb] R2 慢恢复 恢复：t=4479 重新喂狗，停止喂狗窗口 1424 tick（IWDG 名义 998 ms → 真板会先复位）
[hb] 汇总：checks=26 stale_total=9 blocked=9 feeds=17 心跳次数 sensor=30 io=35
[hb] 栈水位（余量/总量，字）：sensor=132/160 io=106/128 health=108/224（判据：绝对 ≥32、相对 ≥25%）
```

读法：

- **心跳与"与"语义**：两轮故障期间 `io` 的 age 最大只有 122 / 112 tick（始终健康），
  但 `feed` 仍然是 `OFF`——只要有一个通道不健康就整体停喂。这是可观测的、不是设计意图的复述。
- **检测延迟**：注入 → 首次 STALE 为 409 / 410 tick，等于 `HB_TIMEOUT_MS(400)` 加上落在
  两次检查之间的余量；`stale_total = 9 = blocked = 9`（3 + 6）、`feeds = 17 = 26 − 9`，三项自洽。
- **停喂窗口与看门狗门限**：R1 为 819 tick、R2 为 1424 tick，正好跨过 IWDG 名义超时 998 ms。
  这把"真板上会不会复位"从猜测变成可计算：窗口短于超时 → 软件自愈先发生；长于超时 → 复位先发生。
  连续两次运行的同项差值 ≤5 tick（817→819、1419→1424），不改变任何结论方向。
- **恢复完整性**：两轮恢复后 `feed=ON` 连续 5 次、`sensor age` 回到 0~15，没有抖动性反复停喂。
- **栈水位**：三个任务全部满足"绝对 ≥32 字 / 相对 ≥25% / 全路径跑完再读"。
- **硬件路径的证据强度**：`nm` 在 `main.c.obj` 与最终 ELF 里都能看到 `f103_watchdog_init` /
  `f103_watchdog_feed`（ELF 中为 `0x0800010C` / `0x08000130`；日志打印 `0x0800010D` / `0x08000131`，
  差异是 Thumb 函数指针的 bit0），`objdump` 里 `0x40003000` 出现 2 次。
  第一版只加了 `__attribute__((used))`，结果 ELF 里两个符号为空、`0x40003000` 出现 0 次——
  `used` 只约束编译器，`--gc-sections` 仍然会删未被引用的段；改成在启动日志里打印函数地址后
  才留在镜像里。这一步属于"进入镜像"级别，不等于"在目标上执行"。

不可外推的部分：

- `819` / `1424` 是固件 tick，受 2.90 倍时基影响，不能读成毫秒级实时性。
- "真板会先复位"是按 `PR=÷64`、`RLR=624`、LSI 40 kHz 的**名义** 998 ms 折算的推论。
  F1 的 LSI 容差为 30~60 kHz（超时约 665~1330 ms），所以 R1 的 819 落在不确定区间，
  只有真板能判定；R2 的 1424 在两端都超过门限。
- IWDG 的计时、复位、`SR` 更新标志和 `RCC->CSR` 的 `IWDGRSTF` 全程未在硬件上验证，
  QEMU 读回全 0 见 §2 探针 `[11]`/`[12]`。
- 恢复动作是"解除注入 + 放行信号量"，不是真实的子系统重启；后者的耗时通常远大于本实验的秒级窗口。

### 3.10 projects/aviation-node 航空主题节点（`logs/aviation_node.log`）

构建体积：FLASH 18256 B / 128 KB（13.93%），RAM 6552 B / 8 KB（79.98%），零告警；
`text 18156 / data 100 / bss 6452`。其中调试注入段占 288 B（同一源码 `-DAV_DEBUG_BLOCK_RX_MS=0`
时为 17968 B、RAM 6544 B）。内核堆：三任务 + 队列 + 互斥量之后余 736 B，稳态 120 B。
下行通道与上行 `STATUS` 分支的历史差分（732 B / 224 B）未在本次重建中逐项复测，只保留总量。
源码见 `projects/aviation-node/`，协议与状态规范见该目录 `docs/protocol.md`。

用例：PC 侧 Python 注入器按 50 Hz 发 `SENSOR`、10 Hz 发 `STATUS`（地面站心跳），剧本 14.3 s
真实时间（20 s 超时窗口内固件时间跑到约 55 s），13 个阶段依次只改一个变量——含连丢 3 帧的
`drop` 段、只发心跳的 `hb_only` 段、只发数据的 `hb_lost` 段。另有一次固件侧的任务阻塞注入
（编译期，`AV_DEBUG_BLOCK_RX_MS=800`，落在静默段内）。同一事件跨运行会漂移且**越靠后越大**
（两次运行逐事件对照：`t≈1100` 差 41、`t≈13000` 差 75、`t≈27000` 差 154，约 0.6% 固件时间，
源于 QEMU 虚拟时钟与注入器墙钟的比例变化），下面片段取自 2026-10-27 的最后一次运行
（`bash run.sh aviation-link`：USART1 日志 + USART2 二进制下行，同一次运行）；
`logs/` 里的文件会被后续运行覆盖，要机器判定请用 `aviation/expect.py` 与 `aviation/link.py`。

```text
[av-node] CRC16-CCITT-FALSE("123456789") = 0x29B1 PASS
[av-node] 判据: 链路超时 300 ms / 数据陈旧 300 ms / CRC 连续 3 帧 / 故障保持 1000 ms
[av-node] IWDG 路径 未启用（QEMU 未建模）；代码地址 init=0x0800010D feed=0x08000131
[av-node] 下行 USART2 已启用：TYPE=0x81 LEN=18 baud=115200（PCLK1=8000000 Hz 假设）
[av-node] 启动调度器，初始空闲堆 736 B
[gate] t=306 feed=OFF reason=FLAGS hb=106 flags=0x01
[mode] t=1130 INIT -> NORMAL flags=0x00 cmd=0 air=150 pitch=100
[mode] t=5274 NORMAL -> DEGRADED flags=0x04 cmd=-50 air=150 pitch=100
[mode] t=8469 NORMAL -> DEGRADED flags=0x08 cmd=-50 air=150 pitch=100
[mode] t=11484 NORMAL -> DEGRADED flags=0x10 cmd=-50 air=150 pitch=100
[mode] t=13636 NORMAL -> DEGRADED flags=0x43 cmd=-50 air=150 pitch=100
[fault] t=13636 set=0x43 clear=0x00 (T=1 STALE=1 CRC=0 RNG=0 RATE=0 SRC=0 GND=1)
[debug] t=16534 rx 阻塞开始 plant=800 ms（其他任务继续运行）
[mode] t=16641 DEGRADED -> SAFE flags=0x43 cmd=-50 air=150 pitch=100
[gate] t=16949 feed=OFF reason=HB+FLAGS hb=415 flags=0x43
[debug] t=17334 rx 阻塞结束 dur=800 ms
[mode] t=19310 DEGRADED -> NORMAL flags=0x00 cmd=0 air=150 pitch=100
[mode] t=22510 NORMAL -> DEGRADED flags=0x02 cmd=-50 air=150 pitch=100
[mode] t=32604 DEGRADED -> NORMAL flags=0x40 cmd=0 air=150 pitch=100
[stat] t=35256 mode=NORMAL cmd=-50 flags=0x40[T=0 STALE=0 CRC=0 RNG=0 RATE=0 SRC=0 GND=1] air=150 pitch=100 age=5 frames=308 crc=30 len=0 type=0 gap=313 dup=0 hb=8 gnd=93 gage=3319 feed=ON(168/182) heap=120 hwm=100/104/88
[mode] t=39387 NORMAL -> DEGRADED flags=0x43 cmd=-50 air=150 pitch=100
[mode] t=42396 DEGRADED -> SAFE flags=0x43 cmd=-50 air=150 pitch=100
```

读法：

- **七类异常各有独立证据**：错帧（`CRC`，连续 3 帧）、越界（`RNG`，单帧）、突变（`RATE`，单帧）、
  静默（`T`+`STALE`+`GND`）、只发心跳（`STALE` 单独）、只发数据（`GND` 单独）、任务阻塞（`hb`）。
  恢复都必须"连续有效帧 + 标志清零"。
- **三层活性已拆开**：`silent` 段三层全停（`t=13636 set=0x43`，三个超时**同一 tick** 到期）；
  `hb_only` 段只停数据层（`t=22510 flags=0x02`，`T=0`）；`hb_lost` 段只停地面站层
  （`[stat] t=35256 T=0 STALE=0 GND=1 gage=3319`，而模式仍是 `NORMAL`）。
  后一条正是 `AV_F_GROUND` 不在 `AV_F_MODE_MASK` 里的证据：心跳已断 3.3 s，系统照常完成
  `SAFE→DEGRADED→NORMAL` 两级恢复（`t=32438` / `t=32604`），若 `GND` 参与 `fault`，
  `good_streak` 会被每帧清零，这条路走不通。
- **安全态的进入与退出都有数字**：`DEGRADED -> SAFE` 停留 3003–3017 ms（多次运行；本轮
  3005 / 3004 / 3009），`SAFE -> DEGRADED` 用 205–246 tick（5 个有效帧，本轮 224 / 222），
  `-> NORMAL` 再用 161–185 tick（3 个）。完整逐事件时间线见 `projects/aviation-node/README.md` §10.2。
- **计数自洽**：`gap=313 = 30（CRC 错帧）+ 280（不发 `SENSOR` 的时隙：静默 100 + 只发心跳 180）
  + 3（丢帧段）`，与 `crc=30` 交叉核对一致；`frames=377`（690 时隙 − 280 − 3 − 30）、`dup=0`。
  `280` 这个数容易被漏——`hb_only` 段仍在发心跳，但 `SENSOR` 序号照旧推进。
- **丢帧只体现在 `gap` 上**：`drop` 段 7.60–7.66 s 连丢 3 帧（约 174 固件 ms，短于 300 ms 门限），
  可机器核对的是两件事——`gap` 比"错帧 + 静默时隙"多出恰好 3（C7），`NORMAL→DEGRADED` 仍是 6 次
  没有多降一次（C13）。这两条**不**证明"丢帧没有推迟任何一次恢复"：注入按墙钟排期、固件按虚拟
  时钟走，落点无法精确对准某个固件时刻，所以"恢复晚了 3 帧"与"完全没有影响"在日志里分不开。
- **任务阻塞链可观测**：固件侧注入让 rx 空转 800 ms（`[debug]`，落在静默段，队列为空），
  health 在 `t=16949` 读到 `hb=415 > 400` 门限，`[gate]` 首次出现 `reason=HB+FLAGS`；
  阻塞结束后 `hb` 回落到 20、原因位退回 `FLAGS`。第一版注入写成忙等（`taskYIELD()`）时
  health 在整个 800 ms 内一次都没运行——`rx 3 > health 2`，监督者被饿死，事后只能读到
  `hb=804` 的"陈旧证据"。这一反例见项目 README §6.6：**监督任务的优先级决定它能否看见忙等**。
  注意本轮 `DEGRADED -> SAFE`（16641）落在阻塞窗口内，那是 13636 起算的停留计时到点，与阻塞无因果。
- **停喂门控**：`feed=OFF` 只出现在 `T`/`STALE` 或心跳超时期间，恢复后回到 `ON`；
  `[gate]` 把原因拆成 `OK`/`FLAGS`/`HB`/`HB+FLAGS` 四态，`HB` 与 `FLAGS` 不再同形。
  本条只证明"该停喂时停了"，`IWDG` 计时与复位仍未建模。
- **下行通道可独立解析**：USART2 抓到 13600 字节 / 544 帧，CRC、长度、类型错误各 0，`SEQ` 0→31
  逐帧 +1，`uptime` 101→54767 ms 严格递增，帧间隔中位 100 ms（最大 117）；与上行日志 10 组同一
  `t` 的记录在 mode/flags/frames/crc/gap 上全部一致。能证明的是"组帧、CRC、序号、布局与两条通道
  一致"，**不能**证明发送抖动或比特级时序（要真板 + 逻辑分析仪）。
- **栈水位**：`rx 100/224`（注入构建；注入前 164）、`ctrl 104/224`、`health 88/256`，按
  "绝对 ≥32 字 / 相对 ≥25%"通过；`rx` 的差额来自 `[debug]` 的两条 `printf`。
  `health` 是本轮唯一**触发扩容**的任务：往 `[stat]` 加 `gnd`/`gage` 两个字段后，它自己那条
  `printf` 是最深的调用，余量从 64 掉到 `56/224 = 25.0%`——正压在判据边界上。
  按"压线通过不算通过"把栈提到 256 字，稳态空闲堆因此从 248 B 降到 120 B。
  这条不是文档问题而是容量问题，也印证了 README §10.5 早先写下的预测。
- **四个由实测发现的缺陷**（细节见项目 README §6.2）：DEGRADED 停留门限与故障保持窗口过近，
  使 400 ms 越界推进到 SAFE；变化率参考"上次有效样本"使保护在连续拒收后自失效；
  跨任务时间戳差用无符号减法导致下溢（`age=4294967292`），`hb` 走同一路径，下溢会让门控误停喂；
  加入地面站心跳后 health 栈压线（见上条）。此外还有一处**判定器**的语义问题：C16 的窗口右端
  必须用 `>=`，否则 `silent` 段里 `GND` 与 `T`/`STALE` 同 tick 到期时会把那次降级错算到 `GND` 头上，
  表现为偶发假失败。
- **机器判定 24/24（上行）+ 7/7（下行）**：`bash run.sh aviation-link` 跑完由 `aviation/expect.py`
  判 24 项不变量（C1–C17：CRC 自检、六类故障位都触发、降级/恢复次数、SAFE 停留门限、`gap` 自洽、
  堆恒定、栈判据、门控"与"语义、输出回中位、丢帧未多降级、阻塞→心跳超时→停喂、时间戳年龄无下溢、
  地面站心跳不改变模式、链路层与数据层分离），由 `aviation/link.py` 判下行 7 项（L1–L7），
  实测全通过、两条退出码均为 0。负向验证在同一份日志的临时副本上做，12 组变异全部按预期失败：
  改坏 `gap` 只 FAIL C7、删窗口内 `HB` 门控行只 FAIL C14、删全部 `[debug]` 行转"不适用"、
  `age` 改成下溢值只 FAIL C15、两类 `feed` 篡改各触发 C10、抹掉 `GROUND` 位触发 C4+C16、
  窗口内插假降级触发 C13+C16、`GROUND` 期间 `stat` 报 SAFE 只失败 C16、`hb_only` 段帧数递增只失败 C17、
  **假降级插在 `GROUND` 同 tick 时 C16 主动弃权只失败 C13**、日志缺失返回 2。
  它判**不变量**而不比对硬编码时刻——跨运行漂移最大实测 154 tick，任何按 `t=` 写的断言都会假失败；
  同理它**不判** CAN/ADC/示波器/`IWDG`——那些在 QEMU 里没有模型。
- **延迟自动汇总**：同一脚本另打印六类延迟——故障置位→降级 `0 tick`（6 次）、故障清除→恢复
  NORMAL `82–163 tick`（不含 SAFE 之后，本轮 82/123/101）、故障清除→`SAFE→DEGRADED`
  `205–246 tick`（两级恢复的第一级，本轮 224/222）、DEGRADED→SAFE `3003–3017 ms`、
  SAFE 停留 `2508–6924 ms`（受剧本恢复时刻控制，不是固件判据）、阻塞开始→心跳判超时
  `415–496 tick`（本轮 415，其中 400 是门限本身）。`0 tick` 量的是
  "标志置位→改模式"，**不是**"故障发生→检出"：后者跨端（PC 注入 vs 固件 tick）没有共同时基，
  只能上真板 + 逻辑分析仪。

不可外推的部分：

- 全部 `t=` 是固件毫秒，受 2.90 倍时基影响；检测延迟只能看比例。
- 下行二进制帧能证明的是**组帧/CRC/序号/布局与两通道一致**；发送抖动、比特级时序与波特率精度
  没有证据（QEMU 串口不做比特级移位），仍需真板 + 逻辑分析仪。
- 下行流的**尾部完整性不可判**：`link.py` 的 `parse()` 在残帧处直接 `break`，截掉 10 字节仍 7/7 通过
  （QEMU 被 `timeout` 杀掉时本来就会截在帧中间，两者无法区分）。
- `CAN`、`ADC`、示波器/逻辑分析仪时序、`IWDG` 复位与 `RCC->CSR` 复位原因**无任何证据**，
  见 `projects/aviation-node/README.md` §9。
- 栈水位是所有路径跑过之后的最低余量，但剧本只覆盖 13 个阶段，未覆盖长稳与极端并发。

### 3.11 F103_15 栈溢出检出（`logs/exp15_stackoverflow.log`）

构建体积：FLASH 10852 B / 128 KB（8.28%），RAM 6368 B / 8 KB（77.73%），零告警；
`text 10752 / data 100 / bss 6268`。比 §3.9 少 4092 B Flash——该目标不链接 `queue.c`、
`timers.c`，也没有信号量、命令队列与看门狗驱动，只留 `task.c` + 移植层 + `heap_4`。

用例：无串口输入，5 s 真实时间。**这个用例的正常终点是钩子里的死循环**，QEMU 必然被
`timeout` 杀掉，所以日志末尾的 `terminating on signal 15` 是预期，不是失败；成败由
`run.sh` 里的两条 grep 判定（必须出现钩子报告，且不得出现 `[so] FAIL`）。
原始输出的关键片段：

```text
[F103_15] configCHECK_FOR_STACK_OVERFLOW=2（2 = paint + SP 双重检查），任务栈 256 字

[so] 任务栈 256 字（1024 B）
[so] 对照 depth=12 → 最低地址 0x200004A0，余量 160 字（判据 ≥32）
[so] 标定 depth=16 → 最低地址 0x20000460，每帧 16 B
[so] depth=16 余量 160 字
[so] depth=20 余量 160 字
[so] depth=24 余量 144 字
[so] depth=28 余量 128 字
[so] depth=32 余量 112 字
[so] depth=36 余量 96 字
[so] depth=40 余量 80 字
[so] depth=44 余量 64 字
[so] depth=48 余量 48 字
[so] depth=52 余量 32 字
[so] depth=56 余量 16 字
[so] depth=60 余量 0 字
[so] 逐级加深：首次余量 <32 字在 depth=56，余量归零在 depth=60（按余量预测 52）
[so] 注入完成：depth=60 时 hwm=0，即栈底 16 B 的 0xA5 已被写穿；越界量不超过一帧（16 B）
[so] 返回后栈指针合法；vTaskDelay(1) 等待切换时的 paint 检查

[STACK OVERFLOW] 任务 probe 的栈被写穿，钩子已触发
```

读法：

- **钩子确实被调用，且任务名正确**：`pcTaskName` 为 `probe`，与执行注入的任务一致；
  触发后不再有任何输出（`for(;;)` 停机）。
- **触发的是 paint 那一半**：任务返回后栈指针合法（`pxTopOfStack` 从未越过栈块），
  被破坏的是栈底 16 B 的 `0xA5A5A5A5`。另一半（SP 越界）刻意不构造。
- **余量判据与检出宏看同一批字节**：`hwm == 0` ⇔ `pxStack` 处首字节已非 `0xA5`，
  所以它和 `configCHECK_FOR_STACK_OVERFLOW` 的后半判据是同一事实的两种读法。
- **余量衰减是线性的**（稳态段每 4 帧减 16 字 = 16 B/帧，与标定一致），12→20 的 8 帧
  不消耗余量（写入落在已使用区域），这解释了"预测 52 / 实测 60"的 8 帧偏差。
  两次运行逐字一致——本实验无时间依赖，不受 2.90 倍时基影响。
- **判定器做过负向验证**：删掉钩子行 → 判失败；注入一条 `[so] FAIL` → 判失败；
  空日志 → 判失败。

不可外推的部分：

- 只证明"栈底 16 B 被写穿后能被检出"，**不**证明"任何栈溢出都能被发现"：越过 16 B
  继续写、或写后又被覆盖，都可能漏检。
- QEMU 不因越界写抛总线错误，真板上"先撞 HardFault 还是先到钩子"的顺序在仿真里得不到答案。
- 越界量的上界由步长决定（本次 16 B = 一帧），改步长必须重新记录；它确实落到了栈块外的
  堆控制区，安全性依赖钩子停机，不能当作常规注入手段。
- 钩子里的 `printf` 在生产代码上不可靠（栈已被破坏、串口可能忙），应改为写固定 RAM 区。

## 4. 反向用例（预期失败，`run_negative.sh`）

这一节的用途与正向用例相反：这些用例**预期失败**，不是为了通过，而是把"不能仿真的实验在
QEMU 里到底是什么表现"记录下来，为 §2 的能力边界提供证据。

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
| `f103_15_freertos_stackoverflow.elf` | 10752 | 100 | 6268 | 正向（栈溢出检出，不链接 `queue.c`/`timers.c`） |
| `aviation_node.elf` | 17928 | 100 | 6436 | 综合项目（`projects/aviation-node`，含下行 `STATUS`） |

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
2. 建立在这条主干之上的 FreeRTOS 同样可验证：任务切换、抢占、队列交接、互斥量、堆与栈指标都表现为稳定、可复现的串口数据（§3.5），因为内核只用 SysTick/PendSV/SVCall，不碰未建模外设。
3. 中断→任务的交接同样可验证（§3.6）：USART1 RXNE 是真实模型，`xQueueSendFromISR`、`portYIELD_FROM_ISR`、队列深度与行解析结果都可用字节数与 `trunc/len` 的算术自洽性核对。
4. 同步原语也在可验证范围内（§3.7）：二值/计数信号量的阻塞与超时、事件组的全会合与超时、`xSemaphoreGiveFromISR` 的事件合并、`uxTaskGetStackHighWaterMark` 的水位数都是稳定读数；`xEventGroupSetBitsFromISR` 依赖 `configUSE_TIMERS=1`，实验 12 的目标未打开，未覆盖。
5. 软件定时器同样可验证（§3.8）：回调确实跑在内核守护任务里（句柄比对）、单次与 `xTimerReset` 复用、周期定时器的自动重装、`xTimerStop` 生效，以及"回调阻塞会压缩错过的节拍但不偏移自身相位"这条反直觉行为，都表现为可复现的数字。
6. GPIO、EXTI、RCC、TIM、I2C、DWT 均未建模，SPI 只实现到寄存器层面。依赖它们的实验在 QEMU 里得不到有效证据。
7. QEMU 的 SYSCLK 是 24 MHz，与固件的 8 MHz 假设相差约 2.9 倍，时间相关结论一律不得来自仿真；QEMU 也没有中断延迟模型，`lat_max` 尤其不可外推。
8. QEMU 用背压代替 UART 溢出，环形缓冲与 RTOS 队列的丢弃路径都无法在仿真中触发。
9. `IWDG` 未建模，看门狗复位路径无法在仿真中验证；仿真侧只能做"心跳标志 + 任务阻塞检测"。
10. 心跳监督这一半可验证（§3.9）：心跳打点、age 超时判定、任一通道不健康即停喂、连续 STALE 后的恢复动作、以及"停喂窗口 vs IWDG 名义超时"的比较，全部是可复现数字；但这不是看门狗验证，只是看门狗**前置条件**的验证。
11. 建立在前两者之上的完整软件数据链可验证（§3.10）：外部注入二进制帧（`SENSOR` 数据 + `STATUS` 地面站心跳）→ RXNE 中断 → 逐字节帧解析（SOF/长度/CRC16）→ 三层有效性判定 + 地面站层 → 模式状态机（INIT/NORMAL/DEGRADED/SAFE）→ 降级与两级恢复 → 停喂门控与统计 → USART2 下行二进制 `STATUS`（100 ms）。七类异常各有独立证据（错帧/越界/突变/静默/只发心跳/只发数据/任务阻塞），计数可交叉核对（`gap=313=30+280+3`），状态机停留时长与门限吻合（3005/3004/3009 ms vs 3000 ms），三层活性可分别置位（`T=0 STALE=1 GND=0` 与 `T=0 STALE=0 GND=1` 两个方向都实测），下行 544 帧可被独立解码并与上行交叉一致。
12. 栈溢出检出机制可验证（§3.11）：`configCHECK_FOR_STACK_OVERFLOW = 2` 的 paint 判据确实生效——注入到栈底 16 B 被写穿后，钩子在下一次任务切换时被调用并报出正确的任务名。因为它只用到内核自身的检查宏与任务切换，不碰未建模外设，所以这是完整的仿真证据，不是"仅构建"。

不能下的结论：

- 不能说 `F103_05` 的环形缓冲"溢出处理正确"——只验证了正常路径与中断通路。
- 不能说 `F103_04` 的"115200 8N1"配置正确——QEMU 不做比特级移位，`BRR` 配错也照样收发。
- 不能用 §3.3 的比例反推真机时基——真机是 8 MHz HSI，QEMU 是 24 MHz，两者没有可比性。
- 不能说 `F103_10` 的采样周期"精确 200 ms"——仿真里它由 24 MHz 时基驱动，2.93 倍偏差依然存在；真机数值未测。
- 不能把 `F103_11` 的 `lat_max=2` 读成"中断延迟 2 ms"——它既受 2.93 倍时基影响，又因为 QEMU 没有中断延迟模型而系统性偏小。
- 不能把 `F103_12` 的 `gives=2` 当作真机的合并次数——串口投递时序、中断延迟、任务优先级都会改变它，可迁移的只有"二值信号量会合并事件"这条语义。
- 不能把 `F103_13` 的 `drift=1` / `drift=61` 读成毫秒级实时性——单位是固件 tick（2.90 倍时基），且 `drift=1` 的成因未分解。
- 不能说 `F103_13` 验证了看门狗——`IWDG` 未建模，复位原因与喂狗失败路径只能上板。
- 不能把 `F103_14` 的 819 / 1424 tick 读成"真板会/不会复位"——前者是名义 998 ms 折算的推论，后者受 LSI 30~60 kHz 容差影响，只有 R2 的 1424 在两端都超门限。
- 不能说 §3.10 验证了 CAN、ADC、`IWDG` 复位或复位原因——这四项没有任何仿真或硬件证据，`aviation_node` 的 IWDG 路径同样只做到"代码进入镜像 + 停喂条件可判定"。
- 不能把 §3.10 的 `[stat] hwm` 读成"任意外部输入下都够用"——剧本只覆盖 13 个阶段，未做长稳与极端并发注入。
- 不能说 §3.11 验证了"任何栈溢出都能被发现"——只触发了 paint 那一半（栈底 16 B），SP 越过栈块的情形没构造；QEMU 也不因越界写抛总线错误，真板上"先 HardFault 还是先到钩子"在仿真里没有答案。
- 不能把 §3.11 的 `hwm == 0` 读成"任务栈不够用"——它是**刻意**注入出来的，正是检出成立的前提。

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
bash run.sh            # 正向：probe + exp04 + exp05 + exp10..exp15 + aviation-link + tickrate
bash run_negative.sh   # 反向：01/02/03/06/08/09
```

`aviation` / `aviation-link` 用例的 ELF 来自 `Linux/projects/aviation-node`（不是 `experiments/`），
构建时同样加 `-DF103_QEMU=ON`，但工具链文件用
`../../experiments/F103_common/cmake/arm-none-eabi.cmake`。两者的区别只有一处：`aviation-link`
额外把第二个串口后端写到 `logs/aviation_downlink.bin` 并调用 `aviation/link.py` 判定下行通道。
