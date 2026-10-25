# Day6 platform 设备与驱动
#
# 两个模块分开编译，用于验证加载顺序不影响匹配。

obj-m += hello_platform_dev.o
obj-m += hello_platform_drv.o

KDIR ?= /lib/modules/$(shell uname -r)/build
ARCH          ?= arm
CROSS_COMPILE ?= arm-linux-gnueabihf-
export ARCH
export CROSS_COMPILE
PWD  := $(shell pwd)

.PHONY: all clean info

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

info:
	@echo "KDIR = $(KDIR)"
	@test -d $(KDIR) && echo "kernel headers: OK" || echo "kernel headers: MISSING"
