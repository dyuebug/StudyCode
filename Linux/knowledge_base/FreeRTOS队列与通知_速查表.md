# FreeRTOS 队列 / 事件组 / 任务通知：速查表

> **一句话定义**：三者都是"任务间传值或传状态"的机制，区别在**传什么、给谁、能不能多对多**——
> 队列传**数据拷贝**（多对多）、事件组传**位状态**（多对多、可"与/或"等）、任务通知传**一个 32 位值**
> （点对点，最轻）。
>
> **本仓库配置**：`configUSE_TASK_NOTIFICATIONS 1`、`configUSE_COUNTING_SEMAPHORES 1`、
> **`configUSE_QUEUE_SETS 0`（⇒ 队列集在本仓库编译不出来）**、
> `configUSE_TIMERS = F103_FREERTOS_USE_TIMERS`（`xEventGroupSetBitsFromISR` 依赖它）。
> 来源：`FreeRTOSConfig.h:40-55`。

---

## 1. 选型：先问三个问题

| 问题 | 队列 | 事件组 | 任务通知 |
| --- | --- | --- | --- |
| 要传的是数据还是状态？ | 数据（有长度、有序） | 状态（位） | 状态或少量数据（1 个 32 位值） |
| 多对多还是点对点？ | 多对多 | 多对多 | **点对点**（只能发给指定任务） |
| 需要"等 N 个条件同时满足"？ | ❌ | ✅ `xEventGroupWaitBits` 的"与" | ❌ 只判 pending，不判位组合 |
| 相对开销 | 中（拷贝 + 链表） | 中 | **最低**（直接写目标任务 TCB） |

**默认建议**：一对一传值优先任务通知；多生产者或需要缓冲的用队列；需要"等 A 且 B"用事件组。

---

## 2. 语义边界

- **队列是拷贝语义**：`xQueueSend` 把数据复制进队列存储区。发大结构体就是真拷贝——
  大于 8–16 字节时考虑传指针（但指针的生命周期要自己管）。
- **队列长度固定**：满了 `xQueueSend` 阻塞到 `xTicksToWait`；ISR 版没有等待参数，满了直接失败。
- **事件组的位是 24 位可用**（高 8 位被内核保留）。
- **事件组 ISR 版是异步的**：`xEventGroupSetBitsFromISR` 返回 `pdPASS` 只表示**请求已排进 daemon 队列**，
  位还没置上、等待者还没被唤醒——要等定时器服务任务跑一次。
- **任务通知的清除点有两个**：`ulBitsToClearOnEntry`（**仅当状态不是 pending 时**执行）与
  `ulBitsToClearOnExit`（取出后执行）。前者"没数据才清"是本意——"我想等新事件，先把旧数据抹掉"。
- **只有一份通知值**：同一任务被通知两次而未取走时，第二次按 `eAction` 覆盖或置位，**不是排队**。

---

## 3. 接口卡片

### `QueueHandle_t xQueueCreate( UBaseType_t uxQueueLength, UBaseType_t uxItemSize )`

作用：动态创建队列。存储区 = `uxQueueLength × uxItemSize`，从 FreeRTOS 堆分配。
返回值和错误：`NULL` = 堆不够（4 KB 堆下很容易）。
常见误用：`uxItemSize` 传成元素个数。

### `BaseType_t xQueueSend( q, pvItemToQueue, xTicksToWait )` / `xQueueReceive( q, pvBuffer, xTicksToWait )`

作用：入队/出队，拷贝语义。
返回值和错误：`pdPASS` / `errQUEUE_FULL` / `errQUEUE_EMPTY`。
常见误用：把 `errQUEUE_FULL` 当成"出错"去断言——队列满在生产者快于消费者时是**正常背压**。

### `BaseType_t xQueueSendFromISR( q, pvItem, pxHigherPriorityTaskWoken )`

作用：中断内入队。
参数：`pxHigherPriorityTaskWoken` 必须初始化 `pdFALSE`；函数可能置 `pdTRUE`。
返回值和错误：ISR 版**没有等待参数**，满了返回 `errQUEUE_FULL`（记为丢弃，要计数）。
**配套**：ISR 末尾必须 `portYIELD_FROM_ISR(x)`，否则"更高优先级任务就绪"要等下一个 tick 才生效。

### `EventGroupHandle_t xEventGroupCreate( void )` / `xEventGroupSetBits` / `xEventGroupWaitBits`

作用：创建/置位/等待事件位。
`xEventGroupWaitBits(eg, bits, xClearOnExit, xWaitForAllBits, ticks)`：
- `xWaitForAllBits = pdTRUE` ⇒ **与**语义（全部置上才返回）；`pdFALSE` ⇒ **或**语义。
- 返回值和错误：返回**返回时的位快照**；超时也返回当前快照，必须自己判断是否满足。
- `xClearOnExit` 只清除**等待的那几位**，不影响其它位。
常见误用：超时后不看返回值就当成功。

### `BaseType_t xEventGroupSync( eg, set, wait, ticks )`

作用：N 个任务"各自到达后再一起走"（会合点）。置 `set` 位 → 等 `wait` 位全齐 → 自动清。
常见误用：与"信号量计数"混用——它保证的是**全部到达**，不是"第 N 个到达"。

### `BaseType_t xTaskNotify( h, ulValue, eAction )` / `xTaskNotifyWait( clearOnEntry, clearOnExit, &val, ticks )`

作用：点对点通知。`eAction` 的五种取值决定"覆盖 / 置位 / 递增"。
返回值和错误：`xTaskNotifyWait` 返回 `pdPASS`（拿到）/ `pdFAIL`（超时）。
常见误用：① 期望"通知会排队"（只有一份值）；② 对 `ulBitsToClearOnEntry` 期待"每次都清"（只在非 pending 时清）。

### `uint32_t ulTaskNotifyTake( BaseType_t xClearCountOnExit, TickType_t ticks )`

作用：把通知当"信号量"用。`pdTRUE` ⇒ 取出后清零（二进制语义）；`pdFALSE` ⇒ 减一（计数语义）。
返回值和错误：返回**清零或减一之前**的值；超时返回 0。

---

## 4. 最小示例

```c
/* 生产者：任务上下文 */
if (xQueueSend(s_q, &sample, pdMS_TO_TICKS(10)) != pdPASS) {
    g_drop++;                        /* 背压：消费者跟不上，要计数不要断言 */
}

/* 消费者：ISR 侧必须带 higher-priority-woken 且末尾 yield */
void USART1_IRQHandler(void)
{
    BaseType_t woken = pdFALSE;
    uint8_t ch;
    if (uart_rx_ready()) {
        ch = uart_read();
        (void)xQueueSendFromISR(s_q, &ch, &woken);   /* 满则丢弃，返回值不能忽略 */
    }
    portYIELD_FROM_ISR(woken);       /* 漏了这句：就绪的高优先级任务要等下一个 tick */
}

/* "等 A 且 B"——只有事件组能表达 */
EventBits_t b = xEventGroupWaitBits(s_eg, BIT_A | BIT_B,
                                    pdTRUE /*清*/, pdTRUE /*与*/, pdMS_TO_TICKS(200));
if ((b & (BIT_A | BIT_B)) != (BIT_A | BIT_B)) {
    /* 超时：b 是当时的快照，不是错误码 */
}
```

---

## 5. 常见错误与陷阱

| 陷阱 | 症状 | 根因 | 处理 |
| --- | --- | --- | --- |
| 把 `errQUEUE_FULL` 当致命错误 | 生产者一快就复位 | 满队列是正常背压 | 计数 + 弃帧策略 |
| 漏 `portYIELD_FROM_ISR` | 高优先级任务延迟到下一 tick | 就绪了但没触发切换 | ISR 末尾必调 |
| `xHigherPriorityTaskWoken` 没初始化 | 偶然切换/不切换 | 初值未定 | 入口 `pdFALSE` |
| 以为 `SetBitsFromISR` 同步生效 | 中断返回后等待者没动 | 走 daemon 队列，异步 | 检查 `configUSE_TIMERS` 与 `INCLUDE_xTimerPendFunctionCall` |
| 在无定时器的工程里用 `SetBitsFromISR` | **链接失败** undefined reference | 条件编译整段去掉 | 两个宏都要开 |
| 以为任务通知能排队 | 连发 5 次只处理 1 次 | 只有一份通知值 | 需要队列语义就用队列 |
| 用任务通知做"位组合"等待 | 判不出来 | 它只判 pending，不判位 | 用事件组 |
| 在 ISR 里等队列 | 死机 / 断言 | ISR 不能阻塞 | 用 `FromISR` 版 |

---

## 6. 调试方法

1. **先分清"没发出去"还是"没收到"**：在发端与收端各加一个计数，跑完一起打印。
   只有发送数、没有接收数 ⇒ 收端没跑或阻塞在别处。
2. **背压要留证据**：丢弃计数（`g_drop`）必须存在并打印。**没有丢弃计数的队列实现，无法判断是否丢过数据。**
3. **观察队列深度**：`uxQueueMessagesWaiting()` 周期打印；长期贴满说明消费者太慢，长期为 0 说明生产者太慢。
4. **事件组超时**：记录"等了多久才超时"，与门限比。本仓库实测 `event timeout waited=300`（门限 300）。
5. **通知漏收**：在同一任务里打印每次取到的值，检查是否符合"覆盖/置位"的预期（不是排队）。

---

## 7. 与相近技术的对比

| 对比 | 队列 | 事件组 | 任务通知 |
| --- | --- | --- | --- |
| 传数据 | ✅ 拷贝 | ❌ 只有位 | ✅ 一个 32 位值 |
| 缓冲 | ✅ 长度可配 | ❌ | ❌ |
| 多对多 | ✅ | ✅ | ❌ 点对点 |
| 与/或等待 | ❌ | ✅ | ❌ |
| ISR 可用 | ✅ `FromISR` | ⚠️ 依赖定时器服务任务 | ✅ `vTaskNotifyGiveFromISR` |
| 相对开销 | 中 | 中 | 最低 |

**与 Linux 的对比**：队列 ≈ 有限长消息队列（但固定长度、拷贝语义）；事件组 ≈ 位图 + `poll`；
任务通知 ≈ 每任务一个 `eventfd`。**都不等价**：FreeRTOS 没有"消息队列 + 优先级队列"的组合。

---

## 8. 验证状态与边界

| 结论 | 级别 | 证据 |
| --- | --- | --- |
| 队列生产/消费计数一致 | **实测** | `exp10_freertos.log`：`produced=consumed=85`、6 s 内 88 条消费 |
| ISR → 队列的截断算术 | **实测** | `exp11_*.log`：`rx` 0→7→14、`trunc=4` 且 `300 = 4×64 + 44` 自洽 |
| 事件组"与"等待 | **实测** | `exp12_sync.log`：`event wait_all got=0x03 waited=200` |
| 事件组超时路径 | **实测** | 同上：`event timeout waited=300` |
| `xEventGroupSetBitsFromISR` 的异步语义 | **未实测** | 讲义为源码核对；本仓库 `configUSE_TIMERS` 按目标开关，**未单独跑过该路径** |
| 队列集 | **不可用** | `configUSE_QUEUE_SETS 0` ⇒ 编译不出 `xQueueCreateSet` 系列 |
| 任务通知的五种 `eAction` | **源码核对** | 讲义 Day08 §12；**无运行读数** |
| `xTaskNotifyWait` 的 `ulBitsToClearOnEntry` 时机 | **源码核对** | 讲义 Day08 §13；无运行读数 |

---

## 证据来源

- 配置：`experiments/F103_common/vendor/freertos/include/FreeRTOSConfig.h:40-55`
- 实测日志：`tests/qemu-stm32/logs/exp10_freertos.log`、`exp11_*.log`、`exp12_sync.log`
- 实验：`experiments/F103_10_FreeRTOS_Tasks/`、`F103_11_FreeRTOS_ISR_Queue/`、`F103_12_FreeRTOS_Sync/`
- 讲义：`notes/FreeRTOS入门/Day05_队列与队列集.md`、`Day08_事件组与任务通知.md`
- 相关：[FreeRTOS 同步语义与优先级反转](FreeRTOS同步语义与优先级反转_速查表.md)、[FreeRTOS 任务与调度](FreeRTOS任务与调度_速查表.md)
