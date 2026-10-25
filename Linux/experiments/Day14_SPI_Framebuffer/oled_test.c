#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/*
 *   ./oled_test            棋盘格
 *   ./oled_test bars       水平横条
 *   ./oled_test fill       全亮
 *   ./oled_test clear      全黑
 *
 * 显存布局：8 页 x 128 列，page-major，1bpp（每字节 8 个像素）。
 * 写满 1024 字节后，通过 sysfs 触发驱动把显存推到 OLED。
 */

#define FB_PATH    "/dev/fb0"
#define FLUSH_PATH "/sys/class/graphics/fb0/flush"
#define XRES 128
#define YRES 64
#define FB_SIZE (XRES * (YRES / 8))

static int flush_screen(void)
{
	ssize_t n;
	int fd;

	fd = open(FLUSH_PATH, O_WRONLY);
	if (fd < 0) {
		printf("open %s failed: %s\n", FLUSH_PATH, strerror(errno));
		return -1;
	}

	n = write(fd, "1", 1);
	close(fd);
	if (n != 1) {
		printf("flush failed: %s\n", strerror(errno));
		return -1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	unsigned char buf[FB_SIZE];
	const char *mode = argc > 1 ? argv[1] : "checker";
	ssize_t n;
	int page, col, fd;

	if (argc > 2) {
		printf("Usage: %s [checker|bars|fill|clear]\n", argv[0]);
		return 1;
	}

	memset(buf, 0, sizeof(buf));

	if (strcmp(mode, "checker") == 0) {
		for (page = 0; page < YRES / 8; page++)
			for (col = 0; col < XRES; col++)
				buf[page * XRES + col] = (col & 8) ? 0xF0 : 0x0F;
	} else if (strcmp(mode, "bars") == 0) {
		for (page = 0; page < YRES / 8; page++)
			memset(&buf[page * XRES], (page & 1) ? 0x00 : 0xFF,
			       XRES);
	} else if (strcmp(mode, "fill") == 0) {
		memset(buf, 0xFF, sizeof(buf));
	} else if (strcmp(mode, "clear") != 0) {
		printf("Usage: %s [checker|bars|fill|clear]\n", argv[0]);
		return 1;
	}

	fd = open(FB_PATH, O_RDWR);
	if (fd < 0) {
		printf("open %s failed: %s\n", FB_PATH, strerror(errno));
		return 1;
	}

	n = write(fd, buf, sizeof(buf));
	if (n != (ssize_t)sizeof(buf)) {
		printf("write failed: %s\n", strerror(errno));
		close(fd);
		return 1;
	}
	close(fd);

	if (flush_screen())
		return 1;

	printf("pattern '%s' written (%d bytes) and flushed\n", mode, FB_SIZE);
	return 0;
}
