# SYS_03：Socket、字节流与多路复用

> 结论先行，六条都由读数支撑：
>
> 1. **TCP 是字节流，UDP 是数据报——这是两种不同的东西，不是"两种传输方式"。**
>    三次 1000 字节的 `write` 被一次 `recv` 收成 **3000**（`boundary=merged`）；
>    三条 1000 字节的 `sendto` 被三次 `recvfrom` 各收 **1000**（`recv_lens=1000,1000,1000`）。
>    要在 TCP 上恢复消息边界只有两条路：长度前缀或分隔符，本实验用 4 字节长度前缀
>    （`framed=3 client_frames=3`）。
> 2. **`EPIPE` 和 `ECONNRESET` 不是同一个错误，触发条件相反。**
>    首版照搬 `SO_LINGER{on,0}`（RST）写法测 SIGPIPE，两次都得到 `ECONNRESET(104)`、
>    `handler_hits=0`——信号一次没来。改成**正常关闭（FIN）**后才得到
>    `ignore_errno=32`(EPIPE)、`handler_errno=32`、`handler_hits=1`。
>    RST 让 socket 直接进错误态；只有 FIN 之后继续写才走 SIGPIPE 路径。
> 3. **半关闭是"我发完了"而不是"连接没了"。** `shutdown(SHUT_WR)` 之后，
>    对端 `recv` 读到 0（`eof_seen=1`），但服务端仍能写（`sent_after_eof=1`），
>    客户端也仍能读（`read_after_shutdown=1`）。把 EOF 当成"不能收"是协议设计事故。
> 4. **短写是常态，不是异常。** 发送缓冲被压到 4 KB、写 1 MiB，
>    非阻塞 `send` 出现 **17 次**部分写（`short_writes=17`）。
>    判据不是"有没有短写"，而是"循环之后 `sent == recv == payload` 且 `mismatch=0`"。
> 5. **`recv` 超时得到 `EAGAIN`，不是 0、也不是错误退出。** `SO_RCVTIMEO=200ms`
>    实测 `elapsed_ms=202`、`errno=11`。把 `-1` 一律当"连接断开"会把超时误判成断连。
> 6. **ET 的"必须读到 `EAGAIN`"可以被证伪，而不是只靠背。**
>    同一份代码，正确版把 100 字节读到 EAGAIN，8 条连接全部完成；
>    只读一次的版本留下 84 字节在内核缓冲里、不再产生事件，
>    8 条连接全部停住（`done=0/8 stalled=1 elapsed_ms=2002`）。
>    **这是 ET 的真实失效方式，不是模拟。**

## 学习目标

1. 能说清 TCP 字节流与 UDP 数据报在"边界""结束条件""错误语义"上的差别。
2. 能写出在短写、短读、`EINTR`、超时、半关闭、对端 RST 下都正确的收发代码。
3. 能区分 `EPIPE`、`ECONNRESET`、`EAGAIN`、`EMSGSIZE` 各自的触发条件。
4. 能用 `select` / `poll` / `epoll` 三套接口实现同一个单线程多连接服务。
5. 能解释 LT 与 ET 的差别，并用"读到 EAGAIN"证明 ET 实现是否正确。
6. 能用长度前缀把字节流切成消息，并说明为什么分隔符方案有额外风险。
7. 能设计"对照组必须真的坏"的判据（`epoll-et-broken`、`reset`、`toobig`）。

## 环境与前置条件

| 项 | 值 |
| --- | --- |
| 平台 | WSL2 Ubuntu，`Linux 6.6.87.2-microsoft-standard-WSL2`，x86_64 |
| 编译器 | `gcc 15.2.0`，`-O2 -g -std=c11 -Wall -Wextra -Werror`，零警告 |
| 依赖 | 只需 libc；不需要 `sudo`、不需要额外库 |
| 网络 | 只用 `127.0.0.1`，监听端口由 `bind(端口 0)` 由内核分配 |
| 前置 | [SYS_02](../SYS_02_IPC_Threads/README.md)：`fork`/fd 语义、`EINTR`、`while` 重试 |

**为什么端口不写死**：`bind` 到端口 0 后 `getsockname` 取回真实端口，
用例可以反复运行、并行运行、失败重跑而不撞 `EADDRINUSE`。
代价是日志里没有固定端口号——判定器也就不需要知道端口。

## 核心概念

### 1. 字节流 vs 数据报

| 维度 | TCP（`SOCK_STREAM`） | UDP（`SOCK_DGRAM`） |
| --- | --- | --- |
| 消息边界 | **没有**，读多少算多少 | **有**，一次 `sendto` 对应一次 `recvfrom` |
| 结束信号 | 所有写端关闭 → `recv` 返回 0 | **没有**，只能靠应用层约定 |
| 可靠/有序 | 是 | 否 |
| 对端关闭的表现 | FIN → 读到 0 | 无表现；`connect` 过的 socket 可能收到 `ECONNREFUSED` |
| 超出缓冲 | 不适用（本来就没有边界） | **静默截断**，`MSG_TRUNC` 才能拿到真实长度 |
| 单包上限 | 不适用 | ≈65507 字节，超出 `EMSGSIZE` |

### 2. 四个 errno 的触发条件（本实验全部实测）

| errno | 值 | 触发条件 | 本实验读数 |
| --- | --- | --- | --- |
| `EPIPE` | 32 | 对端已发 FIN，本地仍 `write`；伴随 `SIGPIPE` | `ignore_errno=32 handler_errno=32` |
| `ECONNRESET` | 104 | 对端发 RST（`SO_LINGER{on,0}` 关闭或异常终止） | `reset errno=104` |
| `EAGAIN` | 11 | 非阻塞下无数据，或 `SO_RCVTIMEO` 超时 | `timeout errno=11` |
| `EMSGSIZE` | 90 | 数据报超过链路/协议上限 | `toobig errno=90` |

**`EPIPE` 与 `ECONNRESET` 最容易混**：两者都是"对端没了"，但一个是 FIN 之后的写入，
一个是 RST 之后的任何操作。服务端把两者当成同一种断连处理通常没错，
但客户端重试策略会完全不同（RST 往往意味着对端进程已崩溃，重试没有意义）。

### 3. 半关闭

`shutdown(fd, SHUT_WR)` 只关写方向，读方向仍可用。它对应 TCP 的一个 FIN。
用途：请求-响应协议里"我发完了，等你的结果"。
本实验的顺序是：

```text
客户端  write("PING") → shutdown(SHUT_WR)
服务端  recv 得到 4 字节 → 再 recv 得到 0（FIN）
服务端  write("PONG")                       ← 此时写方向仍有效
客户端  recv 得到 4 字节                    ← 自己已关写，但仍能读
```

### 4. 短写与短读

`write`/`send` 返回小于请求长度的正数是正常现象：发送缓冲区满了。
必须循环。本实验把 `SO_SNDBUF` 压到 4 KB 再写 1 MiB，短写出现 17 次。
**反面写法**是 `if (write(fd, buf, len) != len) error;`——它在压力下必然误报。

读侧同理：`recv` 一次能拿到多少不确定，只有"读到 0"才表示对端关了写方向。

### 5. LT 与 ET

| 模式 | 语义 | 实现要求 |
| --- | --- | --- |
| 水平触发 LT（`select`/`poll`/默认 `epoll`） | 只要还有数据，每次 `wait` 都会返回 | 读一次也行，下次还会通知 |
| 边沿触发 ET（`EPOLLET`） | 只在状态**变化**时通知一次 | **必须循环读到 `EAGAIN`**，否则剩余数据永远不触发 |

本实验把读块设为 16 字节：100 字节的消息需要 7 次 `recv`。
正确实现循环到 `EAGAIN`；`epoll-et-broken` 只读一次，剩下 84 字节烂在缓冲里，
`epoll_wait` 再也不会返回（`events` 停在 8，`elapsed_ms=2002` 撞到判定上界）。

### 6. 三套多路复用接口

| 接口 | 数据结构 | 上界 | 每次调用成本 | 适合作什么 |
| --- | --- | --- | --- | --- |
| `select` | 三个 `fd_set` 位图 | `FD_SETSIZE`（通常 1024） | 每次重填集合 + 内核 O(n) 扫描 | 连接数少、跨平台 |
| `poll` | `struct pollfd` 数组 | 无（受内存限制） | 内核 O(n) 扫描 | 同上，但没有位图上界 |
| `epoll` | 内核红黑树 + 就绪链表 | 无 | 注册一次，返回仅就绪项 | 大量长连接 |

三者在本实验里服务同样的 8 条连接，读数一致（`done=8/8 echoed=8 mismatch=0 events=16`），
区别在**规模**而不是**功能**——这也是为什么只靠这个实验不能证明 epoll 更快。

## 接口卡片

### `int socket(int domain, int type, int protocol)` / `bind` / `listen` / `accept`

- **作用**：建立监听端点；`accept` 返回一个**新的**连接 fd（监听 fd 继续用）。
- **返回值**：fd ≥0；-1 置 `errno`（`EADDRINUSE`、`EMFILE`、`ECONNABORTED`）。
- **资源所有权**：监听 fd 与每个连接 fd 都要分别 `close`。
- **常见误用**：把 `accept` 返回的 fd 当成监听 fd 继续 `accept`；只关一个。

### `int shutdown(int sockfd, int how)`

- **作用**：`SHUT_WR` / `SHUT_RD` / `SHUT_RDWR` 关闭某个方向。
- **与 `close` 的差别**：`close` 关的是 fd 引用，其他 `dup`/`fork` 的副本仍可读写；
  `shutdown` 直接作用于连接，所有副本都受影响。
- **常见误用**：以为 `shutdown(SHUT_WR)` 之后就不能 `recv`。

### `ssize_t send(int sockfd, const void *buf, size_t len, int flags)`

- **flags 关键项**：`MSG_NOSIGNAL` 抑制 `SIGPIPE`；`MSG_DONTWAIT` 单次非阻塞。
- **返回**：实际写入字节数（可能小于 `len`）；-1 置 `errno`。
- **必须**：循环直到写完，且分别处理 `EINTR`（重试）、`EAGAIN`（等可写）、`EPIPE`（对端关闭）。

### `setsockopt(SOL_SOCKET, SO_RCVTIMEO / SO_SNDTIMEO)`

- **作用**：给阻塞 I/O 加超时。
- **超时表现**：返回 -1、`errno=EAGAIN`（**不是** 0，也不是 `ETIMEDOUT`）。
- **常见误用**：把 `-1` 一律当断连，于是超时被误报成掉线。

### `setsockopt(SOL_SOCKET, SO_LINGER)`

- **作用**：`{l_onoff=1, l_linger=0}` 让 `close` 直接发 RST 而不是 FIN。
- **用途**：故障注入、需要立刻通知对端异常时。
- **副作用**：对端收到 `ECONNRESET`，而不是读到 0；会丢弃未发送数据。

### `recv(sockfd, buf, len, MSG_TRUNC)`（数据报）

- **作用**：返回**数据报的真实长度**，即使缓冲区装不下。
- **普通 `recv` 的行为**：静默截断，返回缓冲区大小，剩余部分丢弃，无任何提示。

### `epoll_create1` / `epoll_ctl` / `epoll_wait`

- **`epoll_ctl(ADD/MOD/DEL)`**：注册/修改/删除关注的事件；`data.u32` 可存自定义整数。
- **`epoll_wait`**：返回就绪事件个数，`timeout` 毫秒；返回 0 表示超时。
- **`EPOLLET`**：边缘触发，必须配合非阻塞 fd 且循环读到 `EAGAIN`。
- **常见误用**：ET 下用阻塞 fd（最后一次 `recv` 会永远阻塞）；ET 下只读一次。

## 最小可运行实验

```text
SYS_03_Socket/
├── net_common.h   日志输出、非阻塞开关、回环监听/连接、write_all、recv_retry
├── tcp_basic.c    A 组：echo / halfclose / reset / sigpipe / timeout / shortwrite
├── udp_basic.c    B 组：boundary / trunc / toobig
├── mux.c          C 组：select / poll / epoll / epoll-et / epoll-et-broken
├── Makefile       三个可执行文件，头文件参与依赖
├── judge.py       25 条断言，只读日志
└── run.sh         三组取证 + 判定
```

## 编译与运行

```bash
cd experiments/SYS_03_Socket
./run.sh                                # 三组全跑 + 判定，全通过时退出码 0
./run.sh tcp                            # 只跑一组（tcp|udp|mux）
./run.sh --judge tcp logs/tcp.log        # 只判定不跑（0/1/2）
make clean
```

## 结果观察

`logs/tcp.log`（原文）：

```text
SYS03|tcp|echo|sent=3000|first_read=3000|boundary=merged|framed=3|client_frames=3|eof=1
SYS03|tcp|halfclose|got_ping=1|eof_seen=1|sent_after_eof=1|client_got=PONG|read_after_shutdown=1
SYS03|tcp|reset|recv_rc=-1|errno=104|detected=1
SYS03|tcp|sigpipe|default_would_kill=1|ignore_writes=1|ignore_errno=32|handler_writes=1|handler_errno=32|handler_hits=1
SYS03|tcp|timeout|rcvtimeo_ms=200|recv_rc=-1|errno=11|elapsed_ms=202|timed_out=1
SYS03|tcp|shortwrite|payload=1048576|sent=1048576|recv=1048576|short_writes=17|mismatch=0|ok=1
```

`logs/udp.log`：

```text
SYS03|udp|boundary|sent_msgs=3|lens=1000,1000,1000|recv_msgs=3|recv_lens=1000,1000,1000|preserved=1
SYS03|udp|trunc|buf=1000|recv_rc=1000|real_len=2000|truncated=1
SYS03|udp|toobig|payload=70000|send_rc=-1|errno=90|detected=1
```

`logs/mux.log`：

```text
SYS03|mux|mode=select|clients=8|accepted=8|echoed=8|done=8|mismatch=0|events=16|elapsed_ms=0|stalled=0|ok=1
SYS03|mux|mode=poll|clients=8|accepted=8|echoed=8|done=8|mismatch=0|events=16|elapsed_ms=0|stalled=0|ok=1
SYS03|mux|mode=epoll|clients=8|accepted=8|echoed=8|done=8|mismatch=0|events=16|elapsed_ms=0|stalled=0|ok=1
SYS03|mux|mode=epoll-et|clients=8|accepted=8|echoed=8|done=8|mismatch=0|events=16|elapsed_ms=0|stalled=0|ok=1
SYS03|mux|mode=epoll-et-broken|clients=8|accepted=8|echoed=0|done=0|mismatch=0|events=8|elapsed_ms=2003|stalled=1|ok=0
```

`events=16` = 8 条服务端连接 + 8 条客户端连接各就绪一次。
`epoll-et-broken` 的 `events=8` 说明它只收到了服务端侧那 8 个事件，
之后再也没有新事件——这正是"没读到 `EAGAIN` 就丢事件"的读数形态。

**跨运行漂移**：上面是 `logs/` 里当前这次运行的原文。重跑一次，下面两个数会变：

| 读数 | 第 1 次 | 第 2 次（当前 `logs/`） | 判据怎么写的 |
| --- | --- | --- | --- |
| `tcp/timeout elapsed_ms` | 203 | 202 | 只判 `≥100` 且 `≤2000` |
| `mux/epoll-et-broken elapsed_ms` | 2002 | 2003 | **不判数值**，只判 `stalled=1 且 done<clients` |

其余读数（`first_read`、`framed`、`short_writes`、各 `errno`、`done`、`events`）两次完全一致，
所以可以写死。`elapsed_ms` 依赖调度，写死必然假失败。

## 实测记录

### 判定器 25 条断言与 16 次验证

对实测日志的临时副本做变异（不重跑程序）：

| 变异 | 期望 | 实际 |
| --- | --- | --- |
| 三组原日志 | 0 | 0（3/3） |
| `echo` 的 `boundary` 改成 `split` | 1 | 1 |
| `halfclose` 的 `eof_seen` 改成 0 | 1 | 1 |
| `reset` 的 `errno` 改成 111 | 1 | 1 |
| `sigpipe` 的 `handler_hits` 改成 0 | 1 | 1 |
| `timeout` 的 `errno` 改成 110 | 1 | 1 |
| `shortwrite` 的 `short_writes` 改成 0 | 1 | 1 |
| `udp boundary` 的 `preserved` 改成 0 | 1 | 1 |
| `udp trunc` 的 `real_len` 改成 1000 | 1 | 1 |
| `udp toobig` 的 `errno` 改成 1 | 1 | 1 |
| `mux select` 的 `done` 改成 7 | 1 | 1 |
| `epoll-et-broken` 改成 `done=8 during…`（不再停住） | 1 | 1 |
| 指向不存在的日志 | 2 | 2 |
| 未知用例名 | 2 | 2 |

### 开发中被实测推翻的一处设计

**首版 `sigpipe` 用例照搬了 `reset` 用例的 `SO_LINGER{on,0}` 写法**，结果：

```text
SYS03|tcp|sigpipe|default_would_kill=1|ignore_errno=104|handler_errno=104|handler_hits=0
```

三次运行都是 `ECONNRESET(104)`、信号处理函数一次没被调用。
翻 `man 7 tcp` 与 `send(2)` 的错误表后确认：`EPIPE` 的前提是"对端已关闭连接"
（收到 FIN 且本地继续写），而 RST 会让 socket 立刻进入错误态，后续操作拿到的是 `ECONNRESET`。
改成正常 `close()`（发 FIN）之后，第一次写仍然成功、第二次得到 `EPIPE(32)` 且信号被计数：

```text
SYS03|tcp|sigpipe|default_would_kill=1|ignore_writes=1|ignore_errno=32|handler_writes=1|handler_errno=32|handler_hits=1
```

**这条差异被保留在代码注释与本文档里，而不是把首版读数删掉。**
两个用例（`reset` 与 `sigpipe`）也因此不能合并——它们测的是两种不同的关闭路径。

## 常见错误和陷阱

1. **把 TCP 当消息队列用**：以为一次 `write` 对应一次 `read`，于是把两个 JSON 拼在一起解析。
2. **`if (write(...) != len) 报错`**：短写被当成错误，压力一大就误报。
3. **把 `recv` 的 `-1` 一律当断连**：`EAGAIN`（超时/非阻塞）、`EINTR` 都不是断连。
4. **把读到 0 当成"不能写"**：0 只表示对端关了写方向，半关闭场景下本端仍可写。
5. **ET 模式配阻塞 fd**：最后一次 `recv` 会永远阻塞，进程挂死。
6. **ET 模式只读一次**：剩余数据不再触发事件（本实验已证伪）。
7. **`accept` 后不设非阻塞**：一条慢连接会拖住整个循环。
8. **监听 fd 与连接 fd 混用**：`accept` 返回新 fd，原 fd 继续监听。
9. **UDP 用"关掉对端就收到 EOF"收尾**：数据报没有 EOF，必须应用层约定。
10. **UDP 接收缓冲小于数据报**：静默截断，`recv` 返回值看起来一切正常。
11. **`SIGPIPE` 默认动作终止进程**：服务端不装处理器也不 `MSG_NOSIGNAL` 就会莫名退出。
12. **`select` 后用 `revents` 之前忘了清零**：`poll` 的 `revents` 是内核写回的，输入时不用清；`select` 的集合每次都要重填。

## 故障排查方法

| 现象 | 先查什么 | 判据 |
| --- | --- | --- |
| 数据看起来"少了一截" | `write` 返回值是否等于请求长度 | 循环之后 `sent == 收到字节数` |
| 进程莫名退出、无 traceback | 是否收到 `SIGPIPE` | `errno==EPIPE` 或 `handler_hits>0` |
| 连接卡住不返回 | 是否有超时；是 `EAGAIN` 还是真断连 | `elapsed_ms` 与 `errno` |
| ET 模式下"部分连接没反应" | 是否循环读到 `EAGAIN` | 事件数停在某一轮不再增长 |
| UDP 收到"残缺"数据 | 接收缓冲是否足够 | `MSG_TRUNC` 的真实长度 |
| UDP 发送报 `EMSGSIZE` | 载荷是否 > 65507 | `errno==90` |
| `select` 返回但没事件 | 集合是否每次重填 | 逐 fd 打印 `FD_ISSET` 结果 |
| 服务端偶发 `ECONNRESET` | 客户端是否 `SO_LINGER{on,0}` 关闭 | 客户端关闭路径 |

## 练习题

### 基础题

1. 把 `tcp_basic` 的 `echo` 用例改成用 `'\n'` 作分隔符分帧，说明它在"消息体本身含换行"
   时会出现什么问题，并给出长度前缀方案的对比结论。
2. 把 `mux` 的 `READ_CHUNK` 从 16 改成 4096，预测 `epoll-et-broken` 会不会还停住，
   先写预测再运行验证，并解释为什么答案取决于 `MSG_BYTES`。

### 改错题

3. 下面这段"回显服务"有什么问题？

   ```c
   int fd = accept(listen_fd, NULL, NULL);
   char buf[1024];
   ssize_t n = read(fd, buf, sizeof buf);
   write(fd, buf, n);
   close(fd);
   ```
   指出至少三处（提示：短读、短写、部分连接关闭）。

4. 下面这段 ET 注册有什么问题？

   ```c
   struct epoll_event ev = { .events = EPOLLIN | EPOLLET, .data.fd = conn };
   epoll_ctl(epfd, EPOLL_CTL_ADD, conn, &ev);
   /* 之后在事件循环里 */
   char buf[1024];
   recv(conn, buf, sizeof buf, 0);
   ```
   说明在什么输入下它会丢数据，并给出最小修复。

### 综合题

5. 给 `mux` 增加"客户端分两批发送"的能力（第一批 50 字节，第二批在首轮事件之后再发 50 字节），
   让正确实现需要多轮事件才能完成，并分析 `epoll-et-broken` 在这个改动下是否仍会停住。
6. 把 `tcp_basic` 改造成真正的服务端/客户端两个进程（用 `fork` 或 `sh -c` 启动），
   并说明这会让判据变成"跨进程时序"后，哪些断言必须放宽、哪些可以收紧。

## 验收标准

- [ ] 能跑 `./run.sh` 得到 25/25 通过，并复述 TCP 与 UDP 的边界差别。
- [ ] 能解释 `EPIPE`、`ECONNRESET`、`EAGAIN`、`EMSGSIZE` 各自的触发条件。
- [ ] 能画出半关闭的四个报文/调用顺序，并说明为什么读到 0 之后还能写。
- [ ] 能写出正确的短写循环，并说明 `EINTR` 与 `EAGAIN` 的处理差异。
- [ ] 能说清 LT 与 ET 的差别，并解释 `epoll-et-broken` 为什么停住。
- [ ] 能解释为什么本实验不能用来证明"epoll 比 select 快"。
- [ ] 能对自己的日志做一次变异，让 `judge.py` 按预期失败。

## 延伸阅读

- `man 2 socket` / `bind` / `listen` / `accept` / `connect` / `send` / `recv` / `shutdown`。
- `man 7 tcp`（状态机、`EPIPE`/`ECONNRESET` 的触发条件）、`man 7 udp`、`man 7 socket`。
- `man 2 epoll_create1` / `epoll_ctl` / `epoll_wait`；`man 7 epoll` 的 "Level-triggered and
  edge-triggered" 与 "Questions and answers" 两节。
- `man 2 select_tut`（含 `EINTR` 处理样例）、`man 2 poll`。
- Stevens《UNIX 网络编程 卷1》第 5–6 章（字节流、粘包与分帧）。
- 后续实验：[`SYS_04_Reactor`](../SYS_04_Reactor/README.md)（把本组的事件循环抽象成 Reactor）。

## 已知限制

- **`mux` 的 8 条连接都在第一轮事件里就完成了**：所有数据在 `accept` 之前已到达接收缓冲，
  所以 `elapsed_ms=0`。本实验证明的是"单线程能同时服务 8 条连接"，
  不是"长时间运行下的调度公平性或惊群问题"。
- **没有压测**：吞吐、延迟分位、连接数上界全部未测，`benchmarks/` 仍为空。
- **没有真实网络**：全部走回环，不涉及 MTU、分片、丢包、乱序、延迟抖动。
- **未覆盖**：`O_NONBLOCK` 的 `connect`+`EINPROGRESS`、`SO_REUSEPORT`、`TCP_NODELAY`、
  `MSG_PEEK`、`TCP_CORK`、`EPOLLONESHOT`、`accept4`、`SIGIO` 异步 I/O。
- **`select` 的 `FD_SETSIZE` 上界没有测**：8 条连接远低于 1024，换成上千条才能观察到。
- **未与 `strace` / `tcpdump` 对照**：本环境没有安装，只能靠程序自身的读数与 `errno`。
- **`udp_basic` 没测乱序、重复、丢包**：回环上这些几乎不会发生，需要 `tc netem` 才能构造。
