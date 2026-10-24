#include <linux/module.h>
#include <linux/fs.h>

/*
 * Day3 字符设备驱动：只观察调用链，不做数据拷贝。
 *
 * read/write 返回 1 但没有调用 copy_to_user/copy_from_user，
 * 因此用户态缓冲区内容不会改变。数据拷贝在 Day4 讲解。
 */

#define DEV_NAME "hello_chrdev"

static int major;

static int hello_open(struct inode *inode, struct file *file)
{
	pr_info("%s: open\n", DEV_NAME);
	return 0;
}

static int hello_release(struct inode *inode, struct file *file)
{
	pr_info("%s: release\n", DEV_NAME);
	return 0;
}

static ssize_t hello_read(struct file *file, char __user *buf,
			  size_t size, loff_t *offset)
{
	pr_info("%s: read size=%zu\n", DEV_NAME, size);
	return 1;
}

static ssize_t hello_write(struct file *file, const char __user *buf,
			   size_t size, loff_t *offset)
{
	pr_info("%s: write size=%zu\n", DEV_NAME, size);
	return 1;
}

static struct file_operations hello_fops = {
	.owner   = THIS_MODULE,
	.open    = hello_open,
	.release = hello_release,
	.read    = hello_read,
	.write   = hello_write,
};

static int __init hello_init(void)
{
	major = register_chrdev(0, DEV_NAME, &hello_fops);
	if (major < 0) {
		pr_err("%s: register_chrdev failed: %d\n", DEV_NAME, major);
		return major;
	}

	pr_info("%s: init, major = %d\n", DEV_NAME, major);
	pr_info("%s: next: sudo mknod /dev/hello_drv c %d 0\n",
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
MODULE_DESCRIPTION("Day3 char device call chain");
