# SYS_02：IPC 与线程同步

> 结论先行，六条都由读数支撑：
>
> 1. **消息边界是通道属性，不是消息属性。** 同一批 3×1000 字节，`pipe` / `fifo` / `UDS-SOCK_STREAM`
>    读一次都拿到 **3000** 字节（`boundary=merged`），只有 `UDS-SOCK_DGRAM` 拿到 3 个独立的
>    1000 字节包（`msgs=3 lens=1000,1000,1000`）。
> 2. **`SOCK_DGRAM` 没有 EOF 语义。** 对端 `close` 之后 `recv` 仍然阻塞——实测直接挂到超时（rc=124），
>    改成应用层 1 字节哨兵才收尾（`eof_via=sentinel`）。字节流相反：关掉**所有**写端副本后 `read` 返回 0。
> 3. **`while` 不是保险，是必需品，而且理由有两个。** 一个是被构造出来的虚假唤醒：
>    `if` 版本 `flag_at_exit=0`、`safe=0`，`while` 版本 `flag_at_exit=1`、`safe=1`。
>    另一个是正常竞争——实测 `stale_wake=18`：醒来时货已经被另一个消费者拿走了。
>    **即使一次虚假唤醒都没有，`if` 也是错的。**
> 4. **数据竞争能被工具检出，也能被读数检出。** 无锁版本 4×20000 次只累加到 21658（丢 58342），
>    TSan 报 2 次 data race；互斥锁与 `_Atomic` 版本都是 80000 且 TSan 零报告。
> 5. **死锁可以不挂死地观测。** 用 `pthread_mutex_timedlock` 把"卡死"变成 `ETIMEDOUT(110)`：
>    锁顺序反转组 `t1_rc=t2_rc=110 detected=1`，统一顺序组 `t1_rc=t2_rc=0 safe=1`。
> 6. **关闭协议决定丢不丢任务，不决定丢不丢线程。** 同一批 200 个任务：优雅关闭 `executed=200 missing=0`，
>    立即关闭 `executed=1 missing=199`，但两组都是 `joined=4/4 fd_leak=0`。
>    丢任务与漏资源是两件事，必须分开判。

## 学习目标

1. 能说清四种 IPC（匿名管道、命名管道、共享内存、Unix Domain Socket）各自保留什么、不保留什么。
2. 能解释字节流与数据报在"消息边界"和"结束条件"上的差别，并写出正确的收尾逻辑。
3. 能用 `pthread_mutex_t` / `pthread_cond_t` 写一个有界队列，并解释为什么等待必须用 `while`。
4. 能说清"被唤醒"与"条件成立"之间的时间差（`stale_wake`），并说明它与虚假唤醒不是一回事。
5. 能用 TSan 定位真实的数据竞争，并区分"有竞争"与"没检测到"。
6. 能用 `timedlock` 把死锁变成可观测的返回值，并用统一加锁顺序消除它。
7. 能设计线程池的关闭协议，并分别验证"任务不丢"与"资源不漏"。

## 环境与前置条件

| 项 | 值 |
| --- | --- |
| 平台 | WSL2 Ubuntu，`Linux 6.6.87.2-microsoft-standard-WSL2`，x86_64 |
| 编译器 | `gcc 15.2.0`，`-O2 -g -std=c11 -Wall -Wextra -Werror -pthread`，零警告 |
| 额外工具 | `-fsanitize=thread`（TSan 可用，`libtsan.so.2`）；不需要 `sudo` |
| 依赖 | 只需要 libc + libpthread（glibc 2.34 起并入 libc）；`run.sh` 用 `bash` + `python3` |
| 前置 | 完成 [阶段 1](../SYS_01_FileIO_Proc/README.md)：文件描述符、`fork`、短读、`EINTR` |

**为什么不用 `valgrind --tool=helgrind`**：本环境没有安装 `valgrind`（也不使用 `sudo` 安装），
TSan 是编译器自带的能力，同样能给出竞争双方的两条栈帧，见 `logs/tsan_plain.raw`。

## 核心概念

### 1. 四种 IPC 的语义对照

| 通道 | 边界 | 结束条件 | 命名 | 同步语义 |
| --- | --- | --- | --- | --- |
| 匿名管道 `pipe()` | **无**（字节流） | 所有写端关闭 → `read` 返回 0 | 无，只能靠继承 | 自带（写入即唤醒读者） |
| 命名管道 `mkfifo` | **无**（字节流） | 同上 | 文件系统里有名字 | 自带 |
| 共享内存 `shm_open+mmap` | 无（就是一块内存） | 没有"结束"，只有 unlink | 有名字（POSIX） | **完全没有**，必须另配同步 |
| `AF_UNIX SOCK_STREAM` | **无**（字节流） | 所有写端关闭 → `recv` 返回 0 | 可有（路径）或无（`socketpair`） | 自带 |
| `AF_UNIX SOCK_DGRAM` | **有**（逐条投递） | **没有 EOF**，对端关了自己不会返回 0 | 同上 | 自带 |

最后一行是本实验最容易踩的地方：数据报的"对端已关闭"不会变成"读到 0 字节"，
必须由应用层约定结束条件。本实验用 1 字节哨兵，并把 `eof_via=sentinel` 写进读数。

### 2. 条件变量：醒来 ≠ 条件成立

`pthread_cond_wait` 返回只说明"有人调用了 signal/broadcast 并且我拿到了锁"，
不说明"我要等的东西现在就有了"。两种情况都会让它白醒：

| 情况 | 成因 | 实测 |
| --- | --- | --- |
| 虚假唤醒 | 实现允许无理由唤醒 | 人为构造：`hits=1`（`if` 与 `while` 都记录到了） |
| 正常竞争 | 另一个消费者先一步取走了唯一一件货 | 自然发生：`stale_wake=19` |

所以等待必须写成：

```c
pthread_mutex_lock(&mu);
while (!condition)          /* 不是 if */
    pthread_cond_wait(&cv, &mu);
/* 到这里才真正持有 condition */
pthread_mutex_unlock(&mu);
```

### 3. `signal` 与 `broadcast` 的选择

`signal` 唤醒**至少一个**等待者，`broadcast` 唤醒**全部**。本实验的关闭协议用 `broadcast`：
关闭时所有工作线程都必须醒来看 `shutdown` 标志，用 `signal` 只叫醒一个，其余线程会一直挂在
`cond_wait` 上，`pthread_join` 就永远等不到。这是一处"看起来对、实际挂死"的典型写法。

### 4. 数据竞争的两种后果

无锁读改写丢更新，是**结果错**；不变量被撕裂（两个字段不同步），是**状态不自洽**。
两者都可以用同一套办法观察：在"读"与"写"之间插 `sched_yield`，把窗口放大到必现。

`volatile` 既不是原子性也不是可见性保证——`counter_plain` 就是 `long`，
编译器优化前它照样丢 57869 次。

### 5. 死锁与加锁顺序

两个线程各持一把锁、再去要对方那把，就构成环。消除办法是**全局统一的加锁顺序**：
所有线程都按 A→B 拿，环就不可能形成。

本实验用 `pthread_mutex_timedlock` 让"死锁"变成可判定的返回值而不是挂起进程——
这不是生产写法（生产中更常用 `trylock` 退避或锁层级断言），但它让故障可观测、可复现、可入判据。

### 6. 线程池的关闭协议

| 协议 | 行为 | 结果 |
| --- | --- | --- |
| 优雅关闭 | 置 `shutdown` 后，工作线程先把队列里的任务做完，队列空了才退出 | `executed=200 missing=0` |
| 立即关闭 | 置 `shutdown` 后工作线程立刻退出，队列里的任务丢弃 | `executed=1 missing=199` |

两者都必须是 `joined=4/4`、`fd_leak=0`。**"不丢任务"和"不漏资源"是两个独立判据**，
优雅关闭写错会漏任务，立即关闭写错会漏线程。

## 接口卡片

### `int pipe(int fds[2])`

- **作用**：创建单向字节流通道，`fds[0]` 读、`fds[1]` 写，只能靠 `fork`/`exec` 传递。
- **返回值**：0 成功，-1 置 `errno`（`EMFILE` / `ENFILE` / `EFAULT`）。
- **资源所有权**：两个 fd 都要由调用方关闭。**读端要读到 EOF，必须关掉所有写端副本**——
  包括 `fork` 出来的子进程手里那份。

### `int mkfifo(const char *path, mode_t mode)`

- **作用**：在文件系统里创建一个命名管道。
- **返回值**：0 成功；-1 置 `errno`（`EEXIST` 已存在）。
- **资源所有权**：它是一个文件系统对象，`unlink` 才消失；`open` 会阻塞直到对端打开。
- **常见误用**：只 `close` 不 `unlink`，下次运行时 `mkfifo` 报 `EEXIST`。

### `int shm_open(const char *name, int flags, mode_t mode)` + `mmap`

- **作用**：创建/打开一块可被多个进程映射的共享内存对象。
- **返回值**：fd ≥0（`shm_open`）；`mmap` 返回指针或 `MAP_FAILED`。
- **资源所有权**：`shm_unlink` 只解除名字，已映射的进程继续可用；最后一个映射消失后回收。
- **关键限制**：**不提供任何同步**。读写顺序、可见性都要自己保证。

### `int socketpair(int domain, int type, int protocol, int sv[2])`

- **作用**：创建一对已连接的、无名的 Unix Domain Socket。
- **返回值**：0 成功；-1 置 `errno`。
- **与 pipe 的差别**：全双工；`SOCK_DGRAM` 保留消息边界；不需要经过文件系统。

### `int pthread_cond_wait(pthread_cond_t *cv, pthread_mutex_t *mu)`

- **作用**：原子地"释放互斥量 + 阻塞等待"，返回时**重新持有**互斥量。
- **返回值**：0；错误不通过 `errno` 返回，而是直接返回错误码。
- **必须配合**：`while` 检查条件；`broadcast` 用于唤醒全部等待者。
- **常见误用**：用 `if`；忘记它返回时已持有锁；在 `signal` 时不持锁导致丢唤醒。

### `int pthread_mutex_timedlock(pthread_mutex_t *mu, const struct timespec *abs_timeout)`

- **作用**：带绝对超时的加锁。
- **返回值**：0 成功；`ETIMEDOUT`（110）超时；`EINTR`、`EDEADLK` 等。
- **注意**：超时是 `CLOCK_REALTIME` 的**绝对**时间，不是相对时长。
- **用途**：让死锁变成可观测的返回值；生产代码里通常配合重试或退避。

### `-fsanitize=thread`

- **作用**：编译期插桩，运行期检测数据竞争与锁使用错误。
- **输出**：`WARNING: ThreadSanitizer: data race` + 双方栈帧。
- **退出码**：报告竞争时默认 `exitcode=66`（可用 `TSAN_OPTIONS=exitcode=N` 改）。
- **局限**：只报告**实际执行到**的竞争；没跑到就不能说明"没问题"。

## 最小可运行实验

```text
SYS_02_IPC_Threads/
├── ipc_basic.c      A：pipe / fifo / shm / UDS-stream / UDS-dgram 五种通道同一套收发
├── condvar.c        B：虚假唤醒（if vs while）+ 有界队列生产者消费者
├── race_detect.c    C：无锁 / 互斥锁 / 原子 三种计数器 + 读写锁不变量
├── deadlock.c       D：加锁顺序反转 vs 统一顺序（timedlock 观测）
├── threadpool.c     E：线程池 + 优雅关闭 vs 立即关闭
├── Makefile         含 `tsan` 目标（单独编 -fsanitize=thread 版本）
├── judge.py         26 条断言，只读日志
└── run.sh           六组取证 + 判定
```

## 编译与运行

```bash
cd experiments/SYS_02_IPC_Threads
./run.sh                              # 六组全跑 + 判定，全通过时退出码 0
./run.sh cond                         # 只跑一组（ipc|cond|race|deadlock|pool|tsan）
./run.sh --judge pool logs/pool.log   # 只判定不跑（0/1/2）
make tsan                             # 单独编 TSan 版本
make clean
```

## 结果观察

`logs/ipc.log`（2026-10-27 一次运行的原文）：

```text
SYS02|ipc|pipe|sent=3000|first_read=3000|reads=1|total=3000|boundary=merged
SYS02|ipc|fifo|created=1
SYS02|ipc|fifo|sent=3000|first_read=3000|reads=1|total=3000|boundary=merged
SYS02|ipc|shm|child_value=12345|same=1|sync_via=pipe
SYS02|ipc|shm|after_unlink=gone|errno=2
SYS02|ipc|uds-stream|sent=3000|first_read=3000|reads=1|total=3000|boundary=merged
SYS02|ipc|uds-dgram|sent=3000|msgs=3|lens=1000,1000,1000|first_len=1000|boundary=preserved|eof_via=sentinel
```

三条字节流通道的 `first_read` 都等于 `sent`（3000），说明三条消息被合并成一次读取；
`uds-dgram` 的 `first_len` 只有 1000，`msgs=3`，边界保住了。

`logs/cond.log`：

```text
SYS02|cond|spurious|check=if|rounds=208|hits=1|flag_at_exit=0|safe=0
SYS02|cond|spurious|check=while|rounds=169|hits=1|flag_at_exit=1|safe=1
SYS02|cond|queue|producers=2|consumers=2|cap=8|produced=1000|consumed=1000|max_depth=8|stale_wake=18|missing=0|dup=0|ok=1
```

`rounds` 是被迫广播的次数（208 / 169），不是稳定性指标——它说明"广播正好落在对方已经阻塞之后"
需要试几次，所以用循环而不是一次性握手。`stale_wake=18` 是正常竞争，不是虚假唤醒。

`logs/race.log`：

```text
SYS02|race|mode=plain|threads=4|iters=20000|expected=80000|final=21658|lost=58342
SYS02|race|mode=mutex|threads=4|iters=20000|expected=80000|final=80000|lost=0
SYS02|race|mode=atomic|threads=4|iters=20000|expected=80000|final=80000|lost=0
SYS02|rwlock|mode=rwlock|readers=3|writer_updates=20000|torn=0|safe=1
SYS02|rwlock|mode=rwlock-broken|readers=3|writer_updates=20000|torn=28482|safe=0
```

`logs/deadlock.log`：

```text
SYS02|deadlock|mode=deadlock|t1_rc=110|t2_rc=110|t1_progress=1|t2_progress=1|detected=1|safe=0
SYS02|deadlock|mode=ordered|t1_rc=0|t2_rc=0|t1_progress=1|t2_progress=1|detected=0|safe=1
```

110 是 `ETIMEDOUT`。两组 `*_progress=1` 说明两个线程都拿到了自己的第一把锁，
差别只在第二个锁的顺序——这是单变量的证明。

`logs/pool.log`：

```text
SYS02|pool|mode=graceful|workers=4|submitted=200|executed=200|missing=0|dup=0|joined=4|max_depth=200|fd_before=5|fd_after=5|fd_leak=0|ok=1
SYS02|pool|mode=immediate|workers=4|submitted=200|executed=1|missing=199|dup=0|joined=4|max_depth=200|fd_before=5|fd_after=5|fd_leak=0|ok=1
```

`logs/tsan.log`（由 `run.sh tsan` 汇总，原始栈帧在 `logs/tsan_*.raw`）：

```text
TSAN|mode=plain|reports=2|rc=66
TSAN|mode=mutex|reports=0|rc=0
TSAN|mode=atomic|reports=0|rc=0
TSAN|mode=rwlock|reports=0|rc=0
TSAN|mode=rwlock-broken|reports=2|rc=66
```

`tsan_plain.raw` 里竞争双方各有一条栈帧，都指向 `race_detect.c` 的 `plain_worker`：

```text
WARNING: ThreadSanitizer: data race (pid=1309)
  Read of size 8 at 0x5555555580f0 by thread T2:
    #0 plain_worker .../race_detect.c:68
  Previous write of size 8 at 0x5555555580f0 by thread T1:
    #0 plain_worker .../race_detect.c:71
  Location is global 'counter_plain' of size 8
```

两条 `#0` 指向同一函数的相邻三行（读 68 / 写 71），`Location` 指明是全局变量 `counter_plain`——
这是"代码里真的没有同步"的直接证据，而不是"结果碰巧算错"。

### 跨运行漂移（为什么判据不写死这些数字）

本节所有读数为同一天两次运行的对照。调度相关的量每次都不同：

| 读数 | 第一次 | 第二次（当前 `logs/`） | 判据怎么写的 |
| --- | --- | --- | --- |
| `plain.lost` | 57869 | 58342 | 只判 `> 0` |
| `rwlock-broken.torn` | 26603 | 28482 | 只判 `> 0` |
| `queue.stale_wake` | 19 | 18 | **不判**，只记录 |
| `spurious.rounds` | 188 / 201 | 208 / 169 | **不判**，只记录 |

只有"必然为某值"的量（`mutex/atomic.lost = 0`、`deadlock.t1_rc = 110`、`executed/submitted` 的关系）
才写进断言。这正是阶段 1 那条教训的延续：**判据要断言关系，不要断言会变的绝对值。**

## 实测记录

### 判定器 26 条断言与 14 条负向验证

对实测日志的临时副本做变异（不重跑程序）：

| 变异 | 期望 | 实际 |
| --- | --- | --- |
| 六组原日志 | 0 | 0（6/6） |
| `pipe` 的 `boundary` 改成 `single` | 1 | 1 |
| `uds-dgram` 的 `msgs` 改成 2 | 1 | 1 |
| `shm` 的 `after_unlink` 改成 `alive` | 1 | 1 |
| `spurious-if` 的 `flag_at_exit` 改成 1 | 1 | 1 |
| `queue` 的 `dup` 改成 1 | 1 | 1 |
| `plain` 的 `lost` 改成 0 | 1 | 1 |
| `rwlock-broken` 的 `torn` 改成 0 | 1 | 1 |
| `deadlock` 的 `t1_rc` 改成 0 | 1 | 1 |
| `immediate` 的 `executed` 改成 200 | 1 | 1 |
| `pool` 的 `fd_leak` 改成 1 | 1 | 1 |
| `tsan` 的 `mutex` 报告数改成 1 | 1 | 1 |
| `tsan` 的 `plain` 报告数改成 0 | 1 | 1 |
| 指向不存在的日志 | 2 | 2 |
| 未知用例名 | 2 | 2 |

### 开发中遇到的三处真实错误

**错误一：管道读端等不到 EOF。** 首版让子进程直接读，但 `fork` 后子进程手里还留着写端的副本，
`read` 循环永远等不到 0，整个用例挂死。修法：读端进程必须先 `close` 写端副本，写端进程先 `close` 读端副本。
**这条错误本质上是阶段 1 的结论——`fork` 复制 fd 表。**

**错误二：`SOCK_DGRAM` 没有 EOF。** 以为"关掉对端 → `recv` 返回 0"，实测直接 `timeout` 杀掉（rc=124）。
修法：发送端在数据后补一条 1 字节哨兵，接收端识别哨兵退出，读数里记 `eof_via=sentinel`。
**字节流的收尾规则不能套到数据报上。**

**错误三：把"醒来时条件不成立"叫成了虚假唤醒。** 首版把队列里的计数器命名为 `spurious`，
实测报 18–19 次。但它记的是"被唤醒后货已被另一个消费者取走"，这是完全正常的竞争，
不是虚假唤醒。已改名为 `stale_wake` 并写清区别——
这个区别恰恰解释了"为什么没有虚假唤醒也必须用 `while`"。

## 常见错误和陷阱

1. **用 `if` 检查条件**：虚假唤醒与正常竞争都会让它带着不成立的条件往下走。
2. **`signal` 用于关闭**：只叫醒一个线程，其余线程挂在 `cond_wait`，`join` 永远等不到。
3. **`fork` 后不关多余的 fd 副本**：读端等不到 EOF，进程挂死。
4. **把 `SOCK_DGRAM` 当成有 EOF**：对端关闭不产生 0 返回值。
5. **认为 `volatile` 能保证原子性**：它既不原子也不保证可见性顺序。
6. **`fd` 泄漏只查 `open` 不查 `/proc/self/fd`**：`dup`、`socket`、`pipe` 都会漏。
7. **把死锁写成"卡住就手动 Ctrl-C"**：锁定不住形式就无法进判据、无法回归。
8. **优雅关闭时忘记 `broadcast` 之后再检查一次队列**：留置任务被丢。
9. **TSan 零报告当成"没有竞争"**：它只报告实际执行到的路径。
10. **`pthread_mutex_timedlock` 传相对时间**：它要的是 `CLOCK_REALTIME` 的**绝对**时刻。

## 故障排查方法

| 现象 | 先查什么 | 判据 |
| --- | --- | --- |
| 读端永远读不到 EOF | 所有写端副本是否关闭 | `read` 返回 0 才说明没写端了 |
| `recv` 在数据报上永久阻塞 | 是否依赖了对端 close 产生 0 | 用哨兵或长度前缀 |
| 消费者偶发拿到空队列 | 是否用了 `if` | 改成 `while` 并复跑 |
| `join` 一直不返回 | 是否用 `signal` 唤醒多个等待者 | 关闭路径必须 `broadcast` |
| 计数器结果偏小 | 是否无锁读改写 | 对比 `expected` 与 `final` |
| TSan 有报告但结果正确 | 竞争不等于当下算错 | 结果是巧合，竞争是真的 |
| 死锁无法复现 | 是否缺少固定交错 | 用 barrier 或 timedlock |
| `fd` 数只增不减 | 每条创建路径是否有对应释放 | `/proc/self/fd` 前后对比 |

## 练习题

### 基础题

1. 把 `condvar.c` 队列的 `signal` 全部改成 `broadcast`，预测 `stale_wake` 会变大还是变小，
   再验证并解释。
2. 把 `race_detect.c` 的 `plain` 模式去掉 `sched_yield`，预测 `lost` 会变成多少，跑 5 次记录分布。

### 改错题

3. 下面这段线程池关闭代码有什么问题？

   ```c
   pthread_mutex_lock(&p->mu);
   p->shutdown = 1;
   pthread_cond_signal(&p->has_work);   /* 只叫醒一个 */
   pthread_mutex_unlock(&p->mu);
   for (int i = 0; i < WORKERS; i++)
       pthread_join(th[i], NULL);
   ```
   指出至少两处，并给出最小修复。

4. 下面这段"安全的共享内存"有什么问题？

   ```c
   int *p = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
   *p = 42;                    /* 进程 A 写 */
   /* 进程 B 直接读 */
   printf("%d\n", *p);
   ```
   说明为什么"能看到同一块内存"不等于"能正确同步"。

### 综合题

5. 给 `threadpool.c` 加"在途任务计数"：`pthread_join` 之前必须保证既没有排队任务也没有执行中任务。
   说明你需要哪些字段、哪些锁，以及如何在关闭路径上验证这个更强的性质。
6. 用 `AF_UNIX SOCK_SEQPACKET` 重做 `ipc_basic` 的一组，说明它在"边界"与"连接"上
   相对 `SOCK_STREAM` / `SOCK_DGRAM` 的差别，并补进判定器。

## 验收标准

- [ ] 能跑 `./run.sh` 得到六组全过，并复述三种字节流与一种数据报的边界差别。
- [ ] 能解释为什么 `SOCK_DGRAM` 需要哨兵而管道不需要。
- [ ] 能说清虚假唤醒与 `stale_wake` 的区别，并解释为什么两者都要求 `while`。
- [ ] 能解释 `plain` 模式为什么必然丢更新，以及 `sched_yield` 在这里的作用。
- [ ] 能说清 `timedlock` 让死锁"可观测"的原理与代价。
- [ ] 能分别验证"任务不丢"与"资源不漏"，并解释为什么它们是两个判据。
- [ ] 能对自己的日志做一次变异，让 `judge.py` 按预期失败。

## 延伸阅读

- `man 2 pipe` / `man 7 pipe`（容量、原子写 `PIPE_BUF`）。
- `man 7 unix`（`SOCK_DGRAM`/`SOCK_SEQPACKET`/`SOCK_STREAM` 的语义差别）。
- `man 3 shm_open`、`man 2 mmap`（`MAP_SHARED` 与可见性）。
- `man 3 pthread_cond_wait` 的 "Spurious wakeups from the pthread_cond_timedwait() or
  pthread_cond_wait() functions may occur" 一段。
- `man 3 pthread_mutex_timedlock`（绝对超时）。
- GCC 手册 `-fsanitize=thread` 与 `TSAN_OPTIONS`。
- 后续实验：[`SYS_03_Socket`](../SYS_03_Socket/README.md)（网络 I/O）。

## 已知限制

- **线程池没有真正经历背压**：`max_depth=200` 是因为提交在持锁状态下一次做完，
  工作线程根本没机会插进来。真实的队列深度压力需要分散提交（例如生产者线程 + 间歇 `sleep`）。
- **未覆盖协程/异步**：本实验全是阻塞式线程模型。
- **`stale_wake` 不是概率指标**：它依赖调度，18–19 只是两次运行的读数，不能当"频率"引用。
- **TSan 只报告执行到的路径**：零报告不等于零竞争；要覆盖分支需要更多输入。
- **死锁只覆盖两把锁的环**：三把锁、条件变量参与、递归锁的 `EDEADLK` 未覆盖。
- **没有测性能**：不加锁、加锁、原子操作的吞吐差别需要单独基准（阶段 6 做）。
- **`shm` 只测了匿名/已 `unlink` 的用法**：跨进程按名字访问、`shm_open` 权限位、`/dev/shm` 落地未覆盖。
