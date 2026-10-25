#include <linux/device.h>
#include <linux/fs.h>
#include <linux/gpio/consumer.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/wait.h>

/*
 * Day12：SR501 人体红外驱动（完善版）
 *
 * 数据流：
 *   人体移动 -> SR501 OUT 电平变化 -> GPIO 中断
 *   -> 硬中断返回 IRQ_WAKE_THREAD
 *   -> 线程读电平、计数、唤醒等待队列
 *   -> 应用 read/poll 返回
 *
 * 设备树：
 *   sr501 {
 *       compatible = "100ask,sr501";
 *       gpios = <&gpio4 19 GPIO_ACTIVE_HIGH>;
 *   };
 */

#define DEV_NAME "sr501"

struct sr501_dev {
	struct gpio_desc *gpiod;
	int irq;
	int value;
	unsigned int events;
	unsigned int read_events;
	wait_queue_head_t wq;
};

static int major;
static struct class *sr501_class;
static struct sr501_dev *the_sr501;

static irqreturn_t sr501_hardirq(int irq, void *data)
{
	return IRQ_WAKE_THREAD;
}

static irqreturn_t sr501_thread(int irq, void *data)
{
	struct sr501_dev *s = data;

	s->value = gpiod_get_value(s->gpiod);
	s->events++;
	wake_up_interruptible(&s->wq);

	return IRQ_HANDLED;
}

static ssize_t sr501_read(struct file *file, char __user *buf,
			  size_t size, loff_t *offset)
{
	struct sr501_dev *s = file->private_data;
	int v;

	if (size < sizeof(v))
		return -EINVAL;

	if (s->read_events == s->events) {
		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;
		if (wait_event_interruptible(s->wq,
					     s->read_events != s->events))
			return -ERESTARTSYS;
	}

	s->read_events = s->events;
	v = s->value;
	if (copy_to_user(buf, &v, sizeof(v)))
		return -EFAULT;

	return sizeof(v);
}

static unsigned int sr501_poll(struct file *file, poll_table *wait)
{
	struct sr501_dev *s = file->private_data;
	unsigned int mask = 0;

	poll_wait(file, &s->wq, wait);
	if (s->read_events != s->events)
		mask |= POLLIN | POLLRDNORM;

	return mask;
}

static int sr501_open(struct inode *inode, struct file *file)
{
	file->private_data = the_sr501;
	return the_sr501 ? 0 : -ENODEV;
}

static const struct file_operations sr501_fops = {
	.owner = THIS_MODULE,
	.open  = sr501_open,
	.read  = sr501_read,
	.poll  = sr501_poll,
};

static int sr501_probe(struct platform_device *pdev)
{
	struct sr501_dev *s;
	int err;

	s = devm_kzalloc(&pdev->dev, sizeof(*s), GFP_KERNEL);
	if (!s)
		return -ENOMEM;

	/* 属性名是 gpios，因此 con_id 传 NULL */
	s->gpiod = devm_gpiod_get(&pdev->dev, NULL, GPIOD_IN);
	if (IS_ERR(s->gpiod)) {
		dev_err(&pdev->dev, "failed to get gpio: %ld\n",
			PTR_ERR(s->gpiod));
		return PTR_ERR(s->gpiod);
	}

	s->irq = gpiod_to_irq(s->gpiod);
	if (s->irq < 0) {
		dev_err(&pdev->dev, "gpiod_to_irq failed: %d\n", s->irq);
		return s->irq;
	}

	init_waitqueue_head(&s->wq);

	err = request_threaded_irq(s->irq, sr501_hardirq, sr501_thread,
				   IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
				   DEV_NAME, s);
	if (err) {
		dev_err(&pdev->dev, "request_threaded_irq failed: %d\n", err);
		return err;
	}

	the_sr501 = s;
	platform_set_drvdata(pdev, s);

	if (IS_ERR(device_create(sr501_class, NULL, MKDEV(major, 0),
				 NULL, DEV_NAME))) {
		free_irq(s->irq, s);
		the_sr501 = NULL;
		return -ENODEV;
	}

	dev_info(&pdev->dev, "probe: irq=%d\n", s->irq);
	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 11, 0)
static void sr501_remove(struct platform_device *pdev)
#else
static int sr501_remove(struct platform_device *pdev)
#endif
{
	struct sr501_dev *s = platform_get_drvdata(pdev);

	free_irq(s->irq, s);
	device_destroy(sr501_class, MKDEV(major, 0));
	the_sr501 = NULL;

	dev_info(&pdev->dev, "remove\n");
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0)
	return 0;
#endif
}

static const struct of_device_id sr501_of_match[] = {
	{ .compatible = "100ask,sr501" },
	{ }
};
MODULE_DEVICE_TABLE(of, sr501_of_match);

static struct platform_driver sr501_driver = {
	.probe  = sr501_probe,
	.remove = sr501_remove,
	.driver = {
		.name           = DEV_NAME,
		.of_match_table = sr501_of_match,
	},
};

static int __init sr501_init(void)
{
	int err;

	major = register_chrdev(0, DEV_NAME, &sr501_fops);
	if (major < 0)
		return major;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	sr501_class = class_create("sr501_class");
#else
	sr501_class = class_create(THIS_MODULE, "sr501_class");
#endif
	if (IS_ERR(sr501_class)) {
		err = PTR_ERR(sr501_class);
		unregister_chrdev(major, DEV_NAME);
		return err;
	}

	err = platform_driver_register(&sr501_driver);
	if (err) {
		class_destroy(sr501_class);
		unregister_chrdev(major, DEV_NAME);
	}
	return err;
}

static void __exit sr501_exit(void)
{
	platform_driver_unregister(&sr501_driver);
	class_destroy(sr501_class);
	unregister_chrdev(major, DEV_NAME);
}

module_init(sr501_init);
module_exit(sr501_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Day12 SR501 PIR sensor driver");
