// SPDX-License-Identifier: GPL-2.0
/*
 * Day15 对照驱动：同样的按键事件，用私有字符设备暴露
 *
 * 这个文件与 key_input_drv.c 的唯一差别是"接口怎么给用户态"：
 *   key_input_drv.c  — 上报给 input core，由 evdev 生成 /dev/input/eventN
 *   本文件           — 自己造 /dev/key_sim，自己定义读出来的是什么
 *
 * 事件源（timer）与上报节拍完全相同，所以两个实验可以直接比。
 *
 * 私有接口协议（本驱动自己定的，用户态必须知道才能用）：
 *   read() 每次返回 1 字节：
 *     '1' = 按下，'0' = 松开
 *   没有时间戳、没有事件类型、没有同步帧——这些都得用户态自己想办法补。
 */

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/uaccess.h>

#define KEY_SIM_INTERVAL_MS 100
#define KEY_SIM_BUF_SIZE    64

struct key_chrdev {
	struct timer_list timer;
	char buf[KEY_SIM_BUF_SIZE];   /* 环形缓冲：timer 写，read 取 */
	unsigned int head;
	unsigned int tail;
	unsigned int dropped;         /* 缓冲满时丢弃的事件数 */
	unsigned int down_count;
	bool pressed;
};

static struct key_chrdev *kc;
static int key_major;

static void key_chrdev_timer(struct timer_list *t)
{
	struct key_chrdev *k = container_of(t, struct key_chrdev, timer);
	char level;

	k->pressed = !k->pressed;
	if (k->pressed)
		k->down_count++;
	level = k->pressed ? '1' : '0';

	if (k->head - k->tail >= KEY_SIM_BUF_SIZE) {
		k->dropped++;
	} else {
		k->buf[k->head % KEY_SIM_BUF_SIZE] = level;
		k->head++;
	}

	pr_info("key_chrdev: queue %c head=%u dropped=%u\n",
		level, k->head, k->dropped);

	mod_timer(&k->timer, jiffies + msecs_to_jiffies(KEY_SIM_INTERVAL_MS));
}

static ssize_t key_read(struct file *file, char __user *ubuf,
			size_t count, loff_t *ppos)
{
	struct key_chrdev *k = file->private_data;

	if (k->tail >= k->head)
		return 0;            /* 无数据：返回 0 = EOF，调用方必须自己区分 */

	if (count > k->head - k->tail)
		count = k->head - k->tail;

	if (copy_to_user(ubuf, &k->buf[k->tail % KEY_SIM_BUF_SIZE], count))
		return -EFAULT;

	k->tail += count;
	return count;
}

static int key_open(struct inode *inode, struct file *file)
{
	file->private_data = kc;
	return 0;
}

static const struct file_operations key_fops = {
	.owner = THIS_MODULE,
	.open  = key_open,
	.read  = key_read,
};

static int __init key_chrdev_init(void)
{
	int err;

	kc = kzalloc(sizeof(*kc), GFP_KERNEL);
	if (!kc)
		return -ENOMEM;

	key_major = register_chrdev(0, "key_sim", &key_fops);
	if (key_major < 0) {
		err = key_major;
		goto err_free;
	}

	timer_setup(&kc->timer, key_chrdev_timer, 0);
	mod_timer(&kc->timer, jiffies + msecs_to_jiffies(KEY_SIM_INTERVAL_MS));

	pr_info("key_chrdev: registered major=%d (mknod /dev/key_sim c %d 0)\n",
		key_major, key_major);
	return 0;

err_free:
	kfree(kc);
	return err;
}

static void __exit key_chrdev_exit(void)
{
	unsigned int dropped;

	timer_delete_sync(&kc->timer);
	unregister_chrdev(key_major, "key_sim");
	/* 先取出再释放：kfree 之后 kc->dropped 是 use-after-free */
	dropped = kc->dropped;
	kfree(kc);

	pr_info("key_chrdev: unloaded, dropped=%u\n", dropped);
}

module_init(key_chrdev_init);
module_exit(key_chrdev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("StudyCode");
MODULE_DESCRIPTION("Day15 control: key event via private char device");
