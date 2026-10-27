# Linux IPC 与线程同步：速查表

> 对应 `CLAUDE.md` §3 阶段 2。实验与判定器在 [`../experiments/SYS_02_IPC_Threads/`](../experiments/SYS_02_IPC_Threads/README.md)。
> 读数来自 WSL2 内核 6.6.87.2 / gcc 15.2.0，`./run.sh` **26/26 断言通过**。

## 1. 先选通道，再选同步

| 需求 | 通道 | 边界语义 | 关闭信号 |
| --- | --- | --- | --- |
| 亲缘进程单向传字节 | `pipe` | **没有**消息边界，多次写可合并成一次读 | 关掉所有写端 → `read` 得 0 |
| 无关进程/有名通道 | `mkfifo` | 同上 | 同上 |
| 本机请求/应答、要传 fd | `socketpair` / `AF_UNIX` `SOCK_STREAM` | 同上 | 同上 |
| 本机报文 | `AF_UNIX` `SOCK_DGRAM` | **保留边界**（一次收一条） | **没有 EOF**，只能靠应用层哨兵 |
| 大块共享、避免拷贝 | `shm_open` + `mmap` | 无边界，需自己加长度/序号 | 无，靠协议与引用计数 |

一句话：**收尾规则跟着通道语义走，不跟着数据长度走。**

## 2. 条件变量的唯一正确形状

```c
pthread_mutex_lock(&mu);
while (!cond) {                       /* 必须 while，不是 if */
    pthread_cond_wait(&cv, &mu);      /* 返回即持锁，且条件可能仍不成立 */
}
/* 消费受保护的状态 */
pthread_mutex_unlock(&mu);
```

两个容易混为一谈的概念：

- **虚假唤醒**：`pthread_cond_wait` 可能在没有 `signal` 时返回。实测：用 `if` 的版本
  `flag_at_exit=0`、`safe=0`；用 `while` 的版本 `flag_at_exit=1`。**只有 `while` 是对的。**
- **醒来时条件不成立**：别的消费者先拿到了。实测有界队列 `stale_wake=19`——这**不是**虚假唤醒，
  但它的存在让 `if` 同样错误。两者用同一个 `while` 一起解决。

`signal` vs `broadcast`：一次 `signal` 只唤醒一个，只有当"任一等待者都能消费掉这次变化"时才够；
`broadcast` 用于"条件对所有人都变了"（如 `shutdown` 标志置位）。

## 3. 四种互斥手段的代价

| 手段 | 保证 | 代价 | 实测 |
| --- | --- | --- | --- |
| 不加保护 | 无 | 计数丢失 | `plain`：`final=22131`（应 80000，**丢 57869**），TSan 报 2 次竞争 |
| `pthread_mutex` | 互斥 + 内存序 | 每次加解锁 | `final=80000`，零 TSan 报告 |
| C11 `atomic` | 单变量原子 RMW | 只能保护一个量，组合操作仍需锁 | `final=80000`，零报告 |
| `pthread_rwlock` | 多读单写 | 写者可能饿死 | `final=80000`，零报告 |

**"没丢计数"不能当"没有竞争"的证据**——`plain` 版本在本机连跑都不一定每次都丢；判据要同时看
计数与 sanitizer 报告。

## 4. 死锁：可观测，不一定可复现

用 `pthread_mutex_timedlock` 把"永远等下去"变成"`ETIMEDOUT` 可观测"：

| 用例 | 读数 |
| --- | --- |
| 交叉加锁（A→B / B→A） | `t1_rc = t2_rc = 110`（`ETIMEDOUT`） |
| 统一加锁顺序 | 双双 `rc=0` |

结论：**死锁要靠"锁顺序约定"预防，`timedlock` 只用于取证**，不能当成修复手段（它把死锁换成了周期性失败）。

## 5. 线程池关闭协议

两种语义都要能测：

| 关闭方式 | 行为 | 实测 |
| --- | --- | --- |
| graceful | 不再接新任务，**执行完队列里剩下的**再退出 | `executed=200` |
| immediate | 置停止标志，队列剩余**丢弃**，唤醒所有 worker | `executed=1` |

两者都必须 `joined=4/4`、`fd_leak=0`——**能停下来**比"停得优雅"更优先。

## 6. 常见错误与陷阱

| 现象 | 根因 | 排查 |
| --- | --- | --- |
| 等 EOF 的收端永远不返回（`rc=124`） | `SOCK_DGRAM` 没有 EOF 语义，对端 `close` 不会让 `read` 得 0 | 用应用层哨兵；本实验首版正是这么挂住的 |
| 用 `if (!cond) wait` 偶发失败 | 虚假唤醒 / 先被同侪拿走 | 改 `while` |
| 线程池 join 卡住 | worker 在 `cond_wait` 上没人 `broadcast` | 关闭路径必须 `broadcast`，且停止标志要在锁内改 |
| 队列"唤醒次数"被当成 bug | 一次 `signal` 可能唤醒到"货已被取走"的线程 | 看 `stale_wake`，配合 `while` 就是正常现象 |
| 有界队列满后生产者死等 | 生产者/消费者共用一把锁却只 `signal` 一个方向 | 满/空各用一个条件变量 |

## 7. 调试方法

1. **`-fsanitize=thread -g`**：竞争的第一手证据；`plain` 组 2 次报告是"确实存在"的最小证明。
2. **`/proc/<pid>/task/*/wchan`**：看每个线程卡在哪个内核函数（本系列定位 accept 挂死就靠它）。
3. **加"计数器 + 期望值"**：`final` 与 `expect` 必须同时打印，否则"差多少"永远说不清。
4. **`pthread_mutex_timedlock` 取证**：只在排查时用，产物是 `ETIMEDOUT` 时间戳而非"修好了"。

## 8. 当前验证状态

- **实测**：五种通道的边界/EOF 语义、`if` vs `while`、有界队列、四种互斥手段、`timedlock` 死锁取证、
  线程池两种关闭协议、TSan 触发。
- **未验证**：线程池背压（`max_depth=200` 由一次性批量提交造成，真实持续压测未做）、
  三把以上锁的死锁、递归锁、性能与吞吐（本组不产出性能结论）。

## 证据来源

- 实验：[`experiments/SYS_02_IPC_Threads/`](../experiments/SYS_02_IPC_Threads/README.md)
- 日志：[`daily_logs/2026-10-27_第24天_日志.md`](../daily_logs/2026-10-27_第24天_日志.md)
