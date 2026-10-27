# SYS_04 · Reactor 三种形态（单 Reactor / Reactor + 线程池 / 主从 Reactor）

对应 `CLAUDE.md` §3 阶段 4「高性能服务器模型」。实验用同一份 reactor 内核与同一份
分帧函数，只换"业务处理在哪个线程里执行"，把三种形态放到同一把尺子下量。

## 学习目标

- 用一份可编译、可判定、可证伪的代码说清三种 Reactor 形态的差别到底在哪一行。
- 把 LT / ET / `EPOLLONESHOT` 从"配置项"变成"能用读数验证的语义"。
- 让破坏版**必然**失效，而不是偶尔失效：没有坏掉的对照组，通过就是空话。
- 建立"一个连接只归一个线程所有"的所有权边界，并用事件通道跨线程交回结果。

## 环境与前置条件

| 项 | 值 |
| --- | --- |
| 发行版 | WSL2 Ubuntu |
| 内核 | `6.6.87.2-microsoft-standard-WSL2`（x86_64） |
| 编译器 | `gcc (Ubuntu 15.2.0-16ubuntu1) 15.2.0` |
| 语言标准 | C11（`-std=c11`），各 `.c` 内 `#define _GNU_SOURCE` |
| 编译选项 | `-O2 -g -Wall -Wextra -Werror`（警告即失败） |
| 依赖 | `-lpthread`、`eventfd`、`epoll` |
| CPU | 32 逻辑核（`nproc`），回环网络 |

前置：阶段 3（`experiments/SYS_03_Socket/`）的非阻塞 I/O、短读短写、`EAGAIN`、
`SIGPIPE`/`EPIPE` 与 `select`/`poll`/`epoll` 基本用法。本实验不再解释这些。

未安装 `gdb`/`strace`/`perf`（无 sudo），故障定位改用 `/proc` 与程序自身日志，
见「故障排查方法」。

## 核心概念

### 三种形态只差一件事

| 形态 | accept 在 | 读/写/分帧在 | 业务耗时在 | 连接归谁 |
| --- | --- | --- | --- | --- |
| `single` 单 Reactor 单线程 | 主线程 | 主线程 | **主线程** | 主线程 |
| `pool` Reactor + 线程池 | 主线程 | 主线程 | **worker 线程** | 主线程（worker 只拿数据副本） |
| `master` 主从 Reactor | 主线程 | **sub 线程** | sub 线程 | 从 accept 起就转交给 sub |

`single` 慢不是因为 `epoll` 慢，是因为 8 条连接 × 20 帧 × 2 ms 的业务耗时全被排进
同一个线程，天然串行。把这段耗时任移到 worker（`pool`）或分摊到多个 `epoll`
（`master`）才有并行度。

### 连接所有权与跨线程回包

`reactor.h` 定的约束：**一个 `conn` 只归一个线程所有**。线程池模式下 worker 拿到的是
`malloc` 出来的数据副本，不是 `conn` 的缓冲区；处理完把结果放进完成队列，再
`write(eventfd)` 把 reactor 线程唤醒，由它在自己的线程里调用 `conn_send`。
允许跨线程动的只有那个 `eventfd`，不是 `conn`。

由此带来一条必须显式处理的顺序约束：**同一连接同时只允许一个在飞的 job**（`c->busy`），
否则两个 worker 的完成顺序可能颠倒，客户端看到的回包顺序就不等于请求顺序。
金丝雀是 `loadgen` 的 `mismatch`（每条连接用不同字节填充，串包会被 `memcmp` 抓到）。

### `EPOLLONESHOT`：投递一次就摘掉

内核在投递一次事件后就把该 fd 从关注集合里摘掉，**即使事件掩码没变**。所以
"重挂"必须再发一次 `epoll_ctl(EPOLL_CTL_MOD)`；写成"掩码没变就不 MOD"（LT 下省系统调用
的写法）在 `EPOLLONESHOT` 下等于永久静默。

它解决的是"同一个 fd 被多个线程同时处理"：`pool` 形态若把就绪连接分给多个线程，
`EPOLLONESHOT` 保证一个 fd 在同一时刻只在一个线程的可读集合里。本实验的
`pool` 形态是"Reactor 单线程读 + 线程池处理业务"，不需要它；`oneshot` 用例单独验证语义。

### 监听 fd 与唤醒 fd 不能带 `EPOLLONESHOT`

监听 fd 没有"重挂"这一步：它被摘掉之后不会有人替它 MIT。首版把事件掩码函数无差别用在
所有 fd 上，结果 oneshot 用例 `accepted=4`（8 条连接只接了一半）。代码里现在分成两个
函数（`reactor.c:31` `ep_mask` 与 `reactor.c:116` `aux_mask`），注释写明原因。

### LT 与 ET 在这个规模下没有可测差异

`single` 339 ms、`et` 338 ms（见「结果观察」）。ET 的价值在"连接多、事件密度高"时省
`epoll_wait` 返回次数，本实验 8 条连接、每帧 204 字节，看不出差别。ET 在本实验里被证明的
只有**正确性**（必须读到 `EAGAIN`，破坏版只读一次就停在 8/160）。

## 系统调用/接口卡片

### `epoll_create1`

作用：创建 epoll 实例。

原型：`int epoll_create1(int flags);`

参数：`EPOLL_CLOEXEC`（`exec` 时自动关闭，避免 fd 泄漏到子进程）。

返回值：成功返回 epoll fd；失败 `-1` 并置 `errno`。

资源所有权：调用方负责 `close`。

最小示例：`int epfd = epoll_create1(EPOLL_CLOEXEC);`

常见误用：用 `epoll_create(0)` 的老写法导致 fd 泄漏给 `exec`。

### `epoll_ctl`

作用：增删改关注项。

原型：`int epoll_ctl(int epfd, int op, int fd, struct epoll_event *ev);`

参数：`op` 为 `ADD`/`MOD`/`DEL`；`ev->events` 由 `EPOLLIN`/`EPOLLOUT`/`EPOLLET`/`EPOLLONESHOT`
按位或；`ev->data` 是内核原样带回的 8 字节（本实验存 fd，再查直连表）。

返回值和错误：`0` 成功；`-1` 且 `errno=EEXIST`（重复 ADD）、`ENOENT`（对未注册 fd 做 MOD/DEL）。

注意：`DEL` 时 `ev` 可为 `NULL`（内核 2.6.9 起）；`MOD` 时 `ev` 必须给，否则 `EFAULT`。

常见误用：以为 `MOD` 会重置"内核已投递"的状态——`EPOLLONESHOT` 下 `MOD` 正是重挂手段，
`LT`/`ET` 下才是"改掩码"。

### `epoll_wait`

作用：等待事件。

原型：`int epoll_wait(int epfd, struct epoll_event *evs, int maxevents, int timeout);`

返回值：就绪 fd 个数；`0` 表示超时；`-1` 且 `errno=EINTR` 应重试。

本实验把 `timeout` 写成 50 ms：主循环还要轮询"是否该退出"，不能无限阻塞。
代价见「已知限制」——50 ms 是空转间隔，不是延迟。

### `eventfd`

作用：线程间唤醒的一次性计数器 fd。

原型：`int eventfd(unsigned int initval, int flags);`

参数：`EFD_NONBLOCK | EFD_CLOEXEC`。本实验只把它当"有东西了"的信号，具体内容走队列，
所以每次唤醒 `write` 1 个 `uint64_t`，处理前 `read` 清零。

资源所有权：调用方 `close`。写满（计数器接近 `ULLONG_MAX-1`）会 `EAGAIN`，本实验当作
"已非零、读端照样会醒"忽略，但代码里显式写了注释而不是静默 `;`。

常见误用：用 `pipe` 代替时忘记非阻塞，导致 worker 也可能阻塞在写端。

### `accept`（非阻塞）

作用：取出已完成三次握手的连接。

关键点：**监听 fd 必须非阻塞**，`accept` 循环靠 `EAGAIN` 退出。否则第二次 `accept` 会把
整个事件循环永久挂住（首版实测，见「故障排查方法」）。`accept` 返回的 fd 也继承
"不继承非阻塞状态"——Linux 下新 fd 不带 `O_NONBLOCK`，必须显式 `fcntl` 设上（`server.c:369`、
`server.c:399`）。

## 最小可运行实验

七个用例，每个 = 起一个服务端 + 跑一次 `loadgen` + 收一份日志：

| # | 用例 | 服务端参数 | 证明什么 |
| --- | --- | --- | --- |
| 1 | `single` | `--mode single --delay-us 2000` | 业务耗时压在事件循环里 → 严格串行 |
| 2 | `pool4` | `--mode pool --workers 4` | 同一耗时移到 worker → 变快，且不丢任务、不乱序 |
| 3 | `master2` | `--mode master --subs 2` | 连接真的分到两个 sub，各带一个 `epoll` |
| 4 | `et` | `--mode single --et` | ET 正确用法：读到 `EAGAIN` |
| 5 | `oneshot` | `--mode single --oneshot --delay-us 500`，客户端 `gap_ms=2` | `EPOLLONESHOT` 每次事件后必须重挂 |
| 6 | `et-broken` | `--mode single --et-broken --max-run 6000` | 破坏版：ET 只读一次 → 剩余字节再不产生事件 |
| 7 | `oneshot-broken` | `--mode single --oneshot-broken --max-run 6000`，`gap_ms=5` | 破坏版：不重挂 → 连接停在第一帧 |

客户端统一 8 连接 × 20 帧 × 200 字节 payload（帧长 204 字节），
`SO_RCVTIMEO = SO_SNDTIMEO = 2000 ms`，`TCP_NODELAY` 打开。

用例 5/7 需要 `gap_ms > 0`：让"一次事件 = 一次到达"成立，否则一帧里挤进多帧时，
"停在第一帧"就变成了"停在第一批"。

## 编译与运行

```bash
cd experiments/SYS_04_Reactor
make                                  # 零警告；-Werror 下警告即失败
./run.sh                              # 七个用例全跑 + 自动判定
./run.sh --judge                      # 只判定已有 logs/reactor.log
./run.sh single pool4                 # 只跑指定用例
python3 -I negtest.py                 # 判定器负向验证（14 次调用）
```

单独起服务端观察：

```bash
./reactor_srv --mode pool --workers 4 --delay-us 2000
# SYS04|srv|ready|port=41235|mode=pool|flags=lt|delay_us=2000|workers=4|subs=0
./loadgen 41235 8 20 200 2000 0
```

端口由内核分配（`bind` 端口 0），`ready` 行报出真实端口，`run.sh` 从日志取值传给客户端——
因此用例可并发、可重跑而不会撞 `EADDRINUSE`。

## 结果观察

环境：WSL2 / `6.6.87.2-microsoft-standard-WSL2` / gcc 15.2.0 / 32 逻辑核。
原始日志：`logs/reactor.log`；判定输出：`logs/judge.log`（`-- 26/26 通过`，
`judge_rc=0`）；负向验证：`logs/negtest.log`（`pass=14 fail=0 total=14`）。

### 客户端读数

| 用例 | `flags` | `verified/expect` | `mismatch` | `timeouts` | `total_ms` | `max_conn_ms` |
| --- | --- | --- | --- | --- | --- | --- |
| `single` | lt | 160/160 | 0 | 0 | **339** | 338 |
| `pool4` | lt | 160/160 | 0 | 0 | **85** | 84 |
| `master2` | lt | 160/160 | 0 | 0 | **169** | 169 |
| `et` | et | 160/160 | 0 | 0 | **338** | 338 |
| `oneshot` | oneshot | 160/160 | 0 | 0 | **98** | 97 |
| `et-broken` | et-broken | **0/160** | 0 | **8** | 4042 | 4042 |
| `oneshot-broken` | oneshot-broken | **8/160** | 0 | **8** | 4053 | 4053 |

### 服务端读数

| 用例 | `accepted` | `handoff` | `frames` | `iters` | `peak` | `q_drops` | `sub0/sub1` | `serve_ms` |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `single` | 8 | 0 | 160 | 44 | 8 | 0 | — | 748 |
| `pool4` | 8 | 0 | 160 | 244 | 8 | 0 | — | 492 |
| `master2` | **0** | **8** | 160 | 6 | 0 | 0 | 80/80 | 560 |
| `et` | 8 | 0 | 160 | 28 | 8 | 0 | — | 747 |
| `oneshot` | 8 | 0 | 160 | 44 | 8 | 0 | — | 507 |
| `et-broken` | 8 | 0 | **8** | 10 | 8 | 0 | — | 6042 |
| `oneshot-broken` | 8 | 0 | **8** | 6 | 8 | 0 | — | 6025 |

### 六条读数解释

1. **串行是真的**：`single` 339 ms ≈ 160 帧 × 2 ms = 320 ms。判据不判绝对值，
   只判 `total_ms ≥ expect × delay_us / 1000 / 2`（= 160 ms），防止"其实没串行却通过"。
2. **线程池把耗时挪走了**：`pool4` 85 ms = `single` 的 25%（4 个 worker，接近线性）。
   `q_drops=0` 说明队列没满——这是"变快"的合法前提，靠丢任务变快不算。
3. **主从模式的 `accepted` 恒为 0 属于正常**：主 reactor 只 `accept` 再转交，
   连接登记在 sub 上。连接数要看 `handoff=8`，`sub0_frames=80 / sub1_frames=80`
   证明两条 sub 真的各干一半。判据因此写 `handoff == clients`，不写 `accepted >= 1`
   ——后者会把正确实现判成失败（首版日志就被自己误读成"一条都没接"）。
4. **ET 与 LT 同量级**：338 vs 339 ms。本实验规模下 ET 没有可测收益，它被证明的
   只有正确性；收益前提（事件密度高）在本实验里不成立，不做外推。
5. **`iters` 不能跨模式比较**：它是 `epoll_wait` 返回 `>0` 的次数，`pool4` 的 244 里
   有约 160 次是 worker 的 `eventfd` 唤醒。`master2` 的 6 是主 reactor 自己的循环次数，
   sub 的循环不在里面。把 `iters` 当吞吐指标读会读错。
6. **`serve_ms ≈ total_ms + idle_exit_ms`**：五个正常用例的这个差值是
   391~409 ms（`--idle-exit` 默认 400）。这说明 `serve_ms` 里含固定 400 ms 空等窗口，
   **不是性能指标**；小于 400 的差值来自"服务端判定空闲的时刻早于客户端收尾"。

### 两处设计被实测推翻

**① 监听 fd 没设非阻塞 → 整个事件循环永久挂死。**
首版 `make_listen` 只做了 `bind`/`listen`，没有 `set_nonblock`。`reactor_run_once` 里
`accept` 循环写成"直到 `errno` 非 `EINTR` 就 `break`"，第二次 `accept` 在空队列上阻塞，
主循环再也不返回。现象：服务端只打出 `ready` 行，客户端 8 个连接全部 `timeouts=8`、
`verified=0`。定位手段见「故障排查方法」。修法见 `server.c:443`。

**② 把 `ep_mask()` 无差别套在监听 fd 上 → `accepted=4`。**
`EPOLLONESHOT` 对监听 fd 同样生效：内核投递一次可读后就把监听项摘掉，而监听 fd 没有
"重挂"这一步，于是只 accept 到当时排队的那 4 条，剩下 4 条永远进不来。
它看起来像客户端问题（`accepted=4 / verified=80`），实际是服务端把自己关掉了。
修法：`aux_mask()` 只带 `EPOLLET`，监听 fd 与 `eventfd` 一概用 `aux_mask`，
连接 fd 才用 `ep_mask`（`reactor.c:112` 的注释）。

### 两个破坏版"必然"失效，而不是偶尔

- `et-broken`：把每次 `recv` 上限压到 128 字节，且读一次就 `break`。一帧 204 字节
  必然读不完，剩余字节留在内核缓冲里——ET 下不再产生新边沿，连接从此静止。
  读数 `frames=8 / verified=0`：服务端只处理了每连接第一帧的一半都不到。
- `oneshot-broken`：注册 `EPOLLONESHOT` 但 `apply_events` 直接 `return 0`，永不重挂。
  配合 `gap_ms=5`，每条连接必然在第二帧之后静止。读数 `frames=8 / verified=8`：
  8 条连接各恰好回一帧，与"投递一次"的语义逐条对上。

两者 `serve_ms` 都撞上 `--max-run 6000`（6042 / 6025）：**停住的连接连 FIN 都收不到**，
服务端的空闲退出判据（`active + pending == 0` 且超过 `idle_exit`）永远不成立。
这本身是一条结论：只靠"连接空闲"做退出判据的服务器，遇到"对端半死不活"就退不出去，
线上必须叠加硬超时。

### 跨运行漂移

| 读数 | 上一次 | 本次（`logs/`） | 判据怎么写的 |
| --- | --- | --- | --- |
| `master2 total_ms` | 171 | 169 | 只判 `≤ 85% × single`，不判绝对值 |
| `pool4 total_ms` | 85 | 85 | 只判 `≤ 60% × single` |
| `single total_ms` | 339 | 339 | 只判下限 `≥ 160`（防"没串行"），不设上限 |
| `et-broken serve_ms` | 6042 | 6042 | **不判数值**，只判 `frames < expect` |

时间读数在回环上每次都会动几十毫秒，所以断言里没有一处把 `339`、`85`、`169`
当常量写死——负向验证里"改 `total_ms=85 → 300`"之所以能抓到，靠的是**倍数关系**，
不是数字相等。

### 判定器负向验证（14 次调用，`logs/negtest.log`）

判定器"通过"本身不构成证据，必须能证明它会抓错。`negtest.py` 只改日志副本、不重跑程序，
每处变异限定在指定用例块内（日志里 `verified=160` 出现 7 次，全局替换等于同时改坏多个用例，
验不出"某条断言真的在看这一行"）。

| # | 变异 | 期望 | 实际 |
| --- | --- | --- | --- |
| 1 | 原日志不变 | rc=0 | rc=0 |
| 2 | `single` 少收一帧 | rc=1 | 抓到 `single/一帧不差` |
| 3 | `single` 内容错一帧 | rc=1 | 抓到 `single/无内容错` |
| 4 | `single` 出现超时 | rc=1 | 抓到 `single/无超时` |
| 5 | `single` 服务端帧数不符 | rc=1 | 抓到 `single/服务端帧数一致` |
| 6 | `pool4` 队列丢任务 | rc=1 | 抓到 `pool4/队列未丢任务` |
| 7 | `pool4` 没有变快（85→300） | rc=1 | 抓到 `pool4/比单线程快` |
| 8 | `master2` 少一个 sub | rc=1 | 抓到 `master2/两个 sub` |
| 9 | `master2` 有连接没转交 | rc=1 | 抓到 `master2/连接全部转交` |
| 10 | `master2` 连接全压在 sub0 | rc=1 | 抓到 `master2/连接真的分到两个 sub` |
| 11 | `single` 其实没串行（339→100） | rc=1 | 抓到 **3 条**（见下） |
| 12 | `et-broken` 居然收齐了（0→160） | rc=1 | 抓到 `et-broken/客户端未收齐` |
| 13 | `oneshot-broken` 服务端没停（8→160） | rc=1 | 抓到 `oneshot-broken/连接停在第一帧` |
| 14 | 日志不存在 | rc=2 | rc=2 |

第 11 处不是单点失败，而是同时打穿 3 条断言：`single` 一旦变快，
"串行确实发生"、"pool4 比单线程快"、"master2 比单线程快"三条一起失败。
这说明三条断言共享同一个前提（`single` 是串行基线），不是三条独立证据。

补一条**判定盲区**的如实记录：`expect` 本身来自日志（`single` 的
`expect=160`），如果有人同时改掉 `expect` 与 `verified`，断言仍会通过。
负向验证没有覆盖"连 `expect` 一起改"，因为那不叫变异，叫换一份日志。

## 常见错误和陷阱

1. **监听 fd 忘记非阻塞**：`accept` 循环第二次阻塞，事件循环永久挂住，症状是"服务端不
   报错、就是不回包"。凡是"循环到 `EAGAIN`"的写法，前置条件都是非阻塞。
2. **`accept` 出的连接 fd 不做非阻塞**：Linux 新 fd 不继承 `O_NONBLOCK`，
   后续 `recv`/`send` 可能在事件循环里阻塞。
3. **`EPOLLONESHOT` 忘记重挂**：掩码没变也必须 `EPOLL_CTL_MOD`。判断依据是
   "内核投递后已摘掉"，不是"掩码是否变化"。
4. **监听 fd 带 `EPOLLONESHOT`**：投递一次就静默，连接数少掉一部分且看起来像客户端问题。
5. **跨线程直接改 `conn`**：线程池模式若在 worker 里 `conn_send`，写缓冲与 `epoll_ctl`
   都会和 reactor 线程打架。允许跨线程的只有 `eventfd`。
6. **同连接多个在飞 job**：`c->busy` 缺失会导致回包乱序，且 `mismatch` 只有在
   payload 按连接区分时才能抓到——若每连接发同样的内容，串包会被"内容一样"掩盖。
7. **用"连接空闲"当唯一退出条件**：对端半死（不回包也不关 fd）时永远退不出去。

## 故障排查方法

无 `gdb`/`strace`，用 `/proc` 与程序自身日志定位。首版挂死时的实际路径：

1. **看服务端输出了什么**：`logs/single.srv.log` 只有 `ready` 行 → 进程活着但进不到事件处理。
2. **确认进程状态与阻塞点**：
   ```bash
   cat /proc/<pid>/wchan        # → inet_csk_accept
   cat /proc/<pid>/status | grep -E 'State|Threads'
   ```
   `wchan = inet_csk_accept` 直接指向"卡在 `accept` 里"，而不是"卡在 `epoll_wait`"
   （后者会是 `ep_poll`）。这一步把搜索范围从"整个网络栈"缩到一行。
3. **看客户端读数**：`verified=0 / timeouts=8` → 连接建立了（否则是 `connect` 失败），
   但数据没回。结合第 2 步即可判断不是客户端 bug。
4. **核对"循环到 EAGAIN"的前置条件**：`accept` 循环的前置是监听 fd 非阻塞。
   补上 `set_nonblock(fd)` 后同一用例立即 `verified=160`。
5. **回归**：`./run.sh` 全跑 + `python3 -I judge.py` 26/26。

可复用的判据：**"进程活着、日志停在某一行、某个线程不推进"这类问题，先看 `wchan`
再猜代码**。`wchan` 把"哪个系统调用"直接给出来，比读代码猜快一个数量级。

## 已知限制

- **这不是吞吐/连接数压测**：8 连接、160 帧、回环网络。给的是"同一把尺子下的形态对比"，
  不是 QPS 结论，不做任何外推。
- `single` 的 339 ms 里包含 `nanosleep` 精度与调度抖动，只当量级看。
- 主循环 `reactor_run_once(..., 50)` 的 50 ms 是空转间隔（为了能轮询退出条件），
  不是延迟指标；真实实现应改用 `signalfd`/`eventfd` 唤醒。
- `conn_flush` 用 `memmove` 搬移剩余写缓冲（`reactor.c:206`）：让"缓冲里的字节 =
  未发出的字节"始终成立，代价是部分写时 O(n) 搬移。真实实现用读/写偏移量。
- 连接表是 `MAX_CONNS=64` 的定长数组 + fd 直查表，未做 fd 复用压力测试。
- **未测**：惊群、`SO_REUSEPORT`、`accept4(SOCK_NONBLOCK)`、`EPOLLEXCLUSIVE`、
  真实网络（高 RTT/丢包）、写缓冲背压下的 `buf_drops`（本实验 `buf_drops=0` 从未触发）。
- 未做内存/线程检查：环境无 `valgrind`/ASan 之外的 TSan 组合验证；
  跨线程共享量已用互斥量保护，但没有竞争检测证据。

## 练习题

默认只给题目与验收标准，参考实现需明确要求后提供。

### 基础题

1. 给 `pool` 形态加一个 `/proc` 之外的可观测输出：每秒打印 `active`/`q_drops`/`frames`
   增量（`SIGALRM` 或单独计时线程），跑 `run.sh pool4` 并从日志判断"队列是否曾经积压"。
   *验收*：能从增量曲线说明积压发生在连接建立阶段还是稳态阶段。
2. 把 `MAX_CONNS` 从 64 改成 8，用 8 个客户端跑通后再提高到 8 个客户端 + 8 帧，
   记录 `conn_alloc` 返回 `NULL` 时的服务端行为（当前是 `close` 新连接）。
   *验收*：能说清"拒绝新连接"与"关闭最老连接"两种策略各自在什么场景更合适。

### 改错题

3. 下面的「重挂」实现错在哪？给出修法并说明判据。
   ```c
   static int apply_events(struct reactor *r, struct conn *c, uint32_t want)
   {
       if (want == c->reg)
           return 0;            /* 掩码没变，省一次系统调用 */
       return ctl(r, EPOLL_CTL_MOD, c->fd, want, ep_mask(r));
   }
   ```
   *验收*：能用 `--oneshot` 跑出 `frames=8` 的读数，改成正确版本后变 160。
4. 下面的 `accept` 循环为什么可能把事件循环挂死？
   ```c
   int afd = accept(lfd, NULL, NULL);
   if (afd >= 0) reactor_add_conn(r, conn_alloc(afd));
   ```
   *验收*：指出至少两处问题（未循环到 `EAGAIN`、新 fd 未设非阻塞），并各给一个观测现象。

### 综合题

5. 把 `pool` 形态的"一连接一个在飞 job"改成"允许多个在飞 job + 回包按序合并"，
   要求 `mismatch=0` 在 8×20 帧下成立，且 `total_ms` 不劣于当前 85 ms 的 1.2 倍。
   *验收*：日志里能证明"同一连接曾同时有两个 job 在飞"且顺序未乱。
6. 给 `master` 形态加一条"sub 线程崩溃后连接如何回收"的路径（例如 sub 的
   `epoll_wait` 返回 `-1` 时把它的 `conn` 全部转交或关闭），并在 `run.sh` 里加一个
   注入用例。
   *验收*：注入后 `frames` 不出现"半截无解释"的情况，或日志里明确写出丢弃原因。

## 验收标准

- [x] `make` 零警告（`-Wall -Wextra -Werror`）。
- [x] 三种形态在同一份协议、同一份分帧函数下都可运行，读到 `frames=160`。
- [x] `python3 -I judge.py logs/reactor.log` → `26/26 通过`，退出码 0。
- [x] 两个破坏版**必然**失效（`et-broken verified=0`、`oneshot-broken frames=8`），
      不是概率失效。
- [x] 判定器负向验证 14 次调用全部符合预期（原日志 0 / 变异 1 / 缺失 2）。
- [x] 能用自己的话解释：三种形态的差别只有一行、`EPOLLONESHOT` 为什么必须重挂、
      监听 fd 为什么不能带 `EPOLLONESHOT`、主从模式 `accepted=0` 为什么正常。
- [x] 留有实测日志（`logs/reactor.log`、`logs/judge.log`、`logs/negtest.log`）与环境记录。
- [ ] 未覆盖：真网络、连接数压测、TSan 竞争检测、`buf_drops` 背压路径。

## 延伸阅读

- `man 7 epoll`（LT/ET 语义、`EPOLLONESHOT`、`EPOLLERR`/`EPOLLHUP` 总是被投递）
- `man 2 eventfd`、`man 2 accept`、`man 2 epoll_ctl`
- 阶段 3：`experiments/SYS_03_Socket/README.md`（非阻塞 I/O、短读短写、`EAGAIN`）
- 阶段 5：`experiments/SYS_05_Coroutine/`（把本实验的"业务耗时"换成挂起/恢复，
  对比线程与协程的调度开销）
- 本实验项目内文件：`reactor.h`（所有权约束与 API）、`reactor.c`（`aux_mask` 与
  `apply_events` 两处注释即两条被实测推翻的结论）、`negtest.py`（判定器自身怎么被验证）
