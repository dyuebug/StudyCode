# Day11 上下文与资源退出
#
# make KDIR=/path/to/target/kernel-tree ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-

obj-m += key_ctx.o

KDIR ?= /lib/modules/$(shell uname -r)/build
ARCH          ?= arm
CROSS_COMPILE ?= arm-linux-gnueabihf-
export ARCH
export CROSS_COMPILE
CC_TARGET     ?= $(CROSS_COMPILE)gcc
PWD  := $(shell pwd)

.PHONY: all clean info

all: key_ctx_test
	$(MAKE) -C $(KDIR) M=$(PWD) modules

key_ctx_test: key_ctx_test.c
	$(CC_TARGET) -Wall -Wextra -O2 $< -o $@

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f key_ctx_test

info:
	@echo "KDIR = $(KDIR)"
	@test -d $(KDIR) && echo "kernel headers: OK" || echo "kernel headers: MISSING"
