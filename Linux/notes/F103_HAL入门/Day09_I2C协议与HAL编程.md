# Day9：I2C 协议与 HAL 编程

**验证状态**：I2C1 配置（100 kHz、7 位地址、PB6/PB7 `AF_OD`）、HAL 的 `CCR`/`TRISE` 算式与
本工程实际取值**已核对源码并手工复算**（`Core/Src/i2c.c:40-77`，`stm32f1xx_hal_i2c.c:525-547`，
`stm32f1xx_hal_i2c.h:657-664`）；两处驱动地址常量（`0x78`、`0xD0`）与两种 `Mem_*` 用法
**已逐函数核对**（`driver_oled.c:126-176`、`driver_mpu6050.c:67-104`）；
**I2C 从未在真板上跑过**（本机无板，OLED 与 MPU6050 均未接）。教材侧内容来自
《HAL快速入门与项目实战》第 8.1–8.4 节。

---

## 学习目标

学完本篇应能用代码证据回答：

1. 教材 8.1.3 的四层软件层次，分别对应本工程的哪几个文件。
2. I2C 只用两根线，靠什么保证"多设备共享不打架"，以及本工程的开漏为什么可以不配上拉。
3. `S`/`P`/`ACK` 三个信号的电平条件，以及"数据只能在 SCL 低电平期间变化"这条约束的来源。
4. `0x78` 与 `0xD0` 是 7 位地址还是 8 位地址，为什么必须左移一位。
5. 教材 8.3.2 那五步配置，在本工程的 `CCR`/`TRISE`/`CR2` 里各是什么数值。
6. `HAL_I2C_Mem_Write` 里的 `MemAddress` 在 OLED 驱动与 MPU6050 驱动里是不是同一个东西。

---

## 环境与前置条件

| 项 | 要求 |
| --- | --- |
| 工程 | `experiments/F103_HAL_DriverLab/` |
| 参考 | `Core/Src/i2c.c`、`Drivers/DshanMCU-F103/driver_oled.c`、`driver_mpu6050.c` |
| 参考 HAL | `stm32f1xx_hal_i2c.c` 的 `HAL_I2C_Init`（`CCR`/`TRISE` 计算） |
| 硬件 | 可选。读屏/读陀螺仪需真板 |
| 前置 | Day4（开漏输出为什么必须上拉）、Day8（`_IT` 系列在本工程的缺口） |

---

## 核心概念

### 1. 四层软件层次：教材那张图落到文件上

教材 8.1.3 把"在 OLED 上显示字符串"拆成四层：

| 层 | 教材描述 | 本工程对应 | 它知道什么 |
| --- | --- | --- | --- |
| L4 应用 | "在哪个位置、显示什么字符" | `IRReceiver_Test` / `Lab_Run` 的调用点 | 业务语义 |
| L3 字符/图片库 | "收集了字符点阵" | `ascii_font[][]`、`chinese_font[]` | 点阵数据 |
| L2 设备协议 | "先设置地址、再发送数据" | `OLED_SetPosition`、`OLED_WriteCmd/WriteNBytes` | SSD1306 的页/列/控制字节 |
| L1 控制器驱动 | "我可以发送 I2C 数据" | `HAL_I2C_Master_Transmit` / `Mem_Write` | 只认字节流 |

这个分层的价值在于**每层只回答一个问题**。L2 不知道字模是什么，L3 不知道 I2C 是什么，
L1 不知道屏幕是什么。换屏（SSD1306 → ST7789）只改 L2；换总线（I2C → SPI）改 L1 与 L2 的接线。

**反例**：`driver_oled.c` 里 `OLED_PutChar` 同时做了 L2（设页/列地址）和 L3（取字模）——
它把两层合在一起了。这是教程代码的常见形态，不是错误，代价是换屏时要连字模逻辑一起改。

### 2. 两根线怎么做到多设备共享：开漏 + 仲裁

I2C 只有 `SCL`（时钟）与 `SDA`（数据），都必须是**开漏**。Day4 §1 已经说过开漏的两个特征：
只能主动拉低、不能主动拉高，高电平靠上拉电阻。放到总线上就是：

```text
        VCC
         │  上拉电阻（模块板上，典型 4.7 kΩ）
         │
   ┌─────┴─────┬─────────┬─────────┐
   │           │         │         │
 MCU SDA   设备A SDA  设备B SDA  ...   ← 每个都是开漏
```

**任何一方拉低，总线就是低**；所有设备都松手，上拉才把它拉回高。这一条同时解决两个问题：

1. **不打架**：两个设备同时拉低不会短路（都是拉到地）。
2. **可仲裁**：A 想发 1（松手）而 B 发 0（拉低），A 读回总线是 0 —— A 立刻知道"我输了"，
   主动退出。这就是教材 8.2.1 那条"多主机、冲突检测与仲裁"的物理基础。

推论：**"拉高"不是任何一个设备做的**，是上拉电阻做的。所以 I2C 线上"读到高电平"其实是
"没人拉低"。这也是 Day4 那条"开漏不配上拉就只剩低电平"的完整解释。

### 3. `S`/`P`/`ACK`：三个靠"违反常规"定义的信号

教材 8.2.3 的三个定义，关键都在**谁在什么时候变**：

| 信号 | 条件 | 记忆点 |
| --- | --- | --- |
| `S`（起始） | **SCL 高**时，SDA 由高→低 | 唯一允许 SCL 高时 SDA 下降的时刻 |
| `P`（结束） | **SCL 高**时，SDA 由低→高 | 唯一允许 SCL 高时 SDA 上升的时刻 |
| `ACK` | 第 9 个 SCL 周期，接收方**拉低 SDA** | 拉低 = 收到，不拉低 = 没收到 |

而普通数据位的规则是（教材 8.2.3 末句）：**"SDA 上的数据必须在 SCL 为高电平期间保持稳定，
SDA 上的数据只能在 SCL 为低电平期间变化。"**

三条规则合起来才有意义：**因为数据只能在 SCL 低时变，所以 SCL 高时的跳变一定是信号**。
`S` 和 `P` 的定义不是任意的，是"从数据规则里挖出来的两个例外"。

这解释了一类真实故障：**总线被拉低后没释放（SDA 卡在 0），主机会误判"另外有人在发数据"**，
于是发不出 `S`。HAL 的头文件注释里专门列了这个硅限制（见 §8）。

### 4. 地址：7 位 + 1 位方向，所以常量都要左移

教材 8.2.4：一帧开始时主机发 8 位，**前 7 位是从机地址，第 8 位是方向（0 写 / 1 读）**。

所以 I2C 里的"设备地址"有两种写法，混淆是最常见的一类 bug：

| 写法 | 值 | 说明 |
| --- | --- | --- |
| 7 位地址（数据手册写的） | SSD1306 `0x3C`、MPU6050 `0x68` | 只有 7 位 |
| 8 位地址（HAL 参数要的） | `0x78`、`0xD0` | 7 位左移 1 位，最低位留给方向 |

本工程两处常量正好各一个（源码核对）：

```c
#define OELD_I2C_ADDR  0x78    /* driver_oled.c:126  —— 注意源码拼写是 OELD，不是 OLED */
#define MPU6050_I2C_ADDR 0xD0  /* driver_mpu6050.c:66 */
```

验算：`0x3C << 1 = 0x78`、`0x68 << 1 = 0xD0`，最低位都是 0（写方向）。
**HAL 内部会自己把最低位改成方向位**，所以传 `0x78` 时读操作实际发的是 `0x79`。
传错（比如直接传 `0x3C`）不会报错，只会得到"设备不应答"——因为 `0x3C` 左移一位的地址
是 `0x78` 的一半，即**另一个地址**。

教材 8.4.3 参数表里那句"设备地址，需要把 7 位地址左移 1 位"就是这个意思。

### 5. 五个寄存器与本工程的取值

教材 8.3.2 给了五步配置。逐项落到本工程（`Core/Src/i2c.c:40-48`）：

```c
hi2c1.Instance         = I2C1;
hi2c1.Init.ClockSpeed  = 100000;              /* 标准模式 100 kHz */
hi2c1.Init.DutyCycle   = I2C_DUTYCYCLE_2;     /* 标准模式下不用，快速模式才有意义 */
hi2c1.Init.OwnAddress1 = 0;                   /* 只做主机，不用自己的从机地址 */
hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
hi2c1.Init.NoStretchMode  = I2C_NOSTRETCH_DISABLE;   /* 允许从机拉低 SCL 拖住主机 */
```

HAL 在 `HAL_I2C_Init` 里把它换算成三个寄存器（`stm32f1xx_hal_i2c.c:525-547`）：

```text
pclk1  = HAL_RCC_GetPCLK1Freq() = 36 MHz        （APB1 = 72/2，Day7 §4 已确认）

CR2.FREQ = I2C_FREQRANGE(pclk1) = pclk1 / 1e6 = 36

TRISE    = I2C_RISE_TIME(36, 100000) = 36 + 1 = 37      （≤100 kHz 走 freqrange+1）

CCR      = I2C_CCR_CALCULATION(36e6, 100e3, 2)
         = ((36 000 000 - 1) / (100 000 × 2)) + 1
         = 179 + 1 = 180 = 0xB4
```

三条都能自己验：

- `CCR = 180` 意味着 `T_high = 180 × (1/36 MHz) = 5 µs`，`DutyCycle_2` 下 `T_low` 也是 5 µs，
  周期 10 µs = **100 kHz**。对上了。
- `TRISE` 是"SCL 上升沿允许的最长时间"，标准模式规范给 1000 ns；以 `1/36 MHz ≈ 27.8 ns`
  为单位就是 36，`+1` 是取整余量。快速模式公式不一样（`freqrange × 300 / 1000 + 1`），
  因为规范上限从 1000 ns 变成 300 ns。
- 若把 `ClockSpeed` 改成 400000，`CCR` 会走快速模式分支并置 `CCR.FS` 位——**同一个寄存器，
  两条完全不同的算法**。

**注意 `DutyCycle` 在 100 kHz 下是死参数**：标准模式 `T_high = T_low` 是规范定的，
HAL 的 `I2C_SPEED_STANDARD` 宏里根本没有 `__DUTYCYCLE__`。CubeMX 默认填 `DUTYCYCLE_2`
只是占位。

### 6. Master 与 Mem：同一个 API 家族，两种语义

教材 8.4.3 说"**MEM 模式只是软件上的概念**，仍然是作为 Master 读写数据"。这句话的要害在于：
`Mem_*` 只是帮你"先发一个地址字节，再发/收数据"，硬件层面和 `Master_*` 完全一样。

但本工程两个驱动用 `Mem_*` 的方式**语义完全不同**：

| 驱动 | 调用 | `MemAddress` 到底是什么 |
| --- | --- | --- |
| MPU6050 | `HAL_I2C_Mem_Read(..., reg, 1, pdata, 1, ...)` | **真寄存器地址**（如 `0x75` = `WHO_AM_I`） |
| OLED | `HAL_I2C_Mem_Write(..., 0x40, 1, buf, length, ...)` | **`0x40` 是控制字节**，SSD1306 没有寄存器地址 |

OLED 这一处是把 `Mem_*` 当成"自动前置一个字节"的便利工具用了。总线上的字节序列是一样的：

```text
写寄存器（MPU6050）：[地址 0xD0] [reg] [data...]
写显存（OLED）：     [地址 0x78] [0x40] [GDDRAM 数据...]
                     ↑ 同一个位置，一个是寄存器号，一个是控制字节
```

**HAL 分不清这两者，因为它只管发字节。** 这就是"L1 不知道屏幕是什么"的具体表现。

反过来说，MPU6050 的**写**寄存器用了 `Master_Transmit` 而不是 `Mem_Write`（`driver_mpu6050.c:82-89`）：

```c
tmpbuf[0] = reg;
tmpbuf[1] = data;
return HAL_I2C_Master_Transmit(g_pHI2C_MPU6050, MPU6050_I2C_ADDR, tmpbuf, 2, MPU6050_TIMEOUT);
```

这正是教材 8.4.3 说的"需要在数据 buffer 里填入 addr 和 val，**不太直观**"那个情形。
同一个驱动的读用 `Mem_Read`、写用 `Master_Transmit`，风格不统一——**能工作，但读的人要多想一步**。

### 7. 控制字节：0x00 与 0x40 的差别

教材 8.5.3 讲了 SSD1306 的 Control byte：

```text
bit7  Co  = 0   后面传输的只有 Data byte（不跟新的控制字节）
bit6  D/C#= 0   命令
bit6  D/C#= 1   数据（写入 GDDRAM）
```

本工程两处正好对应（源码核对）：

```c
tmpbuf[0] = 0;     /* 0x00: Co=0, D/C#=0 → 命令 */   OLED_WriteCmd
tmpbuf[0] = 0x40;  /* 0x40: Co=0, D/C#=1 → 数据 */   OLED_WriteData / WriteNBytes
```

**把这两个常量搞反，现象是"屏幕没反应"或"花屏"**，而且不会有任何错误返回——
`OLED_WriteCmd` 的返回值在 `OLED_Init` 里被全部丢弃（Day6 §4）。

### 8. F1 的硅限制：HAL 头文件里已经列了

`stm32f1xx_hal_i2c.c` 的文件头注释（`:293-310`）列了 F1 上 HAL 已实现规避的硅缺陷。三条与日常编程直接相关：

| 硅限制 | HAL 的规避方式 | 对写代码的影响 |
| --- | --- | --- |
| "Wrong data read into data register" | 轮询/中断模式内部处理 | 单字节 `Master_Receive` 曾不可靠 |
| "Start cannot be generated after a misplaced Stop" | —— | 总线卡死后要复位 I2C（`SWRST`） |
| "Some software events must be managed before the current byte is being transferred" | 建议用 DMA；中断模式必须给 I2C **最高优先级** | 中断方式下 I2C 优先级配低会丢事件 |
| "Mismatch on the Setup time for a repeated Start" | **把频率降到 88 kHz，或用快速模式** | 100 kHz 恰好在临界附近 |

最后一条值得单独说：本工程用的是 **100 kHz**，而 HAL 注释的建议是"降到 88 kHz"。
这不是笔误——`0x78` 那类老型号在 100 kHz 下重复起始时序会略超规范。**这条我只引用 HAL 注释，
未在真板上测量过**（本机没有逻辑分析仪）。真要上板验证，判据是"`HAL_I2C_Master_Receive`
偶发 `HAL_ERROR` 且 `ErrorCode == HAL_I2C_ERROR_AF`"。

顺带说 §8 第三条的直接后果：**本工程 `i2c.c` 里没有 `HAL_NVIC_*`**，
`stm32f1xx_it.c` 里也没有 `I2C1_EV_IRQHandler`（`grep -c I2C` = 0）。
启动文件给它的仍然是（`startup/startup_stm32f103xb.s:328-332`）：

```asm
  .weak I2C1_EV_IRQHandler
  .thumb_set I2C1_EV_IRQHandler,Default_Handler
  .weak I2C1_ER_IRQHandler
  .thumb_set I2C1_ER_IRQHandler,Default_Handler
```

这与 Day8 §6 的 USART 缺口是**同一个形态**：调 `HAL_I2C_Master_Transmit_IT` 得到 `HAL_OK`，
第一次事件中断就进 `b Infinite_Loop`。教材 8.4.4/8.4.5 的中断方式与 DMA 方式，
在本工程**一行都没落地**。

### 9. 时间都花在哪：一次 2 字节传输多久

100 kHz 下每个 SCL 周期 10 µs，每个字节 9 位（8 数据 + 1 ACK）：

```text
OLED_WriteCmd 一次 = 地址 1 字节 + 数据 2 字节 = 3 字节 = 27 个 SCL ≈ 270 µs
OLED_WriteNBytes(8) = 地址 + 控制字节 0x40 + 8 数据 = 10 字节 = 90 个 SCL ≈ 900 µs
```

这在 Day6 §3 的位速率估算里已经出现过（那里按 bit 算，结论一致：一行 16 字符 ≈ 46 ms）。
这里补的是**更精确的算法**：按"字节 × 9 个周期"数，比按 bit 估算更容易复核。

**一个 `OLED_WriteCmd` 270 µs** 这个数字解释了两件事：

1. `OLED_Init` 的 11 步序列（含多条双命令）总耗时在毫秒量级，上电后不能指望"立刻能显示"。
2. `OLED_TIMEOUT = 500` ms 是单次传输超时，等于名义耗时的 **1800 倍**——
   与 Day7 的 `_write` 同样的思路：让正常传输永不超时，超时只代表硬件坏了。

---

## 系统调用/接口卡片

### `HAL_StatusTypeDef HAL_I2C_Init(I2C_HandleTypeDef *hi2c)`

作用：按 `Init` 结构体算出 `CR2.FREQ` / `TRISE` / `CCR` / `CR1`，并调 `MspInit` 配引脚与时钟。

参数：`hi2c->Init.ClockSpeed`、`DutyCycle`、`AddressingMode`、`NoStretchMode` 等。

返回值和错误：`HAL_OK` / `HAL_ERROR`（`I2C_MIN_PCLK_FREQ` 检查不过返回错误——
PCLK1 相对目标速率太低时无法分频）。`Error_Handler()` 在 `MX_I2C1_Init` 里被调。

资源所有权：内部先 `__HAL_I2C_DISABLE` 再 `SWRST` 复位一次，然后重新配置。

常见误用：`ClockSpeed` 设成 400000 却忘了 `DutyCycle` 的意义；或 PCLK1 被改小后没重新算。

### `HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout)`

作用：主机发送 `Size` 字节。序列：`S` → `[DevAddress|W]` → 逐字节 + `ACK` → `P`。

参数：`DevAddress` 是 **8 位形式**（7 位左移 1 位）；`Timeout` 单位 ms。

返回值和错误：`HAL_OK` / `HAL_ERROR`（含 `AF` 无应答）/ `HAL_BUSY` / `HAL_TIMEOUT`。
"设备不存在"表现为 `HAL_ERROR` + `hi2c->ErrorCode == HAL_I2C_ERROR_AF`，**不是超时**。

线程/进程安全：不可重入，靠 `hi2c->State` 互斥。

最小示例：

```c
uint8_t b[2] = { 0x00, 0xAE };   /* 控制字节 + 命令 */
HAL_I2C_Master_Transmit(&hi2c1, 0x78, b, 2, 500);
```

常见误用：传 7 位地址；忽略返回值；在 ISR 里调（270 µs 起）。

### `HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout)`

作用：先在地址后发 `MemAddress`（1 或 2 字节），再发 `Size` 字节数据。

参数：`MemAddSize` 取 `I2C_MEMADD_SIZE_8BIT`（发低字节）或 `_16BIT`（**先高后低**）。

返回值和错误：同 `Master_Transmit`。

常见误用：把"控制字节"当 `MemAddress` 却不自知（OLED 的做法，见 §6）；
`MemAddSize` 传 1/2 之外的数（会走错分支）。

### `HAL_StatusTypeDef HAL_I2C_Mem_Read(...)`

作用：`S` → `[DevAddress|W]` → `MemAddress` → **重复起始** → `[DevAddress|R]` → 读 `Size` 字节。

参数与返回同 `Mem_Write`。

常见误用：以为 `Mem_Read` 是"一次传输"。**它在总线上是两次传输 + 一个重复起始**，
所以耗时约等于两次 `Master_Transmit`。这也是 F1 那条"重复起始时序"硅限制的来源（§8）。

### `HAL_StatusTypeDef HAL_I2C_IsDeviceReady(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint32_t Trials, uint32_t Timeout)`

作用：只发地址、看有没有 `ACK`。返回值就能回答"设备在不在"。

返回值和错误：`HAL_OK`（有应答）/ `HAL_ERROR` / `HAL_BUSY` / `HAL_TIMEOUT`。

常见误用：**本工程没用它**。这是排查"屏幕全黑"最快的入口——比 `OLED_WriteCmd` 的返回值
可靠得多，因为它不动显存、也不依赖驱动逻辑。

---

## 最小可运行实验

目标：不接屏幕，把 §5 的寄存器值从源码推出来，并确认中断路径的缺口。

```bash
cd experiments/F103_HAL_DriverLab

# 1) 速率与引脚（预期 100000 / AF_OD / PB6,PB7；无 Pull 赋值）
grep -n "ClockSpeed\|DutyCycle\|AddressingMode\|NoStretchMode" Core/Src/i2c.c
sed -n '69,80p' Core/Src/i2c.c

# 2) 复算 CCR/TRISE 的两个宏（I2C_CCR_CALCULATION / I2C_RISE_TIME）
grep -n "I2C_CCR_CALCULATION\|I2C_RISE_TIME\|I2C_FREQRANGE" \
     Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_i2c.h

# 3) 中断入口是否落地（预期：it.c 无 I2C，启动文件里是 weak → Default_Handler）
grep -c "I2C" Core/Src/stm32f1xx_it.c
grep -n "I2C1_EV_IRQHandler" startup/startup_stm32f103xb.s
```

第 2 步之后可以手算验证：`((36000000-1)/(100000*2))+1 = 180`、`36+1 = 37`。

---

## 编译与运行

```bash
cmake --build build
```

真板上的运行需要 OLED（`LAB_TEST` 对应编号）或 MPU6050 模块，本机未做。
**排查入口建议先用 `HAL_I2C_IsDeviceReady(&hi2c1, 0x78, 3, 100)`**：返回 `HAL_OK`
说明接线与地址都对，再去看驱动逻辑。

---

## 结果观察

**配置读数**（`Core/Src/i2c.c`）：

```text
I2C1  100 000 Hz  7-bit  DutyCycle_2  NoStretch DISABLE
PB6 = SCL, PB7 = SDA,  GPIO_MODE_AF_OD, GPIO_SPEED_FREQ_HIGH
时钟：__HAL_RCC_GPIOB_CLK_ENABLE() + __HAL_RCC_I2C1_CLK_ENABLE()
Pull：未赋值（结构体 {0}）⇒ GPIO_NOPULL
```

**派生寄存器值**（按 HAL 宏手算，未读回寄存器）：

| 寄存器 | 字段 | 值 | 算式 |
| --- | --- | --- | --- |
| `CR2` | `FREQ` | 36 | `PCLK1 / 1e6` |
| `TRISE` | `TRISE` | 37 (0x25) | `36 + 1`（标准模式） |
| `CCR` | `CCR` | 180 (0xB4) | `((36e6-1)/(100e3×2)) + 1` |
| `CCR` | `FS` | 0 | 标准模式不置位 |

**两处地址与两种 `Mem_*` 语义**（源码核对）：

```text
OLED    0x78 = 0x3C<<1    Mem_Write(memaddr 0x40, size 1)   ← 0x40 是控制字节
MPU6050 0xD0 = 0x68<<1    Mem_Read (memaddr reg,  size 1)   ← reg 是真寄存器地址
MPU6050 写寄存器走 Master_Transmit({reg, data}, 2)          ← 教材说的"不太直观"那种
```

**`Pull` 未赋值的含义**（`i2c.c:74-76`）：`GPIO_InitStruct` 只赋了 `Pin`/`Mode`/`Speed`，
`Pull` 靠行首 `= {0}` 变成 `GPIO_NOPULL`。**开漏 + 无内部上拉，等于完全依赖模块板上的
上拉电阻**。OLED 与 MPU6050 模块普遍自带 4.7 kΩ 上拉，所以能工作；换一块裸芯片就没有上拉了。
这条与 Day4 §1 是同一件事的两种后果：那里是"输出拉不高"，这里是"整个总线浮空"。

**中断路径缺口**：

| 项 | 状态 | 后果 |
| --- | --- | --- |
| `I2C1_EV_IRQHandler` / `I2C1_ER_IRQHandler` | **不存在**（weak → `Default_Handler`） | `_IT`/`_DMA` 一有事件即进 `b Infinite_Loop` |
| `HAL_NVIC_EnableIRQ(I2C1_EV_IRQn)` | 不存在 | 同上 |
| DMA 通道（I2C1_RX/TX） | 未配 | `_DMA` 拿 `NULL` 句柄走不通 |

---

## 常见错误和陷阱

| 陷阱 | 症状 | 根因 | 处理 |
| --- | --- | --- | --- |
| 传 7 位地址给 HAL | 设备永远不应答 | HAL 要 8 位形式 | 左移 1 位（`0x3C`→`0x78`） |
| 控制字节 0x00/0x40 搞反 | 屏幕无反应或花屏 | Co/D/C# 位含义 | 命令 0x00、数据 0x40 |
| 开漏却没上拉 | 总线恒低、读不到高电平 | 开漏不能主动拉高 | 模块上拉，或配 `GPIO_PULLUP` |
| 100 kHz 下偶发重复起始错误 | `HAL_ERROR` + `AF` | F1 硅限制（HAL 注释） | 降到 88 kHz 或改快速模式 |
| 用 `_IT` 但没写 `I2C1_EV_IRQHandler` | 一有事件整机停住 | weak 落 `Default_Handler` | 补 ISR + NVIC |
| 忽略 `Mem_*` 返回值 | 屏黑但"初始化成功" | 返回值被丢弃 | 至少首帧判一次 |
| 把 OLED 的 `0x40` 当寄存器号解释 | 改驱动时找不到对应表 | 它是控制字节 | 查 SSD1306 数据手册第 8 章 |
| 在 ISR 里调 I2C 传输 | 中断拖长几百 µs | 单次 ≈ 270 µs 起 | 只置标志，主循环做 |
| 中断模式给 I2C 低优先级 | 偶发丢事件 | F1 硅限制（HAL 注释建议最高优先级） | 优先级配到最高档之一 |

---

## 故障排查方法

"屏不亮 / 读不到陀螺仪"按**从电气到协议**五步：

1. **量电平**。`SDA`/`SCL` 空闲时应为高（≈3.3 V）。为低 → 被谁拉住了，或没有上拉；
   为 1.6 V 左右 → 悬空。
2. **`HAL_I2C_IsDeviceReady(&hi2c1, addr, 3, 100)`**。`HAL_OK` 说明地址对、设备在、总线上拉正常，
   问题在驱动逻辑；`HAL_ERROR` 且 `ErrorCode == AF` 说明地址错或设备没供电。
3. **`I2C1->CCR` / `TRISE` / `CR2`**。按 §5 的算式反算。这里错了就是"能通信但不稳定"或完全不通。
4. **`GPIOB->CRH` 里 PB6/PB7 的 4 bit**。必须是复用开漏（`CNF=11`）。配成推挽时，
   两个设备一个要拉高一个要拉低就是**电气冲突**，可能损坏引脚。
5. **`I2C1->SR1`/`SR2` 的 `AF`/`BERR`/`ARLO`**。`AF` = 无应答（第 2 步已覆盖）；
   `BERR` = 起止信号错乱（典型的"上次传输没正常结束"）；`ARLO` = 仲裁丢失（多主机时才该出现）。

总线卡死（`SDA` 一直被拉低）的恢复办法是发 9 个 SCL 时钟让从机吐出剩余位，
或直接 `I2C1->CR1 |= I2C_CR1_SWRST` 复位外设——**`HAL_I2C_Init` 内部就做了这个 `SWRST`**，
所以"重新 `MX_I2C1_Init()` 一次"是一个有效的急救动作。

---

## 练习题

### 基础题

题目：SSD1306 的 7 位地址是 `0x3C`。写出 HAL 参数应传的值、读操作时线上实际发的 8 位值、
以及如果误传 `0x3C` 会打到哪个地址。

提示：`<<1`；最低位 0 = 写、1 = 读。

验收标准：参数 `0x78`；读操作线上 `0x79`；误传 `0x3C` 等价于 `0x3C << 1 = 0x78` 的**低位移入**
逻辑上的 `0x1E` 地址（即把 `0x3C` 当 8 位地址解析后得到 7 位地址 `0x1E`），不是屏幕上任何设备。

### 改错题

题目：下面的代码"编译通过、返回值被忽略、屏幕全黑"，指出至少三处问题。

```c
static int oled_cmd(uint8_t cmd)
{
    uint8_t b[2] = { 0x40, cmd };
    return HAL_I2C_Master_Transmit(&hi2c1, 0x3C, b, 2, 10);
}
```

提示：控制字节（§7）、地址形式（§4）、超时（§9）。

验收标准：能指出 ① `0x40` 应为 `0x00`（命令），② 地址应为 `0x78`，
③ 超时 10 ms 相对 270 µs 名义耗时余量偏小（本工程用 500）；
并能说明三处各自的现象为什么都是"黑屏"而无法区分。

### 综合题

题目：若把 `ClockSpeed` 从 100000 改成 400000，算出新的 `CCR`、`TRISE`，说明 `CCR.FS` 与
`DUTYCYCLE_2` 分别起什么作用，并解释为什么这时 `DutyCycle` 不再是死参数。

提示：快速模式 `I2C_SPEED_FAST` 用系数 3（`DUTYCYCLE_2`）或 25（`16_9` 并置 `I2C_DUTYCYCLE_16_9`）；
`TRISE` 走 `freqrange × 300 / 1000 + 1`。

验收标准：`CCR` 约 `(36e6-1)/(400e3×3)+1 = 30`、`TRISE = 36×300/1000+1 = 11`、
`FS` 置位；能说明 `DUTYCYCLE_2` 给 `T_low:T_high = 2:1`、`16_9` 给 `9:16`，
从而影响"同一 CCR 下实际能跑多快"。

---

## 验收标准

- [ ] 能说出教材四层层次分别对应本工程的哪几个文件，并指出 `OLED_PutChar` 合并了哪两层。
- [ ] 能解释开漏 + 上拉如何同时实现"共享不打架"与"冲突仲裁"。
- [ ] 能写出 `S`/`P`/`ACK` 的电平条件，并说明它们为什么是"数据规则的两个例外"。
- [ ] 能把 `0x3C`/`0x68` 与 `0x78`/`0xD0` 对应起来，并说出误传 7 位地址的后果。
- [ ] 能独立算出 `CR2.FREQ=36`、`TRISE=37`、`CCR=180`，并反推实际 SCL 频率为 100 kHz。
- [ ] 能区分 OLED 的 `0x40`（控制字节）与 MPU6050 的 `reg`（寄存器地址），说明 HAL 为何分不清。
- [ ] 能指出本工程 `_IT`/`_DMA` 路径的缺口，并说明与 Day8 的 USART 缺口是同一形态。

## 延伸阅读

- 教材：《HAL快速入门与项目实战(基于DshanMCU-103)》第 8.1 节（I2C 程序层次）、
  第 8.2 节（协议：信号类型、传输格式）、第 8.3 节（STM32 I2C 硬件与五步配置）、
  第 8.4 节（HAL 编程：查询/中断/DMA 三组函数）
- HAL 源码：`Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_i2c.c:505-560`（`HAL_I2C_Init`
  的寄存器计算）、`:293-310`（F1 硅限制清单）
- 宏定义：`Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_i2c.h:657-664`
- 配置：`Core/Src/i2c.c`（`MX_I2C1_Init` + `HAL_I2C_MspInit`）
- 驱动：`Drivers/DshanMCU-F103/driver_oled.c:126-176`（控制字节与 `Mem_Write` 的误用）、
  `driver_mpu6050.c:63-104`（真寄存器地址与 `Master_Transmit` 写法）
- 上一篇：`Linux/notes/F103_HAL入门/Day08_UART_IDLE中断与stdio重定向.md`
- 下一篇：`Linux/notes/F103_HAL入门/Day10_OLED显示汉字与点阵.md`

---

**验证记录**

| 项目 | 内容 |
| --- | --- |
| 日期 | 2026-10-29 |
| 环境一 | Windows 11 + Git Bash；`grep`/`sed` 核对配置与 HAL 宏 |
| 命令一 | `cat Core/Src/i2c.c`；`sed -n '505,560p' stm32f1xx_hal_i2c.c`；`sed -n '655,665p' stm32f1xx_hal_i2c.h` |
| 结果一 | `ClockSpeed=100000`、`DutyCycle_2`、7-bit、`NoStretch DISABLE`；PB6/PB7 `AF_OD`+`HIGH`，`Pull` 未赋值；HAL 用 `I2C_FREQRANGE`/`I2C_RISE_TIME`/`I2C_CCR_CALCULATION` 三个宏算出 `FREQ=36`、`TRISE=37`、`CCR=180` |
| 环境二 | 同上 |
| 命令二 | `sed -n '120,180p' driver_oled.c`；`sed -n '60,110p' driver_mpu6050.c`；`grep -n "I2C1_EV_IRQHandler" startup/startup_stm32f103xb.s`；`grep -c I2C Core/Src/stm32f1xx_it.c` |
| 结果二 | OLED `0x78` + 控制字节 `0x00`/`0x40` + `Mem_Write(memaddr 0x40, size 1)`；MPU6050 `0xD0`、`Mem_Read(reg, 1)`、写寄存器走 `Master_Transmit({reg,data},2)`；`I2C1_EV/ER_IRQHandler` 均为 `.weak` → `Default_Handler`；`it.c` 中 I2C 出现次数为 0 |
| 未覆盖 | **I2C 总线从未在真板上通信**（OLED/MPU6050 均未接）；`CCR=180`/`TRISE=37` 为按 HAL 宏**手算**，未读回寄存器核对；单次 `OLED_WriteCmd ≈ 270 µs` 为按时钟周期推算，无逻辑分析仪读数；HAL 注释里的 F1 硅限制（88 kHz 建议、重复起始时序）**仅引用文档，未复现**；`HAL_I2C_IsDeviceReady`、中断/DMA 路径均未跑通（本工程无对应 ISR 与 DMA 配置） |
