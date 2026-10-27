#!/usr/bin/env bash
# QEMU 仿真验证脚本（在 WSL / Linux 下运行；QEMU 装在此环境，不在 Windows 侧）
#
# 用法：
#   ./run.sh            # 跑全部（probe + exp04/05 + exp10–15 + aviation-link + 时基速率校准）
#   ./run.sh probe      # 只跑外设建模能力探针
#   ./run.sh exp04      # F103_04 UART 轮询回显
#   ./run.sh exp05      # F103_05 UART 中断 + 环形缓冲
#   ./run.sh exp10      # F103_10 FreeRTOS 任务 / 队列 / 互斥量
#   ./run.sh exp11      # F103_11 FreeRTOS 中断 → 任务（USART1 RXNE + 队列）
#   ./run.sh exp12      # F103_12 FreeRTOS 同步原语（信号量 / 事件组 / 中断合并）
#   ./run.sh exp13      # F103_13 FreeRTOS 软件定时器（单次 / 周期 / 到期漂移）
#   ./run.sh exp14      # F103_14 FreeRTOS 心跳与看门狗（监督逻辑；IWDG 未建模）
#   ./run.sh exp15      # F103_15 FreeRTOS 栈溢出检出（钩子触发后停机，以日志判定成败）
#   ./run.sh aviation   # projects/aviation-node 航空主题节点（PC 注入帧 → 模式状态机；跑完自动判定）
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

# 探针多给 USART2 一个宿主出口：`-serial stdio` 占 serial0，`-serial file:` 是 serial1。
# 探针 [13] 往 USART2 写 8 字节特征串，这个文件非空就说明第二串口可用。
run_probe() {
  local elf="$SCRIPT_DIR/probe/build/qemu_stm32_probe.elf"
  local bin="$LOG_DIR/probe_usart2.bin"

  need_elf "$elf"
  rm -f "$bin"
  echo "=== probe ==="
  timeout 5 "$QEMU_BIN" "${QEMU_ARGS[@]}" -serial "file:$bin" -kernel "$elf" </dev/null \
    2>&1 | tr -d '\r' | tee "$LOG_DIR/probe.log"
  if [ -s "$bin" ]; then
    echo "USART2 回传 $(wc -c < "$bin") 字节：$(od -An -tx1 -N16 "$bin" | tr -s ' ')"
  else
    echo "USART2 无字节：QEMU 未把 USART2 接到第二个串口后端"
  fi
  echo
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

# FreeRTOS 只依赖 SysTick / PendSV / SVCall / 队列与堆，不碰未建模外设，
# 所以调度、抢占、队列、互斥量都能在 QEMU 里看到真实行为。
run_exp10() {
  local dir="$LINUX_ROOT/experiments/F103_10_FreeRTOS_Tasks"
  run_case exp10_freertos "$dir/build-qemu/f103_10_freertos_tasks.elf" 6 ""
}

# 中断 → 任务的交接：USART1 RXNE 在 QEMU 里是真实模型，因此
# xQueueSendFromISR / portYIELD_FROM_ISR 的效果可观测（含 lat_max 延迟）。
# 突发用例跑的是"行超长截断"路径，不是队列满丢弃（消费速度够快，丢不进去）。
run_exp11() {
  local dir="$LINUX_ROOT/experiments/F103_11_FreeRTOS_ISR_Queue"
  run_case exp11_isr_queue "$dir/build-qemu/f103_11_freertos_isr_queue.elf" 7 \
    "{ sleep 1; printf 'hello\r\n'; sleep 1; printf 'world\r\n'; sleep 4; }"
  run_case exp11_burst "$dir/build-qemu/f103_11_freertos_isr_queue.elf" 6 \
    "{ sleep 1; head -c 300 /dev/zero | tr '\0' 'A'; printf '\r\n'; sleep 4; }"
}

# 同步原语：二值/计数信号量、事件组、中断→信号量的合并行为。
# 只依赖 SysTick / PendSV / USART1（均已建模），因此全部路径都在仿真里可观测。
run_exp12() {
  local dir="$LINUX_ROOT/experiments/F103_12_FreeRTOS_Sync"
  # feeder 在 1 s（真实时间）投一行 7 字节：此时 core 早已阻塞在 s_uart 上。
  # 固件里的时间窗（800/200/300 tick）按 2.90 倍时基折算约 0.28/0.07/0.10 s，留足余量。
  run_case exp12_sync "$dir/build-qemu/f103_12_freertos_sync.elf" 7 \
    "{ sleep 1; printf 'hello\r\n'; sleep 4; }"
}

# 软件定时器：只依赖 SysTick/PendSV（已建模），守护任务与回调调度全部可观测。
# 全流程固件时间约 2900 ms，按 2.90 倍时基折算约 1 s 真实时间；给 5 s 余量。
run_exp13() {
  local dir="$LINUX_ROOT/experiments/F103_13_FreeRTOS_Timers"
  run_case exp13_timers "$dir/build-qemu/f103_13_freertos_timers.elf" 5 ""
}

# 心跳与看门狗：只有"监督逻辑"这一半可在仿真里验证（心跳、超时判定、停喂、
# 恢复动作全部依赖 SysTick/PendSV/信号量，均已建模）。
# IWDG 计时与复位、RCC->CSR 复位标志未建模（探针 [11]/[12] 实测读回全 0），
# 所以本日志不能证明"看门狗真的会复位"，只能证明"该停喂的时候停了"。
# 全流程固件时间约 3200 ms，按 2.90 倍时基折算约 1.1 s 真实时间；给 5 s 余量。
run_exp14() {
  local dir="$LINUX_ROOT/experiments/F103_14_FreeRTOS_Watchdog"
  run_case exp14_watchdog "$dir/build-qemu/f103_14_freertos_watchdog.elf" 5 ""
}

# 栈溢出检出：只依赖调度器 + trace/打印，QEMU 完全够用。
# 与其它用例的成败判据不同：本实验的**正常终点**是钩子里的死循环，
# 所以 QEMU 一定被 timeout 杀掉（日志末尾 "terminating on signal 15" 是预期，
# 不是失败）。判定只能看日志内容，因此下面用两条 grep 代替人工读日志。
run_exp15() {
  local dir="$LINUX_ROOT/experiments/F103_15_FreeRTOS_StackOverflow"
  local log="$LOG_DIR/exp15_stackoverflow.log"

  run_case exp15_stackoverflow "$dir/build-qemu/f103_15_freertos_stackoverflow.elf" 5 ""

  if ! grep -q '\[STACK OVERFLOW\] 任务 probe 的栈被写穿' "$log"; then
    echo "判定失败：日志中没有栈溢出钩子的报告（configCHECK_FOR_STACK_OVERFLOW 未生效？）" >&2
    return 1
  fi
  if grep -q '\[so\] FAIL' "$log"; then
    echo "判定失败：日志中出现 FAIL 行" >&2
    return 1
  fi
  echo "判定通过：钩子报出任务名 probe，全程无 FAIL 行"
}

# 航空主题地面仿真节点（CLAUDE.md §12 P1 的可仿真部分）：
# PC 侧 Python 注入器按 50 Hz 发帧，固件做帧校验 → 有效性判定 → 模式状态机
# → 降级/恢复。只依赖 USART1/SysTick/PendSV（均已建模），因此整条链路可观测。
# 剧本真实时间约 8.2 s（固件时间约 24 s），给 14 s 余量。
#
# 构建侧前提：动物件需带"任务阻塞"调试注入。该项目 CMakeLists 在 -DF103_QEMU=ON
# 下默认 AV_DEBUG_BLOCK_RX_MS=800，所以常规 QEMU 构建即可；带 -DAV_DEBUG_BLOCK_RX_MS=0
# 重建的镜像不会打印 [debug]，expect.py 的 C14 会自动转成"本条不适用"。
# 跑完由 expect.py 从日志抽取不变量做机器判定（不比对硬编码时刻——跨运行漂移
# 随事件靠后而增大，逐字比对必然假失败）；判定失败时以非零码中止，不再需要人工读日志确认。
run_aviation() {
  local dir="$LINUX_ROOT/projects/aviation-node"
  run_case aviation_node "$dir/build-qemu/aviation_node.elf" 14 \
    "python3 -u '$SCRIPT_DIR/aviation/inject.py'"
  python3 "$SCRIPT_DIR/aviation/expect.py" "$LOG_DIR/aviation_node.log"
}

# 下行二进制通道（USART2）：同一条剧本，但把 USART2 接到宿主文件而不是终端。
# `-serial stdio` 仍是 serial0（USART1：ASCII 日志 + 注入输入），追加的
# `-serial file:` 成为 serial1（USART2：纯二进制）。二进制写进终端会把日志流打乱，
# 所以这里必须落文件。跑完由 link.py 解帧，并用 --ascii 与上行 [stat] 交叉核对。
# 前提已实测：探针 [13] 证明该 QEMU 机型确实把 USART2 接到了第二个后端。
run_aviation_link() {
  local dir="$LINUX_ROOT/projects/aviation-node"
  local elf="$dir/build-qemu/aviation_node.elf"
  local bin="$LOG_DIR/aviation_downlink.bin"

  need_elf "$elf"
  rm -f "$bin"
  echo "=== aviation_link（USART1 → 终端，USART2 → $bin） ==="
  python3 -u "$SCRIPT_DIR/aviation/inject.py" \
    | timeout 14 "$QEMU_BIN" "${QEMU_ARGS[@]}" -serial "file:$bin" -kernel "$elf" \
      2>&1 | tr -d '\r' | tee "$LOG_DIR/aviation_node.log"
  echo
  echo "=== link.py ==="
  ( cd "$SCRIPT_DIR/aviation" && python3 link.py "$bin" --ascii "$LOG_DIR/aviation_node.log" )
  local link_rc=$?
  echo "=== expect.py（同一次运行的上行日志） ==="
  python3 "$SCRIPT_DIR/aviation/expect.py" "$LOG_DIR/aviation_node.log"
  local expect_rc=$?
  # 两条通道分别判定，任一失败即整体失败：下行坏了不能因为上行还绿就放过
  [ "$link_rc" -eq 0 ] && [ "$expect_rc" -eq 0 ]
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
  exp10)    run_exp10 ;;
  exp11)    run_exp11 ;;
  exp12)    run_exp12 ;;
  exp13)    run_exp13 ;;
  exp14)    run_exp14 ;;
  exp15)    run_exp15 || exit 1 ;;
  aviation) run_aviation || exit 1 ;;
  aviation-link) run_aviation_link || exit 1 ;;
  tickrate) run_tickrate ;;
  all)      run_probe; run_exp04; run_exp05; run_exp10; run_exp11; run_exp12; run_exp13; run_exp14; run_exp15 || exit 1; run_aviation_link || exit 1; run_tickrate ;;
  *)        echo "未知用例：$1" >&2; exit 2 ;;
esac
