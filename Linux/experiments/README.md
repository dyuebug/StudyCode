# 单主题实验

每个实验只验证一个主要概念，建议目录结构：

```text
experiments/主题名称/
├── README.md
├── source/
├── build/
└── evidence/
```

实验说明必须写清前置环境、构建命令、运行命令、预期结果、实际结果、错误路径和已知限制。

## 驱动入门实验准备

- [环境与验证指南](驱动入门_环境与验证指南.md)：先确认平台能力、内核与工具链，再进行只打印模块实验；板级代码仅用于匹配的目标系统。
- 各 Day 的 README 末尾「验证状态」写明**已实测**与**未验证**的边界：Day2–Day9 有 QEMU 运行证据（见 `../tests/qemu-vexpress/RESULT.md`）；Day10–Day14 为 QEMU 实测的等价机制 + 本日驱动的交叉编译验证；真实外设行为一律留待目标板。
- 实际命令输出写入 `../daily_logs/`，不在材料准备阶段伪造 evidence。
- [每日任务导航](../daily_tasks/README.md) 区分无板必做和条件实验，原始课程代码保留不变。

## BSP / 启动链实验（Linux）

不属于"某一天"的驱动，而是把启动链本身作为实验对象；复用 `../tests/qemu-vexpress/` 的内核树与 DTB。

| 实验 | 主题 | 验证状态 |
| --- | --- | --- |
| [`BSP_01_Boot_Chain`](BSP_01_Boot_Chain/README.md) | zImage / DTB / initramfs 三件套分工、`rdinit` 与 `init` 的查找顺序、DTB 被 QEMU 改写 | **QEMU 四组对照实测**：`normal` 取证 10 项；`bad-init` 得到 `check access for rdinit=... failed: -2, ignoring` + VFS panic；`no-initrd` 同文案 panic 但缺 `Unpacking initramfs`；`qemu-dtb` 零输出。u-boot 阶段与真机 DDR/时钟未验证 |
| [`BSP_02_UBoot_BlockRoot`](BSP_02_UBoot_BlockRoot/README.md) | u-boot 装载与 `bootargs` 传递、`bootz` 的 fdt 与 initrd 参数、`mkimage -T ramdisk`、ext4 块设备根、只读根与 remount、分区表对设备名的影响、initramfs 与 `root=` 的优先级 | **QEMU 八组对照实测**：`uboot-boot` 全链路（`ext4load` 6405304 B，`/proc/cmdline` == `bootargs`）；`uboot-nofdt` 得 `FDT and ATAGS support not compiled in` 且内核未启动；`uboot-badmagic` 得 `zimage: Bad magic!`；`uboot-initrd` 得 `Unpacking initramfs...` + `Freeing initrd memory: 264K`，根为 `rootfs`；`uboot-initrd-vs-root` 命令行含 `root=/dev/vda` 却**没有任何 `EXT4-fs (vda)`**；`kernel-whole`/`kernel-part` 分别挂到 `vda`/`vda1`；`kernel-nopart` 得 `unknown-block(254,1)`。真实 vexpress u-boot、SD/eMMC 的 `rootwait`、BootROM/SPL、initramfs 两阶段挂根未验证 |
| [`BSP_03_SwitchRoot`](BSP_03_SwitchRoot/README.md) | 两阶段启动与切根：`switch_root` 四步（建 devtmpfs/proc/sys → 挂新根 → `MS_MOVE` 搬运 → `chroot`+`exec`）、用 `f_type` 判"根换了没有"、失败路径的干净收尾 | **QEMU 五组对照实测**：`ok` 得 `root_before=tmpfs(0x1021994)` → `root_after=ext4(0xef53)`、`mounts_dev=devtmpfs`、`dev_vda=present`；`rootfs-ramfs` 同镜像加 `root=` 后 `root_before` 变 `ramfs(0x858458f6)`（证明 rootfs 后端由命令行决定）；`no-move` 根仍换成 ext4 但 `/dev` 退化成 ext4 空目录、`vda` 消失、`/proc` 不可用；`noroot`/`no-stage2` 分别得 `mount`/`exec` 的 `errno=2` 且第二阶段跳过。`--judge` 口径做过 6 条负向验证（含日志不存在返回 2）。**`pivot_root`、busybox `switch_root`、真板启动链未验证** |
| [`BSP_04_PivotRoot`](BSP_04_PivotRoot/README.md) | `pivot_root` 与 `chroot + exec` 的同口径对照：命名空间级换根 vs 进程级换根、`new_root` 必须是挂载点、`MS_MOVE` 是搬运而非重新挂载、旧根 `umount` 恒 `EBUSY` 的真实成因 | **QEMU 五组对照实测（5/5 通过）**：`chroot` 观察者仍见 `tmpfs`+`/init`（只改本进程视角），`pivot` 观察者见 `ext4`+`gone`（命名空间一起换根）；`pivot-nomove` 根已换但 `dev_vda=absent`（`pivot_root` 不搬挂载点）；`pivot-ondir` 对普通目录得 `EINVAL(22)` 且失败后旧根未动；`pivot-late` 为单变量因果实验——`exec` 后旧根仍 `EBUSY(16)`（**否证**"stage1 可执行映射钉住旧根"），只把 0/1/2 从 `/oldroot/dev/console` 重绑到新根 `/dev/console`，同一挂载立刻严格卸载成功；节点由 `init/noinitramfs.c:18` 建（探针 `mode=020600`）、fd 由 `init/main.c:1603` dup。另 pin 一条反直觉事实：chroot 下 `/proc/self/mounts` 隐藏旧根（`mounts_rootfs=0`）。`--judge` 做过 10 条负向验证。**`MS_SHARED` 传播、只读根/overlay、busybox `switch_root`、真板未验证** |

一句话结论：三件套里 DTB 是**必要输入**（拿不到就连串口都出不字），initramfs 决定"根从哪来"，
`rdinit` 决定"根里的谁当 PID 1"——三者失败时的现象互不相同，但两种不同缺失可以 panic 在同一行。

再往下一层（`BSP_02`）：换成 bootloader 引导后，`bootz` 的三个参数少一个内核就不启动；
根换成 ext4 块设备后，内核先把根挂成**只读**、由 init 负责 remount，而 `root=/dev/vda1` 写错时
panic 括号里的 `254:1` 与"没有块设备"的 `0:0` 是两种完全不同的故障。

initramfs 与 `root=` 的关系已用实测钉死：`bootz` 的 initrd 参数只收 uImage/FIT（裸 gzip 被拒），
而一旦 initramfs 里有 `/init`，`root=` 会被**整个跳过**——判据是日志里有没有 `EXT4-fs (xxx): mounted filesystem`，
不是 `bootargs` 里写了什么。

## 系统编程实验（Linux）

对应 `CLAUDE.md` §3 阶段 1–6。与 BSP / MCU 不同，这一组跑在**真实的 Linux 内核**上（WSL2 Ubuntu），
不依赖交叉编译，判据由 `judge.py` 机器校验。

| 实验 | 主题 | 验证状态 |
| --- | --- | --- |
| [`SYS_01_FileIO_Proc`](SYS_01_FileIO_Proc/README.md) | 文件 I/O 与进程生命周期：`O_APPEND` 原子性 vs `lseek+write` 竞态、`O_CLOEXEC`、短读、`EINTR`、僵尸/孤儿/`PR_SET_CHILD_SUBREAPER`、`wait` 状态解码、简化 Shell | **WSL2 实测**（内核 6.6.87.2，gcc 15.2.0）：判定器 19/19 通过。`O_APPEND` 组 5/5 次 `good=1600/1600`，注入 `sched_yield` 的 `lseek` 组 4/4 次丢行（1414/1426/1067/1567，`good=1414` 对 `bytes=12726 = 1414×9` 证明确是整行丢失而非撕裂）；`lseek+natural` 5 次里 1 次丢行（1547），**只记录不断言**。孤儿 `ppid` 实测 369/497（**不是 1**，WSL 自带 subreaper），设 subreaper 后等于祖父 500。9 条负向验证：8 条 `rc=1`、缺日志 `rc=2`、原日志 `rc=0`。**`strace` 级系统调用序列、NFS/overlayfs 上的 `O_APPEND`、写入长度不一的撕裂行未验证** |
| [`SYS_02_IPC_Threads`](SYS_02_IPC_Threads/README.md) | IPC 与线程同步：五种通道的消息边界与 EOF 语义、虚假唤醒 `if` vs `while`、有界队列、无锁/互斥/原子/读写锁不变量、`timedlock` 观测死锁、线程池优雅与立即关闭 | **WSL2 实测**（同上一组环境）：`./run.sh` 26/26 通过。`pipe`/`fifo`/`uds-stream` 的 `first_read` 均等于 `sent=3000`（`boundary=merged`），`uds-dgram` 为 `msgs=3 lens=1000,1000,1000` 且 `eof_via=sentinel`（**`SOCK_DGRAM` 没有 EOF 语义**，首版依赖对端 close 直接挂到 `timeout` rc=124）；`spurious-if` `flag_at_exit=0`/`safe=0`，`spurious-while` `flag_at_exit=1`；队列 `stale_wake=19`（"醒来时货已被别人取走"，**不是**虚假唤醒）；`plain` `final=22131`（丢 57869）且 TSan 报 2 次竞争，而 mutex/atomic/rwlock 全部 80000、零报告；`deadlock` `t1_rc=t2_rc=110`（`ETIMEDOUT`）vs `ordered` 双双 0；线程池 graceful `executed=200` / immediate `executed=1`，两组均 `joined=4/4 fd_leak=0`。**14 条负向验证**：6 份原日志 `rc=0`、12 条变异 `rc=1`、缺日志与未知用例 `rc=2`。**未验证**：线程池背压（`max_depth=200` 由一次性提交造成）、三把锁/递归锁死锁、性能未测 |
| [`SYS_03_Socket`](SYS_03_Socket/README.md) | Socket 语义与多路复用：TCP 字节流 vs UDP 数据报、长度前缀分帧、半关闭、短写循环、`EPIPE`/`ECONNRESET`/`EAGAIN`/`EMSGSIZE`、`select`/`poll`/`epoll`、ET 必须读到 `EAGAIN` | **WSL2 实测**（同上一组环境）：`./run.sh` 14 个用例 rc=0，`judge.py` 25/25 通过（tcp 16、udp 4、mux 5）。三次 1000 字节 `write` 被一次 `recv` 收成 `first_read=3000`（`boundary=merged`），长度前缀恢复出 `framed=3`；UDP 三条数据报各收 1000（`preserved=1`），缓冲不足时 `recv_rc=1000`/`real_len=2000`（`MSG_TRUNC` 才看得到真实长度），70000 字节 `EMSGSIZE(90)`；半关闭 `eof_seen=1 sent_after_eof=1 read_after_shutdown=1`；`SO_RCVTIMEO=200ms` 得 `errno=11 elapsed_ms=203`；1 MiB 写入出现 `short_writes=17`，循环后 `sent=recv=1048576 mismatch=0`；RST 关闭 `errno=104`，**正常 FIN 关闭才是 `EPIPE(32)` + `handler_hits=1`**（首版照搬 RST 写法，信号一次没来）；ET 只读一次的版本 `done=0/8 stalled=1 elapsed_ms=2002`，四种正确后端均 `done=8/8 events=16`。**16 次判定器验证**：3 份原日志 `rc=0`、11 条变异 `rc=1`、缺日志与未知用例 `rc=2`。**未验证**：压测/吞吐/`FD_SETSIZE` 上界、真实网络（MTU/丢包/乱序）、`EPOLLONESHOT`/`accept4`/非阻塞 `connect`、`strace`/`tcpdump` 对照；`mux` 的 `elapsed_ms=0` 表示数据在 `accept` 前已到达，不证明长跑调度公平性 |
| [`SYS_04_Reactor`](SYS_04_Reactor/README.md) | Reactor 三种形态：单 Reactor 单线程 / Reactor + 线程池（`eventfd` 唤醒 + 完成队列 + 每连接一个在飞 job）/ 主从 Reactor（连接转交，sub 各带一个 `epoll`）、`EPOLLONESHOT` 必须显式重挂、监听 fd 不能带 `EPOLLONESHOT`、两个破坏版必须**必然**失效 | **WSL2 实测**（同上一组环境）：`./run.sh` 7 个用例，判定器 26/26 通过。8 连接 × 20 帧 × 200 B（帧长 204 B）、业务 `nanosleep(2ms)`：`single total_ms=339`（串行理论下限 320，判据取下限 160）、`pool4=85`（= `single` 的 25%，`q_drops=0`）、`master2=169`（`handoff=8`，`sub0/sub1=80/80`；主 reactor `accepted=0` 属正常，连接数看 `handoff`）、`et=338`（与 `single` 同量级，**此规模下 ET 无收益**，只证明正确性）、`oneshot=98`（`delay_us=500`、`gap_ms=2`）。破坏版必然失效：`et-broken` 服务端 8 帧/客户端 0 帧（每次只读 128 B 且读一次，剩余字节在 ET 下不再出边沿）、`oneshot-broken` 服务端 8 帧且客户端 `verified=8`（8 条连接各恰好第一帧）；两者因半死连接连 FIN 都收不到，空闲退出判据不成立，只能撞 `--max-run 6000`（`serve_ms=6042/6025`）。`serve_ms - total_ms = 391~409 ms` 恰为 `--idle-exit` 的 400 ms 空等窗口（`serve_ms` 不是性能指标）。**两处设计被实测推翻**：监听 fd 未设非阻塞 → 挂死在 `inet_csk_accept`（`/proc/<pid>/wchan` 定位，客户端 8 次超时）；`ep_mask()` 无差别套在监听 fd 上 → `oneshot` 只 accept 到 4 条（`accepted=4 verified=80`）。**14 次判定器验证**：原日志 `rc=0`、12 条用例内变异 `rc=1`、缺日志 `rc=2`；变异限定在指定用例块内，且"`single` 变快"一处同时打穿 3 条共享基线前提的断言（已如实记录不是独立证据）。**未验证**：真网络与连接数/吞吐扫描、`SO_REUSEPORT`/`EPOLLEXCLUSIVE` 惊群、`accept4`、写缓冲背压（`buf_drops` 全程 0，路径未触发）、TSan 竞争检测 |
| [`SYS_05_Coroutine`](SYS_05_Coroutine/README.md) | C++20 协程：`FrameGuard` 存活帧计数、`Generator`/`Task`、对称转移（`await_suspend` 返回 `coroutine_handle<>`）、`exception_ptr` 传播、单线程 epoll 调度器（`EPOLLONESHOT` + 每次挂起必须重挂）、公平性配额、"挂起即活动"的空闲超时、取消/超时/半关闭/RST 四条路径、协程 vs 每连接一线程 | **WSL2 实测**（同上环境，`g++ 15.2.0`，`-std=c++20`）：18 个用例，判定器 58/58、负向验证 22/22（原日志 `rc=0`、20 条用例内变异 `rc=1`、缺日志 `rc=2`）。基础用例：`gen sum=30 cnt=5 alive_after_iter=1 alive_after_break=0`（遍历后仍占帧、提前 break 也释放）、`task resumes=1 value=42`（三层嵌套只 resume 一次 = 对称转移）、`life done_before=0 alive_after_run=1 alive_after_destroy=0`（`suspend_always` 下"跑完≠释放"）、`leak` vs `leak-broken` 的 `alive_after` 0 vs 100（泄漏变读数）、`sched-order order=01234 hi_first=1 iters=2`。服务端（8 连接 × 20 帧 × 200 B）：`coro-greedy frames=160 max_run=20 iters=24 serve_ms=443` vs `coro-fair（batch=1）max_run=1 iters=42 serve_ms=850`；**同参数 `coro-rr` 的 `max_run` 也是 1**——`max_run` 度量"一次 resume 里内核缓冲给了多少帧"，不是公平性单值指标。`coro-cancel cancelled=2`（客户端 `eof_seen=2`）、`coro-rst resets=4`、`coro-halfclose eof=4`；破坏版只差一个开关：`coro-idle frames=2` vs `eagain-as-eof frames=0`，`coro-cancel use_after_destroy=0` vs `cancel-free use_after_destroy=2 serve_ms=4009`（靠 `--max-run` 退出）。地址空间：协程版 `vm_kb` 恒 7228、线程版 8 线程 269404–400476，两者 `rss_kb` 同为 4352（线程栈是预留未触碰的地址空间，必须看 `VmSize`）。**一处实测推翻**：`--batch 1` 的让出写成 `co_await wait(EPOLLIN)`，而剩余帧在自己缓冲里、内核不再报可读 → `frames=8/160`、客户端 `timeouts=8 total_ms=2054`；改为 `yield_here`（挂起后立刻重新入队）后 `frames=160`。**一处设计期修正**：超时原本靠处理完一帧后 `touch()` 顺延，静默连接永远不设 deadline，改为 `arm()` 里"挂起即活动"（无失败日志，如实标注）。**判定器自身缺陷**：`negtest` 第一版 9 条 basics 变异报 `mutate-not-found`（行尾 `\n` 未 strip → 用例名 `"gen\n"`），若把"没找到变异"当跳过就会假通过，故明确计为 FAIL。**未闭环**：线程版 `vm_kb` 每线程 41–72 MB、跨运行 269–400 MB，空睡线程对照只有 8 MB/线程且线性，成因未定位（基准文档 §6 不写每线程成本结论）；`io_uring`、跨线程协程调度、长稳压测未做 |
| [`SYS_06_Debug_Perf`](SYS_06_Debug_Perf/README.md) | 调试与性能的**替代实现**：`ptrace` + `PTRACE_GET_SYSCALL_INFO` 自写 strace、`sigaltstack`+`backtrace`+`dladdr`+`addr2line` 替代 gdb、ASan/UBSan/TSan 三组对照、`clock_gettime`/`getrusage`/`socketpair` 替代 perf、`perf_event_open` 权限分层 | **WSL2 实测**（同上环境，`gcc 15.2.0`，`-std=c11`）：24 个用例，判定器 **101/101**、负向验证 **23/23**（原日志 `rc=0`、21 条用例内变异 `rc=1` 且落在预期判据、缺日志 `rc=2`）。本机 `strace`/`ltrace`/`gdb`/`valgrind`/`perf`/`cmake` **全部缺失**（`S06\|env\|tool=*` 记录），因此全部以替代实现完成。fd 收支 `fdok net_fd=0` vs `fdleak net_fd=2`（`opens-closes=2`）；**vDSO 盲区**：`syscalls=1034` 中 `clock_gettime=1000`，另 1000 次走 vDSO 的调用**零停点**（差值 34 = 加载器日志开销）；崩溃回溯 `crash-null` 解出 `on_sig(crash.c:52) → f3(crash.c:80) → f2(crash.c:85) → f1(crash.c:90) → main(crash.c:126)`，`crash-deep frames=64` 中 62 帧落在 `deep(crash.c:99)`，同一帧用绝对地址查得 `??`（反例固化）；ASan 四类 `heap-buffer-overflow`/`heap-use-after-free`/`stack-overflow`/`leak` 各 `rc=1`，UBSan 四类原话（`shift exponent 40…`、`signed integer overflow`、`division by zero`、`load of null pointer`），TSan `race rc=66` vs `atomic`/`mutex` `rc=0` 且 `result=expect=400000`；基准 `vdso 15.66 ns` vs `syscall 41.64 ns`（2.66×）、缺页 `first=16384≈pages` / `second=0`、`ctxswitch 42354.8 ns/rt`（1017× syscall）；`perf self_fd=3` 而 `systemwide`/`other` 均 `EACCES(13)`，自测 `cycles=80018326` / `instructions=100000081` / `IPC=1.25`。**两处实测推翻**：`-O1` 删掉死存储使 ASan 首版看不见越界；`-O1` 把"分配后不用"整对消掉（`objdump` 只剩 `ret`），最终靠 `volatile` 函数指针才让 LSan 报出 64 B 泄漏。**未验证**：非子进程 `PTRACE_ATTACH`、root 下系统级 perf、内联帧符号化、`ulimit -s` 参数扫描；`ctxswitch` 21 µs/次偏高且成因未定位，只作同机相对比较 |

一句话结论：`O_APPEND` 的原子性可以测出来，但前提是每个进程拥有**独立**的文件偏移量——
首版让子进程共用父进程 `fork` 前打开的 fd，两组读数完全相同（都 1600），差点得出"教科书错了"的假结论。

阶段 2 的一句话结论：**收尾规则跟着通道语义走，不跟着数据走**——字节流靠"关掉所有写端得到 EOF"，
数据报没有 EOF 只能靠应用层哨兵；同一个把"醒来时条件不成立"命名为虚假唤醒的错误也在这一组被纠正，
它恰好是"即使没有虚假唤醒也必须用 `while`"的证据。

阶段 3 的一句话结论：**对端"没了"有两种，触发条件相反**——RST 让 socket 直接进错误态，
所有后续操作拿到 `ECONNRESET`；只有收到 FIN 之后继续写才走 `EPIPE` + `SIGPIPE`。
首版用 RST 测 SIGPIPE，信号一次没来（`handler_hits=0`、`errno=104`），
换成正常 `close()` 后立刻稳定复现（`handler_hits=1`、`errno=32`）——
同一组里 ET 的"必须读到 `EAGAIN`"也不是靠记，而是靠把读块压到 16 字节让漏读必然停住（`done=0/8`）。

阶段 4 的一句话结论：**三种 Reactor 形态的差别只有一句"业务处理在哪个线程里执行"**——
accept、读、分帧、写三形态共用同一份代码，差别只在 `deploy()` 分支与"连接归谁"的所有权边界。
本阶段两次被读数推翻的都是"看起来像客户端问题"的服务端错误：漏设非阻塞让事件循环挂死在
`inet_csk_accept`（`wchan` 一行定位），以及把 `EPOLLONESHOT` 无差别套到监听 fd 上导致
`accepted=4`——监听 fd 没有人替它重挂。ET 在 8 连接/204 B 帧的规模下与 LT 同量级（338 vs 339 ms），
它被证明的只是正确性，收益前提在本实验里不成立，因此不外推。

阶段 5 的一句话结论：**协程的难点不在语法，而在"这次唤醒是谁保证会来的"**——
每一次 `co_await` 都必须有一个确定会发生的触发源，说不出来的那个挂起点就是丢帧的地方。
`--batch 1` 的让出写成等 `EPOLLIN`，数据却早已在自己的缓冲里，于是 160 帧只回了 8 帧；
同一份代码在 `batch=0` 下完全正常，**只有对照才能发现**。同理，`max_run` 在同参数的
`pipeline` 与 `rr` 客户端下分别是 20 和 1，说明"公平性指标"必须固定客户端行为才可比。
地址空间上，协程版 `VmSize` 零增长而线程版 8 线程涨到 269–400 MB，`RSS` 却完全一样——
线程栈是"预留但不触碰"的地址空间，用 `RSS` 看等于没看。线程版每线程 41–72 MB 的成因
**未定位**（空睡线程对照只有 8 MB/线程），因此不外推每线程成本。

阶段 6 的一句话结论：**"有工具就信工具"是错的，先确认工具和能力都在**——
本机 `strace`/`gdb`/`valgrind`/`perf`/`cmake` 全缺，但五个缺口都不是"做不了"，而是
"要自己搭一层，并说清这层的观测边界"：`ptrace` 版 strace 永远看不见 vDSO（1000 次调用
一个停点都没有，只有强制 `syscall(SYS_clock_gettime,…)` 才现形）；`backtrace`+`addr2line`
能替代 gdb，但**故障 PC 不能减 1、返回地址必须减 1**（首版搞反，`f3` 被解成
`frame_dummy|crtstuff.c:?`）；`perf_event_open` 的 `EACCES(13)` 说明缺的是**权限和二进制**，
而不是计数能力（自身 `fd=3`、真的读到了 `cycles`/`IPC=1.25`）。
最该记住的一条来自 sanitizer：**工具没报 ≠ 缺陷不存在**——`-O1` 会把死存储删掉、
把"分配后不用"整对消掉（`objdump` 里 `leak_alloc` 只剩一条 `ret`），两次都要靠 `volatile`
才让缺陷重新变成可观测的。同理，TSan 报了竞争而计数 `lost=0` 连跑 20 次，
恰恰说明"没丢计数"不能当"没有竞争"的证据。

## MCU 分支实验（STM32F103）

与上面的 Linux 驱动实验并列，不共享构建系统。公共设施在 [`F103_common/`](F103_common/README.md)：
自撰链接脚本、裁剪后的 CMSIS 子集、CMake 工具链文件、OpenOCD 烧写配置。

| 实验 | 主题 | 验证状态 |
| --- | --- | --- |
| [`F103_01_GPIO_Blink`](F103_01_GPIO_Blink/README.md) | 寄存器级 LED 点灯、SysTick 轮询延时 | 构建实测（arm-none-eabi-gcc 14.2.1）；板级现象未验证；QEMU 无证据（GPIO 未建模） |
| [`F103_02_GPIO_Input`](F103_02_GPIO_Input/README.md) | 按键输入、输入上下拉机制、软件消抖与边沿检测 | 构建实测 + 反汇编核对；按键手感未验证；QEMU 无证据 |
| [`F103_03_EXTI_Key`](F103_03_EXTI_Key/README.md) | EXTI14 + NVIC、挂起位清除、中断与主循环分工 | 构建实测 + 向量表与寄存器核对；中断触发未验证；QEMU 无证据（EXTI 未建模） |
| [`F103_04_UART_Echo`](F103_04_UART_Echo/README.md) | USART1 轮询收发、BRR 与时钟依赖、printf 重定向与缓冲策略 | 构建实测 + **QEMU 实测收发与回显**（rx 0→7→12）；真机串口与波特率未验证 |
| [`F103_05_UART_IRQ_Ring`](F103_05_UART_IRQ_Ring/README.md) | USART1 中断接收、SPSC 无锁环形缓冲、ORE 与丢弃计数 | 构建实测 + **QEMU 实测 RXNE 中断收包**（rx 0→7→13）；真机与 ORE 溢出路径未验证 |
| [`F103_06_TIM_PWM_RGB`](F103_06_TIM_PWM_RGB/README.md) | TIM2 三通道 PWM、JTAG 引脚释放、预装载与极性 | 构建实测 + 寄存器路径核对；亮度与波形未验证；QEMU 无证据（TIM 未建模） |
| [`F103_07_I2C_MPU6050`](F103_07_I2C_MPU6050/README.md) | I2C1 主机轮询、单字节读时序、超时与总线恢复 | 构建实测 + 寄存器与指令顺序核对；需外接模块；QEMU 只跑到超时分支（I2C 未建模） |
| [`F103_08_SPI_Flash`](F103_08_SPI_Flash/README.md) | SPI1 主机、模式 0 与 NSS 配置、W25Q 指令协议与 WIP 轮询 | 构建实测 + 寄存器与指令常量核对；需外接模块；QEMU 下 JEDEC 读 00 00 00、自检 FAIL |
| [`F103_09_Delay_Precision`](F103_09_Delay_Precision/README.md) | SysTick / TIM4 / DWT 三种微秒延时、回卷补偿与开销量级 | 构建实测 + 寄存器与指令核对；误差与探针脉宽未验证；QEMU 下挂死（DWT/TIM4 未建模） |
| [`F103_10_FreeRTOS_Tasks`](F103_10_FreeRTOS_Tasks/README.md) | FreeRTOS 任务/队列/互斥量、SVC-PendSV-SysTick 路由、堆与栈高水位 | 构建实测 + **QEMU 实测调度、抢占与队列交接**（6 s 真实时间内 88 条消费、produced=consumed=85、空闲堆恒定 496 B）；真板与绝对毫秒未验证 |
| [`F103_11_FreeRTOS_ISR_Queue`](F103_11_FreeRTOS_ISR_Queue/README.md) | 中断→任务通信：USART1 RXNE、`xQueueSendFromISR`、`portYIELD_FROM_ISR`、优先级门槛 | 构建实测 + **QEMU 实测 ISR 投递与行解析**（rx 0→7→14、突发 300 B 触发 trunc=4 且 `300=4×64+44` 自洽、heap_free 恒定 1048 B）；ISR 丢弃与硬件 ORE 未验证 |
| [`F103_12_FreeRTOS_Sync`](F103_12_FreeRTOS_Sync/README.md) | 二值/计数信号量、事件组、中断→信号量的事件合并、栈水位判据与 4 KB 堆分段记账 | 构建实测 + **QEMU 实测四段同步语义**（`bin waited=801/802`、`take2(0ms)=pdFALSE`、`event wait_all got=0x03 waited=200`、`event timeout waited=300`、`bytes=7 gives=2 coalesced=5`、heap_free 恒定 880 B、三任务水位 86/96/58 字）；`xEventGroupSetBitsFromISR` 与真机时序未覆盖 |
| [`F103_13_FreeRTOS_Timers`](F103_13_FreeRTOS_Timers/README.md) | 软件定时器：守护任务上下文、单次/周期、`xTimerReset`、回调阻塞的代价、5 段堆记账 | 构建实测 + **QEMU 实测定时器语义**（`in_daemon=1`、`once` 两次 `drift=0`、`stalled dur=60` 时 `drift` 仍为 1、fast `#20/#21` 同在 tick 1071 后 `#22` 回到 `drift=1`、heap_free 恒定 736 B、三任务水位 74/58/132 字）；真机毫秒精度与看门狗未验证 |
| [`F103_14_FreeRTOS_Watchdog`](F103_14_FreeRTOS_Watchdog/README.md) | 任务心跳监督、超时判定、"与"语义停喂、连续 STALE 后的恢复动作、IWDG 代码路径 | 构建实测 + **QEMU 实测监督逻辑**（注入→首次 STALE 409/410 tick、两轮停喂窗口 819/1424 tick 跨过 IWDG 名义 998 ms、`stale_total=blocked=9`、`feeds=26−9`、三任务水位 132/106/108 字）；**IWDG 计时与复位、`RCC->CSR` 复位原因未验证（QEMU 未建模）** |
| [`F103_15_FreeRTOS_StackOverflow`](F103_15_FreeRTOS_StackOverflow/README.md) | 栈溢出检出：让 `vApplicationStackOverflowHook` 真的触发；检查时机（切换时、查被换出任务）、`hwm == 0` 作为 paint 被写穿的判据、越界量由步长控制 | 构建实测 + **QEMU 实测钩子触发**（`[STACK OVERFLOW] 任务 probe`）：对照 depth=12 余量 160 字、标定 16 B/帧、逐级加深在 depth=60 归零、稳态段每 4 帧减 16 字；`run.sh exp15` 判定通过并做过三条负向验证；**只覆盖 paint 那一半（栈底 16 B），SP 越过栈块与 MPU 隔离未验证** |
| [`F103_16_FreeRTOS_PriorityInversion`](F103_16_FreeRTOS_PriorityInversion/README.md) | 优先级反转与继承：低持锁 → 中忙等抢占 → 高阻塞；只换锁类型（二值信号量 / 互斥量）做对照；继承只提升持锁者、不能缩短它自己的 delay | 构建实测 + **QEMU 实测两轮对照**：`binary-sem` H 等锁 405 / L 持锁 404 / L 阶段一 354 / L 阻塞期 prio 恒 1；`mutex` 105 / 105 / 52 / prio 1→3→give 后回 1（另一次运行为 412·410·358 与 103·104·53，比值 3.86–4.0）；栈水位 82/192、58/160、64/160、54~62/160 全部过"≥32 字且 ≥25%"；`run.sh exp16` 判定通过（`binary > 2×mutex`），4 条负向验证全部按预期失败。**死锁、继承链、中断延迟、临界区长度未验证** |

QEMU 验证的完整能力边界、原始串口日志与复现命令见 [`../tests/qemu-stm32/`](../tests/qemu-stm32/README.md)。
一句话小结：能仿真验证的链路是"串口 + SysTick + NVIC"以及**建立在它们之上的 FreeRTOS 调度器、
中断→任务交接、同步原语、软件定时器与心跳监督**；
GPIO/TIM/I2C/DWT/IWDG 全部未建模（`RCC->CSR` 复位标志同样读回 0），且 QEMU 的 SYSCLK 是 24 MHz（固件按 8 MHz 假设），
时间相关结论只可用于比较比例，不得用于绝对毫秒。

> 综合项目不放 `experiments/`。航空主题节点在 [`../projects/aviation-node/`](../projects/aviation-node/README.md)：
> 它复用本目录的 `F103_common` 构建设施与 FreeRTOS vendor 子集，但包含协议、状态机与多任务协同，
> 属于"可复现项目"而非"单主题实验"，验收门禁按 `CLAUDE.md` §7「项目完成」执行。

约定：

- 目录名用 ASCII，避免 CMake/Ninja 在 Windows 控制台下的中文路径问题；主题用中文写在 README 里。
- 每个实验一个独立 CMake 工程（各自 `build/`），不合并成顶层多目标工程，便于单独烧写和排障。
- 新增外设时先补 `F103_common/vendor/` 的必要 HAL 模块，再写实验代码，不预先引入整套 HAL。

