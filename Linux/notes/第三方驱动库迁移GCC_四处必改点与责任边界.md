# 第三方驱动库迁到 GCC：四处必改点与"初始化责任边界"

**建立日期**：2026-10-29
**来源实验**：[`experiments/F103_HAL_DriverLab/`](../experiments/F103_HAL_DriverLab/README.md)
**记录性质**：§2 的构建读数、§3 的符号表证据是**实测**（`arm-none-eabi-size`/`nm`/`xxd`）；
§4 的引脚依赖关系是**源码核对**（抄自驱动宏定义与注释）；**无任何板上现象证据**（本机无 ST-Link、无模块）。

---

## 1. 为什么单独立一份笔记

实验 README 回答的是"这个工程怎么建、怎么烧、怎么切模块"，
速查表回答的是"某个模块占哪几个引脚"。
两者都回答不了下一个问题：**再拿到一个只在 Keil 下验证过的驱动库 / 第三方库，我该按什么顺序验它？**

这次踩到的两个教训都属于这个层次，且都不是"多看几行代码"就能想到的：

- 构建全绿，但库要的引脚一个都没配 —— 编译器对引脚配置完全不敏感。
- 一个"看起来只是注释"的地方写错了端口，会在真板上表现成完全无关的外设异常。

这份笔记按"怎么判断"组织，不按知识点罗列。

---

## 2. Keil → GCC：必须动的四处

四处都不能靠"能编译过"来判定改对了，各有各的证据方式。

| # | 要改的 | Keil 侧 | GCC 侧 | 怎么证明改对了 |
| --- | --- | --- | --- | --- |
| 1 | 启动文件 | `startup_stm32f103xb.s`（ARM 汇编语法） | 同名但 GNU 汇编语法 | `xxd -l 8` 读 `.bin`：前 4 字节是初始 SP、后 4 字节是 `Reset_Handler` 地址且末位为 1（Thumb）。再拿 `nm` 的 `Reset_Handler` 地址对一遍 |
| 2 | 链接脚本 | `.sct` 分散加载 | `.ld`（含 `_estack`、`_Min_Heap_Size`、`_Min_Stack_Size`、溢出哨兵段） | `size -A` 看 `.isr_vector` 是否落在 Flash 起始、`._user_heap_stack` 是否生成 |
| 3 | C 运行时 | ARMCC 自带 crt0 | `-nostartfiles` 后需自补 `_init`/`_fini`/`_sbrk`/`_isatty` 等 | 构建输出里**没有** `_close/_lseek/_read/_write ... will always fail` 这类 libnosys 告警 |
| 4 | `printf` 重定向 | 覆盖 `fputc`（MicroLIB 路径） | 覆盖 `_write`（newlib 路径） | `nm` 里 `_write` 在、`fputc` **不在**（被 `--gc-sections` 回收） |

第 3 条值得展开：`-nostartfiles` 不能省，也不能只省一半。省掉 crt0 是为了让启动文件独占复位流程，
但代价是 `_init`/`_fini` 和被 `exit()` 牵连进来的 libnosys 桩全部丢失。
本次实测：不做处理时出 **4 条** `is not implemented and will always fail` 告警；
补两个文件（纯桩 `crt_stubs.c` + 重定向 `retarget.c`）后归零。

第 4 条是最容易误判的：**两套重定向可以同时存在而不冲突**。
`driver_uart.c` 定义了 `fputc`/`fgetc`，本工程又写了 `_write`，链接器不报错、`printf` 也正常出字。
但 `nm` 一查就知道真正生效的是 `_write`，`fputc` 根本没进镜像。
"能跑"在这里完全无法区分"哪条路径在工作"——只有符号表能。

---

## 3. 三条"代码上看不出来"的坑

### 3.1 构建绿 ≠ 配置对

引脚少配 8 个照样 0 error：库只是在运行时 `HAL_GPIO_ReadPin` 一个没初始化的引脚。
错误不会在编译期、链接期、甚至启动期暴露，只会在**具体模块工作时**以"读数恒 0 / 恒 1"的形式出现。

对策是把它变成可检查项：**在编译前先把引脚依赖清单列出来，逐条核对"谁配的"**，见 §4。

### 3.2 向量表可达的东西 `--gc-sections` 裁不掉

实测：`LAB_TEST=0` 时 19 个 `Test` 里 18 个被回收（`nm` 只剩 `Led_Test`，0x28 B），
但 `EXTI15_10_IRQHandler → HAL_GPIO_EXTI_IRQHandler → HAL_GPIO_EXTI_Callback →
IRReceiver_IRQ_Callback / RotaryEncoder_IRQ_Callback` 这条链**仍在镜像里**
（两个回调自身体积 0x88 + 0x74 = 252 B；它们再牵进来的辅助函数成本未单独测）。

原因：中断入口必须出现在向量表里，而向量表总在镜像中 → 入口可达 → 整条调用链可达。
**只要库是"中断驱动 + 弱回调"结构，这部分成本是固定的，不能计入"按需裁剪"。**

### 3.3 EXTI 线号只有一个，端口由 `AFIO->EXTICR` 决定

这是本次最值得记的一条。STM32 的 EXTI 线号（0–15）与 GPIO 端口是**多对一**关系：
EXTI10 只能盯一个端口，默认落在端口 A。

本工程的实际情况：红外接收在 **PB10**，而 **PA10 正是 USART1_RX**。
若不做 `HAL_GPIO_Init(GPIOB, GPIO_PIN_10)` 把 EXTI10 显式绑到端口 B，
串口每收一个字节都会触发一次"红外接收中断"。
代码上完全看不出来——`stm32f1xx_it.c` 里 `HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_10)` 写得没错，
错的是"这条线盯着谁"。

已验证的事实：`HAL_GPIO_Init` 的 IT/EVT 分支确实写 `AFIO->EXTICR`
（`stm32f1xx_hal_gpio.c:292-295`，源码已核对）。
未验证的事实：是否真的会误触发——这需要真板 + 逻辑分析仪，本次没有。

---

## 4. 方法：从库的注释反推初始化依赖清单

第三方库不再自己配引脚，那"谁配"这件事一定写在某处。本次的发现过程可以固化成五步：

1. **grep 库里的自配置代码**：`GPIO_MODE_*` / `HAL_GPIO_Init`。
   本次结果：20 个驱动里只有 6 个自带 `HAL_GPIO_Init`（led/key/active_buzzer/passive_buzzer/motor/ir_receiver），
   其中 `ir_receiver` 那段还在 `#if 0` 里 —— 实际等于 5 个。
2. **grep 库对使用者的要求**：本次的关键词是 `MX_GPIO_Init` / `在MX_` / `已经被配置`。
   命中 6 处，直接给出了引脚号与模式（"Trig PB9, Echo PB8 …"、"PB13 … 输入引脚"、"PB12 … 上升沿触发"）。
   这比读 `Test()` 函数快得多。
3. **对剩余的外设引脚查 MSP**：`HAL_xxx_MspInit` 是否已配（USART/I2C/SPI/TIM/ADC 的 AF 引脚都在这里）。
   本次结论：AF 引脚全齐，缺的只是**纯 GPIO**，范围立刻从 20 个模块缩到 8 个引脚。
4. **核对课程/上游是否有现成配置**：不要自己发明引脚。
   本次查遍课程所有 `gpio.c`，发现"最完整的那份"是手表产品工程、且缺 PB13 →
   说明**上游也不存在一份全配置**，必须自己补，而不是"我漏拷了"。
5. **补丁与生成物分离**：把补充写进 `lab_pins.c` 这种非生成文件，而不是改 CubeMX 的 `gpio.c`。
   文件头逐条写"这条要求来自库的哪句话 + 为什么这么配"，改的时候不用再查一遍。
   代价也要记账（本次 +288 B text）。

---

## 5. 附带的一条：`size` 的 bss 会把预留算成消耗

同一份文档里出现两个 RAM 数字，是本仓库已经踩过一次的坑（见
[`嵌入式内存预算_三种余量口径.md`](嵌入式内存预算_三种余量口径.md)）。本次再验证一次：

```text
$ arm-none-eabi-size build/f103_hal_driverlab.elf
   text    data     bss     dec
  15276     100    3176   18552

$ arm-none-eabi-size -A build/f103_hal_driverlab.elf
.bss                   1636
._user_heap_stack      1540      ← _Min_Heap_Size(512) + _Min_Stack_Size(1024)
```

默认输出的 `bss 3176` 是 `1636 + 1540`。**1540 B 是链接脚本为堆栈做的溢出检查预留，不是运行期消耗。**
把预留当消耗会低估余量，反过来当余量会高估可用空间——两个方向都错。
看 RAM 只有 `size -A` 一个口径可用。

---

## 6. 当前验证状态

| 结论 | 状态 |
| --- | --- |
| 四处必改点可行（构建 0 error / 13 warning） | **实测** |
| 向量表、`.ld` 布局、`--gc-sections` 裁剪 | **实测**（`xxd` + `nm`） |
| `printf` 走 `_write` 而非 `fputc` | **实测**（符号表） |
| `HAL_GPIO_Init` 写 `AFIO->EXTICR` | **源码核对** |
| 驱动对 `MX_GPIO_Init` 的 8 处依赖 | **源码核对**（注释逐条抄录） |
| `EXTI10` 不绑端口 B 会误触发 | **未验证**（需真板 + 逻辑分析仪） |
| `lab_pins.c` 里 PB12/PB13 上下拉 | **工程推断**（与课程 `NOPULL` 有差异，未上板） |
| 任一模块的板上现象 | **全部未验证** |

## 7. 待解决

- 真板到位后按 §3.3 做一次"故意不配 EXTICR"的对照，把"未验证"变成读数。
- PB12/PB13 的上下拉选择需要按键手感与避障稳定性两方面的实测才能定。
- FreeRTOS 版本（课程主线）的同一套构建尚未做；`configMAX_SYSCALL_INTERRUPT_PRIORITY`
  会让 §3.3 的 NVIC 优先级取值从"随便"变成"必须算"，这是下一个坑。
