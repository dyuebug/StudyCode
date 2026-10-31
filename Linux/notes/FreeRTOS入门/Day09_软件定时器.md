# Day09：软件定时器（教材第 16 章）

**验证状态**：回调**确实在守护任务里执行**（`exp13_timers.log`：`in_daemon=1`，用
`xTaskGetCurrentTaskHandle() == xTimerGetTimerDaemonTaskHandle()` 比对得出，不是照抄文档）；
单次定时器的 `Reset` 复用、周期定时器的漂移、**"重装先于回调"**、**追赶补发**全部**实测**；
**源码核对**：`prvProcessExpiredTimer` 的重装顺序、`prvReloadTimer` 的补发循环、`xTimerPendFunctionCall`
的编译前置（`Source/timers.c:42-43` 的 `#error`）；**未验证**：教材 `28_timer_game_sound`
工程本机没有；`xTimerStartFromISR` 等四个 ISR 变体本仓库未使用；命令队列**写满**导致的 `pdFAIL` 未构造出来。

---

## 学习目标

学完本篇应能用代码证据回答：

1. 定时器的"两种类型 × 两个状态"分别是什么？哪一组合是"已停止"的正确表达？
2. 为什么回调**不能**在 Tick 中断里执行？这个措辞是"不方便"还是"不允许"？
3. `xTimerStart()` 返回 `pdPASS` 时，定时器**已经**在跑了吗？如果没跑，超时时间从哪一刻算起？
4. 所有定时器 API 的那个 `xTicksToWait` 参数，等的是什么？**不是**什么？
5. 自动加载定时器在回调**之前**还是**之后**重装？怎么用一条读数证明？
6. 回调耗时 60 tick、周期 200 tick 时，这个定时器自己的下一次到期会推迟吗？
7. 回调耗时 60 tick、另一个定时器的周期是 50 tick 时，那个定时器付出什么代价？**永久偏移还是丢节拍**？
8. `xTimerReset` 与 `xTimerChangePeriod` 各自能做"启动"，它们的差别是什么？
9. 定时器 ID 的读写为什么不经过命令队列？这让它能在什么上下文里用？
10. 教材 16.4 的示例传 `xTicksToWait = 0`，这个 0 的风险是什么？

---

## 环境与前置条件

| 项 | 版本 / 说明 |
| --- | --- |
| 教材 | 第 16 章软件定时器（`rtos.txt:5681-6117`），含 16.1 特性、16.2 上下文、16.3 函数、16.4 示例 |
| 内核 | FreeRTOS **V11.3.0**；实现在 `Source/timers.c` |
| 关键配置 | `configUSE_TIMERS = F103_FREERTOS_USE_TIMERS`（`FreeRTOSConfig.h:50-55`，**默认 0**）；`configTIMER_TASK_PRIORITY = configMAX_PRIORITIES-1`（`:61`）、`configTIMER_QUEUE_LENGTH 5`（`:62`）、`configTIMER_TASK_STACK_DEPTH 160`（`:63`） |
| 实验 | `experiments/F103_13_FreeRTOS_Timers/`（`-DF103_FREERTOS_USE_TIMERS=1` 单独开启） |
| 日志 | `tests/qemu-stm32/logs/exp13_timers.log` |
| 前置 | `Day03`（任务状态与优先级）、`Day06`（阻塞与超时）、`Day08`（§8 已经埋了一个伏笔：事件组的 ISR 版走的就是本章的守护任务） |

---

## 核心概念

### 1. 两类 × 两态：四个组合里只有一个表示"已停用"

教材的闹钟比喻（`rtos.txt:5682-5685`）：可以设"30 分钟后叫你起床"（一次），
也可以设"每隔 1 小时检查机器"（周期）。**无数个软件定时器，都基于同一个 Tick 中断。**

**两种类型**（`rtos.txt:5707-5714`）：

| 类型 | `uxAutoReload` | 行为 |
| --- | --- | --- |
| 一次性（One-shot） | `pdFALSE` | 回调**只调一次**；之后再调只能手工启动，**不会自动重启** |
| 自动加载（Auto-reload） | `pdTRUE` | 到期后**自动**重新装载，回调被周期性调用 |

**两个状态**（`rtos.txt:5717-5721`）：

| 状态 | 含义 |
| --- | --- |
| 运行（Running / Active） | 到期时回调会被调用 |
| 冬眠（Dormant） | 句柄还在，**但不再运行**，回调不会被调用 |

四个组合只有**两态**（类型是创建时定死的，运行期不可改）：

```text
              ┌──────── xTimerStart / xTimerReset ────────┐
              │                                            ▼
        【冬眠 Dormant】                              【运行 Active】
              ▲                                            │
              └──────── xTimerStop ────────────────────────┘
```

两条容易被忽略的事实：

- **`xTimerCreate()` 创建出来的是冬眠态**，不会自己跑。本仓库实验专门利用了这一点：
  `main.c:216` 的注释写"`xTimerCreate` 之后定时器处于休眠状态，不会自己到期"，
  然后先等一个周期再 `xTimerStart`，把"创建"与"启动"两件事分开观测。
- **一次性定时器到期后自动回冬眠**（源码 `timers.c:727-730` 清 `tmrSTATUS_IS_ACTIVE`），
  所以它"复用"必须再启动一次。本仓库的 `fire#2` 就是这条的证据（§7）。

### 2. 为什么不能在 Tick 中断里跑回调：这是"不允许"，不是"不方便"

教材的推导只有三句（`rtos.txt:5730-5737`）：

> 第一印象就是在 Tick 中断里执行：在 Tick 中断中判断定时器是否超时、如果超时了调用它的回调函数。
> FreeRTOS 是 RTOS，**它不允许在内核、在中断中执行不确定的代码**：
> 如果定时器函数很耗时，会影响整个系统。

关键在于"**不允许**"这三个字，以及它的理由不是"慢一点没关系"，而是：

```text
Tick 中断里每多花 1 µs，系统里所有中断的响应延迟上限就多 1 µs，
并且这个代价是全局的、无法被任务优先级调度的。
```

回调函数是**用户写的**——内核无法预知它多长。所以内核连"试一试"的机会都不给，
直接把执行位置搬到**任务上下文**里。这个任务就是守护任务。

### 3. 守护任务：一个不为你专属、却要跑你所有回调的任务

教材的说明（`rtos.txt:5738-5746`）：

> 在某个任务里执行，这个任务就是：RTOS Daemon Task（守护任务）。
> 以前被称为 "Timer server"，但是这个任务要做并不仅仅是定时器相关，所以改名。

**改名这件事本身是信息**："timer server"听起来像一个专用服务，容易让人以为
"我的回调慢了只影响我自己的定时器"。事实上它影响**所有**定时器和**所有**命令。

它的两条配置与它在系统里的位置：

| 项 | 宏 | 本仓库取值（实测日志首行） |
| --- | --- | --- |
| 优先级 | `configTIMER_TASK_PRIORITY` | **4**（`configMAX_PRIORITIES-1`，全系统最高） |
| 命令队列长度 | `configTIMER_QUEUE_LENGTH` | **5** |
| 栈深度 | `configTIMER_TASK_STACK_DEPTH` | **160 字** |

**本仓库取值 4 意味着守护任务比所有应用任务都高**。实验里 `app` 是 2、`monitor` 是 1，
所以定时器命令和回调总是能抢占它们。这个选择有利（及时）也有弊（回调里一旦出问题，
抢占的是整个系统）。教材 16.2.2 用两个例子专门讲这一点（§4）。

**内存账**（实测，日志 `heap 880 → 736`）：

```text
[tmr] 三个 Timer_t 用掉 144 B（堆余量 880 → 736）
```

144 / 3 = **48 B / 个**。此后 `heap_free` 恒为 736 B，说明定时器**不在运行期分配内存**
（每一个都在 `xTimerCreate` 时一次分配完）。这个账要跟 `configTIMER_TASK_STACK_DEPTH = 160 字`
= 640 B 一起算——守护任务本身还要占掉一块常驻栈。

### 4. `xTimerStart()` 返回 `pdPASS` ≠ 定时器已经在跑

这是本章最容易误解的一处，教材用了两个完整例子来讲（`rtos.txt:5749-5852`），值得压成一张表：

| 情形 | 守护任务优先级 vs 调用者 | `xTimerStart` 内部发生什么 | 调用返回时 |
| --- | --- | --- | --- |
| **例 1** | 守护任务**更低** | 命令进队列，守护任务就绪但不能抢占 | 定时器**还没启动**；要等调用者自己阻塞后守护任务才处理 |
| **例 2** | 守护任务**更高** | 命令进队列，守护任务**抢占**调用者，处理完命令，调用者再继续 | 定时器**已经启动**（在调用返回之前就启动了） |

两例的**共同点**才是要点：

> **`xTimerStart()` 只是把 "start timer" 这条命令发给命令队列。**
> 它返回 `pdPASS` 的含义是"命令**进了队列**"，不是"定时器**已经启动**"。

**由此推出的关键结论**（教材 `rtos.txt:5791-5793`、`5842-5844` 各说一遍）：

```text
到期时刻 = 调用 xTimerStart() 的时刻 + 周期
          （不是 "守护任务处理命令的时刻" + 周期）
```

为什么？因为命令里带着的是**启动时刻的时间戳**（源码 `prvProcessReceivedCommands` 里
`pxTimer->xTimerPeriodInTicks + xMessage.xMessageValue`，`:1010`），守护任务据此倒推。
否则守护任务忙的时候，定时器的相位就会被系统负载"拉动"。

教材给了一个具体的数量级：例 1 里定时器在 tX 到期，超时时间是 `tX - t2` 而非 `tX - t4`。

### 5. 那个 `xTicksToWait` 等的是**命令队列**，不是定时器周期

教材反复强调（`rtos.txt:5880-5884`、`5895-5898`）：

> 注意，这些函数的 `xTicksToWait` 表示的是，把命令写入命令队列的超时时间。
> 命令队列可能已经满了，无法马上把命令写入队列里，可以等待一会。
> **`xTicksToWait` 不是定时器本身的超时时间，不是定时器本身的"周期"。**

这张表把两个"超时"彻底分开：

| 概念 | 在哪定 | 表达式 | 语义 |
| --- | --- | --- | --- |
| **周期** | `xTimerCreate` 的 `xTimerPeriodInTicks`（或 `xTimerChangePeriod`） | `300 ms` 等 | 到期后隔多久再到期 |
| **命令队列超时** | 每个 API 的最后一个参数 `xTicksToWait` | `0` / N / `portMAX_DELAY` | 队列满了，等多久把命令塞进去 |

三档取值与 `Day04` 的超时表完全一致：

| 取值 | 语义 | 风险 |
| --- | --- | --- |
| `0` | 不等待，塞不进去就返回 `pdFAIL` | **命令可能直接丢**，且没人察觉（教材 16.4 的示例正是这么写的，§12） |
| N | 最多等 N tick | 调用方被拖住最多 N tick |
| `portMAX_DELAY` | 一直等 | 若守护任务被某个慢回调永占，调用方**永久阻塞** |

`Day08` §7 那个"事件组 ISR 版不是直接置位、而是给守护任务发命令"的机制，
用的就是这条命令队列——所以本章的"队列长度 5"也同时是事件组 ISR 能力的上限。
`Day08` §8 里 `INCLUDE_xTimerPendFunctionCall` 与 `configUSE_TIMERS` 必须同时为 1 的原因，
在 `timers.c:42-43` 是一句编译期硬检查：

```c
#if ( INCLUDE_xTimerPendFunctionCall == 1 ) && ( configUSE_TIMERS == 0 )
    #error configUSE_TIMERS must be set to 1 to make the xTimerPendFunctionCall() function available.
#endif
```

**这是"配错就明确报错"的正面例子**，值得对照 `F103_17` 那种"未建模设备静默读 0"的反面例子记。

### 6. 回调里能做什么、不能做什么

教材列了三条（`rtos.txt:5854-5865`）：

| 约束 | 理由 | 后果（如果违反） |
| --- | --- | --- |
| **不能进入阻塞状态** | 守护任务不是为你一个定时器服务的 | 所有其它定时器**和所有命令**一起被拖住 |
| **不能调 `vTaskDelay()`** | 同上 | 同上，而且是直接违反 |
| 可以调 `xQueueReceive()` 等，但**超时必须为 0** | 允许"试一次"，不允许"等" | 同上 |

这三条其实是一条：**守护任务是共享资源，你在里面花的时间是全局成本。**

本仓库实验把这条做成了可读的代价（`F103_13/main.c:44`）：

```c
#define STALL_TICKS          60U     /* 注入的回调占用：必须 > FAST_PERIOD_MS */
```

`STALL_TICKS > FAST_PERIOD_MS`（60 > 50）是刻意的——否则"守护任务被占住"这段
**不会产生任何可读的延迟**。实验注释里写明：

> 60 > 50 是刻意的：必须让 fast 的某个到期点落在阻塞窗口内，
> 否则这段注入不会产生任何可读的延迟（**第一次实测就是这样失效的**）。

这是一条值得抄下来的实验设计纪律：**故障注入的参数必须越过分界，否则"没看到现象"会被误读成"没有问题"。**

### 7. 重装发生在**回调之前**——这是本章最反直觉的一条

教材没有明说这一点，只说"自动加载定时器到期后会自动启动"（`rtos.txt:5711`）。
源码把顺序钉死了（`timers.c:714-740`）：

```c
static void prvProcessExpiredTimer( const TickType_t xNextExpireTime, const TickType_t xTimeNow )
{
    Timer_t * const pxTimer = ( Timer_t * ) listGET_OWNER_OF_HEAD_ENTRY( pxCurrentTimerList );
    ( void ) uxListRemove( &( pxTimer->xTimerListItem ) );

    if( ( pxTimer->ucStatus & tmrSTATUS_IS_AUTORELOAD ) != 0U )
    {
        prvReloadTimer( pxTimer, xNextExpireTime, xTimeNow );   /* ← 先重装 */
    }
    else
    {
        pxTimer->ucStatus &= ( ( uint8_t ) ~tmrSTATUS_IS_ACTIVE );  /* ← 单次：直接清 active */
    }

    traceTIMER_EXPIRED( pxTimer );
    pxTimer->pxCallbackFunction( ( TimerHandle_t ) pxTimer );    /* ← 后调回调 */
}
```

于是得到一个可以立刻用来判断的结论：

> **自动加载定时器的下一次到期时间，是在回调**开始之前**就算好的，
> 基准是"本次到期时刻 + 周期"，与回调耗时无关。**
> **所以回调慢不会推迟它自己的下一拍**——只要回调耗时小于周期。

本仓库的读数把这条钉死了：

```text
[tmr] 注入：周期回调忙等 60 tick（模拟回调里做重活）
[tmr] stalled period#5 act=1011 ideal=1010 drift=1 dur=60
[tmr] stalled period#6 act=1211 ideal=1210 drift=1 dur=60
[tmr] stalled period#7 act=1411 ideal=1410 drift=1 dur=60
[tmr] stalled period#8 act=1611 ideal=1610 drift=1 dur=60
```

`dur=60` 表示回调确实占了 60 tick，但 `drift` 一直是 **1**（那 1 是 tick 对齐与测量量化的固定偏移，
`normal` 段同样是 1）。周期 200、回调 60，**回调占总周期的 30%，相位一点没动**。

**反向推论**：如果 `dur > period`，重装就会落后，于是进 §8 的补发循环。
这正是"回调不能慢"在数量上的一条界线：**慢于周期才会开始丢节拍，慢于周期之前只是占用别人。**

### 8. 落后之后是"追赶补发"，不是"永久偏移"

`prvReloadTimer` 不是一次插入，而是一个 **while 循环**（`timers.c:695-711`）：

```c
static void prvReloadTimer( Timer_t * const pxTimer, TickType_t xExpiredTime, const TickType_t xTimeNow )
{
    /* 如果下一次到期时间已经过去了，就把到期时间前推一个周期、调一次回调，然后再试 */
    while( prvInsertTimerInActiveList( pxTimer, ( xExpiredTime + pxTimer->xTimerPeriodInTicks ),
                                       xTimeNow, xExpiredTime ) != pdFALSE )
    {
        xExpiredTime += pxTimer->xTimerPeriodInTicks;
        traceTIMER_EXPIRED( pxTimer );
        pxTimer->pxCallbackFunction( ( TimerHandle_t ) pxTimer );    /* ← 循环里补发 */
    }
}
```

`prvInsertTimerInActiveList` 返回 `pdTRUE` 的含义是"**这个到期时间已经过时了**"。
所以在一次守护任务运行中，落后的定时器会被**连续补齐若干次**，补发的回调**共享同一个 tick**。

本仓库的读数正好有 **4 组**这样的重复（不是一次巧合）：

```text
[tmr] after fast#20 act=1071 ideal=1010 drift=61
[tmr] after fast#21 act=1071 ideal=1060 drift=11
[tmr] after fast#22 act=1111 ideal=1110 drift=1
[tmr] after fast#24 act=1271 ideal=1210 drift=61
[tmr] after fast#25 act=1271 ideal=1260 drift=11
[tmr] after fast#26 act=1311 ideal=1310 drift=1
（#28/#29 @1471、#32/#33 @1671 同形）
```

**这条链条可以完整推通**（用同一份日志里的两个读数对上）：

```text
periodic 周期 200 tick，在 tick 1011 开始忙等 60 tick          ← stalled period#5 act=1011 dur=60
  → 守护任务被独占，到 tick 1071 才回到循环                      ← 1011 + 60 = 1071
  → 此时 fast（周期 50）的两个到期点 1010 与 1060 都已过时
  → 循环连续补发两次，两次都记为 tick 1071                      ← fast#20 (ideal 1010) 与 fast#21 (ideal 1060)
  → 下一次到期按 1110 算，实到 1111                             ← fast#22 drift 回到 1
```

三条读法：

1. **两次回调共用 `act=1071`**：这是"同一轮循环里补发"的直接证据，`drift=61` 与 `drift=11`
   只是相对各自理想时刻的差（`1071-1010` 与 `1071-1060`）。
2. **`#22` 回到 `drift=1`**：说明追赶**只补到不落后为止**，节拍网格没有被永久拉歪。
   代价是"**丢节拍**"（这几拍挤在一起），不是"永久偏移"。
3. **四组间隔正好 200 tick**：与 `periodic` 的周期一致，说明每一次忙等都是同一个原因造成的，
   而不是随机抖动。

**跨运行漂移**：实验 `main.c` 的注释里记的是 `#20/#21 均为 1067`，本次日志是 **1071**，
差 4 tick。两次是不同运行，所以本篇所有断言都写成"关系"（同 tick / 回到 drift=1 / 间隔 200），
**不写死具体 tick 数**。

### 9. 四个状态转换函数的语义对照

教材按状态转换图逐一给出（`rtos.txt:5838-6016`）。压成一张表：

| 函数 | 冬眠 → 运行 | 运行中调用 | 无条件改周期 |
| --- | --- | --- | --- |
| `xTimerStart` | ✅ 启动 | **相当于 `xTimerReset`**（重新计时，见下） | ❌ |
| `xTimerStop` | 无变化 | ✅ 回冬眠 | ❌ |
| `xTimerReset` | ✅ 启动 | ✅ 重新确定超时时间 `tX + n` | ❌ |
| `xTimerChangePeriod` | ✅ 启动（**用新周期**） | ✅ 用新周期重新计时 | ✅ |

教材对 `Start` 在运行中调用给了一句很精确的描述（`rtos.txt:5900-5902`）：

> 如果定时器已经被启动，但是它的函数尚未被执行，再次执行 `xTimerStart()` 函数
> 相当于执行 `xTimerReset()`，重新设定它的启动时间。

注意那个前提——"**它的函数尚未被执行**"。这句话的正确读法：两次 `Start` 在守护任务里
都被处理成"设定到期时刻"，后一条覆盖前一条，所以效果等同于 `Reset`。
但如果第一次的到期**已经发生**，第二次 `Start` 只是重新开始一轮，那本来就是 `Reset` 的语义。

`ChangePeriod` 的特殊价值在于它**一个命令同时完成两件事**（教材 16.4 的示例正是这么用的，§10）：
改周期 + 从冬眠转运行。所以"每次用不同时长的一次性定时器"这种场景，
不需要 `ChangePeriod` + `Start` 两条命令，一条就够——**代价是命令队列只占一个位置而不是两个**，
在队列长度只有 5 的时候这不是小事。

### 10. 定时器 ID：唯一不走命令队列的那一对函数

教材 16.3.6 明确写出（`rtos.txt:6040-6044`）：

> 这两个函数**不涉及命令队列**，它们是**直接操作定时器结构体**。

| 函数 | 作用 | 走队列 |
| --- | --- | --- |
| `pvTimerGetTimerID( xTimer )` | 读 `pvTimerID` | ❌ |
| `vTimerSetTimerID( xTimer, pvNewID )` | 写 `pvTimerID` | ❌ |

与"所有其它定时器 API 都发命令"对照，这条例外的后果很实际：

```text
走命令队列：能保证"按顺序"，但有延迟、可能 pdFAIL、不能在回调里安全调用（队列可能满）
不走队列：  立刻生效、不会失败、可以在任何上下文里调 —— 但没有顺序保证、无原子性
```

这也是 `Day07` §2 里"约定 vs 硬检查"的同一类判断：
**一个 API 走不走队列，决定了它能不能在回调里用。**

教材给的 ID 用途（`rtos.txt:6031-6036`）：① 标记"我是哪个定时器"；② **保存参数供回调使用**。
本仓库三个定时器都传 `NULL`（`main.c:219-221`），即没有使用 ID——
所以第二条用途在本仓库**没有任何读数**。

### 11. 教材示例的音效结构，与本仓库的同构对照

教材 16.4（`rtos.txt:6045-6117`）用无源蜂鸣器做游戏音效：撞到砖块时响，过一阵停。

```c
25 static TimerHandle_t g_TimerSound;
58  g_TimerSound = xTimerCreate( "GameSound", 200, pdFALSE, NULL, GameSoundTimer_Func );
                                                            /* ↑ 一次性 */
37  static void GameSoundTimer_Func( TimerHandle_t xTimer ) { PassiveBuzzer_Control(0); }   /* 停 */

78  void buzzer_buzz(int freq, int time_ms) {
80      PassiveBuzzer_Set_Freq_Duty(freq, 50);      /* 设置音调 */
83      xTimerChangePeriod(g_TimerSound, time_ms, 0);   /* 改周期 + 启动，一条命令 */
    }
```

这个设计的两个要点值得单独指出：

1. **`xTimerCreate` 的 `200` 是个"占位周期"**。创建之后从未 `Start`，
   每次都由 `ChangePeriod` 用真实时长覆盖。教材没有说明这一点，
   读者容易以为 200 有含义——它只影响"万一有人 Start 了它"的那一次。
2. **`xTimerChangePeriod` 一条命令完成了"改周期 + 启动"**，正好是 §9 表里那一格。

本仓库 `F103_13` 是**同一机制的非游戏版本**，且把"看不见的"补成了读数：

| 教材示例 | 本仓库对应 | 多出来的东西 |
| --- | --- | --- |
| 一次性定时器停蜂鸣器 | `once`（300 ms） | `drift`、`in_daemon`、`Reset` 复用（`fire#2`） |
| 周期性动作 | `periodic`（200 ms） | `dur`（回调耗时）、`drift` |
| —— | `fast`（50 ms） | **别人的回调慢，我的代价是多少**（§8 的补发） |
| —— | 注入 60 tick 忙等 | 把"回调不能慢"变成可测量的数 |

最后一行是教材完全没有的：**教材告诉你"不要慢"，本仓库量出"慢了之后谁付出多少"。**

### 12. 教材本章的缺陷（只记录，不改教材代码）

| 位置 | 缺陷 | 照抄的后果 |
| --- | --- | --- |
| `16.4` 第 83 行 | `xTimerChangePeriod(g_TimerSound, time_ms, 0)` 的 `xTicksToWait` 传 **0**，且**未检查返回值** | 队列满时命令直接丢，**蜂鸣器会响个不停**（回调永不执行）。这与 §5 的第三档风险是同一件事 |
| `16.4` 第 58 行 | `xTimerCreate` 的周期 `200` 未说明是"占位值"，实际总被 `ChangePeriod` 覆盖 | 读者会去找 200 的业务含义；改坏它也不会有任何现象 |
| `16.2.3` | "回调函数要尽快实行"只给了定性约束，**没有给出与周期的关系** | 缺少 §7 那条界线："回调耗时 < 周期 ⇒ 只占别人；> 周期 ⇒ 开始丢节拍" |
| `16.1` | 时序示图把 `Start` 画成"t1 启动、t7 到期"的精确网格，**未标注"启动是异步的"** | 与 16.2.2 的"`xTimerStart()` 返回不代表已启动"直接矛盾。示图是理想化的，正文才是实际 |
| `16.3.6` | 说 ID 不走队列，但**未提示由此带来的后果**（可安全用于回调、无顺序保证） | 容易在回调里改 ID 后又在任务里读，得到"看起来乱序"的值 |

第一条是最实在的一条：它把 §5 讲得那么清楚的"0 = 不等待"，在示例里自己违反了。

---

## 系统调用/接口卡片

### `TimerHandle_t xTimerCreate( name, period, uxAutoReload, pvTimerID, pxCallback )`

作用：动态创建定时器，**创建后处于冬眠态**。

参数：

| 参数 | 说明 |
| --- | --- |
| `pcTimerName` | 仅调试用（教材："用处不大，仅在调试时用到"） |
| `xTimerPeriodInTicks` | 周期，**以 tick 为单位**——用 `pdMS_TO_TICKS()` 换算 |
| `uxAutoReload` | `pdTRUE` 自动加载 / `pdFALSE` 一次性 |
| `pvTimerID` | 回调里可取出的自定义值，可传 `NULL` |
| `pxCallbackFunction` | `void (*)( TimerHandle_t )` |

返回值和错误：句柄；**堆不足返回 `NULL`**。本仓库实测一个 `Timer_t` 占 **48 B**。

资源所有权：结构体由内核堆分配；`xTimerDelete` 回收。

线程/进程安全：任务态创建。ISR 里不能创建。

最小示例（`F103_13/main.c:219-221`）：

```c
s_once     = xTimerCreate( "once",     pdMS_TO_TICKS( 300U ), pdFALSE, NULL, cb_once );
s_periodic = xTimerCreate( "periodic", pdMS_TO_TICKS( 200U ), pdTRUE,  NULL, cb_periodic );
s_fast     = xTimerCreate( "fast",     pdMS_TO_TICKS(  50U ), pdTRUE,  NULL, cb_fast );
```

常见误用：① 以为创建就会跑（冬眠态，必须 `Start`）；② 周期写成毫秒原值（`300` 而非 `pdMS_TO_TICKS(300)`，
在 `configTICK_RATE_HZ != 1000` 时就不是 300 ms 了）。

### `BaseType_t xTimerStart( TimerHandle_t t, TickType_t xTicksToWait )`

作用：把"启动"命令发进命令队列。**返回不代表定时器已启动**（§4）。

参数：`xTicksToWait` = **命令入队**的超时，不是定时器周期（§5）。

返回值和错误：`pdPASS` = 命令成功入队；`pdFAIL` = 在 `xTicksToWait` 内入队失败（队列满）。

最小示例（`F103_13/main.c:247-260`，每次都检查返回值并区分"队列满"）：

```c
if( xTimerStart( s_once, pdMS_TO_TICKS( 100U ) ) != pdPASS )
{
    LOG( "[tmr] xTimerStart(once) 失败：命令队列满\n" );
}
```

常见误用：① 不检查 `pdFAIL`（命令丢了，定时器根本不会跑，且没有任何后续报错）；
② 传 `portMAX_DELAY` 而守护任务被慢回调占住 → 调用方永久阻塞。

### `BaseType_t xTimerStartFromISR( TimerHandle_t t, BaseType_t *pxHigherPriorityTaskWoken )`

作用：ISR 里启动定时器。**没有 `xTicksToWait`**（中断不能阻塞）。

参数：`pxHigherPriorityTaskWoken` 表示"**守护任务**被唤醒且优先级高于被中断任务"
（不是"业务任务"，与 `Day08` §7 的同类陷阱一致）。

资源所有权：无。

线程/进程安全：仅 ISR。必须接着调 `portYIELD_FROM_ISR()`。

常见误用：漏 `portYIELD_FROM_ISR`（命令处理延迟到下一 tick）；把 `pxWoken` 传 `NULL`。

### `BaseType_t xTimerStop( TimerHandle_t t, TickType_t xTicksToWait )`

作用：运行 → 冬眠。

返回值和错误：`pdPASS` / `pdFAIL`（命令入队失败）。

**验证"真的停住了"的唯一办法是看回调计数是否不再增长**，不能只看返回值。
本仓库就是这么做的（`F103_13/main.c:337-345`）：

```c
[tmr] 已停止：periodic 前=10 后=10 差=0；fast 前=40 后=40 差=0（都应为 0）
```

常见误用：`Stop` 之后立刻假设"再也没有回调"，但**已排队的到期可能还在本轮的补发循环里**（§8）。

### `BaseType_t xTimerReset( TimerHandle_t t, TickType_t xTicksToWait )`

作用：冬眠 → 运行；运行 → 重新确定超时时间（`tX + n`）。

返回值和错误：`pdPASS` / `pdFAIL`。

最小示例（`F103_13/main.c:279`，一次性定时器的复用）：

```c
/* 单次定时器到期后自动回冬眠；xTimerReset 让它重新计时 */
if( xTimerReset( s_once, pdMS_TO_TICKS( 100U ) ) != pdPASS ) { LOG("失败\n"); }
```

实测证据：`once fire#2 act=615 ideal=615 drift=0（reset 生效，单次定时器可复用）`。

常见误用：把 `Reset` 当成"清零计数"或"重新开始一整轮"——它只重设**到期时刻**，不改周期、不清你的回调内状态。

### `BaseType_t xTimerChangePeriod( TimerHandle_t t, TickType_t xNewPeriod, TickType_t xTicksToWait )`

作用：改周期；若在冬眠则**同时启动**（用新周期算到期）。等价于"`Stop` + 设新周期 + `Start`"，
但只占**一条**命令。

参数：`xNewPeriod` 单位 tick；`xTicksToWait` 同样是命令入队超时。

返回值和错误：`pdPASS` / `pdFAIL`。

最小示例（教材 16.4 的用法，`rtos.txt:6080-6083`）：

```c
/* 一条命令：改成 time_ms 这个周期并立即启动 */
xTimerChangePeriod( g_TimerSound, time_ms, 0 );   /* ← 0 = 不等待，可能直接失败 */
```

**改进写法**：把 `0` 换成一个小超时并检查返回值，否则 §12 第一条缺陷就会复现。

常见误用：`xNewPeriod` 传毫秒未换算；传 `0` 又不检查返回值。

### `BaseType_t xTimerDelete( TimerHandle_t t, TickType_t xTicksToWait )`

作用：删除定时器、回收内存。

返回值和错误：`pdPASS` / `pdFAIL`（命令入队失败）。

资源所有权：**删除命令是异步的**——返回 `pdPASS` 只说明命令入队了，结构体可能还没释放。
教材未提示这一点，但与 §4 是同一模式。

常见误用：`Delete` 之后立刻再次使用旧句柄。

### `void *pvTimerGetTimerID( TimerHandle_t t )` / `void vTimerSetTimerID( TimerHandle_t t, void *pvNewID )`

作用：读写定时器 ID。**不走命令队列**（§10），直接操作结构体。

返回值和错误：`pvTimerGetTimerID` 返回 ID；`vTimerSetTimerID` 无返回值、**不会失败**。

线程/进程安全：因为没有队列，任务态与**回调内**都可以调用。代价是**没有顺序保证**。

常见误用：以为它能像其它 API 一样"排队执行"；在多处并发改写时不加额外同步。

### `TaskHandle_t xTimerGetTimerDaemonTaskHandle( void )`

作用：取守护任务句柄。**仅当 `configUSE_TIMERS == 1` 时存在。**

用途（本仓库的用法，`F103_13/main.c:89-94`）：在回调里自证上下文，而不只是相信文档：

```c
if( xTaskGetCurrentTaskHandle() == xTimerGetTimerDaemonTaskHandle() )
{
    s_in_daemon = 1U;
}
```

实测结果 `in_daemon=1`——**回调确实跑在守护任务里**。

常见误用：在 `configUSE_TIMERS == 0` 的工程里调用（编译期不存在，链接失败）。

---

## 最小可运行实验

```text
app     (prio 2)  创建 3 个定时器 → Start → 读到期 tick → 重启/注入/停止
monitor (prio 1)  每 500 ms 打印堆余量与三个任务的栈水位
守护任务 (prio 4) 由 configUSE_TIMERS=1 在 vTaskStartScheduler 时创建，跑所有回调
```

| 定时器 | 周期 | 类型 | 作用 |
| --- | --- | --- | --- |
| `once` | 300 ms | 一次性 | 证明 `xTimerCreate` 不自动启动；证明到期后回冬眠、`Reset` 可复用 |
| `periodic` | 200 ms | 自动加载 | 证明"回调忙等 60 tick 不改变自己的相位"（§7） |
| `fast` | 50 ms | 自动加载 | 承载代价：观测别人忙等时自己丢几拍、之后能否追上（§8） |

设计上有两处刻意：

1. **创建后先等一个周期再 `Start`**，否则"创建即启动"与"手工启动"两种语义分不清。
2. **注入 `STALL_TICKS = 60 > FAST_PERIOD_MS = 50`**。第一次实测没跨过这个分界，
   整段注入**没有产生任何可读的延迟**——参数选择本身就是实验设计的一部分（§6）。

---

## 编译与运行

```bash
cd experiments/F103_13_FreeRTOS_Timers
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake -DF103_QEMU=ON
cmake --build build

cd ../..
timeout 25 qemu-system-arm -M stm32vldiscovery -display none -monitor none \
  -serial stdio -kernel experiments/F103_13_FreeRTOS_Timers/build/f103_13_freertos_timers.elf \
  < /dev/null 2>&1 | tr -d '\r' | tee tests/qemu-stm32/logs/exp13_timers.log

# 判定器
./tests/qemu-stm32/run.sh exp13
```

注意 `F103_FREERTOS_USE_TIMERS` 的开关位置（`FreeRTOSConfig.h:52-55`）：**默认 0**，
只有本实验用 `-DF103_FREERTOS_USE_TIMERS=1` 打开。这样做是为了让 `F103_10`–`F103_12`
的体积与行为保持不变（`FreeRTOSConfig.h:50-51` 的注释写明了这一点）。

---

## 结果观察

### 配置与内存（实测）

```text
[F103_13] configUSE_TIMERS=1 daemon prio=4 queue=5 stack=160 words
[F103_13] 互斥量建好后堆余量 4000 B
[F103_13] 两个任务建好后堆余量 2384 B，启动调度器（守护任务、定时器命令队列与 idle 由内核创建）
[tmr] 三个 Timer_t 用掉 144 B（堆余量 880 → 736）
[tmr] 三个定时器已创建（休眠状态）：once=300 ms 单次，periodic=200 ms，fast=50 ms 周期，daemon 优先级=4
```

| 读数 | 证明 |
| --- | --- |
| `daemon prio=4` = `configMAX_PRIORITIES-1` | 守护任务是全系统最高优先级（`FreeRTOSConfig.h:61`） |
| `queue=5` | 命令队列长度；**同时也是 `Day08` 事件组 ISR 版的并发上限** |
| `144 B / 3 个` | 每个 `Timer_t` 48 B，创建后不再分配（后续 `heap_free` 恒 736 B） |
| "休眠状态" | `xTimerCreate` 不启动——教材 16.1 的"冬眠"落地 |

### 单次定时器：不自动启动，到期回冬眠，`Reset` 可复用（实测）

```text
[tmr] once fire#1 act=310 ideal=310 drift=0 in_daemon=1
[tmr] once fire#2 act=615 ideal=615 drift=0（reset 生效，单次定时器可复用）
```

| 读数 | 证明 |
| --- | --- |
| `drift=0`（周期 300，实到 310，基准 tick 10） | 启动命令被及时处理，到期准点 |
| `in_daemon=1` | **回调确实在守护任务上下文**——句柄比对得出，不是照抄文档 |
| `fire#2` 存在且 `drift=0` | 一次性定时器到期后回冬眠，`xTimerReset` 能重新计时（§9） |

### 周期定时器：回调慢不改变自己的相位（实测）

```text
[tmr] normal period#1 act=211 ideal=210 drift=1 dur=0
[tmr] normal period#4 act=811 ideal=810 drift=1 dur=0
[tmr] stalled period#5 act=1011 ideal=1010 drift=1 dur=60
[tmr] stalled period#8 act=1611 ideal=1610 drift=1 dur=60
[tmr] after  period#9 act=1811 ideal=1810 drift=1 dur=0
```

`dur` 从 0 跳到 60，`drift` **纹丝不动地保持 1**。这就是 §7 那条源码事实的实测证据：
**重装发生在回调之前，所以回调耗时不计入下一拍的基准。**
`drift=1` 这个常量偏移来自 tick 对齐与测量量化，`normal` 段同样是 1，不是误差。

### 轻定时器：别人忙等的代价是"丢节拍"，不是"永久偏移"（实测）

```text
[tmr] normal fast#1 act=61 ideal=60 drift=1
[tmr] normal fast_total=16 max_drift=1

[tmr] after fast#20 act=1071 ideal=1010 drift=61
[tmr] after fast#21 act=1071 ideal=1060 drift=11
[tmr] after fast#22 act=1111 ideal=1110 drift=1
（#24/#25 @1271、#28/#29 @1471、#32/#33 @1671 同形，共 4 组）
[tmr] after fast_total=40 max_drift=61
```

§8 已经把这个链条推通了，这里只补三条**判据级**的读法：

1. **判据不是 `drift=61`，而是"两次回调共享同一个 `act`"**。
   单看某个 `drift` 值无法区分"调度抖动"与"循环补发"；共享 tick 才是补发的指纹。
2. **判据还必须有 `#22` 的 `drift=1`**。没有它，"落后"与"永久拉歪"无法区分。
   **这两条要一起看，缺一条结论就不成立。**
3. **判据的最后一条是"4 组间隔 200 tick"**：与 `periodic` 的周期一致，
   证明 4 次都是同一个原因（periodic 的忙等），而不是偶发抖动。

### 停止：唯一可靠的证据是计数不再增长（实测）

```text
[tmr] 已停止：periodic 前=10 后=10 差=0；fast 前=40 后=40 差=0（都应为 0）
```

`xTimerStop` 返回 `pdPASS` 只说明命令入队了（§4 的同一模式）。
**"停住了"必须用"停止后等待一段时间，计数不变"来证明。**
实验先停 `fast` 再停 `periodic`，注释写明了原因：不停住 `fast`，
后面 600 ms 的观察期它会多出十几次到期，干扰读数。

### 本篇**没有**验证的东西

- **教材 `28_timer_game_sound` 工程本机没有**（依赖 `nwatch` 的蜂鸣器驱动），
  §11 的对照是"机制同构"，不是同一份代码。
- **命令队列写满导致的 `pdFAIL` 没有构造出来**。队列长度 5，实验里最多同时有 3 条命令，
  所以每次 `xTimerStart`/`Stop`/`Reset` 的 `pdFAIL` 分支**一次都没有执行过**。
  §5 那张三档风险表里 `0` 与 `portMAX_DELAY` 的实际后果，只有推理，没有读数。
- **四个 ISR 变体全未使用**：`xTimerStartFromISR`、`xTimerStopFromISR`、`xTimerResetFromISR`、
  `xTimerChangePeriodFromISR`。
- **定时器 ID 完全未使用**（三个定时器都传 `NULL`），§10 的第二条用途（保存参数）无读数。
- **守护任务被饿死的极端情形未测**：如果某个回调永不返回，`xTimerStart` 传
  `portMAX_DELAY` 会不会永久阻塞——推理是"会"，但没有实测。
- **回调内调用带 `0` 超时的 `xQueueReceive`**（教材允许的那一条）未测。
- 本实验的时间基准是"固件毫秒"（`main.c` 头注释：QEMU 的 SYSCLK 是 24 MHz 而固件按 8 MHz 计算，
  实测固件时间比真实时间快 **2.90 倍**），**所以所有绝对值只用于比较比例，不能当实时性结论**。

---

## 常见错误和陷阱

| 陷阱 | 症状 | 根因 | 处理 |
| --- | --- | --- | --- |
| 创建后以为在跑 | 回调从不执行，无任何报错 | `xTimerCreate` 创建的是**冬眠**态 | 必须显式 `Start`/`Reset`/`ChangePeriod` |
| 把 `xTimerStart` 的 `pdPASS` 当"已启动" | 紧接着查状态发现没动 | 返回只表示**命令入队**（§4） | 要确证"在跑"，只能等回调或查剩余时间 |
| 把 `xTicksToWait` 当周期 | 定时器周期完全不对 | 它是**命令队列**入队超时（§5） | 周期只由 `xTimerCreate`/`ChangePeriod` 决定 |
| `xTicksToWait` 传 `0` 又不查返回值 | 偶尔定时器压根不启动 | 队列满时命令直接丢 | 检查 `pdFAIL`；或给一个小超时（§12 第一条） |
| `xTicksToWait` 传 `portMAX_DELAY` | 调用方卡死 | 守护任务被慢回调占住，命令永远塞不进去 | 定时器 API 用小超时，别用 `portMAX_DELAY` |
| 在回调里 `vTaskDelay` | **全系统**定时器全部延迟 | 回调跑在共享的守护任务里（§6） | 回调只记账；重活丢给任务/队列 |
| 在回调里 `printf` | 串口变慢、其它定时器抖动 | 同上，且 `printf` 本身就慢 | 回调里只置标志，由任务打印 |
| 回调耗时接近周期 | 开始出现同 tick 连续回调 | `prvReloadTimer` 的追赶循环（§8） | 把耗时压到远小于周期，或换独立任务 |
| 回调耗时**超过**周期 | 周期定时器实际频率变高（补发） | 落后就要补，补发无上限 | 这是设计错误，不是调参问题 |
| 停止后立刻假设没有回调 | 停止后还看到几次回调 | 已排队的到期可能在本轮补发中 | 用"停止后计数不变"验证（实测 差=0） |
| 用 `xTimerCreate` 的周期做业务参数 | 改了没反应 | 若一直被 `ChangePeriod` 覆盖，它是占位值 | 明确注释它是占位，或干脆用真实值创建 |
| 在 ISR 里用任务版 API | 断言失败/行为未定义 | 任务版可能阻塞在命令队列上 | 用 `...FromISR` + `portYIELD_FROM_ISR` |
| 以为 `xTimerDelete` 是同步的 | 删完还用旧句柄，偶发崩溃 | 删除也是**命令**，异步执行 | 删除后不再引用；必要时自己加同步 |

---

## 故障排查方法

1. **"定时器的回调根本不执行。"** 按顺序查五件事：
   ① `xTimerCreate` 之后有没有 `Start`（冬眠态）；② `Start` 的返回值是不是 `pdFAIL`（队列满）；
   ③ `configUSE_TIMERS` 是不是 1（0 时整个 `timers.c` 不参与编译，`xTimerCreate` 链接失败，
   但如果从别处拿到句柄则更隐蔽）；④ 周期是不是写成了毫秒原值未换算；
   ⑤ 有没有人先把状态清成了冬眠（`xTimerStop`）。
2. **"定时器周期不对。"** 先分清两个数：周期（`xTimerCreate`/`ChangePeriod`）与
   命令队列超时（每个 API 的最后一个参数）。**90% 的"周期不对"是把后者当成了前者。**
3. **"别的定时器变慢了，我的代码没动。"** 找守护任务里的**慢回调**。
   判据：看同一段时间里是否有多个定时器**共享同一个到期 tick**（§8 的补发指纹）。
4. **"回调里调了 `vTaskDelay`，系统像卡住。"** 这就是 §6 的后果。改法：
   回调里只置标志/投递队列，把耗时工作交给独立任务。
5. **"想确认回调跑在哪个任务里。"** 用 `xTaskGetCurrentTaskHandle() == xTimerGetTimerDaemonTaskHandle()`
   比对（本仓库 `in_daemon=1` 就是这么来的），别只看文档。
6. **"`xTimerStart` 之后马上查剩余时间，结果还是旧的。"** 命令可能还没被处理（§4）。
   要观测"启动生效"，等的应该是**到期回调**，不是启动函数的返回。
7. **"停止之后还有回调。"** 可能是本轮补发循环里已排队的到期（§8）。
   用"停止后等一段时间、计数不变"来确证，而不是看 `pdPASS`。

---

## 练习题

### 基础题

1. 定时器的"两种类型 × 两个状态"各是什么？为什么 `xTimerCreate` 出来的是冬眠态？
2. 教材说"不允许在内核、在中断中执行不确定的代码"。这句里的"不确定"指什么？
   换成"在中断里跑回调会更简单"这种说法，会漏掉什么代价？
3. `xTimerStart` 返回 `pdPASS` 时，定时器可能还没启动。请举出两种"必须还没启动"的情形，
   并说明每种情形下超时从哪一刻开始算。
4. 每个定时器 API 的 `xTicksToWait` 等的是什么？它与周期是什么关系？

### 改错题

5. 下面这段在回调里做重活，指出三处问题：

   ```c
   static void on_tick( TimerHandle_t t) {
       printf("tick\n");                    /* ① */
       vTaskDelay(pdMS_TO_TICKS(10));       /* ② */
       xQueueSend( g_q, &data, portMAX_DELAY ); /* ③ */
   }
   ```

6. 下面这段创建后立即使用，说明为什么"看起来能跑"其实是巧合：

   ```c
   TimerHandle_t t = xTimerCreate("t", 100, pdTRUE, NULL, cb);
   xTimerStart(t, 0);
   /* 立刻假设定时器已在运行，开始依赖它的回调更新状态 */
   ```

7. 下面这段想"每次用不同时长"，指出风险：

   ```c
   xTimerChangePeriod(g_buzz, time_ms, 0);      /* 没检查返回值 */
   PassiveBuzzer_Set_Freq_Duty(freq, 50);
   ```

### 综合题

8. 用 `F103_13` 的读数回答：如果 `STALL_TICKS` 从 60 改成 **40**（小于 `FAST_PERIOD_MS = 50`），
   `fast` 还会出现"两次共享同一 tick"吗？给出你的预测、你要采集的量，
   以及"为什么预测错了也是有价值的结果"。
9. 设计一个实验把 `pdFAIL` 分支真的走一次：让命令队列（长度 5）写满。
   要求：① 说明你怎么在不改内核的前提下做到；② 你要采集什么；
   ③ 如果改用 `portMAX_DELAY`，这个实验会变成什么样的"挂死"，你怎么设一个能判定的上界。
10. 把 §7 与 §8 的结论合成一条工程判据，写成形如"回调耗时 ≤ X × 周期，否则必须 Y"的规则，
    并说明这条规则**依赖**哪个实验读数、**不依赖**哪个（提示：哪个读数换环境会漂）。

---

## 验收标准

- [ ] 能画出"冬眠 ↔ 运行"的两状态转换，并说明一次性定时器到期后落在哪个状态。
- [ ] 能解释为什么回调不能在 Tick 中断里跑，用的是"不允许"而不是"不方便"的理由。
- [ ] 能说明 `xTimerStart` 返回值的**确切**含义，并指出超时从哪一刻算起。
- [ ] 能指出所有定时器 API 的 `xTicksToWait` 等的是命令队列，并说出三档取值的风险。
- [ ] 能引用源码说明"重装先于回调"，并用 `dur=60 drift=1` 佐证。
- [ ] 能解释 `fast#20`/`#21` 共享 `act=1071` 的成因，以及为什么 `#22` 回到 `drift=1`。
- [ ] 能说出"追赶补发"的代价是丢节拍而非永久偏移，并指出支撑这条结论的两个读数。
- [ ] 能区分 `Reset` 与 `ChangePeriod` 各自能不能改周期。
- [ ] 能说明定时器 ID 为什么不走队列，以及由此带来的能力与代价。
- [ ] 能指出教材 16.4 传给 `ChangePeriod` 的 `0` 有什么风险。
- [ ] 能列出本篇**未验证**的六项及原因。

---

## 延伸阅读

- 教材第 16 章：`rtos.txt:5681-6117`
  - 16.1 特性：5686–5729（两类两态、时序示图）
  - 16.2 上下文：5730–5865（守护任务的存在理由、两个调度例子、回调三条约束）
  - 16.3 函数：5866–6044（创建/删除/启动停止/复位/改周期/ID）
  - 16.4 示例：6045–6117（`28_timer_game_sound`，蜂鸣器音效）
- 源码：`Source/timers.c`
  - `prvReloadTimer`（695–711）：补发循环
  - `prvProcessExpiredTimer`（714–740）：**重装先于回调**
  - `prvTimerTask`（744–780）：守护任务主循环（先处理到期、再清空命令队列）
  - `prvProcessTimerOrBlockTask`（782–820）：到期判定与阻塞决策
  - `prvProcessReceivedCommands`（934–1050）：命令解析，含 `ChangePeriod` 的重装（1010）
  - `timers.c:42-43`：`INCLUDE_xTimerPendFunctionCall` 与 `configUSE_TIMERS` 的编译期硬检查
- 本仓库实验：`experiments/F103_13_FreeRTOS_Timers/`（`main.c` 441 行）、
  日志 `tests/qemu-stm32/logs/exp13_timers.log`
- 配置：`F103_common/vendor/freertos/include/FreeRTOSConfig.h:50-63`
- 相关笔记：`Day03`（阻塞与超时）、`Day04`（超时三态表）、`Day08`（§7–§8：事件组 ISR 版与本章同一条命令队列）
- 下一篇：`Day10_中断管理与资源管理.md`（教材第 17–19 章）

---

**验证记录**

| 日期 | 环境一 | 命令一 | 结果一 | 环境二 | 命令二 | 结果二 | 未覆盖 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 2026-10-30 | 教材抽取文本 | 读 `rtos.txt:5681-6117` | 16.1–16.4 全章；两类两态（5707-5721）、"不允许在中断中执行不确定代码"（5730-5737）、守护任务改名与两例调度（5738-5852）、超时从调用时刻算（5791-5793、5842-5844）、回调三条约束（5854-5865）、`xTicksToWait` 是命令队列超时（5880-5884）、ID 不走队列（6040-6044）、示例 28 的 `ChangePeriod(…, 0)` 未查返回值（6080-6083） | 本仓库源码 | `sed`/`grep` 读 `Source/timers.c`、`include/FreeRTOSConfig.h` | 重装先于回调（`timers.c:714-740`）；补发 while 循环（`:695-711`）；`prvProcessReceivedCommands` 用命令里的时间戳算周期（`:1010`）；`configTIMER_*` 三项（`FreeRTOSConfig.h:61-63`）；`INCLUDE_xTimerPendFunctionCall` 与 `configUSE_TIMERS` 的 `#error`（`timers.c:42-43`） | 教材 `28_timer_game_sound` 本机无 |
| 2026-10-30 | WSL2 + QEMU 10.2.1 | `qemu-system-arm -M stm32vldiscovery -kernel f103_13_freertos_timers.elf` | `daemon prio=4 queue=5 stack=160`；`in_daemon=1`；`once fire#1 drift=0`、`fire#2 act=615 drift=0`；`stalled period#5–#8 dur=60 而 drift 恒 1`；`fast#20/#21 同 act=1071`、`#22 drift=1`，4 组同形；`fast_total=40 max_drift=61`；停止后 `差=0`；`heap_free` 恒 736 B | 本仓库实验源码 | 读 `F103_13/main.c`（441 行） | `STALL_TICKS=60 > FAST_PERIOD_MS=50` 是刻意设计（首次实测参数不足导致注入无效）；`in_daemon` 用 `xTimerGetTimerDaemonTaskHandle()` 比对而非照抄文档；`report_fast` 只在 `drift` 变化时打印 | 命令队列写满的 `pdFAIL` 未构造；ID 未使用；四个 ISR 变体未用；时间基准受 QEMU 2.90× 偏差影响 |
