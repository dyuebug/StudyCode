#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/version.h>

/*
 * Day7 驱动侧：一个驱动服务多个设备实例。
 *
 * 关键点：
 *   - 每个实例有独立的次设备号（pdev->id）
 *   - 每个实例的私有数据通过 platform_set_drvdata 保存
 *   - probe / remove 成对执行
 */

#define DEV_NAME "led_multi"
#define LED_MAX_CNT 4

/* 与设备侧 led_platform_dev.c 中的布局保持一致 */
struct led_pdata {
	int pin;
};

struct led_desc {
	int pin;
	int minor;
};

static int major;
static struct class *led_class;
static struct led_desc descs[LED_MAX_CNT];

static ssize_t led_write(struct file *file, const char __user *buf,
			 size_t size, loff_t *offset)
{
	struct inode *inode = file_inode(file);
	int minor = iminor(inode);
	unsigned char val;

	if (size < 1)
		return 0;
	if (copy_from_user(&val, buf, 1))
		return -EFAULT;

	if (minor >= LED_MAX_CNT || descs[minor].minor != minor)
		return -ENODEV;

	pr_info("%s: minor=%d pin=%d value=%u\n",
		DEV_NAME, minor, descs[minor].pin, val);
	return 1;
}

static struct file_operations led_fops = {
	.owner = THIS_MODULE,
	.write = led_write,
};

static int led_probe(struct platform_device *pdev)
{
	struct led_desc *desc;
	struct led_pdata *pdata = dev_get_platdata(&pdev->dev);
	int minor;

	if (!pdata) {
		dev_err(&pdev->dev, "missing platform_data\n");
		return -EINVAL;
	}

	minor = pdev->id;
	if (minor < 0 || minor >= LED_MAX_CNT) {
		dev_err(&pdev->dev, "invalid id %d\n", minor);
		return -EINVAL;
	}

	desc = &descs[minor];
	desc->pin = pdata->pin;
	desc->minor = minor;

	platform_set_drvdata(pdev, desc);

	if (IS_ERR(device_create(led_class, NULL, MKDEV(major, minor),
				 NULL, "led%d", minor)))
		return -ENODEV;

	dev_info(&pdev->dev, "probe: minor=%d pin=%d\n", minor, desc->pin);
	return 0;
}

static int led_remove(struct platform_device *pdev)
{
	struct led_desc *desc = platform_get_drvdata(pdev);

	if (desc) {
		device_destroy(led_class, MKDEV(major, desc->minor));
		dev_info(&pdev->dev, "remove: minor=%d\n", desc->minor);
	}
	return 0;
}

static struct platform_driver led_driver = {
	.probe  = led_probe,
	.remove = led_remove,
	.driver = {
		.name = DEV_NAME,
	},
};

static int __init led_init(void)
{
	int err;

	major = register_chrdev(0, DEV_NAME, &led_fops);
	if (major < 0)
		return major;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	led_class = class_create("led_class");
#else
	led_class = class_create(THIS_MODULE, "led_class");
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
MODULE_DESCRIPTION("Day7 single driver serving multiple platform devices");
