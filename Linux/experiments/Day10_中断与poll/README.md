# Day10 实验：中断、阻塞/非阻塞与 poll

## 目的

对比三种应用等待方式，并验证 `ioctl` 控制命令。

## 步骤

1. 合入 `key.dts` 节点并编译 dtb。
2. 编译模块：

```bash
make KDIR=~/100ask_imx6ull-sdk/Linux-4.9.88 ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
```

3. 目标板：

```bash
insmod gpio_key_irq.ko
ls -l /dev/myk
cat /proc/interrupts | grep gpio_key

./button_test -b     # 阻塞，按一次键
./button_test -n     # 非阻塞，无事件时立即 EAGAIN
./button_test -p     # poll，按键后返回
./button_test -c     # 清空计数
```

## 预期输出

```text
$ ./button_test -n
read failed: Resource temporarily unavailable

$ ./button_test -b
waiting...
key event: value=1

$ ./button_test -p
polling...
poll ready: 0x1
key event: value=0
```

## 三种模式对照

| 模式 | 无事件时 | 适用 |
| --- | --- | --- |
| 阻塞 | 睡眠 | 单设备、单任务 |
| 非阻塞 | 立即 `EAGAIN` | 轮询或事件循环 |
| poll | 睡眠直到就绪 | 多设备统一等待 |

## 常见问题

| 现象 | 处理 |
| --- | --- |
| `-EBUSY` | 中断被占用，`cat /proc/interrupts` |
| 无事件 | 触发边沿与电路不符，先改轮询确认电平 |
| `poll` 不返回 | 检查 `wake_up_interruptible` 与 `poll_wait` |
| `ioctl` 返回 `ENOTTY` | 命令号与驱动不一致 |

## 验证状态

**编译实测**（2026-10-25）：`gpio_key_irq.ko` 与 `button_test` 均为 ARM ELF 产物。

**未验证**：真实 GPIO 中断触发、按键事件时序，需要目标板与可触发中断的引脚。
