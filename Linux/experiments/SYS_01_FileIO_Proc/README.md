# SYS_01：文件 I/O、文件描述符与进程生命周期

> 结论先行，五条都由读数支撑而不是引用教科书：
>
> 1. **`O_APPEND` 的原子性是能测出来的。** 8 进程 × 200 行定长记录，注入 `sched_yield` 放大竞态窗口后，
>    `O_APPEND` 组 5/5 次都是 `good=1600/1600`，`lseek + write` 组 4/4 次都丢行（1414 / 1426 / 1067 / 1567）。
> 2. **不改代码就测不出竞态，可能是设计错了。** 首版让子进程共用父进程打开的那**一个** fd，
>    偏移量因此共享、`lseek` 的返回值又不被 `write` 使用，于是两组都得到 1600 行全合法——
>    结论是错的，原因是实验设计错了。
> 3. **竞态是概率性的，不能当判据。** 不加 `sched_yield` 时多次运行只偶发一次丢行（观察值 1547），
>    其余都是 1600。判定器因此只断言注入模式，自然模式只记录不断言。
> 4. **丢行不等于行撕裂。** 本实验的记录是定长 9 字节，被覆盖时整行替换整行，
>    所以 `bad` 恒为 0——「`bad=0`」必须与「`good=1600`」一起看才有意义。
> 5. **僵尸与孤儿的判据要用两个读数配一对。** 僵尸：`/proc/<pid>/stat` 先读到 `Z`，
>    `wait` 之后再读必须打不开；孤儿：不设 `PR_SET_CHILD_SUBREAPER` 时孙进程的 `ppid`
>    **不等于**本进程 pid（两次运行实测是 369 与 497，都不是 1——WSL 里有别的 subreaper），
>    设了之后必须等于。

## 学习目标

1. 能解释 `open` / `read` / `write` / `close` 的返回值和短读短写，并用循环把短读处理干净。
2. 能说清 `O_APPEND` 到底保证了什么，以及为什么 `lseek` + `write` 在并发下会丢数据。
3. 能区分**文件描述符**与**打开的文件描述**（open file description）：`fork` 之后偏移量是共享还是独立，取决于在哪一步 `open`。
4. 能用 `/proc/<pid>/stat` 判断僵尸，用 `waitpid` 的 `WIFEXITED` / `WIFSIGNALED` 区分两种退出方式。
5. 能解释孤儿进程被谁收养，并用 `PR_SET_CHILD_SUBREAPER` 自己指定收养者。
6. 能写一个支持内建命令、重定向与单级管道的简化 Shell，并说清它与 bash 的差距在哪里。

## 环境与前置条件

| 项 | 值 |
| --- | --- |
| 平台 | WSL2 Ubuntu（`Linux 6.6.87.2-microsoft-standard-WSL2`，x86_64） |
| 编译器 | `gcc` 15.2.0，`-O2 -g -std=c11 -Wall -Wextra -Werror`，零警告 |
| 依赖 | 只需要 libc；`run.sh` 用 `bash` + `python3`（判定器） |
| 磁盘 | 工作文件写在 `/tmp`，不污染仓库 |
| 前置 | 无。这是阶段 1 的入口实验 |

**为什么用 WSL 而不是 QEMU**：本实验要观察的是内核的进程与文件语义，
宿主机的真实内核比仿真更可靠，也免去交叉编译。代价是结论绑定 WSL2 内核 6.6，
换内核要重跑一遍再把读数写回本文（`README` 末尾「环境绑定」）。

## 核心概念

### 1. 文件描述符不是文件

`fd` 是一个进程内的小整数，指向进程 fd 表里的一项，该项指向内核里的
**打开的文件描述**（open file description），后者才持有文件偏移量与打开标志。

| 操作 | fd 表项 | 打开的文件描述 | 偏移量 |
| --- | --- | --- | --- |
| `open()` 两次 | 两项 | 两个 | **各自独立** |
| `dup()` / `dup2()` | 两项 | 同一个 | **共享** |
| `fork()` | 子进程复制一份表 | 同一个 | **共享** |

这条差异直接决定了本实验首版为什么失败：想让"lseek 到末尾再写"产生竞态，
必须让每个进程拥有**独立**的偏移量，也就是各自 `open`。

### 2. `O_APPEND` 保证了什么

`O_APPEND` 让"定位到末尾"与"写入"成为内核里的一次原子动作。
没有它时，`lseek(fd, 0, SEEK_END)` 返回的偏移量与随后的 `write` 之间有一段用户态窗口，
别的进程可以在这段时间里也定位到同一处，于是两次写落在同一区间，先写的被后写的覆盖。

覆盖后的文件**看起来是合法的**：总字节数减少、某些行消失，但没有半个行。
这就是为什么判据必须看 `good < expected`，而不是看"有没有非法行"。

### 3. 短读是常态，不是错误

`read(fd, buf, 64)` 返回 16 是正常的：管道里有 16 字节可读，内核不会等凑够 64。

本实验的确定性读数来自 `pipe` 的内存缓冲：写端一次 `write` 1000 字节，
读端用 64 字节缓冲，由于管道缓冲区足够大、写端在读者开始前就写完了，
读端必然得到 15 次 64 字节 + 1 次 40 字节 = 16 次。这条读数可复现，因此可以入判据。

### 4. `EINTR`：被信号打断的系统调用

阻塞在空管道上的 `read` 收到信号时返回 `-1` 并置 `errno = EINTR`。
两种处理方式的后果完全不同：

```c
if (n < 0 && errno == EINTR) continue;   /* 重试：数据还在管道里 */
if (n < 0) return -1;                     /* 放弃：数据被"读走"了一次机会 */
```

`sigaction` 里**不加** `SA_RESTART` 才会走到这条路径；加了之后内核自动重启系统调用，
`EINTR` 分支永远不执行——这也是很多"我处理了 EINTR 但测不到"的原因。

### 5. 僵尸、孤儿与 `PR_SET_CHILD_SUBREAPER`

| 状态 | 判据 | 本实验读数 |
| --- | --- | --- |
| 僵尸 | 子进程已退出、父进程未 `wait`，`/proc/<pid>/stat` 第三字段为 `Z` | `state=Z` |
| 已回收 | `wait` 之后同一路径打不开 | `after_wait\|gone` |
| 孤儿（默认） | 中间层退出后，孙进程被**最近的 subreaper** 收养 | `ppid=369`（≠ 祖父 500） |
| 孤儿（本进程设 subreaper） | 孙进程被本进程直接收养，因此本进程能 `wait` 到它 | `ppid=500`（= 祖父 500） |

**不要假设孤儿的父进程是 1。** 实测 WSL 里是 369 / 497——那是 WSL 自己的 subreaper。
判据写成"是否等于祖父 pid"才稳，写成 `ppid == 1` 会在别的环境里假失败。

### 6. `wait` 状态必须区分两种形态

```c
WIFEXITED(st)   → WEXITSTATUS(st)   /* 正常退出，本实验是 42 */
WIFSIGNALED(st) → WTERMSIG(st)      /* 被信号终止，本实验是 15 */
```

把 `WIFSIGNALED` 的子进程按"退出码 143"处理是常见误判：143 是 shell 的约定，
不是 `wait` 状态的原生表示，直接用 `WEXITSTATUS` 读被信号终止的子进程会得到无意义的值。

## 接口卡片

### `int open(const char *path, int flags, mode_t mode)`

- **作用**：创建或打开文件，返回该进程 fd 表里最小的可用项。
- **返回值**：≥0 成功；-1 置 `errno`（`ENOENT` 路径不存在、`EACCES` 权限、`EMFILE` 进程 fd 用尽）。
- **资源所有权**：成功后由调用方负责 `close`。`exec` 时默认保留，加 `O_CLOEXEC` 才关闭。
- **常见误用**：`open` 后不检查返回值；用 `O_TRUNC` 却在失败分支忘记 `close`。

### `ssize_t read(int fd, void *buf, size_t count)`

- **返回值**：>0 实际读到的字节数（**可以小于 `count`**）；0 表示 EOF；-1 置 `errno`。
- **必须处理**：`EINTR`（重试）、`EAGAIN`（非阻塞且暂无数据）。
- **常见误用**：把 0 与 -1 混为一谈；只调一次就当读全了。

### `ssize_t write(int fd, const void *buf, size_t count)`

- **返回值**：>0 实际写入的字节数（**可以小于 `count`**，管道与 socket 上很常见）；-1 置 `errno`。
- **必须处理**：循环到写满，否则数据会静默丢失。

### `pid_t fork(void)`

- **返回值**：父进程得子进程 pid，子进程得 0，失败得 -1。
- **资源所有权**：子进程复制 fd 表但**不复制**打开的文件描述（同一份，偏移量共享）。
- **常见误用**：在子进程里 `return` 而不是 `_exit`，导致父进程的 stdio 缓冲被刷新两次。

### `pid_t waitpid(pid_t pid, int *status, int options)`

- **返回值**：>0 被回收的子进程 pid；-1 置 `errno`（`ECHILD` 没有可等待的子进程）。
- **必须配合**：`WIFEXITED` / `WEXITSTATUS` / `WIFSIGNALED` / `WTERMSIG` 解码。
- **必须处理**：`EINTR`（被信号打断时要重试，否则会漏掉回收）。

### `int prctl(PR_SET_CHILD_SUBREAPER, 1L, 0L, 0L, 0L)`

- **作用**：把本进程声明为"收养者"，此后本进程的所有后代里成为孤儿的进程都会挂到它名下。
- **返回值**：0 成功，-1 置 `errno`。
- **用途**：自己实现 init 式的进程管理，或者在测试里把"孤儿去哪了"变成可控变量。

## 最小可运行实验

```text
SYS_01_FileIO_Proc/
├── fileio.c         A 组：O_APPEND 原子性 / lseek 竞态 / O_CLOEXEC
├── proc_life.c      僵尸、孤儿与 subreaper、wait 状态解码
├── pipe_read.c      短读与 EINTR
├── minish.c         简化 Shell（内建命令、重定向、单级管道）
├── judge.py         判定器（只读日志）
├── run.sh           编译 → 四组取证 → 判定
└── scripts/minish_input.txt   Shell 的测试脚本
```

`fileio` 的三组模式：

| 模式 | 打开方式 | 写前 | 预期 |
| --- | --- | --- | --- |
| `O_APPEND+yield` | `O_APPEND`，各自 `open` | `sched_yield` | 1600 行全合法（原子） |
| `lseek+yield` | 无 `O_APPEND`，各自 `open` | `lseek(SEEK_END)` → `sched_yield` → `write` | 必然丢行 |
| `lseek+natural` | 同 `lseek+yield` | 无 `yield` | 概率丢行，**只记录不断言** |

`sched_yield` 是**故障注入**：它把"定位之后、写入之前"的窗口放大到必然被别的进程插进来。
两组都注入，所以对照仍然只差 `O_APPEND` 一个变量。这是把概率现象变成确定性判据的手段，
不是自然发生的事实——`lseek+natural` 组保留下来就是为了记录未经注入时的真实概率。

## 编译与运行

```bash
cd experiments/SYS_01_FileIO_Proc
./run.sh                              # 四组全跑 + 判定，全通过时退出码 0
./run.sh pipe                         # 只跑一组
./run.sh --judge pipe logs/pipe.log   # 只判定不跑（0 通过 / 1 失败 / 2 日志不存在）
make clean                            # 清掉二进制
```

## 结果观察

`logs/fileio.log`（2026-10-27 一次运行的原文）：

```text
SYS01|append|mode=O_APPEND+yield|workers=8|lines=200|expected=1600|good=1600|bad=0|bytes=14400|failed_children=0
SYS01|append|mode=lseek+yield|workers=8|lines=200|expected=1600|good=1414|bad=0|bytes=12726|failed_children=0
SYS01|append|mode=lseek+natural|workers=8|lines=200|expected=1600|good=1600|bad=0|bytes=14400|failed_children=0
SYS01|cloexec|parent|keep_fd=3|cloexec_fd=4
SYS01|cloexec|child|keep_alive=1|cloexec_alive=0|errno=9
SYS01|done
```

`errno=9` 是 `EBADF`：子进程对已被 `O_CLOEXEC` 关掉的 fd 调 `fcntl` 得到的错误。

`logs/proc.log`：

```text
SYS01|proc|start|pid=500|ppid=468
SYS01|zombie|before_wait|pid=501|state=Z
SYS01|zombie|reaped|exited=1|code=0
SYS01|zombie|after_wait|gone
SYS01|orphan|grand|subreaper=0|grand_pid=503|ppid=369|grandparent_pid=500
SYS01|orphan|no_subreaper|not_reaped_here
SYS01|orphan|grand|subreaper=1|grand_pid=505|ppid=500|grandparent_pid=500
SYS01|orphan|subreaper_reaped|child=505|ok
SYS01|wait|normal_exit|exited=1|code=42
SYS01|wait|signal_exit|signaled=1|sig=15
SYS01|proc|done
```

`subreaper=0` 那行的 `ppid=369` 与本进程 `pid=500` 不同；`subreaper=1` 那行两者相同（500），
所以本进程能 `wait` 到孙进程。

`logs/pipe.log`：

```text
SYS01|pipe|short|total=1000|reads=16|last_chunk=40|chunk_max=64
SYS01|pipe|eintr|first_read|ret=-1|errno=4|usr1_hits=1
SYS01|pipe|eintr|retry_read|ret=2|errno=0
```

`logs/shell.log`（取证流）与 `logs/minish.stdout`（命令输出）分开：

```text
SYS01|sh|start|pid=511
SYS01|sh|exec|cmd=echo|pid=512|status=0
SYS01|sh|pipe|left=echo|right=wc|st_left=0|st_right=0
SYS01|sh|builtin|pwd|path=/mnt/e/.../SYS_01_FileIO_Proc
SYS01|sh|builtin|cd|path=/
SYS01|sh|builtin|pwd|path=/
SYS01|sh|exec|cmd=echo|pid=515|status=0
SYS01|sh|exec|cmd=cat|pid=516|status=0
SYS01|sh|exec|cmd=echo|pid=517|status=0
SYS01|sh|exec|cmd=cat|pid=518|status=0
SYS01|sh|exec_fail|cmd=nonexistent-xyz|errno=2
SYS01|sh|exec|cmd=nonexistent-xyz|pid=519|status=127
SYS01|sh|builtin|exit|code=0
SYS01|sh|exit|code=0|cmds=11
```

`minish.stdout` 是五行：

```text
hello-minish      ← echo hello-minish
4                 ← echo abc | wc -c
one               ← cat /tmp/sys01_redir.txt（此时文件里只有一行）
one               ← cat < /tmp/sys01_redir.txt 的第一行
two               ← 同上第二行（>> 追加生效）
```

`one` 出现两次不是重复输出：第三次写入（`>`）覆盖成一行 `one`，第四次（`>>`）追加成两行。
把两者分开重定向是有意的：`pwd` 会打印路径，与取证行混在一起时判定器容易把命令输出当成证据。

## 实测记录

### 三组模式的可复现性

| 运行 | `O_APPEND+yield` | `lseek+yield` | `lseek+natural` |
| --- | --- | --- | --- |
| A | 1600 | **1426** | 1600 |
| B | 1600 | **1067** | 1600 |
| C | 1600 | **1567** | 1600 |
| D（即本目录 `logs/fileio.log`） | 1600 | **1414** | 1600 |
| 更早一次（注入代码之前） | 1600 | — | **1547** |

- `O_APPEND` 组 5/5 次完好；`lseek+yield` 组 4/4 次丢行（丢 33–533 行）。
- `lseek+natural` 5 次里只 1 次丢行（1547），丢行量也小。**这条数字不能当判据**。
- 生成的文件：`good=1414` 对应 `bytes=12726`，正好 `1414 × 9`——
  丢的是整行，文件里没有残片。这是下面「`bad=0` 不代表没坏」的直接证据。

### 首版的两处设计错误

**错误一：共用父进程的 fd。** 子进程用的是 `fork` 前 `open` 的那一个 fd，
偏移量在父子间共享；`lseek(fd, 0, SEEK_END)` 的返回值又没被使用，
`write` 用的是 fd 当前偏移量。结果是两组都得到 1600 行 —— 一个"实验证明教科书错了"的假象。
**修法**：让每个子进程各自 `open`，偏移量独立，竞态窗口才真实存在。

**错误二：把概率当判据。** 改完设计后发现损坏只是偶发，原判据 `bad > 0` 会随机失败。
**修法**：引入 `sched_yield` 做故障注入，把窗口放大到必现，并把"自然模式"降级为只记录。

### 判定器的负向验证

对实测日志的临时副本做变异（不重跑程序）：

| 变异 | 退出码 | 结论 |
| --- | --- | --- |
| 原日志 | 0 | 通过（复判同样为 0） |
| `O_APPEND` 组 `good` 改成 1599 | 1 | 失败（原子性被破坏必须被发现） |
| `lseek+yield` 组 `good` 改成 1600 | 1 | 失败（没丢行说明注入失效） |
| `cloexec_alive` 改成 1 | 1 | 失败（`O_CLOEXEC` 没生效） |
| 僵尸状态 `Z` 改成 `S` | 1 | 失败（把活进程/已回收当成僵尸） |
| `subreaper=1` 的 `ppid` 改成 1 | 1 | 失败（收养关系不对） |
| `signaled=1\|sig=15` 改成 `0\|-1` | 1 | 失败（信号终止被当成正常退出） |
| 短读 `reads=16` 改成 15 | 1 | 失败（短读次数不对） |
| stdout 里 `hello-minish` 换成别的 | 1 | 失败（命令输出缺失） |
| 指向不存在的日志 | 2 | 与"判定失败"区分开 |

## 常见错误和陷阱

1. **只调一次 `read` 就当读完**：短读是常态，必须循环到 0。
2. **`write` 不循环**：管道与 socket 上会短写，剩下的数据静默丢失。
3. **把 `errno` 当"当前调用是否成功"**：成功时 `errno` 不会被清零，
   打印成功路径的 `errno` 会显示上一次失败的值（本实验首版就在 `retry_read` 上犯过）。
4. **在子进程里 `return` 而不是 `_exit`**：会刷新父进程的 stdio 缓冲，输出重复。
5. **以为 `fork` 后偏移量独立**：`fork` 复制的是 fd 表，打开的文件描述是共享的。
6. **以为孤儿一定被 PID 1 收养**：有 subreaper 时会被最近的 subreaper 收养。
7. **用 `WEXITSTATUS` 读被信号终止的子进程**：必须先判 `WIFSIGNALED`。
8. **`sched_yield` 当成"让代码更公平"的优化**：它在本实验里是故障注入手段，会显著改变时序。
9. **把 `bad=0` 当成"没坏"**：定长记录被覆盖时不会产生非法行，必须同时看 `good`。

## 故障排查方法

| 现象 | 先查什么 | 判据 |
| --- | --- | --- |
| `lseek` 组也 1600 行全合法 | 子进程是不是各自 `open`；有没有注入 `yield` | 每个子进程里必须有独立的 `open` |
| `O_APPEND` 组丢行 | 是否真的用了 `O_APPEND`；文件系统是否支持原子追加 | `open` 的 `flags` |
| `reads` 不等于 16 | 写端是不是一次写满；管道缓冲是否被别的读者分走 | `total` 与 `last_chunk` |
| `first_read` 的 `ret=0` | 写端是否提前关闭 | 父进程必须在子进程读之前保持写端打开 |
| 没有 `EINTR` | `sigaction` 是否加了 `SA_RESTART`；信号是否在 `read` 之前发出 | `usr1_hits` 必须是 1 |
| 僵尸读不到 `Z` | 子进程是否真的退出；父进程是不是已经 `wait` 了 | 先 `usleep` 再读 `/proc` |
| 孤儿 `ppid == 1` 而不是 subreaper | 环境差异，不是错误 | 判据写成"是否等于祖父 pid" |
| shell 的 `cd` 不生效 | 内建命令是不是在子进程里执行了 | `pwd` 前后必须不同 |
| 管道右端读不到数据 | 父进程有没有关掉两端 | 不关写端，右端永远等不到 EOF |

## 练习题

### 基础题

1. 把 `pipe_read.c` 的读缓冲从 64 改成 4096，预测 `reads` 与 `last_chunk` 会变成什么，再验证。
   哪个读数先暴露"缓冲比数据大"这件事？
2. 在 `fileio.c` 里把记录长度从固定 9 字节改成"随进程号变化"，
   预测 `bad` 会怎么变，并说明为什么。

### 改错题

3. 下面这段"并发追加日志"有什么问题？

   ```c
   int fd = open(path, O_WRONLY | O_CREAT, 0644);   /* fork 之前 */
   for (int i = 0; i < 8; i++)
       if (fork() == 0) {
           for (int k = 0; k < 100; k++) {
               lseek(fd, 0, SEEK_END);
               write(fd, line, len);
           }
           return 0;
       }
   ```
   指出至少两处，并说明各自的后果与验证方法。

### 综合题

4. 让 `minish` 支持 `a | b | c` 三级管道，并加一条用例证明中间进程的
   stdin/stdout 都接对了（提示：用 `cat` 与 `wc -l` 组合）。
5. 给 `minish` 加作业控制的最小版本：`Ctrl-Z` 挂起前台进程组、`fg` 恢复。
   说明你需要哪几个系统调用，以及"为什么本实验一开始就把作业控制列为不做"。

## 验收标准

- [ ] 能跑 `./run.sh` 得到四组全过，并复述 `O_APPEND` 组与 `lseek` 组的唯一差别。
- [ ] 能解释首版"共用父进程 fd"为什么测不出竞态。
- [ ] 能说明为什么 `lseek+natural` 只能记录、不能断言。
- [ ] 能解释 `bad=0` 为什么不能单独作为"没损坏"的证据。
- [ ] 能说清僵尸与"已回收"的判据差别，以及孤儿被谁收养取决于什么。
- [ ] 能对自己的日志做一次变异，让 `judge.py` 按预期失败。

## 延伸阅读

- `man 2 open` 的 `O_APPEND` 段、`man 2 write` 的 "partial writes" 段。
- `man 2 fork` 里关于 "the child inherits copies of the parent's set of open file descriptors"
  与 "share the same open file description" 的区分。
- `man 2 prctl` 的 `PR_SET_CHILD_SUBREAPER`。
- `man 5 proc` 的 `/proc/<pid>/stat` 字段表（第 3 字段是状态字符）。
- `man 7 signal` 的 "Interruption of system calls" 一节。
- 后续实验：[`SYS_02_IPC_Threads`](../SYS_02_IPC_Threads/README.md)（线程与同步）、
  [`SYS_03_Socket`](../SYS_03_Socket/README.md)（网络）。

## 已知限制

- **结论绑定 WSL2 内核 6.6.87.2**。换内核或换文件系统（如 NFS、overlayfs）后
  `O_APPEND` 的原子性保证可能不同，重跑一次再把读数写回本文。
- **`lseek+yield` 的丢行数不可复现**（1426 / 1067 / 1567），只能断言"少于预期"。
- **定长记录掩盖了撕裂**。若要观察半个记录，需要让写入长度不一，或缩小到单字节写入
  （后者在 `PIPE_BUF` 以内，跨管道时另有语义）。
- **`minish` 不是 bash**：没有命令替换、通配符、引号转义、多级管道、`&&` / `||`、
  作业控制、变量展开、脚本函数。它验证的是 fork/exec/wait 与 fd 重定向这一层。
- **没有覆盖 `strace`**：本环境未安装，系统调用顺序靠程序自己打印取证行推断，
  证据强度低于 `strace` 的直接观测。阶段 6 会用 `ptrace` 自己实现一个最小 `strace` 来补这一环。
- **没有信号处理深水区**：`SIGCHLD` 的 `SA_NOCLDWAIT`、`SIGSTOP`/`SIGCONT` 的作业控制语义未覆盖。
