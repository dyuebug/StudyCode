# Linux 文件 I/O 与进程生命周期：速查表

> 对应 `CLAUDE.md` §3 阶段 1。实验与判定器在 [`../experiments/SYS_01_FileIO_Proc/`](../experiments/SYS_01_FileIO_Proc/README.md)。
> 本表结论分两档：**实测**（WSL2 内核 6.6.87.2、gcc 15.2.0、判定器 19/19）与**语义**（POSIX/Linux 规定，
> 本实验未逐条复现）。没有任何一条来自"听说"。

## 1. 一句话定义

文件描述符是**进程内的一个整数索引**，指向内核里的"打开文件描述"；`fork` 复制描述符，
**不复制打开文件描述**——所以父子进程共享偏移量，也共享 `O_APPEND` 这类状态标志。

## 2. 什么时候用哪个

| 需求 | 选择 | 代价 |
| --- | --- | --- |
| 多进程并发追加同一文件 | `O_APPEND`（内核保证"定位到末尾 + 写"是一个原子步骤） | 无法指定写入位置 |
| 需要自己定位再写 | `lseek` + `write` | **非原子**，多进程下会互相覆盖（本表 §4 有实测丢行） |
| 子进程不应继承 | `O_CLOEXEC`（在 `open` 时给，而不是 `open` 后 `fcntl`） | `fcntl` 后设存在 `fork`+`exec` 竞态窗口 |
| 读取长度已知 | 一次 `read` | 只有常规文件能对齐，管道/终端/socket 都不保证 |
| 读取长度未知 | 循环读到 `0`（EOF）或 `EAGAIN` | 需要自己拼缓冲 |

## 3. 接口卡片

### `open(path, flags, mode)` / `close(fd)`

返回：`>=0` 描述符，`-1` 且 `errno` 有效（`ENOENT`/`EACCES`/`EMFILE`/`ENFILE`）。

所有权：返回的 fd 由调用者负责 `close`；`fork` 后父子各持一份**引用**，两边都 `close` 才真正释放。

最小示例与常见误用见 [SYS_01 接口卡片](../experiments/SYS_01_FileIO_Proc/README.md#接口卡片)。

### `read(fd, buf, count)` / `write(fd, buf, count)`

返回：实际字节数（**可以小于 `count`**）、`0`（仅 `read` 表示 EOF）、`-1` 且 `errno` 有效。

三条必须记住的规则：

1. **短读是常态**，不是错误。管道、终端、socket、信号打断都会让它提前返回。
2. `EINTR` 表示"被信号打断"，`errno` 不清零就会误判成"其他错误"——重试前先判 `errno == EINTR`。
3. `EAGAIN`（非阻塞）表示"现在没有数据"，必须与 `EINTR`、EOF 分开处理，混为一谈就是丢数据。

### `fork()` / `exec*()` / `waitpid(pid, &status, options)`

`fork` 返回：子进程 0、父进程子 PID、`-1` 失败。`exec` 成功不返回；失败返回 `-1`（此时原进程继续跑，
必须 `_exit`，不能让子进程回到不该回的地方）。

`waitpid` 的 `status` **必须**用宏解码，不能直接当退出码：

```c
if (WIFEXITED(status))        code = WEXITSTATUS(status);      /* 正常退出 */
else if (WIFSIGNALED(status)) sig  = WTERMSIG(status);         /* 被信号杀死 */
else if (WIFSTOPPED(status))  stop = WSTOPSIG(status);         /* waitpid 带 WUNTRACED */
```

### `prctl(PR_SET_CHILD_SUBREAPER, 1, 0, 0, 0)`

作用：让当前进程接管"孤儿"孙进程，而不是交给 `init`。

为什么需要：容器/守护进程里 `init` 不一定是 PID 1，孤儿会被别的 subreaper 收走，
"孤儿最终归 1 号进程"这个教科书结论在本机不成立（实测 `ppid = 369/497`，见 §4）。

## 4. 实测读数（判定器 19/19）

| 用例 | 读数 | 说明 |
| --- | --- | --- |
| `O_APPEND` 五进程各写 320 行 | 5/5 次 `good=1600/1600` | 原子追加，零丢行 |
| `lseek + write`（注入 `sched_yield` 放大窗口） | 4/4 次丢行，`good=1414/1426/1067/1567` | `bytes=12726 = 1414×9` 证明是**整行丢失**而非撕裂行 |
| `lseek`（不注入） | 5 次里 1 次丢行（1547） | **只记录不断言**：竞态窗口不稳定，断成"必然失败"就是假判据 |
| 孤儿进程 `ppid` | 369 / 497 | **不是 1**：WSL 自带 subreaper |
| 设 `PR_SET_CHILD_SUBREAPER` 后 | `ppid` = 祖父 PID 500 | 收养关系可观测 |

判定器自检：8 条变异 `rc=1`、缺日志 `rc=2`、原日志 `rc=0`。

## 5. 常见错误与陷阱

| 现象 | 根因 | 排查 |
| --- | --- | --- |
| 多进程追加出现丢行 | 用了 `lseek`+`write` 而不是 `O_APPEND` | `lseek` 是独立一步，中间可被抢占 |
| 两组读数完全相同、都"没丢" | 子进程共用 `fork` 前的 fd（共享打开文件描述） | 让每个进程**独立** `open`；否则测的不是同一件事 |
| `read` 返回 0 被当成错误 | 混淆"EOF"与"失败" | 只有 `-1` 才是错误 |
| 退出码永远是 0 | 直接把 `status` 当码用 | 先 `WIFEXITED` |
| 子进程卡在 `exec` 失败的路径继续跑 | `exec` 后没写 `_exit` | `exec` 失败分支必须以 `_exit(127)` 收尾 |
| 孤儿 `ppid` 既不是 1 也不是预期值 | 中间有 subreaper（容器、`systemd --user`、WSL 的 init） | 读 `/proc/<pid>/status` 的 `PPid`，并显式设 subreaper |

## 6. 调试方法

1. **`/proc/<pid>/fd/`**：看某个进程到底开着哪些 fd、指向哪个文件（软链接目标就是真相）。
2. **`/proc/<pid>/status` 的 `PPid`**：确认父子关系与收养结果，比 `ps` 更直接。
3. **`/proc/self/fdinfo/<n>`**：看 `pos`（当前偏移）与 `flags`——判断"两个进程是不是共享同一个打开文件描述"。
4. **`strace -f -e trace=openat,read,write,close`**：确认调用序列与返回值（本机无 `strace` 时，
   替代实现见 [SYS_06](../experiments/SYS_06_Debug_Perf/README.md)）。
5. **计数对账**：`bytes` 与"行数 × 行长"必须能互相验证，否则"丢了几行"只是一个猜不出的数。

## 7. 当前验证状态

- **实测**：`O_APPEND` 原子性、`lseek` 竞态丢行、`O_CLOEXEC` 继承、短读、`EINTR` 重试、
  僵尸/孤儿/`subreaper`、`wait` 状态解码、简化 Shell。
- **未验证**：`strace` 级系统调用序列（本机缺 `strace`）、NFS/overlayfs 上的 `O_APPEND` 语义、
  写入长度不一时的**撕裂行**（本实验所有写入等长）、`io_uring` 路径。

## 证据来源

- 实验与判定器：[`experiments/SYS_01_FileIO_Proc/`](../experiments/SYS_01_FileIO_Proc/README.md)
- 日志：[`daily_logs/2026-10-27_第23天_日志.md`](../daily_logs/2026-10-27_第23天_日志.md)
- 一句话结论：[`experiments/README.md`](../experiments/README.md) 系统编程表首行
