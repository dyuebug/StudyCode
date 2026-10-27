# BSP_04 pivot_root 与 chroot 对照的构建
#
# 产物是两个静态 ARM 可执行文件：
#   stage1 → 打进 initramfs，作为内核的 /init（rdinit）
#   stage2 → 放进真实根（ext4 镜像），换根后由 execve 接管 PID 1

# 不能用 "CC ?= arm-linux-gnueabihf-gcc"：GNU make 有内建默认值 CC=cc，
# 它算"已定义"，?= 不生效，直接 make 会悄悄编出宿主机架构的二进制。
# 只在内建默认值生效时套用交叉编译器，命令行 CC=xxx 与环境变量 CC 仍然优先。
ifeq ($(origin CC),default)
CC := arm-linux-gnueabihf-gcc
endif

CFLAGS  ?= -O2 -g -Wall -Wextra -Werror
LDFLAGS ?= -static

TARGETS := stage1 stage2

.PHONY: all clean

all: $(TARGETS)

stage1: stage1.c proot.h Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

stage2: stage2.c proot.h Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)
