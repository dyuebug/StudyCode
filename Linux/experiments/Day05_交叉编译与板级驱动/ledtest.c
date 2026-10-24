#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/*
 * 板级 LED 测试
 *
 *   ./ledtest on
 *   ./ledtest off
 */

#define DEV_PATH "/dev/myled"

int main(int argc, char **argv)
{
	unsigned char val;
	int fd;

	if (argc != 2) {
		printf("Usage: %s on|off\n", argv[0]);
		return 1;
	}

	if (strcmp(argv[1], "on") == 0)
		val = 1;
	else if (strcmp(argv[1], "off") == 0)
		val = 0;
	else {
		printf("unknown option: %s\n", argv[1]);
		return 1;
	}

	fd = open(DEV_PATH, O_WRONLY);
	if (fd < 0) {
		printf("open %s failed: %s\n", DEV_PATH, strerror(errno));
		return 1;
	}

	if (write(fd, &val, 1) != 1) {
		printf("write failed: %s\n", strerror(errno));
		close(fd);
		return 1;
	}

	close(fd);
	printf("led %s\n", argv[1]);
	return 0;
}
