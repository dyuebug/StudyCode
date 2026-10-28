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
- [BSP 启动链与切根](BSP启动链与切根_速查表.md)：三件套分工、`switch_root` 四步、`f_type` 判据与它的两个坑（rootfs 可能是 tmpfs；devtmpfs 与 tmpfs 同魔数）。五组对照实测，`pivot_root`/真板未验证。
- [Linux 文件 I/O 与进程生命周期](Linux文件IO与进程_速查表.md)：`O_APPEND` 原子性 vs `lseek` 竞态、短读、`EINTR`、僵尸/孤儿/`subreaper`、`wait` 状态解码。判定器 19/19 实测（WSL2）；NFS/overlayfs 与撕裂行未验证。
- [Linux IPC 与线程同步](Linux并发与IPC_速查表.md)：五种通道的边界/EOF 语义、`if` vs `while`、有界队列、四种互斥手段、`timedlock` 死锁取证、线程池两种关闭协议。26/26 实测；背压与吞吐未测。
- [Socket 与 I/O 多路复用](Linux网络与多路复用_速查表.md)：TCP/UDP 语义、长度前缀分帧、半关闭、四个 errno 的触发条件、短写、`select`/`poll`/`epoll` 与 ET 必须读到 `EAGAIN`。25/25 实测；真实网络与压测未做。
- [服务模型与协程](Linux服务模型与协程_速查表.md)：三种 Reactor 形态的唯一差别、`EPOLLONESHOT` 重挂、必然失效的破坏版、协程帧生命周期与"每次 `co_await` 都要有触发源"、`VmSize` vs `RSS`、**项目级八条落地规则**（读缓冲满/信号屏蔽/SIGPIPE/背压/世代号/在飞期间不读 socket/退出排空/跨线程计数）。26/26 与 58/58 实测，`reactor-server` 105/105 实测（默认 + ASan + TSan + UBSan 四个构建）；无吞吐基线。
- [Linux 调试与性能定位](Linux调试与性能_速查表.md)：缺工具时的替代实现与各自的观测边界（`ptrace` 的 vDSO 盲区、回溯地址减 1 规则、三组 sanitizer、`perf_event_open` 权限分层）。101/101 实测；内联帧与非子进程 attach 未验证。
- [STM32F103 外设：ADC / CAN / DMA](STM32F103外设_ADC_CAN_DMA_速查表.md)：时钟与位时序算术、三步"读回配置值"判据、`CNDTR`/`EN` 规则、bxCAN 两段初始化。**构建级 + 寄存器路径证据，功能全部待真板**。
- [每日任务导航](../daily_tasks/README.md)：对应练习、实验前置和源码阅读顺序。
