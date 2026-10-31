# Day16：声音与执行器（有源蜂鸣器 / 无源蜂鸣器 / 直流电机）

**验证状态**：教材 §11.6（有源蜂鸣器）、§11.7（无源蜂鸣器）、§11.19（直流电机）
**已逐节核对原文**（`ch11_drivers.txt:198-358`、`:1717-1985`）；
三个驱动的引脚、极性、时钟使能、PWM 换算、模式切换**逐行源码核对**；
`MX_TIM1_Init` 全文与 `HAL_TIM_MspPostInit` **源码核对**（`Core/Src/tim.c:32-99`、`:250-312`）；
构建**实测**（GCC 14.2.1，0 error / 13 warning，全部来自课程驱动，`logs/build_2026-10-29.log`）。
**全部为上电前的静态证据**：蜂鸣器是否出声、音调是否准确、电机转向与转速
**一个都没观察过**（本机无模块、无 ST-Link）。本篇含**一条被自己推翻的假设**（§6）
与**三条对 Day15 的修正**（§17）。

---

## 学习目标

学完本篇应能用代码证据回答：

1. 有源蜂鸣器与无源蜂鸣器的电路差在哪？"谁产生振荡"这句话决定了驱动代码的什么差别？
2. 三个模块（有源蜂鸣器、无源蜂鸣器、红外发射）共用 PA8。它们"不能同时用"的**具体机制**是什么？
3. `ActiveBuzzer_Init` 与 `Led_Init` 有两处不一致，分别是哪两处？各自后果是什么？
4. `PassiveBuzzer_Init` 为什么用 `TIM_Base_SetConfig` 而不是 `HAL_TIM_Base_Init`？它跳过了什么？
5. 若把 `driver_passive_buzzer.c` 移植到没有 `MX_TIM1_Init` 的工程，会怎样？为什么？
6. TIM1 是高级定时器。`PassiveBuzzer_Control(0)` 让蜂鸣器不响的**硬件路径**是哪几个位？
7. `Period = 1000000 / freq - 1` 的整数除法，在哪个频率区间误差最大？最坏误差是多少音分？
8. `PassiveBuzzer_Set_Freq_Duty(0, 50)` 会发生什么？驱动有参数校验吗？
9. `Pulse = Period * duty / 100` 与 ColorLED 的 `Pulse = c * (Period+1) / 255`，哪个对？
10. DRV8833 真值表的四种状态里，哪两种能调速、哪两种只能定性？
11. 电机 B 反转为什么是 `Pulse = (99 - speed) * 10` 而不是 `speed * 10`？
12. `Motor_SetSpeed(ch, 0)` 实现的是"惯性滑行"还是"制动"？依据哪个真值表行？
13. 教材那句"PWM1 当极性为低，CCR<CNT 输出低电平"对吗？在哪三处被原样复制？

---

## 环境与前置条件

| 项 | 版本 / 说明 |
| --- | --- |
| 教材 | §11.6（`ch11_drivers.txt:198-223`）、§11.7（`:224-358`）、§11.19（`:1717-1985`） |
| 工程 | `experiments/F103_HAL_DriverLab/` |
| 驱动 | `driver_active_buzzer.{c,h}`、`driver_passive_buzzer.{c,h}`、`driver_motor.{c,h}` |
| 平台配置 | `Core/Src/tim.c:32`（`MX_TIM1_Init`）、`:164`（`MX_TIM3_Init`）、`:208`（`HAL_TIM_Base_MspInit`）、`:250`（`HAL_TIM_MspPostInit`） |
| 关键参数 | TIM1：PSC=71 / ARR=999 ⇒ 1 MHz 计数 ⇒ 1 kHz@50%；TIM3：PSC=7 / ARR=999 ⇒ 9 kHz |
| 前置 | `Day02`（GPIO）、`Day04`（工作模式）、`Day13`（定时器与 PWM）、`Day14`（契约与冲突表）、`Day15`（显示与交互） |

---

## 核心概念

### 1. 有源与无源：差别只在"振荡器在哪"

教材两节的开篇各一句话就把区别说清了（`ch11_drivers.txt:203`、`:240`）：

| | 原文 | 含义 |
| --- | --- | --- |
| 有源 | "**只要让 PA8 输出低电平**即可让它发出声音" | 模块内部自带振荡器，给直流就响 |
| 无源 | "**只要让 PA8 输出 PWM 波形**即可让它发出声音" | 内部只有线圈和振膜，振荡由外部提供 |

所以"有源/无源"不是"有没有电源"，**是"有没有振荡源"**。
这条差别直接决定了驱动代码的形态：

| | 有源 | 无源 |
| --- | --- | --- |
| 引脚模式 | `GPIO_MODE_OUTPUT_PP`（推挽输出） | `GPIO_MODE_AF_PP`（复用推挽，接定时器） |
| 控制量 | 一个电平（响 / 不响） | 频率（音调）+ 占空比（响度）+ 启停 |
| 可调音调 | **不能** | **能** |
| 驱动函数数 | 2 个（Init / Control） | 4 个（Init / Control / Set_Freq_Duty / Test） |

关键推论：**有源蜂鸣器占着一个定时器通道是浪费的**——它只需要一根 GPIO。
而无源蜂鸣器不占定时器就发不出声。这一点在引脚规划上有直接后果（见 §2）。

### 2. PA8 的三方争用：不是"插不上"，是"后初始化者改写前者"

速查表 §2 记了 PA8 被三个模块共用（有源蜂鸣器 / 无源蜂鸣器 / 红外发射）。本篇给出**机制**：

三者的 `Init` 函数对同一根引脚做了**互斥的模式设置**：

| 模块 | 调用的配置 | 结果 |
| --- | --- | --- |
| `ActiveBuzzer_Init` | `HAL_GPIO_Init(GPIOA, Mode=GPIO_MODE_OUTPUT_PP)` | PA8 由 **ODR** 驱动 |
| `PassiveBuzzer_Init` | `HAL_GPIO_Init(GPIOA, Mode=GPIO_MODE_AF_PP)` | PA8 由 **TIM1_CH1** 驱动 |
| `MX_TIM1_Init` → `HAL_TIM_MspPostInit` | 同样 `AF_PP`（`tim.c:263-266`） | 同上 |

STM32 的引脚在同一时刻**只有一个输入源**：要么是 GPIO 的 ODR，要么是 AF 输出。`CRL/CRH` 的
`MODE`/`CNF` 位决定归谁。**所以"三个模块共用 PA8"的真实语义是：最后一个执行 `HAL_GPIO_Init` 的把前一个踢掉。**

这解释了教材 §11.6.2 那段话（`ch11_drivers.txt:206-212`）：

> 有源蜂鸣器使用 PA8，把它配置为推挽输出引脚即可。但是无源蜂鸣器也使用 PA8，需要把它配置为
> TIM1_CH1 引脚。我们的程序可以既使用有源蜂鸣器，也使用无源蜂鸣器。所以需要使用代码来配置 PA8。

**"可以既使用……也使用"的正确读法是"两个驱动都编进固件、但运行时只让一个 Init 跑"**，
不是"同时工作"。教材这句话容易被读成后者。

一个额外的、教材没说的细节：`MX_TIM1_Init` 在 `main.c:88` 被无条件调用，
而它内部 `HAL_TIM_MspPostInit(&htim1)` 已经把 PA8 设成了 `AF_PP`。**所以在 `main()` 返回之后、
`StartDefaultTask` 之前，PA8 是 TIM1 通道**。有源蜂鸣器要工作，`ActiveBuzzer_Init` 必须再把它掰回 GPIO——
这正是教材那句"需要使用代码来配置 PA8"的实际必要性。

### 3. `ActiveBuzzer_Init` 少了 `Led_Init` 的两样东西

并排看两个初始化，差异都在"该做的防护省略了"这一侧：

| | `Led_Init`（PC13） | `ActiveBuzzer_Init`（PA8） |
| --- | --- | --- |
| 开 GPIO 时钟 | 有 `__HAL_RCC_GPIOC_CLK_ENABLE()` | **没有** |
| 先写 ODR 再切模式 | 有 `WritePin(RESET)` 在 `HAL_GPIO_Init` **之前** | **没有**（先 `HAL_GPIO_Init`） |
| 模式 | `OUTPUT_PP` | `OUTPUT_PP` |
| 返回类型 | `int`（恒 0） | `void` |

两处省略的后果不同：

- **不开时钟**：依赖别人开 GPIOA。本工程里 `MX_GPIO_Init` / `MX_TIM1_Init` / `lab_pins.c` 都开过
  （`Core/Src/gpio.c`、`tim.c:259`、`lab_pins.c:54`），所以不暴露。但移植到干净工程会**静默无效**。
- **不预置 ODR**：`HAL_GPIO_Init` 切到输出模式的瞬间，引脚输出 ODR 的**当前值**。
  PA8 此前被 `MX_TIM1_Init` 用成 AF 输出，ODR 位没被写过，复位值是 0 ⇒ **切过去就是低电平**。
  低电平对有源蜂鸣器 = **响**。因此 `ActiveBuzzer_Init()` 执行完的瞬间，蜂鸣器会**先响一下**，
  直到 `ActiveBuzzer_Control(0)` 把它写成高。

  这是**可证伪的预测**：`LAB_TEST=1`（有源蜂鸣器）时，上电后应先听到一声短促的"嘀"，
  然后才进入 1 秒响 / 1 秒停的节奏。若确实听到，说明 ODR 预置这一课在本模块上真实存在。

### 4. `PassiveBuzzer_Init` 用 `TIM_Base_SetConfig`：跳过 MspInit 与状态机

对比同一文件的另一个函数，差异很醒目：

```c
/* PassiveBuzzer_Init（首次初始化） */
//HAL_TIM_Base_DeInit(g_HPWM_PassiveBuzzer);      ← 注释掉
g_HPWM_PassiveBuzzer->Init.Period = 999;
//HAL_TIM_Base_Init(g_HPWM_PassiveBuzzer);        ← 注释掉
TIM_Base_SetConfig(g_HPWM_PassiveBuzzer->Instance, &g_HPWM_PassiveBuzzer->Init);   ← 直接写寄存器

/* PassiveBuzzer_Set_Freq_Duty（运行期改频） */
HAL_TIM_Base_DeInit(g_HPWM_PassiveBuzzer);        ← 启用
...
HAL_TIM_Base_Init(g_HPWM_PassiveBuzzer);          ← 启用
```

`HAL_TIM_Base_Init` 做三件事（`stm32f1xx_hal_tim.c`）：

```c
if (htim->State == HAL_TIM_STATE_RESET) { HAL_TIM_Base_MspInit(htim); }  /* ← ① 开时钟 */
htim->State = HAL_TIM_STATE_BUSY;
TIM_Base_SetConfig(htim->Instance, &htim->Init);                        /* ← ② 写 PSC/ARR/CR1 */
htim->State = HAL_TIM_STATE_READY;
```

`TIM_Base_SetConfig` 单独调用 = **只有 ②**，没有 ① 也没有状态机。
所以 `PassiveBuzzer_Init` 的实际前提是：

> **TIM1 的时钟已经开了、`htim1.State` 已经是 READY。**

本工程满足这一点，因为 `main.c:88` 的 `MX_TIM1_Init()` 走完整路径：
`HAL_TIM_Base_Init` → `HAL_TIM_Base_MspInit` → `__HAL_RCC_TIM1_CLK_ENABLE()`（`tim.c:217`）。

顺带一个冗余事实：`MX_TIM1_Init` 里设的 `Prescaler = 71`、`Period = 999`、`Pulse = 499`
（`tim.c:48/50/74`）与 `PassiveBuzzer_Init` 里设的**三个值完全相同**。
也就是说 `PassiveBuzzer_Init` 的这段赋值在本工程里是**纯冗余**——CubeMX 已经按蜂鸣器配好了。

### 5. `freq` 的整数除法：最坏 17 音分，且 `freq=0` 会除零

`PassiveBuzzer_Set_Freq_Duty` 的核心两行：

```c
g_HPWM_PassiveBuzzer->Init.Period = 1000000 / freq - 1;
sConfig.Pulse = (1000000 / freq - 1) * duty / 100;
```

PSC = 71 ⇒ 72 MHz / 72 = **1 MHz 计数**，1 tick = 1 µs。
所以 `1000000 / freq` 个 tick = `1/freq` 秒，公式的量纲是对的。问题在两处：

**① `1000000 / freq` 是整数除法，向下取整。** 实际输出频率是 `1000000 / N`（N = 取整后的商），
不是 `freq`。误差量化：

| 目标 freq | N = 1000000/freq | 实际输出 | 绝对误差 | 音分误差 |
| --- | --- | --- | --- | --- |
| 100 Hz | 10000 | 100.0 Hz | 0 | 0 |
| 262 Hz（C4） | 3816 | 262.05 Hz | +0.05 | +0.36 |
| 440 Hz（A4） | 2272 | 440.14 Hz | +0.14 | +0.55 |
| 523 Hz（C5） | 1912 | 523.01 Hz | +0.01 | +0.04 |
| 1000 Hz | 1000 | 1000.0 Hz | 0 | 0 |
| 3000 Hz | 333 | 3003.0 Hz | +3.0 | +1.73 |
| 9900 Hz | 101 | 9901.0 Hz | +1.0 | +0.17 |
| 9901 Hz | 100 | 10000 Hz | +99 | +17.3 |
| 9950 Hz | 100 | 10000 Hz | +50 | +8.70 |

最坏情况出现在 `N` 最小、而 `freq` 恰在商与下一个整数之间时：相对误差上界约 `1/N`。
`N = 100`（freq ≈ 10 kHz）时最坏约 **1% ≈ 17 音分**（1 音分是 1/100 半音，17 音分人耳可辨但不刺耳）。
`N ≥ 1000`（freq ≤ 1 kHz，覆盖人耳最敏感的乐音区）时最坏 **1.7 音分**，听不出来。

**结论：这条整数除法在"蜂鸣器报警音"场景下够用，在"演奏旋律"场景下不够。**
要精确就该用 `1000000 / freq` 的浮点版或查表，但代价是引入浮点库（Cortex-M3 无 FPU）。

**② 没有任何参数校验。**

```c
PassiveBuzzer_Set_Freq_Duty(0, 50);
```

`1000000 / 0` 是 C 的**未定义行为**。Cortex-M3 无硬件除法，走 `__aeabi_idiv`，
按 AAPCS 的约定除零返回 **0**（不陷入）。于是：

```text
N = 0  ⇒  Period = 0 - 1 = 0xFFFFFFFF  （Init.Period 是 uint32_t）
      ⇒  计数器要数 42.9 亿个 tick ≈ 4295 秒才溢出一次
      ⇒  现象是"没有 PWM"，而不是崩溃
```

即**除零在本平台上表现为静默的"不响"**，看不出是 bug。同理 `freq = 2000000`（N=0）也一样。
`freq = 3000000` ⇒ N=0，`Period=0xFFFFFFFF`；`freq = 1000001` ⇒ N=0。**只要 freq > 1000000 就走这条路。**

这是"驱动缺参数校验"的教科书级例子：**错误被算术吃掉了，只在现象层留下一个模糊的"没反应"。**

### 6. 一条被自己推翻的假设：`PassiveBuzzer_Init` 不开时钟会不会导致无声？

写本篇时的第一个假设是：

> `PassiveBuzzer_Init` 用 `TIM_Base_SetConfig` 绕过了 `HAL_TIM_Base_MspInit`，
> 所以它不开 TIM1 时钟。若没人开过，`PassiveBuzzer_Control(1)` 会静默无声。

按流程去查证，结论是**在本仓库不成立**：

```text
Core/Src/main.c:88    MX_TIM1_Init();        ← 先跑
Core/Src/tim.c:54     HAL_TIM_Base_Init(&htim1);
Core/Src/tim.c:208    HAL_TIM_Base_MspInit()  →  :217  __HAL_RCC_TIM1_CLK_ENABLE();
```

**TIM1 时钟在 `MX_TIM1_Init` 里已经开了，所以本工程的 Test 能工作。**
假设被推翻，如实记录。

但这个假设**没有白做**——它产出了一个可证伪的移植预测：

> 把 `driver_passive_buzzer.c` 单独移植到一个**不调用 `MX_TIM1_Init`** 的工程，
> 只调 `PassiveBuzzer_Init()` + `PassiveBuzzer_Control(1)`：**不会有任何声音，且没有任何返回值提示。**
> 因为 `TIM_Base_SetConfig` / `HAL_TIM_PWM_ConfigChannel` / `HAL_TIM_PWM_Start` 写的全是关闭时钟的寄存器。

这条预测的价值在于它指出**隐式依赖**：驱动的自述里没有一句"需要先初始化 TIM1"。
对照 Day14 §3 的依赖表，这属于"驱动 ↔ CubeMX 初始化"一栏，但教材 §11.7.2 说的是
"**无需**在 STM32CubeMX 里配置 PA8"——**只说了引脚不用配，没说定时器必须已被初始化**。

### 7. TIM1 是高级定时器：`MOE` 与空闲态才是"不响"的真正路径

TIM2/TIM3 是通用定时器，TIM1 是**高级控制定时器**。差别在于 TIM1 多一个 `BDTR` 寄存器：

```c
/* Core/Src/tim.c:84-91 —— MX_TIM1_Init 里 TIM2/TIM3 没有的那一段 */
sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;     /* OSSR = 0 */
sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;    /* OSSI = 0 */
sBreakDeadTimeConfig.LockLevel      = TIM_LOCKLEVEL_OFF;
sBreakDeadTimeConfig.DeadTime       = 0;
sBreakDeadTimeConfig.BreakState     = TIM_BREAK_DISABLE;
sBreakDeadTimeConfig.BreakPolarity  = TIM_BREAKPOLARITY_HIGH;
sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;   /* AOE = 0 */
```

外加 `sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET`（`tim.c:78`）。

于是 `PassiveBuzzer_Control(0)` 调 `HAL_TIM_PWM_Stop` 之后，链路是：

```text
HAL_TIM_PWM_Stop(TIM1, CH1)
  → __HAL_TIM_DISABLE_OCxPRELOAD / 清 CCER 的 CC1E
  → 对 IS_TIM_BREAK_INSTANCE：若所有通道都关了 ⇒ __HAL_TIM_MOE_DISABLE  （MOE = 0）
  → MOE = 0 时，输出由 OSSR/OSSI 与 OISx 决定
  → OSSR = 0（运行态关闭）+ OIS1 = RESET（tim.c:78）
  → PA8 被硬件钉在**低电平**
```

**低电平对蜂鸣器意味着"不响"**（无源蜂鸣器需要振荡，直流电平不产生声音）。
所以"不响"不是"PWM 寄存器被清零"，而是**BDTR 的空闲态接管了引脚**。

为什么要关心这个？因为如果 `AutomaticOutput = ENABLE`（AOE=1），MOE 会在下一个更新事件
自动重新置位，`Stop` 之后引脚可能又活过来——这是高级定时器的经典坑。本工程 AOE=0，
`HAL_TIM_PWM_Start` 里那个显式的 `__HAL_TIM_MOE_ENABLE()` 才是唯一让输出生效的入口。

**这也是 `PassiveBuzzer_Control` 用 `Start/Stop` 而不是把 `CCR` 设 0 的语义差别**：
- `CCR = 0` ⇒ 通道仍在输出，只是恒定低电平（仍由定时器驱动引脚）；
- `Stop` ⇒ 通道关闭 + MOE 关 ⇒ 引脚交给 BDTR 空闲态。
两种都不响，但**引脚的所有权不同**。如果要共用 PA8 给别的模块，必须先 `Stop` 把通道让出来。

### 8. `Pulse` 少一个计数：同一个库里两套写法

同样是"占空比 → CCR"，两个驱动的写法不同：

| 驱动 | 公式 | 满量程 | 正确吗 |
| --- | --- | --- | --- |
| `driver_color_led.c` | `Pulse = c * 2000 / 255`，ARR=1999 | `c=255` ⇒ 2000 = ARR+1 | **对**（见 Day15 §12） |
| `driver_passive_buzzer.c` | `Pulse = (1000000/freq - 1) * duty / 100`，`Period = 1000000/freq - 1` | `duty=100` ⇒ CCR = Period | **差一拍** |
| `driver_motor.c` | `Pulse = speed * 10`，ARR=999 | `speed=99` ⇒ 990 | **差一档**（99 而非 100） |

`Period = 999` 时计数范围是 `0…999`，**一个周期 1000 个 tick**。
`Pulse = 999` ⇒ `CNT = 999` 那一拍 `CNT < CCR` 不成立 ⇒ 输出无效电平 1 tick ⇒ 占空比 999/1000 = **99.9%**。

差一拍在蜂鸣器上**完全听不出来**（蜂鸣器响度对占空比不敏感，50% 是典型值，99.9% 与 100% 无差别）。
但如果哪天用同一个函数驱动**舵机**（500–2500 µs 脉宽对占空比是线性的），这 0.1% 就会变成
角度上的固定偏差——**而 bug 会一直藏着，直到换了负载才暴露。**

这就是 Day15 §12 那条规则的完整版：

> **CCR 的上限是 `Period + 1`，不是 `Period`。写成 `Period` 在"不在意占空比"的负载上永远测不出来。**

### 9. 教材那句 PWM1 注释是全篇最值得记的一处错

同一句话在三处原样出现：

```c
// PWM 输出的两种模式:PWM1 当极性为低,CCR<CNT,输出低电平,反之高电平
```

出处：`driver_passive_buzzer.c`（两处，`Init` 与 `Set_Freq_Duty`）、`driver_motor.c`（两处，
正转与反转分支）、教材 `ch11_drivers.txt`（§11.7.3 与 §11.19.3 的代码块里）。

**这句话是反的。** 正确规则：

```text
PWM1 模式，向上计数：CNT < CCR ⇒ 通道"有效"；否则"无效"。
"有效"是高还是低，由 CCxP（OCPolarity）决定：
  CCxP = 0（HIGH）⇒ 有效 = 高电平
  CCxP = 1（LOW） ⇒ 有效 = 低电平
```

所以：

| 配置 | CNT < CCR 时输出 | CNT ≥ CCR 时输出 |
| --- | --- | --- |
| PWM1 + HIGH | **高** | 低 |
| PWM1 + LOW | **低** | 高 |

教材注写的是"**CCR < CNT**，输出低电平，反之高电平"——即 `CNT > CCR` 时输出低。
按上表，PWM1+LOW 时 `CNT > CCR`（无效）输出的是**高**。**两个分支都说反了。**

更直接的证据在 `driver_motor.c` 里：注释的上一行是 `PWM1 当极性为低`，
下一行却是 `sConfig.OCPolarity = TIM_OCPOLARITY_HIGH;`——**注释说"低"，代码写 `HIGH`，
两句紧挨着矛盾**。只有那一行行尾的注释是对的（"设置极性为高"）。

**为什么这个错误能一直活着？** 因为在它出现的三个负载上，**相位都不影响功能**：

- 蜂鸣器：方波反相 180° 还是同一个方波，声音一样；
- 电机：`driver_motor.c` 的正转分支靠 `IN1=PWM`、`IN2=LOW`，反转靠 `IN1=PWM`、`IN2=HIGH`，
  两个方向各自算过占空比（§12），相位的解释错了但**代码是对的**。

**只有当负载对相位敏感时（比如 ColorLED 的亮度、舵机的脉宽）这个错误才会变成故障。**
这就解释了为什么 ColorLED 的极性注释写的是"硬件上低电平亮灯"——那里**必须**说清极性，
作者在那里写对了，在别处复制时写错了。

### 10. DRV8833 真值表：四种状态，两种能调速

教材 §11.19.2 给了真值表（`ch11_drivers.txt:1745-1770`）：

| IN1 | IN2 | OUT1 | OUT2 | 功能 |
| --- | --- | --- | --- | --- |
| 0 | 0 | Z | Z | **惯性运行 / 快速衰减**（高阻） |
| 0 | 1 | L | H | 反转 |
| 1 | 0 | H | L | 正转 |
| 1 | 1 | L | L | **制动 / 慢速衰减**（绕组短接） |

四种状态里，只有 `0,1` 与 `1,0` 两行是"持续出力"，`0,0` 与 `1,1` 都是"出力为零但性质不同"：

- **`0,0`（高阻）**：电机两端悬空，动能靠反电动势经体二极管泄放 ⇒ **快速衰减**（电流掉得快）；
- **`1,1`（短接）**：绕组被短路，反电动势形成**制动电流**，产生反向力矩 ⇒ **慢速衰减 + 明显制动**。

**这两个"零出力"不是同一件事**：一个是滑行，一个是刹车。
`Motor_SetSpeed(ch, 0)` 用的是哪一行，直接决定"停止"是滑行还是急停。

### 11. 电机 B 的引脚能力不对称，决定了 PWM 表示"制动时间"

教材 §11.19.3 说得很清楚（`ch11_drivers.txt:1784-1788`）：

> 通道 A 使用 PA4、PA0 来控制，这 2 个引脚没有 PWM 功能，所以只需要配置为输出即可。
> **只可以控制 3 个功能：停止、正转、反转。**
>
> 通道 B 使用 PB4、PB15 来控制，PB4 可以配置为 PWM 引脚（TIM3_CHN1），PB15 仍然配置为输出引脚。

于是通道 B 的"调速"必须在**只有一个引脚能输出 PWM** 的约束下实现，而两个方向的语义不同：

**正转**（`IN1=PWM, IN2=LOW`）：

```text
PWM 高电平 → (1,0) → 正转出力
PWM 低电平 → (0,0) → 惯性滑行（快衰减）
⇒ 占空比 = 出力时间占比。speed 越大占空比越大 ⇒ Pulse = speed*10  ✔ 直读
```

**反转**（`IN1=PWM, IN2=HIGH`）：

```text
PWM 高电平 → (1,1) → 制动（慢衰减）
PWM 低电平 → (0,1) → 反转出力
⇒ 占空比 = **制动**时间占比。speed 越大，制动应越少 ⇒ Pulse = (99 - speed)*10  ✔ 反读
```

**这就是 `(99 - speed) * 10` 的来历。** 它不是绕弯子，而是**引脚能力不对称逼出来的**：
如果 PB15 也能输出 PWM，反转就可以写成 `IN2=PWM, IN1=LOW`，占空比又能直读，
两个方向就能共用一套公式。

由此得到一个**可证伪的预测**（教材完全没提）：

> 通道 B 正转是"正转 ↔ 惯性（快衰减）"，反转是"制动 ↔ 反转（慢衰减）"。
> 两种衰减模式下绕组的电流通路不同，**同一个 `|speed|` 的正转与反转，实际转速/转矩不会完全对称**。
> 具体的偏差方向与幅度需要实测（本机无电机、无声级转速计）。

这条不作为结论，只作为上板时的观察项——**"应该对称"是直觉，"是否对称"要靠读数**。

### 12. `speed == 0` 实现的是"制动"，不是"惯性滑行"

`Motor_PWM_SetSpeed` 的第一分支：

```c
if (speed == 0)
{
    HAL_GPIO_WritePin(MOTOR_B_IN1_GPIO_GROUP, MOTOR_B_IN1_GPIO_PIN, GPIO_PIN_SET);   /* IN1 = 1 */
    HAL_GPIO_WritePin(MOTOR_B_IN2_GPIO_GROUP, MOTOR_B_IN2_GPIO_PIN, GPIO_PIN_SET);   /* IN2 = 1 */
    /* 然后两个引脚都配成 OUTPUT_PP */
}
```

`IN1=1, IN2=1` ⇒ 真值表第 4 行 ⇒ **制动 / 慢速衰减**。

而真值表第 1 行 `0,0` 才是惯性滑行。**驱动选了刹车，而不是滑行。**
教材的 `Motor_SetSpeed` 注释只写"0表示停止"——"停止"这个词把两种物理行为盖住了：

| 实现 | 真值表 | 物理 | 电机手感 |
| --- | --- | --- | --- |
| 现在（`1,1`） | 制动 | 绕组短接，反向制动电流 | **急停**，有"顿"感 |
| 改为（`0,0`） | 惯性 | 高阻，反电动势泄放 | 缓慢滑行到停 |

`Motor_Init` 也同理：两个通道初始化时都先把 IN1/IN2 写 `SET`（`driver_motor.c:177-178`、`:197-198`），
所以**上电即制动**，不是自由状态。

这条在工程上是可判定的：**断电（或调 `speed=0`）后电机是否立即停住、还是继续转一会儿**，
肉眼就能区分。它是上板时最便宜的一个判据。

### 13. `Motor_Init` 与 `Motor_SetSpeed` 的模式来回切换

电机 B 的 PB4 在三个模式间来回走：

```text
MX_TIM3_Init        → PB4 = AF_PP   （HAL_TIM_MspPostInit，tim.c:307 附近）
Motor_Init(MOTOR_B) → PB4 = OUTPUT_PP，ODR=1  （制动）
Motor_SetSpeed(B, 50)  → PB4 = AF_PP   （改回 PWM）
Motor_SetSpeed(B, 0)   → PB4 = OUTPUT_PP （又变回 GPIO）
```

**同一个引脚在不同时刻扮演"GPIO 输出"和"定时器通道"**，靠 `HAL_GPIO_Init` 的 `Mode` 切换。
这比 `ColorLED_Set` 的 `Stop → Config → Start` 更激进——ColorLED 从不改引脚模式，只改寄存器。

代价有两条：

1. **每次调速都重新 `HAL_GPIO_Init` 两次**（IN1 的 AF/OUT，IN2 的 OUT）。`HAL_GPIO_Init` 要
   读-改-写 `CRL/CRH`，不是原子操作。在电机上无所谓（KHz 级 PWM 不关心几微秒的配置时间），
   但在高频调速（比如循迹小车的连续 PID 输出）里，这段开销会挤占控制周期。
2. **AF → GPIO 切换时，定时器通道仍在跑**。`Motor_Init` 没调 `HAL_TIM_PWM_Stop`，
   于是 TIM3_CH1 的计数器与 CCR 继续工作，只是输出被断开。下次切回 AF 时，引脚立刻接到
   正在运行的通道上——**相位是随机的**（可能从周期的任意位置开始）。
   对电机无影响；对需要定相的负载（比如 LED 同步调光）就会有可见的不连续。

### 14. `MOTOR_B_IN2_CHANNEL` 是死宏

```c
#define MOTOR_B_IN2_CHANNEL    TIM_CHANNEL_3
```

全仓库**没有一处使用它**（`grep` 只有定义）。PB15 只被当 GPIO 用。

这和 Day15 §6 的 `Led_Control` 死参数 `which` 是同一类问题：**接口预留了但没实现**。
区别是危险程度：

| | 死参数 `which` | 死宏 `MOTOR_B_IN2_CHANNEL` |
| --- | --- | --- |
| 误用后果 | 调用通过、点了别的灯 | **无后果**（没有代码读它） |
| 风险 | 中等（行为与代码不符） | 低（但暗示"PB15 本可以接 TIM3_CH3"） |

它真正的价值是**一条硬件事实的线索**：`TIM_CHANNEL_3` 说明 PB15 在**别的引脚映射下**能做
`TIM3_CH3`。查参考手册：TIM3_CH3 的默认引脚是 **PB0**，重映射后是 **PB4/PB5/PB0**。
PB15 在任何映射下都不是 TIM3_CH3——**这个宏在物理上不可实现**。
所以它不是"预留"，是**残留**：作者一开始可能打算用 TIM3_CH3，发现 PB15 不支持后改成 GPIO 驱动，
宏忘了删。

### 15. 教材 §11.6.2 与 §11.7.2 是逐字重复，§11.7.5 的现象写错了模块

三处对照（`ch11_drivers.txt`）：

| 位置 | 原文 | 问题 |
| --- | --- | --- |
| `:206-212`（§11.6.2） | "有源蜂鸣器使用 PA8……无需在 STM32CubeMX 里配置 PA8。" | —— |
| `:242-248`（§11.7.2） | **同上，一字不差** | 无源蜂鸣器那节的"配置"讲的是两个模块共用引脚，唯独不说无源自己需要什么（TIM1_CH1 + 1 kHz） |
| `:355`（§11.7.5） | "**有源蜂鸣器**保持响 1 秒之后保持不响 1 秒……显示**有源蜂鸣器**的状态" | 这一节是无源蜂鸣器，**主语从上一节复制过来没改** |

第 2 条的实质影响最大：**无源蜂鸣器的"配置"信息（TIM1、PSC=71、ARR=999、PWM 通道 1、极性）在教材里是缺席的**。
唯一的来源是 §11.7.3 那段核心代码——而那段的注释还带着 §9 的方向错误。

第 3 条抹掉的是无源蜂鸣器**唯一有辨识度的现象**：有源蜂鸣器只能"响/不响"，
无源蜂鸣器能**变调**。教材把两者的现象写成同一句，等于没有检验"无源"这件事。

### 16. `PassiveBuzzer_Test` 重复初始化 LCD，`ActiveBuzzer_Test` 不初始化

```c
/* PassiveBuzzer_Test */
void PassiveBuzzer_Test(void) {
    PassiveBuzzer_Init();
    LCD_Init();          ← 自己调
    LCD_Clear();
    while (1) { ... }
}

/* ActiveBuzzer_Test */
void ActiveBuzzer_Test(void) {
    ActiveBuzzer_Init();
    while (1) { LCD_PrintString(...); ... }    ← 直接用，不调 LCD_Init
}
```

两个相邻模块，一个调一个不调。这**修正了 Day15 §3 的结论**（见 §17）。

机制上的后果：`LCD_Init()` 会重发一整套 I2C 初始化（SSD1306 的配置序列）。在
`StartDefaultTask` 已经调过一次的前提下再调一次，是**重复的 I2C 流量**——功能上无害
（SSD1306 的初始化序列是可重入的），但它说明**前置条件没有统一的约定**：
有的 Test 假定"外层已初始化"，有的自己兜底。

`Motor_Test` 也不调 `LCD_Init()`，与 `ActiveBuzzer_Test` 一致。所以是 19 个 Test 里
**2 个自己调（`PassiveBuzzer_Test`、`LCD_Test`）、其余依赖外层**。

---

### 17. 三条对 Day15 的修正

写本篇时逐行核对了 Day15 里几处"待确认"的推断，结果如下：

| Day15 的说法 | 本篇核对结果 | 证据 |
| --- | --- | --- |
| §10 "TIM2 在 PA15/PB3 上的重映射是否配齐需对照 CubeMX 源工程" | **已配齐**。`HAL_TIM_MspPostInit` 的 TIM2 分支末尾有 `__HAL_AFIO_REMAP_TIM2_PARTIAL_1()` | `Core/Src/tim.c:295` |
| §7 "谁开 GPIOB 时钟" | `HAL_TIM_MspPostInit` 的 TIM2 分支开 GPIOB（`:279`）；`lab_pins.c:55` 也开 | `tim.c:278-279`、`lab_pins.c:55` |
| §3 "`LCD_Init()` 在 `for(;;)` 外，是所有 Test 的共同前置" | **不完全对**。`PassiveBuzzer_Test` 内部又调了一遍（§16），说明没有统一约定 | `driver_passive_buzzer.c` `PassiveBuzzer_Test` |
| §1 "§11.3–§11.19 每节都是同一个四段结构" | **11.19 是例外**：`.2` 是"操作方法"（真值表），CubeMX 配置被挪到 `.4`，共五段 | `ch11_drivers.txt:1717-1985` |

前两条是把"未验证"收敛为"源码核对"（**仍不是实测**——`SWJ_CFG` 是否被配是另一件事，
Day14 缺陷②依旧成立：重映射配了，但 JTAG 释放没配）。

后两条是**自我纠正**：Day15 的概括在 11.19 与 `PassiveBuzzer_Test` 上不成立。
保留原笔记不改，在这里登记——这样读者能看到"先概括、再被个案修正"的过程，
比把错误悄悄改掉更接近真实的排查路径。

---

## 系统调用/接口卡片

### `void ActiveBuzzer_Init( void )`

作用：把 PA8 配成推挽输出（GPIO 模式，接管 TIM1_CH1）。

参数：无。

返回值和错误：**`void`，无失败路径**。

资源所有权：**不打开 GPIOA 时钟**（依赖 `MX_GPIO_Init` / `MX_TIM1_Init` / `lab_pins.c`）；
**不预置 ODR** ⇒ 切模式的瞬间输出 ODR 当前值（复位值 0 = 低 = 响，见 §3）。

线程/进程安全：改引脚模式，必须在任何任务使用 PA8 之前完成；FreeRTOS 下建议在调度器启动前调。

常见误用：① 在 `MX_TIM1_Init` 之后忘记调它 ⇒ PA8 仍是 TIM1 通道，`WritePin` 无效；
② 期待它顺带把蜂鸣器关掉（它会让蜂鸣器先响一下）。

### `void ActiveBuzzer_Control( int on )`

作用：响 / 不响。

参数：`on` —— 非 0 响、0 不响。

返回值和错误：`void`。

最小示例（本仓库 `ActiveBuzzer_Test`）：

```c
LCD_PrintString(0, 0, "ActiveBuzzer ON ");
ActiveBuzzer_Control(1);      /* 写 RESET = 低电平 = 响 */
mdelay(1000);
LCD_PrintString(0, 0, "ActiveBuzzer OFF");
ActiveBuzzer_Control(0);      /* 写 SET = 高电平 = 不响 */
mdelay(1000);
```

常见误用：以为参数是电平而不是逻辑（写 `Control(0)` 期待"低电平"得到的是高电平）。

### `void PassiveBuzzer_Init( void )`

作用：把 PA8 配成 `AF_PP`（TIM1_CH1），设 PSC=71 / ARR=999 / CCR=499，配置通道但不启动。

参数：无。

返回值和错误：`void`。

资源所有权：**用 `TIM_Base_SetConfig` 绕过 `HAL_TIM_Base_Init`** ⇒ 不触发 MspInit、不置状态机。
前提是 TIM1 时钟已开、`htim1.State == READY`（§4、§6）。

常见误用：① 移植到没有 `MX_TIM1_Init` 的工程（静默无声）；② 以为它启动了输出
（启动在 `Control(1)` 或 `Set_Freq_Duty` 里）。

### `void PassiveBuzzer_Control( int on )`

作用：启动 / 停止 TIM1_CH1 的 PWM 输出。

参数：`on` —— 非 0 启动、0 停止。

返回值和错误：`void`。

资源所有权：停止时通道关闭 + `MOE = 0` ⇒ PA8 被 BDTR 空闲态钉在**低电平**（§7）。
引脚所有权仍属定时器（不是 GPIO）。

最小示例：

```c
PassiveBuzzer_Set_Freq_Duty(1000, 50);   /* 1 kHz 方波，为响度最优的 50% */
PassiveBuzzer_Control(1);
mdelay(500);
PassiveBuzzer_Control(0);
```

常见误用：先 `Control(1)` 再 `Set_Freq_Duty` ⇒ 会先响一声 1 kHz 再跳到目标频率（`Set_Freq_Duty`
内部先 `Stop` 再重配再 `Start`）。

### `void PassiveBuzzer_Set_Freq_Duty( int freq, int duty )`

作用：设频率（Hz）与占空比（%）。

参数：`freq` —— 单位 Hz，**无校验**；`duty` —— 0–100，**无校验**。

返回值和错误：`void`。

**边界（关键）**：

| 输入 | 结果 |
| --- | --- |
| `freq = 0` | `1000000/0` 未定义行为；Cortex-M3 上得 0 ⇒ `Period = 0xFFFFFFFF` ⇒ **静默无声** |
| `freq > 1000000` | 同上（商为 0） |
| `freq = 1` | `Period = 999999` ⇒ 1 Hz，可实现但听见的是"咔哒" |
| `duty > 100` | `Pulse > Period` ⇒ CCR > ARR ⇒ 恒有效电平 ⇒ 恒定低电平 ⇒ **不响**（不是"最大响度"） |
| `duty < 0` | `Pulse` 为负，赋给 `uint32_t` ⇒ 巨大值 ⇒ 同上 |

资源所有权：每次调用**完整拆建一次**：`Stop` → `HAL_TIM_Base_DeInit`（**关 TIM1 时钟**）
→ 赋值 → `HAL_TIM_Base_Init`（**重开时钟**）→ `ConfigChannel` → `Start`。
`HAL_TIM_Base_DeInit` 会调 `HAL_TIM_Base_MspDeInit` → `__HAL_RCC_TIM1_CLK_DISABLE()`（`tim.c:334`）。

最小示例：

```c
for (int f = 262; f <= 523; f += 26) {   /* C4 到 C5 之间扫频 */
    PassiveBuzzer_Set_Freq_Duty(f, 50);
    PassiveBuzzer_Control(1);
    mdelay(200);
}
PassiveBuzzer_Control(0);
```

常见误用：① 传 0 或超出 1 MHz；② 在中断或高频循环里调用（每次关/开时钟）；
③ 以为 `duty=100` 是"最响"（实际是不响，见上表）。

### `void Motor_Init( int32_t channel )`

作用：把通道的 IN1/IN2 配成推挽输出，先写 `SET`（⇒ 真值表 `1,1` ⇒ 制动）。

参数：`channel` —— `MOTOR_A`(1) 或 `MOTOR_B`(2)；其他值**静默无操作**。

返回值和错误：`void`。`channel` 非法时两个 `if` 都不进，什么也不做，**无任何提示**。

资源所有权：不开 GPIO 时钟；对 `MOTOR_B` 会把 PB4 从 `AF_PP` 改成 `OUTPUT_PP`（§13）。

常见误用：① 传 0 或 3（无操作但无报错）；② 以为初始化后电机是自由状态（实际是制动）。

### `void Motor_SetSpeed( int32_t channel, int32_t speed )`

作用：设速度与方向。

参数：`channel` —— `MOTOR_A` / `MOTOR_B`；`speed` —— 文档写 `-100~100`，
**实际钳位到 `-99~99`**（`if (speed > 99) speed = 99;`）。

返回值和错误：`void`。

**通道差异**：

| 通道 | 能力 | 实现 |
| --- | --- | --- |
| `MOTOR_A` | 只有 停止 / 正转 / 反转（**无调速**） | 纯 GPIO 写 `(IN1, IN2)`，`speed` 的绝对值被忽略 |
| `MOTOR_B` | 停止 / 正转 / 反转 / **调速** | `Motor_PWM_SetSpeed`：正转 `Pulse=speed*10`，反转 `Pulse=(99-speed)*10` |

**`MOTOR_A` 的隐藏行为**：`speed = 5` 与 `speed = 99` 效果完全相同（都是 `1,0` 正转全速）。
文档写"channel - MOTOR_A 或 MOTOR_B，speed 取值 -100~100"，
**没有说 MOTOR_A 不响应数值**——这是最容易误用的一条。

最小示例（本仓库 `Motor_Test` 的正转段）：

```c
speed = 0;
for (i = 0; i < 10; i++) {
    Motor_SetSpeed(MOTOR_A, speed);    /* A 通道：0 → 直接全速正转 */
    Motor_SetSpeed(MOTOR_B, speed);    /* B 通道：0 → 10% → ... → 90% 渐加速 */
    speed += 10;
    mdelay(1000);
}
```

常见误用：① 以为 `MOTOR_A` 能调速（同一段代码里 A 和 B 的行为不同，屏幕上显示的速度只有 B 兑现）；
② 传 `±100`（被钳到 ±99）；③ `speed=0` 当滑行用（实际是制动，§12）。

---

## 最小可运行实验

不新写实验。`LAB_TEST` 选择器（Day14 §5 的 `#if` 机制）：

| `LAB_TEST` | 覆盖教材 | 现象（教材描述，**本仓库未观察**） |
| --- | --- | --- |
| 1 | §11.6 有源蜂鸣器 | 响 1 s / 停 1 s 循环，屏显 `ActiveBuzzer ON` / `OFF` |
| 2 | §11.7 无源蜂鸣器 | 教材把它写成和有源一样（§15 的错误），实际应为**恒定 1 kHz 的"嘀"声**（`Init` 里 ARR=999、duty 50%） |
| 9 | §11.19 直流电机 | 屏显速度值，A/B 两通道同步 0→90→0→-90 循环，每档 1 s |

`LAB_TEST=2` 有一个**读者容易误判的点**：`PassiveBuzzer_Test` 只调 `Init()` + `Control(1)`，
所以它**永远只发 1 kHz**，不会变调。想听不同音调必须自己改用 `Set_Freq_Duty`。
教材 §11.7.5 把这个现象写成"和有源一样"，读者会以为"无源也能只调响停"——**这正是教材那句话的危害**。

---

## 编译与运行

```bash
cd experiments/F103_HAL_DriverLab
cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake
cmake --build build

arm-none-eabi-size build/f103_hal_driverlab.elf
```

`LAB_TEST` 用 `#if` 选择（Day14 §5：`switch` 会让 19 个 Test 全成为可能调用点，
`text` 10 936 → 33 928）。三者对应的体积（Day14 已测）：

| `LAB_TEST` | `text` | 净增 |
| --- | --- | --- |
| 0（无 Test） | 15 276 | —— |
| 12（OLED） | 19 360 | +4 084 |
| 18（MPU6050） | 21 272 | +5 996 |

**烧写与调试：本仓库只做到"命令能拼对"**，未实际烧写（无 ST-Link）。

---

## 结果观察

### 构建（实测，`logs/build_2026-10-29.log`）

```text
0 error / 13 warning（13 条全部来自课程驱动，本工程新增文件零告警）
text 15276 / data 100 / bss 3176
```

本篇三个驱动**零告警**。这并不说明它们正确——§5 的除零、§9 的注释错误、
§12 的制动/滑行混用，**没有一条会触发编译器**。

### 板上现象（**全部未验证**）

教材的三处现象描述（`ch11_drivers.txt:221`、`:355`、`:1984`）在本仓库一条都没有观察过。
按本篇分析，真板上应当重点看四件事：

| 观察项 | 预期（源码推论） | 判据 |
| --- | --- | --- |
| 有源蜂鸣器上电瞬间 | **先响一下再进入节奏**（§3：ODR 复位值 0 = 低 = 响） | 有无短促"嘀" |
| 无源蜂鸣器音调 | 恒定 1 kHz，**不随任何输入变化** | 与"有源"的音色对比 |
| 电机 `speed=0` | **立即停住（制动）**，不是滑行 | 从全速切到 0 后是否继续转 |
| 电机 A vs B | A 只有"全速/停"，B 才是渐加速 | 屏幕上速度从 10 到 90 时 A 是否也变 |

**四条都是可证伪的预测，不是结论。** 上板后要逐条记，包括预测错的。

### 本篇**没有**验证的东西

- 三个模块的**任一板上现象**（出声、音调、转向、转速、停机方式）。
- 有源蜂鸣器上电瞬间那一声"嘀"是否真的存在（§3 的 ODR 推论）。
- 无源蜂鸣器在 `LAB_TEST=2` 下是否真的**只发 1 kHz**（依赖 `MX_TIM1_Init` 已配好，§4）。
- 正转/反转的**转速不对称性**（§11 的预测），需要转速计或声级计。
- `PassiveBuzzer_Set_Freq_Duty(0, …)` 在真机上确实"静默无声"而不是别的方式失败
  （`__aeabi_idiv` 除零返回 0 是 AAPCS 约定，未在目标板上验证）。
- `HAL_TIM_PWM_Stop` 在 TIM1 上是否确实清 `MOE`（§7）——依据 HAL 源码阅读，未在硅上验证；
  本机也没有示波器看 PA8。
- `MOTOR_B_IN2_CHANNEL`（TIM3_CH3）在 PB15 上不可实现这一结论（§14）——依据参考手册的引脚表推断。
- 教材 §11.19 真值表的准确性——**只核对了教材自己的表**，没有对照 DRV8833 数据手册。

---

## 常见错误和陷阱

| 陷阱 | 症状 | 根因 | 处理 |
| --- | --- | --- | --- |
| 有源蜂鸣器写 `Control(0)` 想让它响 | 反了 | 低电平有效（灌电流接法，与 LED 同） | `Control(1)` = 响 |
| 有源和无源"同时用" | 只有一个工作 | 后 `Init` 者改写 PA8 的 `Mode`（§2） | 运行期只让一个 Init 跑 |
| 移植无源蜂鸣器后无声 | 完全静默，无报错 | `PassiveBuzzer_Init` 跳过 MspInit，TIM1 时钟没开（§4、§6） | 先保证 `MX_TIM1_Init` 被调用 |
| `Set_Freq_Duty(0, 50)` | 静默无声 | 除零 → `Period = 0xFFFFFFFF`（§5） | 加 `if (freq < 1 \|\| freq > 1000000) return;` |
| `Set_Freq_Duty(1000, 100)` | 不响 | `Pulse = Period` 且 duty=100 ⇒ 恒低电平（§8、接口卡片边界表） | duty 保持 ≤ 99 |
| 音调不准 | 偏高，且高频偏得更多 | `1000000/freq` 整数除法（§5） | 接受（≤1 kHz 时 1.7 音分）或用浮点/查表 |
| 按教材注释理解 PWM1 极性 | 推断出的电平与实际相反 | 教材那句方向写反（§9） | 以参考手册的"CNT<CCR ⇒ 有效"为准 |
| 以为 `Pulse = Period*duty/100` 是 100% | 占空比恒差 1 tick | CCR 上限是 `Period+1`（§8、Day15 §12） | 用 `(Period+1)*duty/100` |
| 以为 `Motor_SetSpeed(A, 30)` 是 30% 速度 | A 通道恒全速 | A 无 PWM 能力，只读符号（§11、接口卡片） | A 只用 -99/0/99 三档语义 |
| `speed=0` 当"滑行" | 电机急停 | 实现是 `1,1` = 制动（§12） | 要滑行须改成 `0,0` |
| 传 `speed=±100` | 实际是 ±99 | 钳位写的是 `>99`/`<-99` | 按 ±99 设计 |
| 高频调速时控制周期抖动 | 调速响应不匀 | 每次调速两次 `HAL_GPIO_Init`（§13） | 高频场景改用 `__HAL_TIM_SET_COMPARE` |
| `Motor_Init(0)` 无反应也不报错 | 引脚没被配置 | `channel` 非法时静默跳过 | 只用 `MOTOR_A`/`MOTOR_B` |
| 照 §11.7.5 期待无源蜂鸣器"响停循环" | 和服务端描述一致但学不到东西 | 教材那句主语复制错了（§15） | 用 `Set_Freq_Duty` 自己扫频 |

---

## 故障排查方法

1. **"蜂鸣器完全不响。"** 按负载分两路：
   - 有源：① `ActiveBuzzer_Init` 被调了吗（不调则 PA8 仍是 TIM1 通道，`WritePin` 无效）；
     ② GPIOA 时钟开了吗；③ 用万用表量 PA8：`Control(1)` 后应为 **0 V**。
   - 无源：① `MX_TIM1_Init` 被调过吗（§6）；② `PassiveBuzzer_Control(1)` 之后 PA8 有 1 kHz 方波吗
     （没示波器就用万用表直流档：50% 方波的平均值 ≈ 1.65 V，这是最便宜的判据）；
     ③ `freq` 是否在 1…1000000 之内。
2. **"响但音调不对。"** 先算 `N = 1000000/freq`，实际频率是 `1000000/N`。
   偏差随 `N` 变小而变大（§5 表）。要排除整数除法影响，就试 `freq = 1000`（N=1000，误差为 0）。
3. **"无源蜂鸣器只响一下就停。"** 检查是不是先 `Control(1)` 后 `Set_Freq_Duty`——
   后者内部 `Stop` 会先把它关掉（接口卡片"常见误用"）。
4. **"电机不转。"** 顺序：① `MX_TIM3_Init` 是否调过（PB4 的时钟与 AF 配置）；② `Motor_Init(MOTOR_B)`
   把 PB4 改成了 GPIO——`Motor_SetSpeed` 里才会改回 AF，若 `speed` 一直是 0 则永远不转；
   ③ 量 PA4/PA0（通道 A）或 PB4/PB15（通道 B）的实际电平，对着真值表看落在哪一行。
5. **"电机只有全速/停，没有中间档。"** 用了 `MOTOR_A`。A 的两个引脚都不支持 PWM（§11）。
6. **"电机反转比正转明显无力/慢。"** 这是 §11 预测的不对称（快衰减 vs 慢衰减）。
   先确认不是机械原因（同一方向连续跑），再考虑"反转时换用另一引脚做 PWM"（需硬件支持）。
7. **"停机时电机顿一下。"** 正常——`speed=0` 是真值表 `1,1` 的制动，不是滑行（§12）。
   要滑行必须改代码为 `0,0`。
8. **"电机和蜂鸣器互相干扰。"** 检查是不是两个模块的引脚有重叠（PA8 三方、PB4 与 RGB 的 PB3 邻近）。
   `Day14` 的五组冲突表是排查起点。

---

## 练习题

### 基础题

1. "有源"和"无源"的区别在哪？为什么有源蜂鸣器不需要定时器，无源蜂鸣器必须有？
2. PA8 被三个模块共用。"不能同时使用"的硬件机制是什么？为什么教材说"可以既使用有源也使用无源"？
3. `PassiveBuzzer_Init` 用 `TIM_Base_SetConfig` 而不是 `HAL_TIM_Base_Init`，跳过了哪两件事？
4. DRV8833 真值表里 `0,0` 与 `1,1` 都是"零出力"，物理区别是什么？

### 改错题

5. 下面这段想演奏一个音阶，指出至少三处问题：

   ```c
   for (int f = 262; f <= 523; f += 44) {
       PassiveBuzzer_Control(1);
       PassiveBuzzer_Set_Freq_Duty(f, 100);
       mdelay(300);
   }
   PassiveBuzzer_Control(0);
   ```

6. 下面这段想"让电机 B 以 50% 速度滑行停止"，指出问题：

   ```c
   Motor_SetSpeed(MOTOR_B, 50);
   mdelay(2000);
   Motor_SetSpeed(MOTOR_B, 0);       /* 以为这里会惯性滑行 */
   ```

7. 下面这段注释与代码矛盾，指出错在哪、正确的表述是什么：

   ```c
   sConfig.OCMode = TIM_OCMODE_PWM1;         // PWM1 当极性为低,CCR<CNT,输出低电平,反之高电平
   sConfig.OCPolarity = TIM_OCPOLARITY_HIGH; // 设置极性为高
   sConfig.Pulse = speed * 10;
   ```

### 综合题

8. 用 §5 的公式推演：`PassiveBuzzer_Set_Freq_Duty(440, 50)` 时，
   ① `Period` 是多少？② 实际频率是多少 Hz？③ 音分误差是多少？④ CCR 是多少？
   ⑤ 如果要求误差 < 1 音分，`freq` 的上界大概是多少？（提示：cents ≈ 1731·|1e6/(N·freq) − 1|）

9. 设计一个能让 `PassiveBuzzer_Test` **真的检验"无源"这件事**的改法。
   要求：① 至少用 3 个不同频率；② 给出**可判定**的判据（不是"听起来不一样"）；
   ③ 说明为什么不能用"响/不响"作为判据。

10. 电机通道 B 的 PB15 若换成一根能做 `TIM3_CH3` 的引脚（比如 PB0），
    `Motor_PWM_SetSpeed` 会怎么简化？请写出简化后的三行核心代码，并说明这为什么能让
    正转与反转**共用一套占空比公式**。（提示：§11 的两张"PWM 高/低电平"映射表）

11. `PassiveBuzzer_Control(0)` 之后 PA8 是低电平。假设某天要在同一个工程里让 PA8
    改做"红外发射"（也是 TIM1_CH1）。请列出必须做的清理步骤，以及漏掉每一步分别会导致什么。

---

## 验收标准

- [ ] 能说出有源/无源的本质区别（振荡器在哪），并由此推出引脚模式与控制量的差异。
- [ ] 能解释 PA8 三方争用的机制是"`HAL_GPIO_Init` 的 Mode 覆盖"，而不是"插不上去"。
- [ ] 能列出 `ActiveBuzzer_Init` 相对 `Led_Init` 少做的两件事及各自后果（含上电响一下的推论）。
- [ ] 能解释 `TIM_Base_SetConfig` 与 `HAL_TIM_Base_Init` 的三处差别，并说明本工程为何不受影响。
- [ ] 能写出"无源蜂鸣器移植到无 `MX_TIM1_Init` 的工程会无声"这条预测及其机制。
- [ ] 能画出 `Control(0)` 的硬件路径（`Stop` → 清 CC1E → `MOE=0` → OSSR/OIS1 ⇒ 低电平）。
- [ ] 能算出 `1000000/freq` 的整数除法误差，并指出最坏区间与量级（≈17 音分@N=100）。
- [ ] 能说明 `freq=0` 与 `freq>1000000` 都表现为"静默无声"，机制是商为 0。
- [ ] 能判断 `Pulse = Period*duty/100` 与 `Pulse = c*(Period+1)/255` 哪个对，并解释为何前者在本负载上不暴露。
- [ ] 能复述 DRV8833 四行真值表，并指出 `0,0` 与 `1,1` 的物理区别（快衰减 vs 制动）。
- [ ] 能解释 `(99-speed)*10` 的来历（引脚能力不对称 ⇒ PWM 表示制动时间）。
- [ ] 能说出 `speed=0` 是制动不是滑行，以及教材"0表示停止"这个措辞丢掉的信息。
- [ ] 能指出教材那句 PWM1 注释的两处方向错误，并解释它为什么能一直存在。
- [ ] 能列出本篇**未验证**的八项，并说明它们全部止于上电前。
- [ ] 能复述三条对 Day15 的修正，并说明"重映射已配"与"JTAG 未释放"是两件事。

---

## 延伸阅读

- 教材第 11 章：
  - §11.6 有源蜂鸣器：`ch11_drivers.txt:198-223`（§11.6.2 说明"需用代码配置 PA8"）
  - §11.7 无源蜂鸣器：`:224-358`（§11.7.2 与 §11.6.2 **逐字重复**；§11.7.3 核心代码带错误注释；
    §11.7.5 现象主语写成"有源蜂鸣器"）
  - §11.19 直流电机：`:1717-1985`（**唯一五段结构**：`.2` 是操作方法/真值表，CubeMX 配置在 `.4`）
- 驱动源码：`Drivers/DshanMCU-F103/`
  - `driver_active_buzzer.c` — `Init`（不开时钟、不预置 ODR）、`Control`（低有效）
  - `driver_passive_buzzer.c` — `Init` 用 `TIM_Base_SetConfig` 且把 DeInit/Init 注释掉；
    `Set_Freq_Duty` 用 `HAL_TIM_Base_Init` 且每次关/开时钟；整数除法与无校验
  - `driver_motor.c` — `MOTOR_A/B_IN1/IN2_*` 宏、`MOTOR_B_IN2_CHANNEL` 死宏、
    `Motor_Init` 的 `SET,SET`、`Motor_PWM_SetSpeed` 三分支与 `(99-speed)*10`
- 平台配置：`Core/Src/tim.c`
  - `:32-99` `MX_TIM1_Init`（高级定时器：`MasterConfigSynchronization`、`ConfigBreakDeadTime`）
  - `:84-91` BDTR：`AOE = DISABLE`、`OSSR/OSSI = DISABLE`、`DeadTime = 0`
  - `:78` `OCIdleState = TIM_OCIDLESTATE_RESET`
  - `:208-247` `HAL_TIM_Base_MspInit`（TIM1/TIM2 开时钟）
  - `:250-312` `HAL_TIM_MspPostInit`（PA8→TIM1_CH1；TIM2 分支有 `__HAL_AFIO_REMAP_TIM2_PARTIAL_1()`）
- 本仓库前置：`Core/Src/main.c:83-95`（外设初始化顺序）、`Core/Src/lab_pins.c`（8 个纯 GPIO）
- 相关笔记：`Day02`（GPIO 寄存器）、`Day04`（工作模式）、`Day13`（定时器与 PWM 的 PSC/ARR 三角约束）、
  `Day14`（契约、冲突表、三条缺陷）、`Day15`（PWM 极性与 CCR 上限）
- 下一篇：`Day17_传感器采集.md`（教材 §11.8 / §11.9 / §11.16 / §11.17）

---

**验证记录**

| 日期 | 环境一 | 命令一 | 结果一 | 环境二 | 命令二 | 结果二 | 未覆盖 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 2026-10-30 | 教材抽取文本 | 读 `ch11_drivers.txt:198-358`、`:1717-1985` | §11.6.2 与 §11.7.2 **逐字相同**（`:206-212` 与 `:242-248`）；§11.7.5 现象写"有源蜂鸣器"（`:355`）；§11.7.3 核心代码含 `Prescaler=71`、`Period=1000000/freq-1`、`Pulse=(1000000/freq-1)*duty/100`、`OCPolarity=LOW`，并带"PWM1 当极性为低,CCR<CNT,输出低电平"注释；§11.19.2 给 DRV8833 真值表四行（`0,0`＝Z,Z 惯性/快衰减；`1,1`＝L,L 制动/慢衰减）；§11.19.3 明确"A 通道 PA4/PA0 无 PWM 能力，只能停止/正转/反转"，`Pulse=speed*10` 与 `(99-speed)*10`；§11.19 **五段结构**，CubeMX 配置在 `.4` | 驱动源码 | 读 `driver_active_buzzer.c`、`driver_passive_buzzer.c`、`driver_motor.c`、`driver_motor.h` | `ActiveBuzzer_Init` **不开 GPIOA 时钟、不预置 ODR**（`Control` 低有效）；`PassiveBuzzer_Init` 用 `TIM_Base_SetConfig` 且 `DeInit`/`Init` 均被注释，`Set_Freq_Duty` 则启用二者；`PassiveBuzzer_Test` **内部又调 `LCD_Init/LCD_Clear`**；`Motor_Init` 两通道先写 `SET,SET`（制动）；`Motor_PWM_SetSpeed` 三分支，`speed>0` 用 `speed*10`、`speed<0` 用 `(99-speed)*10`；`MOTOR_B_IN2_CHANNEL TIM_CHANNEL_3` **定义了但零引用**；`Motor_SetSpeed` 钳位 `>99`/`<-99` 而注释写 `-100~100` | 三个模块的板上现象全部未观察；`MOE` 清位路径依据 HAL 源码，未示波器验证 |
| 2026-10-30 | 本仓库 `Core/` | `grep -rn "MX_TIM1_Init\|__HAL_RCC_TIM1_CLK_ENABLE\|__HAL_AFIO_REMAP_TIM2"` + 读 `tim.c:32-99`、`:208-312` | `main.c:88` 调 `MX_TIM1_Init`（**:89 TIM2、:90 TIM3、:95 `Lab_Pins_Init`**）；`tim.c:54` `HAL_TIM_Base_Init` → `:217` `__HAL_RCC_TIM1_CLK_ENABLE` ⇒ **假设"无源蜂鸣器无声"在本仓库被推翻**；`MX_TIM1_Init` 的 PSC=71/ARR=999/CCR=499 与 `PassiveBuzzer_Init` 完全相同（冗余）；BDTR `AOE=DISABLE`、`OIS1=RESET`；`HAL_TIM_MspPostInit` 的 TIM2 分支**确有** `__HAL_AFIO_REMAP_TIM2_PARTIAL_1()`（`:295`）⇒ 修正 Day15 §10 | 构建日志 | `logs/build_2026-10-29.log` | 0 error / 13 warning（全部来自课程驱动）；本篇三个驱动**零告警**；`text 15276 / data 100 / bss 3176` | 未烧写、未上板；除零行为未在目标板验证 |

