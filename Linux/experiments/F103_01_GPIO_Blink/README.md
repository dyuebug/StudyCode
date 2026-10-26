# F103_01：寄存器级 LED 点灯

## 学习目标

用最少的代码打通 F103 开发链路的四个环节，把后续实验的共同前置条件一次性验证掉：

1. 交叉编译：`arm-none-eabi-gcc` + CMake + Ninja 生成可执行 ELF。
2. 链接布局：自撰链接脚本给出的向量表、`_estack`、`.data` 的 LMA/VMA 是否符合 Cortex-M3 复位约定。
3. 时钟认知：默认 HSI 8 MHz 下的 `SystemCoreClock` 从哪来、为什么不能写死。
4. GPIO 输出：`RCC->APB2ENR` → `GPIOC->CRH` → `GPIOC->BSRR` 的完整寄存器路径。

刻意不使用 HAL、不使用中断。排障变量越少，链路验证越可信。

## 硬件

| 项 | 值 |
| --- | --- |
| 芯片 | STM32F103C8T6（64 KB Flash @0x08000000，20 KB SRAM @0x20000000） |
| 开发板 | DShanMCU-103 |
| LED | 绿色 LED，PC13 经 1 K 电阻接 3.3 V，**低电平点亮** |
| 烧写器 | ST-Link V2（SWD：SWDIO→PA13，SWCLK→PA14，GND，3.3 V） |

LED 极性的依据是课程手册《HAL 快速入门与项目实战（基于 DshanMCU-103）》§5.3.1：
「PC13 如果是 3.3 V……LED 熄灭。PC13 如果是 0 V，那么电阻和 LED 两端电压差为 3.3 V，LED 被点亮。」

## 构建

```bash
cd Linux/experiments/F103_01_GPIO_Blink
cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake
cmake --build build
```

产物：`build/f103_01_gpio_blink.elf` / `.hex` / `.bin`，以及 `build/f103_01_gpio_blink.map`。

## 烧写

```bash
cmake --build build --target flash
```

等价的手工命令：

```bash
openocd -f ../F103_common/openocd/stlink_f103.cfg \
        -c "program build/f103_01_gpio_blink.elf verify reset exit"
```

也可以把 `build/f103_01_gpio_blink.hex` 交给 STM32CubeProgrammer 或 Keil 烧写。

## 预期现象

LED 以 1 Hz 闪烁（亮 500 ms、灭 500 ms）。

## 实测记录

**构建实测**（2026-10-25，arm-none-eabi-gcc 14.2.1 + CMake 4.0.3 + Ninja 1.13.2，Windows 11）：

```text
Memory region         Used Size  Region Size  %age Used
           FLASH:         772 B        64 KB      1.18%
             RAM:        1544 B        20 KB      7.54%
   text    data     bss     dec     hex filename
    768       4    1540    2312     908 f103_01_gpio_blink.elf
```

零编译告警、零链接告警。`RAM` 的 1544 B 中 1536 B 是链接脚本为堆栈预留的哨兵区，实际变量只有 4 B `.data` + 4 B `.bss`。

其中 16 B Flash 与 4 B bss 并非本实验代码：`f103_tick.c` 为中断模式提供了 `SysTick_Handler`，
它被向量表引用、无法被 `--gc-sections` 回收，所以只用轮询模式的实验也要付这份开销。
未使用的 `f103_tick_init_irq`（0x34）和 `f103_tick_now_ms`（0xC）已确认被回收（map 中地址为 0）。

**布局自检**：

| 检查项 | 实测值 | 应有值 |
| --- | --- | --- |
| `.isr_vector` 首字（初始 MSP） | `0x20005000` | SRAM 末尾 = 0x20000000 + 20 KB |
| 向量表第二字（Reset_Handler） | `0x080001AD` | Flash 内地址，Thumb 位为 1 |
| `.data` VMA / LMA | `0x20000000` / `0x0800038C` | RAM 运行、Flash 存放 |
| `libnosys` / `stdio` 成员 | 未拉入 | 已用 `-nostartfiles` 排除 crt0 |

`main` 反汇编确认写的是 `0x40011010`（`GPIOC->BSRR`），值为 `0x20000000`（`BR13`，点亮）与 `0x2000`（`BS13`，熄灭），与源码意图一致。

**未验证**：真实板卡上的闪烁现象与实测周期。需要接 ST-Link 烧写后目视确认；`SystemCoreClock` 若与真实 HCLK 不符，闪烁周期会等比偏移（这正是本实验要暴露的时钟问题）。

## 关键实现点

### 1. 为什么必须先开 `RCC->APB2ENR` 的 `IOPCEN`

外设寄存器在时钟关闭时不响应写入，而且不报错。漏掉这一步的现象是「代码逻辑正确、编译通过、灯纹丝不动」，是新手最常见的一类无效排查方向。

### 2. 为什么 `CRH` 要「先清零再置位」

`GPIOC->CRH` 复位值是 `0x4444_4444`（每脚浮空输入，`CNF=01`、`MODE=00`）。若只置 `MODE` 位而不清 `CNF`，引脚会停留在「输入模式下的某种复用配置」，输出不生效。

### 3. 为什么用 `BSRR` 而不是 `ODR`

`BSRR` 是只写寄存器，置位/复位互不干扰；`ODR` 需要读-改-写，会误伤同端口的其他引脚，且在多处并发写同一端口时产生竞争。

### 4. 为什么要显式调用 `SystemCoreClockUpdate()`

CubeMX 版本的 `vendor/src/system_stm32f1xx.c` 中，`SystemInit()` 只处理向量表重定位，**不设置 `SystemCoreClock`**。该变量停在初值 `16000000`，而复位后实际是 HSI 8 MHz。直接用会让 `SysTick` 的 1 ms 变成 2 ms，闪烁整体慢一倍。`SystemCoreClockUpdate()` 会读 `RCC->CFGR` 反推真实频率。

这段逻辑已在实验 02 时提取到 `F103_common/src/f103_tick.c`，本实验通过 `f103_tick_init()` 调用。

## 常见错误

| 现象 | 可能原因 | 排查 |
| --- | --- | --- |
| 编译报 `stm32f1xx.h: No such file` | 未传 `CMAKE_TOOLCHAIN_FILE`，或用错 include 路径 | 确认 `-DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake` |
| 找不到 `arm-none-eabi-gcc` | 工具链不在 `PATH` | 传 `-DARM_TOOLCHAIN_DIR="C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/14.2 rel1/bin"` |
| `region RAM overflowed` | 全局变量/大数组超出 20 KB | 看 `.map` 的 `.bss`；注意 1536 B 是预留堆栈 |
| 烧写报 `target not halted` | SWD 接线、供电或速率问题 | `openocd` 配置里把 `adapter speed` 降到 480；确认 BOOT0 接地 |
| 灯常亮或常灭不闪 | 时钟未开、`CRH` 未清零、或板上 LED 极性相反 | 先在 `main` 开头点灯一次（不延时）确认通路，再查 `SysTick` |
| 闪烁周期明显不对 | `SystemCoreClock` 与真实 HCLK 不符 | 确认调用了 `SystemCoreClockUpdate()`；后续配置 PLL 后必须重新调用 |

## 下一步

1. 接板烧写，记录实际闪烁周期，把「未验证」项补成实测。
2. 按键输入见 [`F103_02_GPIO_Input`](../F103_02_GPIO_Input/README.md)：PB14 输入、消抖与边沿检测。
3. 把 SysTick 的轮询延时换成 SysTick 中断，引入 NVIC，为 GPIO 外部中断铺路。
4. 需要 HAL 时再向 `F103_common/vendor/` 增补 HAL 模块，见 `../F103_common/vendor/README.md` 的边界说明。

## 已知限制

- 未配置 PLL，全部代码运行在 8 MHz HSI 上；72 MHz 需要额外写 `RCC->CFGR` 并同步更新 Flash 等待周期。
- `delay_ms` 是忙等，不节能，也不提供精度保证；精确计时留给定时器实验。
- 链接脚本按 64 KB / 20 KB 写死，换型号需同步修改。
