# Day5：主机、交叉编译与目标板流程

**验证状态**：未验证。本篇涉及 ARM 交叉编译与真实板卡硬件（i.MX6ULL GPIO），当前环境无交叉工具链、无目标板，所有运行结果均为预期值。

## 学习目标

学完本篇应能：

1. 说清"主机（host）"与"目标机（target）"的分工。
2. 独立写出一个交叉编译内核模块的 Makefile。
3. 说明 `ARCH`、`CROSS_COMPILE`、`KERN_DIR` 三个变量的作用。
4. 解释板级驱动中 `ioremap`、寄存器写、`class_create`、`device_create` 各自解决什么问题。
5. 描述"编译 → 传输 → 加载 → 验证"的完整部署链。

## 环境与前置条件

| 项目 | 要求 |
| --- | --- |
| 交叉工具链 | 例如 `arm-linux-gnueabihf-gcc`（Cortex-A7 32 位） |
| 已编译的内核树 | `KERN_DIR` 指向目标板内核源码，且已 `make modules_prepare` |
| 目标板或 QEMU | 100ask_imx6ull 开发板或 `100ask_imx6ull-qemu` |
| 传输方式 | NFS 挂载、tftp、scp 或 SD 卡 |

本机现状：WSL 无交叉工具链、无目标内核树、无 QEMU 镜像启动配置。

## 核心概念

### 1. 主机与目标机

```text
主机（x86_64 Ubuntu / WSL）
  ├─ 交叉编译器   生成 ARM 指令
  ├─ 目标内核树   提供头文件与 Module.symvers
  └─ 传输通道     NFS / tftp / scp

目标机（i.MX6ULL / STM32MP157 / QEMU）
  ├─ 运行目标内核
  ├─ insmod 加载 .ko
  └─ 访问真实寄存器与外设
```

关键点：**在主机上编译，在目标机上运行**。`.ko` 的架构与 `vermagic` 必须与目标内核匹配。

### 2. 三个关键变量

| 变量 | 作用 | 示例 |
| --- | --- | --- |
| `ARCH` | 告诉内核构建系统目标架构 | `arm`、`arm64` |
| `CROSS_COMPILE` | 工具链前缀 | `arm-linux-gnueabihf-` |
| `KERN_DIR` | 目标板内核源码目录 | `~/100ask_imx6ull-sdk/Linux-4.9.88` |

三者必须同时正确。只改一个会导致"编译通过但无法加载"或"根本编译不过"。

### 3. 为什么需要目标内核树

模块编译依赖：

```text
$(KERN_DIR)/Makefile          kbuild 入口
$(KERN_DIR)/include/          内核头文件
$(KERN_DIR)/Module.symvers    已导出符号
$(KERN_DIR)/scripts/          构建脚本
```

因此目标内核必须先配置并编译（至少 `make modules_prepare`），否则 `Module.symvers` 缺失，模块会出现 `Unknown symbol`。

### 4. 板级驱动的三段工作

以 i.MX6ULL LED 为例：

```text
1. 引脚复用：IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3 设为 GPIO 模式
2. 方向配置：GPIO5_GDIR 对应位设为输出
3. 电平输出：GPIO5_DR 对应位写 0/1
```

驱动里用 `ioremap()` 把物理地址映射为内核虚拟地址后才能访问：

```c
GPIO5_DR = ioremap(0x020AC000, 4);
*GPIO5_DR |= (1 << 3);
```

### 5. 设备节点自动创建

课程示例在 `led_init` 中调用：

```c
led_class = class_create(THIS_MODULE, "myled");
device_create(led_class, NULL, MKDEV(major, 0), NULL, "myled");
```

这让 udev/mdev 自动创建 `/dev/myled`，不再需要手工 `mknod`。

**版本差异（重要）**：Linux 6.4 起 `class_create()` 去掉了 `owner` 参数，签名为 `class_create(const char *name)`。课程代码面向 Linux 4.9.88，直接拿到新内核编译会报参数数量错误。

### 6. 直接写寄存器 vs gpiolib

课程用裸寄存器操作，目的是看清硬件本质。工程实践应使用：

```text
gpiod_get / gpiod_set_value / gpiod_direction_output
```

好处：跨平台、可读性好、与设备树配合。Day6 之后会逐步过渡到 `platform_driver` + 设备树。

## 系统调用/接口卡片

### `void __iomem *ioremap(phys_addr_t offset, size_t size)`

作用：把物理地址映射为内核可访问的虚拟地址。

参数：物理地址与映射长度。

返回值和错误：成功返回虚拟地址；失败返回 `NULL`。

资源所有权：必须用 `iounmap()` 释放。

线程/进程安全：映射本身在初始化时完成；映射后的寄存器访问需要注意并发与位操作原子性。

最小示例：

```c
GPIO5_DR = ioremap(0x020AC000, 4);
if (!GPIO5_DR)
	return -ENOMEM;
```

常见误用：忘记 `iounmap`；映射长度小于实际访问范围；在 `probe` 失败路径上泄漏映射。

### `struct class *class_create(...)`

作用：创建设备类，供 udev/mdev 生成设备节点。

参数：内核 6.4 之前为 `(owner, name)`，之后为 `(name)`。

返回值和错误：返回 `class` 指针或 `ERR_PTR()`，需用 `IS_ERR()` 判断。

资源所有权：必须 `class_destroy()`。

最小示例（6.4+）：`led_class = class_create("myled");`

常见误用：不判断 `IS_ERR`；忘记 `class_destroy`；沿用旧签名导致编译失败。

### `struct device *device_create(...)`

作用：在类下创建设备，触发 `/dev/<name>` 节点生成。

参数：类、父设备、设备号、驱动数据、设备名。

资源所有权：必须 `device_destroy()`。

最小示例：`device_create(led_class, NULL, MKDEV(major, 0), NULL, "myled");`

常见误用：设备号与注册的 `major` 不一致；忘记销毁导致节点残留。

### 交叉编译命令

```bash
export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-
make -C $(KERN_DIR) M=$(pwd) modules
```

验证产物架构：

```bash
file led_drv.ko
modinfo led_drv.ko | grep vermagic
```

## 最小可运行实验

实验目录：`Linux/experiments/Day05_交叉编译与板级驱动/`

```text
led_drv.c     板级 LED 驱动（i.MX6ULL，直接操作寄存器）
ledtest.c     用户态测试：向 /dev/myled 写 0/1
Makefile      带 ARCH / CROSS_COMPILE / KERN_DIR 的交叉编译规则
README.md     工具链、内核树、部署与验证步骤
```

说明：该实验**必须**在具备 i.MX6ULL 目标环境的条件下运行；在其他板卡上寄存器地址不同，不能直接使用。

## 编译与运行

在主机上：

```bash
cd Linux/experiments/Day05_交叉编译与板级驱动

export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-
export KERN_DIR=~/100ask_imx6ull-sdk/Linux-4.9.88

make
file led_drv.ko
modinfo led_drv.ko | grep vermagic
```

传输到目标板（任选其一）：

```bash
# NFS
cp led_drv.ko ledtest /home/book/nfs_rootfs/

# scp
scp led_drv.ko ledtest root@<board-ip>:/root/
```

在目标板上：

```bash
insmod led_drv.ko
ls /dev/myled
./ledtest on
./ledtest off
dmesg | tail -10
rmmod led_drv
```

## 结果观察

以下为**预期结果，未在本机实测**：

```text
$ file led_drv.ko
led_drv.ko: ELF 32-bit LSB relocatable, ARM, EABI5 version 1 (SYSV)

$ modinfo led_drv.ko | grep vermagic
vermagic: 4.9.88 SMP preempt mod_unload modversions ARMv7 p2v8

$ ls /dev/myled
/dev/myled
```

LED 现象：`./ledtest on` 后板载 LED 点亮，`off` 后熄灭。没有板卡时无法确认。

## 常见错误和陷阱

| 现象 | 原因 | 定位方法 |
| --- | --- | --- |
| `arm-linux-gnueabihf-gcc: not found` | 工具链未安装或不在 `PATH` | `which $(CROSS_COMPILE)gcc` |
| 编译出 x86 模块 | `ARCH` / `CROSS_COMPILE` 未设置 | `file xxx.ko` 查看架构 |
| `No such file or directory: Module.symvers` | 目标内核未编译 | 在 `KERN_DIR` 执行 `make modules_prepare` |
| `insmod: Invalid module format` | vermagic 与目标内核不匹配 | 对比 `modinfo` 与 `uname -r` |
| `class_create` 参数数量不匹配 | 内核 ≥6.4 签名变更 | 检查内核版本与调用形式 |
| `ioremap` 返回 `NULL` 后仍写寄存器 | 未检查返回值 | 增加 `if (!ptr) return -ENOMEM;` |
| LED 无反应 | 引脚复用或 GPIO 编号错误 | 对照板卡原理图与手册 |
| 卸载后 `/dev/myled` 残留 | 未调用 `device_destroy` | 检查 `_exit` 路径 |

## 故障排查方法

1. **先验证工具链**：`$(CROSS_COMPILE)gcc -v` 与 `file xxx.ko`。
2. **再验证内核树**：`ls $(KERN_DIR)/Module.symvers`。
3. **再验证 vermagic**：`modinfo` 与目标 `uname -r` 必须一致。
4. **最后看硬件**：原理图确认引脚、复用模式、GPIO 编号。
5. **失败路径检查**：`ioremap`、`class_create`、`device_create` 的返回值都要判断。

## 练习题

### 基础题

题目：把实验 Makefile 改为通过环境变量传入 `ARCH`、`CROSS_COMPILE`、`KERN_DIR`，不修改文件即可切换目标板。

提示：使用 `?=` 让环境变量优先；未设置时再给默认值。

验收标准：同一份 Makefile 能分别为 i.MX6ULL 与 STM32MP157 构建。

### 改错题

题目：下面的 Makefile 片段有什么问题？

```make
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(pwd) modules
```

提示：这段代码在主机上编译，产物是什么架构？

验收标准：能指出它生成的是主机架构模块，无法在 ARM 目标板加载，并给出交叉编译版本。

### 综合题

题目：把直接写寄存器改为 `gpiod` 接口：从设备树获取 GPIO，用 `gpiod_direction_output` / `gpiod_set_value` 控制 LED。

提示：需要设备树节点与 `of_get_named_gpio` 或 `devm_gpiod_get`；先读目标内核版本中可用的 API。

验收标准：功能与裸寄存器版一致，且代码不再硬编码物理地址。

## 验收标准

- [ ] 能解释主机与目标机的分工，以及为什么必须交叉编译。
- [ ] 能独立配置 `ARCH` / `CROSS_COMPILE` / `KERN_DIR` 并构建出 ARM 模块。
- [ ] 能用 `file` 与 `modinfo` 验证产物架构和 vermagic。
- [ ] 能说明 `ioremap`、`class_create`、`device_create` 的作用与释放路径。
- [ ] 在目标板或 QEMU 上完成一次加载与现象验证（当前环境未完成）。

## 延伸阅读

- `Linux_WEIDS/02_嵌入式Linux驱动入门直播/StudyPlan/DailyDocs/05_Day5_开发板上的第一个驱动_学习讲解.md`
- `Linux_WEIDS/02_嵌入式Linux驱动入门直播/04_板子上的第1个驱动程序_实际操作及编译过程/source/04_driver_for_board/02_led_drv/00_led_drv_simple/`
- `Linux_WEIDS/01_全系列新Linux系列教程之快速入门视频/03_开发板使用手册/`
- 下一篇：`Linux/notes/驱动入门/Day06_分离思想与platform模型.md`

---

**验证记录**

| 项目 | 内容 |
| --- | --- |
| 日期 | 2026-10-25 |
| 环境 | WSL Ubuntu 26.04；无 ARM 交叉工具链、无目标内核树、无目标板 |
| 已执行 | 源码阅读：`imx6ull/led_drv.c`、`Makefile` |
| 未执行 | 交叉编译、传输、`insmod`、LED 现象验证 |
| 结论 | 全部运行结果为预期值，未实测 |
