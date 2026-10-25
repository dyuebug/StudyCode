# Linux 学习工作区

本目录由 `CLAUDE.md` 统一管理。学习计划、日志、复盘、知识库和实验产出都放在本目录下；`Linux_WEIDS/` 是嵌入式 Linux / RTOS 原始资料库，不是学习管理文件的根目录。

## 总控与资料

- `CLAUDE.md`：Linux 学习总控文件。
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

## 当前驱动入门学习入口

本轮使用 14 天驱动入门主线；它是当前驱动分支的学习安排，不覆盖或改写其他 Linux 项目的历史能力记录。

- [每日任务总导航](daily_tasks/README.md)：Day1～Day14 的任务、时长和最小输出。
- [Day1 实际日志](daily_logs/2026-09-28_第1天_日志.md)：概念完成；Day1 用户态实验已在 WSL Ubuntu 26.04 实测通过，日志中的"未验证"保留为当日历史状态。
- [Day2 当前衔接任务](daily_tasks/2026-09-19_第2天_任务.md)：模块生命周期、日志、字符设备和文件身份。
- [Day3 后续入口](daily_tasks/2026-09-19_第3天_任务.md)：构建规则、设备文件、调用链与数据协议；后续可按序独立学习，不必等逐题聊天批改。

Day3～Day14 已预先编排，全部仍为计划。现有文件日期与 Day 序号不完全一致，请按 Day 学习；本批新增文件日期是计划编制日期，实际日期在执行时填写，不重命名历史记录。

### 14 天标准材料

按 `CLAUDE.md` §6 的 12 节结构编写的完整材料，位于 [`notes/驱动入门/`](notes/驱动入门/)：

| Day | 主题 | 验证状态 |
| --- | --- | --- |
| 1 | 应用与驱动整体认知 | 已实测（WSL gcc 15.2.0 + MinGW 15.1.0） |
| 2 | 第一个内核模块 | 已实测（QEMU vexpress-a9） |
| 3 | 驱动结构与调用链 | 已实测（QEMU vexpress-a9） |
| 4 | 地址空间与用户拷贝 | 已实测（QEMU vexpress-a9） |
| 5 | 交叉编译与板级驱动 | 未验证 |
| 6 | 分离思想与 platform | 已实测（QEMU vexpress-a9） |
| 7 | 总线模型深化与复盘 | 已实测（QEMU vexpress-a9） |
| 8 | 设备树入门 | 已实测（QEMU vexpress-a9） |
| 9 | pinctrl 与 GPIO | 已实测（QEMU vexpress-a9） |
| 10 | 中断与 poll | 未验证 |
| 11 | 上下文与资源退出 | 未验证 |
| 12 | SR501 实战 | 未验证 |
| 13 | I2C 与总线分层 | 未验证 |
| 14 | SPI / Framebuffer 与总复盘 | 未验证 |

可运行实验位于 [`experiments/`](experiments/)，每个 Day 一个目录，含驱动、测试程序与设备树片段。

验证方式：

- Day1：WSL gcc 15.2.0 + MinGW gcc 15.1.0 实测。
- Day2/3/4/6/7/8/9：QEMU vexpress-a9 + Linux 7.0.0 实测，证据见 [`tests/qemu-vexpress/RESULT.md`](tests/qemu-vexpress/RESULT.md)。
- Day5/10/11/12/13/14：需要 i.MX6ULL 目标内核、可触发中断、I2C/SPI 外设或真实传感器，当前未覆盖。

## 配套学习材料

| 材料 | 解决的问题 |
| --- | --- |
| [环境与验证指南](experiments/驱动入门_环境与验证指南.md) | Windows/WSL/虚拟机/板卡能力、模块构建加载、交叉编译、故障证据 |
| [接口与版本速查表](knowledge_base/Linux驱动入门_接口与版本速查表.md) | API 职责、返回值、资源生命周期、并发上下文和版本差异 |
| [练习与验收工作簿](notes/驱动入门_Day2-Day14_练习与验收工作簿.md) | 每日三类题、日志记录卡、Day7/Day14 自评和下阶段计划 |

课程讲义继续复用 `Linux_WEIDS/.../StudyPlan/DailyDocs/`，不改动原始课程代码，不将旧示例默认视为当前环境可直接运行或生产级实现。

所有记录必须区分：源码阅读、编译验证、软件接口验证、仿真验证和真实开发板验证。没有实际学习结果时，不预填成功日志、周报或完成率。
