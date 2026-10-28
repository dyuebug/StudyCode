# 服务模型（Reactor / 线程池）与协程：速查表

> 对应 `CLAUDE.md` §3 阶段 4–5。实验：[`SYS_04_Reactor`](../experiments/SYS_04_Reactor/README.md)、
> [`SYS_05_Coroutine`](../experiments/SYS_05_Coroutine/README.md)。判定器分别 **26/26** 与 **58/58**。

## 1. 一句话定义

三种 Reactor 形态的差别只有一句"**业务处理在哪个线程里执行**"；协程的差别是"**业务处理在哪个栈上执行**"。
accept、读、分帧、写四步在三者中完全共用。

## 2. 形态选择

| 形态 | 业务执行位置 | 实测（8 连接 × 20 帧 × 200 B，业务 2 ms/帧） | 适用 |
| --- | --- | --- | --- |
| 单 Reactor 单线程 | 事件循环内 | `total_ms=339`（串行理论下限 320） | 每连接状态少、业务极轻 |
| Reactor + 线程池（4 线程） | worker 线程 | `pool4=85` = 单线程的 25%，`q_drops=0` | 业务有阻塞/CPU 消耗 |
| 主从 Reactor（2 sub） | sub reactor 线程 | `master2=169`，`handoff=8`，`sub0/sub1=80/80` | 连接多、需要按核分片 |
| ET | 同单线程 | `et=338`（与 `single` 同量级，**此规模无收益**） | 只证正确性，不外推性能 |
| `EPOLLONESHOT` + 线程池 | worker 线程 | `oneshot=98`，`delay_us=500`、`gap_ms=2` | 同一连接可能被不同线程处理 |

读数陷阱：`master2` 的主 reactor `accepted=0` 属正常（连接被转交），**连接数要看 `handoff`**；
`serve_ms - total_ms = 391~409 ms` 恰是 `--idle-exit` 的 400 ms 空等窗口，**`serve_ms` 不是性能指标**。

## 3. 四条必须守住的规则

1. **监听 fd 必须非阻塞**。漏设的表现是事件循环挂死在 `inet_csk_accept`，
   `/proc/<pid>/wchan` 一行定位；客户端侧表现为 8 次超时。
2. **监听 fd 与唤醒 fd 不能带 `EPOLLONESHOT`**。无差别套用后 `oneshot` 只 `accepted=4`、`verified=80`——
   监听 fd 没有人替它重挂。
3. **`EPOLLONESHOT` 投递一次就摘掉**，处理完必须 `epoll_ctl(MOD)` 显式重挂；漏掉 = 连接静默死亡。
4. **连接所有权要唯一**。主从转交后，old reactor 不得再读该 fd；线程池方案用"每连接一个在飞 job"
   把并发度钉在 1。

## 4. 破坏版必须"必然"失效

只有必然失效的破坏版才能证明判据有效：

| 破坏版 | 现象 |
| --- | --- |
| ET 只读 128 B 且每轮只读一次 | 服务端 8 帧 / 客户端 0 帧 |
| `oneshot` 不重挂 | 客户端 `verified=8`（8 条连接各只有第一帧） |

两者都因半死连接收不到 FIN，空闲退出判据不成立，只能撞 `--max-run 6000`（`serve_ms=6042/6025`）——
**"靠超时退出"本身就是这类 bug 的形状**。

## 5. 协程：三个不可回避的问题

### 5.1 帧从哪来、到哪去

`co_await` 会在堆上分配协程帧。实测：

- `gen sum=30 cnt=5 alive_after_iter=1 alive_after_break=0`：遍历结束帧仍在，`break` 后才会释放。
- `life done_before=0 alive_after_run=1 alive_after_destroy=0`：`final_suspend` 是
  `suspend_always` 时"跑完 ≠ 释放"，必须显式 `destroy`。
- `leak` vs `leak-broken` 的 `alive_after` 为 0 vs 100——把泄漏变成**读数**。

### 5.2 每次 `co_await` 都要有确定的触发源

实测被推翻的第一版：`--batch 1` 的让出写成 `co_await wait(EPOLLIN)`，
但剩余帧已经在自己的缓冲里、内核不再报可读 → `frames=8/160`、`timeouts=8 total_ms=2054`。
改为 `yield_here`（挂起后立刻重新入队）后 `frames=160`。**同一份代码在 `batch=0` 下完全正常**。

### 5.3 公平性指标必须固定客户端

`coro-greedy`：`frames=160 max_run=20 iters=24 serve_ms=443`；
`coro-fair(batch=1)`：`max_run=1 iters=42 serve_ms=850`。
但**同参数的 `coro-rr` 的 `max_run` 也是 1**——`max_run` 度量的是"一次 resume 里内核缓冲给了多少帧"，
不是公平性单值指标。

### 5.4 取消、超时、断连

| 路径 | 读数 |
| --- | --- |
| 取消 | `coro-cancel cancelled=2`（客户端 `eof_seen=2`） |
| RST | `coro-rst resets=4` |
| 半关闭 | `coro-halfclose eof=4` |
| 空闲超时 | `coro-idle frames=2` vs 破坏版 `eagain-as-eof frames=0` |
| 取消后继续用句柄 | `use_after_destroy=0` vs 破坏版 `=2`（靠 `--max-run` 4009 ms 退出） |

超时设计期修正：原本靠"处理完一帧后 `touch()` 顺延"，静默连接永远不设 deadline；
改为 `arm()` 里"挂起即活动"（**无失败日志**，如实标注为设计期发现）。

### 5.5 协程 vs 每连接一线程

`vm_kb` 恒 7128（协程）vs 8 线程 269404–400476（线程），而两者 `rss_kb` 同为 4352。

**线程栈是"预留但不触碰"的地址空间**：用 `RSS` 看等于没看，必须看 `VmSize`。
线程版每线程 41–72 MB 的成因**未定位**（空睡线程对照只有 8 MB/线程），因此**不外推每线程成本**。

## 6. 常见错误与陷阱

| 现象 | 根因 | 排查 |
| --- | --- | --- |
| 事件循环卡住不响应 | 监听 fd 未设 `O_NONBLOCK` | `/proc/<pid>/wchan` |
| 只 accept 到一半连接 | `EPOLLONESHOT` 套到了监听 fd | 检查 `epoll_ctl` 注册时的事件掩码 |
| 连接静默死亡 | `oneshot` 处理完没重挂 | 打点统计每个 fd 的 MOD 次数 |
| 协程丢帧 | `co_await` 等的事件永远不会来 | 逐个挂起点列出"谁保证会唤醒我" |
| 协程帧泄漏 | `final_suspend` 用 `suspend_always` 且忘了 `destroy` | 用存活计数（`alive_after`）而不是猜 |
| 用 `RSS` 比较线程/协程内存 | 预留页不占 `RSS` | 改看 `VmSize` |

## 7. 工程化落地（项目级）：`reactor-server`

实验证明"理解模型"，项目证明"能交付"。两件事的差别集中在下面七条——它们都是
`projects/reactor-server/` 里**跑出来**的结论，不是从实验直接搬过来的。

| 要点 | 规则 | 踩坑记录 |
| --- | --- | --- |
| 读缓冲满 | ET 下"满"只是"先消费再读"：本轮有进展（未解析字节数下降）就续读，无进展等下一次事件 | 首版把 `rlen >= CAP` 当错误，**12.8 MB 管道化灌入让 16 条连接全 `read_err`**、`rx=65536` 恰等于缓冲上限 |
| 信号屏蔽 | `pthread_sigmask` 必须早于**任何**线程创建，之后才 `signal(SIGPIPE, SIG_IGN)` | worker 继承创建时刻的掩码；`SIGINT` 投给未屏蔽的 worker 会按默认动作杀进程，优雅退出路径永远走不到 |
| `SIGPIPE` | 忽略它，让 `write` 返回 `EPIPE` | 不忽略时对端关闭先触发信号，`conn_flush()` 的 `EPIPE` 分支执行不到 |
| 背压 | 写缓冲满 → 断开并记 `reason=wbuf_full`，不无限排队 | 判据必须二分：`(客户端失败数>0) == (wbuf_full>0)`，否则"服务端悄悄丢"也能全绿 |
| 世代号 | 连接 id = `(slot << 32) \| generation`，迟到结果按世代丢弃；`dropped` 再拆出 `stale_gen` 子集专指"槽位换人" | `conn_by_id()` 返回 NULL 有两种成因（槽位空 / 换了人），在 `dropped` 上完全一样，但**只有后者出错时旧载荷会被写进新连接**。`gen_reuse` 用例（半关闭 → 同槽位新连接 → 旧 job 迟到）：实测 `dropped=1 stale_gen=1`、`stale_received=0`；源码变异删掉世代比较后 `stale_received=1`、C13+C15 失败 |
| 在飞期间不读 socket | 在飞 job 期间不重挂 `EPOLLIN`（避免主线程与 worker 同时处理同一连接）；代价是对端保活不被消费、`last_active` 不刷新 | 因此 `--idle-ms` 必须大于最慢 job 时长，否则正在等结果的正常连接会被空闲清扫断开。`gen_reuse` 首版靠保活帧维持 B 连接，实测 `reason=idle_timeout rx=10` 失败 |
| 退出排空 | 先停"产生新 job"，再等"已有 job 收尾"（`tp_pending()==0` **且**完成队列空），关闭后再 drain 一次 | 只看 `tp_pending()` 会漏掉已完成未取走的结果（`jobs=4` 但 `tx=0`，`dropped`/`leftover` 都是 0）；不停新 job 则管道化输入让排空发散到 2000 ms 门限 |
| 跨线程计数 | 完成队列的 `g_res_queued` 必须经持锁访问器读（`res_pending()`） | 两处"要不要排空"直接读全局变量 = 数据竞争，TSan 报 `post_result` 写 / 主线程读；实际后果是偶发跳过一轮排空，读代码和"连跑 5 次全绿"都发现不了 |

优雅退出顺序（实测 `signo=2 exit=0 leftover_jobs=0`）：
`DEL listen` → `close(listen)` → 置 `draining`（不再提交新 job）→ 等 `tp_pending()==0 && 完成队列为空`
（上限 2000 ms）→ flush 并关闭所有连接 → `tp_shutdown(1)` → 再 drain 一次（迟到结果计入 `dropped`）
→ 打印 summary。

**判据不得假绿**：`backpressure` 首跑 75 条判据全过，但 `wbuf_full=0`——内核缓冲吸收了 32 MB，
被测分支根本没执行。给客户端加 `--read-delay-ms 400` 后才稳定触发。
凡能二分的地方（触发 / 未触发）都要判一次，这与"工具没报 ≠ 缺陷不存在"是同一个坑的两种形态。

**工具也要先证明它开着**：接 ASan/TSan/UBSan 时先跑探针（故意写 use-after-free / 数据竞争 /
有符号溢出，目标必须失败）。探针首版写 `free(p); free(p);`，`-O1` 下 gcc 把第二次 `free` 优化掉，
探针"未报错"——差点被读成"ASan 没装"。探针自己也得先证明没被优化掉。
UBSan 默认是"报告后继续"，那会让退出码失去意义，需加 `-fno-sanitize-recover=all` 才与另外两个同口径。

## 8. 当前验证状态

- **实测**：三种 Reactor 形态、`EPOLLONESHOT` 重挂、两个必然失效的破坏版、协程帧生命周期、
  对称转移、取消/超时/RST/半关闭、`VmSize` vs `RSS`；项目侧 8 用例 / 105 条判据，
  默认、ASan（含泄漏检测）、TSan、UBSan 四个构建各自 105/105 `rc=0`；
  退出路径 20 轮（`make stress` 两种在飞形态）+ `dropped` 确定性用例（`jobs=2 dropped=2 tx=0`）
  + `gen_reuse` 槽位复用用例（`dropped=1 stale_gen=1`，含一次源码变异的反向验证）。
- **未验证**：真网络与连接数/吞吐扫描、`SO_REUSEPORT`/`EPOLLEXCLUSIVE` 惊群、`accept4`、
  `valgrind` / MSan / `clang`、`io_uring`、跨线程协程调度、长稳压测、世代号 `uint32` 回绕；
  `reactor-server` **无吞吐基线**，64 KB 写缓冲与 1024 队列长度**无压测依据**；
  600 s 挂起**未复现也未定位**。

## 证据来源

- [`experiments/SYS_04_Reactor/`](../experiments/SYS_04_Reactor/README.md)、
  [`experiments/SYS_05_Coroutine/`](../experiments/SYS_05_Coroutine/README.md)
- 基准：[`benchmarks/SYS_04_Reactor_2026-10-27.md`](../benchmarks/SYS_04_Reactor_2026-10-27.md)、
  [`benchmarks/SYS_05_Coroutine_2026-10-27.md`](../benchmarks/SYS_05_Coroutine_2026-10-27.md)、
  [`benchmarks/reactor-server_2026-10-27.md`](../benchmarks/reactor-server_2026-10-27.md)
- 项目：[`projects/reactor-server/README.md`](../projects/reactor-server/README.md)
  （§5 架构与取舍、§6 五个实测缺陷、§8 已知限制）
- 日志：[`第26天`](../daily_logs/2026-10-27_第26天_日志.md)、[`第27天`](../daily_logs/2026-10-27_第27天_日志.md)、
  [`第31天`](../daily_logs/2026-10-27_第31天_日志.md)
