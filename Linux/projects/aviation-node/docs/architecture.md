# 架构说明（分层视图）

> 本文给的是"代码怎么分层、依赖朝哪个方向、改一层要连带改什么"。任务时序与实时性边界
> 见 [`task-timing.md`](task-timing.md)，协议与状态迁移规则见 [`protocol.md`](protocol.md)，
> 实测结论见 [`test-report.md`](test-report.md) 与 [`../README.md`](../README.md) §10。
>
> 范围：`projects/aviation-node/` 的可仿真软件。CAN、ADC、`IWDG` 复位、掉电保持**不在**
> 本架构内（无实现、无证据，见 `../README.md` §9）。

## 1. 分层视图

```text
┌──────────────────────────────────────────────────────────────────────────┐
│ L4 应用 / 控制     ctrl 任务：控制律 → 限幅 → 速率限幅 → s_cmd           │  20 ms
│                    模式输出选择（NORMAL 跟随 / DEGRADED 保持 / SAFE 中立）│
├──────────────────────────────────────────────────────────────────────────┤
│ L3 业务逻辑        av_mode   状态机：只看 flags 是否非 0，不看是哪一位    │
│                    av_health 故障位聚合、保持窗口、统计、心跳年龄         │
├──────────────────────────────────────────────────────────────────────────┤
│ L2 数据判定        av_sensor 样本层：数据源 → 范围 → 变化率；last_good    │
│                    av_frame  帧层：SOF/LEN/CRC16，逐字节状态机            │
├──────────────────────────────────────────────────────────────────────────┤
│ L1 运行支撑        rx / ctrl / health 任务、USART1 ISR、队列、互斥量       │
│                    av_downlink 下行 STATUS 帧（USART2 TX，100 ms）        │
│                    f103_uart 驱动、FreeRTOS 内核（调度/队列/信号量）      │
├──────────────────────────────────────────────────────────────────────────┤
│ L0 硬件            USART1 · USART2(TX) · SysTick · NVIC · Flash/SRAM      │
│                    (IWDG 未启用)                                          │
└──────────────────────────────────────────────────────────────────────────┘
```

依赖方向**向下**，没有反向 include：

| 模块 | 依赖 | 提供的接口 |
| --- | --- | --- |
| `av_frame` | 仅 `<stdint.h>` | `av_crc16_ccitt_false`、`av_parser_push` |
| `av_tick` | FreeRTOS（`TickType_t`） | `av_tick_elapsed`（跨任务 tick 差值，防无符号下溢） |
| `av_sensor` | `av_frame`、`av_tick`、FreeRTOS | `av_sensor_accept`（返回 `AV_VERDICT_OK/RANGE/RATE/SOURCE`）、`av_sensor_age` |
| `av_health` | `av_sensor`、`av_tick`、FreeRTOS | `av_health_kick/frame_ok/crc_error/...`、`av_health_flags`、`av_health_rx_age` |
| `av_mode` | FreeRTOS（仅 `TickType_t`/`pdMS_TO_TICKS`） | `av_mode_step`、`av_mode_name` |
| `av_downlink` | `av_frame`（CRC）、`av_health`/`av_sensor`/`av_mode` 类型、`stm32f1xx.h` | `av_downlink_init`、`av_downlink_status` |
| `main.c` | 以上全部 + `f103_uart` | 任务、ISR、喂狗门控、日志、控制律 |

`av_mode` 不认识 `AV_F_*` 位定义，是刻意的：**新增一种故障只需在 `av_health` 加一位，
状态机一行都不用改**。反过来，状态机也不该知道"这是因为 CRC 错还是超时"——那是运维信息，
不是控制决策。

## 2. 运行时视图

```text
 PC 注入 ──> USART1 RXNE ISR ──xQueueSendFromISR──> s_rx_queue(64×1B)
                    │ 读 SR → 读 DR（同时清 RXNE/ORE）→ ORE 分支只计数不投递
                    │ → 入队，失败则 s_rx_drops++ 并 portYIELD_FROM_ISR；中断里从不阻塞
                    ▼
              rx 任务（prio 3）── av_frame 解析 ── 坏帧：av_health.crc_error/len_error/type_error
                    │                    ├── CRC 正确的 SENSOR 帧：av_health.frame_ok(seq)
                    │                    │     （只证明"链路可用"，不代表内容可用 → 这是 T 先于 STALE 清除的原因）
                    │                    └── av_sensor_accept ──> OK: last_good + good_streak
                    │                                          RANGE/RATE/SOURCE: av_health.mark
                    │  每 50 ms：av_health.kick（心跳按"每轮循环"打点，不按字节）
                    ▼
              ctrl 任务（prio 4，20 ms）── av_health_flags(now)
                    │                        └── 只读：T/STALE/CRC/RNG/RATE/SRC
                    ├── av_mode_step（临界区内）──> 模式 + cmd
                    └── [mode]/[fault] 边沿上报
                    ▼
              health 任务（prio 2，100 ms）── rx_age + flags
                    ├── 喂狗门控：rx_live && !(T|STALE)，唯一喂狗点
                    ├── [gate] 变化时上报（原因位 OK/FLAGS/HB/HB+FLAGS）
                    ├── 下行 STATUS 帧（USART2 TX，25 B；TXE 超时只报一次）
                    └── 每 5000 ms：[stat]（堆、栈水位、计数、hb、feed 计数）
```

下行与 `[stat]` 在同一轮 health 里先后发生，但**不是原子的**：`av_downlink_status()`
的 TXE 轮询期间 rx（优先级 3）会抢占并再接受若干帧，因此同一 `t` 的两个视角里
`[stat]` 的 `frames` 可能比下行帧大 1–2。`link.py` 的交叉核对按"只增 + 增量 ≤3"判，
逐字相等会把抢占误判成缺陷（首次实现就这样错过一次）。

共享数据与保护：

| 数据 | 写入者 | 读取者 | 保护方式 |
| --- | --- | --- | --- |
| `s_rx_queue` | ISR | rx | FreeRTOS 队列（`FromISR` 版本） |
| `s_health` 内部状态（stats / hold / kick / last_frame） | rx（`kick` 也在 rx） | ctrl、health | 32 位对齐的单字写；读侧用 `now - stamp` 无符号差值 |
| `s_sensor.last_good` 等 | rx | ctrl、health | 同上（结构体按值赋值在 32 位总线上非原子，但只在语义上要求"旧值或新值"） |
| `s_mode`（mode / enter_tick / good_streak） | rx 只改 `good_streak`；ctrl 改 `mode` | ctrl、health | `taskENTER_CRITICAL()` 包住自增与 `av_mode_step` |
| `printf` | 三个任务 | — | `s_log_mutex` |

## 3. 判定的分层顺序（为什么是这个顺序）

```text
帧层   SOF → LEN → CRC          错一帧就丢，不进样本层
  ↓    （CRC 错连续 3 帧才置 CRC 位，避免偶发误码直接降级）
样本层 数据源标志 → 范围 → 变化率
  ↓    （顺序固定：源不可信时没必要再判范围；范围错时变化率已无意义）
时间层 T（无 CRC 正确帧 300 ms） / STALE（无有效样本 300 ms），两条独立计时
  ↓
状态机 任一位置位 → DEGRADED；持续 3000 ms → SAFE；SAFE 需 5 帧才回落
```

把"帧坏了"和"样本坏了"分开，是为了让日志能回答**该查链路还是该查传感器**：
`crc` 计数涨说明链路/协议；`RNG/RATE` 涨说明数据源；`T` 单独涨说明一个字节都没来。

## 4. 关键取舍（含代价）

| 决定 | 收益 | 代价 / 已知边界 |
| --- | --- | --- |
| 状态机只吃"flags 是否为 0" | 新增故障位不动状态机 | 状态机无法区分严重程度；分级需要另加严重度字段 |
| 单点喂狗（health 独占） | "任一通道不健康即整体停喂"可验证 | 监督任务优先级最低时看不见忙等（`task-timing.md` §3） |
| 门控只看 `T\|STALE` 与 rx 心跳，不看 `CRC/RNG/RATE/SRC` | 单帧坏数据不会复位整机；质量类故障交给模式状态机 | 门控不是"全故障"判据——数据坏了仍在喂狗，靠降级而不靠复位兜底 |
| 故障标志保持 1000 ms | 50 Hz 抖动不会让模式来回跳 | 单帧事件的影响被拉长；已由 3000 ms 停留门限吸收 |
| 变化率参考"上次**收到**的样本"而非"上次**有效**样本"（窗口 200 ms） | 持续突变下突变保护不会自我关闭 | 连续拒收会推进到 `STALE`——首轮实测发现的耦合，见 `test-report.md` §3.3 |
| 恢复需要连续 N 帧 + 无故障 | 边界数据不抖动 | 恢复慢 3–5 帧（实测 122–243 tick） |
| 速率限幅对所有模式生效（`CMD_STEP_CDEG` = 100，即 1.00°/20 ms） | 模式切换本身不产生指令阶跃 | DEGRADED→NORMAL 的收敛被按斜率限制拖慢 |
| 心跳按"每轮循环"打点 | 卡在队列上也能被发现 | 心跳频率随负载变化，不能当周期抖动指标 |
| 下行另开 USART2，不与 ASCII 日志共用一条流 | 日志可继续按行解析，判定器不被二进制打断 | 需要第二个串口后端（QEMU 已实测可用）；地面站要单独接一路 |
| 下行只等 `TXE`、超时即弃帧，不重试 | 监督任务永远不会卡在串口上（它是唯一喂狗者） | 丢帧由宿主 CRC 丢弃，没有重传/流控 |
| 判定器只判不变量 | 跨运行漂移不影响回归 | 无法验证"绝对时刻正确"，门限边界只能靠用例设计 |

## 5. 资源预算

| 项 | 值 | 来源 |
| --- | --- | --- |
| FLASH | 18028 B / 128 KB（13.75%），`text 17928 / data 100` | `size` 实测（含下行） |
| RAM | 6536 B / 8 KB（79.79%），`bss 6436` | 同上 |
| 内核堆 `configTOTAL_HEAP_SIZE` | 4096 B；启动后余 1048 B、稳态 432 B | 运行日志 |
| 任务栈 | rx 176 字 / ctrl 224 / health 224；最低余量 52 / 106 / 64 | `[stat] hwm` |
| 解析缓冲 | `av_parser_t` 含 32 B 载荷缓冲，在 rx 栈上 | 源码 |
| 下行帧缓冲 | 25 B（`AV_DL_FRAME_MAX`），在 health 栈上 | 源码 |
| 下行通道开销 | 每 100 ms 25 B ≈ 250 B/s，115200 下占用 2.2%（理论） | 推算 |

余量判断：432 B 堆不足以再建一个 128 字的任务（栈 512 B + TCB），加任务必须先提高堆尺寸，
而它是共享配置，会影响其他实验的实测体积；`health` 栈余量 64 字最接近红线——加入 25 B
下行缓冲后实测水位仍是 64 字（缓冲落在已用深度之内），但再加字段前应先提到 256 字。

## 6. 变更影响矩阵

| 要改的东西 | 动的层 | 必须连带确认 |
| --- | --- | --- |
| 新增一种故障判据 | L2 判定 + L3 `av_health` | 新位是否进入 `[stat]`/`[fault]` 打印；`expect.py` 的 C4 是否要加一项 |
| 修改模式门限（3/5 帧、3000 ms） | L3 `av_mode.h` | 停留门限仍须明显大于 1000 ms 保持窗口；C5/C6c/C13 的计数期望 |
| 改控制律或限幅 | L4 | 速率限幅与 ctrl 周期（20 ms）绑定；C12 的期望 `cmd` 值 |
| 改 ctrl/health 周期 | L1 + L4 | `CMD_STEP_CDEG` 的斜率语义；`[stat]` 间隔与 `[timing]` 区间；下行的周期随 health 一起变 |
| 新增/修改下行帧 | L2 协议 + L1 `av_downlink` | `protocol.md` §2.2 与 `av_downlink.h` 的定义要同步；`link.py` 的 `PAYLOAD_FMT` 必须一起改（否则会按旧布局解析而 CRC 仍通过） |
| 打开 `IWDG` | L1 | 真板验证前置：喂狗点必须唯一；复位原因要能读 |

## 7. 与真实航电系统的差距（明确边界）

- **单通道**：没有冗余表决、没有通道不一致判据；`protocol.md` §6 列了双通道需要补什么。
- **下行单向**：USART2 只有 TX，没有命令通道、没有对 `STATUS` 的确认，也没有流控/重传。
- **无持久化**：故障记录只在 RAM，复位即丢；掉电保持需要 Flash/备份域。
- **无时间基准**：只有 tick，没有 RTC/时间同步；跨设备对时需要另行设计。
- **无 BSP 抽象**：`f103_uart`、`av_downlink` 直接操作寄存器，换芯片要改驱动层。
- **无 WCET/抖动证据**：本架构只保证"逻辑正确 + 软件时序自洽"，实时性数字待真板。

以上每一条都是"没做"，不是"做了但简化"——引用本项目时按这个口径描述。
