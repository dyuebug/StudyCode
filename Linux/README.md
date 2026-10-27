# Linux 学习工作区

本目录由 `CLAUDE.md` 统一管理。学习计划、日志、复盘、知识库和实验产出都放在本目录下；`Linux_WEIDS/` 是嵌入式 Linux / RTOS 原始资料库，不是学习管理文件的根目录。

## 当前阶段（2026-10-26）

**阶段 7 嵌入式主线：MCU 外设 → FreeRTOS → BSP 启动链**，位于 [`notes/航空转嵌入式路线图.md`](notes/航空转嵌入式路线图.md) 12 周计划的第 5 周（按验收计）；主题已覆盖到第 7–8 周的可仿真部分（2026-10-26 落地）。详细基线与优先级见 `CLAUDE.md` §4.2 与 §12。

- 进行中：FreeRTOS 验收（信号量、事件组、软件定时器、栈监控、栈溢出检出、心跳监督与停喂/恢复已完成 → 待 `IWDG` 复位与复位原因，需真板）。
- 进行中：路线图第 7–8 周航空主题项目的**可仿真部分**——[`projects/aviation-node/`](projects/aviation-node/README.md) 已完成 PC 注入 → 帧校验 → 有效性判定 → 模式状态机 → 降级/恢复 → 停喂门控 → 下行二进制 `STATUS` 帧的全链路实测（证据见 `tests/qemu-stm32/RESULT.md` §3.10）；`bash run.sh aviation-link` 结束时由 `aviation/expect.py` 判 21 项上行不变量、由 `aviation/link.py` 判 7 项下行不变量，任一失败即非零退出。六类故障（错帧/越界/突变/静默/丢帧/任务阻塞，最后一类为固件侧编译期注入）都有用例与机器判定——这是第 9–10 周的主体；CAN、ADC、示波器/逻辑分析仪、`IWDG` 与故障记录掉电保持单列为待真板，复位原因（`RCC->CSR`）在仿真里读回 0，同样只能上板。
- 已完成、待真板补测：MCU 外设 01–09、FreeRTOS 10/11/12/13/14/15/16、驱动入门 Day1–14、BSP_01/02/03/04（验证边界各有 README）。
- 暂停：高性能服务器工程化（`benchmarks/`、`scripts/` 为空；`projects/` 仅有嵌入式主线的 `aviation-node`，与服务器线无关），恢复条件见 `CLAUDE.md` §12「P4：暂停项」。

## 总控与资料

- `CLAUDE.md`：Linux 学习总控文件。
- [`notes/航空转嵌入式路线图.md`](notes/航空转嵌入式路线图.md)：从航空飞管 C 软件转向嵌入式的定位、能力差距与阶段规划，是 `CLAUDE.md` 学习目标的展开说明。
- `job-descriptions.csv`：目标岗位原始数据（职位名、城市、薪资、JD 正文），用于校准学习重点。
- `Linux_WEIDS/`：STM32、FreeRTOS、RT-Thread、Linux 应用、Linux 驱动和 BSP 参考资料。
- `Linux_WEIDS/Docs/Obsidian学习导航/00_学习主页.md`：资料库的 Obsidian 导航入口。

## 学习管理目录

| 目录 | 用途 |
| --- | --- |
| `daily_tasks/` | 每日任务清单 |
| `daily_logs/` | 实际学习日志和验证证据 |
| `weekly_reports/` | 周报 |
| `stage_reports/` | 阶段复盘 |
| `knowledge_base/` | 主题速查表和知识卡片 |
| `notes/` | 概念、源码和机制笔记 |
| `experiments/` | 单主题最小实验 |
| `projects/` | 可复现综合项目 |
| `tests/` | 测试脚本、测试数据和验证说明 |
| `benchmarks/` | 性能与稳定性基准 |
| `scripts/` | 构建、测试、部署和辅助脚本 |

## 驱动入门分支（Day1–Day14）

驱动入门分支已按 14 天走完主线，QEMU 可验证部分已闭环；它是当前驱动分支的学习记录，不覆盖或改写其他 Linux 项目的历史能力记录。

- [每日任务总导航](daily_tasks/README.md)：Day1～Day14 的任务、时长和最小输出。
- [Day1 实际日志](daily_logs/2026-09-28_第1天_日志.md)：概念完成；Day1 用户态实验已在 WSL Ubuntu 26.04 实测通过，日志中的"未验证"保留为当日历史状态。
- [Day2 当前衔接任务](daily_tasks/2026-09-19_第2天_任务.md)：模块生命周期、日志、字符设备和文件身份。
- [Day3 后续入口](daily_tasks/2026-09-19_第3天_任务.md)：构建规则、设备文件、调用链与数据协议；后续可按序独立学习，不必等逐题聊天批改。

Day3～Day14 已编排，验证进度见下方「14 天标准材料」表的「验证状态」列。现有文件日期与 Day 序号不完全一致，请按 Day 学习；任务文件日期是计划编制日期，实际执行日期以 `daily_logs/` 为准，不重命名历史记录。

### 14 天标准材料

按 `CLAUDE.md` §6 的 12 节结构编写的完整材料，位于 [`notes/驱动入门/`](notes/驱动入门/)：

| Day | 主题 | 验证状态 |
| --- | --- | --- |
| 1 | 应用与驱动整体认知 | 已实测（WSL gcc 15.2.0 + MinGW 15.1.0） |
| 2 | 第一个内核模块 | 已实测（QEMU vexpress-a9） |
| 3 | 驱动结构与调用链 | 已实测（QEMU vexpress-a9） |
| 4 | 地址空间与用户拷贝 | 已实测（QEMU vexpress-a9） |
| 5 | 交叉编译与板级驱动 | 部分实测（交叉编译/加载/卸载；寄存器未访问） |
| 6 | 分离思想与 platform | 已实测（QEMU vexpress-a9） |
| 7 | 总线模型深化与复盘 | 已实测（QEMU vexpress-a9） |
| 8 | 设备树入门 | 已实测（QEMU vexpress-a9） |
| 9 | pinctrl 与 GPIO | 已实测（QEMU vexpress-a9） |
| 10 | 中断与 poll | 编译实测 + 机制部分实测（运行用替代模块；中断未覆盖） |
| 11 | 上下文与资源退出 | 编译实测 + 机制部分实测（运行用替代模块；中断未覆盖） |
| 12 | SR501 实战 | 编译实测（ARM）；硬件未验证 |
| 13 | I2C 与总线分层 | 编译实测（ARM）；I2C 未验证 |
| 14 | SPI / Framebuffer 与总复盘 | 编译实测（ARM）；SPI/FB 未验证 |

可运行实验位于 [`experiments/`](experiments/)，每个 Day 一个目录，含驱动、测试程序与设备树片段。

验证方式：

- Day1：WSL gcc 15.2.0 + MinGW gcc 15.1.0 实测。
- Day2/3/4/6/7/8/9：QEMU vexpress-a9 + Linux 7.0.0 实测，证据见 [`tests/qemu-vexpress/RESULT.md`](tests/qemu-vexpress/RESULT.md)。
- Day10/11：本日驱动 `gpio_key_irq.ko` / `key_ctx.ko` 已交叉编译通过；运行验证使用**等价替代模块** `tests/qemu-vexpress/module/event_demo.c`（定时器事件替代硬件中断），覆盖等待队列、poll、ioctl、timer/tasklet/work 与卸载清理；中断投递未覆盖。
- Day5：交叉编译、加载、设备节点与卸载已在 QEMU 实测；i.MX6ULL 寄存器与 LED 现象需要真实目标板。
- Day12/13/14：三个驱动与测试程序已交叉编译为 ARM 产物（2026-10-25）；SR501、AT24C02、SPI OLED 的外设行为需要真实硬件。
- 启动链（Day19）：[`experiments/BSP_01_Boot_Chain/`](experiments/BSP_01_Boot_Chain/README.md) 用四组对照实测 zImage / DTB / initramfs 的分工。实测发现 QEMU 会改写传入的 DTB（`/sys/firmware/fdt` 49086 B ≠ 磁盘 14543 B，memory 节点 1 GiB → 512 MiB），因此属性真值只认运行期 `/sys/firmware/devicetree/base/`；u-boot 阶段与块设备根文件系统未覆盖。
- 启动链下一层（Day20）：[`experiments/BSP_02_UBoot_BlockRoot/`](experiments/BSP_02_UBoot_BlockRoot/README.md) 用八组对照实测 u-boot 装载与参数传递、initrd 传递、ext4 块设备根。实测：`bootz` 不给 fdt 参数时内核不启动（`FDT and ATAGS support not compiled in`），`/proc/cmdline` 与 u-boot `bootargs` 逐字一致；`bootz` 的 initrd 只收 uImage/FIT，裸 gzip initramfs 报 `Wrong Ramdisk Image Format`，经 `mkimage -T ramdisk` 打包后即通（`Unpacking initramfs...`、`Freeing initrd memory: 264K`）；initramfs 里有 `/init` 时 `root=` 被整个跳过（命令行含 `root=/dev/vda` 但日志无 `EXT4-fs (vda)`）；块设备根先挂只读、由 init remount；`root=/dev/vda1` 在没有分区表时报 `unknown-block(254,1)`，与"没有块设备"的 `unknown-block(0,0)` 不同。u-boot 用的是 apt 包里 `qemu_arm`(virt) 构建，机器与 BSP_01 的 vexpress 不同；真实 vexpress u-boot、SD/eMMC 的 `rootwait`、BootROM/SPL、initramfs 两阶段挂根未验证。
- 两阶段切根（Day21）：[`experiments/BSP_03_SwitchRoot/`](experiments/BSP_03_SwitchRoot/README.md) 用五组对照实测 `switch_root` 四步（建 devtmpfs/proc/sys → 挂新根 → `MS_MOVE` 搬运 → `chroot`+`exec`）。实测：`root_before` 的 `f_type` 是 `tmpfs`(0x1021994) 而非教科书说的 `RAMFS_MAGIC`——本内核在命令行无 `root=` 且 `CONFIG_TMPFS=y` 时把 rootfs 建在 shmem 上，加一组 `rootfs-ramfs`（只改命令行）即得到 `0x858458f6`；devtmpfs 以 shmem 为后端，`f_type` 与 tmpfs 相同，所以"搬运成功"只能由 `/proc/self/mounts` 类型串（`mounts_dev=devtmpfs`）与 `/dev/vda` 存在性证明；跳过 `MS_MOVE` 时根照样换成 ext4，但 `/dev` 退化为空目录、设备节点全丢。`pivot_root`、busybox `switch_root`、真板启动链未验证。
- 切根对照（Day22）：[`experiments/BSP_04_PivotRoot/`](experiments/BSP_04_PivotRoot/README.md) 五组同口径对照实测 `pivot_root` 与 `chroot + exec`。两者在进程**内部**读数完全相同，唯一可观测差别在进程之外：切换前 fork 的观察者在 `chroot` 组仍见 `tmpfs` + `/init`（只改本进程视角），在 `pivot` 组见 `ext4` + `gone`（命名空间一起换根）。`pivot-nomove` 证明 `pivot_root` 不搬挂载点（根已换成 ext4 但 `dev_vda=absent`）；`pivot-ondir` 对普通目录 pivot 得 `EINVAL(22)`，失败后观察者仍见旧根。旧根严格 `umount` 恒 `EBUSY(16)`，`exec` 之后也一样——**否证**了"stage1 可执行映射钉住旧根"的假设：真正钉住它的是 `init/noinitramfs.c:18` 的 `default_rootfs()` 建在 rootfs 里的 `/dev/console`（探针 `boot_console|present|mode=020600`），被 `init/main.c:1603 console_on_rootfs()` dup 到 0/1/2；同一次运行里只重绑这三个 fd，同一挂载立刻严格卸载成功（`busy(16)` → `ok`），随后兜底 `MNT_DETACH` 返回 `EINVAL(22)` 正说明它已经不在挂载表里。另实测一条反直觉事实：从 `chroot` 内部读 `/proc/self/mounts`，旧根那一行被内核隐藏（`mounts_rootfs=0`），因此挂载表不能当作"是否释放旧根"的证据。`--judge` 做过 10 条负向验证。
- 优先级反转（Day21）：[`experiments/F103_16_FreeRTOS_PriorityInversion/`](experiments/F103_16_FreeRTOS_PriorityInversion/README.md) 同一骨架只换锁类型：二值信号量轮 H 等锁 405 tick（另一次 412）、持锁者优先级恒 1；互斥量轮 105 tick（另一次 103）、优先级 1→3（`give` 内部回 1）。继承只压缩"被插队"的那段，持锁者自己的 `vTaskDelay` 未变短——这就是等待时间不为 0 的原因；两次运行的漂移区间使判定只能写成 `binary > 2 × mutex`。

## MCU 分支（STM32F103）

驱动入门之后的嵌入式主干，按 `CLAUDE.md` 阶段 7 的「STM32F103 外设 → FreeRTOS」推进。

- [`experiments/F103_common/`](experiments/F103_common/README.md)：自撰链接脚本、裁剪的 CMSIS 子集、CMake 工具链文件、OpenOCD 烧写配置。
- 实验索引：[`experiments/README.md`](experiments/README.md) 的「MCU 分支实验」表。已覆盖 GPIO 输出、按键轮询、EXTI 中断、USART1 轮询收发、串口中断接收与环形缓冲、TIM2 PWM、I2C1 主机读写、SPI1 读写外部 Flash、SysTick/TIM4/DWT 三种微秒延时共九组寄存器级实验，以及在此基础上引入的内置 FreeRTOS V11.3.0 六课（任务/队列/互斥量、中断→任务交接、同步原语与事件组、软件定时器与回调上下文、心跳监督与看门狗路径、栈溢出检出）；均为构建实测与反汇编核对，板级现象待接板确认；I2C 与 SPI 实验还需外接对应模块。
- QEMU 仿真验证：[`tests/qemu-stm32/`](tests/qemu-stm32/README.md)。实测确认 `-M stm32vldiscovery` 只真实建模 USART1–3 与 SysTick（含 NVIC 中断），GPIO/EXTI/RCC/TIM/I2C/DWT/IWDG 未建模、SPI 半建模；因此实验 04/05 的收发与中断通路已在仿真中跑通（原始日志在 `tests/qemu-stm32/logs/`），建立在这条主干上的实验 10（FreeRTOS 调度、队列、堆/栈指标）、实验 11（`xQueueSendFromISR`/`portYIELD_FROM_ISR`、行解析）、实验 12（二值/计数信号量、事件组、`xSemaphoreGiveFromISR` 的事件合并、栈水位判据）、实验 13（守护任务上下文、单次/周期定时器、回调阻塞的代价）、实验 14（心跳超时判定、"与"语义停喂、停喂窗口与 IWDG 名义超时的比较）与实验 15（栈底 16 B 填充被写穿后 `vApplicationStackOverflowHook` 被调用）同样可实测；实验 14 的 IWDG 计时/复位与 `RCC->CSR` 复位原因未建模，只能上板；实验 15 只覆盖 paint 判据，且 QEMU 不因越界写抛总线错误，"先 HardFault 还是先到钩子"在仿真里没有答案。其余实验在仿真里拿不到有效证据。QEMU 的 SYSCLK 为 24 MHz（固件按 8 MHz 假设），实测时基偏差 2.90 倍，时间相关结论一律不得来自仿真。
- 工具链：`arm-none-eabi-gcc` + CMake + Ninja + ST-Link V2 + OpenOCD。
- 综合项目：[`projects/aviation-node/`](projects/aviation-node/README.md)。上行二进制帧（帧头/类型/序号/长度/CRC16）、三层有效性判定（帧/样本/时间）、模式状态机（INIT/NORMAL/DEGRADED/SAFE 与两级恢复）、周期任务 + 健康监控 + 停喂门控；协议规范见 [`docs/protocol.md`](projects/aviation-node/docs/protocol.md)，故障注入测试报告见 [`docs/test-report.md`](projects/aviation-node/docs/test-report.md)，任务时序与实时性边界见 [`docs/task-timing.md`](projects/aviation-node/docs/task-timing.md)，分层架构与变更影响见 [`docs/architecture.md`](projects/aviation-node/docs/architecture.md)。QEMU 下全链路有实测日志；**不是**真实飞控，也不含任何工作项目代码/参数。
- 课程资料 `Linux_WEIDS/DShanMCU-F103/` 不入库（体积原因），实验所需的最小 vendor 子集已随仓库跟踪。

## 配套学习材料

| 材料 | 解决的问题 |
| --- | --- |
| [环境与验证指南](experiments/驱动入门_环境与验证指南.md) | Windows/WSL/虚拟机/板卡能力、模块构建加载、交叉编译、故障证据 |
| [接口与版本速查表](knowledge_base/Linux驱动入门_接口与版本速查表.md) | API 职责、返回值、资源生命周期、并发上下文和版本差异 |
| [练习与验收工作簿](notes/驱动入门_Day2-Day14_练习与验收工作簿.md) | 每日三类题、日志记录卡、Day7/Day14 自评和下阶段计划 |

课程讲义继续复用 `Linux_WEIDS/.../StudyPlan/DailyDocs/`，不改动原始课程代码，不将旧示例默认视为当前环境可直接运行或生产级实现。

所有记录必须区分：源码阅读、编译验证、软件接口验证、仿真验证和真实开发板验证。没有实际学习结果时，不预填成功日志、周报或完成率。
