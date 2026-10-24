# Day09 实验：pinctrl 与 GPIO

## 目的

用设备树 + gpiolib 取代裸寄存器操作，验证 `devm_gpiod_get` 与 `GPIOD_ACTIVE_LOW` 的行为。

## 步骤

### 1. 合入设备树

把 `led_gpio.dts` 的两段内容合入目标板 dts：

- `&iomuxc { ... }` 定义引脚复用组
- 根节点下的 `myled_gpio { ... }` 定义消费者

### 2. 编译

```bash
# dtb
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- dtbs

# 模块
make KERN_DIR=~/100ask_imx6ull-sdk/Linux-4.9.88
```

### 3. 目标板验证

```bash
insmod led_gpio_drv.ko
ls -l /dev/myled_gpio
dmesg | tail -5

printf '\1' > /dev/myled_gpio
printf '\0' > /dev/myled_gpio

cat /sys/kernel/debug/gpio | grep -i led
```

## 预期输出

```text
$ dmesg | tail -2
led_gpio myled_gpio: probe: gpio acquired
led_gpio: set logical value 1
```

`/sys/kernel/debug/gpio` 中应看到该引脚被 `led` 占用。

## 关键检查点

| 检查项 | 方法 |
| --- | --- |
| 设备树节点是否加载 | `ls /proc/device-tree/myled_gpio/` |
| 引脚复用是否生效 | `dmesg \| grep pinctrl` |
| GPIO 是否被占用 | `cat /sys/kernel/debug/gpio` |
| 有效电平是否正确 | 改 `GPIO_ACTIVE_LOW` 观察物理电平反转 |

## 常见问题

| 现象 | 处理 |
| --- | --- |
| `-ENOENT` | 属性名应为 `led-gpios`，与 `con_id="led"` 对应 |
| `-EPROBE_DEFER` | gpio 控制器未就绪，属正常重试 |
| LED 常亮或常灭 | 检查 `GPIO_ACTIVE_LOW` 与电路 |
| 编译缺 `gpiod_*` | 确认内核启用 `CONFIG_GPIOLIB` |

## 验证状态

未在本机验证。需要目标内核树、可加载 dtb 的板卡或 QEMU。
