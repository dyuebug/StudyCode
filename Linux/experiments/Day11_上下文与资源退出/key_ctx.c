#include <linux/device.h>
#include <linux/fs.h>
#include <linux/gpio/consumer.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

/*
 * Day11：线程化中断 + 定时器 + 工作队列 + 正确的卸载顺序
 *
 * 重点不是"功能多"，而是每一层申请的机制都在 remove 中成对释放。
 */

#define DEV_NAME "key_ctx"

struct key_dev {
	struct device *dev;
	struct gpio_desc *gpiod;
	int irq;
	int value;
	unsigned int count;
	unsigned int read_count;
	wait_queue_head_t wq;
	struct timer_list timer;
	struct work_struct work;
};

static int major;
static struct class *key_class;
static struct key_dev *the_key;

static irqreturn_t key_hardirq(int irq, void *data)
{
	/* 上半部只做最少的事 */
	return IRQ_WAKE_THREAD;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0)
static void key_timer_cb(struct timer_list *t)
{
	/* from_timer() 在 6.15 被改名为 timer_container_of()，直接用 container_of 可跨版本 */
	struct key_dev *k = container_of(t, struct key_dev, timer);

	dev_info(k->dev, "timer: count=%u\n", k->count);
}
#else
static void key_timer_cb_old(unsigned long data)
{
	struct key_dev *k = (struct key_dev *)data;

	dev_info(k->dev, "timer: count=%u\n", k->count);
}
#endif

static void key_work_fn(struct work_struct *work)
{
	struct key_dev *k = container_of(work, struct key_dev, work);

	dev_info(k->dev, "work: value=%d count=%u\n", k->value, k->count);
}

static irqreturn_t key_thread(int irq, void *data)
{
	struct key_dev *k = data;

	k->value = gpiod_get_value(k->gpiod);
	k->count++;
	wake_up_interruptible(&k->wq);
	mod_timer(&k->timer, jiffies + HZ / 50);
	schedule_work(&k->work);

	return IRQ_HANDLED;
}

static ssize_t key_read(struct file *file, char __user *buf,
			size_t size, loff_t *offset)
{
	struct key_dev *k = file->private_data;
	int v;

	if (size < sizeof(v))
		return -EINVAL;

	if (k->read_count == k->count) {
		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;
		if (wait_event_interruptible(k->wq, k->read_count != k->count))
			return -ERESTARTSYS;
	}

	k->read_count = k->count;
	v = k->value;
	if (copy_to_user(buf, &v, sizeof(v)))
		return -EFAULT;
	return sizeof(v);
}

static int key_open(struct inode *inode, struct file *file)
{
	file->private_data = the_key;
	return the_key ? 0 : -ENODEV;
}

static const struct file_operations key_fops = {
	.owner = THIS_MODULE,
	.open  = key_open,
	.read  = key_read,
};

static int key_probe(struct platform_device *pdev)
{
	struct key_dev *k;
	int err;

	k = devm_kzalloc(&pdev->dev, sizeof(*k), GFP_KERNEL);
	if (!k)
		return -ENOMEM;

	k->dev = &pdev->dev;
	k->gpiod = devm_gpiod_get(&pdev->dev, NULL, GPIOD_IN);
	if (IS_ERR(k->gpiod))
		return PTR_ERR(k->gpiod);

	k->irq = gpiod_to_irq(k->gpiod);
	if (k->irq < 0)
		return k->irq;

	init_waitqueue_head(&k->wq);
	INIT_WORK(&k->work, key_work_fn);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0)
	timer_setup(&k->timer, key_timer_cb, 0);
#else
	setup_timer(&k->timer, key_timer_cb_old, (unsigned long)k);
#endif

	err = request_threaded_irq(k->irq, key_hardirq, key_thread,
				   IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
				   DEV_NAME, k);
	if (err)
		return err;

	the_key = k;
	platform_set_drvdata(pdev, k);

	if (IS_ERR(device_create(key_class, NULL, MKDEV(major, 0),
				 NULL, "key_ctx"))) {
		free_irq(k->irq, k);
		the_key = NULL;
		return -ENODEV;
	}

	dev_info(&pdev->dev, "probe: irq=%d\n", k->irq);
	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 11, 0)
static void key_remove(struct platform_device *pdev)
#else
static int key_remove(struct platform_device *pdev)
#endif
{
	struct key_dev *k = platform_get_drvdata(pdev);

	/* 顺序：先停事件源，再释放引用 */
	disable_irq(k->irq);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 15, 0)
	timer_delete_sync(&k->timer);	/* 6.15 起 del_timer_sync() 的新名字 */
#else
	del_timer_sync(&k->timer);
#endif
	cancel_work_sync(&k->work);
	free_irq(k->irq, k);

	device_destroy(key_class, MKDEV(major, 0));
	the_key = NULL;

	dev_info(&pdev->dev, "remove: irq/timer/work released\n");
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0)
	return 0;
#endif
}

static const struct of_device_id key_of_match[] = {
	{ .compatible = "100ask,key_ctx" },
	{ }
};
MODULE_DEVICE_TABLE(of, key_of_match);

static struct platform_driver key_driver = {
	.probe  = key_probe,
	.remove = key_remove,
	.driver = {
		.name           = DEV_NAME,
		.of_match_table = key_of_match,
	},
};

static int __init key_ctx_init(void)
{
	int err;

	major = register_chrdev(0, DEV_NAME, &key_fops);
	if (major < 0)
		return major;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	key_class = class_create("key_ctx_class");
#else
	key_class = class_create(THIS_MODULE, "key_ctx_class");
#endif
	if (IS_ERR(key_class)) {
		err = PTR_ERR(key_class);
		unregister_chrdev(major, DEV_NAME);
		return err;
	}

	err = platform_driver_register(&key_driver);
	if (err) {
		class_destroy(key_class);
		unregister_chrdev(major, DEV_NAME);
	}
	return err;
}

static void __exit key_ctx_exit(void)
{
	platform_driver_unregister(&key_driver);
	class_destroy(key_class);
	unregister_chrdev(major, DEV_NAME);
}

module_init(key_ctx_init);
module_exit(key_ctx_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Day11 context rules and resource teardown");
