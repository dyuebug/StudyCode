# Day11：综合机制、上下文与资源退出

**验证状态**：部分实测（QEMU vexpress-a9 + Linux 7.0.0，2026-10-25）。定时器、tasklet、workqueue 与卸载清理顺序已验证，模块可正常卸载并打印 `resources released`；**线程化中断未覆盖**（QEMU 无中断源）。

## 学习目标

学完本篇应能：

1. 在上半部（硬中断）与下半部（tasklet / workqueue / 线程化中断）之间做出选择。
2. 说明定时器、tasklet、workqueue 各自允许的运行上下文。
3. 写出不会在卸载后继续执行的回调清理顺序。
4. 说明 `probe` 与 `remove` 的资源对称关系。
5. 指出课程综合示例把所有机制塞进一个文件的工程问题。

## 环境与前置条件

| 项目 | 要求 |
| --- | --- |
| 前置 | Day10 的中断、等待队列与 poll |
| 目标环境 | 具中断能力的 GPIO 与设备树 |
| 内核配置 | 默认配置即可 |

## 核心概念

### 1. 上下文决定能用什么

| 上下文 | 能否睡眠 | 可用机制 |
| --- | --- | --- |
| 硬中断（上半部） | 否 | 读寄存器、调度下半部 |
| tasklet（软中断） | 否 | 轻量延后处理 |
| 工作队列 | 是 | 需要睡眠或耗时的操作 |
| 线程化中断 | 是 | 需要睡眠的中断处理 |
| 进程上下文 | 是 | `copy_to_user`、`mutex` |

选择顺序：

```text
能在上半部做完    -> 直接做
需要延后且很短    -> tasklet
需要睡眠或较长    -> workqueue 或线程化中断
```

### 2. 线程化中断

```c
request_threaded_irq(irq, hardirq_fn, thread_fn, flags, name, data);
```

- `hardirq_fn` 返回 `IRQ_WAKE_THREAD` 唤醒线程；
- `thread_fn` 在进程上下文执行，可以睡眠、可以 `mutex_lock`。

课程 Day11 与 Day12 的 SR501 都用了这个模式：

```c
static irqreturn_t sr501_isr(int irq, void *dev_id)
{
	return IRQ_WAKE_THREAD;
}
```

### 3. 定时器

```c
setup_timer(&k->timer, key_timer_expire, (unsigned long)k);
mod_timer(&k->timer, jiffies + HZ / 50);
del_timer_sync(&k->timer);   /* 卸载时 */
```

定时器回调运行在软中断上下文：**不能睡眠**。

### 4. tasklet 与 workqueue

| 机制 | 头文件 | 上下文 | 清理 |
| --- | --- | --- | --- |
| tasklet | `<linux/interrupt.h>` | 软中断，不可睡眠 | `tasklet_kill()` |
| workqueue | `<linux/workqueue.h>` | 进程上下文，可睡眠 | `cancel_work_sync()` |

### 5. 卸载顺序是安全的核心

错误顺序会导致 use-after-free：回调在 `kfree` 之后执行。

正确顺序：

```text
1. 禁止新事件：disable_irq / 注销设备节点
2. 停止定时器：del_timer_sync
3. 停止 tasklet：tasklet_kill
4. 取消 work：cancel_work_sync
5. 释放中断：free_irq
6. 释放内存与类设备
```

原则：**先停止"可能还在跑的东西"，再释放它们引用的内存**。

### 6. 并发保护的选择

| 锁 | 可睡眠上下文 | 中断上下文 |
| --- | --- | --- |
| `mutex` | 可用 | 不可用 |
| `spinlock` | 可用（短临界区） | 可用 |
| `spin_lock_irqsave` | 可用 | 可用，且屏蔽本地中断 |

若中断与进程上下文共享数据，使用 `spin_lock_irqsave`；若只在进程上下文，使用 `mutex`。

## 系统调用/接口卡片

### `int request_threaded_irq(unsigned int irq, irq_handler_t handler, irq_handler_t thread_fn, unsigned long flags, const char *name, void *dev)`

作用：注册线程化中断。

参数：`handler` 可为 `NULL`（表示直接使用默认硬中断处理）。

返回值和错误：成功 0；失败返回负错误码。

资源所有权：`free_irq` 释放。

最小示例：

```c
err = request_threaded_irq(irq, hardirq, thread_fn,
			   IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			   "my_key", priv);
```

常见误用：把耗时操作放在 `hardirq`；`thread_fn` 返回非 `IRQ_HANDLED`。

### `void del_timer_sync(struct timer_list *timer)`

作用：删除定时器并等待正在执行的回调结束。

最小示例：`del_timer_sync(&k->timer);`

常见误用：使用 `del_timer`（不等待），在回调仍在执行时释放内存。

### `void tasklet_kill(struct tasklet_struct *t)`

作用：等待 tasklet 执行完毕并禁止再次调度。

常见误用：在 tasklet 自身中调用 `tasklet_kill`（会死锁）。

### `bool cancel_work_sync(struct work_struct *work)`

作用：取消工作项并等待其结束。

返回值和错误：返回 `true` 表示取消了一个尚未执行的工作。

常见误用：在 work 函数内部调用 `cancel_work_sync` 自身。

### `void disable_irq(unsigned int irq)` / `void enable_irq(unsigned int irq)`

作用：禁止/恢复中断。

注意：`disable_irq` 会等待当前处理函数结束，不能在中断上下文中调用。

## 最小可运行实验

实验目录：`Linux/experiments/Day11_上下文与资源退出/`

```text
key_ctx.c       线程化中断 + 定时器 + workqueue + 正确的卸载顺序
key_ctx_test.c  阻塞读与计数查询
key_ctx.dts     设备树片段
Makefile
README.md
```

卸载路径核心：

```c
static int key_remove(struct platform_device *pdev)
{
	struct key_dev *k = platform_get_drvdata(pdev);

	disable_irq(k->irq);
	del_timer_sync(&k->timer);
	cancel_work_sync(&k->work);
	free_irq(k->irq, k);

	device_destroy(key_class, MKDEV(major, 0));
	class_destroy(key_class);
	unregister_chrdev(major, DEV_NAME);
	return 0;
}
```

## 编译与运行

```bash
cd Linux/experiments/Day11_上下文与资源退出
make KERN_DIR=~/100ask_imx6ull-sdk/Linux-4.9.88
```

目标板：

```bash
insmod key_ctx.ko
./key_ctx_test -b        # 阻塞读，按一次键
./key_ctx_test -c        # 查询计数
rmmod key_ctx
dmesg | tail -20
```

连续加载卸载 100 次可检验资源是否对称：

```bash
for i in $(seq 100); do
	insmod key_ctx.ko && rmmod key_ctx || break
done
dmesg | grep -i "bug\|oops\|warning"
```

## 结果观察

以下为**预期结果，未在本机实测**：

```text
$ ./key_ctx_test -b
key event: value=1 count=1

$ ./key_ctx_test -c
count=1

$ rmmod key_ctx
[  ...] key_ctx: remove: timer/work/irq released
```

反复加载卸载后 `dmesg` 不应出现 `BUG`、`WARNING` 或 `use-after-free`。

## 常见错误和陷阱

| 现象 | 原因 | 定位方法 |
| --- | --- | --- |
| 卸载后内核 oops | 定时器/ work 回调访问已释放内存 | 改用 `del_timer_sync` / `cancel_work_sync` |
| `schedule while atomic` | 软中断上下文调用了可睡眠函数 | 检查 ISR/tasklet 中的调用 |
| 死锁 | tasklet 中调用 `tasklet_kill` | 移到进程上下文 |
| 中断计数持续增长但无人处理 | `free_irq` 未执行 | 检查 `remove` 路径 |
| 卸载时卡住 | `del_timer_sync` 与回调互等 | 检查回调是否等待同一把锁 |
| 并发读写数据错乱 | 未加锁或锁类型不当 | 用 `spin_lock_irqsave` 保护共享数据 |

## 故障排查方法

1. 用 `dmesg` 观察加载/卸载日志顺序。
2. 反复加载卸载，检查是否有残留中断或定时器。
3. `cat /proc/interrupts` 对比卸载前后的计数。
4. 打开 `CONFIG_DEBUG_ATOMIC_SLEEP`、`CONFIG_DEBUG_OBJECTS_TIMERS` 辅助定位。
5. 检查每一条 `probe` 中的申请是否在 `remove` 中有对应释放。

## 练习题

### 基础题

题目：把实验中的 `del_timer_sync` 换成 `del_timer`，说明可能的后果并设计复现方法。

提示：让定时器回调执行较长时间。

验收标准：能说明 use-after-free 的触发路径。

### 改错题

题目：下面的 `remove` 有什么问题？

```c
static int key_remove(struct platform_device *pdev)
{
	struct key_dev *k = platform_get_drvdata(pdev);

	kfree(k);
	free_irq(k->irq, k);
	return 0;
}
```

提示：释放顺序与 `free_irq` 的 `dev` 参数。

验收标准：能指出 use-after-free 与 `dev` 失效两点。

### 综合题

题目：把共享计数的保护从 `mutex` 改为 `spin_lock_irqsave`，并说明为什么必须屏蔽本地中断。

提示：中断上下文可能抢占进程上下文中的临界区。

验收标准：代码在中断与进程并发下无数据竞争（用压力测试验证）。

## 验收标准

- [ ] 能根据上下文选择 tasklet / workqueue / 线程化中断。
- [ ] 能写出正确的卸载顺序并解释每一步的必要性。
- [ ] 能区分 `del_timer` 与 `del_timer_sync`。
- [ ] 能说明 `mutex` 与 `spinlock` 的适用边界。
- [ ] 在目标环境完成一次反复加载卸载的稳定性验证（当前环境未完成）。

## 延伸阅读

- `Linux_WEIDS/02_嵌入式Linux驱动入门直播/StudyPlan/DailyDocs/11_Day11_总结驱动机制_学习讲解.md`
- `Linux_WEIDS/02_嵌入式Linux驱动入门直播/11_用一个程序总结驱动机制/source/`
- 下一篇：`Linux/notes/驱动入门/Day12_SR501实战与数据流.md`

---

**验证记录**

| 项目 | 内容 |
| --- | --- |
| 日期 | 2026-10-25 |
| 环境 | QEMU vexpress-a9 + Linux 7.0.0 |
| 已执行 | timer / tasklet / workqueue 运行；`SYS_delete_module` 触发 remove 路径 |
| 实测输出 | `event_demo: tasklet ran`、`event_demo: exit, resources released` |
| 未执行 | 线程化中断、真实硬件中断、压力循环卸载 |
| 结论 | 上下文机制与卸载顺序已实测；中断部分仍需板卡 |
