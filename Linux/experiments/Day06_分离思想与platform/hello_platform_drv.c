#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/version.h>

/*
 * Day6 驱动侧：只描述"怎么操作"，不硬编码硬件资源。
 *
 * 匹配依据：platform_device.name == platform_driver.driver.name
 */

#define DEV_NAME "hello_platform"

static int hello_probe(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "probe matched: %s\n", pdev->name);
	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 11, 0)
static void hello_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "remove\n");
}
#else
static int hello_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "remove\n");
	return 0;
}
#endif

static struct platform_driver hello_driver = {
	.probe  = hello_probe,
	.remove = hello_remove,
	.driver = {
		.name = DEV_NAME,
	},
};

static int __init hello_drv_init(void)
{
	int err;

	err = platform_driver_register(&hello_driver);
	pr_info("%s: driver register -> %d\n", DEV_NAME, err);
	return err;
}

static void __exit hello_drv_exit(void)
{
	platform_driver_unregister(&hello_driver);
	pr_info("%s: driver unregister\n", DEV_NAME);
}

module_init(hello_drv_init);
module_exit(hello_drv_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Day6 platform driver");
