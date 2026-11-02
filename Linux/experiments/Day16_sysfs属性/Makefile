# Day16 sysfs 属性实验构建
#
# make          构建两个 .ko 与用户态测试程序（默认交叉编译到 ARM）
# make info     检查 kernel headers
# make clean    清理

obj-m += param_demo_dev.o
obj-m += param_demo_drv.o

KDIR          ?= /lib/modules/$(shell uname -r)/build
ARCH          ?= arm
CROSS_COMPILE ?= arm-linux-gnueabihf-
CC_TARGET     ?= $(CROSS_COMPILE)gcc

# ARCH / CROSS_COMPILE 必须 export：只在本 Makefile 里赋值不会传给
# kbuild 子进程，模块会按主机架构解析头文件（见 tests/qemu-vexpress/RESULT.md 第 12 条）
export ARCH
export CROSS_COMPILE

PWD := $(shell pwd)

.PHONY: all clean info

all: param_test
	$(MAKE) -C $(KDIR) M=$(PWD) modules

param_test: param_test.c
	$(CC_TARGET) -static -Wall -Wextra -O2 $< -o $@

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f param_test

info:
	@echo "KDIR = $(KDIR)"
	@test -d $(KDIR) && echo "kernel headers: OK" || echo "kernel headers: MISSING"
	@test -f $(KDIR)/.config && grep -q '^CONFIG_SYSFS=y' $(KDIR)/.config \
		&& echo "CONFIG_SYSFS: OK" || echo "CONFIG_SYSFS: MISSING"
