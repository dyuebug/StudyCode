#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

/*
 *   ./button_test -b     阻塞读
 *   ./button_test -n     非阻塞读
 *   ./button_test -p     poll
 *   ./button_test -c     清空计数
 */

#define DEV_PATH "/dev/myk"
#define KEY_IOC_CLEAR _IO('k', 0)

int main(int argc, char **argv)
{
	int fd, v;
	ssize_t n;

	if (argc != 2) {
		printf("Usage: %s -b|-n|-p|-c\n", argv[0]);
		return 1;
	}

	if (argv[1][1] == 'n')
		fd = open(DEV_PATH, O_RDONLY | O_NONBLOCK);
	else
		fd = open(DEV_PATH, O_RDONLY);

	if (fd < 0) {
		printf("open %s failed: %s\n", DEV_PATH, strerror(errno));
		return 1;
	}

	if (argv[1][1] == 'c') {
		if (ioctl(fd, KEY_IOC_CLEAR) < 0)
			printf("ioctl failed: %s\n", strerror(errno));
		else
			printf("count cleared\n");
		close(fd);
		return 0;
	}

	if (argv[1][1] == 'p') {
		struct pollfd pfd = { .fd = fd, .events = POLLIN };

		printf("polling...\n");
		if (poll(&pfd, 1, -1) < 0) {
			printf("poll failed: %s\n", strerror(errno));
			close(fd);
			return 1;
		}
		printf("poll ready: 0x%x\n", pfd.revents);
	}

	if (argv[1][1] == 'b')
		printf("waiting...\n");

	n = read(fd, &v, sizeof(v));
	if (n < 0)
		printf("read failed: %s\n", strerror(errno));
	else
		printf("key event: value=%d\n", v);

	close(fd);
	return 0;
}
