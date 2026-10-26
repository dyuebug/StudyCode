#!/usr/bin/env bash
# QEMU 仿真验证脚本（在 WSL / Linux 下运行；QEMU 装在此环境，不在 Windows 侧）
#
# 用法：
#   ./run.sh            # 跑全部（probe + exp04 + exp05 + 时基速率校准）
#   ./run.sh probe      # 只跑外设建模能力探针
#   ./run.sh exp04      # F103_04 UART 轮询回显
#   ./run.sh exp05      # F103_05 UART 中断 + 环形缓冲
#   ./run.sh tickrate   # 量化"固件毫秒"与真实时间的比例
#
# 前置：
#   - qemu-system-arm >= 10.x（stm32vldiscovery 机器）
#   - 两个实验已用 -DF103_QEMU=ON 构建到 build-qemu/
#
# 输出同时写屏并保存到 logs/ 目录，便于作为回归证据。
#
# 为什么要 sleep 1 再喂数据：固件启动后到 f103_uart_init() 之间有几十微秒；
# 但 QEMU 的串口后端在 USART_CR1.RE 置位前会拒绝接收（can_receive 返回 0），
# 若输入早早全部到达，可能滞留在管道里，表现为 rx=0。延迟投放可稳定复现。

set -u

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
LINUX_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
LOG_DIR="$SCRIPT_DIR/logs"
QEMU_BIN="${QEMU_BIN:-qemu-system-arm}"
QEMU_ARGS=(-M stm32vldiscovery -display none -monitor none -serial stdio)

mkdir -p "$LOG_DIR"

need_qemu() {
  command -v "$QEMU_BIN" >/dev/null 2>&1 || {
    echo "找不到 $QEMU_BIN；请在 WSL 内安装 qemu-system-arm（apt install qemu-system-arm）" >&2
    exit 1
  }
}

need_elf() {
  [ -f "$1" ] || {
    echo "缺少 $1；请先在该实验目录执行：" >&2
    echo "  cmake -S . -B build-qemu -G Ninja -DF103_QEMU=ON \\" >&2
    echo "    -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake" >&2
    echo "  cmake --build build-qemu" >&2
    exit 1
  }
}

# run_case <日志名> <ELF 路径> <超时秒> <喂数据的命令>
run_case() {
  local name="$1" elf="$2" secs="$3" feeder="$4"
  need_elf "$elf"
  echo "=== $name ==="
  if [ -n "$feeder" ]; then
    eval "$feeder" | timeout "$secs" "$QEMU_BIN" "${QEMU_ARGS[@]}" -kernel "$elf" \
      2>&1 | tr -d '\r' | tee "$LOG_DIR/$name.log"
  else
    timeout "$secs" "$QEMU_BIN" "${QEMU_ARGS[@]}" -kernel "$elf" </dev/null \
      2>&1 | tr -d '\r' | tee "$LOG_DIR/$name.log"
  fi
  echo
}

run_probe() {
  run_case probe "$SCRIPT_DIR/probe/build/qemu_stm32_probe.elf" 5 ""
}

run_exp04() {
  local dir="$LINUX_ROOT/experiments/F103_04_UART_Echo"
  run_case exp04_echo "$dir/build-qemu/f103_04_uart_echo.elf" 8 \
    "{ sleep 1; printf 'hello\r\n'; sleep 1; printf 'xyz\r\n'; sleep 4; }"
}

run_exp05() {
  local dir="$LINUX_ROOT/experiments/F103_05_UART_IRQ_Ring"
  run_case exp05_irq_ring "$dir/build-qemu/f103_05_uart_irq_ring.elf" 7 \
    "{ sleep 1; printf 'alpha\r\n'; sleep 1; printf 'beta\r\n'; sleep 4; }"
  # 300 字节突发：验证环形缓冲溢出路径。QEMU 有背压，drop 预期仍为 0
  # （它不会像真 UART 那样在 RDR 未读时置 ORE），这条记录本身就是能力边界的证据。
  run_case exp05_burst300 "$dir/build-qemu/f103_05_uart_irq_ring.elf" 6 \
    "{ sleep 1; head -c 300 /dev/zero | tr '\0' 'A'; printf '\r\n'; sleep 4; }"
}

# 固件里 SysTick 的 LOAD 按 8 MHz 计算，而 QEMU 的 stm32vldiscovery 以 24 MHz
# 驱动 SYSCLK。两者相差 3 倍，表现为"固件毫秒"跑得比真实时间快。
# 本用例把这一点量化成可复现的数字，避免用它去做实时性结论。
run_tickrate() {
  local dir="$LINUX_ROOT/experiments/F103_04_UART_Echo"
  local elf="$dir/build-qemu/f103_04_uart_echo.elf"
  local secs=10
  local out="$LOG_DIR/exp04_tick_rate.log"
  need_elf "$elf"
  echo "=== exp04_tick_rate（真实 ${secs} s） ==="
  timeout "$secs" "$QEMU_BIN" "${QEMU_ARGS[@]}" -kernel "$elf" </dev/null \
    2>&1 | tr -d '\r' > "$out"
  local last
  last=$(sed -n 's/^\[\([0-9]\+\) ms\].*/\1/p' "$out" | tail -1)
  sed -n '/alive/p' "$out" | tail -1
  if [ -n "$last" ]; then
    local denom=$((secs * 1000))
    printf '固件 %s ms / 真实 %s ms ≈ %d.%02d 倍（24 MHz ÷ 8 MHz 应为 3.00）\n' \
      "$last" "$denom" $((last / denom)) $((last % denom * 100 / denom))
  fi
  echo
}

need_qemu
case "${1:-all}" in
  probe)    run_probe ;;
  exp04)    run_exp04 ;;
  exp05)    run_exp05 ;;
  tickrate) run_tickrate ;;
  all)      run_probe; run_exp04; run_exp05; run_tickrate ;;
  *)        echo "未知用例：$1" >&2; exit 2 ;;
esac
