# DShanMCU-F103 模块与驱动速查表

**一句话**：课程驱动只做"业务收发"，引脚与外设初始化一律交给 CubeMX 生成的 `MX_xxx_Init` /
`HAL_xxx_MspInit`；这张表把"哪个模块、用哪个驱动、占哪几个引脚、依赖哪个 HAL 模块、谁来配"一次说清。

**适用范围**：DShanMCU-103（STM32F103C8T6，64 KB Flash / 20 KB SRAM），HSE 8 MHz → PLL×9 = 72 MHz，
APB2 = 72 MHz、APB1 = 36 MHz。驱动取自课程 `Drivers/DshanMCU-F103/`，**未修改**。

**验证状态**：引脚与依赖关系**源码核对**（逐条抄自驱动宏定义与注释，非推断）；
构建**实测**（GCC 14.2.1 + CMake，0 error / 13 warning，见
[`experiments/F103_HAL_DriverLab/`](../experiments/F103_HAL_DriverLab/README.md)）；
**20 个模块的板上功能全部未验证**（本机无外部模块、无 ST-Link）。

---

## 1. 总表：模块 ↔ 驱动 ↔ 引脚 ↔ HAL 依赖

| 模块 | 驱动文件 | 引脚 | HAL 模块 | 引脚由谁配置 |
| --- | --- | --- | --- | --- |
| 板载单色 LED | `driver_led.c` | PC13 | GPIO | 驱动 `Led_Init()`（CubeMX `gpio.c` 也配） |
| 按键 K1 | `driver_key.c` | PB14（输入） | GPIO | 驱动 `Key_Init()` |
| 有源蜂鸣器 | `driver_active_buzzer.c` | PA8（推挽） | GPIO | 驱动 `ActiveBuzzer_Init()` |
| 无源蜂鸣器 | `driver_passive_buzzer.c` | PA8 / TIM1_CH1 | GPIO + TIM | 驱动 + `tim.c` |
| 红外发射 38 kHz | `driver_ir_sender.c` | PA8 / TIM1_CH1 | TIM | 驱动 + `tim.c` |
| 直流电机 DRV8833 | `driver_motor.c` | A: PA4 / PA0<br>B: PB4（TIM3_CH1）/ PB15 | GPIO + TIM | 驱动 + `tim.c` |
| 串口（调试口） | `driver_uart.c` | PA9=TX / PA10=RX（USART1） | UART | `usart.c:HAL_UART_MspInit` |
| RGB 全彩 LED | `driver_color_led.c` | PA15=G(TIM2_CH1)<br>PB3=B(CH2)<br>PA2=R(CH3) | TIM | `tim.c` |
| 光敏电阻 | `driver_light_sensor.c` | PA3 = ADC1_IN3 | ADC | `adc.c:HAL_ADC_MspInit` |
| OLED SSD1306 | `driver_oled.c` | PB6/PB7（I2C1），从机地址 `0x78` | I2C | `i2c.c:HAL_I2C_MspInit` |
| LCD | `driver_lcd.c` | 同 OLED（纯转发封装） | I2C | 同上 |
| MPU6050 | `driver_mpu6050.c` | PB6/PB7（I2C1），写地址 `0xD0` | I2C | 同上 |
| SPI Flash W25Q64 | `driver_spiflash_w25q64.c` | PA5=SCK / PA6=MISO / PA7=MOSI / **PB9=CS** | SPI + GPIO | `spi.c` MSP + 用户配 PB9 |
| HC-SR04 | `driver_ultrasonic_sr04.c` | **PB9=TRIG** / PB8=ECHO | GPIO | 用户配 PB9、PB8 |
| 红外避障 LM393 | `driver_ir_obstacle.c` | PB13（输入） | GPIO | 用户配 PB13 |
| 旋转编码器 EC11 | `driver_rotary_encoder.c` | S1=PB0 / S2=PB1 / KEY=PB12（EXTI 上升） | GPIO + EXTI | 用户配 PB0/PB1/PB12 + 使能 `EXTI15_10_IRQn` |
| 红外接收 1838 | `driver_ir_receiver.c` | PB10（EXTI 双边沿） | GPIO + EXTI | 用户配 PB10 + 使能 `EXTI15_10_IRQn` |
| DS18B20 | `driver_ds18b20.c` | PA1（开漏 + 上拉） | GPIO | 用户配 PA1 |
| DHT11 | `driver_dht11.c` | PA1（开漏 + 上拉） | GPIO | 用户配 PA1 |
| （中断汇总） | `driver_irq.c` | — | — | 定义 `HAL_GPIO_EXTI_Callback`，按 `GPIO_PIN_10/12` 分发 |

"用户配"的 8 个纯 GPIO + NVIC 使能，在本仓库里落在
[`experiments/F103_HAL_DriverLab/Core/Src/lab_pins.c`](../experiments/F103_HAL_DriverLab/Core/Src/lab_pins.c)。

---

## 2. 引脚冲突：三条规则与物理原因

课程第 4 章的冲突表只有六条记录（M2/M4/M6/M9/M10/M12），编号也只在这六个上可用。

| 冲突对 | 规则 | 物理原因（源码核对） |
| --- | --- | --- |
| M2 DS18B20 / M10 DHT11 | **不能同时接** | 两者都用 PA1 |
| M4 红外发射 / M9 无源蜂鸣器 | **不能同时接** | 两者都用 PA8 的 TIM1_CH1；一个通道只有一个占空比/频率 |
| M6 超声波 / M12 SPI Flash | **可同时接，不能同时访问** | 两者都用 PB9（SR04 的 TRIG、W25Q64 的 CS），且要求的空闲电平相反 |

第三条值得记住的推论：W25Q64 片选低有效、空闲应为高；SR04 的 TRIG 靠高脉冲触发、空闲应为低。
**同一根线不可能同时满足两个空闲电平**——这就是"能插、不能同时用"的机制，而不是简单的软件约定。

---

## 3. 驱动对 CubeMX 的依赖清单

`driver_xxx.c` 只调用外设**收发**函数，初始化全在别处。用错的前提是"忘了初始化"：

| 驱动 | 依赖的 `MX_` / MSP 函数 | 缺了会怎样 |
| --- | --- | --- |
| `driver_led` `driver_key` `driver_active/passive_buzzer` `driver_motor` | 自己 `HAL_GPIO_Init` | 基本不依赖外部配置（PA8、PB4 仍需 `tim.c` 提供 PWM 时钟与 AF） |
| `driver_uart` | `MX_USART1_UART_Init()` | `huart1.gState == RESET`，发送直接返回错误 |
| `driver_oled` `driver_lcd` `driver_mpu6050` | `MX_I2C1_Init()` | `HAL_I2C_*` 返回 `HAL_ERROR`/超时，屏不亮 |
| `driver_spiflash_w25q64` | `MX_SPI1_Init()` + PB9 配成输出 | 读 JEDEC ID 得 `00 00 00` |
| `driver_light_sensor` | `MX_ADC1_Init()`（通道 3 = PA3） | `HAL_ADC_PollForConversion` 超时 |
| `driver_color_led` | `MX_TIM2_Init()` + `HAL_TIM_PWM_Start` | 灯不亮（引脚无 PWM） |
| `driver_ir_sender` `driver_passive_buzzer` | `MX_TIM1_Init()` | PA8 无载波，红外无发射、蜂鸣器不响 |
| `driver_motor` | `MX_TIM3_Init()`（PB4 = CH1） | 只能全速/停，无法调占空比 |
| `driver_ultrasonic_sr04` | PB9 输出 / PB8 输入 | 读到恒定值或 0 |
| `driver_ds18b20` `driver_dht11` | PA1 开漏 + 上拉 | 单总线读不到应答（两者共用 PA1，不能同接） |
| `driver_rotary_encoder` `driver_ir_receiver` | PB0/PB1/PB12/PB10 + `EXTI15_10_IRQn` 使能 | 回调永不进入；计数恒 0 |
| `driver_timer`（`udelay`/`mdelay`） | **`stm32f1xx_hal_timebase_tim.c` 把 TIM4 配成 1 MHz** | 延时严重偏大或偏小 |

### 3.1 `driver_timer` 与 HAL 时基共用 TIM4

`driver_timer.c` 直接读 `htim4` 计数器做忙等，而 HAL 时基也被切到 TIM4（1 MHz / 1 ms）。
这不是冲突而是**耦合**：把 HAL 时基换回 SysTick，`udelay()` 就会算错。
课程选 TIM4 做时基的直接原因就是给 `driver_timer.c` 用。代价是 TIM4 不能再被别处占用。

---

## 4. 中断映射

| EXTI 线 | 引脚 | 中断处理函数 | 回调分支 | 谁在响应 |
| --- | --- | --- | --- | --- |
| EXTI10 | PB10 | `EXTI15_10_IRQHandler` | `HAL_GPIO_EXTI_Callback(GPIO_PIN_10)` | `IRReceiver_IRQ_Callback`（红外解码） |
| EXTI11 | PB11 | 同上 | `GPIO_PIN_11` | **无分支**（处理了但什么都不做） |
| EXTI12 | PB12 | 同上 | `GPIO_PIN_12` | `RotaryEncoder_IRQ_Callback`（按键） |

两个坑：

1. **EXTI 线号只有一个，端口由 `AFIO->EXTICR` 决定默认落在端口 A**。
   不显式把 EXTI10 映射到端口 B，它就会盯着 PA10 —— 而 PA10 是 USART1_RX，
   串口每收一字节都触发一次"红外中断"。
   映射由 `HAL_GPIO_Init(GPIOB, GPIO_PIN_10)` 里的 IT/EVT 分支写入
   （`stm32f1xx_hal_gpio.c:292-295`）。
2. `stm32f1xx_it.c` 里有 handler **不等于**中断会来，还必须 `HAL_NVIC_EnableIRQ(EXTI15_10_IRQn)`。

---

## 5. 时钟与频率速查

所有 PWM/载波频率都按 **72 MHz 定时器时钟**硬编码，改系统时钟会静默出错。

| 用途 | 定时器 | PSC | ARR | 结果 |
| --- | --- | --- | --- | --- |
| 无源蜂鸣器（默认） | TIM1_CH1 | 71 | 999 | 72 MHz / 72 / 1000 = **1 kHz**，CCR 499 = 50 % |
| 无源蜂鸣器（可变频） | TIM1_CH1 | 71 | `1e6/freq − 1` | 1 MHz 计数，任意 `freq` |
| 红外发射载波 | TIM1_CH1 | **0** | 1894 | 72 MHz / 1895 ≈ **37.99 kHz**，CCR 473 ≈ 75 % |
| RGB 全彩 LED | TIM2_CH1/2/3 | 359 | 1999 | 72 MHz / 360 / 2000 = **100 Hz**，适合调光 |
| 直流电机 | TIM3_CH1 | 7 | 999 | 72 MHz / 8 / 1000 = **9 kHz** |
| HAL 时基 + `udelay` | TIM4 | 71 | 999 | 1 MHz 计数、1 ms 中断 |
| 调试串口 | USART1 | — | — | 115200 8N1（APB2 = 72 MHz） |
| I2C1 | — | — | — | 100 kHz 标准模式 |
| SPI1 | — | — | — | APB2/4 = **18 MHz**，模式 0（CPOL=0, CPHA=1Edge），软件 NSS |

---

## 6. 接一个新模块的最小步骤（checklist）

1. 查 §1 总表，确认引脚与 HAL 模块；
2. 查 §2 冲突表，确认不与已接模块抢引脚；
3. 在 CubeMX 里把该引脚配成驱动要求的模式（输出/输入/上拉/EXTI），生成代码；
4. 若属于"用户配"的那一组且你不打算重生成 CubeMX 工程，就在
   `Core/Src/lab_pins.c` 里加一段并写清来源；
5. ExtI 类模块：确认 `HAL_NVIC_EnableIRQ` 已使能对应 `EXTIx_y_IRQn`；
6. `main.c` 的 `Lab_Run()` 里加一个 `#elif LAB_TEST == N` 分支（**不要用运行期 `switch`**，
   实测会让 19 个 Test 全部进镜像，10.9 KB → 33.9 KB）；
7. 构建：`cmake -S . -B build ... -DLAB_TEST=N && cmake --build build`；
8. 烧写后核对接线：供电、GND、以及"模组是否需要独立供电"（电机、超声不建议吃板载 3V3）。

---

## 7. 陷阱表

| 现象 | 根因 | 处理 |
| --- | --- | --- |
| 单总线（DHT11/DS18B20）读不到应答 | 引脚没配成"开漏 + 上拉"；或两个模块同时插在 PA1 | 配 `GPIO_MODE_OUTPUT_OD` + `PULLUP`；只插一个 |
| 红外接收频繁误中断 | EXTI10 映射在 PA10（USART1_RX）上 | 显式 `HAL_GPIO_Init(GPIOB, PIN_10)` |
| 编码器按键无反应 | 只配了 EXTI 引脚，没 `HAL_NVIC_EnableIRQ` | 补 NVIC 使能 |
| 电机能转但调不了速 | 没调 `MX_TIM3_Init()`，PB4 不是 TIM3_CH1 的 AF 输出 | 保留 `tim.c`；PWM 的 Start/Stop 由 `driver_motor.c` 自己调 |
| Flash 读 JEDEC = `00 00 00` | CS(PB9) 未配成输出 / SPI1 未 init / 与 SR04 抢 PB9 | 见 §2 与 §3 |
| `udelay` 延时离谱 | HAL 时基不是 TIM4 1 MHz | 保留 `stm32f1xx_hal_timebase_tim.c` |
| `printf` 没输出 | 只加了 `driver_uart.c`（它的 `fputc` 是 Keil/MicroLIB 路径，newlib 下被 GC 掉） | newlib 用 `_write` 重定向 |
| 改了 `LAB_TEST` 镜像不变 | `LAB_TEST` 是 CMake cache 变量 | 重新 configure |

---

## 8. 已知上游缺陷（课程驱动，本仓库不修改）

| 文件 | 位置 | 缺陷 | 影响 |
| --- | --- | --- | --- |
| `driver_ds18b20.c` | 约 463/465/467 行 | `ram[4] & (3<<5) == 0` 被解析为 `ram[4] & ((3<<5)==0)` = `& 0`，恒假 | 9/10/11 bit 分辨率分支是死代码，总是走 12 bit |
| `driver_oled.c` | 约 542 行 `OLED_PutChar` | 用有符号 `char` 索引 `ascii_font[128][16]` | `c ≥ 0x80` 时负下标越界读 |
| `driver_ir_receiver.c` | `IRReceiver_Init()` 注释 | 写"PA10 … 双边沿"，实际代码与分发都是 PB10 | 注释误导；PA10 已被 USART1_RX 占用 |
| `driver_uart.c` | `fputc`/`fgetc` | Keil/MicroLIB 的重定向方式 | 在 newlib 下不被 `printf` 引用，被 `--gc-sections` 回收 |

不修的理由：这些是课程发布的代码，改了就失去与教材逐行对照的价值。要改就先在
`experiments/` 里做对照实验，把"改前/改后"的现象都留下来。

---

## 9. 当前验证状态

| 项 | 状态 |
| --- | --- |
| 20 个驱动 + HAL 全量编译 | **实测**：0 error / 13 warning（13 条全在上游驱动），Flash 15 376 B / 23.5 % |
| 引脚与 HAL 依赖关系 | **源码核对**（抄自驱动宏定义与注释，未推断） |
| 引脚冲突三条规则 | **课程文档给出的规则 + 源码核对物理原因**；未用逻辑分析仪验证 |
| 向量表、GC、编译期裁剪 | **实测**（`.bin` 前 8 字节、`nm` 符号表） |
| 任一模块的板上功能 | **全部未验证**（无板、无模块在手） |
| 烧写/调试链路 | **未验证** |

一句话：这张表能保证"接线不会接错、初始化不会漏"，**不能**保证"接上就一定工作"。

---

## 证据来源

- 驱动源码：`experiments/F103_HAL_DriverLab/Drivers/DshanMCU-F103/driver_*.c`（引脚宏、`#if 0` 段、注释里的 "在 MX_GPIO_Init 里被配置"）
- 外设配置：同目录 `Core/Src/{gpio,usart,i2c,spi,adc,tim}.c` 的 `MX_xxx_Init` 与 `HAL_xxx_MspInit`
- HAL 行为：`Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio.c:292-295`（EXTI 端口映射）
- 构建读数：`experiments/F103_HAL_DriverLab/README.md` §3（`arm-none-eabi-size` / `nm` 原始输出）
- 课程原始工程：`Linux_WEIDS/DShanMCU-F103/5_程序源码/01_单片机程序/`、`02_FreeRTOS程序/`
- 冲突规则与模块编号：课程第 4 章冲突表（仅 M2/M4/M6/M9/M10/M12 六个编号有据）
