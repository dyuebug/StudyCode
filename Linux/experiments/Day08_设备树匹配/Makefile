# Day8 设备树匹配
#
# make KERN_DIR=/path/to/target/kernel
#
# 注意：默认 KDIR 指向主机内核，仅用于语法检查；
#       真正的模块必须在目标内核树中构建。

obj-m += led_dts_drv.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

.PHONY: all clean info

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

info:
	@echo "KDIR = $(KDIR)"
	@test -d $(KDIR) && echo "kernel headers: OK" || echo "kernel headers: MISSING"
