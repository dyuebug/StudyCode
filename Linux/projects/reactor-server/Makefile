# reactor-server —— 宿主机原生构建（Linux / WSL2）
#
# 为什么不用 CMake：本机 WSL2 内没有 cmake（CLAUDE.md §12 P1.6 已记录该限制），
# 用 Makefile 可以一条命令复现；若日后装上 cmake，Makefile 仍是最小依赖。
#
# GNU make 内建 CC=cc，`CC ?= gcc` 永远不会生效（与 SYS_01、BSP_03 同一处坑），
# 所以这里用 origin 判定。
ifeq ($(origin CC),default)
CC := gcc
endif

CFLAGS  ?= -O2 -g -std=c11 -Wall -Wextra -Werror
CFLAGS  += -D_GNU_SOURCE -pthread
LDFLAGS ?=
LDFLAGS += -pthread

# 可选 sanitizer：SAN=address|thread|undefined
#   make check-asan / make check-tsan 会以独立构建目录重跑同一套用例，
#   产物落在 build-asan/ 与 build-tsan/，不与默认构建混用
#   （混用的后果是"改了代码但跑到旧二进制"，这里用不同的路径规避）。
# sanitizer 报告的退出码非 0（ASan 默认 1、TSan 默认 66），用例的 C4/C5
# 判据会因此失败——所以"有报告"和"用例失败"是同一条信号，不需要另设开关。
SAN ?=
SANFLAGS := $(if $(SAN),-fsanitize=$(SAN) -fno-omit-frame-pointer,)
# UBSan 默认是"报告后继续"，那会让 check-ubsan 的退出码失去意义；
# 加 -fno-sanitize-recover=all 让第一次未定义行为就中止进程，
# 与 ASan/TSan 的"有报告即失败"口径一致。
ifeq ($(SAN),undefined)
SANFLAGS += -fno-sanitize-recover=all
endif
CFLAGS  += $(SANFLAGS)
LDFLAGS += $(SANFLAGS)

BUILD   := build$(if $(SAN),-$(SAN),)
SRV     := $(BUILD)/reactor_srv
CLI     := $(BUILD)/loadgen
SRCS    := src/main.c src/server.c src/threadpool.c

.PHONY: all clean check check-asan check-tsan check-ubsan probe-asan probe-tsan probe-ubsan stress bench

all: $(SRV) $(CLI)

$(BUILD):
	mkdir -p $(BUILD)

$(SRV): $(SRCS) src/reactor.h | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDFLAGS)

$(CLI): tests/loadgen.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# 一条命令跑完全部用例并判定；日志落在 tests/logs/
check: all
	BUILD_DIR=$(BUILD) bash tests/run.sh

# sanitizer 生效性探针：这两个目标**必须以失败告终**。
# 若"通过"，说明工具没插桩，那么 check-asan / check-tsan 的绿灯毫无意义。
# 探针只写一个已知缺陷，不依赖被测代码。
probe-asan: | $(BUILD)
	@$(CC) -O1 -g -fsanitize=address -DPROBE_ASAN -pthread \
	    -o $(BUILD)/san_probe tests/san_probe.c
	@if ./$(BUILD)/san_probe >/dev/null 2>&1; then \
	    echo "probe-asan: 未报错 —— ASan 未生效，check-asan 的结论无效" >&2; exit 1; \
	else echo "probe-asan: OK（ASan 按预期报出 heap-use-after-free）"; fi

probe-tsan: | $(BUILD)
	@$(CC) -O1 -g -fsanitize=thread -pthread \
	    -o $(BUILD)/san_probe_tsan tests/san_probe.c
	@if ./$(BUILD)/san_probe_tsan >/dev/null 2>&1; then \
	    echo "probe-tsan: 未报错 —— TSan 未生效，check-tsan 的结论无效" >&2; exit 1; \
	else echo "probe-tsan: OK（TSan 按预期报出数据竞争）"; fi

probe-ubsan: | $(BUILD)
	@$(CC) -O1 -g -fsanitize=undefined -fno-sanitize-recover=all \
	    -DPROBE_UBSAN -o $(BUILD)/san_probe_ubsan tests/san_probe.c
	@if ./$(BUILD)/san_probe_ubsan >/dev/null 2>&1; then \
	    echo "probe-ubsan: 未报错 —— UBSan 未生效，check-ubsan 的结论无效" >&2; exit 1; \
	else echo "probe-ubsan: OK（UBSan 按预期报出 signed integer overflow）"; fi

# 同一套用例跑在 sanitizer 构建上。
#   ASan：抓越界、use-after-free、以及**泄漏**（服务端退出时整条生命周期必须收干净）
#   TSan：抓数据竞争——本项目的核心设计承诺是"连接结构只被主线程访问"，
#         这条承诺光靠读代码验证不了，必须有工具证据
# 两者都用独立构建目录，避免与默认构建互相覆盖；先跑探针再跑用例。
check-asan: probe-asan
	$(MAKE) SAN=address all
	ASAN_OPTIONS=detect_leaks=1 BUILD_DIR=build-address bash tests/run.sh

check-tsan: probe-tsan
	$(MAKE) SAN=thread all
	TSAN_OPTIONS=halt_on_error=1 BUILD_DIR=build-thread bash tests/run.sh

# UBSan 抓的是未定义行为（整数溢出、越界移位、空指针解引用等），
# 与 ASan/TSan 覆盖面不重叠，且 gcc 自带、不需要额外安装。
check-ubsan: probe-ubsan
	$(MAKE) SAN=undefined all
	UBSAN_OPTIONS=print_stacktrace=1 BUILD_DIR=build-undefined bash tests/run.sh

# 退出路径的重复尝试：两种在飞形态各跑若干轮，日志追加到 tests/logs/sigint_stress.log。
# 与 check 分开，因为它是"概率类"验证——不跑不代表有缺陷，跑过也不等于根治。
stress: all
	BUILD_DIR=$(BUILD) bash tests/sigint_stress.sh 12
	JOB_US=200000 SIG_DELAY_MS=50 BUILD_DIR=$(BUILD) bash tests/sigint_stress.sh 8

# 吞吐观测：固定 CPU 亲和性（服务端 = 一个物理核 0-1，客户端 = 另一个物理核的 cpu4），
# 每形态预热 1 次 + 取 5 次分位。与 check 分开：check 判"对不对"，bench 只出"多快"。
# 日志落在 tests/bench_logs/（**不是** tests/logs，后者每次 check 都会被清空）。
# 覆盖项：CPU_SRV / CPU_CLI / RUNS / WARMUP / THREADS，例如
#   THREADS=1 make bench        # 单 worker 对照
#   CPU_SRV=0-3 make bench      # 服务端多核对照
bench: all
	BUILD_DIR=$(BUILD) bash tests/bench.sh

clean:
	rm -rf build build-address build-thread build-undefined tests/logs tests/bench_logs
