# Day06 实验：device / driver / bus 分离

## 目的

验证 `platform_device` 与 `platform_driver` 的匹配与 `probe`/`remove` 时机，并确认加载顺序不影响结果。

## 文件

| 文件 | 作用 |
| --- | --- |
| `hello_platform_dev.c` | 只注册设备 |
| `hello_platform_drv.c` | 只注册驱动 |
| `Makefile` | 构建两个模块 |

## 顺序一：先设备后驱动

```bash
make
sudo insmod hello_platform_dev.ko
sudo insmod hello_platform_drv.ko
sudo dmesg | tail -6

sudo rmmod hello_platform_drv
sudo rmmod hello_platform_dev
```

预期：

```text
hello_platform: device register -> 0
hello_platform: driver register -> 0
hello_platform hello_platform: probe matched: hello_platform
```

## 顺序二：先驱动后设备

```bash
sudo insmod hello_platform_drv.ko
sudo insmod hello_platform_dev.ko
sudo dmesg | tail -6
```

预期 `probe` 同样只执行一次，由设备注册触发匹配。

## 观察点

```bash
ls /sys/bus/platform/devices/ | grep hello
ls /sys/bus/platform/drivers/ | grep hello
```

设备目录下会出现指向驱动的符号链接，表示绑定成功。

## 常见问题

| 现象 | 处理 |
| --- | --- |
| 两个模块都加载但无 probe | 核对两侧 `name` 是否完全一致 |
| 卸载设备时内核告警 | 确认 `.dev.release` 已提供 |
| `insmod` 报 File exists | 同名设备已注册，先卸载 |
| 新内核编译报 remove 签名 | 本代码已用 `LINUX_VERSION_CODE` 分支适配 6.11 |

## 验证状态

未在本机验证（WSL2 无法构建和加载内核模块）。
