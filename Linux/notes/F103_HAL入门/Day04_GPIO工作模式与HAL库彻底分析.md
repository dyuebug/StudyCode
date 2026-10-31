# Day4：GPIO 工作模式与 HAL 库彻底分析

**验证状态**：模式翻译表与锁存行为**已核对源码**（`stm32f1xx_hal_gpio.c` 的 `HAL_GPIO_Init`，
`stm32f1xx_hal_gpio.h:140-154`）；驱动引脚与极性**已核对源码**（`driver_key.c`、`driver_active_buzzer.c`、
`driver_light_sensor.c`）；"六驱动只有 LED 自开时钟"为**逐文件统计实测**；**板上电平现象全部未验证**。
教材侧内容来自《HAL快速入门与项目实战》第 5.7–5.8 节。

---

## 学习目标

学完本篇应能用代码证据回答：

1. GPIO 有哪 8 种模式，输入为什么必须解决"悬空"问题，输出为什么区分推挽与开漏。
2. `GPIO_InitTypeDef` 的 `Mode`/`Pull`/`Speed` 三个字段，分别被翻译成 `CRL`/`CRH` 的哪几个位。
3. **上拉是通过写 `ODR` 实现的**，不是"在 CR 里选上拉"——为什么，怎么从源码看出来。
4. 课程 6 个"自己配引脚"的驱动里，有几个真的能独立工作；剩下的缺什么。
5. 同一个模块为什么教材读 `PB11`、驱动库读 `PA3`，两者是不是矛盾。

---

## 环境与前置条件

| 项 | 要求 |
| --- | --- |
| 工程 | `experiments/F103_HAL_DriverLab/` |
| 参考 | HAL 源码 `Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio.c` |
| 硬件 | 可选。本篇结论以源码与反汇编为主 |
| 前置 | Day2（四步操作）、Day3（结构体偏移与位运算） |

---

## 核心概念

### 1. 八种模式：四个输入 + 四个输出

教材 5.7.1 节给了这张分类，逐项说明"什么时候用"：

| 模式 | `Mode` 宏 | 典型场景 | 关键行为 |
| --- | --- | --- | --- |
| 浮空输入 | `GPIO_MODE_INPUT` + `NOPULL` | 外部已有上下拉的信号 | 高阻，电平由外部决定 |
| 上拉输入 | `GPIO_MODE_INPUT` + `PULLUP` | 按键接地型（内部上拉） | 默认高，按下变低 |
| 下拉输入 | `GPIO_MODE_INPUT` + `PULLDOWN` | 按键接 VCC 型 | 默认低，按下变高 |
| 模拟输入 | `GPIO_MODE_ANALOG` | ADC 采样 | 断开数字输入通路 |
| 推挽输出 | `GPIO_MODE_OUTPUT_PP` | LED、蜂鸣器 | 高/低都由芯片主动驱动 |
| 开漏输出 | `GPIO_MODE_OUTPUT_OD` | 单总线、I2C、多主共享线 | 只能拉低；高电平靠外部上拉 |
| 复用推挽 | `GPIO_MODE_AF_PP` | UART_TX、SPI_SCK | 输出由外设而非 ODR 控制 |
| 复用开漏 | `GPIO_MODE_AF_OD` | I2C 的 SCL/SDA | 同上，且只能拉低 |

**开漏为什么必须配外部上拉**：开漏输出只有下管，导通时拉到 0 V，关断时引脚悬空（高阻）。
没有上拉电阻，关断时电平不确定——这和输入悬空是同一个物理问题。

这也是 I2C 必须用开漏的根本原因：多个设备共享 SDA 线时，任何一方拉低都生效，
但**没有谁能强制拉高**（否则两个设备一个要拉高一个要拉低就是短路）。DHT11/DS18B20 的单总线同理，
所以 `lab_pins.c` 里 PA1 配的是 `GPIO_MODE_OUTPUT_OD` + `PULLUP`。

### 2. 悬空输入：教材那句话是整个 GPIO 章节的支点

教材 5.7.1 节原文："按键不被按下时，电平是多少？答案是：**不知道！**"
PIN 就像一根悬空的电线，与 GND 没有电气连接，无法确定电平。

所以按键电路必须有一端被"钉住"——外部加电阻，或启用芯片内部上下拉。
这三条路选哪条，决定了 `InitTypeDef` 怎么填：

```text
外部已有上拉电阻  → 设 NOPULL（内部再并一个也无害，但没必要）
没有外部上拉      → 设 PULLUP
按键另一端接 VCC  → 设 PULLDOWN
```

本板的按键就是"外部已有上拉"这一类。证据在 `driver_key.c`：

```c
GPIO_InitStruct.Pin  = KEY_GPIO_PIN;        /* PB14 */
GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
GPIO_InitStruct.Pull = GPIO_NOPULL;         /* ← 依赖板上的外部上拉 */
...
if (GPIO_PIN_RESET == HAL_GPIO_ReadPin(KEY_GPIO_GROUP, KEY_GPIO_PIN))
    return 1;                               /* 低 = 按下 ⇒ 另一端接 GND */
```

`NOPULL` + "低电平 = 按下"两个条件同时成立，**只能在有外部上拉时才对**。
换成一块没有外部上拉的板，这段代码会读到随机值——而且是"大部分时候看起来正常"的随机值，
最难查。这条与 `lab_pins.c` 里 PB12/PB13 的上下拉选择是同一类风险，那两处至今仍是工程推断，未上板确认。

### 3. `HAL_GPIO_Init` 如何把三个字段翻译成寄存器位

`CRL`/`CRH` 每个引脚占 4 bit：`CNF[3:2] | MODE[1:0]`。`HAL_GPIO_Init` 的核心是两句加法：

```c
config = GPIO_Init->Speed + GPIO_CR_CNF_GP_OUTPUT_PP;   /* 输出：速度 + CNF */
...
MODIFY_REG((*configregister), ((GPIO_CRL_MODE0 | GPIO_CRL_CNF0) << registeroffset),
                              (config << registeroffset));
```

为什么能直接**相加**？因为 `GPIO_SPEED_FREQ_LOW` 的定义就是 `GPIO_CRL_MODE0_1`，即 MODE 域的 bit1：

| 宏 | 定义 | 数值 | MODE 位 |
| --- | --- | --- | --- |
| `GPIO_SPEED_FREQ_LOW` | `GPIO_CRL_MODE0_1` | 2 (`0b10`) | 2 MHz |
| `GPIO_SPEED_FREQ_MEDIUM` | `GPIO_CRL_MODE0_0` | 1 (`0b01`) | 10 MHz |
| `GPIO_SPEED_FREQ_HIGH` | `GPIO_CRL_MODE0` | 3 (`0b11`) | 50 MHz |

而 `GPIO_CR_CNF_GP_OUTPUT_PP` = 0（推挽的 CNF 就是 `00`），所以 `config = 2` 正好是
"MODE=10、CNF=00"。**速度字段直接就是 MODE 的位图**——Day2 反汇编里 `movs r3, #2` 落在
`GPIO_InitStruct.Speed` 上，运行时被原样加进 `CRH`。

完整翻译表（由源码的 `switch` 分支整理）：

| `Mode` | `config` 的低 4 bit = CNF/MODE | 附加动作 |
| --- | --- | --- |
| `OUTPUT_PP` | `Speed` + `00` → `CNF=00` | —— |
| `OUTPUT_OD` | `Speed` + `01` → `CNF=01` | —— |
| `AF_PP` | `Speed` + `10` → `CNF=10` | —— |
| `AF_OD` | `Speed` + `11` → `CNF=11` | —— |
| `INPUT` + `NOPULL` | `MODE=00`，`CNF=01`（浮空） | —— |
| `INPUT` + `PULLUP` | `MODE=00`，`CNF=10` | **`GPIOx->BSRR = 位图`** |
| `INPUT` + `PULLDOWN` | `MODE=00`，`CNF=10` | **`GPIOx->BRR = 位图`** |
| `ANALOG` | `MODE=00`，`CNF=00` | —— |

### 4. 上拉/下拉靠写 `ODR` 决定——这条只能从源码看出来

F1 的 `CR` 寄存器里，"上拉"和"下拉"是**同一个 CNF 编码 `10`**，区分它们靠的是 `ODR` 对应位的值：

```c
else if (GPIO_Init->Pull == GPIO_PULLUP)
{
    config = GPIO_CR_MODE_INPUT + GPIO_CR_CNF_INPUT_PU_PD;   /* CNF = 10 */
    GPIOx->BSRR = ioposition;                                /* ODR 位 = 1 → 上拉 */
}
else /* GPIO_PULLDOWN */
{
    config = GPIO_CR_MODE_INPUT + GPIO_CR_CNF_INPUT_PU_PD;   /* 同一个 CNF */
    GPIOx->BRR  = ioposition;                                /* ODR 位 = 0 → 下拉 */
}
```

两条结论：

1. **`CR` 只选"要上下拉"，`ODR` 才选"上还是下"**。查手册只看 CR 表，会以为两者无法区分。
2. 这也解释了为什么"配置输入引脚"会动到 `ODR`——`ODR` 在输入模式下的语义被借走了，
   不再是"输出数据"。调试时看到输入引脚改 ODR 不是 bug。

### 5. 六驱动"自配置"表：能自己配引脚 ≠ 能独立工作

课程驱动里有的自己调 `HAL_GPIO_Init`，有的完全依赖 CubeMX。逐文件统计（实测）：

| 驱动 | `HAL_GPIO_Init` 调用 | 自开端口时钟 | 结论 |
| --- | --- | --- | --- |
| `driver_led.c` | 1 | **有**（`GPIOC`） | **自足**：独立可工作 |
| `driver_key.c` | 1 | 无 | 缺 `GPIOB` 时钟 |
| `driver_active_buzzer.c` | 1 | 无 | 缺 `GPIOA` 时钟 |
| `driver_passive_buzzer.c` | 1 | 无 | 缺 `GPIOA` 时钟 |
| `driver_motor.c` | 10 | 无 | 缺 `GPIOA`/`GPIOB` 时钟 |
| `driver_ir_receiver.c` | 1（在 `#if 0` 里） | 无 | 等于不自配，见 §6 |

**6 个自配置的驱动里，只有 1 个同时开了时钟。** 其余 5 个的 `HAL_GPIO_Init` 调用会成功返回，
但寄存器写入被静默丢弃（Day2 §3 第 1 步）。这条在编译期完全看不出来。

本工程之所以没暴露这个问题，是因为 `lab_pins.c` 在 `Lab_Pins_Init()` 里先使能了
`GPIOA`/`GPIOB`/`AFIO` 三个时钟。也就是说：**`lab_pins.c` 不只是补引脚，它还兜住了时钟**。
如果日后把其中的时钟使能语句删掉，症状会是"构建正常、LED 能闪（`driver_led.c` 自开 `GPIOC`）、
其余模块全不动"——一个极容易被判成"模块坏了"的假象。

### 6. 红外接收驱动的自配置被 `#if 0` 关掉了

`driver_ir_receiver.c` 里有一段完整的自配置代码，但整段在 `#if 0` 内，实际不编译。
这意味着它的引脚（PB10）**只能由外部配置**——`lab_pins.c` 承担了这个责任。
而 `driver_irq.c` 按 `GPIO_PIN_10` 分发回调，所以 EXTI10 必须映到端口 B，
否则它会去盯 PA10（USART1_RX），串口每收一个字节都触发一次"红外中断"。

### 7. 光敏电阻：教材读 PB11，驱动库读 PA3，哪个对

教材 5.8.3 节说"读取 **PB11** 即可知道光敏电阻的状态"，这是模块上 **LM393 比较器的数字输出 DO**
（光照越强 → 阻值下降 → `+` 端电压降 → DO 输出低）。
而驱动库 `driver_light_sensor.c` 走的是另一条路：

```c
/* PA3用作ADC(ADC1_IN3) */
```

两条并不矛盾——同一块模块一般同时引出**模拟量 AO** 和**数字量 DO**：

| 路径 | 引脚 | 信号 | 用途 |
| --- | --- | --- | --- |
| 数字 DO（LM393 比较后） | PB11 | 只有高/低 | 阈值判断，"亮/暗"二值 |
| 模拟 AO（分压中点） | PA3 → ADC1_IN3 | 连续电压 | 读具体亮度值 |

教材先教数字输入（第 5 章），驱动库用模拟输入（第 11 章，ADC 已讲过后）。
**这一条是我的推断，未上板确认 PB11 与 PA3 是否都实际引到排针**——按课程接线图与驱动注释推断，
需真板核对。

有源蜂鸣器的极性则两边一致：教材说"让 I/O 引脚输出低电平就可以让它发出声音"，
`driver_active_buzzer.c` 里 `on` 分支写的是 `GPIO_PIN_RESET`。**低电平发声**，
与 LED 的低电平点亮同一个套路——这类板子普遍用"灌电流"驱动。

---

## 系统调用/接口卡片

### `GPIO_InitTypeDef`

作用：描述一个引脚的完整配置，交给 `HAL_GPIO_Init` 翻译成寄存器位。

原型：

```c
typedef struct {
  uint32_t Pin;    /* 位掩码，可 | 多个 */
  uint32_t Mode;   /* GPIO_MODE_xxx */
  uint32_t Pull;   /* NOPULL / PULLUP / PULLDOWN */
  uint32_t Speed;  /* 仅输出模式使用 */
} GPIO_InitTypeDef;
```

参数：`Pin` 是**位掩码**（`GPIO_PIN_13` = `0x2000`），不是位号；其余三项是枚举宏。

返回值和错误：结构体本身无错误概念；`HAL_GPIO_Init` 里
`assert_param(IS_GPIO_MODE(...))` 只在定义了 `USE_FULL_ASSERT` 时才检查。
**未定义时填错 `Mode` 不会有任何提示**，`switch` 的 `default: break` 会直接跳过，
引脚保持原配置。

资源所有权：无。

线程/进程安全：`HAL_GPIO_Init` 内部用 `MODIFY_REG` 做读改写，**非原子**。

最小示例：

```c
GPIO_InitTypeDef s = {0};
s.Pin   = GPIO_PIN_1;
s.Mode  = GPIO_MODE_OUTPUT_OD;   /* 单总线：只能拉低，高靠上拉 */
s.Pull  = GPIO_PULLUP;
s.Speed = GPIO_SPEED_FREQ_LOW;
HAL_GPIO_Init(GPIOA, &s);
```

常见误用：

- `Pin` 传位号 `1` 而不是 `GPIO_PIN_1`（`0x0002`）——恰好都会选中 PA1，于是错误被掩盖；
  但传 `13` 就完全不同了。
- 开漏输出不配上拉，指望"不驱动时就自然是高"——那只是悬空。
- 输入引脚设 `PULLUP` 却又在外部装上拉：无害，但会让"到底谁在拉"变成查不清的问题。

### `HAL_GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)`

作用：读 `IDR` 的对应位，返回 `GPIO_PIN_SET`/`GPIO_PIN_RESET`。

参数：同 `WritePin`，`GPIO_Pin` 是位掩码。

返回值和错误：`GPIO_PinState`（0/1）。不区分"读到 0"与"引脚未配置"。

资源所有权：无。

线程/进程安全：单次读，无副作用。

常见误用：把返回值当布尔用（`if (HAL_GPIO_ReadPin(...))`）——`GPIO_PIN_RESET` 是 0，
逻辑会反。`driver_key.c` 写的是 `if (GPIO_PIN_RESET == ...) return 1;`，
把比较方向反过来写（常量在左）就是为了让编译器在误写 `=` 时报错。

---

## 最小可运行实验

目标：不接硬件，把 §3 的翻译表落到反汇编上——找到 `Speed` 与 `Pull` 分别变成什么。

```bash
cd experiments/F103_HAL_DriverLab

# 1) 看某个驱动的初始化：Pull 分支决定了写 BSRR 还是 BRR
arm-none-eabi-objdump -d --disassemble='Led_Init'      build/f103_hal_driverlab.elf
arm-none-eabi-objdump -d --disassemble='Key_Init'      build/f103_hal_driverlab.elf

# 2) 确认 HAL_GPIO_Init 内部有 BSRR/BRR 两条路径
arm-none-eabi-objdump -d --disassemble='HAL_GPIO_Init' build/f103_hal_driverlab.elf | grep -nE '#(16|20)' | head
```

`#16` = BSRR、`#20` = BRR（偏移由 Day3 的 `offsetof` 实证：BSRR=16、BRR=20）。

---

## 编译与运行

```bash
cmake --build build
```

本篇的"运行"仍是反汇编——`Key_Read()` 需要按键才有意义，本机无板。

---

## 结果观察

**`Led_Init` 的 `Pull` 分支**（Day2 已取读数）：`GPIO_NOPULL` = 0，所以反汇编里
`str r4, [sp, ...]` 存的是 0，走"浮空输入"分支的 `CNF=01`。

**`Key_Init` 的配置**（源码 + 应有反汇编）：

```c
Pin = GPIO_PIN_14 = 0x4000
Mode = GPIO_MODE_INPUT = 0
Pull = GPIO_NOPULL = 0
Speed = 未赋值（保持 {0} 初始化）
```

**六驱动时钟统计**（实测）：

```text
driver_active_buzzer.c   CLK=0  GPIO_Init=1
driver_ir_receiver.c     CLK=0  GPIO_Init=1     ← 该调用在 #if 0 内
driver_key.c             CLK=0  GPIO_Init=1
driver_led.c             CLK=1  GPIO_Init=1     ← 唯一自足
driver_motor.c           CLK=0  GPIO_Init=10
driver_passive_buzzer.c  CLK=0  GPIO_Init=1
```

**引脚与极性核对表**：

| 模块 | 引脚 | 模式 | 有效电平 | 证据 |
| --- | --- | --- | --- | --- |
| 板载 LED | PC13 | 推挽输出 | **低 = 亮** | `driver_led.c:Led_Control` |
| 按键 K1 | PB14 | 输入 + `NOPULL` | **低 = 按下** | `driver_key.c:Key_Init/Key_Read` |
| 有源蜂鸣器 | PA8 | 推挽输出 | **低 = 响** | `driver_active_buzzer.c` |
| 光敏电阻（数字） | PB11 | 输入 | 光照强 → 低 | 教材 5.8.3（**未上板核对**） |
| 光敏电阻（模拟） | PA3 = ADC1_IN3 | 模拟输入 | 电压随光照变 | `driver_light_sensor.c:58` |

---

## 常见错误和陷阱

| 陷阱 | 症状 | 根因 | 处理 |
| --- | --- | --- | --- |
| 输入引脚不设上下拉，外部也没有 | 读数随机、"大部分时候正常" | 引脚悬空 | 设 `PULLUP`/`PULLDOWN`，或加外部电阻 |
| 开漏输出不配上拉 | 只能看到低电平 | 关断时高阻，没有东西拉高 | 启用内部 `PULLUP` 或外加电阻 |
| 以为"上拉由 CR 选择" | 查手册查不到区别 | F1 用 `ODR` 位区分上下拉 | 看 `HAL_GPIO_Init` 的 `BSRR`/`BRR` 分支 |
| 自配置驱动当成自足库 | 只有 LED 能用 | 5/6 驱动不开端口时钟 | 外部先 `__HAL_RCC_GPIOx_CLK_ENABLE()` |
| `Pin` 传位号 | 碰巧对，或全错 | 位掩码 vs 位号 | 一律用 `GPIO_PIN_n` 宏 |
| `if (HAL_GPIO_ReadPin(...))` 当"按下" | 逻辑全反 | `GPIO_PIN_RESET` = 0 | 显式与 `GPIO_PIN_RESET`/`SET` 比较 |
| `Mode` 填错值 | 无报错、引脚没反应 | `assert_param` 默认不生效 | 必要时 `#define USE_FULL_ASSERT` |

---

## 故障排查方法

"引脚读不到 / 输出不动"这类问题，**先分清是电气问题还是配置问题**：

1. **量电平**（有板时）。引脚实际电压是多少？如果是 1.6 V 左右的"中间值"，就是悬空——
   直接指向上下拉配置，而不是代码逻辑。
2. **读 `CR` 的 4 个 bit**。与 §3 的翻译表逐位对照。这是唯一能确认"配置真的写进去了"的方法。
3. **读 `ODR` 的对应位**（输入模式）。上拉应为 1、下拉应为 0。这一位不对，说明 `Pull` 字段没生效。
4. **确认端口时钟**。`RCC->APB2ENR` 的 `IOPxEN`。前 3 步全对但都没生效，问题在这里。
5. **确认引脚没被别处抢走**。同一引脚被 `HAL_GPIO_Init` 配两次（例如 CubeMX 一次、驱动一次）
   以后写的为准——这类"配置被覆盖"在工程里很常见，`lab_pins.c` 与 `gpio.c` 就是这种关系。

本工程有一条现成的教训：`driver_led.c` 自己也配 PC13，`gpio.c` 也配 PC13，
两者配置一致所以看不出区别。**如果哪天改了一处忘了另一处，症状会是"改完没效果"**——
排查时应先搜"这个引脚在几个地方被配置过"。

---

## 练习题

### 基础题

题目：给一张表，把 `GPIO_MODE_OUTPUT_OD` + `GPIO_SPEED_FREQ_HIGH` 与
`GPIO_MODE_INPUT` + `GPIO_PULLDOWN` 两种组合翻译成 `CRL`/`CRH` 的 4 bit 值与附加动作。

提示：`config = Speed + CNF`；下拉写 `BRR`。

验收标准：前者 = `CNF=01, MODE=11`（0b1101），后者 = `CNF=10, MODE=00`（0b1000）且执行 `BRR = 位图`。

### 改错题

题目：下面的代码有什么问题？

```c
void sensor_init(void)
{
    GPIO_InitTypeDef s = {0};
    s.Pin  = GPIO_PIN_11;
    s.Mode = GPIO_MODE_INPUT;
    s.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &s);
}
/* 之后用 HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11) 判断光照 */
```

提示：先确认模块的 DO 输出类型（LM393 是开集/开漏输出），再想外部有没有上拉。

验收标准：能指出"DO 是开集输出，`NOPULL` 会让引脚悬空"，给出改 `PULLUP` 的修正；
并能说明这属于"配置问题"还是"电气问题"。

### 综合题

题目：写一份"新增一个输入模块"的检查清单，至少覆盖：端口时钟、引脚模式、上下拉的判据、
极性确认的证据来源、以及"这个引脚在几个地方被配置过"的排查方法。

提示：参考 §7 的五步排查与 §5 的六驱动表。

验收标准：清单里每条都能落到一个具体的寄存器位或一条具体的源码查证动作，
不出现"检查配置是否正确"这类无法执行的条目。

---

## 验收标准

- [ ] 能说出 8 种模式各自的一个真实使用场景，并解释开漏为什么必须配上拉。
- [ ] 能把 `Mode`/`Pull`/`Speed` 三者翻译成 `CNF[3:2] | MODE[1:0]`，并说明 `config = Speed + CNF` 为什么能相加。
- [ ] 能解释"上拉由 `ODR` 决定"这一 F1 特性，并从源码指出 `BSRR`/`BRR` 两行。
- [ ] 能说出 6 个自配置驱动里有几个能独立工作，剩下的缺什么，以及为什么编译期发现不了。
- [ ] 能解释 PB11 与 PA3 两条光敏路径的区别，并明确标注"未上板核对"的部分。
- [ ] 能独立完成本篇三条 `objdump` 命令，找到 `#16`（BSRR）与 `#20`（BRR）。

## 延伸阅读

- 教材：《HAL快速入门与项目实战(基于DshanMCU-103)》第 5.7 节（GPIO 工作模式与 HAL 源码解析）、
  第 5.8 节（GPIO 输入：按键、光敏、有源蜂鸣器）
- HAL 源码：`Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio.c` 的 `HAL_GPIO_Init`
- 宏定义：`Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_gpio.h:140-154`
- 引脚缺口：[`experiments/F103_HAL_DriverLab/README.md`](../../experiments/F103_HAL_DriverLab/README.md) §6.1
- 下一篇：`Linux/notes/F103_HAL入门/Day05_中断体系与GPIO中断编程.md`

---

**验证记录**

| 项目 | 内容 |
| --- | --- |
| 日期 | 2026-10-29 |
| 环境一 | Windows 11 + Git Bash；`grep`/`sed` 统计课程驱动源码 |
| 命令一 | 遍历 `Drivers/DshanMCU-F103/driver_*.c`，统计 `__HAL_RCC_GPIOx_CLK_ENABLE` 与 `HAL_GPIO_Init` 出现次数 |
| 结果一 | 6 个驱动调 `HAL_GPIO_Init`；只有 `driver_led.c` 自开时钟；`driver_ir_receiver.c` 的那次调用在 `#if 0` 内 |
| 环境二 | 同上 |
| 命令二 | `sed -n '/void HAL_GPIO_Init/,/^}/p' stm32f1xx_hal_gpio.c`；`sed -n '130,160p' stm32f1xx_hal_gpio.h` |
| 结果二 | 输出模式 `config = Speed + CNF`；输入 `PULLUP` 走 `BSRR`、`PULLDOWN` 走 `BRR`，两者 CNF 同为 `10`；`GPIO_SPEED_FREQ_LOW = GPIO_CRL_MODE0_1` = 2 |
| 未覆盖 | 全部板上电平现象（按键、光敏、蜂鸣器）；PB11 与 PA3 是否都引到排针（教材与驱动分别写了两个引脚，本篇判为"两条路径"，**未上板核对**）；开漏输出加内部上拉的实际上升沿速度 |
