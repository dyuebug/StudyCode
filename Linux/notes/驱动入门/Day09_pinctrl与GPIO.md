# Day9：pinctrl 与 GPIO 子系统

**验证状态**：未验证。需要目标内核树、设备树与板卡，当前环境不具备。

## 学习目标

学完本篇应能：

1. 说明 pinctrl 解决了什么问题。
2. 区分 pinmux（引脚复用）与 pinconf（电气配置）。
3. 写出设备树中 `pinctrl-0` 与 `gpios` 的正确用法。
4. 用 `devm_gpiod_get` / `gpiod_set_value` 取代裸寄存器操作。
5. 指出 Day5 直接 `ioremap` 写寄存器的局限。

## 环境与前置条件

| 项目 | 要求 |
| --- | --- |
| 前置 | Day8 的设备树匹配 |
| 目标内核 | 启用 `CONFIG_PINCTRL`、`CONFIG_GPIOLIB` |
| 板卡或 QEMU | i.MX6ULL / STM32MP157 / QEMU |

## 核心概念

### 1. 为什么需要 pinctrl 与 gpiolib

Day5 的写法：

```c
IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3 = ioremap(0x02290000 + 0x14, 4);
*IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3 = ...;
```

问题：

- 物理地址硬编码，换板即失效；
- 复用配置、方向、电平混在驱动里；
- 与其他驱动可能争用同一引脚，无仲裁。

pinctrl + gpiolib 把"引脚怎么配"下沉到设备树，把"引脚怎么用"抽象成统一 API。

### 2. pinctrl 的两类配置

| 类别 | 内容 | 设备树写法 |
| --- | --- | --- |
| pinmux | 引脚功能选择（GPIO / UART / I2C…） | `fsl,pins = <...>` |
| pinconf | 上拉、下拉、驱动强度、速率 | 同一属性中的附加位 |

i.MX6ULL 典型片段：

```dts
&iomuxc {
	pinctrl_led: ledgrp {
		fsl,pins = <
			MX6UL_PAD_SNVS_TAMPER3__GPIO5_IO03 0x10B0
		>;
	};
};
```

### 3. 客户端引用 pinctrl

```dts
myled {
	compatible = "100ask,led";
	pinctrl-names = "default";
	pinctrl-0 = <&pinctrl_led>;
	led-gpios = <&gpio5 3 GPIO_ACTIVE_LOW>;
};
```

- `pinctrl-names` 与 `pinctrl-N` 成对出现，`default` 在设备 probe 前自动应用。
- 驱动不需要写任何复用寄存器。

### 4. GPIO 消费者的写法

新时代写法（推荐）：

```dts
led-gpios = <&gpio5 3 GPIO_ACTIVE_LOW>;
```

驱动：

```c
struct gpio_desc *led;

led = devm_gpiod_get(&pdev->dev, "led", GPIOD_OUT_LOW);
if (IS_ERR(led))
	return PTR_ERR(led);
```

`devm_gpiod_get` 会自动完成：解析 `led-gpios`、申请引脚、设置初始方向/电平。

旧写法（仍常见）：

```c
int gpio = of_get_named_gpio(np, "led-gpios", 0);
gpio_request(gpio, "led");
gpio_direction_output(gpio, 0);
gpio_set_value(gpio, 1);
gpio_free(gpio);
```

### 5. 三个必须理解的细节

1. **`GPIO_ACTIVE_LOW`**：逻辑值 1 表示"有效"，实际电平由该标志取反。驱动只关心逻辑值。
2. **`devm_*` 的生命周期**：绑定到 `&pdev->dev`，设备解绑时自动释放，不需要手写 `gpiod_put`。
3. **名称匹配**：`devm_gpiod_get(dev, "led", ...)` 查找的是 `led-gpios`，不是 `gpios`；用 `NULL` 可匹配 `gpios`。

## 系统调用/接口卡片

### `struct gpio_desc *devm_gpiod_get(struct device *dev, const char *con_id, enum gpiod_flags flags)`

作用：从设备树获取 GPIO 并申请，自动管理生命周期。

参数：`con_id` 对应 `<con_id>-gpios` 属性；`flags` 可取 `GPIOD_IN`、`GPIOD_OUT_LOW`、`GPIOD_OUT_HIGH`。

返回值和错误：成功返回描述符；失败返回 `ERR_PTR()`，需 `IS_ERR()` 判断。

资源所有权：`devm` 版本随设备释放；非 `devm` 版本需 `gpiod_put()`。

最小示例：

```c
led = devm_gpiod_get(&pdev->dev, "led", GPIOD_OUT_LOW);
if (IS_ERR(led))
	return PTR_ERR(led);
```

常见误用：`con_id` 与属性名不匹配；把 `ERR_PTR` 当普通指针使用。

### `void gpiod_set_value(struct gpio_desc *desc, int value)`

作用：设置 GPIO 逻辑电平。

参数：`value` 为逻辑值，已考虑 `ACTIVE_LOW`。

返回值和错误：无返回值。

最小示例：`gpiod_set_value(led, 1);`

常见误用：以为传的是物理电平；在可能睡眠的上下文调用可睡眠版本。

### `int gpiod_direction_output(struct gpio_desc *desc, int value)`

作用：配置为输出并设置初始值。

最小示例：`gpiod_direction_output(led, 0);`

常见误用：在 `devm_gpiod_get` 已指定 `OUT_*` 后重复调用。

### `int of_get_named_gpio(const struct device_node *np, const char *propname, int index)`

作用：旧接口，仅解析设备树得到 GPIO 编号。

返回值和错误：成功返回 GPIO 编号；失败返回负错误码。

资源所有权：返回编号后仍需 `gpio_request` / `gpio_free`。

常见误用：只解析不申请，导致与其他驱动争用。

## 最小可运行实验

实验目录：`Linux/experiments/Day09_pinctrl与GPIO/`

```text
led_gpio.dts      pinctrl 组 + 客户端节点
led_gpio_drv.c    使用 devm_gpiod_get 的驱动
Makefile
README.md         合入 dts、编译、验证步骤
```

核心：

```c
static int led_probe(struct platform_device *pdev)
{
	struct gpio_desc *led;

	led = devm_gpiod_get(&pdev->dev, "led", GPIOD_OUT_LOW);
	if (IS_ERR(led))
		return PTR_ERR(led);

	platform_set_drvdata(pdev, led);
	dev_info(&pdev->dev, "probe: gpio acquired\n");
	return 0;
}
```

## 编译与运行

1. 合入 dts（见 `led_gpio.dts`），编译：

```bash
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- dtbs
```

2. 编译并部署模块：

```bash
cd Linux/experiments/Day09_pinctrl与GPIO
make KERN_DIR=~/100ask_imx6ull-sdk/Linux-4.9.88
```

3. 目标板上：

```bash
insmod led_gpio_drv.ko
dmesg | tail -5
cat /sys/kernel/debug/gpio        # 需要 debugfs
ls /proc/device-tree/myled/
```

## 结果观察

以下为**预期结果，未在本机实测**：

```text
$ dmesg | tail -3
led_gpio: probe: gpio acquired

$ cat /sys/kernel/debug/gpio | grep -i led
 gpio-131 (                    |led                 ) out lo
```

对比 Day5：驱动源码中不再出现任何物理地址。

## 常见错误和陷阱

| 现象 | 原因 | 定位方法 |
| --- | --- | --- |
| `devm_gpiod_get` 返回 `-ENOENT` | 属性名不匹配（`led-gpios` vs `gpios`） | 检查 dts 属性名与 `con_id` |
| 返回 `-EPROBE_DEFER` | gpio 控制器尚未就绪 | 返回该错误码让内核重试 |
| LED 逻辑反了 | 漏写 `GPIO_ACTIVE_LOW` | 对照电路有效电平 |
| 引脚被占用 | 其他驱动先申请 | `cat /sys/kernel/debug/gpio` |
| probe 时崩溃 | 未检查 `IS_ERR` | 增加判断 |
| 引脚功能不对 | 未引用 `pinctrl-0` | 检查节点与 `pinctrl-names` |

## 故障排查方法

1. `cat /sys/kernel/debug/gpio` 查看占用与方向。
2. `ls /proc/device-tree/<node>/` 确认属性存在。
3. `dmesg | grep pinctrl` 查看复用是否应用成功。
4. 确认 `CONFIG_PINCTRL` / `CONFIG_GPIOLIB` 已启用。
5. `-EPROBE_DEFER` 不是错误，等待重试即可。

## 练习题

### 基础题

题目：把 `led-gpios` 改为两个引脚（红、绿），驱动分别控制。

提示：`devm_gpiod_get(dev, "red", ...)` / `devm_gpiod_get(dev, "green", ...)`。

验收标准：两个引脚独立可配，`/sys/kernel/debug/gpio` 中均显示。

### 改错题

题目：下面的代码有什么问题？

```c
struct gpio_desc *led = devm_gpiod_get(&pdev->dev, "gpios", GPIOD_OUT_LOW);
gpiod_set_value(led, 1);
return 0;
```

提示：`con_id` 的匹配规则、`IS_ERR` 检查。

验收标准：能指出两处问题并修正。

### 综合题

题目：实现一个字符设备，把"写 0/1"映射到 `gpiod_set_value`，并支持设备树配置有效电平。

提示：结合 Day3 的 `file_operations` 与 grep 到的 `devm_gpiod_get`。

验收标准：换用 `GPIO_ACTIVE_LOW` 的节点后，同样的写值产生相反物理电平。

## 验收标准

- [ ] 能解释 pinmux 与 pinconf 的区别。
- [ ] 能写出 `pinctrl-0` 与 `<con_id>-gpios` 的正确组合。
- [ ] 能用 `devm_gpiod_get` 完成引脚申请并检查 `IS_ERR`。
- [ ] 能说明 `GPIO_ACTIVE_LOW` 对逻辑值的影响。
- [ ] 在目标环境完成一次 GPIO 控制验证（当前环境未完成）。

## 延伸阅读

- `Linux_WEIDS/02_嵌入式Linux驱动入门直播/StudyPlan/DailyDocs/09_Day9_Pinctrl和GPIO_学习讲解.md`
- `Linux_WEIDS/02_嵌入式Linux驱动入门直播/09_Pinctrl和GPIO/doc_pic/09_Pinctrl和GPIO.md`
- 下一篇：`Linux/notes/驱动入门/Day10_阻塞与非阻塞IO.md`

---

**验证记录**

| 项目 | 内容 |
| --- | --- |
| 日期 | 2026-10-25 |
| 已执行 | 阅读 pinctrl 讲义目录与 Day8 dts 结构 |
| 未执行 | dtb 编译、模块加载、GPIO 观测 |
| 结论 | 运行结果为预期值，未实测 |
