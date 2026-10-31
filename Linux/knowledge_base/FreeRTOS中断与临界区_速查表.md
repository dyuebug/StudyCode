# FreeRTOS 中断与临界区：速查表

> **一句话定义**：FreeRTOS 用 **`BASEPRI`** 屏蔽"优先级数值 ≥ N"的中断来实现临界区，
> 因此**调用 `...FromISR()` 的中断必须落在被屏蔽的那一半**——否则临界区保护它，它却敢改内核数据结构。
>
> **本仓库配置**：`configPRIO_BITS 4`（STM32F1 实现 4 位优先级）、
> `configLIBRARY_LOWEST_INTERRUPT_PRIORITY 15`、`configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5`。
> 换算（`<< (8 - configPRIO_BITS)` = `<< 4`）：
> **`configKERNEL_INTERRUPT_PRIORITY = 0xF0`（最低，PendSV/SysTick）、`configMAX_SYSCALL_INTERRUPT_PRIORITY = 0x50`**。
> 来源：`FreeRTOSConfig.h:90-101`。

---

## 1. 使用场景

| 要做的事 | 用哪种保护 | 代价 |
| --- | --- | --- |
| 改几个共享变量（几条指令） | `taskENTER_CRITICAL()` | 关掉 5–15 的中断，通常几 µs |
| 改链表、多个变量要保持一致 | 临界区**或**信号量 | 临界区更短更可测 |
| ISR 内与任务共享一个计数器 | `taskENTER_CRITICAL_FROM_ISR()` | 只抬 BASEPRI，不动主优先级 |
| ISR 内要发队列/信号量 | `...FromISR()` + `portYIELD_FROM_ISR` | 不关中断 |
| 长时间互斥（含阻塞调用） | ❌ **不能用临界区** | 会把整个系统卡死 |

**判据**：临界区内**只允许**"读 → 算 → 写"几件事。出现函数调用、循环、等待，就是选错了工具。

---

## 2. 优先级数值的方向与分界

**数值越小，优先级越高**（NVIC 规则）。这条在 FreeRTOS 配置里最容易搞反：

```text
优先级数值:   0    1    2    3    4  │  5    6  ...  15
              ├────────────────────┤  │  ├──────────────┤
              │ 不受 BASEPRI 屏蔽    │  │  │ 受 BASEPRI 屏蔽 │
              │ 绝对禁止调 FreeRTOS │  │  │ 可调 FromISR  │
              └────────────────────┘  │  └──────────────┘
                                   BASEPRI = 0x50
```

- `BASEPRI = 0x50` ⇒ 屏蔽**数值 ≥ 5** 的中断；数值 0–4 的中断**照常进入**。
- 因此**数值 0–4 的中断里调任何 FreeRTOS API 都是非法**：临界区挡不住它，它会在内核链表被改到一半时插进来。
- 两个"最低"必须分清：`configKERNEL_INTERRUPT_PRIORITY 0xF0` 是 **PendSV/SysTick** 的优先级（内核自己用，
  保证上下文切换不打断别人的临界区）；`configMAX_SYSCALL_INTERRUPT_PRIORITY 0x50` 是**分界线**。

**一条容易漏的事实**：本仓库**没有显式设置 `PRIGROUP`**（全仓只有 CMSIS 头文件里的定义与 `port.c` 的注释，
没有调用 `NVIC_SetPriorityGrouping`），PRIGROUP 保持复位值 0。STM32F1 的 NVIC 只实现 `IPR[7:4]`，
在 PRIGROUP=0（抢占位 = `IPR[7:1]`）下这 4 位仍**全在抢占字段内**，所以 `<< 4` 的换算成立。
**这是推理，不是读数**——没有用 `AIRCR` 实测确认过分组。若移植到 HAL 工程，`HAL_Init()` 会按
`NVIC_PRIORITYGROUP_4` 重设分组，结论一致，但那是有代码依据的。

---

## 3. 接口卡片

### `taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()`

作用：任务上下文的临界区，内部 `BASEPRI = configMAX_SYSCALL_INTERRUPT_PRIORITY`，退出时恢复。
参数：无。
返回值和错误：无返回值。**必须成对**，可嵌套（内核维护嵌套计数，只有最外层才真正放行）。
常见误用：① 在临界区里 `return`（嵌套计数不平衡，之后再也放不开）；
② 在 ISR 里用这一对（ISR 版的恢复方式不同，混用会破坏 `BASEPRI`）。

### `UBaseType_t taskENTER_CRITICAL_FROM_ISR()` / `taskEXIT_CRITICAL_FROM_ISR( x )`

作用：ISR 内的屏蔽/恢复。返回**进入前的 `BASEPRI` 值**，退出时必须原样传回。
参数：`x` 必须是 `FROM_ISR` 的返回值，**不能自己编一个数**。
常见误用：返回值丢进 `void`；用 `taskEXIT_CRITICAL()` 配它。

### `UBaseType_t portSET_INTERRUPT_MASK_FROM_ISR( void )` / `portCLEAR_INTERRUPT_MASK_FROM_ISR( x )`

作用：**移植层**的接口，`portSET_INTERRUPT_MASK_FROM_ISR()` 就是 `ulPortRaiseBASEPRI()`
（`portable/GCC/ARM_CM3/portmacro.h:117`）。
关系：`taskENTER_CRITICAL_FROM_ISR()` 在多数移植上是它的宏别名，**不要两套混用**。

### `portYIELD_FROM_ISR( x )`

作用：在 ISR 末尾按 `x`（`BaseType_t`，被 `FromISR` 函数置位）决定是否立刻触发上下文切换。
实现：置 PendSV（`portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT`）。
常见误用：漏调用 ⇒ 更高优先级任务要等**下一个 tick** 才被调度，表现为"延迟 1 ms 才响应"。
**漏调用不会报错**，这是最难查的一类。

### `void vTaskSuspendAll( void )` / `BaseType_t xTaskResumeAll( void )`

作用：暂停调度器（**不关中断**）。
与临界区的区别：`SuspendAll` 期间中断照常进，但不会有任务切换；可以调 `pvPortMalloc`。
常见误用：写成 `vTaskSuspendScheduler`——**教材此处是笔误**，正确的名字是 `vTaskSuspendAll`（`tasks.c:6471`）。
`xTaskResumeAll()` 返回 `pdTRUE` 表示这期间有任务被挂起过、需要重新调度。

---

## 4. 最小示例

```c
/* 任务侧：只有几条指令，中间不许 return */
taskENTER_CRITICAL();
{
    g_frames++;
    g_last = s_ring.head;      /* 两个变量必须一起更新 -> 才需要临界区 */
}
taskEXIT_CRITICAL();

/* ISR 侧：抬 BASEPRI 保护共享计数，再发队列 */
void USART1_IRQHandler(void)
{
    UBaseType_t saved = taskENTER_CRITICAL_FROM_ISR();
    uint32_t n = ++g_isr_count;
    taskEXIT_CRITICAL_FROM_ISR(saved);   /* 必须传回 saved */

    BaseType_t woken = pdFALSE;
    uint8_t ch = uart_read();
    (void)xQueueSendFromISR(s_q, &ch, &woken);
    portYIELD_FROM_ISR(woken);           /* 漏了这句 = 白等一个 tick */
}
```

**中断本身的配置**才是同伴：调 `FromISR` 的那个外设，NVIC 优先级数值必须 **≥ 5**。

---

## 5. 常见错误与陷阱

| 陷阱 | 症状 | 根因 | 处理 |
| --- | --- | --- | --- |
| 中断优先级数值 < 5 却调 `FromISR` | 偶发死机 / `configASSERT` 报优先级非法 | 临界区屏蔽不住它 | NVIC 优先级改成 ≥ 5 |
| 漏 `portYIELD_FROM_ISR` | 高优先级任务延迟 1 tick | 没触发 PendSV | ISR 末尾必调 |
| 临界区里 `return` | 之后再也进不去临界区 | 嵌套计数不平衡 | 出临界区只有一个出口 |
| 临界区太长 | 串口丢字节、tick 抖动 | 所有 5–15 中断被延后 | 缩短到几条指令 |
| 在 ISR 里等队列/延时 | 死机 | ISR 不能阻塞 | 用 `FromISR` 非阻塞版 |
| 两套临界区 API 混用 | `BASEPRI` 被恢复成错的值 | 恢复值不匹配 | 任务侧/ISR 侧各用一套 |
| 用临界区做长时间互斥 | 系统整体卡顿 | 关的是**中断**不是锁 | 用互斥量 |
| 忘了 `configCHECK_HANDLER_INSTALLATION` 与向量路由的关系 | 启动即断言失败 | 本仓库走**间接路由**（`freertos_vectors.s` 尾跳转） | 配 `0` 并保持向量表不动 |

---

## 6. 调试方法

1. **先确认"中断进没进"**：ISR 第一行递增一个计数，主循环打印。**"没响应"和"没进中断"是两件事**。
2. **确认优先级数值**：调试时打印 `NVIC_GetPriority(IRQn)`，与 5 比。数值 < 5 就是配置错。
3. **量临界区长度**：进/出各读一次 `DWT->CYCCNT`（本仓库 QEMU 不建模 DWT，`CYCCNTENA` 读回 0，
   只能在真板做），或用一个空闲的定时器计数。
4. **量中断延迟**：GPIO 翻转 + 示波器；或记录 ISR 入口 tick 与事件发生 tick 的差。
   **本仓库没有这项证据**，见 §8。
5. **`configASSERT` 必须真的生效**：确认 `configASSERT` 指向一个会停机的实现。若它被定义成空宏，
   `portASSERT_IF_INTERRUPT_PRIORITY_INVALID` 这类检查等于没开。

---

## 7. 与相近技术的对比

| 对比 | 临界区（`BASEPRI`） | `vTaskSuspendAll` | 互斥量 | 关总中断（`PRIMASK`） |
| --- | --- | --- | --- | --- |
| 关中断 | 只关 ≥5 的 | ❌ 不关 | ❌ 不关 | 全关 |
| 阻止任务切换 | ✅ | ✅ | ❌（阻塞式） | ✅（因为 tick 停了） |
| 可嵌套 | ✅ 计数 | ✅ 计数 | ✅ 递归互斥量 | 需自行处理 |
| 可阻塞 | ❌ | ❌ | ✅ | ❌ |
| 适用 | 几条指令 | 长一点的原子段 | 跨任务互斥 | 极短、需绝对原子 |

**与 Linux 的对比**：`taskENTER_CRITICAL` ≈ `local_irq_save` 的一个子集（只挡下半区，高优先级仍可进）；
`vTaskSuspendAll` ≈ `preempt_disable()`；互斥量的优先级继承 ≈ `rt_mutex`（Linux `PREEMPT_RT` 才有）。
**注意**：FreeRTOS 没有 Linux 的 `spin_lock_irqsave` 那种"自旋 + 关本地中断"的组合语义，
它假设单核且不可抢占的只有调度器。

---

## 8. 验证状态与边界

| 结论 | 级别 | 证据 |
| --- | --- | --- |
| 优先级继承能让持锁任务临时升到高优先级 | **实测** | `exp16_priority_inversion.log`：`mutex` 轮低优先级任务 `prio 1→3`（`give` 内部回 1） |
| 二值信号量**没有**继承，导致反转 | **实测** | 同日志：`binary-sem` 轮高优先级等锁 405 tick vs `mutex` 轮 105 tick |
| 栈溢出能被钩子抓到并停机 | **实测** | `exp15_stackoverflow.log`：`depth=60` 时 `hwm=0` → 钩子报任务名 |
| `BASEPRI = 0x50` 的实际屏蔽行为 | **源码核对** | `FreeRTOSConfig.h:90-101` 的换算；**未用示波器/计数器验证屏蔽范围** |
| 中断优先级配置错误的后果 | **推理** | 机制上成立；**未构造"优先级 < 5 且调 FromISR"的对照实验** |
| `PRIGROUP` 未显式设置的影响 | **推理** | 全仓无 `NVIC_SetPriorityGrouping` 调用；未读 `AIRCR` 确认 |
| 临界区长度与中断延迟 | **未验证** | QEMU 无中断延迟模型；需真板 + 示波器 |
| 漏 `portYIELD_FROM_ISR` 的 1 tick 延迟 | **未验证** | 需对照实验 |

**明确不能说的话**：不能说"本仓库验证了中断响应时间"——只验证了优先级继承的**相对**效果，没有绝对时延证据。

---

## 证据来源

- 配置：`experiments/F103_common/vendor/freertos/include/FreeRTOSConfig.h:90-110`
- 移植层：`experiments/F103_common/vendor/freertos/Source/portable/GCC/ARM_CM3/port.c`、`portmacro.h:117`
- 实测日志：`tests/qemu-stm32/logs/exp16_priority_inversion.log`、`exp15_stackoverflow.log`
- 实验：`experiments/F103_16_FreeRTOS_PriorityInversion/`、`F103_15_FreeRTOS_StackOverflow/`
- 讲义：`notes/FreeRTOS入门/Day10_中断管理与资源管理.md`（教材笔误见 `:252-265`、分组见 `:236`）
- 相关：[FreeRTOS 同步语义与优先级反转](FreeRTOS同步语义与优先级反转_速查表.md)、[FreeRTOS 任务与调度](FreeRTOS任务与调度_速查表.md)
