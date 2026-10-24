# Day6：分离思想与 device / driver / bus 模型

**验证状态**：未验证。WSL2 无法构建和加载内核模块，本篇的 `probe`/`remove` 观测结果均为预期值。

## 学习目标

学完本篇应能：

1. 说明"设备"和"驱动"为什么要分离。
2. 画出 `device → bus → driver` 的匹配流程。
3. 指出 `platform_device` 与 `platform_driver` 各自描述什么。
4. 解释 `probe` 与 `remove` 的触发时机。
5. 说明加载顺序为什么不影响匹配结果。

## 环境与前置条件

| 项目 | 要求 |
| --- | --- |
| 内核头文件 | `/lib/modules/$(uname -r)/build` |
| 权限 | root（加载两个模块） |
| 前置 | Day2 的模块生命周期、Day3 的字符设备 |

## 核心概念

### 1. 分离前后的对比

**不分离**（Day3 的写法）：

```text
一个 .ko 里同时包含：
  硬件信息（寄存器地址、引脚）
  操作逻辑（open/read/write）
```

问题：换一块板子就要改驱动源码；同一份逻辑被复制多份。

**分离后**：

```text
device：描述"有什么硬件资源"（地址、中断号、名称）
driver：描述"怎么操作这类硬件"
bus   ：负责把两者按名字或兼容性匹配起来
```

好处：驱动可复用，硬件差异下沉到设备描述。

### 2. 匹配流程

```text
platform_device_register(dev)
          │
          ▼
   platform_bus 维护 device 链表
          ▲
          │ 比较 name / compatible
          ▼
platform_driver_register(drv)
          │
          ▼
   match 成功 → probe(drv, dev)
   match 失败 → 等待，不做任何操作
```

关键点：**先注册 device 还是先注册 driver 都可以**。总线会在任一侧注册时重新尝试匹配。

### 3. `platform_device` 描述什么

```c
static struct platform_device hello_dev = {
	.name = "100ask_hello",          /* 匹配用的名字 */
	.dev = {
		.release = hello_dev_release, /* 必须提供，否则卸载时告警 */
	},
};
```

更完整的设备还会带：

```c
.resource = resources,               /* 寄存器区间、中断 */
.num_resources = ARRAY_SIZE(resources),
.id = -1,
```

### 4. `platform_driver` 描述什么

```c
static struct platform_driver hello_driver = {
	.probe  = hello_probe,
	.remove = hello_remove,
	.driver = {
		.name = "100ask_hello",
	},
};
```

`probe` 在匹配成功时调用，`remove` 在设备移除或驱动注销时调用。

### 5. 两种匹配方式

| 方式 | 依据 | 使用场景 |
| --- | --- | --- |
| 名字匹配 | `device.name` == `driver.driver.name` | 未使用设备树的教学示例 |
| 设备树匹配 | `driver.driver.of_match_table` 中的 `compatible` | 现代平台，Day8 起使用 |

名字匹配是过渡方案，工程实践应使用 `compatible` + 设备树，这样硬件描述与代码彻底分离。

### 6. 版本差异（重要）

| API | 旧内核 | 新内核 |
| --- | --- | --- |
| `platform_driver.remove` | `int (*remove)(struct platform_device *)` | Linux 6.11 起返回 `void` |
| `class_create` | `(owner, name)` | 6.4 起 `(name)` |
| `driver.name` | 主要匹配手段 | 仍可用，但推荐 `of_match_table` |

课程代码面向 Linux 4.9.88，迁移到新内核前必须核对签名。

## 系统调用/接口卡片

### `int platform_device_register(struct platform_device *pdev)`

作用：向 platform 总线注册一个设备。

参数：已填充 `name`、`resource`（可选）、`dev.release` 的设备结构体。

返回值和错误：成功返回 0；失败返回负错误码。

资源所有权：必须调用 `platform_device_unregister()` 释放。

最小示例：

```c
err = platform_device_register(&hello_dev);
if (err)
	return err;
```

常见误用：缺少 `.dev.release`，卸载时内核打印警告；`name` 与驱动不一致；重复注册同名设备。

### `void platform_device_unregister(struct platform_device *pdev)`

作用：注销设备，触发已绑定驱动的 `remove`。

参数：注册时的同一指针。

返回值和错误：无返回值。

最小示例：`platform_device_unregister(&hello_dev);`

常见误用：在驱动仍在使用资源时注销；忘记注销导致模块无法卸载。

### `int platform_driver_register(struct platform_driver *drv)`

作用：向 platform 总线注册驱动，触发已有设备的匹配。

参数：包含 `probe`、`remove`、`driver.name` 的结构体。

返回值和错误：成功返回 0；失败返回负错误码。

最小示例：

```c
err = platform_driver_register(&hello_driver);
if (err)
	return err;
```

常见误用：`probe` 返回非 0 导致设备绑定失败；`remove` 签名与内核版本不匹配。

### `int (*probe)(struct platform_device *pdev)`

作用：匹配成功后的初始化回调。

返回值和错误：返回 0 表示绑定成功；返回负值表示失败，总线会解绑并等待下次匹配。

资源所有权：probe 中申请的资源应在 `remove` 中释放；推荐使用 `devm_*` 系列自动释放。

最小示例：

```c
static int hello_probe(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "probe\n");
	return 0;
}
```

常见误用：probe 失败后不回收已申请资源；在 probe 中做长时间阻塞操作。

### `int platform_get_resource(struct platform_device *pdev, unsigned int type, unsigned int num)`

作用：从设备资源表取寄存器区间或中断号。

参数：资源类型（`IORESOURCE_MEM` / `IORESOURCE_IRQ`）、序号。

返回值和错误：成功返回资源指针；失败返回 `NULL`。

最小示例：

```c
res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
base = devm_ioremap_resource(&pdev->dev, res);
```

常见误用：不检查 `NULL` 直接解引用；索引写错。

## 最小可运行实验

实验目录：`Linux/experiments/Day06_分离思想与platform/`

```text
hello_platform_dev.c   只注册设备
hello_platform_drv.c   只注册驱动
Makefile               同时构建两个模块
README.md              两种加载顺序的验证步骤
```

驱动核心：

```c
static int hello_probe(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "probe matched\n");
	return 0;
}

static struct platform_driver hello_driver = {
	.probe  = hello_probe,
	.remove = hello_remove,
	.driver = {
		.name = "100ask_hello",
	},
};
```

## 编译与运行

```bash
cd Linux/experiments/Day06_分离思想与platform
make

# 顺序一：先设备后驱动
sudo insmod hello_platform_dev.ko
sudo insmod hello_platform_drv.ko
sudo dmesg | tail -10

sudo rmmod hello_platform_drv
sudo rmmod hello_platform_dev

# 顺序二：先驱动后设备
sudo insmod hello_platform_drv.ko
sudo insmod hello_platform_dev.ko
sudo dmesg | tail -10
```

## 结果观察

以下为**预期结果，未在本机实测**：

顺序一：

```text
[  ...] hello_platform_dev: device registered
[  ...] hello_platform_drv: driver registered
[  ...] hello_platform_drv: probe matched
```

顺序二：

```text
[  ...] hello_platform_drv: driver registered
[  ...] hello_platform_dev: device registered
[  ...] hello_platform_drv: probe matched
```

两种顺序的 `probe` 都只执行一次，说明总线在**后注册的一侧**触发匹配。

卸载时先卸载驱动或先卸载设备，`remove` 都会被调用；若先卸载设备，驱动会收到解绑通知。

## 常见错误和陷阱

| 现象 | 原因 | 定位方法 |
| --- | --- | --- |
| 两个模块都加载了但没有 `probe` | `name` 不匹配 | 对比两个结构体中的字符串 |
| 卸载设备时内核告警 | 缺少 `.dev.release` | 补 `platform_device_release` |
| 模块无法卸载 | 忘记 `platform_*_unregister` | 检查 `_exit` 路径 |
| 重复加载报 `File exists` | 同名设备或驱动已注册 | `ls /sys/bus/platform/devices/` |
| `probe` 反复执行 | probe 返回错误，总线重试匹配 | 检查 probe 返回值 |
| 新内核编译报 remove 签名错误 | 6.11 起返回 `void` | 对照内核版本调整 |

## 故障排查方法

1. **看 sysfs**：`ls /sys/bus/platform/devices/ | grep 100ask` 确认设备已注册。
2. **看驱动目录**：`ls /sys/bus/platform/drivers/` 确认驱动已注册。
3. **看 dmesg**：注册、匹配、probe、remove 每一步都应有日志。
4. **核对字符串**：逐字符比较 `device.name` 与 `driver.driver.name`。
5. **确认返回值**：`platform_device_register` 与 `platform_driver_register` 的错误码要打印出来。

## 练习题

### 基础题

题目：把设备名从 `100ask_hello` 改为 `hello_platform`，验证匹配仍然成功。

提示：两处必须同时修改。

验收标准：`dmesg` 中仍出现 `probe matched`。

### 改错题

题目：下面的 `probe` 有什么问题？

```c
static int hello_probe(struct platform_device *pdev)
{
	struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	void __iomem *base = ioremap(res->start, resource_size(res));
	pr_info("base = %p\n", base);
	return 0;
}
```

提示：考虑 `res` 为 `NULL` 与 `ioremap` 失败两种情况。

验收标准：能指出两处未检查的返回值，并给出修正版。

### 综合题

题目：在 `probe` 中读取设备资源（一段 MEM 区间），用 `devm_ioremap_resource` 映射，并在 `remove` 中确认资源自动释放。

提示：`devm_*` 绑定到 `&pdev->dev`，无需手写 `iounmap`。

验收标准：加载后 `/proc/iomem` 可见映射；卸载后无残留；`dmesg` 无警告。

## 验收标准

- [ ] 能画出 `device → bus → driver → probe` 的匹配流程。
- [ ] 能说明两种加载顺序都能匹配成功的原因。
- [ ] 能指出 `platform_device` 与 `platform_driver` 各自的核心字段。
- [ ] 能说出名字匹配与设备树匹配的适用场景。
- [ ] 在具备条件的环境中完成一次 probe/remove 观测（当前环境未完成）。

## 延伸阅读

- `Linux_WEIDS/02_嵌入式Linux驱动入门直播/StudyPlan/DailyDocs/06_Day6_分离思想与驱动模型入门_学习讲解.md`
- `Linux_WEIDS/02_嵌入式Linux驱动入门直播/05_分离的设计思想与总线设备驱动模型/source/01_platform_bus_dev_drv/`
- 下一篇：`Linux/notes/驱动入门/Day07_总线模型深化与复盘.md`

---

**验证记录**

| 项目 | 内容 |
| --- | --- |
| 日期 | 2026-10-25 |
| 环境 | WSL Ubuntu 26.04，内核头文件缺失 |
| 已执行 | 源码阅读：`hello_dev.c`、`hello_drv.c`、`Makefile` |
| 未执行 | 编译、加载、probe/remove 观测 |
| 结论 | 运行结果为预期值，未实测 |
