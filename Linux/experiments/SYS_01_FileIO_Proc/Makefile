# 阶段 1：文件 I/O 与进程。宿主机原生编译，不做交叉编译。
#
# GNU make 内建 CC=cc，`CC ?= gcc` 永远不会生效（与 BSP_03 同一处坑）。
# 这里用 origin 判定，让 CC 只在我们没指定时才被改写。
ifeq ($(origin CC),default)
CC := gcc
endif

CFLAGS  ?= -O2 -g -std=c11 -Wall -Wextra -Werror
BINS    := fileio proc_life pipe_read minish

all: $(BINS)

$(BINS): %: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(BINS) *.o

.PHONY: all clean
