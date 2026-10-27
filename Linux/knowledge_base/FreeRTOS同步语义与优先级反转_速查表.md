# FreeRTOS 同步语义与优先级反转：速查表

**学习目标**：在选同步原语时先能判断"它有没有持有者、会不会继承、能不能从 ISR 释放"，再去查 API。  
**适用范围**：FreeRTOS-Kernel V10–V11，`configMAX_PRIORITIES = 5`、`configUSE_MUTEXES = 1`、`configUSE_PREEMPTION = 1` 的 Cortex-M3 配置。  
**验证状态**：接口行为与读数**实测**（QEMU `stm32vldiscovery`）；中断延迟、临界区时长、真板复位为**未验证**。
证据见文末"证据来源"。

## 1. 先选语义，再选 API

| 需求 | 用哪个 | 关键约束 |
| --- | --- | --- |
| 互斥（保护共享资源） | `xSemaphoreCreateMutex()` | 有持有者、有优先级继承；**只能任务上下文给/取** |
| 同一任务可重复进入 | `xSemaphoreCreateRecursiveMutex()` + `...RecursiveTake/Give` | 每层都要配一次 Give |
| 任务 ↔ ISR 的单次事件 | `xSemaphoreCreateBinary()` + `xSemaphoreGiveFromISR()` | 无持有者、无继承；可被中断合并 |
| 计数 / 资源池 | `xSemaphoreCreateCounting(max, init)` | 比 N 个二值信号量省堆（实测省约 176 B/3 个） |
| 多事件"与/或" | `xEventGroupCreate()` + `xEventGroupWaitBits(..., xClearOnExit, xWaitForAllBits, ...)` | 位掩码；超时与"已满足"要分开读 |
| 线程安全队列 | `xQueueCreate()` + `xQueueSendFromISR()` | 拷贝语义；队列满要显式处理 |

**二值信号量当锁的三个缺口**（`F103_16` 实测对照）：

| 能力 | 二值信号量 | 互斥量 |
| --- | --- | --- |
| 所有权 | 无：任何任务都能 `give`，误 give 直接破坏互斥 | 有：只有持有者能 `give` |
| 优先级继承 | 无 | 有：等待时把持有者提到等待者的最高优先级 |
| 递归 | 无（同任务重复 take 自锁） | 无（需 Recursive 版本） |

## 2. 优先级反转：成因、读数、继承的边界

三个任务的优先级一摆好，反转就已成立：

```text
prio 3  high ──取锁──► 阻塞（锁在 low 手上）──────────────► 拿到锁
prio 2  mid  ──[纯忙等]──────────────────────────────► 结束
prio 1  low  ──持锁──► [拿不到 CPU] ─────────────────► 释放
```

`F103_16` 同一骨架只换锁类型（单位 tick，实测；两次运行的区间见括注）：

| 锁 | H 等锁 | L 持锁 | L 阶段一 | M 开始 | L 阻塞期 prio | L 释放前 prio |
| --- | --- | --- | --- | --- | --- | --- |
| 二值信号量 | 405（412） | 404 | 354 | 2 | 1 | 1 |
| 互斥量 | 105（103） | 105 | 52 | 2 | **3** | **3** |

两条读数跨运行会漂移：等锁 405–412 / 103–105（比值 3.86–4.0），栈水位 ±8 字。
所以判据只能是"`binary > 2 × mutex`"和"余量 ≥32 字且 ≥25%"这类宽松形式，写死精确值必然假失败。

三条必须记住的边界：

1. **继承提升的是持锁者，不是等待者**。`low` 的两段 `vTaskDelay(50)` 一分都没变短，
   所以 `high` 的下界仍是"持锁者剩余工作量"（实测 105 tick，不是 0）。
2. **`mid` 一旦让出 CPU，反转就消失**。它必须是纯忙等/长计算；`vTaskDelay`、`yield`、
   阻塞在任何对象上都会让实验变成"什么都没测"。
3. **`give` 内部就完成降级**。因此"释放前读优先级"得 3、"释放后读"得 1——
   观测点选错会让继承看起来"没发生"。

## 3. 接口卡片

### `xSemaphoreCreateMutex()` / `xSemaphoreCreateBinary()` / `xSemaphoreCreateCounting(max, init)`

- **返回**：句柄；堆不足返回 `NULL`（必须检查）。互斥量创建即"未持有"，二值信号量创建即"空"。
- **资源**：从 FreeRTOS 堆分配一个 queue 结构（4 KB 堆下约 80 B/个），`vSemaphoreDelete()` 归还。
- **误用**：二值信号量创建后忘记先 `give`，第一个 `take` 直接阻塞（现场表现为"任务不跑"）。

### `xSemaphoreTake(sem, ticks)` / `xSemaphoreGive(sem)`

- **返回**：`pdTRUE` / `pdFALSE`。`portMAX_DELAY` 表示无限等。
- **上下文**：`Give` 不能在 ISR 里用；ISR 只能用 `...FromISR` 变体，且**互斥量不支持**从 ISR 释放。
- **误用**：持有互斥量期间做长阻塞操作，会把整条等待链一起提权，掩盖真正的设计问题。

### `uxTaskPriorityGet(task)` 

- **作用**：读任务**当前**优先级（含继承提升）。`NULL` 表示当前任务。
- **用途**：继承的直接证据（1 → 3 → 1），比"变快了"这种间接推断可靠。

### `xSemaphoreGiveFromISR(sem, pxHigherPriorityTaskWoken)` + `portYIELD_FROM_ISR(x)`

- **作用**：中断里释放信号量；若唤醒了更高优先级任务，退出中断时立刻切换。
- **约束**：ISR 的优先级数值必须 ≥ `configMAX_SYSCALL_INTERRUPT_PRIORITY`（数值越大优先级越低），
  否则会破坏内核临界区。本仓库 `USART1_IRQ_PRIO` 取 5，与其相等。
- **误用**：忘记 `portYIELD_FROM_ISR`，任务要等下一个 tick 才被调度（延迟被放大到 1 tick）。

### `xTaskGetStackHighWaterMark(NULL)`

- **判据（本仓库约定）**：绝对余量 ≥ 32 字、相对余量 ≥ 25%，**压线不算通过**。
- **时机**：任务跑完全部路径后再读。任务已删除或还没跑到最深路径时读到的数字无意义。

## 4. 常见错误与调试

| 现象 | 先查什么 | 判据 |
| --- | --- | --- |
| 高优先级任务超时 | 持锁者是否被中优先级饿死 | 持锁者"阶段一完成"被推迟到中优先级忙等之后 |
| 两轮对照读数几乎一样 | 中断轮里持锁者的实时优先级 | 无继承应为 1，有继承应为等待者的优先级 |
| 日志停在半截、像调度器卡死 | `[MALLOC FAILED]` / `[STACK OVERFLOW]` | 堆余量、任务栈总量、`xTaskCreate` 返回值 |
| 栈水位读到 0 / 很小的值 | 任务是否已结束才读 | 完成事件是否到齐 |
| 二值信号量"锁"莫名失效 | 是否有非持有者 `give` | 二值信号量没有所有权概念，必须靠代码纪律 |

**调试顺序**：先确认钩子有没有停机（`vApplicationMallocFailedHook` /
`vApplicationStackOverflowHook` 里若是死循环，现场会静在原地）→ 再看时间戳顺序 →
最后才怀疑调度逻辑。

## 5. 验证状态与边界

- **实测**：二值信号量与互斥量的等待时间差（405 → 105 tick，另一次 412 → 103）、继承提升与恢复、
  栈水位判据、`configCHECK_FOR_STACK_OVERFLOW = 2` 的钩子触发、`xSemaphoreGiveFromISR` 合并行为。
- **仅构建**：`IWDG` 初始化/喂狗寄存器写入（QEMU 未建模）。
- **未验证**：中断延迟、临界区长度、WCET；tick 与真实时间的比例（QEMU 时基约 2.90 倍，
  只能比 tick 之间的比例，不能换算成秒）。
- **不覆盖**：死锁（两把锁反序获取）、继承链（多级 mutex）。

## 证据来源

- [`experiments/F103_16_FreeRTOS_PriorityInversion`](../experiments/F103_16_FreeRTOS_PriorityInversion/README.md)：两轮对照、栈水位、判定器（含 4 条负向验证）。
- [`experiments/F103_12_FreeRTOS_Sync`](../experiments/F103_12_FreeRTOS_Sync/README.md)：信号量、事件组、`giveFromISR`、超时路径。
- [`experiments/F103_15_FreeRTOS_StackOverflow`](../experiments/F103_15_FreeRTOS_StackOverflow/README.md)：栈判据与钩子触发。
- [`experiments/F103_14_FreeRTOS_Watchdog`](../experiments/F103_14_FreeRTOS_Watchdog/README.md)：监督任务优先级过低会看不见忙等。
- 日志：`tests/qemu-stm32/logs/exp16_priority_inversion.log`、`exp12_sync.log`、`exp15_stackoverflow.log`。
