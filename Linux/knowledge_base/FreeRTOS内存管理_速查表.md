# FreeRTOS 内存管理：速查表

> **一句话定义**：FreeRTOS 把一块**固定的静态数组**当作堆（`ucHeap[]`），用 `pvPortMalloc`/`vPortFree`
> 在上面做定长块分配；本仓库用 **heap_4**（首次适配 + 相邻空闲块合并），堆总大小 **4 KB**。
>
> **本仓库配置**：`configTOTAL_HEAP_SIZE 4 KB`、`configSUPPORT_DYNAMIC_ALLOCATION 1`、
> **`configSUPPORT_STATIC_ALLOCATION 0`（⇒ 本仓库无法静态创建任务/队列）**、
> `configAPPLICATION_ALLOCATED_HEAP 0`（⇒ 堆就是内核里的静态数组，不在 `.data` 手工指定）、
> `configUSE_MALLOC_FAILED_HOOK 1`、`configSTACK_DEPTH_TYPE uint16_t`、`configMINIMAL_STACK_SIZE 128`。
> 实现文件：`f103.cmake` 固定链接 `portable/MemMang/heap_4.c`。

---

## 1. 使用场景

| 场景 | 建议 | 理由 |
| --- | --- | --- |
| 任务栈、TCB、队列、信号量 | ✅ 动态创建 | 本仓库唯一可行路径（静态分配被关） |
| 大块缓冲（采样、协议帧） | ❌ 不要从堆拿 | 4 KB 堆，一次 1 KB 就废掉一个任务 |
| 运行时不定长数据 | ⚠️ 用 `pvPortMalloc` | **不要用 libc 的 `malloc`**，见 §5 |
| 启动时一次分配、永不释放 | ✅ 静态全局数组 | 不从堆拿，还省掉失败分支 |

**判据**：本仓库只有 4 KB 堆。**先算账再创建任务**——任务数一多，`xTaskCreate` 直接返回 `pdFAIL`。

---

## 2. 五种 heap 实现的分界

| 实现 | 能释放 | 合并相邻空闲块 | 确定性 | 典型用途 |
| --- | --- | --- | --- | --- |
| heap_1 | ❌ | — | 最高 | 只创建不删除 |
| heap_2 | ✅ | ❌ | 中 | 旧代码，**有碎片问题** |
| heap_3 | ✅ | — | 低（依赖 libc） | 需要标准库 |
| **heap_4** | ✅ | ✅ | 中 | **本仓库**；通用首选 |
| heap_5 | ✅ | ✅ | 中 | 多个不连续内存区 |

**heap_4 的两条机制**：

1. **首次适配**：空闲块按**地址升序**串成单向链表，分配时取第一个够大的块。不找最优块 ⇒ 快，但会留下碎片。
2. **释放时合并**：把释放块插回链表时，若前后相邻块也空闲就合并成一个。**这是 heap_4 相对 heap_2 唯一的实质改进**。

**它解决不了什么**：合并只能处理**相邻**的空闲块。总量够、但被一个活动块劈成两半时，大分配仍会失败——
这就是"空闲 800 B 却分配不出 700 B"的成因。

---

## 3. 接口卡片

### `void *pvPortMalloc( size_t xWantedSize )`

作用：从 FreeRTOS 堆分配。
参数：`xWantedSize` 会被**向上取整到 8 字节对齐**（实际占用还要加块头 `BlockLink_t`）。
返回值和错误：`NULL` = 堆不足。**本仓库 `configUSE_MALLOC_FAILED_HOOK 1`，此时会先调 `vApplicationMallocFailedHook()`**。
资源所有权：调用方负责 `vPortFree`。
常见误用：把它当 `malloc` 随便用；返回值不判。

### `void vPortFree( void *pv )`

作用：归还并触发相邻块合并。
返回值和错误：无返回值；传 `NULL` 是安全的空操作。
常见误用：**跨堆释放**——`pvPortMalloc` 的指针交给 `free()`，或反过来。两套堆互不认识，必然崩。

### `size_t xPortGetFreeHeapSize( void )`

作用：当前剩余字节数。
常见误用：用它判断"会不会溢出"——**它只是当前值**，跑过峰值后回到高位不代表安全。

### `size_t xPortGetMinimumEverFreeHeapSize( void )`

作用：**历史最低**余量。这才是判断"堆够不够"的依据。
常见误用：与上一个混用。

### `void vApplicationMallocFailedHook( void )`

作用：`pvPortMalloc` 失败时由内核调用（`configUSE_MALLOC_FAILED_HOOK 1` 才要求实现，否则**链接失败**）。
最小实现：关中断 + 打印 + 停机。返回是**无意义**的——分配已经失败了。

### `BaseType_t xPortGetHeapStats( HeapStats_t *pxHeapStats )`（本仓库未启用）

作用：块级统计（最大空闲块、块数、失败次数）。V11 内核提供，但需自行确认是否编入。

---

## 4. 最小示例：先算账再创建

```c
/* 4 KB 堆的账要自己算：栈深是「字」，1 字 = 4 字节 */
#define STK_RX   192   /* 768 B */
#define STK_CTRL 160   /* 640 B */
#define STK_IO   128   /* 512 B */

/* 入口先记一笔，出问题时才知道基线在哪 */
size_t heap0 = xPortGetFreeHeapSize();

if (xTaskCreate(rx_task,  "rx",   STK_RX,   NULL, 3, &s_rx)   != pdPASS ||
    xTaskCreate(ctrl_task,"ctrl", STK_CTRL, NULL, 2, &s_ctrl) != pdPASS ||
    xTaskCreate(io_task,  "io",   STK_IO,   NULL, 1, &s_io)   != pdPASS) {
    /* 不要只打印——任务根本没建起来，后续把 NULL 句柄传下去会静默失败 */
    for (;;) { }
}

/* 跑完关键路径后再看历史最低，不是现在看 */
printf("heap free=%u min=%u\n",
       (unsigned)xPortGetFreeHeapSize(),
       (unsigned)xPortGetMinimumEverFreeHeapSize());
```

---

## 5. 常见错误与陷阱

| 陷阱 | 症状 | 根因 | 处理 |
| --- | --- | --- | --- |
| 4 KB 堆塞太多任务 | `xTaskCreate` 返回 `pdFAIL`，句柄为 `NULL` | 栈 + TCB + 队列全从同一块出 | 算账；失败分支必须停机 |
| 不检查 `xTaskCreate` 返回值 | 日志停在半截、钩子报错 | 后续对 `NULL` 句柄操作 | `configASSERT` 或钩子 |
| 混用 `malloc`/`pvPortMalloc` | 随机崩溃 | 两套堆互不认识 | 全程只用 `pvPortMalloc` |
| 栈深按字节填 | 栈瞬间溢出 | 参数单位是**字** | 字节 ÷ 4 |
| 用 `xPortGetFreeHeapSize` 判安全 | 峰值时崩 | 它看不到历史最低 | 用 `MinimumEverFree` |
| 碎片导致大分配失败 | 总余量看着够却拿不到 | 合并只处理相邻块 | 大缓冲用静态数组 |
| 删任务后以为内存已回收 | 余量不涨 | 回收靠**空闲任务** | 确认空闲任务在跑 |
| 钩子里做准备返回 | 更早失败 | 分配已失败 | 钩子里只停机 + 记录 |

---

## 6. 调试方法

1. **两个余量一起看**：`xPortGetFreeHeapSize()` 是当前，`xPortGetMinimumEverFreeHeapSize()` 是历史最低。
   **判断"会不会爆"只看后者。**
2. **看 `.bss` 里的 `ucHeap`**：`arm-none-eabi-size firmware.elf` 的 `bss` 里应含 4096 B 的 `ucHeap`
   （`configAPPLICATION_ALLOCATED_HEAP 0` 时由内核定义）。`bss` 加上 `.data` 不超过 20 KB SRAM。
3. **算栈的实际占用**：创建时给的栈深 × 4 + TCB（约 80–100 B）+ 任务名缓冲。
   `uxTaskGetStackHighWaterMark()` 给的是**剩余**，不是分配量。
4. **失败先怀疑堆，不要先怀疑代码**：`configMALLOC_FAILED_HOOK` 打开时，堆耗尽会直接进钩子。
5. **栈与堆是两笔账**：任务栈从堆里切出去后就是任务的，不归还；"堆余量低"和"某任务栈要溢出"是两件事。

---

## 7. 与相近技术的对比

| 对比 | FreeRTOS heap_4 | libc `malloc` | 静态数组 | Linux `kmalloc` |
| --- | --- | --- | --- | --- |
| 空间来源 | 固定静态数组 | 系统 brk/mmap | 编译期确定 | 页分配器 |
| 碎片处理 | 合并相邻 | 更复杂（bins/合并） | 无碎片 | slab + buddy |
| 可用性 | 4 KB | 随进程 | 编译期确定 | 大得多 |
| 实时性 | 大致确定（首次适配） | 不确定 | 最快 | 不确定 |

**与 pthread 的对比**：`pthread_create` 的栈由 libc/内核按 `pthread_attr_setstacksize` 分配，
不占进程的"堆"；FreeRTOS 的任务栈与 TCB **都从同一个 `ucHeap` 出**，这就是 4 KB 这么紧张的原因。

---

## 8. 验证状态与边界

| 结论 | 级别 | 证据 |
| --- | --- | --- |
| 堆随任务创建下降 | **实测** | `exp10_freertos.log`：`heap_free` 1112 → 496 B |
| 4 KB 堆可被"四任务 + 三信号量 + 栈上 ctx"耗尽 | **实测** | `F103_16` 首版：`vApplicationMallocFailedHook` 触发，日志停在半截；根因是 `xTaskCreate` 返回值从未检查 |
| 栈水位可观测 | **实测** | 同上日志：154/126/90 字 |
| heap_4 的合并相邻块行为 | **源码核对** | 内核 `heap_4.c` 机制；**无碎片化实验** |
| 静态创建 | **不可用** | `configSUPPORT_STATIC_ALLOCATION 0` ⇒ `xTaskCreateStatic` 编译不出来 |
| 碎片导致的大分配失败 | **未验证** | 需专门构造的分配/释放序列 |
| `xPortGetMinimumEverFreeHeapSize` 的峰值含义 | **源码核对** | 内核维护历史最低；本仓库日志未打印该值 |

**明确不能说的话**：不能说"本仓库的堆管理经过压力测试"——只观察到一次耗尽与一次正常分配。

---

## 证据来源

- 配置：`experiments/F103_common/vendor/freertos/include/FreeRTOSConfig.h:29-37`、`:76-84`
- 实现选择：`experiments/F103_common/cmake/f103.cmake`（`portable/MemMang/heap_4.c`）
- 实测日志：`tests/qemu-stm32/logs/exp10_freertos.log`、`exp16_priority_inversion.log`
- 实验：`experiments/F103_10_FreeRTOS_Tasks/`、`F103_16_FreeRTOS_PriorityInversion/`
- 讲义：`notes/FreeRTOS入门/Day02_源码结构与内存管理.md`
- 相关：[FreeRTOS 任务与调度](FreeRTOS任务与调度_速查表.md)
