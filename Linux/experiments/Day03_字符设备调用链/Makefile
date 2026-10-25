# Day3 字符设备驱动构建
#
# make          构建 hello_chrdev.ko 与用户态测试程序
# make info     检查内核头文件
# make clean    清理

obj-m += hello_chrdev.o

KDIR ?= /lib/modules/$(shell uname -r)/build
ARCH          ?= arm
CROSS_COMPILE ?= arm-linux-gnueabihf-
export ARCH
export CROSS_COMPILE
CC_TARGET     ?= $(CROSS_COMPILE)gcc
PWD  := $(shell pwd)

.PHONY: all clean info

all: hello_chrdev_test
	$(MAKE) -C $(KDIR) M=$(PWD) modules

hello_chrdev_test: hello_chrdev_test.c
	$(CC_TARGET) -Wall -Wextra -O2 $< -o $@

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f hello_chrdev_test

info:
	@echo "KDIR = $(KDIR)"
	@test -d $(KDIR) && echo "kernel headers: OK" || echo "kernel headers: MISSING"
