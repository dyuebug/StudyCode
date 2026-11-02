// SPDX-License-Identifier: GPL-2.0
/*
 * Day15 用户态测试：从 evdev 读按键事件
 *
 * 这个程序的全部工作就是"读标准格式"——它不知道驱动内部怎么实现，
 * 也不需要知道。这就是接入 input 子系统的收益：
 * 驱动换了、硬件换了，只要事件语义一致，这个程序不用改。
 *
 * 对照 Day03 的 hello_chrdev_test：那里必须自己定义协议、
 * 自己解析驱动塞进缓冲区的那几个字节。
 *
 * 构建：交叉编译成 ARM 静态二进制，放进 initramfs。
 */

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define TARGET_NAME   "qemu-key-sim"
#define NEED_KEY_EVS  6
#define TIMEOUT_MS    3000
#define POLL_SLEEP_US 10000

static int failures;

static void mark(int ok, const char *what)
{
	printf("[%s] %s\n", ok ? " OK " : "FAIL", what);
	if (!ok)
		failures++;
}

/*
 * 在 /dev/input/event* 中按名字找到我们的设备。
 * 不能用固定编号：eventN 的 N 由注册顺序决定，换一次运行就可能变。
 */
static int find_event_dev(char *path, size_t pathlen)
{
	char name[64];
	int i;

	for (i = 0; i < 32; i++) {
		int fd;

		snprintf(path, pathlen, "/dev/input/event%d", i);
		fd = open(path, O_RDONLY | O_NONBLOCK);
		if (fd < 0)
			continue;

		memset(name, 0, sizeof(name));
		if (ioctl(fd, EVIOCGNAME(sizeof(name) - 1), name) >= 0 &&
		    strcmp(name, TARGET_NAME) == 0) {
			printf("[INFO] found %s as \"%s\"\n", path, name);
			close(fd);
			return 0;
		}
		close(fd);
	}
	return -1;
}

int main(void)
{
	char path[64];
	struct input_event ev;
	int fd, got_key = 0, got_syn = 0;
	int expect = 1;          /* 定时器首次触发为按下 */
	int waited = 0;

	if (find_event_dev(path, sizeof(path)) < 0) {
		printf("[FAIL] 未找到名为 %s 的 input 设备\n", TARGET_NAME);
		return 1;
	}

	fd = open(path, O_RDONLY | O_NONBLOCK);
	if (fd < 0) {
		printf("[FAIL] open %s: %s\n", path, strerror(errno));
		return 1;
	}

	/*
	 * 退出条件必须等"帧闭合"，不能只等 6 个 EV_KEY。
	 * 实测踩到：只判 got_key < NEED 时，读到第 6 个 EV_KEY 立刻退出，
	 * 而这一帧的 EV_SYN 排在它后面，永远读不到 ⇒ got_syn 恒为 got_key-1，
	 * 于是唯一能证明 input_sync() 生效的判据永远假失败。
	 * 一帧 = EV_KEY + EV_SYN，所以闭合条件是 got_syn >= got_key；
	 * 第 7 个按键在 100 ms 后才来，不会在等待期间混入。
	 */
	while ((got_key < NEED_KEY_EVS || got_syn < got_key) &&
	       waited < TIMEOUT_MS) {
		ssize_t n = read(fd, &ev, sizeof(ev));

		if (n == (ssize_t)sizeof(ev)) {
			if (ev.type == EV_SYN) {
				got_syn++;
				continue;
			}
			if (ev.type != EV_KEY)
				continue;

			if (ev.code != KEY_ENTER) {
				printf("[FAIL] 键码不符: 期望 %d, 实际 %d\n",
				       KEY_ENTER, ev.code);
				failures++;
				break;
			}
			if (ev.value != expect) {
				printf("[FAIL] 第 %d 个事件电平不符: 期望 %d, 实际 %d\n",
				       got_key + 1, expect, ev.value);
				failures++;
			} else {
				printf("[ OK ] ev#%d KEY_ENTER value=%d\n",
				       got_key + 1, ev.value);
			}
			expect = !expect;
			got_key++;
		} else {
			usleep(POLL_SLEEP_US);
			waited += POLL_SLEEP_US / 1000;
		}
	}

	close(fd);

	mark(got_key == NEED_KEY_EVS, "收齐 6 个 EV_KEY 事件");
	/*
	 * EV_SYN 的存在证明 input_sync() 生效了。
	 * 没有 sync，evdev 不会把缓冲当作一帧提交，上面那 6 个事件一个都读不到。
	 * 判据是一帧一个 SYN（got_syn >= got_key），不是"再多读几个"——
	 * 数量对不上就说明有帧没闭合，属于真故障，必须失败。
	 */
	mark(got_syn >= got_key, "EV_SYN 同步事件数与按键事件匹配");

	printf("[SUMMARY] key_events=%d syn_events=%d failures=%d\n",
	       got_key, got_syn, failures);

	printf("RESULT: %s\n", failures == 0 ? "PASS" : "FAIL");
	return failures == 0 ? 0 : 1;
}
