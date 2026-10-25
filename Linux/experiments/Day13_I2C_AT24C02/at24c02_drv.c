#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/version.h>

/*
 * Day13：AT24C02 I2C EEPROM 驱动（教学版）
 *
 * 关键点：
 *   - 随机读必须"先写存储地址，再重复起始读"
 *   - i2c_transfer 返回成功传输的消息数，不是字节数
 *   - EEPROM 写后需要等待内部写周期
 */

#define DEV_NAME "at24c02"
#define EEPROM_SIZE 8

static int major;
static struct class *at24c02_class;
static struct i2c_client *at24c02_client;
static DEFINE_MUTEX(at24c02_lock);

static int at24c02_write_bytes(struct i2c_client *c, u8 addr,
			       const u8 *buf, int len)
{
	u8 tmp[1 + EEPROM_SIZE];
	struct i2c_msg msg = {
		.addr  = c->addr,
		.flags = 0,
		.len   = len + 1,
		.buf   = tmp,
	};
	int ret;

	if (len <= 0 || len > EEPROM_SIZE)
		return -EINVAL;

	tmp[0] = addr;
	memcpy(&tmp[1], buf, len);

	ret = i2c_transfer(c->adapter, &msg, 1);
	if (ret != 1)
		return ret < 0 ? ret : -EIO;

	/* 等待 EEPROM 内部写周期 */
	msleep(10);
	return 0;
}

static int at24c02_read_bytes(struct i2c_client *c, u8 addr,
			      u8 *buf, int len)
{
	u8 mem_addr = addr;
	struct i2c_msg msgs[2] = {
		{
			.addr  = c->addr,
			.flags = 0,
			.len   = 1,
			.buf   = &mem_addr,
		},
		{
			.addr  = c->addr,
			.flags = I2C_M_RD,
			.len   = len,
			.buf   = buf,
		},
	};
	int ret;

	ret = i2c_transfer(c->adapter, msgs, 2);
	if (ret != 2)
		return ret < 0 ? ret : -EIO;

	return 0;
}

static ssize_t at24c02_read(struct file *file, char __user *buf,
			    size_t size, loff_t *offset)
{
	u8 tmp[EEPROM_SIZE];
	int ret;

	if (size > EEPROM_SIZE)
		size = EEPROM_SIZE;

	mutex_lock(&at24c02_lock);
	ret = at24c02_read_bytes(at24c02_client, 0, tmp, size);
	mutex_unlock(&at24c02_lock);
	if (ret)
		return ret;

	if (copy_to_user(buf, tmp, size))
		return -EFAULT;

	return size;
}

static ssize_t at24c02_write(struct file *file, const char __user *buf,
			     size_t size, loff_t *offset)
{
	u8 tmp[EEPROM_SIZE];
	int ret;

	if (size > EEPROM_SIZE)
		size = EEPROM_SIZE;

	if (copy_from_user(tmp, buf, size))
		return -EFAULT;

	mutex_lock(&at24c02_lock);
	ret = at24c02_write_bytes(at24c02_client, 0, tmp, size);
	mutex_unlock(&at24c02_lock);
	if (ret)
		return ret;

	return size;
}

static const struct file_operations at24c02_fops = {
	.owner = THIS_MODULE,
	.read  = at24c02_read,
	.write = at24c02_write,
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 3, 0)
static int at24c02_probe(struct i2c_client *client)
#else
static int at24c02_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
#endif
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 3, 0)
	(void)id;
#endif

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "adapter does not support plain I2C\n");
		return -EOPNOTSUPP;
	}

	at24c02_client = client;
	/* I2C 设备的驱动数据应走 i2c_set_clientdata，而不是 platform_set_drvdata */
	i2c_set_clientdata(client, client);

	if (IS_ERR(device_create(at24c02_class, &client->dev,
				 MKDEV(major, 0), NULL, DEV_NAME)))
		return -ENODEV;

	dev_info(&client->dev, "probe: addr=0x%02x\n", client->addr);
	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 11, 0)
static void at24c02_remove(struct i2c_client *client)
#else
static int at24c02_remove(struct i2c_client *client)
#endif
{
	device_destroy(at24c02_class, MKDEV(major, 0));
	i2c_set_clientdata(client, NULL);
	at24c02_client = NULL;
	dev_info(&client->dev, "remove\n");
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0)
	return 0;
#endif
}

static const struct of_device_id at24c02_of_match[] = {
	{ .compatible = "100ask,at24c02" },
	{ }
};
MODULE_DEVICE_TABLE(of, at24c02_of_match);

static const struct i2c_device_id at24c02_ids[] = {
	{ DEV_NAME, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, at24c02_ids);

static struct i2c_driver at24c02_driver = {
	.probe  = at24c02_probe,
	.remove = at24c02_remove,
	.driver = {
		.name           = DEV_NAME,
		.of_match_table = at24c02_of_match,
	},
	.id_table = at24c02_ids,
};

static int __init at24c02_init(void)
{
	int err;

	major = register_chrdev(0, DEV_NAME, &at24c02_fops);
	if (major < 0)
		return major;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	at24c02_class = class_create("at24c02_class");
#else
	at24c02_class = class_create(THIS_MODULE, "at24c02_class");
#endif
	if (IS_ERR(at24c02_class)) {
		err = PTR_ERR(at24c02_class);
		unregister_chrdev(major, DEV_NAME);
		return err;
	}

	err = i2c_add_driver(&at24c02_driver);
	if (err) {
		class_destroy(at24c02_class);
		unregister_chrdev(major, DEV_NAME);
	}
	return err;
}

static void __exit at24c02_exit(void)
{
	i2c_del_driver(&at24c02_driver);
	class_destroy(at24c02_class);
	unregister_chrdev(major, DEV_NAME);
}

module_init(at24c02_init);
module_exit(at24c02_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Day13 AT24C02 I2C EEPROM driver");
