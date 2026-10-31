# Day13：定时器与 PWM

**验证状态**：教材第 10 章的学习目标、计数器/分频器/ARR/CCR 定义、SysTick 四个寄存器、
`HAL_Init/HAL_InitTick/HAL_SYSTICK_Config` 与 `SysTick_Handler/HAL_IncTick` 两条流程、
`TIM_OC_InitTypeDef` 各字段含义、SWD 与 PA15/PB3/PB4 的关系**已逐句核对教材原文**
（`ch10_timer.txt` 全文 285 行）；`Core/Src/tim.c` 三个 `MX_TIMx_Init`、`HAL_TIM_Base_MspInit`、
`HAL_TIM_PWM_MspInit`、`stm32f1xx_hal_timebase_tim.c` 的 `HAL_InitTick`、`driver_timer.c` 的
`udelay/mdelay/system_get_ns`、四个 PWM 驱动**逐行核对源码**；频率、占空比、误差、体积为
**主机端实测输出**（`lab_timer/lab_timer_calc.out`）与 `arm-none-eabi-size`/`nm` 读数。
**本工程从未上板**：没有示波器波形、没有量过 LED 亮度/电机转速/红外载波，`SWJ_CFG` 未配置
导致 PA15/PB3/PB4 仍属 JTAG 的结论是**源码 + 手册规则推论**，未在真板复现。
另发现 `uwTick` 全仓无递增者，`HAL_Delay` 的条件恒真——**源码可证，未上板复现**。

---

## 学习目标

学完本篇应能用代码证据回答：

1. `CK_PSC`、`PSC`、`CK_CNT`、`CNT`、`ARR` 五个量之间的关系是什么，为什么周期是 `ARR+1` 个计数。
2. F1 上 APB1 分频器不为 1 时，定时器时钟为什么是 `PCLK1` 的**两倍**；这条规则写在哪一行代码里。
3. 把 HAL 时基从 SysTick 换成 TIM4，除了改 `HAL_InitTick` 还要做什么——`uwTick` 由谁加。
4. 主频从 8 MHz 换到 72 MHz 时，时基的 `PSC` 为什么必须重算；不重算会快几倍。
5. `Pulse`/`ARR` 与"占空比"的关系，以及 `OCPolarity=LOW` 时同一组数值为什么能读出两个百分比。
6. 一个定时器的四个通道共用什么、不共用什么；PA8 被两个驱动同时占用会产生什么现象。
7. 定时器没输出时，按什么顺序排查（时钟 → 重映射 → 调试端口 → 极性 → 启动）。

---

## 环境与前置条件

| 项 | 要求 |
| --- | --- |
| 工程 | `experiments/F103_HAL_DriverLab/` |
| 参考 | `Core/Src/tim.c`、`Core/Src/stm32f1xx_hal_timebase_tim.c`、`Core/Src/stm32f1xx_it.c` |
| 驱动 | `Drivers/DshanMCU-F103/driver_timer.c`、`driver_color_led.c`、`driver_passive_buzzer.c`、`driver_ir_sender.c`、`driver_motor.c` |
| 主机验证 | `lab_timer/lab_timer_calc.c`（宿主 gcc，不链接 HAL） |
| 硬件 | 可选。真机验证需示波器量 PA8/PA2/PA15/PB3/PB4 |
| 前置 | Day02/04（GPIO 复用模式）、Day05（NVIC）、Day12（`mdelay` 在 Flash 驱动里的用法） |

---

## 核心概念

### 1. 定时器 = 分频器 + 计数器（教材 10.1.1）

教材把高级定时器的框图简化成两个框：**① 分频器**、**② 16 位计数器**。原话是
"每进来一个时钟（CK_CNT），里面的计数值 CNT counter 就累加，可以从 0 一直累加到 0xFFFF"。

把整条链写全，只有一条路：

```text
CK_PSC ──▶ ┌─────────┐  CK_CNT   ┌──────────┐
(72 MHz)    │  PSC    │ ────────▶ │   CNT    │ ──▶ 等于 ARR 时：回 0 + 产生更新事件
            │ 预分频器 │           │ 16 位计数 │
            └─────────┘           └──────────┘
                ▲                        ▲
        PSC = CK_PSC/CK_CNT - 1      ARR = 一个周期的计数 - 1
```

三个可以直接用的换算：

| 要什么 | 公式 | 本工程实例 |
| --- | --- | --- |
| 计数时钟 | `CK_CNT = CK_PSC / (PSC + 1)` | `72 MHz / 72 = 1 MHz`（PSC=71） |
| 周期 | `T = (ARR + 1) / CK_CNT` | `1000 / 1 MHz = 1 ms`（ARR=999） |
| 频率 | `f = CK_CNT / (ARR + 1)` | `38 994.72 Hz`（PSC=0、ARR=1894） |

**`ARR + 1` 不是笔误**。CNT 从 0 数到 ARR（含）共 `ARR+1` 个值，教材的"从 0 累加到某个数值"
已经隐含了这一点。Day12 的 `W25Q64` 页写也是同一类边界：`0..255` 是 256 个字节，不是 255 个。

教材 10.1.1 的例子"72 MHz 从 0 累加到 0xFFFF 是 65535/72 = 910 µs"，分子是 65535 还是 65536
在 910 µs 这个量级上分辨不出来（相差 0.0139 µs），但换成 1 ms 时基就分辨得出来了——见 §2。

### 2. `udelay` 里的 999 与 1000：同一个 +1 边界

`driver_timer.c:81-83`：

```c
uint32_t reload = __HAL_TIM_GET_AUTORELOAD(hHalTim);   /* = ARR = 999 */
ticks = us * reload / (1000);                          /* 假设 reload 对应 1ms */
```

TIM4 的 `CK_CNT` 是 1 MHz，`ARR = 999`，于是一个完整周期是 `999+1 = 1000` 个 tick = 1 ms。
代码用 `ARR` 而不是 `ARR+1` 去换算，`udelay(1000)` 只要 999 个 tick：

```text
udelay(1000)：写成 ticks = 1000*999/1000 = 999 个 tick
              应为 ticks = 1000*1000/1000 = 1000 个 tick
mdelay(1000) 累计：写成版 = 999000 us，应为 1000000 us
每秒少 1000 us（0.100%），跑 1 小时累计少 3.6 s
```

`mdelay` 是 `for (i=0; i<ms; i++) udelay(1000);`，所以误差按毫秒线性累积。

0.1% 在"点灯、蜂鸣器"里无所谓，但有两处会要命：

1. **Flash 的 `ERASE`/`WRITE` 等待**。Day12 里 `W25Q64_WaitReady` 用 `mdelay(1)` 做轮询间隔，
   500 次超时窗口实际只有 499.5 ms，比声明短 0.5 ms。这类"超时窗口被悄悄缩短"的偏差不会报错，
   只会在临界情况下提前放弃。
2. **拿 `mdelay` 当时钟源**。任何用 `mdelay` 拼出来的"软件定时"都会慢 0.1%，
   累积到一小时就是 3.6 秒。

顺带一个方向差异：`#if 0` 的 SysTick 版本写 `if (tnow < told)`，启用的 TIM4 版本写
`if (tnow > told)`。两者对应"向上计数"和"向下计数"两种硬件，**照抄另一端的分支就是死循环**。
现在的代码是对的（TIM4 向上计数），但这段 `#if 0` 留在文件里，是个诱导源。

### 3. APB1 分频器不为 1 时，定时器时钟要乘 2

这是 F1 系列最容易被忽略的一条：**定时器时钟不等于 APB 总线时钟**。

`stm32f1xx_hal_timebase_tim.c:57-70` 把规则写成了代码：

```c
uwAPB1Prescaler = clkconfig.APB1CLKDivider;
if (uwAPB1Prescaler == RCC_HCLK_DIV1)
  uwTimclock = HAL_RCC_GetPCLK1Freq();
else
  uwTimclock = 2UL * HAL_RCC_GetPCLK1Freq();
uwPrescalerValue = (uint32_t)((uwTimclock / 1000000U) - 1U);
```

本工程 `main.c:193` 是 `APB1CLKDivider = RCC_HCLK_DIV2`，所以：

```text
HCLK = 72 MHz
PCLK1 = 36 MHz          （APB1 = HCLK/2）
TIMxCLK = 2 × 36 = 72 MHz ← 不是 36 MHz

PSC = 72 000 000 / 1 000 000 - 1 = 71        ← 与 htim4.Init.Prescaler 一致
若误按 36 MHz 算：PSC = 35，CK_CNT = 2 MHz，一次更新 2 ms，时基慢一倍
```

所以看到 `PSC = 35` 或 `PSC = 71` 这类"刚好是 36-1 或 72-1"的数，要能反推作者当时用的是哪个时钟，
而不是记住某个魔法数。`MX_TIM2_Init` 的 `PSC = 359` 同理：`72 MHz / 360 = 200 kHz`。

### 4. HAL 时基的两条路径：SysTick 与 TIM4

教材 10.2.1 给了 SysTick 的四个寄存器和两条流程：

| 地址 | 符号 | 作用 |
| --- | --- | --- |
| `0xE000E010` | `SysTick->CTRL` | 控制与状态 |
| `0xE000E014` | `SysTick->LOAD` | 重装载值 |
| `0xE000E018` | `SysTick->VAL` | 当前值 |
| `0xE000E01C` | `SysTick->CALIB` | 校准值 |

```text
初始化：main → HAL_Init → HAL_InitTick(prio) → HAL_SYSTICK_Config(SystemCoreClock/1000)
中断  ：SysTick_Handler → HAL_IncTick() → uwTick += uwTickFreq
```

本工程走的是教材 10.2.2 的路：**用 TIM4 顶替 SysTick**。机制是弱符号覆盖：

```text
stm32f1xx_hal.c:234      __weak HAL_StatusTypeDef HAL_InitTick(...)   ← 默认：配 SysTick
stm32f1xx_hal_timebase_tim.c:42  HAL_StatusTypeDef HAL_InitTick(...)  ← 强定义：配 TIM4
```

链接器选强定义，于是 `HAL_Init()`（`hal.c:160`）和 `HAL_RCC_ClockConfig()`（`hal_rcc.c:945`）
调到的都是 TIM4 版本。`main.c` 里"为后续接管 SysTick 留出余地"说的就是这件事。

**换时基有两个责任，只做第一个会静默失败**：

| 责任 | 落点 | 本工程状态 |
| --- | --- | --- |
| ① 让定时器按 1 ms 产生更新中断 | `HAL_InitTick` 配 TIM4 → `stm32f1xx_it.c:165 TIM4_IRQHandler` | **已做** |
| ② 在更新中断里让 `uwTick` 加一 | 实现 `HAL_TIM_PeriodElapsedCallback` → `HAL_IncTick()` | **缺失** |

证据（全仓 grep）：

```text
$ grep -rn "HAL_TIM_PeriodElapsedCallback\|HAL_IncTick" --include=*.c --include=*.h .
Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal.h:295:void HAL_IncTick(void);
Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_tim.h:2070:void HAL_TIM_PeriodElapsedCallback(...);
Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal.c:293:__weak void HAL_IncTick(void)
```

三个命中全是声明或弱定义，**没有一个调用点**。`TIM4_IRQHandler` 调 `HAL_TIM_IRQHandler(&htim4)`，
后者在更新事件里调用的是 **空的弱回调** `HAL_TIM_PeriodElapsedCallback`，所以 `uwTick` 永远停在 0。

后果不是"时间慢一点"，而是三条判据全部失效：

```text
现状（uwTick 恒 0）：轮询 1000000 次仍未退出（上限 1000000）⇒ 无限循环
补上回调后：第 201000 次轮询退出，对应 201 个 tick（wait=201）

HAL_Delay(ms) 的循环条件 (HAL_GetTick() - tickstart) < wait 恒真
⇒ HAL_Delay 永久挂起
⇒ 所有用 HAL_GetTick() 的超时判据永不超时（HAL_SPI_Transmit 的 Timeout、UART 等待等）
⇒ system_get_ns() 只剩 cnt 部分：每 1 ms 从 0 重来，不是单调的系统时间
```

本仓库里 `Core/Src/lab_oled_hanzi.c:185` 就有 `HAL_Delay(200)`（`LAB_TEST=19`），
按上面的推导它会停在那一行。**这是本篇最有行动价值的一条**，修复只有四行：

```c
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM4)
    HAL_IncTick();
}
```

放在 `stm32f1xx_it.c` 的 `/* USER CODE BEGIN 1 */` 段内即可。**本篇只记录、不修改**：
没有真板，改完无法验证"`uwTick` 开始递增"，按本仓库的规矩不能记为已验证（Day12 对
W25Q64 三处缺陷也是同一处理）。

### 5. 主频变了，时基的 `PSC` 必须重算

`main.c` 的顺序是 `HAL_Init()` → `SystemClock_Config()`。第一句执行时主频还是复位值 HSI 8 MHz：

```text
HAL_Init 时：HCLK=8 MHz，APB1 分频=复位值 DIV1 → TIM4CLK=8 MHz  → PSC = 8/1 - 1 = 7
切到 72 MHz：HCLK=72 MHz，APB1=DIV2            → TIM4CLK=72 MHz → PSC = 72/1 - 1 = 71
若沿用 PSC=7：CK_CNT = 9 MHz，一次更新只花 111.1 us
⇒ 名义 1 ms 的 tick 实际 111 µs，uwTick 快 9 倍：HAL_Delay(1000) 只等 111 ms
```

这就是 `HAL_InitTick` 的注释写 "called ... at any time when clock is configured, by
`HAL_RCC_ClockConfig()`" 的原因。`HAL_RCC_ClockConfig` 在 `hal_rcc.c:945` 无条件重调了一次，
所以本工程不存在这个问题——但**如果把 `HAL_InitTick` 改成"只初始化一次"的写法，这条就立刻变成缺陷**。

顺带一条推论：既然 `HAL_InitTick` 跑两次，`HAL_TIM_Base_Start_IT(&htim4)` 也会跑两次。
HAL 的 `HAL_TIM_Base_Start_IT` 对已启动的定时器返回 `HAL_ERROR`（`State` 已是 `BUSY`），
第二次的返回值被 `HAL_InitTick` 赋给 `status`——本工程没检查 `HAL_Init` 的返回值，所以看不出来。
**不影响功能**，但它意味着"`HAL_Init` 的返回值在这里不可信"。

### 6. PWM：一个 ARR、四个 CCR、一个有效电平

教材 10.1.3 说得很清楚：一个定时器有 **1 个 ARR**（控周期）和 **4 个 CCR**（控四个引脚）。
10.1.2 又补了一句关键的："在上一小节描述 PWM 的概念时，下图的有效信号是高电平。但是在实际电路中，
可能使用低电平作为有效信号。这可以设置定时器，指定'有效信号'为高或低。"

硬件行为（PWM1 模式、向上计数）：

```text
CNT: 0   1   2  ...  CCR-1  CCR  ...  ARR  → 0
     └──── 通道"有效" ────┘└── 通道"无效" ──┘
有效时的管脚电平 = OCPolarity：HIGH → 高电平有效；LOW → 低电平有效
```

所以 `Pulse` 的准确含义是**有效电平的宽度（单位：tick）**——这正是教材 10.3.3 对
`TIM_OC_InitTypeDef.Pulse` 的注释："有效电平的宽度，单位为时钟个数"。

**教材自己有两套口径**，这是本篇最值得记住的一处：

| 出处 | 定义 | 对应量 |
| --- | --- | --- |
| 10.1.2 | `duty = T1/T`，"脉冲周期内**高电平**的时间与整个周期时间的比例" | 引脚高电平占比 |
| 10.3.3 | `Pulse` = "**有效电平**的宽度" | `CCR/(ARR+1)` |
| 驱动注释 | `driver_ir_sender.c:77` `Pulse = 473; // 75%占空比` | 引脚高电平占比 |

红外那一组是 `PSC=0, ARR=1894, Pulse=473, OCPolarity=LOW`：

```text
有效区（引脚低）= 473/1895 = 24.96%
引脚高电平     =      75.04%
```

同一组数值，按 10.1.2 的"高电平"口径是 **75%**（与驱动注释一致），按 10.3.3 的"有效电平"口径是
**25%**。两个说法都对，但**指的不是同一件事**。要回答"红外 LED 到底亮了多久"，还差一个前提：
这个模块是高电平点亮还是低电平点亮。**本仓库没有该模块的原理图**，所以这里只能给两种口径，
不能断言哪一个——`lab_timer_calc.out` 段 4 的输出就是这么写的。

另一个易错点：`Core/Src/tim.c` 三路全是 `TIM_OCPOLARITY_HIGH`，但驱动在 `Init` 里改成了 `LOW`：

| 驱动 | 极性 | 说明 |
| --- | --- | --- |
| `ColorLED_Set` | `LOW` | 注释"低电平亮灯" |
| `PassiveBuzzer_Init` | `LOW` | 注释"低电平亮灯" |
| `IRSender_Init` | `LOW` | 同上 |
| `Motor_Init` | `HIGH` | `BIN1/BIN2` 一个高一个低才转 |

**真正生效的极性由驱动决定，不是 CubeMX 那一份。** 读 `tim.c` 得出的"PA8 高电平有效"是错的。

### 7. 一个定时器被两个驱动抢用：PA8 / TIM1_CH1

`driver_passive_buzzer.c` 和 `driver_ir_sender.c` 都拿 `htim1`，都写 `PSC`/`ARR`，但参数完全不同：

| 谁 | PSC | ARR | Pulse | 极性 | 结果频率 |
| --- | --- | --- | --- | --- | --- |
| `PassiveBuzzer_Init` | 71 | 999 | 499 | LOW | 1000.00 Hz |
| `IRSender_Init` | 0 | 1894 | 473 | LOW | 37994.72 Hz |

后调用的那个覆盖前者，因为 `PSC`/`ARR` 是**同一组寄存器**。若红外在蜂鸣器之后初始化：

```text
蜂鸣器会变成 37994.72 Hz：超出人声/人耳响应范围，等于哑了
```

本工程用 `LAB_TEST = 5`（无源蜂鸣器）与 `LAB_TEST = 6`（红外发射）**编译期互斥**规避——
`main.c` 的 `Lab_Run()` 用 `#if/#elif` 只编一个。所以这不是"已修复"，而是"不可能同时出现"。
一旦有人把两个 Test 拼进同一个程序，或者把两个驱动都初始化，现象就是莫名其妙的频率。

与 Day11 的 PB9（W25Q64 的 CS 与超声波 TRIG 反相位）是同一类**资源冲突**，只是这次的冲突在
"寄存器配置层"而不是"引脚电平层"，更难看见。

### 8. 时钟使能分散在两个 MspInit 里

定时器寄存器写进去没反应，第一嫌疑是**外设时钟没开**。本工程的时钟使能分在三个函数里：

| `MX_TIMx_Init` 调了什么 | 触发哪个 MspInit | 谁开时钟 |
| --- | --- | --- |
| TIM1：`HAL_TIM_Base_Init` + `HAL_TIM_PWM_Init` | `HAL_TIM_Base_MspInit` | `tim.c:217` 开 TIM1 |
| TIM2：同上 | `HAL_TIM_Base_MspInit` | `tim.c:228` 开 TIM2 |
| TIM3：**只有** `HAL_TIM_PWM_Init` | `HAL_TIM_PWM_MspInit` | `tim.c:244` 开 TIM3 |
| `HAL_TIM_MspPostInit`（三个都调） | —— | **不开时钟**，只配引脚复用 |

为什么 TIM3 单独走另一条路？因为 CubeMX 里给 TIM3 选的模式只生成了 PWM 初始化。同一个工程里
两种生成策略并存，是 CubeMX 的正常行为，但**照抄时就会踩坑**：

```text
PassiveBuzzer_Init : 只 TIM_Base_SetConfig() -> 不触发任何 MspInit -> 时钟不开
```

`PassiveBuzzer_Init`（`driver_passive_buzzer.c:53-88`）直接调 `TIM_Base_SetConfig()`，
既没有 `HAL_TIM_Base_Init` 也没有 `HAL_TIM_PWM_Init`，所以**它自己不开 TIM1 时钟**，
能工作完全依赖 `main.c:88` 先调过 `MX_TIM1_Init()`。

单独把 `driver_passive_buzzer.c` 拷进新工程，蜂鸣器不会响，而 HAL **不会返回任何错误**——
它只是把寄存器写进了一个没有时钟的外设。与 Day11 的"CS 没配但 `HAL_SPI_*` 返回 `HAL_OK`"
是同一类失败：**外设不检查前置条件，前置条件只能由调用方保证**。

### 9. 调试端口占着 PA15/PB3/PB4，而本工程没配 `SWJ_CFG`

教材 10.3.1 的原话：

> "当 PA2、PA15、PB3 为低电平时，全彩 LED 里的红、绿、蓝就会发光。"
>
> "DshanMCU-103 使用 SWD 调试接口，可以节省出 TDI（PA15）、TDO（PB3）、TRST（PB4）三个引脚。
> 其中 PA15、PB3 用于全彩 LED，PB4 用于直流电机。**所以需要在 STM32CubeMX 里配置调试接口为 SWD，
> 否则全彩 LED、直流电机无法使用。**"

`tim.c` 里确实做了部分重映射：

```c
tim.c:295  __HAL_AFIO_REMAP_TIM2_PARTIAL_1();   /* TIM2_CH1→PA15, CH2→PB3, CH3→PA2 */
tim.c:316  __HAL_AFIO_REMAP_TIM3_PARTIAL();     /* TIM3_CH1→PB4 */
```

但**重映射不等于释放引脚**。PA15/PB3/PB4 的归属由 `AFIO_MAPR.SWJ_CFG` 决定，复位值是 `000`
= 全功能 JTAG + SWD，此时这三个引脚归调试口。要释放它们、同时保留 SWD，必须写 `010`，
即 HAL 的 `__HAL_AFIO_REMAP_SWJ_NOJTAG()`。

本工程的实际状态：

```text
$ grep -rn "REMAP_SWJ\|AFIO_MAPR\|MAPR\|DBGMCU" --include=*.c Core Drivers/DshanMCU-F103
（无输出）

$ ls *.ioc
（无 .ioc 文件）
```

**没有 `.ioc`，也没有任何 `SWJ_CFG` 写入**，所以按源码推，PA15/PB3/PB4 在真板上仍被 JTAG 占用，
TIM2 的绿/蓝通道与 TIM3 的电机通道都收不到波形。可证伪的预测是：

> `LAB_TEST=3`（全彩 LED）在真板上**只有红色（PA2）亮**，绿、蓝不亮；`LAB_TEST=10`（电机 B）不转。

这条预测同时能区分两种可能：如果三个颜色都正常，说明是别处（比如启动文件或 ST-Link 侧）
配置了 SWJ，我的推论就是错的。**上板时先做这个验证，再决定要不要加这一行。**

修复只有一行，位置在 `HAL_MspInit`（`stm32f1xx_hal_msp.c:63`）的最前面：

```c
__HAL_AFIO_REMAP_SWJ_NOJTAG();   /* 只关 JTAG，保留 SWD */
```

至于为什么本工程会缺这一项：`main.c` 的注释说"与 CubeMX 生成的 `main()` 一致"，
而 CubeMX 会按 `.ioc` 里的 SYS Debug 设置把这一行生成到 `HAL_MspInit` 里。
**本仓库没有 `.ioc`**，手工重建时漏掉它是很自然的事——这也说明"构建绿 ≠ 配置对"
（Day11 已用同样的判据证明过 CS 引脚的问题）。

### 10. `system_get_ns` 的两次读之间有 1 ms 的窗口

`driver_timer.c:141-151`：

```c
uint64_t ns = HAL_GetTick();             /* ① 先读 ms */
cnt    = __HAL_TIM_GET_COUNTER(hHalTim); /* ② 再读 CNT */
reload = __HAL_TIM_GET_AUTORELOAD(hHalTim);
ns = ns * 1000000 + cnt * 1000000 / reload;
```

①② 之间如果发生更新事件（`uwTick+1` 且 `CNT` 归 0），两个值就属于不同的毫秒：

```text
回卷前 tick=100 CNT=995；回卷后 tick=101 CNT=3（真实 101.003 ms）
代码顺序（先 tick 后 CNT）：100.003 ms，偏小 1.000 ms
反序（先 CNT 后 tick）：    101.996 ms，偏大 0.993 ms
```

两种顺序都不精确，误差量级都是**一个更新周期**。标准修法是 tick-CNT-tick 读两次比对，
前后不一致就重读：

```c
uint32_t t1, t2, cnt;
do {
    t1  = HAL_GetTick();
    cnt = __HAL_TIM_GET_COUNTER(&htim4);
    t2  = HAL_GetTick();
} while (t1 != t2);
```

还有一个次要偏差：`cnt * 1000000 / reload` 用 `ARR`（999）做分母，而 CNT 的取值范围是 `0..999`，
正确的毫秒内换算分母应是 `ARR+1`。最大偏差 0.1%，与 §2 是同一个 +1 边界。

在 §4 的缺陷没修之前，这个函数的 `HAL_GetTick()` 部分恒为 0，返回值只会在 `0..999 999 ns`
之间每毫秒重置一次——**不是单调时间**。所以"两次读竞态"是修完 `uwTick` 之后才会显形的问题，
现在它被一个更大的缺陷盖住了。

---

## 系统调用/接口卡片

### `HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *htim, uint32_t Channel)`

作用：启动指定通道的 PWM 输出（使能 CCxE 并启动计数器）。

参数：`htim` 句柄（本工程 `&htim1/&htim2/&htim3`）；`Channel` 取 `TIM_CHANNEL_1..4` 或 `TIM_CHANNEL_ALL`。

返回值和错误：`HAL_OK` / `HAL_ERROR`（句柄状态不对）/ `HAL_BUSY`（`State` 已是 `BUSY`）。

资源所有权：不改句柄之外的状态；启动后硬件持续输出，直到 `Stop` 或复位。

线程/进程安全：**不可重入**，`__HAL_LOCK` 保护；不要在中断里对同一句柄调用。

最小示例：`HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);`

常见误用：只调 `Start` 不配通道 → 输出固定电平；对 `TIM_CHANNEL_ALL` 用了未配置的通道。

### `HAL_StatusTypeDef HAL_TIM_PWM_Stop(TIM_HandleTypeDef *htim, uint32_t Channel)`

作用：关闭通道输出。**不会把引脚拉回默认电平**，只是停止驱动（引脚保持停止瞬间的状态）。

常见误用：以为 `Stop` 会"关灯"。要确定关闭状态，应把 `Pulse` 设为 0 或按极性设成无效电平，
或者干脆把引脚切成普通输出。`ColorLED_Set` 每次先 `Stop` 三个通道再 `ConfigChannel`，
就是为了避免改 `CCR` 时出现半周期的毛刺。

### `HAL_StatusTypeDef HAL_TIM_PWM_ConfigChannel(TIM_HandleTypeDef *htim, const TIM_OC_InitTypeDef *sConfig, uint32_t Channel)`

作用：写某通道的 `CCMR`/`CCER`/`CCR` —— 即模式、极性、`Pulse`。

参数：`sConfig` 至少要有 `OCMode`、`Pulse`、`OCPolarity`、`OCFastMode` 四个字段。
**周期不在这里**，在 `ARR` 里。

返回值和错误：`HAL_OK` / `HAL_ERROR`（`sConfig` 为 `NULL` 或模式非法）。

资源所有权：只写寄存器，不持有指针。

最小示例：见 `driver_ir_sender.c:71-78`。

常见误用：以为 `Pulse` 是百分比；以为改 `Pulse` 会改频率；配了 `OCNPolarity` 却没用互补输出。

### `TIM_OC_InitTypeDef` 的六个字段（教材 10.3.3）

| 字段 | 常用值 | 含义 | 本工程实况 |
| --- | --- | --- | --- |
| `OCMode` | `TIM_OCMODE_PWM1` | `CNT<CCR` 时有效 | 五个驱动全是 PWM1 |
| `Pulse` | 0..`ARR+1` | **有效电平**宽度（tick） | 可等于 `ARR+1`（见 §6，`ColorLED_Set` 最亮） |
| `OCPolarity` | `HIGH` / `LOW` | 有效 = 高 / 低 | `tim.c` 全 HIGH，驱动改 LOW |
| `OCNPolarity` | 同上 | 互补通道的有效电平 | 未使用（`MX_TIM1_Init` 配了但不开互补） |
| `OCFastMode` | `TIM_OCFAST_DISABLE` | 只在 PWM1/PWM2 有意义 | 全 DISABLE |
| `OCIdleState`/`OCNIdleState` | `RESET`/`SET` | 空闲（刹车）时的输出电平 | 全 RESET |

### `uint32_t HAL_GetTick(void)`

作用：返回 `uwTick`（毫秒计数）。

参数：无。

返回值和错误：**无错误路径**。这既是优点也是隐患——它永远"看起来正常"。
本工程因为无人调 `HAL_IncTick`，返回恒为 0（§4）。

线程/进程安全：读单个 `__IO uint32_t`，在 32 位 MCU 上是单指令，中断里读不会撕裂；
但"读两次做差"在回绕点附近会出错（`0xFFFFFFFF → 0`），要用无符号减法。

最小示例：`HAL_Delay(10)` 内部就是 `while ((HAL_GetTick() - tickstart) < 11) {}`。

常见误用：拿它做精确定时（分辨率 1 ms，且依赖中断在场）；在中断优先级高于 tick 中断的地方调用
`HAL_Delay`（永远等不到）。

### `void udelay(int us)` / `void mdelay(int ms)`（课程驱动）

作用：基于 TIM4 的 `CNT` 轮询做忙等延时，**不依赖中断**——这正是它比 `HAL_Delay` 更可靠的原因。

参数：`us` 微秒、`ms` 毫秒。

返回值和错误：无返回值；超过一个更新周期时会靠 `tcnt` 累加跨周期，但每跨一次少算 1 个 tick（§2）。

资源所有权：无；直接读寄存器。

线程/进程安全：纯读，可重入；但忙等期间 CPU 不能做别的事，中断仍能抢占。

最小示例：`mdelay(1);`（`W25Q64_WaitReady` 的轮询间隔）。

常见误用：在中断里调 `mdelay(100)` → 关掉整个系统 100 ms；以为它和 `HAL_Delay` 一样会
让出 CPU（不会）。

### `uint64_t system_get_ns(void)`（课程驱动）

作用：返回"系统时间"（纳秒），由 `HAL_GetTick()` 与 TIM4 `CNT` 拼接。

参数：无。

返回值和错误：无错误路径；但在本工程当前状态下**不是单调时间**（§10）。

常见误用：拿它做时间戳排序；在 `uwTick` 未修好时用它测间隔。

---

## 最小可运行实验

不接板，两段。

**段 A：九段算术与语义校验**（宿主 gcc，不链接 HAL）

```bash
cd experiments/F103_HAL_DriverLab/lab_timer
gcc -std=c11 -Wall -Wextra -o lab_timer_calc lab_timer_calc.c
./lab_timer_calc > lab_timer_calc.out && cat lab_timer_calc.out
```

程序里的 `assert` 是自校验：APB1 倍频后的 `PSC`、`udelay` 的 999/1000、回卷误差的**方向**
都用断言锁住。位定义或公式抄错时程序会直接 abort，而不是打印一个"看起来合理"的数。

**段 B：把定时器驱动编进固件，量代价**

```bash
cd experiments/F103_HAL_DriverLab
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake -DLAB_TEST=5
cmake --build build
arm-none-eabi-size -A build/f103_hal_driverlab.elf | grep -E "^\.text|^\.rodata|^\.data|^\.bss"
cmake -S . -B build -DLAB_TEST=0 && cmake --build build   # 复原
```

---

## 编译与运行

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake \
  -DLAB_TEST=5      # 5=无源蜂鸣器  6=红外发射  3=全彩LED  10=电机
cmake --build build
```

真板上各 Test 的行为（**以下屏幕与器件行为均未验证**）：

| `LAB_TEST` | 现象 | 看点 |
| --- | --- | --- |
| 5 | 蜂鸣器按 `PassiveBuzzer_Test` 的节奏鸣叫 | 若完全不响，先查 §9（PA8 不属 JTAG，问题在时钟或极性） |
| 6 | 红外发射（肉眼不可见） | 用手机摄像头看发射管是否发紫光 |
| 3 | 全彩 LED 变色 | 若只有红色亮，验证 §9 的推论 |
| 10 | 电机 B 转动 | PB4 属 JTAG，最可能不转 |

---

## 结果观察

**段 A 实测输出**（`lab_timer/lab_timer_calc.out`，`rc=0`，零告警）：

```text
== 1. 各定时器的实际配置（后写者覆盖先写者）==
  tim.c MX_TIM1_Init           PSC=71   ARR=999   Pulse=499   HIGH f=  1000.00 Hz  有效区= 49.90%  引脚高= 49.90%  引脚低= 50.10%
  PassiveBuzzer_Init           PSC=71   ARR=999   Pulse=499   LOW  f=  1000.00 Hz  有效区= 49.90%  引脚高= 50.10%  引脚低= 49.90%
  IRSender_Init                PSC=0    ARR=1894  Pulse=473   LOW  f= 37994.72 Hz  有效区= 24.96%  引脚高= 75.04%  引脚低= 24.96%
  tim.c MX_TIM2_Init           PSC=359  ARR=1999  Pulse=0     HIGH f=   100.00 Hz  有效区=  0.00%  引脚高=  0.00%  引脚低=100.00%
  ColorLED_Set(c=0xFF)         PSC=359  ARR=1999  Pulse=2000  LOW  f=   100.00 Hz  有效区=100.00%  引脚高=  0.00%  引脚低=100.00%
  tim.c MX_TIM3_Init           PSC=7    ARR=999    Pulse=900   HIGH f=  9000.00 Hz  有效区= 90.00%  引脚高= 90.00%  引脚低= 10.00%
  stm32f1xx_hal_timebase_tim   PSC=71   ARR=999    Pulse=0     HIGH f=  1000.00 Hz  有效区=  0.00%  引脚高=  0.00%  引脚低=100.00%

== 2. APB1 分频器不是 1 时，定时器时钟要乘 2 ==
  要得到 1 MHz 计数时钟：PSC = 72000000/1000000 - 1 = 71
  若误按 PCLK1=36MHz 算：PSC = 35，CK_CNT = 2000000 Hz，时基慢一倍

== 3. udelay 的 "reload 对应 1ms" 差一位 ==
  mdelay(1000) 累计：写成版 = 999000 us，应为 1000000 us
  每秒少 1000 us（0.100%），跑 1 小时累计少 3.6 s

== 4. 占空比的两个口径：引脚电平 vs 有效区 ==
  红外发射 38kHz CCR/(ARR+1)= 473/1895 有效区= 24.96%  极性=LOW  引脚高= 75.04% 引脚低= 24.96%

== 5. system_get_ns 跨回卷时的两次读 ==
  代码顺序（先 tick 后 CNT）：100.003 ms，偏小 1.000 ms
  反序（先 CNT 后 tick）：    101.996 ms，偏大 0.993 ms

== 6. PA8 / TIM1_CH1 被两个驱动抢用 ==
  若红外在蜂鸣器之后初始化，蜂鸣器会变成 37994.72 Hz：超出人耳响应，等于哑了

== 7. 定时器时钟使能被分散在两个 MspInit 里 ==
  PassiveBuzzer_Init  : 只 TIM_Base_SetConfig   -> 不触发任何 MspInit  -> 时钟不开

== 8. 主频变了，时基的 PSC 必须重算 ==
  若沿用 PSC=7 跑到 72 MHz：CK_CNT=9000000 Hz，一次更新只花 111.1 us

== 9. uwTick 没有递增者时，HAL_Delay 的条件恒真 ==
  现状（uwTick 恒 0）：轮询 1000000 次仍未退出（上限 1000000）⇒ 无限循环
  补上回调后：第 201000 次轮询退出，对应 201 个 tick（wait=201）
```

上面为节省篇幅删了部分重复行，完整 80 行见 `lab_timer_calc.out`。

**段 B 实测体积**（`arm-none-eabi-size -A`，`text` 为 Berkeley `size` 默认口径 = `.text + .rodata`）：

| `LAB_TEST` | `.text` | `.rodata` | `.data` | `.bss` | Berkeley `text` |
| --- | --- | --- | --- | --- | --- |
| 0（仅板载 LED） | 14 780 | 220 | 100 | 1 636 | 15 276 |
| 5（无源蜂鸣器） | 17 688 | 2 292 | 104 | 1 636 | 20 256 |
| 10（电机 B） | 18 168 | 2 292 | 104 | 1 636 | 20 736 |
| **5 − 0** | **+2 908** | **+2 072** | +4 | 0 | **+4 980** |
| **10 − 0** | **+3 388** | **+2 072** | +4 | 0 | **+5 460** |

三次构建的告警都是 **13 条**，与 `LAB_TEST=0` 完全相同（`driver_oled.c` 10 条 + `driver_ds18b20.c` 3 条），
无新增。

`.rodata` 增加的 2 072 B 基本就是字库：

```text
$ arm-none-eabi-nm -S --defined-only build_t5/f103_hal_driverlab.elf | grep ascii_font
080046e8 00000800 R ascii_font      ← 2 048 B
```

**同一个耦合又出现了一次**：`PassiveBuzzer_Test`/`Motor_Test` 用 `LCD_PrintString` 打印，
于是选哪个定时器 Test，都会把那 2 KB 字库拖进镜像。与 Day11 §结果观察里 `W25Q64_Test` 的情况
一模一样。这不是错误，但它说明**驱动文件里的 `Test()` 函数是天然的耦合源**——
真项目里测试代码应留在测试层（Day14 会把它归入"驱动库契约"）。

---

## 常见错误和陷阱

| 陷阱 | 症状 | 根因 | 处理 |
| --- | --- | --- | --- |
| 用 `PCLK1` 当定时器时钟 | 所有频率恰好慢一倍 | APB1 分频 ≠ 1 时 `TIMxCLK = 2×PCLK1` | 按 §3 的代码规则算，别背魔法数 |
| 用 `ARR` 而不是 `ARR+1` | 周期偏短 1/ARR，毫秒级看不出来 | CNT 有 `ARR+1` 个取值 | 写换算时一律 `+1` |
| 换了时基但没实现 `HAL_TIM_PeriodElapsedCallback` | `HAL_Delay` 永久挂起；所有 HAL 超时永不触发 | `uwTick` 无人递增 | 在中断里调 `HAL_IncTick()`（§4） |
| `HAL_InitTick` 只在 `HAL_Init` 里跑一次 | 时基快 9 倍 | 主频 8→72 MHz 后 `PSC` 没重算 | 交给 `HAL_RCC_ClockConfig` 重调（§5） |
| 只看 `tim.c` 的 `OCPolarity` | "我的灯为什么不亮/常亮" | 驱动在 `Init` 里改成了 `LOW` | 以驱动最后写入的值为准（§6） |
| 把 `Pulse` 当百分比 | 频率对、亮度不对 | `Pulse` 是 tick 数，分母是 `ARR+1` | `Pulse = duty×(ARR+1)`（§6） |
| 用"占空比 75%"对着 `CCR` 算 | 与注释差 50% | 高电平口径 vs 有效电平口径 | 明确口径再算（§6） |
| 两个驱动共用一个 `htim` | 频率变成另一方的值 | `PSC`/`ARR` 是同一组寄存器 | 编译期互斥或改通道（§7） |
| 驱动只调 `TIM_Base_SetConfig` | 寄存器写了、外设不动 | 没触发 `MspInit`，时钟没开 | 补 `HAL_TIM_Base_Init` 或保证 `MX_TIMx_Init` 先跑（§8） |
| 没配 `SWJ_CFG` 就用 PA15/PB3/PB4 | 红亮绿蓝不亮、电机不转 | 复位值 `000` = 全 JTAG | `__HAL_AFIO_REMAP_SWJ_NOJTAG()`（§9） |
| 先读 tick 再读 CNT | 秒表偶尔跳 1 ms | 两次读跨了更新事件 | tick-CNT-tick 重读（§10） |
| 在中断里 `mdelay()` | 系统像死机 | 忙等不让出 CPU，且屏蔽了同级中断 | 中断里只置标志 |

---

## 故障排查方法

定时器"没输出"是最常见的问题。按**代价从低到高**的顺序查，前四步都不需要示波器：

1. **外设时钟开了吗**（§8）。`grep -n "__HAL_RCC_TIM1_CLK_ENABLE" Core/Src/tim.c`，
   确认对应 `MspInit` 被触发；再看初始化路径是否真的调了 `HAL_TIM_Base_Init`/`HAL_TIM_PWM_Init`。
   时钟没开时，写寄存器不报错、读回来是 0——这本身就是判据：**写完立刻读，读回 0 就是没时钟**。
2. **重映射配了吗**（§9）。`grep -n "__HAL_AFIO_REMAP_TIM" Core/Src/tim.c`。
   TIM2 在 PA15/PB3 上必须 `PARTIAL_1`，TIM3 在 PB4 上必须 `PARTIAL`；不配就落在默认引脚上。
3. **引脚归谁**（§9）。PA13/14/15、PB3/4 默认属调试口。查有没有 `SWJ_CFG` 写入；
   `red`（PA2）能用而 `green/blue` 不能用，几乎就是这个原因。
4. **极性和 Pulse**（§6）。把 `Pulse` 设成 `ARR+1` 的 100%，按两种极性各试一次：
   一次常亮就说明通道通了，问题在占空比；两次都不亮说明前 3 步没通过。
5. **`Start` 调了吗**（接口卡片）。只 `ConfigChannel` 不 `Start`，`CCR` 写进去了但 `CCxE` 没使能。
   判据：读 `TIMx->CCER` 的 `CC1E` 位。
6. **上示波器**。量对应引脚：有方波 → 问题在负载侧（限流电阻、驱动管、模块供电）；
   无方波 → 回到第 1 步，此时才值得怀疑芯片本身。

排查顺序的原则：**先查"写进去没有"（可读回），再查"线上有没有"（要仪器）**。
第 1–5 步全部只用 `grep` 和读寄存器就能完成，把示波器留到最后。

---

## 练习题

### 基础题

题目：本工程 `TIM3` 的配置是 `PSC=7, ARR=999, Pulse=900`，`OCPolarity=HIGH`；`TIM2` 是
`PSC=359, ARR=1999`。请分别算出：

① 两者的 `CK_CNT` 与输出频率；
② `TIM3` 的引脚高电平占比；
③ `ColorLED_Set(0x00FF00)` 时，G 通道的 `Pulse` 是多少、有效区占比是多少。

提示：`CK_CNT = 72 MHz/(PSC+1)`；`f = CK_CNT/(ARR+1)`；`g = (color>>8)&0xFF`，`Pulse = g*2000/255`。
`TIM3` 的答案是 9 MHz / 9 kHz / 90%。

验收标准：四个数值都算对；能说明 `TIM2` 的 `PSC=359` 是"72 MHz → 200 kHz"而不是"36 MHz → 100 kHz"；
能指出 `g=255` 时 `Pulse=2000=ARR+1` 属于合法取值（恒有效），并说明它与 `Pulse=1999` 的波形差别。

### 改错题

题目：下面这段想"让蜂鸣器以 2 kHz、50% 占空比鸣叫"，但真板上完全没有声音。指出全部问题并修正。

```c
#include "driver_timer.h"

void Buzzer_On(void)
{
    TIM_OC_InitTypeDef cfg = { 0 };

    cfg.OCMode     = TIM_OCMODE_PWM1;
    cfg.OCPolarity = TIM_OCPOLARITY_HIGH;
    cfg.Pulse      = 50;                    /* ① */

    HAL_TIM_PWM_ConfigChannel(&htim1, &cfg, TIM_CHANNEL_1);   /* ② */
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
}
```

另附该工程的相关片段，它们各自也有问题：

```c
htim1.Init.Prescaler = 71;          /* ③ */
htim1.Init.Period    = 999;         /* ③ */
/* ④ 全工程没有任何 HAL_TIM_Base_Init(&htim1) 的调用 */
```

提示：①`Pulse` 是 tick 数不是百分比，2 kHz 需要先算 `ARR`；②没有设置 `PSC`/`ARR`，
12000... 实际用的是谁的值？③这组参数对应 1 kHz，不是 2 kHz；④时钟可能没开。

验收标准：能算出 2 kHz 的 `ARR+1`（`CK_CNT=1 MHz` ⇒ 500 tick ⇒ `ARR=499`）与 `Pulse`（50% ⇒ 250）；
能指出 ② 之前必须先让 `PSC/ARR` 生效（`HAL_TIM_Base_Init` 或 `TIM_Base_SetConfig` + 时钟已开）；
能说明 ④ 若成立则所有寄存器写入都无效，且 HAL 不报错。

### 综合题

题目：请设计一个"用 TIM2 的三个通道做呼吸灯"的方案，要求：

① 给出三路 `Pulse` 随时间变化的公式，并说明为什么用 TIM2 的 100 Hz 更新率做亮度变化会闪；
② 说明 `ColorLED_Set` 每次要 `Stop` → `ConfigChannel` → `Start` 三遍的成本，以及更省的做法；
③ 论证为什么"呼吸灯"必须改 `Pulse`，而不能改 `ARR`。

提示：①100 Hz 周期是 10 ms，逐级改 256 次需要 2.56 s，但人眼的闪烁融合阈值约 50–60 Hz——
关键在"每帧改一次"而不是"每 10 ms 改一次"；②每次 `ConfigChannel` 至少两处寄存器写，
三通道共 6 处以上；③改 `ARR` 会改变频率，三通道共用同一个 `ARR`。

验收标准：能给出 `Pulse(t) = (ARR+1) × (1-cos(2πft))/2` 或等价的线性三角波，并说明线性波在
两端看起来"停顿"的原因；能提出"只改 `CCR`（真项目用 `__HAL_TIM_SET_COMPARE`）而不重配通道"
的方案并说明它避免了 `Stop` 期间的毛刺；能解释"改 `ARR` 会让三路一起变频率，
且 `ARR` 变了以后原来的 `Pulse` 对应的占空比也全变了"。

---

## 验收标准

- [ ] 能画出 `CK_PSC → PSC → CK_CNT → CNT/ARR` 的链路，并由 `PSC/ARR` 反推频率。
- [ ] 能解释为什么周期是 `ARR+1` 个计数，并指出 `udelay` 的 0.1% 偏差来自哪里。
- [ ] 能说出 APB1 分频 ≠ 1 时定时器时钟要乘 2，并在 `HAL_InitTick` 里指出那三行代码。
- [ ] 能说清把时基换成 TIM4 需要哪两项责任，并解释缺少第二项时 `HAL_Delay` 为什么会死循环。
- [ ] 能解释主频从 8 MHz 到 72 MHz 后 `PSC` 为何要从 7 变成 71。
- [ ] 能区分"引脚高电平占比"与"有效电平宽度占比"，并说明教材 10.1.2 与 10.3.3 的差别。
- [ ] 能说明 PA8 为什么不能同时给蜂鸣器和红外用。
- [ ] 能说出 `SWJ_CFG` 与"重映射"是两件事，并给出释放 PA15/PB3/PB4 的那一行。
- [ ] 能独立跑通段 A 的九段实验，读出 999/1000、111.1 µs、1.000 ms 三个关键数字。

## 延伸阅读

- 教材：《HAL快速入门与项目实战(基于DshanMCU-103)》第 10.1（计数器/分频器/PWM 概念）、
  10.2（SysTick 与定时器高精度计时）、10.3（PWM 控制三色灯、`TIM_OC_InitTypeDef`）节
- 外设配置：`Core/Src/tim.c:32-101`（`MX_TIM1_Init`）、`:164-206`（`MX_TIM3_Init`）、
  `:208-247`（`HAL_TIM_Base_MspInit`/`HAL_TIM_PWM_MspInit`）、`:295`/`:316`（两处重映射）
- 时基：`Core/Src/stm32f1xx_hal_timebase_tim.c:42-113`（`HAL_InitTick` 的 APB1 倍频规则）、
  `Core/Src/stm32f1xx_it.c:165-176`（`TIM4_IRQHandler`）
- 驱动：`Drivers/DshanMCU-F103/driver_timer.c:47-100`（`udelay`）、`:110-113`（`mdelay`）、
  `:138-151`（`system_get_ns`）、`driver_passive_buzzer.c:53-88`/`:114-145`、
  `driver_ir_sender.c:59-79`、`driver_color_led.c:69-115`
- HAL 实现：`stm32f1xx_hal.c:160`（`HAL_Init` 调 `HAL_InitTick`）、`:293`（`HAL_IncTick` 弱定义）、
  `stm32f1xx_hal_rcc.c:945`（切频后重调时基）
- 本篇新增：`lab_timer/lab_timer_calc.c`、输出 `lab_timer/lab_timer_calc.out`
- 上一篇：`Linux/notes/F103_HAL入门/Day12_SPI_Flash编程.md`
- 下一篇：`Linux/notes/F103_HAL入门/Day14_驱动库契约与19模块总复盘.md`

---

**验证记录**

| 项目 | 内容 |
| --- | --- |
| 日期 | 2026-10-29 |
| 环境一 | Windows 11 + Git Bash；目录 `experiments/F103_HAL_DriverLab/lab_timer` |
| 命令一 | `gcc -std=c11 -Wall -Wextra -o lab_timer_calc lab_timer_calc.c`；`./lab_timer_calc > lab_timer_calc.out` |
| 结果一 | `rc=0`、零告警、80 行输出；`assert` 全部通过：APB1 倍频后 `PSC=71`（且 `8 MHz→PSC=7`）、`udelay` 999 vs 1000、`mdelay(1000)` 累计 999 000 µs（差 0.100%）、回卷误差方向（代码顺序偏小 1.000 ms / 反序偏大 0.993 ms）、`uwTick` 恒 0 时轮询 10^6 次不退出 |
| 环境二 | 同上；`arm-none-eabi-gcc` 14.2.1 + CMake 4.0.3 + Ninja 1.13.2 |
| 命令二 | `cmake -S . -B build_t{0,5,10} -DLAB_TEST={0,5,10} && cmake --build`；`arm-none-eabi-size -A`；`arm-none-eabi-nm -S`；随后 `-DLAB_TEST=0` 复原 |
| 结果二 | 三次构建均成功、告警均为 13 条（与基线相同、无新增）；`.text` 14 780 → 17 688（`LAB_TEST=5`）/18 168（`=10`），`.rodata` 220 → 2 292，其中 `ascii_font` = 0x800 = 2 048 B 由 `LCD_PrintString` 链条带入；`udelay` 0x44、`mdelay` 0x18、`system_get_ns` 0x7c、`TIM4_IRQHandler` 0x10、`HAL_InitTick` 0xa0 |
| 未覆盖 | **定时器从未上板**：无示波器波形、无亮度/转速/红外载波实测；`SWJ_CFG` 未配置 ⇒ PA15/PB3/PB4 归 JTAG 是**源码 + 手册规则推论**，未在真板复现（可证伪预测见 §9）；`uwTick` 无人递增 ⇒ `HAL_Delay` 死循环是**源码 + 有界模拟推论**，未在真板观察挂起；`HAL_InitTick` 被调两次导致 `HAL_TIM_Base_Start_IT` 第二次返回 `HAL_ERROR` 为源码推理；`IR` 的"75%/25%"两口径因缺原理图无法判定实际导通率；`system_get_ns` 的两读误差为模型计算，未用逻辑分析仪触发更新事件复现 |
