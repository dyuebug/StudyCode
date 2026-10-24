# Linux_WEIDS 资料库地图

> 本文件是资料库的**只读地图与导航**，不覆盖上级 `Linux/CLAUDE.md`；学习流程、模板与质量门禁以上级文件为准。
>
> - 日志、任务、周报统一写入 `Linux/daily_logs/`、`Linux/daily_tasks/`、`Linux/weekly_reports/`，**不写入本目录**。
> - 本库的第三方与课程原始资料**不入版本控制**，下文路径以本地实际存在为准，克隆仓库后可能不存在。
> - 已入库的只有：`README.md`（本文件）、`Docs/`、`02_嵌入式Linux驱动入门直播/StudyPlan/`、`daily_materials/`、`daily_tasks/`。

---

## 1. 资料库定位

本工作区是嵌入式底层学习资料库，不是单一代码项目。

### 1.1 RTOS 资料区

路径（本地，未入库）：

```text
rtos_doc_source/
```

定位：FreeRTOS / RT-Thread / ARM / RISC-V / MCU 项目 / 实时系统内部机制。

重点资料：

```text
rtos_doc_source/RTOSFiles/FilesAnalysis/freertos-linux-driver-systematic-guide.md
rtos_doc_source/RTOSFiles/FilesAnalysis/Study/complete-embedded-rtos-linux-study-plan.md
rtos_doc_source/RTOSCode/FreeRTOS/
```

学习重点：任务创建与删除、优先级与调度、Tick 与延时、队列、信号量、互斥量、优先级反转与优先级继承、事件组、任务通知、软件定时器、中断与任务协作、内存管理。

### 1.2 Linux 快速入门资料区

路径（本地，未入库）：

```text
01_全系列新Linux系列教程之快速入门视频/
```

定位：Linux 基础 + 应用开发 + 早期驱动认知 + 开发板实践 + 项目框架。

学习重点：Linux 基本命令、Ubuntu 环境、GCC / Makefile、文件 I/O、多线程、网络编程、Framebuffer、输入系统、I2C / 串口应用、Hello 驱动、GPIO / LED、设备树、中断、poll / mmap。

### 1.3 Linux 驱动入门主线资料区

路径：

```text
02_嵌入式Linux驱动入门直播/
```

定位：驱动从入门到能读代码、能改小代码、能进入专题的主训练营。

- `StudyPlan/`：**已入库**，本阶段的总控目录，规划优先参考此处。
- `01_…` 至 `30_…` 专题源码目录：**本地，未入库**。

---

## 2. 学习路线

```text
阶段 A：资料地图与环境确认
阶段 B：Linux 应用与驱动整体认知
阶段 C：第一个 Linux 驱动最小闭环
阶段 D：字符设备驱动与 APP 交互
阶段 E：总线 / 设备 / 驱动模型
阶段 F：设备树 / pinctrl / GPIO
阶段 G：中断 / poll / ioctl / 异步通知
阶段 H：SR501 / DHT11 / DS18B20 等传感器实战
阶段 I：I2C / SPI / Input / Framebuffer 专题
阶段 J：BSP / 驱动项目化整理与作品集
```

优先级原则：先主线后专题、先跑通后深入、先能讲清再求复杂、先最小闭环再扩展项目。

虚拟化学习原则：

```text
主机侧优先使用 Ubuntu 虚拟机；
目标侧优先使用资料中的 100ask_imx6ull-qemu 环境；
真实开发板不可用时，用 QEMU 完成编译、加载、日志观察和最小驱动验证闭环。
```

QEMU 阶段要建立的能力：主机环境与目标环境的区别、交叉编译流程、`.ko` 如何进入目标系统、`insmod` / `rmmod` / `dmesg` 闭环、`board_100ask_imx6ull-qemu.c` 与模拟平台的关系。

---

## 3. 驱动入门 14 天主线

**以实际任务文件为准**（`Linux/daily_tasks/`），讲义见 `StudyPlan/DailyDocs/`。

| Day | 主题 | 讲义 |
| --- | --- | --- |
| 1 | 建立 Linux 应用与驱动的整体认知 | `01_Day1_建立整体认知` |
| 2 | 认识第一个内核模块和字符设备驱动 | `02_Day2_认识第一个驱动` |
| 3 | 构建、设备文件与读写调用链 | `03_Day3_彻底分析第一个驱动` |
| 4 | 地址空间、模块信息与用户拷贝 | `04_Day4_辅助信息与地址空间` |
| 5 | 主机、交叉编译与目标板流程 | `05_Day5_开发板上的第一个驱动` |
| 6 | 分离思想与 device/driver/bus | `06_Day6_分离思想与驱动模型入门` |
| 7 | 匹配、probe/remove 与第一阶段复盘 | `07_Day7_总线设备驱动模型深化` |
| 8 | 设备树、硬件描述与驱动匹配 | `08_Day8_设备树入门` |
| 9 | pinctrl、GPIO 与板级资源 | `09_Day9_Pinctrl和GPIO` |
| 10 | 阻塞、非阻塞、poll 与控制接口 | `10_Day10_APP与驱动更多交互方式` |
| 11 | 综合机制、上下文与资源退出 | `11_Day11_总结驱动机制` |
| 12 | SR501 数据流与版本对比 | `12_Day12_进入第一个实战专题` |
| 13 | I2C、AT24C02 与总线分层 | `13_Day13_总线或子系统专题` |
| 14 | SPI / 显示预览与总复盘 | `14_Day14_SPI与Framebuffer预览及总复盘` |

优先参考的规划文档：

```text
02_嵌入式Linux驱动入门直播/StudyPlan/01_学习总路线.md
02_嵌入式Linux驱动入门直播/StudyPlan/02_14天学习计划.md
02_嵌入式Linux驱动入门直播/StudyPlan/03_高优先级资料清单.md
02_嵌入式Linux驱动入门直播/StudyPlan/12_资料阅读顺序总表.md
02_嵌入式Linux驱动入门直播/StudyPlan/20_最终执行建议_先看什么后看什么.md
```

---

## 4. 常见错误记录模板

每份正式学习材料至少包含 5 个常见错误，按下述五段式记录：

```text
错误现象：
错误原因：
如何定位：
正确做法：
记忆方法：
```

---

## 5. 读源码的方法

按以下顺序引导，不要一上来钻进细节 API：

```text
先看目录名
-> 看 README 或讲义
-> 看 Makefile
-> 看编译产物
-> 找入口函数
-> 找核心结构体
-> 找初始化流程
-> 找对外接口
-> 找测试程序
-> 最后看细节 API
```

Linux 驱动源码优先找：

```text
module_init  module_exit  file_operations
open  read  write  ioctl  poll
probe  remove  of_match_table  compatible  platform_driver
```

FreeRTOS 源码优先找：

```text
xTaskCreate  vTaskStartScheduler  vTaskDelay
xQueueCreate  xQueueSend  xQueueReceive
xSemaphoreCreateBinary  xSemaphoreTake  xSemaphoreGive
xEventGroupCreate  xTaskNotify  xTimerCreate
```

---

## 6. 能力分层标准

| 层级 | 目标 | 表现 |
| --- | --- | --- |
| 第 0 层 | 建立地图 | 不清楚应用、驱动、内核关系；`.c`、`.ko`、`.dts`、`Makefile` 混在一起看 |
| 第 1 层 | 能区分角色 | 知道 `printf` 与 `printk` 不同、`main()` 与 `module_init()` 不同、`.ko` 是内核模块产物 |
| 第 2 层 | 看懂简单例子结构 | 会先看 `Makefile`；能区分驱动文件和测试程序；能找出初始化、清理、接口函数 |
| 第 3 层 | 串起多个概念 | 知道 `device / driver / bus` 关系；知道设备树描述硬件；知道 `read/write/ioctl/poll` |
| 第 4 层 | 跟着改代码 | 敢改 `printf` / `printk`；敢改测试程序；知道自己改的是应用层、驱动层还是设备树层 |
| 第 5 层 | 带着问题读专题 | 能判断专题新增了什么机制；能沿设备树、驱动、测试程序三条线读代码 |

---

## 7. 推荐先动手的源码目录

以下路径均在本地资料库中，**未入版本控制**：

```text
02_嵌入式Linux驱动入门直播/01_单片机程序和Linux程序从上层到底层的完整比较/source/01_hello_app
02_嵌入式Linux驱动入门直播/01_单片机程序和Linux程序从上层到底层的完整比较/source/01_hello_drv
02_嵌入式Linux驱动入门直播/02_彻底分析第1个驱动程序/source
02_嵌入式Linux驱动入门直播/04_板子上的第1个驱动程序_实际操作及编译过程/source
02_嵌入式Linux驱动入门直播/07_设备树入门/source
```

初期只做最小修改：改打印内容、增加注释、看懂 Makefile 生成什么、找到模块入口和出口、对照测试程序理解接口、对照设备树理解匹配关系。

---

## 8. 当前进度

本文件不写死当前进度。以 `Linux/daily_logs/` 最新日志和 `Linux/daily_tasks/` 最新任务为准。
