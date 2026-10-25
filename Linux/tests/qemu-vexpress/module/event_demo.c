#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

/*
 * Day10/Day11 验证用模块（QEMU 无硬件中断源）
 *
 * 用内核定时器周期性产生事件，覆盖：
 *   阻塞读 / 非阻塞读 / poll / ioctl / 等待队列
 *   timer / tasklet / workqueue / 卸载清理顺序
 *
 * 不覆盖：request_irq / request_threaded_irq 的真实中断投递。
 */

#define DEV_NAME "event_demo"

#define EVENT_IOC_CLEAR  _IO('q', 0)
#define EVENT_IOC_PAUSE  _IO('q', 1)
#define EVENT_IOC_RESUME _IO('q', 2)

struct event_dev {
	unsigned int count;
	unsigned int read_count;
	int paused;
	wait_queue_head_t wq;
	struct timer_list timer;
	struct tasklet_struct tasklet;
	struct work_struct work;
};

static int major;
static struct class *event_class;
static struct event_dev *the_dev;

static void event_work_fn(struct work_struct *work)
{
	struct event_dev *e = container_of(work, struct event_dev, work);

	pr_info("%s: work ran, count=%u\n", DEV_NAME, e->count);
}

static void event_tasklet_fn(unsigned long data)
{
	struct event_dev *e = (struct event_dev *)data;

	pr_info("%s: tasklet ran, count=%u\n", DEV_NAME, e->count);
}

static void event_timer_fn(struct timer_list *t)
{
	/* container_of 在各版本都可用；from_timer 在 6.15 改名为 timer_container_of */
	struct event_dev *e = container_of(t, struct event_dev, timer);

	e->count++;
	wake_up_interruptible(&e->wq);
	tasklet_schedule(&e->tasklet);
	schedule_work(&e->work);

	mod_timer(&e->timer, jiffies + HZ / 10);
}

static ssize_t event_read(struct file *file, char __user *buf,
			  size_t size, loff_t *offset)
{
	struct event_dev *e = file->private_data;
	unsigned int v;

	if (size < sizeof(v))
		return -EINVAL;

	if (e->read_count == e->count) {
		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;
		if (wait_event_interruptible(e->wq, e->read_count != e->count))
			return -ERESTARTSYS;
	}

	e->read_count = e->count;
	v = e->count;
	if (copy_to_user(buf, &v, sizeof(v)))
		return -EFAULT;

	return sizeof(v);
}

static unsigned int event_poll(struct file *file, poll_table *wait)
{
	struct event_dev *e = file->private_data;
	unsigned int mask = 0;

	poll_wait(file, &e->wq, wait);
	if (e->read_count != e->count)
		mask |= POLLIN | POLLRDNORM;

	return mask;
}

static long event_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct event_dev *e = file->private_data;

	switch (cmd) {
	case EVENT_IOC_CLEAR:
		e->count = 0;
		e->read_count = 0;
		return 0;
	case EVENT_IOC_PAUSE:
		e->paused = 1;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 15, 0)
		timer_delete_sync(&e->timer);
#else
		del_timer_sync(&e->timer);
#endif
		return 0;
	case EVENT_IOC_RESUME:
		e->paused = 0;
		mod_timer(&e->timer, jiffies + HZ / 10);
		return 0;
	default:
		return -ENOTTY;
	}
}

static int event_open(struct inode *inode, struct file *file)
{
	file->private_data = the_dev;
	return the_dev ? 0 : -ENODEV;
}

static const struct file_operations event_fops = {
	.owner          = THIS_MODULE,
	.open           = event_open,
	.read           = event_read,
	.poll           = event_poll,
	.unlocked_ioctl = event_ioctl,
};

static int __init event_init(void)
{
	struct event_dev *e;
	int err;

	e = kzalloc(sizeof(*e), GFP_KERNEL);
	if (!e)
		return -ENOMEM;

	init_waitqueue_head(&e->wq);
	timer_setup(&e->timer, event_timer_fn, 0);
	tasklet_init(&e->tasklet, event_tasklet_fn, (unsigned long)e);
	INIT_WORK(&e->work, event_work_fn);

	major = register_chrdev(0, DEV_NAME, &event_fops);
	if (major < 0) {
		kfree(e);
		return major;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	event_class = class_create("event_class");
#else
	event_class = class_create(THIS_MODULE, "event_class");
#endif
	if (IS_ERR(event_class)) {
		err = PTR_ERR(event_class);
		unregister_chrdev(major, DEV_NAME);
		kfree(e);
		return err;
	}

	if (IS_ERR(device_create(event_class, NULL, MKDEV(major, 0),
				 NULL, "qemu_event"))) {
		class_destroy(event_class);
		unregister_chrdev(major, DEV_NAME);
		kfree(e);
		return -ENODEV;
	}

	the_dev = e;
	mod_timer(&e->timer, jiffies + HZ / 10);

	pr_info("%s: init, major=%d\n", DEV_NAME, major);
	return 0;
}

static void __exit event_exit(void)
{
	struct event_dev *e = the_dev;

	/* 先停事件源，再释放引用 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 15, 0)
	timer_delete_sync(&e->timer);
#else
	del_timer_sync(&e->timer);
#endif
	tasklet_kill(&e->tasklet);
	cancel_work_sync(&e->work);

	device_destroy(event_class, MKDEV(major, 0));
	class_destroy(event_class);
	unregister_chrdev(major, DEV_NAME);
	the_dev = NULL;
	kfree(e);

	pr_info("%s: exit, resources released\n", DEV_NAME);
}

module_init(event_init);
module_exit(event_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Day10/11 timer-driven event demo for QEMU verification");
