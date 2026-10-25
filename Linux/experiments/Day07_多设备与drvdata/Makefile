# Day7 多设备与 drvdata
#
# make / make info / make clean

obj-m += led_platform_dev.o
obj-m += led_platform_drv.o

KDIR ?= /lib/modules/$(shell uname -r)/build
ARCH          ?= arm
CROSS_COMPILE ?= arm-linux-gnueabihf-
export ARCH
export CROSS_COMPILE
CC_TARGET     ?= $(CROSS_COMPILE)gcc
PWD  := $(shell pwd)

.PHONY: all clean info

all: led_platform_test
	$(MAKE) -C $(KDIR) M=$(PWD) modules

led_platform_test: led_platform_test.c
	$(CC_TARGET) -Wall -Wextra -O2 $< -o $@

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f led_platform_test

info:
	@echo "KDIR = $(KDIR)"
	@test -d $(KDIR) && echo "kernel headers: OK" || echo "kernel headers: MISSING"
