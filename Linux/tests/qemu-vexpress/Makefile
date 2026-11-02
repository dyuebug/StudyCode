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
	$(EXP)/Day05_交叉编译与板级驱动/led_drv.ko \
	$(EXP)/Day06_分离思想与platform/hello_platform_dev.ko \
	$(EXP)/Day06_分离思想与platform/hello_platform_drv.ko \
	$(EXP)/Day07_多设备与drvdata/led_platform_dev.ko \
	$(EXP)/Day07_多设备与drvdata/led_platform_drv.ko \
	$(EXP)/Day08_设备树匹配/led_dts_drv.ko \
	$(EXP)/Day09_pinctrl与GPIO/led_gpio_drv.ko \
	$(EXP)/Day15_Input子系统/key_input_drv.ko \
	$(EXP)/Day15_Input子系统/key_chrdev_drv.ko \
	$(EXP)/Day16_sysfs属性/param_demo_dev.ko \
	$(EXP)/Day16_sysfs属性/param_demo_drv.ko

# 用户态测试程序：由 init fork+exec 运行，不是 init 的一部分
USERPROGS := \
	$(EXP)/Day15_Input子系统/key_input_test \
	$(EXP)/Day16_sysfs属性/param_test

EVENT_KO := module/event_demo.ko

.PHONY: all clean modules initramfs dtb event-module

all: initramfs

dtb:
	bash build-dtb.sh

event-module:
	$(MAKE) -C $(KDIR) M=$(PWD)/module ARCH=arm CROSS_COMPILE=$(CROSS) modules

# 必须声明 init.c 为先决条件：没有先决条件的目标在文件已存在时永远被视为最新，
# 导致改了 init.c 也静默沿用旧二进制（2026-10-31 实测踩到，Day15 标记全部未执行）
init: init.c
	$(CC) -static -O2 -Wall -o init init.c

modules:
	cd $(EXP) && for d in Day02_* Day03_* Day04_* Day05_* Day06_* Day07_* Day08_* Day09_* Day15_* Day16_*; do \
		$(MAKE) -s -C $$d all KDIR=$(KDIR) KERN_DIR=$(KDIR) \
			ARCH=arm CROSS_COMPILE=$(CROSS); \
	done

initramfs: init modules event-module
	rm -rf rootfs initramfs.cpio.gz
	mkdir -p rootfs/proc rootfs/sys rootfs/dev
	cp init rootfs/init
	cp $(MODULES) rootfs/
	cp $(EVENT_KO) rootfs/
	cp $(USERPROGS) rootfs/
	cd rootfs && find . -print0 | cpio --null -o -H newc 2>/dev/null | gzip -9 > ../initramfs.cpio.gz
	@ls -lh initramfs.cpio.gz

clean:
	rm -rf rootfs initramfs.cpio.gz init 100ask-vexpress.dtb .100ask-vexpress.pre.dts
	rm -f module/*.ko module/*.o module/.*.cmd module/Module.symvers module/modules.order
