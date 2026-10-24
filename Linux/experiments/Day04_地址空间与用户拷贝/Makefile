# Day4 用户拷贝示例构建
#
# make          构建 hello_copy.ko 与用户态测试程序
# make info     检查内核头文件
# make clean    清理

obj-m += hello_copy.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

.PHONY: all clean info

all: hello_copy_test
	$(MAKE) -C $(KDIR) M=$(PWD) modules

hello_copy_test: hello_copy_test.c
	$(CC) -Wall -Wextra -O2 $< -o $@

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f hello_copy_test

info:
	@echo "KDIR = $(KDIR)"
	@test -d $(KDIR) && echo "kernel headers: OK" || echo "kernel headers: MISSING"
