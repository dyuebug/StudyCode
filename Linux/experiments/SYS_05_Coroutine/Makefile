# SYS_05 · 协程
#
# 两个 C++ 目标（协程基础 + epoll 调度器）与一个 C 客户端。
# 服务端与基础设施按 -Wall -Wextra -Werror 编译，警告即失败。
CXX      ?= g++
CC       ?= cc
CXXFLAGS ?= -O2 -g -std=c++20 -Wall -Wextra -Werror
CFLAGS   ?= -O2 -g -std=c11 -Wall -Wextra -Werror
LDLIBS   := -lpthread

all: coro_basics coro_sched client

coro_basics: basics.cpp coro.cpp coro.h
	$(CXX) $(CXXFLAGS) -o $@ basics.cpp coro.cpp $(LDLIBS)

coro_sched: sched.cpp coro.cpp coro.h
	$(CXX) $(CXXFLAGS) -o $@ sched.cpp coro.cpp $(LDLIBS)

client: client.c
	$(CC) $(CFLAGS) -o $@ client.c $(LDLIBS)

clean:
	rm -f coro_basics coro_sched client

.PHONY: all clean
