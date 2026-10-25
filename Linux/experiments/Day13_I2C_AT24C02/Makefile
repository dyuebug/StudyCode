# Day13 I2C AT24C02
#
# make KERN_DIR=/path/to/target/kernel

obj-m += at24c02_drv.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

.PHONY: all clean info

all: at24c02_test
	$(MAKE) -C $(KDIR) M=$(PWD) modules

at24c02_test: at24c02_test.c
	$(CC) -Wall -Wextra -O2 $< -o $@

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f at24c02_test

info:
	@echo "KDIR = $(KDIR)"
	@test -d $(KDIR) && echo "kernel headers: OK" || echo "kernel headers: MISSING"
