#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/version.h>

/*
 * Day8 驱动侧：通过 compatible 匹配设备树节点，并读取自定义属性。
 *
 * dts 侧的 compatible 必须与 of_device_id 完全一致。
 */

#define DEV_NAME "led_dts"

static int led_probe(struct platform_device *pdev)
{
	u32 pin;
	int err;

	if (!pdev->dev.of_node) {
		dev_err(&pdev->dev, "no device tree node\n");
		return -EINVAL;
	}

	err = of_property_read_u32(pdev->dev.of_node, "pin", &pin);
	if (err) {
		dev_err(&pdev->dev, "failed to read 'pin': %d\n", err);
		return err;
	}

	dev_info(&pdev->dev, "probe: pin=%u\n", pin);
	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 11, 0)
static void led_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "remove\n");
}
#else
static int led_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "remove\n");
	return 0;
}
#endif

static const struct of_device_id led_of_match[] = {
	{ .compatible = "100ask,led" },
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

module_platform_driver(led_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Day8 device tree matching example");
