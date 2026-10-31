# Day1：开发环境与工具链——从 Keil 到 GCC

**验证状态**：构建链**已实测**（Windows 11 + Git Bash，`arm-none-eabi-gcc` 14.2.1 / CMake 4.0.3 / Ninja 1.13.2，
`experiments/F103_HAL_DriverLab/logs/build_2026-10-29.log`）；**烧写与调试链未验证**（本机无 ST-Link 在手）；
教材侧的环境要求取自 `HAL快速入门与项目实战(基于DshanMCU-103)_20240130.pdf` 第 2–3 章。

---

## 学习目标

学完本篇应能用代码证据回答：

1. 教材要求装哪四类软件，本仓库的实验工程为什么一类都没装（只装了工具链 + CMake + OpenOCD），
   以及两者的**对应关系**是什么——Keil 的哪个功能由哪个 GCC 组件顶替。
2. 一次构建从 `.c` 到 `.bin` 经过哪四个阶段，每个阶段失败时症状有什么不同。
3. `-nostartfiles` 省掉的是什么，省掉之后**必须**补哪两个文件，不补会在哪里暴露。
4. 为什么"编译通过"不能证明工具链配对正确——用哪两条命令能证明它真的配对了。
5. 教材 3.4 节那三组引脚冲突，背后的物理约束分别是什么。

---

## 环境与前置条件

| 项 | 教材要求 | 本工程实测版本 | 是否需要 |
| --- | --- | --- | --- |
| 编译器 | Keil MDK（ARMCC/ARMCLANG） | `arm-none-eabi-gcc` 14.2.1 | 二选一 |
| 工程生成 | STM32CubeMX | STM32CubeMX（只用于生成 `Core/`，已随工程保存） | 需要，但不必现装 |
| 烧写 | STM32CubeProgrammer 或 Keil | xPack OpenOCD 0.12.0 | 二选一 |
| 调试 | Keil 内建调试窗口 | `cortex-debug` + OpenOCD（Cursor/VS Code） | 二选一 |
| 芯片支持包 | `Keil.STM32F1xx_DFP.2.3.0.pack` | `Drivers/CMSIS/` + `Drivers/STM32F1xx_HAL_Driver/`（源码随工程） | GCC 侧不需要 Pack |
| 串口驱动 | CH340 驱动 | 同（`printf` 走 USART1，经 USB 串口模块） | 需要 |
| 硬件 | DshanMCU-103 + ST-Link V2 | 同 | 需要 |

实测环境（本机 `arm-none-eabi-gcc --version` 原文）：

```text
arm-none-eabi-gcc (Arm GNU Toolchain 14.2.Rel1 (Build arm-14.52)) 14.2.1 20241119
```

前置知识：会写 C 的指针与结构体（教材第 5.6 节会补，但先有概念更省时间）；知道 `make` 之类构建工具存在即可。

---

## 核心概念

### 1. 两条工具链的角色对应表

教材全程用 Keil，是因为它对 Windows 初学者最省事：双击 `.uvprojx` 就有编译、烧写、断点三件套。
本仓库换成 GCC，不是为了"更专业"，而是因为 Keil 只能在 Windows 上跑、命令行不可脚本化、
`.uvprojx` 是二进制 XML 不适合进版本库做 diff。

换了之后每一个功能都得找到对应的顶替者，这张表就是全部答案：

| 能力 | Keil 侧 | GCC 侧 | 对应关系 |
| --- | --- | --- | --- |
| 编译器 | `armcc` / `armclang` | `arm-none-eabi-gcc` | 同目标、不同前端 |
| 汇编器 | `armasm`（ARM 语法） | `arm-none-eabi-as`（GNU 语法） | **语法不兼容**，启动文件必须换一份 |
| 链接脚本 | `.sct` 分散加载 | `.ld` 链接脚本 | **格式完全不同**，手写一份 |
| C 运行时 | ARMCC 自带 crt0 | newlib + `crt0.o` | 本工程 `-nostartfiles` 后自补桩 |
| `printf` 落地 | 覆盖 `fputc`（MicroLIB） | 覆盖 `_write`（newlib） | **函数名不同** |
| 工程描述 | `.uvprojx` | `CMakeLists.txt` | 文本 ↔ 二进制 XML |
| 烧写 | Keil Download 按钮 | `openocd -c "program ... verify reset exit"` | 同一套 SWD 协议 |
| 调试 | Keil 调试窗口 | OpenOCD + GDB（由 `cortex-debug` 驱动） | 同一套 GDB 远程协议 |

这张表有一个可以直接推断的结论：**换工具链不是"换个编译器"，凡是与编译器绑定的产物都要换**。
启动文件、链接脚本、C 运行时、`printf` 重定向这四处，就是全部绑定点（下一节原理解释，验证方法见 §4.3）。

### 2. 四个构建阶段与各自的失败特征

```text
   .c  ──编译──►  .o  ──链接──►  .elf  ──objcopy──►  .hex / .bin
   .s  ──汇编──►              （地址已定）          （纯二进制，烧写用）
```

| 阶段 | 输入 | 工具 | 失败时的典型症状 | 问题的性质 |
| --- | --- | --- | --- | --- |
| 预处理 | `.c` + 头文件 | `gcc -E` | `fatal error: xxx.h: No such file` | 头文件搜索路径 |
| 编译 | `.c` | `gcc -c` | `implicit declaration`、类型告警 | 源码本身 |
| 汇编 | `.s` | `as` | `Error: bad instruction`、`unexpected` | **汇编语法方言** |
| 链接 | `.o` + `.a` + `.ld` | `ld` | `undefined reference to`、`region RAM overflowed` | 符号缺失 / 内存不够 |
| 转格式 | `.elf` | `objcopy` | 几乎不失败 | —— |

判断问题出在哪一阶段，是排查的第一步。两条经验：

- **`undefined reference to 'HAL_xxx'` 属于链接期，不是编译期**。它的意思是"某个 `.c` 没被编进来"，
  而不是"头文件没找到"。前者去查 `CMakeLists.txt` 的源文件列表，后者去查 `include_directories`，
  两个动作完全不同。
- **`bad instruction` 几乎一定是汇编方言问题**。GCC 的 `.s` 要过 `arm-none-eabi-as`，
  Keil 的 `.s` 要过 `armasm`，两者语法不同源。教材工程的启动文件拿给 GCC 编，第一行就报错。

### 3. `-nostartfiles` 省掉了什么

Cortex-M3 复位后硬件只做一件事：从 `0x08000000` 读第一个字当 MSP、第二个字当 PC。
**后面全是软件的事**：把 `.data` 从 Flash 搬到 RAM、把 `.bss` 清零、调 `main`。这段软件就叫启动代码。

教材的 Keil 工程里，这段由 `startup_stm32f103xb.s`（ARM 语法）+ ARMCC 的 crt0 分工完成。
GCC 这边如果不管，默认会链接 `crt0.o`，它也会做一遍 `.data/.bss`，于是与启动文件重复。
所以本工程用 `-nostartfiles` 把 crt0 摘掉，让启动文件独占复位流程。

代价立刻出现两处，这就是 `Core/Src/crt_stubs.c` 存在的全部理由：

```c
void _init(void) { }   /* newlib 的 __libc_init_array() 会调它，而 crti.o 已不再链接 */
void _fini(void) { }
void *_sbrk(ptrdiff_t incr);   /* malloc/printf 的缓冲都要堆 */
int  _isatty(int fd)  { return 1; }   /* 影响 stdout 是行缓冲还是全缓冲 */
int  _fstat / _close / _lseek / _read;  /* 只为消掉 libnosys 告警 */
```

其中 `_isatty` 最容易忽略却影响最大：返回 0（"不是终端"）会让 newlib 把 stdout 变成**全缓冲**，
表现是 `printf` 明明执行了却不出字，直到缓冲区满或程序退出。本工程返回 1，让 stdout 走行缓冲。

第二处代价是 `_write`，它必须绑在具体串口上，所以单独放在 `Core/Src/retarget.c`，
与硬件无关的桩分开放——换串口只改一行，关掉 stdio 只需不编译这个文件。

### 4. 我在哪一层改代码

工程里有三份来源，混在一起改会互相覆盖：

| 来源 | 文件 | 策略 |
| --- | --- | --- |
| CubeMX 生成 | `gpio.c` `usart.c` `i2c.c` `spi.c` `adc.c` `tim.c` `stm32f1xx_it.c` `stm32f1xx_hal_msp.c` `stm32f1xx_hal_timebase_tim.c` `system_stm32f1xx.c` | **不手改**。要改就回 CubeMX 改再重新生成 |
| 课程官方驱动 | `Drivers/DshanMCU-F103/driver_*.c` | **不手改**。已知缺陷只记录不修（保可比性） |
| 本工程补充 | `main.c` `crt_stubs.c` `retarget.c` `lab_pins.c` `CMakeLists.txt` `ld/` `startup/` `.clangd` | 可改，每个文件头写明"为什么需要它" |

判断标准很简单：**这个文件能不能被工具重新生成**。能，就别手改，下次生成就丢。

### 5. 教材 3.4 节的三组引脚冲突，物理原因是什么

教材第 3.4 节只给结论和一张表，没有解释。结论背后的约束是"一根线不能同时是两种东西"：

| 冲突 | 模块 | 共用引脚 | 物理原因 |
| --- | --- | --- | --- |
| M2 / M10 | DS18B20 / DHT11 | **PA1** | 两者都是单总线协议，靠同一根线的电平时序通信，无法共享 |
| M4 / M9 | 红外发射 / 无源蜂鸣器 | **PA8（TIM1_CH1）** | 一个定时器通道同一时刻只能输出一个占空比/频率 |
| M6 / M12 | 超声波 / SPI Flash | **PB9** | 能同时接但**不能同时访问**：W25Q64 的 CS 低有效（空闲高），SR04 的 TRIG 靠高脉冲触发（空闲低） |

第三条最值得琢磨：它不是"不能接"，而是"空闲电平要求相反"。本工程 `lab_pins.c` 取低，
让 SR04 可用；此时 flash 只是被选中而收不到 SCLK，不会误执行命令。
**这条是源码推论，没有用逻辑分析仪确认过。**

---

## 系统调用/接口卡片

### `int _write(int fd, const char *buf, int len)`

作用：newlib 的 `printf`/`fwrite` 最终调它把字节送出去。覆盖它 = 完成 `printf` 重定向。

原型：

```c
int _write(int fd, const char *buf, int len);
```

参数：`fd` 通常是 1（stdout）或 2（stderr），本实现忽略；`buf` 是**可能含 `'\0'` 的字节流**；
`len` 是字节数。

返回值和错误：成功返回实际写入字节数；失败返回 `-1` 并置 `errno`。
**返回小于 `len` 会被 newlib 当成短写**，可能重试或截断，所以这里要么返回 `len`，要么返回 `-1`。

资源所有权：不拥有 `buf`，不得保留指针。

线程/进程安全：本工程是裸机单线程，无所谓；若将来在中断里调 `printf`，必须换成
环形缓冲 + DMA，否则阻塞发送会长时间占用中断上下文。

最小示例：

```c
int _write(int fd, const char *buf, int len)
{
    (void)fd;
    if (len <= 0) return 0;
    if (huart1.gState == HAL_UART_STATE_RESET) { errno = EIO; return -1; }
    if (HAL_UART_Transmit(&huart1, (uint8_t *)buf, (uint16_t)len,
                          (uint32_t)len * 10U) != HAL_OK) { errno = EIO; return -1; }
    return len;
}
```

常见误用：

- 把它写成 `while (*buf) USART_SendData(*buf++)`：遇到 `'\0'` 就提前结束，`printf("%c", 0)` 失效。
- 用 `Instance` 判空判断"串口是否初始化"：CubeMX 生成的句柄是全局零初始化，
  但 `HAL_UART_Init` 成功前也可能已赋过 `Instance`。要看 `gState`。
- 忘了 `(uint16_t)` 转换：`HAL_UART_Transmit` 的长度参数是 `uint16_t`，超 65535 会被截断。

### `void *_sbrk(ptrdiff_t incr)`

作用：给 newlib 的 `malloc` 提供堆增长。不实现它，`printf` 内部一申请缓冲就进 HardFault。

参数：`incr` 为要增加的字节数，可为负（释放）。

返回值和错误：成功返回**增长前的堆顶**；失败返回 `(void *)-1` 并置 `errno = ENOMEM`。

资源所有权：返回的内存由 newlib 管理，本函数只负责推进指针。

最小示例（本工程的实际边界）：

```c
extern char end;                 /* 链接脚本导出：._user_heap_stack 起点 */
extern char _Min_Heap_Size;      /* 链接脚本的变量，取地址才是数值 */

s_heap_cur = &end;
s_heap_end = &end + (uintptr_t)&_Min_Heap_Size;   /* 512 B */
```

常见误用：用 `_estack - _Min_Stack_Size` 当堆上界。运行期栈从 `_estack` 向下长，
两者之间隔着一整段空闲 RAM，用这个上界会让堆和栈真的撞上。**以链接脚本的预留区为界**，
运行期预算才与链接期检查一致。

### `arm-none-eabi-size [-A] <elf>`

作用：看体积。`-A` 输出分节明细。

参数：不带 `-A` 时把 `.data+.bss` 之类合并显示；`-A` 逐节列出。

返回值和错误：纯输出，无副作用。

常见误用（本仓库踩过两次）：**默认输出的 `bss` 含堆栈预留区**。本工程 `bss 3176`
= `.bss 1636` + `._user_heap_stack 1540`（`_Min_Heap_Size` 512 + `_Min_Stack_Size` 1024）。
1 540 B 是链接期为溢出检查留的位置，不是运行期消耗。看 RAM 只有 `size -A` 一个口径可用。

---

## 最小可运行实验

目标：只做"能构建 + 能读体积 + 能证明工具链配对了"，不碰硬件。

```bash
cd experiments/F103_HAL_DriverLab
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
```

---

## 编译与运行

构建（实测命令，exit=0）：

```bash
cmake --build build
arm-none-eabi-size {,-A} build/f103_hal_driverlab.elf
arm-none-eabi-nm --size-sort -S build/f103_hal_driverlab.elf
xxd -l 8 build/f103_hal_driverlab.bin
```

烧写（硬件到位后；本机未验证）：

```bash
cmake --build build --target flash
# 展开为：openocd -f ../F103_common/openocd/stlink_f103.cfg \
#          -c "program build/f103_hal_driverlab.elf verify reset exit"
```

`CMakeLists.txt` 用 `find_program(openocd)` 判断，找不到时**不注册** `flash` 目标，
而不是注册一个必然失败的目标。

---

## 结果观察

**构建**（`logs/build_2026-10-29.log`）：

```text
0 error / 13 warning
13 条全部来自课程驱动：driver_ds18b20.c 3 条（-Wparentheses）
                     driver_oled.c 2 条（-Wchar-subscripts）+ 8 条（-Wunused-function）
本工程新增文件（main.c / crt_stubs.c / retarget.c / lab_pins.c）零告警
```

**体积**：

```text
text  15276    data  100    bss  3176
.isr_vector 268 / .text 14780 / .rodata 220 / .ARM 8 / .data 100
.bss 1636 / ._user_heap_stack 1540
```

Flash = text + data = 15 376 / 65 536 = **23.5 %**；
RAM 静态 = data + bss = 3 276 / 20 480 = **16.0 %**，其中 1 540 B 是预留。

**工具链配对的两条硬证据**（这两条才是"配对正确"，体积不是）：

```text
$ xxd -l 8 build/f103_hal_driverlab.bin
00000000: 0050 0020 0528 0008      → SP=0x20005000，PC=0x08002804（末位 1 = Thumb）

$ arm-none-eabi-nm build/f103_hal_driverlab.elf | grep Reset_Handler
08002804 T Reset_Handler
```

向量表前两个字与 `nm` 里的符号地址**互相对上**，说明：链接脚本把 `.isr_vector` 放在了 Flash 起始、
启动文件被正确汇编、`ENTRY(Reset_Handler)` 生效。任何一环错，这四个字节就对不上。

---

## 常见错误和陷阱

| 陷阱 | 症状 | 根因 | 处理 |
| --- | --- | --- | --- |
| 用 Keil 的启动文件 | `Error: bad instruction` | ARM 汇编语法 vs GNU 汇编语法 | 换 `startup_stm32f103xb.s` 的 GCC 版本 |
| 不加 `-nostartfiles` | 启动流程被执行两次 | `crt0.o` 与启动文件重复做 `.data/.bss` | 加 `-nostartfiles` |
| 加了 `-nostartfiles` 但不补桩 | 4 条 `_close/_lseek/_read/_write ... will always fail` 告警 | `crti.o` 与 libnosys 不再链接 | 补 `crt_stubs.c` |
| `printf` 不出字 | 程序在跑，串口没输出 | ① `_write` 没实现 ② `_isatty` 返回 0 导致全缓冲 | 实现 `_write`，`_isatty` 返回 1 |
| 两套重定向并存 | 不报错、也能出字 | `fputc`（MicroLIB）与 `_write`（newlib）同时存在，只有后者生效 | `nm` 查符号，别靠"能跑"判断 |
| 把 `size` 的 bss 当 RAM 消耗 | 余量算错 | 含 `._user_heap_stack` 预留 | 用 `size -A` |
| 改 `LAB_TEST` 体积不变 | 编出来的还是旧模块 | `LAB_TEST` 是 CMake cache 变量 | 重新 configure |
| `undefined reference to 'HAL_GPIOEx_ConfigFaultEvent'` | 链接失败 | `HAL_GPIO_MODULE_ENABLED` 需要连带 `stm32f1xx_hal_gpio_ex.c` | 两边对齐 |

---

## 故障排查方法

按"哪一阶段失败"分四步走，每步只做一件事：

1. **定位阶段**。看报错关键词：`No such file` → 预处理；`implicit declaration` → 编译；
   `bad instruction` → 汇编；`undefined reference` → 链接。**不要在这一步猜根因。**
2. **链接期缺符号**，依次查三处：
   - `stm32f1xx_hal_conf.h` 是否 `#define HAL_XXX_MODULE_ENABLED`；
   - `CMakeLists.txt` 的 `HAL_SOURCES` 里有没有对应 `.c`（有的模块还要 `_ex.c`）；
   - 这个符号是不是课程驱动里 `static` 的函数（`static` 不外链，链接期当然找不到）。
3. **编译期缺头文件**，看 `build/compile_commands.json` 里那条命令的 `-I` 列表，
   而不是看 IDE 的红波浪线——IDE 的索引（clangd）与真正编译用的是两套路径。
4. **能构建但疑似配置错**，去查符号表和产物，不要看代码：
   - `nm` 查某个函数在不在镜像里；
   - `xxd -l 8` 查向量表；
   - `size -A` 查分节布局。

第 4 步是本仓库最重要的一条纪律：**构建绿 ≠ 配置对**。少配 8 个 GPIO 也照样 0 error，
因为驱动只是在运行时读写寄存器——编译器对引脚配置完全不敏感。

---

## 练习题

### 基础题

题目：用 `size -A` 与不带 `-A` 的两条命令各看一次，写出两个 bss 数字，并说明差额来自哪个段、为什么它不该算作 RAM 消耗。

提示：差额正好是 `_Min_Heap_Size` 与 `_Min_Stack_Size` 之和；这两个量在 `ld/STM32F103C8Tx.ld` 里。

验收标准：两个数字正确，能指出差额段名 `._user_heap_stack`，并能说清"预留"与"消耗"的区别。

### 改错题

题目：下面的 `_write` 有什么问题？

```c
int _write(int fd, char *buf, int len)
{
    while (len--) {
        while (!(USART1->SR & USART_SR_TXE));
        USART1->DR = *buf++;
    }
    return 0;
}
```

提示：看返回值的语义，再看 `len` 的类型与 `HAL_UART_Transmit` 的长度参数类型。

验收标准：能指出"返回 0 会被 newlib 当成短写"和"绕过 HAL 直接写寄存器会破坏 HAL 的状态机"
两个问题，并给出修正版本（至少修返回值）。

### 综合题

题目：不重新生成工程的前提下，把 `printf` 从 USART1 改到 USART2，需要改哪几个文件、哪几行？
说明为什么这个改动量是"小的"，以及 `.clangd` 里为什么要跟着改同一处路径。

提示：`retarget.c` 只有一处用了 `huart1`；`usart.c` 里 `MX_USART2_UART_Init` 需要先存在。

验收标准：能列出改动的文件与行，能说明"补充文件与硬件解耦"带来的好处；
`.clangd` 那一问能答出"工具链升级后绝对路径变了，`--query-driver` 的 glob 与 `Compiler:` 必须同时匹配"。

---

## 验收标准

- [ ] 能说出 Keil 与 GCC 两条链上"编译器/汇编器/链接脚本/运行时/`printf`/工程描述"六项的一一对应。
- [ ] 能按报错关键词判断失败发生在四个阶段的哪一个，并说出该阶段应该查什么。
- [ ] 能解释 `-nostartfiles` 省掉了什么、为什么必须补 `_init`/`_fini`/`_sbrk`/`_isatty`。
- [ ] 能用 `xxd` 与 `nm` 两条命令证明"工具链配对正确"，并说明为什么体积证明不了这件事。
- [ ] 能复述教材 3.4 节三组冲突，并解释第三条为什么是"能接不能同时访问"。
- [ ] 能独立完成一次全量重建，得到与本篇一致的 0 error / 13 warning 与体积读数。

## 延伸阅读

- 教材：`Linux_WEIDS/DShanMCU-F103/1_学习手册/HAL快速入门与项目实战(基于DshanMCU-103)_20240130.pdf`
  第 2 章（搭建开发环境）、第 3 章（开发板使用，含 3.4 注意事项的冲突表）
- 工程：[`experiments/F103_HAL_DriverLab/README.md`](../../experiments/F103_HAL_DriverLab/README.md)
- 方法论：[`notes/第三方驱动库迁移GCC_四处必改点与责任边界.md`](../第三方驱动库迁移GCC_四处必改点与责任边界.md)
- 速查表：[`knowledge_base/DShanMCU-F103模块与驱动_速查表.md`](../../knowledge_base/DShanMCU-F103模块与驱动_速查表.md)
- 下一篇：`Linux/notes/F103_HAL入门/Day02_芯片与GPIO模块_指针访问硬件.md`

---

**验证记录**

| 项目 | 内容 |
| --- | --- |
| 日期 | 2026-10-29 |
| 环境一 | Windows 11 + Git Bash；`arm-none-eabi-gcc` 14.2.1（Arm GNU Toolchain 14.2.Rel1, Build arm-14.52）；CMake 4.0.3；Ninja 1.13.2 |
| 命令一 | `cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && cmake --build build` |
| 结果一 | exit=0，0 error / 13 warning（全部来自课程驱动），`text 15276 / data 100 / bss 3176` |
| 环境二 | 同上 |
| 命令二 | `xxd -l 8 build/f103_hal_driverlab.bin`；`arm-none-eabi-nm build/f103_hal_driverlab.elf \| grep Reset_Handler` |
| 结果二 | `00 50 00 20 05 28 00 08` → SP=`0x20005000`、PC=`0x08002804`（Thumb 位已置）；`nm` 的 `Reset_Handler=0x08002804`，两者一致 |
| 未覆盖 | 烧写链路（`flash` 目标只验证了"命令能拼出来"）、OpenOCD 与 ST-Link 实际连通性、板上现象；教材第 2 章 Keil/CubeMX 安装步骤未在本机复现（无 Windows 版 Keil 环境） |
