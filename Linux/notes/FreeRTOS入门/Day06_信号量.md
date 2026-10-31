# Day06：信号量（教材第 12 章）

**验证状态**：二值/计数信号量的阻塞、超时、**达到上限后 give 失败**全部**实测**
（`exp12_sync.log`：`bin waited=801/802`、`take2(0ms)=pdFALSE`、`bytes=7 gives=2 coalesced=5`）；
**优先级反转**两轮对照**实测**（`exp16_priority_inversion.log`：`binary-sem` H 等锁 405 vs
`mutex` 105 tick）；**未验证**：教材 12.3–12.5 的三个游戏工程（`19/20/21_semaphore_*`）
本机没有，本篇用 `F103_12`/`F103_16` 的同构实验替代；`xSemaphoreTakeFromISR` 本仓库未使用。

---

## 学习目标

学完本篇应能用代码证据回答：

1. "信号量"这个名字的两个字各指什么？哪一半决定了"计数型"与"二值型"的分界？
2. 信号量与队列的三条结构差异里，哪一条决定了 **give 永不阻塞**？
3. `xSemaphoreCreateBinary()` 创建出来的计数值是多少？为什么教材的示例里要连着 `give` 三次？
4. `vSemaphoreCreateBinary` 与 `xSemaphoreCreateBinary` 的初始值差在哪？哪个已过时？
5. 二值信号量的 give 在计数值已为 1 时返回什么？本仓库哪条读数证明了这一点？
6. 教材 12.5 的优先级反转为什么**必须**让中间任务用 `mdelay` 而不是 `vTaskDelay`？
7. 二值信号量能做互斥吗？能，但会付出什么代价？

---

## 环境与前置条件

| 项 | 版本 / 说明 |
| --- | --- |
| 教材 | 第 12 章信号量（`rtos.txt:4132-4927`），含 12.1 特性、12.2 函数、12.3–12.5 三个示例 |
| 内核 | FreeRTOS **V11.3.0**；信号量实现在 `Source/queue.c`（Day02 §1） |
| 实验 | `F103_12_FreeRTOS_Sync`（二值/计数/事件组/ISR 合并）、`F103_16_FreeRTOS_PriorityInversion` |
| 日志 | `tests/qemu-stm32/logs/exp12_sync.log`、`exp16_priority_inversion.log` |
| 前置 | `Day04`（同步 vs 互斥、四问表）、`Day05`（队列的拷贝语义与唤醒规则） |

---

## 核心概念

### 1. 名字拆开看：一半是"通知"，一半是"数量"

教材的拆字很直白（`rtos.txt:4143-4147`）：

- **信号**：起**通知**作用。
- **量**：表示**资源的数量**。

于是分类只剩一条判据：

| | 计数值范围 | 名字 |
| --- | --- | --- |
| 无限制 | `0 … uxMaxCount` | **计数型**（Counting） |
| 只有两个取值 | `0 / 1` | **二进制**（Binary） |

两个动作：**`give` 让计数 +1，`take` 让计数 −1**（`rtos.txt:4147`）。

教材接着给了计数型的两类典型场景（`rtos.txt:4149-4151`），它们的区别只在"谁负责加回来"：

| 场景 | 谁 give | 谁 take | 计数表达什么 |
| --- | --- | --- | --- |
| **计数事件** | 事件发生方（常是 ISR） | 处理方（任务） | 有多少个事件**还没处理** |
| **管理资源** | 用完资源的一方（常是**同一个**任务） | 访问资源的一方 | 还有多少个资源**空闲** |

这两行的差别看起来只是措辞，但它就是 Day04 §5 那个"同一对象两种用法"的落地：
**"谁 give"决定了这个信号量是"事件通知"还是"锁"。**
Day07 会看到，互斥量把"同一个任务"这条硬编码进了内核。

### 2. give 与 take 的双方不必相同——这是信号量能做生产者-消费者的原因

教材明确写了这一点（`rtos.txt:4153-4161`）：生产者 A、B，消费者 C、D，
初始计数 0。C/D 想获得有两种结果——**阻塞（可定闹钟）或即刻返回失败**；
A/B 生产时计数 +1 **并唤醒**等待的顾客。唤醒规则与队列一模一样：

```text
优先级最高的先醒；优先级相同则等得最久的先醒
```

（`rtos.txt:4160`，与 Day05 §3 的队列规则同源——因为信号量本来就是队列）。

**注意"初值 0"这个前提**：初值 0 时，第一个 `take` 必然失败。
这不是缺陷，而是"资源尚未生产"的正确表达。但它直接导致了 §3 那个坑。

### 3. 与队列的三条结构差异，第二条决定了 give 不阻塞

教材给了一张对比表（`rtos.txt:4166-4176`）：

| | 队列 | 信号量 |
| --- | --- | --- |
| 能装什么 | 可以容纳**多个数据** | **只有计数值**，无法容纳其他数据 |
| 创建时的内存 | 两部分：队列结构体 + **存储数据的空间** | **只分配信号量结构体** |
| **生产者满时** | **可以阻塞** | **不阻塞**，已达最大值时**返回失败** |
| 消费者空时 | 可以阻塞 | 可以阻塞 |

（原文第三行写作"用于不阻塞"，据上下文与 §12.2.3 的返回值说明判断，
语义是"**give 不会阻塞**"——教材在函数说明里明说"计数值已经是最大值，再次调用则返回失败"。）

**为什么 give 不阻塞？** 因为"给不进去"没有等待的意义：计数值已经到顶，
再等也不会有一天突然"有空间"——那个空间只能靠别人 `take` 腾出来，
而 `take` 是由消费者驱动的，不是由"时间流逝"驱动的。
生产者阻塞在这里只会白占一个任务，所以内核选择**立刻返回失败**。

这个设计有代价：**失败容易不被检查**。本仓库 `F103_12` 把它变成可观测的：

```text
bytes=7 gives=2 coalesced=5
```

读法：7 次 `give` 里只有 2 次真的把计数从 0 加到 1，**5 次返回失败**
（因为二值信号量上限 1）。`coalesced`（合并）这个措辞是准确的：
这 5 个事件不是"丢了数据"，而是**被合并进同一个"有事件待处理"的状态里**。

**所以二值信号量表达的是"有没有事件"，不是"有几个事件"。** 要数清个数必须换计数信号量。

### 4. 二值信号量的初值是 0——这是最容易踩的一脚

教材的两处原文必须一起读：

- `xSemaphoreCreateBinary()`："计数值初始值为 **0**"（`rtos.txt:4198`）
- `vSemaphoreCreateBinary()`："计数值初始值为 **1**"，并标注"**过时了**"（`rtos.txt:4199`）

两个名字只差一个字母，初始值相反。用错的表现是：

- 用 `xSemaphoreCreateBinary()` 却按"创建即有资源"写代码 → 第一次 `take` 立刻超时或永久阻塞。
- 用旧的 `vSemaphoreCreateBinary()` 移植到新版内核 → 编译告警/失败，或者删掉它之后忘了补 `give`。

教材的示例 20 里，作者写了三行：

```c
g_xSemTicks = xSemaphoreCreateBinary();
xSemaphoreGive(g_xSemTicks);   /* 对于二进制信号量,它的最大值是1,后面两次give无效 */
xSemaphoreGive(g_xSemTicks);
xSemaphoreGive(g_xSemTicks);
```

**后两行是故意留的反例**，用来演示"上限 1"：三行只等于一次 give。
这种写法在示例里是好的教学，在工程代码里是**掩盖初始值问题**——
正确做法是创建后**恰好** give 一次，并在注释里写清"为什么这一次是必须的"。

### 5. 两种信号量的差异只有两处

教材的对照（`rtos.txt:4180-4188`）：

| | 二进制 | 计数型 |
| --- | --- | --- |
| 创建时初始值 | **固定为 0** | **可以设定** |
| 其他操作 | 一样 | 一样 |

第二行很重要：`give`/`take`/`FromISR` 函数完全共用，**所以二者可以在不改调用点的情况下互换**。
这既是优点（便于改成"多资源"）也是陷阱（见 §7：互斥量看起来也只差一行，但语义完全不同）。

计数型的创建带两个参数（`rtos.txt:4220`）：`xSemaphoreCreateCounting( uxMaxCount, uxInitialCount )`。
教材示例 19 用的是 `(3, 2)`——**最大值 3、初始 2**，于是"三辆车两张通行证"：

```text
car1、car2 一起往右行驶，任何一辆到达右边后 car3 才开始往右行驶
```

这个现象值得对一下账：初始 2 ⇒ 头两辆各 take 一次成功 ⇒ 第三辆阻塞 ⇒
第一辆到终点 give ⇒ 第三辆被唤醒。**"一起"和"之后"这两个词的位置就是判据。**

### 6. 教材 12.5 的优先级反转：关键不在优先级，在中间任务用了 `mdelay`

教材的场景（`rtos.txt:4674-4684`）：

| 任务 | 优先级 | 需不需要信号量 | 延迟方式 |
| --- | --- | --- | --- |
| `car1` | 最低（`osPriorityNormal`） | **需要**，且先拿到了 | 无 |
| `car2` | 中（`+2`） | **不需要** | **`mdelay(50)` 忙等** |
| `car3` | 最高（`+3`） | 需要 | `vTaskDelay(2000)` 先睡 2 s |

链条：`car1` 持锁 → `car2` 就绪且优先级更高，**而且从不阻塞** → `car1` 永远得不到 CPU →
锁永远不释放 → `car3` 永远阻塞。教材的比喻是"小职员拿钥匙、中层领导寸步不让、大老板干等"。

**为什么必须用 `mdelay`？** 教材自己说了（`rtos.txt:4720-4722`）：

> 在车子运行过程中，我们 `R_BSP_SoftwareDelay` 来延时，而不使用 `vTaskDelay`，
> 就是让 `car2_task` 不阻塞，使得 `car1_task` 无法运行。

如果用 `vTaskDelay`，`car2` 会阻塞，`car1` 就有机会跑完并释放锁——**反转就不发生了**。
所以这条实验的成立条件是一条**负向判据**：

> 反转的必要条件不是"优先级有高低"，而是"中间任务在持锁者等待期间**一直占着 CPU**"。

教材还给了一个更强化的观察：

> 如果修改 `car2_task` 的代码，把第 193 行的 `vTaskDelete(NULL)` 去掉，
> 那么即使 `car2` 运行到了终点，`car1` 和 `car3` 也不能运行。（`rtos.txt:4770-4772`）

去掉自杀 ⇒ `car2` 死循环且不阻塞 ⇒ **永久饥饿**。这是"忙等任务必须有终止条件"的反面教材。

### 7. 二值信号量能做互斥，但代价是"没有继承"

教材这一章用信号量做的是"通行证"（资源计数），本质上就是互斥。它能用，但有两个代价：

1. **`give` 不检查持有者**。任何任务都能 `give`，包括从未 `take` 过的任务。
   于是"锁"可以被第三方打开——Day04 §5 的第 4 问在这里失效。
2. **没有优先级继承**。上面那条第 6 节的链条就完整发生了。

本仓库 `F103_16` 把第 2 条做成了两轮对照（同一骨架，只换锁类型）：

| 轮次 | 高优先级任务等锁 | 低优先级任务持锁 | 低优先级在阻塞期的优先级 |
| --- | --- | --- | --- |
| `binary-sem` | **405 tick** | 404 tick | **恒为 1**（不变） |
| `mutex` | **105 tick** | 105 tick | **1 → 3 → give 后回 1** |

**约 3.9 倍**（另一次运行 412/410 vs 103/104，比值 3.86–4.0；跨运行漂移已写在实验 README）。
判据是 `binary > 2 × mutex` 这条宽松算术——正因为跨运行有漂移，判据不能写死具体 tick 数。

这就把 Day07 的动机钉死了：**互斥量唯一多做的事就是"知道锁是谁的"，从而能继承。**
二值信号量不是"差一点的互斥量"，它是"另一种东西被拿来当锁用"。

### 8. 三类"计数"别混：上限、初值、失败语义

学完这一章最容易记混的是三个数。压成一张表：

| 对象 | 创建时的上限 | 创建时的初值 | give 到顶时的行为 |
| --- | --- | --- | --- |
| 计数信号量 `(max, init)` | 自己指定 | 自己指定 | 返回失败（**不阻塞**） |
| 二值信号量 `xSemaphoreCreateBinary()` | **1** | **0** | 返回失败 |
| 旧宏 `vSemaphoreCreateBinary()` | 1 | **1** | 返回失败 |

再对照 Day05 的队列：**队列满时 `xQueueSend` 会阻塞**。同一件事（"给不进去"），
两种对象给两种答案——这就是"选错对象"最常见的后果：
用队列做"事件通知"，生产者在满时会阻塞（把发送方拖住）；用信号量做"事件流"，
事件会被合并（`coalesced=5`，丢计数）。

---

## 系统调用/接口卡片

### `SemaphoreHandle_t xSemaphoreCreateBinary( void )`

作用：创建二值信号量，**计数值初始为 0**。

参数：无。

返回值和错误：句柄；失败（堆不足）返回 `NULL`。

资源所有权：只需信号量结构体（**不需要数据存储区**，这是与队列的内存差异）；
`vSemaphoreDelete` 回收，且只对动态创建的有效。

线程/进程安全：任务态创建；ISR 里不能创建。

最小示例：

```c
s_sem = xSemaphoreCreateBinary();
if (s_sem == NULL) { /* 堆不足 */ }
xSemaphoreGive( s_sem );   /* 必须补这一次：初值是 0，不补则第一次 take 必然失败 */
```

常见误用：① 忘了创建后补一次 `give`；② 以为 `give` 三次就等于计数 3（上限 1，后两次返回失败）。

### `SemaphoreHandle_t xSemaphoreCreateCounting( UBaseType_t uxMaxCount, UBaseType_t uxInitialCount )`

作用：创建计数信号量。

参数：`uxMaxCount` 上限；`uxInitialCount` 初值（`0 ≤ init ≤ max`）。

返回值和错误：句柄或 `NULL`。

资源所有权：同二值。

线程/进程安全：任务态。

最小示例（教材 19 号示例的写法，`rtos.txt:4421`）：

```c
/* 三辆车、两张通行证 */
g_xSemTicks = xSemaphoreCreateCounting( 3, 2 );
```

常见误用：① 把两个参数顺序写反（`(2, 3)` 会被 `configASSERT` 拦下，但在关闭断言的发布版里会变成上限 2 初值 3 的未定义初始化）；
② 用 `init = max` 表示"资源全在"，随后却发现 `take` 数量对不上（每 take 一次就少一个，账要对得上）。

### `BaseType_t xSemaphoreGive( SemaphoreHandle_t s )`

作用：计数值 +1；若有任务在等，唤醒一个（**优先级最高 / 同等则等最久**）。

参数：句柄。

返回值和错误：`pdTRUE` 成功；`pdFALSE` = **计数值已达上限**（二值型即已为 1，计数型即已为 max）。
**这是本 API 唯一会失败的原因，而且它是正常业务状态，不是错误。**

资源所有权：无。

线程/进程安全：任务态。ISR 用 `xSemaphoreGiveFromISR`。

最小示例：

```c
if (xSemaphoreGive( s_sem ) != pdTRUE) { coalesced++; }   /* 正常：已经有事件待处理 */
```

常见误用：① 把 `pdFALSE` 当致命错误去 `printf` 刷屏（它是常态）；
② 不计数就静默忽略——那样"事件被合并了多少次"就永远不知道了。

### `BaseType_t xSemaphoreTake( SemaphoreHandle_t s, TickType_t ticks )`

作用：计数值 −1；为 0 时按 `ticks` 阻塞。

参数：`ticks` 三态与队列一致——`0` 不阻塞 / `portMAX_DELAY` 一直等 / 其它为上限。

返回值和错误：`pdTRUE` 成功；`pdFALSE` = 超时。
**不会"失败于上限"**——take 没有上限问题。

资源所有权：无。

线程/进程安全：任务态可阻塞；ISR 用 `xSemaphoreTakeFromISR`（且 ISR 里 take 是罕见用法，
通常应该由任务 take）。

最小示例（本仓库 `F103_12`）：

```c
if (xSemaphoreTake( s_bin, pdMS_TO_TICKS(800) ) == pdTRUE) { /* 等到 */ }
else { /* 超时：走降级路径 */ }

/* 超时传 0 = 只试一次 */
if (xSemaphoreTake( s_bin, 0 ) != pdTRUE) { /* 现在没有 */ }   /* 实测：take2(0ms)=pdFALSE */
```

常见误用：① 把 `0` 当"永久等待"；② 超时分支什么都不做，等于把阻塞又变回死等。

### `BaseType_t xSemaphoreGiveFromISR( SemaphoreHandle_t s, BaseType_t *pxWoken )`

作用：ISR 里 give。

参数：`pxWoken` 输出"是否唤醒了更高优先级任务"，**必须**接着调 `portYIELD_FROM_ISR`。

返回值和错误：`pdTRUE` / `pdFALSE`（已达上限）。

线程/进程安全：仅 ISR。

常见误用：漏 `portYIELD_FROM_ISR`（唤醒延迟一个 tick）；不检查 `pdFALSE`（丢合并计数）。

### `void vSemaphoreDelete( SemaphoreHandle_t s )`

作用：删除动态创建的信号量。

返回值和错误：`void`，无失败路径。

资源所有权：释放结构体内存。**在仍有任务阻塞于该信号量时删除是危险的**
（被阻塞者的等待对象消失）——工程上应在删除前排空或确认无人等待。

---

## 最小可运行实验

本篇不新写实验，用两个已有实验覆盖教材三节：

| 教材小节 | 替代实验 | 对应读数 |
| --- | --- | --- |
| 12.3 计数型（三车两证） | `F103_12` 的计数信号量段 | `bin waited=801/802`（阻塞→唤醒的时长） |
| 12.4 二值（三车一证）+ 上限 1 | `F103_12` 的 ISR 合并段 | `bytes=7 gives=2 coalesced=5` |
| 12.5 优先级反转 | `F103_16` 两轮对照 | `binary-sem` 405 vs `mutex` 105 tick |
| 超时三态 | `F103_12` | `take2(0ms)=pdFALSE`、`waited=200`/`300` |

**教材的三个游戏工程本机没有**（依赖 `nwatch` 的 LCD/红外驱动），
所以"三辆车一起/依次行驶"这类**现象**无法复现；能复现的是同一机制下的**读数**。

---

## 编译与运行

```bash
# 信号量语义
cd experiments/F103_12_FreeRTOS_Sync
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake -DF103_QEMU=ON
cmake --build build
cd ../..
timeout 25 qemu-system-arm -M stm32vldiscovery -display none -monitor none \
  -serial stdio -kernel experiments/F103_12_FreeRTOS_Sync/build/f103_12_freertos_sync.elf \
  < /dev/null 2>&1 | tr -d '\r' | tee tests/qemu-stm32/logs/exp12_sync.log

# 优先级反转两轮对照
cd experiments/F103_16_FreeRTOS_PriorityInversion && \
  cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake -DF103_QEMU=ON && \
  cmake --build build
```

---

## 结果观察

### 信号量语义（实测）

```text
[bin]   waited=801/802               ← take 阻塞到被唤醒，等待时长可测
[take2] take2(0ms)=pdFALSE           ← 超时 0：试一次就返回
[isr]   bytes=7 gives=2 coalesced=5  ← 7 次 give 只有 2 次成功，5 次因达上限返回 pdFALSE
```

第三条是本篇的核心读数，它同时证明了教科书上的两条：

1. **二值信号量上限为 1** —— `gives` 停在 2（初始 0，第一次 give 成功变 1，第二次之后失败）。
   注意 `gives=2` 说明第一次 give 也被计数了，即**创建后初值确实是 0**（§4）。
2. **give 失败是正常业务状态** —— `coalesced=5` 不是错误计数，而是"这 5 个事件被合并了"。

### 优先级反转（实测，两轮只换锁类型）

```text
binary-sem : H 等锁 405 / L 持锁 404 / L 阶段一 354 / 阻塞期 prio 恒 1
mutex      : H 等锁 105 / L 持锁 105 / L 阶段一  52 / prio 1 → 3（give 内部回 1）
```

比值 ≈ **3.9×**。要点不在数字，而在**唯一改变的变量是锁类型**：
骨架、任务数、优先级、延时全部相同。所以"3.9× 的差"只能归因于优先级继承。

跨运行漂移（另一次 412/410 vs 103/104）已如实记录在实验 README——
这正是判定器只写宽松算术 `binary > 2×mutex` 的原因。

### 本篇**没有**验证的东西

- 教材 `19/20/21_semaphore_*` 三个工程**本机没有**，它们的"三车一起/依次行驶"现象未复现。
- `xSemaphoreTakeFromISR` 本仓库**未使用**，其 `pxWoken` 语义未实测。
- `vSemaphoreCreateBinary`（过时宏）在当前内核版本下是否还可用、是否只是被重定义为
  "create + give"，**未查证**（源码未读）。

---

## 常见错误和陷阱

| 陷阱 | 症状 | 根因 | 处理 |
| --- | --- | --- | --- |
| `CreateBinary` 后不 `give` | 第一次 `take` 必然超时/永久阻塞 | 初值是 **0**（不是 1） | 创建后补一次 `give`，并注释原因 |
| 用 `vSemaphoreCreateBinary` 的旧习惯 | 以为创建即有资源 | 旧宏初值 1，新函数 0 | 统一用新函数；移植时专门查这一处 |
| 用二值信号量数事件次数 | 计数总是偏小 | 上限 1，多余 give 返回 `pdFALSE`（`coalesced=5`） | 换计数信号量；并统计 `coalesced` |
| 把 `give` 的 `pdFALSE` 当错误 | 日志被"失败"刷屏 | 达上限是正常状态 | 命名成 `coalesced`/`already_set` 而不是 `error` |
| 计数信号量参数写反 | 上限/初值错位，`configASSERT` 才拦得住 | `(max, init)` 顺序 | 按名字写，别按位置记 |
| 用二值信号量做互斥 | 高优先级任务被拖数百 tick | **无优先级继承**（`F103_16`：405 vs 105） | 用互斥量（Day07） |
| 用信号量做互斥却让第三方 `give` | 锁被外部打开，继承失效 | `give` 不检查持有者 | 只让持有者释放；或直接用互斥量 |
| 中间任务用忙等延时 | 持锁者被永久饿死 | 忙等不阻塞，占满 CPU（教材 12.5 的 `mdelay`） | 忙等必须可终止；或改 `vTaskDelay` |
| 超时传 `0` 以为永久等 | 一次拿不到就失败 | `0` = 不阻塞 | 永久等待用 `portMAX_DELAY` |
| 删除仍有等待者的信号量 | 等待任务行为未定义 | 删除时未排空 | 删除前确认无人阻塞 |

---

## 故障排查方法

1. **"`take` 一直超时。"** 三步：① 创建的是 `xSemaphoreCreateBinary()` 吗 → 检查有没有补 `give`；
   ② 计数信号量的初值是不是 0；③ 有没有别的任务把资源 take 走了（打印计数或用 `uxSemaphoreGetCount`）。
2. **"事件次数少了。"** 统计 `give` 的返回值。`pdFALSE` 次数 = 被合并次数。
   若合并次数可观，说明**二值信号量选错了**，要换计数型或队列。
3. **"用信号量做锁，偶尔高优先级任务卡住几百 tick。"** 这是优先级反转的典型形态。
   先确认中间任务有没有忙等；再确认锁是二值信号量还是互斥量（`F103_16` 的两轮读数就是判据）。
4. **"`give` 一直返回 `pdFALSE`，但看起来该有事件。"** 计数值还停在 1——
   说明**没有任务在 take**。先查消费者是否卡在别处，不要怀疑 `give` 本身。
5. **"计数信号量的账对不上。"** 列出所有 `give` 与 `take` 调用点，检查是否有一侧在错误路径上漏了调用
   （典型：`take` 成功但后续分支 `return` 前忘了 `give`）。

---

## 练习题

### 基础题

1. 用"谁 give"区分"计数事件"与"管理资源"两种用法，各举一个本仓库或你项目里的例子。
2. `xSemaphoreCreateCounting(3, 2)` 的两个参数分别是什么？说明教材 19 号示例为什么"两辆一起、第三辆之后"。
3. 二值信号量的上限、初值、`give` 到顶的行为，三者各是什么？与 `xQueueSend` 满时的行为对比。

### 改错题

4. 下面这段在多个中断里通知同一个任务，指出问题及两种改法：

   ```c
   void ISR_A(void) { BaseType_t w = pdFALSE; xSemaphoreGiveFromISR(s, &w); portYIELD_FROM_ISR(w); }
   void ISR_B(void) { BaseType_t w = pdFALSE; xSemaphoreGiveFromISR(s, &w); portYIELD_FROM_ISR(w); }
   ```

5. 下面这段初始化二值信号量，说明为什么第一次 `take` 会失败：

   ```c
   s = xSemaphoreCreateBinary();
   /* 期望：创建即有资源 */
   if (xSemaphoreTake(s, 0) != pdTRUE) { printf("err\n"); }
   ```

6. 下面这段用信号量保护 LCD，指出两处设计问题：

   ```c
   void lcd_print(const char *s) {
       xSemaphoreTake(g_lcd, portMAX_DELAY);
       hal_lcd_write(s);                       /* 耗时 20 ms */
       vTaskDelay(pdMS_TO_TICKS(10));          /* 让出 */
       xSemaphoreGive(g_lcd);
   }
   ```

### 综合题

7. 用 `F103_16` 的两轮读数（405 vs 105）回答：如果把中间任务的 `mdelay` 换成 `vTaskDelay(1)`，
   你预测两轮的差值会怎么变？实际跑一次验证，并解释差值变化的方向为什么是那个方向。

8. 设计一个实验，区分"二值信号量合并了事件"与"消费者处理太慢"这两种"计数偏小"。
   要求说明：① 你需要采集哪两个量；② 如何从它们的比值判断是哪一种；③ 判据不能只看 `give` 返回值的理由。

9. 把教材 12.5 的反转链条改造成一个**可判定**的测试：要求① 有一个"反转发生"的读数（不只是现象描述）；
   ② 有一个"关掉继承就必然失败"的对照；③ 说明为什么判据不能写死具体 tick 数。

---

## 验收标准

- [ ] 能拆开"信号量"两个字，并指出哪一半决定二值/计数的分类。
- [ ] 能解释为什么 `give` 永不阻塞，而队列的 `xQueueSend` 会阻塞。
- [ ] 能说出 `xSemaphoreCreateBinary()` 的初值是 0，并解释教材为什么连写三次 `give`。
- [ ] 能区分 `vSemaphoreCreateBinary` 与 `xSemaphoreCreateBinary` 的初始值差异。
- [ ] 能用 `bytes=7 gives=2 coalesced=5` 解释"二值信号量表达有没有，不表达有几个"。
- [ ] 能说清教材 12.5 为什么必须用 `mdelay`，以及换 `vTaskDelay` 后反转消失的原因。
- [ ] 能引用 `F103_16` 的 405 vs 105 说明"二值信号量做锁缺什么"，并指出这指向 Day07。
- [ ] 能列出本篇**未验证**的三项（教材三个工程、`TakeFromISR`、旧宏现状）及原因。

---

## 延伸阅读

- 教材第 12 章：`rtos.txt:4132-4927`
  - 12.1 特性：4143–4188（拆字、两类场景、与队列对比、两种信号量对比）
  - 12.2 函数：4189–4270（创建/删除/give/take 四对）
  - 12.3–12.5 示例：4271–4927（计数型、二进制、优先级反转）
- 源码：`vendor/freertos/Source/queue.c`（`xQueueGenericCreate` 被信号量复用、
  `uxSemaphoreGetCount`、`configASSERT` 对 `uxInitialCount <= uxMaxCount` 的检查）
- 本仓库实验：`F103_12`（语义）、`F103_16`（反转与继承）
- 相关笔记：`Day04`（四种方法的四问）、`Day05`（队列）、`Day07`（互斥量与继承）
- 下一篇：`Day07_互斥量.md`（教材第 13 章）

---

**验证记录**

| 日期 | 环境一 | 命令一 | 结果一 | 环境二 | 命令二 | 结果二 | 未覆盖 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 2026-10-30 | 教材抽取文本 | 读 `rtos.txt:4132-4927` | 12.1–12.5 全章；拆字与两类场景（4143-4151）、与队列四行对比（4166-4176）、两种信号量仅两处差异（4180-4188）、`CreateBinary` 初值 0 / 旧宏初值 1（4198-4199）、示例 20 的三次 give（4740-4744）、12.5 反转链条与"必须用 mdelay"的理由（4674-4772） | WSL2 + QEMU 10.2.1 | `qemu-system-arm -M stm32vldiscovery -kernel f103_12_freertos_sync.elf` | `bin waited=801/802`、`take2(0ms)=pdFALSE`、`bytes=7 gives=2 coalesced=5` | 教材 `19/20/21_semaphore_*` 三个工程本机无；`xSemaphoreTakeFromISR` 未使用；旧宏 `vSemaphoreCreateBinary` 现状未查证 |
| 2026-10-30 | 本仓库源码 + 实验 | `F103_16` 两轮对照（只换锁类型） | `binary-sem` H 等锁 405 / L 持锁 404 / L 阶段一 354 / 阻塞期 prio 恒 1；`mutex` 105 / 105 / 52 / prio 1→3→give 后回 1；比值 3.9×（另一次 412/410 vs 103/104）；栈水位 82/192、58/160、64/160 全过 | 同上 | `qemu-system-arm ... -kernel f103_16_priority_inversion.elf` | 判定器只判 `binary > 2×mutex` 一条宽松算术；4 条负向验证按预期失败 | 死锁、继承链长度、中断延迟、临界区长度未验证 |
