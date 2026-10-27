# Socket 与 I/O 多路复用：速查表

> 对应 `CLAUDE.md` §3 阶段 3。实验与判定器在 [`../experiments/SYS_03_Socket/`](../experiments/SYS_03_Socket/README.md)。
> 14 个用例 `rc=0`、判定器 **25/25**（tcp 16 / udp 4 / mux 5），环境同 WSL2 内核 6.6.87.2。

## 1. 一句话定义

TCP 给你一条**字节流**，UDP 给你**若干条独立报文**；多路复用只告诉你"这个 fd 现在可读/可写"，
不告诉你"能读多少"——两个事实加起来，决定了所有网络代码的形状。

## 2. 语义对照（全部实测）

| 维度 | `SOCK_STREAM`（TCP） | `SOCK_DGRAM`（UDP） |
| --- | --- | --- |
| 消息边界 | **无**：三次 1000 B 写被一次 `recv` 收成 `first_read=3000` | **有**：三条各出 1000，`preserved=1` |
| 收发数量关系 | 1 次写 ≠ 1 次读 | 1 次写 = 1 次读（或整条丢弃） |
| 长度不够的缓冲 | 多余字节留在内核，下次继续 | 多余部分**丢弃**；`recv_rc=1000` / `real_len=2000`，只有 `MSG_TRUNC` 才看得到真实长度 |
| 超长报文 | 拆成多段 | `70000 B` → `EMSGSIZE(90)` |
| 半关闭 | `shutdown(SHUT_WR)` 后可继续读：`eof_seen=1 sent_after_eof=1 read_after_shutdown=1` | 不适用 |
| "对端没了" | 见 §3 两种相反情形 | 无连接，收不到任何通知 |
| 分帧 | **自己做**长度前缀：`framed=3` | 天然分帧，仍需自校验 |

## 3. 四个 errno 的触发条件（本实验全部实测）

| 情形 | errno | 读数 |
| --- | --- | --- |
| 读超时（`SO_RCVTIMEO=200ms`） | `EAGAIN(11)` | `elapsed_ms=203` |
| 对端 RST 之后继续读写 | `ECONNRESET(104)` | RST 用例 |
| 收到 FIN 之后继续写 | `EPIPE(32)` + `SIGPIPE` | `handler_hits=1` |
| 非阻塞读无数据 | `EAGAIN(11)` | ET 用例 |

**最容易记反的一条**：RST 让 socket 直接进错误态，后续操作拿 `ECONNRESET`；
只有**收到过 FIN 再写**才走 `EPIPE` + `SIGPIPE`。首版用 RST 测 `SIGPIPE`，信号一次没来
（`handler_hits=0`、`errno=104`）；换成正常 `close()` 立刻稳定复现。

## 4. 短写与短读

1 MiB 一次 `write` 的结果：`short_writes=17`，循环补写后 `sent=recv=1048576`、`mismatch=0`。

规则：**写必须循环**，且用返回值推进指针；读同理，除非你确定协议长度。

## 5. `select` / `poll` / `epoll`

| 接口 | 复杂度 | 适合 | 陷阱 |
| --- | --- | --- | --- |
| `select` | O(n)，每次重填 `fd_set` | 少量 fd、跨平台 | `FD_SETSIZE` 上限（默认 1024）；`nfds` 要设为最大 fd+1 |
| `poll` | O(n) | 大量 fd、事件结构清晰 | 返回后仍要遍历全部 |
| `epoll` | O(1) 就绪回调 + 一次拷贝 | 高并发、长连接 | LT/ET 语义差异；ET 必须非阻塞并读到 `EAGAIN` |

### ET 为什么必须读到 `EAGAIN`

实测：只读一次（每轮 128 B）的破坏版 → `done=0/8`、`stalled=1`、`elapsed_ms=2002`（超时退出）；
四种正确后端（`select`/`poll`/`epoll-LT`/`epoll-ET`）均 `done=8/8 events=16`。

原因：ET 只在**状态变化**时通知。缓冲区里还剩数据时不再有新边沿，不读完就再也不会被唤醒。

## 6. 常见错误与陷阱

| 现象 | 根因 | 排查 |
| --- | --- | --- |
| 收端根本收不到 `SIGPIPE` | 对端是 RST 断的，不是 FIN | 看 `errno` 是 104 还是 32 |
| 分批发送后"粘包" | TCP 没有消息边界 | 加长度前缀（本实验 `framed=3`） |
| UDP 收到的长度比预期短 | 缓冲太小，多余部分**静默丢弃** | 用 `MSG_TRUNC` 取真实长度 |
| ET 下偶发"卡住不动" | 没读到 `EAGAIN` | 循环读，且 fd 必须 `O_NONBLOCK` |
| `select` 偶发返回但没数据 | `fd_set` 没在每次调用前重置 | 每次重新 `FD_ZERO`+`FD_SET` |
| 一次 `write` 以为写完了 | 短写 | 检查返回值是否等于 `len` |

## 7. 调试方法

1. **`ss -tanp`**：看 `ESTABLISHED`/`CLOSE_WAIT`/`TIME_WAIT` 分布——`CLOSE_WAIT` 堆积 = 自己没关。
2. **`/proc/<pid>/wchan`**：区分"卡在 `accept`"和"卡在 `epoll_wait`"。
3. **打印返回值与 `errno`**：本表每个 errno 都是打印出来的，不是推断的。
4. **构造必然失效的破坏版**：只读一次的 ET 客户端一定停在 `done=0/8`，这种"必然"比"偶发失败"可信。

## 8. 当前验证状态

- **实测**：TCP/UDP 语义、长度前缀分帧、半关闭、`EPIPE`/`ECONNRESET`/`EAGAIN`/`EMSGSIZE`、
  短写、`select`/`poll`/`epoll`、ET 必须读到 `EAGAIN`。
- **未验证**：吞吐/压测与 `FD_SETSIZE` 上界、真实网络（MTU/丢包/乱序）、
  `EPOLLONESHOT`（在 [SYS_04](../experiments/SYS_04_Reactor/README.md) 覆盖）、`accept4`、非阻塞 `connect`、
  `strace`/`tcpdump` 逐包对照。
- `mux` 组的 `elapsed_ms=0` 只说明数据在 `accept` 前已到达，**不证明**长跑调度公平性。

## 证据来源

- 实验：[`experiments/SYS_03_Socket/`](../experiments/SYS_03_Socket/README.md)
- 日志：[`daily_logs/2026-10-27_第25天_日志.md`](../daily_logs/2026-10-27_第25天_日志.md)
