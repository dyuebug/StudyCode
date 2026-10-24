#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 *   ./led_platform_test <minor> on
 *   ./led_platform_test <minor> off
 */

int main(int argc, char **argv)
{
	char path[64];
	unsigned char val;
	int fd, minor;

	if (argc != 3) {
		printf("Usage: %s <minor> on|off\n", argv[0]);
		return 1;
	}

	minor = atoi(argv[1]);
	if (strcmp(argv[2], "on") == 0)
		val = 1;
	else if (strcmp(argv[2], "off") == 0)
		val = 0;
	else {
		printf("unknown option: %s\n", argv[2]);
		return 1;
	}

	snprintf(path, sizeof(path), "/dev/led%d", minor);

	fd = open(path, O_WRONLY);
	if (fd < 0) {
		printf("open %s failed: %s\n", path, strerror(errno));
		return 1;
	}

	if (write(fd, &val, 1) != 1) {
		printf("write failed: %s\n", strerror(errno));
		close(fd);
		return 1;
	}

	close(fd);
	printf("%s -> %s\n", path, argv[2]);
	return 0;
}
