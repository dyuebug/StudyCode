# BSP_03：两阶段启动与切根（initramfs → 真实根 → `switch_root` 四步）

> 结论先行：五组对照全部机器判定通过。切根成功的判据不是"根目录里有文件"，
> 而是 `/` 的 `f_type` 从 `tmpfs`(0x01021994) 变成 `ext4`(0xef53)；`MS_MOVE` 是否生效的判据
> 也不是 `f_type`（本内核区分不了），而是 `/proc/self/mounts` 里 `/dev` 的类型串与 `/dev/vda` 是否存在。
>
> 另有两处**与教科书说法不符、以本内核实测为准**的发现：
> 1. 本内核（7.0.0 树）的 rootfs 后端是 shmem，`f_type` 报 **tmpfs** 魔数，不是常说的 `RAMFS_MAGIC`；
>    只有在命令行加上 `root=` 时才回落到 ramfs。两组用例（`ok` / `rootfs-ramfs`）分别证明了这两面。
> 2. devtmpfs 以内核 shmem 为后端，`statfs` 继承后端实现，因此 `f_type` 与 tmpfs **相同**，
>    靠魔数无法证明"`/dev` 挂的是 devtmpfs"。

## 学习目标

1. 把 `switch_root` 拆成四个可分别验证的动作：建基础树 → 挂新根 → 搬运挂载点 → `chroot` + `exec`。
2. 用 `f_type` 而不是"目录看起来像"来判断根是否换了，并说明这个判据在哪些情况下会失效。
3. 用一组反例证明 `MS_MOVE` 不能省：只 `chroot` 也能换根，但设备节点与 `/proc` 全部丢失。
4. 说清 `chroot` 与 `exec` 各自负责什么：前者改进程视角，后者释放旧根的最后引用。
5. 记录一次"判据按假设写、被实测推翻"的完整过程，并给出改成按实测写之后的负向验证。

## 环境与前置条件

| 项 | 值 |
| --- | --- |
| 机器 | QEMU `-M virt`（ARMv7，512 MB，单核），virtio-blk 作为块设备 |
| 内核 | `~/kbuild/linux-source-7.0.0` 编译的 `arch/arm/boot/zImage` |
| 内核配置（实测） | `CONFIG_DEVTMPFS=y`、`CONFIG_DEVTMPFS_MOUNT` **未设**、`CONFIG_TMPFS=y`、`CONFIG_VIRTIO_BLK=y`、`CONFIG_EXT4_FS=y` |
| 用户态 | 无 busybox（`dpkg -l \| grep -c busybox` = 0），因此 stage1/stage2 都是**手写的静态 C 程序** |
| 编译器 | `arm-linux-gnueabihf-gcc` 15.2.0，`-O2 -g -Wall -Wextra -Werror -static`，零警告 |
| 工具 | `mke2fs`（造 ext4 镜像）、`cpio` + `gzip`（打包 initramfs） |
| 前置 | [`BSP_01`](../BSP_01_Boot_Chain/README.md)（zImage/DTB/initramfs 各管什么）、[`BSP_02`](../BSP_02_UBoot_BlockRoot/README.md)（块设备根） |

产物体积（实测）：

| 文件 | 大小 |
| --- | --- |
| `stage1` | 500372 B |
| `stage2` | 502896 B |
| `initramfs.cpio.gz` | 239138 B |
| `root_ok.ext4` / `root_no_stage2.ext4` | 8388608 B（各 8 MiB） |

## 核心概念

### 1. 四个动作，各自可以被单独否证

```text
① 建运行期基础树   mount devtmpfs /proc /sys          → 宿主的 /dev 空则后面连 /dev/vda 都没有
② 挂新根           mount("/dev/vda", "/mnt/newroot")  → 挂错或没挂，后面全崩
③ 搬运挂载点       mount(..., MS_MOVE) × 3            → 不搬，新根里 /dev 只是空目录
④ 换根 + 换进程    chroot + execve("/stage2")         → 不 exec，旧根永远不释放
```

本实验的五个用例与这四步的对应关系：

| 用例 | 破坏的位置 | 预期现象 |
| --- | --- | --- |
| `ok` | — | 四步全过 |
| `rootfs-ramfs` | —（只改命令行） | 只有 `root_before` 的 `f_type` 变了 |
| `no-move` | ③ | 根换了，但 `/dev` 变成 ext4 空目录、`/proc` 不可用、`vda` 消失 |
| `noroot` | ② | `mount` 返回 `ENOENT`，第二阶段根本没跑 |
| `no-stage2` | ④ | `chroot` 成功，`execve` 返回 `ENOENT` |

### 2. `MS_MOVE` 与"重新挂载"的区别

| | `MS_MOVE` | 再 `mount` 一次 |
| --- | --- | --- |
| 实例数 | 仍是原来那一个（搬过去） | 变成两个独立实例 |
| 旧位置 | 消失 | 依旧挂着 |
| 旧根释放时 | devtmpfs 实例已在新根上，不受影响 | 旧根上的实例随超级块一起销毁，设备节点消失 |

`mount(2)` 的 man 里对 `MS_MOVE` 有一句关键约束：源必须是一个挂载点，目标必须是目录，
且搬运后源位置不再是一个挂载点。本实验的 `move_mountpoint()` 就是按这个语义写的（名字不能叫
`move_mount`——那是 glibc 已声明的系统调用包装函数，见"关键实现点"）。

### 3. `chroot` 不改挂载表，`exec` 才释放旧根

`chroot()` 只改进程的路径解析起点。旧 rootfs 的页仍被两样东西引用着：
本进程的可执行文件映像、以及它已映射的数据页。这也是为什么 `switch_root` 的最后一步一定是
`execve`——换一个进程映像，旧引用才断，内核才能回收那段内存。

本实验能用两条证据把这一点钉住：

- 第二阶段读 `/init` 得 `old_init|gone`：新根视角下它不存在（`chroot` 生效了）；
- 第二阶段的 `cmdline_after` 与第一阶段一致，但 `mounts_root|ext4`：`/proc` 是**搬过来的**实例，
  不是重新挂的（否则内容会是空的、且与旧文件系统无关联）。

### 4. 判据：为什么是 `f_type`，以及它在什么情况下会骗你

`statfs` 的 `f_type` 是内核超级块上的魔数，用户态改不了，也不受目录内容影响。
本实验实测到的相关魔数：

| 路径 | 实测 `f_type` | 说明 |
| --- | --- | --- |
| `/`（切根前） | `0x01021994` tmpfs | 命令行无 `root=` ⇒ rootfs 建在 shmem 上 |
| `/`（切根前，给了 `root=`） | `0x858458f6` ramfs | 见 `rootfs-ramfs` 组 |
| `/`（切根后） | `0xef53` ext4 | 新根 |
| `/dev`（搬运成功） | `0x01021994` tmpfs | **devtmpfs 用 shmem 作后端，魔数与 tmpfs 相同** |
| `/dev`（未搬运） | `0xef53` ext4 | 新根上的普通空目录 |
| `/proc` | `0x9fa0` proc | |
| `/sys` | `0x62656572` sysfs | |

两条必须记住的边界：

1. **`f_type` 区分不了 devtmpfs 与 tmpfs**。要区分只能读 `/proc/self/mounts` 第三列的类型串
   （本实验的 `mounts_dev` 就是这一列）。所以"`/dev` 搬过来了"这条结论由 `mounts_dev|devtmpfs`
   与 `dev_vda|present` 一起支撑，而不是由魔数。
2. **`root_before` 是 tmpfs 而不是 ramfs 是内核行为**，不是观察误差。
   `init/do_mounts.c:init_rootfs()` 在"命令行既没有 `root=` 也没有 `rootfstype=`"且
   `CONFIG_TMPFS=y` 时设 `is_tmpfs = true`，`rootfs_fs_type.init_fs_context` 于是走
   `shmem_init_fs_context()`。`rootfs-ramfs` 组给了 `root=/dev/vda` 之后魔数就变成 `0x858458f6`。

### 5. 为什么失败用例也必须"干净结束"

stage1 与 stage2 都是 PID 1。PID 1 `return` → 内核 panic，串口上会多出与实验无关的一大段栈，
把真正的证据淹掉。所以两条失败路径都是：先打 `SWROOT|fail|...`，再 `reboot(RB_POWER_OFF)`。
配合 QEMU 的 `-no-reboot`，进程干净退出（`exit=0`），日志末尾只剩取证行。

## 接口卡片

### `int mount(const char *src, const char *tgt, const char *fs, unsigned long flags, const void *data)`

作用：挂载或搬运。`flags` 含 `MS_MOVE` 时，`fs` 与 `data` 必须为 `NULL`，`src` 必须是已存在的挂载点。

返回值和错误：0 成功；-1 置 `errno`（`ENOENT`=设备节点或目标目录不存在、`EBUSY`、`EINVAL`=不是挂载点）。

资源所有权：成功即在内核里建立/移动一个挂载实例，引用计数由内核维护。**搬运后旧位置立刻不可访问**，
不需要 `umount`。

常见误用：把 `MS_MOVE` 和 `MS_REMOUNT` 混用；`src` 写成设备路径而非挂载点；目标目录不存在。

### `int chroot(const char *path)`

作用：把当前进程的根目录改成 `path`。

要点：**只改进程视角**。不卸载旧根、不改挂载表、不影响其他进程。需要 `CAP_SYS_CHROOT`
（PID 1 天然满足）。`chroot` 之后必须 `chdir("/")`，否则 `getcwd()` 与新根无关、相对路径仍指向
旧根内部。

常见误用：以为 `chroot` 就是"切根完成"——它没有让旧根消失，只是你看不到它了。

### `int execve(const char *path, char *const argv[], char *const envp[])`

作用：用新程序替换当前进程映像。

返回值和错误：**成功不返回**；失败返回 -1 并置 `errno`（本实验两组失败路径分别拿到 `ENOENT`）。

要点：它才是释放旧 rootfs 引用的那一步。`envp` 传 `{ NULL }` 时新进程没有任何环境变量——
本实验的两个程序都不读环境变量，所以这是刻意的（避免"环境变量从哪来"的假象）。

### `int statfs(const char *path, struct statfs *buf)`

作用：取该路径所在文件系统的统计信息，其中 `f_type` 是超级块魔数。

常见误用：用 `st_dev`（设备号）判断文件系统——同一个设备可以挂多个不同的文件系统；
用目录里有没有文件判断——那只能证明"文件存在"。

### `int reboot(int cmd)`

作用：`RB_POWER_OFF` 关电、`RB_AUTOBOOT` 重启等。需要 `CAP_SYS_BOOT`。

本实验只用 `RB_POWER_OFF`：让 PID 1 的失败路径干净收尾，不让内核 panic。

## 最小可运行实验

`run.sh` 依次做：交叉编译 → 打包 initramfs（只放 `/init`=stage1 与 `/mnt`）→ 造两份 ext4 镜像
（`root_ok.ext4` 含 `/stage2` 与 `/marker`；`root_no_stage2.ext4` 不含 `/stage2`）→ 五组启动 → 逐组判定。

内核命令行只差一处标记：

```bash
ok            console=ttyAMA0 rdinit=/init panic=1
rootfs-ramfs  console=ttyAMA0 rdinit=/init root=/dev/vda rootfstype=ext4 panic=1
no-move       console=ttyAMA0 rdinit=/init panic=1 swroot=nomove
noroot        console=ttyAMA0 rdinit=/init panic=1        # 差别只在 QEMU 不给 -drive
no-stage2     console=ttyAMA0 rdinit=/init panic=1        # 差别只在镜像里没有 /stage2
```

`swroot=nomove` 由 stage1 自己解析（按空白切词后整词比较，不用 `strstr`，避免前缀误命中）。

## 编译与运行

```bash
cd experiments/BSP_03_SwitchRoot
make                                   # 交叉编译器由 Makefile 内部判定
./run.sh                               # 五组全跑 + 逐组判定；全部通过时退出码 0
./run.sh ok                            # 单组
./run.sh --judge ok logs/ok.log        # 只判定不跑 QEMU（0 通过 / 1 失败 / 2 日志不存在）
```

`Makefile` 里那行 `ifeq ($(origin CC),default)` 不是装饰：GNU make 有内建 `CC=cc`，
`CC ?= arm-linux-gnueabihf-gcc` 永远不会生效，会悄悄编出宿主机架构的二进制。

## 预期现象

`logs/ok.log` 的取证行（内核行夹在中间，这里只摘 `SWROOT|`）：

```text
SWROOT|stage|1|pid=1
SWROOT|mount|devtmpfs|ok
SWROOT|mount|proc|ok
SWROOT|mount|sysfs|ok
SWROOT|root_before|type=tmpfs|magic=0x1021994      ← 切根前：initramfs 的 rootfs（shmem 后端）
SWROOT|cmdline|console=ttyAMA0 rdinit=/init panic=1
SWROOT|mount|newroot|ok
SWROOT|newroot_marker|bsp03-rootfs-marker
SWROOT|move|/dev|ok
SWROOT|move|/proc|ok
SWROOT|move|/sys|ok
SWROOT|chroot|ok
SWROOT|stage|2|pid=1                                ← 第二阶段接管，PID 仍是 1
SWROOT|root_after|type=ext4|magic=0xef53            ← 切根后：真正的判据
SWROOT|dev_type|type=tmpfs|magic=0x1021994          ← devtmpfs 与 tmpfs 同魔数，看不出区别
SWROOT|proc_type|type=proc|magic=0x9fa0
SWROOT|sys_type|type=sysfs|magic=0x62656572
SWROOT|mounts_root|ext4
SWROOT|mounts_dev|devtmpfs                          ← 这一行才证明 /dev 挂的是 devtmpfs
SWROOT|marker|bsp03-rootfs-marker
SWROOT|cmdline_after|console=ttyAMA0 rdinit=/init panic=1
SWROOT|old_init|gone                                ← 新根视角下 /init 不存在
SWROOT|dev_vda|present
SWROOT|stage2|done
```

内核侧对应的四行（同一份日志）：

```text
VFS: Finished mounting rootfs on nullfs
Unpacking initramfs...
Run /init as init process
EXT4-fs (vda): mounted filesystem ... r/w with ordered data mode.
```

`VFS: Finished mounting rootfs on nullfs` 是本内核的根挂载方式：不可变的 `nullfs` 作为 VFS 真根，
可变的 rootfs 叠在它上面（`fs/namespace.c`），因此 `statfs("/")` 看到的是 rootfs 的魔数，不是 nullfs。

## 实测记录

证据：`logs/{ok,rootfs-ramfs,no-move,noroot,no-stage2}.log`，一次 `./run.sh` 全部产出。

### 五组读数

| 用例 | `root_before` | `root_after` | `mounts_dev` | `dev_vda` | 第二阶段 | 判定 |
| --- | --- | --- | --- | --- | --- | --- |
| `ok` | tmpfs `0x1021994` | ext4 `0xef53` | devtmpfs | present | 跑了 | PASS |
| `rootfs-ramfs` | **ramfs `0x858458f6`** | ext4 `0xef53` | devtmpfs | present | 跑了 | PASS |
| `no-move` | tmpfs `0x1021994` | ext4 `0xef53` | **absent**（`/proc` 未搬） | **absent** | 跑了 | PASS |
| `noroot` | tmpfs `0x1021994` | — | — | — | **没跑** | PASS |
| `no-stage2` | tmpfs `0x1021994` | — | — | — | **没跑** | PASS |

关键对照只有两行：

- `ok` vs `rootfs-ramfs`：**同一份 initramfs、同一份镜像，只差命令行里的 `root=`**，
  `root_before` 的魔数就变了。这条证明"切根前是 tmpfs"属于内核行为。
- `ok` vs `no-move`：**只差三行 `MS_MOVE`**，`root_after` 仍是 ext4（根确实换了），
  但 `/dev` 从 devtmpfs 变成 ext4 空目录、`/dev/vda` 消失、`/proc` 不可用（`mounts_dev|absent`）。

### 失败路径的错误码

| 用例 | 出错位置 | 读数 |
| --- | --- | --- |
| `noroot` | stage1 `mount("/dev/vda", NEWROOT, "ext4", ...)` | `errno=2`（`ENOENT`：设备节点不存在） |
| `no-stage2` | stage1 `execve("/stage2", ...)` | `errno=2`（`ENOENT`：新根上没有该文件） |

两组都以 `SWROOT|stage2|skipped` 收尾，QEMU `exit=0`，日志里没有内核 panic。

### 判定器的负向验证

`./run.sh --judge <用例> <日志>` 只读日志、不跑 QEMU。对实测日志的临时副本做变异：

| 变异 | 退出码 | 结论 |
| --- | --- | --- |
| 原日志 | 0 | 通过 |
| 删掉 `dev_vda|present` | 1 | 失败（设备节点必须可见） |
| `mounts_dev|devtmpfs` → `mounts_dev|tmpfs` | 1 | 失败（类型串是唯一能区分 devtmpfs 的证据） |
| `root_after|type=ext4` → tmpfs | 1 | 失败（根没换就不能算通过） |
| 追加一行 `SWROOT|fail|injected` | 1 | 失败（任何失败标记都否决） |
| `no-move` 组里 `dev_type|type=ext4` → tmpfs | 1 | 失败（该组的判定点被改掉） |
| 指向不存在的日志 | 2 | 与"判定失败"区分开 |

## 关键实现点

### 1. `move_mount` 是 glibc 已声明的名字

首版把辅助函数命名为 `move_mount`，编译直接失败：

```text
stage1.c:152:13: error: conflicting types for 'move_mount'; have 'void(const char *)'
/usr/arm-linux-gnueabihf/include/sys/mount.h:301:12: note: previous declaration ... 'int(int, const char *, int, const char *, unsigned int)'
```

`move_mount(2)` 是 Linux 5.2 引入的新挂载 API，glibc 已提供包装。改成 `move_mountpoint()`。
教训：**在内核接口密集的代码里，自定义函数名要先避开 glibc 已导出的符号**；
`-Werror` 让这个冲突在编译期就暴露，而不是等到链接或运行期。

### 2. 读 `/proc/cmdline` 必须在挂上 `/proc` 之后

首版把 `show_file("cmdline", "/proc/cmdline")` 放在 `mount("proc", ...)` 之前，
五组日志里全是 `SWROOT|cmdline|absent`——**看起来像内核没传命令行**，其实是读得太早。
把这一行挪到 `mount proc` 之后，立刻变成完整内容。

这类"证据本身是假的"问题最危险：它会让你去怀疑一个根本没问题的环节。

### 3. 判据按假设写，会被实测直接推翻

首版判据写的是教科书的两个结论：

```text
root_before|type=rootfs          # 假设 f_type = RAMFS_MAGIC
dev_type|type=devtmpfs           # 假设 devtmpfs 有自己的魔数
```

结果 `ok` 组直接 FAIL——两条都缺。排查顺序是"先查内核配置，再查源码"：

1. `grep CONFIG_TMPFS .config` → `CONFIG_TMPFS=y`；
2. `init/do_mounts.c:init_rootfs()` 里 `is_tmpfs` 由命令行决定；
3. `include/uapi/linux/magic.h` 里**已经没有 `DEVTMPFS_MAGIC`**，
   `drivers/base/devtmpfs.c` 的注释写着 "Ops are filled in during init depending on underlying
   shmem or ramfs type"。

于是判据改成按实测写，并额外加了两条能真正区分的证据：`mounts_dev`（类型串）与
`dev_vda`（设备节点）。同时补一组 `rootfs-ramfs` 用例把"取决于命令行"这件事也测出来。

**这不是把判据改松，而是把它改到与事实一致**；负向验证（上表 6 条）确认改完的判据依旧会失败。

### 4. 失败收尾必须是断电，不是 `return`

stage1 与 stage2 都是 PID 1。`return` 会让内核 `panic("Attempted to kill init!")`，
串口上多出一段与实验无关的栈，且 QEMU 状态不明。改成 `sw_out(fail) → sync() → reboot(RB_POWER_OFF)`，
配合 `-no-reboot`，`noroot` / `no-stage2` 两组都是 `exit=0` 且日志干净。

## 常见错误和陷阱

1. **用"目录里有没有文件"判断切根**：initramfs 里本来就有 `/init`，数量法会被自己骗过去。
2. **以为 `chroot` 会卸载旧根**：不会。旧根靠 `execve` 放掉最后一个引用，不 `exec` 就一直占内存。
3. **`chroot` 后不 `chdir("/")`**：`getcwd()` 与相对路径仍指向旧根内部。
4. **用 `MS_REMOUNT` 代替 `MS_MOVE`**：产生两份独立实例，旧根释放时设备节点一起没了。
5. **`MS_MOVE` 的目标目录不存在**：`ENOENT`，而且报错点看起来像"设备有问题"，实际是目录没建。
6. **命令行带 `root=` 与不带，`f_type` 不同**：写判据前先在同一内核上实测，不要照背 `RAMFS_MAGIC`。
7. **把 devtmpfs 的魔数当成可区分标志**：本内核它与 tmpfs 同值，必须读 `/proc/self/mounts`。
8. **PID 1 直接 `return`**：内核 panic，日志被栈淹没。

## 故障排查方法

| 现象 | 先查什么 | 判据 |
| --- | --- | --- |
| 没有 `SWROOT|` 行 | 内核有没有跑到 `Run /init`；`rdinit=` 路径对不对 | `rdinit=/init` 且 initramfs 里有 `/init` |
| `mount|newroot|fail|errno=2` | QEMU 有没有 `-drive`；`/dev/vda` 是否存在 | `ls /dev` 里有无 `vda` |
| `stage2` 没接管 | `exec` 行有没有 `fail`；镜像里是否有 `/stage2` | `execve` 的 `errno` |
| 根换了但 `/dev` 空 | 有没有做 `MS_MOVE`；目标目录是否存在 | `mounts_dev` 与 `dev_vda` |
| `cmdline|absent` | 读命令行时 `/proc` 挂了没有 | 该行在 `mount|proc|ok` 之后 |
| `root_before` 与预期魔数不符 | 命令行有没有 `root=`；`CONFIG_TMPFS` | `init_rootfs()` 的 `is_tmpfs` 条件 |
| QEMU 卡住不退出 | PID 1 有没有停在死循环；是否忘了 `reboot` | 最后一行是不是 `stage2|done` 或 `stage2|skipped` |

## 练习题

### 基础题

1. 把 `MS_MOVE` 换成"在新根上重新 `mount` 一次 devtmpfs"，预测 `mounts_dev` 与 `dev_vda` 会变成什么，
   再验证。哪一条读数最先暴露问题？
2. 去掉 stage2 里的 `chdir("/")`（即保留 stage1 的 `chdir` 但删掉 stage2 的），
   设计一个能观察到差异的取证点。

### 改错题

3. 下面这段"切根"有什么问题？
   ```c
   mount("/dev/vda", "/mnt/newroot", "ext4", 0, "");
   chroot("/mnt/newroot");
   execve("/stage2", argv, envp);
   ```
   指出至少两处，并说明各自的后果。

### 综合题

4. 在 stage2 里加一条证据，证明"`/` 这一挂载实例与 `newroot` 这个名字已经无关"
   （提示：`/proc/self/mounts` 的第一列与第二列）。
5. 设计一组用例，让 `mount("/dev/vda", ...)` 成功但 `root_after` 仍不是 ext4
   （提示：镜像里放一个 `tmpfs` 挂载点，或在 stage1 里覆盖挂一层）。
   说明这组用例想证明"只看一个判据不够"的哪一面。

## 验收标准

- [ ] 能按顺序说出四步，并指出每一步漏掉之后哪条日志读数会变。
- [ ] 能解释 `root_before` 为什么是 tmpfs，以及加 `root=` 之后为什么变成 ramfs。
- [ ] 能解释为什么 devtmpfs 的 `f_type` 不能用来证明"搬过来了"，该用哪条证据。
- [ ] 能跑 `./run.sh` 得到 5/5，并复述 `no-move` 组与 `ok` 组的唯一差别。
- [ ] 能对自己的日志做一次变异，让 `--judge` 按预期失败。

## 延伸阅读

- `init/do_mounts.c` 的 `init_rootfs()`、`rootfs_fs_type`：rootfs 后端如何被命令行决定。
- `drivers/base/devtmpfs.c`：devtmpfs 如何借用 shmem/ramfs 的超级块操作。
- `fs/namespace.c` 里的 `nullfs` 与 `VFS: Finished mounting rootfs on nullfs`。
- `man 2 mount` 的 `MS_MOVE` 段、`man 2 chroot`、`man 2 execve`。
- [`BSP_01_Boot_Chain`](../BSP_01_Boot_Chain/README.md)：zImage / DTB / initramfs 各自负责什么。
- [`BSP_02_UBoot_BlockRoot`](../BSP_02_UBoot_BlockRoot/README.md)：块设备根与 initrd 的取舍。

## 下一步

- **用 busybox 复刻同样的流程**：本环境没装 busybox，stage1/stage2 是手写等价实现；
  装上之后可以对照 `busybox switch_root` 的行为（它会 `umount` 旧根、处理 `-c` 控制台）。
- **`pivot_root` 对照**：`pivot_root` 会把旧根搬到指定目录再允许 `umount`，
  与 `chroot + exec` 的取舍值得单独一组实验。
- **坏 bootargs 用例**（第 20 天遗留第 3 条）：需要一个"改了 `bootargs` 仍能引导"的组合，
  才能把"bootloader 参数错"与"内核配置错"分开。

## 已知限制

- **不是真正的 `switch_root`**：没有 `umount` 旧根（当前是 PID 1，旧根上还有本进程的页，
  `umount` 会 `EBUSY`）。真实 `switch_root` 靠 `exec` 之后由新进程回收，本实验只验证到 `exec` 这一步。
- **stage1/stage2 是手写程序**，不是 busybox；因此"`switch_root` 的实际实现细节"（如控制台设备
  的重定向、`/run` 的准备）没有覆盖。
- **无 busybox、无完整用户态**：没有 init 系统、没有 `/sbin/init`，所以只验证了内核与根切换本身。
- **`f_type` 的结论绑定本内核**（7.0.0 树）。换内核对 `root_before` 与 `/dev` 的魔数都可能不同，
  重跑一次 `./run.sh` 就能重新标定。
- **未测 `pivot_root`、未测只读根与叠加挂载**：留给后续实验。
- **QEMU 侧无盘 I/O 时序证据**：ext4 的读写性能与中断行为不在本实验范围。
