# Day10 中断与 poll
#
# make KERN_DIR=/path/to/target/kernel

obj-m += gpio_key_irq.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

.PHONY: all clean info

all: button_test
	$(MAKE) -C $(KDIR) M=$(PWD) modules

button_test: button_test.c
	$(CC) -Wall -Wextra -O2 $< -o $@

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f button_test

info:
	@echo "KDIR = $(KDIR)"
	@test -d $(KDIR) && echo "kernel headers: OK" || echo "kernel headers: MISSING"
