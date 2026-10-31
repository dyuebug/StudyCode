# Day11：SPI 协议与 HAL 编程

**验证状态**：教材 9.1–9.4 的层次划分、四线定义、四种模式、HAL 四组 API **已核对教材原文**
（`ch09_spi.txt` 全文 242 行）；`Core/Src/spi.c` 的 11 个初始化字段、`HAL_SPI_MspInit` 的
引脚模式**逐行核对源码**；`HAL_SPI_Init` 把字段拼成 `CR1` 的写法与位定义**已核对**
（`stm32f1xx_hal_spi.c:90-101`、`stm32f103xb.h:9086-9131`），并用主机端程序复刻出
`CR1=0x030C/0x034C`、`CR2=0x0000`（带 `assert` 自校验）；分频/耗时表与"只收不发"的语义
为**主机端实测输出**（`lab_spi/lab_spi_cr1.out`）；`LAB_TEST=17` 的体积与符号表为**实测**。
**SPI 从未上板**：没有波形、没有读到过 W25Q64 的 ID、没有验证过模式与时序。

---

## 学习目标

学完本篇应能用代码证据回答：

1. SPI 的四根线各管什么，为什么它没有"设备地址"却仍能挂多个从机。
2. 四种模式只在 `CPOL`/`CPHA` 两位上不同，哪两个模式的**采样沿是同一个**，为什么 W25Q64 两种都能用。
3. 为什么"只读"在物理上不成立——`HAL_SPI_Receive` 具体往 MOSI 上发了什么。
4. CubeMX 的 11 个字段怎么变成 `CR1` 的一个 32 位数；`SSI` 为什么必须为 1。
5. `MISO` 为什么被配成普通输入（`GPIO_MODE_INPUT`）而不是复用推挽，与 I2C 的 `AF_OD` 差在哪。
6. 18 MHz 是怎么算出来的；一次 4 KB 读需要多少个 SCK 周期、多长时间。

---

## 环境与前置条件

| 项 | 要求 |
| --- | --- |
| 工程 | `experiments/F103_HAL_DriverLab/` |
| 参考 | `Core/Src/spi.c`、`Drivers/DshanMCU-F103/driver_spiflash_w25q64.c` |
| 主机验证 | `lab_spi/lab_spi_cr1.c`（宿主 gcc，不链接 HAL） |
| 硬件 | 可选。真机验证需 W25Q64 模块插在 M12（与 M6 超声波抢 PB9） |
| 前置 | Day02/04（GPIO 模式）、Day09（I2C 对照、`Mem_*` 语义）、Day10（外部字库的需求） |

---

## 核心概念

### 1. 四层结构：谁决定"发什么"、谁只管"怎么发"

教材 9.1.3 把操作 SPI Flash 的程序分成三层，加上最底下的硬件是四层：

```text
L4 应用      在哪个地址、读多长          W25Q64_Test()            ← 决定"做什么"
L3 设备协议  0x03 + 24 位地址 → 数据     W25Q64_Read()            ← 知道 Flash 要什么格式
L2 控制器    SCK/CPOL/CPHA/CS 怎么动     HAL_SPI_TransmitReceive() ← 只会发字节
L1 硬件      SPI1 外设 + 引脚            PA5/PA6/PA7/PB9
```

这个分层不是教学装饰，它决定改需求时动哪一层：

| 需求变化 | 要动的层 | 不该动的层 |
| --- | --- | --- |
| 换一颗 SPI Flash（命令不同） | L4 的读法、L3 整个换 | L2、L1 不动 |
| 提高 SPI 时钟 | L2 一个分频字段 | L3、L4 不动 |
| 换到 SPI2 通道 | L2 句柄 + L1 引脚 | L3、L4 不动 |

`W25Q64_Read` 里那句注释"自己实现 SPI Flash 的读操作"就是这个意思：**协议在 L3，不在 HAL**。
HAL 不认识 `0x03`，它只认"发 4 个字节、收 4096 个字节"。

### 2. 四根线，片选代替地址

教材 9.2.1：MOSI（主出从入）、MISO（主入从出）、SCLK（时钟）、CS（片选）。

```text
              ┌─────────────┐
      MOSI ──▶│             │
      MISO ◀──│   W25Q64    │
      SCLK ──▶│             │
      CS   ──▶│ (低有效)    │
              └─────────────┘
多从机时：每个从机各占一根 CS，SCLK/MOSI/MISO 三根共用。
```

与 I2C 的对照（教材 9.2.1 的表，补了"为什么"一列）：

| | SPI | I2C | 为什么 |
| --- | --- | --- | --- |
| 通信方式 | 同步串行**全双工** | 同步串行半双工 | SPI 收发各一根数据线 |
| 速度 | 教材写"一般 50 MHz 以下" | 100 k/400 k/3.4 M | 开漏上拉限制 I2C 上升沿 |
| 从机选择 | **引脚片选** | **设备地址** | SPI 无地址字段 |
| 总线线数 | 4（每多一个从机多一根 CS） | 2（地址不冲突就够） | 引脚换地址 |
| 应答 | 无 | 每字节 ACK/NACK | SPI 没有第 9 个时钟 |

**"没有应答"是最容易被忽略的一条**：SPI 写出去的数据，从机收没收到、对不对，主机无从得知。
所以 SPI 设备通常自证：Flash 读 ID、传感器读配置寄存器回读。`W25Q64_WaitReady` 反复读状态寄存器
也是一种自证——它确认的是"芯片内部操作做完了"，不是"我的字节被正确接收了"。

### 3. 全双工的本质：移位寄存器同时进、同时出

教材 9.2.2 那句"主机发送 N 字节给从机时，必定能接收到 N 字节"来自硬件结构：

```text
主机 shift register          从机 shift register
   ┌──────────────┐   MOSI   ┌──────────────┐
   │ bit7 ... bit0│ ───────▶ │ bit7 ... bit0│
   │      ▲       │          │      ▲       │
   │      │SCLK   │   MISO   │      │       │
   │ bit0 ... bit7│ ◀─────── │ bit0 ... bit7│
   └──────────────┘          └──────────────┘
每个 SCLK 周期：两边各左移一位，一个 bit 出、一个 bit 进。
```

推论有两条，都很实用：

1. **想收就得先发**。不存在"只接收"的物理通路，主机必须给出 SCLK，而 SCLK 只有在移位时才产生。
2. **收到的 N 字节有没有意义由从机决定**。写操作时主机忽略收到的字节即可。

第 1 条在 HAL 里有一个非常具体、可核对的实现（本篇最小实验段四会复现）：

```c
/* stm32f1xx_hal_spi.c:977-982 */
if ((hspi->Init.Mode == SPI_MODE_MASTER) && (hspi->Init.Direction == SPI_DIRECTION_2LINES))
{
  hspi->State = HAL_SPI_STATE_BUSY_RX;
  /* Call transmit-receive function to send Dummy data on Tx line and generate clock on CLK line */
  return HAL_SPI_TransmitReceive(hspi, pData, pData, Size, Timeout);
}
```

注意 `pData` 出现了两次：**发送缓冲和接收缓冲是同一个指针**。所谓"Dummy data"不是固定的 `0xFF`，
而是**你缓冲区里原来的内容**。所以 `W25Q64_Rx` 不准备发送内容也能工作（Flash 此时忽略 MOSI），
但缓冲区如果是未初始化栈内存，发出去的就是垃圾——对 Flash 无害，对某些"边收边判命令"的从机就可能有害。

### 4. 四种模式：只在两位上不同

教材 9.2.2 给出 `CPOL`（空闲电平）与 `CPHA`（第几个边沿采样）的组合：

| 模式 | CPOL | CPHA | 空闲电平 | 采样沿 | 移位沿 |
| --- | --- | --- | --- | --- | --- |
| 0 | 0 | 0 | 低 | **上升沿**（首沿） | 下降沿 |
| 1 | 0 | 1 | 低 | 下降沿（次沿） | 上升沿 |
| 2 | 1 | 0 | 高 | 下降沿（首沿） | 上升沿 |
| 3 | 1 | 1 | 高 | **上升沿**（次沿） | 下降沿 |

**Mode 0 与 Mode 3 的采样沿都是上升沿**，区别只在空闲电平（低/高）。这就是为什么一颗 Flash
既支持 Mode 0 又支持 Mode 3 而不必改时序逻辑——采样时刻一样，只是 CS 拉低后第一个时钟沿前的
电平不同。本工程的 `CPOL=LOW` + `CPHA=1EDGE` 就是 **Mode 0**（下面 §5 的 `CR1` 读数可证）。

教材用 `0x56 = 0b0101_0110` 举例（Mode 3）：从机在**上升沿**读 DO 上的电平。把这句话和上表对上，
就知道"为什么先发高位"（`FirstBit = MSB`）也是必须显式配置的一项。

### 5. 11 个字段怎么变成 `CR1`：一位一位对

`spi.c:40-52` 的配置，逐字段落到寄存器。宏值全部来自仓库内头文件，不是记忆：

| 字段 | 取值 | 宏定义 | 落到的位 |
| --- | --- | --- | --- |
| `Mode` | `SPI_MODE_MASTER` | `MSTR\|SSI`（`hal_spi.h:206`） | bit2 + **bit8** |
| `Direction` | `SPI_DIRECTION_2LINES` | `0x0`（`:214`） | 不置位 |
| `DataSize` | `SPI_DATASIZE_8BIT` | `0x0`（`:224`） | `DFF`=0 |
| `CLKPolarity` | `SPI_POLARITY_LOW` | `0x0`（`:233`） | `CPOL`=0 |
| `CLKPhase` | `SPI_PHASE_1EDGE` | `0x0`（`:242`） | `CPHA`=0 |
| `NSS` | `SPI_NSS_SOFT` | `SSM`（`:251`） | bit9 |
| `BaudRatePrescaler` | `SPI_BAUDRATEPRESCALER_4` | `BR_0 = 1<<3`（`:262`） | bit3 |
| `FirstBit` | `SPI_FIRSTBIT_MSB` | `0x0`（`:276`） | `LSBFIRST`=0 |
| `TIMode` | `DISABLE` | `0x0` | — |
| `CRCCalculation` | `DISABLE` | `0x0`（`:293`） | `CRCEN`=0 |
| `CRCPolynomial` | `10` | — | **无效参数**（CRC 关闭时不用） |

`HAL_SPI_Init` 的写法是"每个字段与自己的掩码按位与，然后整体或起来"（`stm32f1xx_hal_spi.c:90-98`），
所以可以直接手算：

```text
CR1 = MSTR(0x004) | SSI(0x100) | SSM(0x200) | BR_0(0x008)
    = 0x030C                      ← 分配后、使能前
CR1 |= SPE(0x040) = 0x034C        ← __HAL_SPI_ENABLE 之后
CR2  = (Init.NSS >> 16) & SSOE = 0x0000   ← 硬件 NSS 输出才置 SSOE
```

主机端程序把这段复刻了一遍并用 `assert` 锁住这两个值（跑通即证明位定义没抄错）。

**`SSI` 为什么必须为 1**：`NSS` 选了 `SOFT` 之后，芯片不再看真实 NSS 引脚，而是看 `SSI` 这一位
当作内部 NSS 电平。如果 `SSI=0`，外设会认为自己被别的 master 拉低了 NSS → 触发 MODF（模式错误）
→ **主机位被硬件清掉**，SPI 停止工作。CubeMX 把 `SSI` 直接塞进 `SPI_MODE_MASTER` 宏里，
就是在替我们挡这个坑（`hal_spi.h:206`）。

### 6. 18 MHz 从哪来，以及为什么 `/2` 不能用

SPI1 挂在 APB2 上，本工程 `APB2CLKDivider = RCC_HCLK_DIV1`、HCLK = 72 MHz ⇒ **PCLK2 = 72 MHz**。
`BR` 的 3 位编码从 0 到 7 对应 `/2 … /256`：

| 宏 | BR code | SCK | 每字节耗时 | 相对 F103 标称上限（18 MHz） |
| --- | --- | --- | --- | --- |
| `_2` | 0 | 36 MHz | 0.222 µs | **超规格** |
| `_4` | 1 | 18 MHz | 0.444 µs | ok（合法配置里最快） |
| `_8` | 2 | 9 MHz | 0.889 µs | ok |
| `_16` | 3 | 4.5 MHz | 1.778 µs | ok |
| `_32` | 4 | 2.25 MHz | 3.556 µs | ok |
| `_64` | 5 | 1.125 MHz | 7.111 µs | ok |
| `_128` | 6 | 562.5 kHz | 14.222 µs | ok |
| `_256` | 7 | 281.25 kHz | 28.444 µs | ok |

以上是主机端程序算出来的表（`lab_spi_cr1.out` 段二）。"18 MHz 上限"是 F103 数据手册的标称值，
**本仓库未收录该手册**，所以只用来解释"为什么 CubeMX 选 `/4` 而不是更快的 `/2`"；
真机上如果实测 36 MHz 也能稳定工作，那只能说明仪器和芯片都在超规格运行，不能反证规格。

### 7. 片选是软件 GPIO，且它和超声波抢同一根线

`NSS = SOFT` 意味着 CS 由代码手动拉，也就是 **PB9**（`driver_spiflash_w25q64.c:49-50`）：

```c
#define W25Q64_CS_GPIO_GROUP GPIOB
#define W25Q64_CS_GPIO_PIN   GPIO_PIN_9
```

三个必须知道的细节：

1. **所有 SPI 传输之间 CS 必须保持低**。`W25Q64_Read` 先 `W25Q64_Tx` 发 4 字节命令，再
   `W25Q64_Rx` 读数据，中间**没有** `Deselect`——因为 W25Q64 靠"CS 低 + 已收到 0x03 + 24 位地址"
   这个状态维持在读数模式。中间抬高 CS 会让它回到空闲，后续读到的全是 0xFF/0x00。
   两次 HAL 调用之间 SCK 会停一下，Flash 允许，因为它是同步从机、没有超时。
2. **驱动注释与我们的工程不符**。`driver_spiflash_w25q64.c:216` 写"片选信号 PB9 在
   `MX_GPIO_Init` 中被配置为输出引脚"，但本工程的 CubeMX 模板 `gpio.c` **只配了 PC13**
   （`grep GPIO_PIN_9 Core/Src/gpio.c` 无命中）。真正配置 PB9 的是我们自加的
   `Core/Src/lab_pins.c:78-83`。也就是说：**把这份驱动原样搬到别的工程，CS 引脚可能根本没配**，
   现象是"所有操作都失败但 `HAL_SPI_*` 返回 `HAL_OK`"（SPI 不检查 CS，它不知道 CS 存在）。
3. **PB9 一根线两个用途、两个相反的空闲电平**：W25Q64 的 CS 低有效（空闲高），
   HC-SR04 的 TRIG 需要高脉冲（空闲低）。所以 M6 与 M12 能同时插、不能同时用。
   我们的 `lab_pins.c` 里把初值取低（让 SR04 可用），代价是**上电后 Flash 一直处于被选中状态**
   直到第一次 `Deselect`。CS 低 + SCLK 静止时 Flash 只是被选中、收不到任何位，不会误执行命令——
   这条是源码推论，**未在真板用逻辑分析仪确认**。

### 8. 三个查询函数不是三个功能，是一个功能的三种包装

| HAL 函数 | 实际行为 | 收到的数据 |
| --- | --- | --- |
| `HAL_SPI_Transmit` | 只发，收到的字节丢在 DR 里不读 | 丢弃，**结束时 HAL 主动清 `OVR`**（`:934-937`） |
| `HAL_SPI_Receive` | = `TransmitReceive(pData, pData)` | 覆盖到 `pData` |
| `HAL_SPI_TransmitReceive` | 双向都取 | 写入 `pRxData` |

`HAL_SPI_Transmit` 结尾那句清 `OVR` 很关键：不读接收寄存器会导致 `OVR`（溢出）置位，
下一次读就会拿到旧数据。HAL 替你清了，所以"发送完再单独接收"这条用法在本工程里是安全的
（`W25Q64_Read` 正是这么写的）。

驱动的三个静态封装正好对应三种用法：

```c
W25Q64_Tx   → HAL_SPI_Transmit          写命令、写数据
W25Q64_Rx   → HAL_SPI_Receive           只读数据（缓冲区原内容当 Dummy 发出去）
W25Q64_TxRx → HAL_SPI_TransmitReceive   收发等长
```

`W25Q64_WaitReady` 用的是第三种：

```c
tx_buf[0] = 0x05;   /* 读状态寄存器 1 */
tx_buf[1] = 0xff;   /* 占位：只为产生 8 个 SCK，把状态字节移进来 */
W25Q64_Select();
W25Q64_TxRx(tx_buf, rx_buf, 2, W25Q64_TIMEOUT);
W25Q64_Deselect();
if ((rx_buf[1] & 1) == 0) return 0;   /* bit0 = BUSY */
```

**第一个收到的字节是垃圾**（发 0x05 那 8 个周期里 MISO 上的内容），状态在第二个字节。
这是"发 N 收 N"最直观的用法：想知道哪一位，就得让它出现在第几个字节上。

### 9. 中断和 DMA 在本工程里用不了

教材 9.4.4/9.4.5 给了 `_IT` 与 `_DMA` 两组函数，并说"需要在 CubeMX 里配置中断/DMA"。我们的工程**没配**：

| 证据 | 命令 | 结果 |
| --- | --- | --- |
| 没有 SPI1 中断服务函数 | `grep SPI1_IRQHandler Core/Src/*.c` | 无命中，只有启动文件的 `.weak` 默认处理（死循环） |
| 没有 NVIC 使能 | `grep SPI1_IRQn Core/Src/*` | 无命中 |
| 没有 DMA 句柄 | `grep DMA1_Channel Core/Src/spi.c stm32f1xx_hal_msp.c` | 无命中 |

所以现在调用 `HAL_SPI_Transmit_IT` 会**启动传输后立刻返回，然后没人处理中断**（中断进
`Default_Handler` 的 `b Infinite_Loop`），表现为卡死。要用必须先补 NVIC + `HAL_NVIC_EnableIRQ`，
DMA 还要补通道——F1 没有 DMAMUX，通道是芯片固定的（具体通道号见参考手册的 DMA 请求映射表，
本仓库未收录，故不给数字）。

DS18B20/Flash 这类"几百字节、以查询为主"的模块用查询方式足够；要不要上 DMA，
判据是"传输期间 CPU 有没有更该做的事"，不是"DMA 更高级"。

---

## 系统调用/接口卡片

### `HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout)`

作用：主机发送 `Size` 个字节；收到的字节丢弃。

参数：`hspi` 句柄（本工程 `&hspi1`）；`pData` 不能为 `NULL`、`Size` 不能为 0（否则立即 `HAL_ERROR`）；
`Timeout` **单位是 ms**（教材写 Tick，本工程 HAL 时基 1 ms ⇒ 等价）。

返回值和错误：`HAL_OK` / `HAL_ERROR`（含 `HAL_SPI_ERROR_FLAG`）/ `HAL_BUSY`（上一次还没完）/ `HAL_TIMEOUT`。

资源所有权：不持有缓冲区，调用返回后可复用；内部用 `__HAL_LOCK` 互斥，所以**不可重入**、不可在中断里调。

最小示例：`W25Q64_Tx(tmpbuf, 4, 500)`。

常见误用：以为"发出去了就等于对方收到了"——SPI 没有应答；把 `Timeout` 当微秒；在中断里调用。

### `HAL_StatusTypeDef HAL_SPI_Receive(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout)`

作用：主机接收 `Size` 个字节。**内部就是 `TransmitReceive(pData, pData)`**。

参数：同 `Transmit`。`pData` 既是发送源也是接收目标。

返回值和错误：同 `Transmit`。

常见误用：以为缓冲区会先被清零——它原来的内容会被打到 MOSI 上；
以为"只收"不会影响从机——若从机在等特定前缀就会出错。

### `HAL_StatusTypeDef HAL_SPI_TransmitReceive(SPI_HandleTypeDef *hspi, uint8_t *pTxData, uint8_t *pRxData, uint16_t Size, uint32_t Timeout)`

作用：同时收发各 `Size` 字节。

参数：`pTxData`/`pRxData` 可为同一指针（HAL 允许）；`Size` 是"'各自'的长度"，不是总和。

返回值和错误：同 `Transmit`；结束时等 `BSY` 清零（`SPI_EndRxTxTransaction`，超时取自 `Timeout`）。

常见误用：把 `Size` 写成收发之和；两个缓冲长度不一致。

### `int W25Q64_Read(uint32_t offset, uint8_t *buf, uint32_t len)`（课程驱动）

作用：读任意地址的 `len` 字节，内部是 `CS 低 → 发 0x03+24 位地址 → 收 len 字节 → CS 高`。

参数：`offset` 0..0x7FFFFF（8 MB）；`len` 由 `uint16_t` 上限约束（HAL 的 `Size` 是 `uint16_t`，
**单次最多 65 535 字节**，读更长要自己分段——驱动没做，这是它的限制之一）。

返回值和错误：返回 `len`；失败返回 -1。**注意：返回值没有区分"部分失败"**，失败即整体放弃。

常见误用：以为它能跨页/跨扇区做特殊处理——读没有页限制，这条只对写成立（Day12）。

### `static int W25Q64_WaitReady(void)`（课程驱动，`static`）

作用：轮询状态寄存器 1 的 `BUSY`（bit0），直到为 0。

返回值和错误：**声明上**超时返回 -1；**实际实现里 -1 分支不可达**：
`while (timeout--)` 结束时 `timeout` 等于 -1，`if (!timeout)` 恒假，于是返回 0。
这是本篇实测的源码事实，属于课程驱动已知缺陷，只记录不修改（Day12 §改错题用它做材料）。

---

## 最小可运行实验

不接板，四段：

```bash
cd experiments/F103_HAL_DriverLab/lab_spi

# 段一~段四：主机端复刻 CR1 编码、分频表、命令帧耗时、全双工语义
gcc -std=c11 -Wall -Wextra -o lab_spi_cr1 lab_spi_cr1.c
./lab_spi_cr1 > lab_spi_cr1.out && cat lab_spi_cr1.out

# 段五：把 W25Q64 驱动编进固件，量它的代价
cd .. && cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake -DLAB_TEST=17 \
  && cmake --build build
arm-none-eabi-nm -S --defined-only build/f103_hal_driverlab.elf | grep -E "W25Q64_|HAL_SPI_"
cmake -S . -B build -DLAB_TEST=0 && cmake --build build   # 复原
```

段一的 `assert` 是自校验：如果位定义抄错、掩码抄错，程序会直接 abort，
而不是"打印一个看起来合理的数"。**判据要能失败**，这条在 Day10 的 `--msb` 里也用过同一招。

---

## 编译与运行

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake \
  -DLAB_TEST=17
cmake --build build
```

真板上：W25Q64 模块插 M12 排母（与 M6 超声波抢 PB9，不能同时访问）。`W25Q64_Test` 会：

1. 逐个扇区（0..2047，即 0..8 MB）擦除，读回 4 字节原始值打印（擦除后应为 `FF FF FF FF`）；
2. 写入 `system_get_ns()` 读到的纳秒计数；
3. 读回比较，一致打印 `Test ok`，否则 `Test Failed` 并打印读到的值。

每步之间 `mdelay(1000)`，所以跑一圈 2048×3 秒 ≈ 1.7 小时。**以上屏幕行为未在真板验证。**

---

## 结果观察

**段一：寄存器编码**（`lab_spi_cr1.out` 实测）：

```text
== 1. CR1/CR2（按 HAL_SPI_Init 的写法复刻）==
CR1(未使能) = 0x0000030C
CR1(已使能) = 0x0000034C
CR2         = 0x00000000  （NSS 走软件，SSOE 不置位）
  CPHA=0 CPOL=0 MSTR=1 SSM=1 SSI=1 SPE=1 LSBFIRST=0 DFF=0 BR=001
  mode=0 (CPOL=0,CPHA=0)  br_code=1  => fPCLK/4
```

`BR=001` 是 `/4`，`CPOL=CPHA=0` 是 Mode 0，`SSM=SSI=1` 是软件片选且内部 NSS 为高——与 §5 的手算一致。

**段二：分频表**（见 §6 表格，实测输出；`_2` 标"超规格"、`_4` 起全部 ok）。

**段三：命令帧耗时**：

```text
  Tx: 03 00 12 34（4 字节命令）
  读 4096 字节共 32800 个 SCK = 1822 us @18MHz
  同样数据量走 I2C 100 kHz（每字节 9 SCL）：369000 us
```

1822 µs 是**理论估算**（只算 SCK，未计 CS 建立/保持、命令前的拉低延迟、`mdelay`）。
它说明量级：同样 4 KB，SPI 比 100 kHz 的 I2C 快约 200 倍——这是 Day10 §7 里"把字库放外部 Flash"
可行的原因：读一个 32 字节汉字 ≈ 16 µs（含 4 字节命令），远小于屏幕上画一次的开销。

**段四：全双工语义**：

```text
  调用前 buf : AA AA AA AA
  调用后 buf : 55 55 55 55
  从机收到的 MOSI 就是调用前的 buf 内容（0xAA），不是 0x00
```

模型里从机把收到的字节取反回吐，所以 `AA` 变成 `55`。这不是真 Flash 的行为，
但它把"发送源 = 接收目标"这件事变成可观察的：**改调用前的 buf，就能改 MOSI 上出现的内容**。

**段五：编进固件的代价**（`LAB_TEST=17` 实测）：

| 构建 | `.text` | `.rodata` | `.data` | Berkeley `text` |
| --- | --- | --- | --- | --- |
| `LAB_TEST=12`（仅 OLED） | 17 088 B | 2 284 B | 104 B | 19 648 B |
| `LAB_TEST=17`（W25Q64） | 19 764 B | 2 348 B | 104 B | 22 388 B |
| **17 − 12** | **+2 676 B** | +64 B | 0 | **+2 740 B** |

符号表（`nm -S`，字节数为含对齐的符号大小，逐项相加会略大于分节增量）：

```text
W25Q64_Test        0x148 = 328      W25Q64_Write       0xb0 = 176
W25Q64_Erase       0x078 = 120      W25Q64_Read        0x5e =  94
W25Q64_WaitReady   0x05a =  90      W25Q64_WriteEnable 0x26 =  38
W25Q64_TxRx        0x024 =  36      W25Q64_Tx / _Rx    0x1c =  28 ×2
W25Q64_Select/Deselect 0x14 = 20 ×2  —— 驱动合计 978 B
HAL_SPI_TransmitReceive 0x24c=588   HAL_SPI_Transmit 0x1b8=440
HAL_SPI_Receive    0x180 = 384      HAL_SPI_Init      0xb6=182
HAL_SPI_MspInit    0x078 = 120      MX_SPI1_Init      0x44= 68  —— HAL 侧合计 1 782 B
```

**一个意外的耦合**：`W25Q64_Test` 用 `LCD_PrintString` 打印，于是 `driver_spiflash_w25q64.c`
`#include "driver_lcd.h"`，选 `LAB_TEST=17` 时整个 OLED 代码栈（`OLED_PrintChar`/`SetPosition`/
`WriteNBytes`…）都被拉进镜像，实测 `LCD_*`+`OLED_*` 合计 **658 B 代码 + 2 048 B 的 `ascii_font`**
（`nm` 显示 `ascii_font` 大小 `0x800`）。也就是说：**一个 Flash 测试函数，为了打印，把显示驱动
一起绑上了**。这不是错误，但它说明"驱动自测函数"是耦合源——真项目里测试代码应留在测试层，
不要混进驱动（Day14 会把它归到"驱动库契约"里讲）。

---

## 常见错误和陷阱

| 陷阱 | 症状 | 根因 | 处理 |
| --- | --- | --- | --- |
| MISO 配成 `GPIO_MODE_AF_PP` | 读回全 `0x00` | 只有主机发出的方向需要推挽输出，MISO 是输入 | 按 `spi.c:85-88` 配 `GPIO_MODE_INPUT` |
| `CPOL`/`CPHA` 与从机不一致 | 收到的第一个字节就错位，全 `0xFF` 或 `0x00` | 采样沿不同，位被切在半路 | 先查从机手册支持的 Mode，再改这两位 |
| 忘拉 CS / 中途抬高 CS | 数据看着像对的但偶发全 `FF` | 从机命令状态被 CS 上升沿复位 | `Tx` 与 `Rx` 之间不要 `Deselect` |
| 把 SPI 当 I2C 找地址 | 找不到"设备地址"参数 | SPI 没有地址字段 | 用 CS 引脚选设备 |
| 指望 `HAL_SPI_Receive` 清空缓冲区 | 收到的数据正确但 MOSI 上有怪字节 | 缓冲区原内容被当 Dummy 发出去 | 需要确定 Dummy 值就用 `TransmitReceive` 显式给 |
| `Size` 用收发之和 / 缓冲区不够 | 越界或数据缺半 | `Size` 是"各自"长度 | 读 Day09 §接口卡片的同类错误 |
| 只用 `Transmit` 后直接读 DR | 读到旧数据 | `OVR`/`RXNE` 状态没清 | 用 `Receive`，别自己读 DR |
| 分频选 `_2` | 18 MHz 以上，通信不稳或根本不工作 | 超过 F103 标称上限 | 用 `_4` |
| 调 `HAL_SPI_Transmit_IT` | 卡死 | 工程没配 NVIC，中断进 `Default_Handler` | 先补 NVIC，或改用查询 |
| CS 引脚没配就搬驱动 | 所有操作返回 `HAL_OK`，Flash 无反应 | SPI 不检查 CS，CS 只是普通 GPIO | 自己配 CS，别信驱动注释 |

---

## 故障排查方法

SPI 通信失败，"全 0x00"和"全 0xFF"两种现象指向完全不同的原因，先分开：

1. **读回全 `0x00`** → MISO 上没有信号。
   - 引脚模式错（配成了输出）、片选没拉低、模块没插/供电、或 CS 与 SR04 抢线时 SR04 在拉低。
   - 先量 PA6 在 CS 低时是否有跳变；没有跳变就查引脚复用。
2. **读回全 `0xFF`** → MISO 被上拉，从机没在驱动它。
   - 命令没被接受（CPOL/CPHA 错、CS 时序错）、或从机在 `BUSY`（擦写未完成）。
   - 用 `0x05` 反复读状态：如果第一次就不是 `FF`，说明位流是对的，问题在命令/状态；
     如果连状态都读成 `FF`，问题在物理层或模式。
3. **模式自检**：把 `CPOL/CPHA` 改成 Mode 3 试一次。Mode 0 与 Mode 3 采样沿相同，
   两者都失败 ⇒ 大概率不是模式问题；一个通一个不通 ⇒ 从机对空闲电平敏感。
4. **确认到底在跟谁通信**：读 JEDEC ID（`0x9F`，标准命令；教材 9.5.2 提到"读 ID"但命令号在图片里，
   本仓库未核对原文）。W25Q64 返回 `EF 40 17`。**ID 能读出来，才说明命令帧、模式、片选三者同时正确**——
   这是最短的自证路径。
5. **最后才怀疑速度**：先降到 `_256`（281 kHz）。查询方式没有时序压力，慢不影响正确性，
   但能排除"上升沿不够陡""杜邦线太长"这类电气问题。

---

## 练习题

### 基础题

题目：本工程 `BR` 配的是 `_4`。请分别算出 `_2`、`_4`、`_8` 三档下"发 4 字节命令 + 读 4 096 字节"
所占的 SCK 周期数与时间，并指出哪一档超出 F103 标称上限。

提示：SCK 周期数 = (4+4096)×8；时间 = 周期数 ÷ SCK 频率。`_4` 的答案是 32 800 个周期 / 1 822 µs。

验收标准：三档数值都算对；能说明"超规格"是指 SCK 频率超过 18 MHz 而不是"芯片不会跑"；
能说出这个计算**不含** CS 建立时间与命令处理时间，属于理论下限。

### 改错题

题目：下面这段想"读 4 个字节"，但读到的始终是 `FF FF FF FF`。指出全部问题并修正。

```c
uint8_t buf[4];
W25Q64_Select();
HAL_SPI_Transmit(&hspi1, (uint8_t *)"\x03\x00\x00\x00", 4, 500);
W25Q64_Deselect();                       /* ① */
HAL_SPI_Receive(&hspi1, buf, 4, 500);    /* ② */
W25Q64_Deselect();
```

另附 `MX_SPI1_Init` 的相关片段，它也有两处与"读 Flash"不匹配：

```c
hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;   /* ③ */
hspi1.Init.CLKPhase    = SPI_PHASE_2EDGE;     /* ③ */
hspi1.Init.DataSize    = SPI_DATASIZE_16BIT;  /* ④ */
```

提示：①位于两次 HAL 调用之间；②在 CS 已抬高的状态下调用；
③把模式改成了 Mode 3（对 W25Q64 仍可用，但要与从机一致，且这里是"想改成 0 却改错"的场景）；
④16 位帧会让命令和地址的字节边界都变。

验收标准：指出 ① 必须删除、② 必须移到 CS 拉低之后；能说明 ④ 会让"4 字节命令"变成 2 个 16 位帧，
命令帧与地址帧错位；能给出正确版本（CS 低 → `Tx` 命令 → `Rx` 数据 → CS 高，8 位、Mode 0）。

### 综合题

题目：Day10 提到"整本字库放不进片内 Flash，可以放外部 W25Q64"。请设计"从 W25Q64 读一个 16×16
汉字并显示"的完整路径，要求：
① 给出字库在 Flash 中的组织方式与一个码点到地址的换算；
② 算出读一个字的耗时（含命令），并与"128×64 屏（8 列 × 4 行 = 32 个汉字）整屏刷新"对比；
③ 说明为什么不能每帧都从 Flash 读所有字。

提示：每个字 32 字节；地址 = 码点偏移 × 32；命令 4 字节 + 数据 32 字节 = 288 个 SCK。
参考 §结果观察的 18 MHz 换算（每字节 0.444 µs）。

验收标准：能给出"码点 → 表内序号 → 地址"的两级换算并说明为什么要一张索引表（汉字码点不连续，
不能直接用码点乘 32）；能算出单字约 16 µs、一屏 8×4=32 个汉字约 0.5 ms（理论下限）；
能论证"每帧重读一屏"在 20 ms 帧周期里只占 2.5%，**不是瓶颈**，真正的瓶颈是 I2C 写屏
（Day10 §8：3.8 ms/汉字）——所以优化方向应放在屏的总线上，而不是 Flash。

---

## 验收标准

- [ ] 能说出四根线的方向与"片选代替地址"的后果（无地址、无应答）。
- [ ] 能默写四种模式的 `CPOL`/`CPHA` 组合，并指出 Mode 0 与 Mode 3 采样沿相同。
- [ ] 能解释为什么"只收不发"不存在，并说清 `HAL_SPI_Receive` 往 MOSI 上发的是什么。
- [ ] 能由 `spi.c` 的 11 个字段手算出 `CR1 = 0x030C`（使能后 `0x034C`），并说明 `SSI` 必须为 1。
- [ ] 能解释 MISO 为什么是 `GPIO_MODE_INPUT`，与 I2C 的 `AF_OD` 差在哪。
- [ ] 能算出 72 MHz / 4 = 18 MHz，并说明为什么 `/2` 不能用。
- [ ] 能独立跑通本篇实验的五段，读出 `CR1`、分频表、命令帧耗时与 `LAB_TEST=17` 的体积代价。

## 延伸阅读

- 教材：《HAL快速入门与项目实战(基于DshanMCU-103)》第 9.1（层次）、9.2（协议与四模式）、
  9.3（STM32 SPI 硬件结构与操作步骤）、9.4（HAL 三组查询/中断/DMA 函数）节
- 课程驱动：`Drivers/DshanMCU-F103/driver_spiflash_w25q64.c:95-140`（三个 SPI 封装）、
  `:153-178`（读状态）、`:214-218`（`W25Q64_Init` 的空实现与其错误注释）、`:231-262`（读）
- 外设配置：`Core/Src/spi.c:30-94`（`MX_SPI1_Init` / `HAL_SPI_MspInit`）
- HAL 实现：`Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_spi.c:90-101`（拼 `CR1`）、
  `:928-937`（清 `OVR`）、`:977-982`（`Receive` 转 `TransmitReceive`）、`:3674-3683`（等 `BSY`）
- 本篇新增：`lab_spi/lab_spi_cr1.c`、输出 `lab_spi/lab_spi_cr1.out`
- 上一篇：`Linux/notes/F103_HAL入门/Day10_OLED显示汉字与点阵.md`
- 下一篇：`Linux/notes/F103_HAL入门/Day12_SPI_Flash编程.md`

---

**验证记录**

| 项目 | 内容 |
| --- | --- |
| 日期 | 2026-10-29 |
| 环境一 | Windows 11 + Git Bash；目录 `experiments/F103_HAL_DriverLab/lab_spi` |
| 命令一 | `gcc -std=c11 -Wall -Wextra -o lab_spi_cr1 lab_spi_cr1.c`；`./lab_spi_cr1` |
| 结果一 | `CR1` 未使能 `0x0000030C`、使能 `0x0000034C`、`CR2=0`，与 `assert` 期望值一致；解码出 `mode=0`、`br_code=1`、`fPCLK/4`；分频表 8 档 SCK 与 `us/Byte`；4 096 字节读 32 800 SCK = 1 822 µs；全双工模型 `AA→55` |
| 环境二 | 同上；`arm-none-eabi-gcc` 14.2.1 + CMake 4.0.3 + Ninja |
| 命令二 | `cmake -S . -B build -DLAB_TEST=17 && cmake --build build`；`arm-none-eabi-size` / `size -A -x` / `nm -S`；随后 `-DLAB_TEST=0` 复原 |
| 结果二 | 构建成功，13 条告警与 `LAB_TEST=0` 时完全相同（`driver_oled.c` 10 条：2 条 `-Wchar-subscripts` + 8 条 `-Wunused-function`；`driver_ds18b20.c` 3 条 `-Wparentheses`；无新增）；`.text` 19 764 / `.rodata` 2 348 / Berkeley `text` 22 388；`W25Q64_*` 符号合计 978 B、`HAL_SPI_*` 合计 1 782 B；`ascii_font` 0x800 = 2 048 B 被 `LCD_PrintString` 链条带入 |
| 未覆盖 | **SPI 从未上板**：无 SCK/MISO 波形、无 W25Q64 ID 读数、无擦写实测；18 MHz 为标称上限对比而非实测；`CR1/CR2` 为源码复刻值，未从真机寄存器读回；1 822 µs 为只算 SCK 的理论估算；中断/DMA 不可用是源码核对结论，未构造"调用 `_IT` 后卡死"的实测；`W25Q64_WaitReady` 的超时分支不可达为源码推理，未用"断掉 MISO"的故障注入验证 |
