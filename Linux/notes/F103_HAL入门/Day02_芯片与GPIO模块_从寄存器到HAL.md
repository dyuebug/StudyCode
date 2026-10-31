# Day2：芯片与 GPIO 模块——从寄存器到 HAL

**验证状态**：反汇编与符号证据**已实测**（`arm-none-eabi-objdump -d build/f103_hal_driverlab.elf`，
2026-10-29，本机 GCC 14.2.1）；HAL 源码行为**已核对**（`stm32f1xx_hal_gpio.c:465-479`）；
**板上现象未验证**（本机无 ST-Link，灯是否真的闪未观察）。教材侧内容来自
《HAL快速入门与项目实战》第 5.1–5.5 节。

---

## 学习目标

学完本篇应能用代码证据回答：

1. 一个 LQFP48 封装的芯片，"引脚编号"和"引脚名"回答的是哪两个不同问题，为什么两个都需要。
2. 点亮一颗 LED 有几种接法，本板是哪种，以及**从驱动源码的哪一行能反推出来**。
3. 让 PC13 输出高电平，硬件上必须按哪四步走；漏掉任一步分别会看到什么现象。
4. 为什么 `BSRR` 比 `ODR` 更适合改单个引脚——用一次读改写加上一次中断就能构造出故障。
5. `HAL_GPIO_WritePin` 到底做了什么，用反汇编的两条指令说明。

---

## 环境与前置条件

| 项 | 要求 |
| --- | --- |
| 工程 | `experiments/F103_HAL_DriverLab/`（已能构建） |
| 工具链 | `arm-none-eabi-gcc` 14.2.1 + `objdump` + `nm` |
| 参考工程 | `experiments/F103_01_GPIO_Blink/`（寄存器级点灯，无 HAL） |
| 硬件 | 可选。本篇全部结论都可以在纯软件侧验证，不需要板子 |
| 前置 | Day1 的构建流程（能跑通 `cmake --build build`） |

本篇的实验**故意不依赖硬件**：读寄存器路径靠反汇编，这是在没有板子时唯一能拿到真证据的方式。

---

## 核心概念

### 1. 引脚编号与引脚名

芯片外壳上有一个圆点，它对应的是 **PIN1**，逆时针依次是 PIN2、PIN3……这叫**引脚编号**。
但编号不告诉你这根脚能做什么，所以数据手册里还有**引脚名**（`PC13`、`PA9`、`VDD`……）。

两者是两套坐标系：

```text
LQFP48 的物理位置        →  PIN1 … PIN48          （焊接、查原理图用）
芯片内部的信号名          →  PA0…PA15, PB0…PB15, PC13…PC15, VDD, VSS…
```

工程代码里**只用引脚名**。`GPIOC` + `GPIO_PIN_13` 这种写法之所以安全，是因为
`gpio.h`/`stm32f1xx.h` 里的宏已经把"端口 + 位号"翻译成了寄存器地址与位掩码，
与封装无关——同一份代码换到 LQFP64 封装的 F103 上仍然成立（只要 PC13 还在）。

### 2. 点灯：四种接法与"低电平点亮"的判据

教材第 5.3.1 节给了四种接法，本质只有两个变量：**引脚输出高还是低算"亮"**，以及**是否需要三极管**。

| 方式 | 接法 | 点亮条件 | 适用 |
| --- | --- | --- | --- |
| 1 | 引脚 → 电阻 → LED 阳极，阴极接地 | 输出 **高** | 引脚能提供足够灌出电流 |
| 2 | 3.3 V → LED 阳极，阴极 → 电阻 → 引脚 | 输出 **低** | 引脚灌电流能力更强时常用 |
| 3 / 4 | 加三极管，引脚驱动基极 | 取决于管子类型 | 引脚驱动能力不足时 |

本板是**方式 2（低电平点亮）**。这条不是猜的，`driver_led.c` 里写得很直白：

```c
int Led_Control(int which, int on)
{
    if (on)
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);   /* on → RESET → 低 */
    else
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);     /* off → SET → 高 */
    return 0;
}
```

`on=1` 写的是 `GPIO_PIN_RESET`，即输出低电平。**API 语义（on/off）与电平语义（高/低）在这里是反的**，
这正是 `Led_Control` 这层封装存在的意义：上层只管 on/off，极性细节封在驱动里。
换一块"高电平点亮"的板，只改这两行，`Led_Test()` 一个字都不用动。

### 3. 让 PC13 输出高电平的四个步骤

教材第 5.3.2 节列出四步，每一步漏掉都有各自的症状：

| # | 动作 | 寄存器/位 | 漏掉的症状 |
| --- | --- | --- | --- |
| 1 | 使能 GPIO 模块时钟 | `RCC->APB2ENR` 的 `IOPCEN`（bit4） | **代码全对、灯不亮**。外设时钟关闭时寄存器不响应写入 |
| 2 | 把引脚连接到 GPIO 模块 | `GPIOC->CRH` 的 CNF13/MODE13 = 00/10 | 引脚仍连着复位默认的"浮空输入"，写 ODR 无效果 |
| 3 | 配置为输出 | 同上（MODE≠00） | 同 2 |
| 4 | 输出高/低 | `GPIOC->BSRR` | 引脚状态不变 |

第 1 步最值得强调：**时钟没开时，对寄存器的一切写入都被静默丢弃**，没有报错、没有异常，
程序照常跑。这是嵌入式里最典型的"代码正确但不工作"。

第 2 步有个容易漏的细节：`CRH` 的复位值是 `0x4444_4444`（每 4 bit 一组），
所以配置前必须**先清零再置位**：

```c
LED_PORT->CRH &= ~(GPIO_CRH_CNF13 | GPIO_CRH_MODE13);   /* 先清 */
LED_PORT->CRH |=  GPIO_CRH_MODE13_1;                     /* MODE=10 → 2 MHz 输出 */
```

只置位不清零，残留的 CNF 位会让引脚变成"复用功能输出"之类，症状依然是"灯不亮但代码没错"。

### 4. `BSRR` 与 `ODR`：一次读改写就能构造出故障

教材第 5.5.2 节对比了两种写法。核心区别是**原子性**：

```c
/* 写法 A：ODR 读—改—写（三条指令，中间可以被中断打断） */
val = *p;             /* 读回整个 16 位 ODR */
val |= (1 << 13);     /* 只改 bit13 */
*p = val;             /* 写回整个 ODR */

/* 写法 B：BSRR 单次写（一条指令，set 用低 16 位，reset 用高 16 位） */
*p = (1 << 13);       /* bit13 → 1：置高 */
*p = (1 << 29);       /* bit13 → 1 但写在高 16 位：置低 */
```

构造故障的最小场景：

```text
主程序：读 ODR → 0x0000
中断发生：把 PB5 置高 → ODR = 0x0020    ← 中断里改了同一端口的另一个引脚
主程序恢复：val |= (1<<13) → 0x2000，写回 ODR = 0x2000
          ↑ PB5 的 0x0020 被覆盖掉了
```

同一端口上两个"写者"（主循环与中断）都用读改写改不同引脚，就会互相吃掉对方的修改。
**BSRR 没有这个问题**：写 1 的位生效、写 0 的位完全不影响引脚，天然只碰你指定的那一位。
这也是本仓库 `F103_01_GPIO_Blink/main.c` 的注释选择 BSRR 的理由，原话是
"BSRR 置位/复位是原子的，不会读回再写回 ODR，也就不会误改其他引脚"。

副作用提醒：`HAL_GPIO_TogglePin` 内部仍然是"读 ODR → 算反 → 写 BSRR"（`stm32f1xx_hal_gpio.c:487-495`），
所以**翻转操作并不原子**。在中断与主循环同时 toggle 同一端口的不同引脚时，它仍有上述窗口。

### 5. HAL 库的本质：两条 `str` 指令

教材第 5.5.1 节的一句话是"HAL 库函数的本质是帮我们操作寄存器"。这句话可以变成读数。

`HAL_GPIO_WritePin` 的源码只有五行（`Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio.c:465-479`）：

```c
if (PinState != GPIO_PIN_RESET)  GPIOx->BSRR = GPIO_Pin;
else                             GPIOx->BSRR = (uint32_t)GPIO_Pin << 16u;
```

编译出来就是（`objdump -d --disassemble=HAL_GPIO_WritePin`，实测）：

```text
0800159a <HAL_GPIO_WritePin>:
 800159a:  cbz   r2, 80015a0        ; PinState == 0 ?
 800159c:  6101  str   r1, [r0, #16]  ; BSRR = Pin        （置位分支）
 800159e:  4770  bx    lr
 80015a0:  0409  lsls  r1, r1, #16     ; Pin << 16
 80015a2:  6101  str   r1, [r0, #16]  ; BSRR = Pin << 16  （复位分支）
 80015a4:  4770  bx    lr
```

`[r0, #16]` 就是 `+0x10` = **BSRR** 的偏移，`lsls #16` 就是"移到高 16 位"。
整个 HAL 调用被编译成了 **一条 `str`**——不是因为它简单，而是因为它本来就只是一次寄存器写。
这一条读数把"库 vs 裸寄存器"的争论直接消掉了：库不引入开销，它只引入**命名与可移植性**。

同样，`Led_Init` 的反汇编能一步步对上源码：

```text
8002492:  ldr   r2, [r3, #24]        ; 读 RCC->APB2ENR（偏移 0x18）
8002494:  orr.w r2, r2, #16          ; 置 bit4 = IOPCEN
8002498:  str   r2, [r3, #24]        ; 写回
800249a:  ldr   r3, [r3, #24]        ; 再读一次 ← __HAL_RCC_GPIO_CLK_ENABLE 的读回延迟
800249c:  and.w r3, r3, #16
...
80024b2:  mov.w r3, #8192            ; 0x2000 = GPIO_PIN_13 → GPIO_InitStruct.Pin
80024b8:  movs  r3, #1               ; GPIO_MODE_OUTPUT_PP = 1 → .Mode
80024bc:  str   r4, ...              ; 0            → .Pull = NOPULL
80024be:  movs  r3, #2               ; GPIO_SPEED_FREQ_LOW = 2 → .Speed
80024c6:  bl    HAL_GPIO_Init
```

那段"再读一次"不是冗余：`__HAL_RCC_GPIO_CLK_ENABLE()` 宏在使能后做一次回读，
作用是让后续访问等时钟真正稳定。少了它，在某些时钟配置下紧接着的外设访问可能读到旧值。

---

## 系统调用/接口卡片

### `void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState)`

作用：把指定端口的指定引脚置高或置低。底层是**一次 BSRR 写入**，原子。

参数：`GPIOx` 是端口基址（`GPIOA/B/C`）；`GPIO_Pin` 是位掩码宏（`GPIO_PIN_13` = `0x2000`，**不是** 13）；
`PinState` 取 `GPIO_PIN_SET`（1）或 `GPIO_PIN_RESET`（0）。

返回值和错误：`void`，无错误返回。参数非法时 `assert_param` 生效需要
`USE_FULL_ASSERT` 已定义，否则**完全不做检查**。

资源所有权：无。不持有任何状态。

线程/进程安全：单次寄存器写，本身不被打断成两半；但"读—改—写"式操作（如 `HAL_GPIO_TogglePin`）
不是原子的。

最小示例：

```c
HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);  /* 低 → 本板点亮 */
HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);    /* 高 → 熄灭 */
```

常见误用：

- 把 `GPIO_Pin` 写成 `13`：`13` = `0b1101`，同时改 PIN0/PIN2/PIN3，而 PIN13 反而没动。
- 以为 `GPIO_PIN_SET` 就是"点亮"：极性由硬件接法决定，本板是 RESET 才亮。
- 在中断与主循环里对同一端口用 `HAL_GPIO_TogglePin`：翻转是读改写，有窗口。

### `void HAL_GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_Init)`

作用：配置引脚的**方向、模式、上下拉、速度**，并（当模式是中断时）写 `AFIO->EXTICR` 选端口。

参数：`GPIO_InitStruct` 的四个字段——`Pin`（位掩码，可 `|` 多个）、`Mode`（`GPIO_MODE_OUTPUT_PP`
/ `INPUT` / `IT_RISING` 等）、`Pull`、`Speed`。

返回值和错误：`void`。

资源所有权：无。

线程/进程安全：非原子，内部按引脚循环写 `CRL`/`CRH`。不要在运行期对同一端口并发调用。

最小示例：

```c
GPIO_InitTypeDef s = {0};
s.Pin   = GPIO_PIN_13;
s.Mode  = GPIO_MODE_OUTPUT_PP;
s.Pull  = GPIO_NOPULL;
s.Speed = GPIO_SPEED_FREQ_LOW;
HAL_GPIO_Init(GPIOC, &s);
```

常见误用：**忘了先 `__HAL_RCC_GPIOx_CLK_ENABLE()`**。函数会成功返回、寄存器写入被静默丢弃。

### `__HAL_RCC_GPIOA_CLK_ENABLE()` 等

作用：置 `RCC->APB2ENR` 对应的 `IOPxEN` 位，并做一次回读延迟。

参数：无（宏）。

返回值和错误：无。

资源所有权：无。时钟是全局状态，**一次使能后一直有效**，没有"关闭"的强制要求。

常见误用：以为它是"打开某个引脚"。它打开的是**整个端口**（16 个引脚共用一组时钟）。

---

## 最小可运行实验

目标：不接硬件，用反汇编证明"HAL 调用 = 一次寄存器写"，并定位端口基址与位掩码。

```bash
cd experiments/F103_HAL_DriverLab

# 1) 两个函数各自的机器码
arm-none-eabi-objdump -d --disassemble='HAL_GPIO_WritePin' build/f103_hal_driverlab.elf
arm-none-eabi-objdump -d --disassemble='Led_Control'       build/f103_hal_driverlab.elf

# 2) 确认 GPIOC 基址字面量
arm-none-eabi-objdump -s -j .text build/f103_hal_driverlab.elf | grep -i '40011000'
```

---

## 编译与运行

```bash
cmake --build build          # 前置：Day1 已 configure
```

本篇没有"运行"——`Led_Test()` 是死循环，需要板子。替代验证手段是上节的三条 `objdump` 命令。

---

## 结果观察

**`Led_Control` 的反汇编**（实测）：

```text
080024d8 <Led_Control>:
 80024da:  cbz   r1, 80024ec                ; on == 0 ?
 80024dc:  movs  r2, #0                     ;   PinState = GPIO_PIN_RESET
 80024de:  mov.w r1, #8192                  ;   GPIO_Pin = 0x2000 = PIN_13
 80024e4:  bl    800159a <HAL_GPIO_WritePin>
 ...
 80024ec:  movs  r2, #1                     ; else: PinState = GPIO_PIN_SET
 80024f4:  bl    800159a <HAL_GPIO_WritePin>
 80024fc:  .word 0x40011000                 ; GPIOC 基址（PC 相对字面量池）
```

三条读数可以直接核对：

| 读数 | 含义 | 与源码对应 |
| --- | --- | --- |
| `0x40011000` | GPIOC 外设基址 | `GPIOC` 宏 |
| `8192` = `0x2000` | bit13 掩码 | `GPIO_PIN_13` |
| `r2 = 0` / `r2 = 1` | RESET / SET | `on ? RESET : SET` 的极性反转 |

**`HAL_GPIO_WritePin` 的反汇编**：只有两条 `str r1, [r0, #16]`（`#16` = BSRR），
复位分支多一条 `lsls r1, r1, #16`。

**`Led_Init` 的反汇编**：`[r3, #24]` 读改写 + 回读 → `APB2ENR` 的 bit4；
栈上四个字段依次是 `Pin=0x2000 / Mode=1 / Pull=0 / Speed=2`，与 `GPIO_InitTypeDef` 的赋值逐项吻合。

---

## 常见错误和陷阱

| 陷阱 | 症状 | 根因 | 定位方法 |
| --- | --- | --- | --- |
| 漏使能端口时钟 | 代码对、灯不亮 | `APB2ENR` 位没开，寄存器写被丢弃 | 反汇编里找 `orr #16`/`#1<<n` 那段；或用调试器读 `RCC->APB2ENR` |
| 配 `CRH` 前没清零 | 引脚变成复用/模拟模式 | 复位值 `0x4444_4444` 残留 CNF 位 | 读 `GPIOC->CRH` 的 13 号 4 bit |
| `GPIO_Pin` 传 `13` 而不是 `GPIO_PIN_13` | 乱亮别的脚 | `13` 是位图不是位号 | 反汇编里看立即数是不是 `0x2000` |
| 用 ODR 读改写 | 偶发"别的引脚被复位" | 与中断竞争，丢失对方的修改 | 查代码里有没有 `->ODR =`；改用 `BSRR` |
| 以为 `SET` 就是点亮 | 逻辑全反 | 极性由硬件接法决定 | 看驱动里 `on` 分支写的是 `SET` 还是 `RESET` |
| 在 `Led_Test()` 里加 `printf` | 灯闪频率变了 | `printf` 是阻塞发送，占用时间 | 看串口上的时间戳差 |

---

## 故障排查方法

"GPIO 不动作"这一类问题的排查顺序是固定的，**从时钟往下**，不要从代码往上猜：

1. **读 `RCC->APB2ENR`**：对应 `IOPxEN` 位是否为 1。用调试器读，或反汇编人工确认使能语句存在。
2. **读 `GPIOC->CRH`**：13 号引脚那 4 bit 是否是"输出模式"。是 `0x4` 说明还是复位默认的浮空输入。
3. **读 `GPIOC->ODR`**：bit13 是否真的变了。变了但引脚没动作 → 问题在 1/2 步（引脚没连到 GPIO 模块）。
4. **量引脚电平**：前三步都对但电压不对，才是硬件问题（LED 接反、限流电阻开路、引脚被外部拉扯）。

反汇编作为第 0 步特别有用：**先确认你写的代码真的被编进去了**。
本仓库遇到过的最迷惑的一类问题是"改了源码但体积/行为没变"，原因是构建缓存没刷新（见 Day1 的 `LAB_TEST`）。
`objdump` 看到的就是镜像里的东西，绕不过这一层。

---

## 练习题

### 基础题

题目：用 `objdump` 找出 `Led_Init` 里使能 GPIOC 时钟的那三条指令，写出各自的地址、指令与含义，
并说明第三条"再读一次"是做什么用的。

提示：`ldr / orr / str` 三连，偏移是 `#24`。

验收标准：三条指令地址正确，能解释 `__HAL_RCC_GPIOx_CLK_ENABLE()` 的回读延迟。

### 改错题

题目：下面的代码有什么问题？

```c
void led_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    GPIOC->CRH |= GPIO_CRH_MODE13_1;
    GPIOC->BSRR = GPIO_BSRR_BS13;
}
```

提示：对比 `CRH` 的复位值与目标值，逐 bit 写出 `|=` 之后的实际内容。

验收标准：能指出"没有清零导致 CNF13 残留"这一缺陷，并给出 `&= ~(...)` 的修正；
进一步能说出复位值 `0x4444_4444` 里 13 号引脚那 4 bit 原本是多少。

### 综合题

题目：假设主循环用 `HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13)` 翻转 LED，
同时 EXTI 中断里用 `HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, ...)` 改 PC14。
构造一个这两个操作互相破坏的执行序列，并给出一个能修掉它的改法。

提示：`TogglePin` 是"读 ODR → 算反 → 写 BSRR"，中断可以插在"读"与"写"之间。

验收标准：能画出"主读 → 中断改 PC14 → 主写回"三步的 ODR 值变化；
修正方法应指出"翻转也要用 BSRR 单写"或"在临界区里做"其中的一种，并说明代价。

---

## 验收标准

- [ ] 能解释引脚编号与引脚名的区别，并说出代码里只用哪一个。
- [ ] 能从 `Led_Control` 的源码反推本板的点亮极性，并用另一种方式（反汇编或原理图）交叉验证。
- [ ] 能按"时钟 → 引脚模式 → 输出数据"的顺序说明漏掉每步的症状。
- [ ] 能用一个执行序列说明 ODR 读改写的竞态，并说出 BSRR 为什么没这个问题。
- [ ] 能用 `objdump` 找出 `BSRR` 的偏移 `#16`，并解释 `lsls #16` 出现在哪条分支。
- [ ] 能独立完成本篇三条 `objdump` 命令，得到与本篇一致的读数。

## 延伸阅读

- 教材：《HAL快速入门与项目实战(基于DshanMCU-103)》第 5.1–5.5 节（芯片封装、GPIO 模块、CubeMX 点灯、HAL 库的本质）
- 寄存器级对照：[`experiments/F103_01_GPIO_Blink/main.c`](../../experiments/F103_01_GPIO_Blink/main.c)
- HAL 源码：`experiments/F103_HAL_DriverLab/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio.c:465-479`
- 速查表：[`knowledge_base/DShanMCU-F103模块与驱动_速查表.md`](../../knowledge_base/DShanMCU-F103模块与驱动_速查表.md)
- 下一篇：`Linux/notes/F103_HAL入门/Day03_C语言基础补充_指针与结构体访问硬件.md`

---

**验证记录**

| 项目 | 内容 |
| --- | --- |
| 日期 | 2026-10-29 |
| 环境一 | Windows 11 + Git Bash；`arm-none-eabi-gcc` 14.2.1；工程 `experiments/F103_HAL_DriverLab/`，`LAB_TEST=0` |
| 命令一 | `arm-none-eabi-objdump -d --disassemble='HAL_GPIO_WritePin' build/f103_hal_driverlab.elf` |
| 结果一 | `str r1, [r0, #16]`（BSRR）；复位分支先 `lsls r1, r1, #16` 再 `str` |
| 环境二 | 同上 |
| 命令二 | `arm-none-eabi-objdump -d --disassemble='Led_Control'`；`--disassemble='Led_Init'` |
| 结果二 | `Led_Control` 中字面量池 `0x40011000`（GPIOC）、立即数 `8192`（PIN_13）、`r2=0/1` 对应 RESET/SET；`Led_Init` 中 `[r3,#24]` 读改写 + 回读 = `APB2ENR` bit4，栈上四字段 = `Pin/Mode/Pull/Speed` |
| 未覆盖 | 板上灯的实际亮灭与极性（需真板）；`CRH` 复位值 `0x4444_4444` 为数据手册值，本轮未上板读寄存器确认；`__HAL_RCC_GPIOx_CLK_ENABLE()` 回读延迟的必要性为源码推论 |
