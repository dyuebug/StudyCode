# SYS_05 · C++20 协程与单线程调度器

> 阶段 5（协程与异步 I/O）。用 C++20 无栈协程实现一个单线程 epoll 服务器，
> 并与"每连接一个线程"做同口径对照。
>
> 所有读数来自 `run.sh` 的一次完整运行（`logs/coro.log`），判定器 `judge.py`
> 58 项全通过，负向验证 `negtest.py` 22/22。

---

## 1. 学习目标

学完并能自己复现以下每一条，才算完成：

1. 说清**协程帧**是什么、存在哪、谁负责销毁，以及 `final_suspend` 的选择如何决定"跑完是否还占帧"。
2. 用 `co_await` 把"等 fd 可读"变成挂起/恢复，并解释为什么必须配合 `EPOLLONESHOT`。
3. 解释**对称转移**（`await_suspend` 返回 `coroutine_handle<>`）与"回到调度器再分发"的区别，以及它如何减少一次指针跳转。
4. 让协程内的异常经 `unhandled_exception` + `exception_ptr` 传播到 `co_await` 点。
5. 用**可读数**证明泄漏（存活帧计数），而不是靠"看起来没问题"。
6. 说明公平性如何被量化（同连接最长连续完成段），以及它的适用边界。
7. 实现取消/超时，并说明"销毁后仍被事件唤醒"为什么只能计数、不能真踩。
8. 用 `VmSize` 而不是 `RSS` 对比线程与协程的地址空间代价，并说明这个对比的局限。

---

## 2. 环境与前置条件

| 项 | 值 |
| --- | --- |
| 系统 | WSL2 Ubuntu，`Linux 6.6.87.2-microsoft-standard-WSL2` x86_64 |
| 编译器 | `g++ (Ubuntu) 15.2.0`，`-std=c++20` |
| 编译选项 | `-O2 -g -Wall -Wextra -Werror`（零警告是硬要求） |
| 依赖 | 无第三方库；`-lpthread` 只给对照用的线程版 |
| 前置 | 阶段 4（`EPOLLONESHOT` 必须每次重挂）、`/proc/self/status` 字段含义 |

不依赖 `libcoro`、`asio`、`folly` 等库：整个调度器 200 行左右，就是为了把
"协程如何与 epoll 结合"摊开看。

---

## 3. 核心概念

### 3.1 协程帧从哪来、到哪去

每次调用一个含 `co_await`/`co_yield` 的函数，编译器在堆上分配一个**帧**，
里面放 promise 对象、形参、局部变量和恢复点。帧的生命周期**不由作用域决定**：

- 有返回对象（`Generator` / `Task`）时，由返回对象的析构或显式 `destroy()` 决定；
- `final_suspend` 返回 `suspend_always` 时，函数跑完仍停在帧里（`done()==true` 但帧还在）；
- 返回 `suspend_never` 时，帧在 `final_suspend` 处自动释放 —— 此时再 `destroy()` 或 `resume()` 都是 UB。

本实验选 `suspend_always`，因为它让"跑完"和"释放"分成两件事，`alive_after_run=1`
和 `alive_after_destroy=0`（用例 `life`）才能把这条约定变成读数。

### 3.2 无栈协程不是"更轻的线程"

无栈协程没有自己的调用栈：挂起时，它的局部变量留在帧里，**控制权回到 resume 它的代码**。
所以：

- 不能在协程里做**阻塞**调用（`sleep`、阻塞 `recv`）——那会把整个线程卡住；
- 所有"等待"必须换成 `co_await` 某个可挂起对象（本实验是 `Sched::Waiter`）；
- 恢复必须由别人驱动（本实验是单线程 epoll 循环）。

### 3.3 `co_await` 的三段协议

```cpp
bool await_ready();                        // true → 不挂起，直接走 await_resume
bool/void await_suspend(coroutine_handle); // false → 不挂起，回到协程；true → 挂起
T await_resume();                          // 恢复后取结果（可重抛异常）
```

`await_suspend` 返回 `coroutine_handle<>` 时是**对称转移**：控制权直接跳到那个协程，
不返回给 `resume()` 的调用者。用例 `task` 靠这一点做到"一次 `resume` 跑完三层
嵌套 `co_await`"（`resumes=1`）：`outer → mid → leaf` 一路跳进去，`leaf` 结束时
`final_suspend` 的 `FinalAwaiter` 再跳回 `mid`，`mid` 结束后跳回 `outer`。

### 3.4 为什么必须 `EPOLLONESHOT`

协程在 `await_suspend` 里挂起前要重新 `epoll_ctl`。若用水平触发（LT，不带 ONESHOT），
内核会在**每次 `epoll_wait`** 都报同一个 fd 可读——包括协程根本没在等它的时候。
一旦某个协程"被唤醒后处理到一半又挂起"，就会与残留的可读事件竞争，把同一个
挂起点唤醒两次。`EPOLLONESHOT` 把"一次事件 = 一次唤醒"变成内核保证，代价是
**每次挂起都必须重挂，掩码没变也要重挂**（阶段 4 的结论在协程层复现）。

### 3.5 公平性怎么量

不用时间片（回环上抖动大），而用**同连接最长连续完成段** `max_run`：
把所有帧的完成顺序记成连接 id 序列，取最长的一段相同 id。`max_run=1` 表示
任意两条连续完成的帧都不来自同一连接 —— 天然轮转。

这个指标只在该用例的客户端行为下有意义，**不是通用健康度**：见 §7 中
`coro-rr`（同样 `--batch 0`，`max_run=1`）—— 因为请求-响应客户端一次只给一帧，
服务端根本没有"批量吃掉"的机会。

### 3.6 取消与超时

- 等待对象除了 fd 事件，还带一个 `deadline`；
- 到期由**调度器**在 `run_once` 里统一判定（不是定时器线程），超时就 `cancel(fd, ETIMEDOUT)`；
- 协程在 `await_resume` 拿到 `-ETIMEDOUT`，自行决定"重试/关闭/计数"。

本实验的取舍：**挂起即活动**（每次 `arm` 都顺延 deadline）。若反过来靠调用方
在处理完一帧后主动 `touch()`，那么"客户端连上但一帧都不发"的连接永远不会被设超时——
这条在 `coro-cancel` 用例设计时发现（见 §8.2）。

---

## 4. 接口卡片

### `co_await s.wait(fd, events[, high])`

作用：挂起当前协程，直到 `fd` 上出现 `events` 或收到取消。

原型：`coro::Sched::Waiter wait(int fd, uint32_t events, bool high = false)`

返回：`0` = 就绪；负 `errno`（`-ETIMEDOUT` / `-EBADF` / `-EPOLLERR` 等）。

资源所有权：**不动 fd**。fd 的关闭由持有者负责（本实验是 `on_done` 里 `close`）。

线程安全：只在调度线程内使用；本实验全程单线程。

最小示例：

```cpp
int r = co_await s->wait(fd, EPOLLIN);
if (r < 0) { st->cancelled++; break; }
```

常见误用：用阻塞 fd 调它（`arm` 会对阻塞 fd 注册，事件来了也读不完）、
在 `await_suspend` 里做 `epoll_ctl` 之外的阻塞操作。

### `co_await s.yield_here(fd)`

作用：**主动让出一轮**，下一轮立刻被重新 `resume`，不等待任何事件。

为什么需要它：数据已经在自己的用户缓冲里时，内核没有可读事件，`wait(EPOLLIN)` 会一直等下去。

所有权：句柄仍挂在槽位上，`alive_slots()` 不变；销毁仍走 `on_done`。

常见误用：用它替代 `wait` 等真实 I/O（会空转）；不带 `fd`（调度器无处挂句柄）。

### `Sched::on_done = [](int fd, coroutine_handle<> h){...}`

作用：协程跑到 `done()` 时，调度器把"销毁权"交回给持有者，避免"调度器替别人
销毁帧"和"没人销毁"两种错误都发生。

约定：`on_done` 里必须依次 `detach(fd)`（`EPOLL_CTL_DEL` + 清槽位）→ `h.destroy()` →
`close(fd)`。顺序反了就是"销毁后仍有事件"（`--cancel-free` 的现场）。

### `Task<T>` / `Generator<T>`

| 成员 | 语义 |
| --- | --- |
| `initial_suspend = suspend_always` | 惰性：创建时不执行 |
| `final_suspend = suspend_always` | 跑完不释放帧，等持有者 `destroy` |
| `yield_value` | 存值并挂起（`Generator`） |
| `return_value` / `return_void` | 存返回值 |
| `unhandled_exception` | `err = current_exception()`，`await_resume` 处重抛 |
| `detach()` | 交出所有权，返回裸 handle |

### `Sched::resume_later(fd, h)`

作用：登记句柄并排进就绪队列。首次启动（`main` 里 accept 之后）和公平性让出
（`yield_here`）走**同一条路径**，避免两套入队逻辑。

---

## 5. 最小可运行实验

18 个用例，分两组。每个用例一个独立进程（服务端额外起一个客户端）。

| 用例 | 形态 | 验证什么 |
| --- | --- | --- |
| `gen` | `coro_basics` | `co_yield` 惰性、单次遍历、提前 break 也释放帧 |
| `task` | 同上 | 对称转移：一次 `resume` 跑完 `outer→mid→leaf` |
| `except` | 同上 | 异常经 `exception_ptr` 传到 `co_await` 点 |
| `life` | 同上 | `suspend_always` 下"跑完仍占帧、destroy 才释放" |
| `resume-twice` | 同上 | `done()` 之后必须拦住，不能 `resume` |
| `leak` / `leak-broken` | 同上 | 100 个帧：正确销毁归零 vs 不销毁停在 100 |
| `sched-order` | 同上 | 就绪队列：高优先级整体在前，且每 fd 每轮至多一次 |
| `coro-greedy` | `coro_sched --batch 0` | 贪心：一次 resume 处理完缓冲 → `max_run=20` |
| `coro-fair` | `--batch 1` | 配额 1 帧 → `max_run=1` |
| `coro-rr` | 请求-响应 | 正确性基线（160 帧全回、字节一致） |
| `coro-cancel` | `--idle-ms 200` + 静默客户端 | `cancel(ETIMEDOUT)` → 关闭 → 客户端读到 EOF |
| `coro-rst` | 客户端 `SO_LINGER{1,0}` | `ECONNRESET` 被单独记录 |
| `coro-halfclose` | 客户端 `shutdown(SHUT_WR)` | `EOF` 与错误区分开 |
| `thread` | `--mode thread` | 每连接一线程，结果与协程版一致 |
| `coro-idle` | 客户端先静默 400ms 再发 | 正常版会等（基线） |
| `eagain-as-eof` | 同上 + `--eagain-as-eof` | 破坏版：EAGAIN 当 EOF → 一帧都收不到 |
| `cancel-free` | 超时 + `--cancel-free` | 破坏版：销毁不 detach → `use_after_destroy` 计数 |

---

## 6. 编译与运行

```bash
cd experiments/SYS_05_Coroutine
make                      # 三个目标，零警告
./run.sh                  # 跑全部用例 → 判定（58 项）
./run.sh --judge          # 只判定已有 logs/coro.log（不重跑）
./run.sh coro-fair        # 只跑一个用例
python3 -I negtest.py     # 判定器的负向验证（22 项）
```

输出两个文件：

- `logs/coro.log` —— 全部用例读数（判定输入）
- `logs/negtest.log` —— 负向验证结果

端口一律由内核分配（`bind(0)`），服务端启动后把端口打进 `ready` 行，客户端从该行取。
**踩过的坑**：早期用 `make ... && ./srv &` 一次性起服务，`&` 作用于整条链，客户端
在新服务端写好 `ready` 之前就从**上一轮**的日志里拿到了旧端口 → `send_fail=8
verified=0 total_ms=0`。现在 `run.sh` 每个用例先 `: > logs/$name.srv.log` 再启动，
并轮询该文件出现 `ready` 才继续。

---

## 7. 结果观察

环境同上；`--delay-us 0`（不注入业务耗时）；客户端 8 连接 × 20 帧 × 200 B。

### 7.1 基础用例

```
gen          sum=30|cnt=5|first=0|alive_after_iter=1|alive_after_break=0|created=2
task         value=42|resumes=1|done=1|alive_before_destroy=1|alive_after=0
except       caught=boom|outer_value=1|alive_after=0
life         alive_before=0|alive_created=1|done_before=0|alive_after_run=1
             |done_after=1|value=5|alive_after_destroy=0
resume-twice would_resume_after_done=1|blocked=1|actual_resume_after_done=0|alive_after=0
leak         frames=100|do_destroy=1|alive_before_cleanup=100|alive_after=0|alive_final=0
leak-broken  frames=100|do_destroy=0|alive_before_cleanup=100|alive_after=100|alive_final=0
sched-order  resumed=5|order=01234|hi_first=1|lo_after=1|iters=2|alive=0
```

六条值得解释的读数：

1. `gen.alive_after_iter=1` —— 遍历结束后帧还在（停在 `final_suspend`）。若把
   `final_suspend` 改成 `suspend_never`，这个数会变 0，但 `value()` 就读不到最后一个值了。
2. `task.resumes=1` —— 三层嵌套只调用了一次 `resume`。这不是"协程跑得快"，
   而是对称转移让控制权没回到调用者。
3. `life.done_before=0|alive_after_run=1` —— 惰性启动：帧已建好（`alive_created=1`），
   但函数体一个字都没执行。
4. `leak-broken.alive_after=100` —— 泄漏在这里是可读的数字，不是"内存一直在涨"。
5. `sched-order.order=01234|hi_first=1` —— 两个高优先级（0、1）整体排在三个普通
   （2、3、4）之前，且 `iters=2` 说明只用两轮就跑完 5 个等待者。
6. `sched-order.alive=0` —— 就绪队列清空后帧全销毁；若 `on_done` 里漏了 `destroy`，
   这个数就停在 5。

### 7.2 服务端用例（`S05|srv|done` / `S05|gen|done`）

| 用例 | frames | max_run | iters | serve_ms | 关键计数 |
| --- | --- | --- | --- | --- | --- |
| `coro-greedy` | 160 | **20** | 24 | 443 | `eof=8` `/` `alive_frames=0` |
| `coro-fair` | 160 | **1** | 42 | 850 | `eof=8` |
| `coro-rr` | 160 | 1 | 46 | 467 | `verified=160` `mismatch=0` |
| `coro-cancel` | 0 | 0 | 32 | 643 | `cancelled=2`；客户端 `eof_seen=2` |
| `coro-rst` | 20 | 1 | 29 | 444 | `resets=4` |
| `coro-halfclose` | 20 | 1 | 29 | 463 | `eof=4`；客户端 `eof_seen=4` |
| `thread` | 160 | 18 | 0 | 443 | `threads=8` |
| `coro-idle` | 2 | 1 | 42 | 811 | 客户端 `verified=2` |
| `eagain-as-eof` | **0** | 0 | 23 | 462 | 客户端 `verified=0` `timeouts=2` |
| `cancel-free` | 0 | 0 | 200 | 4009 | `cancelled=2` `use_after_destroy=2` |

四条主要结论（都只在本环境、本客户端行为下成立）：

1. **`max_run` 由"一次 resume 能看到多少帧"决定，不由"公平性好不好"决定。**
   `coro-greedy` 与 `coro-rr` 用的是完全相同的服务端参数（`--batch 0`），
   `max_run` 分别是 20 和 1，差别全在客户端是否流水线发送。
2. **配额 1 帧的代价是可测的**：`iters` 24→42、`serve_ms` 443→850（同机、同负载，
   两次运行均在 ±5% 内复现）。让出不是免费的。
3. **取消/超时/半关闭/复位是四条不同路径**，`cancelled` / `eof` / `resets` / `errs`
   分开计数才能看出是哪一条被触发：`coro-cancel` 的 `eof=0` 说明它是被服务端关掉的，
   而不是客户端正常结束。
4. **破坏版只改一个开关就改变行为**：`coro-idle`（`frames=2`）与 `eagain-as-eof`
   （`frames=0`）客户端完全一样，服务端只差 `--eagain-as-eof`。

### 7.3 线程 vs 协程的地址空间

| 形态 | `threads` | `vm0_kb`（启动时） | `vm_kb`（收尾前） | `rss_kb` |
| --- | --- | --- | --- | --- |
| `--mode coro` | 0 | 7228 | 7228 | 4352 |
| `--mode thread` | 8 | 7228 | 269404 | 4352 |

- 协程版的 `VmSize` **一个字节都没涨**（帧是 `malloc` 出来的小对象，远小于一页）。
- 线程版 8 个线程涨了 262 MB，而 `RSS` 完全一样 —— 说明这些是**预留但没触碰**的
  地址空间，`RSS` 看不出这个差别，必须看 `VmSize`。
- **未闭环**：同一份代码三次数到 269404 / 334940 / 400476 kB，跨运行差异很大；
  用 8 个只 `sleep()` 的线程做对照只有 8 MB/线程且线性（见
  `benchmarks/SYS_05_Coroutine_2026-10-27.md` §4）。**成因未定位**，所以这里只写
  "线程数越多地址空间越大、协程版恒定"，不写"每线程 X MB"这个结论。

---

## 8. 常见错误和陷阱

### 8.1 让出时等 epoll（实测被推翻的第一版）

第一版 `--batch N` 的实现是"处理完 N 帧就 `co_await s.wait(fd, EPOLLIN)`"。`batch=0`
的贪心版正常（160 帧），`batch=1` 直接坏掉：

```
S05|gen|done|mode=pipeline|verified=8|expect=160|timeouts=8|total_ms=2054
S05|srv|done|batch=1|frames=8|max_run=1|serve_ms=2485
```

原因：客户端把 20 帧一次性发完，服务端第一次 `recv` 就把它们全读进用户缓冲。
此时**内核没有可读数据，`epoll_wait` 不会再报 EPOLLIN**，协程就一直挂着，直到
客户端 2 秒超时。修法是"让出"必须自己把句柄重新排进就绪队列（`yield_here`），
而不是等一个不会来的事件。

### 8.2 靠调用方 `touch()` 刷新超时（设计期发现，无失败日志）

最初超时是"处理完一帧就 `touch(fd)` 顺延"。写 `coro-cancel` 时发现：客户端只连不发，
协程从没处理过任何帧，`touch` 一次都不会被调用 → **deadline 永远不设 → 永不超时**。
改成在 `arm()` 里顺延（挂起即活动），并把 `touch`/`set_deadline` 删掉，
不保留两条刷新路径。

诚实说明：这一条是读代码时发现的，**没有保留"不生效"的实测日志**，
不能算实测证据，只能算设计推演。

### 8.3 事件与超时在同一轮撞车会 resume 两次

`run_once` 先处理 epoll 事件、再扫超时。若同一 fd 两者都命中，会往就绪队列
push 两次，然后 `resume_one` 把它恢复两次 —— 第二次 `await_resume` 读到的是
过期的 `result`。修法是槽位加 `queued` 标志：本轮已排队就不再排，以先到者为准。

### 8.4 `done()` 之后 `resume()` 是 UB，不能当证据

`resume-twice` 用例不真去踩：它记录 `blocked=1`（正确驱动用 `done()` 拦住了）
和 `actual_resume_after_done=0`（真的没踩）。同理 `cancel-free` 破坏版也不真解引用
野指针：它在 `resume_one` 里看到 `destroyed` 就只 `++use_after_destroy_` 并跳过。

**为什么这样做**：UB 的结果不可复现，用它当"证据"等于没有证据。

### 8.5 `EPOLLONESHOT` 下掩码不变也要重挂

`--batch` 让出后再 `wait(fd, EPOLLIN)`，掩码与上次完全相同。带 ONESHOT 时
这次 `epoll_ctl(ADD/MOD)` 仍然必须做；漏掉就是"只被唤醒一次"——`coro-fair`
会退化成 `frames=8`。`arm()` 里的 EEXIST/ENOENT 互换重试是给"槽位与内核状态
短暂不一致"兜底，不是省掉重挂的理由。

### 8.6 判据本身会被自己写坏

`negtest.py` 第一版 22 项里 9 项报 `mutate-not-found`：`mutate()` 没 `strip()` 行尾，
用例名被读成 `"gen\n"`，所有 `basics` 变异都匹配不到。若是"变异没找到就跳过"，
这 9 条断言会**假通过**。所以 `negtest` 明确把"没找到变异"计为 FAIL。

---

## 9. 故障排查方法

| 现象 | 先看什么 | 常见原因 |
| --- | --- | --- |
| 客户端 `send_fail=N`、`total_ms≈0` | `S05|srv|ready` 行的 port | 连到了上一轮的端口（日志没清空） |
| 客户端 `timeouts>0`、`frames` 偏少 | `srv.frames` 与 `gen.verified` 的差 | 让出时等 epoll（§8.1） |
| `alive_frames>0` | `on_done` 里 `detach` 与 `destroy` 的顺序 | 少 `detach` 或重复 `detach` |
| `use_after_destroy>0` | `--cancel-free` 是否被误开 | 销毁后没 `EPOLL_CTL_DEL` |
| `cancelled=0` 但客户端等不到 | `arm()` 是否设了 deadline | 超时挂在了不存在的刷新点上（§8.2） |
| `max_run` 与该用例预期不符 | 客户端的发送方式 | 流水线 vs 请求-响应（§7.2 结论 1） |
| 进程退出但 `serve_ms` 很短 | `accepted>0 && alive_slots()==0` | 正常：空闲退出（400ms）命中 |

排查纪律：先区分是**协程状态机**错还是**判据**错。`judge.py` 的每条断言都对应
一个 `negtest.py` 变异；断言失败时，先用对应变异确认"这条断言真的在看那一行"。

---

## 10. 练习题

**基础**

1. 把 `Task<T>` 的 `initial_suspend` 改成 `suspend_never`，运行 `task` 用例。
   预测 `resumes` 与 `alive_before_destroy` 的值，再实测对照。
2. 给 `Generator<T>` 加 `begin()/end()` 迭代器支持，使 `for (int v : squares(5))` 可用，
   并保证提前 `break` 时帧被销毁（提示：毁构函数 + move 语义）。
3. 让 `Sched::run()` 在没有待恢复协程时必须退出（现在依赖 `alive_slots()`）。
   构造一个"槽位活着但句柄为空"的状态，说明为什么不能只看 `alive_slots()`。

**改错**

4. 下面的 `on_done` 少了一行，找出它并说明症状：
   ```cpp
   s.on_done = [&](int fd, std::coroutine_handle<> h) { h.destroy(); close(fd); };
   ```
5. `run_once` 里把"快照长度"改成 `while (!ready_hi_.empty())`。构造一个用例让它饿死低优先级，
   并说明为什么 `max_run` 看不出这个问题。

**综合**

6. 给调度器加一个 `co_await s.sleep(ms)`（不带 fd）：设计一个不依赖 fd 的槽位或定时器最小堆，
   使 `coro-idle` 能改成"服务端自己休眠 400ms 后主动发一帧"，并要求 `alive_frames=0`。
7. 实现"连接级背压"：当某连接的待发缓冲超过 N 字节，`send` 挂起在 `EPOLLOUT` 上，
   并统计"因背压挂起的次数"。构造一个客户端只发不收，用读数证明背压生效。
8. 把 `--mode thread` 改成线程池（固定 4 线程 + 任务队列），比较两种形态的
   `VmSize`、`serve_ms` 和"连接数超过线程数时的排队行为"，写出适用边界。

> 验收时只给参考实现，不提前给答案。

---

## 11. 验收标准

- [x] `make` 零警告（`-Wall -Wextra -Werror`）。
- [x] `./run.sh` 的 18 个用例跑完，`judge.py` 58 项全通过。
- [x] `negtest.py` 22/22：原日志通过、20 条用例内变异全部被抓、缺日志返回 2。
- [x] 每个读数都在日志里可查（`logs/coro.log`），不是"跑过就算"。
- [x] 破坏版都有配对基线（`coro-idle` vs `eagain-as-eof`）。
- [x] 未闭环项显式标注（§7.3 的 VmSize 成因、§8.2 无失败日志）。
- [ ] 真机多核、`io_uring`、跨线程协程调度：**未做**，不在本实验范围。

能回答下面五个问题才算掌握：

1. `final_suspend` 选 `suspend_always` 和 `suspend_never` 各会带来什么后果？
2. 为什么"让出"不能等 epoll 事件？
3. `max_run` 为什么不是通用的公平性指标？
4. 销毁帧之前为什么必须先 `EPOLL_CTL_DEL`？
5. 为什么协程版 `VmSize` 不涨，而线程版涨得很多，`RSS` 却都不涨？

---

## 12. 延伸阅读

- cppreference：*Coroutines (C++20)* —— `promise_type` 各钩子的调用时机。
- Lewis Baker, *C++ Coroutines: Understanding operator co_await* —— 三段协议与对称转移。
- Lewis Baker, *Asymmetric Transfer* 与 *Symmetric Transfer and Sutter's Mill* —— 为什么对称转移能避免栈增长。
- `man 7 epoll`，特别是 `EPOLLONESHOT` 与 `EPOLLET` 的语义差别。
- 阶段 4（`../SYS_04_Reactor/README.md`）：`EPOLLONESHOT` 必须重挂的实测过程。
- 阶段 6（规划中）：用 `perf`/Sanitizer 复核本实验的取消路径与帧生命周期。
