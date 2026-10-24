#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/*
 * 用户态测试程序
 *
 *   ./hello_chrdev_test -w abc
 *   ./hello_chrdev_test -r
 *
 * 注意：本日驱动不拷贝数据，-r 读到的缓冲区是未初始化的，
 * 这是刻意保留的教学现象，不是缺陷。
 */

#define DEV_PATH "/dev/hello_drv"

int main(int argc, char **argv)
{
	char buf[1024];
	int fd;
	ssize_t n;

	if (argc < 2) {
		printf("Usage: %s -w <string>\n", argv[0]);
		printf("       %s -r\n", argv[0]);
		return 1;
	}

	fd = open(DEV_PATH, O_RDWR);
	if (fd < 0) {
		printf("open %s failed: %s\n", DEV_PATH, strerror(errno));
		return 1;
	}
	printf("open %s ok\n", DEV_PATH);

	if (strcmp(argv[1], "-w") == 0 && argc == 3) {
		n = write(fd, argv[2], strlen(argv[2]) + 1);
		printf("write -> %zd\n", n);
	} else if (strcmp(argv[1], "-r") == 0) {
		memset(buf, 0, sizeof(buf));
		n = read(fd, buf, sizeof(buf));
		printf("read -> %zd\n", n);
		printf("buffer: %s\n", buf);
	} else {
		printf("unknown option: %s\n", argv[1]);
		close(fd);
		return 1;
	}

	close(fd);
	return 0;
}
