// SPDX-License-Identifier: GPL-2.0
/*
 * Day15：把按键接入 input 子系统（对照 Day03 的私有字符设备）
 *
 * 与 Day03/Day10 的根本差别：
 *   Day03 自己定义了"用户态怎么读按键"——open/read 一个私有设备节点，
 *   读回来的是什么格式只有本驱动和配套的 test 程序知道。
 *   Day15 不定义接口，只往 input core 上报"哪个键、按下还是松开"，
 *   input core 负责把事件分发给 evdev，evdev 负责生成标准格式的
 *   /dev/input/eventN。用户态用任何 evdev 客户端都能读。
 *
 * QEMU 里没有真实按键，所以事件源用 timer 模拟：
 *   每 KEY_SIM_INTERVAL_MS 翻转一次电平，上报一次 EV_KEY。
 * 这是本实验唯一的简化点——上报路径本身（input_report_key →
 * input_sync → input core → evdev → 用户态 read）是真实的。
 */

#include <linux/init.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/timer.h>

#define KEY_SIM_INTERVAL_MS 100
#define KEY_SIM_CODE        KEY_ENTER

struct key_sim {
	struct input_dev *idev;
	struct timer_list timer;
	bool pressed;
	unsigned int down_count;
};

static struct key_sim *ks;

static void key_sim_timer(struct timer_list *t)
{
	struct key_sim *k = container_of(t, struct key_sim, timer);

	k->pressed = !k->pressed;

	if (k->pressed)
		k->down_count++;

	/*
	 * 两步缺一不可：
	 *   input_report_key() 只是把事件放进 input 设备的事件缓冲；
	 *   input_sync()       才把这个缓冲当作一帧提交给 input core。
	 * 漏掉 sync，evdev 侧永远收不到——事件"上报成功"但读不到。
	 */
	input_report_key(k->idev, KEY_SIM_CODE, k->pressed ? 1 : 0);
	input_sync(k->idev);

	pr_info("key_sim: report %s count=%u\n",
		k->pressed ? "down" : "up", k->down_count);

	mod_timer(&k->timer, jiffies + msecs_to_jiffies(KEY_SIM_INTERVAL_MS));
}

static int __init key_sim_init(void)
{
	int err;

	ks = kzalloc(sizeof(*ks), GFP_KERNEL);
	if (!ks)
		return -ENOMEM;

	ks->idev = input_allocate_device();
	if (!ks->idev) {
		err = -ENOMEM;
		goto err_free;
	}

	ks->idev->name = "qemu-key-sim";
	ks->idev->phys = "key-sim/input0";
	/*
	 * BUS_VIRTUAL 而不是 BUS_HOST：这个设备不是真实硬件总线上的，
	 * 标错会让 udev 的规则把它当成真按键处理。
	 */
	ks->idev->id.bustype = BUS_VIRTUAL;

	/*
	 * 位图必须显式声明能力：
	 *   evbit  = 支持哪些事件类型（这里是 EV_KEY）
	 *   keybit = 支持哪些具体键码
	 * 不声明这两个位，input_register_device() 能成功，但 evdev 不会
	 * 为这个设备生成任何事件——"注册成功但不产生事件"。
	 */
	__set_bit(EV_KEY, ks->idev->evbit);
	__set_bit(KEY_SIM_CODE, ks->idev->keybit);

	err = input_register_device(ks->idev);
	if (err)
		goto err_free_input;

	timer_setup(&ks->timer, key_sim_timer, 0);
	mod_timer(&ks->timer, jiffies + msecs_to_jiffies(KEY_SIM_INTERVAL_MS));

	pr_info("key_sim: input device registered, code=%d interval=%dms\n",
		KEY_SIM_CODE, KEY_SIM_INTERVAL_MS);
	return 0;

err_free_input:
	input_free_device(ks->idev);
err_free:
	kfree(ks);
	return err;
}

static void __exit key_sim_exit(void)
{
	/*
	 * 顺序不能反：先停 timer，再注销 input 设备。
	 * 反过来会出现 timer 回调访问已释放 idev 的 use-after-free。
	 */
	timer_delete_sync(&ks->timer);
	input_unregister_device(ks->idev);
	kfree(ks);

	pr_info("key_sim: unloaded\n");
}

module_init(key_sim_init);
module_exit(key_sim_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("StudyCode");
MODULE_DESCRIPTION("Day15: key event via input subsystem (timer simulated source)");
