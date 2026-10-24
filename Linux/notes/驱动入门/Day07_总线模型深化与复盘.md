# Day7：总线设备驱动模型深化与第一阶段复盘

**验证状态**：未验证。WSL2 无法构建与加载内核模块，本文的运行结果均为预期值。

## 学习目标

学完本篇应能：

1. 说明 `platform_device_id` 与 `of_device_id` 两种匹配表的分工。
2. 解释一个驱动如何服务多个设备实例，以及次设备号如何参与区分。
3. 说明 `platform_set_drvdata()` / `platform_get_drvdata()` 的用途。
4. 指出课程用 `IORESOURCE_IRQ` 传递引脚号的工程问题。
5. 完成 Day1–Day7 的能力自评。

## 环境与前置条件

| 项目 | 要求 |
| --- | --- |
| 前置 | Day6 的 platform 匹配与 probe/remove |
| 内核头文件 | `/lib/modules/$(uname -r)/build` |
| 权限 | root |

## 核心概念

### 1. 两种匹配表

| 表 | 类型 | 依据 | 适用 |
| --- | --- | --- | --- |
| `id_table` | `struct platform_device_id[]` | `device.name` | 传统 platform 设备 |
| `of_match_table` | `struct of_device_id[]` | `compatible` 字符串 | 设备树 |

两者可以同时存在。内核优先用 `of_match_table` 匹配设备树节点，未命中时回退到 `id_table`。

### 2. 一个驱动服务多个设备

课程 `led_dev.c` / `led_dev2.c` / `led_dev3.c` 注册了三个不同名字的设备，驱动用 `id_table` 分别匹配：

```c
static const struct platform_device_id led_id_table[] = {
	{"100ask_led",   1},
	{"100ask_led_3", 2},
	{"100ask_led_4", 3},
	{ },
};
```

每匹配成功一次，`probe` 执行一次，并分配一个次设备号：

```text
minor 0 -> /dev/100ask_led0
minor 1 -> /dev/100ask_led1
```

### 3. 私有数据的保存与取回

```c
platform_set_drvdata(pdev, &leds_desc[minor]);   /* probe 中保存 */
struct led_desc *led = platform_get_drvdata(pdev); /* remove 中取回 */
```

作用：把"这个设备实例对应的数据"挂在 `platform_device` 上，避免全局查找。

资源所有权：`leds_desc` 是驱动内的静态数组，`remove` 中不释放它，但要销毁对应的设备节点。

### 4. 课程用 IRQ 资源传引脚号的工程问题

课程 `led_dev3.c`：

```c
static struct resource resources[] = {
	{ .start = (3 << 8) | (3), .flags = IORESOURCE_IRQ },
};
```

用中断资源槽位传递"引脚号"，probe 里用 `res->start` 读取。

问题：

- 语义错位：`IORESOURCE_IRQ` 表示中断，不是引脚。
- 可读性差：`(3<<8)|3` 需要额外解释。
- 不可扩展：无法表达多个引脚、有效电平、复用配置。

工程做法：传统平台用 `platform_data`，现代平台用设备树属性（Day8）。

### 5. 第一阶段复盘（Day1–Day7）

| 能力 | 自评标准 |
| --- | --- |
| 文件身份 | 能区分应用、模块、Makefile、设备树 |
| 模块生命周期 | 能独立完成 `make → insmod → dmesg → rmmod` |
| 字符设备 | 能解释 major/minor 与 `file_operations` |
| 用户拷贝 | 能写出带返回值检查和长度限制的 `copy_*_user` |
| 交叉编译 | 能配好 `ARCH` / `CROSS_COMPILE` / `KERN_DIR` |
| 分离思想 | 能画出 device → bus → driver → probe |
| 多实例 | 能说明次设备号与 `drvdata` 的作用 |

任一项无法讲清，回到对应 Day 的材料重做实验。

## 系统调用/接口卡片

### `void platform_set_drvdata(struct platform_device *pdev, void *data)`

作用：把驱动私有数据挂到设备上。

返回值和错误：无返回值。

资源所有权：数据本身由驱动管理；设备注销时不自动释放。

最小示例：`platform_set_drvdata(pdev, desc);`

常见误用：在 `remove` 中释放了静态数组；多个设备共用同一份可变数据。

### `void *platform_get_drvdata(const struct platform_device *pdev)`

作用：取回 `platform_set_drvdata` 保存的指针。

返回值和错误：未设置时返回 `NULL`。

最小示例：`struct led_desc *d = platform_get_drvdata(pdev);`

常见误用：不检查 `NULL` 直接解引用。

### `struct resource *platform_get_resource(struct platform_device *pdev, unsigned int type, unsigned int num)`

作用：按类型和序号取资源。

返回值和错误：不存在时返回 `NULL`。

常见误用：把 IRQ 资源当内存资源；索引越界。

### `int device_create(...)` / `void device_destroy(...)`

作用：创建/销毁 `/dev` 节点。

最小示例：

```c
device_create(led_class, NULL, MKDEV(major, minor), NULL, "100ask_led%d", minor);
device_destroy(led_class, MKDEV(major, minor));
```

常见误用：`minor` 与 `remove` 中取回的值不一致，导致节点残留。

## 最小可运行实验

实验目录：`Linux/experiments/Day07_多设备与drvdata/`

```text
led_platform_dev.c    注册两个设备，用 platform_data 传引脚
led_platform_drv.c    一个驱动服务两个实例，按次设备号区分
led_platform_test.c   向 /dev/led0、/dev/led1 写 1/0
Makefile
README.md
```

核心：

```c
struct led_pdata { int pin; };

static int led_probe(struct platform_device *pdev)
{
	struct led_pdata *pdata = dev_get_platdata(&pdev->dev);
	int minor = pdev->id;
	...
	device_create(led_class, NULL, MKDEV(major, minor), NULL, "led%d", minor);
	platform_set_drvdata(pdev, desc);
	return 0;
}
```

## 编译与运行

```bash
cd Linux/experiments/Day07_多设备与drvdata
make

sudo insmod led_platform_dev.ko
sudo insmod led_platform_drv.ko
ls /dev/led*
sudo dmesg | tail -10

./led_platform_test 0 on
./led_platform_test 1 off

sudo rmmod led_platform_drv
sudo rmmod led_platform_dev
```

## 结果观察

以下为**预期结果，未在本机实测**：

```text
$ ls /dev/led*
/dev/led0  /dev/led1

$ ./led_platform_test 0 on
led0 -> on (pin 3)

$ sudo dmesg | tail -6
led_platform: probe pdev->id=0 pin=3
led_platform: probe pdev->id=1 pin=5
```

## 常见错误和陷阱

| 现象 | 原因 | 定位方法 |
| --- | --- | --- |
| 只创建了一个 `/dev/led*` | `pdev->id` 都是 -1 | 检查设备定义中的 `.id` |
| `probe` 中 `pdata` 为 `NULL` | 设备未填 `.dev.platform_data` | 对照设备结构体 |
| 第二个实例覆盖第一个 | 用了全局单例而不是数组 | 检查 `leds_desc` 索引 |
| 卸载后节点残留 | `remove` 中 minor 取错 | 打印 `drvdata` 内容 |
| 两个设备名字相同 | 注册时冲突 | `ls /sys/bus/platform/devices/` |

## 故障排查方法

1. `ls /sys/bus/platform/devices/ | grep led` 确认设备数量。
2. `ls /sys/bus/platform/drivers/<name>/` 看绑定关系。
3. `dmesg` 中确认 `probe` 执行次数与 `pdev->id`。
4. `ls /dev/led*` 确认节点数量与次设备号。

## 练习题

### 基础题

题目：把设备数量扩展到 3 个，验证 `/dev/led0`～`/dev/led2` 都能创建。

提示：设备数组 + 循环注册。

验收标准：`ls /dev/led*` 显示三个节点，`dmesg` 有三次 probe。

### 改错题

题目：下面的 `remove` 有什么问题？

```c
static int led_remove(struct platform_device *pdev)
{
	struct led_desc *d = platform_get_drvdata(pdev);
	device_destroy(led_class, MKDEV(major, 0));
	return 0;
}
```

提示：多实例场景下硬编码次设备号。

验收标准：能指出节点无法对应销毁的问题，并改为使用 `d->minor`。

### 综合题

题目：把 `platform_data` 改为从设备树读取引脚（`of_property_read_u32`），并同时保留传统设备路径。

提示：`pdev->dev.of_node` 非空时走设备树分支。

验收标准：两种设备都能 probe 成功并正确记录引脚。

## 验收标准

- [ ] 能说明 `id_table` 与 `of_match_table` 的匹配优先级。
- [ ] 能解释一次 probe 对应一个设备实例，以及次设备号的作用。
- [ ] 能正确使用 `platform_set_drvdata` / `platform_get_drvdata`。
- [ ] 能指出用 IRQ 资源传引脚号的三个问题。
- [ ] 完成 Day1–Day7 能力自评，并列出需要重做的实验。

## 延伸阅读

- `Linux_WEIDS/02_嵌入式Linux驱动入门直播/StudyPlan/DailyDocs/07_Day7_总线设备驱动模型深化_学习讲解.md`
- `Linux_WEIDS/02_嵌入式Linux驱动入门直播/06_总线设备驱动模型彻底分析/source/06_bus_dev_drv_detail/`
- 下一篇：`Linux/notes/驱动入门/Day08_设备树入门.md`

---

**验证记录**

| 项目 | 内容 |
| --- | --- |
| 日期 | 2026-10-25 |
| 已执行 | 源码阅读：`led_drv.c`、`led_dev3.c` 完整 probe/remove |
| 未执行 | 编译、加载、多实例验证 |
| 结论 | 运行结果为预期值，未实测 |
