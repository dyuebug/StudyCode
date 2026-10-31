# Day10：中断管理、资源管理与调试（教材第 17–19 章）

**验证状态**：栈溢出的 **paint 半边实测**（`exp15_stackoverflow.log`：逐级加深至 `depth=60` 时
`hwm=0`，切换时钩子报 `任务 probe` 并停机；`run.sh exp15` 判定通过，3 条负向验证按预期失败）；
堆耗尽的钩子**实测**（`F103_13` 首版就是被它停机的，见 `F103_13` README）；
**FromISR 两处用法实测**（`F103_11:104-116`、`F103_12:286-292` 的 `...FromISR` + `portYIELD_FROM_ISR`）；
**源码核对**：`xHigherPriorityTaskWoken` 三态、`portYIELD_FROM_ISR` 与 `portEND_SWITCHING_ISR` 的别名关系、
`taskENTER_CRITICAL` 的 BASEPRI 机制与任务态断言、`taskCHECK_FOR_STACK_OVERFLOW` 的 `=1`/`>1` 两套实现、
`uxTaskGetStackHighWaterMark` 的返回单位。
**未验证**：Trace 宏、运行时间统计、`vTaskSuspendAll`、ISR 版临界区——本仓库前两项的三个配置宏全为 0
（`FreeRTOSConfig.h:71-73`），后两项**无任何实验**；教材的三个示例工程本机没有。

---

## 学习目标

学完本篇应能用代码证据回答：

1. 教材给了"两套 API"的 4 条理由，哪一条是**结构性的**（换不掉）？为什么反过来"FromISR 用在任务里"却允许？
2. `xHigherPriorityTaskWoken` 有几种取值？传 `NULL` 的后果是什么？**为什么**内核不自己切换？
3. `portYIELD_FROM_ISR(x)` 和 `portEND_SWITCHING_ISR(x)` 是什么关系？源码里能证明吗？
4. 教材 17.4 的示例"上机时感觉不到有什么不同"——差别是真的不存在，还是不可观测？
5. `taskENTER_CRITICAL()` 屏蔽的到底是哪些中断？在 ARM_CM3 上它执行的是哪一条指令？
6. 从 ISR 里误调 `taskENTER_CRITICAL()` 会怎样？内核会拦吗？
7. 本仓库的 `configMAX_SYSCALL_INTERRUPT_PRIORITY` 是 `0x50`，哪些优先级的中断因此**不允许**调 FreeRTOS API？
8. `configCHECK_FOR_STACK_OVERFLOW = 2` 是"更精确的方法 2"，还是"方法 1 + 方法 2"？
9. `uxTaskGetStackHighWaterMark` 返回的单位是什么？教材说"N/4"在什么前提下才对？
10. 本仓库能用教材 19.1.3 的 Trace 与 19.2.2 的运行时间统计吗？

---

## 环境与前置条件

| 项 | 版本 / 说明 |
| --- | --- |
| 教材 | 第 17 章中断管理（`rtos.txt:6118-6422`）、第 18 章资源管理（`:6423-6491`）、第 19 章调试与优化（`:6492-6739`） |
| 内核 | FreeRTOS **V11.3.0**；移植层 `Source/portable/GCC/ARM_CM3/`（`port.c`、`portmacro.h`） |
| 关键配置 | `configCHECK_FOR_STACK_OVERFLOW 2`、`configUSE_MALLOC_FAILED_HOOK 1`（`FreeRTOSConfig.h:69-70`）；`configUSE_TRACE_FACILITY 0`、`configUSE_STATS_FORMATTING_FUNCTIONS 0`、`configGENERATE_RUN_TIME_STATS 0`（`:71-73`）；`INCLUDE_uxTaskGetStackHighWaterMark 1`（`:87`）；`configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5`（`:97`） |
| 实验 | `F103_11`（ISR→队列）、`F103_12`（ISR→信号量）、`F103_15`（栈溢出钩子）、`F103_13`（堆耗尽钩子首触发） |
| 日志 | `tests/qemu-stm32/logs/exp15_stackoverflow.log`、`exp11_freertos_isr_queue.log`、`exp12_sync.log` |
| 前置 | `Day03`（任务状态）、`Day05`（`FromISR` 无超时）、`Day07`（互斥量不能在 ISR 里用） |

---

## 核心概念

### 1. 两套 API 的 4 条理由里，只有第 1 条是结构性的

教材先讲中断处理流程（保存现场 → 分辨中断 → 恢复现场），然后给出"ISR 必须快"的两条后果
（`rtos.txt:6120-6136`）：低优先级中断得不到处理、任务被卡住。

接着它列出**为什么不能只留一套 API**（`rtos.txt:6140-6160`）：

| # | 教材理由 | 性质 |
| --- | --- | --- |
| 1 | 增加"当前是否在中断里"的分支，函数更长、更难测试 | **结构性**——每个调用点都付一次分支成本 |
| 2 | 参数不同：任务版要超时，ISR 版必须立刻返回 | 结构性（接口语义不同） |
| 3 | 需要移植层的 `is_in_isr()` 支持 | 移植成本 |
| 4 | 有些架构难以分辨中断上下文 | 平台限制 |

第 1 条和 Day04 §5"为什么不把阻塞写进锁 API"是同一条思路：
**内核不想在热路径上为"你可能不会用到的能力"付成本。**

于是教材给了三条解法（`rtos.txt:6162-6176`）：延迟到任务处理、用 `FromISR`、库自带 OS 抽象层。
其中第二条附了一句极重要的括号说明：

> （`FromISR` 函数在任务里也可以使用，反之不行）

**为什么允许反向？** 因为 `FromISR` 版的约束更严——不阻塞、立刻返回、只做最小操作。
一个"不阻塞的函数"在任务里调用当然合法，只是失去了阻塞能力而已。
本仓库**没有**这种用法（两处 `FromISR` 都在真 ISR 里），所以这条是教材原文，不是实测。

### 2. `xHigherPriorityTaskWoken` 是三态，且**可以传 NULL**

教材给了三态（`rtos.txt:6180-6190`）：

| 取值 | 含义 | 调用方要做的事 |
| --- | --- | --- |
| `pdFALSE` | 没有唤醒更高优先级任务 | 什么也不做 |
| `pdTRUE` | 唤醒了更高优先级任务 | 立即 `portYIELD_FROM_ISR` |
| （传 `NULL`） | 不在乎 | **不会切换**，被唤醒的任务等到下一个 tick |

三态的第三行是"能力"也是"陷阱"：`NULL` 合法，编译器不报错，但语义退化。

教材 17.4 的示例正好演示了这个退化（`rtos.txt:6280-6320`）：
改前三次 `xQueueSendFromISR(..., NULL)`；改后引入 `xHigherPriorityTaskWoken` 并传给
`portYIELD_FROM_ISR`。教材对这次改动的效果自述是：

> 本程序上机时，我们感觉不到有什么不同。

这句话比它看起来重要——**它是"现象不可观测 ≠ 没有差别"的现成实例。**
差别在"高优先级任务晚多久开始运行"：不 yield 时晚**最多一个 tick**（1 ms 量级），
而这个示例里按键处理任务本来就要等按键，多 1 ms 完全看不出来。
判据不是"眼睛看不出来"，而是"tick 边界上有没有 PendSV 挂起"——那需要逻辑分析仪或 trace。

Day05 §4 讲 `F103_11` 的 `trunc=4` 时说过同一件事的另一面：
**能计数才能判定**。这里是"不能计数就只能靠感觉"，而感觉会给出错误结论。

### 3. 内核不在 API 内部切换：三条理由指向同一个判据

教材问"为什么不在函数内部直接切换"（`rtos.txt:6196-6210`），给了三条：

| 理由 | 展开 |
| --- | --- |
| 效率 | 一次 ISR 里连调 N 个 `FromISR`，只在最后切一次；内部切换要切 N 次 |
| 更可控 | 切换点由调用方决定，写在代码里 |
| 可移植性 | 切换机制在移植层（PendSV / SVC / 软件中断），内核层不碰 |

第三条最容易忽略：`portYIELD()` 在 ARM_CM3 上是**挂起 PendSV**，不是立即跳转
（`portmacro.h` 里 `portYIELD()` → `portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT`）。
中断返回时硬件才做栈切换。所以"内部切换"在 ARM 上甚至**做不到立即**，
只能"挂起后再挂起"——挂了 N 次还是一次。

### 4. `portYIELD_FROM_ISR` 就是 `portEND_SWITCHING_ISR` 的别名——源码可证

教材说两个宏"做的事情完全一样，新版统一用后者"（`rtos.txt:6216-6218`）。源码比这句话更直接：

```c
/* Source/portable/GCC/ARM_CM3/portmacro.h:98-110 */
#define portEND_SWITCHING_ISR( xSwitchRequired ) \
    do { if( xSwitchRequired != pdFALSE ) { traceISR_EXIT_TO_SCHEDULER(); portYIELD(); } \
         else { traceISR_EXIT(); } } while( 0 )

/* :111 —— 就这一行，没有别的实现 */
#define portYIELD_FROM_ISR( x )    portEND_SWITCHING_ISR( x )
```

两个可读出来的事实：

1. **"统一用后者"的实质是"保留别名"**，不是"后者被重写"。
   `portYIELD_FROM_ISR` 从来没有自己的实现——它是宏别名，编译后完全等价。
2. **判断条件是 `!= pdFALSE`，不是 `== pdTRUE`**。二者在 `pdFALSE=0/pdTRUE=1` 时等价，
   但如果某个移植层的 `FromISR` 函数返回了别的非零值（例如 `errQUEUE_FULL` 的 `-1`），
   `== pdTRUE` 会漏判。这条**只能靠读源码知道**，教材没写。

### 5. 中断延迟处理：t1–t4 与"deferred processing task"

教材 17.2 给了四时刻时序（`rtos.txt:6230-6260`）：中断发生（t1）→ ISR 只做最小事并唤醒任务（t2）
→ 中断返回（t3）→ 任务开始处理（t4）。教材的核心论断是：

> 从 t2 到 t4 之间，中断已经返回，其他中断可以继续被响应。

这条论断的代价是**引入了延迟**（t4−t2），也就是"中断延迟处理"这个名字里"延迟"的来源。
本仓库 `F103_11` 就是这一形态：ISR 里只 `xQueueSendFromISR` 一个字节
（`main.c:104`），拼行与打印都在任务里。

`F103_11:129` 还记了一条本仓库特有的坑：

```text
ISR 里的 portYIELD_FROM_ISR 会挂起 PendSV，但内核还没有当前任务
```

即**调度器启动之前**的 ISR 里 yield，PendSV 挂起了却没有任务可切。
这不是教材覆盖的场景，但它在"初始化阶段就开中断"的工程里很常见。

### 6. 三种独占方法：公平竞争、灭掉中断、灭掉调度

教材第 18 章的开场（`rtos.txt:6425-6431`）把三种方法用一个比喻串起来：

| 方法 | 教材措辞 | 代价 |
| --- | --- | --- |
| 公平竞争 | 用互斥量 | 依赖持有者守规矩（Day07 §2）；有继承成本 |
| 屏蔽中断 | "中断要跟我抢？我屏蔽中断" | **中断无法运行**，且任务调度依赖中断 ⇒ 也不会有任务切换 |
| 暂停调度 | "其他任务要跟我抢？我禁止调度器" | 中断照常，只是不切任务 |

第二、三行是本章的全部内容，也是最容易被误用的两行：
**它们都"能防住别人"，但防的方式不同，恢复的东西也不同。**

### 7. `taskENTER_CRITICAL` 在 ARM_CM3 上是抬高 BASEPRI，不是 `cpsid i`

教材说"低优先级的中断被屏蔽了：优先级低于、等于 `configMAX_SYSCALL_INTERRUPT_PRIORITY`"
（`rtos.txt:6445-6447`）。这句话的**方向容易读反**——ARM 里"优先级高"= 数值小。压成数值表：

```text
BASEPRI = configMAX_SYSCALL_INTERRUPT_PRIORITY = 5 << 4 = 0x50   （FreeRTOSConfig.h:100-101）

中断优先级数值  0x00–0x4F  →  BASEPRI 挡不住  →  照常响应  →  【不允许】调 FreeRTOS API
中断优先级数值  0x50–0xFF  →  BASEPRI 挡住    →  被屏蔽    →  【允许】调 FromISR API
```

本仓库 `FreeRTOSConfig.h:92` 的注释已经把这个方向写清了："数值 0~4 的中断不受临界区屏蔽"。

源码层面的两个事实，教材都没写：

```c
/* portmacro.h:119 —— "关中断"的真身 */
#define portDISABLE_INTERRUPTS()   vPortRaiseBASEPRI()

/* portmacro.h:228-243 —— 抬高 BASEPRI，返回【原来的】值 */
portFORCE_INLINE static uint32_t ulPortRaiseBASEPRI( void )
{
    __asm volatile ( " mrs %0, basepri \n mov %1, %2 \n msr basepri, %1 \n isb \n dsb \n"
                     : "=r"(ulOriginalBASEPRI), "=r"(ulNewBASEPRI)
                     : "i"(configMAX_SYSCALL_INTERRUPT_PRIORITY) : "memory" );
}
```

1. **ARM_CM3 移植层的"关中断"不是全关**。它把 BASEPRI 抬到 `0x50`，
   数值 0–4 的中断**仍在跑**。所以 `taskENTER_CRITICAL` 保护的临界区，
   只能防住"会调 FreeRTOS API 的那一半世界"。
2. 返回原 BASEPRI 值是给 `taskENTER_CRITICAL_FROM_ISR` 用的（见 §8）。

**这条直接解释了一个常见故障**：在优先级 0 的中断里改一个和任务共享的变量，
加 `taskENTER_CRITICAL` **完全无效**——因为那个中断根本没被挡住。
教材 18.1.1 只提了一句"这些中断 ISR 里不允许使用 FreeRTOS 的 API"，
没提"它们也不受临界区保护"这另一半。

### 8. 从 ISR 误调 `taskENTER_CRITICAL`：内核真的会拦

教材只说"任务中用 `taskENTER_CRITICAL`，ISR 中用 `..._FROM_ISR`"（`rtos.txt:6435-6442`）。
源码里多了一层运行时检查：

```c
/* port.c:426-440 */
void vPortEnterCritical( void )
{
    portDISABLE_INTERRUPTS();
    uxCriticalNesting++;

    if( uxCriticalNesting == 1 )   /* 只在最外层检查，否则 configASSERT 自身用临界区会误报 */
    {
        configASSERT( ( portNVIC_INT_CTRL_REG & portVECTACTIVE_MASK ) == 0 );
    }
}
```

`portVECTACTIVE_MASK` 读的是 SCB->ICSR 的 VECTACTIVE 字段：**非 0 表示当前在中断里**。
所以从 ISR 调 `taskENTER_CRITICAL()` 会断言失败——**前提是 `configASSERT` 有效**
（本仓库 `FreeRTOSConfig.h:116-122` 定义为 `vAssertCalled(__FILE__, __LINE__)`）。

注意 `if( uxCriticalNesting == 1 )` 这个条件本身就是一条设计纪律：
注释说明了原因——`configASSERT` 自己的实现里如果也用临界区，递归调用会误报。
**"检查逻辑不能依赖被检查的机制"**，这条在 Day06 §4 的"初值 0 要补 give"里是同一类思维。

ISR 版为什么必须配对恢复？因为它要恢复**原值**而不是归零：
`portSET_INTERRUPT_MASK_FROM_ISR()` = `ulPortRaiseBASEPRI()`（返回旧值，
`portmacro.h:117`），`portCLEAR_INTERRUPT_MASK_FROM_ISR(x)` = `vPortSetBASEPRI(x)`
（写回旧值，`:118`）。如果 ISR 被更高优先级中断嵌套过，归零会把外层设的屏蔽一起撤掉。

### 9. `vTaskSuspendAll` 不关中断——教材这里写错了函数名

教材 18.2 先正确声明了两个函数（`rtos.txt:6471`、`:6481`）：

```c
void vTaskSuspendAll( void );
BaseType_t xTaskResumeAll( void );
```

但四行后的示例（`rtos.txt:6483-6484`）写成了：

```c
vTaskSuspendScheduler();
xTaskResumeScheduler();
```

**这两个函数在 FreeRTOS 里不存在。** 紧接着 `:6486-6487` 又把上一节的结论复制过来：

> 这套 vTaskSuspendScheduler()/xTaskResumeScheduler() 宏，是可以递归使用的，
> 它的内部会记录嵌套的深度，只有嵌套深度变为 0 时，调用 taskEXIT_CRITICAL() 才会重新使能中断。

三处错误叠在一起：

| 位置 | 错误 | 后果 |
| --- | --- | --- |
| `:6483-6484` | 函数名 `vTaskSuspendScheduler` / `xTaskResumeScheduler` 不存在 | 照抄**编译失败**（这个比静默错好） |
| `:6486` | 同一句里把这对函数叫"宏" | 术语错，它们是函数 |
| `:6486-6487` | "只有嵌套深度变为 0 时，调用 `taskEXIT_CRITICAL()` 才会重新使能中断" | **把 18.1.1 的结论搬到了 18.2**。`xTaskResumeAll` 恢复的是**调度**，它不碰中断使能；而 `taskEXIT_CRITICAL` 是上一节的 API |

正确表述应该是：`vTaskSuspendAll`/`xTaskResumeAll` 只暂停**任务切换**，中断照常响应；
嵌套计数归零时恢复的是"可以切任务"这件事，不是"中断可以被响应"。

**为什么这一节特别值得记**：18.1 与 18.2 的区别正是"屏蔽中断 vs 屏蔽调度"。
教材的复制粘贴把两者的差异抹掉了，而这恰好是本章唯一的知识点。
照抄的读者会以为两者都能"关中断"，于是在需要"中断照常、只是别切任务"的场景里用错工具。

### 10. 教材 18.1.1 还有一处拼写错误

`rtos.txt:6438`、`:6450`、`:6452` 出现三次 `taskENTER_CRITICA()`——**漏了一个 `L`**。
真实符号是 `taskENTER_CRITICAL()`。同样表现为编译失败，不静默。

三处（§9 的两处 + 本节一处）合起来说明一件事：**这一章的文字没有经过编译验证。**
所以本章的代码块只能当"意图说明"，不能当"可编译示例"。

### 11. 四种调试手段与本仓库的可用性对照

教材 19.1 列了四种（`rtos.txt:6492-6496`）：打印、`configASSERT`、Trace、Hook。
把"教材说什么"和"本仓库能不能用"并排放，差别很大：

| 手段 | 教材立场 | 本仓库配置 | 实际可用性 |
| --- | --- | --- | --- |
| `printf`（实现 `fputc`） | 工程用 microlib，实现 `fputc` 即可 | 自撰 `retarget.c`（见 `F103_HAL_DriverLab`） | **可用**（全部实验都在用） |
| `configASSERT` | 可加 `printf("%s %s %d", __FILE__, __FUNCTION__, __LINE__)` | `vAssertCalled(__FILE__, __LINE__)`（`:116-122`） | **可用**，但**故意不含 `__FUNCTION__`**（见 §12） |
| Trace 宏 | 19.1.3 列了 13 个宏 | `configUSE_TRACE_FACILITY 0`（`:71`） | **不可用**——宏本身可以定义，但 `traceTASK_SWITCHED_*` 之外的多数宏要靠 `configUSE_TRACE_FACILITY` 编译进去 |
| Hook（malloc / 栈溢出） | 19.1.4–19.1.5 | 两个都**已开**（`:69-70`） | **可用且实测触发过** |
| 运行时间统计（19.2.2） | 需更快的定时器 | `configGENERATE_RUN_TIME_STATS 0`、`configUSE_TRACE_FACILITY 0`、`configUSE_STATS_FORMATTING_FUNCTIONS 0`（`:71-73`） | **完全不可用**——三个宏全 0，`vTaskList`/`vTaskGetRunTimeStats` 不存在编译产物 |

**最后一行必须讲清**，因为它决定了"本仓库能不能回答哪个任务占了多少 CPU"：
答案是**不能**，只能回答"哪个任务栈还剩多少"（§14）。教材 19.2.2 的原理与配置在本仓库
是纯文字知识，任何"实测 CPU 占用"的说法在这里都没有依据。

### 12. `configASSERT` 的两种写法，本仓库为什么选后者

教材建议的写法（`rtos.txt:6505-6510`）：

```c
#define configASSERT(x) \
    if (!x) { printf("%s %s %d\r\n", __FILE__, __FUNCTION__, __LINE__); while(1); }
```

本仓库的写法（`FreeRTOSConfig.h:116-122`）：

```c
#define configASSERT( x )                                                    \
    do { if( ( x ) == 0 ) { vAssertCalled( __FILE__, __LINE__ ); } } while( 0 )
```

三处差别，每一处都有理由：

| 差别 | 教材 | 本仓库 | 为什么 |
| --- | --- | --- | --- |
| 包 `do{}while(0)` | 没有 | 有 | 裸 `if` 在 `if (a) configASSERT(x); else ...` 里会把 `else` 抢走（悬空 else）。这是宏的常识，教材漏了 |
| 带 `__FUNCTION__` | 有 | **没有** | `__FUNCTION__` 是 GCC 扩展（C23 起才是 `__func__`）；更实际的理由是**省栈和省 flash**——断言路径已经在异常里，多一个字符串只增加定位成本的下限 |
| 用 `printf` | 有 | 改成 `vAssertCalled` 回调 | **串口本身可能就是出错的那一环**。`configASSERT` 要能在一个"打印已经不能用"的系统里工作 |

第三条是本篇最值得带走的一条：
**诊断工具不能依赖被诊断的通道。** 这与 §7 的 `if( uxCriticalNesting == 1 )`
（检查逻辑不能依赖被检查的机制）是同一个原则的两次出现。

### 13. 栈溢出检测：`=2` 是"方法 1 **或** 方法 2"，不是"更精确的方法 2"

教材把两种方法并列（`rtos.txt:6657-6662`）：

| 方法 | 教材描述 | 教材评价 |
| --- | --- | --- |
| 1 | 切换出去之前，运行现场都在栈里，此时"很可能"是峰值 | 很高效，但**不精确** |
| 2 | 建栈时填 `0xa5`，检查最后 16 字节 | 不如方法 1 快，但能捕获几乎所有溢出 |

源码里 `configCHECK_FOR_STACK_OVERFLOW` 的取值不是"选哪种方法"，而是**"用几种方法"**：

```c
/* stack_macros.h:70-84  —— =1：只看 SP */
#if ( ( configCHECK_FOR_STACK_OVERFLOW == 1 ) && ( portSTACK_GROWTH < 0 ) && ... )
    if( pxCurrentTCB->pxTopOfStack <= pxCurrentTCB->pxStack + portSTACK_LIMIT_PADDING )
        vApplicationStackOverflowHook( ... );

/* stack_macros.h:103-121 —— >1：SP 越界 【或】 栈底 4 个 32 位字不是 0xa5a5a5a5 */
#if ( ( configCHECK_FOR_STACK_OVERFLOW > 1 ) && ( portSTACK_GROWTH < 0 ) && ... )
    const uint32_t ulCheckValue = 0xa5a5a5a5U;
    if( ( pxCurrentTCB->pxTopOfStack <= pxCurrentTCB->pxStack + portSTACK_LIMIT_PADDING ) ||
        ( pulStack[0] != ulCheckValue ) || ( pulStack[1] != ulCheckValue ) ||
        ( pulStack[2] != ulCheckValue ) || ( pulStack[3] != ulCheckValue ) )
        vApplicationStackOverflowHook( ... );
```

三个教材没写的要点：

1. **`>1` 是"或"**——它**保留**了 SP 检查（方法 1），再**加上** paint 检查（方法 2）。
   所以 `=2` 不是"换成更精确的方法"，而是"两种都做"。
2. **`pulStack[0..3]` 正好 16 字节**（4 × `uint32_t`）。教材"检查最后 16 字节"这个数字
   **就是从这四行来的**，不是随口说的。
3. **检查点在 `vTaskSwitchContext` 里、切换前**（`tasks.c:5164`，注释写着
   "Before the currently running task is switched out"），检查对象是 `pxCurrentTCB`。
   这就是"方法 1 不精确"的机制原因：**只看到切换那一刻的 SP**。
   函数 A 吃了 200 字节又返回、SP 回到原处，A 的峰值不留任何痕迹。
4. `portSTACK_LIMIT_PADDING` 默认 **0**（`stack_macros.h:52-53`）——本仓库没有额外余量，
   所以 SP 一越过栈底就报。

本仓库 `configCHECK_FOR_STACK_OVERFLOW 2`（`FreeRTOSConfig.h:70`），
即"两种都做"。`F103_15` 触发的正是**第二种**：`main.c:12-13` 的注释写得很明确——
"任务返回后栈指针本身合法，被破坏的是栈块底部 16 字节的 0xA5 填充（`=2` 的第一个检查）"。

### 14. `uxTaskGetStackHighWaterMark` 的返回值：教材的"N/4"是平台结论

教材明确写了单位（`rtos.txt:6684`）：

> 假设从栈尾开始连续为 0xa5 的栈空间是 N 字节，返回值是 N/4。

源码核对（`tasks.c:6370-6385`）：

```c
static configSTACK_DEPTH_TYPE prvTaskCheckFreeStackSpace( const uint8_t * pucStackByte )
{
    configSTACK_DEPTH_TYPE uxCount = 0U;
    while( *pucStackByte == ( uint8_t ) tskSTACK_FILL_BYTE )
    {
        pucStackByte -= portSTACK_GROWTH;      /* 向下增长就往下走 */
        uxCount++;                             /* 逐【字节】数 0xa5 */
    }
    uxCount /= ( configSTACK_DEPTH_TYPE ) sizeof( StackType_t );   /* ← 除的是 sizeof，不是 4 */
    return uxCount;
}
```

结论修正：**返回值 = 空闲字节数 / `sizeof(StackType_t)`**。

- 本仓库 `StackType_t = uint32_t`（32 位栈，`configUSE_16_BIT_TICKS` 未定义）⇒ `N/4`，教材成立。
- 若 `StackType_t = uint16_t`（16 位栈配置）⇒ 返回 `N/2`，教材的"N/4"就**错一倍**。

所以教材这句话是**平台相关结论被写成了通例**。判据级的写法是：
**先查 `sizeof(StackType_t)`，再解释返回值**；直接背"N/4"换平台就错。

另外两个默认行为要一起看：
- 传入 `NULL` 表示查**自己**（`INCLUDE_uxTaskGetStackHighWaterMark 1`，`FreeRTOSConfig.h:87`）。
- 它只读 paint 的残留，所以**必须在任务跑过最深的路径之后再读**——
  `F103_15` 的判据就是"`hwm=0` 即栈底 16 B 已被写穿"。这句话的前提是
  §13 的第 2 点：paint 检查和 hwm 读的是**同一段 16 字节**，所以两者能互相印证。

### 15. 本仓库的栈溢出实测读数（`F103_15`）

这是本篇唯一"钩子真的响了一次"的证据，分三段（`exp15_stackoverflow.log`）：

```text
配置      configCHECK_FOR_STACK_OVERFLOW=2（2 = paint + SP 双重检查），任务栈 256 字
① 对照    depth=12 → 最低地址 0x200004A0，余量 160 字（判据 ≥32）→ 钩子不响   ✓ 不误报
② 标定    depth=16 → 最低地址 0x20000460 ⇒ 每帧 16 B                    ← 不猜深度
③ 注入    depth 16→60 余量逐级 152/152/144/128/112/96/80/64/48/32/16/0
          首次余量 <32 在 depth=56；余量归零在 depth=60（按余量线性预测是 52）
          depth=60 时 hwm=0 ⇒ 栈底 16 B 的 0xA5 被写穿；越界量不超过一帧（16 B）
          → [STACK OVERFLOW] 任务 probe 的栈被写穿，钩子已触发
```

三条读法，每条都对应一个判据设计教训：

1. **必须先用 `depth=12` 证明"钩子不误报"**，否则后面"钩子响了"不能归因于溢出。
   这与 Day06 的 `coalesced` 是同一类纪律：**先证明判据能说"没发生"**。
2. **必须先标定 16 B/帧，再挑深度**。首版用"余量反推栈底地址"的算术漏检
   （解得 `0x20000240`，注入点在 `0x20000248`，落在 16 B paint 之外）——
   **判据要用余量，不要用地址算术**。这条已经写进 `main.c:8-9` 的注释。
3. **`hwm=0` 与"钩子响了"是两个事件，前者先于后者**。钩子是在**下一次任务切换时**
   才被检查到的（§13 第 3 点），所以日志里"写穿"和"报告"之间有延迟。
   判据只写"两者都发生"，不写"同时发生"。

### 16. 教材三章共 6 处缺陷（只记录，不改教材代码）

| 位置 | 缺陷 | 照抄的后果 |
| --- | --- | --- |
| `rtos.txt:6483-6484` | `vTaskSuspendScheduler()` / `xTaskResumeScheduler()` **不存在**（正确是 `vTaskSuspendAll` / `xTaskResumeAll`，`rtos.txt:6471`/`:6481` 就在上方四行） | 编译失败 |
| `rtos.txt:6486-6487` | 把这对函数叫"宏"，并复制了 18.1.1 的结论"`taskEXIT_CRITICAL()` 才会重新使能中断" | **概念错**：`xTaskResumeAll` 恢复的是**调度**不是中断。这一章唯一的知识点被抹掉 |
| `rtos.txt:6438/6450/6452` | `taskENTER_CRITICA()` 漏 `L`（三次） | 编译失败 |
| `rtos.txt:6445-6447` | "低优先级的中断被屏蔽"——方向对但**全是文字**，没有给数值范围 | 读者容易把"优先级低"与"数值小"混起来，用错 BASEPRI 判断 |
| `rtos.txt:6684` | "返回值是 N/4" 写死 4 | 换成 16 位栈配置就错一倍（§14） |
| `rtos.txt:6657-6662` | 方法 1 / 方法 2 并列表述，未说明 `=2` 是"两者都做" | 以为 `=2` 放弃了 SP 检查（§13） |

六处里三处是**编译期能发现的**（函数名、拼写），三处是**只在运行时才暴露的**
（BASEPRI 方向、返回单位、`=2` 的语义）。后三处危险得多——
它们不会让你编译失败，只会让你**得出结论时用错前提**。

---

## 系统调用/接口卡片

### `portYIELD_FROM_ISR( BaseType_t xSwitchRequired )`

作用：若刚刚的 `FromISR` 调用唤醒了更高优先级任务，**挂起一次 PendSV**，
让中断返回时发生任务切换。

参数：`xHigherPriorityTaskWoken` 的值。`!= pdFALSE` 即切换（`portmacro.h:100`）。

返回值和错误：宏，无返回值。

资源所有权：无。

线程/进程安全：**仅 ISR 上下文有意义**。在任务里调用会挂起 PendSV，
但任务态本来就会被调度器抢占，通常无意义。

最小示例（本仓库 `F103_11:104-116`）：

```c
BaseType_t higher_prio_woken = pdFALSE;
if( xQueueSendFromISR( s_rx_queue, &item, &higher_prio_woken ) != pdPASS ) { trunc++; }
portYIELD_FROM_ISR( higher_prio_woken );
```

常见误用：① `FromISR` 传 `NULL` 却仍调 `portYIELD_FROM_ISR(pdTRUE)`（假的 yield）；
② 两个 `FromISR` 各用一个局部 woken 变量，只 yield 了后一个（前者被唤醒的任务要多等一个 tick）；
③ 写成 `portYIELD_FROM_ISR( &woken )`（传指针，非零恒真 ⇒ 每次都切换）。

### `portEND_SWITCHING_ISR( xSwitchRequired )`

作用：`portYIELD_FROM_ISR` 的**本体**（`portmacro.h:98-110`）。功能完全相同。

常见误用：无——两者可互换。**但不要在两个实现里选"更新的那个"**：
`portYIELD_FROM_ISR` 就是别名，不存在"新旧差异"。

### `void taskENTER_CRITICAL( void )` / `void taskEXIT_CRITICAL( void )`

作用：把 BASEPRI 抬到 `configMAX_SYSCALL_INTERRUPT_PRIORITY`，屏蔽**数值 ≥ 0x50** 的中断；
内部 `uxCriticalNesting` 计数支持递归，归零时才写回 BASEPRI=0。

参数：无。

返回值和错误：`void`。**误用于 ISR 会 `configASSERT` 失败**（`port.c:436-439`）。

资源所有权：无（不涉及内存）。

线程/进程安全：**仅任务上下文**。可嵌套，必须配对。

最小示例：

```c
taskENTER_CRITICAL();
s_shared++;              /* 只放对共享状态的判断与修改 */
taskEXIT_CRITICAL();     /* 中间不得调用任何可能阻塞的 API */
```

常见误用：① 在优先级 0–4 的中断里指望它保护共享数据（**挡不住**，§7）；
② 临界区里调 `printf` / `vTaskDelay`（前者可能阻塞在串口，后者直接断言）；
③ 漏 `taskEXIT_CRITICAL`（BASEPRI 永久 0x50，所有低优先级中断死掉，且 `uxCriticalNesting` 不为 0 会掩盖后续断言）。

### `UBaseType_t taskENTER_CRITICAL_FROM_ISR( void )` / `taskEXIT_CRITICAL_FROM_ISR( UBaseType_t saved )`

作用：ISR 版临界区。**返回原 BASEPRI**，必须用同一变量恢复
（`portmacro.h:117-118`：`ulPortRaiseBASEPRI()` / `vPortSetBASEPRI(x)`）。

参数：`saved` = `ENTER` 的返回值。

返回值和错误：`ENTER` 返回原掩码；`EXIT` 为 `void`。

常见误用：① 用 `taskEXIT_CRITICAL()` 配对 ISR 版（嵌套计数与 BASEPRI 两套机制混用）；
② 忽略返回值、直接写 `vPortSetBASEPRI(0)`（把外层屏蔽也撤了）。

### `void vTaskSuspendAll( void )` / `BaseType_t xTaskResumeAll( void )`

作用：暂停/恢复**任务调度**。中断**照常响应**——这是它与 `taskENTER_CRITICAL` 的根本区别。

参数：无。

返回值和错误：`xTaskResumeAll` 返回 `pdTRUE` 表示"暂停期间有更高优先级任务就绪了"
（教材 `rtos.txt:6475-6479`："可以不理会这个返回值"）。

资源所有权：无。

线程/进程安全：仅任务上下文；可嵌套。

常见误用：① 以为它能关中断（§9，教材自己也写错了）；
② 在暂停调度期间调用**会阻塞的 API**——阻塞需要调度器参与，此时行为退化；
③ 调用 `vTaskSuspendAll` 后又调 `taskENTER_CRITICAL`（两者可叠加，但代码里几乎总是"其中一个就够了"，
叠加说明设计没想清）。

### `void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName )`

作用：栈检查失败时的回调，由 `taskCHECK_FOR_STACK_OVERFLOW()` 在
`vTaskSwitchContext` 里调用（`tasks.c:5164`）。

参数：`xTask` 出事的任务句柄；`pcTaskName` 它的名字（指向 TCB 内的 `pcTaskName`）。

返回值和错误：`void`。**必须不返回**——检查点已经在异常里，返回会继续跑一个已损坏的任务。

资源所有权：不要在其中分配/释放任何东西（堆可能已损坏）。

最小示例（本仓库 `F103_15:232-240`）：

```c
void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName )
{
    ( void ) xTask;
    taskDISABLE_INTERRUPTS();
    printf( "\r\n[STACK OVERFLOW] 任务 %s 的栈被写穿，钩子已触发\r\n", pcTaskName );
    for( ;; ) { }
}
```

常见误用：① 钩子里 `return`；② 钩子里 `printf` 但串口本身依赖别的任务（**暂停在这里，那个任务永远不会跑**）；
③ 不打印任务名（`pcTaskName` 是本 API 最值钱的部分，栈溢出最难的就是定位是哪个任务）。

### `void vApplicationMallocFailedHook( void )`

作用：`pvPortMalloc` 失败时回调（需 `configUSE_MALLOC_FAILED_HOOK 1`，本仓库 `:69` 已开）。

返回值和错误：`void`，**必须不返回**。

最小示例（本仓库 `F103_12:306-313`）：

```c
void vApplicationMallocFailedHook( void )
{
    taskDISABLE_INTERRUPTS();
    printf( "\r\n[MALLOC FAILED] 堆耗尽，检查 configTOTAL_HEAP_SIZE\r\n" );
    for( ;; ) { }
}
```

常见误用：**把这个钩子当成"内存不足会被处理"**。它不处理，它只是"在堆耗尽那一刻停下来"。
`F103_13` 的首版就是被它停机的——4 KB 堆被"四任务 + 三个完成信号量 + ctx 在栈上"耗尽，
日志停在半截，根因是 `xTaskCreate` 的返回值从未检查（见 `F103_13` README 的"首版失败"段）。

### `UBaseType_t uxTaskGetStackHighWaterMark( TaskHandle_t xTask )`

作用：返回任务栈的**最低空闲量**（沿栈底向上数连续 `0xa5` 的字节数 ÷ `sizeof(StackType_t)`）。

参数：`xTask`；传 `NULL` 查当前任务。

返回值和错误：单位是 **字（`StackType_t` 个数）**，不是字节（§14）。无错误路径。

资源所有权：无（只读 TCB 与栈）。

线程/进程安全：任务态调用。**只在"最深的路径已经跑过"之后读才有意义**——
读的是 paint 残留，不是运行期采样。

常见误用：① 当成字节数（256 字栈读成 256 B，以为还剩很多）；
② 在任务刚创建时读（恒等于栈大小，无信息）；
③ 用它判断"还够不够"却不留余量——本仓库判据是"**绝对 ≥32 字且相对 ≥25%**"两条同时满足
（`F103_12`/`F103_13` 的 README）。

---

## 最小可运行实验

本篇用三个已有实验覆盖三章：

| 教材章节 | 替代实验 | 覆盖到什么 |
| --- | --- | --- |
| 17 中断管理（`FromISR` + `portYIELD_FROM_ISR`） | `F103_11`（ISR→队列）、`F103_12`（ISR→信号量） | 两处标准写法实测；`xHigherPriorityTaskWoken` 的**非 NULL** 用法 |
| 19.1.4 Malloc Hook | `F103_13` 首版失败 | 钩子**真实触发一次**（堆耗尽停机） |
| 19.1.5 栈溢出 Hook + 19.2.1 高水位 | `F103_15` | `=2` 的 paint 半边实测；`hwm` 判据全表 |
| 18 资源管理 | **无实验** | BASEPRI 机制、任务态断言、`vTaskSuspendAll` 全部只有源码核对 |
| 19.1.3 Trace、19.2.2 运行时间统计 | **无实验**（配置全 0） | 不可用 |

`F103_15` 的骨架（`main.c`）：

```text
probe 任务（256 字栈，prio 2）
  depth=12  递归 eat_stack(depth)  → 记录最低栈地址 → 对照：余量 160 字，钩子不响
  depth=16  同上                   → 标定每帧字节数（本机 16 B）
  depth=16..60 逐级加深            → 余量单调降到 0 → 钩子在下一次切换时报名字并停机
monitor 任务（prio 1）               → 每级打印 hwm 与当前 depth
```

---

## 编译与运行

```bash
# 栈溢出实验
cd experiments/F103_15_FreeRTOS_StackOverflow
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake -DF103_QEMU=ON
cmake --build build

cd ../..
timeout 30 qemu-system-arm -M stm32vldiscovery -display none -monitor none \
  -serial stdio -kernel experiments/F103_15_FreeRTOS_StackOverflow/build/f103_15_stackoverflow.elf \
  < /dev/null 2>&1 | tr -d '\r' | tee tests/qemu-stm32/logs/exp15_stackoverflow.log

# 判定器（两条 grep：写了 hwm=0、钩子报出了任务名）
./tests/qemu-stm32/run.sh exp15
```

`F103_11` 的 ISR 侧需要注入串口输入，见该实验 README；`F103_12` 同理。

---

## 结果观察

### 栈溢出（实测，`exp15_stackoverflow.log`）

```text
configCHECK_FOR_STACK_OVERFLOW=2（2 = paint + SP 双重检查），任务栈 256 字
对照   depth=12 → 最低地址 0x200004A0，余量 160 字（判据 ≥32）        ← 不误报
标定   depth=16 → 最低地址 0x20000460，每帧 16 B                      ← 不猜深度
逐级   depth=16..60  余量 152 152 144 128 112 96 80 64 48 32 16 0
      首次 <32 在 depth=56；归零在 depth=60（按余量预测 52）
      depth=60 时 hwm=0 ⇒ 栈底 16 B 的 0xA5 被写穿；越界 ≤ 一帧（16 B）
[STACK OVERFLOW] 任务 probe 的栈被写穿，钩子已触发
```

三条读法（与 §15 的判据教训一一对应）：先证明不误报 → 再标定 → 才注入。

### ISR 两套 API（实测，`F103_11` / `F103_12`）

```text
F103_11  rx 0 → 7 → 14；突发 300 B → trunc=4；300 = 4×64 + 44
F103_12  bytes=7 gives=2 coalesced=5
```

这两处 `...FromISR` + `portYIELD_FROM_ISR` 是**正确写法**的实测，
但**没有做"去掉 `portYIELD_FROM_ISR` 会怎样"的对照**——
所以"延迟一个 tick"这个后果在本仓库是教材 17.4 的推论（且教材自己说"感觉不到"），
不是本仓库的读数。

### 堆耗尽钩子（实测，`F103_13` 首版）

```text
[MALLOC FAILED] 堆耗尽，检查 configTOTAL_HEAP_SIZE
→ 日志停在半截，后续全部任务不再运行
```

这次触发是**事故而不是设计**：`xTaskCreate` 返回值未检查，栈上放了 `ctx`，
4 KB 堆被耗尽。它的价值在于证明**钩子和停机行为都是真的**。

### 本篇**没有**验证的东西

- **Trace 宏一个都没试**（`configUSE_TRACE_FACILITY 0`）。
- **运行时间统计完全不可用**（三个宏全 0）；"哪个任务占多少 CPU"本仓库答不了。
- **`vTaskSuspendAll` / `xTaskResumeAll` 无实验**，其"中断照常、不切任务"的差别未测。
- **ISR 版临界区无实验**；`taskENTER_CRITICAL_FROM_ISR` 的返回值语义来自源码。
- **优先级 0–4 的中断不受临界区保护**这条是源码推论（`portmacro.h:100`），未做成用例。
- **`configASSERT` 的断言路径没有主动触发过**（只在 §8 的机制描述里引用 `port.c:436-439`）。
- 教材三个示例工程（`29_fromisr_game` 等）**本机没有**，"感觉不到区别"未复现。
- 栈溢出的 **SP 越界那一半未覆盖**（会先撞坏相邻堆块，`F103_15` 明确不做）；
  MPU 隔离、`portSTACK_LIMIT_PADDING > 0` 的余量语义未测。

---

## 常见错误和陷阱

| 陷阱 | 症状 | 根因 | 处理 |
| --- | --- | --- | --- |
| ISR 里调任务版 API | 断言失败，或内核链表被破坏 | 任务版可阻塞、会调调度器 | 一律用 `...FromISR` |
| `FromISR` 传 `NULL` | 高优先级任务延迟一个 tick 才跑 | 没人告诉内核"要切换" | 传 `&woken` 并 `portYIELD_FROM_ISR(woken)` |
| 多次 `FromISR` 各用一个 woken | 部分唤醒被丢掉 | 前面的 woken 未参与 yield | **共用一个** woken 变量 |
| `portYIELD_FROM_ISR(&woken)` | 每次都切换（多余开销） | 传了指针，恒非零 | 传值 |
| 在优先级 0–4 的中断里用 `taskENTER_CRITICAL` 保护共享数据 | 保护完全无效 | BASEPRI=0x50 挡不住更高优先级（§7） | 这类中断里只用无锁原子操作，或降低其优先级到 ≥5 |
| 从 ISR 调 `taskENTER_CRITICAL` | `configASSERT` 失败 | `port.c:436-439` 检查 VECTACTIVE | 用 `..._FROM_ISR` |
| ISR 临界区用 `taskEXIT_CRITICAL` 配对 | 嵌套计数与 BASEPRI 两套机制混用，屏蔽泄漏 | 两套 API 的恢复机制不同 | 严格成对使用同一套 |
| 以为 `vTaskSuspendAll` 关中断 | 中断里仍在跑，共享变量被改 | 它只暂停**调度**（§9） | 要挡中断用临界区；要挡任务用 suspend |
| 照抄教材 18.2 的 `vTaskSuspendScheduler` | 编译失败 | 函数不存在（§9） | 用 `vTaskSuspendAll` / `xTaskResumeAll` |
| 钩子里 `return` | 继续运行一个已损坏的系统 | 检查点已在异常路径 | 打印后死循环 |
| 钩子里 `printf` 但串口靠任务驱动 | 卡死，什么也没打印 | 打印依赖的机制已被停住 | 钩子里改用阻塞式直写寄存器 |
| 以为 `configCHECK_FOR_STACK_OVERFLOW=2` 放弃了 SP 检查 | 误判检测能力 | `>1` 是"或"，两种都做（§13） | 读 `stack_macros.h:103-121` |
| 用高水位反推栈底地址做判据 | 差几个字节，漏检 | 地址算术与 paint 边界的假设不一定成立 | **用余量归零判穿**（`F103_15` 首版的教训） |
| 把 `hwm` 当字节数 | 以为还剩很多 | 返回单位是 `sizeof(StackType_t)` 个数（§14） | 除以/乘以字长再说 |
| 任务刚创建就读 `hwm` | 等于栈大小，无信息 | paint 还没被消耗 | 在最深路径跑过之后再读 |
| 指望 `vTaskList` / `vTaskGetRunTimeStats` | 链接失败/符号不存在 | 三个配置宏全 0（`:71-73`） | 先开 `configGENERATE_RUN_TIME_STATS` + 更高精度定时器 |

---

## 故障排查方法

1. **"中断里一调 API 就死。"** 三步：① 是不是调了任务版（`xQueueSend` 而不是 `xQueueSendFromISR`）；
   ② 是不是在 `taskENTER_CRITICAL` 里调了任何可能阻塞的 API；
   ③ 断言信息里的文件/行号——本仓库 `configASSERT` 带 `__FILE__`/`__LINE__`，直接定位。
2. **"高优先级任务反应慢一个 tick。"** 搜所有 `FromISR` 调用点，检查：
   ① 第三参数是否传 `NULL`；② 是否每个调用点各用一个 woken 变量；
   ③ 是否忘了 `portYIELD_FROM_ISR`（**这是最常见的漏一行**）。
3. **"加了临界区还是被改。"** 查那个中断的优先级数值：若 < 5（数值），BASEPRI=0x50 挡不住它。
   本仓库可用 `NVIC_GetPriority()` 打印实际数值，并与 `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY`
   （`FreeRTOSConfig.h:97`，值 5）对比。
4. **"vTaskSuspendAll 之后中断还是进来了。"** 这是**正确行为**（§9）。若想要中断也不进来，
   说明选错了工具——用 `taskENTER_CRITICAL`；若只想让中断进来但别切任务，说明选对了。
5. **"内存不足，日志停在半截。"** 若看到 `[MALLOC FAILED]`，说明是 `pvPortMalloc` 失败。
   排查顺序：**先数 `xTaskCreate` / `xQueueCreate` 的返回值有没有检查**（`F103_13` 首版就是这里漏的），
   再算 `configTOTAL_HEAP_SIZE` 的账（Day02 §6 的账本口径）。
6. **"栈溢出钩子从没响过，是不是没开？"** 两个前提：`configCHECK_FOR_STACK_OVERFLOW != 0`
   **且** `vApplicationStackOverflowHook` 有实现（只声明不实现会链接失败或落进默认的空实现）。
   本仓库两者都满足，`F103_15` 已实测触发。
7. **"想知道哪个任务吃 CPU。"** 本仓库**做不到**（§11 最后一行）。要做得先开三个宏
   并实现 `portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()` 与 `portGET_RUN_TIME_COUNTER_VALUE()`
   （教材 `rtos.txt:6694-6700`），代价是多一个高精度定时器。
8. **"`hwm` 读数看不懂。"** 先确认 `sizeof(StackType_t)`（本仓库 4），再确认读数时机（跑过最深路径）。

---

## 练习题

### 基础题

1. 教材给了"两套 API"的 4 条理由。指出哪条是结构性的，并解释为什么 `FromISR` 反向用在任务里却允许。
2. `xHigherPriorityTaskWoken` 的三种状态各是什么？传 `NULL` 合法但不推荐，理由是什么？
3. 在源码里找到 `portYIELD_FROM_ISR` 的定义（`portmacro.h:111`），说明它为什么不算"另一个宏"。
4. 画出 BASEPRI=0x50 时的屏蔽范围，标出"哪些中断能进来但不许调 FreeRTOS API"。
5. `taskENTER_CRITICAL` 和 `vTaskSuspendAll` 各自挡住什么、不挡什么？各举一个适用场景。

### 改错题

6. 下面这段 ISR 有四处问题：

   ```c
   void USART1_IRQHandler(void) {
       BaseType_t w1 = pdFALSE, w2 = pdFALSE;
       taskENTER_CRITICAL();
       xQueueSend( q1, &c1, portMAX_DELAY );
       xQueueSendFromISR( q2, &c2, &w2 );
       taskEXIT_CRITICAL();
       portYIELD_FROM_ISR( &w2 );
   }
   ```

7. 下面这段想"暂停调度保护共享数据"，说明它错过了什么：

   ```c
   vTaskSuspendAll();
   s_shared = new_value;     /* 期望：中断也不能改 */
   xTaskResumeAll();
   ```

8. 下面这段栈检查有至少两处问题：

   ```c
   void probe(void *p) {
       uint8_t buf[64];
       eat_stack(depth);
       printf("stack left=%u bytes\n", (unsigned)uxTaskGetStackHighWaterMark(NULL));
   }
   ```

### 综合题

9. 用 `F103_15` 的骨架，设计一个**区分**"SP 越界"与"paint 被写穿"的实验。要求：
   ① 说明两种失败分别要怎么才能触发；② 为什么本实验只做了后者；
   ③ 给出一个"预期必须失败"的负向验证（提示：删钩子行 / 注入假 FAIL / 空日志，
   本实验的判定器已经做过这三条）。

10. 教材 `rtos.txt:6657` 的方法 1 说"切换前现场即峰值"。请构造一个**方法 1 漏检、方法 2 能检**的场景，
    写出代码骨架，并说明为什么 `=2` 能覆盖它（提示：函数吃栈后返回，SP 回到原处）。

11. 若把本仓库换成 `StackType_t = uint16_t`（16 位栈配置），`F103_15` 的判据要怎么改？
    逐条列出：`hwm` 的读数含义、`depth` 与余量的换算、判定器的 grep 是否需要改。

12. 教材 19.1.3 的 trace 宏表有 13 个宏。挑其中三个，说明在**本仓库不打开**
    `configUSE_TRACE_FACILITY` 的前提下，它们是否还能用，为什么。

---

## 验收标准

- [ ] 能说出"两套 API"四条理由里哪条是结构性的，并解释 `FromISR` 反向使用的合法性。
- [ ] 能默写 `xHigherPriorityTaskWoken` 三态，说出 `NULL` 的后果与"共用 woken 变量"的理由。
- [ ] 能引用 `portmacro.h:98-111` 证明 `portYIELD_FROM_ISR` 是别名，并指出判断条件是 `!= pdFALSE`。
- [ ] 能用教材 17.4 的"感觉不到不同"举例说明"现象不可观测 ≠ 无差别"。
- [ ] 能画出 `BASEPRI = 0x50` 的屏蔽范围，并指出"数值 0–4 能进来但不许调 API"。
- [ ] 能解释 `port.c:436-439` 的 VECTACTIVE 断言及其 `uxCriticalNesting == 1` 条件。
- [ ] 能指出教材 18.2 的三处错误（函数名不存在、称"宏"、抹掉"调度 vs 中断"差别）。
- [ ] 能说明 `configCHECK_FOR_STACK_OVERFLOW = 2` 是"SP 检查 **或** paint 检查"，并引用 `stack_macros.h:103-121`。
- [ ] 能解释为什么"16 字节"这个数字来自 `pulStack[0..3]`。
- [ ] 能说出 `hwm` 的单位是 `sizeof(StackType_t)` 个数，并指出教材"N/4"的平台前提。
- [ ] 能引用 `F103_15` 的三段读数（不误报 / 标定 16 B / `depth=60` 归零）及其判据顺序。
- [ ] 能列出本仓库**不可用**的两类调试手段（Trace、运行时间统计）及原因。
- [ ] 能列出本篇**未验证**的八项。

---

## 延伸阅读

- 教材第 17 章：`rtos.txt:6118-6422`
  - 17.1 两套 API：6140–6220（4 条理由、3 个解法、"感觉不到不同"）
  - 17.2 中断延迟处理：6230–6260（t1–t4、《deferred processing task》）
  - 17.4 示例：6280–6320（`DispatchKey` 改前/改后）
- 教材第 18 章：`rtos.txt:6423-6491`
  - 18.1.1 任务中屏蔽中断：6435–6460（`configMAX_SYSCALL_INTERRUPT_PRIORITY` 方向、递归、
    以及 `taskENTER_CRITICA` 的三处拼写错）
  - 18.1.2 ISR 中屏蔽中断：6460–6465（`_FROM_ISR` 与返回值）
  - 18.2 暂停调度器：6466–6487（**函数名错 + 语义错**）
- 教材第 19 章：`rtos.txt:6492-6739`
  - 19.1.1–19.1.2 打印与断言：6497–6530（`fputc`、`configASSERT` 两种写法、队列与中断级断言）
  - 19.1.3 Trace：6531–6647（13 个 trace 宏）
  - 19.1.4–19.1.5 Hook：6648–6662（malloc / 栈溢出 / 两种方法）
  - 19.2 优化：6666–6739（高水位、运行时间统计、`uxTaskGetSystemState`/`vTaskList`/`vTaskGetRunTimeStats`）
- 源码：`Source/portable/GCC/ARM_CM3/portmacro.h` — `portEND_SWITCHING_ISR`（98）、
  `portYIELD_FROM_ISR`（111）、`portSET/CLEAR_INTERRUPT_MASK_FROM_ISR`（117-118）、
  `portDISABLE/ENABLE_INTERRUPTS`（119-120）、`ulPortRaiseBASEPRI`（228）、`vPortSetBASEPRI`（248）
- 源码：`Source/portable/GCC/ARM_CM3/port.c` — `vPortEnterCritical`（426）、`vPortExitCritical`（443）
- 源码：`Source/include/stack_macros.h` — `=1` 分支（70、87）、`>1` 分支（103、125）、
  `portSTACK_LIMIT_PADDING` 默认值（52）
- 源码：`Source/tasks.c` — `taskCHECK_FOR_STACK_OVERFLOW()` 调用点（5164、5265）、
  `prvTaskCheckFreeStackSpace`（6370）、`tskSTACK_FILL_BYTE`（121）、`memset` 填充（1847）
- 本仓库实验：`F103_11`（ISR→队列）、`F103_12`（ISR→信号量）、`F103_13`（malloc hook 首触发）、
  `F103_15`（栈溢出 + 高水位）
- 相关笔记：`Day03`（任务状态与 tick）、`Day05`（`FromISR` 无超时）、`Day07`（ISR 里为什么不能用互斥量）、
  `Day09`（`configASSERT` 与 `#error` 的对照）
- 上一篇：`Day09_软件定时器.md`（教材第 16 章）；本篇是 FreeRTOS 系列最后一篇

---

**验证记录**

| 日期 | 环境一 | 命令一 | 结果一 | 环境二 | 命令二 | 结果二 | 未覆盖 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 2026-10-30 | 教材抽取文本 | 读 `rtos.txt:6118-6739` | 第 17 章 4 条理由（6140-6160）与"感觉不到不同"（6280-6320）；第 18 章三种独占方法（6425-6431）、BASEPRI 方向（6445-6447）、`taskENTER_CRITICA` 三处拼写错（6438/6450/6452）、**`vTaskSuspendScheduler`/`xTaskResumeScheduler` 不存在（6483-6484）**、紧跟的语义复制错（6486-6487）；第 19 章四种手段、13 个 trace 宏、两种栈检查方法（6657-6662）、`hwm` 的"N/4"（6684）、运行时间统计三项配置（6694-6695） | 本仓库源码 | `grep`/`sed` 读 `portmacro.h`、`port.c`、`stack_macros.h`、`tasks.c`、`FreeRTOSConfig.h` | `portYIELD_FROM_ISR` 就是 `portEND_SWITCHING_ISR` 别名（`portmacro.h:111`），判断条件 `!= pdFALSE`（`:100`）；`portDISABLE_INTERRUPTS` = `vPortRaiseBASEPRI()`（`:119`）；`taskENTER_CRITICAL` 任务态断言 VECTACTIVE（`port.c:436-439`）；`>1` 是"SP 越界 **或** `pulStack[0..3]`≠`0xa5a5a5a5`"（`stack_macros.h:103-121`）；`hwm` 除 `sizeof(StackType_t)` 而非硬编码 4（`tasks.c:6382`）；检查点在 `vTaskSwitchContext` 切换前（`tasks.c:5164`） | Trace、`vTaskSuspendAll`、ISR 临界区均无实验；`configASSERT` 断言路径未主动触发 |
| 2026-10-30 | WSL2 + QEMU 10.2.1 | `qemu-system-arm -M stm32vldiscovery -kernel f103_15_stackoverflow.elf` + `run.sh exp15` | `configCHECK_FOR_STACK_OVERFLOW=2`；对照 `depth=12` 余量 160 字不误报；标定 `depth=16` ⇒ 16 B/帧；逐级 152→0；`depth=60` 时 `hwm=0`（栈底 16 B 的 0xA5 被写穿），钩子报 `任务 probe` 并停机；判定器通过，3 条负向验证（删钩子行 / 注入 FAIL / 空日志）按预期失败 | 本仓库配置 | `grep -n` `FreeRTOSConfig.h` | `configUSE_MALLOC_FAILED_HOOK 1`（`:69`）、`configCHECK_FOR_STACK_OVERFLOW 2`（`:70`）、`configUSE_TRACE_FACILITY 0` / `configUSE_STATS_FORMATTING_FUNCTIONS 0` / `configGENERATE_RUN_TIME_STATS 0`（`:71-73`）⇒ **Trace 与运行时间统计在本仓库不可用**；`INCLUDE_uxTaskGetStackHighWaterMark 1`（`:87`）；`configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5` ⇒ BASEPRI `0x50`（`:97`、`:100-101`） | 堆耗尽钩子只在 `F103_13` 首版事故中触发过，不是受控用例；`F103_11`/`F103_12` 未做"去掉 `portYIELD_FROM_ISR`"的对照 |
