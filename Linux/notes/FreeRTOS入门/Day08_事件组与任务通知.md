# Day08：事件组与任务通知（教材第 14–15 章）

**验证状态**：事件组的"与"语义与超时**实测**（`exp12_sync.log`：`event wait_all got=0x03 waited=200`、
`event timeout got=0x00 waited=300`）；事件组位宽、`EventBits_t` 的实际类型、**ISR 版为何不在本仓库编译产物里**
全部**源码核对**（`event_groups.h`、`event_groups.c`、`FreeRTOS.h`、`FreeRTOSConfig.h`，行号见下）；
**未验证**：任务通知在本仓库**零使用**（`grep` 无任何调用点），故其全部结论来自教材与源码；
广播（两个任务同时被唤醒）、`xEventGroupSync`、`xEventGroupSetBitsFromISR` 三条路径**无实验**。

---

## 学习目标

学完本篇应能用代码证据回答：

1. 事件组"就是一个整数"——它的类型是什么？为什么这个类型**必然**跟着 `configUSE_16_BIT_TICKS` 走？
2. 本仓库一共能表达多少个事件位？为什么不是 32 个？
3. 事件组与队列/信号量只有两处分歧，分别是哪两处？第二处为什么带来"要不要清除"这个选择？
4. `xClearOnExit = pdTRUE` 解决了什么？为什么"先 WaitBits 再 ClearBits"不行？
5. 教材 AND/OR 对照表里有一行的文字与掩码对不上——是哪一行？照抄的后果是什么？
6. `xEventGroupSetBitsFromISR` 返回 `pdPASS` 时，事件位**已经**置上了吗？为什么这么设计？
7. 本仓库为什么**根本编译不出** `xEventGroupSetBitsFromISR`？两个宏各是什么？
8. 任务通知的五条限制里，哪三条是"事件组能做的事它做不到"？
9. `ulTaskNotifyTake` 的 `xClearCountOnExit` 两个取值，分别等价于哪种对象？
10. `xTaskNotify` 的五种 `eAction` 各等价于什么？唯一会失败的是哪一种？

---

## 环境与前置条件

| 项 | 版本 / 说明 |
| --- | --- |
| 教材 | 第 14 章事件组（`rtos.txt:5068-5395`）、第 15 章任务通知（`rtos.txt:5396-5680`） |
| 内核 | FreeRTOS **V11.3.0**；实现于 `Source/event_groups.c`、`tasks.c`（通知部分） |
| 关键配置 | `configUSE_16_BIT_TICKS` **未定义** ⇒ 32 位 ⇒ **24 个可用事件位**；`configUSE_TASK_NOTIFICATIONS 1`（`:40`）；`configTASK_NOTIFICATION_ARRAY_ENTRIES 1`（`:41`）；`configUSE_TIMERS = F103_FREERTOS_USE_TIMERS = 0`（`:52-55`）；`INCLUDE_xTimerPendFunctionCall` **未定义 ⇒ 0**（`FreeRTOS.h:300-301`） |
| 实验 | `F103_12_FreeRTOS_Sync`（事件组"与" + 超时两条路径）；任务通知**无实验** |
| 前置 | `Day04`（四问表：第 3 问"唤醒几个"）、`Day05`（队列做不到广播）、`Day07`（互斥量） |

---

## 核心概念

### 1. "事件组就是一个整数"——但这个整数的类型是 `TickType_t`

教材的说法（`rtos.txt:5090-5097`）：每一位表示一个事件，值 1 表示发生，任务和 ISR 都能读写，
可以等某一位、某些位中的任意一个（"或"），也可以等多位（"与"）。

接着它给了位宽规则：

| `configUSE_16_BIT_TICKS` | 整数宽度 | 可用事件位 |
| --- | --- | --- |
| `1` | 16 位 | **低 8 位**（bit0–bit7） |
| `0` | 32 位 | **低 24 位**（bit0–bit23） |

教材对"为什么 tick 的配置会影响事件组"的解释是（`rtos.txt:5096-5099`）：

> `configUSE_16_BIT_TICKS` 是用来表示 Tick Count 的，怎么会影响事件组？这只是基于效率来考虑

**源码给出的原因比"效率"更硬**（`event_groups.h:113`）：

```c
typedef TickType_t               EventBits_t;
```

`EventBits_t` **就是** `TickType_t`，是同一个类型。所以位宽不是"为了效率选了同样宽度"，
而是**根本没另立类型**。两者的关联是类型别名，不是设计上的巧合。

保留位的掩码也在同一处（`event_groups.h:46/51/56`）：

```c
16 位：eventEVENT_BITS_CONTROL_BYTES = 0xff00U                 /* 高 8 位留内核 */
32 位：eventEVENT_BITS_CONTROL_BYTES = 0xff000000U             /* 高 8 位留内核 */
64 位：eventEVENT_BITS_CONTROL_BYTES = 0xff00000000000000U     /* 高 8 位留内核（V11 新增） */
```

**本仓库的落地读数**：`configUSE_16_BIT_TICKS` 未定义 ⇒ `EventBits_t = TickType_t = uint32_t`
⇒ **可用事件位是 bit0–bit23 共 24 个**。教材只讲了 16/32 两档（它成书时还没有 64 位 tick），
第三行是这一版新增的——读到"两档"时要记得还有第三档。

一个直接后果：**写 `(1 << 24)` 及以上的掩码是无效的**，那些位属于高 8 位控制字节。
本仓库 `F103_12/main.c:47-49` 只用了 bit0–bit2，离边界很远。

### 2. 与队列/信号量的两处分歧——第二处才是事件组的本质

教材把差异收敛成两问（`rtos.txt:5104-5114`）：

| | 队列 / 信号量 | 事件组 |
| --- | --- | --- |
| **唤醒谁** | 事件发生时只唤醒**一个**任务 | 唤醒**所有符合条件的**任务（"广播"） |
| **是否消耗** | **消耗型**：数据被读走就没了，信号量被取走就减了 | 被唤醒的任务可以**保留**，也可以**清除** |

第二行值得多想一层：为什么"可以不清除"是必要的？

因为事件组表达的是**状态**而不是**资源**。"串口就绪"这个状态被一个任务看到之后，
它并没有"被消耗"——串口仍然就绪，下一个任务来读也应该是就绪的。
如果强行做成消耗型，第二个任务就只能看到"已经被人拿走了"，那是信号的语义，不是状态的语义。

而"可以选择清除"则是因为**有些场景确实是事件式的**：两个任务都在等"系统初始化完成"，
第一个到达的任务清掉位之后，第二个就不必再做一遍重复初始化。**同一个对象，两种用法，
区别在 `xClearOnExit`。**

这也解释了 Day05 §9 最后一行"1 任务 → N 任务：队列做不到"——那里教材的解法是复制 N 份队列，
而事件组一条 `SetBits` 就够。代价见 §3：**广播能力是靠"不清除"换来的，清除要自己做原子操作**。

### 3. `xClearOnExit` 不是便利选项，它是原子性要求

教材在这里写得非常准确（`rtos.txt:5257-5265`）：

> 你可以使用 `xEventGroupWaitBits()` 等待期望的事件，它发生之后再用 `xEventGroupClearBits()` 来清除。
> 但是这两个函数之间，有可能被其他任务或中断抢占，它们可能会修改事件组。
> 可以使用设置 `xClearOnExit` 为 `pdTRUE`，使得对事件组的测试、清零都在
> `xEventGroupWaitBits()` 函数内部完成，**这是一个原子操作**。

两个函数之间那段窗口会发生什么，具体推一遍：

```text
T1: WaitBits 返回（bit0 已置位）
    ← 窗口开始：T1 可能在这里被抢占，或来一个中断
ISR: SetBits(bit1)          ← 新事件插进来了
T1: ClearBits(bit0)         ← 只清 bit0？还是清 bit0|bit1？两种都错：
                              只清 bit0 → bit1 留下，但 T1 并不知道它来过
                              清两位   → 把从未见过的 bit1 一起吞掉
    ← 窗口结束
```

所以 `xClearOnExit = pdTRUE` 买到的不是"少写一行"，而是**"我看到的和我清掉的严格是同一批位"**。
这跟 Day05 §8 的"读队列集再读队列之间有竞态窗口"是同一类问题：
**两步操作之间只要能被抢占，就必须怀疑第二步的前提还成立。**

### 4. AND 与 OR：一条参数，三个例子，一处教材印错

`xEventGroupWaitBits` 的两个布尔参数决定了全部语义：

| 参数 | `pdTRUE` | `pdFALSE` |
| --- | --- | --- |
| `xWaitForAllBits` | 等**全部**位（AND） | 等**任意一个**位（OR） |
| `xClearOnExit` | 返回前清掉 `uxBitsToWaitFor` 指定的位 | 不清 |

教材的三行对照表（`rtos.txt:5220-5243`），我按"事件值 / 等待掩码 / all 标志"重新排：

| 事件组当前值 | 等待掩码 | `xWaitForAllBits` | 结果（教材原话） |
| --- | --- | --- | --- |
| `0100` | `0101` | `pdTRUE` | 期望 bit0、bit2 都 1，只有 bit2 满足 → **阻塞** |
| `0100` | `0110` | `pdFALSE` | 期望 bit**0**、bit2 某一个为 1，当前满足 → **成功** |
| `0100` | `0110` | `pdTRUE` | 期望 bit1、bit2 都 1，不满足 → **阻塞** |

**第二行印错了。** 掩码 `0110` 是 bit1|bit2，不是 bit0|bit2；教材的文字写成"bit0, bit2"，
与第一行的文字撞了。第三行用的是同一个掩码 `0110`，文字写的却是"bit1, bit2"——**这是正确的那份**。
所以第二行应为"期望 bit1、bit2 某一个为 1"。

后果不是笔误级别的：照抄第二行的人会以为"`0110` 表示 bit0 和 bit2"，
那么他写的 `(1<<0)|(1<<2)` = `0101` 与 `(1<<1)|(1<<2)` = `0110` 就永远对不上自己的预期。

**正确读法只有一条**：掩码的第 n 位对应事件组的第 n 位，逐位对齐，不要读文字。

### 5. 三个示例是两次单变量改动，读的时候要对齐

教材 14.3–14.5 是三个连续工程（`rtos.txt:5290-5385`）：

| 示例 | 工程 | 改了什么 | 现象 |
| --- | --- | --- | --- |
| 14.3 广播 | `23_eventgroup_broadcast` | car1 到终点 `SetBits(1<<0)`；car2、car3 都 `WaitBits(1<<0, pdTRUE, pdFALSE, ...)` | "car1 运行到终点后，car2、car3 **同时**启动" |
| 14.4 或 | `24_eventgroup_or` | car1 set bit0、car2 set bit1；car3 等 `(1<<0)\|(1<<1)`，**all = pdFALSE** | "任何一个到了终点，car3 就启动" |
| 14.5 与 | `25_eventgroup_and` | **只改 car3 的倒数第二个参数**为 `pdTRUE` | "**都**到达终点后，car3 才启动" |

14.4 → 14.5 是干净的单变量对照（教材自己点明了"只是 car3 的代码发生了变化"）。
14.3 与它们的关系则不同：14.3 演示的是"一份事件给多个等待者"，14.4/14.5 演示的是
"多个事件给一个等待者"。**方向相反，不能混成一个对照。**

注意 14.3 里 `xClearOnExit` 传的是 `pdTRUE`：car2 和 car3 **都**带 `pdTRUE`。
这意味着谁先醒谁把 bit0 清掉——但两者都在同一次 `SetBits` 里被唤醒了，
所以并不影响"同时启动"。**如果两个等待者的优先级不同、或者其中一个晚到**，
先到的那个清位之后，后到的就等不到了。这条边界教材没写，但它是"广播 + 清除"组合的真实风险。

### 6. `xEventGroupSync`：一次调用完成"我到齐了 + 等别人"

教材的比喻是炒菜/买酒/摆台（`rtos.txt:5267-5285`）：每个人做完自己的事要告诉别人，
还要等别人也做完，**大家一起做完才开饭**。三个动作压在一个函数里：

```text
xEventGroupSync( eg, uxBitsToSet, uxBitsToWaitFor, xTicksToWait )
                   ↑ 我完成了什么      ↑ 我要等哪些位全中
```

三条与 `WaitBits` 的差别：

1. **`uxBitsToSet` 先设位**——"我完成了"这一步不必单独调 `SetBits`。
2. **等待默认是 AND 语义**：`uxBitsToWaitFor` 里所有位都要为 1 才返回。**没有 all 参数可选**。
3. **成功返回时会自动清除**（`rtos.txt:5279`）。所以它不能像 `WaitBits` 那样"留着让别人也看到"。

第 3 条是它和"广播"的根本区别：**`Sync` 是"集合点"，用过就散**；
`WaitBits(clear=pdFALSE)` 是"公告板"，谁都能反复看。选错了就会出现
"第二个任务永远等不到集合"或"同一个事件被重复处理"。

### 7. ISR 版是**异步**的：`pdPASS` 不代表位已经置上

这是本章最容易被误读的一处。教材讲得很清楚（`rtos.txt:5170-5178`）：

> ISR 中的函数……最多只会唤醒 1 个任务。但是设置事件组时，有可能导致多个任务被唤醒，
> 这会带来很大的不确定性。所以 `xEventGroupSetBitsFromISR` 函数**不是直接去设置事件组**，
> 而是给一个 FreeRTOS 后台任务（daemon task）发送队列数据，由这个任务来设置事件组。

源码确认了实现方式（`event_groups.c:826`）：

```c
xReturn = xTimerPendFunctionCallFromISR( &vEventGroupSetBitsCallback, ( void * ) xEventGroup,
                                         ( uint32_t ) uxBitsToSet, pxHigherPriorityTaskWoken );
```

走的是**定时器命令队列**的 pend-function 机制。于是有两个必然结论：

| 你以为 | 实际 |
| --- | --- |
| 返回 `pdPASS` = 位已置好 | `pdPASS` = **请求已成功排进命令队列**。位什么时候置上取决于 daemon task 何时运行 |
| 中断返回后等待者立刻就绪 | 要等 daemon task 先跑、执行 pend 回调、再由它唤醒等待者 |

**为什么要这么绕？** 因为"一个 `SetBits` 可能唤醒多个任务"这件事，在 ISR 的上下文里
没法安全完成：唤醒多个任务需要逐个操作就绪链表、可能触发多次抢占决策，
这远超"中断里只做最少搬运"的原则。交给 daemon task 之后，中断里只剩一次入队。

`pxHigherPriorityTaskWoken` 的语义也随之变了：它表示 **daemon task 的优先级高于被中断的任务**，
而不是"某个等待事件的业务任务优先级高"。这一条极易混淆，因为排队/信号量的 FromISR 版里，
同一个参数指的是业务任务。

### 8. 但本仓库根本编译不出 ISR 版——两道宏关

`xEventGroupSetBitsFromISR` 与 `xEventGroupClearBitsFromISR` 在源码里被一个条件编译包住
（`event_groups.c:504`、`815`）：

```c
#if ( ( INCLUDE_xTimerPendFunctionCall == 1 ) && ( configUSE_TIMERS == 1 ) )
    BaseType_t xEventGroupSetBitsFromISR( ... ) { ... }
#endif
```

对照本仓库：

| 宏 | 本仓库取值 | 位置 |
| --- | --- | --- |
| `configUSE_TIMERS` | `F103_FREERTOS_USE_TIMERS` = **0**（默认） | `FreeRTOSConfig.h:52-55` |
| `INCLUDE_xTimerPendFunctionCall` | **未定义 ⇒ 0** | 内核默认值 `FreeRTOS.h:300-301` |

**所以在本仓库，无论怎么设置 `F103_FREERTOS_USE_TIMERS`，这两个函数都不存在。**
即使把定时器打开，还差第二个宏——而 `FreeRTOSConfig.h` 通篇没有定义它。

这是一个"教材不会告诉你、但一写就链接失败"的坑，形态与 `F103_17` 那条
"平台不支持的寄存器写入会被静默忽略"同类：**能力缺失不报错，只在用到时暴露**。
反过来，这也解释了为什么本仓库的任务侧 `xEventGroupSetBits` 能用（`F103_12/main.c:233`、`235`）——
它没有这两道条件编译。

要用 ISR 版事件组，需要**同时**做两件事：

```c
#define configUSE_TIMERS                 1     /* 或 F103_FREERTOS_USE_TIMERS=1 */
#define INCLUDE_xTimerPendFunctionCall   1     /* 这一条教材没提，本仓库也没定义 */
```

### 9. 任务通知：把中间结构体省掉，代价是五条限制

教材的读法很有意思（`rtos.txt:5397-5404`）：把"任务通知"**反过来读成"通知任务"**。
队列/信号量/事件组都是"双方通过一个中间结构体通信"；任务通知则是**数据直接存放在目标的 TCB 里**：

| 对象 | 数据放在哪 | 需要提前创建吗 |
| --- | --- | --- |
| 队列 / 信号量 / 事件组 | 独立的结构体 | 需要（`xQueueCreate` / `xSemaphoreCreate` / `xEventGroupCreate`） |
| **任务通知** | **接收任务的 TCB** | **不需要** |

教材列的五条限制（`rtos.txt:5410-5428`），我按"限制的是什么"重新分类：

| 限制 | 本质是缺了哪种能力 | 队列 | 信号量 | 事件组 |
| --- | --- | --- | --- | --- |
| 不能发数据给 ISR | ISR 没有 TCB | ✗（也能发给 ISR？不，队列只能给任务） | ✗ | ✗ |
| 数据只能给该任务独享 | **没有多读者** | ✓ | ✓ | ✓ |
| 无法缓冲数据（只有一个通知值） | **没有深度** | ✓（深度 N） | 计数型可到 N | ✓（多位） |
| 无法广播给多个任务 | **没有多等待者** | ✓ | ✓ | ✓ |
| 发送受阻时发送方**无法阻塞等待** | **没有背压** | ✓（满可阻塞） | ✗（满即返回失败） | —— |

最后一行值得特别注意：**任务通知在这一点上和信号量一致**（Day06 §3 的"give 永不阻塞"），
而与队列不一致。所以"用任务通知替代队列"时，你同时也丢掉了"生产者被背压"这个性质——
发送方要么成功、要么立刻失败，没有"等对方腾地方"这一档。

### 10. 三态 + 一个值：任务通知的全部状态

TCB 里就两个字段（`rtos.txt:5433-5441`）：

```c
volatile uint32_t ulNotifiedValue[ configTASK_NOTIFICATION_ARRAY_ENTRIES ];
volatile uint8_t  ucNotifyState[  configTASK_NOTIFICATION_ARRAY_ENTRIES ];
```

本仓库 `configTASK_NOTIFICATION_ARRAY_ENTRIES = 1`（`:41`）⇒ 每个任务**只有一个**通知值和一个状态。

三态（`rtos.txt:5443-5449`）：

| 状态 | 值 | 含义 |
| --- | --- | --- |
| `taskNOT_WAITING_NOTIFICATION` | 0 | 初始状态，没在等 |
| `taskWAITING_NOTIFICATION` | 1 | 正在等（阻塞中） |
| `taskNOTIFICATION_RECEIVED` | 2 | **pending**：有数据，待取 |

"pending"这个词在本章是关键词：**它不只是"收到过"，而是"收到过且还没被取走"**。
`eSetValueWithoutOverwrite` 的唯一失败条件就是它（§12）。

这个 "1 个值 + 3 个状态" 的极小结构，正好解释了为什么它能模拟五种对象——
每种对象需要的状态位，都能被这两个字段编码出来。下面两张表就是这个编码方案。

### 11. 简化版：两个函数覆盖"计数"与"二值"

| 动作 | 任务态 | ISR 态 |
| --- | --- | --- |
| 发出 | `xTaskNotifyGive( xTaskToNotify )` | `vTaskNotifyGiveFromISR( h, &woken )` |
| 取出 | `ulTaskNotifyTake( xClearCountOnExit, xTicksToWait )` | —— |

发送侧只有一种行为：**通知值 +1、状态置 pending**（`rtos.txt:5476-5479`）。

取出侧的 `xClearCountOnExit` 决定它模拟的是哪种对象：

| `xClearCountOnExit` | 返回前对通知值做什么 | 等价于 |
| --- | --- | --- |
| `pdTRUE` | **清零** | **二进制信号量**：只关心"有没有"，来了 5 次也只算 1 次 |
| `pdFALSE` | **减一** | **计数型信号量**：来了 5 次就能取 5 次 |

这正好是 Day06 §3 那条 `gives=2 coalesced=5` 的另一种实现方式——**同样的语义，更省内存**。
区别只在：二值信号量要给 7 次才有 2 次成功；任务通知的 7 次 give 全部成功（值变成 7），
但用 `pdTRUE` 取时只取到"有"这一个信息，用 `pdFALSE` 取时能取 7 次。

教材还给了返回值语义（`rtos.txt:5525-5531`）：**返回的是"清零或减一之前"的值**。
所以 `ulTaskNotifyTake(pdTRUE, ...)` 的返回值总是 1（成功）——这也是为什么
"用 `pdTRUE` 就拿不到次数"。

### 12. 专业版：五种 `eAction` 就是五种对象的编码表

这是本篇最该背下来的一张表（`rtos.txt:5545-5580`）：

| `eAction` | 对通知值做什么 | 等价对象 | 备注 |
| --- | --- | --- | --- |
| `eNoAction` | **不动**，只把状态置 pending | 二进制信号量 | `ulValue` 被忽略 |
| `eSetBits` | `通知值 \|= ulValue` | **事件组**（位语义） | 单值版事件组，不能等"多位组合" |
| `eIncrement` | `通知值 + 1` | 计数型信号量 | **等同于 `xTaskNotifyGive`** |
| `eSetValueWithoutOverwrite` | 非 pending 时才写入 | **长度为 1 的队列** | **pending 时返回 `pdFAIL`** |
| `eSetValueWithOverwrite` | 无条件覆盖 | **邮箱**（= `xQueueOverwrite`） | 旧值直接丢 |

**唯一的失败路径**（`rtos.txt:5557-5563`）：`eSetValueWithoutOverwrite` 且状态为 pending。
其余四种"大部分调用都会成功"。

这五行的判别力在于：它把"我想用哪种对象"翻译成一个枚举值。
但注意 `eSetBits` 那一行的边界——它是**位或**，不是"可以等某几位都置上"。
事件组的"与"语义（§4）在任务通知里**没有对应物**，因为 `xTaskNotifyWait` 只能判断
"状态是不是 pending"，不能判断"通知值的某几位是不是都 1"。
要"与"，只能自己拿到值以后用 `if ((val & mask) == mask)` 判——**那就不是原子等待了**。

### 13. `xTaskNotifyWait` 的两个清除点，以及取值发生在哪一步

三个参数的时间顺序是固定的，顺序搞错就会"取到被清过的值"（`rtos.txt:5600-5620`）：

```text
进入函数
  ├─ ulBitsToClearOnEntry：仅当状态**不是** pending 时执行
  │                        通知值 = 通知值 & ~ulBitsToClearOnEntry
  ├─ 若状态为 pending → 立即成功；否则按 xTicksToWait 阻塞
  └─ 退出（非超时）
       ├─ ① 先把通知值赋给 *pulNotificationValue     ← 赋值在前
       └─ ② 再执行 & ~ulBitsToClearOnExit            ← 清除在后
```

三个容易踩的点：

1. **`ulBitsToClearOnEntry` 只在"没数据"时执行**。它的本意是"我想等新事件，先把旧数据抹掉"。
   如果已经有 pending 数据，它**不清**——因为那正是你要取的东西。
2. **赋值在清除之前**。教材专门写明（`rtos.txt:5610-5612`），否则传 `ULONG_MAX` 就会拿到 0。
3. **`ulBitsToClearOnExit` 只在"因得到数据而退出"时执行**，超时退出不清。

教材示例里 `xTaskNotifyWait(~0, ~0, &val, portMAX_DELAY)`（`rtos.txt:5662`）用的
`~0` 就是 `0xFFFFFFFF` = `ULONG_MAX`，进出都全清——等价于"把通知值当成一次性信箱"。

### 14. 那个 `do { } while (val != 100)` 暴露了"它不是队列"

教材示例 car3 的写法（`rtos.txt:5655-5664`）：

```c
do {
    xTaskNotifyWait(~0, ~0, &val, portMAX_DELAY);
} while (val != 100);
```

为什么要循环？因为**通知值可能不是 100**——别的代码也可能给 car3 发通知，而
"我等的值是 100"这件事只能由调用者在拿到值之后自己判断。内核只保证
"状态是 pending 时把你的值给我"，不保证"值恰好是你要的那个"。

这是任务通知与队列的一个隐藏差别：**队列能做"按内容过滤"的阻塞等待吗？也不能**，
但它至少能"一次进来一个、按序取出"，所以调用者不用循环丢数据。
而任务通知的 `eSetValueWithOverwrite` 会**覆盖**——第 101 个通知到达时，
那个 100 就被无声地替换掉了，循环也救不回来。

所以 `do-while` 这个写法虽然"能用"，但它同时说明：**当"值不匹配"会发生时，
用任务通知就已经在跟它的"无缓冲、可覆盖"性质较劲了。** 这种场景应该用队列。

### 15. 教材本章的缺陷（只记录）

| 位置 | 缺陷 | 后果 |
| --- | --- | --- |
| `14.2.4` 三行对照表第 2 行 | 掩码 `0110` 的文字写成"bit0, bit2"（应为 bit1, bit2），与第 1 行文字重复 | 读者会误判掩码与位的对应关系（§4） |
| `14.2.3` | 只说"给后台任务发队列数据"，未说明该函数**同时**依赖 `configUSE_TIMERS` 与 `INCLUDE_xTimerPendFunctionCall` | 在没有定时器的工程里链接失败，且报错位置在调用点而非配置处（§8） |
| `14.1.1` | 只讲 16/32 两档位宽；V11 已支持 64 位 tick（`eventEVENT_BITS_CONTROL_BYTES` 三档齐全） | 换到 64 位 tick 平台时"低 24 位"的结论失效 |
| `14.3` | car2、car3 都传 `xClearOnExit = pdTRUE`，未讨论"先醒者清位、后到者等不到"的边界 | 广播 + 清除的组合风险被隐藏（§5） |
| `15.2.3` | `eAction` 表里 `eSetBits` 未提示"任务通知没有 AND 等待" | 会以为它能完全替代事件组（§12） |

---

## 系统调用/接口卡片

### `EventGroupHandle_t xEventGroupCreate( void )`

作用：动态创建事件组，内部位清零。

参数：无。

返回值和错误：句柄；堆不足返回 `NULL`。

资源所有权：结构体由内核堆分配；`vEventGroupDelete` 释放。**删除时仍有任务在等**属于危险用法。

线程/进程安全：任务态创建；ISR 里不能创建。

最小示例（`F103_12/main.c:353`）：

```c
s_events = xEventGroupCreate();
if (s_events == NULL) { /* 堆不足 */ }
```

常见误用：忘记检查 `NULL`；以为"位宽"是独立的配置项（它由 `configUSE_16_BIT_TICKS` 决定）。

### `EventBits_t xEventGroupSetBits( EventGroupHandle_t eg, const EventBits_t bits )`

作用：把 `bits` 里为 1 的位在事件组中置 1，**并唤醒所有"非阻塞条件成立"的任务**。

参数：`bits` 可以多位，按位或上去。

返回值和错误：返回**设置前**的事件值。教材注："没什么意义，因为很可能已经被其他任务修改了"
（`rtos.txt:5140`）——它**不能**用来判断"我有没有置位成功"。

资源所有权：无。

线程/进程安全：任务态。ISR 用 `xEventGroupSetBitsFromISR`（本仓库不可用，§8）。

最小示例（`F103_12/main.c:233-235`，两次相隔 200 ms，专门用来验证"与"语义）：

```c
xEventGroupSetBits( s_events, EV_BIT_ARITH );
vTaskDelay( pdMS_TO_TICKS( 200U ) );
xEventGroupSetBits( s_events, EV_BIT_SENSOR );
```

常见误用：① 用返回值判断"是否置上了"；② 传了 bit24 及以上（属于控制字节，§1）。

### `EventBits_t xEventGroupWaitBits( eg, uxBitsToWaitFor, xClearOnExit, xWaitForAllBits, xTicksToWait )`

作用：等 `uxBitsToWaitFor` 里的位（全中或任一），可选在返回前清除。

参数：

| 参数 | 取值 | 语义 |
| --- | --- | --- |
| `xClearOnExit` | `pdTRUE` | 返回前清掉 `uxBitsToWaitFor` 指定的位（**原子**，§3） |
| `xWaitForAllBits` | `pdTRUE` | AND：全部为 1 才返回 |
| | `pdFALSE` | OR：任一为 1 即返回 |
| `xTicksToWait` | `0` / `portMAX_DELAY` / N | 与 Day04 的三态一致 |

返回值和错误：**返回事件值**（不是布尔！）。成功时是"非阻塞条件成立"时的值；
超时是超时时刻的值。**所以判断"等到没等到"不能只看返回值**——
本仓库的做法是看 `got` 与期望掩码：`got=0x03` 表示 bit0|bit1 都置上了。

资源所有权：无。

线程/进程安全：任务态可阻塞。

最小示例（`F103_12/main.c:135-141`）：

```c
EventBits_t got = xEventGroupWaitBits( s_events,
                                       EV_BIT_ARITH | EV_BIT_SENSOR,
                                       pdTRUE,      /* clear on exit */
                                       pdTRUE,      /* wait for all */
                                       pdMS_TO_TICKS( 2000U ) );
LOG("[sync] event wait_all got=0x%02lx waited=%lu ticks\n", (unsigned long)got, ...);
```

常见误用：① 用 `== pdTRUE` 判返回值（它是位图）；② 用 `xClearOnExit = pdFALSE` 却想让事件"只被消费一次"；
③ 等待掩码里包含保留位。

### `EventBits_t xEventGroupClearBits( eg, const EventBits_t bits )`

作用：清位。**注意：如果是为了"测试 + 清除"，应该用 `xClearOnExit`，不要调这个函数**（§3）。

返回值和错误：清除前的事件值。

线程/进程安全：任务态；ISR 版 `xEventGroupClearBitsFromISR` 本仓库不可用。

### `EventBits_t xEventGroupSync( eg, uxBitsToSet, uxBitsToWaitFor, xTicksToWait )`

作用：设置自己的位 + 等所有位全中 + **成功后自动清除**（AND 语义固定，无 all 参数）。

参数：`uxBitsToSet` "我完成了什么"；`uxBitsToWaitFor` "我要等哪些位全中"。

返回值和错误：成功时为条件成立时的事件值；超时为超时时刻的值。

线程/进程安全：任务态。

最小示例：

```c
/* 三个任务各自做完后调用；都到齐才一起返回 */
xEventGroupSync( eg, MY_BIT, ALL_BITS, portMAX_DELAY );
```

常见误用：把它当 `WaitBits` 用（它**会清除**，别人看不到你设的位）。

### `BaseType_t xEventGroupSetBitsFromISR( eg, bits, &pxHigherPriorityTaskWoken )`（本仓库**不可用**）

作用：ISR 里请求置位。**不是直接置位**，而是把请求排进定时器命令队列，
由 daemon task 执行（`event_groups.c:826`）。

参数：`pxHigherPriorityTaskWoken` 表示 **daemon task 优先级高于被中断任务**（不是业务任务，§7）。

返回值和错误：`pdPASS` = **请求已入队**，不代表位已置上；`pdFAIL` = 队列满或不可用。

前提：`configUSE_TIMERS == 1` **且** `INCLUDE_xTimerPendFunctionCall == 1`。
**本仓库两项都不满足**（§8），调用会链接失败。

### `BaseType_t xTaskNotifyGive( TaskHandle_t h )`

作用：`通知值 + 1`，状态置 pending。

参数：目标任务句柄（`xTaskCreate` 时传出）。

返回值和错误：教材说"必定返回 `pdPASS`"（`rtos.txt:5510`）。

资源所有权：数据存进**目标任务的 TCB**，发送方不再持有。

线程/进程安全：任务态；ISR 用 `vTaskNotifyGiveFromISR`。

最小示例：

```c
static TaskHandle_t s_worker;
xTaskCreate( worker, "w", 128, NULL, 2, &s_worker );   /* 句柄必须保存 */
...
xTaskNotifyGive( s_worker );
```

常见误用：① 忘了保存任务句柄（没有句柄就发不了通知）；② 以为能广播（只能一个）。

### `uint32_t ulTaskNotifyTake( BaseType_t xClearCountOnExit, TickType_t xTicksToWait )`

作用：等"通知值 > 0"，取出并清零或减一。

参数：`xClearCountOnExit` — `pdTRUE` 清零（二进制信号量）/ `pdFALSE` 减一（计数信号量）。

返回值和错误：**清零或减一之前**的值。超时返回 0。
所以 `pdTRUE` 模式下返回值恒为 1，拿不到"来了几次"。

最小示例：

```c
ulTaskNotifyTake( pdTRUE, portMAX_DELAY );   /* 等价于取二值信号量 */
```

### `BaseType_t xTaskNotify( h, ulValue, eAction )` / `BaseType_t xTaskNotifyWait( clearOnEntry, clearOnExit, &value, ticks )`

作用：专业版发送 / 取出。`eAction` 的五种取值见 §12；`xTaskNotifyWait` 的三个参数顺序见 §13。

返回值和错误：`xTaskNotify` 返回 `pdPASS`，**只有一种失败**：
`eSetValueWithoutOverwrite` 且目标为 pending ⇒ `pdFAIL`。
`xTaskNotifyWait` 返回 `pdPASS`（拿到）/ `pdFAIL`（超时）。

常见误用：① 传 `pulNotificationValue = NULL` 又想读值；② 在 `eSetValueWithoutOverwrite`
失败时不停重试（它不会因为等一会儿就成功，除非对方先取走）。

---

## 最小可运行实验

### 事件组（`F103_12`，实测）

```text
core (prio 3)  ──WaitBits(EV_BIT_ARITH|EV_BIT_SENSOR, clear=pdTRUE, all=pdTRUE, 2000 ms)──
feeder (prio 2) ──SetBits(EV_BIT_ARITH)── 200 ms ──SetBits(EV_BIT_SENSOR)──
core           ──WaitBits(EV_BIT_NEVER, clear=pdTRUE, all=pdTRUE, 300 ms)──   ← 无人置位，必超时
```

这个设计有两处值得学：

1. **特意把两次 `SetBits` 相隔 200 ms**，所以 `waited` 这个数值本身就是判据：
   若第一次置位就唤醒，`waited ≈ 0`；若两次都被当作一次，`waited ≈ 0`；只有
   "等两个都置位"才会得到 `waited ≈ 200`。
2. **`EV_BIT_NEVER` 这个位从来没有任务会去置**，专门用来走超时分支，
   验证"等不到要返回 0 而不是挂死"。

### 任务通知（本仓库**无实验**）

`grep -rn "xTaskNotify\|ulTaskNotifyTake" --include=*.c F103_*/main.c` **零命中**。
所以 §9–§14 的全部结论来自教材原文与源码阅读，**没有一条是本机跑出来的**。

---

## 编译与运行

```bash
cd experiments/F103_12_FreeRTOS_Sync
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake -DF103_QEMU=ON
cmake --build build

cd ../..
timeout 25 qemu-system-arm -M stm32vldiscovery -display none -monitor none \
  -serial stdio -kernel experiments/F103_12_FreeRTOS_Sync/build/f103_12_freertos_sync.elf \
  < /dev/null 2>&1 | tr -d '\r' | tee tests/qemu-stm32/logs/exp12_sync.log

# 只取事件组两行
grep -E "event (wait_all|timeout)" tests/qemu-stm32/logs/exp12_sync.log
```

---

## 结果观察

### 事件组（实测）

```text
[sync] event wait_all got=0x03 waited=200 ticks
[sync] event timeout  got=0x00 waited=300 ticks
```

逐条读：

| 读数 | 证明 |
| --- | --- |
| `got=0x03` | 两个位都置上了。返回值是**位图**，不是布尔——`0x03` = bit0\|bit1 |
| `waited=200` | **"与"语义生效**。两次 `SetBits` 相隔 200 ms；若第一次置位就唤醒，`waited` 会是 0 |
| `clear=pdTRUE` 下第二次仍能重新等待 | 位已被清掉，所以下一次等待从零开始（`EV_BIT_NEVER` 那条不受影响） |
| `event timeout got=0x00 waited=300` | 超时返回的是**超时时刻的事件值**（0），不是"失败"信号。`xTicksToWait=300` 与实测 `waited=300` 吻合 |

**判据要点**：`waited=200` 这个数只能由"等全部位"产生。如果同一段代码里
`xWaitForAllBits` 被误改成 `pdFALSE`，`waited` 会掉到接近 0，而 `got` 仍可能是 `0x03`——
**只看 `got` 判不出这个错误，必须同时看 `waited`。**

### 任务通知（**未实测**）

本仓库没有任何任务通知调用点。§10–§14 的三态、`eAction` 五种映射、
`xTaskNotifyWait` 的清除顺序，全部是**源码/教材核对**。
**不得写成"已验证"。**

### 本篇**没有**验证的东西

- 任务通知的实验**完全没有**：三态转换、`eSetValueWithoutOverwrite` 的 `pdFAIL`、
  `ulTaskNotifyTake(pdTRUE)` 与 `(pdFALSE)` 的差异、`xTaskNotifyWait` 的取值/清除顺序，一条读数都没有。
- **广播没有对照**：§5 说"一次 `SetBits` 唤醒多个任务"，本仓库只测了"等全部位"，
  没测"两个任务同时等、`SetBits` 一次唤醒几个"。这与 `Day04` 留下的同一条缺口是同一个。
- **`xEventGroupSync` 无实验**，其"自动清除"行为来自教材原文。
- **`xEventGroupSetBitsFromISR` 无法实验**——本仓库编译不出这个符号（§8）。
- 教材 14.3/14.4/14.5 三个 `nwatch` 工程本机没有，"car 同时/依次启动"的现象未复现。
- 64 位 tick 的 56 位可用位宽（`event_groups.h:139`）未验证，本仓库未开启该配置。

---

## 常见错误和陷阱

| 陷阱 | 症状 | 根因 | 处理 |
| --- | --- | --- | --- |
| 用 `== pdTRUE` 判 `xEventGroupWaitBits` 返回值 | 永远判不成"成功" | 返回的是**位图**，`0x03` 不等于 `1` | 与期望掩码做 `&`，或直接看 `got == mask` |
| 先 `WaitBits` 再 `ClearBits` | 偶发把没见过的事件吞掉 | 两步之间可被抢占（§3） | 用 `xClearOnExit = pdTRUE` |
| 传 bit24 及以上的掩码 | 位设不上，无报错 | 高 8 位是控制字节（§1） | 本仓库只用 bit0–bit23 |
| `xClearOnExit = pdFALSE` 却想要"只消费一次" | 第二个等待者又醒了一次 | 保留语义 = 广播，不是一次性 | 想一次性就清除；想广播就别清 |
| 以为 `SetBitsFromISR` 返回 `pdPASS` 就是置上了 | 中断返回后等待者没动 | 它是**异步**的，走 daemon 队列（§7） | 检查 `configUSE_TIMERS` 与 `INCLUDE_xTimerPendFunctionCall` |
| 在无定时器的工程里调 `xEventGroupSetBitsFromISR` | 链接失败：undefined reference | 条件编译把它整段去掉了（§8） | 两个宏都要开 |
| 用 `xEventGroupSync` 当公告板 | 别人看不到你设的位 | 它成功返回时**自动清除** | 要广播用 `WaitBits(clear=pdFALSE)` |
| 忘了保存 `xTaskCreate` 的句柄 | 无法发任务通知 | 通知必须点名接收者 | `xTaskCreate` 的最后一个参数传 `&handle` |
| 用任务通知替代队列做缓冲 | 中间的数据全丢 | 只有一个通知值，`eSetValueWithOverwrite` 直接覆盖 | 需要深度就换队列 |
| 用任务通知做"与"等待 | 判不出来 | `xTaskNotifyWait` 只判 pending，不判位组合（§12） | 用事件组 |
| 以为 `eSetValueWithoutOverwrite` 会重试 | 一直 `pdFAIL` | 对方不取走就永远 pending | 换成 `WithOverwrite`，或由调用方显式重发 |
| `ulTaskNotifyTake(pdTRUE)` 想数次数 | 总是得到 1 | `pdTRUE` = 清零语义 | 要次数用 `pdFALSE` 或计数信号量 |
| `xTaskNotifyWait` 传 `ULONG_MAX` 又拿不到值 | `*value` 是 0 | 赋值在清除之前——若顺序记反会以为被清了 | 记 §13 的顺序：先赋值、后清除 |

---

## 故障排查方法

1. **"事件组等不到。"** 四步查：① 掩码与置位用的位是不是同一批（§4 的教材印错就是这类）；
   ② `xWaitForAllBits` 是 `pdTRUE` 还是 `pdFALSE`；③ 有没有人用 `xClearOnExit = pdTRUE`
   提前把位清掉了；④ 返回值与掩码按位比，不要当布尔。
2. **"`SetBits` 之后等待者没醒，但返回值是成功。"** 先确认用的是不是 `...FromISR` 版本——
   ISR 版是**异步**的（§7），唤醒要等 daemon task。任务版不会这样。
3. **"编译报 `xEventGroupSetBitsFromISR` 未定义。"** 不是拼写错。查两个宏：
   `configUSE_TIMERS` 与 `INCLUDE_xTimerPendFunctionCall`（§8）。
4. **"优先级看起来没恢复/任务优先级乱跳。"** 事件组**不涉及**优先级继承；若看到优先级变化，
   去查是不是同时在用互斥量（Day07 §5 的 `DisinheritAfterTimeout`）。
5. **"任务通知丢了数据。"** 属性问题，不是 bug：只有一个通知值。要缓冲就换队列；
   要"最新值"就用 `eSetValueWithOverwrite` 并明确接受丢失。
6. **"`xTaskNotifyWait` 取到的值不对。"** 按 §13 的顺序自查：
   `ulBitsToClearOnEntry` 只在非 pending 时执行；赋值发生在 `ulBitsToClearOnExit` **之前**。
7. **"不知道选事件组还是任务通知。"** 按 Day04 §7 的第 2、3 问：
   要**多个任务**都收到 → 事件组；要**点名的单个任务**、且不在乎缓冲 → 任务通知。

---

## 练习题

### 基础题

1. `EventBits_t` 的实际类型是什么？为什么改 `configUSE_16_BIT_TICKS` 会改变事件组的可用位宽？
2. 本仓库能表达多少个事件位？第 24 位归属谁？写 `(1 << 24)` 会发生什么？
3. 事件组与队列/信号量的两处分歧是什么？"可以不清除"为什么是必要的（用"串口就绪"举例）？
4. `eAction` 的五个取值各等价于什么对象？哪个是唯一可能失败的？

### 改错题

5. 下面这段等待两位都置上，找出两处问题：

   ```c
   if (xEventGroupWaitBits(eg, BIT_A | BIT_B, pdTRUE, pdTRUE, portMAX_DELAY) == pdTRUE) {
       do_work();
   }
   ```

6. 下面这段在 ISR 里置位，说明为什么可能编译不过、以及即使编译过会发生什么：

   ```c
   void TIM2_IRQHandler(void) {
       BaseType_t woken = pdFALSE;
       xEventGroupSetBitsFromISR( g_eg, BIT_TICK, &woken );
       portYIELD_FROM_ISR( woken );
   }
   ```

7. 下面这段用任务通知做"积压计数"，说明为什么数据会丢：

   ```c
   /* 多个 ISR */ vTaskNotifyGiveFromISR( g_worker, &woken );
   /* 消费者  */
   for (;;) {
       xTaskNotifyWait(0, ULONG_MAX, &val, portMAX_DELAY);   /* 取最新值 */
       total += val;
   }
   ```

### 综合题

8. 用 `F103_12` 的骨架补一个**广播对照实验**：两个任务同时 `WaitBits` 同一位（一个 clear、一个不 clear），
   一次 `SetBits`。要求：① 预测谁醒谁不醒；② 给出你要打印的读数；③ 说明这个实验为什么必须
   等"两个任务都进入阻塞态"之后才能 `SetBits`（提示：Day04 综合题 8 是同一个坑）。

9. 把教材 14.4（或）与 14.5（与）的差异，改造成一个**可判定**的用例：
   同一个二进制，用 `-DWAIT_ALL=0/1` 编译两次，用"`waited` 的数值"作为判据。
   要求说明：为什么判据不能用 `got`。

10. 设计一个实验区分"任务通知的 `eSetValueWithoutOverwrite` 失败"与"发送方根本没发"。
    要求：① 采集哪两个量；② 说明为什么只看 `pdFAIL` 不够；
    ③ 如果目标永远不取走通知，这个 `pdFAIL` 会持续多久？

---

## 验收标准

- [ ] 能说出 `EventBits_t` 就是 `TickType_t`，并解释教材"基于效率"的说法与源码的差别。
- [ ] 能算出本仓库可用事件位是 bit0–bit23，并说明 bit24 以上为什么不行。
- [ ] 能用"串口就绪"解释为什么事件组不能是消耗型。
- [ ] 能推出"先 WaitBits 再 ClearBits"的两个错误分支，并说明 `xClearOnExit` 买到的是什么。
- [ ] 能指出教材 AND/OR 表第二行的掩码-文字错配，并说明照抄的后果。
- [ ] 能解释 `SetBitsFromISR` 的异步性，以及 `pdPASS` 的确切含义。
- [ ] 能说出本仓库编译不出 ISR 版事件组的两个宏，并各指出它们的位置。
- [ ] 能默写 `eAction` 五行的对象映射与唯一失败条件。
- [ ] 能说清 `ulTaskNotifyTake` 两个取值分别等价于哪种信号量。
- [ ] 能按顺序说出 `xTaskNotifyWait` 的入口清除、取值、出口清除三步。
- [ ] 能列出本篇**未验证**的六项及原因。

---

## 延伸阅读

- 教材第 14 章：`rtos.txt:5068-5395`
  - 14.1 概念与操作：5086–5135（整数模型、两处分歧）
  - 14.2 函数：5136–5288（创建/删除/置位/等待/清除/同步点）
  - 14.3–14.5 三个示例：5290–5385（广播、或、与）
- 教材第 15 章：`rtos.txt:5396-5680`
  - 15.1 特性与限制：5406–5455（五条限制、TCB 字段、三态）
  - 15.2 两套函数：5456–5630（简化版、专业版、`eAction` 表、`Wait` 参数表）
  - 15.3 示例：5631–5670（`27_tasknotification_car_game`）
- 源码：`Source/event_groups.c` — `xEventGroupSetBits`（唤醒所有）、`xEventGroupWaitBits`、
  `xEventGroupSync`、`xEventGroupSetBitsFromISR`（815，条件编译）、`xEventGroupClearBitsFromISR`（504）
- 源码：`Source/include/event_groups.h:46/51/56`（控制字节）、`:113`（`EventBits_t`）、
  `Source/include/FreeRTOS.h:300-301`（`INCLUDE_xTimerPendFunctionCall` 默认 0）
- 本仓库实验：`F103_12`（事件组"与" + 超时）
- 相关笔记：`Day04`（四问表第 3 问）、`Day05`（队列做不到广播）、`Day09`（定时器命令队列与 daemon task）、
  `Day14`（教材缺陷记账口径）
- 下一篇：`Day09_软件定时器.md`（教材第 16 章）

---

**验证记录**

| 日期 | 环境一 | 命令一 | 结果一 | 环境二 | 命令二 | 结果二 | 未覆盖 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 2026-10-30 | 教材抽取文本 | 读 `rtos.txt:5068-5680` | 第 14 章 14.1–14.5、第 15 章 15.1–15.3 全读；两处分歧（5104-5114）、位宽规则（5090-5099）、AND/OR 三行表（5220-5243，第 2 行文字错）、`ClearOnExit` 原子性（5257-5265）、同步点（5267-5285）、FromISR 走 daemon（5170-5178）、五条限制（5410-5428）、三态（5443-5449）、`eAction` 五种（5545-5580） | 本仓库源码 | `grep`/`sed` 读 `event_groups.h/.c`、`FreeRTOS.h`、`FreeRTOSConfig.h` | `EventBits_t = TickType_t`（`event_groups.h:113`）；控制字节三档（`:46/51/56`）；`SetBitsFromISR` 在 `configUSE_TIMERS && INCLUDE_xTimerPendFunctionCall` 之下（`event_groups.c:815`）；该宏默认 0（`FreeRTOS.h:300`）⇒ **本仓库不可用** | 64 位 tick 配置未验；`ClearBitsFromISR` 同样不可用 |
| 2026-10-30 | WSL2 + QEMU 10.2.1 | `qemu-system-arm -M stm32vldiscovery -kernel f103_12_freertos_sync.elf` | `event wait_all got=0x03 waited=200`；`event timeout got=0x00 waited=300`；两次 `SetBits` 相隔 200 ms ⇒ `waited=200` 证明 AND 语义 | 本仓库源码 | `grep -rn "xEventGroup\|xTaskNotify\|ulTaskNotifyTake" F103_*/main.c` | 事件组 5 处调用均在 `F103_12`；**任务通知零命中**；`configUSE_TASK_NOTIFICATIONS=1`（`:40`）、`ARRAY_ENTRIES=1`（`:41`） | 广播、`Sync`、任务通知全部无实验；教材三个 `nwatch` 工程本机无 |
