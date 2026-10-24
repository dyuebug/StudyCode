# 内核外部模块构建（kbuild）
#
# 要求：/lib/modules/$(uname -r)/build 存在
# 用法：make          构建 hello_module.ko
#       make info     检查内核头文件是否存在
#       make clean    清理产物

obj-m += hello_module.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

.PHONY: all clean info

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

info:
	@echo "KDIR = $(KDIR)"
	@test -d $(KDIR) && echo "kernel headers: OK" || echo "kernel headers: MISSING"
