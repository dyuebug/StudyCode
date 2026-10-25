# Day12 SR501
#
# make KDIR=/path/to/target/kernel-tree ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-

obj-m += sr501_drv.o

KDIR ?= /lib/modules/$(shell uname -r)/build
ARCH          ?= arm
CROSS_COMPILE ?= arm-linux-gnueabihf-
export ARCH
export CROSS_COMPILE
CC_TARGET     ?= $(CROSS_COMPILE)gcc
PWD  := $(shell pwd)

.PHONY: all clean info

all: sr501_test
	$(MAKE) -C $(KDIR) M=$(PWD) modules

sr501_test: sr501_test.c
	$(CC_TARGET) -Wall -Wextra -O2 $< -o $@

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f sr501_test

info:
	@echo "KDIR = $(KDIR)"
	@test -d $(KDIR) && echo "kernel headers: OK" || echo "kernel headers: MISSING"
