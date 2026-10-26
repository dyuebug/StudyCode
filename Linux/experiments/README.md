# 单主题实验

每个实验只验证一个主要概念，建议目录结构：

```text
experiments/主题名称/
├── README.md
├── source/
├── build/
└── evidence/
```

实验说明必须写清前置环境、构建命令、运行命令、预期结果、实际结果、错误路径和已知限制。

## 驱动入门实验准备

- [环境与验证指南](驱动入门_环境与验证指南.md)：先确认平台能力、内核与工具链，再进行只打印模块实验；板级代码仅用于匹配的目标系统。
- 各 Day 的 README 末尾「验证状态」写明**已实测**与**未验证**的边界：Day2–Day9 有 QEMU 运行证据（见 `../tests/qemu-vexpress/RESULT.md`）；Day10–Day14 为 QEMU 实测的等价机制 + 本日驱动的交叉编译验证；真实外设行为一律留待目标板。
- 实际命令输出写入 `../daily_logs/`，不在材料准备阶段伪造 evidence。
- [每日任务导航](../daily_tasks/README.md) 区分无板必做和条件实验，原始课程代码保留不变。

## MCU 分支实验（STM32F103）

与上面的 Linux 驱动实验并列，不共享构建系统。公共设施在 [`F103_common/`](F103_common/README.md)：
自撰链接脚本、裁剪后的 CMSIS 子集、CMake 工具链文件、OpenOCD 烧写配置。

| 实验 | 主题 | 验证状态 |
| --- | --- | --- |
| [`F103_01_GPIO_Blink`](F103_01_GPIO_Blink/README.md) | 寄存器级 LED 点灯、SysTick 轮询延时 | 构建实测（arm-none-eabi-gcc 14.2.1）；板级现象未验证 |
| [`F103_02_GPIO_Input`](F103_02_GPIO_Input/README.md) | 按键输入、输入上下拉机制、软件消抖与边沿检测 | 构建实测 + 反汇编核对；按键手感未验证 |
| [`F103_03_EXTI_Key`](F103_03_EXTI_Key/README.md) | EXTI14 + NVIC、挂起位清除、中断与主循环分工 | 构建实测 + 向量表与寄存器核对；中断触发未验证 |
| [`F103_04_UART_Echo`](F103_04_UART_Echo/README.md) | USART1 轮询收发、BRR 与时钟依赖、printf 重定向与缓冲策略 | 构建实测 + 寄存器路径核对；串口收发未验证 |
| [`F103_05_UART_IRQ_Ring`](F103_05_UART_IRQ_Ring/README.md) | USART1 中断接收、SPSC 无锁环形缓冲、ORE 与丢弃计数 | 构建实测 + 向量表与 NVIC 核对；中断触发未验证 |
| [`F103_06_TIM_PWM_RGB`](F103_06_TIM_PWM_RGB/README.md) | TIM2 三通道 PWM、JTAG 引脚释放、预装载与极性 | 构建实测 + 寄存器路径核对；亮度与波形未验证 |
| [`F103_07_I2C_MPU6050`](F103_07_I2C_MPU6050/README.md) | I2C1 主机轮询、单字节读时序、超时与总线恢复 | 构建实测 + 寄存器与指令顺序核对；需外接模块，I2C 未验证 |
| [`F103_08_SPI_Flash`](F103_08_SPI_Flash/README.md) | SPI1 主机、模式 0 与 NSS 配置、W25Q 指令协议与 WIP 轮询 | 构建实测 + 寄存器与指令常量核对；需外接模块，SPI 未验证 |

约定：

- 目录名用 ASCII，避免 CMake/Ninja 在 Windows 控制台下的中文路径问题；主题用中文写在 README 里。
- 每个实验一个独立 CMake 工程（各自 `build/`），不合并成顶层多目标工程，便于单独烧写和排障。
- 新增外设时先补 `F103_common/vendor/` 的必要 HAL 模块，再写实验代码，不预先引入整套 HAL。

