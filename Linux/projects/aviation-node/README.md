# aviation-node：航空主题地面仿真节点（可仿真部分）

> 结论先行：QEMU 里可以验证的是一条**纯软件数据链**——PC 注入帧 → 中断收包 →
> 帧校验（SOF/LEN/CRC）→ 样本有效性（范围/变化率/数据源）→ 模式状态机 →
> 降级/恢复 → 停喂门控与统计。时钟、串口、中断、调度都来自真实模型，所以这条链
> 上每一环都有可复现的数字。
>
> **不能**验证的是真实硬件量：CAN 总线、ADC 采样、示波器/逻辑分析仪时序、IWDG
> 复位与复位原因。这些在 QEMU 中未建模，见 §9「硬件依赖项（待真板）」，本文不把
> 它们写成"已实现"。
>
> 本项目是**教学级地面仿真**，不对应任何真实机型；不使用任何工作项目的代码、数据、
> 参数或接口。

配套文档：

- [`docs/protocol.md`](docs/protocol.md)：帧格式、CRC 参数、故障位与模式迁移、需求追踪矩阵。
- [`docs/test-report.md`](docs/test-report.md)：**八类**异常（错帧/越界/突变/静默/丢帧/只发心跳/只发数据/任务阻塞）的注入用例、预期、实测证据与机器判定，另含下行通道与时间戳年龄下溢两条记录。
- [`docs/task-timing.md`](docs/task-timing.md)：任务时序图、优先级取舍、有证据/无证据的时序结论。
- [`docs/architecture.md`](docs/architecture.md)：分层视图、模块依赖、运行时交互、资源预算与变更影响矩阵。
- [`docs/resource-review.md`](docs/resource-review.md)：RAM 三个口径（链接器尾部 / FreeRTOS 堆 / MSP 主栈）的实测分解，双通道冗余的成本估算与 8 KB 目标下的可行性结论。

## 1. 项目定位与边界

对应 `Linux/CLAUDE.md` §12「P1：启动航空主题项目的可仿真部分（路线图第 7–8 周）」。
四项内容与本仓库的落地位置一一对应：

| P1 要求 | 落地位置 | 证据强度 |
| --- | --- | --- |
| 传感器模拟输入（PC 经 UART 注入） | `tests/qemu-stm32/aviation/inject.py` + `src/main.c` 的 RXNE ISR | **实测**（`logs/aviation_node.log`） |
| 数据有效性判断与模式状态机 | `src/av_frame.c`、`src/av_sensor.c`、`src/av_mode.c` | **实测** |
| 周期任务 + 健康监控 + 降级路径 | `src/main.c`（rx/ctrl/health 三任务）、`src/av_health.c` | **实测** |
| 硬件依赖项单列并标注待真板 | 本文件 §9 | **未验证**（无硬件） |

明确不做的事：真实飞控律、真实传感器标定、CAN 驱动、持久化故障记录（Flash/备份寄存器）。
后者要真板，见 §9。

## 2. 环境与前置条件

| 项 | 值 |
| --- | --- |
| 目标 | STM32F103（Cortex-M3）；QEMU `-M stm32vldiscovery`（STM32F100RB，8 KB SRAM） |
| 内核 | FreeRTOS-Kernel V11.3.0（`experiments/F103_common/vendor/freertos` 裁剪子集） |
| 编译器 | `arm-none-eabi-gcc` 14.2 rel1，`-mcpu=cortex-m3 -mthumb -mfloat-abi=soft`，零警告 |
| 时基 | SysTick 1000 Hz（固件毫秒） |
| 构建 | CMake + Ninja，`-DF103_QEMU=ON` 使用 8 KB 链接脚本 |
| 注入器 | Python 3（只用标准库，不依赖 pyserial） |
| 前置 | `F103_11`（RXNE → 队列）、`F103_14`（心跳与"与"语义停喂） |

**时间口径**：本项目所有 `t=`、`ms` 都是**固件毫秒**。QEMU 以 24 MHz 驱动 SYSCLK，
固件按 8 MHz 计算，实测时基偏差 2.90 倍（`tests/qemu-stm32/RESULT.md` §3.3）。
时间类结论只能看比例，不能读成实时性指标。

## 3. 系统结构

```text
PC (inject.py, SENSOR 50 Hz + STATUS 10 Hz)
      │  AA 55 TYPE SEQ LEN PAYLOAD CRC16
      ▼
USART1 RXNE ISR ──xQueueSendFromISR──> rx 任务（优先级 3，224 字）
                                          │ 帧层：SOF/LEN/CRC
                                          ├─ TYPE 0x02 STATUS ──> av_health_ground_ok()
                                          │                       （只刷链路层，不刷数据层）
                                          │ 样本层：范围/变化率/数据源
                                          ├──> 有效样本 ──> s_sensor.last_good
                                          │                  + good_streak
                                          └──> 故障标记 ──> av_health 标志位图
                                                                │
        ctrl 任务（优先级 4，20 ms）<───────────────────────────┤（纯函数读取）
          ├─ 模式状态机 INIT/NORMAL/DEGRADED/SAFE（只看 MODE_MASK 子集）
          └─ 控制输出：限幅 → 速率限幅 → s_cmd
                                                                │
        health 任务（优先级 2，100 ms）<────────────────────────┤
          ├─ 喂狗门控（心跳 + 无 TIMEOUT/STALE，"与"语义）
          ├─ 每 100 ms 发下行 STATUS 帧（USART2 TX，25 B）
          └─ 每 5000 ms 打印 [stat]

USART2 TX ──> PC（link.py：解帧 + 与 [stat] 同时刻交叉核对）
```

| 任务 | 优先级 | 栈（字） | 周期 | 时限来源 |
| --- | --- | --- | --- | --- |
| `rx` | 3 | 224 | 事件驱动（队列 200 ms 超时打心跳） | 字节到达速率 |
| `ctrl` | 4 | 224 | 20 ms（50 Hz） | 被控对象时间常数 |
| `health` | 2 | 224 | 100 ms | 故障需要多快报出来 |

优先级取 `ctrl > rx > health`：控制律的周期性是硬约束，解析和上报都不能挤掉它。
这三个周期是**设计选择**，抖动/最坏执行时间要真板 + 示波器才能给，见 §9。

两条串口各管一件事：USART1 是"人读的 ASCII 日志 + 注入输入"，USART2 是"机器读的二进制
状态帧"。混在一条线上的后果不是"不好看"，而是日志无法按行解析、判定器被任意字节打断。

上面是数据链路速览；**分层划分、模块依赖方向、共享数据的保护方式、资源预算与变更影响矩阵**
见 [`docs/architecture.md`](docs/architecture.md)。

## 4. 核心概念

### 4.1 帧协议（详见 [`docs/protocol.md`](docs/protocol.md)）

```text
AA 55 | TYPE | SEQ | LEN | PAYLOAD(LEN) | CRC16-LE
CRC16-CCITT-FALSE（0x1021，初值 0xFFFF，不反射），覆盖 TYPE..PAYLOAD
SENSOR 载荷 8 B（上行 0x01）：airspeed(i16,0.1 m/s) pitch(i16,0.01°) roll(i16,0.01°) flags(u16)
STATUS 载荷 4 B（上行 0x02，地面站心跳）：ground_uptime_ms(u32)
STATUS 载荷 18 B（下行 0x81，USART2）：uptime(u32) mode(u8) flags(u8) cmd(i16)
                                 airspeed(i16) pitch(i16) frames(u16) crc(u16) gap(u16)
```

上行 `SENSOR`/`STATUS` 与下行 `STATUS` 用同一套头与 CRC 规则，负载布局见
[`docs/protocol.md`](docs/protocol.md) §2.1 / §2.2。**上行 `0x02` 与下行 `0x81` 同名不同向**：
两条独立链路，各自维护序号与超时，不互相刷新对方的活性。

CRC 不含两个 SOF 字节：SOF 只负责定位帧边界，CRC 只负责内容完整性。绑在一起会让
"同步错了"和"数据错了"在日志里长得一样。

### 4.2 有效性判定分三层，失败含义不同

| 层 | 判据 | 置位标志 | 处置 |
| --- | --- | --- | --- |
| 帧层 | SOF / LEN / CRC | `CRC`（连续 3 帧） | 丢帧，不参与后续判定 |
| 样本层 | 范围 / 变化率 / 数据源标志 | `RNG` / `RATE` / `SRC` | 丢样本，不刷新 `last_good` |
| 时间层（数据） | 无有效样本 300 ms | `STALE` | 降级 |
| 时间层（链路） | 无 CRC 正确帧 300 ms | `T` | 降级 |
| 地面站层 | 无 `STATUS` 心跳 500 ms | `GND` | **只记录，不降级** |

`T` 与 `STALE` 必须分开：帧一直来但全部越界，与一个字节都没来，在日志里是两种完全
不同的故障（前者切备份传感器，后者查链路）。实测中"CRC 错帧阶段"两者同时置位是
正确行为——持续错帧意味着链路对应用层而言已经断了。

`GND` 单独成层：地面站失联与"数据能不能用"无关，它不进 `AV_F_MODE_MASK`，因此**不
改变飞行模式**；但地面站心跳会刷新链路层的 `T` 计时（心跳帧证明线是通的），所以它
又与 `T` 共享一半语义——两者拆开而不是合并，见 §10.8。

### 4.3 恢复必须比降级更难

```text
INIT      --3 个连续有效帧-->            NORMAL
NORMAL    --任一故障-->                  DEGRADED
DEGRADED  --连续 3 个有效帧且无故障-->    NORMAL
DEGRADED  --故障持续 3000 ms-->          SAFE
SAFE      --连续 5 个有效帧且无故障-->    DEGRADED（不直接回 NORMAL）
INIT      --3000 ms 内始终无有效帧-->     SAFE
```

两级恢复（SAFE→DEGRADED→NORMAL）是刻意设计：跳过中间态等于"一次跳回正常"，
而跳过的那些判据正是用来确认数据已经稳定的。

上表的"故障"一律指 `flags & AV_F_MODE_MASK`，不是"`flags` 非 0"。掩码是白名单：
新增故障位默认不参与模式判定，要参与必须显式加进去。地面站心跳（`AV_F_GROUND`）就是
不参与的例子——若用黑名单，一个与飞行无关的失联会悄悄改变飞行模式。

### 4.4 故障标志必须"保持"，且保持窗口决定停留门限的下界

`RNG`/`RATE`/`SRC` 由单帧事件产生。若下一帧正常就立即清除，这种 50 Hz 的抖动会让
模式在边界值上来回跳。所以标志在最后一次出现后保持 `AV_FAULT_HOLD_MS = 1000 ms`。

**首轮实测暴露的耦合**：`AV_DEGRADED_TIMEOUT_MS` 原为 2000 ms，只比保持窗口大一倍，
结果一次约 400 ms 的越界事件就占满了停留计时，直接推进到 `SAFE`（日志
`t=11176 DEGRADED -> SAFE flags=0x08`，`SAFE` 由越界触发，明显不合理）。
修正为 3000 ms 后，"在 DEGRADED 停留 2000 ms 以上"才真正等价于"故障持续 2000 ms 以上"。
设计约束：`DEGRADED 停留门限 > 故障保持窗口 + 一个采样周期`。

### 4.5 喂狗条件是"与"，且只由 health 任务调用

```c
feed_ok = rx 心跳未超时 && !(TIMEOUT | STALE)
```

若各任务各喂各的，卡死的任务不喂，别的任务照样把狗喂饱——看门狗对卡死完全失效
（`F103_14`）。本项目里 `watchdog_feed_if_enabled()` 是唯一调用点，且 `IWDG` 未启用
（QEMU 未建模），所以**这里只验证了"什么时候该停喂"**，不是看门狗验证。

停喂原因不是布尔量，而是一对独立条件的组合，`[gate]` 行按变化打印：

| `reason` | 含义 | 排查方向 |
| --- | --- | --- |
| `OK` | 两个条件都健康，`feed=ON` | — |
| `FLAGS` | 心跳正常，但 `T`/`STALE` 置位 | 查数据链路与传感器 |
| `HB` | 心跳超时，`T`/`STALE` 都未置位 | 查 rx 任务是否在推进 |
| `HB+FLAGS` | 两者同时不健康 | 通常出现在长时间中断之后 |

`HB` 与 `FLAGS` 必须分开：任务卡住和数据坏掉在 `[stat]` 里都只是 `feed=OFF`，
只有原因位才能区分"该重启"和"该换传感器"。

## 5. 接口卡片

### `av_parser_push( av_parser_t *p, uint8_t byte, av_frame_t *out )`

作用：按字节推进的帧状态机，返回 `AV_FRAME_NONE / OK / BAD_CRC / BAD_LEN`。

常见误用：把"凑够 15 字节"当成一帧——一旦失步（少一个字节）就永久错位；本实现逐字节
找 SOF，出错后自动回到找 SOF0，并且处理了 `AA AA 55` 这种重复前缀。

### `av_crc16_ccitt_false( const uint8_t *d, uint32_t n )`

作用：标准 CRC16-CCITT-FALSE。校验值 `crc("123456789") == 0x29B1` 在启动时打印，
不一致则拒绝启动调度器。

为什么要在启动时自检：协议算错时 PC 与 MCU 会全帧互斥，现象是"板子安安静静"，
这是最难查的一类故障。用一个已知答案把它钉在启动日志里。

### `av_sensor_accept( av_sensor_t *s, const av_sensor_sample_t *raw, TickType_t now )`

作用：范围 + 变化率 + 数据源判定，返回 `OK / RANGE / RATE / SOURCE`。
只有 `OK` 才刷新 `last_good`。

关键点：变化率的参考时刻用 `last_seen_tick`（上一**收到**的样本），不是
`last_good_tick`（上一**有效**样本）。理由见 §6 的第二个实测缺陷。

### `av_mode_step( av_mode_ctx_t *ctx, uint32_t flags, TickType_t now )`

作用：推进模式状态机。只吃 `flags & AV_F_MODE_MASK` 是否为 0，不关心是哪一位——
新增一种**数据类**故障只需在健康层加一位并把它加进掩码，状态机逻辑不动。

### `av_health_ground_ok( void )` / `av_health_ground_age( TickType_t now )`

作用：前者在收到一帧 CRC 正确的 `STATUS` 时调用，刷新链路层时间戳、记录心跳时间戳、
`ground_frames++`；后者返回距最后一帧心跳的毫秒数。

关键点：**只刷新链路层，不刷新数据层**。心跳到达证明这条线是通的（所以 `T` 不该置位），
但心跳不携带样本（所以 `STALE` 该按样本年龄独立判定）。它也不参与 `last_seq`/`seq_gaps`
——那是 `SENSOR` 的序号空间，混用会让缺口统计失真。

`ground_age` 不返回哨兵值：从未收到心跳时读数是"上电以来的时长"，与 `rx_age` 在首次
kick 之前的口径一致；返回哨兵（如 `0xFFFFFFFF`）会被下游读成时间戳下溢。

### `av_downlink_status( now, mode, flags, cmd, sensor, stats )`

作用：把当前状态打成 25 字节 `STATUS` 帧从 USART2 发出，返回 1 = 整帧写完、0 = `TXE`
等待超时（帧被截断，宿主按 CRC 丢弃）。

关键点：**不重试**。等待上限是 `AV_DL_TXE_GUARD` 次轮询，因为调用者是 health——唯一
喂狗的任务。为了一帧状态把监督任务挂在串口上，是拿"整机不复位"换"日志好看"。

### `taskENTER_CRITICAL() / taskEXIT_CRITICAL()`

`good_streak` 由 rx 递增、由 ctrl 在状态切换时清零，是跨任务的读-改-写。
单核**不能**假设 `uint8_t` 自增是原子的：编译器可能生成 load-modify-store，任务
可能在中间被抢占。两处都放在临界区里。

### `xQueueSendFromISR( s_rx_queue, &ch, &woken )`

中断里只做三件事：清硬件标志、把 1 字节搬进队列、决定是否立刻切换。队列满时直接
丢弃并计数（`s_rx_drops`），绝不阻塞——中断上下文没有任务可以挂起。

## 6. 关键实现点

### 6.1 逐字节解析，而不是"攒够一帧再解析"

串口是字节流，没有帧边界。状态机版本不需要额外的行缓冲（39 字节栈上就够），
失步后自动重同步，代价只是最坏多丢一帧。

### 6.2 三个由实测发现的缺陷（都不是编译期能发现的）

| 缺陷 | 症状 | 根因 | 修法 |
| --- | --- | --- | --- |
| `SAFE` 被短故障触发 | 一次约 400 ms 的越界，日志出现 `DEGRADED -> SAFE flags=0x08` | `DEGRADED` 停留门限（2000 ms）只比故障保持窗口（1000 ms）大一倍，保持窗口自己就占满了计时 | 门限提到 3000 ms；把"门限 > 保持窗口"写成头文件里的设计约束 |
| `RATE` 阶段没有 `STALE` | 突变期间应无有效样本，却始终未出现 `STALE` | 变化率参考时刻用了 `last_good_tick`，被拒帧不刷新它；连续拒绝 200 ms 后窗口判定失效，突变值反而被**接受** | 改用 `last_seen_tick`（上一收到的样本）；只有真正的链路空档才跳过变化率判定 |
| `[stat] age=4294967292` | 某条 stat 的年龄字段是 ~4.29e9 | 时间戳由 rx（优先级 3）写入，可能比 health 的快照晚 1～2 tick，无符号减法下溢 | 抽出 `av_tick.h::av_tick_elapsed`，负差值压 0；`expect.py` 加 C15 判据 |

第三个是加入下行通道后跑交叉核对时才被注意到的（同一次运行里既有正常 `age=38`，也出现过
一次下溢值）。它不只是"日志难看"：`hb` 走的是同一条减法路径，下溢会被门控读成"心跳早断了"
并**误停一次喂狗**。修完之后 C15 在全部 QEMU 运行里都通过。

教训：**保护逻辑自己也会失效**。第二个缺陷的表现是"保护看起来在工作（RATE 置位了），
但被保护的条件已经不成立"——只看标志位是否置位，不会发现它。

### 6.3 序号缺口只能证明"少收到"，不能证明"丢了哪一帧"

终态 `gap=313 = 30（CRC 错帧）+ 280（静默时隙：`silent` 100 + `hb_only` 180）+ 3（`drop` 段）`，
与 `crc=30` 对照才能区分"错帧被丢弃"和"根本没发"。两者分开统计是刻意设计：真实链路上"错帧"要查物理层，
"丢帧"要查缓冲与调度。注意 `hb_only` 段**地面站心跳照发、`SENSOR` 序号照常推进**，缺口必须计入——
漏掉它 `gap` 会稳定少 180，而现象看起来与心跳无关（见 `tests/qemu-stm32/README.md`「剧本相关的常量」）。

### 6.4 心跳点的位置决定它能不能发现卡死

rx 的心跳打在**每轮主循环**，不是"每收到一个字节"。若打在收字节之后，任务卡在队列上
时就永远不心跳——而这正是要被发现的形态（`F103_14` 的注入实验证明了这一点）。

### 6.5 静态计数的读取面

`av_health_stats()` 返回指向模块静态结构的指针，rl 任务写、health 任务读。字段都是
32 位对齐，单字读取在 M3 上不会撕裂；这不是通用做法，只是本项目的**单写单读**约定。
若将来增加写者，必须改成"写者填快照 + 临界区拷贝"。

### 6.6 监督者的优先级，决定了它能不能"看见"忙等

任务阻塞注入的第一版写成忙等（`while (...) { taskYIELD(); }`），实测得到一个反例：
800 ms 阻塞期间 health **一次都没运行**，直到阻塞结束后的第一拍才读到 `hb=804`。
原因是优先级 `rx 3 > health 2`——health 的延时到期后虽然就绪，仍抢不到 CPU；
`taskYIELD()` 让给的是"最高优先级的就绪任务"，也就是 rx 自己。

这不是实现瑕疵，而是看门狗设计的硬约束：

- **软件可见性**：监督任务必须不低于被监督任务的优先级，否则对方的忙等会把监督者
  一起饿死，日志里只会留下"阻塞结束后心跳突然很老"的痕迹，没有"正在进行"的证据。
- **硬件兜底**：本项目里喂狗只发生在 health 里。health 被饿死 ⇒ 没人喂狗 ⇒ `IWDG`
  复位。所以 CPU 忙等这类故障**仍会被硬件抓住**，只是丢了归因（不知道该怪谁）。

因此注入改为每 tick `vTaskDelay(1)` 让出一次：监督链变为可观测，代价是注入模型是
"任务不推进"而非"CPU 忙等"。两种形态的差别就是本节这段文字——**没有哪一种更"真"，
只有看得见和看不见**。

### 6.7 调试注入只在 QEMU 构建里编译

`AV_DEBUG_BLOCK_RX_MS`（默认：`F103_QEMU` 下 800，否则 0）与 `AV_DEBUG_BLOCK_AT_MS`
（默认 16500）由 `CMakeLists.txt` 以 `-D` 传入。为 0 时整段代码在 `#if` 之后不参与编译，
真机固件里不存在这条路径——注入**没有运行时开关**，编进去就改变行为，所以不能靠
"记得别开"来保证生产构建干净。

时刻取 16500（固件 ms）落在剧本静默段（约 14.7–17.7 s）内：队列为空，阻塞既不丢字节，
也不改变 `crc`/`gap` 的计数自洽性（实测阻塞前后 `gap` 仍为 `30 + 100 + 3`）。

## 7. 注入剧本

`tests/qemu-stm32/aviation/inject.py`，SENSOR 50 Hz + STATUS（地面站心跳）10 Hz，
每段只改一个变量：

| 真实秒 | 阶段 | 内容 | 预期 |
| --- | --- | --- | --- |
| 0.50–2.00 | `valid` | 正常帧（15.0 m/s，+1.00°） | INIT → NORMAL |
| 2.00–2.60 | `badcrc` | CRC 字段取反 | CRC + T + STALE → DEGRADED |
| 2.60–3.20 | `valid` | 恢复 | 连续 3 帧后 NORMAL |
| 3.20–3.60 | `range` | pitch = 90.00°（> 60°） | RNG → DEGRADED，随后 STALE |
| 3.60–4.30 | `valid` | 恢复 | 保持窗口到期后 NORMAL（窗口宽度必须 ≥ 保持期，见下） |
| 4.30–4.55 | `rate` | pitch 1.00° → 45.00°（> 20°） | RATE → DEGRADED，随后 STALE |
| 4.55–5.00 | `valid` | 回原值 | 恢复 |
| 5.00–7.00 | `silent` | 不发任何字节（含心跳） | T + STALE + GND → DEGRADED → SAFE |
| 7.00–7.60 | `valid` | 恢复 | SAFE → DEGRADED → NORMAL |
| 7.60–7.66 | `drop` | 连丢 3 帧（60 ms，约 174 固件 ms < 300 ms 门限） | 不降级，只增 `gap` |
| 7.66–8.20 | `valid` | 恢复稳态 | 确认丢帧没有留下标志 |
| 8.20–11.80 | `hb_only` | **只发心跳**，停发 SENSOR | STALE 单独置位、T 不置位；数据层真的断了，仍会正常降级到 SAFE |
| 11.80–14.30 | `hb_lost` | **只发 SENSOR**，停发心跳 | GND 置位，**模式不变**（数据恢复了就回 NORMAL） |

序号按 50 Hz 网格递增，**静默期也同样 +1**，这样恢复后接收侧能看到 seq 跳变，
"丢帧统计"才有可核对的对象。`SENSOR` 与 `STATUS` 的序号各自独立计数。

`silent` 段必须同时停发心跳：否则链路层（`T`）的计时一直被心跳刷新，剧本就测不到它。
`hb_only` 段的长度（3.6 s 真实 ≈ 10.6 s 固件）按"段内至少有 2 条 `[stat]`"定——
C17 要比较相邻两条 `[stat]` 之间 `frames` 是否不变，只有 1 条时无从判定。

`valid` 窗口的**宽度**同样是设计约束，不是随手填的：故障标志保持 1000 ms 固件时间
（≈345 ms 真实时间），从最后一个故障帧起算。原先 `range` 之后的窗口只到 4.00 s，扣掉
保持期后真正能用来恢复的余量只剩几十毫秒，是否在 `rate` 段之前回到 NORMAL 全看抖动——
实测加一条下行串口就把结果翻过去了（`NORMAL→DEGRADED` 从 5 次变 4 次，C13 失败）。
窗口拉到 4.30 s 后，"越界 → 恢复 → 突变再次降级"这条链才是确定性的。
`expect.py` 的 `--expected-down 6` 就是按这条链定出来的，六次分别是：`badcrc`、`range`、
`rate`、`silent`、`hb_only`，以及**剧本放完后**的收尾静默（注入器 14.3 s 退出、QEMU 继续
跑到超时，链路无字节 → T/STALE → 降级）。最后一次不是剧本里的故障段；把预期写成 5
会让"丢帧多降一次"与"少一次收尾降级"互相抵消，判据反而失去意义。

另有一条**不在 `inject.py` 里**的故障注入：任务阻塞。PC 侧无法让 MCU 的任务卡住，
所以它由固件侧按编译期常量触发（`AV_DEBUG_BLOCK_RX_MS` / `AV_DEBUG_BLOCK_AT_MS`，
见 §6.6、§6.7 与 §8）。注入点刻意选在静默段内，因此它与剧本正交：不改变任何一帧的
收发，只改变 rx 任务是否在推进。

## 8. 编译与运行

```bash
# 构建（Windows 侧，仓库根为 Linux/）
cd projects/aviation-node
cmake -S . -B build-qemu -G Ninja -DF103_QEMU=ON \
  -DCMAKE_TOOLCHAIN_FILE=../../experiments/F103_common/cmake/arm-none-eabi.cmake
cmake --build build-qemu

# 运行（WSL / Linux 侧，QEMU 装在这里）
cd tests/qemu-stm32
bash run.sh aviation        # 原始输出落到 logs/aviation_node.log，跑完自动判定
bash run.sh aviation-link   # 同一条剧本 + 下行 USART2 落 logs/aviation_downlink.bin，双通道判定
python3 aviation/inject.py --list   # 只看剧本，不发数据

# 只复跑判定（不重跑 QEMU）：24 项不变量，退出码 0/1/2
python3 aviation/expect.py logs/aviation_node.log
python3 aviation/expect.py --silent-slots 280 --dropped-frames 3 --expected-down 6 logs/aviation_node.log

# 只复核下行（L1–L7，其中 L7 与上行 [stat] 交叉核对）
python3 aviation/link.py logs/aviation_downlink.bin --ascii logs/aviation_node.log
```

`-DF103_QEMU=ON` 的构建默认带任务阻塞注入（`AV_DEBUG_BLOCK_RX_MS=800`）。要复现
"没有注入"的镜像，显式传 `-DAV_DEBUG_BLOCK_RX_MS=0`；此时日志里没有 `[debug]` 行，
`expect.py` 的 C14 会转为"未启用（本条不适用）"，其余 23 项照常判定。真板构建
（不带 `F103_QEMU`）默认 0，注入代码不参与编译。

**两条通道的产物必须来自同一次运行**：`run.sh aviation` 只重写 `.log`，不重写 `.bin`。
只跑它之后再跑 `link.py --ascii`，L7 会拿旧 `.bin` 的帧去新 `.log` 里找同 `t` 的
`[stat]`，匹配不到就判"样本不足"并失败（不是判定器的缺陷，是产物不同源）。要复核下行
就重跑 `aviation-link`。另外 `link.py` 单独跑时 L1–L6 仍基于 `.bin` 自身，只有 L7 需要
`.log`；因此 `aviation-link` 把三者绑在同一个用例里。

`run.sh` 用管道把注入器的 stdout 接进 QEMU 的 `-serial stdio`，不依赖 pyserial，
也不需要虚拟串口设备。`aviation-link` 在此基础上追加一个 `-serial file:` 作为 serial1
（USART2），二进制不落终端，避免污染日志流。该机型把 USART2 接到第二个后端这一点
是**先实测再设计**的：探针 [13] 写出 8 字节特征串并在宿主文件里读回。`expect.py` 与 `link.py`
接在同一用例末尾，两条通道的退出码相"与"：任一失败时 `run.sh aviation-link` 以非零码退出，
`run.sh all` 也会在此中止；通过时 `expect.py` 另外打印六类延迟汇总（见
`tests/qemu-stm32/README.md` 的「延迟汇总」）。

## 9. 硬件依赖项（待真板）

以下项**没有**在 QEMU 中验证，也没有真板证据。它们不阻塞本项目的软件链路，但
简历/面试里若要声称"做过 CAN/ADC/看门狗"，必须先补这一节。

| 项 | 想验证什么 | 现在用什么替代 | 真板上要补的证据 |
| --- | --- | --- | --- |
| **CAN** | 真实总线帧、验收滤波、错误帧与总线关闭 | UART 单链路 | CAN2.0B 发送/接收计数、`ESR` 错误码、终端电阻与波形 |
| **ADC** | 真实模拟量采样、量纲与参考电压误差 | PC 注入整数样本 | ADC1 规则组 + DMA，与万用表读数比对 |
| **示波器** | 任务周期抖动、中断响应、GPIO 打点 | 只有固件 tick 差值（且被 2.90 倍时基污染） | GPIO 翻转 + 实测周期与最坏执行时间 |
| **逻辑分析仪** | UART/CAN 时序、波特率误差、帧间隔 | QEMU 无比特级时序 | 波形与解码结果 |
| **IWDG** | 复位路径与复位原因 | `F103_14` 的软件监督（停喂判定） | `RCC->CSR` 的 `IWDGRSTF`、LSI 30–60 kHz 下的实际超时窗口 |
| **持久化故障记录** | 复位后仍能读到上一次故障 | 无（只用 RAM 计数） | Flash 页写或备份寄存器 |

对应的工程动作：真板到手后，`F103_WATCHDOG_ENABLE` 才能从 0 改为 1；在此之前
喂狗只在软件侧记账，`[stat]` 里的 `feed=ON/OFF` 不代表硬件行为。

## 10. 实测记录

日志：`tests/qemu-stm32/logs/aviation_node.log`（剧本 14.3 s 真实时间；`run.sh` 给 20 s 超时窗口，
固件时间跑到约 55 s，`frames` 停在 377——690 个时隙减去 280 静默时隙、3 丢帧、30 错帧）。

**读数口径**：本节所有 `t=`、`[timing]`、栈水位都取自当前 `logs/aviation_node.log`（2026-10-27 最后一次
`bash run.sh aviation-link` 写入，与 `logs/aviation_downlink.bin` 是**同一次运行**），与仓库里这份日志
逐行一致；`run.sh` 再跑一次就会覆盖它，本节数字随之失效。
§10.3 另给多次运行的区间。同一事件跨运行会漂移且越靠后越大
（逐事件对照：`t≈1100` 差 41、`t≈13000` 差 75、`t≈27000` 差 154），
所以不要拿本文的 `t=` 逐字比对旧日志。要机器判定用 `aviation/expect.py`（判不变量），
要最新延迟读数看它的 `[timing]` 段。

### 10.1 构建体积与堆

| 项 | 值 |
| --- | --- |
| FLASH | 18256 B（128 KB 的 13.93%） |
| RAM | 6552 B（8 KB 的 79.98%），含 `configTOTAL_HEAP_SIZE = 4096`；链接器尾部余 1640 B **是 MSP 主栈缓冲，不是可分配内存**，三个口径的分解见 `docs/resource-review.md` |
| `text` / `data` / `bss` | 18156 / 100 / 6452 B |
| 编译警告 | 0（`--clean-first` 全量重建，`grep -c` 统计为 0） |
| 启动时空闲堆 | 736 B（三个任务 + 队列 + 互斥量之后，idle 尚未创建） |
| 稳态空闲堆 | 120 B（全部 `[stat]` 行恒定） |

调试注入的代价：同一份源码 `-DAV_DEBUG_BLOCK_RX_MS=0`（`build-noinj`）时 FLASH 为 17968 B、
RAM 为 6544 B，即注入段占 **288 B FLASH / 8 B RAM**（`[debug]` 两条 `printf` 加循环；
RAM 的 8 B 是注入用的函数内 `static` 跨过了对齐边界）。

加入上行 `STATUS` 的代价：与加入之前的构建（18028 B / 6536 B）相比 **+224 B FLASH /
+16 B RAM**（`rx` 栈从 176 提到 224 另有 +4 B FLASH，不占 RAM）。这些都是两次构建 `size`
的差，**未**做函数级拆分——它同时包含 `STATUS` 分支、`av_health_ground_ok`、以及
`[stat]`/`[fault]` 新增字段的格式串，不能读成"心跳功能占 224 B"。RAM 的 16 B 是
`s_last_ground_tick`(4) + `s_ground_seen`(4) + `stats.ground_frames`(4) 加对齐。

**任务栈不占 `text`/`bss` 之外的 RAM 统计口径**：`rx` 176→224 与 `health` 224→256 都不改变
RAM 总量——任务栈来自 `ucHeap`，而 `ucHeap` 是 `.bss` 里固定大小的一块，多分配的栈只减少
堆余量（440 → 248 → 120 B）。所以"栈加够了"和"RAM 还够"是两件互相挤压的事。

120 B 余量已不足以再建一个 128 字的任务（栈 512 B + TCB），"再加一个任务"必须先提高
`configTOTAL_HEAP_SIZE`，这属于共享配置，会影响其他实验的实测体积。
双通道冗余的 RAM 成本与"8 KB 下能否链接"的结论见 `docs/resource-review.md` §4：
独立任务方案约 +1164 B，在 `stm32vldiscovery` 布局下**直接链接失败**。

### 10.2 模式时间线

| 事件 | 固件 t（ms） | 标志 |
| --- | --- | --- |
| 上电后首次链路超时（尚无帧） | 320 | T |
| 首帧到达，超时清除 | 1028 | — |
| INIT → NORMAL | 1130 | 无 |
| CRC 错帧阶段 → DEGRADED | 5274 | CRC |
| 持续错帧 → STALE | 5401 | STALE, CRC |
| 恢复，标志全清 | 6825 | — |
| DEGRADED → NORMAL | 6907 | 无 |
| 越界帧 → DEGRADED | 8469 | RNG |
| STALE 置位 | 8713 | STALE, RNG |
| STALE 清除 | 9575 | RNG |
| RNG 保持到期 | 10518 | — |
| DEGRADED → NORMAL | 10641 | 无 |
| 突变帧 → DEGRADED | 11484 | RATE |
| STALE 置位 | 11728 | STALE, RATE |
| STALE 清除 | 12191 | RATE |
| RATE 保持到期 | 13134 | — |
| DEGRADED → NORMAL | 13235 | 无 |
| 剧本进入 `silent`（5.00 s）→ DEGRADED | 13636 | T, STALE, **GND**（三者同一 tick，`set=0x43`） |
| **rx 阻塞注入开始**（`[debug]`，非故障） | 16534 | T, STALE |
| DEGRADED → SAFE | 16641 | T, STALE |
| **health 读到 `hb=415 > 400`**，`[gate] reason=HB+FLAGS` | 16949 | T, STALE |
| **rx 阻塞注入结束**（`dur=800`） | 17334 | T, STALE |
| 门控原因回落 `FLAGS`（`hb=20`） | 17556 | T, STALE |
| `silent` 段结束（7.00 s）：T、STALE、GND 同一行清除（`clear=0x43`） | 18925 | — |
| SAFE → DEGRADED | 19149 | 无 |
| DEGRADED → NORMAL | 19310 | 无 |
| 剧本进入 `hb_only`（8.20 s）：SENSOR 停发 → DEGRADED | 22510 | **STALE（T=0，GND=0）** |
| 数据层持续不可用 → SAFE | 25514 | STALE |
| 剧本进入 `hb_lost`（11.80 s）：SENSOR 恢复，STALE 清除 | 32216 | — |
| 地面站心跳超时（与 SAFE→DEGRADED 同一 tick） | 32438 | **GND** |
| DEGRADED → NORMAL（**GND 仍在置位**） | 32604 | GND |
| 剧本结束（14.3 s，地面站停止注入）→ DEGRADED | 39387 | T, STALE, GND |
| → SAFE | 42396 | T, STALE, GND |

阻塞注入的五行是本表里唯一"由固件而非 PC 制造"的事件，所以它们有确定的固件时刻；
PC 剧本的事件时刻会随跨运行漂移（见 §10.3 末段）。四处容易被读错的地方：

- `DEGRADED → SAFE`（16641）落在阻塞窗口**之内**。这不是"阻塞导致进 SAFE"：停留计时从
  13636 起算、由 ctrl 推进，与 rx 是否活着无关。上一轮运行该迁移落在阻塞结束之后，
  两个读数都合法——把两件事读成因果就错了。阻塞结束到 SAFE 只有 16641−16534=107 tick，
  不足以构成因果；真正的因果证据是 16949 那行 `[gate] reason=HB+FLAGS`。
- 13636 那行 **GND 与 T/STALE 落在同一 tick**（`set=0x43`），上一轮则相差 27 ms
  （GND 先）。这不是随机噪声：GND 门限比链路门限长 200 ms，但最后一次心跳可能比最后
  一帧数据早到（心跳 10 Hz、数据 50 Hz），两个效应方向相反、量级相近，所以谁先到期
  取决于静默起点落在 100 ms 心跳网格的哪个位置。判定器因此不用固定顺序，见 C16。
- 18925 一次性清除 `0x43`：`silent` 结束时 SENSOR 与 STATUS 在同一次注入重启里恢复，
  两边的"最后一次到达"也被同一个时刻刷新。
- 32604 那行 `DEGRADED -> NORMAL flags=0x40` 是最直接的"解耦"证据：地面站心跳在 32438 就已
  判超时（`GND=1`），一帧 SENSOR 数据照样把系统从 SAFE 恢复到 NORMAL；再往后到 `t=35256`
  的 `[stat]` 里 `gage=3319`（心跳已断 3.3 s），模式仍是 `NORMAL`、数据层无任何故障位。

**丢帧段不会单独出现在这张表里**：7.60–7.66 s 连丢 3 帧，它不产生新的故障位，也不多降一次级。
可核对的只有两处——`gap` 多出恰好 3（C7）、`NORMAL→DEGRADED` 次数不变（C13）。反过来，
本节也**不能**断言"丢帧没有推迟任何一次恢复"：注入按墙钟排期、固件按虚拟时钟走，落点无法对准
固定的固件时刻，若正好压在某次恢复窗口上，那次 `-> NORMAL` 会晚 3 帧，日志里分不出来。

### 10.3 检测与恢复延迟（同一份日志的差值）

| 项 | 实测 | 起点 |
| --- | --- | --- |
| CRC 判据 | 连续 3 帧（50 Hz ⇒ 约 60 ms 固件） | 第一帧 CRC 错；单帧 CRC 错不降级，避免偶发误码误触发 |
| 范围/突变判据 | 单帧，**同 tick** 降级 | 越界帧被拒；不需要累计确认 |
| 链路超时 | 300 ms 门限 + ≤20 ms 检查颗粒 | 最后一帧 CRC 正确的时刻 |
| 数据陈旧 | 300 ms | 最后一**有效**样本；与链路超时独立计时 |
| 地面站心跳超时 | 500 ms（5 个心跳周期）+ ≤20 ms 检查颗粒 | 最后一帧 `STATUS` |
| 任务阻塞 → 心跳判超时 | 415 tick（本轮；其中 400 是门限本身，其余是 health 的 100 ms 检查颗粒；跨运行 415–496） | rx 阻塞开始；`[debug]` → `[gate] reason=HB*` |
| DEGRADED → SAFE | 3003–3017 ms（多次运行；本轮 3005 / 3004 / 3009） | 进入 DEGRADED；门限 3000 ms |
| 故障清除 → NORMAL | 82–163 tick（3 个有效帧；本轮 82 / 123 / 101） | 最后一类标志清除 |
| 故障清除 → `SAFE→DEGRADED` | 205–246 tick（5 个有效帧；本轮 224 / 222） | 同上；两级恢复的第一级 |
| `SAFE→DEGRADED` → NORMAL | 本轮 161 tick | 上一行的迁移完成时刻 |
| 进入 SAFE → 退出 SAFE | 本轮 2508 / 6924 ms | **不是固件判据**，由剧本何时恢复注入决定 |

同一状态迁移可能有两个读数：从"故障清除"算与从"进入 SAFE"算不是一件事，表中按起点分开列。
"任务阻塞 → 心跳判超时"同理：它的起点是阻塞开始（固件事件），不是 PC 侧的投帧时刻。

**跨运行偏差不是常数。** 逐事件对照两次运行的同一事件：`t≈1100` 差 41 tick、`t≈7500` 差 30、
`t≈13000` 差 75、`t≈27000` 差 154，即约 0.6% 固件时间的累计漂移——QEMU 虚拟时钟相对注入器
墙钟的比例随主机负载变化，越靠后的事件差得越多。所以上表的**区间**可以引用，**单次 `t=` 不能
跨运行逐字比对**；也难以靠"给时间戳加容差"写出稳定测试，判定器因此改判不变量（见
`tests/qemu-stm32/aviation/expect.py`）。门限类结论（3000 ms、连续 N 帧）不受影响。

本表数值不需要手算：`python3 tests/qemu-stm32/aviation/expect.py logs/aviation_node.log`
的 `[timing]` 段会直接给出六类延迟。注意其中"故障置位 → 降级"读数为 `0 tick`，它量的是
"标志位被置上 → 状态机改模式"，不是"故障发生 → 被检出"——后者跨端没有共同时基。

### 10.4 计数自洽

终态：`frames=377`、`crc=30`、`len=0`、`type=0`、`gap=313`、`dup=0`、`gnd=93`。
`gap = 30（CRC 错帧）+ 280（静默时隙：`silent` 100 + `hb_only` 180）+ 3（丢帧段）= 313`，
与 `crc=30` 交叉核对一致。`frames = 690 − 280 − 3 − 30 = 377`：690 个时隙里发出去 407 帧，
其中 30 帧 CRC 错被拒。

`gnd=93` 同样可手算：13.8 s 里每 5 个时隙一帧 = 138 帧，减去 `silent`（100 时隙 = 20 帧）
与 `hb_lost`（125 时隙 = 25 帧）= 93。这个数字与设计完全一致，说明 `NO_STATUS_PHASES`
与 `STATUS_EVERY` 没有错位——曾经用它排除了一个"QEMU 丢字节"的错误猜测。

### 10.5 栈水位（余量/总量，字）

| 任务 | 余量 | 相对余量 | 判据（绝对 ≥32 / 相对 ≥25%） |
| --- | --- | --- | --- |
| `rx` | 100/224 | 44.6% | 通过（注入后最低值；注入前 164） |
| `ctrl` | 104/224 | 46.4% | 通过（首条 `[stat]` 读到 112，此后恒定 104） |
| `health` | 88/256 | 34.4% | 通过 |

`rx` 的 100 是**调试注入构建**的最低值：`[debug]` 那两条 `printf` 在 rx 自己的栈上跑，
比注入前的 164 多吃 64 字。非注入构建（`-DAV_DEBUG_BLOCK_RX_MS=0`）不背这笔开销，
但也不该把 164 当成"还有 164 字可用"——真正的最坏值取决于最长的一次 `printf`。

`health` 从 224 提到 256，是这条判据自己触发的结果：往 `[stat]` 加 `gnd`/`gage` 两个字段
之后，它的余量从 64 掉到 **56/224 = 25.0%**，正好压在阈值上。这不是"勉强通过"，
而是"下次再加一个字段就失败"；同一份 README 早先写过"若继续加，应先把 `health` 栈提到
256 字"，本次按它执行。提到 256 后读到 88/256，同时稳态堆从 248 降到 120 B。

首条 `[stat]` 的 164 / 112 / 96 比后续的 100 / 104 / 88 大：水位读的是**自己这次 `printf`
之前**的深度，而 `[stat]` 那一行本身就是这几个任务里最长的一次格式化调用。判据按最低值取。
这条也解释了为什么"往 `[stat]` 加字段"是栈风险最高的改动：它直接加深 health 自身的最坏栈。

### 10.6 日志原文（关键片段）

```text
[av-node] CRC16-CCITT-FALSE("123456789") = 0x29B1 PASS
[av-node] 判据: 链路超时 300 ms / 数据陈旧 300 ms / CRC 连续 3 帧 / 故障保持 1000 ms
[av-node] 模式: INIT→NORMAL(3 帧) / 故障→DEGRADED / 持续 3000 ms→SAFE / SAFE→DEGRADED(5 帧)
[av-node] IWDG 路径 未启用（QEMU 未建模）；代码地址 init=0x0800010D feed=0x08000131
[av-node] 下行 USART2 已启用：TYPE=0x81 LEN=18 baud=115200（PCLK1=8000000 Hz 假设）
[av-node] 启动调度器，初始空闲堆 736 B
[fault] t=320 set=0x01 clear=0x00 (T=1 STALE=0 CRC=0 RNG=0 RATE=0 SRC=0 GND=0)
[fault] t=1028 set=0x00 clear=0x01 (T=0 STALE=0 CRC=0 RNG=0 RATE=0 SRC=0 GND=0)
[mode] t=1130 INIT -> NORMAL flags=0x00 cmd=0 air=150 pitch=100
[mode] t=5274 NORMAL -> DEGRADED flags=0x04 cmd=-50 air=150 pitch=100
[fault] t=5401 set=0x02 clear=0x00 (T=0 STALE=1 CRC=1 RNG=0 RATE=0 SRC=0 GND=0)
[fault] t=6825 set=0x00 clear=0x06 (T=0 STALE=0 CRC=0 RNG=0 RATE=0 SRC=0 GND=0)
[mode] t=11484 NORMAL -> DEGRADED flags=0x10 cmd=-50 air=150 pitch=100
[fault] t=11728 set=0x02 clear=0x00 (T=0 STALE=1 CRC=0 RNG=0 RATE=1 SRC=0 GND=0)
[mode] t=13636 NORMAL -> DEGRADED flags=0x43 cmd=-50 air=150 pitch=100
[fault] t=13636 set=0x43 clear=0x00 (T=1 STALE=1 CRC=0 RNG=0 RATE=0 SRC=0 GND=1)
[gate] t=13731 feed=OFF reason=FLAGS hb=197 flags=0x43
[debug] t=16534 rx 阻塞开始 plant=800 ms（其他任务继续运行）
[mode] t=16641 DEGRADED -> SAFE flags=0x43 cmd=-50 air=150 pitch=100
[gate] t=16949 feed=OFF reason=HB+FLAGS hb=415 flags=0x43
[debug] t=17334 rx 阻塞结束 dur=800 ms
[gate] t=17556 feed=OFF reason=FLAGS hb=20 flags=0x43
[fault] t=18925 set=0x00 clear=0x43 (T=0 STALE=0 CRC=0 RNG=0 RATE=0 SRC=0 GND=0)
[mode] t=19149 SAFE -> DEGRADED flags=0x00 cmd=0 air=150 pitch=100
[mode] t=19310 DEGRADED -> NORMAL flags=0x00 cmd=0 air=150 pitch=100
[mode] t=22510 NORMAL -> DEGRADED flags=0x02 cmd=-50 air=150 pitch=100
[fault] t=32216 set=0x00 clear=0x02 (T=0 STALE=0 CRC=0 RNG=0 RATE=0 SRC=0 GND=0)
[mode] t=32438 SAFE -> DEGRADED flags=0x40 cmd=0 air=150 pitch=100
[fault] t=32438 set=0x40 clear=0x00 (T=0 STALE=0 CRC=0 RNG=0 RATE=0 SRC=0 GND=1)
[mode] t=32604 DEGRADED -> NORMAL flags=0x40 cmd=0 air=150 pitch=100
[stat] t=50334 mode=SAFE cmd=0 flags=0x43[T=1 STALE=1 CRC=0 RNG=0 RATE=0 SRC=0 GND=1] air=150 pitch=100 age=11266 frames=377 crc=30 len=0 type=0 gap=313 dup=0 hb=66 gnd=93 gage=18397 feed=OFF(208/292) heap=120 hwm=100/104/88
```

`cmd` 的三种行为可直接对照：NORMAL 下由控制律给出 `-50`（=−1.00°×0.5，空速误差为 0）；
DEGRADED 保持 `-50`（不跟随已判为不可信的数据）；SAFE 回 `0`（中立位），且切换过程
仍受每 20 ms 最多 1.00° 的速率限幅约束。

GND 的三段存在感也可以直接对照：`t=13636` 与 T/STALE 同一行置位（链路整段断开）、
`t=18925` 与它们同一行清除（链路整段恢复）、`t=32438` **单独**置位（只断地面站），
后者期间模式照常走到 NORMAL。

### 10.7 下行二进制通道（USART2）

抓取与判定的完整命令是 `bash run.sh aviation-link`：它把第二个串口后端写进
`logs/aviation_downlink.bin`，再以 `link.py`（不变量 L1–L7）与 `expect.py`（C1–C17）
分别判定**两条通道**，任一失败即整体失败。下列读数是同一次运行的产物。

| 项 | 值 |
| --- | --- |
| 原始字节 / 解出帧 | 13600 B / 544 帧 |
| CRC 错 / 长度错 / 类型错 / 跳过字节 | 0 / 0 / 0 / 0 |
| 帧间隔（固件 ms） | 中位 100，最小 100，最大 117（设计 100） |
| `SEQ` | 0 → 31（回绕两次），逐帧 +1，无跳号无重复 |
| `uptime` | 101 → 54767 ms，严格递增 |
| 模式覆盖 | INIT / NORMAL / DEGRADED / SAFE 四种都出现过 |
| 与上行日志交叉核对 | 10 组同一 `t` 的记录，mode/flags/frames/crc/gap 全部一致 |
| 判定结果 | L1–L7 全通过；上行 log 的 `[timing]` 见 §10.3 |

两点读数纪律：

- **544 帧不能与 690 个上行时隙混用**。前者是 health 任务在约 54.8 s 固件时间里每 100 ms
  一帧的结果，后者是注入剧本的时隙总数（固定 50 Hz），两者之间没有任何换算关系。
- 帧间隔最大值 117 而不是 100，是 health 被高优先级任务（ctrl 20 ms、rx 50 Hz）抢占加上
  自身 `printf` 的正常抖动；`link.py` 只报告不判定，因为 QEMU 时基是固件的 2.90 倍，
  绝对间隔没有校准意义。真正要证明"周期稳定"需要真板 + 逻辑分析仪。

### 10.8 地面站心跳：三层活性拆开看

上行 `STATUS`（0x02，10 Hz）加入后，系统里有三个互不相同的"年龄"：

| 层 | 判据来源 | 刷新者 | 超时 | 置位 | 影响模式？ |
| --- | --- | --- | --- | --- | --- |
| 链路层 | 最近 CRC 正确帧（任意类型） | SENSOR **或** STATUS | 300 ms | `T` | 是 |
| 数据层 | 最近**有效样本** | 只有 SENSOR | 300 ms | `STALE` | 是 |
| 地面站层 | 最近 `STATUS` 心跳 | 只有 STATUS | 500 ms | `GND` | **否** |

为什么不是两层：链路层问的是"这条线还通吗"，地面站层问的是"对端还在吗"。两者在同一条
物理链路上，但**方向相反**——传感器停发时线还是通的（心跳在刷），地面站停发时数据也可能
还在来。合并成一个位，就没法区分"该查线路"还是"该查对端"。

本轮剧本给了两个方向的对照，两条都在 `logs/aviation_node.log` 里：

| 段 | 实测 | 读法 |
| --- | --- | --- |
| `hb_only`（8.20–11.80 s，只发心跳） | `t=25196 T=0 STALE=1 GND=0 gage=181 frames=252` | 线通的（`T=0`）、对端在的（`GND=0`），但**没有数据**（`STALE=1`）。系统照常降级到 DEGRADED → SAFE——心跳不能替代数据 |
| `hb_lost`（11.80–14.30 s，只发数据） | `t=35256 T=0 STALE=0 GND=1 gage=3319 mode=NORMAL` | 数据和链路都正常，只有对端不见了。模式**不变** |

`hb_only` 段里系统进了 SAFE，这不是 bug：数据层真的没有样本，而 `AV_DEGRADED_TIMEOUT_MS`
只问"故障持续多久"。想"有心跳就继续飞"需要的是另一条策略（用外推值维持），不在本项目的
范围内——本项目只证明**这两件事被分开了**。

判定由 C16 / C17 两条不变量负责，两条都做过负向验证（对同一份日志做破坏性修改后重跑
`expect.py`，四份变异全部被抓住、退出码均为 1）：

| 变异 | 期望 | 实测结果 |
| --- | --- | --- |
| 删掉 GROUND 上升沿 | C16 FAIL | FAIL C16「GROUND 从未置位」 |
| 在 GROUND 窗口内插入一次 `NORMAL→DEGRADED` | C16 FAIL | FAIL C16「t=13666 之后仍有 1 次降级」，同时 FAIL C13（降级 7 次 ≠ 6） |
| GROUND 期间所有 `[stat]` 改成 `mode=SAFE` | C16 FAIL | FAIL C16「GROUND 置位期间没有一条 stat 处于 NORMAL」 |
| 让相邻 `[stat]` 的 `frames` 递增 | C17 FAIL | FAIL C17「帧数不变的相邻对 0 个」 |

第二条顺带被 C13 抓到，说明两个判据**独立地**看见了同一次多降——这正是要的：一条判据的
漏网不该由另一条来兜底，但两条都该看得见。

C16 的窗口右端用"下一次 `T`/`STALE` 上升沿"而不是固定时长：剧本结束时注入器退出、链路
真的断了，那次降级是**应该**发生的，不能算进本条。右端取 `>=` 而不是 `>`：`silent` 段里
GROUND 可能与 `T`/`STALE` 落在同一 tick（本轮 `t=13636 set=0x43` 就是），此时窗口为空，
那次上升沿无法归因——而不是让它一路延伸到剧本末尾去抓 `silent` 的降级。两轮运行分别
命中"同 tick"与"差 27 ms"两种形状，用 `>` 会偶发 FAIL。

已知边界：**从未收到过地面站心跳不算失联**（`s_ground_seen` 门控，与"从未收到有效样本
不算陈旧"同构）。上电初期地面站还没连上是正常过程，若算作失联，每次冷启动都会先报一次
GROUND。代价是"地面站一直没连上"不可见：`[stat] gnd=0 gage=<上电以来>` 只能由人读出来，
不是故障位。

## 11. 常见错误和陷阱

| 错误 | 后果 | 正确做法 |
| --- | --- | --- |
| CRC 覆盖 SOF 字节 | 同步错误与数据错误无法区分 | CRC 只覆盖 TYPE..PAYLOAD |
| 把"收到字节"当"链路健康" | 持续错帧被当成正常 | 用 CRC 正确的帧刷新链路时间戳 |
| 只保留一个"超时"标志 | 拉断线、全错帧、全越界三种故障在日志里同形 | TIMEOUT / STALE / RANGE 分开 |
| 恢复门限与降级门限相同 | 边界数据让模式 50 Hz 抖动 | 恢复要连续 N 帧且无故障 |
| 故障标志立即清除 | 同上 | 保持窗口 + 停留门限 > 保持窗口 |
| 变化率参考"上次有效样本" | 连续拒收后保护自失效（本项目实测） | 参考"上次收到的样本" |
| 心跳打在"收到数据之后" | 卡在收数据上的任务永远"活着" | 心跳每轮循环打 |
| 多任务各自喂狗 | 卡死任务不喂，其他任务喂饱 | 汇总后"与"语义，唯一喂狗点 |
| 监督任务优先级低于被监督任务 | 对方忙等时监督者被饿死，停喂判定只能事后发生（本项目实测） | 监督者优先级 ≥ 被监督者；否则只能靠 `IWDG` 兜底，且丢失归因 |
| 停喂只记一个 bool | "任务卡住"和"数据坏了"在日志里同形，排查方向相反 | 记原因位（`[gate] reason=HB/FLAGS/HB+FLAGS`） |
| 把地面站心跳当作"数据类故障" | 地面站失联会把飞行模式打进 DEGRADED → SAFE | 模式判定用白名单 `AV_F_MODE_MASK`，`AV_F_GROUND` 不进掩码 |
| 心跳到达就认为"数据可用" | 只发心跳、没有样本时不降级，坏数据继续被用 | 心跳只刷新链路层；`STALE` 只看有效样本年龄 |
| 用黑名单排除不参与判定的故障位 | 新位的默认语义变成"参与"，与飞行无关的故障会悄悄改模式 | 白名单：漏加只影响可见性，不出安全问题 |
| 用 `now > last` 比 tick | 49.7 天回绕点误判 | `(uint32_t)(now - last)` |
| 跨任务读-改-写不加临界区 | 丢计数、丢状态 | `taskENTER_CRITICAL()` |

## 12. 故障排查方法

1. **一帧都收不到**：先看 `[stat] frames=0` 且 `crc=0`——没有字节到，查注入器是否在跑、
   串口是否被前一个 QEMU 占用；若 `crc` 在涨，说明字节到了但帧结构不对，先核对 CRC 参数
   与自检输出。
2. **`crc` 一直涨但 PC 侧算的是对的**：比对启动自检 `0x29B1`；再检查 CRC 覆盖范围
   （是否误含 SOF）与字节序（CRC 字段是小端）。
3. **模式一直在 DEGRADED 出不来**：看 `[fault]` 行的 `clear=` 是否有变化。若标志已清而
   模式不动，查 `good_streak` 是否被某个每 20 ms 就置位的标志反复清零。
4. **`SAFE` 来得莫名其妙**：算一下 `DEGRADED 门限 − 故障保持窗口`，若接近 0，就是本节
   §4.4 那个耦合。
5. **`gap` 增长快于预期**：注意 `gap` 统计的是序号跳变，CRC 错帧也会计入（因为它们的序号
   没被"看到"）。与 `crc` 对照后再下结论。
6. **`feed=OFF` 一直是 OFF**：先看 `T`/`STALE`；再看 `hb`（rx 心跳年龄），超过 400 说明
   rx 任务卡住，而不是数据问题。
7. **仿真跑得比剧本快**：正常。固件毫秒比真实时间快 2.90 倍，剧本按真实秒排期。

## 13. 练习题

**基础**

1. 把 `AV_LINK_TIMEOUT_MS` 从 300 改成 100，预测哪些阶段会出现额外的 `T` 标志位沿，
   跑完对照。
2. 把注入频率从 50 Hz 改成 20 Hz，说明为什么 `AV_RATE_WINDOW_MS`（200 ms）仍然够用，
   以及改成 5 Hz 时会出什么问题。

**改错**

3. 把 `AV_GOOD_TO_NORMAL` 改成 1，观察越界/突变阶段结束后模式是否开始抖动，并用日志
   给出抖动频率。
4. 把 `av_sensor_accept` 的变化率参考时刻改回 `last_good_tick`，复现 §6.2 的第二个缺陷，
   写出你能看到的现象差异。

**综合**

5. 上行 `STATUS`（`0x02`）地面站心跳**已经实现**，把它当参考实现读（`av_frame.h` 的类型
   定义、`av_health_ground_ok`、`[stat]` 的 `gnd`/`gage` 字段、C16/C17）。在此基础上扩展：
   让上行 `STATUS` 携带一个 `ground_flags` 字段（地面站自报的健康位）。要求：
   (a) 地面站自报故障**不得**改变飞行模式；(b) `[stat]` 里独立统计；(c) 写一条能抓住
   "它误参与了模式判定"的判据。说明为什么它和 `AV_F_GROUND` 同属"对端的事"，
   而不是数据类故障——以及为什么"对端的事"永远不该进 `AV_F_MODE_MASK`。
6. 实现"故障记录"的 RAM 侧原型：把最近 8 次模式切换（时刻 + 标志位图）环形存储，
   在 `[stat]` 里输出最近一条。说明上真板后改用备份寄存器/Flash 需要改哪些地方。
7. 设计"降级后的替代数据"：在 `SAFE` 之前插入一个使用上一有效样本外推的中间模式，
   写清外推失效条件，以及它会不会掩盖传感器故障。

## 14. 验收标准

- [x] PC 能通过 UART 注入传感器帧，MCU 侧有帧计数与 CRC 统计。
- [x] CRC 自检用已知答案（`0x29B1`）在启动时通过。
- [x] 四类异常（错帧 / 越界 / 突变 / 静默）各有独立标志位与日志证据。
- [x] 每个异常都能进入 `DEGRADED`，且恢复需要"连续有效帧 + 标志清除"。
- [x] 长时间故障能推进到 `SAFE`，恢复走两级路径。
- [x] 喂狗门控为"与"语义，日志中 `feed=ON/OFF` 与标志位一致。
- [x] 计数器自洽（`gap = crc + 静默时隙 + 丢帧`）。
- [x] 三个任务栈水位按"绝对 ≥32 字 / 相对 ≥25%"判定通过（100/224、104/224、88/256）。
- [x] 连丢 3 帧（短于超时门限）只增 `gap`、不降级，由 C13 机器判定。
- [x] 任务阻塞注入（编译期，仅 QEMU 构建）：`hb` 越过 400 ms 门限，`[gate]` 出现 `reason=HB`，
      阻塞结束后 `hb` 回落；由 C14 机器判定（未启用注入时转"本条不适用"）。
- [x] 24 项不变量由 `tests/qemu-stm32/aviation/expect.py` 机器判定通过（`run.sh aviation-link` 退出码 0）；
      改坏 `gap` 返回 1 且只 FAIL C7，删掉 HB 门控行返回 1 且只 FAIL C14，日志缺失返回 2。
      C16/C17 另有四份破坏性变异的负向验证，见 §10.8。
- [x] 下行二进制 `STATUS` 帧（USART2，`TYPE 0x81`、18 B 载荷、100 ms），由 `aviation/link.py` 判 7 项
      并通过与上行同一 `t` 的交叉核对；实测 544 帧 / 13600 B、CRC 全通过、`SEQ` 连续。
- [x] 上行 `STATUS` 地面站心跳（`TYPE 0x02`、4 B 载荷、10 Hz）：刷新链路层、不刷新数据层；
      不进 `AV_F_MODE_MASK`，地面站失联不改变飞行模式。`hb_only` / `hb_lost` 两个方向各有一段
      实测证据，由 C16 / C17 机器判定（`t=25196 STALE=1 GND=0`；`t=35256 GND=1 mode=NORMAL`）。
- [ ] **待真板**：CAN 总线帧、ADC 采样、示波器周期测量、`IWDG` 复位与复位原因、下行发送抖动。
- [ ] **待真板**：故障记录的掉电保持（备份寄存器或 Flash 页）。
- [ ] 未完成：下行命令通道（USART2 目前只有 TX）。

## 15. 已知限制

- QEMU 时基偏差 2.90 倍，所有毫秒数字只能作比例参考；周期抖动与最坏执行时间无从测量。
- 下行是**单向**二进制通道：没有命令、没有确认、没有流控/重传；丢一帧就少一个 100 ms 快照，
  由地面站按 `SEQ` 缺口自行判断。发送只等 `TXE`，超时即弃帧（理由见 `docs/architecture.md` §4）。
- 上下行各用自己的计数器（上行 `SENSOR` SEQ、上行 `STATUS` SEQ、下行 `SEQ`、下行 `uptime_ms`），
  四者没有公共时基；跨端"故障发生 → 本机检出"的延迟无法用它算。
- 控制律、限幅值、参考空速都是教学级参数，与任何真实机型无关。
- 只支持单传感器、单实例、无冗余通道；双通道表决留给后续。
- 变化率判据对"真实阶跃"会持续拒收（直到回到原值或出现 >200 ms 空档）。这是
  `RATE` 保护与"允许机动"之间的取舍，真实系统需要多帧中值/斜率一致性判据。
- `av_health_stats()` 的单写单读约定不是通用做法，增加写者必须加保护。
- 地面站心跳的"从未收到不算失联"门控（§10.8）让"地面站一直没连上"不可见：
  `[stat] gnd=0 gage=<上电以来>` 只能人工判断，不是故障位。
- 稳态空闲堆 **120 B**，已接近这条链路的余量下限：`health` 栈提到 256 字用掉了 128 B
  （任务栈来自 `ucHeap`）。再要加任务或加栈，必须先提高 `configTOTAL_HEAP_SIZE`。
  绝对读数还受 `ucHeap` 链接对齐影响（早期构建读到过 432 / 440 / 248），比较不同构建
  要看差值，不要把某个具体数字当成常量。
- 8 KB SRAM 只在 QEMU 缺省机型的限制下成立；真板 F103C8T6 有 20 KB，堆/栈余量不可
  直接外推。

## 16. 下一步

- 真板闭环：§9 的六项，优先 CAN 与 IWDG（前者补齐通信能力，后者补齐复位路径）。
- 路线图第 9–10 周：把注入剧本扩展为**自动化故障注入套件**。已落地——`aviation/expect.py`
  的 24 项不变量判定与 `aviation/link.py` 的 7 项下行判定接入 `run.sh aviation-link`，
  并自动打印六类检测/恢复延迟；日志从"人工看"变成"机器判"。
  七类故障已覆盖：错帧 / 越界 / 突变 / 静默 / 丢帧 / 任务阻塞（编译期注入，不是 PC 侧剧本）/
  地面站失联（`hb_lost`），另有"只发心跳、没有数据"（`hb_only`）作为数据层的对照。
  剩余一类**在 QEMU 里做不了**：
  - **复位原因**：`RCC->CSR` 读回 0（探针 `[11]`/`[12]`），`IWDG` 也不计时，必须真板。
- 判定器自身也需要维护：C16/C17 的窗口定义依赖剧本的段界（`silent` 与 `hb_only` 相邻），
  改 `inject.py` 的段序就要重看这两条，`--silent-slots 280` 也要跟着改。
- 第 11 周：补架构图、任务时序图、状态机图与需求-测试追踪矩阵（`docs/protocol.md`
  已有一份追踪矩阵雏形）。
