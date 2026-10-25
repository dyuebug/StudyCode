#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/*
 *   ./key_ctx_test -b    阻塞等待一次按键
 *   ./key_ctx_test -n    非阻塞查询
 */

#define DEV_PATH "/dev/key_ctx"

int main(int argc, char **argv)
{
	int fd, v;
	ssize_t n;

	if (argc != 2) {
		printf("Usage: %s -b|-n\n", argv[0]);
		return 1;
	}

	fd = open(DEV_PATH, (argv[1][1] == 'n') ? (O_RDONLY | O_NONBLOCK)
					       : O_RDONLY);
	if (fd < 0) {
		printf("open failed: %s\n", strerror(errno));
		return 1;
	}

	n = read(fd, &v, sizeof(v));
	if (n < 0)
		printf("read failed: %s\n", strerror(errno));
	else
		printf("key value=%d\n", v);

	close(fd);
	return 0;
}
