# F103_20：逻辑分析仪（8 通道、buffer 模式）

**目的**：把官方教材第 4 章（`hal.txt:223-262`）里"用 STM32F103 做一个逻辑分析仪"的方案
变成一个**可判定**的实现：哪些结论能在一台没有板的机器上算出来、哪些必须上真板，逐条分开。

**不做什么**：不实现 SUMP 二进制协议、不对接 PulseView、不做外部触发。
理由见 §10。

**记录性质**：主机端两个判定程序**实测**（本机跑出读数）；QEMU 里的数据链路**实测**
（`tests/qemu-stm32/logs/exp20_logicanalyzer.log`）；定时器采样路径**未验证**——
QEMU 的 TIM2 整段没有模型（§8），真板上也没有测过。

---

## 1. 环境与前置条件

| 项 | 版本 / 说明 |
| --- | --- |
| 交叉编译器 | `arm-none-eabi-gcc` 14.2.1（Arm GNU Toolchain 14.2.Rel1） |
| 构建 | CMake 4.0.3 + Ninja 1.13.2 |
| 宿主编译器 | `gcc` 13.x（只用于 `host/` 下两个程序） |
| 仿真 | `qemu-system-arm` 10.2.1（WSL），机器 `stm32vldiscovery` |
| 目标板 | STM32F103C8T6，64 KB Flash / 20 KB SRAM，HCLK 72 MHz |
| 公共设施 | `../F103_common/`（链接脚本、启动文件、`f103_tick`、`f103_uart`） |

---

## 2. 目录结构

```text
F103_20_LogicAnalyzer/
├── la_core.h / la_core.c   # 可移植核心：打包、环形缓冲、帧格式、UART 还原、容量算术
├── main.c                  # 固件：TIM2 采样 + 读 IDR + 环形缓冲 + 串口流出 + 启动探针
├── host/
│   ├── lab_la_proto.c      # 判定程序 ①：54 项断言，钉住核心逻辑（不接板）
│   └── lab_la_arith.c      # 判定程序 ②：容量/带宽/量化/中断开销六节算术（不接板）
├── CMakeLists.txt
├── build-qemu/             # F103_QEMU=ON，LA_SYNTHETIC=1
└── build-board/            # 默认，读 PA0..PA7
```

`la_core.c` 是**同一份源码**同时被固件和两个主机程序链接——这是本工程的核心设计，
详见 §4。

---

## 3. 教材第 4 章说了什么

原文要点（`hal.txt:223-262`，逐条对应行号）：

| 行 | 内容 |
| --- | --- |
| 225 | 逻辑分析仪 = 采集引脚高低电平，并按协议分析多引脚信息 |
| 228-235 | 两种方案：**buffer 模式**（FPGA 高速采集 → DRAM → 单片机上传，采样率高、时长受 DRAM 限制）与 **stream 模式**（边采边传，时长长、采样率受接口限制） |
| 239 | 本教程不用 FPGA，用 STM32F103 做 demo |
| 243-247 | 采集 = **信号电平 + 高精度时间**，"在中断函数里把引脚状态、时间记录在内存里" |
| 251-255 | 上传先串口（简单但效率低），再 USB CDC |
| 258-262 | PC 端用开源 PulseView；通信协议是 ⑧ 逻辑分析仪 SUMP 协议 |

教材给的是**方案与取舍**，没有代码，也没有任何数字。所以本工程的原创部分只能是
"把方案变成可判定的算术与协议"——这正是 `host/` 下两个程序在做的事。

---

## 4. 为什么把核心拆出来

教材这一章没有代码，于是问题不是"抄不抄得到"，而是**"我这个实现到底对不对"凭什么判定**。

QEMU 的 `stm32vldiscovery` 既不可用 GPIO 输入（`neg_f103_02_gpio_input.log` 是空日志），
也不建模 DWT（`neg_f103_09_delay_precision.log` 里 `CYCCNTENA` 读回 0），
TIM2 更是整段没有模型（§8）。也就是说，**与硬件有关的那一半在仿真里无法判定**。

所以把与硬件无关的四件事抽进 `la_core.c`：

1. 采样打包（8 根引脚 → 1 字节）
2. 环形缓冲的满时策略
3. 帧格式（SUMP 深度字段、样本行编码）
4. UART 波形还原

它们由 `host/lab_la_proto.c` 在宿主上逐条判定。固件链接的是**同一份 `.c`**，
不是"再写一遍近似实现"——这样"主机端测过的逻辑"就是"板上跑的同一份逻辑"。

---

## 5. 算术：三种模式各自的天花板

`host/lab_la_arith.c` 的输出（理论估算，不是实测；唯一例外是容量值本身，那是软件事实）：

### 5.1 buffer 模式：20 KB SRAM 换到多长的窗口

预留 4 KB 给栈与非采样变量 → 16 384 个打包样本（1 B/样本）：

| 采样率 | 窗口 |
| --- | --- |
| 10 kSa/s | 1638.4 ms |
| 100 kSa/s | 163.8 ms |
| 500 kSa/s | 32.8 ms |
| 1 MSa/s | 16.4 ms |
| 2.25 MSa/s | 7.3 ms |

**1 MSa/s 下整块 SRAM 只够 16.4 ms**——这就是教材说 FPGA 方案要外挂 DRAM 的算术依据：
16 KB 与 1 GB 差 65 536 倍，采样率不变时窗口也差 65 536 倍。

### 5.2 存储格式

同一块 SRAM：打包 1 B/样本 → 16 384 样本；直接存 `uint32_t` 的 IDR → 4 096 样本，**比值 4.00×**。
"反正 IDR 一次就读到"是省 5 行代码、丢 3/4 采样深度的典型交换。

### 5.3 stream 模式：串口把采样率压到 11.5 kSa/s

115200 bps ÷ 10 bit/B = 11 520 B/s → 1 B/样本时上限 **11 520 Sa/s**。
填满 16 384 样本只要 16 384 µs，上传要 **1 422 222 µs**（1422 ms），**上传比采集慢 86.8 倍**。

这就是教材"先串口、再 USB CDC"的顺序不是偏好而是被带宽逼出来的：USB FS bulk 常在
1 MB/s 量级，才勉强追上 1 MSa/s 的 8 通道打包流。

### 5.4 量化误差

判据：相对误差 = Ts/W ≤ 1/10 ⇒ fs ≥ 10/W。

| 脉冲宽 W | 需要 fs | 1 MSa/s 下误差 | 够不够 |
| --- | --- | --- | --- |
| 1 µs | 10 MHz | 100.0 % | 不够 |
| 10 µs | 1 MHz | 10.0 % | 够（刚好） |
| 100 µs | 100 kHz | 1.0 % | 够 |
| 1000 µs | 10 kHz | 0.1 % | 够 |

**所以 1 MSa/s 只能把 10 µs 以上的脉冲测到 10 % 误差以内**。测 1 µs 需要 10 MSa/s，F103 做不到。
这也是调 I2C/SPI 时序要看**边沿位置**而不是脉冲宽度的原因：边沿可以靠 EXTI 打时间戳，
精度不取决于采样率。

### 5.5 采样率天花板

72 MHz、一次采样中断 32 周期 → 2.25 MSa/s（100 % CPU）；40 周期 → 1.80 MSa/s。
留 50 % CPU 给主循环时保守值是 0.90 MSa/s。工程上取 **100 kSa/s** 留足余量。

抖动比平均开销更难缠：若 UART RX 中断优先级高于采样中断，每收一个字节就插队一次，
后果不是"少采几个点"，而是**时间轴不均匀**——FFT 与协议解码都会因此判错。

---

## 6. 协议与帧格式

本工程用的是**简化 ASCII 流**，明确**不是** SUMP 二进制协议，两者不能互相解析：

```text
[la] s=<idx>,<hex>          # 一个采样点：序号 + 8 通道打包值（两位十六进制）
[la] mode=isr|sw need=N got=M dropped=D isr=C
[la] decode n=K bytes=4C 41 21
[la] verdict=PASS|FAIL|NA
```

SUMP 那部分只实现了深度字段的解析（`la_sump_depth`）：`'1'..'4'` 对应 2^(8N) 个样本，
`'4'` 即 2^32 超出 `uint32_t`，**返回 `UINT32_MAX` 而不是截断伪装成可寻址**。

选择 ASCII 的理由：能在串口助手里直接看，调试成本最低。代价是体积——
一行 `s=1234,AB` 约 11 字节，比 SUMP 的位打包记录大一个数量级。这也是 §5.3 里
"上传比采集慢 87 倍"的直接原因。

---

## 7. 构建与运行

### 7.1 固件（QEMU，合成采样源）

```bash
cmake -S . -B build-qemu -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake \
  -DF103_QEMU=ON
cmake --build build-qemu
```

`F103_QEMU=ON` 时 `CMakeLists.txt` 追加 `LA_SYNTHETIC=1`：采样源改成按时间合成的
`'L','A','!'` 三个字节的 8N1 波形（每 bit 8 个采样点），通道 1..7 固定为 `0x2A`。
固定图案的用处：解码只取通道 0，但若打包把未驱动通道写成别的值，样本行里立刻能看出来。

### 7.2 固件（真板，读 PA0..PA7）

```bash
cmake -S . -B build-board -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake
cmake --build build-board
cmake --build build-board --target flash      # 需 ST-Link + OpenOCD
```

### 7.3 主机端判定程序（不接板、不链接 HAL）

```bash
gcc -std=c11 -Wall -Wextra -O2 -I. -o host/lab_la_proto host/lab_la_proto.c la_core.c
gcc -std=c11 -Wall -Wextra -O2 -I. -o host/lab_la_arith host/lab_la_arith.c la_core.c
./host/lab_la_proto        # 期望末行 PASS，rc=0
./host/lab_la_arith        # 期望末行"全部 assert 通过"，rc=0
```

这两个目标**不写进 CMakeLists**：本工程用 `arm-none-eabi` 工具链，
`add_executable` 出来的主机目标也会被交叉编译器编成 ARM 二进制，在 Windows/Linux 上跑不起来。
与 `F103_HAL_DriverLab` 的做法一致，用宿主 gcc 单独编。

### 7.4 QEMU 运行

```bash
timeout 25 qemu-system-arm -M stm32vldiscovery -display none -monitor none \
  -serial stdio -kernel build-qemu/f103_20_logicanalyzer.elf < /dev/null \
  2>&1 | tr -d '\r' | tee ../../tests/qemu-stm32/logs/exp20_logicanalyzer.log
```

---

## 8. 结果观察（实测）

### 8.1 主机端

| 程序 | 读数 |
| --- | --- |
| `lab_la_proto` | **54 项断言全部通过**，`rc=0`，末行 `PASS` |
| `lab_la_arith` | `rc=0`，六节全部 `assert` 通过，末行"全部 assert 通过" |

覆盖范围：打包位序（6 项）、环形缓冲满时策略与回卷（14 项）、SUMP 深度（6 项）、
样本行编码（7 项）、UART 还原含帧错误与截断（15 项）、容量算术含溢出与除零（7 项）。

### 8.2 固件在 QEMU 里（`logs/exp20_logicanalyzer.log`，42 行）

```text
[la] F103_20 logic analyzer, ch=8, mask=0x00FF
[la] src=synthetic (LA_SYNTHETIC=1)
[probe] TIM2 PSC=0 ARR=0 CR1=0x0000 DIER=0x0000
[probe] CNT|stuck|a=0 b=0|定时器未推进
[la] mode=sw need=256 got=256 dropped=0 isr=0
[la] decode n=3 bytes=4C 41 21
[la] verdict=PASS
[la] s=0,2B  ... s=15,2B     ← 空闲高：0x2A | 1 = 0x2B
[la] s=16,2A ... s=31,2A     ← s=16 起是起始位：0x2A | 0 = 0x2A
```

三条结论：

1. **定时器整段没有模型**：写进去的 `PSC / ARR / CR1 / DIER` **一个都没读回来**（全 0）。
   这不是"计数器不跑"，而是"寄存器不存在"——两者对判据的影响完全不同：前者可以等，后者永远等不到。
   所以探针先打印寄存器回读、再打印 `CNT` 变化，两行合起来才可判。
2. **降级路径有效**：`mode=sw`（主循环 1 ms 节拍）采满 256 个样本、`dropped=0`，
   整条"打包 → 缓冲 → 帧格式 → 解码"链路照样跑通。
3. **端到端解码正确**：`decode n=3 bytes=4C 41 21` = `'L','A','!'`，`verdict=PASS`。

### 8.3 判据自证（负向验证）

把解码器的 `spp` 从 8 改成 7（波形仍是 8 采样/bit），其余不动，重新构建运行：

```text
[la] decode n=1 bytes=0D
[la] verdict=FAIL
```

`verdict` 会翻成 FAIL，说明这一行不是常量。改回 8 后恢复 `n=3 / PASS`。

### 8.4 体积

| 构建 | FLASH | RAM | 说明 |
| --- | --- | --- | --- |
| `build-qemu` | 7 008 B / 128 KB（5.35 %） | 3 032 B / 8 KB（37.01 %） | QEMU 链接脚本是 8 KB SRAM 布局 |
| `build-board` | 6 620 B / 64 KB（10.10 %） | 3 032 B / 20 KB（14.80 %） | 真板目标 |

两者都零 warning。RAM 里 1 024 B 是两个 512 字节缓冲（`g_buf` + `g_flat`）。

---

## 9. 主机端判定抓到的两个真实缺陷

这两个都是**没有板也能抓、有板很难定位**的类型，保留在代码注释里作为判据存在价值的证据。

### 9.1 采样点漏算起始位，整帧偏移 1 bit

首版写的是 `s + spp/2 + b*spp`，正确值是 `s + spp + spp/2 + b*spp`——
忘了起始位本身也占一个 `spp`。后果：第 8 个数据位被当成停止位校验，
**任何含 0 数据位的帧都被判成帧错误**。

症状会是"有些字节能解出来、有些解不出来"，在示波器/逻辑分析仪上看起来更像信号质量问题，
而不是代码错误。`lab_la_proto.c` 的"三字节解码数量 = 3"一行直接把它抓出来。

### 9.2 解完一帧后跳到下一帧的起始位上，每段波形只解出第一帧

首版解完一帧后 `i = s + spp*10`，正好落在下一帧起始位的低电平上。
而边沿检测的前提是 `samples[i]` 为高、`samples[i+1]` 为低——`prev` 一开始就是 0，
会被"还在低电平里"的分支一路跳过。改成 `i = s + 9*spp`（停在**停止位**的高电平上）
后恢复正常。

这条特别值得记：**它是"改对一个 bug 之后才暴露的第二个 bug"**。第一个修好后，
第 1 个字节解出来了、第 2、3 个没有——如果只验"能解出一个字节"，第二个永远不会被发现。

---

## 10. 已知限制与未验证

| 项 | 状态 | 原因 |
| --- | --- | --- |
| TIM2 定时采样 | **未验证** | QEMU 整段无模型（§8.2）；真板未测 |
| 读 PA0..PA7 的真实波形 | **未验证** | 无板、无示波器、无逻辑分析仪 |
| 100 kSa/s 的实际时间精度与抖动 | **未验证** | 需真板 + 参考信号 |
| 外部触发（EXTI 启动采集） | **未实现** | 需真板才能判"触发时刻对不对" |
| 采样期间关中断 | **未实现** | 会影响串口收字节，需真板权衡 |
| SUMP 二进制协议 / PulseView 对接 | **未实现** | 本工程用 ASCII 流，两者不能互相解析 |
| UART 还原在真实引脚上的效果 | **不适用** | 100 kSa/s 采 115200 bps 时 1 bit 只有 0.87 个采样点，连采样定理都不满足；固件会打印 `decode skipped` |
| 采样率上限 1.8–2.25 MSa/s | **理论估算** | 32/40 周期是按常见 ISR 结构估的，未在真板用 DWT 实测 |

---

## 11. 常见故障

| 现象 | 原因 | 处理 |
| --- | --- | --- |
| `[probe] CNT|stuck` 且寄存器全 0 | 该机器/该芯片没有 TIM2 模型，或 RCC 时钟没开 | 先看 `RCC->APB1ENR` 的 `TIM2EN` 是否真的置位；未建模时降级为 `mode=sw` |
| `[probe] CNT|stuck` 但 PSC/ARR 读回正确 | 时钟没开或 `CEN` 没置位 | 检查 `RCC_APB1ENR_TIM2EN` 与 `CR1.CEN` |
| `decode n=0` | 波形起始不是"空闲高 → 下降沿"，或 `spp` 与实际不符 | 先确认 `dump_samples` 里有 `0x2B → 0x2A` 的跳变 |
| `decode` 出的字节成对错误 | `spp` 偏小/偏大，采样点漂出 bit 中点 | 按 §8.3 的方式做一次负向验证确认灵敏度 |
| `dropped > 0` | 缓冲小于采集需求 | 加大 `BUF_SAMPLES`，或降低采样率；不要靠覆盖旧样本来"解决" |
| 串口输出丢字符 | 采样中断与 UART 抢占 | 本工程用轮询时基（`f103_tick_init`，不占 SysTick 中断）就是为了减少这一项 |

---

## 12. 证据来源

- 教材原文：`C:/Users/yanc/AppData/Local/Temp/dshan/hal.txt:223-262`（第 4 章全文）
- 主机端读数：`./host/lab_la_proto`（54 项）、`./host/lab_la_arith`（六节算术）
- QEMU 读数：`../../tests/qemu-stm32/logs/exp20_logicanalyzer.log`（42 行）
- 构建读数：`build-qemu/`、`build-board/` 的链接阶段体积输出
- QEMU 能力边界：`../../tests/qemu-stm32/logs/neg_f103_02_gpio_input.log`（空日志）、
  `neg_f103_09_delay_precision.log`（`DWT CYCCNTENA` 读回 0）
- 讲义：`../../notes/F103_HAL入门/Day20_逻辑分析仪项目拆解.md`
