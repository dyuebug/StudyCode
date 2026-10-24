#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

/*
 * Day4 用户拷贝示例（修正版）
 *
 * 与课程 05_transfer_data 的差别：
 *   1. 按 size 截断，避免越界写用户缓冲区
 *   2. 检查 copy_*_user 的返回值
 *   3. 返回实际拷贝的字节数
 */

#define DEV_NAME "hello_copy"

static int major;
static int ker_val = 123;

module_param(ker_val, int, 0644);
MODULE_PARM_DESC(ker_val, "value exposed through /dev/hello_copy");

static ssize_t hello_read(struct file *file, char __user *buf,
			  size_t size, loff_t *offset)
{
	size_t n;

	if (size == 0)
		return 0;

	n = min(size, sizeof(ker_val));
	if (copy_to_user(buf, &ker_val, n))
		return -EFAULT;

	pr_info("%s: read %zu byte(s), ker_val=%d\n", DEV_NAME, n, ker_val);
	return n;
}

static ssize_t hello_write(struct file *file, const char __user *buf,
			   size_t size, loff_t *offset)
{
	size_t n;

	if (size == 0)
		return 0;

	n = min(size, sizeof(ker_val));
	if (copy_from_user(&ker_val, buf, n))
		return -EFAULT;

	pr_info("%s: write %zu byte(s), ker_val=%d\n", DEV_NAME, n, ker_val);
	return n;
}

static struct file_operations hello_fops = {
	.owner = THIS_MODULE,
	.read  = hello_read,
	.write = hello_write,
};

static int __init hello_init(void)
{
	major = register_chrdev(0, DEV_NAME, &hello_fops);
	if (major < 0) {
		pr_err("%s: register_chrdev failed: %d\n", DEV_NAME, major);
		return major;
	}

	pr_info("%s: init, major = %d\n", DEV_NAME, major);
	pr_info("%s: next: sudo mknod /dev/hello_copy c %d 0\n",
		DEV_NAME, major);
	return 0;
}

static void __exit hello_exit(void)
{
	unregister_chrdev(major, DEV_NAME);
	pr_info("%s: exit\n", DEV_NAME);
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Day4 user/kernel copy example");
