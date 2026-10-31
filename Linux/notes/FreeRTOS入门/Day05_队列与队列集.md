# Day05：队列与队列集（教材第 11 章）

**验证状态**：队列的创建/发送/阻塞/计数**实测**（`exp10_freertos.log`：`produced=consumed=55`、
`dropped=0`、`queued=0`）；**ISR 侧发送与"满就丢"实测**（`exp11_freertos_isr_queue.log`：
`rx 0→7→14`、突发 300 B 触发 `trunc=4` 且 `300 = 4×64 + 44`）；
**队列集完全没有实验**——本仓库未使用 `xQueueCreateSet`，`xQueueAddToSet`/`xQueueSelectFromSet`
的结论全部来自教材原文与源码阅读。`xQueuePeek`/`xQueueOverwrite`/`xQueueSendToFront`
同样只有源码核对，没有实测。

---

## 学习目标

学完本篇应能用代码证据回答：

1. 队列的"长度"和"数据大小"在什么时候确定？为什么这两项必须在创建时给出？
2. FreeRTOS 用的是**拷贝**还是**引用**？教材给了 6 条理由，哪一条是**架构性**的（换不掉）？
3. 多个任务等同一个空队列，数据来了**唤醒谁**？这个规则为什么注定队列实现不了"公平轮转"？
4. 任务侧写满会阻塞，ISR 侧写满会怎样？本仓库哪条读数证明了这一点？
5. `xQueueOverwrite` 为什么要求队列长度必须为 1？它和 `xQueueSend` 的语义差在哪？
6. 队列集的长度为什么要等于**各成员队列长度之和**，而不是最大值？
7. 队列集解决的是"多个队列怎么一起等"，那么它**没有**解决什么？

---

## 环境与前置条件

| 项 | 版本 / 说明 |
| --- | --- |
| 教材 | 第 11 章队列（`rtos.txt:3491-4131`），含 11.1 特性、11.2 函数、11.3–11.7 五个示例工程 |
| 内核 | FreeRTOS **V11.3.0**；队列实现在 `Source/queue.c` |
| 实验 | `F103_10_FreeRTOS_Tasks`（任务↔任务，深度 8）、`F103_11_FreeRTOS_ISR_Queue`（ISR→任务） |
| 日志 | `tests/qemu-stm32/logs/exp10_freertos.log`、`exp11_freertos_isr_queue.log` |
| 前置 | `Day03`（任务状态与超时）、`Day04`（四问与五对象对照表） |

---

## 核心概念

### 1. 五个特性里，前三个是"创建时一次性决定"

教材的简化操作列出（`rtos.txt:3498-3504`）：

| 特性 | 含义 | 什么时候定 |
| --- | --- | --- |
| **长度（length）** | 能存多少个 item | **创建时** |
| **每个数据大小固定** | 同一队列里所有 item 一样大 | **创建时** |
| **FIFO** | 写尾、读头 | 运行期 |
| 可强制写头部 | `xQueueSendToFront` 插队 | 运行期 |
| 可覆盖头部 | 长度必须为 1 | 运行期 + 创建时的约束 |

前两项在创建时定，是因为**队列的内存在 `xQueueCreate` 里一次性分配完**：
`uxQueueLength × uxItemSize` 加上结构体开销，直接从 `pvPortMalloc` 拿。
这也解释了 `F103_10` 的堆账：队列深度 8 一建好，那部分 RAM 就再也回不来了
（`heap_free` 从 4096 掉到 1112 B，队列是其中一项，见 Day02 §6）。

**队列不能"长"也不能"换元素类型"。** 要传不同大小的东西，唯一办法是：
① 用足够大的联合体/结构体；② 传指针（回到 §2 的引用方案，自己管生命周期）；
③ 拆成两个队列。这三条各有代价，没有免费的第四条。

### 2. 拷贝 vs 引用：6 条理由里只有一条换不掉

教材明确 FreeRTOS 用**拷贝值**，并给了 6 条好处（`rtos.txt:3512-3520`）。
逐条看它们的可替代性：

| 理由 | 能靠"用引用"绕过去吗 |
| --- | --- |
| 局部变量的值可发送，函数退出回收不影响 | 不能——引用会立刻悬空 |
| 无需额外分配 buffer | 不能——引用要自己分配并管理 |
| 局部变量可马上再次使用 | 不能——同第一条 |
| 发送/接收解耦：接收者不必知道数据是谁的，也不需发送者释放 | **不能，这是架构性的** |
| 数据太大可用地址 | 可以（教材自己也说"实在太大"就用地址） |
| 队列空间由内核分配 | 不能——引用方案里这块内存归谁管说不清 |
| MPU 系统下引用需双方都有访问权限，拷贝则无此限制 | 不能——这是硬件语义 |

**只有一条是"可以商量"的**：数据太大时用地址（`rtos.txt:3518`）。
其余五条的共同点是**所有权**：拷贝之后，队列里的数据归内核，
不受发送方的栈帧、生命周期、权限域影响。

这直接推出一条工程判据：

> **能用拷贝就别用指针。** 一旦传指针，你就得自己回答"谁分配、谁释放、什么时候释放、
> 发送方在接收方处理完之前不能改动它"——这四个问题本来正是队列替你解决的。

本仓库两个实验都传值（`F103_11` 传一行文本，`F103_10` 传采样结构体），没有一处传指针。

### 3. 阻塞的两侧对称，但唤醒规则只有一个

读队列（`rtos.txt:3538-3546`）：队列空 → 可阻塞 + 超时；有数据 → 被唤醒；一直没数据 → 超时后也可就绪。
写队列（`rtos.txt:3548-3556`）：队列满 → 可阻塞 + 超时；有空间 → 被唤醒；一直没空间 → 超时后也可就绪。

**读写都对称地支持阻塞**，这是队列与信号量最大的结构差异之一——
信号量只有"取不到"会阻塞，"给"从不阻塞（因为给是往上加计数）。

唤醒规则（教材写了两遍，`rtos.txt:3542-3544` 与 `3551-3553`）：

```text
多个任务等待同一个队列 → 唤醒 优先级最高的
                       优先级相同 → 唤醒 等待时间最久的
```

**这条规则决定了队列实现不了公平轮转。** 一个高优先级任务永远排在低优先级之前，
即使低优先级已经等了一万年。所以：

- 需要公平：把两个任务设成**同一优先级**（这时才轮到"等最久的先来"，即 FIFO 等待队列）。
- 需要绝对响应：保持优先级差，接受低优先级可能饿死。

这也是 Day03 §7 那条"关掉抢占后优先级失去意义"的另一面：
**优先级的实际后果全在这些等待队列的排序里**。

### 4. ISR 侧没有"阻塞"，所以满就是丢

`xQueueSendToBackFromISR` 的签名里**没有 `xTicksToWait`**（`rtos.txt:3592-3600`），
只有 `pxHigherPriorityTaskWoken`。这不是省略——中断里不能阻塞，
所以"队列满"在 ISR 里只能**立刻失败**。

本仓库 `F103_11` 把这条做成了可观测的（`exp11_freertos_isr_queue.log`）：

```text
rx 0 → 7 → 14
突发 300 B → trunc=4
300 = 4 × 64 + 44
```

读法：队列深度 64 项，一次灌 300 字节时 **4 次写入失败**（`trunc=4`），
其余 260 个字节进队。`300 = 4×64 + 44` 这个等式是自洽性校验——
它说明"丢的正是队列满之后的那些"，不是随机丢。

**判据要点**：`trunc` 必须计数并打印。不计数的话，症状是"偶尔少一行"，
而"少一行"在串口日志里几乎无法与"主机没发"区分开。

同时注意第三个参数：`pxHigherPriorityTaskWoken` 必须传给 `portYIELD_FROM_ISR()`，
否则"高优先级任务被唤醒"这件事要等到下一个 tick 才生效——
这就是 Day03 §2 里 `xTaskResumeFromISR` 那条"忽略返回值 = 延迟生效"的同构问题。

### 5. `xQueueSend` 与 `xQueueSendToFront` 不是 LIFO

教材把 `xQueueSend` 说明为"等同于 `xQueueSendToBack`"（`rtos.txt:3560-3568`）。
`ToFront` 是**插队到头部**，于是下一次 `xQueueReceive` 先拿到它。

但这**不构成后进先出**：插队之后，队列其余部分仍然是 FIFO。
想真正做 LIFO，队列做不到——要么用两个队列互相倒，要么干脆用数组+索引自己管。
教材把它列出，是因为它解决了"这条消息比刚才那几条都急"的场景，
而不是"我要一个栈"。

### 6. `xQueueOverwrite`：长度必须为 1，语义是"最新值"

教材加了一条硬约束（`rtos.txt:3654-3656`）：

> 注意，队列长度必须为 1。当队列满时，这些函数会覆盖里面的数据，这也意味着这些函数**不会被阻塞**。

两个后果：

- **不阻塞**：所以它能在 ISR 里用而不需要担心"没空间"。
- **会丢旧值**：这是它的目的，不是缺陷——它表达的是"状态"，不是"事件流"。
  典型用法：传感器最新一次读数、当前模式、最新按键。

这与 Day04 表格里"队列能传数据"并不矛盾，但要点必须说清：
**同一个队列，长度 1 + Overwrite 是"状态信箱"，长度 N + Send 是"事件流"。**
选错了就会出现"我只想拿最新值，结果取到一串过期值"或"我明明发了一串，只收到最后一个"。

`xQueuePeek` 是另一个方向的变体（`rtos.txt:3662-3672`）：复制但**不移除**，
所以"一旦有数据，之后每次偷看都能成功"。它的用途是**广播式读取**——
但注意它仍是"谁先读谁读到同一个值"，多个任务靠 Peek 读队列并不能构成真正的广播，
因为**没有机制保证每个任务都 peek 到同一条**（谁在两次写之间没读，就漏了）。
真正的广播要用事件组（Day08）。

### 7. 队列集：解决"同时等多个队列"，代价是长度要相加

教材先讲清楚了**为什么要它**（`rtos.txt:3714-3718`）：

> 假设有 2 个输入设备：红外遥控器、旋转编码器……InputTask 如何及时读取到多个队列的数据？
> 要使用队列集。

并且给了一条很硬的架构理由——**驱动不该知道业务**：

> 在红外遥控器的驱动程序里，不应该有游戏相关的代码，这样，切换使用场景时，
> 这个驱动程序还可以继续使用。

如果没有队列集，InputTask 只能在两个队列之间轮流"试一次"（超时 0 轮询），
或者固定等一个队列——前者浪费 CPU，后者会丢掉另一个设备的事件。

**队列集的本质**（`rtos.txt:3720`）：**"队列集的本质也是队列，只不过里面存放的是'队列句柄'。"**

七步用法（`rtos.txt:3722-3730`）：

```text
a. 创建队列 A，长度 n1
b. 创建队列 B，长度 n2
c. 创建队列集 S，长度 = n1 + n2        ← 注意是加，不是取最大
d. 把 A、B 加入 S
e. 写 A → 句柄 A 顺便进 S
f. 写 B → 句柄 B 顺便进 S
g. InputTask 读 S 得到一个句柄 → 知道哪个队列有数据 → 再读那个队列
```

**为什么长度是 n1 + n2？** 因为集里是"每个待处理 item 一个句柄"。
最坏情况是 A 满（n1 个）同时 B 也满（n2 个），此时 S 里要有 n1 + n2 个句柄才装得下。
取 `max(n1, n2)` 的话，两个队列同时积压时集会满，而**集满了会怎样**是必须自己回答的问题
（教材没写；按 §4 的推理，任务侧读集只是"读不到"，但句柄漏登记会导致那个队列的数据再也等不到
——所以长度不足是**静默丢事件**，不是丢数据）。

### 8. 队列集**没有**解决的三件事

教材只讲能力，这里把边界列清，因为它们决定了"什么时候队列集不够用"：

| 队列集做不到 | 为什么 | 替代方案 |
| --- | --- | --- |
| **多个任务各读一份**（广播） | 还是"唤醒优先级最高的一个" | 事件组 + 各自缓存最新值 |
| **一次等多条件成立** | 读集返回的是**一个**句柄 | 事件组（与/或语义） |
| **数据的优先级/合并** | 谁先塞谁先出，无优先级 | 多队列 + 自己的调度策略 |
| **消除"读集再读队列"的两步** | 集只告诉你"有"，不搬运数据 | 没有办法，这就是它的语义 |

最后一条是最容易被忽略的**竞态窗口**：`xQueueSelectFromSet` 返回句柄之后、
真正 `xQueueReceive` 之前，别的任务可能已经把这个队列读空了。
所以 **`xQueueReceive` 的返回值必须检查**，不能假定"集说有了就一定有"。
这一点与 Day04 §6 的"带 x 前缀必须查返回"是同一条纪律。

### 9. 三个示例工程讲了三种"数据流方向"

教材给了五个示例（`rtos.txt:3680-4131`），值得按**数据流方向**而不是按难度排：

| 示例 | 数据流 | 它证明了什么 |
| --- | --- | --- |
| `13_queue_game` | ISR → 1 任务 | 队列替代环形缓冲：`xQueueSendFromISR` + `xQueueReceive(portMAX_DELAY)`，把轮询改成事件驱动（教材原话："第 15 行是原来的代码，它使用轮询的方式读取遥控键值，效率很低"） |
| `14_queue_game_multi_input` | 2 个 ISR/驱动 → 1 任务 | 一个任务要收多路，但还不知道队列集 |
| `15_queueset_game` | 2 队列 → 1 任务（用集） | 队列集把"轮流试"变成"谁有数据就唤醒我" |
| `17_queue_car_dispatch` | 1 ISR → **N 队列** | 反向广播：中断里把同一份键值**写多个队列**，每个任务各自有一份 |
| （无） | 1 任务 → N 任务 | 队列做不到（唤醒规则只挑一个） |

最后一行是本篇最重要的一条结论：**队列天生是"一份数据给一个人"。**
要"一份数据给多人"，教材的做法不是找一个更聪明的队列，而是**复制 N 份**（`17_queue_car_dispatch`），
代价是 N 次 memcpy 与 N 份缓冲。这正好解释了 Day04 表里为什么把"广播"这一格单独给了事件组。

---

## 系统调用/接口卡片

### `QueueHandle_t xQueueCreate( UBaseType_t uxQueueLength, UBaseType_t uxItemSize )`

作用：动态创建队列，内存在函数内部从 FreeRTOS 堆分配。

参数：
- `uxQueueLength`：最多存多少个 item（≥ 0；0 是合法的，但那就成了纯同步对象）。
- `uxItemSize`：**每个 item 的字节数**。传 0 会创建一个"只有计数没有数据"的队列——
  这正是信号量的实现方式（Day02 §1）。

返回值和错误：成功返回句柄；失败返回 `NULL`（内存不足）。
**`NULL` 必须检查**，否则后续所有操作都在拿空指针。

资源所有权：内存（长度 × 大小 + 结构体）由内核堆分配；`vQueueDelete` 释放。
**只有动态创建的才能 `vQueueDelete`。**

线程/进程安全：任务态调用。**中断里不能创建队列。**

最小示例（本仓库 `F103_10/main.c`）：

```c
s_q_samples = xQueueCreate( QUEUE_DEPTH, sizeof(sample_t) );   /* QUEUE_DEPTH = 8 */
if (s_q_samples == NULL) { LOG("创建队列失败：heap 不足\n"); }
```

常见误用：① `uxItemSize` 传成"指针大小"而非结构体大小（`sizeof(p)` vs `sizeof(*p)`）；
② 不检查 `NULL`；③ 以为之后能改长度。

### `BaseType_t xQueueSend( QueueHandle_t q, const void *item, TickType_t ticks )`

作用：写队列**尾部**（等价于 `xQueueSendToBack`）。

参数：`item` 是**指针，但内容会被复制**（复制多少由创建时的 `uxItemSize` 决定）；
`ticks` = `0`（满则立刻返回）/ `portMAX_DELAY`（一直等）/ 其它值。

返回值和错误：`pdPASS` 成功；`errQUEUE_FULL` 失败（满且超时）。
写法 `== pdPASS` 或 `!= pdPASS`，**不要**写 `== pdTRUE`（值是 1 但不是同一族常量）。

资源所有权：复制完成后与 `item` 无关——`item` 可以是栈上的局部变量。

线程/进程安全：任务态可阻塞。ISR 里必须用 `...FromISR`。

最小示例：

```c
sample_t s = { .value = 42, .tick = xTaskGetTickCount() };
if (xQueueSend( q, &s, 0 ) != pdPASS) { /* 队列满：计数并丢 */ }
```

常见误用：① 传指针后以为队列"持有"了那个对象，随后改动它；
② 用 `== pdTRUE` 判 `errQUEUE_FULL` 之外的失败；③ 在 ISR 里调它（不能阻塞）。

### `BaseType_t xQueueSendToBackFromISR( QueueHandle_t q, const void *item, BaseType_t *pxWoken )`

作用：ISR 里写队列尾部。

参数：**没有超时参数**——中断不能阻塞，满就立即失败。
`pxWoken` 输出"是否唤醒了更高优先级任务"，必须接着调 `portYIELD_FROM_ISR(*pxWoken)`。

返回值和错误：`pdPASS` / `errQUEUE_FULL`。**`errQUEUE_FULL` 在 ISR 里是常态**，
必须计数（本仓库 `F103_11` 的 `trunc`）。

资源所有权：同 `xQueueSend`（复制）。

线程/进程安全：仅 ISR。

最小示例（教材 `13_queue_game` 的写法，`rtos.txt:3694-3698` + 本仓库 `F103_11`）：

```c
struct input_data idata = { .dev = 0, .val = 0 };
BaseType_t woken = pdFALSE;
if (xQueueSendToBackFromISR( g_xQueuePlatform, &idata, &woken ) != pdPASS) { trunc++; }
portYIELD_FROM_ISR( woken );
```

常见误用：① 漏掉 `portYIELD_FROM_ISR`，唤醒延迟到下一个 tick；
② 不检查返回值，满时静默丢（`F103_11` 的 `trunc` 就是为这个存在的）；
③ 把 `pxWoken` 传 `NULL`（教材示例里也这么写过）——那样就没有让出机会了。

### `UBaseType_t uxQueueMessagesWaiting( const QueueHandle_t q )` / `uxQueueSpacesAvailable( const QueueHandle_t q )`

作用：查询"当前有几个数据" / "还剩几个空位"。

返回值：个数。两者之和恒等于创建时的 `uxQueueLength`（除非队列正在被并发修改）。

资源所有权：无。

线程/进程安全：可在任务态调用。**读到的值立刻就会过期**——
两个任务之间的瞬间内可能已经有别人读写过，不能用于"先查空再取"这类判断
（那本身就是 check-then-act，见 Day01 §5）。

最小示例（`F103_10` 的 monitor 任务）：

```c
LOG("produced=%u consumed=%u dropped=%u queued=%u\n", ..., (unsigned)uxQueueMessagesWaiting( s_q_samples ));
```

常见误用：`if (uxQueueMessagesWaiting(q) > 0) { xQueueReceive(q, ...); }`——
两个任务同时这样写就会有一个拿到 `errQUEUE_EMPTY`。**直接 `xQueueReceive` 并检查返回值。**

### `BaseType_t xQueueReceive( QueueHandle_t q, void *buf, TickType_t ticks )` / `xQueueReceiveFromISR(...)`

作用：读头部并**移除**。

返回值和错误：`pdPASS` / `errQUEUE_EMPTY`。

其余同发送函数：`buf` 由调用者提供、大小必须 ≥ `uxItemSize`（否则溢出，编译器不会拦）；
ISR 版没有超时，队列空就立刻返回 `errQUEUE_EMPTY`。

常见误用：`buf` 比 `uxItemSize` 小。这是**静默的栈越界**，
`configASSERT` 在 `xQueueReceive` 里通常不检查这一点。

### `QueueSetHandle_t xQueueCreateSet( const UBaseType_t uxLen )` / `xQueueAddToSet(...)` / `xQueueSelectFromSet(...)`

作用：创建队列集 / 把成员加入 / 阻塞等待"任一成员有数据"。

参数：`uxLen` = **各成员队列长度之和**（教材 `rtos.txt:3726`）；`xQueueAddToSet` 的
第一个参数是成员句柄；`xQueueSelectFromSet` 带 `xTicksToWait`，语义与 `xQueueReceive` 一致。

返回值和错误：`xQueueCreateSet` 返回句柄或 `NULL`；`xQueueAddToSet` 返回 `pdTRUE`/`pdFALSE`；
`xQueueSelectFromSet` 返回**成员队列句柄**或 `NULL`（超时）。

资源所有权：新建的队列集由内核堆分配；成员队列本身不变。

线程/进程安全：任务态。设 `configUSE_QUEUE_SETS = 1` 才有这套 API——
**本仓库未定义该宏，所以队列集在本仓库是编译不过的**（这是本篇无法实测的直接原因）。

最小示例：

```c
QueueSetHandle_t set = xQueueCreateSet( Q_A_LEN + Q_B_LEN );
xQueueAddToSet( q_a, set );
xQueueAddToSet( q_b, set );
for (;;) {
    QueueHandle_t got = xQueueSelectFromSet( set, portMAX_DELAY );
    if (got == q_a) { xQueueReceive( q_a, &item, 0 ); }   /* 返回值仍要查 */
    else if (got == q_b) { xQueueReceive( q_b, &item, 0 ); }
}
```

常见误用：① 集长度写成 `max(n1, n2)`；② 拿到句柄后就假定队列里一定有数据（竞态窗口）；
③ 用 `== pdTRUE` 判"是不是 q_a"（返回值是句柄，不是布尔）。

---

## 最小可运行实验

### 任务↔任务（`F103_10`）

```text
produce (prio 2)  ──xQueueSend(q, &sample, 0)──▶  q[8]
consume (prio 3)  ◀──xQueueReceive(q, &sample, portMAX_DELAY)──
monitor (prio 1)  ──每 1000 ms 打印 queued / produced / consumed / dropped
```

三个计数的分工恰好覆盖队列的三种病：

| 读数 | 非 0 说明 |
| --- | --- |
| `dropped > 0` | 生产者赶不上（发送超时/满） |
| `queued > 0` 持续 | 消费速度不足，积压在队列里 |
| `produced > consumed` 且 `dropped == 0` | 消费者卡死（既不消费也没报丢） |

### ISR→任务（`F103_11`）

USART1 `RXNE` 中断里逐字节 `xQueueSendToBackFromISR`，任务侧 `xQueueReceive` 拼行。
它的价值在于把 §4 的"ISR 满就丢"变成一条可判定的读数（`trunc`）。

---

## 编译与运行

```bash
# 任务↔任务
cd experiments/F103_10_FreeRTOS_Tasks
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake -DF103_QEMU=ON
cmake --build build
cd ../..
timeout 25 qemu-system-arm -M stm32vldiscovery -display none -monitor none \
  -serial stdio -kernel experiments/F103_10_FreeRTOS_Tasks/build/f103_10_freertos_tasks.elf \
  < /dev/null 2>&1 | tr -d '\r' | tail -3

# ISR→任务（需要先注入串口输入，见 F103_11 README）
cd experiments/F103_11_FreeRTOS_ISR_Queue && cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake -DF103_QEMU=ON && cmake --build build
```

---

## 结果观察

### 任务↔任务（实测）

```text
[monitor] tick=11062 produced=55 consumed=55 dropped=0 queued=0
```

`dropped=0` 与 `queued=0` 同时成立，说明**队列深度 8 足够**、消费速度跟得上。
这是"队列不丢东西"的正面证据；反面证据在下面。

### ISR→任务（实测）

```text
rx 0 → 7 → 14
突发 300 B → trunc=4
300 = 4 × 64 + 44
```

同一份"不丢"的判据，在 ISR 侧**注定不成立**：任务侧满了可以等，ISR 侧满了只能丢。
`trunc=4` 不是 bug，是 `FromISR` 语义的必然结果——**能做的只有数清丢了几次**。

### 队列集（**未实测**）

本仓库 `FreeRTOSConfig.h` 未定义 `configUSE_QUEUE_SETS`，队列集 API 不在编译产物里。
教材的七步流程与长度公式（`n1+n2`）来自原文，§8 的四条边界来自推理。
**这一节没有任何本机读数，不得写成"已验证"。**

---

## 常见错误和陷阱

| 陷阱 | 症状 | 根因 | 处理 |
| --- | --- | --- | --- |
| `uxItemSize` 传指针大小 | 结构体只存进前 4 字节，读出来是垃圾 | `sizeof(p)` vs `sizeof(*p)` | 传 `sizeof(struct)`；接收方 `buf` 也要够大 |
| 接收 `buf` 比 item 小 | 栈越界，偶发 HardFault | 内核按 `uxItemSize` 复制，不检查 `buf` | 用 `typeof` 或同一 typedef |
| ISR 里用 `xQueueSend` | 断言失败或卡死 | ISR 不能阻塞 | 用 `...FromISR`，并处理 `errQUEUE_FULL` |
| ISR 满时静默丢 | 偶发少数据，无法定位 | 没检查返回值 | 计数（`F103_11` 的 `trunc`）并打印 |
| 漏 `portYIELD_FROM_ISR` | 高优先级任务延迟一个 tick 才跑 | `pxWoken` 被忽略 | 必须传 `&woken` 并调用 |
| 传指针进队列 | 悬空指针 / 数据被改动 | 队列复制的是指针值，不是对象 | 尽量传值；传指针时自己定生命周期 |
| 用 `uxQueueMessagesWaiting > 0` 再取 | 偶发 `errQUEUE_EMPTY` | check-then-act 竞态 | 直接 `xQueueReceive` 查返回值 |
| 队列长度 1 却用 `xQueueSend` | 满时阻塞/丢，拿不到"最新值" | `Send` 是"追加"，不是"覆盖" | 要最新值用 `xQueueOverwrite`（仅长度 1） |
| 队列长度 N 却用 `xQueueOverwrite` | 断言失败（长度必须 1） | API 硬约束 | 改长度，或改回 `Send` |
| 队列集长度取 `max(n1,n2)` | 两队列同时积压时静默丢事件 | 集里是"每个待处理 item 一个句柄" | 长度 = `n1 + n2` |
| 拿到集返回的句柄就直接读 | 偶发 `errQUEUE_EMPTY` | 读集与读队列之间有竞态窗口 | 仍要检查 `xQueueReceive` 返回值 |
| 指望队列做广播 | 只有一个任务收到 | 唤醒规则只挑最高优先级的一个 | 复制 N 份（教材 `17_..._dispatch`）或换事件组 |

---

## 故障排查方法

1. **"收不到数据。"** 顺序：① `xQueueCreate` 返回值是否 `NULL`；② 发送方返回值是否
   `errQUEUE_FULL`；③ `uxQueueMessagesWaiting` 是否一直 0（若一直是 0 而生产者也说成功，
   说明读的不是同一个句柄）。
2. **"偶发少数据。"** 先看接收方有没有 `errQUEUE_EMPTY` 分支，再看发送方有没有 `errQUEUE_FULL`。
   **两边都检查才会知道丢在哪一侧**——`F103_11` 的 `trunc` 就是这一条的实现。
3. **"任务不阻塞，CPU 100%。"** 接收调用里超时传了 `0`。改 `portMAX_DELAY`，
   或先确认"轮询"确实是你想要的。
4. **"改了结构体后数据全乱。"** 改了 `uxItemSize` 的源结构体但没重建队列（长度 × 大小在创建时定死），
   或某一侧的 `buf` 没跟着改。
5. **"高优先级任务没被唤醒。"** ISR 侧检查 `portYIELD_FROM_ISR`；任务侧检查是否用了
   `xQueueSend` 但超时为 0（发送成功 != 对方立刻运行，抢占由调度器决定）。
6. **"想同时等两个队列却卡住。"** 确认 `configUSE_QUEUE_SETS` 是否打开；
   本仓库默认关闭，此时只能用"超时 0 轮流试"，那会浪费 CPU 且不保证及时。

---

## 练习题

### 基础题

1. 队列的哪两项必须在创建时确定？如果把 `uxItemSize` 从 8 改成 16，已创建的队列会怎样？
2. 教材说 FreeRTOS 用拷贝。请从"所有权"角度解释：为什么拷贝之后，发送方的栈帧可以被回收？
3. 多个任务等同一个队列，数据来了唤醒谁？这条规则在什么情况下**恰好**等价于 FIFO？

### 改错题

4. 下面这段在 ISR 里写队列，找出三处问题：

   ```c
   void USART1_IRQHandler(void) {
       uint8_t c = USART1->DR;
       xQueueSend( g_q, &c, portMAX_DELAY );
   }
   ```

5. 下面这段想取"最新的传感器值"，说明为什么实际会取到一串过期值：

   ```c
   /* 生产者 */ xQueueSend( q, &reading, 0 );
   /* 消费者 */ xQueueReceive( q, &reading, portMAX_DELAY );
   ```

6. 下面这段用队列集等两个队列，指出两处问题：

   ```c
   QueueSetHandle_t s = xQueueCreateSet( MAX(len_a, len_b) );
   xQueueAddToSet( q_a, s ); xQueueAddToSet( q_b, s );
   for (;;) {
       QueueHandle_t h = xQueueSelectFromSet( s, portMAX_DELAY );
       xQueueReceive( h, &item, 0 );
   }
   ```

### 综合题

7. 用 `F103_10` 的读数回答：如果 `QUEUE_DEPTH` 从 8 改成 2，`dropped` / `queued` 各会怎么变？
   给出你的预测，再实际改一次并对比。若 `dropped` 仍为 0，说明你的预测错在哪一环。

8. `F103_11` 的 `trunc=4` 是"队列满时丢"。请设计一个方案让它**不丢**，
   并说明你为此付出了什么（提示：ISR 里唯一能做的是加快排空，或者换数据结构）。

9. 把教材 `17_queue_car_dispatch` 的"一个中断写 N 个队列"与"事件组广播"做对照：
   ① 两者都能让 N 个任务都收到，差别是什么？② 如果 N 个任务里有一个很慢，
   队列方案与事件组方案分别会付出什么代价？③ 你会在什么场景下**故意**选较贵的那一个？

---

## 验收标准

- [ ] 能说出队列的哪两项在创建时定死，以及"想改"时的三条退路。
- [ ] 能指出教材 6 条"用拷贝"的理由里哪一条是架构性的、哪一条可以商量。
- [ ] 能默写唤醒规则，并解释它为什么使队列无法做公平轮转。
- [ ] 能解释 `trunc=4` 与 `300 = 4×64 + 44` 各自证明了什么。
- [ ] 能说清 `xQueueOverwrite` 的两条约束（长度 1、不阻塞）与它的语义（状态而非事件流）。
- [ ] 能解释队列集长度为什么是 `n1 + n2`，以及长度不足的后果是丢事件而非丢数据。
- [ ] 能说出队列集**做不到**的四件事，并各给一条替代方案。
- [ ] 能指出本篇**未验证**的三项（队列集全部、Peek、Overwrite）及原因。

---

## 延伸阅读

- 教材第 11 章：`rtos.txt:3491-4131`
  - 11.1 特性：3498–3556（五特性、拷贝 vs 引用、阻塞与唤醒规则）
  - 11.2 函数：3557–3679（创建/复位/删除/写/读/查询/覆盖与偷看）
  - 11.3–11.7 五个示例：3680–4131（`13_queue_game`、`14_..._multi_input`、队列集、`17_..._dispatch`）
- 源码：`vendor/freertos/Source/queue.c`（`xQueueGenericCreate`、`prvCopyDataToQueue`、
  `xQueueGenericSend` 里的唤醒排序）
- 本仓库实验：`F103_10`（任务↔任务）、`F103_11`（ISR→任务 + `trunc`）
- 相关笔记：`Day04`（四问与五对象表）、`Day06`（信号量就是"长度/大小特化的队列"）、`Day08`（事件组与广播）
- 下一篇：`Day06_信号量.md`（教材第 12 章）

---

**验证记录**

| 日期 | 环境一 | 命令一 | 结果一 | 环境二 | 命令二 | 结果二 | 未覆盖 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 2026-10-30 | 教材抽取文本 | 读 `rtos.txt:3491-4131` | 11.1–11.7 全章；拷贝 6 条理由（3512-3520）、唤醒规则两处（3542-3544、3551-3553）、`FromISR` 无超时（3592-3600）、`Overwrite` 长度必须 1（3654-3656）、队列集本质与七步（3714-3730） | WSL2 + QEMU 10.2.1 | `qemu-system-arm -M stm32vldiscovery -kernel f103_10_freertos_tasks.elf` | `produced=55 consumed=55 dropped=0 queued=0`、`heap_free` 恒定 496 B | 队列集、`xQueuePeek`、`xQueueOverwrite`、`xQueueSendToFront` 全部无实测 |
| 2026-10-30 | 本仓库源码 | `grep` `configUSE_QUEUE_SETS` / `xQueueCreate` | `configUSE_QUEUE_SETS` **未定义** ⇒ 队列集 API 不在编译产物里；`F103_10` 用 `xQueueCreate(8, sizeof(sample_t))`；`F103_11` 用 `xQueueSendToBackFromISR` + `portYIELD_FROM_ISR` | 同上 | `qemu-system-arm ... -kernel f103_11_freertos_isr_queue.elf` | `rx 0→7→14`；突发 300 B → `trunc=4`，`300 = 4×64 + 44`；`heap_free` 恒定 1048 B | ISR 侧真实 ORE 溢出未验证；队列深度上限未扫描 |
