#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/fb.h>
#include <linux/gpio/consumer.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/string.h>
#include <linux/version.h>

/*
 * Day14：SSD1306 OLED 最小 Framebuffer 驱动
 *
 * 特点：
 *   - 128x64 单色，1bpp，显存 1024 字节
 *   - 命令/数据由 dc-gpios 区分
 *   - 通过 sysfs flush 手动触发整屏刷新（避免 defio 的复杂度）
 *
 * 显存布局：8 页 x 128 列，page-major
 */

#define DEV_NAME "oled_fb"
#define XRES 128
#define YRES 64
#define PAGES (YRES / 8)
#define FB_SIZE (XRES * PAGES)

static struct spi_device *oled_spi;
static struct gpio_desc *oled_dc;
static struct fb_info *oled_fb_info;

static void oled_cmd(u8 cmd)
{
	gpiod_set_value(oled_dc, 0);
	spi_write(oled_spi, &cmd, 1);
}

static void oled_data(const u8 *buf, int len)
{
	gpiod_set_value(oled_dc, 1);
	spi_write(oled_spi, buf, len);
}

static void oled_init_sequence(void)
{
	oled_cmd(0xAE);        /* display off */
	oled_cmd(0xD5); oled_cmd(0x80);
	oled_cmd(0xA8); oled_cmd(0x3F);
	oled_cmd(0xD3); oled_cmd(0x00);
	oled_cmd(0x40);
	oled_cmd(0x8D); oled_cmd(0x14);
	oled_cmd(0x20); oled_cmd(0x00);
	oled_cmd(0xA1);
	oled_cmd(0xC8);
	oled_cmd(0xDA); oled_cmd(0x12);
	oled_cmd(0x81); oled_cmd(0xCF);
	oled_cmd(0xD9); oled_cmd(0xF1);
	oled_cmd(0xDB); oled_cmd(0x40);
	oled_cmd(0xA4);
	oled_cmd(0xA6);
	oled_cmd(0xAF);        /* display on */
}

static void oled_flush(void)
{
	const u8 *p = oled_fb_info->screen_base;
	int page;

	for (page = 0; page < PAGES; page++) {
		oled_cmd(0xB0 + page);   /* set page */
		oled_cmd(0x00);          /* low column */
		oled_cmd(0x10);          /* high column */
		oled_data(p + page * XRES, XRES);
	}
}

static ssize_t flush_store(struct device *dev, struct device_attribute *attr,
			   const char *buf, size_t count)
{
	oled_flush();
	dev_info(dev, "flush %d bytes\n", FB_SIZE);
	return count;
}
static DEVICE_ATTR_WO(flush);

/*
 * 没有 fb_ops，/dev/fb0 就无法 read/write，register_framebuffer 也会被 fbcon 拒绝。
 * 显存在 CPU 可直接访问的内存中，因此使用 sys_* 系列通用实现。
 */
static const struct fb_ops oled_fb_ops = {
	.owner        = THIS_MODULE,
	.fb_read      = fb_sys_read,
	.fb_write     = fb_sys_write,
	.fb_fillrect  = sys_fillrect,
	.fb_copyarea  = sys_copyarea,
	.fb_imageblit = sys_imageblit,
};

static int oled_probe(struct spi_device *spi)
{
	dma_addr_t phy;
	int err;

	oled_spi = spi;

	oled_dc = devm_gpiod_get(&spi->dev, "dc", GPIOD_OUT_LOW);
	if (IS_ERR(oled_dc)) {
		dev_err(&spi->dev, "failed to get dc gpio: %ld\n",
			PTR_ERR(oled_dc));
		return PTR_ERR(oled_dc);
	}

	oled_fb_info = framebuffer_alloc(0, &spi->dev);
	if (!oled_fb_info)
		return -ENOMEM;

	oled_fb_info->var.xres = XRES;
	oled_fb_info->var.yres = YRES;
	oled_fb_info->var.xres_virtual = XRES;
	oled_fb_info->var.yres_virtual = YRES;
	oled_fb_info->var.bits_per_pixel = 1;
	oled_fb_info->var.vmode = FB_VMODE_NONINTERLACED;

	strscpy(oled_fb_info->fix.id, DEV_NAME,
		sizeof(oled_fb_info->fix.id));
	oled_fb_info->fix.smem_len = FB_SIZE;
	oled_fb_info->fix.type = FB_TYPE_PACKED_PIXELS;
	oled_fb_info->fix.visual = FB_VISUAL_MONO10;
	oled_fb_info->fix.line_length = XRES / 8;
	oled_fb_info->fbops = &oled_fb_ops;

	/* 必须传入真实设备：dma_alloc_wc(NULL, ...) 会在内部解引用 dev 而崩溃 */
	oled_fb_info->screen_base = dma_alloc_wc(&spi->dev, FB_SIZE, &phy,
						 GFP_KERNEL);
	if (!oled_fb_info->screen_base) {
		err = -ENOMEM;
		goto err_release;
	}
	memset(oled_fb_info->screen_base, 0, FB_SIZE);
	oled_fb_info->fix.smem_start = phy;

	oled_init_sequence();

	err = register_framebuffer(oled_fb_info);
	if (err)
		goto err_free;

	err = device_create_file(oled_fb_info->dev, &dev_attr_flush);
	if (err)
		goto err_unregister;

	dev_info(&spi->dev, "probe: %dx%d %dbpp\n",
		 XRES, YRES, oled_fb_info->var.bits_per_pixel);
	return 0;

err_unregister:
	unregister_framebuffer(oled_fb_info);
err_free:
	dma_free_wc(&spi->dev, FB_SIZE, oled_fb_info->screen_base,
		    oled_fb_info->fix.smem_start);
err_release:
	framebuffer_release(oled_fb_info);
	oled_fb_info = NULL;
	return err;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 11, 0)
static void oled_remove(struct spi_device *spi)
#else
static int oled_remove(struct spi_device *spi)
#endif
{
	device_remove_file(oled_fb_info->dev, &dev_attr_flush);
	unregister_framebuffer(oled_fb_info);
	dma_free_wc(&spi->dev, FB_SIZE, oled_fb_info->screen_base,
		    oled_fb_info->fix.smem_start);
	framebuffer_release(oled_fb_info);
	oled_fb_info = NULL;
	dev_info(&spi->dev, "remove\n");
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0)
	return 0;
#endif
}

static const struct of_device_id oled_of_match[] = {
	{ .compatible = "100ask,oled" },
	{ }
};
MODULE_DEVICE_TABLE(of, oled_of_match);

static struct spi_driver oled_driver = {
	.probe  = oled_probe,
	.remove = oled_remove,
	.driver = {
		.name           = DEV_NAME,
		.of_match_table = oled_of_match,
	},
};

module_spi_driver(oled_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Day14 minimal SSD1306 framebuffer driver");
