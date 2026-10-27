# SYS_03 阶段 3：网络编程
# 只依赖 libc，不需要额外库；`-Werror` 保证零警告才开始取证。

CC ?= gcc
CFLAGS ?= -O2 -g -std=c11 -Wall -Wextra -Werror
LDFLAGS ?=

BINS = tcp_basic udp_basic mux

all: $(BINS)

# net_common.h 是头文件依赖：改了它必须重编，否则取证用的还是旧语义
tcp_basic: tcp_basic.c net_common.h
	$(CC) $(CFLAGS) -o $@ tcp_basic.c $(LDFLAGS)

udp_basic: udp_basic.c net_common.h
	$(CC) $(CFLAGS) -o $@ udp_basic.c $(LDFLAGS)

mux: mux.c net_common.h
	$(CC) $(CFLAGS) -o $@ mux.c $(LDFLAGS)

clean:
	rm -f $(BINS) *.o

.PHONY: all clean
