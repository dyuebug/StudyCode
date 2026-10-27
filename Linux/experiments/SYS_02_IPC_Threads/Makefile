# SYS_02 阶段 2：IPC 与线程
#
# make            编译全部（-Wall -Wextra -Werror，零警告）
# make tsan       额外编出 TSan 版 race_detect_tsan（-fsanitize=thread）
# make clean
#
# 注意 GNU make 内建变量 CC 默认是 cc，会用低版本别名覆盖 gcc；这里只在
# 用户没有显式指定时才改成 gcc。

ifeq ($(origin CC),default)
CC := gcc
endif

CFLAGS  ?= -O2 -g -std=c11 -Wall -Wextra -Werror
LDLIBS  += -pthread
CFLAGS  += -pthread

BINS := ipc_basic condvar race_detect deadlock threadpool
TSAN_BIN := race_detect_tsan

.PHONY: all tsan clean

all: $(BINS)

$(BINS): %: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

# TSan 需要 -O1 附近的优化级别与调试信息；和主构建分开，避免污染普通产物的时序。
tsan: $(TSAN_BIN)

$(TSAN_BIN): race_detect.c
	$(CC) -O1 -g -std=c11 -Wall -Wextra -Werror -fsanitize=thread \
		-o $@ $< -pthread

clean:
	rm -f $(BINS) $(TSAN_BIN)
