# Linux 调试与性能定位：速查表

> 对应 `CLAUDE.md` §3 阶段 6。实验：[`SYS_06_Debug_Perf`](../experiments/SYS_06_Debug_Perf/README.md)
> （24 用例，判定器 **101/101**，负向验证 23/23）。
> **前提事实**：本机 `strace`/`ltrace`/`gdb`/`valgrind`/`perf`/`cmake` **全部缺失**，
> 因此本表的所有工具都是**替代实现**，且每条都写明观测边界。

## 1. 一句话定义

"有工具就信工具"是错的：先确认**工具在不在**、**能力够不够**，再确认**它看见的边界在哪**。
三个缺口都能自己补，补出来的层要主动声明"我看不见什么"。

## 2. 工具缺口与替代

| 缺什么 | 替代实现 | 观测边界（必读） |
| --- | --- | --- |
| `strace` | `ptrace` + `PTRACE_GET_SYSCALL_INFO` | **看不见 vDSO**；只跟子进程（未做 `PTRACE_ATTACH` 任意进程） |
| `gdb` | `sigaltstack` + `backtrace()` + `dladdr()` + `addr2line` | 内联帧无法还原；`-g` 与 `-O` 冲突时行号会漂 |
| `valgrind` | ASan / UBSan / TSan | 只能发现被编译器**保留下来**的缺陷（见 §5） |
| `perf` | `clock_gettime` / `getrusage` / `socketpair` 往返 | 只有同机相对值，没有硬件 PMU 事件 |
| `cmake` | 直接 `gcc` 命令 | 无 |

## 3. 关键读数

### 3.1 vDSO 盲区（最值得记的一条）

`syscalls=1034` 中 `clock_gettime=1000`；另外 1000 次走 vDSO 的调用**零停点**。
差值 34 = 加载器日志开销。

**含义**：任何基于 `ptrace` 的追踪器都会系统性漏掉 vDSO 调用。要看见它，只能强制走真正的系统调用
（`syscall(SYS_clock_gettime, ...)`）。

### 3.2 崩溃回溯

```text
crash-null: on_sig(crash.c:52) → f3(crash.c:80) → f2(crash.c:85) → f1(crash.c:90) → main(crash.c:126)
crash-deep: frames=64，其中 62 帧落在 deep(crash.c:99)
```

**地址减法规则相反**（首版搞反，`f3` 被解成 `frame_dummy|crtstuff.c:?`）：

- 故障 PC（`ucontext` 里的 `REG_RIP`）：**不减 1**；
- 返回地址（`backtrace()` 给出的）：**必须减 1**，否则落到下一行。

同一帧用绝对地址直接查 `addr2line` 得 `??`——`PIE` 下必须先减去模块基址（`dladdr`）。

### 3.3 sanitizer 的失败形态

| 工具 | 用例 | 结果 |
| --- | --- | --- |
| ASan | `heap-buffer-overflow` / `heap-use-after-free` / `stack-overflow` / `leak` | 各 `rc=1` |
| UBSan | 移位越界 / 有符号溢出 / 除零 / 空指针解引用 | 各打印原文（`shift exponent 40…` 等） |
| TSan | 竞争 `rc=66` vs `atomic`/`mutex` `rc=0`，`result=expect=400000` | 竞争可复现 |

### 3.4 基准（同机相对值）

| 操作 | 读数 |
| --- | --- |
| vDSO `clock_gettime` | 15.66 ns |
| 真正系统调用 | 41.64 ns（**2.66×**） |
| 缺页 | `first=16384≈pages`、`second=0` |
| 线程上下文切换 | 42354.8 ns/次（≈1017× syscall） |

`ctxswitch` 21 µs/次偏高且**成因未定位**，只作同机相对比较，不写绝对结论。

### 3.5 `perf_event_open` 的权限分层

| 范围 | 结果 |
| --- | --- |
| 自身 | `self_fd=3`，`cycles=80018326`、`instructions=100000081`、`IPC=1.25` |
| 全系统 / 其他进程 | `EACCES(13)` |

**缺的是权限和二进制，不是计数能力**——`perf_event_open` 本身可用。

## 4. fd 收支与资源泄漏

| 用例 | `net_fd` | 判据 |
| --- | --- | --- |
| `fdok` | 0 | `opens - closes = 0` |
| `fdleak` | 2 | `opens - closes = 2` |

方法：**在进程内计数 `open`/`close`，比在外部数 `/proc/<pid>/fd` 更可靠**（后者受采样时刻影响）。

## 5. 编译器会把你放进去的缺陷删掉

两处实测推翻：

1. `-O1` 删除死存储，ASan 首版**看不见**越界；
2. `-O1` 把"分配后不用"整对消掉——`objdump` 里 `leak_alloc` **只剩一条 `ret`**，
   最终靠 `volatile` 函数指针才让 LSan 报出 64 B 泄漏。

**推论**：sanitizer 没报 ≠ 缺陷不存在。写缺陷用例时先用 `objdump` 确认缺陷代码还在。

## 6. 常见错误与陷阱

| 现象 | 根因 | 排查 |
| --- | --- | --- |
| 回溯符号全是 `??` | 传了绝对地址，或 PIE 未减基址 | `dladdr` 取基址；返回地址减 1 |
| 追踪器"少了很多调用" | vDSO | 强制 `syscall()` 对照 |
| sanitizer 无输出 | 缺陷被优化掉 | `objdump` 确认；`volatile` / 关 `-O` |
| `perf_event_open` 全 `EACCES` | 缺 `CAP_PERFMON`/`perf_event_paranoid` | 先用自身范围（pid=0）验证能力 |
| 基准数字不可复现 | 未固定 CPU/未多次采样 | 记录环境与采样次数，只比同机相对值 |
| 泄漏检测靠"再看一眼 `/proc/fd`" | 采样时刻错过峰值 | 进程内计 `open`/`close` |

## 7. 当前验证状态

- **实测**：`ptrace` 追踪与 vDSO 盲区、`backtrace`+`addr2line` 回溯（含 `crash-deep` 64 帧）、
  ASan/UBSan/TSan 三类、四组基准、`perf_event_open` 权限分层。
- **未验证**：非子进程的 `PTRACE_ATTACH`、root 下的系统级 `perf`、内联帧符号化、
  `ulimit -s` 参数扫描；`ctxswitch` 偏高成因未定位。

## 证据来源

- 实验：[`experiments/SYS_06_Debug_Perf/`](../experiments/SYS_06_Debug_Perf/README.md)
- 基准：[`benchmarks/SYS_06_Debug_Perf_2026-10-27.md`](../benchmarks/SYS_06_Debug_Perf_2026-10-27.md)
- 日志：[`daily_logs/2026-10-27_第28天_日志.md`](../daily_logs/2026-10-27_第28天_日志.md)
