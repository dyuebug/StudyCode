# Day07：互斥量（教材第 13 章）

**验证状态**：优先级继承**实测**（`exp16_priority_inversion.log`：`mutex` 轮 H 等锁 105 vs
`binary-sem` 轮 405 tick；`prio 1→3`，give 时回 1）；**继承的判定条件、holder 的写入与清除、
递归锁的持有者校验**全部**源码核对**（`Source/queue.c`、`Source/tasks.c`，V11.3.0，行号见下）；
**未验证**：教材 `22_mutex_priority_inversion` 工程本机没有；递归锁、交叉死锁**无任何实验**；
"B 替 A 解锁"的后果链是**源码推理**，未做成可运行用例。

---

## 学习目标

学完本篇应能用代码证据回答：

1. 用信号量做互斥，教材说需要"两个前提"——这两个前提在约束谁？内核为什么不管？
2. 教材说"谁上锁谁释放：只是约定"，这句话在源码里能找到证据吗？在哪一行？
3. 为什么这个"约定"不是礼节问题？违反它会让互斥**永久失效**的链条是什么？
4. 优先级继承的触发条件是"等待者优先级 ≥ 持锁者"还是"严格大于"？同优先级会继承吗？
5. 继承改的是 `uxPriority` 还是 `uxBasePriority`？两者分工是什么？
6. 等待者**超时退出**时，持锁者的优先级是恢复原值，还是退到别的地方？为什么？
7. 为什么"互斥量不能在 ISR 里用"？除了"规范不允许"，机制上的理由是什么？
8. 递归锁解决哪一种死锁？不解哪一种？

---

## 环境与前置条件

| 项 | 版本 / 说明 |
| --- | --- |
| 教材 | 第 13 章互斥量（`rtos.txt:4927-5068`），含 13.1 使用场合、13.2 函数、13.3 优先级继承、13.4 递归锁、13.5 常见问题 |
| 内核 | FreeRTOS **V11.3.0**；互斥量实现在 `Source/queue.c`，继承实现在 `Source/tasks.c` |
| 关键配置 | `configUSE_MUTEXES 1`（`FreeRTOSConfig.h:42`）、`configUSE_RECURSIVE_MUTEXES 1`（`:43`）、`INCLUDE_xTaskGetCurrentTaskHandle 1`（`:86`）——三项都已开启 |
| 实验 | `F103_16_FreeRTOS_PriorityInversion`（两轮对照）、`F103_10`–`F103_14`（五处日志锁） |
| 前置 | `Day04`（同步 vs 互斥、四问）、`Day06`（信号量的上限与"give 不检查持有者"） |

---

## 核心概念

### 1. 教材的两个"前提"，都在约束**非持有者**

教材开篇的比喻是上厕所（`rtos.txt:4928-4937`）：

> 怎么独享厕所？自己开门上锁，完事了自己开锁。你当然可以进去后，让别人帮你把门：
> 但是，命运就掌握在别人手上了。

然后它明确列出用信号量做互斥需要的**两个前提**：

| 前提 | 教材原话 | 在约束谁 |
| --- | --- | --- |
| ① | 任务 B 很老实，不撬门（一开始不 `give` 信号量） | **等待者**不能自己给自己放行 |
| ② | 没有坏人：别的任务不会 `give` 信号量 | **第三方**不能替别人放行 |

两条的共同点很关键：**它们约束的都是"没有拿到锁的任务"**。
换句话说，如果一个"有钥匙的人"（持有者）和一个"没钥匙的人"都能开门，
那这把锁保护的就不是"谁在用"，而是"谁先路过"。

所以互斥量要解决的不是"计数"（Day06 的二值信号量已经有计数了），
而是**所有权**。教材把它的名字拆得很清楚（`rtos.txt:4939-4941`）：
**量** = 值为 0/1；**互斥** = 用来实现互斥访问；
核心在于"**谁上锁，就只能由谁开锁**"。

### 2. 教材说"这只是约定"——源码证实它是对的

紧接着教材写了一句很罕见的自曝（`rtos.txt:4942-4944`）：

> 很奇怪的是，FreeRTOS 的互斥锁，并没有在代码上实现这点：
> 即使任务 A 获得了互斥锁，任务 B 竟然也可以释放互斥锁。
> 谁上锁、谁释放：只是约定。

这句话可以直接拿源码核对。本仓库 V11.3.0 的三处相关代码：

| 位置 | 做了什么 | 有没有校验持有者 |
| --- | --- | --- |
| `queue.c:1698-1706`（take 成功） | `xMutexHolder = pvTaskIncrementMutexHeldCount()` | —— 这是**写入** holder |
| `queue.c:2405-2420`（give，`prvCopyDataToQueue`） | 遇 mutex：`xTaskPriorityDisinherit( xMutexHolder )` + `xMutexHolder = NULL` | **没有**。函数里根本没有"当前任务 == holder"的比较 |
| `queue.c:773`、`queue.c:833`（递归锁 give/take） | `if( xMutexHolder == xTaskGetCurrentTaskHandle() )` 才操作，否则 `pdFAIL` | **有** |

于是得到一个比教材更精确的结论：

> **"谁上锁谁开锁"在普通互斥量（`xSemaphoreCreateMutex`）里是约定，没有代码检查；
> 在递归锁（`xSemaphoreCreateRecursiveMutex`）里才是硬检查。**

教材把两者混在一起说，读者容易以为"互斥量会拒绝别人解锁"。**它不会。**
`xSemaphoreGive()` 对一把不属于自己的互斥量会成功返回 `pdTRUE`，锁被打开。

### 3. 违反约定不是"没礼貌"，它会让互斥**永久失效**

补这一步是本篇最该记住的东西。假设 A 持有互斥量 M，B 调了 `xSemaphoreGive(M)`：

```text
1. B give        → 计数 0→1（锁被打开）；xTaskPriorityDisinherit(A) 恢复 A 的优先级；
                   xMutexHolder = NULL                     ← holder 被清掉
2. A 继续执行     → A 仍以为自己在临界区里（它没有"我持有锁"的软件标记，
                   只有 xMutexHolder 这一个记录，而记录已经不是它了）
3. C take        → 成功，计数 1→0，holder = C       ← 现在 C 也在临界区
4. A 用完 give   → 计数 0→1；xTaskPriorityDisinherit(C) 恢复 C 的优先级（C 仍持锁！）；
                   holder = NULL
5. D take        → 成功，计数 1→0，holder = D       ← 三个任务同时在临界区
```

两个后果比"锁被打开一次"严重得多：

- **互斥从此失效且不再自愈**。计数会随每次非法 give 回到 1，锁永远是"可用"状态。
- **优先级继承被错误地恢复**。第 4 步 `xTaskPriorityDisinherit(C)` 在 C 还持锁时就把它降回原优先级，
  于是 C 再次被中间优先级任务抢占——反转重新出现，而且没有任何日志。

这段链条是**源码推理**（依据 §2 的三处代码），不是实测。但它明确了一条工程判据：

> 互斥量不是"锁"，它是"**锁 + 一个单字段的所有权记录**"。
> 所有权记录一旦被第三方破坏，锁与继承一起失效，而没有任何返回值会告诉你发生了这件事。

这也是为什么 §5 会看到：FreeRTOS 的继承**完全依赖** `xMutexHolder` 这一个人。

### 4. 继承的判定：严格小于、只升不降、base 与 current 分工

教材给出的规则是（`rtos.txt:4996-5004`）：

> 假设持有互斥锁的是任务 A，如果更高优先级的任务 B 也尝试获得这个锁……
> 于是任务 A 就继承了任务 B 的优先级……等任务 A 释放互斥锁时，它就恢复为原来的优先级。

源码把它补精确了（`tasks.c:6650-6670`）：

```c
if( pxMutexHolder != NULL )
{
    /* 持锁者优先级 < 尝试取锁者优先级 才继承 */
    if( pxMutexHolderTCB->uxPriority < pxCurrentTCB->uxPriority )
    { ... pxMutexHolderTCB->uxPriority = pxCurrentTCB->uxPriority; ... }
}
```

三条细节，教材没写但会考：

| 细节 | 源码依据 | 后果 |
| --- | --- | --- |
| **严格小于**才继承 | `tasks.c:6661` 的 `<` | **同优先级不继承**。两个同优先级任务争同一把锁时，反转仍可能发生（教材 13.5 只留了标题没写正文，见 §9） |
| 改的是 `uxPriority`（当前），不碰 `uxBasePriority`（原始） | `tasks.c:6697`、`6753` 起的 `xTaskPriorityDisinherit` | 恢复时从 `uxBasePriority` 取回，所以"继承 → 恢复"是对称的 |
| 只升不降 | 条件里只有"持锁者更低才改" | 持锁者本来就比等待者高时，**什么也不做**（`F103_16` 的 `binary-sem` 轮 prio 恒 1 就是这一支） |

本仓库 `F103_16` 的读数正好把这三条落到地上（同一骨架、只换锁类型）：

```text
binary-sem : H 等锁 405 / L 持锁 404 / L 阶段一 354 / 阻塞期 prio 恒 1   ← 无继承
mutex      : H 等锁 105 / L 持锁 105 / L 阶段一  52 / prio 1 → 3（give 回 1）
```

`prio 1 → 3` 是 §4 第一行（严格小于，1 < 3）；`give 回 1` 是第二行（从 `uxBasePriority` 恢复）。
**判据只能靠这两条一起看**：只看"等锁时间变短"无法区分"继承发生了"与"这次运气好"。

### 5. 继承发生在"进入阻塞那一刻"，而超时退出的恢复**不是**简单回原值

这一条是 V11 相对早期版本的改进，教材没有涉及，但它是理解"继承状态机"的关键。

继承的调用点在 take 的阻塞路径上（`queue.c:1786-1795`）：

```c
if( prvIsQueueEmpty( pxQueue ) != pdFALSE )      /* 队列空 = 计数 0，进不去 */
{
    if( pxQueue->uxQueueType == queueQUEUE_IS_MUTEX )
    {
        taskENTER_CRITICAL();
        xInheritanceOccurred = xTaskPriorityInherit( pxQueue->u.xSemaphore.xMutexHolder );
        taskEXIT_CRITICAL();
    }
    vTaskPlaceOnEventList( &( pxQueue->xTasksWaitingToReceive ), xTicksToWait );   ← 之后才阻塞
```

要点：**继承发生在调用真正进入阻塞态之前**，而且**与超时长短无关**——哪怕传 `0` 也会走这条路径
（传 `0` 时 `vTaskPlaceOnEventList` 之后立刻被 `prvUnlockQueue` 唤醒并返回 `errQUEUE_EMPTY`）。

超时退出时（`queue.c:1851-1872`），恢复调用是：

```c
uxHighestWaitingPriority = prvGetHighestPriorityOfWaitToReceiveList( pxQueue );
vTaskPriorityDisinheritAfterTimeout( pxQueue->u.xSemaphore.xMutexHolder, uxHighestWaitingPriority );
```

函数名里的 `AfterTimeout` 和那个参数是重点：

> **超时不是把持锁者降回原优先级，而是降到"仍然在等同一把锁的最高优先级"。**

举例：A(prio 1) 持锁，B(prio 3) 等锁 → A 继承到 3；B 超时放弃，而 C(prio 2) 还在等 →
A 应降到 **2**，不是 1。否则 C 又要重新把它提回去，多一次无意义的优先级抖动。

这一条也是"判据不能只看单次读数"的原因：`F103_16` 里 L 的 prio 在 give 时回 1，
是因为**没有别的任务还在等**，不是因为"恢复总是回原值"。

### 6. ISR 拿锁没有 holder，所以继承在机制上不适用

教材只写了一句"互斥量不能在 ISR 中使用"（`rtos.txt:4991`）。源码给了更深的理由
（`tasks.c:6656-6657` 的注释）：

> If the mutex is taken by an interrupt, the mutex holder is NULL.
> Priority inheritance is not applied in this scenario.

为什么 ISR 拿锁 holder 会是 `NULL`？因为记录 holder 的那段代码在 `xQueueSemaphoreTake`（任务版）里
（`queue.c:1704`），而 ISR 走的是 `xSemaphoreTakeFromISR` → `xQueueReceiveFromISR`
（`semphr.h:675`）——**那条路径根本不写 holder**。

于是因果链是：

```text
ISR 拿锁 → holder 保持 NULL → 后续任务等这把锁时 xTaskPriorityInherit(NULL) 直接返回
          → 继承不发生 → 反转无解
```

所以"ISR 不能用互斥量"不是礼节，而是**这个对象的所有权模型只对任务成立**。
中断没有 TCB，也就没有"优先级"可继承。要在 ISR 里做互斥，只能用临界区（关中断），
而那样就放弃了阻塞与超时（Day04 §5）。

### 7. 教材 13.3 是**单变量**对照：只改创建那一行

教材把两版并排放（`rtos.txt:5008-5022`）：

```c
第 268 行  //g_xSemTicks = xSemaphoreCreateCounting(1, 1);
第 269 行    g_xSemTicks = xSemaphoreCreateMutex();
```

> 把第 268 行打开、第 269 行去掉，就会有优先级反转的问题。
> 把第 268 行去掉、第 269 行打开，就解决了优先级反转的问题。

这个对照设计得干净，有一点值得学：**信号量版用的是 `xSemaphoreCreateCounting(1, 1)` 而不是
`xSemaphoreCreateBinary()`。** 原因在 Day06 §4——`CreateBinary` 初值为 0，还要额外补一次 `give`；
用 `(1,1)` 才能让两版的初始化**行数一致**，否则对照就多了一个变量。

教材给的现象链（`rtos.txt:5024-5028`）：

```text
car1 先运行一会 → car2 运行一会 → car3 启动，拿不到锁而阻塞，并把 car1 的优先级提升
→ car1、car2 交替运行（car1 继承后为 3 > car2 的 2，能抢占；但 car1 自己会 vTaskDelay，car2 就有机会）
→ car1 到终点释放 → car3 运行
```

对照 Day06 §6 的 12.5：同一骨架、同样是 `mdelay` 忙等的 car2，**唯一换的是锁类型**，
反转就消失了。**这正是 `F103_16` 复现的东西**——教材的三个游戏工程本机没有，
但同一机制的读数（405 vs 105、`prio 1→3`）有。

### 8. 递归锁：只解自我死锁，不解交叉死锁

教材区分了两种死锁（`rtos.txt:5032-5058`）：

| 类型 | 场景 | 递归锁能解吗 |
| --- | --- | --- |
| **交叉死锁** | A 拿 M1 要 M2，B 拿 M2 要 M1 | **不能**。这是锁顺序问题，与锁的可重入性无关 |
| **自我死锁** | A 拿锁 M → 调库函数 → 库函数又拿同一把 M → A 阻塞等 A 释放 | **能**。递归锁允许同一任务重复 take |

递归锁的语义（`rtos.txt:5060-5062`）：take N 次就要 give N 次才释放。
实现上是一个计数器 `uxRecursiveCallCount`（`queue.c:77`、`629`、`783`、`835`）：

```c
/* xQueueTakeMutexRecursive */
if( xMutexHolder == xTaskGetCurrentTaskHandle() ) {
    uxRecursiveCallCount++;                     /* 已持有：只加计数，不碰信号量 */
    configASSERT( uxRecursiveCallCount );       /* 溢出检查（queue.c:838） */
} else {
    xQueueSemaphoreTake( pxMutex, xTicksToWait );   /* 首次：真正取锁 */
}

/* xQueueGiveMutexRecursive */
if( xMutexHolder == xTaskGetCurrentTaskHandle() ) {
    uxRecursiveCallCount--;
    if( uxRecursiveCallCount == 0 ) { xQueueGenericSend( ... ); }   /* 归零才真正释放 */
    xReturn = pdPASS;
} else { xReturn = pdFAIL; }                    /* 非持有者：明确失败（queue.c:808-812） */
```

**注意 `pdFAIL` 那两行**：递归锁对"非持有者"是**拒绝**的，普通互斥量是**接受**的。
这正好和 §2 的表对得上——同一个内核里，两个对象对同一件事的态度相反。

递归锁的代价（教材没列）：

- 每个锁多一个 `UBaseType_t` 字段（ARMv7-M 上 4 B）；
- take/give **必须严格配对**，少一次 give = 锁永不释放；多一次 give = `pdFAIL`（有校验，比普通互斥量好）；
- 递归 API 与普通 API **不能混用**：`semphr.h:316` 明确写"递归 take 不得用于
  `xSemaphoreCreateMutex()` 创建的锁"；反向（普通 `xSemaphoreGive` 用于递归锁）虽不会断言，
  但不会递减 `uxRecursiveCallCount`，等于把锁留在一个既非持有也非释放的状态。

### 9. 教材本章的三处缺陷（只记录，不改教材代码）

按 `Day14` 的口径把这些记下来，因为它们直接影响"照着书敲代码"的正确性：

| 位置 | 缺陷 | 照抄的后果 |
| --- | --- | --- |
| `13.2.2` | "获得（ISR 版本）"后面印的是 `xSemaphoreGiveFromISR( ... )` —— 应为 `xSemaphoreTakeFromISR` | 不会编译失败（`semphr.h:675` 里该函数存在），而是**两次 give、零次 take**，逻辑全错且没有报错 |
| `13.5` | 标题印成"7.8 常见问题"，章号残留；**正文为空**，只有一句"使用互斥量的两个任务是相同优先级时的注意事项" | 这一节实质缺失。而它恰好是 §4 表里"同优先级不继承"的关键场景 |
| `13.3` | 只说"`22_mutex_priority_inversion` 主要看 `nwatch\game2.c`"，没说它与 `21_semaphore_priority_inversion` 只差第 268/269 行 | 读者要自己 diff 两个工程才能确认"单变量对照"，容易误以为两版结构不同 |

第三条的价值不是挑错，而是提醒：**教材没写"只改一行"时，要自己验证是不是只改了一行**，
否则拿到的"解决/不解决"结论里可能混着别的变量。

---

## 系统调用/接口卡片

### `SemaphoreHandle_t xSemaphoreCreateMutex( void )`

作用：创建互斥量，**初始计数为 1**（可直接 take）。

参数：无。

返回值和错误：句柄；堆不足返回 `NULL`。

资源所有权：只需结构体（不需要数据存储区）；`vSemaphoreDelete` 回收。
结构体里多两个字段：`xMutexHolder`（持有者 TCB 指针）与 `uxRecursiveCallCount`。

线程/进程安全：**只能任务态调用**（§6：ISR 里没有 holder 可记）。

最小示例（本仓库 `F103_10/main.c:166`）：

```c
s_log_mutex = xSemaphoreCreateMutex();
if (s_log_mutex == NULL) { /* 堆不足，后续所有 LOG 都会失去保护 */ }
```

常见误用：① 忘了 `configUSE_MUTEXES` 必须为 1（本仓库 `:42` 已开）；
② 以为它会拒绝"别人替你解锁"（§2：不会）；③ 在 ISR 里 take。

### `BaseType_t xSemaphoreGive( SemaphoreHandle_t h )`

作用：释放互斥量 / 给信号量。对互斥量而言，释放时会
`xTaskPriorityDisinherit( xMutexHolder )` 并清空 holder（`queue.c:2411-2412`）。

返回值和错误：`pdTRUE` / `pdFALSE`（已满）。
**对互斥量"别人替你给"也返回 `pdTRUE`**——这不是 bug，是 §2 的设计。

资源所有权：释放后 `xMutexHolder` 变 `NULL`。

最小示例见 `F103_16` 的两轮骨架（`use_mutex` 只是创建时的一个分支）：

```c
xSemaphoreTake( c->lock, portMAX_DELAY );
/* 临界区：只放共享状态的读写，不放松手即阻塞的调用 */
xSemaphoreGive( c->lock );
```

### `BaseType_t xSemaphoreTake( SemaphoreHandle_t h, TickType_t ticks )`

作用：获取互斥量；拿不到时**先继承优先级、再阻塞**（`queue.c:1786-1795`）。

返回值和错误：`pdTRUE` / `pdFALSE`（超时）。超时会触发
`vTaskPriorityDisinheritAfterTimeout( holder, 仍在等的最高优先级 )`（§5）。

### `SemaphoreHandle_t xSemaphoreCreateRecursiveMutex( void )` / `xSemaphoreTakeRecursive(...)` / `xSemaphoreGiveRecursive(...)`

作用：可重入锁，同一任务可多次 take。

参数：take 带 `xTicksToWait`（与普通版一致）；give 无参数。

返回值和错误：take 返回 `pdTRUE`/`pdFAIL`（超时）；
**give 在非持有者调用时返回 `pdFAIL`**（`queue.c:808-812`），
这是它相对普通互斥量**多出来**的一层保护。

资源所有权：`uxRecursiveCallCount` 归持有者独占修改，无需额外互斥（`queue.c:780-782` 注释说明理由：
只有一个持有者，所以它不会并发变化）。

常见误用：① take/give 次数不等（少 give 必锁死）；② 与 `xSemaphoreTake`/`xSemaphoreGive` 混用；
③ 以为它能解决交叉死锁。

### `SemaphoreHandle_t xSemaphoreCreateMutexStatic( StaticSemaphore_t *buf )`

作用：静态分配版。本仓库 `configSUPPORT_STATIC_ALLOCATION = 0`（`FreeRTOSConfig.h:33`），
**这一族在本仓库不可用**——用它会链接失败。

---

## 最小可运行实验

`experiments/F103_16_FreeRTOS_PriorityInversion/`——同一骨架跑两轮，唯一变量是 `use_mutex`：

```text
L (prio 1)  持锁 → 做三段工作（中间 vTaskDelay 让出）
M (prio 2)  不需要锁，忙等 mdelay  ← 这正是教材 12.5 的 car2
H (prio 3)  在 L 持锁期间 take 同一把锁
```

| 轮次 | 锁 | 观察点 |
| --- | --- | --- |
| `binary-sem` | `xSemaphoreCreateBinary()` | H 等锁时长、L 阻塞期优先级 |
| `mutex` | `xSemaphoreCreateMutex()` | 同上 + `xMutexHolder` 生效后的继承 |

这与教材 13.3 的第 268/269 行对照**同构**，但锁的类型是 binary 而非 counting(1,1)
（本仓库两轮都显式补了初值，所以初始化行数仍然一致）。

另外本仓库有 **五处** `xSemaphoreCreateMutex()` 用作日志锁：`F103_10:166`、`F103_11:241`、
`F103_12:349`、`F103_13:416`、`F103_14:417`。它们的角色是"教材 13.1 说的串口混杂"的正面例子——
多个任务 `printf` 时不加锁就会交错。**但这五处都没有做"去掉锁会怎样"的对照**，
所以那条危害在本仓库是教材推论，不是实测（见下节）。

---

## 编译与运行

```bash
cd experiments/F103_16_FreeRTOS_PriorityInversion
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake -DF103_QEMU=ON
cmake --build build

cd ../..
timeout 30 qemu-system-arm -M stm32vldiscovery -display none -monitor none \
  -serial stdio -kernel experiments/F103_16_FreeRTOS_PriorityInversion/build/f103_16_priority_inversion.elf \
  < /dev/null 2>&1 | tr -d '\r' | tee tests/qemu-stm32/logs/exp16_priority_inversion.log

# 判定器（只判 binary > 2×mutex 这一条算术）
./tests/qemu-stm32/run.sh exp16
```

---

## 结果观察

### 两轮对照（实测）

```text
binary-sem : H 等锁 405 / L 持锁 404 / L 阶段一 354 / 阻塞期 prio 恒 1
mutex      : H 等锁 105 / L 持锁 105 / L 阶段一  52 / prio 1 → 3（give 内部回 1）
```

三条读法：

1. **`H 等锁` ≈ `L 持锁`**：H 的等待时间几乎等于 L 的整个持锁时间，
   说明 H 确实是"从 L 拿到锁那一刻起就开始等"，中间没有别的延迟混进来。
2. **`prio 恒 1` vs `prio 1→3`**：这是**机制证据**，比时长更硬。
   时间会受调度抖动影响，优先级字段不会——它只有"被改"和"没被改"两种。
3. **比值 3.9×**（另一次 412/410 vs 103/104，比值 3.86–4.0）：跨运行有漂移，
   所以判定器只写 `binary > 2×mutex` 这条宽松算术。**不要**把 405/105 当常数写进判据。

优先级继承的**唯一变量**是锁类型：骨架、三任务优先级、延时、栈深度全部相同。
这是"3.9× 只能归因于继承"的前提。若同时改了延时或优先级，这个结论就不成立。

### 本篇**没有**验证的东西

- 教材 `22_mutex_priority_inversion` 工程**本机没有**，其"car1/car2 交替运行"的现象未复现。
- **递归锁完全没有实验**。§8 的行为全部来自源码阅读；`uxRecursiveCallCount` 的实际值、
  多次 take 后的释放时机都没有读数。
- **交叉死锁与自我死锁都没有复现**。FreeRTOS 也没有死锁检测（源码里没有相关机制），
  但"没检测到"和"检测了也没报"在本仓库无法区分。
- **"B 替 A 解锁"的后果链（§3）是源码推理**，没有写成可运行用例。要证明它需要
  在临界区里插打印并观察"三个任务同时在临界区"，本仓库没做。
- 五处日志锁**没有做"去掉锁"的对照**，"不加锁会混杂"仍是教材推论。
- 同优先级任务争锁时的反转（教材 13.5 本该讲的内容）**未测**。

---

## 常见错误和陷阱

| 陷阱 | 症状 | 根因 | 处理 |
| --- | --- | --- | --- |
| 以为互斥量会拒绝"别人解锁" | 锁被打开，无任何报错 | `prvCopyDataToQueue` 不校验持有者（`queue.c:2405`） | 把它当"约定"守；需要硬检查就用递归锁 |
| 第三方替持有者 give | 计数回到 1，互斥永久失效，继承错乱 | holder 被清空（§3 链条） | 代码审查层面禁止；或改用递归锁（有校验） |
| 用互斥量做"事件通知" | take 一方永远不释放，其他任务全卡 | 互斥量的语义是"谁拿谁还"，不是"发一次事件" | 通知用信号量/任务通知（Day06/Day08） |
| 在 ISR 里 take 互斥量 | 断言失败，或继承完全失效 | ISR 路径不写 holder（`semphr.h:675` → `xQueueReceiveFromISR`） | ISR 用临界区或 `...FromISR` 版信号量 |
| 两个同优先级任务争同一把锁 | 偶尔仍出现长等待 | 继承条件是**严格小于**（`tasks.c:6661`） | 避免用优先级排序去解决同优先级争用；用别的机制 |
| 忘记 `configUSE_MUTEXES 1` | `xSemaphoreCreateMutex` 未定义/退化为信号量语义 | 编译期开关 | 本仓库 `:42` 已开；移植时先查这一项 |
| 递归锁 take/give 次数不等 | 锁永不放行 | `uxRecursiveCallCount` 未归零（`queue.c:786`） | 配对；把 take/give 放在同一函数的两端 |
| 普通 `xSemaphoreGive` 用于递归锁 | 锁状态不一致（计数不清零又不释放） | 该 API 不递减 `uxRecursiveCallCount` | 递归锁只用递归 API（`semphr.h:316` 的对称约束） |
| 用递归锁解交叉死锁 | 仍然死锁 | 可重入性解决的是"同一任务重复取"，不是"锁顺序" | 统一加锁顺序；或改成一次只持一把 |
| 把 405/105 写进判据 | 换个环境就假失败 | 跨运行漂移 3.86–4.0 | 判"相对关系"，不判绝对值 |
| 临界区里调用阻塞 API | 高优先级任务被拖到超时 | 取锁后又 `vTaskDelay`/`printf` 阻塞 | 临界区只包"对共享状态的判断与修改"（Day04 §5） |

---

## 故障排查方法

1. **"加了互斥量，但两个任务还是同时在用资源。"** 按顺序查：
   ① 是不是有人**没拿锁就进去**（漏了 take）；② 是不是**第三方 give 过**（§3 的链条，会留下"计数为 1"的痕迹）；
   ③ 是不是在 ISR 里访问了同一资源（ISR 拿锁形同无锁，§6）。
2. **"高优先级任务等锁很久，但看代码是互斥量。"** 先确认三件事：
   ① 确实是 `xSemaphoreCreateMutex()` 而不是 `xSemaphoreCreateBinary()`（两者只差一行，`F103_16` 就是比这个）；
   ② `configUSE_MUTEXES` 是 1；③ 中间任务是否在忙等（忙等即使有继承，也只是把持锁者提上来，不是消除争用）。
3. **"优先级看起来没恢复。"** 打印 `uxPriority` 与 `uxBasePriority` 两个值。
   若 `uxPriority` 高于 `uxBasePriority`，说明继承还在生效——可能还有别的任务在等同一把锁（§5 的
   `DisinheritAfterTimeout` 逻辑）。这不是泄漏，是"退到仍在等的最高优先级"。
4. **"用递归锁后锁死。"** 数 take/give 次数。最隐蔽的形态是**提前 return**：
   take 成功 → 中途 `if (...) return` → give 被跳过。用 `goto cleanup` 或单出口风格。
5. **"怀疑交叉死锁。"** FreeRTOS **没有死锁检测**，能做的只有：给每个任务打印"想拿锁 X / 拿到锁 X / 放锁 X"，
   跑一次卡住后看日志里最后一条"想拿"是谁。这不是诊断工具，是事后取证。
6. **"换成互斥量后 RAM 不够了。"** 每个互斥量比二值信号量多两个字段（holder + 递归计数），
   在 4 KB 堆上要算进账（Day02 §6 的账本）。

---

## 练习题

### 基础题

1. 用信号量做互斥的两个前提各约束谁？为什么这两个前提内核都没有强制？
2. 教材说"谁上锁谁释放：只是约定"。在源码里找到 give 路径不校验持有者的那一段，说明它做了什么、
   没做什么。
3. 优先级继承的判定条件是什么？同优先级的两个任务争锁会继承吗？为什么这个设计合理？

### 改错题

4. 下面这段用互斥量做"事件通知"，指出问题：

   ```c
   /* 生产者 */ xSemaphoreGive( g_mtx );
   /* 消费者 */ xSemaphoreTake( g_mtx, portMAX_DELAY );  do_work();
   ```

5. 下面这段从 ISR 里取互斥量，说明两处问题：

   ```c
   void EXTI0_IRQHandler(void) {
       BaseType_t woken = pdFALSE;
       if (xSemaphoreTake( g_mtx, 0 ) == pdTRUE) {
           g_shared++ ;
           xSemaphoreGive( g_mtx );
       }
       portYIELD_FROM_ISR( woken );
   }
   ```

6. 下面这段用递归锁保护一个函数，找出至少两处问题：

   ```c
   void cfg_set(int v) {
       xSemaphoreTakeRecursive( g_cfg, portMAX_DELAY );
       if (v < 0) return;                 /* 早退 */
       g_value = v;
       xSemaphoreGive( g_cfg );
   }
   void cfg_dump(void) {
       xSemaphoreTake( g_cfg, portMAX_DELAY );   /* 普通 API */
       printf("%d\n", g_value);
       xSemaphoreGive( g_cfg );
   }
   ```

### 综合题

7. 用 §3 的链条推演：A 持锁、B 替 A give、C 随后 take。要求写出每一步的
   计数、`xMutexHolder`、A/C 的 `uxPriority` 与 `uxBasePriority`，并说明第几步之后互斥已经失效。
   再回答：如果这中间 H（最高优先级）来 take，会发生什么？继承能救回来吗？

8. 设计一个实验，区分"继承真的发生了"与"这次刚好跑得快"。要求：
   ① 给出你要采集的至少两个量；② 说明为什么只看总耗时不够；
   ③ 给出一个"预期必须失败"的对照（提示：`F103_16` 已经给了骨架，你要说明负向验证怎么做）。

9. 教材 13.5 只剩标题。请你补上它的正文：两个**同优先级**任务争同一把互斥量时，
   ① 继承会发生吗？② 反转还会发生吗？③ 什么机制能缓解（提示：时间片与 `configUSE_TIME_SLICING`）？
   用 `F103_16` 的骨架改造一版验证你的判断。

---

## 验收标准

- [ ] 能说出用信号量做互斥的两个前提各约束谁，以及内核为什么不强制。
- [ ] 能在源码里指出 give 路径不校验持有者的位置（`queue.c:2405-2420`），
      并说明递归锁为什么相反（`queue.c:773`）。
- [ ] 能用"计数 / holder / 优先级"三样东西推演"第三方解锁"的完整后果链。
- [ ] 能说出继承的三个细节：严格小于、只改 `uxPriority`、只升不降。
- [ ] 能解释超时退出时为什么是"退到仍在等的最高优先级"而不是原值。
- [ ] 能说明 ISR 拿锁为什么继承不适用（holder 为 NULL 的机制原因）。
- [ ] 能引用 `F103_16` 的 `prio 1→3` 与"give 回 1"分别对应哪条规则。
- [ ] 能区分递归锁解决与不解决的两种死锁，并说出它的 `pdFAIL` 保护。
- [ ] 能列出本篇**未验证**的六项及原因。

---

## 延伸阅读

- 教材第 13 章：`rtos.txt:4927-5068`
  - 13.1 使用场合：4946–4990（非原子访问、函数重入、临界资源）
  - 13.2 函数：4991–5010（创建/删除/give/take 与 `configUSE_MUTEXES`）
  - 13.3 优先级继承：5011–5029（第 268/269 行对照与现象链）
  - 13.4 递归锁：5030–5066（死锁、自我死锁、三组函数）
  - 13.5 常见问题：5067（**只有标题，正文缺失**）
- 源码：`Source/queue.c` — `xQueueCreateMutex`（626）、递归 give/take（758–850）、
  take 的继承点（1786–1795）、超时恢复（1851–1872）、give 的恢复与清 holder（2405–2420）
- 源码：`Source/tasks.c` — `xTaskPriorityInherit`（6650）、`xTaskPriorityDisinherit`（6753）、
  `pvTaskIncrementMutexHeldCount`（7677）
- 本仓库实验：`F103_16`（两轮对照）；`F103_10`–`F103_14` 的日志锁
- 相关笔记：`Day04`（四问表的第 4 问）、`Day06`（二值信号量为什么做不到这件事）、
  `Day08`（事件组的位操作）、`Day15`/`Day14`（教材缺陷记账的口径）
- 下一篇：`Day08_事件组与任务通知.md`（教材第 14–15 章）

---

**验证记录**

| 日期 | 环境一 | 命令一 | 结果一 | 环境二 | 命令二 | 结果二 | 未覆盖 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 2026-10-30 | 教材抽取文本 | 读 `rtos.txt:4927-5068` | 13.1–13.5 全章；两个前提（4928-4937）、"谁上锁谁释放只是约定"（4942-4944）、继承规则（4996-5004）、第 268/269 行单变量对照（5008-5022）、现象链（5024-5028）、两类死锁与递归锁（5032-5062）、13.5 标题残留"7.8"且正文为空 | 本仓库源码 | `grep`/`sed` 读 `queue.c`、`tasks.c`、`semphr.h`、`FreeRTOSConfig.h` | give 不校验持有者（`queue.c:2405-2420`）；递归锁校验（`:773`、`:833`，不符 `pdFAIL`）；继承严格小于（`tasks.c:6661`）；超时用 `DisinheritAfterTimeout` + 仍在等的最高优先级（`queue.c:1851-1872`）；`xSemaphoreTakeFromISR` 存在（`semphr.h:675`）⇒ 教材 13.2.2 印错函数名成立 | 递归锁、交叉死锁、自我死锁、同优先级反转均无实验 |
| 2026-10-30 | WSL2 + QEMU 10.2.1 | `qemu-system-arm -M stm32vldiscovery -kernel f103_16_priority_inversion.elf` | `mutex` 轮 H 等锁 105 / L 持锁 105 / L 阶段一 52 / `prio 1→3`（give 回 1）；`binary-sem` 轮 405 / 404 / 354 / prio 恒 1；比值 3.9×（另一次 412/410 vs 103/104）；栈水位 82/192、58/160、64/160 全过 | 本仓库源码 | `grep -rn xSemaphoreCreateMutex` | 五处日志锁（`F103_10:166`、`F103_11:241`、`F103_12:349`、`F103_13:416`、`F103_14:417`）；**无**递归锁使用点 | "去掉日志锁会混杂"未对照；"B 替 A 解锁"后果链是源码推理，未做用例 |
