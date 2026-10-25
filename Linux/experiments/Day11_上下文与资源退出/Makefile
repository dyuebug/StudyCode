# Day11 上下文与资源退出
#
# make KERN_DIR=/path/to/target/kernel

obj-m += key_ctx.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

.PHONY: all clean info

all: key_ctx_test
	$(MAKE) -C $(KDIR) M=$(PWD) modules

key_ctx_test: key_ctx_test.c
	$(CC) -Wall -Wextra -O2 $< -o $@

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f key_ctx_test

info:
	@echo "KDIR = $(KDIR)"
	@test -d $(KDIR) && echo "kernel headers: OK" || echo "kernel headers: MISSING"
