# FreeRTOS 软件定时器：速查表

> **一句话定义**：软件定时器是内核用**一个**定时器服务任务（daemon task）按 tick 管理的一组"到期就调回调"
> 的软件对象；它不占用硬件定时器，但**回调跑在 daemon 任务里，不能阻塞**。
>
> **本仓库配置**：`configUSE_TIMERS = F103_FREERTOS_USE_TIMERS`（**按目标开关**）、
> `configTIMER_TASK_PRIORITY = configMAX_PRIORITIES - 1 = 4`、`configTIMER_QUEUE_LENGTH 5`、
> `configTIMER_TASK_STACK_DEPTH 160`。来源：`FreeRTOSConfig.h:55-63`。

---

## 1. 使用场景

| 场景 | 用软件定时器？ | 理由 |
| --- | --- | --- |
| 周期 10 ms 以上的超时/心跳 | ✅ | 精度一个 tick，实现简单 |
| 微秒级或严格抖动要求 | ❌ | tick 精度（本仓库 1 ms），且回调受其它任务抢占 |
| 回调里要做阻塞操作（等队列/延时） | ❌ | 会卡死整个 daemon，**所有**定时器一起失效 |
| 只需要"延时一段时间后做一次" | ⚠️ 可用 `vTaskDelay` | 定时器更灵活但要额外栈与队列 |

**判据**：只要回调里出现任何可能阻塞的调用，就应改成"定时器只发信号，工作交给任务"。

---

## 2. 状态机与"谁启动它"

```
   xTimerCreate()
        │
        ▼
  【冬眠 Dormant】◄──────────── xTimerStop()
        │                              ▲
        │ xTimerStart / xTimerReset    │
        ▼                              │
  【运行 Active】──到期──► 回调 ──┬──► 周期型：自动重装，回 Active
                                 └──► 单次型：回冬眠
```

**三个反直觉的边界**：

1. **`xTimerCreate` 创建出来的是冬眠态**，不会自己跑。必须显式 `Start` / `Reset` / `ChangePeriod`。
2. **单次定时器到期后自动回冬眠**（内核清 `tmrSTATUS_IS_ACTIVE`），但句柄仍有效，`xTimerReset` 可复用。
3. **`xTimerChangePeriod` 在冬眠态时会顺带启动**——一条命令等于"改周期 + 启动"。

---

## 3. 接口卡片

### `TimerHandle_t xTimerCreate( pcTimerName, xTimerPeriodInTicks, uxAutoReload, pvTimerID, pxCallback )`

作用：创建定时器，**创建后处于冬眠态**。
参数：`xTimerPeriodInTicks` 是 **tick 数**，不是毫秒——要用 `pdMS_TO_TICKS(300)`；直接填 `300` 在
1000 Hz 下恰好是 300 ms，但在别的 tick 率下就错。`uxAutoReload` `pdTRUE` = 周期型。
返回值和错误：`NULL` = 堆不够。
常见误用：以为创建即运行；周期填毫秒原值。

### `BaseType_t xTimerStart( t, xTicksToWait )`

作用：冬眠 → 运行。
参数：**`xTicksToWait` 是"把命令写进命令队列"的超时，不是定时器周期**（见 §5）。
返回值和错误：`pdPASS` = 命令入队成功；`pdFAIL` = 在等待时间内没排进队列（队列满）。
常见误用：把它理解成"等多久定时器才启动"。

### `BaseType_t xTimerStop( t, xTicksToWait )`

作用：运行 → 冬眠。已在冬眠时无变化（不是错误）。

### `BaseType_t xTimerReset( t, xTicksToWait )`

作用：重新确定超时时刻为 **`tX + n`**（从现在起重新计时）。
冬眠态调用等价于 `Start`；运行态调用等于"把到期时间往后推"。
典型用途：看门狗——每收到一帧就 `Reset`，断流自然到期。

### `BaseType_t xTimerChangePeriod( t, xNewPeriod, xTicksToWait )`

作用：改周期；**若在冬眠则同时启动**（用新周期算到期）。等价于"`Stop` + 设新周期 + `Start`"。
常见误用：以为只改周期不启动。

### `*FromISR` 变体（四个：Start / Stop / Reset / ChangePeriod）

作用：中断内下命令。返回 `pdPASS` = 命令已入队（**不是**已生效）。
配套：需要 `pxHigherPriorityTaskWoken` 与 `portYIELD_FROM_ISR`。

---

## 4. 最小示例

```c
static void on_tick(TimerHandle_t t)
{
    /* 回调运行在 daemon 任务里：绝对不要在这里阻塞 */
    g_fire++;
    g_last_tick = xTaskGetTickCount();     /* 只记录，不做耗时的事 */
    xSemaphoreGive(s_sem);                 /* 要工作 -> 交给任务 */
}

s_once = xTimerCreate("once", pdMS_TO_TICKS(300), pdFALSE, NULL, on_tick);
s_cyc  = xTimerCreate("cyc",  pdMS_TO_TICKS(300), pdTRUE,  NULL, on_tick);

if (xTimerStart(s_once, pdMS_TO_TICKS(10)) != pdPASS) {
    /* 命令没排进队列：定时器根本没启动，必须处理 */
}
xTimerChangePeriod(g_buzz, pdMS_TO_TICKS(ms), 0);  /* 0 = 不等待；队列满则静默失败 */
```

---

## 5. 常见错误与陷阱

| 陷阱 | 症状 | 根因 | 处理 |
| --- | --- | --- | --- |
| 回调里阻塞 | **所有**定时器一起停摆 | 只有一个 daemon 任务 | 回调只发信号 |
| 创建后以为在跑 | 回调从不执行，无任何报错 | `xTimerCreate` 是冬眠态 | 显式 `Start` |
| `xTicksToWait` 传 0 且不查返回值 | 命令被丢，回调永不执行 | 命令队列只有 5 格 | 传非 0 并检查返回值 |
| 把 `xTicksToWait` 当周期 | 周期完全不对 | 参数含义是队列超时 | 周期在 `Create`/`ChangePeriod` 里 |
| 周期填毫秒原值 | 换 tick 率后全错 | 单位是 tick | `pdMS_TO_TICKS()` |
| 周期型当单次用 | 回调反复触发 | `uxAutoReload = pdTRUE` | 改 `pdFALSE` |
| 以为 `SetBitsFromISR` 同步 | 等待者延迟 | 走同一个 daemon 队列 | 见 [队列与通知](FreeRTOS队列与通知_速查表.md) |

---

## 6. 调试方法

1. **确认回调真的跑在 daemon 里**：在回调里取 `xTaskGetCurrentTaskHandle()`，与 `xTimerGetTimerDaemonTaskHandle()`
   比较。本仓库实测 `in_daemon=1`。
2. **测漂移而不是测绝对值**：记录相邻两次回调的 tick 差 `d`，与周期比。
   `d == 周期` ⇒ 无漂移；`d == 周期 + 常数` ⇒ 有固定偏移（通常是重装时机）。
3. **区分"漂移"与"追赶补发"**：定时器被长时间阻塞后，恢复时可能连续补发。
   判据要看**连续多次**的 `d`：一次 `d` 偏大后回到正常，是补发；每次都偏大，是永久偏移。
4. **看命令队列满不满**：`xTimerStart` 返回 `pdFAIL` 就是队列满。`configTIMER_QUEUE_LENGTH 5` 很小。
5. **回调时长**：在回调里记开始/结束 tick，若差 >1 就说明回调太长，正在拖累其它定时器。

---

## 7. 与相近技术的对比

| 对比 | 软件定时器 | 硬件定时器 | `vTaskDelay` |
| --- | --- | --- | --- |
| 数量 | 任意多（受堆） | 芯片固定几个 | 每个任务一个 |
| 精度 | 1 tick | 可达 ns | 1 tick |
| 回调上下文 | daemon 任务（可被抢占） | 中断 | 当前任务 |
| 阻塞影响 | **拖累全部定时器** | 拉长中断延迟 | 只影响自己 |
| 适合 | 超时、心跳、去抖 | PWM、精确采样 | 简单延时 |

**与 Linux 的对比**：≈ `timerfd` + 一个事件循环；但 FreeRTOS 的"服务任务"是**单一优先级**，
没有 Linux 的 hrtimer 那种独立上下文。

---

## 8. 验证状态与边界

| 结论 | 级别 | 证据 |
| --- | --- | --- |
| 回调确实跑在 daemon 任务里 | **实测** | `exp13_timers.log`：`in_daemon=1` |
| 单次定时器两次到期 `drift=0` | **实测** | 同上：`once` 两次 `drift=0` |
| 单次到期后回冬眠、`Reset` 可复用 | **实测** | 同上：`fire#2` 存在 |
| 被阻塞后出现追赶补发而非永久偏移 | **实测** | 同上：`stalled dur=60` 时自身 `drift` 仍为 1；fast 定时器 `#20/#21` 同在 tick 1071，`#22` 回到 `drift=1` |
| 回调内阻塞会拖累全部定时器 | **推理** | 机制上成立（单 daemon 任务），**未做受控实验** |
| 周期型长期漂移 | **未验证** | 需真板长跑 |
| 四个 `FromISR` 变体 | **未实测** | 讲义记为"全未使用" |

---

## 证据来源

- 配置：`experiments/F103_common/vendor/freertos/include/FreeRTOSConfig.h:55-63`
- 实测日志：`tests/qemu-stm32/logs/exp13_timers.log`
- 实验：`experiments/F103_13_FreeRTOS_Timers/`
- 讲义：`notes/FreeRTOS入门/Day09_软件定时器.md`
- 相关：[FreeRTOS 队列与通知](FreeRTOS队列与通知_速查表.md)、[FreeRTOS 任务与调度](FreeRTOS任务与调度_速查表.md)
