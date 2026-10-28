# reactor-server —— epoll ET + EPOLLONESHOT + 线程池的最小可运行服务器

本目录是 `Linux/CLAUDE.md` §3 阶段 4（高性能服务器模型）的**综合项目**，用于把
`experiments/SYS_03_Socket/`、`experiments/SYS_04_Reactor/` 里拆开验证的机制合成一个
可构建、可测试、可解释的程序，并留下完整的判据与失败记录。

**边界声明**：这是学习项目，不是生产服务器。它没有 TLS、没有协议版本协商、没有背压信号
（只有断开）、没有持久化。吞吐有本机 loopback 的实测读数（§7），但固定亲和性、只有 5 个样本、
无 p99，**不构成基线，也不能当 QPS 用**。

---

## 1. 目录结构

```text
reactor-server/
├── Makefile              # 一条命令构建；为什么不用 CMake 见 §2
├── README.md
├── src/
│   ├── reactor.h         # 公共类型：payload_t / 线程池接口 / srv_opts
│   ├── main.c            # 参数解析与进程入口
│   ├── server.c          # epoll 事件循环、连接生命周期、完成队列、优雅退出
│   └── threadpool.c      # 有界任务队列 + 固定 worker
├── tests/
│   ├── loadgen.c         # 压测与协议校验客户端（逐字节图案比对）
│   ├── gen_reuse.py      # 槽位复用用例的专用客户端（时序由 EOF 推进，不靠 sleep）
│   ├── run.sh            # 八个用例：起服务 → 打负载 → 收尾
│   ├── judge.py          # 机器判据（105 项）
│   ├── sigint_stress.sh  # 退出路径的重复尝试（概率类验证）
│   ├── bench.sh          # 吞吐观测：固定亲和性 + 预热 + 5 次分位（不判对错，只出读数）
│   ├── san_probe.c       # sanitizer 生效性探针（故意写缺陷，目标必须失败）
│   ├── logs/             # 用例实测日志（入库，作为证据）
│   └── bench_logs/       # 吞吐原始读数与每轮 srv/cli 日志（按 TAG 分目录，入库）
├── build/                # 默认构建产物（不入库）
├── build-address/        # ASan 构建产物（不入库）
├── build-thread/         # TSan 构建产物（不入库）
└── build-undefined/      # UBSan 构建产物（不入库）
```

---

## 2. 构建

```bash
cd projects/reactor-server
make              # 零警告：-Wall -Wextra -Werror
make check        # 构建 + 跑完八个用例 + 机器判定（105 项）
make check-asan   # 同一套用例跑在 AddressSanitizer 构建上（含泄漏检测）
make check-tsan   # 同一套用例跑在 ThreadSanitizer 构建上
make check-ubsan  # 同一套用例跑在 UBSan 构建上（未定义行为）
make stress       # 退出路径的重复尝试（两种在飞形态，20 轮）
make bench        # 吞吐观测：固定 CPU 亲和性、预热 1 次 + 5 次分位
make clean
```

三种 sanitizer 构建各自落在 `build-address/`、`build-thread/`、`build-undefined/`，与默认 `build/` 隔离——
混用构建目录的后果是"改了代码却跑到旧二进制"。`check-*` 会先跑
`probe-asan` / `probe-tsan` / `probe-ubsan`：探针**故意**写一个释放后读/数据竞争/有符号溢出，
**目标必须失败**；若探针"通过"，说明工具没插桩，那么 sanitizer 的绿灯毫无意义（§7）。

UBSan 默认是"报告后继续"，那会让退出码失去意义，所以 `SAN=undefined` 时额外加
`-fno-sanitize-recover=all`，让第一次未定义行为就中止——与另外两个工具"有报告即失败"同口径。

环境（实测）：WSL2，内核 `6.6.87.2-microsoft-standard-WSL2`，`gcc 15.2.0`，`python3 3.14.4`。

**为什么用 Makefile 而不是 CMake**：本机 WSL2 内没有 `cmake`（`CLAUDE.md` §12 P1.6 已记录该限制）。
为了不让"装工具"变成跑通项目的前置条件，用 Makefile；若日后装上 cmake，Makefile 仍是最小依赖。
Makefile 里对 `CC` 用 `ifeq ($(origin CC),default)` 判定，是因为 GNU make 内建 `CC=cc`，
`CC ?= gcc` 永远不会生效（与 `SYS_01`、`BSP_03` 同一处坑）。

---

## 3. 运行

```bash
# 服务端：--port 0 表示让内核分配端口，实际端口打进 SRV|listen 日志行
./build/reactor_srv --port 8080 --threads 4 --idle-ms 10000

# 客户端：8 连接 × 20 帧 × 200 字节
./build/loadgen --port 8080 --clients 8 --frames 20 --size 200
```

`loadgen` 的选项：

| 选项 | 作用 |
| --- | --- |
| `--clients N` | 并发连接数（每连接一个线程） |
| `--frames N` | 每连接帧数 |
| `--size N` | 单帧 payload 字节数（1–8192） |
| `--hold-ms N` | 连接建立后先停顿，用于错开活跃期 |
| `--half-close` | 发完就 `shutdown(SHUT_WR)`，验证服务端的半关闭处理 |
| `--idle-after-ms N` | 发完后静默 N 毫秒，再探测服务端是否已关闭 |
| `--pipeline` | 先发完全部帧再统一读回，让服务端写缓冲积压 |
| `--read-delay-ms N` | pipeline 下延迟 N 毫秒才开始读，把写缓冲压到上限 |

**协议**：2 字节大端长度 + payload；服务端原样回显同一帧。客户端把每个字节做成
`(conn*31 + frame + i) & 0xFF` 的确定性图案，所以回显比对能抓出顺序错、截断、串包三类问题，
而不只是统计字节数。

服务端退出：`SIGINT` / `SIGTERM` 走优雅关闭，最后打印 `SRV|summary|...` 与 `SRV|exit|code=0`。

---

## 4. 测试与判据

```bash
make check   # 等价于 bash tests/run.sh
```

`run.sh` 为每个用例独立起停一个服务进程，端口由内核分配并从 `SRV|listen` 读回，
所以用例之间不会撞 `EADDRINUSE`，也可以并发重跑。所有等待都有上界：
服务 5 s 内没 ready 判失败，进程 10 s 内没退出就 `SIGKILL` 并记 `killed`。
**"判定器挂住"和"用例失败"必须是两种可区分的现象**——这是从 `SYS_06` 沿用下来的原则。

| 用例 | 参数 | 验证目标 |
| --- | --- | --- |
| `echo` | 8×20×200 | 基本回显链路、按序、计数自洽 |
| `pipeline` | 8×50×4000，`--pipeline` | 单次灌入 1.6 MB 远超 64 KB 读缓冲，验证"缓冲满≠错误" |
| `idle_timeout` | `--idle-ms 500`，4×1×200，`--idle-after-ms 1500` | 空闲扫描真的由服务端断开 |
| `half_close` | 4×10×500，`--half-close` | 对端半关闭后把剩余回显写完再关 |
| `backpressure` | 2×1000×8000，`--pipeline --read-delay-ms 400` | 写缓冲上限触发 `wbuf_full`，且客户端可见失败 |
| `dropped` | `SRV_JOB_US=1000000`、`--idle-ms 200`、2×1×200 | 在飞 job 的结果到达时连接已关闭，必须计进 `dropped` 而非静默丢弃 |
| `gen_reuse` | `SRV_JOB_US=600000`、`--idle-ms 5000`、专用客户端 `gen_reuse.py` | 迟到结果到达时**槽位已被新连接复用**，必须按世代丢弃，不能写进新连接 |
| `graceful` | `SRV_JOB_US=20000`，跑到一半 `SIGINT` | 在飞 job 不丢、无半开连接、退出码 0 |

`judge.py` 的 105 项判据分两层：

- **公共层 C1–C9**：服务起得来、跑完、退出码 0、`active=0`、`errors=0`、`accepted==closed`、客户端跑完。
- **用例层 C10+**：按用例语义追加，例如 `jobs == accepted×frames`、`rx == tx`、
  关闭原因集合、`timeouts >= accepted`、`leftover_jobs == 0`、`(fail>0) == (wbuf_full>0)`、
  `dropped == jobs`。

判的是**不变量**而不是具体数值：连接数、线程数、机器都可以换，计数关系不能变。

### `dropped` 用例为什么需要 1 秒的 job

"迟到结果"只能在**连接先关、结果后到**的窗口里产生。空闲清扫（`sweep_idle`）不检查
`in_flight`，所以只要 job 跑得比空闲门限久，这个顺序就是必然的：`idle=200 ms` 先关连接，
`job=1000 ms` 后完成，中间 700 ms 余量。客户端只发 1 帧，因此每个连接恰好产生 1 个必然被丢弃的结果。

实测（`tests/logs/dropped.srv.log`）：`jobs=2 dropped=2 tx=0 timeouts=2`，
四个 `SRV|shutdown|phase=` 全部在 `at=706` 打齐——说明这批结果是在**退出排空阶段**
被记账的，而不是在主循环里。

这个用例的价值在于让"结果悄悄消失"和"一切正常"可区分：没有它，`dropped=0` 与
"确实没有迟到结果"在日志上一模一样。

### `gen_reuse` 用例为什么用半关闭而不是保活帧

`dropped` 只覆盖"结果到达时槽位空着"。世代号真正要防的是**同一个槽位换了人**：
旧 job 的结果若按槽位投递，会打进新连接，而 `dropped` 计数仍然漂亮。

构造顺序（每一步由可观测事件推进，不靠 sleep）：

1. A 发 1 帧 400 B，然后 `shutdown(SHUT_WR)`；服务端读到 EOF 主动 close，A 读到 EOF
   ——这就是"槽位已释放"的证据，不是猜测。
2. A 的 job 仍在跑（`SRV_JOB_US=600000`），结果稍后到达。
3. B 立刻连接。唯一的空闲槽位就是 A 用过的那个，服务端给它新世代号（高 32 位相同、低 32 位 +1）。
4. B 发 1 帧 8 B 后只读。空闲门限 5000 ms 远大于 job 时长，B 不需要保活。
5. A 的结果迟到时必须按世代丢弃，B 不能收到 A 的 0x41 载荷，但自己 0x42 的回显要正常。

**为什么不用保活帧**：服务端在一个 job 在飞期间**不再读该连接的 socket**（§5.2 第 3 条），
保活数据留在内核缓冲里、`last_active` 不刷新，连接照样被空闲清扫关掉。首版就是这么失败的：
`reason=idle_timeout|rx=10`，B 只送进 1 帧。细节见 §6.6。

判据先判**前提**再判结果：C10 同槽位、C11 世代递增、C12 客户端见到 EOF；前提不成立时
用例应报"前提失败"，而不是在退化条件下悄悄通过。C13–C16 才检查 `stale_gen`、`dropped` 关系、
`stale_received=0`、`echo_b>=1`。

### 退出路径的重复尝试：`make stress`

`make check` 里的 `graceful` 只跑一次，而 §6.2 的 600 s 挂起是**概率现象**——一次通过说明不了什么。
`tests/sigint_stress.sh` 把它变成可重复的尝试：每轮独立起停服务、在指定时刻发信号、
10 s 未退出就抓 `/proc/<pid>/task/*/{wchan,syscall}` 现场再 `SIGKILL`。

| 配置 | 参数 | 目的 |
| --- | --- | --- |
| A（默认） | `job_us=20000`、信号在 200 ms 落下、12 轮 | job 大多已完成，验"信号→退出"这条快路径 |
| B（在飞） | `JOB_US=200000 SIG_DELAY_MS=50`、8 轮 | 信号落下时每条连接都有 job 在跑，逼出排空路径 |

判定三态：`ok` / `hung`（超时不退，附线程现场）/ `silent_drop`（`jobs>0` 但 `tx==0`，
见 §6.4）。日志追加到 `tests/logs/sigint_stress.log`，`make stress` 两种配置一起跑。

`hung` 的现场转储除了 `/proc/<pid>/task/*/{wchan,syscall}`，还会抓服务端已打出的
`SRV|shutdown|phase=*`（§5.6）。有了这五个阶段时间戳，"卡住"至少能定位到具体阶段；
本机没有 `gdb`，这是能拿到的最接近栈的证据。

### sanitizer：`make check-asan` / `check-tsan` / `check-ubsan`

同一套八个用例、同一份 `judge.py`，只是换成插桩构建。启用前先跑生效性探针：

| 目标 | 探针 | 探针结果 | 用例结果 |
| --- | --- | --- | --- |
| `make check-asan` | `tests/san_probe.c` 里 `volatile` 指针 free 后读 | `probe-asan: OK`（ASan 报 heap-use-after-free） | 105/105、`rc=0`，`ldd` 可见 `libasan.so.8` |
| `make check-tsan` | 两个线程无同步自增同一个 `int` | `probe-tsan: OK`（TSan 报 data race） | 105/105、`rc=0` |
| `make check-ubsan` | `volatile int x = INT_MAX` 后 `x + 1` | `probe-ubsan: OK`（UBSan 报 signed integer overflow） | 105/105、`rc=0` |

三个工具覆盖面不重叠：ASan 管内存生命周期，TSan 管跨线程可见性，UBSan 管语言层面的未定义行为
（整数溢出、越界移位、空指针解引用等）。三者都通过只说明"这三类问题没被触发"，
不代表代码正确——**没有报告的前提是有探针证明工具真的插上了桩**。

**探针踩过一次坑**：首版写 `free(p); free(p);`，`-O1` 下 gcc 直接把第二次 `free` 优化掉了
（double free 是 UB，编译器可以假定不发生），探针于是"未报错"，差点被读成"ASan 没装"。
改成 `volatile` 指针的 use-after-free 后稳定报出。**探针本身也得先证明它没被优化掉。**

TSan 抓到过一个真实缺陷（§6.5），证伪方式是把修复回退掉重新编译：

| 版本 | 同一负载（8 连接 × 20 帧 × 200 B，`JOB_US=20000`） | 退出码 |
| --- | --- | --- |
| 修复前（`g_res_queued` 无锁读） | 1 条 `WARNING: ThreadSanitizer: data race`，写方 `post_result`／读方主线程 | **66** |
| 修复后（`res_pending()`） | 0 条报告，`accepted=8 closed=8 jobs=160 dropped=0` | 0 |

### 负向验证（已做，2026-10-27）

在日志的临时副本上做 20 组变异，全部按预期判失败，原样日志判通过，目录缺失返回 2：

| 变异 | 结果 |
| --- | --- |
| M0 原样 | `rc=0` |
| M1 `SRV|exit|code=0` → `1` | `rc=1`（C4） |
| M2 删除 `SRV|summary` | `rc=1`（C3，且后续判据不崩） |
| M3 `accepted=8|closed=8` → `closed=7` | `rc=1`（C9） |
| M4 pipeline `rx==tx` 改坏 | `rc=1`（C11） |
| M5 `reason=idle_timeout` → `peer_eof` | `rc=1`（C11） |
| M6 `reason=wbuf_full` → `peer_eof` | `rc=1`（C11：有失败却无 `wbuf_full`） |
| M7 `leftover_jobs=0` → `3` | `rc=1`（C11） |
| M8 删除 `half_close.cli.log` | `rc=1`（C0） |
| M9 日志目录不存在 | `rc=2` |
| M10 `dropped` 用例 `dropped=2` → `0` | `rc=1`（C10、C11） |
| M11 `dropped` 用例 `tx=0` → `404` | `rc=1`（C12） |
| M12 `dropped` 用例 `reason=idle_timeout` → `read_err` | `rc=1`（C13） |
| M13 删除 `dropped` 用例全部 `SRV|shutdown|phase=` 行 | `rc=0`（纯诊断行，不参与判定——这条是**预期不失败**的对照） |
| M14 `gen_reuse` 的 `stale_gen=1` → `0` | `rc=1`（C13） |
| M15 `gen_reuse` 的 `dropped=1` → `0` | `rc=1`（C14） |
| M16 把 `gen_reuse` 第二条 `SRV|accept` 的 id 改成另一槽位 | `rc=1`（C10，前提不成立） |
| M17 把 `gen_reuse` 第二条 `SRV|accept` 的世代号改成不递增 | `rc=1`（C11，前提不成立） |
| M18 `gen_reuse` 客户端 `stale_received=0` → `1` | `rc=1`（C15） |
| M19 `gen_reuse` 客户端 `echo_b=1` → `0` | `rc=1`（C16） |
| M20 `gen_reuse` 同时改坏 `stale_gen=0` 与 `dropped=0` | `rc=1`（C13 + C14） |

另做了一次**源码变异**（不是日志变异）：把 `conn_by_id()` 的 `c->id != id` 判断删掉后重编译跑
`gen_reuse`，得 `stale_received=1`、`stale_gen=0`，C13 + C15 失败。日志变异只能证明"判据会响"，
源码变异才能证明"**这条分支被执行时判据真的抓得住真实故障**"——两者都做才算完整。

---

## 5. 架构与关键取舍

### 5.1 线程模型：单线程 I/O + 线程池计算

主线程独占**全部** socket I/O；worker 只做业务计算，结果经"完成队列 + `eventfd`"回投。
这样连接结构体只被主线程访问，没有锁、没有数据竞争。代价是 payload 要拷两次
（进 job 一次、回结果一次），换来的是可推理的生命周期。

**"没有数据竞争"这句话的适用范围要说准**：它只覆盖连接结构体。完成队列的计数
`g_res_queued` 是跨线程共享的，所有访问必须走锁或访问器（`res_pending()`）——
曾经有两处直接读全局变量，是 TSan 抓出来的真实竞争，见 §6.5。

`threadpool.c` 用有界队列（1024）+ 两个条件变量；队列满时 `tp_submit` 返回 -1，
服务端按背压处理（断连），不无限排队。

### 5.2 ET 的三条硬约束

1. `accept4` 与 `read` 必须循环到 `EAGAIN`，否则边沿会丢。
2. `EPOLLONESHOT` 下每次事件处理后必须显式重挂（`EPOLL_CTL_MOD`），否则连接永久沉默。
3. 在飞 job 期间**不**重挂 `EPOLLIN`——否则主线程会和 worker 同时处理同一条连接。

三者都集中在 `conn_service()`：读 → 解析 → 刷写 → 重挂。

### 5.3 读缓冲满不是错误

ET 下"缓冲满"只是"先消费再读"：边沿已经被消费掉，不主动再读就再也收不到通知。
`conn_service()` 用一个有界循环处理这件事——只要本轮有进展（未解析字节数下降）就继续读，
无进展就退出等下一次事件。**把满缓冲当错误，会让"管道化灌入 > 64 KB"的正常连接被误杀**
（这是本项目的第一个实测缺陷，见 §6）。

### 5.4 背压策略：宁可断开，不可静默丢

写缓冲（64 KB）放不下新回显时直接断开，`reason=wbuf_full`。
理由是"宁可让对端知道被拒，也不要让单连接吃掉全部内存"。
判据 C11 强制 `(客户端失败数>0) == (wbuf_full 次数>0)`——不允许出现"服务端丢了数据但客户端以为成功"。

### 5.5 连接世代号

连接 id 编码为 `(slot << 32) | generation`。worker 的结果按 id 回投，槽位被复用时世代不匹配，
迟到结果不会投给新连接，只计入 `dropped`；`dropped` 里再分离出 `stale_gen` 子集，
专指"槽位已被另一代连接占用"这一种。

**为什么必须分开计数**：`conn_by_id()` 返回 NULL 有两个成因——槽位空着（连接已关闭）
和槽位换了人（世代不匹配）。两者在 `dropped` 上完全一样，但**只有后者出错时，
旧载荷会被写进新连接**，也就是数据串包。只判 `dropped >= 1` 无法断言这条分支被执行过。

**已验证**（`gen_reuse` 用例，见 §4）：
- 正向：同一次运行里 `SRV|accept|id=1` → `close|id=1|reason=peer_eof` → `SRV|accept|id=2`
  （同槽位、世代 +1）→ `SRV|stale|id=1` → `summary … dropped=1 stale_gen=1`；
  客户端 `stale_received=0`（没收到 A 的 400 B 载荷）、`echo_b=1`（自己的回显正常）。
- 反向（源码变异）：把 `conn_by_id()` 的 `c->id != id` 判断删掉后重编译，同一用例得
  `stale_received=1`、`stale_gen=0`，判据 C13 + C15 失败。**这就是缺了世代号时的真实后果**，
  而不是"计数差 1"。

### 5.6 优雅退出顺序

`DEL listen` → `close(listen)` → **置 `g_draining`（不再提交新 job）** → 等在飞 job 排空
（`tp_pending()==0` 且完成队列为空，最多 2000 ms）→ flush 并关闭所有连接 → `tp_shutdown(1)`
→ 再 drain 一次（把"连接已关闭后到达"的结果计进 `dropped`）→ 打印 summary。

两个条件缺一不可：只等 `tp_pending()` 会漏掉"已完成但未被取走"的结果（§6.4）；
不断掉新 job 的产生，`tp_pending()` 永远回不到 0（管道化输入会一直续命）。

退出路径的五个阶段各打一条带耗时的 `SRV|shutdown|phase=<name>|at=<ms>`：
`begin` → `drained` → `conns_closed` → `tp_joined` → `final_drain`。
起因是 §8 第 9 条那个 600 s 挂起——本机没有 `gdb`，无法在挂起瞬间取栈，
这五个时间戳至少能把"卡在哪一步"变成可判定的事实。**它只提高可诊断性，不修复任何已知根因。**

---

## 6. 实测缺陷与修复（2026-10-27）

五个都是在写/跑用例、接工具的过程中被实测打出来的，不是推演出来的。
第 4 条是补完 `make stress` 之后才暴露的，第 5 条是接上 TSan 之后才暴露的。
§6.6 不算缺陷，是写 `gen_reuse` 时被实测撞出来的一条**设计事实**——它决定了用例怎么写。

### 6.1 读缓冲满被误判为 `read_err`

- **现象**：`16 clients × 100 frames × 8000 B --pipeline`（共 12.8 MB），16 条连接全部
  `reason=read_err`，服务端 `rx=65536`（正好等于 `RBUF_CAP`），客户端 `fail=16`。
- **根因**：旧 `conn_read()` 在 `rlen >= RBUF_CAP` 时返回 -1，调用方一律 `conn_close("read_err")`。
  而 ET 边沿已经被消费，正确做法是"先让 `conn_pump` 消费，再继续读"。
- **修复**：`conn_read()` 改为把"满"作为输出参数返回；新增 `conn_service()` 做
  "读 → 解析 → 刷写"的有界循环，有进展就续读，无进展就等下一次事件。
- **回归判据**：`pipeline` 用例（8×50×4000，单次灌入 1.6 MB，26 倍于读缓冲）。
  修复后同一场景 `sent=12800000 recv=12800000 fail=0 mismatch=0 jobs=1600`，全部 `peer_eof`。

### 6.2 信号屏蔽晚于线程创建

- **现象**：一次手动测试（16 连接背压场景，旧版 `conn_read`）中，`kill -INT` 后进程 600 s 未退出。
  这个现象与 §6.1 的读缓冲缺陷在排查中被一起消除，**未能单独定位根因**，如实记录。
- **排查中确认的独立缺陷**：`tp_start()` 在 `pthread_sigmask(SIG_BLOCK)` **之前**调用。
  按 POSIX，worker 继承创建时刻的信号掩码，而进程定向信号可投递给任一未屏蔽线程；
  内核若把它投给 worker，默认动作会终止进程，`signalfd` 那条优雅退出路径就被绕过了。
  这一条是**源码层面的确定事实**（掩码继承语义），不是由上面的挂起现象推断出来的。
- **修复**：把信号屏蔽与 `signal(SIGPIPE, SIG_IGN)` 移到 `tp_start()` 之前。
  修复后 `graceful` 用例连跑 3 次均稳定出现 `SRV|signal|signo=2`、`SRV|exit|code=0`、
  `leftover_jobs=0`；`SIGTERM` 同样有效。

### 6.3 `SIGPIPE` 未忽略

- **根因**：对端已关闭时 `write` 会先触发 `SIGPIPE`（默认终止进程），`conn_flush()` 里的
  `EPIPE` 分支根本执行不到。
- **修复**：`signal(SIGPIPE, SIG_IGN)`，让 `write` 返回 `EPIPE`，走 `-1` 硬错误分支。

### 6.4 退出时在飞 job 的结果被静默丢弃

- **现象**：`make stress` 的第二种配置（`JOB_US=200000 SIG_DELAY_MS=50`，4 条管道化连接）
  8/8 轮都是 `exit_rc=0`、`leftover_jobs=0`、`dropped=0`——**看起来完全正常**，
  但 `jobs=4`、`tx=0`：4 个已经跑完的 job，回显一个字节都没写回。
- **根因**：排空循环只判 `tp_pending()`。worker 的顺序是"先 `node->fn()` 把结果放进完成队列、
  再 `g_running--`"，于是 `tp_pending()` 归零时，结果可能还躺在 `g_res_queued` 里；
  循环在那一轮退出，结果随连接关闭被丢掉。计数上也看不出来——`dropped` 的定义是
  "结果到达时连接已关闭"，而此刻连接还没关。
- **第一次修法不完整**：把 `g_res_queued > 0` 加进循环条件后，管道化输入会不断产生新 job
  （每 200 ms 一批、完成即提交下一批），排空条件永不满足 → 2000 ms 门限到点，
  读数变成 `leftover_jobs=4 dropped=4 elapsed≈2183 ms`。
  **"等排空"必须先停"产生新工作"**，否则排空是发散的。
- **修复**：新增 `g_draining`；置位后 `conn_service()` 不再 `conn_pump()`，只 `conn_flush()`；
  循环条件改为 `(tp_pending() > 0 || g_res_queued > 0)`；循环体后再 drain 一次
  （补上最后一次 `nanosleep` 期间入队的结果）；`tp_shutdown(1)` 之后再 drain 一次，
  把"连接关闭后到达"的结果计进 `dropped` 而不是静默丢弃。
- **回归判据**：`tests/sigint_stress.sh` 的 `silent_drop`——`jobs > 0 && tx == 0` 判失败。
  旧二进制 3/3 判 `silent_drop`，修复后 8/8 为 `ok`（`tx=808`、`dropped=0`、
  `leftover_jobs=0`、`elapsed 156–172 ms`）。这条判据的价值在于：**它要求"行为真的发生"**，
  只看退出码或 `leftover_jobs` 会让这个缺陷一直隐身。

### 6.5 完成队列计数的无锁读（TSan 抓到的数据竞争）

- **现象**：接上 TSan 后，8 连接 × 20 帧的常规负载下服务端以 66 退出，
  报告 `WARNING: ThreadSanitizer: data race`：worker 在 `post_result()` 里写 `g_res_queued`
  （持锁），主线程在事件循环尾部与退出排空循环里**不持锁**读同一个 `int`。
- **根因**：`g_res_queued` 的写入都在 `g_res_lock` 内，但两处"要不要排空"的判断
  直接读了全局变量。编译器的优化与 CPU 的重排序都可能让主线程读到旧值。
  实际后果不是崩溃，而是**偶发地跳过一轮排空**——结果留到下一轮，或在退出时被算作遗留。
  这类缺陷光靠读代码看不出来，靠"连跑 5 次全绿"更看不出来。
- **修复**：新增 `res_pending()`（持锁读），两处判断统一走它。
- **证伪**：把修复回退掉重新编译，同一负载下 TSan 报 1 条竞争、进程 `rc=66`；
  修复后 0 条报告、`rc=0`。见 §4「sanitizer」的 A/B 表。

教训与 §6.4 是同一条：**"设计上只有一个线程碰这块数据"是需要被验证的断言，不是注释写完就成立的**。

### 6.6 在飞 job 期间不读 socket ⇒ 保活帧无效

- **发现过程**：`gen_reuse` 首版让 B 连接发保活帧维持活性（每 100 ms 一帧），
  `--idle-ms 200`。结果 `judge.py` 报 105/105 里失败 2 条，服务端日志是
  `SRV|close|id=2|reason=idle_timeout|rx=10`——B 只送进了 1 帧，其余保活帧
  根本没被读走就被空闲清扫关掉了。
- **根因**：`conn_service()` 在 `conn_pump()` 提交 job 后直接 `return`（§5.2 第 3 条：
  在飞期间不重挂 `EPOLLIN`）。所以在飞那段时间里，对端发来的数据留在**内核接收缓冲**，
  服务端既不计入 `rx`，也不刷新 `last_active`；`sweep_idle()` 只认 `last_active`，
  于是"对端一直在发数据"和"对端完全静默"对空闲清扫来说没有区别。
- **这是设计取舍，不是 bug**：不重挂 `EPOLLIN` 是为了避免主线程和 worker 同时处理同一条连接
  （§5.2 第 3 条）。代价就是上面这条——在飞期间的活性无法从对端维持。
- **对用例的影响**：`gen_reuse` 必须把空闲门限设得远大于 job 时长（5000 ms vs 600 ms），
  并用 `shutdown(SHUT_WR)` + EOF 来证明槽位已释放，而不是靠保活帧。改动见 §4 的用例表。
- **对使用者的影响**（已知限制，见 §8）：若 `--idle-ms` 小于最慢 job 的耗时，
  正常连接也会被判空闲——门限必须 ≥ 最慢 job 时长，否则服务端会主动断开还在等结果的连接。
  本用例的 `dropped` 之所以稳定，正是利用了这一点（§4）。

---

## 7. 实测结果（2026-10-27，WSL2 / gcc 15.2.0 / 32 核）

`make check` 每次 105/105 判据通过、退出码 0。下表是 2026-10-27 最后一次运行的关键读数：

| 用例 | 服务端 summary | 客户端 done | 判据 |
| --- | --- | --- | --- |
| `echo` | `accepted=8 closed=8 rx=32320 tx=32320 jobs=160 errors=0` | `fail=0 mismatch=0 ms=12` | C1–C9 + 用例层全过 |
| `pipeline` | `accepted=8 closed=8 rx=1600800 tx=1600800 jobs=400` | `fail=0 mismatch=0 ms=40` | C1–C9 + 用例层全过 |
| `half_close` | `accepted=4 closed=4 rx=20080 tx=20080 jobs=40` | `fail=0 mismatch=0 ms=5` | C1–C9 + 用例层全过 |
| `idle_timeout` | `accepted=4 closed=4 timeouts=4 rx=808 tx=808` | `idle_closed=4 ms=1503` | 关闭原因全为 `idle_timeout` |
| `backpressure` | `accepted=2 closed=2 rx=5732472 tx=5519614 jobs=707` | `sent=16000000 recv=248000 fail=2 ms=437` | 两条连接均 `wbuf_full` |
| `dropped` | `accepted=2 closed=2 jobs=2 dropped=2 stale_gen=0 tx=0 timeouts=2` | `sent=400 recv=0 fail=2 ms=203` | `dropped == jobs`，`stale_gen == 0`（槽位是空的，不是换人） |
| `gen_reuse` | `accepted=2 closed=2 jobs=2 dropped=1 stale_gen=1 rx=412 tx=10` | `eof_a=1 stale_received=0 echo_b=1 bytes=10 ms=1203` | C10–C16 全过（前提 + 结果） |
| `graceful` | `accepted=4 closed=4 jobs=18 leftover_jobs=0` | `fail=4 ms=192` | 退出码 0，关闭原因全为 `shutdown` |

`backpressure` 的 `rx` / `tx` / `jobs` 在多次运行间会漂移（实测 `jobs` 700–707、`tx` 相差约 50 KB），
因为断连时刻取决于内核收发缓冲的填充速度；判据只判"要么全成功、要么断开且客户端可见失败"，
**不判这些绝对值**。其余用例的计数关系稳定。

`gen_reuse` 的完整服务端时序（`tests/logs/gen_reuse.srv.log`）：

```text
SRV|accept|id=1|active=1
SRV|close|id=1|reason=peer_eof|rx=402|tx=0     ← A 半关闭，服务端主动关，槽位释放
SRV|accept|id=2|active=1                       ← B 复用同一槽位（slot 相同、generation +1）
SRV|stale|id=1                                 ← A 的迟到结果到达，槽位已换人
SRV|close|id=2|reason=peer_eof|rx=10|tx=10     ← B 只收到自己的 8 B 回显
SRV|summary|...|dropped=1|stale_gen=1|leftover_jobs=0
```

四个构建目录各自 105/105、退出码 0：

| 构建 | 目标 | 结果 | 附加证据 |
| --- | --- | --- | --- |
| 默认 `build/` | `make check` | 105/105 `rc=0` | 上表读数 |
| ASan `build-address/` | `make check-asan` | 105/105 `rc=0` | `ldd` 见 `libasan.so.8`；`detect_leaks=1` 下无泄漏报告 |
| TSan `build-thread/` | `make check-tsan` | 105/105 `rc=0` | 0 条竞争报告（修复前版本同负载 `rc=66`，见 §6.5） |
| UBSan `build-undefined/` | `make check-ubsan` | 105/105 `rc=0` | 0 条未定义行为报告，`-fno-sanitize-recover=all` 下任一报告即中止 |

**上表是单次观测，不是吞吐数据**：八个用例的目的就是"跑通并判定"，它们的 `ms=` 由用例自身的等待
（`idle_timeout` 1500 ms、`gen_reuse` 1203 ms）或断连时刻主导，不能当性能读数。

吞吐另用 `make bench` 测：固定 CPU 亲和性（服务端 `0-1` = 一个物理核、客户端 `4` = 另一个物理核）、
预热 1 次 + 取 5 次分位、每次运行先过功能判据。完整数据、口径与边界见
[`benchmarks/reactor-server_2026-10-27.md`](../../benchmarks/reactor-server_2026-10-27.md) §9。关键读数：

| 形态 | 配置 | 中位 帧/s | 中位 MiB/s |
| --- | --- | --- | --- |
| `roundtrip` | 8 连接 × 2000 帧 × 256 B | 80 000 | 19.53 |
| `pipeline` | 8 连接 × 1000 帧 × 4096 B | 24 768 | 96.75 |
| `single-conn` | 1 连接 × 8000 帧 × 256 B | 8 138 | 1.99 |

两点结论必须一起说，否则上面的数字会被误读（§9.4、§9.5）：

1. **线程池在这套负载上没有吞吐收益**：`THREADS=4` 与 `THREADS=1` 的差异（200 vs 188、323 vs 315 ms）
   落在各配置的极值区间内。job 本体只是一次 `memcpy`，而每个 job 要付一次 `eventfd` 跨线程唤醒。
2. **worker 被铺到另一个物理核时反而显著变慢**：服务端给两个物理核（`0-3`）+ 4 worker 时
   roundtrip 变成 531 ms，而同一亲和性下 1 个 worker 是 197 ms。现象稳定但**机制未定位**
   （本机无 `perf`），所以只作为本环境的注意事项，不写成"多核更慢"的普遍结论。

这些数字只在声明的亲和性下成立，仍然只有 loopback、5 个样本、无 p99，**不是 QPS 结论**。

`make stress` 两种配置的实测（2026-10-27）：

| 配置 | 轮数 | 结果 | 关键读数 |
| --- | --- | --- | --- |
| A `job_us=20000` / 信号 @200 ms | 12 | 12/12 `ok` | `elapsed 5–8 ms`、`leftover_jobs=0`、`dropped=0` |
| B `job_us=200000` / 信号 @50 ms | 8 | 8/8 `ok` | `elapsed 156–172 ms`、`jobs=4`、`tx=808`、`dropped=0` |

配置 B 的 156–172 ms ≈ 信号后剩余的那部分 job 时长（200 ms 减去 50 ms 的延迟），
说明退出确实**等了在飞 job**，而不是直接放弃。

---

## 8. 已知限制（未验证项如实列出）

1. **没有跨环境对照**：只在本机 WSL2 单机跑过，未在裸机 Linux、不同内核、不同 `tcp_rmem/wmem` 下验证。
2. **`backpressure` 的确定性依赖内核缓冲参数**：本机 `tcp_rmem=4096 131072 6291456`、
   `tcp_wmem=4096 16384 4194304`。换机器后若内核缓冲更大，同参数可能全成功；
   判据因此写成"要么全成功、要么断开且客户端可见失败"，而不是"必须出现 `wbuf_full`"。
3. **退出时写缓冲里的数据不保证写完**：退出路径只 `conn_flush()` 一次、不等 `EPOLLOUT`。
   这是有意的取舍（退出要有上界），对端可能看到尾部截断。本轮 `graceful` 恰好 `rx==tx==3636`，
   但这是"数据少、一次写完"的结果，不是保证。
4. **单连接同时只有一个在飞 job**：用来保证回显顺序，代价是单连接无法利用多核；
   跨连接的并行度由连接数决定。
5. **`MAX_FRAME = 8192`、`MAX_CONNS = 1024` 是编译期常量**，没有做动态扩容。
6. **sanitizer 覆盖到 ASan / TSan / UBSan，不含 MSan 与 `valgrind`**：三者都是 gcc 自带、无需安装；
   `valgrind` 本机没有、`-fsanitize=memory` 需要 `clang`（本机没有），都没跑。
   ASan 的泄漏检测只在进程正常退出时生效——被 `SIGKILL` 的轮次不产生泄漏结论。
7. **只支持 IPv4 loopback 与明文 TCP**，没有 TLS、没有鉴权。
8. **空闲门限必须大于最慢 job 的耗时**：在飞期间服务端不读该连接的 socket（§6.6），
   对端的保活流量不会被消费，也不刷新 `last_active`。若 `--idle-ms` 小于 job 时长，
   正在等结果的正常连接会被空闲清扫主动断开。这不是 bug，是 §5.2 第 3 条取舍的必然结果，
   但使用者必须知道这条约束。`dropped` 用例正是把它当成确定性来源。
9. **`stale_gen` 只覆盖"同进程内槽位复用"**：世代号是 `uint32` 且进程重启后从 1 重新计数
   （`g_gen` 是静态变量）。跨进程复用槽位编号是无关的——结果队列随进程一起销毁。
   没有做长跑后世代号回绕（2^32 次连接）的验证。
10. **600 s 挂起仍未闭环**：`make stress` 两种配置共 20 轮未复现（A 12/12、B 8/8 均在时限内退出），
   但**未复现 ≠ 已修复**。已补的两件事只是把诊断成本降下来，不是修复：
   ① 退出路径的五个 `SRV|shutdown|phase=` 时间戳（§5.6）；
   ② `hung` 时把已打出的阶段行连同 `/proc/<pid>/task/*/{wchan,syscall}` 一起入库。
   要真正定位，仍需一台有 `gdb`/`pstack` 的环境，在挂起瞬间抓全线程栈。
11. **吞吐数字只在声明的亲和性下成立**（`benchmarks/reactor-server_2026-10-27.md` §9）：
   本机 cpu0 与 cpu1 是同一物理核的两个超线程，改成"看起来对称"的 0/1 配对会把服务端和客户端
   压到同一个物理核上。换配置、换机器、换内核缓冲参数都可能得到不同量级。
   只有 loopback、5 个样本、无 p99，**不能当 QPS 用**。
12. **线程池在这套负载上不是吞吐手段**（§7、§9.4）：job 只是一次 `memcpy`，
   每个 job 要付一次 `eventfd` 跨线程唤醒；`THREADS=4` 与 `THREADS=1` 差异不可测，
   而把 4 个 worker 铺到另一个物理核会显著变慢（机制未定位，本机无 `perf`）。
   若要用这个池子装真实业务，先测 job 的真实耗时，再决定线程数。

---

## 9. 下一步

- 换一台有 `gdb`/`pstack` 的环境跑 `make stress`，把 600 s 挂起的根因从"未复现"推到"能解释"；
  本机只能做到"五个阶段时间戳 + 线程现场"，无法取栈。
- 有 `valgrind` / `clang` 的环境补 MSan 与 Valgrind 的对照（UBSan 已接入，见 §2）。
- 吞吐已有固定亲和性 + 5 次分位的读数（§7、`benchmarks/…` §9），但还差三项：
  ① 用 `perf` 定位"worker 跨物理核变慢"的机制（本机没有 `perf`）；
  ② 扩大负载找出真正的饱和点——现在三个配置都还在"每连接 1 个在飞 job"的线性区，
  没有把服务端压到 CPU 饱和；
  ③ 与 `SYS_04_Reactor` 的三种形态在同一套 `bench.sh` 口径下做对照。
