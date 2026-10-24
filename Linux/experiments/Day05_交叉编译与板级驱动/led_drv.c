#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <asm/io.h>

/*
 * Day5 板级 LED 驱动（i.MX6ULL）
 *
 * 硬件假设：
 *   LED 接在 GPIO5_IO03
 *   引脚复用寄存器 SNVS_TAMPER3: 0x02290000 + 0x14
 *   GPIO5_GDIR: 0x020AC004
 *   GPIO5_DR  : 0x020AC000
 *
 * 其他板卡的地址不同，不能直接复用。
 */

#define DEV_NAME "100ask_led"

static int major;
static struct class *led_class;

static void __iomem *iomux_snvs_tamper3;
static void __iomem *gpio5_gdir;
static void __iomem *gpio5_dr;

static ssize_t led_write(struct file *filp, const char __user *buf,
			 size_t count, loff_t *ppos)
{
	unsigned char val;

	if (count < 1)
		return 0;

	if (copy_from_user(&val, buf, 1))
		return -EFAULT;

	if (val)
		writel(readl(gpio5_dr) & ~(1 << 3), gpio5_dr);
	else
		writel(readl(gpio5_dr) | (1 << 3), gpio5_dr);

	return 1;
}

static int led_open(struct inode *inode, struct file *filp)
{
	/* 复用为 GPIO 模式 */
	writel((readl(iomux_snvs_tamper3) & ~0xf) | 0x5, iomux_snvs_tamper3);

	/* 配置为输出 */
	writel(readl(gpio5_gdir) | (1 << 3), gpio5_gdir);

	return 0;
}

static struct file_operations led_fops = {
	.owner = THIS_MODULE,
	.open  = led_open,
	.write = led_write,
};

static int __init led_init(void)
{
	int err;

	major = register_chrdev(0, DEV_NAME, &led_fops);
	if (major < 0) {
		pr_err("%s: register_chrdev failed: %d\n", DEV_NAME, major);
		return major;
	}

	iomux_snvs_tamper3 = ioremap(0x02290000 + 0x14, 4);
	gpio5_gdir         = ioremap(0x020AC004, 4);
	gpio5_dr           = ioremap(0x020AC000, 4);

	if (!iomux_snvs_tamper3 || !gpio5_gdir || !gpio5_dr) {
		err = -ENOMEM;
		goto err_unmap;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	led_class = class_create("myled");
#else
	led_class = class_create(THIS_MODULE, "myled");
#endif
	if (IS_ERR(led_class)) {
		err = PTR_ERR(led_class);
		goto err_unmap;
	}

	if (IS_ERR(device_create(led_class, NULL, MKDEV(major, 0), NULL, "myled"))) {
		err = -ENODEV;
		goto err_class;
	}

	pr_info("%s: init, major = %d, /dev/myled ready\n", DEV_NAME, major);
	return 0;

err_class:
	class_destroy(led_class);
err_unmap:
	if (iomux_snvs_tamper3)
		iounmap(iomux_snvs_tamper3);
	if (gpio5_gdir)
		iounmap(gpio5_gdir);
	if (gpio5_dr)
		iounmap(gpio5_dr);
	unregister_chrdev(major, DEV_NAME);
	return err;
}

static void __exit led_exit(void)
{
	device_destroy(led_class, MKDEV(major, 0));
	class_destroy(led_class);

	iounmap(iomux_snvs_tamper3);
	iounmap(gpio5_gdir);
	iounmap(gpio5_dr);

	unregister_chrdev(major, DEV_NAME);
	pr_info("%s: exit\n", DEV_NAME);
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Day5 board-level LED driver for i.MX6ULL GPIO5_IO03");
