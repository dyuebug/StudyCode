# SYS_04 · Reactor 三种形态
#
# 三个目标：reactor_srv（服务端加模式开关）、loadgen（多连接压测客户端）。
# 服务端严格按 -Wall -Wextra -Werror 编译；警告即失败，不留"下次再清"。
CC      ?= cc
CFLAGS  ?= -O2 -g -std=c11 -Wall -Wextra -Werror
LDLIBS  := -lpthread

all: reactor_srv loadgen

reactor_srv: server.c reactor.c reactor.h
	$(CC) $(CFLAGS) -o $@ server.c reactor.c $(LDLIBS)

loadgen: loadgen.c
	$(CC) $(CFLAGS) -o $@ loadgen.c $(LDLIBS)

clean:
	rm -f reactor_srv loadgen

.PHONY: all clean
