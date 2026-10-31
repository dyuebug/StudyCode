# Day3：C 语言基础补充——指针、结构体与位操作如何访问硬件

**验证状态**：本篇 §3 的全部读数**已实测**（Windows 11 + Git Bash + MinGW `gcc` 15.1.0，
`experiments/F103_HAL_DriverLab/lab_c_basics/`）；偏移量与 HAL 结构体定义**已核对源码**
（`stm32f1xx_hal_gpio.h:46-60`、`stm32f103xb.h:357-366`）；驱动缺陷**已用主机程序复现**。
教材侧内容来自《HAL快速入门与项目实战》第 5.6 节。

---

## 学习目标

学完本篇应能用代码证据回答：

1. CPU 凭什么"写一个地址就等于配置引脚"——从地址到外设模块之间发生了什么。
2. 为什么 `int *p = (int *)0x40001000; *p = 1;` 能在 STM32 上生效，却不能在 PC 上随手这么写。
3. 结构体的字段顺序为什么**必须**等于寄存器偏移顺序，错一个字段会发生什么。
4. 教材 5.6.2 节示例里的 `(GPIO_REGS *)0x40011000` 注释成 `gpioa` 是笔误，正确的是哪个端口，怎么证明。
5. `cfg & (3<<5) == 0` 为什么恒假，用一次真实运行说明它造成的后果。
6. 教材说"64 位处理器上 `long` 是 8 字节"，这句话在 Windows 上是否成立。

---

## 环境与前置条件

| 项 | 要求 |
| --- | --- |
| 主机编译器 | 任意 C11 编译器（本篇实测 MinGW `gcc` 15.1.0） |
| 目标工程 | `experiments/F103_HAL_DriverLab/`（只用于读头文件与反汇编） |
| 硬件 | **不需要**。本篇所有读数都在主机侧产生 |
| 前置 | Day2（GPIO 四步与 BSRR/ODR 的区别） |

本篇是唯一一篇完全不需要目标板的——因为讨论的是 C 语言语义，不是硬件行为。

---

## 核心概念

### 1. 地址总线：写一个地址，为什么会选中某个外设

SoC 内部有一张**地址映射表**（厂家预设）。CPU 发出地址，内存管理单元按地址范围决定选中谁：

```text
CPU ──addr──► [ 地址译码 ] ──片选──► RAM      （0x2000_0000 起，20 KB）
                    │                 Flash    （0x0800_0000 起，64 KB）
                    │                 GPIOA    （0x4001_0800）
                    ├───────────────► GPIOC    （0x4001_1000）  ← 本板 LED
                    │                 USART1   （0x4001_3800）
                    └───────────────► ...
```

**同一时刻只有一个片选有效**，其余设备"像不存在一样"。因此：

- 写 `0x40011010` → 落到 GPIOC 的 BSRR，**不会**顺手改到 RAM，因为地址不重叠。
- 写一个没有映射的地址（如 F103 的 `0x40006400` CAN）→ 触发总线错误，最终成为 HardFault。
  本仓库 `F103_18_CAN_Loopback` 在 QEMU 里实测到的正是这条路径（`BFAR=0x40006400`）。

所以"用指针访问硬件"没有任何魔法：**外设寄存器就是一段被映射成内存的地址**。

### 2. 指针大小：教材那句话只在"目标"上成立

教材 5.6.1 节说"无论何种指针，对于 32 位处理器，指针变量的大小都是 4 字节"。这条**只对目标芯片成立**。

本篇程序在主机上跑出的实况（实测）：

```text
[1] sizeof: void*=8  int*=8  short*=8  long*=8  char*=8
[1] sizeof: uint8_t=1  uint16_t=2  uint32_t=4  uint64_t=8  long=4
```

两条要分开读：

- **指针全是 8 字节** → 本机是 x86_64，指针宽度等于地址总线宽度。在 STM32F103（32 位）上会是 4。
  **同一行代码在两个平台上 `sizeof` 不同**，这是最容易在"主机验证过的代码"里埋下的坑。
- **`long` 是 4 字节** → 教材说"64 位处理器下 `long` 为 8 字节"，在 Windows 上**不成立**。
  Windows 用 **LLP64** 模型（`long` 固定 4 字节，只有指针变 8），Linux 用 **LP64**（`long` 变 8）。

这就是为什么教材紧接着推荐 `uint8_t`/`uint16_t`/`uint32_t`：**定宽类型没有平台差异**。
在嵌马上这都不是"风格问题"，是"同一份代码换平台就错"的问题。

### 3. 结构体偏移：`p->BSRR` 到底访问了哪个地址

教材 5.6.2 节的推理链是：结构体成员在内存里依次排列，`p->member` 的地址 = `p` + 成员偏移。
把 `p` 设成外设基址、把字段顺序设成寄存器顺序，就得到了"用结构体访问一组寄存器"。

这条链可以量化（实测 `offsetof`）：

```text
[2] offsetof: CRL=0 CRH=4 IDR=8 ODR=12 BSRR=16 BRR=20 LCKR=24
[2] sizeof(GPIO_REGS_DEMO)=28
```

- 每个字段都是 `uint32_t`，所以偏移是 4 的倍数，**没有任何填充**（`sizeof` = 7×4 = 28）。
- **`BSRR=16`** 正好是 Day2 反汇编里 `str r1, [r0, #16]` 的那个 `#16`。
  两条独立证据（主机 `offsetof`、目标反汇编）互相印证：`HAL_GPIO_WritePin` 写的就是 BSRR。

真实的 HAL 定义在 `stm32f103xb.h:357-366`：

```c
typedef struct
{
  __IO uint32_t CRL;
  __IO uint32_t CRH;
  __IO uint32_t IDR;
  __IO uint32_t ODR;
  __IO uint32_t BSRR;
  __IO uint32_t BRR;
  __IO uint32_t LCKR;
} GPIO_TypeDef;
```

`__IO` 展开为 `volatile`。这不是修饰符洁癖：**没有 `volatile`，编译器有权把两次读寄存器合并成一次**，
而外设寄存器的值可能被硬件改动。例如"轮询等待 `TXE` 置位"的循环，优化后可能变成死循环。

**字段顺序就是契约**。多写一个 `RESERVED`、少写一个字段、或者顺序写反，
所有后续偏移整体错位——编译不会报错，运行期写到别的寄存器上。

### 4. 一处需要纠正的教材示例

教材 5.6.2 节末尾的示例：

```c
GPIO_REGS *gpioa = (GPIO_REGS *)0x40011000;
gpioa->BSRR = (1<<13);
```

变量名叫 `gpioa`，但 `0x40011000` 是 **GPIOC** 的基址。三项独立核对：

```text
stm32f103xb.h:575   #define PERIPH_BASE      0x40000000UL
stm32f103xb.h:583   #define APB2PERIPH_BASE  (PERIPH_BASE + 0x00010000UL)
stm32f103xb.h:602   #define GPIOA_BASE       (APB2PERIPH_BASE + 0x00000800UL)   → 0x40010800
stm32f103xb.h:604   #define GPIOC_BASE       (APB2PERIPH_BASE + 0x00001000UL)   → 0x40011000
```

再加 Day2 的 `objdump` 字面量池 `0x40011000`（来自 `GPIOC` 宏），共三条证据。
**变量名是笔误，地址是对的**——所以按教材代码写 LED 能亮，但把它当"GPIOA"去理解就会错。
这也是读教材的一条通用纪律：**符号名和数值冲突时，查头文件，不猜。**

### 5. 位运算：三条惯用法与一个真实的优先级陷阱

教材 5.6.5 节给了三种形态，实测（`lab_c_basics` 的 `[4]` 组）：

```text
set   bit7,bit8 : 0x123 -> 0x1A3        a | (1<<7) | (1<<8)
clear bit7,bit8 : 0x123 -> 0x023        a & ~((1<<7) | (1<<8))
set field[8:7]=3: 0x123 -> 0x1A3        a & ~(3<<7)  再 | (val<<7)
```

"设置一段字段为某个值"必须**先清后或**，两步不能合成一步：直接 `|` 会把旧值里该段残留的 1 留下。

现在看课程驱动里的一处真实缺陷（`driver_ds18b20.c:463-467`）：

```c
if (ram[4] & (3<<5) == 0)              return ...;  /* 9bit  */
else if (ram[4] & (3<<5) == (1<<5))    return ...;  /* 10bit */
else if (ram[4] & (3<<5) == (2<<5))    return ...;  /* 11bit */
```

C 的优先级里 `==` **高于** `&`，所以实际结合是 `ram[4] & ((3<<5) == 0)`。
而 `(3<<5)` = 96，`96 == 0` 是 0，于是整条条件变成 `ram[4] & 0`，**恒为 0（假）**。

本篇程序把这三行原样复刻了一份，同时写一份加括号的正确版本，实测对照：

```text
[3] 表达式自检: (3<<5)=96  ((3<<5)==0)=0  (0x60 & ((3<<5)==0))=0
[3] cfg=0x00  buggy=12bit  fixed=9bit
[3] cfg=0x20  buggy=12bit  fixed=10bit
[3] cfg=0x40  buggy=12bit  fixed=11bit
[3] cfg=0x60  buggy=12bit  fixed=12bit
```

**四种配置下 buggy 版本一律返回 `12bit`**——分辨率分支形同虚设，无论配置寄存器写什么，
驱动都按 12 位解析。这是"代码能编译、能运行、读数也在合理范围，但功能是错的"的典型。

编译器其实早就说了。构建日志里正好三条（`logs/build_2026-10-29.log`）：

```text
driver_ds18b20.c:463:20: warning: suggest parentheses around comparison in operand of '&' [-Wparentheses]
driver_ds18b20.c:465:25: warning: suggest parentheses around comparison in operand of '&' [-Wparentheses]
driver_ds18b20.c:467:25: warning: suggest parentheses around comparison in operand of '&' [-Wparentheses]
```

主机侧编译本篇程序时，`gcc 15.1.0` 也报了同样 3 条。**跨编译器一致**说明这不是某个编译器的怪癖——
`-Wparentheses` 是能直接抓住这类错误的开关，而它的默认开启等级是 `-Wall`。
本工程保留这 13 条告警而不加 `-Wno-*`，理由就在这里：**告警是缺陷线索，不是噪音**。

### 6. 字节序

同一段数据在内存里的排列有两种约定。实测本机：

```text
[5] 0x12345678 在内存中的低->高地址字节: 78 56 34 12 (little endian)
```

低字节存低地址 = 小端。ARM Cortex-M 默认小端，所以"在 PC 上验证过的字节序假设"搬过去仍然成立——
但**这条恰好是少数可以直接搬的结论**，不要因此以为别的东西也能搬（见 §2 的指针大小）。

字节序真正会咬人的地方是**跨设备传数据**：把 `uint32_t` 直接 `memcpy` 进串口缓冲区发给 PC，
两端字节序不同就要翻转。STM32 与 x86 都是小端，所以用"结构体直接发"的写法在本组合下**碰巧**能通，
换一个设备就不行。正确做法是显式按字节组装（本仓库 `aviation-node` 的下行帧就是这么做的）。

---

## 系统调用/接口卡片

### `offsetof(type, member)`

作用：编译期求某成员在结构体内的字节偏移。验证"结构体布局 = 寄存器布局"的唯一直接手段。

原型：`size_t offsetof(type, member)`（`<stddef.h>`）

参数：`type` 是结构体类型名，`member` 是成员名（**不带** `->`）。

返回值和错误：编译期常量，无错误。

资源所有权：编译期展开，无运行期行为。

最小示例：

```c
printf("BSRR=%zu\n", offsetof(GPIO_REGS_DEMO, BSRR));   /* 16 */
```

常见误用：用 `sizeof` 反推偏移（`sizeof` 含填充）；或者用结构体指针地址相减（运行期、且依赖布局）。

### `volatile`（`__IO`）

作用：告诉编译器"这个对象的值可能在两条语句之间被外部改变"，禁止合并、消除、重排相关访问。

原型：`__IO uint32_t BSRR;` → `volatile uint32_t BSRR;`

参数：修饰类型。

返回值和错误：无。

资源所有权：无。

最小示例：

```c
while (!(USART1->SR & USART_SR_TXE)) { }   /* 没有 volatile 时可能变成死循环 */
```

常见误用：以为 `volatile` 能保证原子性。**不能**——它只约束编译器的优化，不约束 CPU 的读写拆分，
也不提供任何互斥。要原子就用 `BSRR` 这类单次写，或关中断。

### 位运算三件套

| 意图 | 写法 | 要点 |
| --- | --- | --- |
| 置位 | `x \|= (1u << n)` | 只影响第 n 位 |
| 清位 | `x &= ~(1u << n)` | 必须取反成掩码 |
| 设一段字段 | `x = (x & ~(mask << shift)) \| (val << shift)` | **先清后或**，两步 |
| 判断字段 | `(x & (mask << shift)) == expected` | **括号必须加在外层** |

常见误用：`if (x & flag == 0)`。`==` 优先于 `&`，等价于 `x & (flag==0)`。
编译器在 `-Wall` 下会报 `-Wparentheses`，但只在"比较对象看起来像常量"时才报得准。

---

## 最小可运行实验

目标：在主机上验证四条前提（指针大小、结构体偏移、位运算优先级、字节序）。

工程与源码：[`experiments/F103_HAL_DriverLab/lab_c_basics/lab_c_basics.c`](../../experiments/F103_HAL_DriverLab/lab_c_basics/lab_c_basics.c)

```bash
cd experiments/F103_HAL_DriverLab/lab_c_basics
gcc -std=c11 -Wall -Wextra -O0 lab_c_basics.c -o lab_c_basics
./lab_c_basics
```

---

## 编译与运行

构建时会出现 3 条 `-Wparentheses`——**这是预期结果**，程序的 `accuracy_buggy()` 就是故意复刻缺陷的，
详见下文"结果观察"。

---

## 结果观察

完整输出（实测，原样抄录）：

```text
[1] sizeof: void*=8  int*=8  short*=8  long*=8  char*=8
[1] sizeof: uint8_t=1  uint16_t=2  uint32_t=4  uint64_t=8  long=4
[2] offsetof: CRL=0 CRH=4 IDR=8 ODR=12 BSRR=16 BRR=20 LCKR=24
[2] sizeof(GPIO_REGS_DEMO)=28
[3] 表达式自检: (3<<5)=96  ((3<<5)==0)=0  (0x60 & ((3<<5)==0))=0
[3] cfg=0x00  buggy=12bit  fixed=9bit
[3] cfg=0x20  buggy=12bit  fixed=10bit
[3] cfg=0x40  buggy=12bit  fixed=11bit
[3] cfg=0x60  buggy=12bit  fixed=12bit
[4] set   bit7,bit8 : 0x123 -> 0x1A3
[4] clear bit7,bit8 : 0x123 -> 0x023
[4] set field[8:7]=3: 0x123 -> 0x1A3
[5] 0x12345678 在内存中的低->高地址字节: 78 56 34 12 (little endian)
```

四处读数各自证明了什么：

| 读数 | 证明 |
| --- | --- |
| `void*=8 …` | 本机指针 8 字节；教材"32 位下 4 字节"只对目标成立 |
| `long=4` | Windows 是 LLP64，教材"64 位下 long 为 8 字节"**不成立** |
| `BSRR=16`、`sizeof=28` | 无填充、偏移严格 4 字节递增；与目标反汇编的 `[r0, #16]` 一致 |
| `buggy=12bit` 四条 | 缺陷在主机上可复现，与目标构建日志的 3 条告警互为证据 |
| `78 56 34 12` | 小端；STM32 与 x86 相同，这条假设可以搬 |

---

## 常见错误和陷阱

| 陷阱 | 症状 | 根因 | 处理 |
| --- | --- | --- | --- |
| `if (x & flag == 0)` | 分支永远走不到 | `==` 优先于 `&` | 外层加括号；开 `-Wall` |
| 结构体字段漏写/多写 | 写到错误寄存器，无编译错误 | 布局契约被破坏 | 用 `offsetof` 逐项对齐数据手册 |
| 寄存器指针不加 `volatile` | 轮询循环优化成死循环 | 编译器认为值不会变 | 用 `__IO`/`volatile` |
| 在主机上验证过就认为目标也对 | `sizeof`、`long` 不一致 | LLP64/LP64 差异 | 用定宽类型；指针大小按目标核对 |
| 多个字段读改写不用掩码 | 相邻字段被清零 | 只置位没清位 | `x = (x & ~(m<<s)) \| (v<<s)` |
| 把 `int` 直接 `memcpy` 发出去 | 换设备后数据错 | 依赖字节序 | 显式按字节组装 |
| 以为 `volatile` 保证原子 | 偶发数据撕裂 | 只约束编译器 | 用单次写或临界区 |

---

## 故障排查方法

位运算类 bug 的排查有一个很轻的入口：**把表达式单独算一遍**。

1. **先分离**。把 `if (a & b == c)` 里的三个子表达式分别打印出来——本篇程序的 `[3]` 组就是这个动作。
   一个恒假/恒真的条件，几乎都在这一步现形。
2. **再看告警**。`-Wall -Wextra` 加上去重编译一次。`-Wparentheses` 专门抓运算符优先级，
   `-Wchar-subscripts` 抓有符号下标，`-Wunused-function` 抓死代码。本节涉及的 13 条告警里有 5 条是真缺陷。
3. **核对布局**。寄存器写错位置时，用 `offsetof` 把结构体偏移打出来，与数据手册的寄存器表逐行对。
4. **最后怀疑编译器**。`-O0` 与 `-O2` 各编一次对照——如果行为不同，多半是缺 `volatile`
   或某处 UB，而不是编译器错了。

---

## 练习题

### 基础题

题目：在本篇的 `GPIO_REGS_DEMO` 里插一个 `uint16_t XX;` 字段放在 `IDR` 之后，重新跑 `offsetof`，
写出 `ODR`/`BSRR` 的新偏移，并解释为什么会发生这个变化。

提示：`uint32_t` 需要 4 字节对齐。

验收标准：能指出出现了填充（`ODR` 偏移不再是 12），并说明"结构体字段顺序与类型都不能随便改"。

### 改错题

题目：下面的代码有什么问题？

```c
#define REG  (*(volatile uint32_t *)0x4001100C)
...
REG |= (1 << 13);   /* 想把 PC13 拉高 */
REG |= (1 << 14);   /* 想把 PC14 拉高 */
```

提示：这段代码在 `ODR` 上做读改写，且中间可能有中断。

验收标准：能指出"读改写非原子"这一缺陷，并能构造"中断改 PC14 → 主循环写回把 PC14 覆盖"的执行序列；
修正方向应指向 `BSRR`。

### 综合题

题目：写一段不依赖结构体的代码，只用 `uint32_t` 指针把 `GPIOC` 的 `CRH` 的 13 号引脚配成
"2 MHz 推挽输出"，复位该引脚的其余位，然后点亮 LED。要求不改变其他 15 个引脚的配置。

提示：`CRH` 偏移 0x04；13 号引脚的位域是 bit[23:20]；先清后或。

验收标准：能写出正确的掩码与偏移，并说明为什么清零那一步不能省；
能算出 `0x4444_4444` 里 13 号位域的原值并说明它对应什么模式。

---

## 验收标准

- [ ] 能解释"写地址 = 选中外设"的译码过程，并说明为什么写到未映射地址会 HardFault。
- [ ] 能说出教材"指针都是 4 字节"在什么前提下成立，以及本机为什么读到 8。
- [ ] 能说明 `long` 在 Windows 与 Linux 上的差异（LLP64 vs LP64），并给出跨平台写法。
- [ ] 能用 `offsetof` 证明 `BSRR` 在偏移 16，并与目标反汇编的 `[r0, #16]` 对上。
- [ ] 能独立跑出 `buggy=12bit` 四条读数，并解释 `==` 优先于 `&` 的完整结合过程。
- [ ] 能纠正教材示例里 `gpioa` 与 `0x40011000` 的不一致，并说出三条核对依据。

## 延伸阅读

- 教材：《HAL快速入门与项目实战(基于DshanMCU-103)》第 5.6 节（指针、结构体、进制、字节序、位操作）
- 实验源码：[`experiments/F103_HAL_DriverLab/lab_c_basics/lab_c_basics.c`](../../experiments/F103_HAL_DriverLab/lab_c_basics/lab_c_basics.c)
- HAL 定义：`Drivers/CMSIS/Device/ST/STM32F1xx/Include/stm32f103xb.h:357-366`（`GPIO_TypeDef`）、
  `:575-604`（基址宏）
- 已知缺陷：[`experiments/F103_HAL_DriverLab/README.md`](../../experiments/F103_HAL_DriverLab/README.md) §10.1
- 下一篇：`Linux/notes/F103_HAL入门/Day04_GPIO工作模式与HAL库彻底分析.md`

---

**验证记录**

| 项目 | 内容 |
| --- | --- |
| 日期 | 2026-10-29 |
| 环境一 | Windows 11 + Git Bash；MinGW `gcc` 15.1.0（x86_64-posix-seh） |
| 命令一 | `gcc -std=c11 -Wall -Wextra -O0 lab_c_basics.c -o lab_c_basics.exe && ./lab_c_basics.exe` |
| 结果一 | 同"结果观察"的 12 行输出；编译期 3 条 `-Wparentheses`（与目标构建日志同一缺陷） |
| 环境二 | 同上 |
| 命令二 | `arm-none-eabi-objdump -d --disassemble='HAL_GPIO_WritePin' build/f103_hal_driverlab.elf` |
| 结果二 | `str r1, [r0, #16]` → `#16` 与主机侧 `offsetof(BSRR)=16` 一致 |
| 未覆盖 | `sizeof` 在 STM32 目标上的实测值（本篇为 x86_64 读数，目标侧按 32 位推断为 4，未在板上验证）；`-O2` 下缺 `volatile` 的失败复现（只做了源码说明，未构造对照实验） |
