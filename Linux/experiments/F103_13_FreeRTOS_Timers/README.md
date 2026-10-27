# F103_13：FreeRTOS 软件定时器（单次 / 周期 / 回调上下文 / 回调不得阻塞）

实验 10–12 里"周期性动作"都靠 `vTaskDelay` 写在任务循环里。本实验换成**软件定时器**：
不用为每个周期动作建任务，但代价被集中到一个地方——所有回调都在**同一个守护任务**里串行执行。
这个代价有多大，是可以量出来的：本实验用"一个 200 ms 定时器故意忙等 60 tick"来量化。

第一次实测时这个演示是**失效的**：注入 40 tick 忙等后，200 ms 定时器自己的漂移没有变化。
原因不是注入没生效，而是自动重装在回调**之前**完成。这条记录留在 README 里，
因为它说明"看起来合理的实验设计"也可能测不到想测的东西。

## 学习目标

- 能说清软件定时器回调跑在哪个任务上下文里，并能用句柄比对**验证**而不是背文档。
- 能区分 `xTimerCreate` / `xTimerStart` / `xTimerReset` / `xTimerStop` 四个动作的语义，
  并解释"创建之后不会自己到期"这一点。
- 能用实测数字回答"回调里做重活会付出什么代价"，并说清**不会**发生什么（不偏移自身相位）。
- 能把"回调里禁止做的事"落到具体后果上：阻塞、`printf`、动态分配、拿互斥量。
- 能用栈判据发现"守护任务栈不够"这类问题，并说明为什么它比普通任务更危险。

## 环境与前置条件

| 项目 | 值 |
| --- | --- |
| 工具链 / 构建 | `arm-none-eabi-gcc 14.2.1`、CMake 4.0.3、Ninja 1.13.2，零告警 |
| 内核 | FreeRTOS V11.3.0（`../F103_common/vendor/freertos/`） |
| 新增内核文件 | `Source/timers.c`（与上游 V11.3.0 md5 一致，未改动） |
| 新增配置 | `F103_FREERTOS_USE_TIMERS=1` → `configUSE_TIMERS=1`；`configTIMER_TASK_PRIORITY=4`、`configTIMER_QUEUE_LENGTH=5`、`configTIMER_TASK_STACK_DEPTH=160` |
| 仿真 | QEMU 10.2.1 `-M stm32vldiscovery`（只用 SysTick + NVIC + USART1，均已建模） |
| 硬件假设 | 无外设依赖；真板需要的就是 SysTick 与串口（用于观察） |

前置：实验 10（任务/队列/堆）、实验 12（信号量、栈水位判据）。

**与前序实验的隔离**：`configUSE_TIMERS` 变成了按目标开关。实验 10–12 不定义
`F103_FREERTOS_USE_TIMERS`，取值仍为 0，体积与行为不变（构建输出可对照 RESULT.md §3.5–§3.7）。

## 核心概念

### 1. 回调跑在守护任务里，不是中断里

`configUSE_TIMERS = 1` 时，`vTaskStartScheduler()` 会额外创建一个**定时器守护任务**
（`prvTimerTask`，优先级 `configTIMER_TASK_PRIORITY = 4`）和一个**命令队列**。
`xTimerStart` 之类调用只是往队列里投一条命令，真正的到期处理与回调执行都在守护任务里。

所以回调里发生的事，等价于"占住一个优先级为 4 的普通任务"：
它不会破坏中断上下文，但它会**推迟所有其它定时器**，也会推迟它自己队列里的后续命令。

本实验在回调里做一次句柄比对来证明这件事，而不是照抄文档：

```c
static volatile uint32_t s_in_daemon;
...
if( xTaskGetCurrentTaskHandle() == xTimerGetTimerDaemonTaskHandle() ) { s_in_daemon = 1U; }
```

实测 `in_daemon=1`。判据很朴素：若回调真跑在 ISR 里，`xTaskGetCurrentTaskHandle()`
返回的是被中断打断的那个任务句柄，比对不可能恒为真。

### 2. 四个动作的语义

| 动作 | 语义 | 常见误解 |
| --- | --- | --- |
| `xTimerCreate` | 只创建，**处于休眠状态** | 以为"创建即启动"，于是多等一个周期才发现没跑 |
| `xTimerStart` | 启动：从现在起再等一个周期 | 以为它会立刻执行一次 |
| `xTimerReset` | 重新计时（未启动时等价于 start） | 以为只能用于单次定时器——周期定时器也能用它"重新对齐相位" |
| `xTimerStop` | 停止；已到期但还没执行的回调**不会**被取消 | 以为 stop 能撤回已经在队列里的执行 |

"单次"（`uxAutoReload = pdFALSE`）到期后自动回到休眠，想再来一次必须由软件再次触发；
"周期"（`pdTRUE`）到期后自动重装。两者的差别只在**重装是否发生**，回调代码完全一样。

### 3. 自动重装的基准是"到期时刻"，不是"执行时刻"

这是本实验最重要的结论，也是第一次演示失效的原因。`timers.c` 的
`prvReloadTimer()`（本仓库 `Source/timers.c:695`）：

```c
while( prvInsertTimerInActiveList( pxTimer,
                                   ( xExpiredTime + pxTimer->xTimerPeriodInTicks ),
                                   xTimeNow, xExpiredTime ) != pdFALSE )
{
    xExpiredTime += pxTimer->xTimerPeriodInTicks;   /* 追赶 */
    pxTimer->pxCallbackFunction( ( TimerHandle_t ) pxTimer );  /* 连续补发 */
}
```

由此得到两条可观测行为：

1. **回调阻塞不改变本定时器自身的相位**：重装在回调被调用**之前**已经完成，
   所以 200 ms 定时器即使每次回调都忙等 60 tick，下一拍仍然准时（实测 `drift=1` 不变）。
2. **错过的节拍会在同一个 tick 内连续补发**：保护别人占住守护任务期间到期的定时器，
   其下一拍已经过去，`while` 循环会把它推进到未来并**额外调用一次回调**。
   实测 fast 定时器的 `#20` 与 `#21` 同在 tick 1071 执行，随后 `#22` 回到理想网格。

所以"回调里做重活"的代价是：**在占住期间别的定时器不能执行，且错过的节拍被压缩丢失**，
而不是"自己的周期永久偏移"。这两种说法的区别，正是能不能正确评估实时性的区别。

### 4. 回调里禁止做的事

| 禁止 | 后果 |
| --- | --- |
| `vTaskDelay` / 任何阻塞调用 | 守护任务停摆，所有定时器一起迟到（本实验的故障注入就是这一条） |
| `printf` 等重 I/O | 同上；115200 下一行 40 字符约 3.5 ms（实验 11 已量化），会挤掉数个 tick |
| `pvPortMalloc` / `vTaskCreate` | 守护任务的栈与堆都在临界资源上，失败会直接进 Hook 死循环 |
| 拿普通互斥量 | 若持有者优先级低于 4，会把守护任务拖住（优先级继承只在本任务链内生效） |
| 假设"回调里能等" | 回调是**顺序执行**的，一个慢回调推迟的是全部定时器 |

正确做法：回调里只做"置标志 / 计数 / 投队列"，重活在普通任务里做。

### 5. 栈判据：守护任务同样要过线

沿用实验 12 的三条判据（绝对 ≥32 字、相对 ≥25%、跑完全部路径再读）。
本实验第一次构建就把它们撞响了：monitor 128 字只剩 26 字、守护任务 128 字只剩 28 字，
都低于绝对余量。处理方式是加栈而不是放宽判据：

- `monitor` → 160 字（实测最终余量 58 字，36%）；
- `configTIMER_TASK_STACK_DEPTH` → 160 字（实测最终余量 132 字，82%）。

守护任务栈不够特别危险：它的溢出发生在内核定时器逻辑里，表现为随机丢失到期或
直接进 `vApplicationStackOverflowHook`，很难与"定时器写错了"区分。

## 接口卡片

### `TimerHandle_t xTimerCreate(const char *name, TickType_t period, UBaseType_t auto_reload, void *id, TimerCallbackFunction_t cb)`

作用：创建软件定时器，返回句柄；**创建后不启动**。

- `period` 用 `pdMS_TO_TICKS()` 换算；`0` 不合法（语义是"一次都不到期"）。
- `auto_reload = pdTRUE` 周期，`pdFALSE` 单次。
- 返回 `NULL` 表示堆不足，必须检查。
- 需要 `configUSE_TIMERS = 1`；否则链接期报 `undefined reference to xTimerCreate`。

### `BaseType_t xTimerStart(TimerHandle_t t, TickType_t block)`

作用：启动定时器，从现在起等一个 `period` 后第一次到期。

- `block` 是**投递命令**的等待上限（命令队列满时才会用到），不是到期等待。
- 返回 `pdPASS` 表示命令入队成功，不代表定时器已经开始计时。
- 对处于运行状态的定时器调用等于无操作。

### `BaseType_t xTimerReset(TimerHandle_t t, TickType_t block)`

作用：重新计时。未启动时等价于 `xTimerStart`。

- 单次定时器最常用的"延时重启"手段：每次触发都把它推后一个完整周期。
- 周期定时器上用它可以把相位重新对齐到当前时刻。

### `BaseType_t xTimerStop(TimerHandle_t t, TickType_t block)`

作用：停止，之后不再到期。

- 已在队列中等待执行的那次到期**不会被撤回**，回调仍可能执行一次。
- 判据必须是"停止后计数不再增长"，本实验用 600 ms 静置 + 前后计数比对来验证。

### `TaskHandle_t xTimerGetTimerDaemonTaskHandle(void)`

作用：拿到守护任务句柄，可用于读它的栈水位或做上下文自证。

- 只在 `configUSE_TIMERS = 1` 时存在。
- 守护任务由内核创建，任务栈来自 `configTIMER_TASK_STACK_DEPTH`。

### `TaskHandle_t xTaskGetCurrentTaskHandle(void)`

作用：当前任务的句柄。回调里调用返回的就是守护任务句柄——这是"上下文自证"的依据。

- 需要 `INCLUDE_xTaskGetCurrentTaskHandle = 1`（本仓库已开）。

### 内部参考：`static void prvReloadTimer(Timer_t *, TickType_t xExpiredTime, TickType_t xTimeNow)`

作用：周期定时器的重装。**不是公开 API**，但它是理解第 3 条结论的唯一入口。

- 新到期时刻 = `xExpiredTime + period`，若已过去则循环推进并**连续补发回调**。
- 回调在 `prvProcessExpiredTimer()` 里**最后**才被调用，所以重装永远先于回调执行。

## 最小可运行实验

```text
F103_13_FreeRTOS_Timers/
├── CMakeLists.txt     # f103_add_firmware(... DEFINES F103_FREERTOS_USE_TIMERS=1)
│                      # + f103_add_freertos(... MODULES timers)
├── main.c             # 两个任务 + 三个定时器 + 故障注入
└── build-qemu/
```

```text
app      (prio 2, 192 字)
 ├─ 创建 once(300 ms 单次) / periodic(200 ms 周期) / fast(50 ms 周期)
 ├─ start ×3 → 等 once fire#1 → reset → 等 fire#2
 ├─ 等 periodic 4 期（normal 基线）
 ├─ 注入：周期回调忙等 60 tick → 等 4 期 → 解除
 ├─ 等 2 期（after 段）→ 停 fast → 停 periodic → 静置 600 ms 验证
 └─ 按计数不再增长判"停住了"
monitor  (prio 1, 160 字)  每 500 ms 打印堆余量与 app/monitor/daemon 的栈水位
守护任务 (prio 4, 160 字)  内核创建：全部定时器命令与回调都在这里执行
```

`STALL_TICKS=60 > FAST_PERIOD_MS=50` 是刻意的：必须让 fast 的某个到期点落在阻塞窗口内，
否则整段注入不会产生任何可读的延迟。第一次用 40 tick 时就是这样失效的。

## 编译与运行

```bash
cd Linux/experiments/F103_13_FreeRTOS_Timers
cmake -S . -B build-qemu -G Ninja -DF103_QEMU=ON \
  -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake \
  -DCMAKE_MAKE_PROGRAM="<ninja 路径>"
cmake --build build-qemu

cd ../../tests/qemu-stm32
bash run.sh exp13        # WSL / Linux 下执行
```

真机：去掉 `-DF103_QEMU=ON`、构建目录换 `build`，再 `cmake --build build --target flash`。
本实验不依赖任何未建模外设，真机上唯一需要确认的是串口可观测。

## 预期现象

```text
[F103_13] configUSE_TIMERS=1 daemon prio=4 queue=5 stack=160 words
[F103_13] 互斥量建好后堆余量 4000 B
[F103_13] 两个任务建好后堆余量 2384 B，启动调度器（守护任务、定时器命令队列与 idle 由内核创建）
[tmr] 三个 Timer_t 用掉 144 B（堆余量 880 → 736）
[tmr] once fire#1 act=310 ideal=310 drift=0 in_daemon=1
[tmr] once fire#2 act=615 ideal=615 drift=0（reset 生效，单次定时器可复用）
[tmr] normal period#1 act=211 ideal=210 drift=1 dur=0
[tmr] 注入：周期回调忙等 60 tick（模拟回调里做重活）
[tmr] stalled period#5 act=1011 ideal=1010 drift=1 dur=60
[tmr] after fast#20 act=1071 ideal=1010 drift=61
[tmr] after fast#21 act=1071 ideal=1060 drift=11
[tmr] after fast#22 act=1111 ideal=1110 drift=1
[tmr] 已停止：periodic 前=10 后=10 差=0；fast 前=40 后=40 差=0（都应为 0）
```

判定要点：

- `in_daemon=1`：回调确实在守护任务里。若为 0，说明句柄比对写错或配置未生效。
- `drift` 在 `normal` 段应为 0 或 1：差 1 来自"到期发生在 tick 边界之后一瞬"。
- `dur=60` 说明故障注入生效；若为 0，注入没跑。
- `stalled` 段 200 ms 定时器的 `drift` **仍然是 1**——这不是 bug，见核心概念 3。
- `fast#20/#21` 同一 tick、且 `#22` 回到 `drift=1`：错过的节拍被压缩补发，不是永久偏移。
- `已停止` 两行差值都为 0：`xTimerStop` 之后确实不再到期。

## 实测记录

环境：QEMU 10.2.1 `-M stm32vldiscovery`，gcc 14.2.1 `-Og -g3`。
原始日志：[`exp13_timers.log`](../../tests/qemu-stm32/logs/exp13_timers.log)（68 行，含 20 行 `[res]` 周期采样）。

### 构建体积

| 区域 | 已用 | 占比（QEMU 128 KB / 8 KB） |
| --- | --- | --- |
| FLASH | 18392 B | 14.03% |
| RAM | 6864 B | 83.79% |

`text 18292 / data 100 / bss 6764`，构建告警 **0** 条。
比实验 12 多 2404 B Flash：`timers.c` 的实现 + 第三个定时器与两段报告代码。
RAM 比实验 12 多 456 B，主要是 `s_fast_act[48]` 与 `s_per_dur[24]` 两处记录数组。

### 内核堆（4 KB）的五段账

| 阶段 | 占用 | 来源 |
| --- | --- | --- |
| 互斥量（`s_log_mutex`） | 96 B | `4096 − 4000`，建好后直接读余量 |
| 两个任务（192 + 160 字栈 + TCB） | 1616 B | `4000 − 2384` |
| 守护任务 + 命令队列(5) + idle | 1504 B | `2384 − 880`，全部由 `vTaskStartScheduler` 创建 |
| 三个 `Timer_t` | 144 B | `880 → 736`，每个 48 B |
| 稳态余量（20 次采样恒定） | 736 B | `[res]` 行 |

五行相加 = 4096 B。**守护任务不是免费的**：它加命令队列、idle 一共吃掉 1504 B，
接近实验 12 里三个任务的总和（2232 B 中含 idle 616 B）。这是一条实用结论：
"用定时器省任务"只在定时器数量远小于任务数量时成立。

### 行为实测

| 观测 | 实测值 | 说明 |
| --- | --- | --- |
| 回调上下文 | `in_daemon=1` | 句柄比对通过 |
| 单次定时器 fire#1 | `act=310 ideal=310 drift=0` | `create` 后先 `start`，300 tick 后到期 |
| `xTimerReset` 后 fire#2 | `act=615 ideal=615 drift=0` | 单次到期后休眠，reset 让它重新跑一轮 |
| 200 ms 定时器 normal 段 | `drift=1`，`dur=0` | 回调只记账，周期误差 1 tick |
| 200 ms 定时器 stalled 段 | `drift=1`，`dur=60` | **回调慢不影响自身相位** |
| 50 ms 定时器 normal 段 | `fast_total=16`，`max_drift=1` | 16 次到期全部在网格上 |
| 50 ms 定时器 stalled 段 | `#20/#21` 同在 `act=1071`，`#22` 回到 `drift=1` | 阻塞窗口内的两拍被压缩到同一 tick |
| 停止验证 | `periodic 10→10`、`fast 40→40` | 差值为 0 才算"停住了" |
| 堆余量 | `736 B` 恒定（20 次采样） | 演示过程中无动态分配 |

### 栈水位（含修正过程）

| 任务 | 分配（字） | 最小剩余（字） | 已用 | 相对余量 | 判据 |
| --- | --- | --- | --- | --- | --- |
| app | 192 | **74** | 118 | 39% | 通过 |
| monitor | 160 | **58** | 102 | 36% | 通过（128 字时只剩 26 字，**不通过**） |
| 守护任务 | 160 | **132** | 28 | 82% | 通过（128 字时只剩 28 字，**不通过**） |

两次不达标都是实测发现、当场改配置解决的，而不是事后从文档里推测出来的。
monitor 的栈消耗主要来自 `printf` 的整型格式化，与实验 12 的 102 字一致。
守护任务只用 28 字执行回调——但它的栈要留给**最慢的那个回调**，所以不能按当前用量定值。

### 对这几个数字的正确读法

- **tick 不是真实毫秒**：QEMU 固件时基是真实时间的 2.90 倍（见 RESULT.md §3.3）。
  `drift=61` 是 61 个固件 tick，约 21 ms 真实时间；本实验的漂移只能当**机制证据**。
- **`drift=1` 的成因未进一步分解**：可能是 tick 边界对齐，也可能是回调执行耗时。
  要分解需要 DWT 或更高分辨率时基，两者在本仓库的 QEMU 环境里都不可用。
- **`fast#20/#21` 压缩补发是内核行为，不是本实验的假设**：
  与 `timers.c:702-710` 的 `while` 循环一致，可交叉验证。

## 关键实现点

### 1. 用"第二个高频定时器"把阻塞代价暴露出来

只在被注入的那个定时器上量漂移，永远读不到延迟——因为它的重装先于回调。
换成"旁观者"定时器（50 ms）后，延迟立刻变成可读数字，且能区分
"延迟执行"与"永久偏移"两种完全不同的后果。**指标的选取决定了实验能不能看到现象。**

### 2. 故障注入必须有开关，且必须写清"这是故障"

`s_stall` 是普通变量而不是编译期常量，注入与解除都在日志里留有痕迹
（`注入：…` / `注入结束：…`）。否则后来的人看到 `dur=60` 会以为是正常行为。

### 3. 回调里只写 `volatile` 计数，不调用任何 API

三个回调都不 `printf`、不 `vTaskDelay`、不分配内存。日志全部由任务打印，
用互斥量串起整行，避免 `printf` 被抢占拆行。

### 4. 停止判据是"计数不再增长"，不是"API 返回 pdPASS"

`xTimerStop` 返回成功只说明命令入队成功。所以停之后静置 600 ms 再比对前后计数，
差值 0 才是证据。

### 5. `F103_FREERTOS_USE_TIMERS` 是按目标开关，不污染其它实验

`FreeRTOSConfig.h` 里 `configUSE_TIMERS` 直接取自该宏（默认 0），
实验 13 在 CMake 的 `DEFINES` 里给 1。这样实验 10–12 的 Flash/RAM 与行为完全不变，
不需要维护两份配置文件。

### 6. 先停高频定时器再做"停止验证"

`fast` 若不停，600 ms 静置期间它还会再到期十几次，`fast_total` 会漂移，
读起来像是"停止失败"。测量顺序本身也是设计的一部分。

## 常见错误和陷阱

| 现象 | 原因 | 处理 |
| --- | --- | --- |
| `undefined reference to xTimerCreate` | 没开 `configUSE_TIMERS` 或没链接 `timers.c` | 定义 `F103_FREERTOS_USE_TIMERS=1` + `MODULES timers` |
| 程序启动后立刻 `configASSERT` | 开了 `configUSE_TIMERS` 但 `configTIMER_TASK_PRIORITY` 未定义或越界 | 必须 `0 ≤ prio < configMAX_PRIORITIES` |
| 定时器"创建了但不动" | `xTimerCreate` 只创建不启动 | 补 `xTimerStart`（或 `xTimerReset`） |
| 单次定时器只跑一次 | 这就是单次语义 | 需要重复触发就 `xTimerReset`，或改 `pdTRUE` |
| 回调里 `printf` 后定时器全部迟到 | 守护任务被 I/O 占住 | 回调只置标志，打印放到任务里 |
| 回调里 `vTaskDelay` 导致系统卡死感 | 守护任务停摆，且 `prvTimerTask` 里延时等同自锁 | 回调内禁止任何阻塞调用 |
| 停止后回调还执行了一次 | 到期时命令已入队，`stop` 撤不回 | 判据用"计数不再增长"，回调里再查一次状态位 |
| `drift` 一直是 1，看不到注入效果 | 只在被注入的定时器上量漂移 | 用第二个高频定时器做旁观者 |
| 守护任务栈溢出 | `configTIMER_TASK_STACK_DEPTH` 按普通任务经验给 | 至少给最慢回调留出余量，并按水位判据复核 |

## 故障排查方法

1. **先看初始化两行**：`configUSE_TIMERS=1` 与 `daemon prio/queue/stack` 是否符合预期，
   不符说明宏没传进去（看 CMake 的 `DEFINES`）。
2. **`in_daemon=0`**：`xTimerGetTimerDaemonTaskHandle()` 返回 `NULL`（配置未生效）
   或回调实际不在守护任务里（自定义了 `prvTimerTask`）。
3. **`drift` 远超 1 且出现在 normal 段**：有更高优先级任务长期占用 CPU，
   或 `printf` 拖住了守护任务——先看是不是回调里打印了。
4. **`dur` 恒为 0 但注入日志已打印**：`s_stall` 没被写入（被编译器缓存、或写在回调之后），
   检查 `volatile` 与写入顺序。
5. **`fast` 的漂移一直增长不回落**：与 `timers.c` 的 `prvReloadTimer` 行为不符，
   优先怀疑 `fast_total` 超过 `FAST_LOG_MAX`（记录数组截断，后续索引错位）。
6. **停止后计数仍在增长**：`xTimerStop` 之后又被 `xTimerReset` 唤起，
   或有两处代码在启动同一个定时器。
7. **守护任务水位持续下降**：某个回调路径第一次被走到（水位是历史最小值，只减不增）。

## 练习题

基础题：

1. 把 `ONCE_PERIOD_MS` 改成 0，观察 `xTimerCreate` 的断言或行为，说明为什么 `period = 0` 不合法。
2. 去掉 `xTimerReset` 那一行，记录 `once fire#2` 是否还会出现，并解释单次/周期的差别。
3. 把 `FAST_PERIOD_MS` 改成 200（与 periodic 同周期），重跑并解释为什么两个定时器的到期会
   在同一个 tick 内**依次**执行。

改错题：

4. 下面回调有五处问题，逐条指出并改成"回调里只置标志"的写法：
   ```c
   void cb_bad(TimerHandle_t t)
   {
       printf("tick\n");                    /* 1 */
       xSemaphoreTake(s_bus_lock, portMAX_DELAY);  /* 2 */
       uint8_t *p = pvPortMalloc(64);       /* 3 */
       vTaskDelay(pdMS_TO_TICKS(10));       /* 4 */
       xTimerStop(t, 0);                    /* 5 */
   }
   ```

综合题：

5. 把故障注入换成"在回调里轮询等一个由 app 任务 set 的事件组"，观察会发生什么，
   并解释为什么这比忙等更糟（提示：守护任务优先级 4，app 是 2）。
6. 给 `periodic` 增加 `xTimerChangePeriod()` 动态改周期（例如跑 4 期后从 200 改成 400），
   记录改周期后的第一次到期时刻，并说明它是否重新对齐到"当前时刻 + 新周期"。
7. 用 `xTimerGetExpiryTime()` 在回调内读出"下一次到期时刻"，与本实验用
   `base + n × period` 的推算方式对比，说明两种方法各自的适用条件与误差来源。

## 验收标准

- [ ] 能说清定时器回调跑在哪个上下文，并能用一条日志证明它。
- [ ] 能区分 `create` / `start` / `reset` / `stop` 四个动作，并说出各自的判据。
- [ ] 能解释"回调忙等 60 tick、自己却仍然 drift=1"，并从 `prvReloadTimer` 找到依据。
- [ ] 能解释 `fast#20/#21` 同一 tick 的原因，并说清"丢节拍"与"永久偏移"的区别。
- [ ] 能列出回调内禁止的四类操作及其后果。
- [ ] 能用栈判据判断守护任务是否够用，并说明为什么它比普通任务更危险。
- [ ] 能把 4 KB 堆拆成五段并对上总数（96 + 1616 + 1504 + 144 + 736 = 4096）。
- [ ] 能列出本实验未覆盖的路径（见「已知限制」）。

## 延伸阅读

- 内核源码：`../F103_common/vendor/freertos/Source/timers.c`
  - `prvReloadTimer()`（695 行起）：重装与追赶逻辑，本实验第 3 条结论的唯一依据。
  - `prvProcessExpiredTimer()`（714 行起）：重装先于回调调用。
  - `prvTimerTask()`（744 行起）：命令队列与到期列表的合流点。
- FreeRTOS 官方文档：Software Timers 章节，重点看 "Timer callback functions must not block"。
- 实验 11 的 `printf` 代价量化（一行 40 字符约 3.5 ms），用来估算回调里打印的代价。
- `xTimerPendFunctionCall()`：把任意函数推迟到守护任务执行的机制，
  也是 `xEventGroupSetBitsFromISR` 的实现基础（实验 12 的已知限制）。

## 下一步

1. **心跳与看门狗**：`IWDG` 在 QEMU 未建模，仿真侧只能做"任务心跳标志 + 阻塞检测"，
   复位路径必须真板闭环（`CLAUDE.md` §12 P0 最后一项）。
2. **把 `xEventGroupSetBitsFromISR` 补上**：现在 `configUSE_TIMERS` 已有按目标开关，
   可以在实验 12 的配置里打开它，补掉那条"已知限制"。
3. **动态改周期与相位对齐**：练习 6 的产物可以并入本实验，作为"运行期调参"的用例。
4. **真板验证**：在 8 MHz HSI 上重跑，记录真实毫秒下的 `drift`，
   替换本 README 中来自 QEMU 的数值。

## 已知限制

- **未验证真实时间精度**：QEMU 时基是真实时间的 2.90 倍，`drift=1` 与 `drift=61`
  都按固件 tick 计，不能换算成真机毫秒。
- **未覆盖 `xTimerChangePeriod` / `xTimerPendFunctionCall`**：
  两者都属"运行期改变行为"的接口，留作练习与下一步。
- **`drift=1` 的成因未分解**：tick 边界对齐与回调执行耗时混在一起，
  需要 DWT 或高分辨率时基才能分离，本仓库 QEMU 环境都不具备。
- **未做看门狗与复位路径**：`IWDG` 未建模，仿真里无法观测复位原因寄存器。
- **未在真板运行**：所有数值来自 QEMU，栈水位也与真机编译器输出可能不同。
- **RAM 83.79% 是按 QEMU 的 8 KB 链接脚本算的**，真板 C8T6 有 20 KB，不构成约束。
