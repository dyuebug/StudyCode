# Day10：中断、阻塞/非阻塞、poll 与控制接口

**验证状态**：编译实测 + 机制部分实测（QEMU vexpress-a9 + Linux 7.0.0，2026-10-25）。本日驱动 `gpio_key_irq.ko` 已交叉编译通过；阻塞读、非阻塞 `EAGAIN`、`poll` 与 `ioctl` 由**替代模块 `event_demo`** 验证通过——运行期证据不来自本日驱动本身；**硬件中断投递（`request_irq` / `request_threaded_irq`）未覆盖**。证据见 `Linux/tests/qemu-vexpress/RESULT.md`。

## 学习目标

学完本篇应能：

1. 从设备树取得中断号并注册中断处理函数。
2. 说明阻塞读与非阻塞读的区别，以及 `O_NONBLOCK` 的作用。
3. 用等待队列实现"有数据才返回"的读。
4. 用 `poll` 让应用同时等待多个设备。
5. 说明 `ioctl` 与 `read/write` 的分工。

## 环境与前置条件

| 项目 | 要求 |
| --- | --- |
| 前置 | Day8/Day9 的设备树与 GPIO |
| 目标环境 | 板卡或 QEMU，具中断能力的 GPIO |
| 内核配置 | `CONFIG_GPIOLIB`、中断控制器驱动 |

## 核心概念

### 1. 中断的四个要素

```text
触发源  GPIO 电平变化 / 边沿
中断号  由设备树或平台资源提供
处理函数 内核在中断上下文调用
触发类型 IRQF_TRIGGER_RISING / FALLING / HIGH / LOW
```

课程 `01_irq_for_test` 的核心只有三行：

```c
irq = of_irq_get(pdev->dev.of_node, 0);
request_irq(irq, gpio_key_isr, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
	    "100ask_gpio_key", NULL);
free_irq(irq, NULL);
```

### 2. 中断上下文的两条铁律

1. **不能睡眠**：不能调用会阻塞的函数（`msleep`、`mutex_lock`、`copy_to_user` 的直接使用等）。
2. **要快**：耗时工作交给下半部（tasklet / workqueue / 线程化中断）。

### 3. 阻塞与非阻塞

| 方式 | 行为 | 应用写法 |
| --- | --- | --- |
| 阻塞 | 无数据时睡眠，直到有数据或被信号打断 | `read(fd, ...)` |
| 非阻塞 | 无数据立即返回 `-EAGAIN` | `open(..., O_NONBLOCK)` |
| 多路等待 | 同时等多个 fd | `poll()` / `select()` / `epoll()` |

驱动侧统一用等待队列：

```c
wait_event_interruptible(wq, !is_empty());
```

非阻塞分支：

```c
if (file->f_flags & O_NONBLOCK)
	return -EAGAIN;
```

### 4. poll 的三步

```text
1. 应用调用 poll()，注册等待
2. 驱动 poll_wait(file, &wq, wait) 把当前等待项挂到自己的队列
3. 条件满足时驱动唤醒队列，poll 返回就绪掩码
```

返回值是位掩码：`POLLIN`、`POLLOUT`、`POLLERR` 等。

### 5. ioctl 的定位

| 接口 | 适合 |
| --- | --- |
| `read/write` | 数据流 |
| `ioctl` | 控制命令：清空缓冲、设置模式、读取状态 |
| `sysfs` | 简单配置项 |

`ioctl` 命令号用 `_IO` / `_IOR` / `_IOW` 宏生成，避免与其他驱动冲突。

### 6. 课程 Day11 中的完整机制清单

`10_read_key_irq_poll_fasync_block_timer_tasklet_workqueue_threadedirq` 一个文件里集中了：

```text
阻塞读    wait_event_interruptible
poll      poll_wait
异步通知  fasync_helper / kill_fasync
定时器    setup_timer / mod_timer
tasklet   tasklet_init / tasklet_schedule
工作队列  INIT_WORK / schedule_work
线程化中断 request_threaded_irq / IRQ_WAKE_THREAD
```

这是 Day11 的复盘素材，但不应作为工程模板——它把所有机制塞进一个文件。

## 系统调用/接口卡片

### `int request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags, const char *name, void *dev)`

作用：注册中断处理函数。

参数：中断号、处理函数、触发标志、名称、传给处理函数的私有指针。

返回值和错误：成功 0；失败返回负错误码（`-EBUSY`、`-EINVAL`）。

资源所有权：必须 `free_irq(irq, dev)`，且 `dev` 必须一致。

线程/进程安全：处理函数运行在中断上下文。

最小示例：

```c
err = request_irq(irq, isr, IRQF_TRIGGER_RISING, "my_key", priv);
```

常见误用：`free_irq` 传错 `dev`；共享中断未加 `IRQF_SHARED`；在 ISR 中睡眠。

### `int of_irq_get(struct device_node *dev, int index)`

作用：从设备树取中断号。

返回值和错误：成功返回中断号；失败返回负错误码或 0。

最小示例：`irq = of_irq_get(pdev->dev.of_node, 0);`

常见误用：不检查返回值；节点中没有 `interrupts` 属性。

### `wait_event_interruptible(wq, condition)`

作用：在条件不满足时睡眠，可被信号打断。

返回值和错误：返回 0 表示条件满足；返回 `-ERESTARTSYS` 表示被信号打断。

最小示例：

```c
if (wait_event_interruptible(wq, !is_empty()))
	return -ERESTARTSYS;
```

常见误用：条件表达式带副作用；忘记处理 `-ERESTARTSYS`。

### `void poll_wait(struct file *filp, wait_queue_head_t *wq, poll_table *wait)`

作用：把当前 `poll` 等待项挂到驱动的等待队列。

返回值和错误：无返回值。

最小示例：

```c
static __poll_t my_poll(struct file *f, poll_table *wait)
{
	__poll_t mask = 0;

	poll_wait(f, &wq, wait);
	if (!is_empty())
		mask |= EPOLLIN | EPOLLRDNORM;
	return mask;
}
```

常见误用：返回类型写成 `unsigned int`（新内核要求 `__poll_t`）；忘记调用 `poll_wait` 导致永远不就绪。

### `long unlocked_ioctl(struct file *f, unsigned int cmd, unsigned long arg)`

作用：实现控制命令。

返回值和错误：成功返回 0 或正整数；失败返回负错误码。

最小示例：

```c
switch (cmd) {
case MY_IOC_CLEAR:
	clear_buffer();
	return 0;
default:
	return -ENOTTY;
}
```

常见误用：返回 `-EINVAL` 代替 `-ENOTTY`（无法区分"命令不支持"与"参数错误"）；未校验 `arg`。

## 最小可运行实验

实验目录：`Linux/experiments/Day10_中断与poll/`

```text
gpio_key_irq.c    线程化中断 + 等待队列 + poll + ioctl
button_test.c     阻塞读、非阻塞读、poll 三种模式
key.dts           设备树片段
Makefile
README.md
```

核心：

```c
static irqreturn_t key_hardirq(int irq, void *data)
{
	return IRQ_WAKE_THREAD;
}

static irqreturn_t key_thread(int irq, void *data)
{
	struct key_dev *k = data;

	k->value = gpiod_get_value(k->gpiod);
	k->count++;
	wake_up_interruptible(&k->wq);
	return IRQ_HANDLED;
}
```

## 编译与运行

```bash
cd Linux/experiments/Day10_中断与poll
make KDIR=~/100ask_imx6ull-sdk/Linux-4.9.88 ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
```

目标板：

```bash
insmod gpio_key_irq.ko
ls /dev/myk

# 阻塞读
./button_test -b

# 非阻塞读
./button_test -n

# poll
./button_test -p
```

## 结果观察

以下为**预期结果，未在本机实测**：

```text
$ ./button_test -b
waiting...
key event: value=1 count=1

$ ./button_test -n
no data (EAGAIN)

$ ./button_test -p
poll ready: POLLIN
key event: value=0 count=2
```

## 常见错误和陷阱

| 现象 | 原因 | 定位方法 |
| --- | --- | --- |
| `request_irq` 返回 `-EBUSY` | 中断被其他驱动占用 | `cat /proc/interrupts` |
| 中断不触发 | 触发类型与硬件不符 | 改 `IRQF_TRIGGER_*` |
| 系统卡死 | ISR 中睡眠或耗时操作 | 把工作移到线程化中断 |
| `poll` 永远不就绪 | 忘记 `poll_wait` 或没唤醒 | 检查 `wake_up` 调用 |
| 非阻塞读不返回 `EAGAIN` | 未检查 `O_NONBLOCK` | 在 `read` 开头分支 |
| 卸载后中断仍触发 | 未 `free_irq` | 检查 `remove` 路径 |
| `ioctl` 返回 `EINVAL` | 命令号不匹配 | 打印 `cmd` 的十六进制值 |
| 编译报 `macro 'key_init' passed 1 arguments, but takes just 0` | 函数名与 `<linux/key.h>` 的 `#define key_init()` 冲突 | 改名为 `key_irq_init` |

## 故障排查方法

1. `cat /proc/interrupts` 确认中断注册与计数变化。
2. `dmesg` 查看 probe 与 ISR 日志。
3. 用阻塞/非阻塞/poll 三种模式分别验证。
4. `ioctl` 失败时打印命令号，与用户态定义比对。

## 练习题

### 基础题

题目：给驱动增加 `ioctl` 命令，用于清空事件计数。

提示：`_IO('k', 0)` 定义命令号。

验收标准：调用后 `count` 归零，再次读事件时从 1 开始。

### 改错题

题目：下面的 ISR 有什么问题？

```c
static irqreturn_t key_isr(int irq, void *data)
{
	struct key_dev *k = data;
	msleep(10);
	k->value = gpiod_get_value(k->gpiod);
	copy_to_user(k->ubuf, &k->value, sizeof(int));
	return IRQ_HANDLED;
}
```

提示：中断上下文的睡眠与用户拷贝限制。

验收标准：能指出两处违规，并改为线程化中断 + 唤醒等待队列。

### 综合题

题目：实现 `fasync` 异步通知：按键时向注册了 `SIGIO` 的进程发送信号。

提示：`fasync_helper` 在 `fasync` 回调中调用，`kill_fasync(&fasync, SIGIO, POLL_IN)` 在唤醒处调用。

验收标准：应用设置 `O_ASYNC` 后不阻塞也能收到信号。

## 验收标准

- [ ] 能从设备树取中断号并成功注册。
- [ ] 能解释阻塞、非阻塞、poll 三者的适用场景。
- [ ] 能写出带 `O_NONBLOCK` 分支的 `read`。
- [ ] 能实现 `poll` 并返回正确掩码。
- [ ] 在目标环境完成一次中断触发与读取验证（当前环境未完成）。

## 延伸阅读

- `Linux_WEIDS/02_嵌入式Linux驱动入门直播/StudyPlan/DailyDocs/10_Day10_APP与驱动更多交互方式_学习讲解.md`
- `Linux_WEIDS/02_嵌入式Linux驱动入门直播/10_应用程序和驱动程序的更多交互方式/source/01_irq_for_test/`
- 下一篇：`Linux/notes/驱动入门/Day11_综合机制与资源退出.md`

---

**验证记录**

| 项目 | 内容 |
| --- | --- |
| 日期 | 2026-10-25 |
| 环境 | QEMU vexpress-a9 + Linux 7.0.0；定时器驱动事件（100ms） |
| 已执行（编译） | `gpio_key_irq.c` 交叉编译为 ARM ELF `gpio_key_irq.ko`，测试程序 `button_test` 同为 ARM |
| 已执行（运行） | 非阻塞读 `EAGAIN`、`poll` 返回 `POLLIN`、阻塞读返回事件、`ioctl` PAUSE/CLEAR/RESUME |
| 实测输出 | 五项断言全部 `[ OK ]` |
| 验证载体 | 运行验证使用替代模块 `tests/qemu-vexpress/module/event_demo.c`，不是本日驱动 |
| 未执行 | 本日驱动的运行期行为、硬件中断注册与投递（QEMU 无外部输入） |
| 结论 | 本日驱动编译通过；等待队列与多路复用机制已由等价模块实测；中断部分仍需板卡 |
