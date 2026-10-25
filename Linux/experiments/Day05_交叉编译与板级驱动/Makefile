# Day5 交叉编译示例
#
# 使用前设置：
#   export ARCH=arm
#   export CROSS_COMPILE=arm-linux-gnueabihf-
#   export KERN_DIR=~/100ask_imx6ull-sdk/Linux-4.9.88
#
# 也可在命令行覆盖：
#   make KERN_DIR=/path/to/kernel

obj-m += led_drv.o

KERN_DIR     ?= $(HOME)/100ask_imx6ull-sdk/Linux-4.9.88
ARCH          ?= arm
CROSS_COMPILE ?= arm-linux-gnueabihf-
export ARCH
export CROSS_COMPILE
PWD          := $(shell pwd)

CC_TARGET := $(CROSS_COMPILE)gcc

.PHONY: all clean info

all: ledtest
	$(MAKE) -C $(KERN_DIR) M=$(PWD) modules

ledtest: ledtest.c
	$(CC_TARGET) -Wall -O2 $< -o $@

clean:
	$(MAKE) -C $(KERN_DIR) M=$(PWD) clean
	rm -f ledtest

info:
	@echo "ARCH          = $(ARCH)"
	@echo "CROSS_COMPILE = $(CROSS_COMPILE)"
	@echo "KERN_DIR      = $(KERN_DIR)"
	@command -v $(CC_TARGET) >/dev/null && echo "cross gcc: OK" || echo "cross gcc: MISSING"
	@test -f $(KERN_DIR)/Module.symvers && echo "Module.symvers: OK" || echo "Module.symvers: MISSING"
