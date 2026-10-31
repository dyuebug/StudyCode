# FreeRTOS 任务与调度：速查表

> **一句话定义**：任务是一个带独立栈、由调度器按优先级抢占执行、可被阻塞/挂起/延时的无限循环函数；
> 调度器在每次 tick 或显式让出时选"最高优先级的就绪任务"运行。
>
> **本仓库配置**：`configUSE_PREEMPTION 1`、`configUSE_TIME_SLICING 1`、`configMAX_PRIORITIES 5`、
> `configTICK_RATE_HZ 1000`、`configMINIMAL_STACK_SIZE 128`、`configTOTAL_HEAP_SIZE 4 KB`、heap_4、
> `configSUPPORT_STATIC_ALLOCATION 0`（⇒ 本仓库只能动态创建任务）。
> 来源：`experiments/F103_common/vendor/freertos/include/FreeRTOSConfig.h:14-36`。

---

## 1. 使用场景

| 场景 | 该不该拆任务 | 理由 |
| --- | --- | --- |
| 周期性采集 + 数据处理 + 通信 | ✅ 拆 | 三者阻塞点不同，单循环会让慢操作拖住采样 |
| 只有一条直线逻辑、无阻塞等待 | ❌ 不拆 | 任务切换有开销，裸机超级循环更简单 |
| 需要严格等间隔执行 | ✅ 拆，且用 `xTaskDelayUntil` | `vTaskDelay` 会累积漂移 |
| 中断里做长处理 | ❌ 中断只搬运，处理交给任务 | ISR 里不能阻塞，且会拉长中断延迟 |

**优先级分配口径**：数值越大优先级越高（`configMAX_PRIORITIES 5` ⇒ 合法值 0–4，0 是空闲任务）。
按"截止期越短、越不能等"排高，而不是按"逻辑上更重要"排。

---

## 2. 调度边界

- **抢占**：更高优先级任务就绪的那一刻（tick 中断或 `FromISR` + `portYIELD_FROM_ISR`）就切换，
  不等当前任务让出。
- **时间片**：同一优先级的多个就绪任务按 tick 轮转，每个吃满 1 个 tick（`1000 Hz` ⇒ 1 ms）后换人。
  这是 `configUSE_TIME_SLICING 1` 的行为；置 0 则同优先级不轮转。
- **空闲任务**：优先级 0，`configMINIMAL_STACK_SIZE` 128 字。它必须存在——删任务时释放内存靠它。
- **调度器不返回**：`vTaskStartScheduler()` 成功后永远不返回；返回了就是堆不足或创建空闲任务失败。

---

## 3. 接口卡片

### `BaseType_t xTaskCreate( pvTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxCreatedTask )`

作用：动态创建任务，栈与 TCB 从 FreeRTOS 堆分配。
参数：`usStackDepth` 单位是**字（word，4 字节）**，不是字节；`uxPriority` 0–4。
返回值和错误：`pdPASS` / `pdFAIL`（堆不够）。**必须检查**——失败时任务根本不存在，后续句柄为空。
资源所有权：任务自己 `return` 不得调用（会走 `vTaskDelete(NULL)` 的未定义路径）；应在末尾写 `vTaskDelete(NULL)`。
常见误用：按字节填栈深（填 128 以为 128 字节，实际 512 字节）；不查返回值。

### `void vTaskDelay( TickType_t xTicksToDelay )`

作用：相对延时——从**调用时刻**起睡 N 个 tick。
返回值和错误：无返回值。传 `0` 等价于 `taskYIELD()`，不是"睡 0"。
常见误用：在周期循环里用它做节拍 ⇒ 每轮的实际周期 = 周期 + 执行时间，**误差累积**。

### `BaseType_t xTaskDelayUntil( TickType_t *pxPreviousWakeTime, TickType_t xTimeIncrement )`

作用：绝对延时——保证"上一次唤醒时刻 + N"，执行时间被吸收进周期内，**不累积漂移**。
返回值和错误：没跟上节拍时返回 `pdFALSE`（说明已经迟了，本周期被跳过）。
常见误用：`*pxPreviousWakeTime` 忘记由函数自己更新，手动改它会算错。

### `UBaseType_t uxTaskGetStackHighWaterMark( TaskHandle_t xTask )`

作用：返回任务运行至今**剩余栈的最小值**，单位**字**。
返回值和错误：0 表示已经贴底（可能已写穿）。传 `NULL` 表示当前任务。
资源所有权：只读，不得用于任何分配决策。
常见误用：刚创建就读（此时还没走到最深路径，数字虚高）；把它当"已用栈"（它是**剩余**）。

### `void vTaskSuspend( TaskHandle_t )` / `void vTaskResume( TaskHandle_t )`

作用：挂起/恢复。挂起的任务不参与调度，且**无法被自身恢复**。
常见误用：用挂起代替同步——挂起不会让出锁，也不携带数据。

### `void vTaskDelete( TaskHandle_t xTask )`

作用：删除任务。传 `NULL` 删除自己。
资源所有权：被删任务的栈和 TCB 由**空闲任务**回收；空闲任务被饿死时内存不回收。
常见误用：删掉别的任务后立刻以为内存已释放。

---

## 4. 最小示例

```c
/* 周期任务：注意 xTaskDelayUntil 的初值和返回值 */
static void rx_task(void *arg)
{
    TickType_t last = xTaskGetTickCount();
    for (;;) {
        do_work();
        if (xTaskDelayUntil(&last, pdMS_TO_TICKS(10)) == pdFALSE) {
            g_miss++;                    /* 已经迟了，本周期被跳过——要计数，不要静默 */
        }
    }
    vTaskDelete(NULL);                   /* 循环不会退出，退出路径必须显式删自己 */
}

BaseType_t ok = xTaskCreate(rx_task, "rx", 160, NULL, 3, &s_rx);
configASSERT(ok == pdPASS);              /* 不检查返回值 = 任务可能根本没建起来 */
```

---

## 5. 常见错误与陷阱

| 陷阱 | 症状 | 根因 | 处理 |
| --- | --- | --- | --- |
| 栈深单位填字节 | 栈瞬间溢出，钩子报任务名 | `usStackDepth` 是**字** | 字 = 字节 / 4 |
| 不查 `xTaskCreate` 返回值 | 句柄为空，后续操作静默失败 | 4 KB 堆下多建一个任务就可能 `pdFAIL` | `configASSERT` 或钩子 |
| 用 `vTaskDelay` 做周期 | 周期越跑越长 | 相对延时累加执行时间 | 换 `xTaskDelayUntil` |
| 刚建完就读栈水位 | 余量看着很足，跑一段就溢出 | 水位是历史最低，还没走到最深 | 全路径跑完再读 |
| 同优先级任务互相饿死 | 只有部分任务在跑 | 无阻塞点 + `configUSE_TIME_SLICING 0` | 开时间片或加阻塞点 |
| 任务函数里 `return` | 行为未定义 | 任务不允许返回 | 末尾 `vTaskDelete(NULL)` |

---

## 6. 调试方法

1. **看栈**：`uxTaskGetStackHighWaterMark()` 逐任务打印。判据用**绝对 + 相对**双门限
   （本仓库口径：绝对 ≥32 字且相对 ≥25%），单看绝对值会放过"大栈任务只剩 40 字"。
2. **看堆**：`xPortGetFreeHeapSize()` 与 `xPortGetMinimumEverFreeHeapSize()`。前者是当前余量，
   后者是历史最低——**判断"会不会溢出"要看后者**。
3. **看周期**：在任务里打印 `xTaskGetTickCount()` 的相邻差，与目标周期比。差值是"每轮多花的 tick"。
4. **看任务是否真的在跑**：每轮递增一个计数并在外部打印。**"没输出"和"没在跑"是两件事**——
   先确认串口通，再判断任务死。
5. 本仓库 `configUSE_TRACE_FACILITY 0`、`configGENERATE_RUN_TIME_STATS 0`
   ⇒ **`vTaskList()` / `vTaskGetRunTimeStats()` 编译不出来**，只能靠上面四项手工观测。

---

## 7. 与相近技术的对比

| 对比项 | FreeRTOS 任务 | 裸机超级循环 | 中断 |
| --- | --- | --- | --- |
| 并发性 | 真并发（抢占） | 伪并发（顺序） | 抢占但不可阻塞 |
| 栈 | 每任务独立 | 共用主栈 | 共用被中断者的栈 |
| 阻塞等待 | ✅ | ❌ 只能轮询 | ❌ 绝对禁止 |
| 开销 | 每 tick 切换 + 每任务栈 | 无 | 入栈出栈 |
| 适用 | 多阻塞点、有截止期 | 单一直线逻辑 | 短小、时间确定 |

**与 pthread 的差别**：FreeRTOS 任务是**静态优先级 + 可选时间片**，没有 `pthread_cond_wait` 那种
"释放锁并睡眠"的原子语义（要用队列/信号量表达）；栈大小在创建时固定，不能增长。

---

## 8. 验证状态与边界

| 结论 | 级别 | 证据 |
| --- | --- | --- |
| 任务创建/优先级/延时/栈水位可观测 | **实测** | `tests/qemu-stm32/logs/exp10_freertos.log`：栈水位 154/126/90 字；`produced=consumed=85`；6 s 内 88 条消费 |
| 堆随任务创建下降 | **实测** | 同上：`heap_free` 1112 → 496 B（4 KB 堆） |
| 同优先级时间片轮转 | **源码核对** | `configUSE_TIME_SLICING 1` 已确认，但**未做同优先级双任务的对照实验** |
| `xTaskDelayUntil` 抖动 | **未验证** | 需真板与逻辑分析仪；QEMU 时基有 2.9 倍失真，周期数字不可外推 |
| 静态创建任务 | **未实现** | `configSUPPORT_STATIC_ALLOCATION 0`，本仓库**不具备**该能力 |
| 真板抢占时延 | **未验证** | QEMU 无中断延迟模型 |

**明确不能说的话**：不能说"本仓库验证了 FreeRTOS 调度器的实时性"——只验证了任务的创建、优先级与栈/堆可观测量。

---

## 证据来源

- 配置：`experiments/F103_common/vendor/freertos/include/FreeRTOSConfig.h:14-36`
- 实测日志：`tests/qemu-stm32/logs/exp10_freertos.log`
- 实验：`experiments/F103_10_FreeRTOS_Tasks/`
- 讲义：`notes/FreeRTOS入门/Day03_任务管理.md`
- 相关：[FreeRTOS 中断与临界区](FreeRTOS中断与临界区_速查表.md)、[FreeRTOS 内存管理](FreeRTOS内存管理_速查表.md)
