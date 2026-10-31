# Day8：UART IDLE 中断与 stdio 重定向

**验证状态**：HAL 的 `ReceiveToIdle` 三件套、`__HAL_UART_CLEAR_IDLEFLAG` 的实际展开、
`HAL_UARTEx_RxEventCallback` 的事件语义**已核对源码**（`stm32f1xx_hal_uart.c:1783-1836`、
`:2710`，`stm32f1xx_hal_uart.h:504-510, 542, 759-784`）；**本工程没有 `USART1_IRQHandler`**
为逐文件核对（`Core/Src/stm32f1xx_it.c` 全文 + `startup/startup_stm32f103xb.s:346-347`）；
`_isatty` / `_read` 的缓冲语义**已核对源码**（`Core/Src/crt_stubs.c`）；**IDLE 从未在真板上
产生过**（本机无板、无 USB-TTL）。教材侧内容来自《HAL快速入门与项目实战》第 7.4–7.5 节。

---

## 学习目标

学完本篇应能用代码证据回答：

1. DMA 已经让"搬数据"不占 CPU 了，为什么还需要 IDLE 中断——它到底补的是哪一环。
2. IDLE 的触发条件精确是什么，为什么"引脚一开始就是空闲的"却不会一直触发。
3. 清 IDLE 标志为什么不能只写 `SR` 一位，HAL 那句 `__HAL_UART_CLEAR_IDLEFLAG` 实际展开成什么。
4. `RxEventCallback(huart, Size)` 里的 `Size` 在中断模式与 DMA 模式下分别是谁、代表什么。
5. 本工程今天调用 `HAL_UART_Receive_IT` 会发生什么，为什么这个结论能从启动文件直接读出来。
6. 教材 7.5 的 `fputc`/`fgetc`/`__backspace` 三件套，在 GCC + newlib 下对应成哪几个符号；
   少写 `_isatty` 时 `printf` 会是什么表现。

---

## 环境与前置条件

| 项 | 要求 |
| --- | --- |
| 工程 | `experiments/F103_HAL_DriverLab/` |
| 参考 | `Core/Src/usart.c`、`Core/Src/retarget.c`、`Core/Src/crt_stubs.c`、`Core/Src/stm32f1xx_it.c` |
| 参考 HAL | `stm32f1xx_hal_uart.c` 的 `HAL_UARTEx_ReceiveToIdle_*` 与 `HAL_UART_IRQHandler` |
| 硬件 | 可选。IDLE 需要真实接收数据才能产生 |
| 前置 | Day7（UART 帧、波特率、`_write` 与 `fputc` 的分工） |

---

## 核心概念

### 1. DMA 补了什么，还缺什么

教材 7.3.6 用 DMA 解决了"每搬一个字节都要进一次中断"。但 DMA 只回答了一个问题：
**怎么搬**。它不回答另一个：**什么时候算搬完了**。

教材 7.4.1 把这个缺口说得很清楚：DMA 提高 CPU 效率，但"**无法预知要接收多少数据**"。
于是有三种可能的"结束"：

| 结束方式 | 触发者 | 回调 |
| --- | --- | --- |
| 收满指定数量（如 100 字节） | DMA `TC` | `HAL_UART_RxCpltCallback` |
| 总线空闲了（只来了 60 字节就不来了） | **IDLE** | `HAL_UARTEx_RxEventCallback` |
| 出错了（`ORE`/`FE`/`NE`） | 错误中断 | `HAL_UART_ErrorCallback` |

**关键点：DMA 的"完成"是按长度算的，而协议的"完成"往往按时间算。** 主机发一串不定长命令，
它不会告诉你"我要发 37 个字节"，它只是发完之后停手。停手这件事，只能由总线状态告诉我们。
这就是 IDLE 存在的唯一理由。

### 2. IDLE 的触发条件：为什么不会一开始就触发

教材原文："当我们使能 IDLE 中断后，它并不会立刻产生，而是：**至少收到 1 个数据后**，
发现在一个字节的时间里，都没有接收到新数据，才会产生 IDLE 中断。"

这个"至少收到 1 个数据"是硬件行为，不是 HAL 加的保险。原因是 `IDLE` 位的语义依赖
接收通路已经被"激活"过：`RXNE` 至少要置位过一次。所以下面两条推得出来：

- **上电后就开的引脚不会刷 IDLE**。空闲态本身不是事件，"空闲下来"才是。
- **最后一个字节之后要等一个完整字节时间**。`115200 8N1` 下就是 Day7 算的
  **≈ 86.8 µs**。这也是 IDLE 不能用来切"逐字节"节拍的原因——它是帧尾检测，不是计时器。

一个常见误解是"IDLE ≈ 超时"。差别在于：超时是**可配的**（你可以设 10 ms），
IDLE 是**固定的**（一个字节时间），且不需要任何定时器资源。代价是它只能检测"停顿 1 字节"，
检测不出"停顿 100 µs 但我要 10 ms"这种需求。

### 3. 清 IDLE 标志：一句宏，两次读

`__HAL_UART_CLEAR_IDLEFLAG` 看起来像"写 1 清某位"，实际展开成（`stm32f1xx_hal_uart.h:504-510, 542`）：

```c
#define __HAL_UART_CLEAR_PEFLAG(__HANDLE__)     \
  do{                                           \
    __IO uint32_t tmpreg = 0x00U;               \
    tmpreg = (__HANDLE__)->Instance->SR;        /* 先读 SR */   \
    tmpreg = (__HANDLE__)->Instance->DR;        /* 再读 DR */   \
    UNUSED(tmpreg);                             \
  } while(0U)

#define __HAL_UART_CLEAR_IDLEFLAG(__HANDLE__) __HAL_UART_CLEAR_PEFLAG(__HANDLE__)
```

**它清的不是一位，而是一串**：`PE / FE / NE / ORE / IDLE` 五个标志都是"读 SR 再读 DR"才清。
这在 RM0008 的寄存器描述里是成组定义的。

三条推论，都要能自己推出来：

1. **只读 SR 不清**。所以"读一下 `SR` 看看有没有 IDLE"这种探针代码会**反复看到同一个 IDLE**，
   看起来像"IDLE 一直在触发"。
2. **这个宏有副作用**：它把 `DR` 读走了。如果此刻 `RXNE` 也置位，那个字节就被**丢弃**，
   而调用方毫不知情。HAL 在 `ReceiveToIdle` 的启动路径里用它是安全的（那时还没开始收），
   但如果你在手写的 ISR 里为了清 IDLE 而调它，就要接受"可能吃掉一个字节"。
3. `tmpreg` 被赋值两次但只读不写，`UNUSED` 是给编译器的信号——**这两次读是刻意保留的**，
   不是冗余代码。删掉第二次读，IDLE 就不清了。

### 4. `ReceiveToIdle` 三件套与两个回调

教材 7.4.1 给了三行表，本工程 HAL 里都在（`stm32f1xx_hal_uart.h:759-762`）：

| 方式 | 函数 | 长度从哪知道 |
| --- | --- | --- |
| 查询 | `HAL_UARTEx_ReceiveToIdle(huart, pData, Size, &RxLen, Timeout)` | **出参 `RxLen`** |
| 中断 | `HAL_UARTEx_ReceiveToIdle_IT(huart, pData, Size)` | 回调的 `Size` 形参 |
| DMA | `HAL_UARTEx_ReceiveToIdle_DMA(huart, pData, Size)` | 回调的 `Size` 形参 |

注意第一行与后两行的差别：查询方式有 `RxLen` 出参，所以调用方**当场**就知道收了多少；
后两种方式的行长度**只能从回调里拿到**，没有别的途径。

`HAL_UARTEx_RxEventCallback(huart, Size)`（`__weak`，`stm32f1xx_hal_uart.c:2710`）是这三种方式的
共同出口。HAL 的注释（`:1810-1836`）明确了事件类型与 `Size` 的关系：

```text
中断模式：
  HAL_UART_RXEVENT_TC   收满预期数量
  HAL_UART_RXEVENT_IDLE 空闲提前结束（收到的数据 < 预期）

DMA 模式：
  HAL_UART_RXEVENT_TC   收满预期数量
  HAL_UART_RXEVENT_HT   收到一半
  HAL_UART_RXEVENT_IDLE 空闲提前结束（收到的数据 < 预期）
```

要区分"是收满还是超时结束"，就调 `HAL_UARTEx_GetRxEventType(huart)`（`:764`）。
**这个函数只能在回调里调**，事后调拿到的是下一次的值——HAL 注释原话是
"expected to be called within the user implementation of Rx Event Callback"。

一个必须自己补上的推论：`Size` 是**本次已收到的累计字节数**，不是"本次新增"。所以在
DMA 循环模式（`Circular`）下回调可能被反复调用，每次拿到的 `Size` 是从缓冲区头开始算的位置，
**新增量要自己用上一次的值做差**。这条我只从 HAL 注释的措辞推出（"nb of received data"），
**未在真板上验证过循环模式下的实际数值序列**。

### 5. 启动路径里藏着一个失败分支

`HAL_UARTEx_ReceiveToIdle_DMA` 的骨架（`stm32f1xx_hal_uart.c:1783-1805`）：

```c
huart->ReceptionType = HAL_UART_RECEPTION_TOIDLE;
huart->RxEventType   = HAL_UART_RXEVENT_TC;
status = UART_Start_Receive_DMA(huart, pData, Size);

if (status == HAL_OK) {
    if (huart->ReceptionType == HAL_UART_RECEPTION_TOIDLE) {   /* 还会是 TOIDLE 吗？ */
        __HAL_UART_CLEAR_IDLEFLAG(huart);
        ATOMIC_SET_BIT(huart->Instance->CR1, USART_CR1_IDLEIE);
    } else {
        /* 启动时已经有挂起的错误（典型是 ORE），接收被中止，
           ReceptionType 被 HAL 重置成 STANDARD ⇒ 这里必须报错 */
        status = HAL_ERROR;
    }
}
```

这段代码在回答一个很实际的问题：**启动之前就有 `ORE` 怎么办**。答案是不静默继续——
`ReceptionType` 被 HAL 内部改回 `STANDARD` 就是"接收已经废了"的信号，函数返回 `HAL_ERROR`。

**这解释了 Day7 §5 那条 `ORE` 规则的另一面**：`ORE` 不只是"丢一个字节"，它会让整个
`ReceiveToIdle` 启动失败。所以返回值得判，不能像 `driver_oled.c` 那样一律丢弃。

同理 `HAL_UART_Abort`（`:1858-1866`）在收尾时会检查 `ReceptionType == TOIDLE` 才去清 `IDLEIE`。
`IDLEIE` 是 `CR1` 上的一位，**不关它，中止后 IDLE 中断还会继续进**——这是"停了一半"的典型形态。

### 6. 本工程今天的真实状态：没有 `USART1_IRQHandler`

这一条是全篇最该记住的，因为它不需要板子就能验证。

`Core/Src/stm32f1xx_it.c` 全文只有四个处理函数：`NMI_Handler`、`HardFault_Handler`、
`MemManage_Handler`、`BusFault_Handler`、`UsageFault_Handler`、`DebugMon_Handler`、
`TIM4_IRQHandler`、`EXTI15_10_IRQHandler`。**没有 `USART1_IRQHandler`。**

而启动文件里写的是（`startup/startup_stm32f103xb.s:346-347`）：

```asm
  .weak USART1_IRQHandler
  .thumb_set USART1_IRQHandler,Default_Handler
```

`Default_Handler` 的实体（同文件 `:113-116`）：

```asm
Default_Handler:
Infinite_Loop:
  b Infinite_Loop
```

三者连起来得到一条**必然结论**：

> 在这份工程里调用 `HAL_UART_Receive_IT`（或任何 `_IT`/`ReceiveToIdle_IT`），
> 只要真收到**一个**字节，CPU 就会跳进 `Default_Handler` 的死循环，整机停在那里，
> 没有任何输出，也没有任何报错。

这不是"可能"，是链接期就确定的：`.weak` 定义没被用户实现覆盖，就指向死循环。
**排查时表现为"一接串口就死机"**，而代码里 `HAL_UART_Receive_IT` 的返回值是 `HAL_OK`。
这条与 Day5 §5 的"`__weak` 空回调会让中断正常进、正常返回、什么也不发生"是同一类陷阱的
**更狠版本**：那个至少系统还活着。

补上它需要三件事，缺一不可（对照 Day5 的三块寄存器）：

```c
/* 1. Core/Src/stm32f1xx_it.c */
void USART1_IRQHandler(void) { HAL_UART_IRQHandler(&huart1); }

/* 2. 启动接收 */
HAL_UARTEx_ReceiveToIdle_IT(&huart1, g_rxbuf, sizeof g_rxbuf);

/* 3. 使能 NVIC —— 这一条最容易被漏，因为前两步都不会报错 */
HAL_NVIC_SetPriority(USART1_IRQn, 1, 0);
HAL_NVIC_EnableIRQ(USART1_IRQn);
```

第 3 步漏掉的症状是"ISR 写好了、`CR1` 的中断位也置了，但永远不进"——
与 Day5 §2 那张"漏掉 NVIC"的表完全一致。

### 7. 教材 7.5 的三件套，在 GCC 下对应成什么

教材 7.5 是 **Keil + MicroLIB** 路线，要求实现三个函数：

```c
int fputc(int ch, FILE* stream);   /* printf 的落点 */
int fgetc(FILE *f);                /* scanf/getchar 的落点 */
int __backspace(FILE *stream);     /* MicroLib 输入缓冲回退一位 */
```

`__backspace` 这个函数名很陌生，它的用途是**给 scanf 的"读取失败要回退一个字符"用**——
教材链接的 ARM 文档标题就叫 "Re-implementing `__backspace` in the C library"。
教材的 `fgetc` 实现里那个 `g_backspace` 标志位就是配合它：`__backspace()` 只置标志，
`fgetc()` 下次被调用时先吐回上一个字符。

GCC + newlib 走的是另一套（Day7 §7 已给出）：`_write` / `_read`，没有 `fputc`/`__backspace`。
所以本工程 `driver_uart.c` 里那个 `fputc` **是死代码**，而教材 7.5 的完整实现
**在本工程里不能直接照抄**——照抄的结果是"编译通过、`printf` 无输出"。

对照表：

| 教材（MicroLIB） | 本工程（newlib） | 本工程是否实现 |
| --- | --- | --- |
| `fputc` | `_write` | ✅ `Core/Src/retarget.c` |
| `fgetc` | `_read` | ⚠️ **是桩，返回 0** |
| `__backspace` | 无对应 | —— |

### 8. `_isatty` 与 `_read`：两个不显眼但决定行为的桩

`Core/Src/crt_stubs.c` 里有两行最容易被当成样板代码跳过：

```c
int _isatty(int fd) { (void)fd; return 1; }        /* 声称"都是终端" */

int _read(int fd, char *ptr, int len) {            /* 输入没有实现 */
    (void)fd; (void)ptr; (void)len;
    return 0;                                       /* 0 = EOF */
}
```

**`_isatty` 返回 1 是 `printf` 能立刻出字的前提。** newlib 初始化 `stdout` 时问一次 `_isatty`：

| `_isatty` 返回 | stdout 缓冲方式 | 现象 |
| --- | --- | --- |
| 1（终端） | **行缓冲**：遇 `'\n'` 就 flush | `printf("x\n")` 立刻出字 |
| 0（非终端） | **全缓冲**：攒满 `BUFSIZ` 才 flush | `printf` 长时间"没输出"，然后一大坨涌出 |

所以"`printf` 不输出"的第一嫌疑不是 `_write` 写错了，而是**缓冲**。判断入口：
把 `printf` 后面加一句 `fflush(stdout)`，如果这时出字了，问题在缓冲不在重定向。

`_read` 返回 0 意味着 `getchar()`/`scanf()` **立刻拿到 EOF**。这就是为什么本工程只能用
`driver_uart.c` 的 `fgetc`（直接读 USART，不走 stdio）来做输入——教材 7.5 那套"完善 stdio"
在本工程里**没有做**，只在输出方向做了一半。

这条也说清了 `crt_stubs.c` 注释里那句话的分量：**桩函数不是"占位"，它们的返回值就是行为**。
返回 1 和返回 0 的差别是"能看见输出"和"看不见输出"。

---

## 系统调用/接口卡片

### `HAL_StatusTypeDef HAL_UARTEx_ReceiveToIdle_DMA(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)`

作用：启动最多 `Size` 字节的 DMA 接收，收满（TC）或总线空闲（IDLE）都会结束并回调。

参数：`pData` 必须在整个接收期间有效（DMA 直接写内存，不经过栈保护）；`Size` 是 `uint16_t`，
F1 的 DMA `CNDTR` 是 16 位，**超过 65535 会被截断**。

返回值和错误：`HAL_OK` / `HAL_ERROR`（参数非法，或启动时已有挂起错误如 `ORE` 导致
`ReceptionType` 被重置）/ `HAL_BUSY`（`RxState != READY`，上一次接收还没结束）。

资源所有权：HAL 把 `huart->RxState` 置为 `BUSY_RX`，回调返回前不改回；**必须在回调里重新启动**
才能收下一帧。`IDLEIE` 由 HAL 置位，Abort/结束时由 HAL 清。

线程/进程安全：不可重入。在 ISR 里重复调用得到 `HAL_BUSY`。

最小示例：

```c
static uint8_t g_rxbuf[256];

void app_uart_start(void)
{
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, g_rxbuf, sizeof g_rxbuf);
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    /* Size = 本次已收到的字节数；Type 区分"收满"与"空闲结束" */
    if (HAL_UARTEx_GetRxEventType(huart) == HAL_UART_RXEVENT_IDLE) {
        /* 不定长帧：通常也在这里处理 */
    }
    app_parse(g_rxbuf, Size);
    HAL_UARTEx_ReceiveToIdle_DMA(huart, g_rxbuf, sizeof g_rxbuf);  /* 必须重启 */
}
```

常见误用：
- 回调里不重启 → 只收一帧。
- 回调里 `printf`/刷 OLED → 阻塞（Day7 §5、Day6 §3）。
- 把 `Size` 当"新增" → 循环模式下会重复处理旧数据。
- 忘了 `DMA` 通道与 NVIC（`DMA1_Channel5_IRQn`），症状是"启动了但回调不来"。

### `HAL_StatusTypeDef HAL_UARTEx_ReceiveToIdle_IT(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)`

作用：中断方式的不定长接收。每来一字节进一次 `RXNE` 中断；收满或 IDLE 时回调。

与 DMA 版的差别：**CPU 每字节都要进中断**（这是教材说的"中断方式"的固有成本），
但不需要配 DMA 通道，`pData` 也不是 DMA 直接写的。

额外要求：必须存在 `USART1_IRQHandler` 并已使能 `USART1_IRQn`——**本工程今天都没有**（§6）。

### `HAL_UART_RxEventTypeTypeDef HAL_UARTEx_GetRxEventType(UART_HandleTypeDef *huart)`

作用：取"是哪种事件导致回调"。

返回值和错误：`HAL_UART_RXEVENT_TC` / `_HT` / `_IDLE`。无错误返回。

常见误用：在回调之外调用（拿到的是下一次的值，HAL 注释明确要求只能在回调内用）。

### `void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)`（`__weak`）

作用：不定长接收的统一出口。`__weak` 空实现位于 `stm32f1xx_hal_uart.c:2710`。

常见误用：与 `HAL_UART_RxCpltCallback` 混淆。两者**都会被调用**（收满时 HAL 先调 RxEvent
再决定是否调 RxCplt），只写一个时另一条路径可能空转。建议只用一个、并在里面显式判 `Type`。

### `int _write(int fd, const char *buf, int len)`

作用：newlib 的 stdout/stderr 落点，`printf` 最终走到这里。

参数：`len` 是本次要写的字节数；`retarget.c` 里转成 `(uint16_t)` 传给 HAL，**超过 65535 截断**。

返回值和错误：成功返回 `len`（必须等于 `len`，否则 newlib 认为写失败）；
`huart1.gState == HAL_UART_STATE_RESET` 时置 `errno = EIO` 返回 -1。

常见误用：返回 0 或正数但小于 `len` → 表现为"打印被截断"；在 `_write` 里再 `printf` → 递归。

---

## 最小可运行实验

目标：不接串口，用符号与启动文件把 §6 的"必死"结论坐实。

```bash
cd experiments/F103_HAL_DriverLab

# 1) 工程里到底有没有 USART1 的中断入口（预期：无输出）
grep -n "USART1_IRQHandler\|USART1_IRQn" Core/Src/stm32f1xx_it.c Core/Src/*.c

# 2) 镜像里这个符号指向谁（预期：地址 = Default_Handler）
arm-none-eabi-nm build/f103_hal_driverlab.elf | grep -E "USART1_IRQHandler|Default_Handler"

# 3) 确认它落在死循环上（预期：b.n 自身）
arm-none-eabi-objdump -d --disassemble='Default_Handler' build/f103_hal_driverlab.elf
```

第 2 步是决定性证据：如果 `USART1_IRQHandler` 与 `Default_Handler` 是**同一个地址**，
说明没有任何用户实现覆盖它，中断一到就是死循环。
第 3 步反过来提供机制：那条 `b.n` 就是"死机"的实体。

---

## 编译与运行

```bash
cmake --build build
```

真正的 IDLE 实验需要真板 + USB-TTL：PC 端发不定长数据，观察回调是否在最后一字节后
≈ 86.8 µs 触发（本机未做）。

---

## 结果观察

**HAL 侧能力**（源码核对，全部存在）：

```text
HAL_UARTEx_ReceiveToIdle          查询，出参 RxLen
HAL_UARTEx_ReceiveToIdle_IT       中断
HAL_UARTEx_ReceiveToIdle_DMA      DMA
HAL_UARTEx_GetRxEventType         TC / HT / IDLE
HAL_UARTEx_RxEventCallback        __weak，stm32f1xx_hal_uart.c:2710
__HAL_UART_CLEAR_IDLEFLAG         = __HAL_UART_CLEAR_PEFLAG（读 SR 再读 DR）
```

**工程侧缺口**（逐文件核对）：

| 项 | 状态 | 后果 |
| --- | --- | --- |
| `USART1_IRQHandler` | **不存在** | 任何 `_IT` 接收，收到 1 字节即进 `b Infinite_Loop` |
| `HAL_NVIC_EnableIRQ(USART1_IRQn)` | **不存在** | 即使补了 ISR 也不进 |
| `IDLEIE` | 从未置位 | IDLE 中断从未使能 |
| DMA 通道（USART1_RX = DMA1_Ch5） | 未配 | `ReceiveToIdle_DMA` 会拿 `NULL` 句柄走不通 |
| `_read` | 返回 0（桩） | `getchar`/`scanf` 立刻 EOF |
| `_isatty` | 返回 1 | stdout 行缓冲，`printf` 遇 `\n` 即出字 |

**结论一句话**：本工程的串口**只有"单向、阻塞、轮询"这一条通路是通的**。
教材 7.3 的中断方式、7.3.6 的 DMA、7.4 的 IDLE，**代码全没落地**。
这不是缺陷，是范围——`F103_HAL_DriverLab` 的目标是"把 19 个模块跑起来"，
不是"实现一个完整串口框架"。但读教材时必须知道这个边界，否则会以为"照着写了就能用"。

---

## 常见错误和陷阱

| 陷阱 | 症状 | 根因 | 处理 |
| --- | --- | --- | --- |
| 调 `_IT` 但没写 `USART1_IRQHandler` | 一收数据整机停住 | 落到 `Default_Handler` 的 `b .` | 补 ISR + NVIC |
| 写了 ISR 但没 `HAL_NVIC_EnableIRQ` | 完全不进中断 | `NVIC->ISER` 位为 0 | 补 `SetPriority` + `EnableIRQ` |
| 以为 IDLE 一上电就触发 | "为什么没中断" | 至少要先收到 1 字节 | 先发数据再观察 |
| 只读 `SR` 想清 IDLE | IDLE 反复出现 | 清 IDLE 需读 SR **再**读 DR | 用 `__HAL_UART_CLEAR_IDLEFLAG` |
| 手写 ISR 里清 IDLE | 偶发丢一字节 | 该宏会把 `DR` 读走 | 判 `RXNE` 后再清，或走 HAL |
| 回调里不重启接收 | 只收一帧 | `RxState` 停在 `BUSY_RX` | 回调末尾重新 `ReceiveToIdle_*` |
| 把回调 `Size` 当"新增字节数" | 循环模式下重复处理 | `Size` 是累计位置 | 自己存上次值做差 |
| `printf` 长时间不输出 | 最后一次性涌出 | `_isatty` 返回 0 → 全缓冲 | 返回 1，或 `fflush(stdout)` |
| `scanf`/`getchar` 立刻 EOF | 读不到任何输入 | `_read` 返回 0 | 用驱动 `fgetc`，或实现 `_read` |
| 照抄教材 `fputc` 到 GCC 工程 | `printf` 无输出 | newlib 走 `_write` | 看 `nm` 确认调用链 |

---

## 故障排查方法

"不定长接收没反应"按**从使能到数据**五步，每步一个可读状态：

1. **`USART1_IRQn` 是否使能**。`NVIC->ISER` 对应位。为 0 → 停在第一步，后面全不用查。
2. **`USART1->CR1` 的 `RXNEIE` / `IDLEIE`**。`RXNEIE=0` 说明接收中断压根没开；
   `IDLEIE=0` 说明用的是普通 `Receive_IT` 而不是 `ReceiveToIdle_*`。
3. **`huart1.RxState`**。停在 `BUSY_RX` 说明上一次接收没结束（回调里没重启）。
4. **`USART1->SR` 的 `IDLE`/`RXNE`/`ORE`**。`ORE` 置位 → 见 Day7 §5，接收已经废了；
   `IDLE=1` 但没进中断 → 回到第 1、2 步。
5. **`huart1.ReceptionType`**。等于 `STANDARD` 而你以为在用 `TOIDLE`，说明启动那次
   `ReceiveToIdle_*` 返回了 `HAL_ERROR`（§5），返回值被丢弃了。

"`printf` 不输出"的排查方向完全不同，从**缓冲**查起而不是从硬件：

1. `fflush(stdout)` 后是否出字。出 → 缓冲问题（`_isatty`）。
2. `_write` 是否被调用（断点或计数）。
3. `huart1.gState` 是否 `RESET`（`retarget.c` 在这种情况返回 -1）。
4. `USART1->CR1` 的 `TE`、`CR2` 的 `STOP`、`BRR` 是否与 PC 端一致（Day7 §4）。

---

## 练习题

### 基础题

题目：PC 端向板子发 `"OK"` 两个字节后停手（`115200 8N1`）。若用 `ReceiveToIdle_DMA`，
从第一个字节起始位到 IDLE 中断触发，大约经过多长时间？其中"等 IDLE"占多少？

提示：每字节 10 bit（Day7 §2）；IDLE 判据是"一个字节时间内无新数据"。

验收标准：能算出 ≈ 2×86.8 µs + 86.8 µs ≈ 260 µs，并指出 IDLE 等待占 ≈ 86.8 µs；
能说明这个延迟与波特率成正比、与数据量无关。

### 改错题

题目：下面的代码"编译通过、返回 `HAL_OK`，但接上串口就死机"，指出原因并给出最小修正。

```c
static uint8_t g_rxbuf[64];

void app_init(void)
{
    HAL_UARTEx_ReceiveToIdle_IT(&huart1, g_rxbuf, sizeof g_rxbuf);
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    g_last_len = Size;
}
```

提示：§6 的三处缺口逐个对；还要看回调里少做了什么。

验收标准：能指出 **① 缺 `USART1_IRQHandler`（落到 `Default_Handler` 的 `b .`）**、
**② 缺 `HAL_NVIC_EnableIRQ(USART1_IRQn)`**、**③ 回调里没重启接收**；
并说明三个缺口的症状为什么各不相同（死机 / 无声无息 / 只收一帧）。

### 综合题

题目：给本工程补一个"不定长命令行"接收模块，要求：① 使用 `ReceiveToIdle_DMA`；
② 以 `'\n'` 为帧尾，超长（>256 字节）时丢弃并计数；③ 说明 `Size` 与"新增字节数"的关系，
以及为什么缓冲区不能在回调里用栈变量。

提示：DMA 直接写 `pData`，与 CPU 执行栈帧无关；`Size` 语义见 §4 末尾。

验收标准：能写出 `HAL_UARTEx_RxEventCallback` 的完整处理（判 `Type`、累加/重置、重启）
且明确"`dma` 缓冲必须是 `static`/全局"；能说明超长帧为什么要先判再截断，
而不是直接 `memcpy` 到固定缓冲。

---

## 验收标准

- [ ] 能说明 DMA 补的是"怎么搬"、IDLE 补的是"什么时候完"，并列出三种结束条件与对应回调。
- [ ] 能解释 IDLE 为什么不会一上电就触发，以及"一个字节时间"在本工程是多少微秒。
- [ ] 能写出 `__HAL_UART_CLEAR_IDLEFLAG` 的实际展开，并说出它顺带清掉的另外四个标志。
- [ ] 能说清 `RxEventCallback` 的 `Size` 在中断/DMA 模式下的语义与 `GetRxEventType` 的三个取值。
- [ ] 能从 `stm32f1xx_it.c` 与启动文件推出"本工程调 `_IT` 会死机"，并用 `nm` 坐实。
- [ ] 能说出教材 `fputc`/`fgetc`/`__backspace` 在 newlib 下对应哪些符号、哪些没实现。
- [ ] 能解释 `_isatty` 返回 1 与 0 的差别，以及"`printf` 不输出"为什么先查缓冲。

## 延伸阅读

- 教材：《HAL快速入门与项目实战(基于DshanMCU-103)》第 7.4 节（IDLE 中断）、
  第 7.5 节（完善程序与 stdio、MicroLib 的 `fputc`/`fgetc`/`__backspace`）
- HAL 源码：`Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_uart.c:1571-1836`（三种 `ReceiveToIdle`、
  事件语义）、`:2710`（`__weak` 回调）、`:1858-1866`（`Abort` 清 `IDLEIE`）
- 宏定义：`Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_uart.h:504-510, 542, 759-784`
- 本工程缺口：`Core/Src/stm32f1xx_it.c`（无 `USART1_IRQHandler`）、
  `startup/startup_stm32f103xb.s:113-116, 346-347`（`Default_Handler` 的 `b .`）
- stdio：`Core/Src/retarget.c`（`_write`）、`Core/Src/crt_stubs.c`（`_isatty` / `_read`）
- 上一篇：`Linux/notes/F103_HAL入门/Day07_UART协议与HAL编程.md`
- 下一篇：`Linux/notes/F103_HAL入门/Day09_I2C协议与HAL编程.md`

---

**验证记录**

| 项目 | 内容 |
| --- | --- |
| 日期 | 2026-10-29 |
| 环境一 | Windows 11 + Git Bash；`grep`/`sed` 核对 HAL 与启动文件 |
| 命令一 | `grep -n "__HAL_UART_CLEAR_IDLEFLAG\|__HAL_UART_CLEAR_PEFLAG" stm32f1xx_hal_uart.h`；`sed -n '1755,1836p' stm32f1xx_hal_uart.c`；`grep -n "ReceiveToIdle\|RxEventCallback" stm32f1xx_hal_uart.h` |
| 结果一 | `CLEAR_IDLEFLAG` = `CLEAR_PEFLAG`（读 `SR` 再读 `DR`）；三种 `ReceiveToIdle` 与 `GetRxEventType`/`RxEventCallback` 均在；`ReceiveToIdle_DMA` 在 `ReceptionType` 被重置时返回 `HAL_ERROR`；事件语义注释在 `:1822-1836` |
| 环境二 | 同上 |
| 命令二 | `cat Core/Src/stm32f1xx_it.c`；`grep -n "USART1_IRQHandler" startup/startup_stm32f103xb.s`；`sed -n '110,116p' startup/startup_stm32f103xb.s`；`cat Core/Src/crt_stubs.c` |
| 结果二 | `stm32f1xx_it.c` 只有 TIM4 与 EXTI15_10 两个外设 ISR，**无 USART1**；启动文件 `.weak USART1_IRQHandler` → `Default_Handler`，其体为 `b Infinite_Loop`；`_isatty` 返回 1、`_read` 返回 0 |
| 未覆盖 | **IDLE 从未在真板上产生过**；"`_IT` 一收数据即死机"为启动文件与 `nm` 层面的**静态推断**，未上板复现；`_isatty` 返回 0 导致全缓冲的行为来自 newlib 约定，**未做对照构建验证**；DMA 循环模式下 `Size`（累计位置）的实际数值序列未实测；`USART1_IRQn` 使能与 DMA1_Ch5 配置均未在本工程落地，故 `ReceiveToIdle_*` 的三条路径**一次都没跑通** |
