# SYS_06 · 调试与性能基准记录（2026-10-27）

## 1. 目的与范围

**这不是 CPU/IO 压测，不产生 QPS、吞吐或"本机性能多少"的结论。** 本记录只回答四个
可对照的问题：

1. 同一个 `clock_gettime`，走 vDSO 与强制进内核，差多少倍？（这是"工具看不见"的代价量级）
2. 首次缺页与再次访问同一批页，差多少倍？（这是"一页一次缺页"前提是否成立的自检）
3. `socketpair` 往返一次与一次真系统调用，差几个数量级？
4. `perf_event_paranoid=2` 下，`perf_event_open` 到底在哪一层被拒？

## 2. 环境

| 项 | 值 |
| --- | --- |
| 内核 | `6.6.87.2-microsoft-standard-WSL2` x86_64 |
| 发行版 | WSL2 Ubuntu |
| 编译器 | `gcc 15.2.0`，基线 `-g -Wall -Wextra -Werror -std=c11`，`bench`/`perf_probe` 追加 `-O2` |
| CPU | `nproc=32`（WSL2 虚拟，**未绑定核、未隔离**，存在跨核迁移） |
| 权限 | `kernel.perf_event_paranoid=2`、`yama/ptrace_scope=1`、非 root |
| 工具 | `perf` / `strace` / `gdb` / `valgrind` / `cmake` **均未安装**（`S06\|env\|tool=*` 有记录） |

## 3. 方法

```bash
cd experiments/SYS_06_Debug_Perf
./run.sh --judge                     # 判定已有 logs/s06.log
./run.sh bench-vdso                  # 单跑一个用例，只出原始读数
```

- **三轮取中位**：`bench` 每个用例跑 3 轮，最后打印 `median_ns`；原始三行都留在日志里，
  便于看离散度（本例三行差 < 2%）。
- **计时器**：`CLOCK_MONOTONIC`，包住 N 次操作整体计时后除以 N。不逐次计时（那会把测量开销算进去）。
  两个耗时用例分别测 `clock_gettime(CLOCK_MONOTONIC)`（走 vDSO）与 `syscall(SYS_getpid)`（真进内核）。
- **缺页计数**：`getrusage(RUSAGE_SELF).ru_minflt` 前后取差；两遍遍历同一块 64 MiB 区域
  （16384 页，`MADV_NOHUGEPAGE` 关掉大页）。
- **上下文切换**：`socketpair` + `fork`，父子各 20000 次阻塞往返；`/proc/self/status`
  的 `voluntary_ctxt_switches` 读前后差。
- **硬件计数**：`perf_event_open`（`pid=0`、`exclude_kernel=1`）数 `PERF_COUNT_HW_CPU_CYCLES`
  与 `PERF_COUNT_HW_INSTRUCTIONS`，`work(20000000)` 个线性同余迭代。

## 4. 原始数据

### 4.1 vDSO vs 真系统调用

```
S06|bench|case=vdso|run=0|ns_per_op=15.62|acc=101465
S06|bench|case=vdso|run=1|ns_per_op=15.88|acc=99646
S06|bench|case=vdso|run=2|ns_per_op=15.66|acc=100071
S06|bench|case=vdso|median_ns=15.66|ops=200000

S06|bench|case=syscall|run=0|ns_per_op=41.47|acc=0
S06|bench|case=syscall|run=1|ns_per_op=41.68|acc=0
S06|bench|case=syscall|run=2|ns_per_op=41.64|acc=0
S06|bench|case=syscall|median_ns=41.64|ops=200000
```

比值 **2.66×**（判据只要求 ≥ 1.5×）。

### 4.2 缺页：首次 vs 再次

```
S06|bench|case=pagefault|run=0|pages=16384|first_minflt=16386|second_minflt=0|first_ns_per_page=1628.0|second_ns_per_page=10.8
S06|bench|case=pagefault|run=1|pages=16384|first_minflt=16384|second_minflt=0|first_ns_per_page=638.5|second_ns_per_page=9.8
S06|bench|case=pagefault|run=2|pages=16384|first_minflt=16384|second_minflt=0|first_ns_per_page=600.0|second_ns_per_page=9.6
```

`first_minflt` 与 `pages=16384` 基本相等（run=0 多 2 次，来自基准自身的初始化），
`second_minflt=0`——**第二遍一页都没缺**，前提成立。

### 4.3 上下文切换

```
S06|bench|case=ctxswitch|roundtrips=20000|ok=1|ns_per_rt=42354.8|ns_per_switch=21177.4|voluntary=20456|nonvoluntary=0
```

### 4.4 硬件计数

```
S06|perf|paranoid=2|self_fd=3|self_errno=0|sys_fd=-1|sys_errno=13|other_fd=-1|other_errno=13
S06|perf|counters|cycles=80018326|instructions=100000081|ipc=1.25|insn_fd=4|sink=...
```

`instructions / 20000000 ≈ 5.0` 条/迭代，与循环体（乘、加常量、加 i、加 1、比较跳转）量级一致。

## 5. 可下的结论与不可下的结论

**可以下（同一次运行内的相对关系，判据已固化在 `judge.py`）：**

| 关系 | 实测 | 说明 |
| --- | --- | --- |
| `syscall > 1.5 × vdso` | 2.66× | vDSO 是"不进内核"的快速路径，差 2 倍以上说明差异真实存在 |
| `first_minflt ≈ pages` | 16384/16386 | 缺页度量在这台机器上可用 |
| `second_minflt ≈ 0` | 0 | 页已映射，第二遍不缺页 |
| `ns_per_rt > 10 × syscall` | 1017× | 一次往返含两次调度与两次唤醒，量级差正确 |
| `paranoid ≥ 1 ⇒ systemwide EACCES` | 2 / 13 | 权限分层成立 |

**不可下（已在 README §9 与本节标注）：**

- 不能把 15.66 ns / 41.64 ns 当"这台机器的系统调用开销"——WSL2 虚拟化层与调度抖动都算在里面，
  换一个内核、换一台机器都会变。
- 不能把 21.2 µs/次上下文切换当典型值。裸机通常是个位数微秒，本值**偏高且成因未定位**
  （WSL2 上跨核迁移与虚拟化退出成本是主要嫌疑，未做绑定核对照）。
- 不能从 `cycles=80018326` 推出任何"指令级优化空间"结论——`exclude_kernel=1`，
  且未记录 CPU 频率，无法换算时间。
- `vdso` 与 `syscall` 的 `acc` 含义不同（前者累加 `tv_nsec` 的奇偶，后者累加 `getpid` 返回值），
  两列数值不可直接比较，它只是防优化消除的副作用检查。

## 6. 局限

1. **单机单次**：每组 3 轮，没有跨机器、跨内核版本对照。
2. **未绑定核**：`taskset` 未使用，切换成本里混有跨核迁移。
3. **未做统计显著性**：3 轮中位数的离散度 < 2%（`vdso`）与 < 0.5%（`syscall`），
   但这只说明本轮稳定，不是置信区间。
4. **缺页只测匿名页**：文件映射缺页（`majflt` / 页缓存命中）未覆盖。
5. **`perf` 只测了自身用户态**：systemwide 与 root 下的行为仅证明"被拒"，未证明"可用时怎么用"。
