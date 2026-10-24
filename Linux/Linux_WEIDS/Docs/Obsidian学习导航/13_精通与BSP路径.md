# 精通与 BSP 路径：平台启动、系统交付与异构多核

[[00_学习主页|返回学习主页]] · [[10_资料库总索引|资料库总索引]] · [[12_进阶驱动路径|前置：进阶驱动]] · [[15_源码与构建入口索引|源码与构建入口]] · [[14_航空背景项目映射|航空项目映射]]

> [!goal] 本阶段目标
> 从“能读、改一个单独驱动”升级到“理解板级硬件描述、镜像布局、启动链、用户态服务与协处理器固件如何共同构成系统”。

## 进入条件

- [ ] 已完成一个字符设备或总线设备驱动闭环。
- [ ] 能解释 `compatible`、`probe/remove`、GPIO/I2C/SPI 资源获取。
- [ ] 能使用 `dmesg`、串口日志或用户态测试程序定位基础问题。
- [ ] 有清晰的 Linux 内核版本、工具链和目标板/QEMU 环境记录。

## 模块 A：i.MX6ULL / QEMU 与板级驱动

### 为什么先走 QEMU

无实体板时，QEMU 可以先建立：

```text
Ubuntu 主机
-> 交叉编译
-> 目标 Linux 内核树
-> 外部模块 .ko
-> 模拟目标板
-> insmod / dmesg / rmmod
```

QEMU 不能完全替代真实硬件时序、电气问题和所有外设，但适合学习模块构建、加载、日志与最小驱动闭环。

### 资料入口

- `01_全系列新Linux系列教程之快速入门视频/03_开发板使用手册/100ASK_IMX6ULL-QEMU使用及开发教程_高级用户使用手册.pdf`
- `01_全系列新Linux系列教程之快速入门视频/05_嵌入式Linux驱动开发基础知识/source/`
- `02_嵌入式Linux驱动入门直播/04_板子上的第1个驱动程序_实际操作及编译过程/source/`
- [[15_源码与构建入口索引|QEMU 板级代码和 Kbuild 入口]]

### 学习主题

- ARM 交叉工具链、`ARCH`、`CROSS_COMPILE`、`KERN_DIR`。
- 内核配置、已编译内核树与外部模块的版本关系。
- GPIO、pinctrl、时钟、设备树资源的板级绑定。
- 从直接寄存器映射理解硬件，再迁移到内核资源管理 API。

## 模块 B：镜像、分区、RootFS 与系统交付

资料：

- `01_全系列新Linux系列教程之快速入门视频/06_实战项目/01_电子产品量产工具/预习资料/tools/emmc_genimage.cfg`
- `01_全系列新Linux系列教程之快速入门视频/06_实战项目/01_电子产品量产工具/预习资料/tools/sdcard_genimage.cfg`
- `01_全系列新Linux系列教程之快速入门视频/06_实战项目/01_电子产品量产工具/预习资料/tools/genimage-10.tar.xz`

应理解的不是工具参数本身，而是系统交付结构：

```text
启动引导区（U-Boot 镜像）
-> boot 分区（kernel / DTB）
-> rootfs 分区
-> 应用、配置、日志与升级策略
```

> [!warning] 资料边界
> 此资料库存在 `genimage` 配置和镜像材料，但没有足够证据表明它是一个完整、可在目录顶层直接构建的 Buildroot 或 Yocto 工程。学习时把它作为镜像布局资料，不要误判为完整发行版源码树。

## 模块 C：IMX6ULL 裸机与启动底层

资料：

- `01_全系列新Linux系列教程之快速入门视频/10_裸机开发/01_100ASK_IMX6ULL裸机程序/README.md`
- `01_全系列新Linux系列教程之快速入门视频/10_裸机开发/01_100ASK_IMX6ULL裸机程序/`

学习目标：

- 复位后最早执行的代码、向量表、时钟和内存初始化。
- 镜像、加载地址与跳转的基本关系。
- MCU 启动和 Linux SoC 启动的共同点与差异。

不要把这一部分当作 Linux 驱动前置硬门槛；它是理解 BSP 和启动链的深化主题。

## 模块 D：STM32MP157 A7 + M4 异构系统

### 系统分工

```text
Cortex-A7 + Linux
    设备管理、文件系统、网络、配置、日志、用户态服务

Cortex-M4 + FreeRTOS / 裸机
    外设实时控制、精确定时、快速事件处理

RemoteProc + OpenAMP / RPMsg
    固件加载、生命周期控制、共享内存与核间消息
```

### 资料入口

- `01_全系列新Linux系列教程之快速入门视频/11_STM32MP157_M4专题/`
- `01_全系列新Linux系列教程之快速入门视频/11_STM32MP157_M4专题/source/01_LED/LED/CM4/RemoteProc/fw_cortex_m4.sh`
- `01_全系列新Linux系列教程之快速入门视频/11_STM32MP157_M4专题/source/22_A7_M4_UserModeComm/rpmsg_user/CM4/Core/Src/main.c`
- `01_全系列新Linux系列教程之快速入门视频/11_STM32MP157_M4专题/source/22_A7_M4_UserModeComm/rpmsg_user/CM4/OPENAMP/openamp.c`

### 必须读懂的启动路径

```text
Linux RemoteProc sysfs
-> /lib/firmware 中的 M4 ELF
-> remoteproc state = start
-> M4 启动
-> resource table
-> shared memory / virtio vring
-> OpenAMP / RPMsg endpoint
-> A7 与 M4 交换消息
```

其中 `fw_cortex_m4.sh` 直接向 `/sys/class/remoteproc/remoteproc0/firmware` 和 `state` 写入固件名、`start` 或 `stop`，是理解 Linux 如何管理协处理器固件的最小实例。

### 三层设备树意识

STM32MP157 专题中可看到分别面向：

```text
TF-A
U-Boot
Linux kernel
```

的设备树材料。它说明一个板级硬件描述可能在不同启动阶段被不同软件组件使用；但本资料库主要保存的是示例和适配片段，不是完整 TF-A/U-Boot/Linux 源码仓库。

## 精通阶段的产出标准

- 一份外部模块构建环境清单：主机、工具链、内核、目标系统、命令。
- 一份设备树资源到驱动 `probe()` 的追踪图。
- 一份 `genimage` 分区布局说明。
- 一份 RemoteProc / RPMsg 消息和资源生命周期说明。
- 一个独立的小系统：Linux 负责配置和日志，RTOS/MCU 负责实时采集或控制。

下一步项目设计见 [[14_航空背景项目映射|航空背景项目映射]]。
