#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/*
 *   ./sr501_test          阻塞等待一次事件
 *   ./sr501_test -p       poll 等待
 *   ./sr501_test -n       非阻塞查询
 */

#define DEV_PATH "/dev/sr501"

int main(int argc, char **argv)
{
	int fd, v;
	ssize_t n;

	if (argc > 2) {
		printf("Usage: %s [-p|-n]\n", argv[0]);
		return 1;
	}

	if (argc == 2 && strcmp(argv[1], "-n") == 0)
		fd = open(DEV_PATH, O_RDONLY | O_NONBLOCK);
	else
		fd = open(DEV_PATH, O_RDONLY);

	if (fd < 0) {
		printf("open failed: %s\n", strerror(errno));
		return 1;
	}

	if (argc == 2 && strcmp(argv[1], "-p") == 0) {
		struct pollfd pfd = { .fd = fd, .events = POLLIN };

		printf("polling for motion...\n");
		if (poll(&pfd, 1, -1) < 0) {
			printf("poll failed: %s\n", strerror(errno));
			close(fd);
			return 1;
		}
		printf("poll ready: 0x%x\n", pfd.revents);
	} else if (argc == 1) {
		printf("waiting for motion...\n");
	}

	n = read(fd, &v, sizeof(v));
	if (n < 0)
		printf("read failed: %s\n", strerror(errno));
	else
		printf("motion event: value=%d\n", v);

	close(fd);
	return 0;
}
