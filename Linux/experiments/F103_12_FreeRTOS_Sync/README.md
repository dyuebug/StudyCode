# F103_12：FreeRTOS 同步原语（二值信号量 / 计数信号量 / 事件组 / 中断→信号量）

实验 10 用队列传**数据**，实验 11 让**中断**发起传输。两者共用一套"排队"语义：
发一次、收一次，元素不会丢。本实验换成只传**状态**的三件工具，并回答一个队列回答不了的问题：
*7 个字节进来，需要唤醒任务 7 次吗？*

答案取决于用哪种原语。这一条差别直接决定中断风暴时任务会不会被淹没，
也是"队列 vs 信号量"在面试里最常被追问的边界。

## 学习目标

- 能说清二值信号量、计数信号量、事件组三者的语义差别，并给出各自的适用场景。
- 能用 `xSemaphoreCreateCounting(N, init)` 做资源槽位，并解释为什么"取不到"要挂起而不是轮询。
- 能说清 `xEventGroupWaitBits` 的 `xWaitForAllBits` 与 `xClearOnExit` 两个开关的组合含义。
- 能在 ISR 里用 `xSemaphoreGiveFromISR` + `portYIELD_FROM_ISR` 完成"中断→任务"的交接，
  并从实测计数读出**事件合并**（coalescing）的量。
- 会用 `uxTaskGetStackHighWaterMark` 给每个任务定量判断栈余量，并给出可执行的判据。
- 能把 4 KB 内核堆按"同步对象 / 任务栈 / idle"三段记账，而不是只看最后剩多少。

## 环境与前置条件

| 项目 | 值 |
| --- | --- |
| 工具链 / 构建 | `arm-none-eabi-gcc 14.2.1`、CMake 4.0.3、Ninja 1.13.2 |
| 内核 | FreeRTOS V11.3.0（`../F103_common/vendor/freertos/`），配置同实验 10 |
| 新增内核文件 | `Source/event_groups.c`（从上游 V11.3.0 原样复制，md5 与上游一致） |
| 硬件假设 | PA9 = TX，PA10 = RX，115200 8N1，USART1 = IRQ 37 |
| 仿真 | QEMU 10.2.1 `-M stm32vldiscovery`（USART1 + NVIC 是真实模型） |

前置：实验 10（任务/队列/互斥量）、实验 11（`FromISR` + `portYIELD_FROM_ISR`）。
本实验**不需要**任何未建模外设，四段演示全部在仿真里可观测。

## 核心概念

### 1. 三种原语的分工

| 原语 | 语义 | 能存几个"事件" | 典型用途 | 本项目对应 |
| --- | --- | --- | --- | --- |
| 队列 | 先进先出，**传值** | N（深度） | 数据流、命令流 | 实验 10 / 11 |
| 二值信号量 | "发生过"的**布尔量** | 1 | 事件通知、ISR 推迟处理 | `s_bin` / `s_uart` |
| 计数信号量 | 可数**资源槽位** | N（上限） | 缓冲池、并发数限制 | `s_pool` |
| 事件组 | 一组**布尔位** | 32 位，每位独立 | 多条件会合、状态广播 | `s_events` |
| 互斥量 | 带**所有权**的二值信号量 | 1 | 保护共享资源（优先级继承） | `s_log_mutex` |

一句话记忆：**要传数据用队列，要传"发生了/可用"用信号量，要传"哪些条件满足了"用事件组。**

### 2. 二值信号量会合并事件，队列不会

```text
7 个字节到达（同一个 ISR，各一次 give）：
  队列（深度 32）：排 7 项 → 任务醒来 7 次
  二值信号量     ：第 1 次 give 成功，之后装满 → 剩余 give 全部返回 pdFALSE
                  → 任务最多醒来 2 次（取走后若又有字节进入，会再形成 1 个事件）
```

这不是缺陷，是设计。采集类场景"有新数据了"只需要知道有，不需要知道有几个；
控制类场景每个采样点都不能丢，就必须用队列。**判据是"丢失一次事件会不会改变行为"。**

### 3. 事件组的两个布尔开关

`xEventGroupWaitBits(group, bits, xClearOnExit, xWaitForAllBits, xTicksToWait)`

| `xWaitForAllBits` | `xClearOnExit` | 语义 |
| --- | --- | --- |
| `pdTRUE` | `pdTRUE` | 所有位都置位才返回，返回后清掉（本实验用这个） |
| `pdTRUE` | `pdFALSE` | 所有位都置位才返回，位保留（状态监控用） |
| `pdFALSE` | `pdTRUE` | 任一位置位就返回，返回后清掉（事件竞争用） |
| `pdFALSE` | `pdFALSE` | 任一位置位就返回，位保留（广播用） |

返回值是**退出时的事件位快照**，不是"我等的那些位"。超时返回时它是 0 或部分位——
所以判断成功与否必须比对 `(got & 等待掩码) == 等待掩码`，不能只看返回值非 0。

### 4. `FromISR` 的唯一出口是 `portYIELD_FROM_ISR`

`xSemaphoreGiveFromISR(sem, &woken)` 永不阻塞：信号量已满就返回 `pdFALSE`，不排队、不等待。
它把"是否唤醒了更高优先级任务"写进 `woken`，由 ISR 在**最后**统一 `portYIELD_FROM_ISR(woken)`。
一次 ISR 里可以 give 多个对象，只需 yield 一次。

### 5. 栈水位的判据

`uxTaskGetStackHighWaterMark(h)` 返回**历史最小剩余量**（单位：字，1 字 = 4 B），只减不增。

判据（本实验采用，写在 README 里而不是凭感觉）：

1. **绝对余量 ≥ 32 字（128 B）**：覆盖最深的一次中断嵌套 + `printf` 内部调用。
2. **相对余量 ≥ 25%**：留出后续加逻辑（新分支、新日志）的空间。
3. 水位数**在系统跑完全部路径后**读取：任务没执行过的分支不会体现在水位里。

三条都满足才算"栈够用"。只满足其中一条就应当调整 `xTaskCreate` 的栈深度并重跑。

## 接口卡片

### `SemaphoreHandle_t xSemaphoreCreateBinary(void)`

作用：创建**初值为空**的二值信号量（不能传初始可用数）。

- 返回 `NULL` 表示堆不足；必须检查——`xSemaphoreTake(NULL)` 会断言失败。
- 空二值信号量 `take(0)` 返回 `pdFALSE`，本实验在 `main()` 里显式验证一次，
  避免把"能立刻拿到"误当正常。
- 若需要"初始可用"，用 `xSemaphoreCreateCounting(1, 1)`。

### `SemaphoreHandle_t xSemaphoreCreateCounting(UBaseType_t max, UBaseType_t init)`

作用：创建计数信号量，取值区间 `[0, max]`。

- `init` 是初始可用数；本实验 `(3, 1)`：上限 3、初始 1，于是"取光"可在无延时下确定复现。
- `xSemaphoreGive` 超过 `max` 会返回 `pdFALSE`（V11 默认断言，取决于 `configASSERT` 配置）。
- 常见误用：拿它当"事件计数"去数中断次数——超出 `max` 的部分同样被丢弃。

### `BaseType_t xSemaphoreTake(SemaphoreHandle_t s, TickType_t ticks)`

作用：取信号量，不可用时按 tick 阻塞。

- 返回 `pdTRUE` 取到；`pdFALSE` 超时。`0` 表示只尝试一次不阻塞。
- 互斥量借用此 API，但**必须由获取者释放**；普通信号量可由任意任务释放（ISR 也可以）。
- 超时参数用 `pdMS_TO_TICKS(ms)` 换算，不要直接写毫秒数。

### `BaseType_t xSemaphoreGiveFromISR(SemaphoreHandle_t s, BaseType_t *woken)`

作用：在中断里释放信号量。

- 永不阻塞；已满返回 `pdFALSE`（这是**合并语义**，不是错误）。
- `woken` 为输入输出参数；不关心就传 `NULL`，但那样必须确认不需要立即切换。

### `EventBits_t xEventGroupWaitBits(EventGroupHandle_t g, EventBits_t bits, BaseType_t clear, BaseType_t all, TickType_t ticks)`

作用：按位等待事件，返回退出时的位快照。

- 只有**置位**操作会唤醒等待者；清位（`xEventGroupClearBits`）不会。
- `bits` 不能包含 `configASSERT` 保留的位（`event_groups.h` 里定义为最高几位）。
- 同一任务不能同时等两个事件组（一个任务只有一套等待列表项）。

### `void xEventGroupSetBits(EventGroupHandle_t g, EventBits_t bits)`

作用：置位并可唤醒等待者。任务上下文版本；中断里用 `xEventGroupSetBitsFromISR`，
后者实际是"把置位动作挂到定时器守护任务"，因此**需要 `configUSE_TIMERS = 1`**——
本工程 `configUSE_TIMERS = 0`，所以中断里不能直接置位（见「已知限制」）。

### `UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t t)`

作用：返回任务栈的历史最小剩余量（字）。

- 需要 `INCLUDE_uxTaskGetStackHighWaterMark = 1`（本仓库配置已开）。
- **任务被删除后句柄失效**，再用它读水位就是访问已释放内存——本实验因此让 feeder 常驻不删。

### `size_t xPortGetFreeHeapSize(void)`

作用：当前内核堆剩余字节。

- `heap_4` 的剩余量在**第一次分配时**才初始化，所以启动时直接读会得到 0，
  只能在建完对象之后反算："用掉 = `configTOTAL_HEAP_SIZE` − 剩余"。

## 最小可运行实验

```text
F103_12_FreeRTOS_Sync/
├── CMakeLists.txt     # f103_add_freertos(... MODULES event_groups)
├── main.c             # 三个任务 + USART1 ISR + 四段演示
└── build-qemu/
```

```text
core (prio 3, 192 字)
 ├─ A  等 s_bin（portMAX_DELAY）           ← feeder 800 tick 后 give
 ├─ B  取 s_pool ×1 → 0 超时再取 ×1       ← 失败；feeder 归还 2 个后再取 2 次
 ├─ C  等 ARITH|SENSOR（all, 2000 tick）   ← feeder 先置 ARITH、200 tick 后置 SENSOR
 │     等 NEVER（300 tick）               ← 无人置位，走超时
 └─ D  等 s_uart（3000 tick）→ 再 slow-loop ← USART1 ISR give
feeder (prio 2, 128 字)  按上述节拍 give / setBits，之后常驻 1 s 周期延时
monitor (prio 1, 160 字) 每 1000 tick 打印堆余量与三个任务的栈水位
USART1_IRQHandler        读 SR/DR → xSemaphoreGiveFromISR → portYIELD_FROM_ISR
```

`main()` 在启动调度器前把 5 个同步对象全部建好，并把余量打印出来，
所以"建对象花了多少、建任务花了多少"是两笔可见的账。

## 编译与运行

```bash
cd Linux/experiments/F103_12_FreeRTOS_Sync
cmake -S . -B build-qemu -G Ninja -DF103_QEMU=ON \
  -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake \
  -DCMAKE_MAKE_PROGRAM="<ninja 路径>"
cmake --build build-qemu

cd ../../tests/qemu-stm32
bash run.sh exp12       # 单个用例（WSL / Linux 下执行）
```

真机：去掉 `-DF103_QEMU=ON`、构建目录换 `build`，然后 `cmake --build build --target flash`。

## 预期现象

```text
[F103_12] FreeRTOS V11.3.0 sync primitives, HCLK=8000000 Hz, tick=1000 Hz
[F103_12] pool=3 (init 1), USART1_IRQ prio=5
[F103_12] 五个同步对象（4 个信号量 + 1 个事件组）用掉 368 B
[F103_12] 二值信号量初值为空（符合预期）
[F103_12] 初始空闲堆 1496 B，启动调度器
[sync] bin waited=802 ticks, take ok
[sync] pool take1 ok, count=0
[sync] pool take2(0ms) result=0, count=0
[sync] pool refill take ok, count=0
[sync] pool refill take ok, count=0
[sync] event wait_all got=0x03 waited=200 ticks
[sync] event timeout got=0x00 waited=300 ticks
[sync] isr wake#1 bytes=7 gives=2
[sync] isr wake#2 bytes=7 gives=2
[sync] isr 空闲 300 tick 后无新事件，结束采集
[sync] isr summary bytes=7 gives=2 takes=2 coalesced=5
[sync] done heap_free=880 B
```

判定要点：

- `bin waited` 应≈800（feeder 的延时）+1～2 tick 调度开销。
- `pool take2(0ms) result=0` 中的 `0` 就是 `pdFALSE`：资源已取光，0 超时不阻塞立刻返回。
- `event wait_all got=0x03` 同时含两位；**`waited` 应≈200**（第二位在 200 tick 后才置位）——
  若≈0，说明 `xWaitForAllBits` 被写成了 `pdFALSE`（任一位即返回）。
- `event timeout got=0x00 waited=300`：超时值精确等于请求值，说明没有被误置位。
- `isr summary` 中 `gives < bytes` 即事件被合并的证据；`coalesced` 就是差额。

## 实测记录

环境：QEMU 10.2.1 `-M stm32vldiscovery`，gcc 14.2.1 `-Og -g3`。原始日志：
[`exp12_sync.log`](../../tests/qemu-stm32/logs/exp12_sync.log)（39 行）。

### 构建体积

| 区域 | 已用 | 占比（QEMU 128 KB / 8 KB） |
| --- | --- | --- |
| FLASH | 15988 B | 12.20% |
| RAM | 6408 B | 78.22% |

`text 15888 / data 100 / bss 6308`，构建告警 **0** 条。
比实验 11 多 2372 B Flash：`event_groups.c` 的实现 + 四段演示代码。
RAM 比实验 11 多 8 B，因为 `bss` 里的 4 KB 堆是固定占用，任务栈与同步对象都从堆内切分。
注意 **RAM 78% 是按 QEMU 的 8 KB 算的**；真板 C8T6 有 20 KB，这一项不构成约束。

### 内核堆（4 KB）的三段账

| 阶段 | 占用 | 说明 |
| --- | --- | --- |
| 同步对象 ×5 | **368 B** | 4 个信号量（每个 ≈ Queue_t + 事件存储区）+ 1 个事件组 |
| 三个任务 | **2232 B** | 栈 192+128+160 = 480 字（1920 B）+ 3 个 TCB + heap_4 块头 |
| idle 任务 | **616 B** | `configMINIMAL_STACK_SIZE = 128` 字 + TCB，由 `vTaskStartScheduler` 创建 |
| 稳态余量 | **880 B** | 20 次采样恒定不变，说明演示过程没有动态分配 |

这四行合起来正好对上 `4096 B`。**只看"最后剩 880 B"无法判断哪一段吃得多**，
这是把记账分开打印的原因。

### 四段演示的实测值

| 演示 | 期望 | 实测 | 说明 |
| --- | --- | --- | --- |
| A 二值信号量 | 阻塞≈800 tick | `waited=801`、`802`（两次运行） | 差值来自 tick 边界对齐，±1 属正常 |
| B 计数信号量取光 | `take2` 返回 `pdFALSE` | `result=0, count=0` | `pdFALSE == 0`，6 次采样一致 |
| B 归还后取值 | 两次都能取到 | `refill take ok` ×2，每次 `count=0` | 取走即减 1，所以打印时均为 0 |
| C 事件组全会合 | 返回 `0x03`，等待≈200 | `got=0x03, waited=201` / `200` | 第二位晚 200 tick 置位，`all` 语义成立 |
| C 事件组超时 | 返回 `0x00`，等待 300 | `got=0x00, waited=300` | 精确到请求值，无提前返回 |
| D 中断→信号量 | `gives < bytes` | `bytes=7, gives=2, takes=2, coalesced=5` | 7 字节只形成 2 个"事件" |

### D 段为什么是 2 次而不是 1 次

7 个字节进了同一个 ISR 路径，但成功的 give 是 2 次，要能逐步讲清：

```text
byte1  → give 成功（信号量空 → 满），gives=1，设置 woken → 挂起 PendSV
byte2~6 → give 失败（已满），事件被合并
        （USART1 IRQ 优先级 5 高于 PendSV，所以这些 ISR 先跑完，core 还没机会取）
core 取走 → 信号量空
byte7  → give 又成功，gives=2
core 打印 wake#1（此时 bytes=7, gives=2）→ 再慢一拍取走第二个事件（wake#2）
```

所以 `coalesced = 7 − 2 = 5` 的含义是"5 个字节没有形成独立事件"，
而不是"5 个字节丢了"——字节本身已在 ISR 里读走（本实验丢弃内容，只保留计数）。
若换成实验 11 的深度 32 队列，这 7 个字节会形成 7 项，任务被唤醒 7 次。

### 栈水位与其判据

| 任务 | 分配（字） | 最小剩余（字） | 已用 | 相对余量 | 绝对余量 | 判据 |
| --- | --- | --- | --- | --- | --- | --- |
| core | 192 | **86** | 106 | 45% | 344 B | 通过 |
| feeder | 128 | **96** | 32 | 75% | 384 B | 通过 |
| monitor | 160 | **58** | 102 | 36% | 232 B | 通过 |

按「核心概念 5」的三条判据（绝对 ≥32 字、相对 ≥25%、全路径跑完再读），三个任务都通过。
两点必须解释清楚：

- **水位数只会变小**。第一次采样的 `core=90 / monitor=136` 都比最终值大，
  因为那时 core 还没跑完四段演示、monitor 只打印过一次。**早期读数不能当结论。**
- **monitor 只有 58 字余量，虽然"通过"但最接近红线**。它调用 `printf`（含整型格式化），
  栈消耗比 core 的打印路径更深——这也是把 `LOG` 宏包在互斥量里、而不是在 monitor 里塞更多字段的原因。

### 对这几个数字的正确读法

- **tick 不等于真实毫秒**：QEMU 的固件时基是真实时间的 2.90 倍（24 MHz vs 8 MHz 假设）。
  `waited=800` tick ≈ 276 ms 真实时间。本实验所有"等待时间"结论只能当**语义证据**，不能当实时性数据。
- **`gives=2` 依赖 QEMU 的中断投递时序**。真机上串口字节间隔可能跨越 tick 边界，
  合并次数会不同——**结论是"会合并"，具体的 2 不能迁移到硬件**。
- **`coalesced` 是"未形成独立事件"而非"丢数据"**：本实验 ISR 里丢弃了字节内容，
  只保留计数，所以不能反推"数据完整性"。

## 关键实现点

### 1. 计数信号量用 `(3, 1)` 而不是 `(3, 3)`

初始给 3 个可用，`take` 永远成功，"取不到"的分支就测不到。给 1 个，
让"取光 → 0 超时失败 → 归还 → 带超时取到"这条完整路径在**无任何 sleep** 的前提下确定复现。
**可测性优先于"看起来自然"的初值。**

### 2. 0 超时 take 与带超时 take 都要出现

`take(0)` 验证"资源不可用时立即返回"，`take(1000 ms)` 验证"资源不可用时挂起而不是空转"。
两者都打印返回值与当时的计数——**只打印"成功"的日志无法区分"成功"和"没走这条路"**。

### 3. 事件组的超时分支必须测

只测全会合成功，等于没测 `xWaitForAllBits`。本实验专门等了 `EV_BIT_NEVER`
——一个永远不会被置位的位——并且断言 `waited == 300`。
若这个值变成 0，说明超时参数写错（例如把 tick 当毫秒）或被误置位，是很有价值的回归信号。

### 4. 中断里只做三件事

读 `SR`/`DR`、计数、`give`。**没有任何 `printf`**——实验 11 已量化过轮询发送的代价
（一行 40 字符约 3.5 ms，期间 SysTick 被推迟）。

### 5. feeder 常驻不删

最初版本让 feeder 执行完 `vTaskDelete(NULL)` 自删，并演示堆被回收（`heap_free` 880 → 1496）。
但这会让 monitor 拿着**已失效的句柄**去读栈水位——`uxTaskGetStackHighWaterMark`
访问的是已归还给堆的 TCB 与栈，属于未定义行为。
现在 feeder 常驻（每 1 s 空转一次），代价是 880 B 余量不回收，换来的是**所有读数都指向活任务**。
若确实要演示回收，正确做法是让 feeder 在自删前把句柄置 `NULL`，且 monitor 读到 `NULL` 时跳过。

### 6. `MODULES event_groups` 是显式开关

`f103_add_freertos()` 默认只链接 `list/queue/tasks` + 移植层 + `heap_4`。
事件组的实现在独立文件里，不显式加入就会在链接期报 `undefined reference to xEventGroupCreate`。
CMake 侧用 `MODULES` 参数按需加入，找不到文件时在**配置阶段**就报错，而不是等到链接。

## 常见错误和陷阱

| 现象 | 原因 | 处理 |
| --- | --- | --- |
| `undefined reference to xEventGroupCreate` | 没链接 `event_groups.c` | `f103_add_freertos(... MODULES event_groups)` |
| `undefined reference to xTimerPendFunctionCallFromISR` | 用了 `xEventGroupSetBitsFromISR` 但 `configUSE_TIMERS = 0` | 保持 `configUSE_TIMERS = 0` 时只能用任务侧 `xEventGroupSetBits` |
| 二值信号量"丢了"中断 | 把合并当成丢失 | 需要每事件一份时改用队列或计数信号量 |
| `take(0)` 总返回 `pdFALSE` | 计数信号量初始值给成 0 | 用 `(max, init)` 明确初始可用数 |
| 事件组等不到，`waited` ≈ 0 | `xWaitForAllBits` 传成 `pdFALSE` | 全会合必须 `pdTRUE` |
| 事件组每次都要重新等 | `xClearOnExit` 传成 `pdTRUE` 后没有重新置位 | 按语义选择是否清除 |
| 栈水位读出一个荒谬值 | 任务已删除，句柄失效 | 删除前注销句柄，或让任务常驻 |
| 读堆余量得到 0 | `heap_4` 首次分配前未初始化 | 先建对象，再用 `configTOTAL_HEAP_SIZE − 剩余` 反算 |
| `xSemaphoreGive` 之后任务没动 | ISR 里漏了 `portYIELD_FROM_ISR(woken)` | 补上，且用变量而非常量 |
| 运行中触发 `configASSERT` | USART1 优先级数值 < 5 | 优先级数值 ≥ `configMAX_SYSCALL_INTERRUPT_PRIORITY` |

## 故障排查方法

1. **先看初始化行**：`二值信号量初值为空` 与 `五个同步对象…用掉 N B` 正常，才继续往下看。
2. **`bin waited` 明显偏小**（如 <100）：feeder 的延时没生效，或 core 没真正阻塞（信号量初值非空）。
3. **`event wait_all` 的 `got` 只有一位**：`xWaitForAllBits` 传错，或第二个 `SetBits` 没执行到（看 feeder 是否被更高优先级任务饿死）。
4. **`isr summary` 里 `bytes=0`**：RXNE 中断没使能，或宿主机喂数据太晚（`run.sh` 的 `sleep 1` 需放宽）。
5. **`gives == bytes`**：消费者比生产者快，没观察到合并——把 D 段的 300 tick 间隔调大，或让 core 先延时。
6. **`heap_free` 逐次下降**：有动态分配泄漏（本实验期望恒定 880 B）。
7. **某个任务水位突然很小**：该任务第一次跑到了新分支（例如更深的 `printf` 格式），水位是历史最小值，不会回升。

## 练习题

基础题：

1. 把 `POOL_INIT` 从 1 改成 3，重跑并解释为什么 `pool take2(0ms) result` 会变成 1、`count` 会变成 1。
2. 把 D 段的 `xSemaphoreTake(s_uart, 0)` 改成 `xSemaphoreTake(s_uart, pdMS_TO_TICKS(300))`，
   观察 `takes` 与 `coalesced` 如何变化，并说明这对"消费者节奏决定合并量"意味着什么。

改错题：

3. 下面代码有三处问题，逐条指出并改正：
   ```c
   void USART1_IRQHandler(void)
   {
       xSemaphoreGive(s_uart);            /* 1 */
       xSemaphoreTake(s_uart, 10);        /* 2 */
       if (xEventGroupSetBitsFromISR(s_events, 0x01, NULL) == pdFAIL) { } /* 3 */
   }
   ```

综合题：

4. 用**计数信号量**替代 D 段的二值信号量，上限设为 8，观察 7 个字节是否形成 7 次 give，
   并对比 RAM 占用（`五个同步对象…用掉` 这一行）与 `heap_free`。
5. 给 monitor 增加"水位是否低于判据"的检查：低于 32 字时打印 `[WARN]`，
   然后在 core 里加一次深层递归或大块 `printf` 触发它，验证告警确实能发出。
6. 把 `EV_BIT_NEVER` 改由 feeder 在 500 tick 后置位，让"超时"变成"迟到成功"，
   记录 `waited` 与 `got`，说明这类测试为什么必须区分"超时"和"临界时刻到达"。

## 验收标准

- [ ] 能说清二值信号量、计数信号量、事件组、互斥量各自解决哪类问题。
- [ ] 能解释 `xWaitForAllBits` / `xClearOnExit` 两个开关的组合语义。
- [ ] 能在 QEMU 中复现四段演示，并说明每一段的关键数字来自哪里。
- [ ] 能用实测的 `bytes` / `gives` 解释"事件合并"，并说清它为什么不是丢数据。
- [ ] 能按三条判据判断某个任务的栈是否够用，并能指出本实验中最接近红线的任务。
- [ ] 能把 4 KB 内核堆拆成"同步对象 / 任务 / idle / 余量"四段并对上总数。
- [ ] 能列出本实验未覆盖的路径（`xEventGroupSetBitsFromISR`、硬件真实时序）。

## 延伸阅读

- FreeRTOS《Mastering the FreeRTOS Real Time Kernel》：Semaphores / Event Groups 两章，重点看"binary vs counting"的取舍。
- 内核源码：`../F103_common/vendor/freertos/Source/event_groups.c` 的 `xEventGroupWaitBits`（看它如何用 `uxControlBits` 区分"任务侧/ISR 侧"的置清）。
- 内核源码：`queue.c` 的 `xQueueGenericSend`——信号量就是"元素大小为 0/1 的队列"，理解这一点后大量 API 差异会消失。
- 移植层：`portmacro.h` 的 `portYIELD_FROM_ISR`，以及 `port.c` 里 `xPortPendSVHandler` 的切换过程。

## 下一步

1. **软件定时器**：`configUSE_TIMERS = 1` + `MODULES timers`，把"半行超时"做成 `xTimerStart` 用例，
   同时补上 `xEventGroupSetBitsFromISR` 的前置条件。
2. **栈监控定量化**：把水位判据做成编译期常量与运行期告警，接入 `[WARN]` 输出。
3. **看门狗心跳**：`IWDG` 在 QEMU 未建模，需要在真板闭环；仿真侧只能做"心跳标志 + 任务阻塞检测"。
4. **真板验证**：真机上重做 D 段，记录真实字节间隔下的 `gives`，替换本 README 中"2 次"这一 QEMU 值。

## 已知限制

- **`xEventGroupSetBitsFromISR` 未覆盖**：它依赖 `configUSE_TIMERS = 1`（把置位挂到守护任务），
  本工程为 0，因此中断里只能 give 信号量，不能置位事件组。这是刻意的配置取舍，不是遗漏。
- **D 段的合并次数不可迁移到真机**：`gives=2` 由 QEMU 的串口投递时序决定，
  真机字节间隔与中断延迟都不同；可以迁移的只有"会合并"这条定性结论。
- **没有优先级反转用例**：互斥量的优先级继承在本实验里只用到（`s_log_mutex`），没有构造反转场景。
- **没有阻塞定位用例**：只有 `waited` 观测，未做"任务卡在某信号量上"的运行时检测。
- **软件定时器 / 看门狗未做**：见「下一步」，属于 `CLAUDE.md` §12 P0 的剩余项。
- **未在真板运行**：所有结论来自 QEMU 仿真，时基偏差 2.90 倍，栈水位也与真机的编译器输出可能不同。
- **RAM 78% 是按 8 KB 的 QEMU 链接脚本算的**，不能直接套到 C8T6（20 KB）的占用率上。
