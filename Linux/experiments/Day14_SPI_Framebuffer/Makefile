# Day14 SPI Framebuffer
#
# make KDIR=/path/to/target/kernel-tree ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-

obj-m += oled_fb.o

KDIR ?= /lib/modules/$(shell uname -r)/build
ARCH          ?= arm
CROSS_COMPILE ?= arm-linux-gnueabihf-
export ARCH
export CROSS_COMPILE
CC_TARGET     ?= $(CROSS_COMPILE)gcc
PWD  := $(shell pwd)

.PHONY: all clean info

all: oled_test
	$(MAKE) -C $(KDIR) M=$(PWD) modules

oled_test: oled_test.c
	$(CC_TARGET) -Wall -Wextra -O2 $< -o $@

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f oled_test

info:
	@echo "KDIR = $(KDIR)"
	@test -d $(KDIR) && echo "kernel headers: OK" || echo "kernel headers: MISSING"
