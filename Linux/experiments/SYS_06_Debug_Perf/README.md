# SYS_06 · 调试、性能与工程化（受限环境下的替代实现）

> 阶段 6。目标定的三件事——定位崩溃、看清系统调用、拿到可复现的性能数字——在**没有**
> `strace` / `gdb` / `valgrind` / `perf` / `cmake` 的机器上完成。
>
> 全部读数来自 `run.sh` 的一次完整运行（`logs/s06.log`，352 行）：
> 24 个用例，判定器 `judge.py` 101 项全通过，负向验证 `negtest.py` 23/23（原日志 `rc=0`、
> 21 条用例内变异 `rc=1`、缺日志 `rc=2`）。

---

## 1. 学习目标

学完并能自己复现以下每一条，才算完成：

1. 用 `ptrace` 自己写一个最小 `strace`，说清 `PTRACE_SYSCALL` 的一次调用为什么产生**两个**停点，
   以及怎么只用 `WSTOPSIG` 把系统调用停点与普通信号停点分开。
2. 解释**vDSO 盲区**：为什么 `clock_gettime(CLOCK_MONOTONIC)` 在 trace 里看不见，
   以及怎么用"强制真 syscall"的对照组把它变成读数。
3. 在没有 `gdb` 的情况下定位崩溃：`sigaltstack` + `SA_SIGINFO` + `backtrace()` + `dladdr()` + `addr2line`。
4. 说清回溯地址的两条规则：**故障 PC 不能减 1，返回地址必须减 1**；以及 PIE 下为什么
   绝对地址查符号表只会得到 `??`。
5. 用三行退出码区分 ASan / UBSan / TSan 的失败形态（1 / 1 / 66），并区分"越界"与"泄漏"两类报告。
6. 说明**编译器会消掉你要找的缺陷**：死存储被删、`malloc` 对整对被消，各自如何绕过。
7. 用 `perf_event_open` 的返回值与 `errno` 把"没有 perf"拆成**权限**与**二进制**两层，
   并证明自身用户态计数其实是可用的。
8. 在没有 `perf` 的情况下设计可复现的基准：vDSO vs syscall、缺页计数、
   socketpair 往返上下文切换，并说明每个数字的**边界**。

---

## 2. 环境与前置条件

| 项 | 值 |
| --- | --- |
| 系统 | WSL2 Ubuntu，`Linux 6.6.87.2-microsoft-standard-WSL2` x86_64 |
| 编译器 | `gcc (Ubuntu) 15.2.0`，`-std=c11`（本阶段无 C++） |
| 编译选项 | 基线 `-g -Wall -Wextra -Werror`；ASan/UBSan/TSan 用例各自追加对应 `-fsanitize=` |
| CPU / 权限 | `nproc=32`，`kernel.perf_event_paranoid=2`，`yama/ptrace_scope=1` |
| 工具可用性 | **有**：`make` `addr2line` `nm` `objdump` `readelf` `python3`；**无**：`strace` `ltrace` `gdb` `valgrind` `perf` `cmake`（`S06\|env\|tool=*` 全部记录在日志里） |
| 前置 | 阶段 1（`open`/`read`/`write`/`fork`/`execve`）、阶段 2（`pthread` 与数据竞争）、阶段 3（`socketpair`/`EAGAIN`） |

**不使用 `sudo` 安装任何东西**。缺的工具一律以"受限环境下的替代实现"完成，并把
"受限到什么程度"写成读数，而不是写成"没做"。

---

## 3. 核心概念

### 3.1 `ptrace` 版 strace 的三块拼图

追踪一个进程要三件事同时成立：

1. **建立追踪关系**：子进程 `PTRACE_TRACEME` + `raise(SIGSTOP)`，父进程 `waitpid` 等到它停下，
   然后 `execv`——只追踪到 `exec` 之后的目标程序，不追踪中间夹层。
2. **拿到系统调用停点**：`PTRACE_SETOPTIONS` 打开 `PTRACE_O_TRACESYSGOOD`，
   此后系统调用停点的 `WSTOPSIG` 是 `SIGTRAP | 0x80`（即 133），而普通 `SIGTRAP` 仍是 5。
   **没有这一位就无法区分**"撞了断点"和"进了系统调用"。
3. **读出参数与返回值**：`PTRACE_GET_SYSCALL_INFO`（request `0x420e`）一次给出
   `op`（ENTRY/EXIT）、`arch`、`ip`、`sp`，以及 entry 的 `nr + args[6]`、exit 的 `rval + is_error`。
   比自己按 arch 读寄存器可靠得多。

一次 `openat` 因此产生**两个**停点（进入、返回），统计时只能数 entry，否则数量翻倍。

### 3.2 vDSO 盲区：工具看不见的，恰好是最常用的

`clock_gettime(CLOCK_MONOTONIC)` 在 glibc 里走 **vDSO**——纯用户态读共享页，**根本不进内核**。
所以 `ptrace` 一个 syscall 停点都不会看到。这不是实现 bug，是这类工具的观测边界。

本实验把它变成对比读数（用例 `trace-vdso`）：

| 调用方式 | 次数 | trace 是否看见 |
| --- | --- | --- |
| `clock_gettime()`（glibc，走 vDSO） | 1000 | 否 |
| `syscall(SYS_clock_gettime, ...)`（强制进内核） | 1000 | 是，`clock_gettime=1000` |

判据不写死绝对数，而是三条关系：`900 ≤ 见到的次数 ≤ 1100`、`见到的次数 ≤ 程序自报的 raw_calls`、
`总 syscall 数 - 见到的次数 ∈ [0,100]`（实测 34 = 加载器 + 写日志那几十次）。
第三条是交叉核对：若 1000 次 vDSO 也进了内核，差值会跳到 1000 以上。

### 3.3 没有 gdb：崩溃点要靠自己接

链路是 `sigaltstack` → `SA_SIGINFO` 处理器 → `backtrace()` → `dladdr()` → `addr2line`：

- **必须换栈**（`SA_ONSTACK`）：SIGSEGV 如果发生在栈快满时，在原栈上跑处理器会二次崩溃。
- **故障 PC 从 `ucontext` 取**：`uc_mcontext.gregs[REG_RIP]`。`backtrace()` 的第一帧一般是
  处理器自身，真正出错那条指令的地址只有这里能拿到。
- **算偏移而不是留绝对地址**：`dladdr()` 给出模块基址，`off = addr - dli_fbase`。
  PIE 可执行文件每次加载基址都不同（本次 `0x5d4b4e396...`，上次 `0x633fdc7b0...`），
  只有偏移在文件里稳定。
- **`addr2line -f -C -e <bin> <off>`** 把偏移翻成"函数 + 文件:行"。

### 3.4 回溯里两种地址，减 1 的规则相反

| 帧的类型 | 处理 | 为什么 |
| --- | --- | --- |
| **故障 PC**（出事那条指令） | **原样查** | 它本身就是"正在执行的指令"地址 |
| **返回地址** | **必须减 1** | 返回地址指向 `call` 的**下一条**指令，可能已落进下一个函数 |

首版对故障 PC 也减了 1，结果把 `f3` 解成了 `frame_dummy|crtstuff.c:?`——教训写在 §8。

反例也一起钉死（`is_pc=1` 的那一帧用运行时绝对地址再查一次）：`func=??|loc=??:0`。
这不是"偶尔解不出"，是**方法错了就必定解不出**。

### 3.5 三组 sanitizer 的失败形态不一样

| 工具 | 失败时的退出码 | 判定信号 | 本实验覆盖 |
| --- | --- | --- | --- |
| ASan | `1` | `ERROR: AddressSanitizer: <kind>` / `ERROR: LeakSanitizer: detected memory leaks` | 越界、UAF、栈溢出、泄漏 |
| UBSan | `1`（配 `-fno-sanitize-recover=all`） | `runtime error: ...`（原话） | 移位数越界、有符号溢出、除零、空指针 |
| TSan | `66`（`halt_on_error` 的默认退出码） | `WARNING: ThreadSanitizer: data race` | 竞争 / 原子 / 互斥三组对照 |

关键设计：`sani_case()` 抽取的是**工具自己打印的类别名**，不是我们注入时用的用例名
（`asan-leak` 期望 `kind=leak` 而不是 `kind=asan-leak`）。否则判定器只是在自证。

### 3.6 编译器会把你放进去的缺陷删掉

这一节是本阶段最有价值的实测结论，两次都真实发生过：

1. **死存储**：`char *p = malloc(8); p[8] = 42;` 之后没人读——`-O1` 判定这次写无效，整句删掉，
   ASan 看到的是一个没有越界的程序，`rc=0`。改用 `volatile char *vp = p; vp[8] = 42;` 后
   `heap-buffer-overflow WRITE of size 1` 如约出现。
2. **整对消除**：`void *p = malloc(64); (void)p;` 分配后既不读也不 `free`——`-O1` 把这一对一起消掉，
   `objdump` 显示 `leak_alloc` 只剩一条 `ret`，加 `-fno-builtin-malloc` 也无效。
   最终写法是 `static void *(*volatile g_alloc)(size_t) = malloc;`（让编译器失去 malloc 的内建知识）
   + 短暂存进全局再清空（残留指针留在栈上同样会让 LSan 认为"还活着"），两步缺一不可。

**结论**：`工具没报` ≠ `缺陷不存在`。它可能只是被优化掉了。

### 3.7 "没有 perf"是两层问题，不是一层

`perf_event_open` 的三次调用给出分层证据：

- `self`（`pid=0`、`exclude_kernel=1`）：`fd=3`——**自身用户态计数是可用的**；
- `systemwide`（`pid=-1,cpu=0`）：`-1 / EACCES(13)`；
- `other pid`（`pid=1`）：`-1 / EACCES(13)`。

配合 `perf_event_paranoid=2`，可以判定：**缺的是权限和二进制两层，不是能力**。
所以本阶段自己读 `cycles` / `instructions` 得到 IPC（1.25），而没有 `perf stat`。

首版手抄了一个"精简"的 `perf_event_attr`（48 字节），三次调用全部 `E2BIG(7)`：
内核按 `size` 字段判版本，抄漏字段就整条路走不通。改用 `<linux/perf_event.h>` 的真结构体。

### 3.8 没有 perf 时的基准：每个数字都要有"为什么可信"

| 手段 | 度量 | 前提与边界 |
| --- | --- | --- |
| `CLOCK_MONOTONIC` 包住 N 次调用，三轮取中位 | 单次操作耗时 | 只有相对关系有效，绝对值受 CPU 频率/WSL 调度影响 |
| `getrusage` 的 `ru_minflt` 前后差 | 缺页次数 | 必须 `MADV_NOHUGEPAGE`，"一页一次缺页"才成立 |
| `socketpair` 往返 + 阻塞读写 | 上下文切换 | 每往返至少 2 次切换；实测值含 WSL2 虚拟化开销 |
| `/proc/self/status` 的 `ctxt` 差 | 切换次数 | 需要长窗口，短窗口噪声大于信号 |

---

## 4. 接口卡片

### `ptrace(PTRACE_GET_SYSCALL_INFO)`

作用：一次取出系统调用停点的进入/返回信息。

原型：`ptrace(PTRACE_GET_SYSCALL_INFO, pid, sizeof(struct syscall_info), &info)`

参数：第三个参数是缓冲区大小，第四个是输出。

返回值和错误：返回实际写入字节数；`EPERM`（没追踪关系）、`EIO`（不是 syscall 停点）。

资源所有权：无，只读查询。

线程/进程安全：只能由追踪者对被追踪者调用。

最小示例：见 `src/mini_trace.c` 的 `INFO_OP_ENTRY/EXIT` 分支。

常见误用：结构体手抄（版本不匹配）、在非 syscall 停点上调用。

### `backtrace()` / `dladdr()`

作用：从当前调用栈抓取返回地址；把地址映射回"模块 + 基址"。

原型：`int backtrace(void **buf, int size);` / `int dladdr(const void *addr, Dl_info *info);`

参数：`buf` 需要 `-rdynamic` 才能解出可执行文件自身的符号名；`Dl_info` 给出 `dli_fbase`。

返回值和错误：`backtrace` 返回帧数；`dladdr` 失败返回 0，此时不能拿 `dli_fbase` 做减法。

资源所有权：`backtrace_symbols` 要 `free`；`dladdr` 无。

线程/进程安全：安全，但异步信号处理里只能用 `backtrace`（不能 `malloc`）。

常见误用：忘了 `-rdynamic`/`-fno-omit-frame-pointer`，拿到的全是 `??`。

### `perf_event_open()`

作用：打开一个性能计数器。

原型：`syscall(SYS_perf_event_open, &attr, pid, cpu, group_fd, flags)`

参数：`attr.size` **必须**等于结构体真实大小；`pid=0` 表示自身，`cpu=-1` 表示跟随进程。

返回值和错误：成功返回 fd；`EACCES`（paranoid 限制）、`E2BIG`（`size` 不对）、`ENOENT`（事件不支持）。

资源所有权：调用者 `close`。

线程/进程安全：fd 属于线程（`pid=0` 时）。

常见误用：`pid=-1` 想数"自己"（那是 systemwide，需要权限）。

### 退出码速查

| 退出码 | 含义 |
| --- | --- |
| ASan `1` | 报告了错误（含 LSan 泄漏） |
| UBSan `1` | 报告了 UB（仅在 `-fno-sanitize-recover=all` 下） |
| TSan `66` | 报告了竞争并停机 |

---

## 5. 最小可运行实验

24 个用例，按"证据类型"分五组：

| 组 | 用例 | 要证明什么 |
| --- | --- | --- |
| `trace` | `fdok` / `fdleak` / `vdso` | fd 收支可被判出（`net_fd` 0 vs 2）；vDSO 调用不可见 |
| `crash` | `null` / `deep` / `ok` | SIGSEGV 能被接住、回溯能翻成源码行、64 层深栈不丢帧 |
| `asan` | `ok` / `heap-overflow` / `uaf` / `stack-overflow` / `leak` | 四类内存缺陷各自被点名，干净用例不误报 |
| `ubsan` | `ok` / `shift` / `ovf` / `div` / `null` | 四类 UB 的"原话"可被抽取 |
| `tsan` | `race` / `atomic` / `mutex` | 同一骨架换同步手段：报 / 不报 / 不报 |
| `bench` | `vdso` / `syscall` / `pagefault` / `ctxswitch` | 四组可复现的相对关系 |
| `perf` | `perf` | 权限分层与自身计数可用 |

---

## 6. 编译与运行

```bash
cd Linux/experiments/SYS_06_Debug_Perf

./run.sh                # 构建 + 24 个用例 + 判定（rc=0 才算通过）
./run.sh --judge        # 只判定已有 logs/s06.log（不重跑）
./run.sh --negtest      # 只做判定器的负向验证（需先有一份通过的日志）
./run.sh crash-null     # 只跑一个用例，不判定

make check              # 确认三组 sanitizer 运行时真的被链接（nm -D 检查符号）
```

产物：

| 路径 | 内容 |
| --- | --- |
| `logs/s06.log` | 主日志，352 行，一行一个可判定读数 |
| `logs/<case>.err` | 每个用例的**原始 stderr**（sanitizer 报表原文、崩溃报表原文） |
| `logs/build.log` | 构建输出（成功时为空，`make -s`） |

`run.sh` 的 `run_case` 先 `: > logs/<name>.err` 清空再跑，避免上一轮的旧报表被当成本轮证据。

---

## 7. 结果观察

### 7.1 fd 收支（`trace`）

| 用例 | syscalls | opens | closes | net_fd | 说明 |
| --- | --- | --- | --- | --- | --- |
| `fdok` | 40 | 5 | 5 | **0** | 配对基线 |
| `fdleak` | 38 | 5 | 3 | **2** | 开 3 关 1，泄漏 2 个 |

判据用三条：`fdok.net_fd == 0`、`fdleak.net_fd == 2`、`fdleak.opens - closes == 2`。
最后一条与绝对值无关，改 libc 版本不会误报。

### 7.2 vDSO 盲区（`trace-vdso`）

`syscalls=1034`、`clock_gettime=1000`、`execve=1`、`child_rc=0`。
差值 34 是加载器与写日志的开销——**另外 1000 次 vDSO 调用一个停点都没有**。

### 7.3 崩溃回溯（`crash`）

`crash-null`：`sig=11`、`addr=(nil)`、`frames=9`，符号链：

| 帧 | 函数 | 位置 | 查询地址 | 备注 |
| --- | --- | --- | --- | --- |
| 0 | `on_sig` | `crash.c:52` | `0x13eb` | 返回地址，减 1 |
| 2 | `f3` | `crash.c:80` | `0x1309` | **故障 PC，原样查**（`is_pc=1`） |
| 3 | `f2` | `crash.c:85` | `0x1320` | |
| 4 | `f1` | `crash.c:90` | `0x1337` | |
| 5 | `main` | `crash.c:126` | `0x1752` | |
| 8 | `_start` | `??:?` | `0x1244` | `start.S` 无行号信息 |

另外 3 帧落在 libc（`mod=other`），按模块基址过滤，不查表。

`crash-deep`：`frames=64`（数组打满），其中 **62 帧**解到 `deep|crash.c:99`。
`crash-ok`：`frames=0`、`rc=0`——没有故障时不产生任何报表。

反例（同一次运行内）：

```
S06|crash|sym|case=crash-null|i=2|func=f3|loc=crash.c:80|q=0x1309
S06|crash|abs|case=crash-null|i=2|func=??|loc=??:0
```

同一帧、同一份二进制，只用绝对地址查就是 `??`。

### 7.4 三组 sanitizer

| 用例 | 退出码 | 判定信号（工具原话） |
| --- | --- | --- |
| `asan-ok` | 0 | `kind=none` |
| `asan-heap-overflow` | 1 | `heap-buffer-overflow` |
| `asan-uaf` | 1 | `heap-use-after-free` |
| `asan-stack-overflow` | 1 | `stack-overflow` |
| `asan-leak` | 1 | `leak`（`64 byte(s) leaked in 1 allocation(s)`） |
| `ubsan-ok` | 0 | `msg=none` |
| `ubsan-shift` | 1 | `shift exponent 40 is too large for 32-bit type '` |
| `ubsan-ovf` | 1 | `signed integer overflow` |
| `ubsan-div` | 1 | `division by zero` |
| `ubsan-null` | 1 | `load of null pointer of type 'volatile int'` |
| `tsan-race` | **66** | `race=1` |
| `tsan-atomic` | 0 | `race=0`，`result=expect=400000` |
| `tsan-mutex` | 0 | `race=0`，`result=expect=400000` |

`tsan-race` 的同一次运行里 `result=400000`、`lost=0`——**20 次运行都没丢更新**。
所以"计数没丢"不能作为"没有竞争"的证据（判据只认 `race=1`，这条计数只作 INFO 记录）。

### 7.5 基准（`bench`）

| 用例 | 三轮读数 | 中位 | 判据 |
| --- | --- | --- | --- |
| `vdso`（`clock_gettime`） | 15.62 / 15.88 / 15.66 ns | **15.66** | `> 0` |
| `syscall`（`syscall(SYS_getpid)`） | 41.47 / 41.68 / 41.64 ns | **41.64** | `≤ 500`，且 `≥ 1.5 × vdso`（实测 **2.66×**） |
| `pagefault` | 首次缺页 16386 / 16384 / 16384；再次 0 / 0 / 0 | — | `min(首次) ≥ 15000`、`max(再次) ≤ 100`、比值 `< 0.01` |
| `ctxswitch` | — | `ns_per_rt=42354.8`、`ns_per_switch=21177.4` | `≥ 10 × syscall`（实测 **1017×**） |

缺页一栏的分层读数（`pages=16384`）：首次 `600~1628 ns/页`，再次 `9.6~10.8 ns/页`，
证明"`MADV_NOHUGEPAGE` + 一页一次缺页"这个前提成立；否则"页数 ≈ 缺页数"会失真。

`ctxswitch` 的 `voluntary=20456`、`nonvoluntary=0`：20000 次往返各自阻塞在 `socketpair` 上，
切换全部是主动让出，与设计一致。

**这个 21 µs/次偏高**（裸机典型个位数微秒）。成因未定位——WSL2 上存在虚拟化与跨核迁移成本，
所以本阶段只用它做**同机相对比较**（`ctxswitch` vs `syscall` 的量级差），不外推绝对性能。

### 7.6 perf 权限分层（`perf`）

```
S06|perf|paranoid=2|self_fd=3|self_errno=0|sys_fd=-1|sys_errno=13|other_fd=-1|other_errno=13
S06|perf|counters|cycles=80018326|instructions=100000081|ipc=1.25|insn_fd=4|sink=...
```

`work(20000000)` 得到约 5 条指令/迭代、IPC 1.25——这是一次真的硬件计数，不是估算。

### 7.7 判定器与负向验证

```
S06|judge|summary|checks=101|pass=101|fail=0
S06|neg|summary|cases=23|pass=23|fail=0
```

负向验证覆盖：原日志 `rc=0`、缺日志 `rc=2`、**21 条用例内变异 `rc=1` 且失败项落在预期判据上**。
每条变异都要求"锚点在日志里真实存在"，否则计 FAIL——"变异没找到"不能当"通过"。

首版把锚点写成了**字面量**（`syscalls=1034`、`median_ns=41.64`、`ns_per_rt=42354.8`），
重跑一次全量就有 3 条锚点丢失（这些读数本来就每次不同）——规则正确地报了 FAIL，
于是改成"按用例块作用域 + 正则锚点"：变异只能落在目标块内，数值用 `[\d.]+` 匹配。
同时发现符号行由 `symbolize.py` 统一追加在日志末尾，**物理上全在最后一个用例块里**，
而 `judge.py` 按行内 `case=` 归档，所以"改哪一块"与"该哪条判据红"经常不是同一个名字
（表里用注释标出）。

---

## 8. 常见错误和陷阱

1. **死存储被优化掉**：见 §3.6 第 1 条。注入缺陷后先看 `objdump` 或改 `volatile`。
2. **`malloc`/`free` 整对被消除**：见 §3.6 第 2 条。`-fno-builtin` 不够，需要 `volatile` 函数指针。
3. **故障 PC 减了 1**：会解成前一个符号（首版得到 `frame_dummy|crtstuff.c:?`）。
4. **拿绝对地址查符号表**：PIE 下必然 `??`，而且没有任何报错提示你方法错了。
5. **手抄内核结构体**：`perf_event_attr` 抄成 48 字节 → 全部 `E2BIG(7)`；
   `ptrace_syscall_info` 也不能抄错，`union` 的 `op` 决定读哪一支。
6. **`WSTOPSIG` 不判 `| 0x80`**：把所有 `SIGTRAP` 都当系统调用停点，计数直接翻倍。
7. **`-Werror=use-after-free` 拦下故意的 UAF**：GCC 15 会把 `free(p); p[0]` 当编译错误。
   本实验只在 `use_after_free()` 函数体内 `#pragma GCC diagnostic ignored`，把范围压到最小，
   并把"不打 sanitizer 也未必跑得到"这件事记在这里。
8. **`SOCK_DGRAM` 没有 EOF**（阶段 2 的旧账在本阶段复现路径上仍有效）：
   `socketpair` 测往返时，退出只能靠计数，不能靠对端 `close` 触发 `read=0`。
9. **跨运行漂移**：`addr`、`pc`、`q=` 每次都不同，判据只能比"偏移是否解出函数名"，
   不能比具体地址。

---

## 9. 故障排查方法

本阶段要形成的排查顺序（也适用于别的项目）：

1. **现象归类**：编译不过 / 跑不过 / 跑过但结果不对 / 结果对但慢。
2. **有没有工具**：`command -v` 先探，探到缺就换替代实现，别先下"做不了"的结论。
3. **要证据不要印象**：fd 泄漏看 `net_fd`，内存缺陷看工具原话，崩溃看 `函数+行号`。
4. **一次只改一个变量**：`fdleak` vs `fdok` 只差是否 `close`；`race` vs `atomic` vs `mutex`
   只差同步手段。
5. **判定器要能被证伪**：写完先跑 `--negtest`，确认每条判据都有至少一条变异能让它红。
6. **优化后的缺陷也要防**：`-O1` 能让缺陷消失，`-O2` 同样能；先确认程序真的执行了缺陷路径。

### 本实验未验证的事

- **`yama/ptrace_scope=1` 下追踪非子进程**：本实验只追踪 `fork` 出来的子进程（`TRACEME`），
  没有测 `PTRACE_ATTACH` 到无关进程（需要提权）。
- **`perf` 的 systemwide 计数**：只证明被拒（`EACCES`），没有测 root 下的行为。
- **WSL2 上 `ctxswitch` 21 µs/次偏高**：成因未定位，不作为性能结论。
- **`addr2line` 对内联帧**：本实验全是 `noinline`/`-O1`，内联帧的"同一行多帧"未覆盖。
- **ASan 的 `stack-overflow` 依赖 8 MB 栈上限**：`ulimit -s` 改小后阈值会变，本次未做参数扫描。

---

## 10. 练习题

> 默认只给题目与验收标准，不给答案。需要参考实现时再单独要。

**基础题**

1. 给 `mini_trace` 加上"按 fd 号记录打开/关闭时间戳"，输出每个 fd 的存活时长。
   验收：`fdleak` 能报出 2 个存活 fd 各自的存活时间；`fdok` 一个都没有。
2. 让 `mini_trace` 只打印 `errno` 非 0 的系统调用。
   验收：`victim` 里人为加一次 `open("/no/such")`，能且只能看到这一条的 `-ENOENT`。

**改错题**

3. 下面这段为什么在 ASan 下什么都不报？

   ```c
   void f(void) { char *p = malloc(8); p[8] = 42; }
   ```

   验收：能用 `objdump -d` 指出该语句被删，并给出两种修法（`volatile` / 改用返回值）。

4. 下面这份崩溃处理器在深栈下会二次崩溃，为什么？

   ```c
   signal(SIGSEGV, handler);   /* 处理器里调用 printf + backtrace */
   ```

   验收：指出缺 `sigaltstack` + `SA_ONSTACK`，并说明为什么 `printf` 也不能用。

**综合题**

5. 写一个"只看自己关心的系统调用"的过滤器：接受一个名字列表，其余边跑边丢，
   并统计被丢弃的条数。
   验收：对 `victim vdso` 过滤 `openat,write`，`dropped + kept == 总停点数`。

6. 用 `getrusage` 的 `ru_minflt` 测"读 16 MB 文件"的缺页数，并与 `pages` 比较。
   验收：能解释为什么两者**不相等**（页缓存命中时根本不缺页），并给出让它们接近的构造方法。

---

## 11. 验收标准

- [x] 24 个用例全部执行，`logs/s06.log` 352 行，`logs/<case>.err` 保留工具原始输出。
- [x] 判定器 101 项全通过；每条判据只比"读数之间的关系"或"工具原话"，不写死时间与地址。
- [x] 负向验证 23/23：原日志 `rc=0`、21 条变异 `rc=1` 且落在预期判据、缺日志 `rc=2`。
- [x] 缺工具的事实被记录成读数（`S06|env|tool=*`），而不是口头声明。
- [x] 每个"替代实现"都给出观测边界：vDSO 看不见、PIE 不能用绝对地址、`ctxswitch` 绝对值不可外推。
- [ ] **未做**：非子进程 `PTRACE_ATTACH`、root 下的系统级 perf、内联帧符号化、
      `ulimit -s` 参数扫描（见 §9）。

能用自己的话解释 §3.1–§3.8 中的任意四节，并提供对应读数，即视为达到阶段 6 的学习目标。

---

## 12. 延伸阅读

- `man 2 ptrace`（`PTRACE_SYSCALL`、`PTRACE_SETOPTIONS`、`PTRACE_GET_SYSCALL_INFO`）
- `man 2 perf_event_open`（`perf_event_attr` 的 `size` 版本协商、`perf_event_paranoid` 语义）
- `man 3 backtrace` / `man 3 dladdr` / `man 1 addr2line`
- `man 2 getrusage`（`ru_minflt` / `ru_majflt` 的区别）
- `man 2 madvise`（`MADV_NOHUGEPAGE` 为何是本实验的前提）
- GCC 文档：`-fsanitize=address,undefined,thread`、`-fno-sanitize-recover`、`-fno-builtin`
- `Documentation/admin-guide/perf-security.rst`（内核树，`perf_event_paranoid` 的权威说明）
- 相邻阶段：[`../SYS_05_Coroutine/`](../SYS_05_Coroutine/README.md)（`/proc/self/status` 与地址空间对比）、
  [`../SYS_03_Socket/`](../SYS_03_Socket/README.md)（`EAGAIN`/短写，本阶段基准的前置）

