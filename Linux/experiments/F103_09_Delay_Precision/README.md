# F103_09：三种微秒延时实现的对比（SysTick / TIM4 / DWT）

## 学习目标

1. **计数器延时的本质**：忙等一个硬件计数器，而不是空循环计数。前者不受编译器优化影响，后者换一个 `-O` 级别就变。
2. **三种时基的取舍**：SysTick 递减 24 位、TIM4 递增 16 位、DWT 递增 32 位；分辨率、回卷窗口、是否与 HAL/RTOS 争用时基各不相同。
3. **回卷处理**：递减计数器（SysTick）与递增计数器（TIM4）的差值算法写法不同，漏掉回卷补偿会得到"偶尔延时变成 0"的怪现象。
4. **分辨率不等于精度**：8 MHz 下 1 µs 只有 8 个核心周期，函数调用加读计数器就超过它——微秒级延时的下限由开销决定。
5. **溢出意识**：课程实现里的 `us * (LOAD + 1) / 1000` 在 72 MHz 下，`us ≥ 59653` 时 32 位乘法溢出，延时静默失效。
6. **裁判不能是自己**：用 DWT 量 SysTick 和 TIM4，再用 TIM4 量 DWT，交叉验证换算链路。

行为：上电打印三个时基的配置，然后对 0 / 1 / 10 / 100 / 1000 / 10000 µs 六档、三种方法做 18 次测量并打印误差表；
之后进入探针模式，PC13 以 1 ms 低 / 1 ms 高翻转，逻辑分析仪或示波器可直接核对真实脉宽。

## 环境与前置条件

| 项 | 值 |
| --- | --- |
| 芯片 | STM32F103C8T6（Cortex-M3 r1p1） |
| 系统时钟 | HSI 8 MHz，**不配置 PLL**（`SystemCoreClock` 刷新后为 8000000） |
| 外接模块 | **不需要**。PC13 为板载 LED，可兼作示波器探针 |
| 工具链 | `arm-none-eabi-gcc 14.2.1` + CMake 4.0.3 + Ninja 1.13.2 |
| 串口 | USART1，PA9/PA10，115200 8N1 |

本实验不调用 `f103_tick_init()`（它会配置 SysTick），因此必须在初始化串口前显式调用
`SystemCoreClockUpdate()`，否则 `SystemCoreClock` 停在初值 16 MHz，UART 波特率和全部周期换算都会差一倍。

## 核心概念

### 三种时基对比

| 项 | SysTick | TIM4 | DWT CYCCNT |
| --- | --- | --- | --- |
| 计数方向 | 递减 | 递增 | 递增 |
| 位宽 | 24 位 | 16 位 | 32 位 |
| 计数时钟 | HCLK | HCLK → PSC 分频到 1 MHz | HCLK |
| 本实验回卷周期 | 1 ms（LOAD=7999） | 65.536 ms（ARR=0xFFFF） | 536.87 s（2^32/8 MHz） |
| 分辨率 | 1 个 HCLK 周期（0.125 µs） | 1 µs | 1 个 HCLK 周期（0.125 µs） |
| 是否占用 HAL 时基 | **是**（`HAL_GetTick()` 靠它） | 否 | 否 |
| 额外外设 | 无（内核自带） | 需要 TIM4 及 APB1 时钟 | 无（内核调试组件） |
| 初始化代价 | 写 LOAD/VAL/CTRL | 开时钟 + PSC/ARR/EGR/CR1 | 置 DEMCR.TRCENA + CTRL.CYCCNTENA |

结论：在本实验的 8 MHz 条件下，三者**分辨率相同或接近**，差别在"回卷窗口"和"谁拥有 SysTick"。
课程 1002 把 HAL 时基整体从 SysTick 换成 TIM4，本质是把 SysTick 让出来，而不是让延时突然变准。

### 为什么分辨率不等于精度

8 MHz 下 1 µs = 8 个周期。`delay_dwt_us(1)` 至少要做：读 `s_cycles_per_us`、乘法、读一次 `CYCCNT`、
`subs/cmp/bcc` 至少一轮——这已经十几条指令。于是请求 1 µs 实际得到 2～4 µs，相对误差超过 100%。

这不是实现缺陷，而是"延时越短、固定开销占比越大"的物理事实。真正需要亚微秒时序的场合（WS2812、DHT11）
必须改用硬件外设（定时器输出比较、SPI、PWM+DMA），软件忙等做不到。

### 递减与递增的回卷计算

```c
/* SysTick：递减，跨过 0 点时补 LOAD + 1 个周期 */
counted += (told >= tnow) ? (told - tnow) : (told + load + 1U - tnow);

/* TIM4：递增，跨过 ARR 时补 65536 */
counted += (now >= last) ? (now - last) : (0x10000U - last + now);

/* DWT：32 位无符号减法天然处理一次回卷，只要两次读取之间不满一圈 */
elapsed = (uint32_t)(DWT->CYCCNT - start);
```

## 接口卡片

### `delay_systick_us(uint32_t us)`

作用：用 SysTick 的递减计数实现微秒忙等。

原型：`static void delay_systick_us(uint32_t us);`

前置：`SysTick->CTRL` 已使能（本实验由 `systick_init_ms()` 配置，1 ms 负载、不开中断）。

资源所有权：只读 `SysTick->VAL` / `LOAD`，不改 `CTRL`，不影响 `SysTick_Handler`。

上限：目标周期数 `us × (HCLK/1e6)` 用 32 位存放，8 MHz 下约 536 s，72 MHz 下约 59.6 s。

常见误用：把 `LOAD` 改成别的值来"提高精度"——那会同时改掉 `HAL_GetTick()` 和 RTOS 的心跳。

### `delay_tim_us(uint32_t us)`

作用：用 TIM4 的 1 µs 计数实现微秒忙等。

原型：`static void delay_tim_us(uint32_t us);`

前置：`tim4_init_1mhz()` 已把 PSC 配到 `TIMxCLK/1e6 - 1`、ARR 置 0xFFFF 并启动。

返回值/错误：无；计数器异常时表现为死循环，需要靠探针或调试器确认。

资源所有权：占用 TIM4；本实验不占用其通道引脚，因此与 `F103_06` 的 TIM2 PWM 不冲突。

### `delay_dwt_us(uint32_t us)`

作用：用内核周期计数器实现微秒忙等。

原型：`static void delay_dwt_us(uint32_t us);`

前置：`CoreDebug->DEMCR |= TRCENA`，`DWT->CTRL |= CYCCNTENA`（`dwt_init()` 返回读回结果）。

返回值/错误：`dwt_init()` 返回 0 表示 `CYCCNTENA` 未置上，此时所有测量恒为 0。

常见误用：用有符号比较。`DWT->CYCCNT - start` 必须按 32 位无符号减法处理。

## 最小可运行实验

`run_benchmark()` 对每个用例记录 `DWT->CYCCNT` 起止值，打印：

```text
请求us    方法        DWT周期     实测us   误差us
```

- 裁判是 DWT，被测是 SysTick / TIM4 / DWT；
- 最后单独用 TIM4 量一次 `delay_dwt_us(1000)`，作为"裁判自己也被人量过"的交叉校验；
- `us = 0` 的行打印的是**固定开销**，不是误差。

探针模式：`PC13` 每 1 ms 翻转一次，供逻辑分析仪直接测量脉宽；每 500 次打印一行。

## 编译与烧写

```bash
cd Linux/experiments/F103_09_Delay_Precision
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake \
  -DCMAKE_MAKE_PROGRAM="<你的 ninja 路径>"
cmake --build build
cmake --build build --target flash
```

## 预期现象

下列数值**按指令周期与配置推算，不是实测结果**：

```text
[F103_09] HCLK = 8000000 Hz，1 us = 8 cycles
SysTick : LOAD = 7999（1 ms），CTRL = 0x5
TIM4    : PSC = 7，ARR = 65535（1 MHz，65.536 ms 回卷）
DWT     : CYCCNTENA 读回 = 1

请求us    方法        DWT周期     实测us   误差us
----------------------------------------------------
     0  SysTick          ...       1.x     +1.x
...
交叉校验：delay_dwt_us(1000) 由 TIM4 裁判测得 1000 us
```

各档的**预期量级**（不是精确值）：

| 请求 | 预期实测 | 相对误差量级 | 说明 |
| --- | --- | --- | --- |
| 0 µs | 1～2 µs | — | 函数调用 + 读计数器的固定开销 |
| 1 µs | 2～4 µs | +100% 以上 | 目标小于开销，无实际意义 |
| 10 µs | 10.5～12 µs | +5%～20% | 开销仍可见 |
| 100 µs | 101～102 µs | 1%～2% | 开始收敛 |
| 1000 µs | 1001～1002 µs | ≤0.2% | — |
| 10000 µs | 10000～10002 µs | <0.1% | — |

三种方法在同一档上的差异应只有几个周期——它们数的是同一个 HCLK。若某一列明显偏大，
优先检查该方法的初始化（例如 TIM4 的 PSC 算错、DWT 未使能）。

探针模式预期每约 1 s 打印一行：

```text
[probe] PC13 已翻转 500 次（理论 1000 ms）  DWT = ...
```

## 实测记录

**构建实测**（2026-10-26，arm-none-eabi-gcc 14.2.1 + CMake 4.0.3 + Ninja 1.13.2，Windows 11）：

```text
Memory region         Used Size  Region Size  %age Used
           FLASH:        6536 B        64 KB      9.97%
             RAM:        1984 B        20 KB      9.69%
   text    data     bss     dec     hex filename
   6440      96    1888    8424    20e8 f103_09_delay_precision.elf
```

`.bin` 实测 6536 B。零编译告警、零链接告警。RAM 的 1984 B 中 1536 B 是链接脚本
`._user_heap_stack` 的预留哨兵（512 B 堆 + 1 KB 栈，仅用于链接期溢出检查），
312 B 是 newlib 的 `__sf`，其余是 `__sglue`、`_impure_data` 与本实验的少量全局变量。

**寄存器与指令核对**（反汇编，非推测）：

| 意图 | 生成指令 | 结论 |
| --- | --- | --- |
| SysTick 1 ms 负载 | `umull` + `lsrs #6`（除以 1000）→ `subs #1` → `str [0xE000E014]` | LOAD = HCLK/1000−1 ✓ |
| SysTick 清 VAL | `movs #0` → `str [0xE000E018]` | ✓ |
| SysTick 使能 | `movs #5` → `str [0xE000E010]` | CLKSOURCE\|ENABLE，无 TICKINT ✓ |
| SysTick 回卷补偿 | `add r3,r4`(LOAD) → `subs r3,r2` → `adds #1` | `told + LOAD + 1 − tnow` ✓ |
| TIM4 时钟 | `ldr [0x4002101C]` → `orr #4` → 写回 | APB1ENR.TIM4EN ✓ |
| TIM4 预分频 | 载入 cycles/us → `subs #1` → `uxth` → `str [0x40000828]` | PSC=7 ✓ |
| TIM4 周期 | `movw #65535` → `str [0x4000082C]` | ARR=0xFFFF ✓ |
| TIM4 启动 | `movs #1`→EGR；`str 0`→SR；`str 1`→CR1 | UG → 清 UIF → CEN ✓ |
| DWT 使能 | `orr #0x01000000` → `str [0xE000EDFC]` | DEMCR.TRCENA ✓ |
| DWT 清零/使能 | `str 0` → `[0xE0001004]`；`orr #1` → `[0xE0001000]` | CYCCNT / CYCCNTENA ✓ |
| DWT 延时 | 读 `[0xE0001004]` → `subs` → `cmp` → `bcc` | 32 位无符号差值 ✓ |
| 探针引脚 | `bic #0x00F00000` + `orr #0x00200000` → `[0x40011004]` | PC13 推挽 2 MHz ✓ |
| 探针置高 | `mov #0x2000` → `str [0x40011010]` | BSRR.BS13（LED 灭） ✓ |
| 浮点库 | `__aeabi_f*` / `__aeabi_d*` 符号数为 0 | 误差用定点打印，未引入软浮点 ✓ |

**未验证**：上表所有"实测 us"与误差、探针脉宽、DWT 在**不接调试器**时是否仍然计数。
这些必须烧到板子上、并用逻辑分析仪量 PC13 才能确认，README 不把它们当成既有结果。

## 关键实现点

### 1. 为什么还要 DWT

SysTick 已被 HAL/RTOS 占用，TIM 又要吃一个外设和中断向量。DWT 是 Cortex-M3 调试组件里的周期计数器，
不占外设、不动 NVIC、回卷窗口 536 s，最适合做"遥控器级别"的一次性延时。

代价是它属于调试子系统：必须先置 `DEMCR.TRCENA`，再置 `DWT.CTRL.CYCCNTENA`；
`dwt_init()` 返回读回值，就是为了不把"没使能成功"误当成"延时为 0"。

### 2. TIM4 的 PSC 依赖 APB1 预分频

F1 的定时器时钟规则：APB1 预分频系数为 1 时 `TIMxCLK = PCLK1`，不为 1 时是 `PCLK1 × 2`。
本工程不碰 `RCC_CFGR`（复位值 PPRE1=000），所以 `TIM4CLK = HCLK = 8 MHz`，PSC = 7 得到 1 MHz。

若照抄"PSC = 71 就是 1 MHz"的写法（那是 72 MHz 的结论），在本工程里计数频率会变成 111 kHz，
所有延时拉长 9 倍。配置前先确认 `RCC_CFGR.PPRE1`，比记住某个数字可靠。

### 3. `EGR = UG` 之后再清 `SR`

写 `PSC`/`ARR` 只进入预装载寄存器，要等下一次更新事件才生效。`UG` 强制立即装载，
代价是同时置 `UIF`。本实验不使能 TIM4 中断，但把 `UIF` 清掉，避免以后有人加中断时被幽灵标志打扰。

### 4. 中断对所有三种方法一视同仁

三者都是"等到计数够了"，中断进来只会让**实际耗时变长**，不会变短。所以它们保证的是"至少 N 微秒"，
不是"正好 N 微秒"。需要抖动数据时，要关中断测量或统计分布——本实验未做。

### 5. 课程的三个问题

| 位置 | 问题 | 本实验的做法 |
| --- | --- | --- |
| `driver_timer.c` 的 `ticks = us*(load+1)/1000` | 72 MHz 下 `load+1 = 72000`，`us ≥ 59653` 时 32 位乘法溢出，延时静默变成接近 0 | 改成 `us × (HCLK/1e6)`，量纲相同、8 MHz 下上限约 536 s |
| `mdelay(ms)` = 循环 `ms` 次 `udelay(1000)` | 每次都要重新进入延时函数并重新计算目标周期，固定开销累积 `ms` 次；`mdelay(0)` 与"不延时"无法区分 | 本实验不做毫秒封装；需要毫秒时直接 `delay_*_us(ms * 1000)`，注意 32 位上限 |
| `system_get_ns()` 的 TIM 分支 | `cnt = tnow` 隐含三个假设：计时器与 `HAL_GetTick()` 同源、`ARR+1` 恰好 1 ms、两次读取原子。第三条不成立，边界处会读出约 1 ms 误差甚至回退 | 本实验不实现该函数；需要绝对时标时，用 DWT 读 64 位扩展或加临界区 |

另外 1002 工程把 HAL 时基从 SysTick 换成了 TIM4（`stm32f1xx_hal_timebase_tim.c`）。
这是"让出 SysTick"的做法，代价是 `HAL_InitTick()` 与中断优先级都要跟着改；
本实验保留 SysTick 做 HAL 时基、另开 TIM4 做延时源，两种资源互不干扰。

### 6. 探针放在 PC13 的理由

PC13 是板载 LED 引脚，`F103_01` 用过同一引脚与同一套 CRH 配置（构建与反汇编核对，未上板验证）。
低电平点亮——用逻辑分析仪测脉宽时，人眼也能看到 LED 是否在以约 500 Hz 闪。
缺点是 LED 负载会让边沿稍缓，亚微秒级测量应改用空载引脚。

## 常见错误和陷阱

| 现象 | 可能原因 | 排查 |
| --- | --- | --- |
| 所有"实测 us" 都是 0 | DWT 未使能（`CYCCNTENA` 读回 0） | 打印 `dwt_init()` 返回值；检查 `DEMCR.TRCENA` |
| 延时整体慢 2 倍 | `SystemCoreClock` 未刷新（停在 16 MHz） | 初始化顺序：先 `SystemCoreClockUpdate()` 再算周期/波特率 |
| 延时慢约 9 倍 | TIM4 的 PSC 照抄了 72 MHz 的参数 | 确认 `RCC_CFGR.PPRE1`，重算 `TIMxCLK/1e6 - 1` |
| 延时偶尔变成接近 0 | 回卷补偿写错，或用了有符号比较 | 检查递减分支的 `+LOAD+1`、DWT 的 `uint32_t` 差值 |
| 1 µs 档误差超过 100% | 固定开销大于目标 | 正常现象；亚微秒需求改硬件外设 |
| 串口乱码 | `SystemCoreClockUpdate()` 在 `f103_uart_init()` 之后调用 | 调整顺序 |
| 探针没有波形 | `probe_init()` 漏开 `IOPCEN`，或 PC13 被别的实验占用 | 读回 `GPIOC->CRH` 的 bit[23:20] |

## 故障排查方法

1. **先看配置打印**：`HCLK`、`SysTick->LOAD`、`TIM4->PSC/ARR`、DWT 读回值四行就能排除大半问题。
2. **看 `us = 0` 那一行**：它是固定开销的基线。若它已经是几十微秒，说明有中断在抢 CPU。
3. **交叉校验不通过**（TIM4 量 DWT 得到 2000 µs 之类）：说明 DWT 与 TIM4 的换算差了一倍，先查 HCLK 和 PSC。
4. **用探针而非串口下结论**：串口打印本身耗时数百微秒，无法反映微秒级时序；PC13 脉宽才是证据。
5. **关闭中断对照**：在测量循环外包 `__disable_irq()` / `__enable_irq()`，若误差立即消失，就是中断抖动。

## 练习题

1. **基础题**：把 `delay_systick_us` 改成用 `LOAD = 0xFFFFFF` 的自由运行模式（每 2.097 s 回卷），
   要求 `delay_systick_us(3000000)` 仍然正确。提示：目标周期数要用 `uint64_t`。
2. **改错题**：下面的实现为什么在 72 MHz 下对 `delay_us(60000)` 失效？
   ```c
   uint32_t ticks = us * (SysTick->LOAD + 1U) / 1000U;
   ```
   请给出两种修法，并说明各自的溢出边界。
3. **综合题**：用 TIM2 的输出比较通道驱动 PC13，做出 1 Hz / 50% 占空比、完全不占 CPU 的闪烁，
   再用逻辑分析仪对比它与 `delay_dwt_us` 版本在中断干扰下的抖动差异。
4. **思考题**：如果系统里同时有 FreeRTOS（SysTick 归 RTOS）、HAL（`HAL_GetTick`）和一个需要
   ±0.5 µs 时序的外设，三种时基分别该由谁使用？为什么不能同时抢占 SysTick？

验收标准：能说清三者的回卷窗口与开销量级；能解释"分辨率 ≠ 精度"；能在探针上量到与串口打印一致的脉宽。

## 验收标准

- [ ] 能说明三种时基的位宽、时钟来源与回卷周期。
- [ ] 能解释为什么 1 µs 延时误差最大、1000 µs 以上误差收敛。
- [ ] 能复述课程实现的溢出条件与修法。
- [ ] 能读懂 `DWT->CYCCNT - start` 为什么必须是无符号减法。
- [ ] 能用逻辑分析仪核对 PC13 的 1 ms 脉宽。

## 延伸阅读

- RM0008《STM32F10xxx 参考手册》：SysTick 章节（24 位递减、LOAD/VAL/CALIB）、TIMx 章节（PSC/ARR/EGR）。
- 《ARMv7-M Architecture Reference Manual》：DWT 章节（CYCCNT、DEMCR.TRCENA）。
- 课程源码 `Linux_WEIDS/DShanMCU-F103/.../[10]_定时器/1001_systick_high_precision`、`1002_timer_high_precision`。
- RT-Thread 的 `udelay()` 实现（课程的 SysTick 版本即由此而来）。

## 下一步

1. 烧到板上，把真实周期数与误差补进本文档，并用逻辑分析仪确认 PC13 脉宽。
2. 做中断抖动统计：在 1 ms SysTick 中断下重复测量 1000 次，打印最小/最大/均值。
3. 把三种延时抽到 `F103_common`，供后续 DHT11 / WS2812 一类严时序外设复用。
4. 进入 FreeRTOS 阶段：`vTaskDelay()` 与忙等延时的取舍、tick 精度、以及为什么 RTOS 里不应长时间忙等。

## 已知限制

- 只在 HSI 8 MHz 下设计；未验证 72 MHz（PLL）下的误差与溢出边界。
- **QEMU 无法验证本实验**：`-M stm32vldiscovery` 未建模 TIM4 与 DWT，`CNT`/`CYCCNT` 恒 0。
  实测表现为 `DWT CYCCNTENA 读回 = 0`、`TIM4 PSC = 0，ARR = 0`，打印到 TIM4 第一档就挂在
  `delay_tim_us` 的死循环里（见 `Linux/tests/qemu-stm32/RESULT.md` §4）。这条负结果同时也说明
  `dwt_init()` 的读回自检是有用的：它没有把"未使能"伪装成"延时为 0"。
- 未使用中断，因此表中的数值不含中断抖动。
- DWT 是否在"不接调试器"时仍计数，未实测。
- 未实现 64 位绝对时标、未做多次测量的统计分布。
- 探针只用了 PC13 一个引脚，未做多通道对比。
