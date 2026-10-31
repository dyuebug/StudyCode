# F103_HAL_DriverLab：DShanMCU-103 官方驱动库的 GCC/CMake 构建与 Cursor 调试环境

**目的**：把课程官方提供的 HAL 驱动库（`Drivers/DshanMCU-F103/`，20 个模块 + 1 个 IRQ 汇总）
从 Keil MDK 迁到 `arm-none-eabi-gcc` + CMake/Ninja，并在 Cursor 里补齐"能编译、能烧写、能断点"的完整链路。

**不做什么**：不改驱动源码、不改 CubeMX 生成的外设初始化（只新增 `Core/Src/lab_pins.c` 补一个模板缺口，
理由见 §6）。课程工程仍是权威来源，这里的差异全部记账，便于对照。

**记录性质**：构建与产物**实测**（本文所有体积/警告数都是本机跑出来的）；板上现象**全部未验证**
（本机无 ST-Link 在手，见 §9）。文档不把"编出来了"写成"跑通了"。

---

## 1. 环境与前置条件

| 项 | 本机实测版本 |
| --- | --- |
| 工具链 | `arm-none-eabi-gcc` 14.2.1 20241119（Arm GNU Toolchain 14.2.Rel1, Build arm-14.52） |
| 构建 | CMake 4.0.3 + Ninja 1.13.2 |
| 调试/烧写 | xPack OpenOCD 0.12.0+dev-02228 |
| 硬件 | ST-Link V2（SWD 4 线：3V3 / GND / SWDIO / SWCLK） |
| 目标板 | DShanMCU-103（STM32F103C8T6，64 KB Flash @0x08000000，20 KB SRAM @0x20000000） |
| 公共设施 | `../F103_common/`（自撰链接脚本、CMake 工具链文件、OpenOCD 配置） |

工具链可用性检查（缺任一项构建或烧写会直接失败）：

```bash
arm-none-eabi-gcc --version
cmake --version && ninja --version
openocd --version
```

---

## 2. 目录结构与三份来源

> **`Drivers/` 不入库**：STM32 CMSIS 与 HAL 驱动是 vendor 源码（27 MB / 248 文件），
> 按本仓库惯例只保留许可证声明（见 `experiments/F103_common/vendor/`），
> `.gitignore` 已排除 `Linux/experiments/F103_HAL_DriverLab/Drivers/`。
> 克隆后需从课程材料复制 `DshanMCU-F103/`、`STM32F1xx_HAL_Driver/`、`CMSIS/`
> 三个目录到 `Drivers/` 下才能构建。

```text
F103_HAL_DriverLab/
├── CMakeLists.txt          # 构建描述（唯一需要手改的构建文件）
├── .clangd                 # clangd 指向 build/compile_commands.json + 编译器绝对路径
├── .vscode/                # settings / tasks / launch（Cursor 与 VS Code 通用）
├── Core/                   # 外设初始化：CubeMX 生成 + 五个手写补充
│   ├── Inc/                #   （含 stm32f1xx_hal_conf.h、lab_pins.h、hanzi_font.h、lab_oled_hanzi.h）
│   └── Src/
│       ├── main.c          #   手写：启动顺序 + LAB_TEST 分发（课程原在 freertos.c）
│       ├── gpio.c usart.c i2c.c spi.c adc.c tim.c
│       │                   #   CubeMX 生成，来自 01_freertos_template
│       ├── stm32f1xx_it.c stm32f1xx_hal_msp.c
│       ├── stm32f1xx_hal_timebase_tim.c   # HAL 时基改用 TIM4
│       ├── system_stm32f1xx.c
│       ├── crt_stubs.c     #   手写：-nostartfiles 后缺失的 _init/_fini 与 syscall 桩
│       ├── retarget.c      #   手写：printf → USART1（newlib 的 _write）
│       ├── lab_pins.c      #   手写：驱动要求、模板 gpio.c 没配的纯 GPIO
│       ├── hanzi_font.c    #   手写：教材 8.5.4 的 16x16 点阵 + 按码点查表（不含 HAL）
│       └── lab_oled_hanzi.c #  手写：UTF-8 解码 + 分两页写显存（LAB_TEST=19）
├── Drivers/
│   ├── DshanMCU-F103/      # 课程官方驱动，20 个 driver_*.c + driver_irq.c（原样）
│   ├── STM32F1xx_HAL_Driver/   # ST HAL（原样）
│   └── CMSIS/              # Cortex-M3 内核头 + F1 器件头（原样）
├── startup/startup_stm32f103xb.s   # GCC 语法启动文件（Keil 用的是 ARM 汇编版）
├── ld/STM32F103C8Tx.ld             # 链接脚本（Keil 用的是 .sct 分散加载）
├── lab_c_basics/           # 主机端 C 语言练习（教材 5.6），不参与固件构建
│   └── lab_c_basics.c      #   用普通 gcc 编译运行，验证指针/结构体/位运算
├── lab_oled_hanzi/         # 主机端点阵渲染器（教材 8.5），不参与固件构建
│   ├── render_hanzi.c      #   把 g_hanzi16 打成 ASCII 图，证 bit0 在上
│   └── render_output.txt / render_msb.txt   # 两种位序的实测输出
├── lab_spi/                # 主机端 SPI 校验（教材 9），不参与固件构建
│   ├── lab_spi_cr1.c       #   复刻 CR1 位域与 BR 分频表（Day11）
│   └── lab_w25q64_defects.c#   擦除对齐/页回卷/1→0/超时分支四段缺陷（Day12）
├── lab_timer/              # 主机端定时器校验（教材 10），不参与固件构建
│   └── lab_timer_calc.c    #   PSC/ARR 换算、APB1 倍频、udelay 999/1000、占空比两口径（Day13）
├── lab_contract/           # 主机端驱动库契约扫描（教材 11），不参与固件构建
│   └── scan_drivers.sh     #   20 个头文件 / 75 个函数：Init/DeInit/Test、返回类型、外设依赖（Day14）
└── logs/build_2026-10-29.log       # 本次全量重建的原始编译输出（13 条告警的出处）
```

讲义见 `Linux/notes/F103_HAL入门/Day01..Day14`（对照教材逐节拆解，每篇给出本篇落点与验证记录）。

三份来源必须分清，出问题时才知道该看哪：

| 来源 | 文件 | 改动策略 |
| --- | --- | --- |
| CubeMX 生成 | `gpio.c` `usart.c` `i2c.c` `spi.c` `adc.c` `tim.c` `stm32f1xx_it.c` `stm32f1xx_hal_msp.c` `stm32f1xx_hal_timebase_tim.c` `system_stm32f1xx.c` | 不手改。要改就在 CubeMX 里改后重新生成 |
| 课程官方驱动 | `Drivers/DshanMCU-F103/driver_*.c` | 不手改。已知缺陷只记录不修（§10），保持与课程可比 |
| 本工程补充 | `main.c` `crt_stubs.c` `retarget.c` `lab_pins.c` `hanzi_font.c` `lab_oled_hanzi.c` `CMakeLists.txt` `ld/` `startup/` `.clangd` `.vscode/` | 可改。每个文件头都写了"为什么需要它" |

---

## 3. 构建

```bash
# 首次配置（生成 build/ 与 compile_commands.json）
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# 构建
cmake --build build

# 只构建、不改任何状态（CI 友好）
cmake --build build --target clean && cmake --build build
```

产物：`build/f103_hal_driverlab.{elf,hex,bin,map}`。
POST_BUILD 会用 `arm-none-eabi-size -A -x` 打印分节体积，改完代码不必手敲 size。

**实测结果（本机，`LAB_TEST=0`）**：

```text
text  15276    data  100    bss  3176
.isr_vector 268 / .text 14780 / .rodata 220 / .ARM 8 / .data 100 / .bss 1636 / ._user_heap_stack 1540
```

- Flash 占用 = text + data = 15 376 B / 65 536 B = **23.5 %**
- RAM 静态占用 = data + bss = 3 276 B / 20 480 B = **16.0 %**，其中 1 636 B 是真变量，
  1 540 B 是链接脚本里 `._user_heap_stack`（`_Min_Heap_Size` 512 + `_Min_Stack_Size` 1024）的
  **预留区**，不是实际用量。默认 `size` 把这两个段合并显示成 bss 3176，只有 `size -A`
  才看得到 `._user_heap_stack` 是独立段；把预留算成消耗会得出错误的余量结论。

**告警**：0 error / 13 warning，13 条**全部来自课程驱动**，没有一条来自本工程新增文件：

| 文件 | 条数 | 内容 |
| --- | --- | --- |
| `driver_ds18b20.c` | 3 | `-Wparentheses`：`ram[4] & (3<<5) == 0` 的优先级陷阱（§10.1） |
| `driver_oled.c` | 2 | `-Wchar-subscripts`：用有符号 `char` 下标访问 `ascii_font`（§10.2） |
| `driver_oled.c` | 8 | `-Wunused-function`：只在特定显示模式用到的私有函数 |

保留这 13 条告警而不是加 `-Wno-*` 压掉：其中 5 条指向的是**真实缺陷**，另外 8 条是
"驱动自带全套能力但本工程没全用到"的正常现象。

---

## 4. 烧写与调试

### 4.1 命令行

```bash
cmake --build build --target flash
# 展开为：openocd -f ../F103_common/openocd/stlink_f103.cfg \
#          -c "program build/f103_hal_driverlab.elf verify reset exit"
```

`CMakeLists.txt` 用 `find_program(openocd)` 判断：找不到 openocd 时**不注册** `flash` 目标
（而不是注册一个必然失败的目标）。本机查到
`C:/Users/.../xpack-openocd-0.12.0-7/bin/openocd.exe`。

### 4.2 Cursor / VS Code

三个任务（`Ctrl+Shift+B` 默认跑 build）：

| 任务 | 等价命令 |
| --- | --- |
| `F103: configure` | `cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=... -DCMAKE_EXPORT_COMPILE_COMMANDS=ON` |
| `F103: build` | `cmake --build build`（`dependsOn: configure`，`problemMatcher: $gcc`） |
| `F103: clean` | `cmake --build build --target clean` |
| `F103: flash` | `cmake --build build --target flash`（`dependsOn: build`） |

调试：`launch.json` 用 `cortex-debug` + `servertype: openocd`，
`configFiles` 指向 `../F103_common/openocd/stlink_f103.cfg`，`runToEntryPoint: main`，
`preLaunchTask: F103: build`。按 F5 即"构建 → 烧写 → 停在 main"。

---

## 5. 切换测试模块：`LAB_TEST`

`main.c` 的 `Lab_Run()` 用**预处理** `#if LAB_TEST == N` 选模块：

```bash
cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake \
      -DLAB_TEST=12          # 0 是默认值（板载 LED，不需要外接模块）
cmake --build build
```

编号表见 `main.c:Lab_Run()`。要点：

- **必须用 `#if` 而不是运行期 `switch`**。实测：改成 `switch` 后，19 个 `Test` 全部成为
  可能的调用点而不能被回收，`text` 从 10 936 B 涨到 33 928 B。改回 `#if` 后
  `arm-none-eabi-nm` 里只剩被选中的那一个 `*_Test` 符号。
- 每次切换都要重新 configure（`LAB_TEST` 是 `CACHE STRING`，改了才重新编译）。
- 体积随模块增长（实测，2026-10-29 重测，`arm-none-eabi-size` 的 Berkeley `text` 列）：
  `LAB_TEST=0` → 15 276 B；`=12`（OLED）→ 19 648 B；`=18`（MPU6050）→ 21 560 B；
  `=19`（OLED + 16×16 汉字，自加模块）→ 20 584 B。64 KB Flash 装得下任意单个模块。
  分节读数（`size -A -x`）用于算模块代价，见 §5.1。

GC 证据（`LAB_TEST=0`，其余 18 个 `Test` 全部被 `--gc-sections` 回收）：

```text
$ arm-none-eabi-nm --size-sort -S build/f103_hal_driverlab.elf | grep '_Test$'
080024e0 00000028 T Led_Test
```

但也有一条**回收不掉**的：向量表里的 `EXTI15_10_IRQHandler` 永远存在 →
`HAL_GPIO_EXTI_IRQHandler` → `HAL_GPIO_EXTI_Callback`（`driver_irq.c`）→
`IRReceiver_IRQ_Callback` + `RotaryEncoder_IRQ_Callback`。这条链在 `LAB_TEST=0` 时
仍在镜像里（实测符号大小：`IRReceiver_IRQ_Callback` 0x88 = 136 B、
`RotaryEncoder_IRQ_Callback` 0x74 = 116 B，两者合计 252 B；它们再牵进来的定时器辅助函数
成本**没有单独测过**，不做估算）。这是"中断驱动型库"的结构性成本：
只要向量表里有这个中断入口，回调链就必须在镜像里。

### 5.1 自加模块的体积代价（以 16×16 汉字为例）

Berkeley `text` 不能直接相减比较不同模块，因为它把 `.text`/`.rodata`/`.data` 混在一起。
算一个模块的代价要挑**对照组**：`LAB_TEST=19`（OLED + 汉字）与 `LAB_TEST=12`（纯 OLED）
唯一的差别就是 `hanzi_font.c` + `lab_oled_hanzi.c`。

| 构建 | `.text` | `.rodata` | `.data` |
| --- | --- | --- | --- |
| `LAB_TEST=0`（默认，仅 PC13） | 14 780 | 220 | 100 |
| `LAB_TEST=12`（课程 OLED 用例） | 17 088 | 2 284 | 104 |
| `LAB_TEST=18`（MPU6050） | 18 972 | 2 312 | 104 |
| `LAB_TEST=19`（OLED + 汉字） | 17 896 | 2 412 | 108 |
| **19 − 12** | **+808** | **+128** | +4 |

`.rodata` 的 +128 B 里 96 B 是三个字形（3 × 32 B），其余是表项与对齐填充。
按每字 32 B 线性外推：3500 常用字 ≈ 109 KB > 64 KB Flash，所以字库只能走子集
或放到外部 W25Q64（M12）。推导与位序证明见 `notes/F103_HAL入门/Day10_OLED显示汉字与点阵.md`。

**对照组必须换一个理由**：`LAB_TEST=0` 连 OLED 驱动都不会入镜像（`--gc-sections` 回收），
拿它减只能得到"OLED + 汉字"的合计，得不到汉字本身的代价。

---

## 6. 模块 ↔ 驱动 ↔ 引脚 ↔ 谁来配

引脚数据的来源是**驱动源码里的宏定义与注释**，不是猜测。完整对照表（含冲突规则、
HAL 模块依赖）见
[`../../knowledge_base/DShanMCU-F103模块与驱动_速查表.md`](../../knowledge_base/DShanMCU-F103模块与驱动_速查表.md)。
这里只列"谁来配引脚"这一列，因为它是本工程唯一需要补的地方。

| 模块 | 驱动 | 引脚 | 谁配置 |
| --- | --- | --- | --- |
| 板载单色 LED | `driver_led.c` | PC13 | 驱动 `Led_Init()`（CubeMX 的 `gpio.c` 也配了） |
| 按键 K1 | `driver_key.c` | PB14 输入 | 驱动 `Key_Init()` |
| 有源蜂鸣器 | `driver_active_buzzer.c` | PA8 推挽 | 驱动 `ActiveBuzzer_Init()` |
| 无源蜂鸣器 | `driver_passive_buzzer.c` | PA8 / TIM1_CH1 | 驱动 + `tim.c` |
| 红外发射 38 kHz | `driver_ir_sender.c` | PA8 / TIM1_CH1 | `tim.c` |
| 直流电机 DRV8833 | `driver_motor.c` | A: PA4/PA0；B: PB4(TIM3_CH1)/PB15 | 驱动 + `tim.c` |
| 串口 | `driver_uart.c` | PA9/PA10 USART1 | `usart.c:HAL_UART_MspInit` |
| RGB 全彩 LED | `driver_color_led.c` | PA15=G(TIM2_CH1)、PB3=B(CH2)、PA2=R(CH3) | `tim.c` |
| 光敏电阻 | `driver_light_sensor.c` | PA3 = ADC1_IN3 | `adc.c:HAL_ADC_MspInit` |
| OLED SSD1306 | `driver_oled.c` | PB6/PB7 I2C1，从机地址 `0x78` | `i2c.c:HAL_I2C_MspInit` |
| LCD | `driver_lcd.c` | 同 OLED（纯转发封装） | 同上 |
| MPU6050 | `driver_mpu6050.c` | PB6/PB7 I2C1，写地址 `0xD0` | 同上 |
| SPI Flash W25Q64 | `driver_spiflash_w25q64.c` | PA5/PA6/PA7 SPI1 + CS=PB9 | `spi.c` MSP + **`lab_pins.c`（PB9）** |
| HC-SR04 | `driver_ultrasonic_sr04.c` | TRIG=PB9、ECHO=PB8 | **`lab_pins.c`** |
| 红外避障 LM393 | `driver_ir_obstacle.c` | PB13 输入 | **`lab_pins.c`** |
| 旋转编码器 EC11 | `driver_rotary_encoder.c` | S1=PB0、S2=PB1、KEY=PB12（EXTI 上升） | **`lab_pins.c`** |
| 红外接收 1838 | `driver_ir_receiver.c` | PB10（EXTI 双边沿） | **`lab_pins.c`**（驱动内自配置被 `#if 0` 关掉） |
| DS18B20 | `driver_ds18b20.c` | PA1 开漏 + 上拉 | **`lab_pins.c`** |
| DHT11 | `driver_dht11.c` | PA1 开漏 + 上拉 | **`lab_pins.c`** |

### 6.1 为什么要 `lab_pins.c`（一个真实缺口，不是画蛇添足）

驱动源码自己写明了"引脚由 `MX_GPIO_Init` 负责"，例如：

- `driver_ultrasonic_sr04.c`：`Trig PB9, Echo PB8 在MX_GPIO_Init里被初始化为输出/输入引脚了`
- `driver_spiflash_w25q64.c`：`片选信号PB9在MX_GPIO_Init中被配置为输出引脚`
- `driver_ir_obstacle.c`：`PB13在MX_GPIO_Init里被初始化为输入引脚了`
- `driver_rotary_encoder.c`：`PB0,PB1 ... 配置为输入引脚` / `PB12 ... 中断引脚,上升沿触发`
- `driver_dht11.c` / `driver_ds18b20.c`：要求数据引脚预先配成 `open drain, pull-up`

本工程的外设初始化取自 `02_FreeRTOS程序/01_文档配套的源码/01_freertos_template`，
它的 `gpio.c` **只配置了 PC13**（已核实：全文只有 `GPIOC` + `GPIO_PIN_13` 一处 `HAL_GPIO_Init`）。
课程里也没有任何一份 CubeMX 工程把这 20 个模块的引脚一次配齐——
`03_nwatch_dshanmcu-f103_all` 是手表产品，只配它用到的引脚，而且缺 PB13。

所以这些驱动在"模板 + 驱动"的组合下会读到未初始化的引脚。`lab_pins.c` 就是补这一段：
**8 个纯 GPIO + EXTI15_10 的 NVIC 使能**，全部写在一个文件里，不属于 CubeMX 生成物，
重新生成 `Core/` 不会覆盖。代价是 +288 B text（15 276 − 14 988）。

一条容易漏的：EXTI 线号 10 只有一个，映射到哪个端口由 `AFIO->EXTICR` 决定。
`HAL_GPIO_Init(GPIOB, PIN_10)` 会写这条映射（`stm32f1xx_hal_gpio.c:292-295` 已核对源码）。
**不配这一步**，EXTI10 默认落在端口 A，而 PA10 正是 USART1_RX —— 串口每收一个字节
都会触发一次"红外中断"。是否真的会误触发未在真板上抓过波形，但配置必要性有源码与反汇编支撑。

---

## 7. 引脚冲突：三条规则与它们的物理原因

课程第 4 章给的冲突表是三条：

| 冲突 | 模块 | 规则 | 本工程复核到的物理原因 |
| --- | --- | --- | --- |
| M2 / M10 | DS18B20 / DHT11 | 不能同时接 | 两者都用 **PA1**，单总线无法共享 |
| M4 / M9 | 红外发射 / 无源蜂鸣器 | 不能同时接 | 两者都用 **PA8 的 TIM1_CH1**，一个定时器通道只能有一个占空比 |
| M6 / M12 | 超声波 / SPI Flash | 可以同时接，**不能同时访问** | 两者都用 **PB9**（SR04 的 TRIG、W25Q64 的 CS） |

第三条的"能接不能同时访问"值得单独解释：这是**一根线两种空闲电平**的矛盾。
W25Q64 的片选低有效，空闲应为高；SR04 的 TRIG 靠 ≥10 µs 高脉冲触发，空闲应为低。
`lab_pins.c` 取低（与课程 `03_nwatch` 一致）：SR04 才能正常工作；flash 在 CS 低、
SCLK 静止时只是被选中而收不到任何位，不会误执行命令。这条是**源码推论**，
未用逻辑分析仪确认。

模型编号只采用课程冲突表里出现过的 M 号；表中没有的模块只写名称，不编造编号。

---

## 8. Cursor 配置：为什么是 clangd 而不是 cpptools

Cursor 默认走 **Open VSX** 扩展市场，而 `ms-vscode.cpptools`（C/C++ 官方扩展）
**不在** Open VSX 上。所以在 Cursor 里想有跳转/补全/错误提示，实际可行的是 **clangd**。

坑在"clangd 凭什么信任交叉编译器"。clangd 默认只对系统编译器用内置的搜索路径；
对 `arm-none-eabi-gcc` 需要显式授权，否则所有 CMSIS 头都会报"找不到"：

- `.vscode/settings.json` 里的 `--query-driver` 支持**逗号分隔的 glob**，
  写成 `C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/*/bin/arm-none-eabi-*`
  可以跨工具链版本不换配置。
- `.clangd` 的 `CompileFlags.Compiler` 必须是**绝对路径**，且要能匹配上某个 `query-driver` glob，
  两者不一致时 clangd 会忽略你的指定。

第二个坑：**Windows 上 CMake 4.0 会把 `compile_commands.json` 里的编译器写成 8.3 短路径**，
例如 `C:\PROGRA~2\ARMGNU~1\14EFD8~1.2RE\bin\AR19DD~1.EXE`。
clangd 靠"可执行文件名像不像编译器"来做驱动启发式判断，短名猜不出来，
于是就算 `query-driver` 写对了也不生效。所以 `.clangd` 里必须显式写 `Compiler:` 绝对路径。

另外两个必要项：

- `C_Cpp.intelliSenseEngine: "disabled"`：装了 cpptools 时它和 clangd 会打架。
- `cmake.configureOnOpen: false`：否则每次开工程都触发一次 configure，
  在没装 CMake Tools 的环境里会持续报红。

调试端用 `cortex-debug`（在 Open VSX 上有），`configFiles` 对应 `-f` 参数、
`openOCDLaunchCommands` 对应 `-c` 参数——这两个字段名最容易写错。

---

## 9. 验证状态

| 项 | 状态 | 证据 |
| --- | --- | --- |
| GCC/CMake 完整构建 | **实测** | 0 error / 13 warning（13 条全在上游驱动），`text 15276 / data 100 / bss 3176` |
| 向量表布局 | **实测** | `bin` 前 8 字节 = `00 50 00 20 05 28 00 08` → SP=`0x20005000`、PC=`0x08002804`(Thumb)；`nm` 中 `Reset_Handler` 正在 `0x08002804` |
| `--gc-sections` 真的在裁 | **实测** | `LAB_TEST=0` 时 `nm` 里只有 `Led_Test`，其余 18 个 Test 符号不存在 |
| `#if` 优于运行期 `switch` | **实测** | 改 `switch` 后 text 10 936 → 33 928，改回 `#if` 复原 |
| printf → USART1 | **仅源码/符号级** | `nm` 中存在 `_write`、**不存在** `fputc`（见 §10.3）；串口波形未抓 |
| `lab_pins.c` 的引脚配置 | **构建级 + 反汇编核对** | `Lab_Pins_Init` 在镜像内（`0x08000ae0`）；EXTI 映射由 `HAL_GPIO_Init` 写 `AFIO->EXTICR` 已核对 HAL 源码 |
| 烧写 / 调试链路 | **未验证** | 本机无 ST-Link 在手，`flash` 目标只做了"命令能拼出来"级别的检查 |
| 任一模块的板上现象 | **未验证** | **全部** 20 个模块的功能现象未在本机观察过 |

**本机没有外部模块**，因此下面这些都无法测：OLED 是否点亮、DHT11 是否回数据、
电机是否转、编码器计数是否与实际旋转一致、红外是否解出用户码、W25Q64 的 JEDEC ID 是否为
`EF 40 17`、MPU6050 的 `WHO_AM_I` 是否为 `0x68`。

---

## 10. 已知缺陷与差异（只记录，不擅改）

### 10.1 `driver_ds18b20.c` 的优先级陷阱（真实缺陷）

```c
if (ram[4] & (3<<5) == 0)      /* 463/465/467 行附近 */
```

`==` 的优先级高于 `&`，实际解析为 `ram[4] & ((3<<5) == 0)` = `ram[4] & 0`，
**恒为 0**。后果是 9 bit / 10 bit / 11 bit 分辨率的分支永远不会进入，
无论配置寄存器写的是什么都会走 12 bit 分支。要修就得加括号：`(ram[4] & (3<<5)) == 0`。
不修的理由：这是上游课程代码，改了就失去与教材的逐行可比性；缺陷已在此记账。

### 10.2 `driver_oled.c` 的 `char` 下标越界（真实缺陷）

```c
void OLED_PutChar(uint8_t x, uint8_t y, char c)   // 约 542 行
```

用有符号 `char` 去索引 `ascii_font[128][16]`。`c >= 0x80` 时下标为负，越界读。
编译器已用 `-Wchar-subscripts` 报 2 条。修法是把形参改成 `uint8_t` 并在调用处显式转换。
本工程不显示非 ASCII 字符时不会触发，但这是一个真实的 UB。

### 10.3 `driver_uart.c` 的 `fputc` 在 newlib 下是死代码

`driver_uart.c` 实现了 `fputc`/`fgetc`——这是 **Keil MDK 传统**的重定向方式
（MicroLIB 的 `printf` 走 `fputc`）。newlib 的 `printf` 走的是 `_write`，
所以本工程另写了 `Core/Src/retarget.c`。

实测证据：`nm` 里**没有** `fputc` 符号（被 `--gc-sections` 回收），而有 `_write`。
也就是说这两套重定向同时存在时，真正生效的是 `_write`，`driver_uart.c` 的
`fputc`/`fgetc` 只在该文件自己的 `Test` 里被直接调用时才会留下。
`driver_uart.c` 的 `UART_Init()` 是空函数（串口初始化全在 `usart.c`）。

### 10.4 `driver_ir_receiver.c` 的注释笔误

`IRReceiver_Init()` 里的注释写 `PA10在MX_GPIO_Init()中已经被配置为双边沿触发`，
但它自己 `#if 0` 掉的自配置代码是 `GPIO_PIN_10` + `GPIOB`，而且 `driver_irq.c`
按 `GPIO_PIN_10` 分发、PA10 已被 USART1_RX 占用。**PA10 是注释笔误，实际是 PB10。**
本工程按 PB10 配置（`lab_pins.c`）。

### 10.5 本工程与课程配置的已知差异

| 项 | 课程 | 本工程 | 理由 |
| --- | --- | --- | --- |
| 构建系统 | Keil MDK（ARMCC + `.sct`） | CMake + Ninja + GCC（`.ld`） | 要能在 Linux/WSL 与 CI 上构建 |
| 启动文件 | `startup_stm32f103xb.s`（ARM 汇编） | 同名但 GCC 语法版 | 汇编器不同，不能共用 |
| 重定向 | `fputc`（MicroLIB） | `_write`（newlib，`retarget.c`） | §10.3 |
| `_sbrk` 等 syscall | ARMCC 自带 | `crt_stubs.c` 手写 | `-nostartfiles` 后需自补 |
| PB12 上拉 | `NOPULL`（推断板上有外部上拉） | `PULLUP` | 有外部上拉时并联无害，没有时它是唯一保证。**未在真板验证** |
| PB13 上拉 | 注释未提 | `PULLUP` | LM393 开集输出，不加会上拉会读到悬空。**推断，真板待验** |
| `EXTI15_10` 优先级 | `(5, 0)` | `(0, 0)` | 裸机无 RTOS，不涉及 FreeRTOS 的优先级门槛 |

### 10.6 `driver_timer.c` 与 HAL 时基共用 TIM4（重要依赖）

`driver_timer.c` 的 `udelay()`/`mdelay()` 直接读 `htim4` 的计数器做忙等，
而 `stm32f1xx_hal_timebase_tim.c` 把 HAL 时基也切到了 TIM4（1 MHz、1 ms 更新）。
这不是冲突而是**耦合**：`udelay` 的正确性依赖"TIM4 被配成 1 MHz 计数"。
换成 SysTick 做时基，`driver_timer.c` 会算错延时。课程用 TIM4 做时基正是为此。

---

## 11. 常见故障

| 现象 | 原因 | 处理 |
| --- | --- | --- |
| 链接期 `undefined reference to 'HAL_xxx'` | `stm32f1xx_hal_conf.h` 启用了某模块，但 `CMakeLists.txt` 的 `HAL_SOURCES` 里没有对应 `.c` | 两边对齐。加的模块还要连带 `_ex.c`（如 `HAL_GPIO_MODULE_ENABLED` 需要 `stm32f1xx_hal_gpio_ex.c`） |
| `stm32f1xx_hal_gpio_ex.h: No such file or directory` | `Drivers/.../Inc/` 拷不全 | 整目录同步，不要挑文件 |
| `multiple definition of 'ascii_font'` | 把 `ascii_font.c` 当独立编译单元加入 | 它是被 `driver_oled.c` `#include` 的数据文件，**不要**加进 `DRIVER_SOURCES` |
| 4 条 `_close/_lseek/_read/_write ... will always fail` | newlib 的 `libnosys` 桩 | 用 `-nostartfiles` + `crt_stubs.c` + `retarget.c`（仓内既有做法） |
| clangd 所有头文件报红 | 没配 `--query-driver`，或 `compile_commands.json` 里是 8.3 短路径 | 见 §8，`.clangd` 写 `Compiler:` 绝对路径 |
| 改完 `LAB_TEST` 体积没变 | `LAB_TEST` 是 cache 变量，改完需重新 configure | 重跑 `cmake -S . -B build ... -DLAB_TEST=N` |
| 驱动读到恒为 0 / 悬空的值 | 引脚配置在 `MX_GPIO_Init` 里，而模板 `gpio.c` 只有 PC13 | 见 §6.1，`lab_pins.c` 已补 |

---

## 12. 证据来源

- **方法论文档**：[`notes/第三方驱动库迁移GCC_四处必改点与责任边界.md`](../../notes/第三方驱动库迁移GCC_四处必改点与责任边界.md)
  —— 本文讲"这个工程怎么建"，那份讲"下一个只在 Keil 下验证过的库该按什么顺序验"
- 构建：`cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake` → `cmake --build build`，本机 exit=0，13 warning / 0 error；原始输出 [`logs/build_2026-10-29.log`](logs/build_2026-10-29.log)
- 体积与符号：`arm-none-eabi-size {,-A} build/f103_hal_driverlab.elf`、`arm-none-eabi-nm --size-sort -S`
- 向量表：`xxd -l 8 build/f103_hal_driverlab.bin` → `0050 0020 0528 0008`
- 引脚：`Drivers/DshanMCU-F103/driver_*.c` 的宏定义与 `MX_GPIO_Init` 相关注释（逐条抄录，未推断）
- HAL 源码：`Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio.c:292-295`（EXTI 端口映射）
- 课程原始工程：`Linux_WEIDS/DShanMCU-F103/5_程序源码/`
  - `01_单片机程序/01_文档配套的源码/01_template/`（最小模板）
  - `02_FreeRTOS程序/01_文档配套的源码/01_freertos_template/`（本工程 `Core/` 的来源）
  - `02_FreeRTOS程序/03_参考的源码/DshanMCU-F103/03_nwatch_dshanmcu-f103_all/`（引脚配置的主要参照）
- 模块编号与冲突规则：课程第 4 章冲突表（只有 M2/M4/M6/M9/M10/M12 六个编号有据可查）
