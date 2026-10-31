# 学习笔记

用于保存概念笔记、源码阅读笔记、机制对比和问题复盘。

建议按以下结构记录：

```text
主题
-> 前置概念
-> 源码入口
-> 数据流或调用链
-> 正常路径
-> 异常路径
-> 当前验证状态
-> 待解决问题
```

源码笔记必须注明文件路径、入口函数、编译产物和实际验证范围。

## 驱动入门材料

- [Day2～Day14 练习与验收工作簿](驱动入门_Day2-Day14_练习与验收工作簿.md)：基础题、改错题、综合题、每日记录卡和两次阶段验收；不预填答案或实测结果。
- [每日任务导航](../daily_tasks/README.md)：按 Day 找已有讲义与实际源码入口。

## F103 HAL 驱动入门材料（`F103_HAL入门/`）

对照官方教材《HAL快速入门与项目实战（基于DshanMCU-103）》逐节拆解，每篇按同一模板组织：
核心概念 → 接口卡片 → 最小实验 → 编译运行 → 结果观察 → 陷阱 → 排查 → 练习 → 验收 → 验证记录。
实验载体是 `experiments/F103_HAL_DriverLab/`（CubeMX 初始化 + 课程驱动 + GCC/CMake 构建）。

| 篇 | 主题 | 教材章节 | 工程落点 |
| --- | --- | --- | --- |
| [Day01](F103_HAL入门/Day01_开发环境与工具链_从Keil到GCC.md) | 开发环境与工具链：从 Keil 到 GCC | 2–3 | CMake/ld/启动文件 |
| [Day02](F103_HAL入门/Day02_芯片与GPIO模块_从寄存器到HAL.md) | 芯片与 GPIO 模块：从寄存器到 HAL | 5.1–5.5 | `gpio.c`、`driver_led.c` |
| [Day03](F103_HAL入门/Day03_C语言基础补充_指针与结构体访问硬件.md) | C 语言基础补充：指针与结构体访问硬件 | 5.6 | CMSIS `typedef struct` |
| [Day04](F103_HAL入门/Day04_GPIO工作模式与HAL库彻底分析.md) | GPIO 工作模式与 HAL 库彻底分析 | 5.7–5.8 | `HAL_GPIO_Init` 源码 |
| [Day05](F103_HAL入门/Day05_中断体系与GPIO中断编程.md) | 中断体系与 GPIO 中断编程 | 6.1–6.3 | `EXTI`、`driver_key.c` |
| [Day06](F103_HAL入门/Day06_OLED调试与环形缓冲区.md) | OLED 调试与环形缓冲区 | 6.4–6.5 | `driver_oled.c`（8×16 ASCII） |
| [Day07](F103_HAL入门/Day07_UART协议与HAL编程.md) | UART 协议与 HAL 编程 | 7.1–7.3 | `usart.c`、`driver_uart.c` |
| [Day08](F103_HAL入门/Day08_UART_IDLE中断与stdio重定向.md) | UART IDLE 中断与 stdio 重定向 | 7.4–7.5 | `retarget.c` |
| [Day09](F103_HAL入门/Day09_I2C协议与HAL编程.md) | I2C 协议与 HAL 编程 | 8.1–8.4 | `i2c.c`、`driver_mpu6050.c` |
| [Day10](F103_HAL入门/Day10_OLED显示汉字与点阵.md) | OLED 显示汉字与点阵 | 8.5 | 自加 `hanzi_font.c`、`lab_oled_hanzi.c`（`LAB_TEST=19`） |
| [Day11](F103_HAL入门/Day11_SPI协议与HAL编程.md) | SPI 协议与 HAL 编程 | 9.1–9.4 | `spi.c`、`lab_spi/lab_spi_cr1.c`（CR1 复刻与分频表） |
| [Day12](F103_HAL入门/Day12_SPI_Flash编程.md) | SPI Flash 编程（W25Q64） | 9.5 | `driver_spiflash_w25q64.c`、`lab_spi/lab_w25q64_defects.c`（页回卷与三处缺陷） |
| [Day13](F103_HAL入门/Day13_定时器.md) | 定时器与 PWM | 10 | `tim.c`、`stm32f1xx_hal_timebase_tim.c`、`lab_timer/lab_timer_calc.c`（九段算术与语义校验） |
| [Day14](F103_HAL入门/Day14_驱动库契约与19模块总复盘.md) | 驱动库契约与 19 模块总复盘 | 11 | `lab_contract/scan_drivers.sh`（20 个头文件 / 75 个函数的契约扫描与冲突对账） |
| [Day15](F103_HAL入门/Day15_显示与交互.md) | 显示与交互：LED / OLED / 按键 / RGB | 11.3、11.4、11.5、11.15 | `driver_led.c`、`driver_oled.c`、`driver_key.c`、`driver_rgb.c`；`lab_pins.c` 的 EXTI 端口映射 |
| [Day16](F103_HAL入门/Day16_声音与执行器.md) | 声音与执行器：有源/无源蜂鸣器 / 直流电机 | 11.6、11.7、11.19 | `htim1`/CH1 共享、`OCPolarity=LOW` 的占空比口径、PWM 调速与方向 |
| [Day17](F103_HAL入门/Day17_传感器采集.md) | 传感器采集：DHT11 / DS18B20 / 光敏 / MPU6050 | 11.8、11.9、11.16、11.17 | `lab_sensor/`（时序与协议复算）、单总线临界区、`udelay` 依赖 |
| [Day18](F103_HAL入门/Day18_测距与位置.md) | 测距与位置：红外避障 / 超声波 / 旋转编码器 | 11.10、11.11、11.12 | `lab_motion/lab_motion_timing.c`（27 判据：时基回绕、相位扫描、除零、死代码） |
| [Day19](F103_HAL入门/Day19_红外与存储.md) | 红外收发与 SPI FLASH：红外接收 / 红外发射 / W25Q64 | 11.13、11.14、11.18 | `lab_ir/lab_ir_w25q64.c`（34 判据：68 边沿记账、坏时基不可解、`Erase` 的 `&&`、`WaitReady` 超时返回成功） |
| [Day20](F103_HAL入门/Day20_逻辑分析仪项目拆解.md) | 逻辑分析仪项目拆解 | 第 4 章 | 主机端 54 项断言 + 六节算术 `assert`（采样率/存储/带宽/触发/协议解码） |

Day01–Day20 已全部完成，覆盖教材第 2–11 章（第 4 章由 Day20 补，第 11 章的 17 个模块小节由 Day15–Day19 细拆）。
三组主机端可复算载体：`lab_ir/`、`lab_motion/`、`lab_sensor/`，各自 `./build.sh` 一键构建 + 判据 + 负向验证。

## FreeRTOS 入门材料（`FreeRTOS入门/`）

对照 FreeRTOS 官方教程（19 章）逐章拆解，模板与 F103 HAL 系列一致。
证据落点是仓库内已实测的 QEMU/真板实验（`experiments/F103_1*_FreeRTOS_*/`）与 `tests/qemu-stm32/logs/`。

| 篇 | 主题 | 教材章节 | 证据落点 |
| --- | --- | --- | --- |
| [Day01](FreeRTOS入门/Day01_裸机程序设计模式与多任务.md) | 裸机程序设计模式与多任务 | 第 2 章 | 四种裸机模式的缺陷推理（教材原文），未上板 |
| [Day02](FreeRTOS入门/Day02_源码结构与内存管理.md) | 源码结构与内存管理 | 第 7–8 章 | 仓库文件清单与 `FreeRTOSConfig.h` 配置项源码核对；heap_4 行为 |
| [Day03](FreeRTOS入门/Day03_任务管理.md) | 任务管理 | 第 9 章 | `exp10`：任务创建/优先级/延时/栈水位 |
| [Day04](FreeRTOS入门/Day04_同步互斥与通信总览.md) | 同步互斥与通信总览 | 第 10 章 | 总览章，无新 API；对比表的每个边界指向后续各篇的实测 |
| [Day05](FreeRTOS入门/Day05_队列与队列集.md) | 队列与队列集 | 第 11 章 | `exp10_freertos.log`：生产/消费计数、阻塞与超时 |
| [Day06](FreeRTOS入门/Day06_信号量.md) | 信号量 | 第 12 章 | `exp12_sync.log`：二值/计数、`xSemaphoreGiveFromISR`、达到上限后 give 失败 |
| [Day07](FreeRTOS入门/Day07_互斥量.md) | 互斥量 | 第 13 章 | `exp16_priority_inversion.log`：优先级继承对照（403 vs 104 tick） |
| [Day08](FreeRTOS入门/Day08_事件组与任务通知.md) | 事件组与任务通知 | 第 14–15 章 | `exp12_sync.log`：`wait_all` 与超时路径 |
| [Day09](FreeRTOS入门/Day09_软件定时器.md) | 软件定时器 | 第 16 章 | `exp13_timers.log`：`in_daemon=1`、单次/周期、drift、追赶补发 |
| [Day10](FreeRTOS入门/Day10_中断管理与资源管理.md) | 中断管理、资源管理与调试 | 第 17–19 章 | `exp15_stackoverflow.log`：栈溢出 paint 半边、高水位、`FromISR` 边界 |

Day01–Day10 已全部完成（教材 19 章全覆盖）。栈溢出只覆盖 paint 半边，MPU 隔离与真板 `IWDG` 未验证；
各篇「验证记录」逐条列清了实测 / 源码核对 / 未验证三档。


## 阶段 7 主线材料

- [航空转嵌入式路线图](航空转嵌入式路线图.md)：12 周计划、验收缺口与面试表述口径。
- [模拟面试材料（证据版）](航空转嵌入式_模拟面试材料.md)：60 秒自我介绍、5 分钟项目介绍、架构与时序口头版、
  两个故障定位故事、一次有读数的性能对照、一次有反例的取舍、局限性清单与"不能说"口径纠正表。
  只用仓库里有证据的条目，硬件类一律标"待真板"。
- [STM32MP157 异构系统：资料盘点与能力边界](STM32MP157异构系统_资料盘点与边界.md)：25 项资料的实况
  （CM4 裸机例程 22 个、A7/M4 通信两个例程**只有 CM4 工程骨架、无应用源码**）、
  唯一实体源码 `rpmsg_client_sample.c` 107 行的驱动模型与五个坑（通道名 ≠ 模块名、`rpmsg_send` 静默丢包、
  `KERN_DEBUG` 默认看不见、`.ko` 与内核树必须配对）、以及与教材抽取文本同源的限制——**整个资料目录被
  `.gitignore` 忽略，引用不可复核**。源码核对，异构通信零验证，明确写了"现在不该做什么"。

## 方法论笔记（按"怎么判断"组织，不按知识点罗列）

- [BSP 启动参数排查方法论](BSP启动参数_排查方法论.md)：把"bootloader 参数错"与"内核配置错"分开的四道门、
  两个 `Unable to mount root fs` 的区分算术（`unknown-block(0,0)` vs 真实主次设备号）、
  五个容易踩的坑（initramfs 让 `root=` 失效、`console=` 无失败模式、panic 退出码 124、
  `init=` 错反而是内核最健康的一组）。实测来源 `BSP_05` 六组对照。
- [嵌入式内存预算：三种余量口径](嵌入式内存预算_三种余量口径.md)：链接器尾部余量 / FreeRTOS 动态余量 /
  链接脚本静态预留三者不能混读；`size` 的 `bss` 含堆栈预留（6452 vs 实际 4912）；
  加功能的四步核算（估字节 → 改哪个量 → 先算链接 → 别借主栈的钱）。实测来源 `aviation-node` 的
  `size`/`nm` 与 `[stat]` 日志，方案成本为算术推算。
- [第三方驱动库迁到 GCC：四处必改点与"初始化责任边界"](第三方驱动库迁移GCC_四处必改点与责任边界.md)：
  启动文件/链接脚本/crt 桩/`printf` 重定向四处，每处的**证据方式**（为什么"能跑"不算证明）；
  "构建绿 ≠ 配置对"、"向量表可达的链裁不掉"、"EXTI 线号只有一个端口"三条代码上看不出的坑；
  从库注释反推初始化依赖清单的五步方法。构建读数实测，引脚依赖源码核对，板上现象未验证。
