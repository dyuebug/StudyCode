// SPDX-License-Identifier: GPL-2.0
/*
 * Day16 设备侧：platform_device + sysfs 参数
 *
 * 与 Day06 的唯一差别是驱动侧多挂了一组 sysfs 属性。
 * 设备侧仍然只描述"有什么硬件"，不含任何逻辑。
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#define DEV_NAME "param_demo"

static void param_dev_release(struct device *dev)
{
	pr_info("%s: device released\n", DEV_NAME);
}

static struct platform_device param_dev = {
	.name = DEV_NAME,
	.id   = -1,
	.dev  = {
		.release = param_dev_release,
	},
};

static int __init param_dev_init(void)
{
	int err;

	err = platform_device_register(&param_dev);
	pr_info("%s: device register -> %d\n", DEV_NAME, err);
	return err;
}

static void __exit param_dev_exit(void)
{
	platform_device_unregister(&param_dev);
	pr_info("%s: device unregister\n", DEV_NAME);
}

module_init(param_dev_init);
module_exit(param_dev_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Day16 sysfs param demo device");
