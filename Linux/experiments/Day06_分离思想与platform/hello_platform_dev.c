#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>

/*
 * Day6 设备侧：只描述"有什么硬件"，不包含任何操作逻辑。
 *
 * 名字必须与驱动侧一致：hello_platform
 */

#define DEV_NAME "hello_platform"

static void hello_dev_release(struct device *dev)
{
	pr_info("%s: device released\n", DEV_NAME);
}

static struct platform_device hello_dev = {
	.name = DEV_NAME,
	.id   = -1,
	.dev  = {
		.release = hello_dev_release,
	},
};

static int __init hello_dev_init(void)
{
	int err;

	err = platform_device_register(&hello_dev);
	pr_info("%s: device register -> %d\n", DEV_NAME, err);
	return err;
}

static void __exit hello_dev_exit(void)
{
	platform_device_unregister(&hello_dev);
	pr_info("%s: device unregister\n", DEV_NAME);
}

module_init(hello_dev_init);
module_exit(hello_dev_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Day6 platform device");
