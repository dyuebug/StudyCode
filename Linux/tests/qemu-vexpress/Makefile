# QEMU vexpress-a9 验证载荷构建
#
# 用法：
#   make           构建 init 并打包 initramfs
#   make clean     清理
#
# 依赖：
#   arm-linux-gnueabihf-gcc、cpio、gzip
#   KDIR 指向已编译的 ARM 内核源码树

CROSS   ?= arm-linux-gnueabihf-
CC      := $(CROSS)gcc
KDIR    ?= $(HOME)/kbuild/linux-source-7.0.0
EXP     := ../../experiments

MODULES := \
	$(EXP)/Day02_首次模块加载/hello_module.ko \
	$(EXP)/Day03_字符设备调用链/hello_chrdev.ko \
	$(EXP)/Day04_地址空间与用户拷贝/hello_copy.ko \
	$(EXP)/Day06_分离思想与platform/hello_platform_dev.ko \
	$(EXP)/Day06_分离思想与platform/hello_platform_drv.ko \
	$(EXP)/Day07_多设备与drvdata/led_platform_dev.ko \
	$(EXP)/Day07_多设备与drvdata/led_platform_drv.ko

.PHONY: all clean modules initramfs

all: initramfs

init:
	$(CC) -static -O2 -Wall -o init init.c

modules:
	cd $(EXP) && for d in Day02_* Day03_* Day04_* Day06_* Day07_*; do \
		$(MAKE) -s -C $$d all KDIR=$(KDIR) ARCH=arm CROSS_COMPILE=$(CROSS); \
	done

initramfs: init modules
	rm -rf rootfs initramfs.cpio.gz
	mkdir -p rootfs/proc rootfs/sys rootfs/dev
	cp init rootfs/init
	cp $(MODULES) rootfs/
	cd rootfs && find . -print0 | cpio --null -o -H newc 2>/dev/null | gzip -9 > ../initramfs.cpio.gz
	@ls -lh initramfs.cpio.gz

clean:
	rm -rf rootfs initramfs.cpio.gz init
