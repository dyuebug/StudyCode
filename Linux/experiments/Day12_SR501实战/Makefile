# Day12 SR501
#
# make KERN_DIR=/path/to/target/kernel

obj-m += sr501_drv.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

.PHONY: all clean info

all: sr501_test
	$(MAKE) -C $(KDIR) M=$(PWD) modules

sr501_test: sr501_test.c
	$(CC) -Wall -Wextra -O2 $< -o $@

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f sr501_test

info:
	@echo "KDIR = $(KDIR)"
	@test -d $(KDIR) && echo "kernel headers: OK" || echo "kernel headers: MISSING"
