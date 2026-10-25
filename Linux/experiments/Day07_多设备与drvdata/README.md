# Day07 实验：一个驱动服务多个设备

## 目的

验证 `pdev->id`、次设备号、`platform_set_drvdata` / `platform_get_drvdata` 在多实例场景下的作用。

## 步骤

```bash
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

## 预期输出

```text
$ ls /dev/led*
/dev/led0  /dev/led1

$ sudo dmesg | grep led_multi
led_multi: probe: minor=0 pin=3
led_multi: probe: minor=1 pin=5
```

写入后：

```text
led_multi: minor=0 pin=3 value=1
led_multi: minor=1 pin=5 value=0
```

## 与课程示例的差异

| 项目 | 课程 `led_dev3.c` | 本实验 |
| --- | --- | --- |
| 引脚传递 | `IORESOURCE_IRQ.start` 塞 `(3<<8)\|3` | `platform_data.pin` |
| 设备数量 | 3 个不同 name | 2 个同 name、不同 id |
| 次设备号 | 全局计数器 | `pdev->id` |

## 验证状态

**已实测**（2026-10-25，QEMU 10.2.1 `vexpress-a9` + Linux 7.0.0 + arm-linux-gnueabihf-gcc 15.2.0）：

- 交叉编译：`led_platform_dev.ko` / `led_platform_drv.ko` 均为 ARM ELF
- 两个同 name、不同 id 的设备分别 probe 为 `led_multi.0` / `led_multi.1`，次设备号为 0 / 1
- `platform_data.pin` 经 probe 传入驱动，`/dev/led0`、`/dev/led1` 均创建成功
- 写入 `/dev/led0` 后驱动侧打印 `minor=0 pin=3 value=1`

**未验证**：真实 GPIO 输出与并发写。详见 [`../../tests/qemu-vexpress/RESULT.md`](../../tests/qemu-vexpress/RESULT.md)。
