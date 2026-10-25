/*
 * QEMU vexpress-a9 验证用 init
 *
 * 作用：
 *   1. 挂载 proc / sysfs / devtmpfs
 *   2. 依次加载 Day2/3/4/6/7 的内核模块
 *   3. 对字符设备执行读写，打印 PASS/FAIL
 *   4. 主动重启，配合 qemu -no-reboot 让虚拟机退出
 *
 * 约束：
 *   - 静态链接（initramfs 中没有 libc）
 *   - 作为 PID 1 运行
 */

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/sysmacros.h>
#include <unistd.h>

#define EVENT_IOC_CLEAR  _IO('q', 0)
#define EVENT_IOC_PAUSE  _IO('q', 1)
#define EVENT_IOC_RESUME _IO('q', 2)

static int failures;

static void mark(int ok, const char *what)
{
	printf("[%s] %s\n", ok ? " OK " : "FAIL", what);
	if (!ok)
		failures++;
}

static int load_module(const char *path)
{
	long ret;
	int fd;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		printf("[FAIL] open %s: %s\n", path, strerror(errno));
		failures++;
		return -1;
	}

	/* finit_module 由内核直接读取模块文件，避免用户缓冲区映射问题 */
	ret = syscall(SYS_finit_module, fd, "", 0);
	if (ret < 0) {
		int saved = errno;

		printf("[FAIL] finit_module %s: %s\n", path, strerror(saved));
		failures++;
		close(fd);
		return -1;
	}

	close(fd);
	printf("[ OK ] loaded %s\n", path);
	return 0;
}

static int get_major(const char *name)
{
	char line[128], nm[64];
	int maj;
	FILE *f = fopen("/proc/devices", "r");

	if (!f)
		return -1;

	while (fgets(line, sizeof(line), f)) {
		if (sscanf(line, "%d %63s", &maj, nm) == 2 &&
		    strcmp(nm, name) == 0) {
			fclose(f);
			return maj;
		}
	}
	fclose(f);
	return -1;
}

static int make_node(const char *path, const char *dev_name)
{
	int maj = get_major(dev_name);

	if (maj < 0) {
		printf("[FAIL] %s not found in /proc/devices\n", dev_name);
		failures++;
		return -1;
	}
	if (mknod(path, S_IFCHR | 0666, makedev(maj, 0)) < 0 &&
	    errno != EEXIST) {
		printf("[FAIL] mknod %s: %s\n", path, strerror(errno));
		failures++;
		return -1;
	}
	printf("[ OK ] mknod %s major=%d\n", path, maj);
	return 0;
}

static void test_day03(void)
{
	char buf[8] = { 0 };
	ssize_t n;
	int fd;

	if (make_node("/dev/hello_chrdev", "hello_chrdev"))
		return;

	fd = open("/dev/hello_chrdev", O_RDWR);
	mark(fd >= 0, "Day3 open /dev/hello_chrdev");
	if (fd < 0)
		return;

	n = write(fd, "abc", 3);
	mark(n == 1, "Day3 write returns 1 (callback stub)");
	n = read(fd, buf, sizeof(buf));
	mark(n == 1, "Day3 read returns 1 (no data copied)");
	close(fd);
}

static void test_day04(void)
{
	int v = 0, w = 456;
	ssize_t n;
	int fd;

	if (make_node("/dev/hello_copy", "hello_copy"))
		return;

	fd = open("/dev/hello_copy", O_RDWR);
	mark(fd >= 0, "Day4 open /dev/hello_copy");
	if (fd < 0)
		return;

	n = read(fd, &v, sizeof(v));
	mark(n == sizeof(v) && v == 123, "Day4 read initial value = 123");

	n = write(fd, &w, sizeof(w));
	mark(n == sizeof(w), "Day4 write 456");

	v = 0;
	n = read(fd, &v, sizeof(v));
	mark(n == sizeof(v) && v == 456, "Day4 read back 456");

	{
		char one = 0;

		n = read(fd, &one, 1);
		mark(n == 1, "Day4 short read returns 1 (no overflow)");
	}
	close(fd);
}

static void test_day06(void)
{
	mark(access("/sys/bus/platform/drivers/hello_platform", F_OK) == 0,
	     "Day6 platform driver registered");
}

static void test_day07(void)
{
	unsigned char on = 1;
	int fd;

	mark(access("/dev/led0", F_OK) == 0, "Day7 /dev/led0 created");
	mark(access("/dev/led1", F_OK) == 0, "Day7 /dev/led1 created");

	fd = open("/dev/led0", O_WRONLY);
	if (fd >= 0) {
		mark(write(fd, &on, 1) == 1, "Day7 write to /dev/led0");
		close(fd);
	} else {
		mark(0, "Day7 open /dev/led0");
	}
}

static void test_day08(void)
{
	mark(access("/sys/bus/platform/devices/myled_for_test", F_OK) == 0,
	     "Day8 DT node present (myled_for_test)");
	mark(access("/sys/bus/platform/drivers/led_dts", F_OK) == 0,
	     "Day8 driver registered");
}

static void test_day09(void)
{
	int fd, v = -1;
	ssize_t n;

	mark(access("/dev/myled_gpio", F_OK) == 0,
	     "Day9 /dev/myled_gpio created");

	fd = open("/dev/myled_gpio", O_RDWR);
	if (fd < 0) {
		mark(0, "Day9 open /dev/myled_gpio");
		return;
	}

	mark(write(fd, "\1", 1) == 1, "Day9 set logical 1");
	n = read(fd, &v, sizeof(v));
	mark(n == sizeof(v) && v == 1, "Day9 read back 1");

	mark(write(fd, "\0", 1) == 1, "Day9 set logical 0");
	n = read(fd, &v, sizeof(v));
	mark(n == sizeof(v) && v == 0, "Day9 read back 0");

	close(fd);
}

static void test_event(void)
{
	struct pollfd pfd;
	int fd, fdn, v = 0;
	ssize_t n;

	mark(access("/dev/qemu_event", F_OK) == 0,
	     "Day10 /dev/qemu_event created");

	fd = open("/dev/qemu_event", O_RDWR);
	if (fd < 0) {
		mark(0, "Day10 open /dev/qemu_event");
		return;
	}

	/* 先停事件源并清零，保证非阻塞读结果确定 */
	mark(ioctl(fd, EVENT_IOC_PAUSE) == 0, "Day10 ioctl PAUSE");
	mark(ioctl(fd, EVENT_IOC_CLEAR) == 0, "Day10 ioctl CLEAR");

	fdn = open("/dev/qemu_event", O_RDONLY | O_NONBLOCK);
	mark(fdn >= 0, "Day10 open nonblocking");
	if (fdn >= 0) {
		n = read(fdn, &v, sizeof(v));
		mark(n < 0 && errno == EAGAIN,
		     "Day10 nonblocking read returns EAGAIN");
		close(fdn);
	}

	mark(ioctl(fd, EVENT_IOC_RESUME) == 0, "Day10 ioctl RESUME");

	pfd.fd = fd;
	pfd.events = POLLIN;
	pfd.revents = 0;
	v = poll(&pfd, 1, 2000);
	mark(v > 0 && (pfd.revents & POLLIN), "Day10 poll reports POLLIN");

	v = 0;
	n = read(fd, &v, sizeof(v));
	mark(n == sizeof(v) && v > 0, "Day10 blocking read returns event");

	close(fd);
}

static void test_unload(const char *name)
{
	long r = syscall(SYS_delete_module, name, O_NONBLOCK);

	if (r == 0)
		printf("[ OK ] unload %s (remove path)\n", name);
	else
		mark(0, "unload module");
}

static void test_day05(void)
{
	mark(access("/dev/myled", F_OK) == 0,
	     "Day5 cross-built module loaded, /dev/myled created");
	/* 注意：不打开设备，避免访问 i.MX6ULL 寄存器地址 */
}

int main(void)
{
	printf("\n===== qemu-vexpress verification =====\n");

	if (mount("proc", "/proc", "proc", 0, NULL) < 0 && errno != EBUSY)
		mark(0, "mount /proc");
	if (mount("sysfs", "/sys", "sysfs", 0, NULL) < 0 && errno != EBUSY)
		mark(0, "mount /sys");
	if (mount("devtmpfs", "/dev", "devtmpfs", 0, NULL) < 0 && errno != EBUSY)
		mark(0, "mount /dev");

	/* Day2：最小模块 */
	load_module("/hello_module.ko");

	/* Day3：字符设备调用链 */
	if (load_module("/hello_chrdev.ko") == 0)
		test_day03();

	/* Day4：用户/内核拷贝 */
	if (load_module("/hello_copy.ko") == 0)
		test_day04();

	/* Day6：platform 匹配 */
	load_module("/hello_platform_dev.ko");
	if (load_module("/hello_platform_drv.ko") == 0)
		test_day06();

	/* Day7：多实例 + drvdata */
	load_module("/led_platform_dev.ko");
	if (load_module("/led_platform_drv.ko") == 0)
		test_day07();

	/* Day8：设备树匹配 */
	load_module("/led_dts_drv.ko");
	test_day08();

	/* Day9：GPIO 消费者（gpiolib） */
	if (load_module("/led_gpio_drv.ko") == 0)
		test_day09();

	/* Day5：交叉编译产物 + 加载/卸载流程（不访问板级寄存器） */
	if (load_module("/led_drv.ko") == 0) {
		test_day05();
		test_unload("led_drv");
	}

	/* Day10/11：定时器事件 + 等待队列 + poll + ioctl + 卸载清理 */
	if (load_module("/event_demo.ko") == 0) {
		test_event();
		test_unload("event_demo");
	}

	printf("===== result: %s (failures=%d) =====\n",
	       failures ? "FAILED" : "PASSED", failures);

	sync();
	reboot(RB_AUTOBOOT);
	return failures ? 1 : 0;
}
