# 故障注入测试报告（`projects/aviation-node`）

> 记录性质：本报告中**所有数字都来自 `tests/qemu-stm32/logs/aviation_node.log` 与
> `logs/aviation_downlink.bin` 的实测输出**，两者由同一次 `bash run.sh aviation-link`
> 产生（2026-10-27）。标「推算」的是由实测值算出的差值，标「未验证」的是本环境没有
> 证据的部分（真板项）。没有这些标注的结论不得引用。

- 被测对象：`projects/aviation-node/`（教学级地面仿真节点，**不是**飞控）
- 证据日志：`tests/qemu-stm32/logs/aviation_node.log`、`logs/aviation_downlink.bin`
- 机器判定：`aviation/expect.py`（**24** 项不变量，退出码 0/1/2）与 `aviation/link.py`
  （下行通道 L1–L7，退出码 0/1/2）；判定清单见 `tests/qemu-stm32/README.md`
- 协议与状态规范：[`protocol.md`](protocol.md)；构建与运行：[`../README.md`](../README.md) §8

## 1. 环境与判定口径

| 项 | 值 |
| --- | --- |
| 目标 / 仿真器 | STM32F103（Cortex-M3）/ QEMU 10.2.1 `-M stm32vldiscovery` |
| 内核 | FreeRTOS-Kernel V11.3.0（裁剪子集），SysTick 1000 Hz |
| 编译器 | `arm-none-eabi-gcc` 14.2 rel1，`-mcpu=cortex-m3 -mthumb -mfloat-abi=soft`，零警告（`--clean-first` 全量重建） |
| 构建 | `-DF103_QEMU=ON`（8 KB 链接脚本），`AV_DEBUG_BLOCK_RX_MS=800` |
| 固件体积 | FLASH 18256 B（13.93%）、RAM 6552 B（79.98%）、稳态空闲堆 120 B；`text/data/bss = 18156/100/6452` |
| 调试注入段成本 | 288 B FLASH / 8 B RAM（`-DAV_DEBUG_BLOCK_RX_MS=0` 的 `17968/6544` 与本构建差分） |
| 注入器 | `aviation/inject.py`，`SENSOR` 50 Hz + `STATUS` 10 Hz、**13 段**剧本、14.3 s 墙钟（固件约 41 s） |
| 任务栈 | rx 224 / ctrl 224 / health 256 字（`ucHeap` 内分配，`configTOTAL_HEAP_SIZE = 4096`） |

**时间口径**：所有 `t=` 与 `ms` 都是**固件毫秒**。QEMU 以 24 MHz 驱动 SYSCLK 而固件按 8 MHz
计算，实测时基偏差 2.90 倍；且跨运行漂移随时长累积（`t≈1100` 差 41、`t≈27000` 差 154）。
因此本报告只引用**同一份日志内的差值**或**跨运行的区间**，不引用单次绝对时刻作为判据。

**判定分三档**（与 `CLAUDE.md` §1.1 一致）：实测 / 构建级 / 未验证。本报告的用例全部是实测，
但只覆盖软件数据链；硬件外设见 §6。

## 2. 用例与结果总览

剧本每段只改一个变量，故障类别与 `av_health.h` 的标志位一一对应：

| # | 故障类别 | 注入手段（墙钟时段） | 预期 | 实测结论 | 判定 |
| --- | --- | --- | --- | --- | --- |
| 1 | 错帧 | `badcrc` 2.00–2.60 s，30 帧 CRC 取反 | 连续 3 帧才置 `CRC`；`crc` 计数 +30 | 降级 1 次，`crc=30` | C4 / C7 / C13 |
| 2 | 数据越界 | `range` 3.20–3.60 s，pitch 90.00° | 单帧即拒，`RNG` 置位，同拍降级 | 置位与降级同 tick | C4 / C12 |
| 3 | 数据突变 | `rate` 4.30–4.55 s，pitch 1.00°→45.00° | 拒收且不刷新 `last_good`，300 ms 后 `STALE` | `RATE` 后 244 tick `STALE` | C4 / C11 |
| 4 | 通信静默 | `silent` 5.00–7.00 s，无字节（**心跳同时停**） | `T`+`STALE`+`GND` → DEGRADED → 3000 ms → SAFE | 三个超时同 tick 到期；停留 3005 ms | C4 / C6a / C6c / C10 / C13 |
| 5 | 丢帧 | `drop` 7.60–7.66 s，连丢 3 帧（`SEQ` 仍递增） | 只增 `gap` 3，不降级 | `gap=313`，降级恰 6 次 | C7 / C13 |
| 6 | 只发心跳 | `hb_only` 8.20–11.80 s，`SENSOR` 停发 180 时隙、`STATUS` 继续 | 数据层 `STALE`、链路层 `T=0`；降级 1 次 | `t=25196 T=0 STALE=1 GND=0 gage=181` | C4 / C13 / C17 |
| 7 | 只发数据 | `hb_lost` 11.80–14.30 s，`STATUS` 停发、`SENSOR` 继续 | `GND` 置位但**不**改变飞行模式 | `t=32604 DEGRADED→NORMAL flags=0x40`；`t=35256 mode=NORMAL gage=3319` | C4 / C16 |
| 8 | 任务阻塞 | 固件编译期注入，`t=16534` 起 800 ms | 心跳年龄 > 400 ms → 停喂，原因含 `HB` | `hb=415`，`reason=HB+FLAGS` | C14 |
| 9 | 时间戳年龄下溢 | 无需注入（rx 抢占 health 后的时间戳先后） | `age`/`hb` 恒 < 2³¹ | 本轮 0 例；修复前抓到过 `age=4294967292` | C15 |

第 9 项不是剧本里的故障段，而是实测中抓到的固件缺陷：`age` 曾是 `now - 时间戳` 的无符号减法，
当 rx（优先级 3）在 health（优先级 2）取 `now` 之后才写入时间戳时下溢。**`hb` 走同一条路径**，
下溢会让门控误读成"心跳早断了"并停一次喂狗——所以它不只是日志难看。修法见
`../README.md` §6.2，回归判据是 C15。

第 4、6、7 项共同构成"三层活性"的完整对照：`silent` 三层全停，`hb_only` 只停数据层，
`hb_lost` 只停地面站层。三层拆开的推理见 `../README.md` §10.8。

判定结果：**上行 24/24 通过（退出码 0）、下行 7/7 通过（退出码 0）**。

## 3. 逐项记录

### 3.1 错帧（`badcrc`）

- 预期：单帧 CRC 错不降级（避免偶发误码误触发），连续 3 帧置 `CRC` 并降级。
- 实测：`[mode] t=5274 NORMAL -> DEGRADED flags=0x04` 与 `[fault] t=5274 set=0x04` 同 tick；
  `[fault] t=5401 set=0x02`（`STALE` 随后置位——错帧不刷新 `last_good`）；
  `[fault] t=6825 clear=0x06` 后 `[mode] t=6907 DEGRADED -> NORMAL`。
- 交叉核对：`crc=30` 与 600 ms × 50 Hz 一致；`gap` 中计入这 30 帧；`NORMAL→DEGRADED`
  总数恰为剧本预期的 6 次，说明 30 帧错帧没有反复触发。
- 检测与恢复：置位 → 降级 **0 tick**（同拍）；故障清除 → NORMAL **82 tick**。

  82 tick 短于"3 帧 × 58 tick ≈ 174"不是异常（推算，可由代码路径核对）：`good_streak`
  由 rx 在收到有效帧时自增，ctrl 只在看到 `fault != 0` 时清零。`[fault] clear` 那一拍
  streak 可能已累积 1–2，剩余缺口才是 1–2 个帧间隔。区间 82–163 tick（跨运行）。

### 3.2 数据越界（`range`）

- 预期：CRC 正确的帧仍需范围判据；单帧越界即拒收并降级。
- 实测：`[mode] t=8469 NORMAL -> DEGRADED flags=0x08` 与 `[fault] t=8469 set=0x08` 同一 tick；
  `[fault] t=8713 set=0x02`（`STALE`）；`[stat] t=10086 cmd=-50` 保持前一模式的值
  （未跟随越界值）。
- 检测与恢复：置位 → 降级 **0 tick**；`RNG` 保持窗口到期（`t=10518 clear=0x08`）→ NORMAL
  **123 tick**。

### 3.3 数据突变（`rate`）

- 预期：变化率判据以"上一次**收到**的样本"为参考；被拒样本不刷新 `last_good`，
  于是持续拒收会推进到 `STALE`（这是设计意图，也是首轮实测发现的耦合，见 `../README.md` §6.2）。
- 实测：`[mode] t=11484 NORMAL -> DEGRADED flags=0x10` 与 `[fault] t=11484 set=0x10`；
  `[fault] t=11728 set=0x02`（`STALE`）。
- 读数解释（推算，可由日志核对）：`STALE` 从**最后一个有效样本**起算 300 ms，而 `RATE` 边沿本身
  已经晚于该样本约一个帧间隔——50 Hz 的注入帧在 2.90 倍时基下相当于每 **58 固件 tick** 一帧。
  所以 11728 − 11484 = 244 tick 是"300 ms 门限 − 一个帧间隔"，不是门限提前生效。
  佐证：`STALE` 一直持续到 `t=12191` 才清除（463 tick ≈ 8 个被拒帧）。
- 检测与恢复：置位 → 降级 0 tick；最后一类标志清除（`t=13134 clear=0x10`）→ NORMAL **101 tick**。

### 3.4 通信静默（`silent`）

- 预期：300 ms 判 `T`（无 CRC 正确帧）与 `STALE`（无有效样本）；地面站心跳同时停发，
  500 ms 后判 `GND`。三者是三条独立计时，但本段里前两者到期点相同。
- 实测：`[fault] t=13636 set=0x43`（`T`+`STALE`+`GND` **同一 tick**）、
  `[gate] t=13731 feed=OFF reason=FLAGS hb=197`、`[mode] t=16641 DEGRADED -> SAFE`
  （停留 **3005 ms**，门限 3000）。
- 恢复（两级）：`t=18925 clear=0x43` → `t=19149 SAFE -> DEGRADED`（**224 tick**）→
  `t=19310 DEGRADED -> NORMAL`（**161 tick**，含 2 帧抢占量化的下界不稳定，见 §3.1）。
- 剧本结束时再次进入 DEGRADED→SAFE：`t=39387 set=0x03`（此时 `GND` 已由上一段保持）、
  `t=42396 DEGRADED -> SAFE`（停留 **3009 ms**）：同一门限的第二次独立观测。
- 注意 `DEGRADED → SAFE`（16641）落在阻塞注入窗口 [16534, 17334] **之内**，但两者无因果：
  停留计时由 ctrl 从 13636 起推进，与 rx 是否卡住无关。见 `../README.md` §10.2。
- `set=0x43` 这个形状同时是 C16 窗口语义的来源：`GND` 与 `T`/`STALE` 落在同一 tick 时，
  "GROUND 上升沿之后、下一次链路上升沿之前"这个窗口为空，判定器主动弃权（§4 M11）。

### 3.5 丢帧（`drop`）

- 预期：连丢 3 帧的时长约 174 固件 ms（3 × 20 ms 周期，含文本间隙），短于 300 ms 超时门限，
  因此**只**应体现在序号缺口上。
- 实测：`gap` 从 `t=20169` 的 130 增到 `t=25196` 的 133，最终
  `gap=313 = 30（错帧）+ 280（静默时隙，含 `hb_only` 的 180）+ 3（丢帧）`，`dup=0`；
  `NORMAL→DEGRADED` 恰为 6 次，没有多降一次。
- 局限（重要）：这两条**不能**证明"丢帧没有推迟任何一次恢复"。注入按墙钟排期、固件按虚拟时钟走，
  落点无法对准固定固件时刻；若丢帧正好压在某次恢复窗口上，那次 `-> NORMAL` 会晚 3 帧，
  日志里与"完全没影响"分不出来。

### 3.6 只发心跳（`hb_only`）

- 预期：`SENSOR` 停发、`STATUS` 继续 → 数据层 `STALE`、链路层不置 `T`、地面站层不置 `GND`。
  这是"链路通但数据不可用"的唯一可构造形态，也是 `AV_F_TIMEOUT` 与 `AV_F_STALE` 必须分开计时的
  直接证据。
- 实测：`[mode] t=22510 NORMAL -> DEGRADED flags=0x02`——**只有 `STALE`，没有 `T`**；
  `[stat] t=25196 mode=DEGRADED flags=0x02[T=0 STALE=1 … GND=0] frames=252 gage=181`。
  `frames` 从上一段直到本段结束保持 252 不变（`t=30215` 仍是 252），而 `gage=181 ms` 说明
  心跳新鲜、`T=0` 说明链路被判为通。
- 长期持续：`[mode] t=25514 DEGRADED -> SAFE`（停留 **3004 ms**），与 `silent` 同一门限。
- 判定：C17 用"相邻两条 `[stat]` 帧数不变 + `T=0` + `GND=0` + `gage ≤ 500`"识别这一段，
  并要求其中至少一条 `STALE=1`。删除这段或不发心跳都会让它失败（§4 M10）。

### 3.7 只发数据（`hb_lost`）

- 预期：`SENSOR` 继续、`STATUS` 停发 → 500 ms 后 `GND` 置位，但 `AV_F_GROUND` 不在
  `AV_F_MODE_MASK` 里，**飞行模式不应改变**。
- 实测：`gage` 反推最后一帧心跳在 `t≈31937`（`t=35256` 时 `gage=3319`），
  `[fault] t=32438 set=0x40` 与之相差 501 ms，与 `AV_GROUND_TIMEOUT_MS = 500` 一致（推算）。
- 关键读数是恢复路径：`[mode] t=32438 SAFE -> DEGRADED flags=0x40` 与 `GROUND` 上升沿同 tick，
  随后 `[mode] t=32604 DEGRADED -> NORMAL flags=0x40`——**地面站已失联，系统仍完成两级恢复**。
  若 `GND` 参与 `fault`，`good_streak` 会被每帧清零，这一步不可能发生。
- 稳态证据：`[stat] t=35256 mode=NORMAL flags=0x40[T=0 STALE=0 … GND=1] gage=3319`——
  心跳已断 3.3 s，模式仍是 NORMAL，数据层没有任何故障位。
- 判定：C16 的两条子判据（窗口内无迁移 + 置位期间有 `stat` 处于 NORMAL）同时依赖这一段的形状。

### 3.8 任务阻塞（固件侧注入）

- 为什么只能在固件侧注入：PC 侧无法让 rx 任务停止推进；停发字节只会产生 `T`/`STALE`（§3.4 已覆盖）。
  因此用编译期开关 `AV_DEBUG_BLOCK_RX_MS`，QEMU 构建 800 ms、真板默认 0（不编译该段）。
- 落点：`AV_DEBUG_BLOCK_AT_MS = 16500`，位于静默段内（队列为空），所以不丢字节、
  不扰动解析器状态，`crc`/`gap` 算术不受影响。
- 实测：`[debug] t=16534 rx 阻塞开始 plant=800 ms` →
  `[gate] t=16949 feed=OFF reason=HB+FLAGS hb=415` →
  `[debug] t=17334 结束 dur=800` → `[gate] t=17556 feed=OFF reason=FLAGS hb=20`（`HB` 位撤销）。
- 检测延迟 **415 tick**：其中 400 是门限本身，其余是 health 的 100 ms 检查颗粒（跨运行 415–496）。
- 阻塞结束（17334）到标志全清（18925）之间隔着 1591 tick，**不是**阻塞的恢复代价：
  注入落在剧本 `silent` 段内，清零点由剧本恢复段决定。
- 已知设计约束（首版实测暴露）：若注入写成 `taskYIELD()` 忙等，rx（优先级 3）会饿死
  health（优先级 2），监督链在 800 ms 内一次都跑不到，只能事后读到陈旧证据。
  这条反例见 `../README.md` §6.6——**监督任务的优先级决定它能否"看见"忙等**。

### 3.9 下行二进制通道（`aviation-link`）

- 预期：USART2 上的 `STATUS` 帧可被独立解析，且与同一次运行的 ASCII 日志交叉一致。
- 实测：`13600` 字节解出 `544` 帧，CRC/长度/类型错误各 0、跳过字节 0；`SEQ` 0→31
  （`(544−1) mod 256 = 31`，逐帧 +1 无跳号）；`uptime` 101→54767 ms 严格递增；
  帧间隔中位 100 ms、最小 100、最大 117（设计 100）。
- 交叉核对：10 组同一 `t` 的记录，mode/flags/frames/crc/gap 全部一致（L7）。
- 判定：`link.py` 7/7 通过。判据是**不变量**，不比对绝对时刻，理由同 §1 的时间口径。
- **已知盲区**：L1–L7 不检查流的末尾是否完整。把 bin 尾部截掉 10 字节（小于一帧）仍然 7/7 通过
  （§4 N3）——因为 QEMU 被 timeout 杀掉时本来就会在帧中间截断，两者无法区分。
  尾部丢失 ≤1 帧不可检出；要检出需真板上的帧计数与地面站期望值对账。

### 3.10 时间戳年龄下溢（非剧本故障）

- 现象：`[stat] … age=4294967292`（= −4 的无符号解释），由跨任务读取时间戳的先后引起。
- 影响：`hb` 走同一条差值路径，下溢会让门控读成"心跳早断了"并误停一次喂狗。
- 修法：统一走 `av_tick.h::av_tick_elapsed`（无符号差值，不直接减）。回归判据 C15
  （`age`/`hb` 均 < 2³¹）。本轮 10 条 `[stat]` 全部在合理区间。

## 4. 自动判定与负向验证

`expect.py` 从日志抽 **24** 项不变量（C1–C17，其中 C4 按六个故障位计 6 项），不比对硬编码时刻。
`link.py` 另判下行通道的 7 项（L1–L7）。判据清单见 `tests/qemu-stm32/README.md`「机器判定」一节。

负向验证（2026-10-27，全部在**日志/二进制的临时副本**上做文本与字节变异，不重跑 QEMU、
不改仓库文件）：

| 变异 | 期望 | 实测 |
| --- | --- | --- |
| M1 `gap=313`→`314` | 只 FAIL C7 | `FAIL C7 … gap=314，期望 30 + 280 + 3 = 313`，exit=1 |
| M2 删除 `reason=HB+FLAGS` 行 | 只 FAIL C14 | `FAIL C14 … 窗口内无 HB 门控行（窗口内 0 条）`，exit=1 |
| M3 删除全部 `[debug]` 行（模拟注入关闭） | 24/24，C14 标注"不适用" | 无 FAIL，exit=0 |
| M4 某条 `age` → `4294967292` | 只 FAIL C15 | `FAIL C15 … t=25196 age=4294967292`，exit=1 |
| M5 `feed=OFF`→`ON`（该行有 T/STALE） | 只 FAIL C10 | `FAIL C10 … 1 条 stat 的 feed 与 flags/hb 矛盾`，exit=1 |
| M6 `feed=ON`→`OFF`（该行无阻塞原因） | 只 FAIL C10 | 同上，走"两条原因都不成立却停喂"分支 |
| M7 抹掉所有 `set` 掩码与 `stat` 的 `GND` 位 | FAIL C4(GND) + C16 | `FAIL C4 故障位 GND 至少置位一次`、`FAIL C16 … GROUND 从未置位`，exit=1 |
| M8 在第二个 GROUND 窗口内插一次假降级（`t=35000`） | FAIL C13 + C16 | `FAIL C13 … 7 次，期望 6 次`、`FAIL C16 … t=32438 之后（到 39387）仍有 1 次降级/进 SAFE`，exit=1 |
| M9 GROUND 置位期间的 `stat` 改报 SAFE（`cmd` 同步为 0） | 只 FAIL C16 | `FAIL C16 … 没有一条 stat 处于 NORMAL`，exit=1 |
| M10 `hb_only` 段内 `frames` 递增 | 只 FAIL C17 | `FAIL C17 … 帧数不变的相邻对 0 个`，exit=1 |
| M11 假降级插在第一个 GROUND 上升沿**同一 tick** | 只 FAIL C13，C16 弃权 | `FAIL C13 … 7 次`；C16 **PASS** |
| M12 日志路径不存在 | 退出码 2 | exit=2 |
| N1 下行 bin 改一个载荷字节 | FAIL L2 + L3 | `FAIL L2 坏帧 1 个`、`FAIL L3`，5/7，exit=1 |
| N2 下行 bin 改一个 SOF 字节 | 只 FAIL L3 | 只 L3 失败，6/7，exit=1 |
| N3 下行 bin 截断尾部 10 字节 | —— | **7/7 通过，exit=0**（判据盲区，见 §3.9） |
| N4 下行 bin 路径不存在 | 退出码 2 | exit=2 |

三条值得单独说明：

- **M11 是 C16 窗口语义的直接证据**：`>=` 让"GROUND 上升沿与链路上升沿同 tick"时窗口为空，
  判定器主动弃权而不是把 `silent` 段那次降级错算到 `GND` 头上。两轮运行恰好分别命中
  `set=0x43`（同 tick）与 `set=0x40`（不同 tick）两种形状，用 `>` 会偶发假失败。
- **M9 的第一次尝试并不干净**：只把 `mode` 改成 `SAFE` 而 `cmd` 仍是 `-50`，结果 C12
  同时失败（"SAFE 回中位"）。这说明变异本身也要做干净——否则"多失败一项"会被误读成
  "判据互相耦合"。表里记的是修正后的结果。
- **M3 保证"注入关掉"与"判定失效"在输出里可区分**：总数仍是 24，覆盖状态写在 C14 的说明里。

## 5. 延迟统计

`expect.py` 的 `[timing]` 段（纯统计，不参与判定）本轮输出：

| 延迟 | 实测 | 起点 | 性质 |
| --- | --- | --- | --- |
| 故障置位 → 降级 | 0 tick × 6 | 标志位被置上 | 不是"检测延迟"，两者常落在同一 tick |
| 故障清除 → NORMAL | 82 / 123 / 101 tick | 最后一类标志清除 | 3 个连续有效帧；下界受帧相位影响，不稳定（§3.1） |
| 故障清除 → `SAFE→DEGRADED` | 224 / 222 tick | 同上（SAFE 之后） | 5 个连续有效帧 |
| DEGRADED → SAFE | 3005 / 3004 / 3009 ms | 进入 DEGRADED | 3000 ms 门限 + 20 ms 网格 |
| 进入 SAFE → 退出 SAFE | 2508 / 6924 ms | 进入 SAFE | **不是固件判据**，由剧本恢复时刻决定 |
| `SAFE→DEGRADED` → NORMAL | 161 tick | 第一级恢复 | 3 个连续有效帧 |
| 阻塞开始 → 心跳判超时 | 415 tick | `[debug]` 行 | 400 门限 + 100 ms 采样颗粒 |

跨运行区间（引用区间、不比对单次 `t=`）：清除→NORMAL **82–163** tick；清除→`SAFE→DEGRADED`
**205–246** tick；`SAFE→DEGRADED`→NORMAL **161–185** tick；DEGRADED→SAFE **3003–3017** ms；
阻塞→心跳 **415–496** tick。

**"故障发生 → 被检出"这一段测不了**：故障发生在 PC 侧（墙钟），固件按虚拟时钟走，
两端没有共同时基，跨端对齐只能上真板 + 逻辑分析仪。本报告的所有延迟都是**固件侧**
（标志置位 / 状态迁移 / 阻塞开始）到固件侧的量。

## 6. 未覆盖与风险

| 项 | 状态 | 说明 |
| --- | --- | --- |
| CAN、ADC | **未验证** | QEMU 无模型，代码未实现；见 `../README.md` §9 |
| 示波器 / 逻辑分析仪时序 | **未验证** | 无仪器、无真板 |
| `IWDG` 计时与复位 | **构建级** | 寄存器写入路径已编入镜像（`nm`/`objdump` 可查），QEMU 中 `PR`/`RLR`/`SR` 读回 0，不计时 |
| `RCC->CSR` 复位原因 | **未验证** | 各 `*RSTF` 读回 0，复位原因不可读 |
| 故障记录掉电保持 | **未做** | 需 Flash/备份域，仿真不建模 |
| 硬件 ORE 溢出、RTOS 队列满丢弃 | **不可触发** | QEMU 用背压替代溢出，`qdrop`/`ore` 恒 0 |
| 长稳、抖动、最坏执行时间 | **未测量** | 剧本仅 14.3 s 墙钟；WCET 需真板 + 仪器 |
| 门限的精确边界（如"单帧/双帧 CRC 错不降级"） | **未单独立例** | 日志分辨率是 ctrl 的 20 ms 网格；"连续 3 帧"只有间接证据：600 ms 错帧期只产生 1 次降级、`crc=30` 全部计入 `gap` |
| 栈水位判据 | 实测（软件侧） | 100/224、104/224、88/256 字；`rx` 的 100 出现在阻塞注入构建，无注入构建为 164，差额是最长一次 `[debug]` `printf` |
| 下行帧周期稳定性 / 抖动 | **未测量** | 只有固件时间的中位 100 ms、最大 117；QEMU 串口无比特级时序 |
| 下行流尾部完整性 | **判据空白** | §3.9 / §4 N3：截断小于一帧不可检出 |
| 下行流控 / 重传 / 命令通道 | **未做** | USART2 只有 TX（`CR1 = UE\|TE`），丢帧由宿主 CRC 丢弃，见 `protocol.md` §6 |

**QEMU 特有的失真**（引用本报告数字时的前提）：时基 2.90 倍；串口无比特级时序；
GPIO/EXTI/TIM/RCC/IWDG 未建模。所有与**绝对时间**有关的结论只能在真板上复核。

## 7. 复现

```bash
cd Linux/tests/qemu-stm32
bash run.sh aviation                       # 只跑上行通道 + expect.py（覆盖 logs/aviation_node.log）
bash run.sh aviation-link                  # 额外抓 USART2 下行 + link.py，两条通道一起判定
python3 aviation/expect.py logs/aviation_node.log   # 只复跑上行判定，退出码 0/1/2
python3 aviation/link.py logs/aviation_downlink.bin --ascii logs/aviation_node.log

# 关掉固件侧注入重建镜像（C14 转为"不适用"）：
cd ../../projects/aviation-node
cmake -S . -B build-qemu -DF103_QEMU=ON -DAV_DEBUG_BLOCK_RX_MS=0 && cmake --build build-qemu
```

复跑会改变所有 `t=` 读数；判定器只判不变量，因此仍应稳定通过 **24/24**（上行）与 **7/7**（下行）。
`aviation` 与 `aviation-link` 都会覆盖 `logs/aviation_node.log`，两者的差别只在于后者多了
第二个串口后端——它会让时序轻微偏移，这正是 §2 第 3 项时段从 4.00–4.25 s 放宽到 4.30–4.55 s 的原因。
若某次运行在 C6c / C13 / C14 / C16 上失败，先看当时的主机负载（漂移会放大门限附近的读数），
再看日志本身。注意 `link.py` 依赖同目录的 `expect.py` 与 `inject.py`，不能用 `python3 -I` 运行
（隔离模式会移除脚本目录，`ModuleNotFoundError: No module named 'expect'`）。
