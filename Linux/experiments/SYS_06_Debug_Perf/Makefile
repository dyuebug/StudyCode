# 阶段 6：调试、性能与工程化
#
# 本目录刻意用 GCC 自带能力替代缺失的工具链：
#   strace  -> src/mini_trace.c（ptrace 自实现）
#   gdb     -> src/crash.c + addr2line（SIGSEGV 处理器）
#   valgrind-> ASan / UBSan / TSan 三组
#   perf    -> src/bench.c（clock_gettime + getrusage）+ src/perf_probe.c（证明 perf_event_open 受限）

CC      ?= gcc
BASE    := -g -Wall -Wextra -Werror -std=c11
OPT     := -O2
BIN     := build
SRC     := src

.PHONY: all clean check

all: $(BIN)/mini_trace $(BIN)/victim $(BIN)/crash \
     $(BIN)/sani_asan $(BIN)/sani_ubsan $(BIN)/sani_tsan \
     $(BIN)/bench $(BIN)/perf_probe

$(BIN):
	mkdir -p $(BIN)

# 普通二进制：优化开满
$(BIN)/mini_trace: $(SRC)/mini_trace.c | $(BIN)
	$(CC) $(BASE) $(OPT) -o $@ $<

$(BIN)/victim: $(SRC)/victim.c | $(BIN)
	$(CC) $(BASE) $(OPT) -o $@ $<

$(BIN)/bench: $(SRC)/bench.c | $(BIN)
	$(CC) $(BASE) $(OPT) -o $@ $<

$(BIN)/perf_probe: $(SRC)/perf_probe.c | $(BIN)
	$(CC) $(BASE) $(OPT) -o $@ $<

# 崩溃回溯：-O1 且保留帧指针，-rdynamic 让动态符号表带上函数名
$(BIN)/crash: $(SRC)/crash.c | $(BIN)
	$(CC) $(BASE) -O1 -fno-omit-frame-pointer -rdynamic -o $@ $<

# 三组 sanitizer：-O1 + 帧指针，保证报告里的栈帧可读
# -fno-builtin-malloc/-free：否则 GCC 能证明"分配后既不读也不 free"时把整对调用消掉，
# 泄漏用例会编译成一条 ret，LSan 什么都看不到（实测踩过，README §8.1）
$(BIN)/sani_asan: $(SRC)/sani_asan.c | $(BIN)
	$(CC) $(BASE) -O1 -fno-omit-frame-pointer -fno-builtin-malloc -fno-builtin-free \
	  -fsanitize=address -o $@ $<

$(BIN)/sani_ubsan: $(SRC)/sani_ubsan.c | $(BIN)
	$(CC) $(BASE) -O1 -fno-omit-frame-pointer -fsanitize=undefined -fno-sanitize-recover=all -o $@ $<

$(BIN)/sani_tsan: $(SRC)/sani_tsan.c | $(BIN)
	$(CC) $(BASE) -O1 -fno-omit-frame-pointer -fsanitize=thread -o $@ $< -lpthread

# 确认三个 sanitizer 运行时真的被链接进来（缺库时 make 必须失败，不能静默降级）
check: all
	@for b in $(BIN)/sani_asan $(BIN)/sani_ubsan $(BIN)/sani_tsan; do \
	  if nm -D "$$b" | grep -qE '__asan_init|__ubsan_handle|__tsan_init'; then \
	    echo "S06|build|sanitizer|bin=$$b|linked=1"; \
	  else \
	    echo "S06|build|sanitizer|bin=$$b|linked=0"; exit 1; \
	  fi; \
	done

clean:
	rm -rf $(BIN)
