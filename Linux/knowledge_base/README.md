# 知识库

这里保存可复用的主题速查表，例如：

- `Linux用户态与内核态_速查表.md`
- `字符设备驱动_速查表.md`
- `FreeRTOS任务与队列_速查表.md`
- `DeviceTree与platform_速查表.md`

每条结论应记录来源、适用场景、核心接口、错误陷阱、调试方法和当前验证状态。

## 已整理材料

- [Linux 驱动入门：接口与版本速查表](Linux驱动入门_接口与版本速查表.md)：模块、字符设备、用户拷贝、platform、设备树、GPIO、等待/poll 与总线分层。当前为源码整理材料，未在目标内核运行验证。
- [FreeRTOS 同步语义与优先级反转](FreeRTOS同步语义与优先级反转_速查表.md)：先选语义再选 API、二值信号量当锁的三个缺口、优先级反转的实测读数与继承的边界、栈水位判据。接口行为与读数实测（QEMU），中断延迟/临界区/WCET 未验证。
- [FreeRTOS 任务与调度](FreeRTOS任务与调度_速查表.md)：`configMAX_PRIORITIES 5` ⇒ 合法优先级 0–4、抢占与时间片的边界、`vTaskDelay` 漂移 vs `xTaskDelayUntil` 不漂移、栈深单位是**字**、栈水位"绝对 ≥32 字且相对 ≥25%"双门限。任务创建/栈堆读数实测；真板抢占时延与 `xTaskDelayUntil` 抖动未验证；`configSUPPORT_STATIC_ALLOCATION 0` ⇒ 静态创建**不可用**。
- [FreeRTOS 队列 / 事件组 / 任务通知](FreeRTOS队列与通知_速查表.md)：三者选型表（数据/状态、多对多/点对点、能否"等 A 且 B"）、队列是拷贝语义、事件组 ISR 版**异步**（`pdPASS` = 已入队非已置位）、任务通知只有一份值且 `ulBitsToClearOnEntry` 仅在非 pending 时执行。队列与事件组读数实测；**`configUSE_QUEUE_SETS 0` ⇒ 队列集编译不出来**。
- [FreeRTOS 软件定时器](FreeRTOS软件定时器_速查表.md)：`xTimerCreate` 创建后是**冬眠态**、`xTicksToWait` 是命令队列超时不是周期、`xTimerReset` 重算 `tX + n`、回调跑在唯一的 daemon 任务里所以**不能阻塞**。`in_daemon=1` 与"阻塞后是追赶补发而非永久偏移"实测；回调阻塞拖累全部定时器为机制推理，未做受控实验。
- [FreeRTOS 内存管理](FreeRTOS内存管理_速查表.md)：heap_4 的首次适配 + 相邻块合并、4 KB 堆的账该怎么算、`xPortGetMinimumEverFreeHeapSize` 才是判断溢出的依据、两套堆不可混用。一次真实堆耗尽（`F103_16` 首版四任务 + 三信号量）与 `heap_free 1112 → 496` 实测；碎片化未做实验。
- [FreeRTOS 中断与临界区](FreeRTOS中断与临界区_速查表.md)：`BASEPRI = 0x50` 的分界含义（数值 0–4 不被屏蔽且禁止调 API）、`taskENTER_CRITICAL_FROM_ISR` 必须传回保存值、漏 `portYIELD_FROM_ISR` 的静默 1 tick 延迟、`vTaskSuspendAll` 的正确名字（教材笔误）。优先级继承的相对效果实测；屏蔽范围与中断延迟**未用仪器验证**，`PRIGROUP` 未显式设置一条为推理。
- [BSP 启动链与切根](BSP启动链与切根_速查表.md)：三件套分工、`switch_root` 四步、`f_type` 判据与它的两个坑（rootfs 可能是 tmpfs；devtmpfs 与 tmpfs 同魔数）。五组对照实测，`pivot_root`/真板未验证。
- [Linux 文件 I/O 与进程生命周期](Linux文件IO与进程_速查表.md)：`O_APPEND` 原子性 vs `lseek` 竞态、短读、`EINTR`、僵尸/孤儿/`subreaper`、`wait` 状态解码。判定器 19/19 实测（WSL2）；NFS/overlayfs 与撕裂行未验证。
- [Linux IPC 与线程同步](Linux并发与IPC_速查表.md)：五种通道的边界/EOF 语义、`if` vs `while`、有界队列、四种互斥手段、`timedlock` 死锁取证、线程池两种关闭协议。26/26 实测；背压与吞吐未测。
- [Socket 与 I/O 多路复用](Linux网络与多路复用_速查表.md)：TCP/UDP 语义、长度前缀分帧、半关闭、四个 errno 的触发条件、短写、`select`/`poll`/`epoll` 与 ET 必须读到 `EAGAIN`。25/25 实测；真实网络与压测未做。
- [服务模型与协程](Linux服务模型与协程_速查表.md)：三种 Reactor 形态的唯一差别、`EPOLLONESHOT` 重挂、必然失效的破坏版、协程帧生命周期与"每次 `co_await` 都要有触发源"、`VmSize` vs `RSS`、**项目级八条落地规则**（读缓冲满/信号屏蔽/SIGPIPE/背压/世代号/在飞期间不读 socket/退出排空/跨线程计数）。26/26 与 58/58 实测，`reactor-server` 105/105 实测（默认 + ASan + TSan + UBSan 四个构建）；无吞吐基线。
- [Linux 调试与性能定位](Linux调试与性能_速查表.md)：缺工具时的替代实现与各自的观测边界（`ptrace` 的 vDSO 盲区、回溯地址减 1 规则、三组 sanitizer、`perf_event_open` 权限分层）。101/101 实测；内联帧与非子进程 attach 未验证。
- [STM32F103 外设：ADC / CAN / DMA](STM32F103外设_ADC_CAN_DMA_速查表.md)：时钟与位时序算术、三步"读回配置值"判据、`CNDTR`/`EN` 规则、bxCAN 两段初始化。**构建级 + 寄存器路径证据，功能全部待真板**。
- [DShanMCU-F103 模块与驱动](DShanMCU-F103模块与驱动_速查表.md)：20 个模块的「驱动 ↔ 引脚 ↔ HAL 模块 ↔ 谁配引脚」总表、三条引脚冲突的物理原因（同一根线两种空闲电平）、驱动对 CubeMX 初始化与 TIM4 时基的依赖、EXTI 线号只有一个端口这一坑。**引脚与依赖源码核对，构建实测；板上功能全部未验证**。
- [逻辑分析仪与 SUMP 协议](逻辑分析仪与SUMP协议_速查表.md)：三个硬上限的算术（采样内存定窗口、接口带宽定 stream 速率、采样率定可测的最窄脉冲）、打包 1 B vs `uint32_t` 的 4 倍深度差、"读回全 0 必须与不计数分开"、环形缓冲满时丢新的代价。核心逻辑 54/54 + 六节 `assert` 主机端实测、QEMU 端到端链路实测；**TIM2 在 QEMU 未建模，SUMP 二进制协议未实现，真板波形未验证**。
- [每日任务导航](../daily_tasks/README.md)：对应练习、实验前置和源码阅读顺序。
