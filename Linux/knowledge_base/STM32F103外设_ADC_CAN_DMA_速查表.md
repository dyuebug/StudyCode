# STM32F103 外设速查：ADC / CAN / DMA

> 对应 `CLAUDE.md` §3 阶段 7（路线图第 3–4 周）。实验：
> [`F103_17_ADC_Polling`](../experiments/F103_17_ADC_Polling/README.md)、
> [`F103_18_CAN_Loopback`](../experiments/F103_18_CAN_Loopback/README.md)、
> [`F103_19_DMA_Mem2Mem`](../experiments/F103_19_DMA_Mem2Mem/README.md)。
>
> **证据等级**：三者的寄存器序列、位时序算术与构建产物为**实测/构建级**；
> 功能行为（读数、收发、中断）在 QEMU 里**无法验证**，全部**待真板**。

## 0. 先确认这个外设在 QEMU 里的失败形态

| 外设 | QEMU `stm32vldiscovery` | 表现 |
| --- | --- | --- |
| ADC1 | 未建模 | 所有寄存器恒读 0；"等硬件清零"的判据会**秒过**（假阳性） |
| DMA1 | 未建模 | `CCR` 读回 0，`TCIF` 永不置位 |
| CAN1 | **地址段未映射** | 第一次访问 `0x40006400` 即 `BusFault → HardFault` |

判据写法因此必须先"读回配置值"，再看行为。详见 [F103_17 README §关键实现点 1](../experiments/F103_17_ADC_Polling/README.md#1-一次假阳性以及它为什么值得记下来)。

## 1. ADC1（内部通道优先）

### 必须先算的三件事

```text
ADCCLK = PCLK2 / ADCPRE，上限 14 MHz
本工程 PCLK2 = 8 MHz，ADCPRE = 00 → ADCCLK = 4 MHz（250 ns/周期）
单次转换 = (采样周期 + 12.5) / ADCCLK
         = (239.5 + 12.5) / 4 MHz ≈ 63 µs
```

### 顺序（漏一步的后果）

| 步骤 | 漏掉会怎样 |
| --- | --- |
| `RCC->APB2ENR |= ADC1EN` | 寄存器写入无效，全读 0 |
| `CR2.ADON = 1` | 校准与转换都不进行 |
| `CR2.RSTCAL = 1` → 等清零 | 存在固定偏移 |
| `CR2.CAL = 1` → 等清零 | 同上，且偏移随温度漂 |
| `CR2.TSVREFE = 1` | ch16/ch17 **读到 0**，不报错 |
| `SQR3 = ch`（`SQR1.L = 0`） | 采到别的通道 |
| `CR2.SWSTART = 1` | `EOC` 永不置位 |
| 读 `DR & 0xFFF` | —— |

### 两个容易记错的细节

- **读 `DR` 本身就清 `EOC`**，不要手工清，否则可能丢掉"刚完成的那次"。
- 内部通道采样时间不能省（温度传感器要求 ≥17.1 µs，源阻抗高）。

### 码值 → 物理量

```c
Vdda  = Vrefint_typ * 4095 / code_vref;        /* 1.20 V 典型值，离散 ±3% */
Vsense = code_ts * Vdda / 4095;
T = 25 + (1.43f - Vsense) / 0.0043f;           /* 典型参数，误差可达数摄氏度 */
```

出厂标定：`TS_CAL1 @0x1FFFF7B8`、`VREFINT_CAL @0x1FFFF7BA`、`TS_CAL2 @0x1FFFF7C2`。
读到 `0x0000`/`0xFFFF` 说明地址或型号不对。

## 2. bxCAN（回环先通，再接总线）

### 两段状态机

```text
MCR.INRQ = 1  →  等 MSR.INAK = 1     （此时 BTR / 过滤器才可写）
… 配置 …
MCR.INRQ = 0  →  等 MSR.INAK = 0
```

**不在初始化模式下写 `BTR`/过滤器会被静默忽略**，不报错——"发出去收不到"最常见的根因。
过滤器还多一层 `FMR.FINIT = 1`。

### 位时序必须现算

```text
tq = (BRP + 1) / PCLK1
1 位 = 1(SYNC) + TS1 + TS2 个 tq
本工程 PCLK1 = 8 MHz，BRP = 0 → tq = 125 ns
TS1 = 12、TS2 = 1 → 16 tq × 125 ns = 2 µs → 500 kbps
采样点 = (1 + 13) / 16 = 87.5%
BTR = LBKM(bit30) | TS1(bit16)=12 | TS2(bit20)=1 | BRP=0 = 0x401C0000
```

照抄 36 MHz / 72 MHz 例程会差整数倍。CAN 在 **APB1**（`APB1ENR.CAN1EN`），不是 APB2。

### 过滤器四步（组为单位）

| 位 | 0 | 1 |
| --- | --- | --- |
| `FM1R.FBM0` | 一个 32 位 | 两个 16 位 |
| `FS1R.FSC0` | 掩码模式 | 标识符列表 |
| `FFA1R.FFA0` | 挂 FIFO0 | 挂 FIFO1 |
| `FA1R.FACT0` | 未激活 | 激活 |

顺序：`FMR |= FINIT` → 位宽/模式/分配 → 写 `FR1`/`FR2` → `FA1R |= FACT0` → `FMR &= ~FINIT`。
**掩码全 0 = 全通**，调试阶段先全通，把"链路是否通"与"过滤是否正确"分开。

### 发送与接收

- `TIR` **最后写**：`TXRQ` 一置位硬件就开始发送，先写数据再写 `TIR` 才能保证帧完整。
- `TSR` 是 `rc_w1`（写 1 清）；多邮箱时必须只清自己那一位。
- 收完必须 `RF0R |= RFOM0` 释放输出邮箱，否则 `FMP0` 停在 1，**下一帧永远进不来**。

### 为什么先做回环

正常模式没有收发器时收不到 ACK，默认 `NART=0` 会一直重发，`RQCP0` 长时间不来，
容易被误读成"代码有问题"。`BTR.LBKM=1` 把"配置 → 发送 → 过滤 → 接收"在单板上走完。

## 3. DMA1（内存到内存）

### 寄存器顺序

```c
/* 重装前必须关 EN，并等它读回 0（硬件在当前传输结束后才清 EN） */
DMA1_Channel1->CCR &= ~DMA_CCR_EN;
while (DMA1_Channel1->CCR & DMA_CCR_EN) { }
DMA1_Channel1->CPAR  = (uint32_t)src;
DMA1_Channel1->CMAR  = (uint32_t)dst;
DMA1_Channel1->CNDTR = words;
DMA1_Channel1->CCR   = MEM2MEM | PINC | MINC | DIR | PSIZE_1 | MSIZE_1;  /* = 0x4AD1 */
DMA1_Channel1->CCR  |= DMA_CCR_EN;
```

### 三条硬规则

1. **`CNDTR` 递减到 0 = 完成**；`EN = 1` 时 `CNDTR` 不可写。
2. **标志只能由 `IFCR` 清**，写 `ISR` 无效。忘了清会得到"假完成"：
   上一次的 `TCIF=1` 被读成本次的。
3. **mem2mem 下源端被当作"外设端"**，不置 `PINC` 就是同一个字写 N 次——完成标志照来，数据全错。

### 判据不要用 `CNDTR`

`CNDTR` 是**边读边变**的量（256 字在 8 MHz 下只要几十微秒）。判据要用不随时间变化的量：
"启动瞬间标志是否为 1"。反之，`CCR` 的读回值可以当第一判据（未建模时直接读 0）。

## 4. 通用陷阱

| 现象 | 根因 | 排查 |
| --- | --- | --- |
| 校准/标志"一次就成功" | 外设未建模/未上电，"等清零"假通过 | 加"写入后读回是否置位" |
| CAN 发得出去收不到 | 过滤器没激活或 `FINIT` 没清 | 读回 `FMR`/`FA1R` |
| 只收到第一帧 | 没置 `RFOM0` | 读 `RF0R.FMP0` 是否停在 1 |
| DMA 数据全错但标志正常 | `PINC` 没置 | 读回 `CCR` |
| 内部通道全 0 | `TSVREFE` 没置 / 采样时间太短 | 读回 `CR2`、`SMPR1` |
| 波特率差整数倍 | 照抄别的时钟树 | 按 `RCC_CFGR` 现算 `tq` |

## 5. 当前验证状态

| 外设 | 构建 | 寄存器路径 | 功能 |
| --- | --- | --- | --- |
| ADC1 | 零告警，FLASH 6408 B | `ADON` 读回 0（未建模） | **待真板** |
| CAN1 | 零告警，FLASH 7048 B | 反汇编 2 处 `0x40006400` 字面量 | **待真板**（QEMU 直接 HardFault） |
| DMA1 | 零告警，FLASH 6684 B | `CCR` 读回 0（未建模） | **待真板** |

**未验证清单**：ADC 两路 `code`、`Vdda` 反推、出厂标定可读性；CAN 的 `INAK` 时序、`BTR` 读回、
过滤匹配、回环收发比对、真实收发器波形与采样点；DMA 的 `TCIF`、`CNDTR` 归零、数据比对、
中断次数与真实耗时。本文档不把这些写成既有结果。

## 证据来源

- 三份实验 README 与 QEMU 原始日志：[`tests/qemu-stm32/logs/`](../tests/qemu-stm32/logs/)
  （`neg_f103_17_adc_polling.log`、`neg_f103_18_can_loopback.log`、`neg_f103_19_dma_mem2mem.log`）
- 能力边界：[`tests/qemu-stm32/README.md`](../tests/qemu-stm32/README.md)
- 日志：[`daily_logs/2026-10-27_第29天_日志.md`](../daily_logs/2026-10-27_第29天_日志.md)
