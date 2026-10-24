#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>

/*
 * Day7 设备侧：注册两个实例，用 platform_data 传递引脚号。
 *
 * 相比课程用 IORESOURCE_IRQ 塞引脚号，platform_data 语义更清晰。
 */

#define DEV_NAME "led_multi"

struct led_pdata {
	int pin;
};

static struct led_pdata led0_pdata = { .pin = 3 };
static struct led_pdata led1_pdata = { .pin = 5 };

static void led_dev_release(struct device *dev)
{
	pr_info("%s: device %s released\n", DEV_NAME, dev_name(dev));
}

static struct platform_device led_devs[] = {
	{
		.name = DEV_NAME,
		.id   = 0,
		.dev  = {
			.platform_data = &led0_pdata,
			.release       = led_dev_release,
		},
	},
	{
		.name = DEV_NAME,
		.id   = 1,
		.dev  = {
			.platform_data = &led1_pdata,
			.release       = led_dev_release,
		},
	},
};

static int __init led_dev_init(void)
{
	int i, err;

	for (i = 0; i < ARRAY_SIZE(led_devs); i++) {
		err = platform_device_register(&led_devs[i]);
		if (err) {
			pr_err("%s: register device %d failed: %d\n",
			       DEV_NAME, i, err);
			while (--i >= 0)
				platform_device_unregister(&led_devs[i]);
			return err;
		}
	}

	pr_info("%s: %zu device(s) registered\n", DEV_NAME, ARRAY_SIZE(led_devs));
	return 0;
}

static void __exit led_dev_exit(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(led_devs); i++)
		platform_device_unregister(&led_devs[i]);

	pr_info("%s: devices unregistered\n", DEV_NAME);
}

module_init(led_dev_init);
module_exit(led_dev_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Day7 multi-instance platform devices");
