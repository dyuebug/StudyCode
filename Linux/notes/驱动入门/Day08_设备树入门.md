# Day8：设备树入门与驱动匹配

**验证状态**：已实测（QEMU vexpress-a9 + Linux 7.0.0，2026-10-25）。自定义 dtb 中新增节点，驱动通过 `compatible` 匹配并读到 `pin = <3>`，证据见 `Linux/tests/qemu-vexpress/RESULT.md`。

## 学习目标

学完本篇应能：

1. 说明 `.dts`、`.dtb`、`.dtc` 三者的关系。
2. 写出一个包含 `compatible` 与自定义属性的设备树节点。
3. 用 `of_match_table` 让驱动匹配设备树节点。
4. 用 `of_property_read_string` / `of_property_read_u32` 读取属性。
5. 指出课程示例中 `tmp_str[6] - '0'` 的脆弱之处。

## 环境与前置条件

| 项目 | 要求 |
| --- | --- |
| 目标内核源码树 | 含 `arch/arm/boot/dts/` 与 `scripts/dtc/` |
| 交叉工具链 | 与目标板匹配 |
| 目标板或 QEMU | 能加载自定义 dtb |
| 前置 | Day6/Day7 的 platform 模型 |

## 核心概念

### 1. 三个文件

```text
board.dts   文本源文件，人写
   │ dtc 编译（内核构建系统自动调用）
   ▼
board.dtb   二进制设备树，内核读取
```

`dtc` 是设备树编译器，通常不需要单独调用，`make dtbs` 会自动完成。

### 2. 节点与属性

课程 `1.dts` 中的最小节点：

```dts
/ {
	myled: myled_for_test {
		pin = "gpio5_3";
	};
};
```

| 组成 | 含义 |
| --- | --- |
| `myled:` | 标签，供其他位置用 `&myled` 引用 |
| `myled_for_test` | 节点名 |
| `pin = "gpio5_3"` | 自定义字符串属性 |

驱动要匹配还需要 `compatible`：

```dts
myled: myled_for_test {
	compatible = "100ask,led";
	pin = "gpio5_3";
};
```

### 3. `compatible` 的作用

`compatible` 是"这个设备兼容哪个驱动"的声明。驱动侧：

```c
static const struct of_device_id dts_device_ids[] = {
	{ .compatible = "100ask,led" },
	{ /* sentinel */ }
};

static struct platform_driver led_driver = {
	.probe  = led_probe,
	.remove = led_remove,
	.driver = {
		.name = "100ask_led",
		.of_match_table = dts_device_ids,
	},
};
```

匹配成功后，内核为节点创建设备，调用 `probe`。

### 4. 从节点读属性

```c
const char *pin_str;

if (of_property_read_string(pdev->dev.of_node, "pin", &pin_str))
	return -EINVAL;
```

整型属性用：

```c
u32 pin;

if (of_property_read_u32(pdev->dev.of_node, "pin", &pin))
	return -EINVAL;
```

读取失败返回负错误码，必须检查。

### 5. 课程示例的脆弱点

```c
leds_desc[minor].pin = tmp_str[6] - '0';
```

它假设字符串固定为 `"gpio5_3"` 且引脚号正好在索引 6。

问题：

- 换成 `"gpio12_3"` 立刻错误；
- 无法处理两位数引脚；
- 无法表达 GPIO 控制器编号。

工程做法：`pin` 直接写成整型，或使用 `gpios = <&gpio5 3 GPIO_ACTIVE_LOW>;` 由 gpiolib 解析（Day9）。

### 6. 运行期查看设备树

```bash
ls /proc/device-tree/
cat /proc/device-tree/myled_for_test/compatible
hexdump -C /proc/device-tree/myled_for_test/pin
```

`/proc/device-tree` 是内核解析后的只读视图，可直接确认节点是否被正确加载。

## 系统调用/接口卡片

### `int of_property_read_string(const struct device_node *np, const char *propname, const char **out_string)`

作用：读取字符串属性。

返回值和错误：成功 0；属性不存在或类型不符返回负错误码。

最小示例：`of_property_read_string(np, "pin", &s);`

常见误用：不检查返回值；把返回的指针当可写内存。

### `int of_property_read_u32(const struct device_node *np, const char *propname, u32 *out_value)`

作用：读取 32 位整型属性。

返回值和错误：同上。

最小示例：`of_property_read_u32(np, "pin", &pin);`

常见误用：dts 中写成字符串却在驱动里按整型读。

### `const struct of_device_id *of_match_device(const struct of_device_id *matches, const struct device *dev)`

作用：手动查找匹配项，用于取得 `.data` 中携带的配置。

最小示例：

```c
const struct of_device_id *m = of_match_device(dts_device_ids, &pdev->dev);
if (m) cfg = m->data;
```

常见误用：在已经匹配成功的 probe 中重复匹配并忽略 `NULL`。

## 最小可运行实验

实验目录：`Linux/experiments/Day08_设备树匹配/`

```text
led_dts.dts      设备树节点片段
led_dts_drv.c    使用 of_match_table 的驱动
Makefile
README.md        如何合入内核树、编译 dtb、加载模块
```

关键代码：

```c
static int led_probe(struct platform_device *pdev)
{
	u32 pin;

	if (of_property_read_u32(pdev->dev.of_node, "pin", &pin))
		return -EINVAL;

	dev_info(&pdev->dev, "probe: pin=%u\n", pin);
	return 0;
}
```

## 编译与运行

1. 把 `led_dts.dts` 中的节点合入目标板 dts：

```dts
/ {
	myled: myled_for_test {
		compatible = "100ask,led";
		pin = <3>;
	};
};
```

2. 编译 dtb：

```bash
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- dtbs
```

3. 编译模块并部署：

```bash
cd Linux/experiments/Day08_设备树匹配
make KERN_DIR=~/100ask_imx6ull-sdk/Linux-4.9.88
```

4. 目标板启动新 dtb 后：

```bash
insmod led_dts_drv.ko
cat /proc/device-tree/myled_for_test/compatible
dmesg | tail -5
```

## 结果观察

以下为**预期结果，未在本机实测**：

```text
$ cat /proc/device-tree/myled_for_test/compatible
100ask,led

$ dmesg | tail -3
led_dts: probe: pin=3
```

若 `probe` 未执行，先确认 `compatible` 字符串完全一致（含逗号）。

## 常见错误和陷阱

| 现象 | 原因 | 定位方法 |
| --- | --- | --- |
| `probe` 不执行 | `compatible` 不一致 | 对比 dts 与 `of_device_id` |
| 属性读取返回错误 | 类型不符（字符串 vs 整型） | `hexdump` 看 `/proc/device-tree` |
| dtb 未生效 | 板子仍用旧 dtb | 确认启动时加载的文件 |
| `make dtbs` 失败 | 未设置 `ARCH` | 补环境变量 |
| 引脚号错误 | 字符串下标解析 | 改用整型属性 |
| 节点被覆盖 | 同名节点重复定义 | 查看 `dtc` 警告 |

## 故障排查方法

1. **确认 dtb 已加载**：`cat /proc/device-tree/model`。
2. **确认节点存在**：`ls /proc/device-tree/`。
3. **确认属性类型**：`hexdump -C` 查看原始字节。
4. **确认匹配字符串**：逐字符核对，注意逗号与大小写。
5. **确认驱动已注册**：`ls /sys/bus/platform/drivers/`。

## 练习题

### 基础题

题目：给节点增加整型属性 `default-state = <1>;`，驱动 probe 时读取并打印。

提示：`of_property_read_u32`。

验收标准：`dmesg` 输出 `default-state=1`。

### 改错题

题目：下面的代码有什么问题？

```c
static int led_probe(struct platform_device *pdev)
{
	const char *s;
	of_property_read_string(pdev->dev.of_node, "pin", &s);
	u32 pin = s[6] - '0';
	dev_info(&pdev->dev, "pin=%u\n", pin);
	return 0;
}
```

提示：考虑读取失败与字符串长度变化。

验收标准：能指出未检查返回值、下标假设脆弱两点。

### 综合题

题目：为同一驱动支持两种硬件：`compatible = "100ask,led-a"` 与 `"100ask,led-b"`，通过 `of_device_id.data` 携带不同配置。

提示：`{ .compatible = "100ask,led-a", .data = &cfg_a }`。

验收标准：两种节点分别 probe 并打印各自配置。

## 验收标准

- [ ] 能说明 dts、dtb、dtc 的关系。
- [ ] 能写出带 `compatible` 与自定义属性的节点。
- [ ] 能配置 `of_match_table` 并解释匹配过程。
- [ ] 能正确读取字符串与整型属性并检查返回值。
- [ ] 在目标环境完成一次 dtb 加载与 probe 验证（当前环境未完成）。

## 延伸阅读

- `Linux_WEIDS/02_嵌入式Linux驱动入门直播/StudyPlan/DailyDocs/08_Day8_设备树入门_学习讲解.md`
- `Linux_WEIDS/02_嵌入式Linux驱动入门直播/07_设备树入门/source/07_platform_driver_for_dts/`
- 下一篇：`Linux/notes/驱动入门/Day09_pinctrl与GPIO.md`

---

**验证记录**

| 项目 | 内容 |
| --- | --- |
| 日期 | 2026-10-25 |
| 环境 | QEMU vexpress-a9 + Linux 7.0.0，自定义 dtb |
| 已执行 | 追加 `myled_for_test` 节点、cpp+dtc 编译 dtb、加载驱动 |
| 实测输出 | `led_dts myled_for_test: probe: pin=3`；`/sys/bus/platform/devices/myled_for_test` 存在 |
| 未执行 | 真实板卡 |
| 结论 | 设备树匹配与属性读取已实测通过 |
