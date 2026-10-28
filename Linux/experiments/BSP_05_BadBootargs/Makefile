# BSP_05 坏 bootargs 用例的构建
#
# 产物是一个静态 ARM 可执行文件 init → 放进 ext4 镜像的 /sbin/init。
# 本实验**不用 initramfs**：只要 initrd 存在，内核就会优先执行它里面的 /init，
# root= 根本不会被尝试 —— 那样就测不到"内核层 root= 参数错"。

# 同 BSP_03：GNU make 的内建默认值 CC=cc 算"已定义"，?= 不会生效。
ifeq ($(origin CC),default)
CC := arm-linux-gnueabihf-gcc
endif

CFLAGS  ?= -O2 -g -Wall -Wextra -Werror
LDFLAGS ?= -static

TARGETS := init

.PHONY: all clean

all: $(TARGETS)

init: init.c Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)
