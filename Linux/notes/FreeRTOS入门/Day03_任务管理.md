# Day03：任务管理（教材第 9 章）

**验证状态**：任务创建/优先级/延时/栈水位**实测**（`tests/qemu-stm32/logs/exp10_freertos.log`，QEMU）；
配置项**源码核对**（`F103_common/vendor/freertos/include/FreeRTOSConfig.h`）；
**未验证**：真实硬件上的抢占时延、时间片轮转的实际波形、`vTaskDelayUntil` 的周期抖动——
本机无板，QEMU 的 SYSCLK 是 8 MHz（固件按 8 MHz 假设，与真板 72 MHz 不同），
时间结论只可用于比较比例。

---

## 学习目标

学完本篇应能用代码证据回答：

1. 任务函数为什么"不能返回"？返回了会发生什么？
2. 就绪、阻塞、挂起三种"非运行"状态，靠什么区分？各自只能由谁来解除？
3. `configMAX_PRIORITIES = 5` 时优先级有几个取值？`configUSE_PORT_OPTIMISED_TASK_SELECTION = 1` 为什么要求它不超过 32？
4. `vTaskDelay(200)` 与 `xTaskDelayUntil(&pre, 200)` 的差别，哪一个的**周期**才不随执行时间漂移？
5. 栈深度填 256 是"256 字节"还是"256 字"？凭什么证明它够用？
6. `vTaskStartScheduler()` 自己会创建任务吗？吃多少内存？

---

## 环境与前置条件

| 项 | 版本 / 说明 |
| --- | --- |
| 教材 | 《FreeRTOS 内核实现与应用开发实战指南》第 9 章（`rtos.txt:2440-3168`） |
| 内核 | FreeRTOS **V11.3.0**，`F103_common/vendor/freertos/` |
| 端口 | `portable/GCC/ARM_CM3/port.c`，`portable/MemMang/heap_4.c` |
| 编译器 | `arm-none-eabi-gcc` 14.2.1 |
| 仿真 | `qemu-system-arm` 10.2.1（WSL），机器 `stm32vldiscovery` |
| 实验 | `experiments/F103_10_FreeRTOS_Tasks/`，日志 `tests/qemu-stm32/logs/exp10_freertos.log` |
| 关键配置 | `configUSE_PREEMPTION=1`、`configUSE_TIME_SLICING=1`、`configUSE_PORT_OPTIMISED_TASK_SELECTION=1`、`configMAX_PRIORITIES=5`、`configTICK_RATE_HZ=1000`、`configMINIMAL_STACK_SIZE=128`、`configSTACK_DEPTH_TYPE=uint16_t`、`configUSE_IDLE_HOOK=0`、`configTOTAL_HEAP_SIZE=4096` |

---

## 核心概念

### 1. 任务就是一个永不返回的函数

教材的定义只有一句："任务就是一个函数"，原型 `void ATaskFunction( void *pvParameters )`（`rtos.txt:2456-2457`）。

三条约束比原型本身重要（`rtos.txt:2458`）：

- **不能返回。** 任务函数体通常是 `for(;;)`。真要从循环里退出，退出前必须
  `vTaskDelete(NULL)`——否则返回地址指向的栈帧已经不归它了，行为未定义。
- **同一函数可以创建多个任务。** 区别只有两处：各自的栈、各自的 `pvParameters`。
- **局部变量天然隔离，全局/静态变量只有一份。** 前者在各自栈里，后者要自己加锁。

这解释了一个常见误解："我每个任务都调同一个函数，所以它们是同一份代码在跑。"
代码是同一份，**状态不是**——栈是各自的。反过来，函数里一个 `static int cnt;`
就把这份隔离悄悄取消了。

### 2. 三种"非运行"状态靠"谁能解除我"区分

教材把"非运行"细分成三种（`rtos.txt:2670-2672`），本篇按**解除条件**重排，因为这才是排障时要问的问题：

| 状态 | 在等什么 | 谁能解除 | 解除后 |
| --- | --- | --- | --- |
| **就绪 Ready** | 只是没轮到 | 调度器（更高优先级者让出 CPU） | 直接运行 |
| **阻塞 Blocked** | 时间到 或 某个同步事件 | 时间到 / 别的任务或中断给出事件 | 变就绪 |
| **挂起 Suspended** | 等别人显式唤醒 | `vTaskResume()` / `xTaskResumeFromISR()` | 变就绪 |

两个容易记反的点：

- 阻塞可以**带超时**："等待队列数据，超时 10 ms"——10 ms 内有数据就成功返回，时间到了没数据就超时返回
  （`rtos.txt:2682`）。这个超时参数在后续队列/信号量/事件组接口里长得一模一样，是同一套机制。
- **挂起没有超时，也没有事件能唤醒它**。它只能被显式 resume。所以"挂起"适合调试试错，
  不适合生产代码里的暂停——教材也这么说："实际开发中，暂停状态用得不多"（`rtos.txt:2689`）。

教材还给了一句很重的判据：如果把 `vTaskDelay` 注释掉，任务 1、2"根本没有执行的机会"，被"饿死"（`rtos.txt:2675-2676`）。
**同优先级也一样被饿死**——因为时间片只在 tick 中断里切，而一个死循环任务根本不返回。

### 3. 优先级：数值越大越高，最高就绪者立刻执行

教材明确：取值范围 `0 ~ (configMAX_PRIORITIES - 1)`，**数值越大优先级越高**（`rtos.txt:2620`）。
与我们做的 HAL 实验里 `NVIC_SetPriority` 数值越小越高**正好相反**，是同名不同义的两个概念，不要串。

调度器找"最高优先级的就绪任务"有两种实现（`rtos.txt:2621-2622`）：

| 方法 | 开关 | 限制 | 代价 |
| --- | --- | --- | --- |
| 通用 C 实现 | `configUSE_PORT_OPTIMISED_TASK_SELECTION = 0` | 无 | 遍历就绪表，慢，占代码 |
| 架构优化（汇编） | `= 1` | **`configMAX_PRIORITIES ≤ 32`** | 用一条"找 32 位数最高置位"的指令 |

为什么是 32？因为就绪表被实现成 **`uint32_t` 位图**，每位对应一个优先级。CM3 的 `CLZ` 指令能一次找出最高有效位。
本仓库正是 `= 1` + `configMAX_PRIORITIES = 5`，符合约束（`FreeRTOSConfig.h:16-17`）。
**若把优先级数量改到 33 以上，这个开关就必须关掉**，否则位图装不下，是静默错误而不是编译错误。

### 4. Tick：时间片、延时和超时的共同单位

`configTICK_RATE_HZ` 决定 tick 间隔：100 时为 10 ms（`rtos.txt:2627`）。本仓库是 **1000**，
即 **1 tick = 1 ms**，这也是为什么代码里能写 `pdMS_TO_TICKS(200)` 而看不出换算。

同优先级任务"轮流"靠 tick 中断：t2 时刻进 tick 中断 → 选出下一个任务 → 中断返回时切过去
（`rtos.txt:2631`）。教材特意提醒：**任务运行的时间并不严格从 t1/t2/t3 开始**
（`rtos.txt:2631`）——切换发生在中断里，是有开销的，这个开销就是后续"抖动"的来源。

`vTaskDelay(2)` 的语义是"至少等 2 个 tick 中断"，不是"精确等 20 ms"——教材说
"有可能经过 1 个 Tick 多一点就返回了"（`rtos.txt:2637`）。所以：

- 需要**精确**时间：用硬件定时器 + 中断，不要指望 tick。
- 需要**不随执行时间漂移的周期**：用 `xTaskDelayUntil`，见下条。

### 5. `vTaskDelay` 测"间隔"，`xTaskDelayUntil` 测"周期"

这是本篇最实用的一条差别（`rtos.txt:2911-2915`）：

```text
vTaskDelay(n)          : 进入 → 退出 的间隔 至少 n 个 tick
xTaskDelayUntil(&pre,n): 前后两次**退出**的间隔 至少 n 个 tick
```

差别只在"上一个周期干了多久"：

| 周期内工作量 | `vTaskDelay(200)` | `xTaskDelayUntil(&pre, 200)` |
| --- | --- | --- |
| 5 ms | 周期 200+5 = 205 ms | 周期 200 ms |
| 50 ms | 周期 200+50 = 250 ms | 周期 200 ms |
| 300 ms（超时） | 周期 300 ms，相位全乱 | 立即返回，追赶，仍按 200 ms 排 |

**所以凡是"每 N ms 做一次"的任务，都该用 `xTaskDelayUntil`。**
本仓库的 `F103_10` 用的是 `vTaskDelay`（`main.c:81`、`main.c:107`），
因为它要演示的是"任务间交接"，而不是周期精度。实测把这点暴露得很清楚：

```text
[consume] seq=1 tick=1        ← 首个样本
[consume] seq=2 tick=203      ← 间隔 202
...
[monitor] tick=1003 → 2012 → 3017 → 4023   ← 间隔 1009/1005/1006
```

设定 `SAMPLE_PERIOD_MS=200`、`REPORT_PERIOD_MS=1000`，实测间隔是 **202–203** 与 **1003–1011**。
多出来的 2–11 tick 就是"执行时间 + tick 相位"。换成 `xTaskDelayUntil` 后这部分应被吸收——
**但本仓库没有做这个对照实验，所以这句话是推论，不是实测结论。**

### 6. 空闲任务：不是"没用的兜底"，它有一个硬职责

教材给了两个理由（`rtos.txt:2935-2937`）：

1. **释放被删除任务的内存。** `vTaskDelete()` 只把任务挂到终止列表，真正 `free` 的动作
   发生在空闲任务里。所以**用了 `vTaskDelete` 就必须保证空闲任务能跑到**。
2. **调度器必须总能找到一个可运行任务。** 良好设计的任务都是事件驱动的、平时阻塞，
   万一全阻塞了，得有人跑。

它的两个性质固定：**优先级 0**、**永不阻塞**。

钩子函数（`vApplicationIdleHook`）的三类用途（`rtos.txt:2940-2941`）：
低优先级后台事、**测 CPU 占用率**（空闲任务能跑 = 高优先级都停了）、进省电模式。
它的限制也很硬：**钩子内不能让空闲任务进入阻塞或挂起状态**，否则内存回收就停了
（`rtos.txt:2942`）。前提是 `configUSE_IDLE_HOOK = 1` 并实现该函数（`rtos.txt:2945`）。

本仓库 `configUSE_IDLE_HOOK = 0`（`FreeRTOSConfig.h:66`），即**没有实现钩子**。
这意味着两件事：不能测 CPU 占用率；且凡是用了 `vTaskDelete` 的实验，
回收完全依赖空闲任务被调度到（`F103_10` 不删任务，所以这一点在本实验里不构成风险）。

### 7. 调度算法是三个开关的组合，不是"一个模式"

教材把它们拆成三问（`rtos.txt:2956-2980`），对应五个组合 A–E：

| 问 | 配置项 | 本仓库 |
| --- | --- | --- |
| 高优先级就绪能否立刻抢占？ | `configUSE_PREEMPTION` | **1**（抢占） |
| 同优先级是否轮流？ | `configUSE_TIME_SLICING` | **1**（时间片轮转） |
| 空闲任务是否让位给用户任务？ | `configIDLE_SHOULD_YIELD` | 未显式定义（走内核默认） |

教材的对照实验结论值得记：

- **关掉抢占**：优先级"失去意义"，一个不让出的任务会跑到底（`rtos.txt:3090-3092`）。
  这解释了一个排查陷阱——"我明明把优先级调高了却没生效"，先看 `configUSE_PREEMPTION`。
- **关掉时间片轮转**：只有"高优先级就绪"和"高优先级不再运行"两个时刻才切换，
  其余时间任务能连续跑很久（`rtos.txt:3126`）。吞吐上去、响应变差。
- **空闲任务让步**：让步时空闲任务的执行时间片非常小（`rtos.txt:3164`）。

本仓库用的是最常用的 A 组（抢占 + 时间片轮转 + 空闲让步）。
`configIDLE_SHOULD_YIELD` 本仓库没有显式写，属于"依赖内核默认值"——
这类未显式声明的配置在换内核版本时是风险点，**读代码时应当把它当作已知缺口，而不是已知行为**。

### 8. 栈深度是"字"不是"字节"，而唯一能证明够用的是水位

教材原文：`usStackDepth` "单位是 word，比如传入 100，表示栈大小为 100 word，也就是 400 字节"，
并且"怎么确定栈的大小，并不容易，很多时候是估计。精确的办法是看反汇编"（`rtos.txt:2493`）。

"精确的办法是看反汇编"在实战里成本太高。可执行的办法是**水位 + 判据**：

- `uxTaskGetStackHighWaterMark(handle)` 返回**历史最低剩余量**（单位同样是字）。
- 判据取"绝对 + 相对"两条：剩余 ≥ 32 字（留给中断嵌套与库函数），且剩余/总深度 ≥ 25 %。
- **必须让全路径（含错误路径、最坏分支）跑完再读**，否则读到的"余量充足"只说明"还没走到最深处"。

本仓库 `F103_10` 的实测（`exp10_freertos.log:10`）：

| 任务 | 优先级 | 深度（字） | 最低余量（字） | 已用占比 | 剩余占比 |
| --- | --- | --- | --- | --- | --- |
| `consume` | 3 | 256 | 154 | 39.8 % | 60.2 % |
| `produce` | 2 | 160 | 126 | 21.3 % | 78.8 % |
| `monitor` | 1 | 192 | 90 | 53.1 % | 46.9 % |

三者都过"≥32 字且 ≥25 % 余量"。

**这张表要按"剩余占比"读，不能按"余量绝对值"读。** 三个余量分别是 154 / 126 / 90 字，
绝对值最小的是 `monitor`；而按余量占总深度的比例，最紧的也是它（46.9 %），
最宽松的反而余量第二小的 `produce`（78.8 %）。也就是说：**一个填了 1024 字节栈、
还剩 154 字没用的任务，并不比一个只填 640 字节、还剩 126 字没用的任务更危险。**

`monitor` 之所以最紧，是因为它要做 `printf`——库函数吃栈很深。
这解释了为什么 `F103_13` 在按同一判据复核时把 monitor 与守护任务从 128 字提到 160 字
（当时的读数是 26/128 与 28/128 字，**已经跌破 32 字绝对门限**）。
`configCHECK_FOR_STACK_OVERFLOW = 2` 的存在意义就在这里，而它**只在切换时检查**，
溢出发生在任务内部最深处时可能抓不到（`F103_15` 的实测结论：paint 那一半能抓，
SP 越过栈块抓不到）。

---

## 系统调用/接口卡片

### `BaseType_t xTaskCreate(...)`

作用：动态分配栈与 TCB，创建任务。

原型：

```c
BaseType_t xTaskCreate( TaskFunction_t pxTaskCode,
                        const char * const pcName,
                        const configSTACK_DEPTH_TYPE usStackDepth,
                        void * const pvParameters,
                        UBaseType_t uxPriority,
                        TaskHandle_t * const pxCreatedTask );
```

参数：

| 参数 | 含义 | 本仓库类型/取值 |
| --- | --- | --- |
| `pxTaskCode` | 任务函数 | `void f(void *)`，永不返回 |
| `pcName` | 名字，**内核不使用**，仅供调试 | 长度 ≤ `configMAX_TASK_NAME_LEN` |
| `usStackDepth` | 栈深，单位 **word** | `configSTACK_DEPTH_TYPE = uint16_t` ⇒ 上限 65535 字 |
| `pvParameters` | 传给任务函数的参数 | 用同一函数建多个任务时靠它区分 |
| `uxPriority` | `0 ~ configMAX_PRIORITIES-1`，**越大越优先** | 传过大值会被**钳到最大值**，不报错 |
| `pxCreatedTask` | 输出句柄，可传 `NULL` | 后续 `vTaskDelete`/改优先级要用 |

返回值和错误：成功 `pdPASS`；失败 `errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY`。
教材在这里专门纠正了文档："文档里都说失败时返回值是 `pdFAIL`，这不对。
`pdFAIL` 是 0，`errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY` 是 -1"（`rtos.txt:2507`）。

**这条纠错有实际后果**：写成 `if (xTaskCreate(...) == pdFAIL)` 判断失败，
在堆不足时**永远不成立**（返回 -1 不是 0），任务创建失败会被静默忽略。
`F103_16` 的首版正是这么栽的——4 KB 堆被"四任务 + 三个完成信号量 + 栈上 ctx"耗尽，
`xTaskCreate` 返回值从未检查，钩子停机让日志停在半截（`F103_16` README 首版失败已记录）。

资源所有权：栈与 TCB 由内核堆分配，任务被删除且空闲任务回收后释放。

线程/进程安全：任务态可调用；中断里不能调用（用 `xTaskCreate` 的 ISR 变体思路不成立——
FreeRTOS 不支持在 ISR 里建任务）。

最小示例（本仓库真实代码，`main.c:177-179`）：

```c
/* 先建 consumer，让它进入阻塞态；producer 一投递就会触发抢占 */
xTaskCreate( consumer_task, "consume", 256, NULL, 3, &s_h_consumer );
xTaskCreate( producer_task, "produce", 160, NULL, 2, &s_h_producer );
xTaskCreate( monitor_task,  "monitor", 192, NULL, 1, &s_h_monitor );
```

常见误用：① 把栈深当字节填（填 256 以为 256 字节，实际 1024 字节，白占 768 字）；
② 不检查返回值；③ 优先级传超界后以为会报错。

### `void vTaskDelete( TaskHandle_t xTaskToDelete )`

作用：删除任务。

参数：句柄；传 `NULL` 表示**删除自己**（教材的"自杀/被杀/杀人"三分法，`rtos.txt:2597`）。

返回值和错误：无返回值。句柄无效时行为未定义。

资源所有权：**调用的瞬间并不释放内存**。任务是挂到终止列表，
真正 `free` 在**空闲任务**里做（`rtos.txt:2935`）。所以用了它就必须保证空闲任务能跑到。

线程/进程安全：任务态可调用。ISR 里用 `vTaskDeleteFromISR` 思路不存在（内核无此 API）；
ISR 里不要删任务。

最小示例：

```c
for (;;) { /* 主体 */ }
vTaskDelete( NULL );   /* 只在循环被 break 出来时才会执行到 */
```

常见误用：① 任务函数结尾忘了删自己；② 依赖"删了立刻省内存"，
结果空闲任务被饿死，内存一直不回收。

### `void vTaskDelay( const TickType_t xTicksToDelay )`

作用：相对延时，至少等 N 个 tick。

参数：tick 数。**用 `pdMS_TO_TICKS(ms)` 换算**，这样换 `configTICK_RATE_HZ` 不用改代码
（`rtos.txt:2639`）。

返回值和错误：无。`xTicksToDelay = 0` 等价于 `taskYIELD()`（让出一次），不是"立即返回"的巧合。

资源所有权：无。

线程/进程安全：任务态专用；ISR 里必须用 `vTaskDelayUntil` 的 ISR 对应物也不存在——
ISR 里不能阻塞。

最小示例（`main.c:81`）：

```c
vTaskDelay( pdMS_TO_TICKS( SAMPLE_PERIOD_MS ) );
```

常见误用：① 以为它是精确延时（教材明确说不是，`rtos.txt:2637`）；
② 用它做"每 N ms 一次"的周期任务，导致周期 = N + 执行时间。

### `BaseType_t xTaskDelayUntil( TickType_t * const pxPreviousWakeTime, const TickType_t xTimeIncrement )`

作用：**绝对时刻**延时——阻塞到 `*pxPreviousWakeTime + xTimeIncrement`。

参数：`pxPreviousWakeTime` 必须由调用者保存并在每次调用后自动更新（内核会写回）；
`xTimeIncrement` 是周期，不是"剩余时间"。

返回值和错误：返回 `pdFALSE` 表示**已经错过唤醒时刻**（因为执行时间超过了周期），
此时函数立即返回不阻塞。**这个返回值常被忽略，而它恰恰是"周期来不及"的唯一信号。**

资源所有权：无。

线程/进程安全：任务态专用。

最小示例（教材写法，`rtos.txt:2924-2928`）：

```c
preTime = xTaskGetTickCount();
for (;;) {
    do_work();
    vTaskDelayUntil( &preTime, 500 );   /* 每 500 tick 一次，不随 do_work 耗时漂移 */
}
```

常见误用：① 把 `preTime` 建成局部变量且每轮重置（退化成 `vTaskDelay`）；
② 不检查返回值，周期超时后相位静默漂移。

### `UBaseType_t uxTaskPriorityGet( const TaskHandle_t xTask )` / `void vTaskPrioritySet( TaskHandle_t xTask, UBaseType_t uxNewPriority )`

作用：读/写任务优先级。

参数：`xTask` 传 `NULL` 表示**自己**；`uxNewPriority` 取值 `0 ~ configMAX_PRIORITIES-1`。

返回值和错误：`Get` 返回当前优先级；`Set` 无返回值，超界值被钳位。

资源所有权：无。

线程/进程安全：任务态可调用；ISR 里用 `vTaskPrioritySet` 会破坏调度器状态，
优先级变更在 ISR 侧的合法路径是优先级继承（内核自动做）。

最小示例：

```c
UBaseType_t p = uxTaskPriorityGet( NULL );   /* 我现在的优先级 */
vTaskPrioritySet( NULL, p );                 /* 改回去（无实际变化） */
```

常见误用：① 把"改优先级"当成"暂停任务"——改了优先级任务仍在就绪/阻塞表里；
② 在 `configUSE_MUTEXES = 1` 且任务持有互斥量时手动降优先级，
与优先级继承打架（`F103_16` 实测：`mutex` 轮里持锁者 `prio 1→3`，`give` 内部才回 1）。

### `void vTaskSuspend( TaskHandle_t xTaskToSuspend )` / `void vTaskResume( TaskHandle_t xTask )` / `BaseType_t xTaskResumeFromISR( TaskHandle_t xTask )`

作用：挂起 / 恢复。

参数：句柄，`NULL` 表示自己（仅 `Suspend` 支持）。

返回值和错误：`xTaskResumeFromISR` 返回 `pdTRUE` 表示恢复后需要**立刻触发一次上下文切换**，
调用者必须据此做 `portYIELD_FROM_ISR()`。**忽略这个返回值 = 恢复要等到下一个 tick 才生效。**

资源所有权：无。

线程/进程安全：`vTaskSuspend`/`vTaskResume` 任务态；`xTaskResumeFromISR` 仅 ISR。

最小示例：教材的红外遥控器暂停/继续播放音乐（`rtos.txt:2698`）。

常见误用：① ISR 里调 `vTaskResume`（必须用 `...FromISR`）；
② 用挂起实现"暂停"，结果超时机制对它无效，任务永久挂着。

### `UBaseType_t uxTaskGetStackHighWaterMark( TaskHandle_t xTask )`

作用：返回任务**历史最低剩余栈深**，单位 word。

参数：句柄；`NULL` 表示自己。

返回值和错误：剩余 word 数。返回值 **0 表示可能已经溢出**（不是"刚好用完"，
而是"检测到的最低点已经是 0，最深处可能已经写穿了栈底"）。

资源所有权：无。

线程/进程安全：任务态可调用；ISR 里也能读（只读），但读到的是调用那一刻的值。

最小示例（`main.c:117-119`）：

```c
LOG( "[res] heap_free=%u B, stack_hwm consume=%u produce=%u monitor=%u words\n",
     (unsigned)xPortGetFreeHeapSize(),
     (unsigned)uxTaskGetStackHighWaterMark( s_h_consumer ), ... );
```

常见误用：① 任务刚跑几轮就读，读到的是"还没走到最深处"的乐观值；
② 把 0 当成"正好够用"。

---

## 最小可运行实验

`experiments/F103_10_FreeRTOS_Tasks/`：三任务 + 一队列 + 一日志互斥量。

```text
produce (prio 2, 160 字)  ──每 200 ms 投递──▶  queue(depth=8)
                                                  │
consume (prio 3, 256 字)  ◀──阻塞等待────────────┘
monitor (prio 1, 192 字)  ──每 1000 ms 打印 produced/consumed/dropped/queued + 栈水位
```

设计上刻意做的三件事：

1. **先建 `consume`**，让它先进入阻塞态；`produce` 一投递就触发抢占——这是本条实验
   唯一能直接观察"抢占发生"的地方。
2. **`monitor` 放在最低优先级**：它只能在高优先级都阻塞时才跑，所以它的 `tick` 间隔
   本身就携带了"系统忙不忙"的信息。
3. **日志走互斥量**：`printf` 不是原子的，多任务直接打会串行交叉。

---

## 编译与运行

```bash
cd experiments/F103_10_FreeRTOS_Tasks
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake \
  -DF103_QEMU=ON
cmake --build build

cd ../..
timeout 25 qemu-system-arm -M stm32vldiscovery -display none -monitor none \
  -serial stdio -kernel experiments/F103_10_FreeRTOS_Tasks/build/f103_10_freertos_tasks.elf \
  < /dev/null 2>&1 | tr -d '\r' | tee tests/qemu-stm32/logs/exp10_freertos.log
```

注意 CMake 里的 `NO_TICK`：不链接 `f103_tick.c`，因为 `SysTick_Handler` 由 FreeRTOS 接管，
两个同时存在会符号重定义——这也是"移植 FreeRTOS 时最先撞上的链接错误"。

---

## 结果观察

日志头两行给出环境，这是所有时间结论的前提：

```text
[F103_10] FreeRTOS V11.3.0 on STM32F103, HCLK=8000000 Hz, tick=1000 Hz
任务已创建，启动调度器（初始空闲堆 1112 B）
```

**`HCLK=8000000` 是 QEMU 下的配置，不是真板的 72 MHz。** 任何"毫秒级"结论
只在本环境内可比，不能外推到真板（`CLAUDE.md` §12 已把这条写成通则）。

### 栈与堆

```text
[res] heap_free=496 B, stack_hwm consume=154 produce=126 monitor=90 words
```

日志里这一行重复出现 20 次，**数值全程不变**——说明三任务在 11 s 内没有
增长型分配，栈也没有随轮次加深。这本身就是一条"稳态"证据。

堆的两笔账（第一笔实测、第二笔推算）：

| 时点 | `heap_free` | 差额 |
| --- | --- | --- |
| 创建队列 + 互斥量 + 三任务后 | **1112 B** | — |
| 启动调度器后 | **496 B** | 616 B |

第二笔 616 B 是 **空闲任务**。按 `configMINIMAL_STACK_SIZE = 128` 字算：
栈 512 B + TCB 约 92 B ≈ 604 B，与实测 616 B 吻合到 12 B 以内。
这是教材"`vTaskStartScheduler()` 内部会创建空闲任务"（`rtos.txt:2936`）的一次可见证据——
**推算式核对，不是单独实测**。

反过来算第一笔：4 KB 堆 − 1112 B = 2984 B，与"三任务栈 1024+640+768 = 2432 B
+ 三 TCB ≈ 276 B + 队列/互斥量 ≈ 200 B"同量级。这解释了 `F103_16` 为什么会撞墙：
四个任务 + 三个完成信号量就把 4 KB 吃干净了。

### 周期与漂移

```text
[consume] seq=1 tick=1  → seq=2 tick=203     间隔 202
[monitor] tick=1003 → 2012 → 3017 → 4023      间隔 1009 / 1005 / 1006
```

设定 200 ms / 1000 ms，实测 202–203 / 1003–1011。差额来自"执行时间 + tick 相位"。
这与 §5 的结论一致：`vTaskDelay` 的周期会漂移，**工程上"每 N ms"应当用 `xTaskDelayUntil`**。

### 交接完整性

```text
[monitor] tick=11062 produced=55 consumed=55 dropped=0 queued=0
```

`produced == consumed`、`dropped = 0`、`queued = 0` 三件事同时成立，
说明队列深度 8 足够、消费速度跟得上生产速度。**任一项不为 0 都指向不同的病**：
`dropped > 0` 是生产者赶不上消费，`queued` 持续非 0 是积压，`produced > consumed` 且没有
`dropped` 则是消费者卡死——这三种要分开看。

---

## 常见错误和陷阱

| 陷阱 | 症状 | 根因 | 处理 |
| --- | --- | --- | --- |
| 用 `== pdFAIL` 判 `xTaskCreate` 失败 | 堆不足时任务静默不创建 | 失败返回 `-1`（`errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY`），`pdFAIL` 是 0 | 判 `!= pdPASS`；教材已明确纠正（`rtos.txt:2507`） |
| 栈深按字节填 | RAM 白占 3/4，或以为够用其实不够 | 单位是 word | 记住 `100 word = 400 B`；用 `uxTaskGetStackHighWaterMark` 验证 |
| 任务函数返回 | 随机 HardFault / 跳飞 | 返回地址所在的栈帧已不属于该任务 | 循环退出前 `vTaskDelete(NULL)` |
| 用 `vTaskDelay` 做周期任务 | 周期 = 设定值 + 执行时间，越忙越慢 | `vTaskDelay` 测"间隔"不测"周期" | 改 `xTaskDelayUntil(&pre, n)`，并检查其返回值 |
| 忽略 `xTaskDelayUntil` 的 `pdFALSE` | 周期超时后相位静默错乱 | 返回值是"已错过唤醒时刻"的唯一信号 | 收到 `pdFALSE` 时记录/降频，不要当无事 |
| 优先级数量 > 32 且开着优化选择 | 调度器选出错误任务 | 就绪表是 `uint32_t` 位图，装不下第 33 位 | `configMAX_PRIORITIES ≤ 32`，否则关掉 `configUSE_PORT_OPTIMISED_TASK_SELECTION` |
| 用 `vTaskDelete` 却不给空闲任务机会 | 内存只增不减 | 回收发生在空闲任务里 | 确保有空闲窗口；钩子里禁止阻塞 |
| ISR 里调 `vTaskResume` | 断言/死机 | 必须用 `xTaskResumeFromISR` | 并处理它的返回值 → `portYIELD_FROM_ISR()` |
| 把 FreeRTOS 优先级和 NVIC 优先级混谈 | "优先级调高了反而更慢" | 前者**越大越优先**，后者**越小越优先** | 分开记；`MAX_SYSCALL_INTERRUPT_PRIORITY` 才是两者的接缝 |
| 刚启动就读栈水位 | 余量看着充足，之后溢出 | 水位是历史最低，此刻还没走到最深处 | 全路径跑完再读；配合 `configCHECK_FOR_STACK_OVERFLOW = 2` |
| 任务里用 `static` 局部变量 | 两个任务互相踩数据 | 静态变量只有一份副本 | 需要隔离就用任务栈上的局部变量或 `pvParameters` |

---

## 故障排查方法

按"能不能被观测到"排序，逐层缩小：

1. **任务根本没跑。** 先看 `xTaskCreate` 返回值（不是 `== pdFAIL`）。
   再确认 `vTaskStartScheduler()` 之后**没有**打印"调度器未能启动"——
   本实验 `main.c:186` 就有这一行兜底；它出现即调度器没起来（通常是堆不足）。
2. **任务跑了但轮不到。** 看 `configUSE_PREEMPTION`；看是否有更高优先级任务死循环
   （教材的反例：注释掉 `vTaskDelay` 后低优先级被饿死，`rtos.txt:2675`）。
3. **周期不对。** 打印相邻两次的 `xTaskGetTickCount()` 差值，与设定值对比。
   差值是"设定 + 执行时间" ⇒ 用 `vTaskDelay`；差值忽大忽小 ⇒ 检查是否有高优先级抢占。
4. **栈溢出。** 读 `uxTaskGetStackHighWaterMark`；接 `vApplicationStackOverflowHook` 抓任务名。
   注意 `configCHECK_FOR_STACK_OVERFLOW = 2` 只在**切换时**检查，
   任务内部最深处溢出可能漏检（`F103_15` 实测：余量归零能抓，SP 越过栈块抓不到）。
5. **内存泄漏。** 周期性打印 `xPortGetFreeHeapSize()`；若每次删除任务后不回升，
   说明空闲任务没跑到（钩子卡住或优先级设计让空闲任务永远不执行）。
6. **日志交错/乱码。** 多任务共用一个 `printf`。本实验用互斥量包住日志，
   这就是"共享资源必须显式保护"的最小例子。

---

## 练习题

### 基础题

1. 把 `configTICK_RATE_HZ` 从 1000 改成 100，`pdMS_TO_TICKS(200)` 变成多少 tick？
   若某处直接写了 `vTaskDelay(200)`，行为会怎么变？
2. `configMAX_PRIORITIES = 5` 时合法优先级有哪些值？传入 7 会发生什么（查手册与源码确认）？
3. 用 `exp10_freertos.log` 的读数算出 `produce` 的栈实际使用率，并判断它离"32 字门限"还有多少余量。

### 改错题

4. 下面这段创建任务，找出两处问题并说明后果：

   ```c
   if (xTaskCreate( worker, "w", 128, NULL, 9, &h ) == pdFAIL) {
       printf("create failed\r\n");
   }
   ```

5. 下面这段想做"每 100 ms 一次"的周期任务，说明为什么实际周期会越来越大：

   ```c
   for (;;) {
       do_work();                       /* 耗时 15~40 ms 不定 */
       vTaskDelay( pdMS_TO_TICKS(100) );
   }
   ```

6. 某任务里写了 `static uint8_t buf[64];` 作为工作缓冲，用同一函数创建了两个任务。
   指出会出什么问题，并给出两种改法（各自的代价是什么？）。

### 综合题

7. 把 `F103_10` 的 `produce`/`monitor` 改成 `xTaskDelayUntil`，要求：
   ① 预测改动后两个间隔的读数会如何变化；② 实际跑一次并对比；
   ③ 若某个任务的执行时间偶尔超过周期，`xTaskDelayUntil` 会返回什么，你打算怎么记录它。

8. `configUSE_IDLE_HOOK = 0` 时无法测 CPU 占用率。请设计一个不小于 5 行的方案：
   用空闲钩子 + 一个计数变量 + `monitor` 任务，估算 CPU 占用率；
   说明你如何避免"钩子里的计数被 `monitor` 撕裂读"的问题。

9. 把 `CONFIG_MAX_PRIORITIES` 改成 33 并保持 `configUSE_PORT_OPTIMISED_TASK_SELECTION = 1`，
   先预测会发生什么，再实际构建（本实验可承受），并解释你观察到的现象与
   `tasks.c` 里就绪表的数据类型是什么关系。

---

## 验收标准

- [ ] 能解释"任务函数不能返回"的机制，而不是只背结论。
- [ ] 能给出三种非运行状态的**解除条件**，并指出哪种没有超时。
- [ ] 能说清 `configMAX_PRIORITIES ≤ 32` 与 `CLZ` 指令、`uint32_t` 位图的关系。
- [ ] 能举一个"用 `vTaskDelay` 做周期会漂移"的具体数字（本实验是 202 vs 200）。
- [ ] 能用 `uxTaskGetStackHighWaterMark` 的三个读数判断哪个任务的栈最紧，并说明为什么不是"余量最小的那个"。
- [ ] 能算出空闲任务占的 616 B，并说明这是推算还是实测。
- [ ] 能在不看文档的情况下写出 `xTaskCreate` 的六个参数及各自陷阱。
- [ ] 能说出本实验**未验证**的三项（真板抢占时延、轮转波形、`xTaskDelayUntil` 抖动）及原因。

---

## 延伸阅读

- 教材第 9 章：`rtos.txt:2440-3168`
- 源码：`vendor/freertos/Source/tasks.c`（就绪表、`vTaskSwitchContext`、`vTaskDelay`/`xTaskDelayUntil`）
- 端口：`vendor/freertos/portable/GCC/ARM_CM3/port.c`（`PendSV`、`SysTick`、`vPortStartFirstTask`）
- 本仓库配置：`vendor/freertos/include/FreeRTOSConfig.h`
- 相关实验：`F103_11`（中断→任务交接）、`F103_12`（同步原语）、`F103_15`（栈溢出检出）、`F103_16`（优先级反转）
- 下一篇：`Day04_同步互斥与通信总览.md`（教材第 10 章）

---

**验证记录**

| 日期 | 环境一 | 命令一 | 结果一 | 环境二 | 命令二 | 结果二 | 未覆盖 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 2026-10-30 | 教材抽取文本 | 读 `rtos.txt:2440-3168` | 第 9 章全 8 节；教材自行纠正 `pdFAIL` 说法（2507 行）、`usStackDepth` 单位为 word（2493 行）、`vTaskDelay` 不精确（2637 行） | WSL2 + `qemu-system-arm` 10.2.1 | `timeout 25 qemu-system-arm -M stm32vldiscovery -kernel build/f103_10_freertos_tasks.elf` | `FreeRTOS V11.3.0 / HCLK=8000000 / tick=1000`；`heap_free` 1112 → 496 B；`stack_hwm consume=154 produce=126 monitor=90 words`；consume 间隔 202–203、monitor 间隔 1003–1011；`produced==consumed`、`dropped=0`、`queued=0` | 真板抢占时延、时间片轮转波形、`xTaskDelayUntil` 抖动 |
| 2026-10-30 | 本仓库源码 | `grep` `FreeRTOSConfig.h` / `main.c` | `configUSE_PREEMPTION=1`、`configUSE_TIME_SLICING=1`、`PORT_OPTIMISED=1`、`MAX_PRIORITIES=5`、`TICK_RATE_HZ=1000`、`MINIMAL_STACK_SIZE=128`、`USE_IDLE_HOOK=0`；三任务 `256/160/192` 字、优先级 `3/2/1` | 同上 | 同上 | 空闲任务的 616 B 为**推算**（128 字栈 + TCB），与实测差额吻合到 12 B 以内，未单独测量 | `configIDLE_SHOULD_YIELD` 本仓库未显式定义，行为依赖内核默认值，未做对照实验 |
