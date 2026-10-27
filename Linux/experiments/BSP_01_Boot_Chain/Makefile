# BSP_01 启动链取证装置的 init 构建
#
# 产物是一个静态 ARM 可执行文件，随 initramfs 进内核，不依赖目标侧任何库。

# 注意：不能用 "CC ?= arm-linux-gnueabihf-gcc"——GNU make 有内建默认值 CC=cc，
# 那个默认值算"已定义"，?= 不会生效，直接 make 会悄悄编出宿主机架构的二进制。
# 只在内建默认值生效时才套用交叉编译器：命令行 CC=xxx 和环境变量 CC 仍然优先。
ifeq ($(origin CC),default)
CC := arm-linux-gnueabihf-gcc
endif

CFLAGS  ?= -O2 -g -Wall -Wextra -Werror
LDFLAGS ?= -static

TARGET := init

.PHONY: all clean

all: $(TARGET)

$(TARGET): init.c Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)
