# BSP_02 取证 init：静态链接，不依赖任何用户态库/init 系统。
#
# "CC ?=" 在这里是错的：GNU make 内建的 CC=cc 算"已定义"，?= 不会生效，
# 直接 make 会编出 x86-64 的 init，进内核后报 "Failed to execute /init (error -8)"（ENOEXEC）。
ifeq ($(origin CC),default)
CC := arm-linux-gnueabihf-gcc
endif

CFLAGS  ?= -O2 -g -Wall -Wextra -Werror
LDFLAGS ?= -static

.PHONY: all clean

all: init

init: init.c Makefile
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f init
