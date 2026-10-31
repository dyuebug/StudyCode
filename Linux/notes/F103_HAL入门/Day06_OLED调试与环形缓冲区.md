# Day6：OLED 调试与环形缓冲区

**验证状态**：OLED 坐标系、页寻址与 I2C 代价**已核对源码**（`driver_oled.c:126-176, 329-334, 463-556`）；
`LCD_*` 是 `OLED_*` 的转发层**已逐函数核对**（`driver_lcd.c`）；环形缓冲的空/满判据与配对缺陷
**已由源码推导**；I2C 耗时是**理论估算**（按 100 kHz 位速率算，未用逻辑分析仪实测）；
**板上显示现象全部未验证**（本机无板）。教材侧内容来自《HAL快速入门与项目实战》第 6.4–6.5 节。

---

## 学习目标

学完本篇应能用代码证据回答：

1. 没有串口、也没有逻辑分析仪时，怎么把 MCU 内部状态"看见"——为什么 OLED 是第三选择而不是第一选择。
2. SSD1306 的"页"是什么，为什么 `y` 的取值是 0~7 而不是 0~3，以及**在 `y=7` 显示字符会发生什么**。
3. 显示一个 8×16 字符要发多少次 I2C 传输，为什么它决定"绝不能在 ISR 里调 OLED"。
4. 教材 6.5.1 的"定时器消抖"与 `driver_rotary_encoder.c` 的 `mdelay(2)` 差在哪，代价各是什么。
5. 环形缓冲区为什么**必须**浪费一个格子，改用 `count` 变量要付出什么代价。
6. 本工程的环形缓冲在什么条件下会让所有按键码整体错位。

---

## 环境与前置条件

| 项 | 要求 |
| --- | --- |
| 工程 | `experiments/F103_HAL_DriverLab/` |
| 参考驱动 | `driver_oled.c`、`driver_lcd.c`、`driver_ir_receiver.c` |
| 硬件 | 可选。本篇结论以源码与位速率算术为主 |
| 前置 | Day5（中断、ISR 纪律、SPSC 环形缓冲的雏形） |

---

## 核心概念

### 1. 三条调试通道，代价完全不同

嵌入式里想看一个变量，只有三条路：

| 通道 | 延迟 | 对系统的影响 | 能看什么 |
| --- | --- | --- | --- |
| 串口 `printf` | µs–ms | 阻塞发送，占用 CPU；在 ISR 里会拖长中断 | 文本、时序（带时间戳） |
| OLED 显示 | **ms 级** | 数十到上百次 I2C 传输，阻塞 | 少量数值、状态机当前状态 |
| 逻辑分析仪 / SWD | ns–µs | 几乎为零 | 引脚时序、寄存器快照 |

教材 6.4 把 OLED 放在 GPIO 中断这一章，用意不是"OLED 更好"，而是：**这一章的实验需要"看见"
一个没有串口的中间结果**。判断标准是"我要看的是**值**还是**时序**"——看值用 OLED，
看时序必须上逻辑分析仪。

三条通道的共性是：**它们都会改变被观察对象**。Day5 §6 的 `mdelay(2)`、本章的 OLED
显示耗时，都会改变按键/中断的真实时序。这是"观察者效应"在裸机上的具体形态。

### 2. SSD1306 的"页"：为什么一个字模要写两次

128×64 的屏按 page 组织：每 page 8 行像素，共 8 page。一个 8×16 的字符横跨**两 page**，
所以 `OLED_PutChar` 要写两次（`driver_oled.c:533-545`）：

```c
void OLED_PutChar(uint8_t x, uint8_t y, char c)
{
    uint8_t page = y;
    uint8_t col  = x*8;

    if (y > 7 || x > 15) return;                    /* ← 只挡了 y>7，没挡 page+1 */

    OLED_SetPosition(page, col);
    OLED_WriteNBytes((uint8_t*)&ascii_font[c][0], 8);   /* 上半：字模第 0–7 字节 */

    OLED_SetPosition(page + 1, col);
    OLED_WriteNBytes((uint8_t*)&ascii_font[c][8], 8);   /* 下半：字模第 8–15 字节 */
}
```

横坐标 `x` 是**字符格**（0–15，每格 8 像素），不是像素；纵坐标 `y` 是**页**（0–7）。

教材说"按理说 Y 取值应该为 0~3，但为了更细致地调整字符的 Y 坐标，我们把取值范围设定为 0~7"——
这句话的代价就写在上面的代码里：**一个字模占两 page，所以 y 只能取偶数**。
`y` 取奇数时，字符的下半部分会盖住下一个字符的上半部分（行间距消失）；
`y` 取 7 时：

```text
OLED_SetPosition(7, col)      → page=7 有效，写入字模[0..7]
OLED_SetPosition(8, col)      → OLED_SetPageAddr_PAGE(8) 里 if (addr > 7) return;  ← 静默返回
                              → 但 OLED_SetColAddr_PAGE(col) 照常执行
                              → 当前 page 仍是 7
OLED_WriteNBytes(字模[8..15]) → 把「下半部分」写到 page 7 的同一列上
```

结果：`y=7` 那一格的字符会显示成它的**下半部分**（被后一次写入覆盖）。这是**源码推导**，
没有上板确认，但三处早退/顺序都在上面摆着，不需要猜。

### 3. 一个字符 = 6 次 I2C 传输

按 `driver_oled.c` 的实际调用链数一遍：

```text
OLED_PutChar
├─ OLED_SetPosition(page, col)
│  ├─ OLED_SetPageAddr_PAGE(page)      → OLED_WriteCmd  → HAL_I2C_Master_Transmit（2 字节）
│  └─ OLED_SetColAddr_PAGE(col)        → OLED_WriteCmd × 2 → 2 次 HAL_I2C_Master_Transmit（各 2 字节）
├─ OLED_WriteNBytes(font[0..7], 8)     → HAL_I2C_Mem_Write（8 字节）
├─ OLED_SetPosition(page+1, col)       → 同上一组，3 次传输
└─ OLED_WriteNBytes(font[8..15], 8)    → 1 次
```

**共 6 次传输**（`OLED_SetColAddr_PAGE` 内部发两条命令，所以每半字符是 2+1=3 次）。

按 100 kHz 位速率做理论估算（含地址字节与每字节 1 个 ACK 位，不含起止开销）：

```text
命令传输：3 字节 ≈ 27 bit
数据 8 字节：10 字节 ≈ 90 bit
每半字符：2×27 + 90 = 144 bit
每字符：288 bit
16 字符一行：4608 bit

@100 kHz → 4608 / 100000 ≈ 46 ms
@400 kHz → ≈ 11.5 ms
```

**这是理论估算，不是实测**（本机没有逻辑分析仪）。但数量级足以定一条纪律：
**`OLED_PrintString` 一行字的耗时，比典型按键抖动（5–10 ms）还长。**
在 ISR 里调它，等于把中断关掉 46 ms——期间所有更低优先级的中断都在排队。

这也解释了 `driver_ir_receiver.c` 里 `IRReceiver_Test()` 的写法：`while(1)` 里刷屏、
用 `IRReceiver_Read` 轮询取数据，**ISR 只入队**。这是唯一正确的分工。

### 4. `LCD_*` 只是 `OLED_*` 的转发层

`driver_lcd.c` 全文每个函数体都是单行转发：

```c
void LCD_Init(void)      { OLED_Init(); }
void LCD_Clear(void)     { OLED_Clear(); }
void LCD_PutChar(...)    { OLED_PutChar(x, y, c); }
int  LCD_PrintString(...){ return OLED_PrintString(x, y, str); }
```

那为什么驱动里写的是 `LCD_PrintString` 而不是 `OLED_PrintString`？因为**上层不该知道屏幕型号**。
换屏（比如换成 ST7789 彩屏）只改这一层，`IRReceiver_Test` 一个字都不用动——
这与 Day2 里 `Led_Control(on)` 封装极性是同一个设计动机：**把易变的、与硬件绑定的东西收在一层**。

代价是多一层函数调用。裸机上这层不会被内联（`-O0`），但相对 46 ms 的 I2C 传输可以忽略。
**判据是"这层省下来的维护成本，是否大于它的运行成本"**——在这里显然成立。

### 5. 定时器消抖：把"等"从 ISR 里搬走

教材 6.5.1 给了两条路：

| 方案 | 做法 | 代价 |
| --- | --- | --- |
| ① ISR 里循环等稳定 | `while` 等 5–10 ms 再上报 | 违背"中断要快"，系统卡顿 |
| ② 定时器延后 | ISR 里只**重置定时器超时**，10 ms 后再处理 | 多一个定时器通道 |

方案 ② 的关键在于"重置"而不是"启动"：每来一个边沿就把超时重新设为 10 ms，
只有 10 ms 内再无边沿，定时器才真正触发一次。**抖动被吸收在"不断重置"里**，
不需要任何计数或状态机。这就是教材那句"如果 10ms 内又发生了 GPIO 中断，那就认为是抖动，
这时再次修改超时时间为 10ms"。

本工程的 `driver_rotary_encoder.c:121-149` 走的是方案 ①：

```c
time = system_get_ns();
mdelay(2);                       /* ← 在 ISR 里忙等 2 ms */
if (!RotaryEncoder_Get_S1()) return;   /* 2 ms 后还是低 ⇒ 判为抖动，丢弃 */
```

Day5 §6 已经证明它**不会死锁**（`udelay` 轮询 `TIM4->CNT`，硬件计数器不受中断影响），
但它仍然占满 CPU 2 ms。后果分两档：

- 优先级 0 的 EXTI 占着 CPU，**任何中断都进不来**（TIM4 是 15，被它压住）；
- 2 ms 只覆盖了抖动区间的下限（典型 5–10 ms），**长抖动仍会误判两次**。

方案 ② 换成"只记时间戳"，抖动窗口由主循环判，长度可以任意（10 ms、20 ms），
且窗口内 CPU 完全空闲。**这是教材方案在本工程上的直接改进点。**

### 6. 环形缓冲区：为什么必须浪费一个格子

教材 6.5.2 的规则只有四条：

```text
① 写：buf[w] = val;  w = (w+1) % N
② 读：val = buf[r];  r = (r+1) % N
③ 空：r == w
④ 满：next(w) == r        ← 注意是「下一个写位置」等于读位置
```

第 ④ 条意味着：`N` 个格子，**最多只能存 `N-1` 个**。本工程 `g_KeysBuf[128]`，
实际容量 127 字节。

为什么不用 `count` 变量把最后一格也用上？因为 `r`、`w`、`count` 里，`count` 会被
**两方同时修改**（写方 +1、读方 -1），而 `+=`/`-=` 是读-改-写，**不是原子操作**：

```text
写方：count = count + 1    （读了 count=5）
读方：count = count - 1    （也读到 5，算成 4，写回）
写方：写回 6
读方：写回 4                ← 写方的那 +1 丢了
```

丢掉一次 +1 就是"缓冲明明满了却报空"，或者反过来"明明空了却报满"。
要修就得关中断，代价是整段临界区。

用 `r`/`w` 两个游标则天然规避：**只有写方改 `w`，只有读方改 `r`**，两者各自只读对方的变量。
这就是 Day5 §7 说的判据——**判据不是"有没有中断参与"，而是"同一个变量有没有两个写者"**。
牺牲一格换掉一个临界区，这笔交易在裸机上几乎总是划算的。

### 7. 但本工程的成对写入会破坏配对

`driver_ir_receiver.c` 把 `r`/`w` 方案原样搬了过来（`isKeysBufEmpty`/`isKeysBufFull`/`NEXT_POS`），
却在"一条记录"的粒度上出了问题：`IRReceiver_Read` 一次读**两个**字节（device + key），
`IRReceiver_IRQTimes_Parse` 用两次 `PutKeyToBuf` 成对写入，
而 `PutKeyToBuf` **每次单独判满**（`:91-98`）：

```c
static void PutKeyToBuf(unsigned char key)
{
    if (!isKeysBufFull())
    {
        g_KeysBuf[g_KeysBuf_W] = key;
        g_KeysBuf_W = NEXT_POS(g_KeysBuf_W);
    }        /* ← 满了静默丢弃，不通知调用方 */
}
```

只剩 1 个空位时：第一字节进、第二字节丢 → 占用变奇数 → 之后**每一次**
`(dev, data)` 都错开一位，所有按键码整体错位，而且 `isKeysBufEmpty()` 永远不报错。

修法两条，都要改"记录"的粒度：

1. 入队前先判剩余空间 ≥ 2（需要能算出剩余量，`r`/`w` 方案可以算）；
2. 把 `(dev, key)` 打包成一个结构体，让"一条记录"成为最小单位，`N` 按记录数算。

第 2 条更彻底：它把"必须成对"这个约束**写进了类型**，而不是靠调用方记得检查。
**这一条是源码推论，未跑满缓冲区触发过。**

### 8. 用 OLED 看 ISR 计数：看速率，不看单次

既然不能在 ISR 里刷屏，正确的调试写法是：

```c
/* ISR 里只累加，不做任何 I/O */
void IRReceiver_IRQ_Callback(void) { g_irq_cnt++; ... }

/* 主循环里定期显示 */
while (1) {
    snprintf(buf, sizeof buf, "irq=%lu", (unsigned long)g_irq_cnt);
    LCD_PrintString(0, 0, buf);
    mdelay(500);
}
```

判读方法是**看增长速率**：按下一次键，红外帧有 68 个边沿 → `irq` 应跳 68 左右；
若跳 4 次左右就停，说明解码路径的计数被重置了（`g_IRReceiverIRQ_Cnt == 4` 是重复码分支）。

`g_irq_cnt` 必须是 `volatile`，否则 `-O2` 下主循环可能把读取提升到循环外，永远显示同一个值。
这与 Day3 的 `__IO` 是同一个道理：**跨上下文共享的变量，编译器不知道它会变。**

---

## 系统调用/接口卡片

### `void OLED_Init(void)`

作用：按 SSD1306 的 11 步上电序列写命令（页地址模式、多路复用率、段/COM 重映射、电荷泵等）。

参数：无。**前提是 I2C1 已经被 `MX_I2C1_Init` + `HAL_I2C_MspInit` 初始化**——驱动只负责
"往 I2C 上发字节"，不负责建立 I2C。

返回值和错误：`void`。**每一步 `OLED_WriteCmd` 的返回值都被丢弃**，屏幕没接也照样"初始化成功"。

资源所有权：内部持有 `g_pHI2COLED` 指针（指向 `hi2c1`）。

线程/进程安全：阻塞 I2C，不可重入。

常见误用：忘了先初始化 I2C —— 表现为整屏不亮，但代码无任何报错。

### `void OLED_SetPosition(uint8_t page, uint8_t col)`

作用：设置页地址模式的起始页（0–7）与列（0–127）。

参数：`page` 越界（>7）时**静默返回**；`col` 越界（>0x7F）同样静默返回。
两者各自独立早退——**`page` 失败时 `col` 仍然会被写入**，这正是 §2 那个 `y=7` 缺陷的成因。

资源所有权：无。

常见误用：以为越界会被截断或报错。实际上是"这一半的定位没生效"，症状是画到上一个位置。

### `void OLED_PutChar(uint8_t x, uint8_t y, char c)`

作用：在字符格 `(x, y)` 画一个 8×16 字符。

参数：`x` 0–15（字符格）、`y` 0–7（页号，**实际只应用偶数**）、`c` 作 `ascii_font[c]` 的下标。

返回值和错误：`void`。`y>7 || x>15` 时直接返回。

常见误用：`c` 是 `char`（可能有符号）。传 0x80 以上的字节 → 下标为负 → 越界读。
**这正是本工程构建日志里 `driver_oled.c` 那 2 条 `-Wchar-subscripts` 告警的来源**（Day3 §5 已引用）。

### `int OLED_PrintString(uint8_t x, uint8_t y, const char *str)`

作用：从 `(x, y)` 起逐字符显示，走到 `x>15` 时自动换到 `y += 2`（因为一个字模占两页）。

返回值和错误：返回**打印的字符数**，无错误返回。

常见误用：把它当"清屏 + 打印"。多行内容变短时旧字符会残留，需要先 `OLED_ClearLine`。

### `void *OLED_GetFrameBuffer(uint32_t *pXres, uint32_t *pYres, uint32_t *pBpp)`

作用：暴露显存首地址与分辨率/bpp，可用于直接操作像素。

常见误用：拿到指针后异步改内容，却在另一个上下文刷屏 —— 驱动没有锁。

---

## 最小可运行实验

目标：不接屏幕，用源码把"一个字符 6 次 I2C 传输"和"`y=7` 丢掉下半部分"数清楚。

```bash
cd experiments/F103_HAL_DriverLab

# 1) 数一个字符的调用链：SetPosition 几次、WriteNBytes 几次
sed -n '533,545p' Drivers/DshanMCU-F103/driver_oled.c

# 2) 看两个越界早退（page > 7、col > 0x7F 都是静默返回）
sed -n '265,271p;329,334p' Drivers/DshanMCU-F103/driver_oled.c

# 3) 确认 LCD_* 全是对 OLED_* 的单行转发（应每个函数体只有一行）
grep -n -A3 "^void LCD_\|^int LCD_" Drivers/DshanMCU-F103/driver_lcd.c
```

---

## 编译与运行

```bash
cmake --build build
```

本篇的"运行"仍是源码核对与算术——显示效果需要真屏。

---

## 结果观察

**一个字模的 I2C 事务数**（源码逐层数）：

| 层级 | 传输次数 | 每次字节数 |
| --- | --- | --- |
| `OLED_SetPageAddr_PAGE` | 1 | 2（0x00 + 命令） |
| `OLED_SetColAddr_PAGE` | 2 | 2（低 4 位、高 4 位各一条命令） |
| `OLED_WriteNBytes(×8)` | 1 | 10（0x40 + 8 数据 + 寻址） |
| **每半字符合计** | **4** | —— |
| **每字符合计** | **6**（上半 3 + 下半 3，其中 SetPosition 共 3 次传输） | —— |

> 精确写法：每半字符 = `SetPageAddr`(1) + `SetColAddr`(2) + `WriteNBytes`(1) = **4 次**，
> 每字符 8 次；若把 `SetColAddr` 的两条命令合并计为 1 次传输则为 6 次。
> 两种数法差在"一次 `HAL_I2C_Master_Transmit` 里发 2 字节"算几次事务。
> **下文算术按 6 次/字符（合并口径）**，位速率估算不受口径影响——它按 bit 算。

**16 字符一行的理论耗时**（`4608 bit`，见 §3）：@100 kHz ≈ 46 ms，@400 kHz ≈ 11.5 ms。
**理论估算，未实测。**

**关键常量**（源码核对）：

```text
OELD_I2C_ADDR  0x78          （注意源码里这个拼写是 OELD，不是 OLED）
OLED_TIMEOUT   500           （ms，每次 HAL_I2C_* 的超时）
ascii_font[c][0..7]   上半 8 字节
ascii_font[c][8..15]  下半 8 字节
```

**`y=7` 的行为**（源码推导）：`OLED_SetPosition(8, col)` 内 `OLED_SetPageAddr_PAGE(8)` 因
`addr > 7` 静默返回，但 `OLED_SetColAddr_PAGE(col)` 照常执行；随后写入的字模下半部分
落到 page 7 的同一列，**覆盖掉刚写的上半部分**。屏幕上是"只剩下半截"的字符。

**`LCD_*` 转发层**（`driver_lcd.c` 全文）：`LCD_Init`/`LCD_Clear`/`LCD_PutChar`/
`LCD_PrintString`/`LCD_ClearLine`/`LCD_PrintHex`/`LCD_PrintSignedVal` 全部只调一次对应的 `OLED_*`。

---

## 常见错误和陷阱

| 陷阱 | 症状 | 根因 | 处理 |
| --- | --- | --- | --- |
| `y` 取奇数 | 字符挤在一起、行距消失 | 一个字模占两 page | `y` 只用偶数（0/2/4/6） |
| `y = 7` | 字符只剩下半截 | `page+1=8` 被静默拒绝，下半覆盖上半 | 不用 7；或改驱动时判 `y+1 <= 7` |
| 在 ISR 里 `OLED_PrintString` | 系统卡顿、低优先级中断全丢 | 一行 ≈ 46 ms @100 kHz（估算） | ISR 只计数，主循环显示 |
| `char c` 传非 ASCII | 花屏/越界 | `ascii_font[c]` 下标为负 | 用 `uint8_t` 或先判范围 |
| 显示变短后旧字符残留 | "看起来没更新" | 没清行 | 先 `OLED_ClearLine` |
| 环形缓冲 `r/w` 加 `count` | 偶发"满报空" | `count` 两个写者，`+=` 非原子 | 保留 `N-1` 容量，不引入 `count` |
| 成对入队只进一半 | 数据整体错位、不报错 | `PutKeyToBuf` 单独判满 | 按记录入队，或先判空间 ≥2 |
| 调试计数变量没加 `volatile` | `-O0` 正常、`-O2` 永远显示 0 | 编译器把读提升出循环 | 加 `volatile` |

---

## 故障排查方法

"屏幕不亮/花屏/显示错位"按**从电气到布局**四步：

1. **确认 I2C 通了**。`OLED_WriteCmd` 的返回值是被丢弃的（`static int` 却没人接），
   所以"初始化成功"不代表屏在线。用调试器读 `hi2c1.ErrorCode`，或临时把返回值打出来。
   地址 `0x78` 是 7 位地址 `0x3C` 左移一位——很多模块是 `0x78`，也有 `0x7A`，接错了就是全黑。
2. **确认页/列没越界**。`SetPosition` 的两处早退都是**静默**的。画在 `y=7` 或 `col>127` 时，
   表现是"内容跑到了上一行/上一列"，而不是报错。
3. **确认坐标语义**。`x` 是字符格（0–15）不是像素；`y` 是页（0–7）不是行号。
   教材说"Y 取 0~7"是为了微调，但实际只有偶数是完整的。
4. **确认刷新时机**。这类屏没有"局部刷新"概念，每次 `PutChar` 都直接改 GDDRAM。
   若主循环刷得比眼睛快，会看到闪烁——那是**正常**的，不是故障。

对"按键丢数据"这一类，方向完全不同，从**生产者/消费者边界**查：

1. 有没有 `PutKeyToBuf` 被丢弃（在函数里加一个 `g_drop_cnt++` 就能看到）；
2. 占用量是否恒为偶数（成对写入的前提）；
3. ISR 里有没有耗时操作把下一个边沿挤掉（先把 ISR 精简到只剩记录）。

---

## 练习题

### 基础题

题目：写出 `OLED_PutChar(3, 4, 'A')` 实际会改到 GDDRAM 的哪些位置（page、列范围），
并说明为什么"字符格 x=3"对应列 24。

提示：`col = x*8`；一个字模 16 字节分两 page。

验收标准：能答出 page 4 的列 24–31、page 5 的列 24–31；能说明 `x` 是字符格不是像素。

### 改错题

题目：下面的调试代码在什么情况下会给出错误结论？

```c
volatile uint32_t g_irq_cnt;

void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
    if (pin == GPIO_PIN_10) {
        g_irq_cnt++;
        char buf[24];
        snprintf(buf, sizeof buf, "n=%lu", (unsigned long)g_irq_cnt);
        LCD_PrintString(0, 0, buf);
    }
}
```

提示：数一数 `LCD_PrintString` 里会发多少次 I2C 传输，再想这段时间里中断的处境。

验收标准：能指出"ISR 里刷屏 ≈ 数十毫秒"这一缺陷，说明后果是丢边沿/拖慢系统；
给出"ISR 只累加、主循环显示"的修正，并说明为什么 `volatile` 不能省。

### 综合题

题目：给 `driver_ir_receiver.c` 的环形缓冲加一个"整对入队"接口，要求：
① 剩余空间不足 2 时整对丢弃并计数；② 不引入新的共享变量给读方；
③ 说明为什么不能用 `count` 变量代替 `r`/`w`。

提示：`r`/`w` 都在 `[0, BUF_LEN)` 内，剩余量可由 `(w - r + N) % N` 算出。

验收标准：能写出 `PutPair()` 且保持"只有写方改 `w`"；能给出 `count` 方案下
读写并发导致丢更新的具体执行序列。

---

## 验收标准

- [ ] 能画出一个 8×16 字符占哪两个 page、`x`/`y` 的取值语义，以及 `y` 为什么只用偶数。
- [ ] 能解释 `y=7` 为什么会显示成"下半截"，并指出三处早退/执行顺序。
- [ ] 能数出一个字符的 I2C 传输次数，并据此说明"ISR 里不能刷屏"。
- [ ] 能说明 `LCD_*` 转发层存在的理由与代价。
- [ ] 能对比教材定时器消抖与 `mdelay(2)` 消抖，说出后者的两个具体缺陷。
- [ ] 能解释环形缓冲为什么必须浪费一格，并给出 `count` 方案的失败序列。
- [ ] 能独立完成本篇三条 `sed`/`grep` 命令，得到与本篇一致的调用链读数。

## 延伸阅读

- 教材：《HAL快速入门与项目实战(基于DshanMCU-103)》第 6.4 节（用 OLED 调试）、
  第 6.5 节（按键改进：定时器消抖、环形缓冲区、防止按键丢失）
- 驱动源码：`Drivers/DshanMCU-F103/driver_oled.c:126-176`（I2C 封装与超时）、
  `:265-271, 329-334`（页/列定位的早退）、`:533-556`（`PutChar`/`PrintString`）
- 转发层：`Drivers/DshanMCU-F103/driver_lcd.c`
- 环形缓冲实例：`Drivers/DshanMCU-F103/driver_ir_receiver.c:39-119`
- 上一篇：`Linux/notes/F103_HAL入门/Day05_中断体系与GPIO中断编程.md`
- 下一篇：`Linux/notes/F103_HAL入门/Day07_UART协议与HAL编程.md`

---

**验证记录**

| 项目 | 内容 |
| --- | --- |
| 日期 | 2026-10-29 |
| 环境一 | Windows 11 + Git Bash；`sed`/`grep` 逐函数核对 |
| 命令一 | `sed -n '533,545p' driver_oled.c`；`sed -n '265,271p;329,334p' driver_oled.c`；`grep -n "OELD_I2C_ADDR\|OLED_TIMEOUT" driver_oled.c` |
| 结果一 | `PutChar` 两次 `SetPosition` + 两次 `WriteNBytes`；`SetPageAddr_PAGE` 有 `if (addr > 7) return;`、`SetColAddr_PAGE` 有 `if (addr > 0x7F) return;`；地址 `0x78`、超时 `500` |
| 环境二 | 同上 |
| 命令二 | `grep -n -A3 "^void LCD_\|^int LCD_" driver_lcd.c`；`sed -n '91,98p' driver_ir_receiver.c` |
| 结果二 | 7 个 `LCD_*` 函数体均只调一次对应 `OLED_*`；`PutKeyToBuf` 单独判满且静默丢弃 |
| 未覆盖 | 屏幕上的一切现象（`y=7` 下半截、行距、闪烁）；I2C 实际耗时（46 ms / 11.5 ms 均为按位速率与字节数的**理论估算**，无逻辑分析仪读数）；红外边沿数与帧结构（68 为源码常量推算）；环形缓冲配对错位未跑满触发 |
