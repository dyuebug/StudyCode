#include <linux/device.h>
#include <linux/fs.h>
#include <linux/gpio/consumer.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/wait.h>

/*
 * Day10：中断 + 等待队列 + poll + ioctl
 *
 * 设备树：
 *   key {
 *       compatible = "100ask,gpio_key";
 *       gpios = <&gpio4 19 GPIO_ACTIVE_LOW>;
 *   };
 */

#define DEV_NAME "gpio_key_irq"
#define KEY_IOC_CLEAR _IO('k', 0)

struct key_dev {
	struct gpio_desc *gpiod;
	int irq;
	int value;
	unsigned int count;
	unsigned int read_count;
	wait_queue_head_t wq;
};

static int major;
static struct class *key_class;
static struct key_dev *the_key;

static irqreturn_t key_isr(int irq, void *data)
{
	struct key_dev *k = data;

	k->value = gpiod_get_value(k->gpiod);
	k->count++;
	wake_up_interruptible(&k->wq);
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

static unsigned int key_poll(struct file *file, poll_table *wait)
{
	struct key_dev *k = file->private_data;
	unsigned int mask = 0;

	poll_wait(file, &k->wq, wait);
	if (k->read_count != k->count)
		mask |= POLLIN | POLLRDNORM;
	return mask;
}

static long key_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct key_dev *k = file->private_data;

	switch (cmd) {
	case KEY_IOC_CLEAR:
		k->count = 0;
		k->read_count = 0;
		return 0;
	default:
		return -ENOTTY;
	}
}

static int key_open(struct inode *inode, struct file *file)
{
	file->private_data = the_key;
	return the_key ? 0 : -ENODEV;
}

static const struct file_operations key_fops = {
	.owner          = THIS_MODULE,
	.open           = key_open,
	.read           = key_read,
	.poll           = key_poll,
	.unlocked_ioctl = key_ioctl,
};

static int key_probe(struct platform_device *pdev)
{
	struct key_dev *k;
	int err;

	k = devm_kzalloc(&pdev->dev, sizeof(*k), GFP_KERNEL);
	if (!k)
		return -ENOMEM;

	k->gpiod = devm_gpiod_get(&pdev->dev, NULL, GPIOD_IN);
	if (IS_ERR(k->gpiod))
		return PTR_ERR(k->gpiod);

	k->irq = gpiod_to_irq(k->gpiod);
	if (k->irq < 0)
		return k->irq;

	init_waitqueue_head(&k->wq);

	/* 双边沿：按下与松开都产生事件 */
	err = request_irq(k->irq, key_isr,
			  IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			  DEV_NAME, k);
	if (err)
		return err;

	the_key = k;
	platform_set_drvdata(pdev, k);

	if (IS_ERR(device_create(key_class, NULL, MKDEV(major, 0),
				 NULL, "myk")))
		return -ENODEV;

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

	free_irq(k->irq, k);
	device_destroy(key_class, MKDEV(major, 0));
	the_key = NULL;
	dev_info(&pdev->dev, "remove\n");
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0)
	return 0;
#endif
}

static const struct of_device_id key_of_match[] = {
	{ .compatible = "100ask,gpio_key" },
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

static int __init key_init(void)
{
	int err;

	major = register_chrdev(0, DEV_NAME, &key_fops);
	if (major < 0)
		return major;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	key_class = class_create("key_class");
#else
	key_class = class_create(THIS_MODULE, "key_class");
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

static void __exit key_exit(void)
{
	platform_driver_unregister(&key_driver);
	class_destroy(key_class);
	unregister_chrdev(major, DEV_NAME);
}

module_init(key_init);
module_exit(key_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Day10 IRQ + waitqueue + poll + ioctl");
