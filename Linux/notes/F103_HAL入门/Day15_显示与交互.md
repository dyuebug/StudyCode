# Day15：显示与交互（LED / OLED / 按键 / RGB）

**验证状态**：教材 §11.3（LED）、§11.4（OLED）、§11.5（按键）、§11.15（RGB 全彩）
**已逐节核对原文**（`ch11_drivers.txt:135-197`、`:1535-1559`）；
四个驱动的引脚、极性、时钟使能、函数体**逐行源码核对**（`driver_led.c`、`driver_key.c`、
`driver_color_led.c`、`driver_lcd.c`、`Core/Src/tim.c`）；构建**实测**
（GCC 14.2.1，0 error / 13 warning，全部来自课程驱动，`logs/build_2026-10-29.log`）。
**全部为上电前的静态证据**：点亮顺序、按键抖动、OLED 实际显示、RGB 混色效果
**一个都没观察过**（本机无模块、无 ST-Link）。凡"会看到……"一律转述教材，不是本仓库读数。

---

## 学习目标

学完本篇应能用代码证据回答：

1. 教材 §11.3–§11.19 十个小节的结构完全一样，但其中**哪一小节在可检索文本里是空的**？
2. 19 个 `*_Test()` 为什么"每次只能运行最前面的一个"？这条限制的机制在哪一行？
3. `StartDefaultTask` 里的 `LCD_Init(); LCD_Clear();` 为什么在 `for(;;)` **外面**？
4. `Led_Control(LED_GREEN, 1)` 实际写的是高电平还是低电平？这暴露了什么硬件接法？
5. `Led_Init` 里"先 `WritePin(RESET)` 再 `HAL_GPIO_Init`"的顺序是为了防什么？
6. `Led_Control` 的第一个参数 `which` 真的起作用吗？
7. `Key_Init` 与 `Led_Init` 在"开关 GPIO 时钟"这件事上为什么不一样？后果是什么？
8. 按键用了 `GPIO_NOPULL`，那松开时的电平由谁提供？驱动里有没有消抖？
9. `driver_lcd.c` 有 148 行却几乎没有逻辑——它为什么存在？代价是什么？
10. RGB 的三个通道为什么是 CH1=G、CH2=B、CH3=R，而不是按红绿蓝排？
11. `Pulse = c * 2000 / 255` 里的 `2000` 为什么不是 `1999`？差值会带来什么？
12. RGB 用 PA15/PB3 做输出，这两个引脚复位后归谁？本仓库处理了吗？

---

## 环境与前置条件

| 项 | 版本 / 说明 |
| --- | --- |
| 教材 | 第 11 章 §11.3（`ch11_drivers.txt:135-155`）、§11.4（`:156-176`）、§11.5（`:177-197`）、§11.15（`:1535-1559`） |
| 工程 | `experiments/F103_HAL_DriverLab/` |
| 驱动 | `Drivers/DshanMCU-F103/driver_led.{c,h}`、`driver_key.{c,h}`、`driver_color_led.{c,h}`、`driver_lcd.{c,h}`、`driver_oled.{c,h}` |
| 构建 | GCC 14.2.1 + CMake + Ninja，`arm-none-eabi`；日志 `logs/build_2026-10-29.log` |
| 关键配置 | TIM2：PSC=359 / ARR=1999（`Core/Src/tim.c:117-119`）⇒ 72 MHz / 360 / 2000 = **100 Hz** |
| 前置 | `Day02`（GPIO 与寄存器）、`Day04`（GPIO 工作模式与 HAL 源码）、`Day06`（OLED 与环形缓冲）、`Day14`（契约与冲突表） |

---

## 核心概念

### 1. 十个小节一个模板，而"配置"那一栏在文本里是空的

教材 §11.3–§11.19 每一节都是同一个四段结构：

```text
11.x.1 硬件接线      → 一句话 + 一张图
11.x.2 STM32CubeMX 配置 → "xxx，配置如下：" + 一张图
11.x.3 代码调用      → 去 freertos.c 把某个 *_Test() 取消注释
11.x.4 上机实验      → "会看到……"
```

**四段里唯一有实质文字的是第 3 段**，而它的内容几乎全是"取消注释"。
第 2 段的原文是怎样的？`§11.3.2` 全文只有一句（`ch11_drivers.txt:139-140`）：

> LED 使用 PC13 引脚，配置如下：

然后就是页码。**所有引脚模式、速度、上下拉、复用编号都在图里**——
PDF 抽成文本后，这部分信息量为零。§11.4.2、§11.5.2 前两次也一样。

这个事实决定了一件事：**"照教材配置 CubeMX"这条路，在纯文本条件下走不通。**
要回答"PC13 配成什么模式"，只能去驱动源码里找（`driver_led.c` 的 `HAL_GPIO_Init` 参数），
或者去 CubeMX 工程里找（本仓库的来源 `01_freertos_template` **没配**这些引脚，见 Day14 §lab_pins）。

所以本篇的组织方式与教材相反：
**从驱动的 `HAL_GPIO_Init` 参数反推配置，再拿教材的现象描述当验收目标。**

### 2. 19 个 `*_Test()` 全部自带死循环——这是"每次只能跑一个"的机制

教材在 §11.3.3 里给了一条硬约束（`ch11_drivers.txt:146`）：

> 因为每个测试函数中都使用到死循环，所以每次只能运行位于最前面的测试项

机制在驱动里一眼可见（`driver_led.c`）：

```c
void Led_Test(void)
{
    Led_Init();
    while (1)                       /* ← 就在这里 */
    {
        Led_Control(LED_GREEN, 1);  mdelay(500);
        Led_Control(LED_GREEN, 0);  mdelay(500);
    }
}
```

`Key_Test`、`ColorLED_Test` 也都是 `while (1)`。于是 `StartDefaultTask` 的
`for(;;) { Led_Test(); }` 里，**第一行之后的代码永远不会执行**。

这不是"注意一下"的问题，而是一种设计模式的代价：
把"初始化 + 主循环 + 退出"三段塞进一个函数，这个函数就成了一个 `main()`，
**两个这样的函数不可能共存**。对照本仓库 `projects/aviation-node/` 的分层（L4 应用 / L3 业务 / L2 判定 / L1 支撑），
那里每个模块只暴露"一步"函数，主循环只有一处——两种结构的差别是"可组合"与"不可组合"。

### 3. `LCD_Init(); LCD_Clear();` 在循环外，是在给 19 个 Test 铺路

`StartDefaultTask` 的开头两行（`ch11_drivers.txt:148`）：

```c
LCD_Init();
LCD_Clear();
for(;;) { /* 取消注释某一个 Test */ }
```

这两行**不属于任何一个 Test**，却在所有 Test 之前执行。原因要看测试函数用了什么：

| Test | 用到 LCD / OLED | 证据 |
| --- | --- | --- |
| `Led_Test` | **不用** | 只有 `Led_Control` + `mdelay` |
| `Key_Test` | **用** | `LCD_PrintString(0,0,"Key Test: ")`、`LCD_ClearLine`（`driver_key.c`） |
| `ColorLED_Test` | **用** | `LCD_PrintString(0,0,"Show Color: ")`、`LCD_PrintHex`（`driver_color_led.c`） |

所以那两行是**给下游 Test 兜底**：把"显示屏已经就绪"这个前提从每个 Test 里提出来，
变成一次性的前置条件。如果把它写进 `Key_Test` 内部，`ColorLED_Test` 就又得重写一遍。

**这条纪律值得带走**：当 N 个用例共享同一个昂贵前置时，前置应该上移到调用方，
而不是在每个用例里复制。反例是 `Led_Init` 在 `Led_Test` 里、`Key_Init` 在 `Key_Test` 里——
初化被分散到各 Test，于是"某个 Test 忘了 init"这类错误无法被统一发现。

### 4. 低电平亮：LED 与 RGB 是同一个极性，来自同一个电路原因

`driver_led.c` 的 `Led_Control`：

```c
int Led_Control(int which, int on)
{
    if (on)  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);   /* 亮 = 低电平 */
    else     HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);     /* 灭 = 高电平 */
    return 0;
}
```

`driver_color_led.c` 里是同一件事的两个表述：

```c
sConfigR.OCPolarity = TIM_OCPOLARITY_LOW;   /* 设置极性为低(硬件上低电平亮灯) */
```

驱动注释把原因写在括号里：**硬件上低电平亮灯**。这是**灌电流（sink）**接法：
`VCC → 限流电阻 → LED → GPIO`，GPIO 拉低时电流灌进芯片，灯亮。
STM32 的 IO 灌电流能力通常强于拉电流，所以板载指示灯偏爱这种接法。

两个后果，都属于"不读源码会踩"的类型：

1. **`Led_Control(which, 1)` 用逻辑 1 表示亮，用物理 0 表示亮。** 中间那一层转换就是这两行。
   直接 `HAL_GPIO_WritePin(..., GPIO_PIN_SET)` 以为能点亮，得到的是灭。
2. **上电瞬间的默认状态是灭。** `Led_Init` 里先 `WritePin(RESET)` 再配置——见 §5。

### 5. `Led_Init` 的三步顺序：时钟 / 先写电平 / 再配输出

```c
int Led_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();                                    /* ① 先开时钟 */

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);           /* ② 先把输出锁存器写成 0 */

    GPIO_InitStruct.Pin   = GPIO_PIN_13;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);                          /* ③ 最后才切成输出 */
    return 0;
}
```

三行都很关键，顺序都不能换：

| 步 | 不这么做会怎样 |
| --- | --- |
| ① 开时钟 | 对未开时钟的端口写寄存器**静默无效**——不报错，什么都不发生。这是 STM32 最经典的"代码对但没反应" |
| ② 先写 ODR | `CRL/CRH` 切成输出模式的那一瞬间，引脚立刻输出 **ODR 的当前值**。ODR 复位值是 0，如果 LED 是灌电流接法，**上电就会闪一下亮** |
| ③ 最后切模式 | 如果在 ② 之前切，中间那一小段就是"输出 + 未知电平" |

实测数值？没有——本仓库没有上板，**②这一条是手册语义 + 电路推理，不是观测**。
但它的判据很干净：**若上电时灯闪一下，就是 ②③ 顺序反了或漏了 ②。**

### 6. `Led_Control` 的第一个参数是死参数

函数声明（`driver_led.h`）说得很明白：

```c
/* which-哪个LED, 在driver_led.h里定义, 比如LED_GREEN */
int Led_Control(int which, int on);
```

而实现里 **`which` 从头到尾没被读过一次**（`driver_led.c`）。头文件也只定义了一个 `LED_GREEN 1`。

这是"接口预留但未实现"的典型形态。它的风险不是编译错误，而是**读代码的人会以为它生效**：

```c
Led_Control(LED_RED, 1);    /* 编译通过（LED_RED 未定义则会报），实际点亮的是 PC13 */
Led_Control(999, 1);        /* 也通过，也点 PC13 */
```

对照 Day14 的契约扫描：20 个驱动里 `21 int` + `52 void`，`Led_Control` 属于少数"返回 `int` 却从不返回非 0"的那一类——
**返回值也在扮演死参数**：`return 0;` 是唯一出口，调用方无法从返回值得到任何信息。
这与 Day14 §4 的"69% 函数返回 `void`"是同一个谱系：这套库的返回值约定没有真正建立。

### 7. `Key_Init` 不开 GPIOB 时钟——同一套库里两处不一致

把两个 Init 并排看，差别立刻出现：

| | `Led_Init` | `Key_Init` |
| --- | --- | --- |
| 开 GPIO 时钟 | **有** `__HAL_RCC_GPIOC_CLK_ENABLE()` | **没有** |
| 依赖 CubeMX | 注释说"为了不依赖 cubemx，此函数也实现了 GPIO 的配置" | 无此说明 |
| 引脚 | PC13 输出 | PB14 输入 |
| 上下拉 | `GPIO_NOPULL` | `GPIO_NOPULL` |

`Key_Init` 不开关钟，意味着它**假定有人已经开了 GPIOB**。谁开？在本仓库里是
`Core/Src/lab_pins.c`（Day14 补的 8 个纯 GPIO，其中包含 PB14）；在课程工程里是 CubeMX 的 `gpio.c`。

后果是一条**顺序敏感**的规则：`Key_Init()` 必须在 GPIOB 时钟使能之后调用，
否则 `HAL_GPIO_Init` 写进去的配置**静默无效**（同 §5 第 ① 行）。
而 `Key_Init` 的返回值是 `void`——**它没有任何方式告诉你这件事失败了**。

对照 Day07 的互斥量"给不进去返回 `pdFALSE`"、Day08 的事件组"返回 `pdTRUE` 才算成功"：
**一个不返回状态的初始化函数，等于把错误检测责任全推给调用方的"记得"**。

### 8. `Key_Read` 的极性与"谁提供上拉"

```c
#define KEY_GPIO_GROUP GPIOB
#define KEY_GPIO_PIN   GPIO_PIN_14

void Key_Init(void) {
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;      /* 不用内部上拉/下拉 */
    HAL_GPIO_Init(KEY_GPIO_GROUP, &GPIO_InitStruct);
}

int Key_Read(void) {
    if (GPIO_PIN_RESET == HAL_GPIO_ReadPin(KEY_GPIO_GROUP, KEY_GPIO_PIN))
        return 1;        /* 低电平 = 按下 */
    else
        return 0;        /* 高电平 = 松开 */
}
```

三条推论：

1. **按下时引脚被拉到地**（低电平）⇒ 按键另一端接 GND，低有效。
2. **`GPIO_NOPULL` ⇒ 松开时的"高"必须由外部上拉电阻提供。** 如果板上没有外部上拉，
   松开时引脚**悬空**，读回值不确定——可能随机在高/低之间跳，表现为"按键自己乱动"。
   驱动没写这件事，因为它假定底板已经接了上拉。
3. **没有消抖。** `Key_Read` 是一次采样，`Key_Test` 直接用它驱动 LED 与显示。
   机械按键的抖动窗口是 **5–20 ms 量级**，而 `Key_Test` 循环一次只需几十微秒，
   所以一次按下会让 `Led_Control` 在一次按键期间反复翻转。

第 3 条是本篇**最值得记住的一个缺口**：教材 §11.5.4 说"按下按键 LED 灯亮起，松开按键 LED 灯熄灭"
（`ch11_drivers.txt:194`）——这句话把抖动整个隐藏了。真板上更可能看到的是
**按下瞬间的一次闪烁**，或者看起来"正常"（因为人眼对 10 ms 以下的翻转不敏感）。

判据级的结论：**"看起来正常"不能证明没有抖动。** 要证明得用示波器或逻辑分析仪抓 PB14 波形
（对应本仓库 `Day20` 的逻辑分析仪项目，那里有完整的方法论）。

### 9. `driver_lcd.c` 是 148 行的纯转发层——它换来什么

`driver_lcd.c` 的实现几乎每一行都是同一个形状：

```c
void LCD_ClearLine(uint8_t x, uint8_t y)   { OLED_ClearLine(x, y); }
int  LCD_PrintHex(uint8_t x, uint8_t y, uint32_t val, uint8_t pre) { return OLED_PrintHex(x, y, val, pre); }
int  LCD_PrintString(uint8_t x, uint8_t y, const char *str)        { return OLED_PrintString(x, y, str); }
```

`driver_lcd.h` 也有 8 个对应声明。**718 行的 OLED 驱动 + 148 行的转发层**，
转发层本身不产生任何新行为。

它换来的是**名字**：所有 `*_Test()` 里写的都是 `LCD_xxx`，而实际硬件是 I2C OLED。
这样做的代价与收益：

| | 收益 | 代价 |
| --- | --- | --- |
| 名字解耦 | 以后换屏（比如真并口 LCD）只改 `driver_lcd.c` 的实现 | 多一层函数调用（编译器多半内联） |
| 阅读误导 | —— | 读者看到 `LCD` 会以为是并口屏，要去翻 `driver_lcd.c` 才发现是 I2C |
| 一致性 | 19 个 Test 都用同一套名字 | 两套 API 并存（`OLED_xxx` 与 `LCD_xxx` 都能调），**谁该用哪套没有约定** |

最后一行是实际风险：`driver_oled.h` 与 `driver_lcd.h` 同时可见时，**两个名字都能用**。
本仓库的 `driver_key.c` 选了 `LCD_PrintString`，而 `driver_oled.c` 内部当然是 `OLED_*`。
一套驱动里两种命名并存，是"抽象层没划清"的典型症状。

还有一个返回值细节值得记：`LCD_PrintString` 的文档写"返回值：打印了多少个字符"，
`Key_Test` 正是靠它做列偏移：

```c
len = LCD_PrintString(0, 2, "Key : ");
len += LCD_PrintString(len, 2, val ? "Pressed" : "Released");
LCD_ClearLine(len, 2);          /* 把剩下的列清掉，防残影 */
```

**`LCD_ClearLine(len, 2)` 这一步不能省。** OLED 没有"背景色"概念，`PrintString` 只写字符点阵，
上一轮更长的字符串会留下尾巴。所以正确写法是"写完 → 清剩余列"。
保守化一步：`len` 是 `int`，传进去的参数是 `uint8_t`——隐式窄化，行长超过 255 才会出问题。

### 10. RGB 的三通道映射：CH1=G、CH2=B、CH3=R

`driver_color_led.c` 的映射定义：

```c
#define CHANNEL_RED   TIM_CHANNEL_3
#define CHANNEL_GREEN TIM_CHANNEL_1
#define CHANNEL_BLUE  TIM_CHANNEL_2
```

不是 R/G/B 顺序。原因是**引脚与通道的对应由硬件连线决定**，不是自由选择：

| 颜色 | 引脚 | 通道 | 来源 |
| --- | --- | --- | --- |
| G | PA15 | TIM2_CH1 | 教材 §11.15.2（`ch11_drivers.txt:1544`） |
| B | PB3 | TIM2_CH2 | 同上 |
| R | PA2 | TIM2_CH3 | 同上 |

TIM2 的四个通道固定绑在 PA0/PA1/PA2/PA3（CH1–CH4）上，但**重映射**可以把 CH1/CH2
搬到 PA15/PB3。也就是说这张表背后一定有一次 `AFIO` 重映射配置——
本仓库的 `Core/Src/tim.c` 里只能看到 `MX_TIM2_Init()`，重映射是否配齐需要对照 CubeMX 源工程。

**这是本篇又一个"配置在文本里为空"的实例**：教材说"这 3 个引脚被分别配置为 TIM2_CHN1/CHN2/CHN3"，
但没说怎么配的。而"能不能用 PA15"恰恰取决于 `__HAL_AFIO_REMAP_TIM2_PARTIAL_1` 之类的调用。

### 11. PA15 / PB3 / PB4 复位后归 JTAG，本仓库没配

这条在 Day14 已作为缺陷②记过，这里给出它在**本模块**的具体后果：

STM32F103 复位后 `AFIO_MAPR` 的 `SWJ_CFG = 000`，即 **JTAG + SW 全开**：

| 引脚 | 复位功能 | 想当 GPIO 需要 |
| --- | --- | --- |
| PA13 | JTMS / SWDIO | 不能释放（调试口） |
| PA14 | JTCK / SWCLK | 不能释放（调试口） |
| PA15 | **JTDI** | `SWJ_CFG` 设为"JTAG 禁用、SW 使能"或"全禁用" |
| PB3 | **JTDO** | 同上 |
| PB4 | **NJTRST** | 同上 |

RGB 全彩用 **PA15 与 PB3** 两个。也就是说：**不配 `SWJ_CFG`，这两个通道不会输出 PWM**，
表现为"蓝色和绿色不亮/常亮，只有红色正常"（红色用 PA2，不受影响）。

Day14 查证过：本仓库的来源工程 `01_freertos_template` **没有任何 `.ioc`，也没有
`__HAL_AFIO_REMAP_SWJ_*` 调用**。所以这条在真板上的预期现象是：

> `LAB_TEST` 选 RGB 时，只有红色随颜色变化，绿/蓝恒为某一定值。

这是**可证伪的预测**（Day14 已按同一格式记录另外两条）。**先证伪，再改代码，改后复测同一现象**——
这个顺序不能反，否则"改好了"永远无法归因。

### 12. `Pulse = c * 2000 / 255` 里的 2000 不是笔误

`ColorLED_Set` 把 8 位通道值换算成 CCR：

```c
sConfigR.Pulse = r * 2000 / 255;
```

而 `Core/Src/tim.c:119` 里 `htim2.Init.Period = 1999`（ARR = 1999）。看起来"2000 > 1999，超了"。
其实**这正是对的**，因为：

- ARR = 1999 ⇒ 计数器 `CNT` 的范围是 `0 … 1999`，**一个周期共 2000 个计数**。
- PWM1 + `OCPolarity = LOW` ⇒ `CNT < CCR` 时输出**有效电平（低）**。
- `r = 255` ⇒ `Pulse = 2000` ⇒ `CNT (0…1999)` **恒小于** CCR ⇒ 整个周期恒有效 ⇒ **全亮**。

如果按"直觉"写成 `r * 1999 / 255`，`r = 255` 时 `Pulse = 1999`，
那么 `CNT = 1999` 那一拍 `CNT < CCR` 不成立 ⇒ 全亮时漏掉一个 tick ⇒ 占空比 99.95%。
**肉眼完全看不出来，但它就是"不满"**。

所以规则是：**CCR 的上限是 `Period + 1`，不是 `Period`**。
这条在调光、舵机、电机里都会再遇到（Day16 的电机 `Pulse` 换算同一公式）。

另外注意 `HAL_TIM_PWM_ConfigChannel` **不做裁剪**：`Pulse = 2000` 会原样写进 CCR。
CCR > ARR 是合法的，硬件语义就是"永不翻转"。

### 13. `ColorLED_Set` 的 Stop → Config → Start 三步

```c
HAL_TIM_PWM_Stop(g_HPWM_ColorLED, CHANNEL_RED);   /* ... 三个通道 */
HAL_TIM_PWM_ConfigChannel(g_HPWM_ColorLED, &sConfigR, CHANNEL_RED);  /* ... 三个 */
HAL_TIM_PWM_Start(g_HPWM_ColorLED, CHANNEL_RED);  /* ... 三个 */
```

每次改颜色都停一次再起一次。**能不能只改 CCR？** 能——`__HAL_TIM_SET_COMPARE(&htim2, ch, val)`
直接写寄存器，不改模式、不重启通道。那驱动为什么每次都重配？

因为它把"模式、极性、快速模式、Pulse"打包成一个完整的 `TIM_OC_InitTypeDef` 重新下发。
好处是**状态唯一**（不依赖上一次留下了什么），坏处是每次改色都多两次 Stop/Start，
在 100 Hz 的 PWM 上表现为**换色瞬间可能出现一次不完整的周期**。

判据级的读法：这不是 bug，是**取舍写在了代码里而没写进文档**。
如果以后要做"呼吸灯"（每 10 ms 改一次 CCR），这个写法就会变成性能问题——
那时应该换成 `__HAL_TIM_SET_COMPARE`。**同一个函数在不同刷新率下结论不同**，这是典型的"适用边界"问题。

### 14. 100 Hz 调光频率是怎么来的

`Core/Src/tim.c:117-119`：

```c
htim2.Init.Prescaler = 359;
htim2.Init.Period    = 1999;
```

`72 MHz / (359+1) / (1999+1) = 72e6 / 360 / 2000 = 100 Hz`。

为什么是 100 Hz？两个约束夹出来的：

| 约束 | 要求 |
| --- | --- |
| 不能太低 | 低于约 **60–80 Hz** 时人眼能看出闪烁；摄影/录像会更明显（拍出滚动条纹） |
| 不能太高 | 频率越高，每个亮度级的**时间分辨率越差**；还要留出 8 位（256 级）的调节范围 |

100 Hz + 2000 个计数 ⇒ 每级亮度约 8 个计数，**8 位精度刚好够用**。
若改成 1 kHz，同样 2000 个计数只够 200 级，8 位颜色就得丢低位。

这条与 Day13 定时器笔记里的 PWM 结论同源：**PSC 定时间分辨率，ARR 定频率与占空比精度，两者与目标频率构成一个三角约束**。

### 15. 教材 §11.5.2 把"按键"写成了"LED"

`ch11_drivers.txt:182` 原文：

> LED 使用 PB14 引脚，配置如下：

而 §11.5 讲的是**按键**（标题："按键驱动使用方法"），PB14 也是按键引脚。
这是从 §11.3.2（"LED 使用 PC13 引脚"）复制后忘了改主语的痕迹。

危害不大（读者看上下文能反应过来），但它和 Day14 记录的"教材内部对占空比有两个定义"是同一类问题：
**同一份文档里被复制的段落，主语/口径会悄悄漂移。**

---

## 系统调用/接口卡片

### `int Led_Init( void )`

作用：把 PC13 配成推挽输出，并把输出锁存器预置为低（灌电流接法下 = 灭）。

参数：无。

返回值和错误：恒返回 `0`，**没有失败路径**（见 §6）。

资源所有权：打开 GPIOC 时钟（`__HAL_RCC_GPIOC_CLK_ENABLE()`），无释放函数——
本库 20 个驱动**零 `DeInit`**（Day14 §4）。

线程/进程安全：裸机/单任务下调用；FreeRTOS 下建议在创建任务前调用一次。

最小示例：

```c
if (Led_Init() != 0) { /* 本库不会走到这里 */ }
Led_Control(LED_GREEN, 1);   /* 亮（写 RESET） */
```

常见误用：① 以为 `which` 参数生效；② 在上电闪灯问题上忽略"先写 ODR 再切模式"的顺序（§5）。

### `int Led_Control( int which, int on )`

作用：点亮/熄灭板载 LED。

参数：`which` —— **被忽略**；`on` —— 1 亮、0 灭（逻辑值，与物理电平相反）。

返回值和错误：恒 `0`。

线程/进程安全：直接写 ODR，是原子的单次写；多任务共用也只会"最后写的赢"，不会半状态。

常见误用：① 传入未定义的颜色常量期待点别的灯；② 以为 `on=1` 是高电平。

### `void Key_Init( void )`

作用：把 PB14 配成输入，`Pull = GPIO_NOPULL`。

参数：无。

返回值和错误：**`void`——失败无法得知**（时钟未开时静默无效，§7）。

资源所有权：**不打开 GPIOB 时钟**，依赖调用方。

常见误用：① 在 GPIOB 时钟使能之前调用；② 期望内部上拉（没有，需要外部电阻，§8）。

### `int Key_Read( void )`

作用：读一次 PB14，低电平返回 1（按下），高电平返回 0（松开）。

参数：无。

返回值和错误：`1` / `0`。**没有消抖、没有边沿检测、没有长按**。

线程/进程安全：单次读，可任意调用。

最小示例（本仓库 `Key_Test` 的核心三行）：

```c
val = Key_Read();
Led_Control(LED_GREEN, val);                       /* 按下就点亮 */
len = LCD_PrintString(0, 2, val ? "Pressed" : "Released");
```

常见误用：① 把它当"按键事件"用（它只返回**当前电平**，按住期间每次调用都是 1）；
② 不做消抖就驱动有副作用的动作（计数、切换状态机）；
③ 在 `GPIO_NOPULL` 且无外部上拉时读，得到悬空值。

### `void ColorLED_Init( void )`

作用：调用 `MX_TIM2_Init()` 初始化 TIM2 的 PWM 基座。

参数：无。

返回值和错误：`void`；`MX_TIM2_Init` 内部失败时无处上抛。

资源所有权：TIM2 句柄 `htim2` 是全局的（`extern TIM_HandleTypeDef htim2;`）。

常见误用：以为它启动了通道——**通道启动在 `ColorLED_Set` 里**，`Init` 只配基座。

### `void ColorLED_Set( uint32_t color )`

作用：设置 RGB 三通道占空比。`color` 格式 `0x00RRGGBB`。

参数：`color` 的高 8 位被忽略（只取 bit0–23）。

返回值和错误：`void`。

资源所有权：每次调用都会 Stop → Config → Start 三个通道（§13）。

最小示例（本仓库 `ColorLED_Test`）：

```c
uint32_t color = 0;
ColorLED_Set(color);          /* 黑（全 0 ⇒ 三通道 Pulse=0 ⇒ 恒无效 ⇒ 灭） */
color += 200000;              /* 步进 = 0x030D40，不是"每通道 +1" */
color &= 0x00ffffff;
```

常见误用：① 以为 `+= 200000` 是线性渐变（它是**在 24 位整数上加**，所以 R/G/B 分量各自跳变）；
② 把 `Pulse` 上限写成 `Period`（§12）；③ 每次改色都重配模式却在做高频呼吸灯（§13）。

### `int LCD_PrintString( uint8_t x, uint8_t y, const char *str )`（→ `OLED_PrintString`）

作用：在 OLED 的 `(x, y)` 处打印字符串；`y` 是**页**（8 像素一行）。

参数：`x` 列（0–15）、`y` 页（0–7）、`str`。

返回值和错误：**打印的字符数**（`Key_Test` 靠它算列偏移）。

常见误用：① 不清残留列（要配 `LCD_ClearLine(len, y)`，§9）；
② 把返回值当成功标志（它恒为长度，不表示显示成功）。

---

## 最小可运行实验

本篇不新写实验。`F103_HAL_DriverLab` 的 `LAB_TEST` 就是实验选择器：

| `LAB_TEST` | 覆盖教材 | 现象（教材描述，**本仓库未观察**） |
| --- | --- | --- |
| 1（LED） | §11.3 | 亮 500 ms、灭 500 ms，循环 |
| 12（OLED/LCD） | §11.4 | 屏上显示 `LCD_Test` 指定的字符 |
| 13（Key） | §11.5 | 屏显"Pressed/Released"，按键控制 LED |
| 8（ColorLED） | §11.15 | 每 1 s 换色，屏显当前 hex |

`LAB_TEST` 用 `#if` 而不是运行期 `switch`，理由见 Day14 §5：
`switch` 会让 19 个 `Test` 全成为可能调用点（`text` 10 936 → 33 928），
`#if` 下 `nm` 只剩被选中的那一个。

选择方式：

```c
/* Core/Src/main.c 或 CMakeLists 的编译期定义 */
#define LAB_TEST  13
```

---

## 编译与运行

```bash
cd experiments/F103_HAL_DriverLab
cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake
cmake --build build

# 体积与符号核对（Day14 的口径）
arm-none-eabi-size build/f103_hal_driverlab.elf
arm-none-eabi-nm build/f103_hal_driverlab.elf | grep -i "_Test"
```

**烧写与调试：本仓库只做到"命令能拼对"**，未实际烧写（无 ST-Link）。
`flash` 目标见工程 README §8。

---

## 结果观察

### 构建（实测，`logs/build_2026-10-29.log`）

```text
0 error / 13 warning（13 条全部来自课程驱动，本工程新增文件零告警）
text 15276 / data 100 / bss 3176
Flash 23.5 %，RAM 静态 16.0 %（其中 1540 B 是 ._user_heap_stack 预留，非消耗）
```

13 条 warning 的分部：`driver_ds18b20.c` 3 条 `-Wparentheses`、`driver_oled.c` 2 条
`-Wchar-subscripts` + 8 条 `-Wunused-function`（`OLED_WriteData`、`OLED_H_Scroll` 等 8 个未被调用的私有函数）。

**与本篇相关的部分：这四个驱动（LED / Key / ColorLED / OLED-LCD）没有产生任何 warning。**
"没 warning"不等于"对"——§6 的死参数、§7 的不一致、§8 的无消抖都不会触发编译器。

### 板上现象（**全部未验证**）

教材 §11.3.4 / §11.4.4 / §11.5.4 / §11.15.4 的"会看到……"（`ch11_drivers.txt:152`、`:173`、`:194`、`:1557`）
在本仓库**一条都没有观察过**。按 §11 的分析，真板上最可能出现的偏差是：

| 模块 | 教材说 | 本仓库的预期风险（源码推论） |
| --- | --- | --- |
| LED | 500 ms 亮灭 | 可能有上电闪一下（若 ②③ 顺序被改了） |
| Key | 按下亮、松开灭 | **抖动导致按下瞬间闪烁**（无消抖，§8） |
| ColorLED | 每秒换色 | **绿/蓝可能恒不变化**（PA15/PB3 未配 `SWJ_CFG`，§11） |
| OLED | 显示字符 | 若 `MX_I2C1_Init` 未调用则整屏全黑（Day14 §3 的依赖表） |

**这张表不是结论，是待证伪的预测。** 上板后要逐条记录下来，包括"预测错了"的那些。

### 本篇**没有**验证的东西

- 四种模块的**任一板上现象**（点灯、闪烁、按键、混色、OLED 显示）。
- LED 上电闪灯的**存在性**——§5 的 ② 完全来自手册与电路推理。
- 按键抖动幅度与持续时间——需要示波器/逻辑分析仪，本机没有。
- PA15/PB3 的 `SWJ_CFG` 实际状态——`01_freertos_template` 里没有 `.ioc`，只能推断"没配"。
- TIM2 在 PA15/PB3 上的**重映射是否配齐**（§10）——本仓库 `tim.c` 只看到 `MX_TIM2_Init`。
- `Pulse = 2000` 时硬件是否真的恒有效（§12）——只能靠参考手册语义，没有示波器验证。
- `LCD_PrintString` 的返回值是否等于"实际点亮的字符数"（日光下看不清时的行为未知）。
- `which` 参数"被忽略"这件事**是否只是当前版本**——没有 git 历史可查（课程库是整包分发的）。

---

## 常见错误和陷阱

| 陷阱 | 症状 | 根因 | 处理 |
| --- | --- | --- | --- |
| 写 `GPIO_PIN_SET` 以为能点亮 | 灯不亮 | 灌电流接法，低电平亮（§4） | 用 `Led_Control(..., 1)` |
| 改 `Led_Control` 的 `which` 想换灯 | 无变化 | 参数未使用（§6） | 直接改用对应驱动的 API |
| `Key_Init` 前没开 GPIOB 时钟 | 按键永为同一个值 | `HAL_GPIO_Init` 对未开时钟端口静默无效（§7） | 先 `__HAL_RCC_GPIOB_CLK_ENABLE()` |
| 期望按键有内部上拉 | 松开时读数乱跳 | `Pull = GPIO_NOPULL`（§8） | 确认底板有外部上拉；或改成 `GPIO_PULLUP` |
| 不做消抖就数按键次数 | 一次按下计多次 | 无消抖，机械抖动 5–20 ms（§8） | 加 10–20 ms 延时确认，或用定时器采样 |
| 把 `Key_Read` 当事件 | 按住期间重复触发 | 它返回**当前电平**，不是边沿 | 自己做"上次值 → 本次值"的比较 |
| `LCD_PrintString` 后不清行 | 长字符串留下残影 | OLED 无背景色（§9） | 配 `LCD_ClearLine(len, y)` |
| 两个 Test 同时取消注释 | 只有前面那个生效 | 每个 `*_Test` 自带 `while(1)`（§2） | 一次只留一个 |
| `Pulse` 写成 `c*Period/255` | 最亮时略暗（不可见） | CCR 上限是 `Period+1`（§12） | 用 `Period+1` |
| 用 `Pulse` 做高频呼吸灯 | 换色瞬间有跳变 | 每次 Stop→Config→Start（§13） | 高频改色用 `__HAL_TIM_SET_COMPARE` |
| 以为 RGB 亮 = `SWJ_CFG` 已配 | 绿/蓝不亮 | PA15/PB3 复位归 JTAG（§11） | 显式配 `SWJ_CFG` 释放 JTAG 引脚 |
| 照教材 §11.5.2 找"LED 的 PB14" | 找不到 | 教材笔误，PB14 是按键（§15） | 以驱动宏定义为准 |

---

## 故障排查方法

1. **"LED 完全没反应。"** 顺序：① `Led_Init` 有没有被调用；② GPIOC 时钟开了吗（对未开时钟的端口写是**静默无效**）；
   ③ 用万用表量 PC13 静态电平——若恒高，说明 `Led_Control(...,1)` 没被执行或极性搞反。
2. **"上电时灯闪一下。"** 查 `Led_Init` 里 `WritePin(RESET)` 是否在 `HAL_GPIO_Init` **之前**（§5）。
3. **"按键一直显示 Pressed。"** 三种可能：① 引脚被外部拉死在低；② `GPIO_NOPULL` 且没有外部上拉，
   读回悬空值恰好为低；③ EXTI 或别的模块把 PB14 复用走了。先量静态电平，再查时钟。
4. **"一次按下，状态跳了好几次。"** 抖动。判据不是"看起来稳不稳"，而是**在 PB14 上加逻辑分析仪看 20 ms 窗口**。
   软件侧的临时手段：读两次、间隔 10–20 ms、一致才认。
5. **"OLED 全黑。"** 先查 `MX_I2C1_Init()` 是否被调用（Day14 §3 的依赖表），
   再查 I2C 速率（100 kHz 标准模式）、上拉电阻、从机地址 `0x78`。
6. **"RGB 只有红色能变。"** 直接对应 §11 的 `SWJ_CFG`：绿用 PA15（JTDI）、蓝用 PB3（JTDO）。
   确认有没有 `__HAL_AFIO_REMAP_SWJ_NOJTAG()` 之类的调用。
7. **"RGB 换色时偶发一次闪烁。"** §13 的 Stop→Config→Start 三步造成的半个周期。
   连续改色（呼吸灯）时应改 `__HAL_TIM_SET_COMPARE`。
8. **"最亮时颜色偏暗/偏冷。"** 查 `Pulse` 的换算上限是不是写成了 `Period`（§12）；
   再查 `OCPolarity` 是否与硬件接法一致（低电平亮）。

---

## 练习题

### 基础题

1. 教材十个小节的四段结构里，哪一段在可检索文本里是空的？这对"照着书配 CubeMX"意味着什么？
2. 为什么 19 个 `*_Test()` 只能跑一个？指出实现这一限制的具体代码形态。
3. `StartDefaultTask` 里 `LCD_Init(); LCD_Clear();` 为什么在 `for(;;)` 外面？举出两个用到它的 Test。
4. 灌电流接法下，`Led_Control(LED_GREEN, 1)` 写的是高电平还是低电平？为什么这样接？
5. RGB 的三个通道为什么不是 R/G/B 顺序？给出引脚与通道的对应表。

### 改错题

6. 下面这段初始化 LED，指出两处问题：

   ```c
   GPIO_InitTypeDef g = {0};
   g.Pin = GPIO_PIN_13; g.Mode = GPIO_MODE_OUTPUT_PP; g.Pull = GPIO_NOPULL;
   HAL_GPIO_Init(GPIOC, &g);
   HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
   ```

7. 下面这段按键处理，指出三处问题：

   ```c
   if (Key_Read()) {
       g_count++;                      /* 统计按下次数 */
       Led_Control(LED_GREEN, 1);
   }
   ```

8. 下面这段 RGB 设置，指出问题并给出修正：

   ```c
   sConfig.Pulse = r * 1999 / 255;     /* ARR = 1999 */
   ```

### 综合题

9. 用 §12 的推导回答：若把 TIM2 的 `Period` 改成 999（100 Hz → 200 Hz），
   `ColorLED_Set` 里的换算公式要怎么改？亮了 255 时 CCR 是多少？为什么不能直接沿用 `*2000/255`？

10. 设计一个**可判定**的消抖方案，要求：① 说明判据（不是"看起来稳"）；
    ② 给出抖动窗口的量级依据；③ 说明为什么"加 10 ms 延时"在 RTOS 里可能比在裸机里更危险
    （提示：Day03 的任务阻塞与 Day04 的临界区）。

11. 教材 §11.5.4 说"按下按键 LED 亮起，松开熄灭"。请用 §8 与 §11 的分析构造**两个反例**：
    ① 一个说明"现象正常但实现有缺陷"；② 一个说明"现象异常但去错了地方找原因"。
    每个反例都要写清"要观测什么才能区分"。

12. 把 `driver_lcd.c` 这层转发删掉，直接让 19 个 Test 调用 `OLED_xxx`。列出：
    ① 收益（少一层、少 148 行）；② 代价（换屏时要改 N 处）；③ 你会怎么选，理由是什么。

---

## 验收标准

- [ ] 能说出教材十个小节里哪一段在文本中为空，以及由此导致的检索策略改变。
- [ ] 能指出"每次只能跑一个 Test"的机制（`while(1)`）与它暴露的设计问题（不可组合）。
- [ ] 能解释 `LCD_Init()` 在循环外的原因，并举出两个依赖它的 Test。
- [ ] 能说明 LED 与 RGB 同为低电平亮的电路原因（灌电流），以及极性转换发生在哪两行。
- [ ] 能说出 `Led_Init` 三步顺序各自防的是什么，尤其是"先写 ODR"防上电闪灯。
- [ ] 能指出 `Led_Control` 的 `which` 是死参数，并说明这类"预留接口"的风险。
- [ ] 能对比 `Led_Init` 与 `Key_Init` 在时钟使能上的不一致及其后果（静默无效）。
- [ ] 能解释 `GPIO_NOPULL` + 低有效 ⇒ 依赖外部上拉，以及驱动**没有消抖**这一缺口。
- [ ] 能说明 `driver_lcd.c` 这层转发换来什么、代价是什么、两套命名并存的隐患。
- [ ] 能给出 RGB 三通道的引脚/通道对应表，并解释为什么不是 R/G/B 顺序。
- [ ] 能推导 `Pulse = c*2000/255` 中 `2000` 的来历（`Period+1`），以及写成 1999 的偏差。
- [ ] 能说明 PA15/PB3 复位归 JTAG，未配 `SWJ_CFG` 时的**可证伪现象**。
- [ ] 能说出 100 Hz 调光频率的两个约束，以及 PSC/ARR/精度的三角关系。
- [ ] 能列出本篇**未验证**的八项，并说明它们全部止于上电前。

---

## 延伸阅读

- 教材第 11 章：
  - §11.3 LED：`ch11_drivers.txt:135-155`（§11.3.2 的"配置如下"后为空）
  - §11.4 OLED：`:156-176`
  - §11.5 按键：`:177-197`（**`:182` 把"按键"写成"LED"**）
  - §11.15 RGB：`:1535-1559`（`:1544` 给出 PA15/PB3/PA2 与 CH1/CH2/CH3 的对应，但重映射方式未写）
- 驱动源码：`Drivers/DshanMCU-F103/`
  - `driver_led.c` — `Led_Init`（三步顺序、唯一一处 GPIO 时钟使能）、`Led_Control`（死参数）
  - `driver_key.c` — `Key_Init`（无时钟、`NOPULL`）、`Key_Read`（低有效）、`Key_Test`（无消抖）
  - `driver_color_led.c` — 通道映射、`OCPolarity=LOW`、`Pulse=c*2000/255`、Stop→Config→Start
  - `driver_lcd.c` — 148 行纯转发层；`driver_lcd.h:78` 的 `LCD_PrintString` 声明
- 平台配置：`Core/Src/tim.c:102-150`（`MX_TIM2_Init`：PSC=359、ARR=1999 ⇒ 100 Hz）
- 本仓库前置：`Core/Src/lab_pins.c`（8 个纯 GPIO，含 PB14）
- 相关笔记：`Day02`（GPIO 寄存器）、`Day04`（工作模式与 HAL 源码）、`Day06`（OLED 与环形缓冲）、
  `Day09`（I2C 协议）、`Day13`（定时器与 PWM）、`Day14`（契约、冲突表、三条缺陷）、
  `Day20`（逻辑分析仪——本篇 §8 的消抖判据需要它）
- 下一篇：`Day16_声音与执行器.md`（教材 §11.6 / §11.7 / §11.19）

---

**验证记录**

| 日期 | 环境一 | 命令一 | 结果一 | 环境二 | 命令二 | 结果二 | 未覆盖 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 2026-10-30 | 教材抽取文本 | 读 `ch11_drivers.txt:135-197`、`:1535-1559` | 四节均为此结构：接线 / CubeMX 配置 / 代码调用 / 上机实验；**§11.3.2 与 §11.4.2 的"配置如下"后无任何文字参数**（信息在图中）；每节都强调"每个测试函数中都使用到死循环，所以每次只能运行位于最前面的测试项"（`:146`/`:167`/`:188`/`:1548`）；`StartDefaultTask` 里 `LCD_Init(); LCD_Clear();` 在 `for(;;)` 外（`:148`/`:153`/`:190`/`:1553`）；**`:182` 把"按键"写成"LED 使用 PB14"**；§11.15.2 给出 PA15/PB3/PA2 = CH1/CH2/CH3（`:1544`） | 驱动源码 | `cat`/`sed`/`grep` 读 `driver_led.c`、`driver_key.c`、`driver_color_led.c`、`driver_lcd.c`、`driver_lcd.h`、`driver_oled.h`、`Core/Src/tim.c` | `Led_Control` 中 `on=1` 写 `GPIO_PIN_RESET`（低电平亮）；`which` **从未被读**；`Led_Init` 是 20 个驱动里唯一的 GPIO 时钟使能，且"先 `WritePin(RESET)` 再 `HAL_GPIO_Init`"；`Key_Init` **不开 GPIOB 时钟**、`Pull=GPIO_NOPULL`、`Key_Read` 低有效、**无消抖**；`Key_Test` 依赖 `LCD_PrintString`；`driver_lcd.c` 148 行纯转发；通道映射 `CHANNEL_RED=TIM_CHANNEL_3` / `GREEN=1` / `BLUE=2`；`Pulse=c*2000/255` 而 `Period=1999`；`OCPolarity=TIM_OCPOLARITY_LOW`；TIM2 PSC=359/ARR=1999 ⇒ 100 Hz | 四种模块的板上现象全部未观察；`SWJ_CFG` 未配为源码推断 |
| 2026-10-30 | WSL2 + GCC 14.2.1 + CMake/Ninja | `cmake --build build`（`LAB_TEST=0`） | `logs/build_2026-10-29.log`：0 error / 13 warning，13 条**全部来自课程驱动**（`driver_ds18b20` 3 条 `-Wparentheses`、`driver_oled` 2 条 `-Wchar-subscripts` + 8 条 `-Wunused-function`）；本篇四个驱动**零告警**；`text 15276 / data 100 / bss 3176` | 本仓库源码 | `grep LCD_PrintString` | 定义在 `driver_lcd.c:84`、声明在 `driver_lcd.h:78`（**曾误判为"定义有声明无"，已核实为假**）；`driver_lcd.c` 每个函数体都是一行 `return OLED_xxx(...)` | 未烧写、未调试、未上板；`Pulse=2000` 的硬件行为靠手册语义 |
