# QEMU 验证结果

**日期**：2026-10-25
**环境**：QEMU 10.2.1 `vexpress-a9`，Linux 7.0.0（Ubuntu `linux-source-7.0.0`）
**工具链**：arm-linux-gnueabihf-gcc 15.2.0
**载荷**：initramfs 921 KB，含静态 init 与 11 个外部模块

## 内核配置

基础配置 `vexpress_defconfig`，另需：

```bash
./scripts/config --enable ARCH_VIRT              # 带出 ARM_AMBA
./scripts/config --enable GPIO_GENERIC_PLATFORM  # basic-mmio-gpio
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- olddefconfig
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- zImage modules
```

> 注意：`olddefconfig` 必须带 `ARCH=arm`，否则配置会被重算成主机 x86，导致后续模块结构体布局错误。

## 命令

```bash
make initramfs
make dtb
bash run.sh
```

## 实测输出（首次验证，Day2–Day7）

```text
===== qemu-vexpress verification =====
[ OK ] loaded /hello_module.ko
hello_module: init
[ OK ] loaded /hello_chrdev.ko
[ OK ] mknod /dev/hello_chrdev major=248
[ OK ] Day3 open /dev/hello_chrdev
[ OK ] Day3 write returns 1 (callback stub)
[ OK ] Day3 read returns 1 (no data copied)
[ OK ] loaded /hello_copy.ko
[ OK ] mknod /dev/hello_copy major=247
[ OK ] Day4 open /dev/hello_copy
[ OK ] Day4 read initial value = 123
[ OK ] Day4 write 456
[ OK ] Day4 read back 456
[ OK ] Day4 short read returns 1 (no overflow)
[ OK ] loaded /hello_platform_dev.ko
[ OK ] loaded /hello_platform_drv.ko
[ OK ] Day6 platform driver registered
[ OK ] loaded /led_platform_dev.ko
[ OK ] loaded /led_platform_drv.ko
[ OK ] Day7 /dev/led0 created
[ OK ] Day7 /dev/led1 created
[ OK ] Day7 write to /dev/led0
hello_platform hello_platform: probe matched: hello_platform
led_multi led_multi.0: probe: minor=0 pin=3
led_multi led_multi.1: probe: minor=1 pin=5
led_multi: minor=0 pin=3 value=1
===== result: PASSED (failures=0) =====
```

## 实测输出（扩展验证，Day8–Day9）

自定义设备树 `100ask-vexpress.dts`：

- 禁用 `gpio-leds`，释放 `v2m_led_gpios`
- 追加 `myled_for_test`（`compatible = "100ask,led"`）
- 追加 `myled_gpio`（`compatible = "100ask,led-gpio"`，`led-gpios = <&v2m_led_gpios 0 GPIO_ACTIVE_HIGH>`）

```text
[ OK ] loaded /led_dts_drv.ko
[ OK ] Day8 DT node present (myled_for_test)
[ OK ] Day8 driver registered
[ OK ] loaded /led_gpio_drv.ko
[ OK ] Day9 /dev/myled_gpio created
[ OK ] Day9 set logical 1
[ OK ] Day9 read back 1
[ OK ] Day9 set logical 0
[ OK ] Day9 read back 0
===== result: PASSED (failures=0) =====

led_dts myled_for_test: probe: pin=3
```

## 实测输出（Day5，交叉编译与模块生命周期）

```text
[ OK ] Day5 cross-built module loaded, /dev/myled created
100ask_led: init, major = 244, /dev/myled ready
100ask_led: exit
[ OK ] unload led_drv (remove path)
```

> 限制：板级寄存器（i.MX6ULL GPIO5 / IOMUXC）在 vexpress 上无对应硬件，未访问。
> 本验证覆盖交叉编译、架构校验、加载、设备节点创建与卸载路径。

## 实测输出（Day10–Day11，定时器事件替代硬件中断）

```text
[ OK ] loaded /event_demo.ko
[ OK ] Day10 /dev/qemu_event created
[ OK ] Day10 ioctl PAUSE
[ OK ] Day10 ioctl CLEAR
[ OK ] Day10 open nonblocking
[ OK ] Day10 nonblocking read returns EAGAIN
[ OK ] Day10 ioctl RESUME
[ OK ] Day10 poll reports POLLIN
[ OK ] Day10 blocking read returns event
event_demo: tasklet ran, count=1
[ OK ] Day11 unload triggers remove path
===== result: PASSED (failures=0) =====
event_demo: exit, resources released
```

> 限制：QEMU vexpress 没有可注入的中断源，`request_irq` / `request_threaded_irq`
> 的真实投递未验证。上面的验证覆盖等待队列、`poll`、`ioctl`、timer、tasklet、
> workqueue 与卸载清理顺序。

## 结论

| Day | 验证项 | 结果 |
| --- | --- | --- |
| 2 | 交叉编译、模块加载、printk | 通过 |
| 3 | `register_chrdev`、`mknod`、open/write/read/release | 通过 |
| 4 | `copy_to_user` / `copy_from_user`、短读边界 | 通过 |
| 5 | 交叉编译、加载、设备节点、卸载 | 通过（寄存器未访问） |
| 6 | platform 设备与驱动匹配、probe | 通过 |
| 7 | 多实例、次设备号、drvdata、设备节点 | 通过 |
| 8 | 设备树 compatible 匹配、属性读取 | 通过 |
| 9 | `devm_gpiod_get`、GPIO 写入与读回 | 通过 |
| 10 | 阻塞/非阻塞/poll/ioctl | 通过（中断未覆盖） |
| 11 | timer/tasklet/workqueue、卸载清理 | 通过（中断未覆盖） |

## 未覆盖

- Day5：i.MX6ULL 寄存器写入与 LED 点亮现象需要真实目标板。
- Day10/11：需要可触发的中断源，QEMU 无外部输入，`request_irq` 的真实投递未验证。
- Day12/13/14：需要 SR501、AT24C02、SPI OLED，`vexpress-a9` 未建模。
- 模块卸载走 `SYS_delete_module` 系统调用（Day5、Day10/11 已覆盖）；`insmod` / `rmmod` 命令行工具路径未验证。

## 过程中发现并修复的问题

1. **Day7 驱动缺少 `.remove` 版本分支**：Linux 6.11 起 `platform_driver.remove` 返回 `void`，编译报 `incompatible pointer type`。
2. **Day9–Day12 缺少 `<linux/of.h>`**：`struct of_device_id` 不完整，编译失败。
3. **initramfs 缺少 `/proc`、`/sys` 目录**：挂载失败，`/proc/devices` 解析不到主设备号。
4. **`init_module` 返回 `EFAULT`**：改用 `finit_module` 由内核直接读取模块文件。
5. **`make olddefconfig` 未带 `ARCH=arm`**：配置被重算成 x86，模块结构体布局错误，加载时在 `platform_device_add` 触发内核 oops。恢复 ARM 配置并全量重编后解决。
6. **vexpress GPIO 驱动识别错误**：`v2m_*_gpios` 由 `basic-mmio-gpio`（`GPIO_GENERIC_PLATFORM`）处理，不是 PL061；且 `sys_mci` 仅 2 个引脚、`sys_led` 被 gpio-leds 占用，最终禁用 gpio-leds 后使用 `sys_led` 第 0 号引脚。
