// SPDX-License-Identifier: GPL-2.0
/*
 * Day16 用户态测试：用最普通的文件 I/O 访问 sysfs 属性
 *
 * 这个程序不包含任何驱动头文件，也不需要 mknod：
 * sysfs 路径是内核统一的命名约定，属性就是普通文本文件。
 * 与 Day03/Day04 的私有字符设备相比，少了两件事——
 *   1. 不用 mknod，节点由 driver core 自动创建
 *   2. 不用自定义二进制协议，读写的都是可读文本
 *
 * 构建：交叉编译成 ARM 静态二进制，放进 initramfs。
 */

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BASE     "/sys/devices/platform/param_demo"
#define P_INTVL  BASE "/interval"
#define P_STATS  BASE "/stats"

static int failures;

static void mark(int ok, const char *what)
{
	printf("[%s] %s\n", ok ? " OK " : "FAIL", what);
	if (!ok)
		failures++;
}

static int read_file(const char *path, char *buf, size_t len)
{
	ssize_t n;
	int fd = open(path, O_RDONLY);

	if (fd < 0)
		return -1;
	n = read(fd, buf, len - 1);
	close(fd);
	if (n < 0)
		return -1;
	buf[n] = '\0';
	return 0;
}

static ssize_t write_file(const char *path, const char *val)
{
	ssize_t n;
	int fd = open(path, O_WRONLY);

	if (fd < 0)
		return -1;
	n = write(fd, val, strlen(val));
	close(fd);
	return n;
}

/* 从 "ticks=12 writes=3 ..." 里取出指定键的数值 */
static unsigned int get_stat(const char *buf, const char *key)
{
	const char *p = strstr(buf, key);
	unsigned int v = 0;

	if (!p)
		return 0;
	p += strlen(key);
	sscanf(p, "%u", &v);
	return v;
}

int main(void)
{
	char buf[256], orig[32];
	unsigned int t0 = 0, t1 = 0, w, r;
	ssize_t n;

	mark(access(P_INTVL, F_OK) == 0, "Day16 interval 属性存在");
	mark(access(P_STATS, F_OK) == 0, "Day16 stats 属性存在");

	if (read_file(P_INTVL, orig, sizeof(orig)) < 0) {
		mark(0, "Day16 读初始 interval");
		return 1;
	}
	printf("[INFO] initial interval=%s", orig);
	mark(atoi(orig) == 500, "Day16 初始 interval = 500");

	/* 1. 合法写入：write 必须返回完整字节数 */
	n = write_file(P_INTVL, "200\n");
	printf("[INFO] write \"200\\n\" rc=%zd (expect 4)\n", n);
	mark(n == 4, "Day16 写 200 返回 4 字节（store 返回 count）");
	if (read_file(P_INTVL, buf, sizeof(buf)) == 0)
		mark(atoi(buf) == 200, "Day16 读回 200");

	/* 2. 越界：驱动返回 -EINVAL，用户态拿到 EINVAL */
	errno = 0;
	n = write_file(P_INTVL, "5\n");
	mark(n < 0 && errno == EINVAL, "Day16 越界写得到 EINVAL");
	printf("[INFO] out-of-range write rc=%zd errno=%d(%s)\n",
	       n, errno, strerror(errno));

	/* 3. 非数字：kstrtouint 失败，同样转成 EINVAL */
	errno = 0;
	n = write_file(P_INTVL, "abc\n");
	mark(n < 0 && errno == EINVAL, "Day16 非数字写得到 EINVAL");

	/* 4. 只读属性不可写 */
	errno = 0;
	n = write_file(P_STATS, "x");
	mark(n < 0, "Day16 只读属性 stats 拒绝写入");
	printf("[INFO] write to RO attr rc=%zd errno=%d(%s)\n",
	       n, errno, strerror(errno));

	/* 5. 改写周期后 tick 频率必须真的变化 */
	if (write_file(P_INTVL, "50\n") == 3 &&
	    read_file(P_STATS, buf, sizeof(buf)) == 0) {
		t0 = get_stat(buf, "ticks=");
		usleep(1000000);
		if (read_file(P_STATS, buf, sizeof(buf)) == 0)
			t1 = get_stat(buf, "ticks=");
		printf("[INFO] interval=50ms, ticks %u -> %u in 1s\n", t0, t1);
		/*
		 * 新周期 50 ms 意味着 1 s 内约 20 次；若 interval 没生效
		 * （仍是 200 ms）只有约 5 次。判据取 >=10 以留出调度抖动。
		 */
		mark(t1 - t0 >= 10, "Day16 新周期生效（1s 内 tick >= 10）");
	} else {
		mark(0, "Day16 重新读取 stats");
	}

	/*
	 * 计数一致性：成功写入 2 次（200、50），被拒 2 次（5、abc）。
	 * 注意写只读属性 stats 的失败**不计入 rejects**——那一步在 kernfs 层
	 * 就被拒绝，驱动的 store() 根本没被调用。这两类失败的位置不同。
	 */
	if (read_file(P_STATS, buf, sizeof(buf)) == 0) {
		printf("[INFO] %s", buf);
		w = get_stat(buf, "writes=");
		r = get_stat(buf, "rejects=");
		mark(w == 2, "Day16 writes 计数 = 2");
		mark(r == 2, "Day16 rejects 计数 = 2");
	} else {
		mark(0, "Day16 读 stats");
	}

	printf("[SUMMARY] failures=%d\n", failures);
	printf("RESULT: %s\n", failures == 0 ? "PASS" : "FAIL");
	return failures == 0 ? 0 : 1;
}
