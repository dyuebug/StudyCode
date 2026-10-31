# Day7：UART 协议与 HAL 编程

**验证状态**：USART1 配置（115200 8N1、PA9/PA10）、时钟树（HSE 8 MHz → PLL×9 = 72 MHz）与
`retarget.c` 的 `_write` **已核对源码**；波特率分频比与字节耗时是**按寄存器公式计算**（已给出算式，
可复核）；`driver_uart.c` 两套重定向并存**已逐函数核对**；**串口从未在真板上收发过**
（本机无板，USB-TTL 未接）。教材侧内容来自《HAL快速入门与项目实战》第 7.1–7.3 节。

---

## 学习目标

学完本篇应能用代码证据回答：

1. 同步传输与异步传输差哪三件事，为什么 UART 只需要三根线却必须"提前约定"。
2. 一帧 UART 数据的位序是什么，`115200 8N1` 一个字节占多少位、耗时多少微秒。
3. 波特率与比特率什么时候相等、什么时候不相等——教材那段"电压分级"的例子在讲什么。
4. `72 MHz` 下把 115200 写进 `BRR` 得到的实际波特率误差是多少，为什么这里恰好是 0。
5. 查询/中断/DMA 三种方式的函数对应关系，以及教材给每种方式标的那条"缺点"。
6. 本工程里 `printf` 走的是 `_write` 还是 `fputc`，另一个为什么不生效却仍被编译。

---

## 环境与前置条件

| 项 | 要求 |
| --- | --- |
| 工程 | `experiments/F103_HAL_DriverLab/` |
| 参考 | `Core/Src/usart.c`、`Core/Src/retarget.c`、`Drivers/DshanMCU-F103/driver_uart.c` |
| 硬件 | 可选。串口自环/回显需真板 + USB-TTL 模块 |
| 前置 | Day1 §3（`_write` 与 `fputc` 的分工）、Day5（中断上下文纪律） |

---

## 核心概念

### 1. 同步与异步：教材那张三行表

|  | 信号线 | 速率 | 抗干扰 |
| --- | --- | --- | --- |
| 同步传输 | 多：**时钟线 + 数据线** | 可变，提时钟频率即可 | 强 |
| 异步传输 | 少：**只要数据线** | **双方提前约定** | 弱 |

这张表的推论只有一句：**省掉时钟线，就要用"约定"补上**。约定包括三件事——
为一位占多长时间（波特率）、一共几位、有没有校验位。任一项不一致，收到的就是乱码，
而且**不会有任何错误提示**（除了帧错误/校验错误这类粗粒度标志）。

这也是"串口乱码"最常见的原因：不是线接错了，是两边约定不同。

### 2. 一帧的位序：为什么必须 LSB 先发

教材 7.2.1 的传输流程：

```text
空闲(1) ─┐
         └─ 起始位(0，1 位宽) ─ 数据位 bit0 … bit7（LSB 先发） ─ [校验位] ─ 停止位(1) ─ 空闲(1)
```

三个必须记住的细节：

1. **起始位是"下降沿"**。空闲为高，来一个低就是帧开始。接收方检测到下降沿后，
   教科书说"再等 1.5 位"才开始采样——实际硬件用 16 倍过采样，在每位的**中点**采样，
   这样对时钟偏差的容忍度最大。
2. **LSB 先发**。所以 `'A'` = `0b0100_0001` 在线上是先 1、后 0、后 0…。这条与"大端小端"无关，
   是不同的东西，别混。
3. **停止位可以是 1、1.5 或 2 位**。本工程是 1 位。

按 `115200 8N1`（8 数据位、无校验、1 停止位）数：

```text
每字节位数 = 1(起始) + 8(数据) + 0(校验) + 1(停止) = 10 bit
字节率 = 115200 / 10 = 11520 B/s
单字节耗时 = 1 / 11520 ≈ 86.8 µs
```

**86.8 µs/字节是全篇最有用的一个数**：任何"发 1 KB 要多久"的问题都取决于它
（1024 × 86.8 µs ≈ 89 ms）。

### 3. 波特率与比特率：教材那段"电压分级"在讲什么

教材举了个例子：若把电压分成 4 档（0~0.7 V、0.8~1.5 V、1.6~2.3 V、2.4~3.3 V），
每个"波形"能表示 2 bit，于是一个字节只要发 4 次——此时 **比特率 = 波特率 × 2**。

这个例子说明的是**波特率的定义**：`1 秒内的符号（波形）数`，而不是比特数。
UART 每个符号只表示 1 bit（高/低两档），所以本工程里：

```text
波特率 = 比特率 = 115200
```

之所以要区分这两个词，是因为在别的调制方式里它们不等。**在 UART 语境下可以认为相等，
但不要在"为什么 115200 配 8N1 之后有效数据只有 11520 B/s"这个问题上把两个概念搅在一起**——
那个 10 倍差不是波特率/比特率之差，而是**帧开销**（起始位 + 停止位）。

### 4. `BRR` 的算法与误差：为什么 72 MHz 下是 0 误差

F1 的波特率由 `USARTDIV` 决定，`BRR` 的高 12 位是整数部分、低 4 位是小数部分（1/16 分辨率）：

```text
USARTDIV = PCLK / (16 × Baud)

PCLK2 = 72 MHz（USART1 挂在 APB2，见 system_stm32f1xx.c / main.c 的注释）
USARTDIV = 72 000 000 / (16 × 115 200) = 39.0625
整数部分 = 39 = 0x27
小数部分 = 0.0625 × 16 = 1
BRR = (39 << 4) | 1 = 625 = 0x271

实际波特率 = 72 000 000 / (16 × 39.0625) = 72 000 000 / 625 = 115 200   ← 误差 0
```

作为对照，若同一个波特率挂在 36 MHz 的 APB1 上：

```text
36 000 000 / (16 × 115 200) = 19.53125 → 小数 0.53125×16 = 8.5，取整为 9
USARTDIV = 19 + 9/16 = 19.5625
实际 = 36 000 000 / (16 × 19.5625) = 36 000 000 / 313 ≈ 115 015   ← 误差 ≈ 0.16 %
```

0.16% 仍在 UART 的容忍范围（一般要求总和 < 2~3%），但它解释了"为什么换个串口，
波特率误差会变"——**误差取决于 `PCLK` 与目标波特率的整除关系，不取决于波特率本身**。

`OverSampling = UART_OVERSAMPLING_16` 就是上面那个 16；改成 8 倍过采样时公式里的 16 变 8，
能支持更高波特率，但对时钟偏差更敏感。

### 5. 三种编程方式与它们的"缺点"

教材 7.3.2 的对照表，加一列本工程的实际用法：

| 方式 | 发送 | 接收 | 完成回调 | 教材标注的缺点 |
| --- | --- | --- | --- | --- |
| 查询 | `HAL_UART_Transmit` | `HAL_UART_Receive` | 无 | 发送死等；**接收容易丢** |
| 中断 | `HAL_UART_Transmit_IT` | `HAL_UART_Receive_IT` | `TxCpltCallback` / `RxCpltCallback` | **接收仍容易丢** |
| DMA | `HAL_UART_Transmit_DMA` | `HAL_UART_Receive_DMA` | `TxHalfCplt` / `TxCplt` / `RxHalfCplt` / `RxCplt` | —— |

两条"缺点"要读懂它们的**指向**：

- 查询方式的"接收容易丢"是因为**你必须主动去读**。两个字节间隔只有 86.8 µs，
  主循环若在刷 OLED（§Day6：一行 46 ms @100 kHz），中间会漏掉几百个字节。
  F1 只有 1 字节的 `DR` 寄存器，**没有 FIFO**，所以漏一个就是 `ORE` 溢出。
- 中断方式的"接收仍容易丢"是因为 `HAL_UART_Receive_IT` **一次只收固定长度**。
  收完 N 个字节就停，回调里必须再调一次才能继续；这中间的空档就是丢数据的窗口。
  教材 7.3.5 的改造（"一开始就使能接收中断 + 中断里存环形 buffer"）就是为了消除这个空档。

`ORE`（Overrun Error）在 HAL 里表现为"后续接收全部失败"，因为 `ORE` 一旦置位而没清，
`RXNE` 就再也置不起来。**这就是 `driver_uart.c:73` 那句 `__HAL_UART_CLEAR_OREFLAG` 存在的理由**——
但它在**每次 `fgetc` 开始时清一次**，只能救"上一轮残留"，救不了"这一轮读的过程中溢出"。

### 6. 本工程的实际链路：`printf` → `_write` → 阻塞发送

```text
用户 printf("...")
   └─ newlib 格式化到内部缓冲
      └─ _write(fd, buf, len)            Core/Src/retarget.c
         └─ HAL_UART_Transmit(&huart1, buf, len, len*10ms)   ← 阻塞
            └─ 逐字节写 USART1->DR，等 TXE
```

超时是 `len * 10`（ms）。按 86.8 µs/字节算，`len=1` 时超时 10 ms ≈ **名义耗时的 115 倍**。
这个余量是有意的：它让"正常发送"绝不会超时，只有硬件真出问题才返回 `HAL_ERROR`。

配置落在两处（`usart.c`）：

```c
huart1.Instance       = USART1;
huart1.Init.BaudRate  = 115200;
huart1.Init.WordLength= UART_WORDLENGTH_8B;
huart1.Init.StopBits  = UART_STOPBITS_1;
huart1.Init.Parity    = UART_PARITY_NONE;
huart1.Init.Mode      = UART_MODE_TX_RX;
huart1.Init.OverSampling = UART_OVERSAMPLING_16;

/* HAL_UART_MspInit：引脚侧 */
PA9  → GPIO_MODE_AF_PP + GPIO_SPEED_FREQ_HIGH   （TX 由外设驱动，复用推挽）
PA10 → GPIO_MODE_INPUT + GPIO_NOPULL            （RX 只是输入）
__HAL_RCC_USART1_CLK_ENABLE()                   （USART 时钟，与 GPIOA 时钟是两回事）
```

**这解释了 Day2 §3 的四步表在串口上的形态**：`APB2ENR` 里要置**两个**位——`USART1EN` 和 `IOPAEN`。
少一个的现象分别是"寄存器写不进"和"引脚没连上"。

### 7. `driver_uart.c` 里有两套重定向，只有一套生效

```c
int fputc(int c, FILE *f)                      /* 课程原版：Keil + MicroLIB 的重定向点 */
{
    HAL_UART_Transmit(g_HDebugUART, (const uint8_t *)&c, 1, 500);
    return c;
}
```

在 Keil/MicroLIB 下，`printf` 最终调 `fputc`。但在 GCC + newlib 下，`printf` 走的是
**`_write`**（Day1 §3 的表）。所以：

| 函数 | newlib 下是否生效 | 依据 |
| --- | --- | --- |
| `fputc`（`driver_uart.c:60`） | **否**，死代码 | newlib 的 `printf` 不调 `fputc` |
| `_write`（`retarget.c`） | **是** | 实测 `printf` 有输出（Day1 的构建/运行链） |
| `fgetc`（`driver_uart.c:67`） | **是**，但仍可用 | `UART_Test` **直接**调它，不经过 stdio |

`fgetc` 是活的这一点最容易误判：它被 `UART_Test()` 直接调用（`driver_uart.c:105`），
不存在"是不是 stdio 的重定向点"这个问题。

**为什么保留 `fputc` 而不删**：它是课程原始代码，删了就无法与教材对照。
代价是"两套重定向并存"——这正是 Day1 陷阱表里那一行"不报错、也能出字"的来源。
判据是**符号表**而不是"能不能跑"：

```bash
arm-none-eabi-nm build/f103_hal_driverlab.elf | grep -E ' (T|t) (fputc|_write)$'
```

两个符号都在镜像里，但只有 `_write` 在 `printf` 的调用链上。

### 8. `fgetc` 的三个具体问题

```c
int fgetc(FILE *f)
{
    uint8_t ch = 0;
    __HAL_UART_CLEAR_OREFLAG(g_HDebugUART);            /* ① 只在开头清一次 */
    HAL_UART_Receive(g_HDebugUART, (uint8_t *)&ch, 1, HAL_MAX_DELAY);   /* ② 永不超时 */
    HAL_UART_Transmit(g_HDebugUART, (uint8_t *)&ch, 1, HAL_MAX_DELAY);  /* ③ 回显也永不超时 */
    return ch;
}
```

1. **`HAL_MAX_DELAY`**：没数据就永远阻塞。主循环调它 → 系统除了等串口什么也做不了。
2. **不判返回值**：`HAL_UART_Receive` 返回 `HAL_TIMEOUT`/`HAL_ERROR` 时 `ch` 仍是 0，
   于是"没收到"被当成"收到了 0x00"。
3. **逐字节收发**：1 个字节 3 次 HAL 调用（收发各一次 + 清标志），开销远大于必要。

正确形态就是教材 7.3.5 的改造：**接收中断里只把字节塞进环形缓冲**，主循环从缓冲取。
Day6 的 `r`/`w` 判据在这里同样适用（只有一个写者 = ISR，一个读者 = 主循环）。

---

## 系统调用/接口卡片

### `HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, const uint8_t *pData, uint16_t Size, uint32_t Timeout)`

作用：阻塞发送 `Size` 个字节，逐字节等 `TXE` 后写 `DR`，全部写完后等 `TC`（发送完成）。

参数：`Size` 是 **`uint16_t`**——超过 65535 会被截断（不是报错）。`Timeout` 单位 ms，
`HAL_MAX_DELAY` 表示不超时。

返回值和错误：`HAL_OK` / `HAL_ERROR`（外设出错或已处于忙）/ `HAL_BUSY`（句柄被占用）/ `HAL_TIMEOUT`。

资源所有权：内部把 `huart->gState` 置为 `HAL_UART_STATE_BUSY_TX`，返回前恢复。

线程/进程安全：**不可重入**。同一句柄被两处同时调用，第二个拿 `HAL_BUSY`。
裸机上表现为"另一处 printf 的内容丢了"，不报错。

最小示例：

```c
HAL_UART_Transmit(&huart1, (uint8_t *)"ok\r\n", 4, 100);
```

常见误用：在 ISR 里用它（86.8 µs × 长度 的阻塞）；忽略 `HAL_BUSY`；传 `len` 不给 `uint16_t` 范围检查。

### `HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout)`

作用：阻塞接收 `Size` 个字节。

参数与返回：同上。`Timeout` 到期返回 `HAL_TIMEOUT`，**此时 `pData` 里可能只有部分数据**——
没有"收到了几个"的输出参数，这是这个 API 的结构性限制。

常见误用：`Timeout = HAL_MAX_DELAY` 且无外部唤醒手段（`fgetc` 就是这个写法）。

### `HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)`

作用：启动一次 N 字节的中断接收。收满 `Size` 后自动停止，并调 `HAL_UART_RxCpltCallback`。

参数：`Size` 通常设为 1（每来一个字节进一次中断）。

返回值和错误：同 `Transmit`。

资源所有权：**调用方必须保证 `pData` 在回调之前一直有效**。传栈上局部变量是经典 bug。

常见误用：只在 `main` 里调一次就以为"一直收"——收满就停，必须在回调里重新启动。

### `void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)`（`__weak`）

作用：用户侧接收完成回调，ISR 上下文。

常见误用：在回调里 `printf`（阻塞）、刷 OLED（46 ms 量级）、或调 `HAL_Delay`（优先级关系中
会冻结，见 Day5 §6）。正确做法：只入环形缓冲 + 立刻重新 `HAL_UART_Receive_IT`。

---

## 最小可运行实验

目标：不接串口线，把"配置 → BRR → 字节耗时 → 重定向路径"逐项落到源码与符号上。

```bash
cd experiments/F103_HAL_DriverLab

# 1) 四个关键字段与引脚
grep -n "BaudRate\|WordLength\|StopBits\|Parity\|OverSampling" Core/Src/usart.c
sed -n '60,92p' Core/Src/usart.c          # MspInit：PA9 复用推挽、PA10 输入

# 2) 两套重定向都在镜像里，但只有一个是活的
arm-none-eabi-nm build/f103_hal_driverlab.elf | grep -E ' (T|t) (fputc|_write)$'

# 3) printf 的调用链（应看到 _write 被调用，fputc 没有调用者）
arm-none-eabi-objdump -d build/f103_hal_driverlab.elf | grep -n "bl.*<\?_write" | head
```

第 3 条是决定性证据：`fputc` 有定义但**没有 `bl` 指向它**，说明它不在任何调用链上。

---

## 编译与运行

```bash
cmake --build build
```

真板上的自环测试（硬件到位后，本机未验证）：短接 TX/RX，运行
`LAB_TEST=<UART 对应编号>` 的构建，观察 `UART_Test` 的回显。

---

## 结果观察

**配置读数**（`Core/Src/usart.c`）：

```text
USART1  115200  8B  1 stop  no parity  TX_RX  oversampling 16
PA9  = GPIO_MODE_AF_PP, GPIO_SPEED_FREQ_HIGH
PA10 = GPIO_MODE_INPUT, GPIO_NOPULL
时钟：__HAL_RCC_USART1_CLK_ENABLE() + __HAL_RCC_GPIOA_CLK_ENABLE()
```

**派生量**（按公式计算，不是实测）：

| 量 | 值 | 算式 |
| --- | --- | --- |
| 每字节位数 | 10 | 1 起始 + 8 数据 + 0 校验 + 1 停止 |
| 字节率 | 11520 B/s | 115200 / 10 |
| 单字节耗时 | ≈ 86.8 µs | 1 / 11520 |
| `BRR` | 0x271 (625) | `(39<<4)｜1`，39.0625 = 72e6/(16×115200) |
| 波特率误差 | **0 %** | 72e6 / 625 = 115200 |

**重定向路径**（源码逐层核对）：

```text
printf → newlib → _write (retarget.c) → HAL_UART_Transmit(&huart1, …, len*10 ms) → USART1->DR
fputc (driver_uart.c:60)  ← 已定义、未被调用（newlib 不用它）
fgetc (driver_uart.c:67)  ← 被 UART_Test 直接调用，HAL_MAX_DELAY 永久阻塞
```

**`GPIO_InitStruct` 的一个隐患**（`usart.c:77-85`）：配 PA9 时只赋了 `Pin`/`Mode`/`Speed`，
**`Pull` 没有显式赋值**，靠函数开头的 `= {0}` 变成 `GPIO_NOPULL`。
当前正确，但只要有人把这段改成复用同一个结构体而不重置，`Pull` 会带上一次的值。
**判据：`HAL_GPIO_Init` 读的是整个结构体，没赋的字段不是"用默认值"，是"用内存里剩下的值"。**

---

## 常见错误和陷阱

| 陷阱 | 症状 | 根因 | 处理 |
| --- | --- | --- | --- |
| 两边波特率不一致 | 全是乱码 | 异步无时钟线 | 核对两边 `BaudRate` |
| 8N1 里以为能传 115200 B/s | 速率差 10 倍 | 帧开销 2 位 | 有效字节率 = 波特率/10 |
| `ORE` 后彻底收不到 | 只收到前几个字节，之后全无 | `ORE` 未清则 `RXNE` 不再置位 | 清 `ORE`；改用中断 + 环形缓冲 |
| `HAL_UART_Receive_IT` 只调一次 | 收 N 个字节后停 | 收满即停 | 回调里重新启动 |
| 回调里传栈上缓冲 | 随机数据错乱 | `pData` 悬空 | 用 `static`/全局缓冲 |
| 回调里 `printf`/刷屏 | 丢字节、系统卡 | 阻塞时间 >> 86.8 µs | 只入缓冲 |
| `Size` 传 `int` 且 > 65535 | 只发前 65535 个 | 参数是 `uint16_t` | 分块发 |
| 忘了 `USART1` 时钟或 `GPIOA` 时钟 | 完全无输出 / 引脚无波形 | `APB2ENR` 两个位 | 逐位核对 |
| Keil 的 `fputc` 当 GCC 的重定向点 | `printf` 无输出 | newlib 走 `_write` | 看 `nm` 找调用链 |
| `fgetc` 用 `HAL_MAX_DELAY` | 主循环彻底卡住 | 无超时 | 给超时或改中断 |

---

## 故障排查方法

"串口没输出 / 输出乱码"按**从物理到协议**五步，每步只看一个东西：

1. **有没有波形**（有示波器/逻辑分析仪时）。TX 脚上完全没有边沿 → 到第 2 步；
   有边沿但占空比/周期不对 → 到第 4 步。
2. **`APB2ENR` 的两个位**：`USART1EN` 与 `IOPAEN`。缺前者则寄存器写不进；缺后者则引脚没连到外设。
3. **`GPIOA->CRH` 里 PA9 的 4 bit**：应为"复用推挽"（`CNF=10`）。配成普通推挽输出时，
   TX 引脚会被 GPIO 模块抢占，串口发不出去。
4. **`USART1->BRR`**：按 §4 的公式反算，与预期比对。这里错了就是乱码，但**一切其他都正常**。
5. **`USART1->SR` 的 `ORE`/`FE`/`NE`**：`ORE` 置位说明接收端在丢数据（见 §5），
   `FE` 说明采样点错（波特率或停止位不符）。

"能发不能收"这类问题，第 5 步是最快的入口：`ORE` 一置位，后面全废。
临时验证的办法是**先不读**，看 `RXNE` 是否能置起来——能置说明硬件收到了，问题在读取逻辑。

---

## 练习题

### 基础题

题目：`115200 8N1` 下，发送 1 KB 数据需要多少毫秒？改用 `9600 8N1` 又是多少？
如果要让 1 KB 在 100 ms 内发完，最低波特率是多少（8N1）？

提示：每字节 10 位；`时间 = 字节数 × 10 / 波特率`。

验收标准：115200 → ≈ 89 ms；9600 → ≈ 1067 ms；100 ms 内 → ≥ 102400，向上取常用值 115200。

### 改错题

题目：下面的接收代码有什么问题？给出修正。

```c
uint8_t g_rx;

void uart_start(void)
{
    HAL_UART_Receive_IT(&huart1, &g_rx, 1);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    printf("rx=%02X\r\n", g_rx);
}
```

提示：① 收满 1 字节之后 `Receive_IT` 还在运行吗；② 回调里 `printf` 要多久。

验收标准：能指出"回调里必须重新 `HAL_UART_Receive_IT`，否则只收一个字节"，
以及"`printf` 阻塞 86.8 µs/字节起，会丢后续字节"，给出"入环形缓冲 + 重启接收"的修正。

### 综合题

题目：用 §4 的算式，分别计算 `PCLK2 = 72 MHz` 下 9600、115200、921600 三个波特率的
`BRR` 与误差，并判断哪个超出了 UART 可容忍范围（按总和 < 3%）。

提示：`USARTDIV = PCLK/(16×Baud)`；小数部分取 `round(frac×16)`，再反算实际波特率。

验收标准：三个 `BRR` 与误差都算对，并能指出 921600 在 16 倍过采样下误差偏大
（`72e6/(16×921600) = 4.8828…`，小数取整误差显著），应改用 8 倍过采样或换更高时钟。

---

## 验收标准

- [ ] 能画出 UART 一帧的位序（起始/数据 LSB 先/校验/停止），并算出 115200 8N1 的字节耗时。
- [ ] 能说明波特率与比特率在 UART 中相等、但"有效字节率"因帧开销而差一截。
- [ ] 能用 `USARTDIV` 公式算出 `BRR = 0x271` 并证明误差为 0，同时给出 36 MHz 下的对照。
- [ ] 能说出查询/中断/DMA 六组函数的对应关系，以及两条"容易丢"的各自原因。
- [ ] 能指出本工程 `_write` 与 `fputc` 谁生效、为什么不生效的那个仍在镜像里。
- [ ] 能列出 `fgetc` 的三个具体问题，并给出基于环形缓冲的改法。
- [ ] 能独立完成本篇三条命令，用 `nm` + `objdump` 证明 `fputc` 不在调用链上。

## 延伸阅读

- 教材：《HAL快速入门与项目实战(基于DshanMCU-103)》第 7.1 节（同步/异步）、
  第 7.2 节（UART 协议与硬件结构）、第 7.3 节（三种编程方式）
- 配置：`Core/Src/usart.c`（`MX_USART1_UART_Init` + `HAL_UART_MspInit`）
- 重定向：`Core/Src/retarget.c`（`_write`）、`Drivers/DshanMCU-F103/driver_uart.c:60-80`（`fputc`/`fgetc`）
- 时钟树：`Core/Src/main.c:72`（HSE 8 MHz → PLL×9 = 72 MHz）、`stm32f1xx_hal_conf.h:87`
- 上一篇：`Linux/notes/F103_HAL入门/Day06_OLED调试与环形缓冲区.md`
- 下一篇：`Linux/notes/F103_HAL入门/Day08_UART_IDLE中断与stdio重定向.md`

---

**验证记录**

| 项目 | 内容 |
| --- | --- |
| 日期 | 2026-10-29 |
| 环境一 | Windows 11 + Git Bash；`grep`/`sed` 核对配置与重定向 |
| 命令一 | `grep -n "BaudRate\|WordLength\|StopBits\|Parity\|OverSampling" Core/Src/usart.c`；`sed -n '60,92p' Core/Src/usart.c`；`cat Core/Src/retarget.c` |
| 结果一 | 115200 / 8B / 1 stop / none / TX_RX / OVERSAMPLING_16；PA9 `AF_PP+HIGH`、PA10 `INPUT+NOPULL`；`_write` 用 `HAL_UART_Transmit(..., len*10ms)` 并在 `gState==RESET` 时返回 -1 |
| 环境二 | 同上 |
| 命令二 | `sed -n '30,110p' Drivers/DshanMCU-F103/driver_uart.c`；`grep -n "HSE_VALUE" Core/Inc/stm32f1xx_hal_conf.h`；`sed -n '65,100p' Core/Src/main.c` |
| 结果二 | `fputc`/`fgetc` 均定义，`fgetc` 用 `HAL_MAX_DELAY` 且被 `UART_Test` 直接调用；`HSE_VALUE 8000000`；`main.c` 注释确认 PLL×9 = 72 MHz |
| 未覆盖 | 串口实际收发（自环回显、乱码复现、`ORE` 触发）全部未上板验证；**86.8 µs/字节、`BRR=0x271`、误差 0% 均为按公式计算，无实测**；`fputc` 不在调用链上为符号级推断（未给 `objdump` 的 `bl` 计数原始输出）；`fgetc` 溢出丢字节未构造复现 |
