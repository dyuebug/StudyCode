# F103_04：USART1 轮询收发与 printf 重定向

## 学习目标

1. **复用功能**：为什么串口引脚不能配成普通推挽输出，`CNF=10`（复用）与 `CNF=00`（通用）差在哪。
2. **波特率的来源**：`BRR` 由哪个时钟决定，为什么它对 `SystemCoreClock` 极其敏感。
3. **状态寄存器语义**：`TXE` 与 `TC` 的区别、`RXNE` 如何清除、`ORE` 为什么必须「先读 SR 再读 DR」。
4. **stdio 重定向**：`printf` 到串口要补哪些符号；`_isatty` 如何改变 stdout 的缓冲策略。

行为：上电打印一行配置信息，随后每秒打印一次心跳；从串口工具输入的字符原样回显。

## 硬件

| 项 | 值 |
| --- | --- |
| 串口 | USART1 |
| TX | PA9，复用推挽输出，50 MHz |
| RX | PA10，浮空输入 |
| 参数 | 115200 8N1，无硬件流控 |
| 主机侧 | 板载 USB 转串口（CH340 类）或外接 USB-TTL，3.3 V 电平 |

课程配置依据：`[07]_串口(UART)/0701_uart_poll/Core/Src/usart.c` —— `PA9 = USART1_TX`（`GPIO_MODE_AF_PP`、`HIGH`）、
`PA10 = USART1_RX`（`GPIO_MODE_INPUT`、`NOPULL`）、`BaudRate = 115200`。
PA10 采用浮空输入是因为主机 TX 始终驱动该线；若板上无外部驱动且线缆悬空，应改为上拉输入（`CNF=10` 且 `ODR10=1`）。

## 构建与烧写

```bash
cd Linux/experiments/F103_04_UART_Echo
cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake
cmake --build build
cmake --build build --target flash
```

烧写后用任意串口工具打开对应端口，**115200 8N1**，回车键按 `\r\n` 发送。

## 预期现象

```text
[F103_04] USART1 echo, PCLK2=8000000 Hz, BRR=0x045, 115200 8N1
[1000 ms] alive, rx=0, over=0
```
在其中输入字符会立即回显；换行后计数器递增，`over` 始终为 0。

## 实测记录

**构建实测**（2026-10-25，arm-none-eabi-gcc 14.2.1 + CMake 4.0.3 + Ninja 1.13.2，Windows 11）：

```text
Memory region         Used Size  Region Size  %age Used
           FLASH:        4948 B        64 KB      7.55%
             RAM:        1976 B        20 KB      9.65%
   text    data     bss     dec     hex filename
   4852      96    1880    6828    1aac f103_04_uart_echo.elf
```

零编译告警、零链接告警。相比实验 03（976 B）增加约 4 KB，全部来自 `printf` 及其 newlib 依赖；
`bss` 增加 332 B，是 newlib 静态分配的 `FILE` 结构与缓冲区。

**寄存器路径核对**（`f103_uart_init` / `uart_gpio_init` 反汇编，非推测；实现在 `F103_common/src/f103_uart.c`）：

| 代码意图 | 生成指令 | 结论 |
| --- | --- | --- |
| 开 GPIOA 时钟 | `orr #4` 到 `0x40021000+0x18` | APB2ENR bit2 = IOPAEN ✓ |
| 开 USART1 时钟 | `orr #0x4000` 到 `0x40021000+0x18` | bit14 = USART1EN ✓ |
| PA9 复用推挽、PA10 浮空输入 | `bic #0xFF0` / `orr #0x4B0` 到 `0x40010800+0x04` | CRH，PA9=0xB、PA10=0x4 ✓ |
| 关 CR1/CR2/CR3 后配 BRR | `str 0` 到 `+0x0C`/`+0x10`/`+0x14` | USART1 基址 0x40013800 ✓ |
| `BRR = (SystemCoreClock + baud/2) / baud` | `ldr [0x20000000]` → `add r2, r4,lsr #1` → `udiv` → `str [+0x08]` | 读的是 RAM 中的 `SystemCoreClock` 变量 ✓ |
| `UE \| TE \| RE` | `movw r2,#0x200C` → `str [+0x0C]` | UE=0x2000、TE=0x8、RE=0x4 ✓ |
| `_sbrk` 上界 | `end`(0x200001B8) + `_Min_Heap_Size`(0x200) | 堆上限 0x200003B8，与链接脚本预留一致 ✓ |

**QEMU 仿真实测**（2026-10-26，QEMU 10.2.1 `-M stm32vldiscovery`，`-DF103_QEMU=ON` 构建）：

```text
[F103_04] USART1 echo, PCLK2=8000000 Hz, 115200 8N1
[2000 ms] alive, rx=0, over=0
hello

[3000 ms] alive, rx=7, over=0
...
xyz

[6000 ms] alive, rx=12, over=0
```

输入 `hello\r\n`（7 字节）与 `xyz\r\n`（5 字节）后，`rx` 由 0 → 7 → 12，回显逐字节正确，`over` 始终 0。
说明轮询收发的寄存器路径在 QEMU 的 USART 模型上成立。原始日志：`Linux/tests/qemu-stm32/logs/exp04_echo.log`。

**未验证**：真实串口收发、波特率误差在实物上的容错、USB 转串口驱动的缓冲行为。需要接板与串口工具确认。
另外 QEMU 不做比特级移位，`BRR` 配错也照样收发，因此"115200 8N1"这一项**不能**由仿真结果背书；
QEMU 的 SYSCLK 是 24 MHz，心跳里的"毫秒"约为真实时间的 2.9 倍（见 `tests/qemu-stm32/RESULT.md` §3.3）。

## 关键实现点

### 1. 串口引脚必须是「复用」而不是「通用输出」

`CRH` 的 `CNF[1:0]` 决定引脚由谁驱动：

| 用途 | `CNF` | `MODE` | 本实验取值 |
| --- | --- | --- | --- |
| 通用推挽输出 | `00` | `11` | 0x3 |
| 复用推挽输出 | `10` | `11` | **0xB（PA9）** |
| 浮空输入 | `01` | `00` | **0x4（PA10）** |
| 上拉/下拉输入 | `10` | `00` | 0x8，方向由 `ODR` 决定 |

配成通用输出时，CPU 写 `ODR` 会直接驱动引脚，与 USART 外设的输出打架，现象是「能发但全是错码」或「完全无输出」。

### 2. `BRR` 完全取决于 PCLK2

同步/异步模式下，`baud = PCLK2 / BRR`（16 倍过采样）。PCLK2 由 `RCC->CFGR` 的 `PPRE2` 从 HCLK 分频得到，本例未分频。

| 场景 | `SystemCoreClock` | `BRR` | 实际波特率 | 与目标误差 |
| --- | --- | --- | --- | --- |
| 本实验（HSI 8 MHz） | 8 000 000 | 69 = `0x45` | 115 942 | +0.64% |
| 忘记 `SystemCoreClockUpdate()` | 16 000 000（错误初值） | 139 = `0x8B` | 57 554 | −50% |
| 日后开 PLL 到 72 MHz | 72 000 000 | 625 = `0x271` | 115 200 | 0% |

第二行就是「串口乱码」的典型成因：代码算的是 16 MHz 下的分频值，硬件跑在 8 MHz 上。
本项目把 `SystemCoreClockUpdate()` 收在 `f103_tick_init_irq()` 里，所以 **`uart_init()` 必须在其之后调用**。

### 3. `TXE` 与 `TC` 不是一回事

- `TXE`（bit7）：发送数据寄存器空，可以写下一个字节 → 用于 `uart_putc` 的循环条件。
- `TC`（bit6）：移位寄存器也空了，整帧真正发完 → 用于 `_write` 返回前确认。

只等 `TXE` 就复位或断电，最后一两个字节会被截断。`_write` 在结尾等 `TC` 就是为了这个。

### 4. `ORE` 的清除靠「读 SR 再读 DR」，且顺序不能反

`ORE`（溢出）表示上一字节还没读走、新字节已经到达。它没有独立的清除位，必须按「读 `SR` → 读 `DR`」的序列清除。
本实验的 `uart_poll()` 先取一次 `SR` 快照，若有 `ORE` 读 `DR` 清标志，然后**丢弃**该字节并计入 `over`——
溢出时丢了多少无法恢复，静默吞掉比报错更难排查。

### 5. printf 需要补五个符号，其中 `_isatty` 最容易被忽略

`_write` 与 USART1 的寄存器配置都在 `F103_common/src/f103_uart.c`（实验 05 起复用同一份实现，
避免每个实验复制一遍串口初始化）。其余符号在 `F103_common/src/crt_stubs.c`：

| 符号 | 作用 | 缺失后果 |
| --- | --- | --- |
| `_sbrk` | newlib 需要少量堆内存时调用 | libnosys 版本恒返回 −1，`printf` 可能静默失败 |
| `_isatty` | 决定 stdout 缓冲策略 | libnosys 返回 0 → **全缓冲**，不够一行不调用 `_write`，表现为「printf 不输出」 |
| `_close`/`_fstat`/`_lseek`/`_read` | stdio 初始化会引用 | 四条 `is not implemented and will always fail` 链接告警 |

`_isatty` 返回 1 后 stdout 采用**行缓冲**：遇到 `\n` 才调用 `_write`。所以格式串必须以 `\r\n` 结尾，
否则内容留在缓冲区里看不到。若确实需要逐字节立即输出，调用 `setvbuf(stdout, NULL, _IONBF, 0)`。

另注意：newlib-nano 默认不支持 `%f`，需要 `-Wl,-u,_printf_float`，代价约 +2.5 KB Flash；本实验不涉及浮点。

### 6. 为什么轮询回声会丢字节

回显路径是单字节的，但心跳一行约 35 字节，按 115200 计算需要约 3 ms。这 3 ms 内到达的字符没人读 `DR`，
后续字节触发 `ORE` 并被丢弃。这不是实现错误，而是**轮询模型的能力边界**：
只要主循环有阻塞式输出，就必须把接收搬到中断里并加环形缓冲——这是下一个实验的内容。

## 常见错误

| 现象 | 可能原因 | 排查 |
| --- | --- | --- |
| 完全无输出 | 未开 USART1/GPIOA 时钟；`UE` 未置位；TX/RX 接反 | 量 PA9 空闲电平应为高；反汇编确认 `0x4001380C` 写了 0x200C |
| 输出乱码 | `BRR` 与 PCLK2 不匹配（常见：`SystemCoreClock` 仍是 16 MHz 初值） | 打印 `SystemCoreClock` 与 `BRR`；确认先调 `f103_tick_init_irq()` |
| 输出但没有回显 | 配成通用输出（`CNF=00`）或 PA10 未配输入 | 反汇编确认 CRH 写入值为 `0x4B0` |
| printf 一个字都不出 | `_isatty` 缺失 → 全缓冲；或格式串没有 `\n` | 补 `_isatty`；格式串结尾加 `\r\n`；调试时加 `fflush(stdout)` |
| 链接报 4 条 libnosys 告警 | 未提供 `_close/_fstat/_lseek/_read` | 见 `crt_stubs.c` |
| `over` 持续增长 | 主循环阻塞期间到达的数据被覆盖 | 属轮询模型的固有限制，改用中断 + 环形缓冲 |
| 只能收到第一个字符 | 读到 `RXNE` 后没有读 `DR`，标志一直不清 | 确认每次判定后都读了一次 `DR` |

## 下一步

1. 接板验证：常温下连续输入长文本，观察 `over` 是否增长；用逻辑分析仪或示波器量 PA9 波形确认 115200。
2. 中断接收 + 环形缓冲见 [`F103_05_UART_IRQ_Ring`](../F103_05_UART_IRQ_Ring/README.md)：ISR 只写缓冲，主循环取走，解决第 6 点（对应课程 `0702_uart_interrupt`、`0703_uart_circle_buffer`）。
3. 把实验 03 的按键事件用 `printf` 打印时间戳，让「事件—时间」关系可观测。
4. 提高波特率到 921600 或改为 DMA 发送（课程 `0704_uart_dma`），比较 CPU 占用。

## 已知限制

- 轮询模型，主循环阻塞时会丢接收数据；无硬件流控，无法反压主机。
- 未处理帧错误（`FE`）、噪声（`NE`）、校验错误（`PE`）与空闲中断（`IDLE`）。
- `_write` 是阻塞式发送，长字符串会占用主循环；`printf` 依赖行缓冲，需要 `\r\n` 触发刷新。
- 波特率误差 +0.64%，在 115200 下可接受；更高波特率需重新评估 BRR 取整误差。
- 堆只有 512 B（`_Min_Heap_Size`），仅够 newlib 少量内部结构；不适合 `malloc` 大块内存。
