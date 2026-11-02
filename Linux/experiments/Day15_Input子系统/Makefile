# Day15 Input 子系统实验构建
#
# make          构建两个 .ko 与用户态测试程序（默认交叉编译到 ARM）
# make native   用本机内核头文件构建（只用于语法检查，跑不了）
# make info     检查内核头文件
# make clean    清理
#
# 本实验需要一个**开启了 CONFIG_INPUT_EVDEV** 的内核树。
# 本机验证用的是 ~/kbuild/linux-source-7.0.0（见 tests/qemu-vexpress）。

obj-m += key_input_drv.o
obj-m += key_chrdev_drv.o

KDIR          ?= /lib/modules/$(shell uname -r)/build
ARCH          ?= arm
CROSS_COMPILE ?= arm-linux-gnueabihf-
export ARCH
export CROSS_COMPILE
CC_TARGET     ?= $(CROSS_COMPILE)gcc
PWD           := $(shell pwd)

.PHONY: all clean info native

all: key_input_test
	$(MAKE) -C $(KDIR) M=$(PWD) modules

key_input_test: key_input_test.c
	$(CC_TARGET) -static -Wall -Wextra -O2 $< -o $@

# 静态链接：本程序要放进 initramfs 里作为普通进程运行，没有动态库
# （init 是 PID 1 的特殊情况，测试程序不是）

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f key_input_test

info:
	@echo "KDIR = $(KDIR)"
	@test -d $(KDIR) && echo "kernel headers: OK" || echo "kernel headers: MISSING"
	@test -f $(KDIR)/.config && grep -q '^CONFIG_INPUT_EVDEV=y' $(KDIR)/.config \
		&& echo "CONFIG_INPUT_EVDEV: OK" || echo "CONFIG_INPUT_EVDEV: MISSING (evdev 不可用)"
