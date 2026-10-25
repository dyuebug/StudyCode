# Day8 设备树匹配
#
# make KDIR=/path/to/target/kernel-tree ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
#
# 注意：默认 KDIR 指向主机内核，仅用于语法检查；
#       真正的模块必须在目标内核树中构建。

obj-m += led_dts_drv.o

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
