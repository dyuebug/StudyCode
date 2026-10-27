# BSP_04：`pivot_root` 与 `chroot + exec` 的同口径对照

> 结论先行（全部为本机实测，非引用）：
>
> 1. **作用域不同**是两者唯一能在进程**之外**观测到的差别：`chroot` 之后，fork 出的观察者仍看到
>    `tmpfs` 与 `/init`（命名空间没动）；`pivot_root` 之后，观察者看到 `ext4` 且 `/init` 消失
>    （整个命名空间一起换根）。在进程**内部**，两者的读数完全一样。
> 2. `pivot_root` **不搬挂载点**：跳过 `MS_MOVE` 的那组里，`/dev` 里没有任何设备节点
>    （`dev_vda=absent`），而根确实换成了 ext4。
> 3. `new_root` **必须是挂载点**：对 ext4 上的普通目录 `/sub` 调 `pivot_root` 得到 `EINVAL(22)`，
>    失败后观察者仍看到旧根 —— 失败不会留下半个状态。
> 4. 旧根**严格 `umount` 恒 `EBUSY(16)`**，`exec` 之后也一样。**原先"是 stage1 的可执行映射钉住了
>    旧根"的假设被实测否证**。真正的直接原因是：内核在 exec init 之前把 `rootfs` 里的
>    `/dev/console` dup 到 **0/1/2**（`init/main.c:1603`），这三个 fd 跟着进程走、`exec` 换不掉，
>    而 `/dev/console` 这个 inode 就在旧 rootfs 上（由 `init/noinitramfs.c:18` 的
>    `default_rootfs()` 创建，mode `020600`，本实验有探针读数）。同一次运行里**只**把 0/1/2
>    重绑到新根的 `/dev/console`，同一挂载立刻就能严格卸载 —— 因果由此坐实。
> 5. 由此得出实践结论：自己写 switch_root 时**要么重绑 stdio，要么直接用 `MNT_DETACH`**；
>    不要用"严格 `umount` 成功"当作切根完成的判据。
> 6. 另有一条反直觉事实：**从 `chroot` 内部读 `/proc/self/mounts`，旧根那一行被内核隐藏**
>    （`mounts_rootfs|0`），所以挂载表**不能**用来判断"chroot 有没有释放旧根"。
>    本实验把它写成了显式断言，防止日后有人拿它当证据。

## 学习目标

1. 说清 `chroot` 与 `pivot_root` 的作用域差别，并指出这个差别为什么只能在进程外观测。
2. 用观察者（fork + 延时）把"命名空间换没换根"变成一条可机器判定的读数。
3. 用一组反例证明 `pivot_root` 不搬挂载点，另一组证明 `new_root` 必须是挂载点。
4. 把 `EBUSY(16)` 从"残留状态"的猜测，追到内核函数与具体的 fd。
5. 设计一个单变量因果实验（只改 stdio 绑定），而不是用两次不同的运行去"大致说明"。
6. 记录一次"假设被自己的实验推翻"的完整过程，并给出改写后的判据与负向验证。

## 环境与前置条件

| 项 | 值 |
| --- | --- |
| 机器 | QEMU `-M virt`（ARMv7，512 MB，单核），virtio-blk 提供 `/dev/vda` |
| 内核 | `~/kbuild/linux-source-7.0.0` 构建的 `arch/arm/boot/zImage`，`Linux version 7.0.14`（实测打印） |
| 内核配置（实测） | `CONFIG_DEVTMPFS=y`、`CONFIG_DEVTMPFS_MOUNT` **未设**、`CONFIG_BLK_DEV_INITRD=y`、`CONFIG_TMPFS=y`、`CONFIG_EXT4_FS=y` |
| 用户态 | 无 busybox，`stage1`/`stage2` 都是手写静态 C 程序 |
| 编译器 | `arm-linux-gnueabihf-gcc` 15.2.0，`-O2 -g -Wall -Wextra -Werror -static`，零警告 |
| 工具 | `mke2fs`（ext4 镜像）、`cpio` + `gzip`（initramfs）、`qemu-system-arm` 10.2.1 |
| 前置 | [`BSP_03_SwitchRoot`](../BSP_03_SwitchRoot/README.md)（`switch_root` 四步与 `chroot + exec` 基线） |

产物体积（实测）：

| 文件 | 大小 |
| --- | --- |
| `stage1`（initramfs 的 `/init`） | 511332 B |
| `stage2`（真实根的 `/stage2`） | 510080 B |
| `build/initramfs.cpio.gz` | 252159 B |
| `build/root_ext4.img` | 8388608 B（8 MiB） |

initramfs 里**只有** `/init` 与 `/mnt` 两项。这一点很重要：`/dev/console` 不是我们放的（见
"核心概念 5"）。

## 核心概念

### 1. 作用域：进程 vs 命名空间

`chroot(path)` 只改**当前进程** `fs_struct` 里的 `root`。挂载表、其它进程、监听中的文件描述符
一概不动。所以：

- 进程内部看起来"根换了"（`/` 是 ext4）；
- 但旧根仍挂在命名空间里，只是本进程的路径解析不再经过它 —— 而且**它在 `/proc/self/mounts` 里也
  被内核藏掉了**（见第 6 节）。

`pivot_root(new, put_old)` 动的是**挂载命名空间**：内核把 `new` 挂到原来根的位置，把旧根搬到
`put_old`，并把**所有**"根指向旧根"的进程的 `fs->root` 一起改到新根（`chroot_fs_refs()`）。

结论：想证明"换的是命名空间而不是进程视角"，必须在**切换之前** fork 出一个进程，让它切换之后
再看自己的 `/`。这就是 `stage1.c::spawn_watcher()` 存在的唯一理由。

### 2. `pivot_root` 的三条门槛

按 `fs/namespace.c::path_pivot_root()`（本内核 4633 行起）的检查顺序，最常踩的三条是：

1. `new_root` 必须是**挂载点**（`path_mounted(new)`），普通目录直接 `EINVAL(22)`；
2. `put_old` 必须**可达且在 `new_root` 之下**（`is_path_reachable(old_mnt, ..., new)`）；
3. `put_old` 不能与当前根在**同一个文件系统**上（`new_mnt == root_mnt || old_mnt == root_mnt` →
   `EBUSY(16)`）。这也是为什么标准的 put_old 写成 `NEWROOT"/oldroot"` 而不是 "/oldroot"。

另外：`new_root` 也不能是 `MNT_LOCKED`，两端的父挂载不能是 shared。本实验全程单命名空间、无共享
挂载，所以这几条不在测量范围内（见"已知限制"）。

### 3. `MS_MOVE` 是"搬"，不是"再挂一次"

`mount("/dev", "/mnt/newroot/dev", NULL, MS_MOVE, NULL)` 之后，**旧位置立刻不可访问**，挂载实例
本身（设备号、inode、`mnt_id`）不变。`pivot-nomove` 组跳过这三步，结果：

```text
PROOT|move|/dev|skipped
PROOT|pivot_root|ok|new=/mnt/newroot|put=/mnt/newroot/oldroot
PROOT|dev_type|type=ext4|magic=0xef53     ← /dev 只是 ext4 上的空目录
PROOT|dev_vda|absent                       ← 设备节点没了
```

这条对照的意义：`pivot_root` 换的是"根"，不是"挂载点名册"。换根成功不等于新根可用。

### 4. `EBUSY` 只有两个来源

`umount2(path, 0)` 走到 `do_umount()`（`fs/namespace.c:1864`），真正会返回 `EBUSY` 的只有最后一处：

```c
retval = -EBUSY;
if (!propagate_mount_busy(mnt, 2)) {   /* fs/namespace.c:1954 */
        umount_tree(mnt, UMOUNT_PROPAGATE|UMOUNT_SYNC);
        retval = 0;
}
```

而 `propagate_mount_busy()`（`fs/pnode.c:423`）只判两件事：

```c
if (!list_empty(&mnt->mnt_mounts) || do_refcount_check(mnt, refcnt))
        return 1;
```

即 **① 有子挂载** 或 **② 引用计数超过"树内一份 + 本次调用一份"**。所以"旧根卸不掉"要先分清
是哪一类，本实验用两条读数把它们分开：

- `mountinfo` 的 `parent` 列能直接数出 ① 是否成立（实测：没有任何挂载以旧根 id 为父）；
- `/proc/self/fd` 的 `readlink` 能直接看到 ② 的持有者（实测：`0/1/2 → /oldroot/dev/console`）。

### 5. 那只 `fd` 是从哪来的（本实验最关键的一条链）

```text
init/noinitramfs.c:18   default_rootfs()  ← rootfs_initcall，obj-y 恒编译
                        在 rootfs 里建 /dev (0755)、/dev/console (chr 5:1, 0600)、/root
init/main.c:1603        console_on_rootfs()
                        filp_open("/dev/console") 后 init_dup() ×3 → fd 0/1/2
execve("/init")         只换映像，不动 fd 表
stage1/stage2           继承这三个 fd，它们仍是旧 rootfs 上的那个 inode
```

本实验在 `stage1` 入口加了一行探针，读数与源码逐位吻合：

```text
PROOT|boot_console|present|mode=020600
```

`020600` = `S_IFCHR|0600`，正是 `default_rootfs()` 里 `init_mknod()` 的参数。这条探针的作用是
把"旧根里的 `/dev/console` 是内核建的、不是我们 cpio 里带的"钉死，否则因果链缺一环。

### 6. 本内核的根布局：nullfs 在下面，rootfs 在上面

启动日志里有：

```text
VFS: Finished mounting rootfs on nullfs
```

`fs/namespace.c:6142 init_mount_tree()` 建两个挂载：`nullfs`（`mnt_id` 1）与 `rootfs`（`mnt_id` 2），
后者挂在前者之上；命名空间的根是 nullfs，所有进程的 `fs->root` 指向 rootfs。`pivot-late` 的
`mountinfo` 读数正好对上：

```text
PROOT|mountinfo|2|20|/oldroot     ← 旧 rootfs，保留了启动时的 mnt_id 2，父是 20
PROOT|mountinfo|20|1|/            ← ext4 新根，父是 1（nullfs）
```

这两行同时说明：`pivot_root` 之后新根是 **nullfs 的孩子**，旧 rootfs 变成新根的孩子 —— 拓扑上
"旧根在 `/oldroot`"，不是"旧根消失了"。

### 7. 反直觉：`chroot` 下挂载表看不见旧根

`chroot` 组的 `mounts_rootfs|0`、`mounts_count|4`，与 `pivot & umount` 组**完全一样**。
机制是 `/proc/self/mounts` 的 `show_vfsmnt()` 会跳过不可达的挂载（`is_path_reachable()`），
而 chroot 之后本进程的 root 已经不在旧根那一支上了。

所以：**挂载表不能用来判断 chroot 有没有释放旧根**。旧根是否可达，`pivot-late` 的卸载前读数是
可用的对照（那时它挂在 `/oldroot`，表里就有 `PROOT|mounts|0|/oldroot|rootfs`，`mounts_rootfs|1`）。

## 接口卡片

### `long syscall(SYS_pivot_root, const char *new_root, const char *put_old)`

作用：把当前挂载命名空间的根切到 `new_root`，旧根挂到 `put_old`。

原型：glibc **没有**包装函数（不是 POSIX），只能走 `syscall()`。

参数：`new_root` 必须是挂载点；`put_old` 必须是 `new_root` 之下已存在的目录，且与当前根不同
文件系统。

返回值和错误：`0` 成功；`-1` 置 `errno`：`EINVAL(22)`（不是挂载点 / `put_old` 不可达 /
`MNT_LOCKED` / 绝对根 / shared 挂载）、`EBUSY(16)`（`put_old` 与当前根同一文件系统）、
`ENOENT(2)`（`new_root` 已被 unlink）、`EPERM`（无 `CAP_SYS_ADMIN`）。

资源所有权：成功即刻改变命名空间，不需要额外的"提交"动作。

常见误用：`put_old` 用 `/oldroot`（在当前根上，与 `new_root` 不同挂载但同属旧根那一支，本实验
的失败面就是 `EBUSY`/`EINVAL` 之一）；忘记 `chdir("/")`，于是 cwd 仍指向旧根内部。

### `int mount(const char *src, const char *tgt, const char *fs, unsigned long flags, const void *data)`

本文只关心 `MS_MOVE`：`fs` 与 `data` 必须为 `NULL`，`src` 必须是已存在的挂载点，`tgt` 必须是
已存在的目录。搬运后旧位置立刻不可访问。

常见误用：把 `MS_MOVE` 当成"复制挂载"；`tgt` 不存在（`ENOENT`）；对非挂载点 `src` 操作（`EINVAL`）。

### `int chroot(const char *path)`

作用：改**本进程**的 `fs->root`。不卸载、不改挂载表、不影响其它进程。

返回值和错误：`0` / `-1`（`ENOENT`、`EPERM` 无 `CAP_SYS_CHROOT`）。

常见误用：以为 `chroot` 之后旧根就没了（见核心概念 1、7）；`chroot` 后不 `chdir("/")`。

### `int umount2(const char *target, int flags)`

作用：`flags=0` 严格卸载（有引用就失败）；`MNT_DETACH` 惰性摘除（先从命名空间拿掉，最后引用
释放时再真正销毁）。

返回值和错误：`0`；`EBUSY(16)`（子挂载或引用计数，见核心概念 4）；`EINVAL(22)`（`MNT_LOCKED`、
不是挂载点、**目标已经不是挂载**）；`EPERM`。

本实验的一个自洽细节：严格卸载成功之后，兜底的 `umount2("/oldroot", MNT_DETACH)` 返回
`errno=22` —— 因为那时它已经**不是挂载**了。这条"失败"反而是"前面真的卸掉了"的证据。

### `int statfs(const char *path, struct statfs *buf)`

作用：取路径所在文件系统的统计信息，`f_type` 是超级块魔数。

本实验实测到的魔数：

| 文件系统 | `f_type` |
| --- | --- |
| ext4 | `0xef53` |
| tmpfs / devtmpfs / rootfs(shmem) | `0x01021994` |
| proc | `0x9fa0` |

常见误用：用 `f_type` 区分 devtmpfs 与 tmpfs —— **它们同魔数**（devtmpfs 以内核 shmem 为后端）。
要分就查 `/proc/self/mounts` 的第三列类型串。

### `/proc/self/mountinfo` 的前五列

```text
id  parent  major:minor  root  mountpoint  ...
```

`id` / `parent` 是挂载实例之间的父子关系，**跨运行稳定的是拓扑、不是具体数值**（本实验 `20`/`2`
这类编号每次启动可能不同，判据里不许写死）。

### `readlink("/proc/self/fd/N")`

作用：给出该 fd 所指文件的**当前路径**。它把"哪个 fd 钉住了哪个挂载"变成可读的证据，本实验靠它
拿到 `0/1/2 → /oldroot/dev/console`。注意这是**路径**证据，不是引用计数证据：路径相同不代表
count 一定超限，必须配合卸载结果一起判。

## 最小可运行实验

拓扑（`pivot` 组，切根前后）：

```text
切换前                                    切换后
─────────────────────────────            ─────────────────────────────
nullfs (id 1)                            nullfs (id 1)
 └── rootfs (id 2)   ← fs->root            └── ext4 (id 20, 新根)
      ├── /init                                  ├── /stage2  /marker
      ├── /mnt/newroot → ext4                    ├── /dev  (devtmpfs, MS_MOVE)
      │     ├── /dev (devtmpfs, MS_MOVE)         ├── /proc (proc,    MS_MOVE)
      │     ├── /proc (proc,   MS_MOVE)          ├── /sys  (sysfs,   MS_MOVE)
      │     └── /sys  (sysfs,  MS_MOVE)          └── /oldroot → rootfs
      └── /dev /proc /sys  (空目录)                   (旧根仍被 0/1/2 钉住)
```

五组只差内核命令行里的一个参数，其余（`stage1`/`stage2`/initramfs/ext4 镜像）完全相同：

| 组 | `proot=` | 与基准的差别 | 想证明什么 |
| --- | --- | --- | --- |
| `chroot` | `chroot` | `chroot` + `chdir` + `exec` | 基线：只改进程视角 |
| `pivot` | `pivot` | `pivot_root` + `chdir`，stage1 里卸旧根 | 命名空间换根；严格卸载失败后用 DETACH |
| `pivot-late` | `pivot-late` | 不 fork 观察者，卸载交给 stage2 | 旧根被谁钉住（因果实验） |
| `pivot-nomove` | `pivot-nomove` | 跳过三次 `MS_MOVE` | 换根不搬挂载点 |
| `pivot-ondir` | `pivot-ondir` | 对 `/mnt/newroot/sub` pivot | `new_root` 必须是挂载点 |

## 编译与运行

```bash
# 全部五组（交叉编译 → 打包 initramfs → 造 ext4 → 逐组跑 QEMU → 逐组机器判定）
./run.sh

# 单组
./run.sh pivot-late

# 只判定已有日志（用于对日志做变异验证判据，不重跑 QEMU）
./run.sh --judge pivot-late logs/pivot-late.log
echo "rc=$?"        # 0 通过 / 1 判定失败 / 2 日志不存在
```

前置：`qemu-system-arm`、`arm-linux-gnueabihf-gcc`、`mke2fs`、`cpio`、`gzip`，以及已构建的
ARMv7 内核树（`KERNEL_TREE` 可覆盖，默认 `~/kbuild/linux-source-7.0.0`）。

日志是取证材料，随仓库入库（`logs/*.log`）；`build/`、`stage1`、`stage2` 不入库。

## 预期现象

`./run.sh` 的判定输出（实测）：

```text
   [PASS] chroot
   [PASS] pivot
   [PASS] pivot-late
   [PASS] pivot-nomove
   [PASS] pivot-ondir

合计：5 组通过，0 组失败
```

关键读数（`logs/pivot-late.log` 原文）：

```text
PROOT|boot_console|present|mode=020600
PROOT|mounts|0|/oldroot|rootfs          ← 卸载前：旧根可达，在表里
PROOT|mounts_count|5
PROOT|mounts_rootfs|1
PROOT|fd|1|/oldroot/dev/console         ← stdio 指向旧根里的 console
PROOT|late_umount_keep_stdio|busy|errno=16
PROOT|ctrl|tmpfs|umount_ok              ← 对照组：严格卸载本身可用
PROOT|stdio_rebind|ok
PROOT|fd|1|/dev/console                 ← 唯一变量改完了
PROOT|late_umount_after_rebind|ok       ← 同一挂载，严格卸载成功
PROOT|late_umount_detach|fail|errno=22  ← 已经卸掉，DETACH 无事可做
PROOT|mounts_count|4
PROOT|mounts_rootfs|0
```

## 实测记录

### 五组读数

| 组 | 观察者看到的 `/` | 观察者的 `/init` | 旧根处理 | 其它关键读数 |
| --- | --- | --- | --- | --- |
| `chroot` | `tmpfs 0x1021994` | `visible` | 没动 | `mounts_rootfs=0`、`mounts_count=4`、`dev_vda=present` |
| `pivot` | `ext4 0xef53` | `gone` | `umount_oldroot\|busy\|errno=16` → `umount_oldroot_detach\|ok` | `mounts_root=ext4`、`mounts_dev=devtmpfs`、`mounts_rootfs=0` |
| `pivot-late` | —（无观察者） | — | 见上：`busy(16)` → 重绑 stdio → `ok` | 卸载前 `mounts_count=5`/`rootfs=1`，卸载后 `4`/`0` |
| `pivot-nomove` | `ext4 0xef53` | `gone` | `busy(16)` → `detach|ok` | `move|/dev|skipped`、`dev_type=ext4`、`mounts_dev=absent`、`dev_vda=absent` |
| `pivot-ondir` | `tmpfs 0x1021994` | `visible` | 没发生 | `pivot_root\|fail\|...\|errno=22`、`stage2\|skipped` |

`pivot-ondir` 的那两条观察者读数同时证明：**失败路径没有留下半个状态**（根没换、旧根还在）。

### 判定器的 10 条负向验证

全部在**同一份日志的临时副本**上做（`/tmp`），不重跑 QEMU，确认判据没有被改松：

| # | 变异 | 期望 | 实测 `rc` |
| --- | --- | --- | --- |
| 1 | `late_umount_keep_stdio\|busy\|errno=16` → `\|ok` | 失败 | 1 |
| 2 | 删掉 `stdio_rebind\|ok` | 失败 | 1 |
| 3 | `late_umount_after_rebind\|ok` → `\|busy` | 失败 | 1 |
| 4 | 重绑后的 `fd\|1\|/dev/console` 改成 `/oldroot/dev/console` | 失败 | 1 |
| 5 | 删掉 `ctrl\|tmpfs\|umount_ok`（对照组） | 失败 | 1 |
| 6 | 删掉卸载前的 `fd\|1\|/oldroot/dev/console` | 失败 | 1 |
| 7 | 删掉 `mounts_rootfs\|1`（卸载前有旧根） | 失败 | 1 |
| 8 | 日志不存在 | `rc=2` | 2 |
| 9 | 未知用例名 | `rc=1` | 1 |
| 10 | 原日志（不变异） | 通过 | 0 |

第 4 条尤其重要：`PROOT|fd|1|/oldroot/dev/console` **确实包含**子串 `/dev/console`，如果判据写成
`fd|1|/dev/console` 就会被它蒙混过去。判据必须带上前面的 `/oldroot` 或写成整行匹配。

### 被推翻的假设

| 假设 | 检验方式 | 实测 | 结论 |
| --- | --- | --- | --- |
| 旧根 `EBUSY` 是因为 stage1 的可执行映射还在里面，`exec` 之后就能严格卸载 | `pivot-late` 组：stage1 不卸，stage2 卸 | `late_umount_keep_stdio\|busy\|errno=16` | **否证**：exec 之后依旧忙 |
| 旧根有子挂载导致 `EBUSY` | 数 `mountinfo` 的 `parent` | 没有挂载以旧根 id 为父 | 否证 |
| 是某个 libc/脚本留下的 fd | 读 `/proc/self/fd` | 只有 0/1/2（console）与 3（读 fd 目录本身） | 否证 |
| 本命名空间里严格卸载根本不可用 | 现挂一个 tmpfs 再严格卸载 | `ctrl\|tmpfs\|umount_ok` | 否证 |
| 继承的 0/1/2 是唯一剩余的引用 | 只重绑 0/1/2，其它不动 | `late_umount_after_rebind\|ok` | **成立** |

## 关键实现点

### 1. 观察者必须在切换**之前** fork，并且要等得到

`chroot` 与 `pivot_root` 在第二阶段的所有读数完全一样（`/` 是 ext4、`/dev` 是 devtmpfs、
旧 `/init` 看不到、挂载表也一样）。唯一差别在进程之外，所以 `stage1` 在切换前 `fork`，子进程
`nanosleep(100ms)` 后打印自己的 `/`。

`stage2` 开头 `nanosleep(800ms)`、失败路径 `nanosleep(400ms)` 后再断电，都是为了让观察者那两行
不被 `reboot(RB_POWER_OFF)` 截断。**这是取证需求，不是"等一等比较稳"**：`pivot-ondir` 组在
`pivot_root` 失败后立刻断电，没有这 400 ms 就丢掉了"失败没有改状态"的证据。

### 2. 判据只写不变量，不写具体编号

`mnt_id` 每次启动都会变（本实验里 `2`/`20` 是这一次的编号），`mounts` 的行序也不稳定。判据只写：

- `mounts_count` 从 5 → 4、`mounts_rootfs` 从 1 → 0（同一份日志内的一对）
- `fd|1|` 的前后两个取值
- `busy(16)` → `ok` 的顺序

写死 `20`、`2` 这类编号的判据，下一次启动就会假失败。

### 3. 因果实验的标准形状：同进程、同对象、只改一个变量

`pivot-late` 组不是"另外跑一次看看能不能卸"，而是在**同一次运行**内：

```text
卸载 #1（带继承 stdio） → 失败 EBUSY
重绑 0/1/2             → 唯一变量
卸载 #2（同一挂载）     → 成功
```

两次调用之间没有 `exec`、没有 `chdir`、没有别的清理。这样"是 stdio 钉住旧根"才是从证据推出来的，
而不是从两个不同的运行里拼接出来的。判据也按这个形状写：`busy` → `rebind ok` → `ok` 三条缺一不可。

### 4. `MNT_DETACH` 的"失败"可能是好消息

严格卸载已经成功时，再 `umount2(..., MNT_DETACH)` 会得到 `EINVAL(22)`。本实验把它也打出来并
写进判据（`late_umount_detach|fail|errno=22`），因为它正面说明"旧根真的不在挂载表里了"。
判据同时要求 `mounts_rootfs|0`，两条互相印证。

### 5. `-Werror` 下也要写清缓冲上界

`snprintf(path, sizeof path, "/proc/self/fd/%s", de->d_name)` 在 `-Werror` 下触发
`-Wformat-truncation`（`d_name` 最长 255 字节）。改成 320 字节并注明理由，而不是关掉该警告。

### 6. 失败收尾断电，不 `return`

`stage1` 是 PID 1，`return` 会 panic 出一大段无关栈，把证据淹掉。所有失败路径统一走
`fail_and_poweroff()`：先打 `PROOT|fail|<where>|errno=<n>`，留 400 ms 窗口，再 `sync` + 断电。

## 常见错误和陷阱

| 现象 / 写法 | 原因 | 正确做法 |
| --- | --- | --- |
| `pivot_root` 返回 `EINVAL(22)` | 对普通目录 pivot | `new_root` 必须是挂载点 |
| `pivot_root` 返回 `EBUSY(16)` | `put_old` 与当前根同一文件系统 | `put_old` 放在 `new_root` 之下 |
| 切根成功了，但 `/dev` 里没有设备 | 忘了 `MS_MOVE` | 换根前把 `/dev`、`/proc`、`/sys` 搬进新根 |
| `umount` 旧根恒 `EBUSY` | 0/1/2 指向旧根里的 `/dev/console` | 重绑 stdio，或用 `MNT_DETACH` |
| `umount` 旧根 `EBUSY`，且 `getcwd` 仍在旧根 | 切根后没 `chdir("/")` | 切完立刻 `chdir("/")` |
| 用 `/proc/self/mounts` 判断 chroot 有没有释放旧根 | 内核隐藏不可达的挂载 | 用进程外的观察者 |
| 用 `f_type` 区分 devtmpfs 与 tmpfs | 两者同魔数 `0x01021994` | 查 mounts 的类型串 |
| 判据里写死 `mnt_id` / 行序 | 跨运行会变 | 只写不变量与先后关系 |

## 故障排查方法

1. **先定错误码**：`22` 看"是不是挂载点 / `put_old` 位置"；`16` 看"引用或子挂载"；`19(ENODEV)`
   说明该文件系统类型不允许从用户态挂载（本实验对 `rootfs` 做对照时就是这个）。
2. **`EBUSY` 先分两类**：`/proc/self/mountinfo` 的 `parent` 列数子挂载；`/proc/self/fd` 找持有者。
   两者都没有时，再怀疑"另一个进程的 root/cwd"。
3. **别信进程内读数**：`chroot` 与 `pivot_root` 在进程内不可区分，必须用进程外观察者。
4. **判据先跑正例再跑负例**：对日志做变异，确认每一条断言都能单独失败；否则判据只是"没报错"。

## 练习题

### 基础题

1. 把 `pivot` 组的 `chdir("/")` 去掉（`pivot_root` 之后不 chdir），预测 `umount_oldroot` 的结果，
   并在 `stage2` 里加一条能证明"cwd 钉住旧根"的读数（提示：`/proc/self/cwd`）。
2. `pivot-late` 组重绑 stdio 之后如果**不**卸载，`mounts_rootfs` 应该是几？说明为什么"重绑"本身
   不会改变挂载表。

### 改错题

3. 下面这段"切根"有什么问题？至少指出三处，并说明各自的后果。
   ```c
   mount("/dev/vda", "/mnt/newroot", "ext4", 0, "");
   mount("/dev", "/mnt/newroot/dev", NULL, MS_MOVE, NULL);
   syscall(SYS_pivot_root, "/mnt/newroot", "/oldroot");
   execve("/stage2", argv, envp);
   ```
4. 下面这条判据看似严格，实际可能假通过，指出原因并给出改法：
   ```bash
   grep -q 'PROOT|fd|1|/dev/console' "$log"
   ```

### 综合题

5. 设计一组用例，让**旧根有子挂载**从而 `EBUSY`（不借助 stdio），并用 `mountinfo` 的 `parent`
   列给出判定。要求：`ctrl|tmpfs|umount_ok` 仍成立，即"严格卸载本身可用"。
6. 把 `pivot-late` 改成"重绑 stdio 到 `/dev/null` 而不是 `/dev/console`"，预测输出会怎样变化，
   再验证。这条用例想说明"取证程序自己不能依赖被它研究的对象"。

## 验收标准

- [ ] 能说明 `chroot` 与 `pivot_root` 的作用域差别，并解释为什么只能用进程外观察者证明。
- [ ] 能背诵 `pivot_root` 的三条门槛，并说出违反时各自的 `errno`。
- [ ] 能讲清 `EBUSY` 的两个来源，并给出区分它们的两条读数。
- [ ] 能复述 `0/1/2 → /oldroot/dev/console` 这条链，包括节点由谁创建、fd 由谁 dup。
- [ ] 能跑 `./run.sh` 得到 5/5，并复述五组之间"只差一个命令行参数"。
- [ ] 能对自己的日志做至少三条变异，让 `--judge` 按预期失败。
- [ ] 能解释为什么 `late_umount_detach|fail|errno=22` 是"卸载成功"的证据而不是问题。

## 延伸阅读

- `fs/namespace.c::path_pivot_root()`（7.0.0 树 4633 行起）：三条门槛的具体检查顺序。
- `fs/namespace.c::do_umount()`（1864 行起）与 `fs/pnode.c::propagate_mount_busy()`（423 行起）：
  `EBUSY` 的唯一来源。
- `fs/namespace.c::init_mount_tree()`（6142 行起）：`nullfs` + `rootfs` 的双层根布局。
- `init/noinitramfs.c::default_rootfs()`（18 行起）与 `init/main.c::console_on_rootfs()`（1603 行起）：
  `/dev/console` 与 0/1/2 的来源。
- `Documentation/filesystems/ramfs-rootfs-initramfs.rst`：rootfs 与 nullfs 的设计说明。
- `fs/super.c` / `may_umount_tree()`：为什么"树内一份 + 调用一份"的基线是这么定的。
- [`BSP_03_SwitchRoot`](../BSP_03_SwitchRoot/README.md)：`chroot + exec` 基线与 `switch_root` 四步。
- [`BSP_01_Boot_Chain`](../BSP_01_Boot_Chain/README.md)：zImage / DTB / initramfs 各自负责什么。

## 下一步

- **完整的 `switch_root` 等价实现**：在 `stage2` 里重绑 `/dev/console`、切到只读、再 `exec` 真正的
  init，把本实验的结论直接落成一个可复用函数。
- **`MS_SHARED` 传播**：本实验全程单命名空间、无共享挂载；`path_pivot_root()` 对 shared 挂载直接
  `EINVAL`，这条件值得单独一组反例。
- **只读根与叠加挂载**：`pivot_root` 之后把旧根设为只读、在上面挂 overlay，属于真实 BSP 的常见形态。
- **`f_type` 与类型串的分工**：把 BSP_03/04 两处"魔数骗人"的场合合并成一张速查表。

## 已知限制

- **QEMU 单核、无真实硬件**：没有中断时序、没有盘 I/O 延迟证据；结论只覆盖本内核与本配置。
- **无 busybox**：`stage1`/`stage2` 是手写程序，因此"真实 `switch_root` 的行为"（例如
  busybox 的 `-c` 控制台处理、`/run` 准备）没有覆盖。
- **没有测多进程/多命名空间**：观察者只有一个 fork 出的子进程；容器式嵌套命名空间不在范围内。
- **没有测共享挂载与 `MNT_LOCKED` 路径**：源码里相关分支存在，但本实验一组反例都没覆盖。
- **`mnt_id` 与行序不稳定**：判据只使用不变量；如需引用编号，只能引用同一份日志内的相对关系。
- **`rootfs` 类型无法从用户态再挂一个做对照**：`mount("rootfs", ...)` 返回 `ENODEV(19)`，
  所以"整类被钉住"与"这一实例被钉住"只能靠 0/1/2 的重绑实验间接区分。
- **结论绑定内核 7.0.14**：换内核后 `nullfs` 布局、`default_rootfs()` 行为都可能变化，
  重跑 `./run.sh` 即可重新标定（判据里的 `boot_console|present|mode=020600` 是最先会失败的一条）。
