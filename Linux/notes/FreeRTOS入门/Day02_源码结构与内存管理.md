# Day02：源码结构与内存管理（教材第 7–8 章）

**验证状态**：仓库内文件清单与配置项**源码核对**（`ls`/`grep` 读数附在验证记录里）；
堆的消耗读数**实测**（`exp10_freertos.log`：1112 B → 496 B）；
**未验证**：本仓库只有 heap_4，**没有**做 heap_1/2/3/5 的对照实验，
所以五种实现的差异全部来自教材原文与源码阅读，不是本机实测。

---

## 学习目标

学完本篇应能用代码证据回答：

1. FreeRTOS 最核心的两个 `.c` 文件是哪两个？为什么 `queue.c` 是"基本必需"而 `timer.c` 只是"可选"？
2. 移植到新芯片，需要动几个文件？它们分别解决什么问题？
3. `#include "FreeRTOS.h"` 为什么必须排在其他 FreeRTOS 头文件之前？
4. libc 的 `malloc/free` 有 7 条问题，其中哪一条会让"偶发死机"最难复现？
5. `heap_2` 与 `heap_4` 的分界线是什么？举一个只有 `heap_4` 能救回来的序列。
6. `configTOTAL_HEAP_SIZE` 在哪一种 heap 实现下失效？为什么？
7. `pdPASS` 与 `pdFAIL` 的值分别是多少？为什么 `xTaskCreate` 失败不能用 `== pdFAIL` 判断？

---

## 环境与前置条件

| 项 | 版本 / 说明 |
| --- | --- |
| 教材 | 第 7 章源码概述（`rtos.txt:2068-2282`）、第 8 章内存管理（`rtos.txt:2283-2439`） |
| 内核 | FreeRTOS **V11.3.0** |
| 仓库位置 | `experiments/F103_common/vendor/freertos/` |
| 堆实现 | **`heap_4.c`**（`Source/portable/MemMang/heap_4.c`，本仓库只有这一种） |
| 端口 | `Source/portable/GCC/ARM_CM3/{port.c, portmacro.h}` |
| 堆大小 | `configTOTAL_HEAP_SIZE = 4 × 1024 = 4096 B` |
| 关键配置 | `configSUPPORT_DYNAMIC_ALLOCATION=1`、`configSUPPORT_STATIC_ALLOCATION=0`、`configAPPLICATION_ALLOCATED_HEAP=0`、`configUSE_MALLOC_FAILED_HOOK=1` |

---

## 核心概念

### 1. 只有两个文件是"必需"，其余按需裁剪

教材给了一张分级表（`rtos.txt:2079-2106`）：

| 文件 | 教材定级 | 它提供什么 | 不用它的代价 |
| --- | --- | --- | --- |
| `tasks.c` | **必需** | 任务、调度、就绪表、上下文切换 | 无法调度 |
| `list.c` | **必须** | 双向链表（就绪/阻塞/挂起/终止四个列表的基础） | `tasks.c` 编译不过 |
| `queue.c` | **基本必需** | 队列 + **全部信号量/互斥量** | 没有队列、没有信号量、没有互斥量 |
| `timers.c` | 可选 | 软件定时器 | 失去 `xTimerCreate` 系列 |
| `event_groups.c` | 可选 | 事件组 | 失去 `xEventGroup*` |
| `croutine.c` | 可选（**过时**） | 协程 | 不必考虑，教材明确说过时 |

**"信号量在 `queue.c` 里"是这张表最反直觉的一条。**
新手常以为信号量是独立实现，实际上二值信号量就是一个"深度为 1、数据长度为 0"的队列。
本仓库正是这个构成（`Source/` 下只有 5 个 `.c`）：

```text
Source/{tasks,list,queue,event_groups,timers}.c   ← 没有 croutine.c
```

这条事实有直接后果：**想省 Flash 而裁掉 `queue.c`，等于同时裁掉信号量与互斥量**，
`xSemaphoreCreateMutex()` 立刻变成未定义符号。

### 2. 移植的全部内容，就是 `portable/[编译器]/[架构]/` 下的两个文件

教材用一个路径就讲清了移植的边界（`rtos.txt:2110-2111`）：
`portable/[compiler]/[architecture]/`，例如 `RVDS/ARM_CM3`。

本仓库对应 `Source/portable/GCC/ARM_CM3/`，两个文件：

| 文件 | 职责 |
| --- | --- |
| `port.c` | `SVC`/`PendSV`/`SysTick` 三个异常处理、`vPortStartFirstTask`、`pxPortInitialiseStack`（伪造初始栈帧） |
| `portmacro.h` | 临界区宏（`portENTER_CRITICAL`）、`portYIELD`、`StackType_t`/`TickType_t` 等类型 |

"移植只要写两个文件"这句话听起来轻，但**难的是 `pxPortInitialiseStack`**：
它要在一个还没跑过的任务栈上手工摆出一个"像被中断打断过"的栈帧，
使得第一次 `PendSV` 返回时硬件自动恢复 R4–R11、R0–R3、R12、LR、PC、xPSR。
摆错一个字段，症状是任务第一次运行时 `PC` 飞到野地址——这类问题在本机没有板时**无法验证**，
属于本仓库整个 FreeRTOS 系列的共同盲区。

### 3. 头文件包含顺序不是风格问题，是编译期约束

教材给了 3 个头文件目录（`rtos.txt:2119-2121`），并强调：

> 使用 FreeRTOS API 函数时，必须包含此文件（`FreeRTOS.h`）。在 `FreeRTOS.h` 之后，
> 再去包含其他头文件，比如：`task.h`、`queue.h`、`semphr.h`、`event_group.h`（`rtos.txt:2133`）

原因在 `FreeRTOS.h` 里：它读 `FreeRTOSConfig.h`，把 `configUSE_*` 算成一批派生宏
（如 `configMAX_PRIORITIES`、`INCLUDE_vTaskDelay`），后面所有头文件都用这些宏做条件编译。
**先包含 `task.h` 再包含 `FreeRTOS.h`，会得到一堆"宏未定义"或"结构体字段不存在"的错**，
而且错误位置出现在 `task.h` 里，很容易往错的方向查。

本仓库的目录布局与教材（CubeMX 生成）**不同**，这一点必须按仓库实际情况记：

| 教材（CubeMX） | 本仓库 |
| --- | --- |
| `Middlewares/Third_Party/FreeRTOS/Source/include` | `vendor/freertos/Source/include` |
| `Middlewares/.../portable/[compiler]/[arch]` | `vendor/freertos/Source/portable/GCC/ARM_CM3` |
| `Core/Inc/FreeRTOSConfig.h` | `vendor/freertos/include/FreeRTOSConfig.h` |

配置文件只放在 `include/` 下（同级没有内核头文件），这是因为本仓库把内核当"第三方库"
原样 vendored 进来，配置单独放一层——`F103_common/cmake/f103.cmake` 负责把两个 include 都加进搜索路径。

### 4. 不用 libc `malloc` 的七条理由，最要命的是"时间不确定"

教材列了 7 条（`rtos.txt:2287`）：不适合资源紧缺系统、实现复杂占代码空间、
**非线程安全**、**运行时间不确定**、内存碎片化、换编译器要复杂配置、难调试。

其中真正决定"嵌入式不能用 libc malloc"的是第 4 条。前几条都可以商量——
代码空间大一点、碎片多一点，多数项目能忍。但**运行时间不确定**在实时系统里是硬伤：

> 一次 `malloc` 可能花 2 µs，也可能花 200 µs（要遍历空闲链表、可能要 `brk`/`sbrk`）。
> 若它发生在控制环里，最坏情况执行时间（WCET）就无从计算。

这也是为什么 FreeRTOS 的 `pvPortMalloc` 虽然是"自己的 malloc"，教材仍然强调
**heap_4 的"执行时间是不确定的"**（`rtos.txt:2374`）——它只是把不确定性从"操作系统级"
缩小到"遍历一个 4 KB 数组"。要确定性就得上 heap_1（只分配不回收，时间确定）。

顺带纠正一个常被混淆的说法（教材专门提醒，`rtos.txt:2288-2289`）：
**堆和栈不是一回事**。堆是一块空闲内存 + 管理函数；栈是函数调用时保存局部变量与环境的地方，
它可以**从堆里分配**——FreeRTOS 就是这么做的（每个任务的栈由 `pvPortMalloc` 出来）。

### 5. 五种 heap 的分水岭只有两问

教材给的表格（`rtos.txt:2302-2334`）可以压成两问，回答完就选出来了：

| 问 | 是 | 否 |
| --- | --- | --- |
| **要用 `vPortFree` 吗？** | heap_2/3/4/5 | **heap_1**（只分配不回收，时间确定、无碎片） |
| **相邻空闲块要合并吗？** | **heap_4/5** | heap_2（最佳匹配但不合并，有碎片；仅"分配尺寸恒定"场景无碍） |

另外两条分支：

- **heap_3**：直接包 libc `malloc/free`，靠"先挂起调度器"实现线程安全。
  代价是 `configTOTAL_HEAP_SIZE` **失效**（堆由链接器决定），且 `xPortGetFreeHeapSize` 不可用
  （`rtos.txt:2360-2361`、`2414`）。
- **heap_5**：算法与 heap_4 **相同**，区别只在能管理**多块不连续**的内存，
  且必须先调 `vPortDefineHeapRegions()` 才能用 `pvPortMalloc`（`rtos.txt:2377-2378`）。

本仓库用 **heap_4**，这也是教材与新工程共同推荐的选择（heap_2"只是为了兼容以前的代码"，`rtos.txt:2349`）。

### 6. heap_4 的"首次适应 + 合并"，用本仓库的数字走一遍

教材的算法描述（`rtos.txt:2366-2367`）：

> pvPortMalloc 想申请 20 字节 → 找出**第 1 个**能满足的块（200 字节）→ 切成 20 + 180

**"首次适应"（first fit）与"最佳匹配"（best fit）的差别是搜到第一个够用的就停**，
不遍历全部。所以 heap_4 的耗时取决于"空闲块排列顺序"，这就是它"时间不定"的来源。

真正解决问题的是**合并**（coalesce）。教材举了 6 步 A–F（`rtos.txt:2369`），关键在最后两步：

```text
E: 释放 Queue   → Queue 前后各有一块空闲
F: 释放 User    → User 前后的空闲 + User 自身 → 合并成一大块
```

**若没有 F 这一步的合并，同样释放三次之后，堆会变成"很多小块、单块都不够用"**——
这才是"碎片"的可操作定义：不是总空闲量不够，而是**最大连续空闲块**不够。
heap_2 缺的正是这一步。

用本仓库的实测数字体会一下"最大连续块"为什么重要：

| 时点 | `xPortGetFreeHeapSize()` | 说明 |
| --- | --- | --- |
| 建完队列 + 互斥量 + 三任务 | **1112 B** | 从 4096 B 里扣掉 |
| 启动调度器后 | **496 B** | 空闲任务（128 字栈 + TCB）又吃掉 616 B |

此时 496 B 若被切成 8 个 62 B 的碎片，**下一次 `xTaskCreate`（至少要 128 字栈 = 512 B + TCB）必然失败**，
尽管"总空闲 496 B"听起来还能塞点东西。所以判"还有没有内存"不能看总空闲，
要看 **最大块**——而 FreeRTOS 没有提供"最大空闲块"这个 API，
只能靠 `xPortGetMinimumEverFreeHeapSize()` 看历史最低水位间接判断。

### 7. 两个堆读数 API，各自有"不可用"的场合

教材把这两条限制写得很清楚，但很容易漏读（`rtos.txt:2413-2418`）：

| API | 返回 | 限制 |
| --- | --- | --- |
| `xPortGetFreeHeapSize()` | 当前空闲字节数 | **heap_3 中无法使用**（堆不归 FreeRTOS 管） |
| `xPortGetMinimumEverFreeHeapSize()` | 运行至今空闲量的**历史最小值** | **只有 heap_4 / heap_5 支持** |

第二条的用途常被低估：它是"这个堆到底够不够"的**唯一事后证据**。
本仓库 `F103_10` 每轮打印的 `heap_free=496 B` 全程不变，说明稳态无泄漏；
但"稳态不变"不等于"峰值够用"——**峰值要看历史最小值**。本仓库没有打印它，
这是可以补的一条读数（教材给的建议正好相反：用 `xPortGetFreeHeapSize` 反推
`configTOTAL_HEAP_SIZE` 能否调小，`rtos.txt:2413`）。

### 8. 命名规范是一张索引表，不是审美

教材把前缀拆成"返回值类型 + 定义文件"两部分（`rtos.txt:2234-2250`）：

| 前缀 | 含义 | 例 |
| --- | --- | --- |
| `v` | `void` 返回值 | `vTaskPrioritySet`（`tasks.c`） |
| `x` | `BaseType_t` 返回值 | `xQueueReceive`（`queue.c`） |
| `pv` | 返回 `void *` | `pvTimerGetTimerID`（`timers.c`） |
| `uc`/`pc` | `uint8_t` / `char *` | `pcTaskGetName` |

**读函数名就能知道它会不会失败、定义在哪**。这条在排障时很实用：
`xSemaphoreTake` 以 `x` 开头 ⇒ 有返回值 ⇒ 必须检查；
`vTaskDelay` 以 `v` 开头 ⇒ 无返回值 ⇒ 没有失败路径可查。

宏的前缀同理（`rtos.txt:2255-2277`）：`port` → `portable.h`/`portmacro.h`；
`task` → `task.h`；`pd` → `projdefs.h`；`config` → `FreeRTOSConfig.h`；`err` → `projdefs.h`。

而四个"继承认知"的常量值是（`rtos.txt:2273-2278`）：

```text
pdTRUE = 1    pdFALSE = 0
pdPASS = 1    pdFAIL  = 0
```

**`pdFAIL == 0` 这一点，直接解释了 Day03 的那个坑**：
`xTaskCreate` 失败返回 `errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY`（-1），
用 `== pdFAIL`（== 0）判断恒为假，任务创建失败被静默吞掉。
`F103_16` 的首版就是这么挂的——4 KB 堆被"四任务 + 三个完成信号量 + 栈上 ctx"耗尽，
`xTaskCreate` 返回值从未检查，栈溢出钩子停机，日志停在半截。

### 9. 两个数据类型决定"时间"与"返回值"的宽度

教材在 `portmacro.h` 一节讲了这个（`rtos.txt:2194-2197`）：

| 类型 | 由什么决定 | 本仓库 |
| --- | --- | --- |
| `TickType_t` | `configUSE_16_BIT_TICKS` 定义则 `uint16_t`，否则 `uint32_t` | **未定义 ⇒ `uint32_t`** |
| `BaseType_t` | 架构最高效宽度（32 位机 = `uint32_t`） | `uint32_t` |

`TickType_t` 是 **32 位**这件事有一个隐蔽后果：`tick` 计数约 49.7 天回绕一次
（2³² ms @ 1000 Hz）。所有"比较两个 tick 时间戳"的代码必须用
`(TickType_t)(now - then)` 做**无符号差**，而不是 `now > then`。
本仓库在 `aviation-node` 项目里踩过同构的坑：跨任务 tick 差用无符号减法会下溢
（`[stat] age=4294967292`），为此专门加了 `av_tick_elapsed()`——
**同一条算术错误在裸机项目上出现过一次，就没必要在 FreeRTOS 上再犯一次。**

---

## 系统调用/接口卡片

### `void * pvPortMalloc( size_t xWantedSize )` / `void vPortFree( void *pv )`

作用：FreeRTOS 的堆分配/释放，对应 libc 的 `malloc`/`free`。

参数：`xWantedSize` 字节数；`vPortFree` 传 `NULL` 时**不做任何事**（这是允许的，不报错）。

返回值和错误：成功返回**对齐后可用**的指针；失败返回 `NULL`，
且若 `configUSE_MALLOC_FAILED_HOOK == 1` 会调用 `vApplicationMallocFailedHook()`。

资源所有权：`pvPortMalloc` 分配、`vPortFree` 释放。**必须成对**，
且释放后指针不得再使用（heap_4 会把该块并入空闲链表，可能立刻被别的分配拿走）。

线程/进程安全：**是**（`tasks.c`/`heap_4.c` 内部用临界区或调度器挂起保护）。
但这不代表"可以随便在 ISR 里调"——ISR 里不能分配。

最小示例：

```c
uint8_t *p = pvPortMalloc( 64 );
if ( p == NULL ) { /* 必须处理：内存不足 */ }
vPortFree( p );
p = NULL;   /* 防重复释放 */
```

常见误用：① 不检查 `NULL`；② `pvPortFree(NULL)` 后以为"释放了两次会报错"（它静默返回）；
③ 把 `pvPortMalloc` 的返回当 libc 指针传给 `free()`（两个堆，混用必崩）。

### `size_t xPortGetFreeHeapSize( void )`

作用：当前剩余堆字节数。

参数：无。

返回值和错误：字节数；**heap_3 下不可用**（会得到无意义值或链接失败）。

资源所有权：无。

线程/进程安全：只读；但值随时会变，多任务下不可作为"绝对可用量"依据。

最小示例（`main.c:117-119`）：

```c
LOG( "[res] heap_free=%u B, ...\n", (unsigned)xPortGetFreeHeapSize(), ... );
```

常见误用：用一次读数判断"还剩多少"，忽略它看不到**最大连续块**（见 §6）。

### `size_t xPortGetMinimumEverFreeHeapSize( void )`

作用：运行至今空闲量的**历史最小值**——也就是"峰值用掉了多少"。

返回值和错误：字节数；**只有 heap_4 / heap_5 支持**。

最小示例：

```c
printf( "heap min=%u B, now=%u B\r\n",
        (unsigned)xPortGetMinimumEverFreeHeapSize(),
        (unsigned)xPortGetFreeHeapSize() );
```

常见误用：在 heap_2/heap_3 上调用（编译期就会报未定义，或运行期返回 0）。

### `void vApplicationMallocFailedHook( void )`

作用：`pvPortMalloc` 失败时的钩子。

前提（教材 `rtos.txt:2435`）：① `configUSE_MALLOC_FAILED_HOOK` 定义为 1；
② 提供该函数实现。本仓库条件①**已满足**（`FreeRTOSConfig.h:69`）。

返回值和错误：`void`；**钩子返回后** `pvPortMalloc` 仍然返回 `NULL`，所以调用方仍要检查。

最小示例：

```c
void vApplicationMallocFailedHook( void )
{
    printf( "[MALLOC FAILED] free=%u B min=%u B\r\n",
            (unsigned)xPortGetFreeHeapSize(),
            (unsigned)xPortGetMinimumEverFreeHeapSize() );
    for (;;) { __asm volatile("nop"); }   /* 停机，保住现场 */
}
```

常见误用：钩子里 `printf` 之后继续往下跑——**钩子停不住系统**，调用方若不检查 `NULL`
就是空指针解引用。`F103_16` 的首版正是钩子停机、日志停在半截，反而掩盖了真因。

---

## 最小可运行实验

本篇没有新写实验，证据来自两个已有的：

| 实验 | 用到的读数 | 它证明了什么 |
| --- | --- | --- |
| `F103_10_FreeRTOS_Tasks` | `heap_free` 1112 → 496 B，全程不变 | 动态分配的**账目可算**；稳态无泄漏 |
| `F103_16_FreeRTOS_PriorityInversion` | 4 KB 堆被耗尽、钩子停机、日志半截 | 堆不足的**失败形态**，以及"不检查返回值"如何掩盖它 |

这两个实验合起来给出本篇的核心判据：
**堆相关的结论必须有两类读数——"用掉多少"（可算）与"峰值用掉多少"（`xPortGetMinimumEverFreeHeapSize`，本仓库未采集）。**

---

## 编译与运行

```bash
# 观察堆账目
cd experiments/F103_10_FreeRTOS_Tasks
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake -DF103_QEMU=ON
cmake --build build
timeout 25 qemu-system-arm -M stm32vldiscovery -display none -monitor none \
  -serial stdio -kernel build/f103_10_freertos_tasks.elf < /dev/null 2>&1 | tr -d '\r' \
  | grep -E "初始空闲堆|heap_free" | head -3
```

若要把堆调到"刚好不够"，改 `vendor/freertos/include/FreeRTOSConfig.h:34`
的 `configTOTAL_HEAP_SIZE`（例如 `3U * 1024U`），重新构建后应看到
`创建队列/互斥量失败：heap 不足`（`main.c:167`）或
"调度器未能启动"（`main.c:186`）——**这是能主动复现的失败路径**。

---

## 结果观察

### 堆账目（实测）

```text
任务已创建，启动调度器（初始空闲堆 1112 B）
[res] heap_free=496 B, stack_hwm consume=154 produce=126 monitor=90 words
```

反推第一段消耗：

```text
configTOTAL_HEAP_SIZE 4096 B
 − 三任务栈 256+160+192 = 608 字 × 4 = 2432 B
 − 三 TCB（每约 92 B）          ≈ 276 B
 − 队列 depth=8 × sample_t + 队列结构 ≈ 200 B
 − 互斥量结构                   ≈ 100 B
 ────────────────────────────────
 ≈ 1088 B   （实测 1112 B，差 24 B 为对齐与结构体实际尺寸）
```

第二段：1112 − 496 = **616 B**，与"空闲任务 128 字栈（512 B）+ TCB（约 92 B）"吻合
到 12 B 以内。**这一条是推算式核对，不是单独测量**——`xPortGetMinimumEverFreeHeapSize()`
本仓库没有打印，所以"峰值消耗"缺一个独立读数。

### 一种失败形态（`F103_16`）

```text
现象：日志在某一行突然中断，没有收尾；栈溢出钩子触发停机
根因：4 KB 堆被「四任务 + 三个完成信号量 + 栈上的 ctx」耗尽
掩盖因素：xTaskCreate 的返回值从未检查，失败被静默忽略
```

这段记录的价值在于它同时展示了三件事：**堆不足是渐进的**（不是一上来就崩）、
**失败钩子只报不救**、**不检查返回值会让根因离现象很远**。

---

## 常见错误和陷阱

| 陷阱 | 症状 | 根因 | 处理 |
| --- | --- | --- | --- |
| 用 `== pdFAIL` 判 `xTaskCreate` 失败 | 堆不足时任务静默不创建 | 失败返回 -1；`pdFAIL == 0`（`rtos.txt:2273-2278`） | 判 `!= pdPASS` |
| 先包含 `task.h` 再包含 `FreeRTOS.h` | `task.h` 里大量"宏未定义" | `FreeRTOS.h` 负责计算配置派生宏 | 顺序固定：`FreeRTOS.h` 在最前 |
| 以为信号量是独立模块 | 裁掉 `queue.c` 后信号量全丢 | 信号量 = 深度/数据长度特化的队列，实现在 `queue.c` | 裁 `queue.c` 前先确认没用到任何 `xSemaphore*` |
| 用总空闲量判断"还能建任务" | 总空闲够、单块不够，创建仍失败 | 堆有碎片；heap_2 还不合并 | 看 `xPortGetMinimumEverFreeHeapSize`；换 heap_4/5 |
| 在 heap_3 上设 `configTOTAL_HEAP_SIZE` | 改了没效果 | heap_3 用 libc 堆，由链接器决定（`rtos.txt:2360`） | 改链接脚本；或换 heap_4 |
| 在 heap_3 上调 `xPortGetFreeHeapSize` | 值无意义/链接失败 | 该 API 在 heap_3 不可用（`rtos.txt:2414`） | 用 libc 侧手段或换 heap_4 |
| 在 heap_5 上直接 `pvPortMalloc` | 立即返回 `NULL` | 必须先 `vPortDefineHeapRegions()`（`rtos.txt:2378`） | 启动时先定义区域，且数组末项 `{NULL,0}` |
| 混用 `pvPortMalloc` 与 `free` | 随机崩溃/堆结构损坏 | 两个堆，地址空间不重叠 | 配对的 `free` 也必须换 `vPortFree` |
| 用 16 位 tick 却跑长期任务 | 约 65 s 后时间判断全错 | `configUSE_16_BIT_TICKS` 使 `TickType_t=uint16_t` | 32 位架构保持 32 位 tick（本仓库未定义该项） |
| `now > then` 比较 tick | 回绕时判断反了 | 32 位 tick 约 49.7 天回绕 | 一律 `(TickType_t)(now - then)` |
| 钩子里 `printf` 后继续执行 | 空指针解引用 | 钩子只通知，不中断 `pvPortMalloc` | 钩子里停机，或调用方必须检查 `NULL` |
| 堆泄漏靠"看起来还行"判断 | 长时间运行后突然分配失败 | 缺少峰值与趋势读数 | 周期打印 `xPortGetFreeHeapSize`，长跑看趋势 |

---

## 故障排查方法

1. **构建报"未定义符号 `xQueueGenericCreate`"。** 说明 `queue.c` 没参与编译。
   同理会连带丢信号量/互斥量——先看 `Source/*.c` 清单。
2. **构建报 `configMAX_PRIORITIES` 等宏未定义。** 头文件顺序错，或没把
   `vendor/freertos/include`（配置文件所在目录）加进搜索路径。
3. **任务创建返回非 `pdPASS`。** 先打印 `xPortGetFreeHeapSize()`；
   若显示还有余量，怀疑碎片（换 heap_4/5、或固定各对象尺寸）。
4. **运行一段时间后 `pvPortMalloc` 返回 `NULL`。** 三步定位：
   ① 打印 `xPortGetMinimumEverFreeHeapSize()`，看峰值是否贴住 `configTOTAL_HEAP_SIZE`；
   ② 检查是否有 `pvPortMalloc` 没有配对 `vPortFree`（任务/队列/信号量删除后是否真被回收）；
   ③ 若用了 `vTaskDelete`，确认空闲任务能跑到（回收在空闲任务里做，Day03 §6）。
5. **换编译器后堆行为变了。** 检查是否误用了 heap_3（它依赖 libc）。
6. **崩溃现场看不清。** 在 `vApplicationMallocFailedHook` 与
   `vApplicationStackOverflowHook` 里都打印"当前/最小堆 + 出错任务名 + tick"，然后停机。

---

## 练习题

### 基础题

1. 说出"必需 / 基本必需 / 可选"三档各有哪些文件，并说明 `queue.c` 为什么是"基本必需"。
2. `pdPASS`、`pdFAIL`、`pdTRUE`、`pdFALSE` 的值分别是？它们定义在哪个头文件？
3. `TickType_t` 在本仓库是多少位？由哪个宏决定？若改成 16 位，多长时间回绕一次？

### 改错题

4. 下面这段在启动时创建任务，找出两处问题：

   ```c
   #include "task.h"
   #include "FreeRTOS.h"

   void app_start(void) {
       if (xTaskCreate( worker, "w", 128, NULL, 2, NULL ) == pdFAIL) {
           LCD_PrintString(0, 0, "task failed");
       }
   }
   ```

5. 下面这段在 heap_5 上工作，指出为什么第一次分配就失败：

   ```c
   void app_init(void) {
       void *p = pvPortMalloc( 32 );
       HeapRegion_t r[] = { { (uint8_t *)0x20000000UL, 0x2000 }, { NULL, 0 } };
       vPortDefineHeapRegions( r );
   }
   ```

6. 某任务周期性 `pvPortMalloc(48)`/`vPortFree`，运行几小时后 `pvPortMalloc` 开始返回 `NULL`，
   但 `xPortGetFreeHeapSize()` 仍显示 600 多字节。解释这个组合为什么可能同时成立，
   并给出至少两种定位手段。

### 综合题

7. 把 `F103_10` 的 `configTOTAL_HEAP_SIZE` 依次改成 4096 / 3072 / 2048，要求：
   ① 预测每次构建后 `heap_free` 与"能否创建成功"；② 实际跑一遍填表；
   ③ 找出"刚好失败"的那个阈值，并与 §6 的堆账目对照。

8. 给本仓库补一条 `xPortGetMinimumEverFreeHeapSize()` 读数并接入 `[res]` 日志行。
   要求说明：为什么它比"每次打印当前空闲量"更能回答"这个堆够不够"。
   若把任务全部删掉再重建，这个最小值会回升吗？为什么？

9. 假设你的产品要求"任何一次 `pvPortMalloc` 的最坏耗时 ≤ 20 µs（72 MHz）"。
   请设计一个测量方案（本机无板，说明你会在真板上怎么做、用什么证据），
   并说明 heap_4 与 heap_1 在这个指标上为什么结论完全不同。

---

## 验收标准

- [ ] 能背出 `Source/` 下 5 个 `.c` 各自的职责，并解释为什么信号量在 `queue.c`。
- [ ] 能说清移植只改 `port.c` + `portmacro.h`，以及 `pxPortInitialiseStack` 为什么难。
- [ ] 能解释 `FreeRTOS.h` 必须最先包含的机制原因，而不是"规定如此"。
- [ ] 能给出 libc `malloc` 不适用于实时系统的**一条**硬理由并解释。
- [ ] 能画出 heap_4 的"分配—释放—合并"三步，并说明 heap_2 缺哪一步导致碎片。
- [ ] 能说出 `xPortGetFreeHeapSize` 与 `xPortGetMinimumEverFreeHeapSize` 各自在哪种 heap 上不可用。
- [ ] 能解释 `pdFAIL == 0` 与 `xTaskCreate` 失败的返回值 -1 为什么会造成静默失败。
- [ ] 能算出本仓库 `F103_10` 的 1112 → 496 B 两笔账，并标明哪笔是推算。

---

## 延伸阅读

- 教材第 7 章：`rtos.txt:2068-2282`；第 8 章：`rtos.txt:2283-2439`
- 源码：`Source/list.c`（`vListInitialise`/`vListInsertEnd`/`vListInsert`/`uxListRemove`）
- 源码：`Source/portable/MemMang/heap_4.c`（`prvInsertBlockIntoFreeList` 就是"合并"）
- 配置：`vendor/freertos/include/FreeRTOSConfig.h:32-37, 69`
- 相关实验：`F103_10`（堆账目）、`F103_16`（堆耗尽的失败形态）
- 上/下一篇：`Day01_裸机程序设计模式与多任务.md`、`Day03_任务管理.md`

---

**验证记录**

| 日期 | 环境一 | 命令一 | 结果一 | 环境二 | 命令二 | 结果二 | 未覆盖 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 2026-10-30 | 教材抽取文本 | 读 `rtos.txt:2068-2439` | 第 7 章 7 节 + 第 8 章 4 节；`pd*pd*` 常量值（2273-2278）、heap 五种对照（2302-2334）、heap_3 的 `configTOTAL_HEAP_SIZE` 失效（2360）、两个堆 API 的限制（2413-2418） | 本仓库 | `ls Source/*.c`、`find -name heap_*.c` | `tasks/list/queue/event_groups/timers.c` 五个，**无 `croutine.c`**；堆只有 `heap_4.c`；端口 `portable/GCC/ARM_CM3/{port.c,portmacro.h}`；配置在 `vendor/freertos/include/FreeRTOSConfig.h` | heap_1/2/3/5 均未在本仓库做过对照实验 |
| 2026-10-30 | 本仓库源码 | `grep` `FreeRTOSConfig.h` | `TOTAL_HEAP_SIZE=4096`、`SUPPORT_DYNAMIC_ALLOCATION=1`、`SUPPORT_STATIC_ALLOCATION=0`、`APPLICATION_ALLOCATED_HEAP=0`、`USE_MALLOC_FAILED_HOOK=1`、`16_BIT_TICKS` 未定义（⇒ `TickType_t=uint32_t`） | WSL2 + QEMU | `qemu-system-arm -M stm32vldiscovery -kernel f103_10_freertos_tasks.elf` | `heap_free` 1112 → 496 B，全程不变；两笔账反推与实测差 24 B / 12 B 以内 | 峰值消耗（`xPortGetMinimumEverFreeHeapSize`）未采集；`pxPortInitialiseStack` 无板无法验证 |
