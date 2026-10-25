# Day12 实验：SR501 人体红外驱动

## 接线

| SR501 | 板卡 |
| --- | --- |
| VCC | 3.3V |
| GND | GND |
| OUT | GPIO4_IO19（示例） |

注意：SR501 供电电压与输出电平以模块手册为准，部分模块为 5V。

## 步骤

1. 合入 `sr501.dts`，编译 dtb。
2. 编译模块：

```bash
make KERN_DIR=~/100ask_imx6ull-sdk/Linux-4.9.88
```

3. 目标板：

```bash
insmod sr501_drv.ko
ls -l /dev/sr501
cat /proc/interrupts | grep sr501

./sr501_test          # 阻塞等待人体移动
./sr501_test -p       # poll
./sr501_test -n       # 非阻塞

rmmod sr501_drv
```

## 预期输出

```text
$ ./sr501_test
waiting for motion...
motion event: value=1

$ ./sr501_test
waiting for motion...
motion event: value=0
```

## 无中断时的替代方案

若 GPIO 不支持中断，或 `gpiod_to_irq()` 返回负值：

1. 改用内核线程每 20ms 轮询 `gpiod_get_value`。
2. 变化时 `wake_up_interruptible`。
3. `remove` 中调用 `kthread_stop`。

更详细的实现参考课程 `16_实战_SR501驱动_使用内核线程实现_答疑`。

## 常见问题

| 现象 | 处理 |
| --- | --- |
| 从不触发 | 先用轮询确认电平与接线 |
| 一直触发 | 输入悬空或灵敏度高，加下拉/调电位器 |
| 抖动导致连续事件 | 增加 50ms 时间窗去抖 |
| `-ENODEV` | dts 未生效或 `gpios` 属性名错误 |
| `gpiod_to_irq` 返回负值 | GPIO 控制器不支持中断，改用线程轮询 |

## 验证状态

未在本机验证。需要真实 SR501 模块与目标板。
