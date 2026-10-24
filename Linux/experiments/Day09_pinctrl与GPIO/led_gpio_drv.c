#include <linux/device.h>
#include <linux/fs.h>
#include <linux/gpio/consumer.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/version.h>

/*
 * Day9 GPIO 消费者驱动
 *
 * 与 Day5 的差别：
 *   - 不出现任何物理地址
 *   - 引脚、复用、有效电平全部来自设备树
 *   - 使用 gpiolib 统一接口
 */

#define DEV_NAME "led_gpio"

struct led_gpio {
	struct gpio_desc *led;
	int minor;
};

static int major;
static struct class *led_class;
static struct led_gpio *the_led;

static ssize_t led_write(struct file *file, const char __user *buf,
			 size_t size, loff_t *offset)
{
	struct led_gpio *lg = file->private_data;
	unsigned char val;

	if (!lg || size < 1)
		return -EINVAL;
	if (copy_from_user(&val, buf, 1))
		return -EFAULT;

	gpiod_set_value(lg->led, !!val);
	pr_info("%s: set logical value %u\n", DEV_NAME, !!val);
	return 1;
}

static int led_open(struct inode *inode, struct file *file)
{
	if (!the_led)
		return -ENODEV;
	file->private_data = the_led;
	return 0;
}

static struct file_operations led_fops = {
	.owner = THIS_MODULE,
	.open  = led_open,
	.write = led_write,
};

static int led_probe(struct platform_device *pdev)
{
	struct led_gpio *lg;

	lg = devm_kzalloc(&pdev->dev, sizeof(*lg), GFP_KERNEL);
	if (!lg)
		return -ENOMEM;

	/* 查找 led-gpios 属性；con_id 为 "led" */
	lg->led = devm_gpiod_get(&pdev->dev, "led", GPIOD_OUT_LOW);
	if (IS_ERR(lg->led)) {
		dev_err(&pdev->dev, "failed to get led gpio: %ld\n",
			PTR_ERR(lg->led));
		return PTR_ERR(lg->led);
	}

	lg->minor = 0;
	the_led = lg;
	platform_set_drvdata(pdev, lg);

	if (IS_ERR(device_create(led_class, NULL, MKDEV(major, 0),
				 NULL, "myled_gpio"))) {
		the_led = NULL;
		return -ENODEV;
	}

	dev_info(&pdev->dev, "probe: gpio acquired\n");
	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 11, 0)
static void led_remove(struct platform_device *pdev)
{
	device_destroy(led_class, MKDEV(major, 0));
	the_led = NULL;
	dev_info(&pdev->dev, "remove\n");
}
#else
static int led_remove(struct platform_device *pdev)
{
	device_destroy(led_class, MKDEV(major, 0));
	the_led = NULL;
	dev_info(&pdev->dev, "remove\n");
	return 0;
}
#endif

static const struct of_device_id led_of_match[] = {
	{ .compatible = "100ask,led-gpio" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, led_of_match);

static struct platform_driver led_driver = {
	.probe  = led_probe,
	.remove = led_remove,
	.driver = {
		.name           = DEV_NAME,
		.of_match_table = led_of_match,
	},
};

static int __init led_init(void)
{
	int err;

	major = register_chrdev(0, DEV_NAME, &led_fops);
	if (major < 0)
		return major;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	led_class = class_create("led_gpio_class");
#else
	led_class = class_create(THIS_MODULE, "led_gpio_class");
#endif
	if (IS_ERR(led_class)) {
		err = PTR_ERR(led_class);
		unregister_chrdev(major, DEV_NAME);
		return err;
	}

	err = platform_driver_register(&led_driver);
	if (err) {
		class_destroy(led_class);
		unregister_chrdev(major, DEV_NAME);
		return err;
	}

	return 0;
}

static void __exit led_exit(void)
{
	platform_driver_unregister(&led_driver);
	class_destroy(led_class);
	unregister_chrdev(major, DEV_NAME);
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Day9 GPIO consumer via pinctrl and gpiolib");
