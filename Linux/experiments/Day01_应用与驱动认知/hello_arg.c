#include <stdio.h>

/*
 * Day1 最小实验：观察 argc 与 argv
 *
 * 编译：gcc -Wall -Wextra -std=c11 hello_arg.c -o hello_arg
 * 运行：./hello_arg Linux driver
 */

int main(int argc, char **argv)
{
	int i;

	printf("argc = %d\n", argc);
	for (i = 0; i < argc; i++)
		printf("argv[%d] = %s\n", i, argv[i]);

	if (argc >= 2)
		printf("Hello, %s!\n", argv[1]);
	else
		printf("Hello, world!\n");

	return 0;
}
