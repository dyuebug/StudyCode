# Day5：中断体系与 GPIO 中断编程

**验证状态**：EXTI 分发链与 `GPIO_MODE_IT_*` / `GPIO_MODE_EVT_*` 的位编码**已核对源码**
（`stm32f1xx_hal_gpio.c:265-337`、`:546-568`，`stm32f1xx_hal_gpio.h:107-127`）；
NVIC 优先级配置**已核对源码**（`lab_pins.c` 的 `EXTI15_10_IRQn = 0`、`stm32f1xx_hal_conf.h:132`
的 `TICK_INT_PRIORITY = 15`）；"`MX_GPIO_Init` 只配了 PC13"为**逐文件核对**；
**板上中断从未触发过**（本机无 ST-Link，未接红外/编码器模块）。教材侧内容来自
《HAL快速入门与项目实战》第 6.1–6.3 节。

---

## 学习目标

学完本篇应能用代码证据回答：

1. 一次按键（或红外边沿）从引脚电平变化到进入 `HAL_GPIO_EXTI_Callback`，中间经过哪三个寄存器块。
2. `GPIO_MODE_IT_RISING_FALLING` 与 `GPIO_MODE_EVT_RISING_FALLING` 的数值差在哪一位，用错会怎样。
3. `EXTI10` 这个"10"指的是**引脚号**还是**端口号**，同一时刻它能挂几个端口，由谁决定。
4. 本工程两个驱动的注释都说"引脚已在 `MX_GPIO_Init` 里配好"，这句话在这里是否成立，谁替它们补了。
5. 中断里调 `mdelay(2)` 会不会死锁——分别对 `udelay` 与 `HAL_GetTick()` 给出结论和依据。

---

## 环境与前置条件

| 项 | 要求 |
| --- | --- |
| 工程 | `experiments/F103_HAL_DriverLab/` |
| 参考驱动 | `driver_ir_receiver.c`、`driver_rotary_encoder.c`、`driver_irq.c` |
| 参考 HAL | `stm32f1xx_hal_gpio.c` 的 `HAL_GPIO_Init` EXTI 分支与 `HAL_GPIO_EXTI_IRQHandler` |
| 硬件 | 可选。本篇结论全部可由源码与分散加载结果证明 |
| 前置 | Day2（BSRR/ODR）、Day4（GPIO 模式与 `HAL_GPIO_Init` 翻译表） |

---

## 核心概念

### 1. 为什么用中断：轮询的代价是可量化的

教材 6.1 节用按键引出中断。把差别写成数字：按键最短抖动约 5 ms，按下动作 50–200 ms。
若主循环每 10 ms 轮询一次，抖动期内会读到 3–5 次跳变；若每 1 ms 轮询一次，代价是主循环
每分钟被打断 6 万次，而这些轮询里 99.9% 什么都没发生。

中断把"等待"变成硬件的事：电平一变，CPU 立刻跳到 ISR。代价是**执行上下文变了**——
ISR 里不能有长循环、不能等锁、不能假设全局变量不被别人改。

### 2. 从引脚到 ISR：三个寄存器块

```text
   引脚电平变化
        │
        ▼
  ┌──────────────┐   ① 边沿/电平检测（在 AFIO 之前的 EXTI 模块内）
  │ EXTI->RTSR   │   上升沿使能  ← HAL_GPIO_Init 的 RISING_EDGE 分支
  │ EXTI->FTSR   │   下降沿使能
  └──────┬───────┘
         │ 触发
         ▼
  ┌──────────────┐   ② 挂起与屏蔽
  │ EXTI->PR     │   挂起位：边沿一到就置 1（软件写 1 清零）
  │ EXTI->IMR    │   中断屏蔽：为 1 才把挂起转成中断请求
  │ EXTI->EMR    │   事件屏蔽：为 1 只产生事件，不产生中断 ← 关键区别
  └──────┬───────┘
         │
         ▼
  ┌──────────────┐   ③ 内核侧：NVIC
  │ NVIC_ISER    │   HAL_NVIC_EnableIRQ(EXTI15_10_IRQn)
  │ NVIC_IPR     │   HAL_NVIC_SetPriority(...)
  └──────┬───────┘
         │
         ▼
   EXTI15_10_IRQHandler → HAL_GPIO_EXTI_IRQHandler(Pin) → 清 PR → HAL_GPIO_EXTI_Callback(Pin)
```

三者**缺一不可**，而且漏掉哪一个的症状完全不同：

| 漏掉 | 症状 |
| --- | --- |
| `RTSR`/`FTSR` 两个都没置 | 引脚电平再怎么变，`PR` 都不置位——完全是死的 |
| `IMR` 没置（但 `EMR` 置了） | `PR` 会置位，但 CPU 不进 ISR——这就是 `GPIO_MODE_EVT_*` 的行为 |
| `NVIC` 没使能 | `IMR` 置了、`PR` 会置，但请求到不了内核，进不了 ISR |

中间这一档最容易被误判成"硬件没接好"。它在**本工程的驱动源码里真实存在**，见 §3。

### 3. `GPIO_MODE_IT_*` 与 `GPIO_MODE_EVT_*`：差在第三位半字节

HAL 用 `0xX0yz00YZ` 的编码把四件事压进一个 `uint32_t`（`stm32f1xx_hal_gpio.h:107-127`）：

```c
#define GPIO_MODE_IT_RISING          0x10110000u
#define GPIO_MODE_IT_FALLING         0x10210000u
#define GPIO_MODE_IT_RISING_FALLING  0x10310000u
#define GPIO_MODE_EVT_RISING         0x10120000u
#define GPIO_MODE_EVT_FALLING        0x10220000u
#define GPIO_MODE_EVT_RISING_FALLING 0x10320000u
                         ↑
                    第 15:12 位 = 1 表示 IT，2 表示 EVT
```

`HAL_GPIO_Init` 用两条 `&` 判断把它拆开：

```c
if ((GPIO_Init->Mode & GPIO_MODE_EVT) == GPIO_MODE_EVT)
    SET_BIT(EXTI->EMR, iocurrent);      /* 只产生事件 */
else
    CLEAR_BIT(EXTI->EMR, iocurrent);

if ((GPIO_Init->Mode & GPIO_MODE_IT) == GPIO_MODE_IT)
    SET_BIT(EXTI->IMR, iocurrent);      /* 才产生中断 */
else
    CLEAR_BIT(EXTI->IMR, iocurrent);
```

**两者互斥**：IT 模式的宏里没有 `GPIO_MODE_EVT` 那一位，所以 IT 模式下会显式 `CLEAR_BIT(EMR)`；
反过来 EVT 模式下会 `CLEAR_BIT(IMR)`。用 `EVT` 配一个"中断引脚"，结果是**边沿有、挂起有、中断没有**。

本工程 `driver_ir_receiver.c:291` 那段被 `#if 0` 关掉的自配置，写的是：

```c
GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING_FALLING;   /* ← 事件模式，不是中断 */
```

如果哪天有人把 `#if 0` 改成 `#if 1` 想"让驱动自给自足"，红外接收会**静默失效**：
`lab_pins.c` 已经把 EXTI10 配成 `GPIO_MODE_IT_RISING_FALLING`，而这段 `EVT` 配置后执行，
会把 `IMR` 的 bit10 清掉。**后写者胜**，功能就没了。这是一条不能用眼睛看出来的坑。

### 4. `EXTI10` 的 10 是引脚号，不是端口号

F1 的 EXTI 有 16 条线，**每条线对应一个引脚号，但可选的端口由 `AFIO->EXTICR` 决定**。
`HAL_GPIO_Init` 在 EXTI 分支里做这件事（`stm32f1xx_hal_gpio.c:288-295`）：

```c
__HAL_RCC_AFIO_CLK_ENABLE();                                  /* ← 必须开 AFIO 时钟 */
temp = AFIO->EXTICR[position >> 2u];
CLEAR_BIT(temp, (0x0Fu) << (4u * (position & 0x03u)));
SET_BIT(temp, (GPIO_GET_INDEX(GPIOx)) << (4u * (position & 0x03u)));
AFIO->EXTICR[position >> 2u] = temp;
```

推论有两条：

1. **`EXTI10` 同一时刻只能挂一个端口**。写了 `HAL_GPIO_Init(GPIOB, PIN_10)` 之后，
   `EXTI10` 就从"默认的 PA10"搬到了 PB10。这也是 `lab_pins.c` 那段注释强调的重点。
2. **AFIO 时钟没开，`EXTICR` 的写入同样被静默丢弃**。`HAL_GPIO_Init` 自己会调
   `__HAL_RCC_AFIO_CLK_ENABLE()`，所以走 HAL 不会漏——但手写寄存器版本很容易漏。

本板的情况：`PA10` 是 `USART1_RX`（`usart.c` 里配的）。若 `EXTI10` 还留在端口 A，
串口每收到一个字节（起始位下降沿）都会触发一次 EXTI10 中断，`HAL_GPIO_EXTI_Callback(GPIO_PIN_10)`
就会把它当成红外边沿交给 `IRReceiver_IRQ_Callback()`。**这一条是本工程按源码推的必然结果，
未在真板上抓波形确认**，但它解释了为什么 `lab_pins.c` 必须显式配 PB10 而不是"反正都一样"。

### 5. HAL 的分发链与清理顺序

```c
void HAL_GPIO_EXTI_IRQHandler(uint16_t GPIO_Pin)     /* stm32f1xx_hal_gpio.c:546 */
{
  if (__HAL_GPIO_EXTI_GET_IT(GPIO_Pin) != 0x00u)     /* 读 EXTI->PR */
  {
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_Pin);              /* 写 1 清 PR —— 先清 */
    HAL_GPIO_EXTI_Callback(GPIO_Pin);                /* 再调回调 —— 后处理 */
  }
}
```

三条要紧的：

- **先清后调**。若反过来，回调里耗时超过一次边沿间隔，`PR` 会一直被重新置位 → 反复重入。
- `HAL_GPIO_EXTI_Callback` 是 `__weak` 的空函数（`:561`）。没写自己的版本，中断会**正常进、正常清、
  正常返回**，什么也不发生——排查时极易被当成"没触发"。
- `stm32f1xx_it.c` 的 `EXTI15_10_IRQHandler` 对 `PIN_10`/`PIN_11`/`PIN_12` 各调一次。
  **`PIN_11` 在本工程没有任何配置**（`lab_pins.c` 配了 PB13、PB10、PB12、PB0/1、PA1、PB9、PB8），
  所以那一行永远读到 `PR` 为 0，是死代码。它不影响功能，但会让"到底哪条线在触发"变得含混。

### 6. ISR 里能调什么：用源码判，不要凭印象

`driver_rotary_encoder.c:121-149` 的 ISR 里有一句 `mdelay(2);`。教科书答案会说"死锁"，
但**在这里不成立**，因为 `mdelay` 的实现不依赖中断：

```c
/* driver_timer.c:46-106，实际走 #else 分支 */
uint32_t reload = __HAL_TIM_GET_AUTORELOAD(hHalTim);   /* TIM4->ARR = 999 */
told = __HAL_TIM_GET_COUNTER(hHalTim);                 /* TIM4->CNT */
while (1) {
    tnow = __HAL_TIM_GET_COUNTER(hHalTim);              /* 轮询硬件计数器 */
    if (tcnt >= ticks) break;
}
```

它直接读 `TIM4->CNT`。**计数器由硬件推进，与中断是否被抢占无关**，所以 `mdelay(2)` 在 ISR 里
确实会返回，只是把 CPU 卡在那里 2 ms。

但同一份 ISR 里的 `time = system_get_ns();`（`:127`）就不同了：

```c
uint64_t ns = HAL_GetTick();          /* ← 依赖 uwTick，uwTick 在 TIM4 中断里自增 */
```

而本工程 `stm32f1xx_hal_conf.h:132` 写的是 `TICK_INT_PRIORITY = 15U`（最低），
`lab_pins.c` 给 `EXTI15_10_IRQn` 配的是 `0`（最高）。Cortex-M3 的优先级数值越小越优先，
**优先级 15 的 TIM4 无法抢占优先级 0 的 EXTI**。所以在这段 ISR 内部：

| 依赖 | ISR 内是否还能前进 | 依据 |
| --- | --- | --- |
| `TIM4->CNT`（`mdelay`/`udelay`） | **能**，硬件计数 | `driver_timer.c:84-104` |
| `HAL_GetTick()` / `uwTick` | **不能**，冻结在进入 ISR 时的值 | `TICK_INT_PRIORITY=15U` vs `EXTI=0`；`uwTick` 在 `TIM4_IRQHandler` 里自增 |

`time` 在 `mdelay` **之前**读，所以 `g_speed` 的分子分母不受这 2 ms 影响——这是"碰巧对"，
不是设计。把 `time = system_get_ns();` 挪到 `mdelay(2);` 之后，`time - pre_time` 就会丢掉这 2 ms。

结论按可迁移的方式表述：**判断"ISR 里能不能调某个函数"，看它最终等的是什么**——
等硬件寄存器可以，等软件计数（`uwTick`/RTOS tick）要看优先级关系，等锁或等别的任务一定不行。

`mdelay(2)` 本身仍有问题：它把 CPU 占满 2 ms，期间任何优先级 ≤ 0 的中断都进不来。
正确做法是"时间戳记录 + 主循环里判抖动"，而不是在 ISR 里等。

### 7. 环形缓冲：为什么这里可以不加锁

`driver_ir_receiver.c:39-119` 用一个 128 字节的环形缓冲把 ISR 与主循环解耦：

```c
static unsigned char g_KeysBuf[BUF_LEN];
static int g_KeysBuf_R, g_KeysBuf_W;          /* 只有 ISR 写 W，只有主循环写 R */
```

这是**单生产者单消费者（SPSC）**结构，可以无锁：
ISR 只改 `g_KeysBuf_W`，主循环只改 `g_KeysBuf_R`，两者各自读对方的值。
`int` 是 32 位且自然对齐，Cortex-M3 上单次读写不会撕裂——所以这里**不需要关中断**。

对比 Day2 的 `ODR` 读改写：那里是**两个写者改同一份数据**（读-改-写），必须用 `BSRR` 或用临界区。
**判据不是"有没有中断参与"，而是"同一个变量有没有两个写者"。**

### 8. 但这个环形缓冲有一个真实的配对缺陷

`IRReceiver_Read()` 一次取**两个**字节（device + key），`IRReceiver_IRQTimes_Parse()`
用两次 `PutKeyToBuf` 成对写入。而 `PutKeyToBuf` 每次单独判满：

```c
static void PutKeyToBuf(unsigned char key)
{
    if (!isKeysBufFull()) { ... g_KeysBuf_W = NEXT_POS(g_KeysBuf_W); }
    /* 满了就静默丢弃 —— 第二次可能丢，第一次已经进去了 */
}
```

**只剩 1 个空位时，第一字节进、第二字节被丢**，缓冲占用变成奇数。
此后每一次 `IRReceiver_Read` 取到的 `(dev, data)` 都错开一位——所有按键码整体错位，
而且 `isKeysBufEmpty()` 永远不报错。这是"数据不对但不报错"的又一类。

修法有两条，都要求改调用方语义：入队请求要么整对成功、要么整对失败（先判剩余空间 ≥2），
或者把 `(dev, key)` 打包成一个结构体，让"一条记录"成为最小单位。
**这一条是源码推论，未在真板上跑满缓冲区触发过。**

---

## 系统调用/接口卡片

### `HAL_GPIO_Init` 的 EXTI 分支

作用：给某个引脚建立"引脚号 → EXTI 线 → 边沿检测 → 中断/事件屏蔽 → 端口映射"这一整串配置。

关键参数（EXTI 相关只用到前两个）：

| 字段 | 取值 | 落到哪里 |
| --- | --- | --- |
| `Mode` | `GPIO_MODE_IT_RISING` / `_FALLING` / `_RISING_FALLING` | `EXTI->RTSR`、`EXTI->FTSR`、`EXTI->IMR` |
| `Mode` | `GPIO_MODE_EVT_*` | 同上，但置的是 `EXTI->EMR`，且清 `IMR` |
| `Pull` | `GPIO_PULLUP` 等 | 同 Day4：`BSRR`/`BRR` 决定 `ODR` 位 |
| `Pin` | 位掩码 | 位号决定 `EXTICR` 的 4 bit 位置与掩码 |

返回值和错误：`void`。`Mode` 非法时无提示（除非 `USE_FULL_ASSERT`），该引脚不会进入 EXTI 配置，
也**不会**清掉已有配置——于是"配了等于没配，但之前的配置还在"。

资源所有权：无。

线程/进程安全：非原子；会顺带使能 AFIO 时钟。

最小示例：

```c
GPIO_InitTypeDef s = {0};
s.Pin  = GPIO_PIN_10;
s.Mode = GPIO_MODE_IT_RISING_FALLING;   /* IT，不是 EVT */
s.Pull = GPIO_PULLUP;
HAL_GPIO_Init(GPIOB, &s);               /* 顺带把 EXTI10 映射到端口 B */
```

常见误用：

- 用 `GPIO_MODE_EVT_*`。配置"成功"，但永远不进 ISR。
- 只配引脚不配 NVIC：`lab_pins.c` 末尾那三行（`SetPriority` + `EnableIRQ`）就是为此存在。
- 以为配了 `Pull` 就不用外部上拉：开集输出（1838 红外接收头）仍然需要。

### `HAL_NVIC_SetPriority(IRQn, PreemptPriority, SubPriority)`

作用：写 `NVIC_IPR`，决定这个中断能不能抢占别人、能不能被别人抢占。

参数：`PreemptPriority` 数值**越小越优先**；`SubPriority` 只在抢占优先级相同时比较（F1 的
`HAL_NVIC_SetPriority` 走 `NVIC_EncodePriority`，本工程 `__NVIC_PRIO_BITS = 4`，范围 0–15）。

返回值和错误：`void`。传入 ≥ 16 会被 `assert_param` 拦，未开断言时**静默取模**。

线程/进程安全：无。

常见误用：把 EXTI 设为 0 却不知道 `TICK_INT_PRIORITY` 是 15 —— 于是 ISR 里 `HAL_GetTick()`
永不前进（§6）。两者必须成对考虑。

### `HAL_GPIO_EXTI_IRQHandler(uint16_t GPIO_Pin)`

作用：判挂起 → 清挂起 → 调 `HAL_GPIO_EXTI_Callback`。

参数：位掩码，与 `GPIO_PIN_n` 一致。

返回值和错误：`void`。

线程/进程安全：在 ISR 上下文执行；`PR` 的"写 1 清零"本身是原子的。

常见误用：在自己的 `EXTIx_IRQHandler` 里手动清 `PR` 后又调它——重复清无害，
但若**先调回调再清**，回调耗时会导致反复重入。

### `HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)`（`__weak`）

作用：用户侧的分发点。本工程在 `driver_irq.c` 里重写，按 `GPIO_PIN_10` / `GPIO_PIN_12` 分派。

参数：位掩码。**注意是引脚号，不含端口信息**——所以 `GPIO_PIN_10` 既可能是 PA10 也可能是 PB10，
由 `EXTICR` 决定。这也正是 §4 那个坑的根源。

返回值和错误：`void`。

常见误用：在回调里写 `printf`（阻塞发送）、调 `mdelay`（见 §6）、或申请动态内存。

---

## 最小可运行实验

目标：不接硬件，把"三个寄存器块"落到反汇编与符号上。

```bash
cd experiments/F103_HAL_DriverLab

# 1) 确认 EXTI10 被映射到端口 B（EXTICR 写入路径）
arm-none-eabi-objdump -d --disassemble='Lab_Pins_Init' build/f103_hal_driverlab.elf \
  | grep -nE 'AFIO|EXTICR|#8|#-?4' | head

# 2) 确认两个 IRQ 入口都在镜像里
arm-none-eabi-nm build/f103_hal_driverlab.elf | grep -E 'EXTI15_10_IRQHandler|HAL_GPIO_EXTI_IRQHandler|HAL_GPIO_EXTI_Callback'

# 3) 确认回调不是 __weak 空版本（应指向 driver_irq.c 的那份）
arm-none-eabi-objdump -d --disassemble='HAL_GPIO_EXTI_Callback' build/f103_hal_driverlab.elf
```

第 3 条最关键：`__weak` 与用户实现在链接期只保留一个。`nm` 里只看到一个地址，
**看不出是哪一份**；反汇编里看到 `switch`/比较链才是用户版本。

---

## 编译与运行

```bash
cmake --build build
```

本篇没有"运行"——中断需要引脚真的跳变，本机无板。

---

## 结果观察

**`MX_GPIO_Init` 的实际内容**（`Core/Src/gpio.c`，逐行核对）：

```text
使能时钟：GPIOC、GPIOD
初始电平：PC13 = RESET
HAL_GPIO_Init 调用次数：1（PC13，推挽输出）
```

**没有** `GPIO_MODE_IT_*`，**没有** `HAL_NVIC_*`。这与两个驱动的注释直接矛盾：

| 驱动 | 注释原文 | 本工程实际 |
| --- | --- | --- |
| `driver_ir_receiver.c:287` | "PA10 在 MX_GPIO_Init() 中已经被配置为双边沿触发" | **未配置**；且引脚应为 PB10 而非 PA10 |
| `driver_rotary_encoder.c:166-167` | "PB0,PB1 在 MX_GPIO_Init 中被配置为输入引脚"、"PB12 ... 中断引脚" | **未配置** |

**替它们补上的是 `Core/Src/lab_pins.c`**：

```text
PA1   开漏+上拉        DHT11 / DS18B20 单总线
PB9   推挽输出，初值低  SR04 TRIG == W25Q64 CS（同一引脚）
PB8   输入，无上下拉    SR04 ECHO
PB13  输入+上拉         红外避障 LM393（开集）
PB0/1 输入+上拉         EC11 S1/S2
PB10  输入+上拉，IT 双边沿  红外接收 1838
PB12  输入+上拉，IT 上升沿  EC11 按键
末尾：HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0) + HAL_NVIC_EnableIRQ(EXTI15_10_IRQn)
```

**优先级关系**（决定 §6 的结论）：

```text
__NVIC_PRIO_BITS = 4  → 优先级 0..15，0 最高
EXTI15_10_IRQn   = 0  （lab_pins.c 手工设）
TIM4_IRQn        = 15 （TICK_INT_PRIORITY，HAL_Init 传入 HAL_InitTick）
⇒ TIM4 不能抢占 EXTI；ISR 内 HAL_GetTick() 冻结
```

**`driver_irq.c` 的分发表**：`GPIO_PIN_10` → `IRReceiver_IRQ_Callback()`；
`GPIO_PIN_12` → `RotaryEncoder_IRQ_Callback()`；其余走 `default: break`。

**两处语义可疑但未上板确认的地方**：

1. `RotaryEncoder_IRQ_Callback` 的注释写"S1 上升沿触发中断"，但实际中断源是 **PB12（按键）**。
   函数里读 S1/S2 判方向、累加 `g_count`，逻辑上把"按键按下"当成了"旋转一格"。
2. `EXTI15_10_IRQHandler` 里对 `GPIO_PIN_11` 也调了一次 handler，但全工程没有配置 11 号引脚。

---

## 常见错误和陷阱

| 陷阱 | 症状 | 根因 | 处理 |
| --- | --- | --- | --- |
| 用 `GPIO_MODE_EVT_*` 配中断脚 | 电平在跳、ISR 不进 | 只置 `EMR`，清了 `IMR` | 改成 `GPIO_MODE_IT_*` |
| 忘 `HAL_NVIC_EnableIRQ` | `EXTI->PR` 置位、CPU 不进 | NVIC 屏蔽 | 配 `SetPriority` + `EnableIRQ` |
| 没配 `EXTICR` 就指望 PB10 | 触发源变成 PA10（USART1_RX） | EXTI 线号与端口解耦 | 显式 `HAL_GPIO_Init(GPIOB, PIN_10)` |
| `GPIO_MODE_EVT` 覆盖了 `IT` 配置 | 原本能用的中断突然失效 | 后写者胜，`IMR` 被清 | 同一引脚只在一处配 |
| ISR 里调 `HAL_GetTick()` 等时间 | 值不前进 | TIM4 优先级 15 < EXTI 0 | 提高 TIM4 优先级，或改硬件计数 |
| ISR 里 `mdelay`/长循环 | 系统响应变慢、低优先级中断被饿死 | 忙等占 CPU | 只记时间戳，主循环里判 |
| 环形缓冲成对写入只进一半 | 数据整体错位、不报错 | `PutKeyToBuf` 单独判满 | 按"记录"入队，或先判空间 ≥2 |
| 只写 `HAL_GPIO_EXTI_Callback` 却复写错签名 | 链接到 `__weak` 空版本 | `uint16_t` vs `uint32_t` 之类 | 反汇编确认是用户版本 |

---

## 故障排查方法

"中断没进"的排查顺序，**从内核往引脚退**，每步只读一个寄存器：

1. **`NVIC->ISER`** 对应位是否为 1。为 0 → 问题在 `HAL_NVIC_EnableIRQ` 没执行（或 NVIC 未初始化）。
2. **`EXTI->IMR`** 的对应位是否为 1。为 0 而 `EXTI->EMR` 为 1 → 这条线被配成了**事件**模式。
3. **`EXTI->RTSR`/`FTSR`** 是否至少一位为 1。都为 0 → 边沿方向配错或压根没配。
4. **`EXTI->PR`** 是否置位。置位了而 CPU 没进 → 回到第 1 步（被屏蔽）；不置位 → 引脚侧没有边沿。
5. **`AFIO->EXTICR[n]`** 的 4 bit 是否指向你想要的端口。指错端口时，别的引脚在替你触发。
6. **引脚本身**：`CR` 的 4 bit 是否为输入模式、`ODR` 位是否与 `Pull` 一致（Day4 §4）。

第 5 步是本篇最容易漏的一步，也是最贵的一步：它错了以后，第 1–4 步**全部看起来正常**。

真要上板定位时，配一个"已知在闪的引脚"（例如让主循环每 100 ms 翻转一个空闲脚），
把它接到示波器/逻辑分析仪的另一通道，**用同一时间轴对比"边沿"与"ISR 里的翻转"**。
本仓库 `F103_*` 系列的硬件结论至今都标注"待真板"，理由就是缺这一条时间轴证据。

---

## 练习题

### 基础题

题目：写出 `GPIO_MODE_IT_FALLING` 与 `GPIO_MODE_EVT_FALLING` 的数值，标出不同的那一位，
并说明用 `EVT` 版本配一个"要进中断"的引脚时，`RTSR/FTSR/EMR/IMR` 四个寄存器各是什么状态。

提示：`0xX0yz00YZ` 编码里的 `z`；`HAL_GPIO_Init` 的 IT/EVT 两个 `if` 是互斥的。

验收标准：两个数值正确（`0x10210000` / `0x10220000`），能指出 `z` 位，
并说明 `FTSR` 置 1、`EMR` 置 1、`IMR` 被清 0。

### 改错题

题目：下面的配置"能编译、能返回，但中断永远不进"，指出问题并修正。

```c
static void sensor_irq_init(void)
{
    GPIO_InitTypeDef s = {0};
    s.Pin  = GPIO_PIN_12;
    s.Mode = GPIO_MODE_IT_RISING;
    s.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &s);
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
}
```

提示：数一数三块寄存器（边沿、屏蔽、NVIC）里哪一块没配。

验收标准：能指出缺 `HAL_NVIC_EnableIRQ(EXTI15_10_IRQn)`，并说明症状是
"`EXTI->PR` 会置位但 CPU 不进 ISR"，与"引脚没接"如何区分。

### 综合题

题目：把 `RotaryEncoder_IRQ_Callback` 改造成"ISR 只记录时间戳、主循环做防抖与计数"的版本。
要求：① ISR 内不出现任何延时；② 说明改动后 `HAL_GetTick()` 在 ISR 里能否用；
③ 说明防抖窗口取多少、依据是什么。

提示：§6 的两张表；机械触点抖动典型 1–5 ms。

验收标准：ISR 里只有读引脚 + 存时间戳（可用 `TIM4->CNT` 或 `uwTick`）；
能说明改了以后 `g_speed` 的分母不再受 ISR 耗时影响；
防抖窗口给出数值并指出这是经验值而非实测。

---

## 验收标准

- [ ] 能画出"引脚 → RTSR/FTSR → PR/IMR → NVIC → ISR → 回调"五级链路，并说出漏每级的症状。
- [ ] 能指出 `IT` 与 `EVT` 宏的差异位，并解释为什么 `EVT` 会让"中断不进但边沿有"。
- [ ] 能解释 `EXTI10` 的 10 是引脚号，以及 `AFIO->EXTICR` 决定它挂哪个端口。
- [ ] 能指出本工程 `MX_GPIO_Init` 只配了 PC13，并说出两个驱动注释为什么不成立、谁补的。
- [ ] 能分别回答"ISR 里 `mdelay(2)` 会不会死锁"与"ISR 里 `HAL_GetTick()` 会不会前进"，并给出源码依据。
- [ ] 能说明 SPSC 环形缓冲为什么可以不加锁，以及成对写入的配对缺陷。
- [ ] 能独立完成本篇三条 `objdump`/`nm` 命令，确认回调是用户版本而非 `__weak`。

## 延伸阅读

- 教材：《HAL快速入门与项目实战(基于DshanMCU-103)》第 6.1–6.3 节（中断概念、EXTI、GPIO 中断编程）
- HAL 源码：`Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio.c:265-337`（EXTI 配置）、`:546-568`（分发链）
- 宏定义：`Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_gpio.h:107-127`
- 本工程补齐：`Core/Src/lab_pins.c`、`Drivers/DshanMCU-F103/driver_irq.c`
- 下一篇：`Linux/notes/F103_HAL入门/Day06_中断调试与环形缓冲.md`

---

**验证记录**

| 项目 | 内容 |
| --- | --- |
| 日期 | 2026-10-29 |
| 环境一 | Windows 11 + Git Bash；`grep`/`sed` 逐行核对源码 |
| 命令一 | `sed -n '255,340p' stm32f1xx_hal_gpio.c`；`sed -n '100,132p' stm32f1xx_hal_gpio.h`；`sed -n '546,568p' stm32f1xx_hal_gpio.c` |
| 结果一 | EXTI 分支先开 AFIO 时钟再写 `EXTICR[position>>2]` 的 4 bit；IT 置 `IMR`、EVT 置 `EMR` 且互斥；`IT_RISING_FALLING = 0x10310000`、`EVT_RISING_FALLING = 0x10320000`；IRQHandler 先清 `PR` 再调 `__weak` 回调 |
| 环境二 | 同上 |
| 命令二 | `cat Core/Src/gpio.c Core/Src/stm32f1xx_it.c Core/Src/lab_pins.c`；`grep -n TICK_INT_PRIORITY Core/Inc/stm32f1xx_hal_conf.h` |
| 结果二 | `MX_GPIO_Init` 仅有 1 处 `HAL_GPIO_Init`（PC13）、无 `HAL_NVIC_*`；`EXTI15_10_IRQHandler` 分发 PIN_10/11/12；`lab_pins.c` 补 PB10（IT 双边沿）、PB12（IT 上升沿）并以优先级 0 使能 NVIC；`TICK_INT_PRIORITY = 15U` |
| 未覆盖 | 全部板上中断行为：EXTI10/EXTI12 是否真的触发、红外 1838 与 EC11 的波形、`EVT` 覆盖 `IT` 的失效复现；`mdelay` 在 ISR 中不返回的**反例**未构造（只证明了它读硬件计数器）；环形缓冲配对错位未跑满缓冲区触发 |
