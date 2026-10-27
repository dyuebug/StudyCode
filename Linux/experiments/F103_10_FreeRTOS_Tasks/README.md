# F103_10：FreeRTOS 最小可运行系统（任务 / 队列 / 互斥量）

在 F103 上把 FreeRTOS 内核跑起来，让"调度器真的在切换任务"变成串口上可核对的输出，
而不是靠灯闪猜。这是从裸机寄存器编程进入 RTOS 的第一道门：以后所有 FreeRTOS 实验
（信号量、软件定时器、事件组、中断与任务通信）都以本实验的构建方式和异常路由为底座。

## 学习目标

- 说清 FreeRTOS 在 CM3 上接管了哪三个异常（SVC / PendSV / SysTick），以及为什么必须是它们。
- 能解释"向量表名字与移植层函数名不一致"时为什么要用尾跳转而不是 C 包装函数。
- 用队列解耦生产者与消费者，用互斥量保护共享的串口输出。
- 会读三个关键指标：`xPortGetFreeHeapSize()`、`uxTaskGetStackHighWaterMark()`、任务 tick 计数。
- 能判断"时间相关的结论"在 QEMU 里哪部分可信（相对比例）、哪部分不可信（绝对毫秒）。

## 环境与前置条件

| 项目 | 版本 / 位置 |
| --- | --- |
| 交叉工具链 | `arm-none-eabi-gcc 14.2.1`（Windows 侧，MSYS2 / xpack 安装） |
| 构建 | CMake 4.0.3 + Ninja 1.13.2（Ninja 不在 PATH，需显式 `-DCMAKE_MAKE_PROGRAM=`） |
| FreeRTOS 内核 | V11.3.0，已裁剪并入库：`../F103_common/vendor/freertos/`（来源与裁剪范围见 `../F103_common/vendor/freertos/README.md`） |
| 仿真 | QEMU 10.2.1 `-M stm32vldiscovery`（装在 **WSL Ubuntu** 内，不在 Windows 侧） |
| 硬件 | STM32F103C8T6（64 KB Flash / 20 KB SRAM）；本实验的 QEMU 链接脚本按 **F100RB** 的 128 KB / 8 KB 编址，见下文 |

前置阅读：本仓库 `F103_01`～`F103_05`（GPIO、SysTick 轮询延时、USART1 轮询与中断）。
FreeRTOS 的移植层依赖 SysTick 与 PendSV，若不清楚异常进入/返回的硬件动作，先补 `F103_03` 的 EXTI/NVIC 章节。

## 核心概念

### 1. 三个异常的职责

| 异常 | 移植层函数 | 作用 |
| --- | --- | --- |
| SVCall | `vPortSVCHandler` | 首次启动调度器：从 MSP 切到 PSP，把栈指针指向第一个任务的栈，执行 `bx r14` 进入任务 |
| PendSV | `xPortPendSVHandler` | 任务切换本体：保存当前任务上下文（R4–R11）到其栈、更新 `pxCurrentTCB`、恢复下一个任务上下文 |
| SysTick | `xPortSysTickHandler` | 每 tick 递增节拍计数、处理延时到期、需要时挂起 PendSV 请求切换 |

关键点：**PendSV 被设为最低优先级**，所以任务切换只会在所有中断都退出之后发生，
不会打断一个正在执行的中断服务程序。这也是"临界区只关中断、不关调度器"能工作的前提。

### 2. 向量表名字不匹配：间接路由

启动文件 `startup_stm32f103xb.s` 的向量表写死了 `SVC_Handler` / `PendSV_Handler` / `SysTick_Handler`，
FreeRTOS 移植层导出的却是 `vPortSVCHandler` / `xPortPendSVHandler` / `xPortSysTickHandler`。
本实验用 `../F103_common/src/f103_freertos_vectors.s` 里的三条 `b`（尾跳转）把前者接到后者上，并用
`configCHECK_HANDLER_INSTALLATION = 0` 告诉内核"我是间接路由，别做直接路由校验"。

### 3. 任务状态与抢占

本实验用三个优先级不同的任务，让调度行为可以**从时间戳上读出来**：

```text
优先级 3  consume  —— 阻塞在 xQueueReceive(portMAX_DELAY)
优先级 2  produce  —— 每 200 ms 投一条，投递成功即被 consume 抢占
优先级 1  monitor  —— 每 1000 ms 打印计数
```

`xQueueSend` 唤醒更高优先级的等待者时，内核挂起 PendSV，**在中断退出时**完成切换。
所以 `[consume]` 行的 tick 与生产时刻几乎相同，而不是等到下一个 tick。

### 4. heap_4 与 RAM 预算

`heap_4.c` 提供带合并的首次适配分配：任务栈、TCB、队列、互斥量全部从同一个
`ucHeap[configTOTAL_HEAP_SIZE]` 里切。F103C8T6 只有 20 KB SRAM，本实验把堆设为 4 KB，
并且**把剩余堆和每个任务的栈高水位打出来**，否则"RAM 用了 78%"这种数字无法回答
"是哪个任务快把栈用穿了"。

### 5. 相对延时

`vTaskDelay(n)` 是**相对**延时：从调用时刻起阻塞 n 个 tick，而不是对齐到 n 的整数倍。
所以周期的实际值是 `周期 + 本周期执行时间`，会持续偏大几个 tick——这是读日志时必须预期到的现象，
需要严格等间隔时应改用 `xTaskDelayUntil()`。

## 接口卡片

### `BaseType_t xTaskCreate(TaskFunction_t fn, const char *name, configSTACK_DEPTH_TYPE depth, void *arg, UBaseType_t prio, TaskHandle_t *handle)`

作用：动态创建任务（栈 + TCB 都从 FreeRTOS 堆分配）。

- `depth` 单位是**字（4 字节）**，不是字节：`256` 即 1 KB。`configMINIMAL_STACK_SIZE` 同理。
- `prio` 越大越高，范围 `0 ~ configMAX_PRIORITIES-1`；本配置 `configMAX_PRIORITIES = 5`。
- 返回 `errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY` 表示堆不足；栈不足则运行期才由溢出钩子暴露。
- 资源所有权：栈和 TCB 由内核管理，`vTaskDelete` 后才回收；`handle` 可为 `NULL`。

### `void vTaskDelay(const TickType_t xTicksToDelay)`

作用：让当前任务进入阻塞态至少 n 个 tick。

- 换算用 `pdMS_TO_TICKS(ms)`，依赖 `configTICK_RATE_HZ`；本配置 1000 Hz，故 1 tick = 1 ms。
- 相对语义，会累积执行时间的偏移；周期任务优先用 `xTaskDelayUntil()`。
- 不能在中断里调用；中断里只能用 `...FromISR()` 系列。

### `QueueHandle_t xQueueCreate(UBaseType_t length, UBaseType_t itemSize)`

作用：创建定长队列，元素按值拷贝（不是存指针）。

- 内存 = `length * itemSize` + 队列控制块，都来自 FreeRTOS 堆。
- 返回 `NULL` 表示堆不足；本实验在 `main()` 里显式检查。
- 队列本身是任务间/任务与中断间通信的**唯一推荐**原语（数据 + 同步一次完成）。

### `BaseType_t xQueueSend(QueueHandle_t q, const void *item, TickType_t wait)`

作用：向队列尾部投递一份拷贝。

- 满时阻塞最多 `wait` 个 tick；本实验用 50 ms 上限，宁可丢一条并计数，也不让采集任务堵死。
- 中断里用 `xQueueSendFromISR()`，并用返回值 `xHigherPriorityTaskWoken` 决定是否 `portYIELD_FROM_ISR()`。

### `BaseType_t xQueueReceive(QueueHandle_t q, void *buf, TickType_t wait)`

作用：取走队首元素；`portMAX_DELAY` 表示无限等待（任务进入阻塞态，不占 CPU）。

### `SemaphoreHandle_t xSemaphoreCreateMutex(void)`

作用：创建互斥量，用于保护 `printf` 这类非原子共享资源。

- 互斥量带**优先级继承**：低优先级持有者会被临时提升到等待者的优先级，缓解优先级反转。
- 只能任务用，不能中断用（中断里用二值信号量）。
- 使用半径要小：本实验只包住 `printf`，不包住队列操作。

### `TickType_t xTaskGetTickCount(void)`

作用：读取调度器启动以来的 tick 数。换算成毫秒需 `* portTICK_PERIOD_MS`。

### `UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t t)`

作用：返回该任务**历史最小剩余栈**（单位：字）。用于判断栈是否给够，`0` 表示已经用穿。

### `size_t xPortGetFreeHeapSize(void)`

作用：读取 FreeRTOS 堆剩余字节数（`heap_4` 提供）。不返回碎片信息，但足以发现泄漏趋势。

## 最小可运行实验

```text
F103_10_FreeRTOS_Tasks/
├── CMakeLists.txt          # NO_TICK + f103_add_freertos
├── main.c                  # 三个任务、队列、互斥量、三个钩子
└── build-qemu/             # 仿真构建产物（不入库）

../F103_common/src/f103_freertos_vectors.s   # SVC/PendSV/SysTick 尾跳转（公共，由 f103_add_freertos 加入）
```

`main()` 的顺序有讲究：

1. `SystemCoreClockUpdate()` + `f103_uart_init(115200)` —— 串口必须在调度器之前可用，否则早期断言无处打印。
2. `xQueueCreate` / `xSemaphoreCreateMutex` —— 在创建任务之前把对象备好，避免任务里再判断 `NULL`。
3. **先建 consumer（优先级最高）**，让它一启动就阻塞在队列上；再建 producer。这样 producer 第一次投递就能观察到抢占。
4. `vTaskStartScheduler()` —— 正常情况下不返回；返回只意味着堆不够。

钩子（`vAssertCalled` / `vApplicationMallocFailedHook` / `vApplicationStackOverflowHook`）
在裸机上没有 OS 帮忙兜底，只能关中断并停住，等调试器或串口输出。本实验三个钩子都被触发过是**不该发生**的事。

## 编译与烧写

### 仿真（QEMU，WSL 内运行）

```bash
# 在 Linux/experiments/F103_10_FreeRTOS_Tasks 下
cmake -S . -B build-qemu -G Ninja -DF103_QEMU=ON \
  -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake \
  -DCMAKE_MAKE_PROGRAM="C:/Users/yanc/AppData/Local/Microsoft/WinGet/Packages/Ninja-build.Ninja_Microsoft.Winget.Source_8wekyb3d8bbwe/ninja.exe"
cmake --build build-qemu

# 在 Linux/tests/qemu-stm32 下
./run.sh exp10        # 6 秒，日志写入 logs/exp10_freertos.log
```

`-DF103_QEMU=ON` 会切到 `../F103_common/ld/STM32F100RB_qemu.ld`（128 KB Flash / 8 KB RAM）。
**8 KB RAM 不是 C8T6 的真实配置**，所以本实验在 QEMU 里的 RAM 占用率偏悲观——真机上 20 KB 会宽松得多。

### 真机（ST-Link）

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake \
  -DCMAKE_MAKE_PROGRAM="<ninja 路径>"
cmake --build build
cmake --build build --target flash      # 需要 openocd（配置见 F103_common/openocd/）
```

真机串口：USART1 PA9(TX)/PA10(RX)，115200 8N1。

## 预期现象

```text
[F103_10] FreeRTOS V11.3.0 on STM32F103, HCLK=8000000 Hz, tick=1000 Hz
任务已创建，启动调度器（初始空闲堆 1112 B）
[consume] seq=1 tick=1
[consume] seq=2 tick=206
...
[monitor] tick=1006 produced=5 consumed=5 dropped=0 queued=0
[res] heap_free=496 B, stack_hwm consume=154 produce=126 monitor=90 words
```

判定要点：

- 每 1000 个 tick 有 **5 条** `consume`，即 200 ms 生产周期与 1000 ms 上报周期严格成 1:5。
- `produced == consumed`、`dropped == 0`、`queued == 0`：消费能力不低于生产，队列从未积压。
- `heap_free` 在 17 次采样中**恒定不变**：没有持续分配/泄漏。
- `stack_hwm` 稳定（不逐次下降）才说明栈够用；若逐次变小则仍有更深调用路径未被覆盖。

## 实测记录

以下全部为 **QEMU 10.2.1 / `-M stm32vldiscovery`** 的实测数据，不是推算。
原始日志：[`../../tests/qemu-stm32/logs/exp10_freertos.log`](../../tests/qemu-stm32/logs/exp10_freertos.log)。

### 构建体积（arm-none-eabi-gcc 14.2.1，`-Og -g3`）

| 区域 | 已用 | 区域大小 | 占比 |
| --- | --- | --- | --- |
| FLASH | 13280 B | 128 KB | 10.13% |
| RAM | 6400 B | 8 KB | 78.13% |

其中 `ucHeap` 固定占 4096 B（bss）。**RAM 占比高主要来自堆，不是代码膨胀**：
改用 C8T6 链接脚本（20 KB SRAM）构建同一份代码，实测 FLASH 13280 B / 64 KB（20.26%）、RAM 6400 B / 20 KB（31.25%）。
两个数字都来自链接器输出，**未上板**。

> 构建告警：0 条（`-Wall -Wextra`）。

### 调度与队列（6 秒真实时间，17 次 monitor 采样）

| 指标 | 实测值 |
| --- | --- |
| `consume` 总条数 | 88（末条 `tick=17584`） |
| 生产间隔（tick 差） | 201×15、202×49、203×22、204×1（众数 **202**） |
| 上报间隔（tick 差） | 1003–1010（众数 ≈ 1006） |
| `produced / consumed / dropped / queued` | 85 / 85 / 0 / 0（末次采样） |

两个"偏大"的读法（都属预期，不是缺陷）：

1. **生产间隔 202 而非 200**：`vTaskDelay(200)` 是相对延时，实际周期 = 200 + 本周期执行时间（队列投递 + 唤醒切换 + printf）。1 tick 的 `pdMS_TO_TICKS` 取整再加约 1 tick 的执行开销，正好落在 201–203。
2. **上报间隔 ≈ 1006 而非 1000**：monitor 每次要打两行共 150+ 字节到 115200 的串口（约 13 ms 发送时间被 `printf` 内的忙等吃掉），这部分时间自然叠加到相对延时上。

### 内存（`heap_4`，`configTOTAL_HEAP_SIZE = 4096`）

| 时点 | 空闲堆 | 说明 |
| --- | --- | --- |
| 创建队列/互斥量/三个任务后 | **1112 B** | 队列 8×8 B + 控制块、互斥量、3×(TCB + 栈 256/160/192 字) ≈ 2984 B |
| 调度器启动后（空闲任务就位） | **496 B** | 空闲任务 TCB + 128 字栈再吃掉约 616 B |
| 运行 17 次采样（约 17.6 秒固件时间） | **496 B 恒定** | 无泄漏 |

栈高水位（剩余量，字；括号内为按 4 字节换算的已用量）：

| 任务 | 分配 | 剩余（首次 → 末次） | 已用 |
| --- | --- | --- | --- |
| consume | 256 字 | 154 → 154 | 102 字 ≈ **408 B** |
| produce | 160 字 | 126 → 126 | 34 字 ≈ **136 B** |
| monitor | 192 字 | 98 → 90 | ≈102 字 ≈ **408 B** |

**这组数字暴露了一个真实约束**：4 KB 堆在 `heap_4` 下只剩 496 B。再开一个任务、
一个软件定时器或一个流缓冲就会落到 `vApplicationMallocFailedHook`。
`main.c` 里 print 在栈上的开销也可疑——带 `printf` 的任务（consume / monitor）
栈用量是纯计算任务（produce）的 **3 倍**，因为 newlib 的 `printf` 路径较深。

### 时基保真度

固件末次 tick 17584 / 真实 6000 ms ≈ **2.93 倍**（理论 24 MHz ÷ 8 MHz = 3.00）。
原因见 [`../../tests/qemu-stm32/RESULT.md`](../../tests/qemu-stm32/RESULT.md)：
QEMU 的 `stm32vldiscovery` 以 24 MHz 驱动 SYSCLK，而固件按 8 MHz 计算 SysTick 重装值。

因此本实验在仿真中**可验证**的是：调度顺序、抢占关系、队列交接、互斥量、堆与栈的稳定性、
以及各周期之间的**相对比例**（200 : 1000 → 1 : 5 完全吻合）。
**不可验证**的是：绝对 200 ms / 1000 ms、真机上的 tick 精度、以及任何实时性结论。

## 关键实现点

### 1. 为什么用 `b` 尾跳转，而不是 C 包装函数

`vPortSVCHandler` 与 `xPortPendSVHandler` 是 `naked` 函数，结尾执行 `bx r14`
（`r14` 里是 `EXC_RETURN`）直接回到任务上下文，**永远不会 `pop` 返回**。
若写成：

```c
void PendSV_Handler(void) { xPortPendSVHandler(); }   /* 错 */
```

编译器会为包装函数建栈帧，而这个栈帧再也没有机会弹出。PendSV 每个 tick 都进一次，
MSP 会一路下移，最终踩穿栈底——现象是"跑几十秒后随机 HardFault"，极难定位。
`b` 不产生栈帧，语义等价于"向量表直接指向移植层函数"。

顺带解释 `configCHECK_HANDLER_INSTALLATION = 0`：向量表里存的是 `PendSV_Handler` 的地址，
不是 `xPortPendSVHandler` 的地址，port.c 的"直接路由"校验必然失败（这是官方文档所说的间接路由）。

### 2. `NO_TICK`：SysTick 不能有两个主人

`F103_common` 的 `f103_tick.c` 也定义了 `SysTick_Handler`。与 FreeRTOS 同时链接就是符号重定义，
所以本实验在 `f103_add_firmware(... NO_TICK ...)` 里显式排除它，毫秒延时统一改用 `vTaskDelay`。

### 3. 先建消费者

如果先建 producer，它会在 consumer 存在之前就跑起来，第一次投递的抢占关系就观察不到（队列已有数据，消费者立刻取走，两者顺序看起来"正常"）。
先建高优先级消费者 → 它阻塞 → producer 投递 → 唤醒 → **立即抢占**，tick 差能证明这一点。

### 4. 互斥量只包 `printf`，不包业务

`LOG()` 宏把一次 `printf` 调用整体放进互斥量。若把队列操作也包进去，
持锁时间变长，优先级继承会把低优先级任务的优先级抬高，反而增加切换；
同时容易出现"持锁时又去等队列"的锁顺序问题。

### 5. 用 `pdMS_TO_TICKS`，不写死 tick 数

`pdMS_TO_TICKS(200)` 在 1000 Hz 配置下等于 200；一旦把 `configTICK_RATE_HZ` 改成 100，
它会自动变成 20。写死 `vTaskDelay(200)` 则会把周期变成 2 秒——这类移植错误很难一眼看出。

## 常见错误和陷阱

| 现象 | 原因 | 处理 |
| --- | --- | --- |
| `undefined reference to vTaskDelay` | V11 的 `INCLUDE_vTaskDelay` 默认是 0 | 在 `FreeRTOSConfig.h` 里显式置 1 |
| 链接时报 `SysTick_Handler` 重复定义 | 同时链接了 `f103_tick.c` | 用 `NO_TICK` 排除 |
| 启动后几十秒随机 HardFault | 用 C 函数包装 PendSV/SVC 处理程序，MSP 持续下移 | 改汇编尾跳转 `b` |
| 进 `vApplicationStackOverflowHook` | 任务栈单位是字，按字节估算会小 4 倍；`printf` 很吃栈 | 栈深度按字给，参考上表高水位留 30% 余量 |
| 进 `vApplicationMallocFailedHook` | 4 KB 堆装不下新增任务/定时器 | 提高 `configTOTAL_HEAP_SIZE`，或复用对象 |
| 串口输出现半行 | 抢占打断 `printf` | 用互斥量包住整行 |
| 中断里调用 `xQueueSend` 卡死 | 非 `FromISR` 版本会试图阻塞 | 统一用 `...FromISR` 并处理 `xHigherPriorityTaskWoken` |
| 中断优先级配成 0–4 后调用 API 触发断言 | 高于 `configMAX_SYSCALL_INTERRUPT_PRIORITY` 的中断不受临界区屏蔽 | 能调用 FreeRTOS API 的中断优先级数值必须 ≥ 5 |

## 故障排查方法

1. **先看堆**：`heap_free` 每次是否递减？递减则有泄漏（通常是把任务栈当作可复用的）。
2. **再看栈**：`stack_hwm` 是否逐次变小？变小说明还没有覆盖到最深的调用路径，继续观察或加大栈。
3. **看顺序**：`[consume]` 的 tick 是否紧跟生产时刻？若总是慢一个 tick，说明唤醒后没有抢占，检查优先级是否写反。
4. **看比例**：`consume` 条数与 monitor 采样次数的比值应恒为 5:1。偏离说明某个周期的执行时间已经接近周期本身。
5. **看断言**：`configASSERT` 触发时会打印 `文件:行号`。`port.c` 里的断言几乎总指向优先级配置或异常路由问题。
6. **仿真里不要看绝对时间**：用 `run.sh exp10` 得到的 tick 比值（约 2.9–2.93）解释一切"变快"的现象。
7. **真机排查**：GDB 连 ST-Link，在 `vApplicationStackOverflowHook` 与 `prvTaskExitError` 下断点，看 `pxCurrentTCB` 指向哪个任务。

## 练习题

基础题：

1. 把 `SAMPLE_PERIOD_MS` 改成 50，重算"1000 tick 内应有多少条 `consume`"，跑 QEMU 验证。若不等，先算清执行时间占比再判断是不是 bug。
2. 把 `configTICK_RATE_HZ` 改成 100，说明 `pdMS_TO_TICKS(200)` 变成多少、串口输出频率如何变化。

改错题：

3. 下面两行二选一，说明哪一行会在真机上随机 HardFault，为什么：
   ```c
   void PendSV_Handler(void) { xPortPendSVHandler(); }
   /* 或 */
   __asm("b xPortPendSVHandler");
   ```
4. 某同学把任务栈写成 `xTaskCreate(task, "t", 256 * 4, NULL, 1, NULL)`（以为 256 是字节），
   分析 RAM 占用会变成多少、在哪一步失败。

综合题：

5. 加一个 `[res]` 采样之外的**计数器任务**（优先级 0），统计空闲任务的运行时间（提示：`vApplicationIdleHook`
   或 `configGENERATE_RUN_TIME_STATS`），据此估算 CPU 占用率。说明为什么不能直接读 `uxTaskGetSystemState`。
6. 把 `xQueueSend` 的等待时间从 50 ms 改成 0，并把消费任务临时挂起（`vTaskSuspend`），
   验证 `dropped` 增长路径。记录触发条件，并解释为什么这条路径在 QEMU 里同样可信。

## 验收标准

- [ ] 能画出一个 tick 内发生的事：SysTick 中断 → 节拍递增 → 唤醒 producer → 挂起 PendSV → 退出中断时切换。
- [ ] 能解释 SVC / PendSV / SysTick 三个异常的分工，以及为什么 PendSV 必须最低优先级。
- [ ] 能说明"向量表名字不一致"的两种解法及各自的代价。
- [ ] 能用 `heap_free` 与 `stack_hwm` 两个指标回答"RAM 够不够"。
- [ ] 能解释生产间隔 202 而不是 200 的两个来源。
- [ ] 能在 QEMU 中复现本实验并得到 5:1 的采样比例与恒定 `heap_free`。
- [ ] 能列出至少三条"本实验在仿真中不可验证"的结论。

## 延伸阅读

- FreeRTOS 官方：《Mastering the FreeRTOS Real Time Kernel》第 1–2 章（任务管理）、第 4 章（队列）。
- ARM：《Cortex-M3 Devices Generic User Guide》——异常进入/返回、`EXC_RETURN` 编码、`PSP`/`MSP` 切换。
- 内核源码（已入库，可对照本实验读）：`../F103_common/vendor/freertos/Source/portable/GCC/ARM_CM3/port.c`
  的 `xPortPendSVHandler`、`vPortSVCHandler`、`xPortStartScheduler`。
- 项目内相关复盘：`../../tests/qemu-stm32/RESULT.md`（仿真能力边界与时基失真）。

## 下一步

1. **任务间通信扩展**：用二值信号量做中断→任务通知（接到 USART1 的 RXNE 中断，替换轮询/HAL 回调）。
2. **软件定时器**：`configUSE_TIMERS = 1` + 定时器服务任务，注意它会额外吃堆（本实验只剩 496 B）。
3. **优先级反转实验**：构造三任务 + 互斥量的经典场景，用 tick 序列证明优先级继承生效。
4. **真机验证**：拿到板子后重做本节"实测记录"表格，把 QEMU 数据与真实 200 ms 周期对照，补齐"未验证"项。

## 已知限制

- **只在 QEMU 中运行过**。真板（STM32F103C8T6）的绝对周期、tick 精度、栈用量均未实测。
- QEMU 用 F100RB 的 128 KB Flash / 8 KB RAM 编址，**78.13% 的 RAM 占用率不代表 C8T6 的实际情况**；按 C8T6 链接脚本构建为 20.26% / 31.25%，但两者都只是链接器数字，未上板。
- `dropped` 路径与队列溢出：本实验从未触发（消费能力远大于生产）。未验证。
- 优先级继承：`printf` 竞争从未发生（consume 与 monitor 不重叠持锁到达），因此优先级继承只停留在配置层面，**未实测**。
- 三个钩子函数（断言 / 堆失败 / 栈溢出）均未触发过，只有代码路径，没有运行时证据。
- 未使用 `configUSE_TRACE_FACILITY` / `vTaskList`，任务状态表无法导出。
- `heap_4` 的碎片行为未做长期观察（仅 16 秒固件时间，17 次采样）。
- QEMU 的时基是 24 MHz 对固件 8 MHz，倍率约 2.93，**任何绝对毫秒结论都不得来自仿真**。
