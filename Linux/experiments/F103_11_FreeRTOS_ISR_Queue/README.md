# F103_11：中断 → 任务通信（USART1 RXNE + FreeRTOS 队列）

实验 10 的队列两端都是任务，通信时机由任务自己决定。本实验把发起方换成**中断**，
用来回答一个只有中断上下文才有的问题：*ISR 知道自己唤醒了谁，要不要立刻切过去？*

这是 RTOS 工程里出错最集中的一条边界：用错 API 版本（`xQueueSend` vs `xQueueSendFromISR`）、
忘记 `portYIELD_FROM_ISR`、中断优先级配到临界区保护之外，三种错误都能"看起来正常"地跑很久。

## 学习目标

- 能把"ISR 里只能做三件事"落到代码：清标志、搬数据、决定是否切换。
- 说清 `xQueueSendFromISR` 与 `xQueueSend` 的差别，以及为什么后者在 ISR 里会挂死。
- 会用 `xHigherPriorityTaskWoken` + `portYIELD_FROM_ISR` 让中断退出后立刻切换到被唤醒的任务。
- 能解释 NVIC 优先级为什么必须 ≥ `configMAX_SYSCALL_INTERRUPT_PRIORITY`，且能算出本工程的门槛是 5。
- 能从 `max lat` 字段读出"中断投递 → 任务取走"的 tick 差，并知道这个数在 QEMU 里为什么不能当实时性指标。

## 环境与前置条件

| 项目 | 值 |
| --- | --- |
| 工具链 / 构建 | `arm-none-eabi-gcc 14.2.1`、CMake 4.0.3、Ninja 1.13.2（同实验 10） |
| 内核 | FreeRTOS V11.3.0（`../F103_common/vendor/freertos/`），配置同实验 10 |
| 硬件假设 | PA9 = TX，PA10 = RX，115200 8N1，USART1 = IRQ 37 |
| 仿真 | QEMU 10.2.1 `-M stm32vldiscovery`（USART1 是**真实模型**，RXNE 中断可用） |

前置：实验 05（寄存器级 USART1 中断接收）、实验 10（FreeRTOS 任务/队列/互斥量）。
不熟悉 `SR/DR` 的读清语义，先回实验 05 看 `USART_SR_RXNE | USART_SR_ORE` 的处理。

## 核心概念

### 1. 中断上下文能做什么

| 能做 | 不能做 | 原因 |
| --- | --- | --- |
| `xQueueSendFromISR` / `xQueueReceiveFromISR` | `xQueueSend` / `xQueueReceive` | 后者在满/空时按 tick 阻塞，中断里没有"任务"可挂起 |
| `xSemaphoreGiveFromISR` | `xSemaphoreTake` | 同上 |
| `xTaskGetTickCountFromISR` | `vTaskDelay` | 中断不能被自己延时 |
| 读写 `volatile` 标量 | `printf` / 轮询发送串口 | 115200 下一个字符约 87 µs，一行 40 字符 ≈ 3.5 ms，期间所有中断被推迟 |

### 2. `xHigherPriorityTaskWoken` 是唯一由 ISR 决定的事

```c
BaseType_t woken = pdFALSE;
xQueueSendFromISR(q, &item, &woken);   // 若唤醒了更高优先级任务，函数把它置 pdTRUE
portYIELD_FROM_ISR(woken);             // pdTRUE → 置 PendSV，退出中断时立刻切换
```

漏写 `portYIELD_FROM_ISR` 不会丢数据，但被唤醒的任务要等到**下一个 tick** 才运行，
延迟从"微秒级"退化到"最多 1 个 tick"。在 1000 Hz 配置下就是最多 1 ms——多数场合能容忍，
在高速采集或控制环里足以造成溢出。

### 3. 中断优先级门槛

FreeRTOS 的临界区不关全局中断，而是通过 `BASEPRI` 屏蔽"优先级数值 ≥ `configMAX_SYSCALL_INTERRUPT_PRIORITY`"的中断：

```text
本工程：configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5  →  BASEPRI = 0x50
优先级数值   0  1  2  3  4 │ 5  6  ...  15
            ← 不受临界区屏蔽 →│← 被屏蔽（因此可以安全调用 FreeRTOS API）
```

若把 USART1 配成 4 以下，它能在内核正修改队列链表时插进来，再调用 `xQueueSendFromISR`，
破坏数据结构——现象是随机丢字节、`uxQueueMessagesWaiting` 返回荒谬值或直接 HardFault。
**规则一句话：能调用任何 FreeRTOS API 的中断，优先级数值必须 ≥ 5。**

## 接口卡片

### `BaseType_t xQueueSendFromISR(QueueHandle_t q, const void *item, BaseType_t *pxHigherPriorityTaskWoken)`

作用：从中断上下文向队列尾部投递一份拷贝。

- **永不阻塞**：队列满时立即返回 `errQUEUE_FULL`，由调用者决定丢弃还是计数。
- `pxHigherPriorityTaskWoken` 是**输入输出参数**：进来时可以是 `pdFALSE`，唤醒更高优先级任务时被置 `pdTRUE`。
  可以在一次 ISR 里多次调用，只需在最后 `portYIELD_FROM_ISR` 一次。
- 传 `NULL` 也合法（不关心是否唤醒），但那样就必须自己确认"不需要立刻切换"，否则引入延迟。
- 中断里用 `...FromISR` 系列时，禁止调用任何可能引起阻塞的 API。

### `void portYIELD_FROM_ISR(BaseType_t x)`

作用：`x == pdTRUE` 时请求一次上下文切换（CM3 上写 `SCB->ICSR = PENDSVSET`）。

- 宏在 ARM_CM3 上可能展开为空语句，**必须带分号**，否则 `if/else` 会编译错。
- 真正切换发生在中断退出时（PendSV 最低优先级），ISR 本身不会被打断。
- 参数直接传 `xHigherPriorityTaskWoken` 变量，不要写 `portYIELD_FROM_ISR(pdTRUE)`——那会在每次中断后都切换，白白增加开销。

### `TickType_t xTaskGetTickCountFromISR(void)`

作用：中断里读取节拍计数。任务里对应 `xTaskGetTickCount()`。

- 两者实现不同：中断版不关中断、不做补偿，只做一次 32 位读。
- 本实验用它给每个字节打"到达时刻"，任务侧算差值即得延迟。

### `void NVIC_SetPriority(IRQn_Type irq, uint32_t prio)` / `NVIC_EnableIRQ`

作用：配置并使能外设中断。

- CMSIS 的 `prio` 是**未移位的逻辑优先级**（0～15），函数内部按 `8 - __NVIC_PRIO_BITS` 左移到寄存器高 4 位。
- 与 `configLIBRARY_*_INTERRUPT_PRIORITY` 比较时用逻辑值，与寄存器比较时用移过位的值——这是最常见的对不上号的地方。

## 最小可运行实验

```text
F103_11_FreeRTOS_ISR_Queue/
├── CMakeLists.txt     # NO_TICK + f103_add_freertos（路由由公共 CMake 加入）
├── main.c             # ISR + 消费任务 + 统计任务
└── build-qemu/
```

数据流：

```text
PC 串口 ──► USART1 RXNE ──► USART1_IRQHandler
                              ├─ 读 SR/DR（清标志）
                              ├─ xTaskGetTickCountFromISR()   记到达时刻
                              ├─ xQueueSendFromISR(深度 32)    满则丢弃并计数
                              └─ portYIELD_FROM_ISR(woken)
                                        ↓ (PendSV，中断退出时)
                               consume 任务（优先级 3）醒来
                              ├─ 累计字符直到 '\n'
                              ├─ 行超长（≥63 字节）则丢弃计数
                              └─ 打印 [line N] "..." len=K
```

两个刻意的设计：

1. **中断使能写在 `consume` 任务里**，不是 `main()` 里。原因见「常见错误和陷阱」第 4 条。
2. **队列元素带 `tick` 字段**，否则"中断到任务的延迟"无法观测，只能靠猜。

## 编译与烧写

```bash
cd Linux/experiments/F103_11_FreeRTOS_ISR_Queue
cmake -S . -B build-qemu -G Ninja -DF103_QEMU=ON \
  -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake \
  -DCMAKE_MAKE_PROGRAM="<ninja 路径>"
cmake --build build-qemu

cd ../../tests/qemu-stm32
bash run.sh exp11      # 两个用例：两行文本 + 300 字节突发
```

真机：去掉 `-DF103_QEMU=ON`、构建目录换 `build`，然后 `cmake --build build --target flash`。

## 预期现象

```text
[F103_11] FreeRTOS ISR→task: USART1 RXNE + queue(depth=32), NVIC prio=5 (= MAX_SYSCALL)
任务已创建，启动调度器（RXNE 中断由 consume 任务使能）
[stats] tick=1001 rx=0 lines=0 qdrop=0 ore=0 trunc=0 queued=0 lat_max=0 heap_free=1048
[line 1] "hello" len=5
[stats] tick=3008 rx=7 lines=1 qdrop=0 ore=0 trunc=0 queued=0 lat_max=2 heap_free=1048
[line 2] "world" len=5
[stats] tick=6020 rx=14 lines=2 qdrop=0 ore=0 trunc=0 queued=0 lat_max=2 heap_free=1048
```

判定要点：

- `rx` 每次输入 `hello\r\n` 应 +7（含 `\r` 与 `\n`），`lines` +1。
- `queued` 应长期为 0：消费速度高于输入速率。
- `lat_max` 是全程最大值，只增不减；新的一次运行从 0 重新开始。

## 实测记录

环境：QEMU 10.2.1 `-M stm32vldiscovery`，gcc 14.2.1 `-Og -g3`。原始日志：
[`exp11_isr_queue.log`](../../tests/qemu-stm32/logs/exp11_isr_queue.log)、
[`exp11_burst.log`](../../tests/qemu-stm32/logs/exp11_burst.log)。

### 构建体积

| 区域 | 已用 | 占比（QEMU 128 KB / 8 KB） |
| --- | --- | --- |
| FLASH | 13616 B | 10.39% |
| RAM | 6400 B | 78.13% |

比实验 10 多 336 B Flash（ISR + 行缓冲 + 统计字段），RAM 相同——因为 4 KB 堆是 bss 里的固定占用，
任务栈与队列都从堆内切分，不改变 `data + bss` 总量。
堆内余量实测 **1048 B**（实验 10 是 496 B）：本实验只建两个任务，队列加大 192 B 抵不上少一个任务的栈。
构建告警 0 条。

### 用例 1：两行文本（7 秒真实时间）

| 指标 | 实测 |
| --- | --- |
| `rx` | 0 → 7 → 14（每次输入恰好 +7） |
| `lines` | 0 → 1 → 2 |
| `[line 1]` / `[line 2]` | `"hello"` / `"world"`，`len=5`（`\r` 被忽略） |
| `qdrop` / `ore` / `trunc` | 0 / 0 / 0 |
| `queued` | 0（每次采样都是 0） |
| `lat_max` | **2 tick** |
| `heap_free` | **1048 B 恒定**（20 次采样无变化） |
| `stats` 间隔 | 1001、2006、3008、4015…（≈1002–1006 tick） |

### 用例 2：300 字节无换行突发（6 秒真实时间）

```text
[line 1] "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" len=44
[stats] tick=3005 rx=302 lines=1 qdrop=0 ore=0 trunc=4 queued=0 lat_max=2 heap_free=1048
```

算术自洽性核对（这条是本实验最硬的证据）：每轮截断存 63 字节、丢 1 字节（共 64），
`300 = 4×64 + 44` → `trunc=4`、末行 `len=44`，与输出完全一致。
说明 ISR 侧逐字节投递、任务侧累积与截断计数三处都没有错位或丢帧。
`rx=302` 多出的 2 是随突发一起发出的 `\r\n`。

### 对这两个数字的正确读法

- **`lat_max=2` tick 不代表 2 ms**：QEMU 的固件时基是真实的 2.93 倍（24 MHz vs 8 MHz 假设），
  2 tick ≈ 0.68 ms 真实时间——但这仍不能当实时性结论，因为 QEMU 没有总线/中断延迟模型，
  中断几乎"瞬时"到达，真机的 `lat_max` 只会更大。
- **`qdrop` 恒为 0**，说明队列从未满：消费任务一次取一个字节、只在整行结束才 `printf`，
  处理速度远高于 115200 的输入速率。**ISR 侧丢弃分支没有被测到**（见「已知限制」）。
- **`ore` 恒为 0**：QEMU 不做比特级移位，不会产生真 UART 的溢出，这条分支同样未验证。

## 关键实现点

### 1. 为什么 `printf` 绝对不能进 ISR

`f103_uart_putc` 是轮询发送：写 `DR` 后等 `TXE`。115200 下每字符约 87 µs，一行 40 字符约 3.5 ms。
这 3.5 ms 里 SysTick 被推迟（同优先级或更低的中断都进不来），节拍计数落后，
`vTaskDelay` 到期时间全部顺延。在 RTOS 里，**ISR 的唯一时间预算应该是微秒级**，
任何"等硬件"的操作都必须交给任务。

### 2. 队列元素为什么要带 tick

只传 `uint8_t` 也能工作，但"中断 → 任务"的延迟就只能靠推断。加一个 `TickType_t`
（元素从 1 字节变 8 字节，队列多占 224 B RAM）换来可观测性，是值得的交换——
这是本实验能在无示波器条件下拿到运行时证据的原因。

### 3. 行累积放在任务里，不放在 ISR 里

ISR 每字节只做一次投递，行的语义（`\r` 忽略、`\n` 结束、超长截断）全部在任务里。
好处：协议解析变复杂时不会挤占中断时间，而且解析逻辑可以用普通 C 调试。

### 4. 中断使能放在任务里

若在 `vTaskStartScheduler()` 之前使能 RXNE，而恰好此时来了一个字节，ISR 会调用
`xQueueSendFromISR` 并在 `portYIELD_FROM_ISR` 里挂起 PendSV——但此时内核还没有当前任务
（`pxCurrentTCB` 为空），切换无处可去。把使能动作放进 `consume` 任务，天然保证
"有任何中断发生时，调度器已经在跑"。

（这条是**分析结论**，未在 QEMU 或硬件上复现该崩溃；QEMU 的时序也几乎不可能踩中这个窗口。）

## 常见错误和陷阱

| 现象 | 原因 | 处理 |
| --- | --- | --- |
| 中断里调用 `xQueueSend` 后卡死 | 队列满 → 按 tick 阻塞，中断无法挂起 | 一律用 `...FromISR` 版本 |
| 数据收到了，但任务延迟 1 ms 才醒 | 漏 `portYIELD_FROM_ISR`，或传了 `pdFALSE` | 检查 `woken` 是否被置位并传给宏 |
| 偶发丢字节 / 队列状态错乱 | USART1 优先级被配成 0–4，逃出临界区保护 | 优先级数值 ≥ `configMAX_SYSCALL_INTERRUPT_PRIORITY` |
| `portYIELD_FROM_ISR` 附近编译报错 | 宏展开为空语句，缺分号 | 后面补分号 |
| ISR 里 `printf` 后系统节拍变慢 | 轮询发送阻塞数毫秒，SysTick 被推迟 | ISR 只投递，输出交给任务 |
| 中断一次都不进 | `CR1` 的 `RXNEIE` 没置位，或 `NVIC_EnableIRQ` 漏调 | 两处都要做；`RXNEIE` 同时放行 RXNE 与 ORE |
| 行输出被截成两半 | 两个任务同时 `printf` | 用互斥量包住整行（实验 10 的 `LOG` 宏） |

## 故障排查方法

1. **先确认中断在进**：把 `rx` 计数放到 ISR 里（本实验已如此），若 `rx` 不涨说明中断没使能。
2. **再看队列是否满**：`queued` 长期大于 0 → 消费太慢；恒为 0 且 `qdrop` 涨 → 队列深度不够。
3. **看 `lat_max`**：恒为 0 或 1 说明切换及时；持续 ≥ 2 且随负载增长，检查是否漏了 `portYIELD_FROM_ISR`。
4. **`ore` 增长**：真机上说明 ISR 响应太慢（被更高优先级中断或临界区挡住），检查最长的关中断区间。
5. **断言落在 `port.c`**：几乎总是优先级配置问题（`configASSERT( ucCurrentPriority >= ucMaxSysCallPriority )` 一类）。
6. **QEMU 与真机的差异**：仿真里中断没有延迟、时基快 2.93 倍，`lat_max` 只能当"上限存在性"参考。

## 练习题

基础题：

1. 把队列深度从 32 改成 4，用 `bash run.sh exp11` 的突发用例观察 `qdrop` 是否出现；解释为什么当前配置下它是 0。
2. 把 `portYIELD_FROM_ISR(woken)` 改成 `portYIELD_FROM_ISR(pdFALSE)`，观察任务被唤醒的时机与 `lat_max` 的变化，并说明代价。

改错题：

3. 下面代码有三处违反 ISR 规则，逐条指出并改正：
   ```c
   void USART1_IRQHandler(void)
   {
       printf("rx=%c\n", USART1->DR);
       xQueueSend(s_rx_queue, &byte, 0);
       vTaskDelay(1);
   }
   ```

综合题：

4. 让 `consume` 任务在打印前 `vTaskDelay(pdMS_TO_TICKS(20))`（模拟慢消费者），
   再用 300 字节突发，让 `qdrop` 真正增长；记录 `queued` 峰值与 `qdrop` 的关系，
   并解释为什么丢包发生在 ISR 而不是任务。
5. 把 `USART1_IRQ_PRIO` 改成 4，用 `configASSERT` 观察是否触发断言，说明断言背后检查的是哪个不变量。
6. 增加一条第二队列：ISR 同时向"原始字节"和"行完成"两个队列投递（行完成只投递行号），
   比较单队列版本的耦合度与 RAM 占用，说明什么时候值得拆。

## 验收标准

- [ ] 能说清 ISR 里三件合法的事，以及为什么 `printf` 不在其中。
- [ ] 能解释 `xHigherPriorityTaskWoken` 的语义与漏用 `portYIELD_FROM_ISR` 的后果。
- [ ] 能算出本工程的中断优先级门槛（5）并配对 CMSIS 的逻辑优先级与寄存器值。
- [ ] 能在 QEMU 中复现两个用例，并用 `300 = 4×64 + 44` 验证截断路径。
- [ ] 能说明 `lat_max=2` 为什么不能当实时性指标。
- [ ] 能列出本实验未覆盖的两条分支（ISR 侧丢弃、硬件 ORE）。

## 延伸阅读

- FreeRTOS《Mastering the FreeRTOS Real Time Kernel》：中断管理与"Deferred interrupt processing"章节。
- 内核源码：`../F103_common/vendor/freertos/Source/queue.c` 的 `xQueueGenericSendFromISR`（看它如何设置 `pxHigherPriorityTaskWoken`）。
- 移植层：`../F103_common/vendor/freertos/Source/portable/GCC/ARM_CM3/portmacro.h` 中 `portYIELD_FROM_ISR` / `portEND_SWITCHING_ISR` 的定义。
- ARM Cortex-M3 文档：`BASEPRI` 与优先级分组，理解"为什么不关全局中断也能保护临界区"。

## 下一步

1. **信号量版**：用 `xSemaphoreGiveFromISR` + `xSemaphoreTake` 替代队列做纯同步，对比 RAM 与延迟。
2. **软件定时器**：`configUSE_TIMERS = 1`，把行超时（半行不完整）交给定时器任务处理。
3. **真机验证**：上板后重做本节的 `lat_max` 与 `ore`，QEMU 三个"未验证"项届时才能补齐。

## 已知限制

- **ISR 侧丢弃分支未实测**：`qdrop` 全程为 0，消费速度远高于输入。练习题 4 给出了强制触发的做法，但结果尚未记录。
- **硬件 ORE 未实测**：QEMU 不做比特级移位，不会溢出。
- **`lat_max` 不可作为实时性指标**：QEMU 无中断延迟模型，且时基快 2.93 倍；真机数值未知。
- **"调度器启动前使能中断会崩溃"是分析结论**，未复现。
- 行缓冲固定 64 B，超长行直接丢弃（只计数），没有分片或流式处理——这是刻意的简化。
- 未使用 `configUSE_TRACE_FACILITY`，无法导出任务状态；未接入 `vApplicationIdleHook` 统计 CPU 占用。
- 只在 QEMU 中运行，真板的串口接线（PA9/PA10）、电平、波特率均未验证。
