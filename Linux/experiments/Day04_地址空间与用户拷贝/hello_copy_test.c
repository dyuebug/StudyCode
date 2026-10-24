#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * 用户态测试
 *
 *   ./hello_copy_test -r        读 4 字节
 *   ./hello_copy_test -w 456    写一个整数
 *   ./hello_copy_test -s        只读 1 字节（验证驱动不会越界）
 */

#define DEV_PATH "/dev/hello_copy"

static void usage(const char *prog)
{
	printf("Usage: %s -r\n", prog);
	printf("       %s -w <int>\n", prog);
	printf("       %s -s\n", prog);
}

int main(int argc, char **argv)
{
	unsigned char buf[16];
	int fd;
	ssize_t n;

	if (argc < 2) {
		usage(argv[0]);
		return 1;
	}

	fd = open(DEV_PATH, O_RDWR);
	if (fd < 0) {
		printf("open %s failed: %s\n", DEV_PATH, strerror(errno));
		return 1;
	}

	if (strcmp(argv[1], "-r") == 0) {
		memset(buf, 0, sizeof(buf));
		n = read(fd, buf, sizeof(int));
		printf("read -> %zd, value = %d\n", n, *(int *)buf);
	} else if (strcmp(argv[1], "-s") == 0) {
		memset(buf, 0, sizeof(buf));
		n = read(fd, buf, 1);
		printf("read short -> %zd, byte0 = %u\n", n, buf[0]);
	} else if (strcmp(argv[1], "-w") == 0 && argc == 3) {
		int v = atoi(argv[2]);
		n = write(fd, &v, sizeof(v));
		printf("write -> %zd, value = %d\n", n, v);
	} else {
		usage(argv[0]);
		close(fd);
		return 1;
	}

	close(fd);
	return 0;
}
