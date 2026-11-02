// SPDX-License-Identifier: GPL-2.0
/*
 * Day16 驱动侧：把驱动参数暴露成 sysfs 属性
 *
 * 解决的问题：私有字符设备只能靠 read/write/ioctl 交互，
 * 而"改一个参数、看一个统计"这类运维动作，内核的标准答案是 sysfs。
 *
 * 本实验暴露两个属性：
 *   interval  RW  周期（ms），范围 10..5000，越界写返回 -EINVAL
 *   stats     RO  计数：ticks / writes / rejects
 *
 * 三个必须说清的语义：
 *   1. store() 返回 count 才表示"字节全部消费"。返回小于 count 的正数时，
 *      内核的 kernfs 写循环会把剩余字节再喂一次，表现为"echo 一次，驱动收到两次"。
 *   2. 拒绝写入要返回负 errno，用户态 write() 得到该 errno；返回 0 会被当成
 *      "没写进去"并重试（同样落入上面的循环）。
 *   3. 卸载顺序：先移除 sysfs 组，再停定时器。反过来的窗口里 store() 仍可被调用，
 *      会改到正在被定时器读的结构体。
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/sysfs.h>
#include <linux/timer.h>

#define DRV_NAME       "param_demo"
#define INTERVAL_MIN   10
#define INTERVAL_MAX   5000

struct param_ctx {
	struct timer_list timer;
	unsigned int interval_ms;
	unsigned int ticks;
	unsigned int writes;
	unsigned int rejects;
};

static struct param_ctx *ctx;

static void param_timer(struct timer_list *t)
{
	struct param_ctx *c = container_of(t, struct param_ctx, timer);

	c->ticks++;
	pr_info("%s: tick=%u interval=%ums\n",
		DRV_NAME, c->ticks, READ_ONCE(c->interval_ms));

	mod_timer(&c->timer, jiffies + msecs_to_jiffies(READ_ONCE(c->interval_ms)));
}

static ssize_t interval_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	return sysfs_emit(buf, "%u\n", READ_ONCE(ctx->interval_ms));
}

static ssize_t interval_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	unsigned int val;
	int err;

	err = kstrtouint(buf, 0, &val);
	if (err) {
		ctx->rejects++;
		pr_info("%s: reject non-numeric write (err=%d)\n", DRV_NAME, err);
		return err;
	}

	if (val < INTERVAL_MIN || val > INTERVAL_MAX) {
		ctx->rejects++;
		pr_info("%s: reject out-of-range %u (allowed %u..%u)\n",
			DRV_NAME, val, INTERVAL_MIN, INTERVAL_MAX);
		return -EINVAL;
	}

	WRITE_ONCE(ctx->interval_ms, val);
	ctx->writes++;
	pr_info("%s: interval set to %ums by userspace\n", DRV_NAME, val);

	return count;      /* 必须返回 count，理由见文件头第 1 条 */
}

static ssize_t stats_show(struct device *dev,
			  struct device_attribute *attr, char *buf)
{
	return sysfs_emit(buf, "ticks=%u writes=%u rejects=%u interval=%u\n",
			  ctx->ticks, ctx->writes, ctx->rejects,
			  READ_ONCE(ctx->interval_ms));
}

static DEVICE_ATTR_RW(interval);
static DEVICE_ATTR_RO(stats);

/*
 * 用 attribute_group 一次注册：每条 DEVICE_ATTR 展开成 dev_attr_xxx，
 * 再取 .attr 指针放进数组。逐个 device_create_file() 也行，
 * 但成组注册的失败回滚是内核统一处理的，卸载只需一次调用。
 */
static struct attribute *param_attrs[] = {
	&dev_attr_interval.attr,
	&dev_attr_stats.attr,
	NULL,
};

static const struct attribute_group param_group = {
	.attrs = param_attrs,
};

static int param_probe(struct platform_device *pdev)
{
	int err;

	ctx = devm_kzalloc(&pdev->dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->interval_ms = 500;

	err = sysfs_create_group(&pdev->dev.kobj, &param_group);
	if (err) {
		dev_err(&pdev->dev, "sysfs_create_group failed: %d\n", err);
		return err;
	}

	timer_setup(&ctx->timer, param_timer, 0);
	mod_timer(&ctx->timer, jiffies + msecs_to_jiffies(ctx->interval_ms));

	dev_info(&pdev->dev, "probe ok, attrs at /sys/devices/platform/%s/\n",
		 DRV_NAME);
	return 0;
}

static void param_remove(struct platform_device *pdev)
{
	/* 先断开用户态入口，再停事件源：反序会留下"能写但没人读"的窗口 */
	sysfs_remove_group(&pdev->dev.kobj, &param_group);
	timer_delete_sync(&ctx->timer);

	dev_info(&pdev->dev, "remove ok, ticks=%u writes=%u rejects=%u\n",
		 ctx->ticks, ctx->writes, ctx->rejects);
	/* ctx 由 devm 管理，不需要 kfree */
}

static struct platform_driver param_driver = {
	.probe  = param_probe,
	.remove = param_remove,
	.driver = {
		.name = DRV_NAME,
	},
};

module_platform_driver(param_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Day16: driver params exposed via sysfs attribute_group");
