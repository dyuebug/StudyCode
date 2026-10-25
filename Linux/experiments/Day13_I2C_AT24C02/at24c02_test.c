#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define DEV_PATH "/dev/at24c02"
#define SIZE 8

int main(void)
{
	unsigned char wr[SIZE] = { 'H', 'E', 'L', 'L', 'O', '0', '8', '9' };
	unsigned char rd[SIZE] = { 0 };
	int fd;
	ssize_t n;

	fd = open(DEV_PATH, O_RDWR);
	if (fd < 0) {
		printf("open failed: %s\n", strerror(errno));
		return 1;
	}

	n = write(fd, wr, SIZE);
	if (n != SIZE) {
		printf("write failed: n=%zd err=%s\n", n, strerror(errno));
		close(fd);
		return 1;
	}
	printf("write ok: %zd bytes\n", n);

	n = read(fd, rd, SIZE);
	if (n != SIZE) {
		printf("read failed: n=%zd err=%s\n", n, strerror(errno));
		close(fd);
		return 1;
	}
	printf("read  ok: %zd bytes\n", n);

	close(fd);

	if (memcmp(wr, rd, SIZE) == 0) {
		printf("data match\n");
		return 0;
	}

	printf("data mismatch\n");
	printf("wr: %.*s\n", SIZE, wr);
	printf("rd: %.*s\n", SIZE, rd);
	return 1;
}
