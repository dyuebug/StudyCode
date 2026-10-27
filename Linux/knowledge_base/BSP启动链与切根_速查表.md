# BSP 启动链与切根：速查表

**学习目标**：把"内核启动到执行真实根上的 init"拆成可分别验证的环节，并知道每个环节错了会留下什么证据。  
**适用范围**：ARMv7 + initramfs + 块设备真实根；实测环境 QEMU `-M virt` + Linux 7.0.0 树。  
**验证状态**：切根四步与三条失败路径**实测**；u-boot `extlinux`/`bootflow`、`pivot_root`、真板启动链为**未验证**。

## 1. 三件套各自负责什么

| 件 | 由谁提供 | 负责 | 少了会怎样 |
| --- | --- | --- | --- |
| `zImage` | `-kernel` 或 bootloader 加载 | 内核本身 | 什么都不启动 |
| DTB | `-dtb` / 由 bootloader 传入 | 机器描述（内存、串口、`/chosen`） | 常在串口初始化前就停住，一个字符都没有 |
| initramfs | `-initrd` / bootloader 的 ramdisk | 早期用户态（`rdinit`） | 内核找不到 `rdinit` 时回落去挂 `root=` |

证据点：`Unpacking initramfs...`、`Run /init as init process`、`VFS: Mounted root ...`。
判"跑的是不是我传的那份 DTB"看 `/sys/firmware/devicetree/base/model` 与自定义节点，
不要看 `-dtb` 参数本身。

## 2. 两阶段启动：initramfs → 真实根 → `switch_root` 四步

```text
① 建运行期基础树   mount devtmpfs /proc /sys
② 挂新根           mount("/dev/vda", "/mnt/newroot", "ext4", 0, "")
③ 搬运挂载点       mount("/dev", "/mnt/newroot/dev", NULL, MS_MOVE, NULL)   ×3
④ 换根 + 换进程    chroot("/mnt/newroot") → chdir("/") → execve("/stage2", ...)
```

| 步 | 漏掉的后果 | 日志里怎么看出来 |
| --- | --- | --- |
| ① | 连 `/dev/vda` 节点都不存在，②直接 `ENOENT` | `mount|newroot|fail|errno=2`，`/dev` 为空 |
| ② | 后面全崩 | 同上；或挂上但内容不是预期镜像（`newroot_marker` 不符） |
| ③ | 根换了，但 `/dev` 是空目录、`/proc` 不可用、设备节点全丢 | `mounts_dev|absent`、`dev_vda|absent` |
| ④ | 旧 rootfs 永远有引用，内存不回收 | 没有第二阶段日志；`exec` 行报 `errno` |

**为什么③必须 `MS_MOVE` 而不是重新 `mount`**：重新挂载会产生两份独立实例；旧根释放时，
挂在旧根上的那份 devtmpfs 随超级块销毁，设备节点一起消失。`MS_MOVE` 是"搬"，实例只有一个。

**为什么④必须 `exec`**：`chroot()` 只改进程的路径解析起点，不卸载旧根。旧 rootfs 的页仍被
本进程的可执行映像与数据页引用，只有换掉进程映像（`execve`）才放掉最后一个引用。

## 3. 判据：`f_type` 与它的两个坑

`statfs(path).f_type` 是超级块魔数，用户态改不了，也不受目录内容影响。

| 路径 / 场景 | 实测 `f_type` | 说明 |
| --- | --- | --- |
| `/`，命令行**无** `root=` | `0x01021994` tmpfs | rootfs 后端被建成 shmem |
| `/`，命令行**有** `root=` | `0x858458f6` ramfs | rootfs 后端回落到 ramfs |
| `/`（切根后） | `0xef53` ext4 | 新根 |
| `/dev`（`MS_MOVE` 过） | `0x01021994` tmpfs | **devtmpfs 用 shmem 作后端，与 tmpfs 同魔数** |
| `/dev`（未搬运） | `0xef53` ext4 | 新根上的普通空目录 |
| `/proc` / `/sys` | `0x9fa0` / `0x62656572` | proc / sysfs |

两个坑（都已在 `BSP_03` 里实测并写进判据）：

1. **`f_type` 区分不了 devtmpfs 与 tmpfs**。要区分只能读 `/proc/self/mounts` 第三列的类型串。
   所以"`/dev` 搬过来了"的证据是 `mounts_dev` = `devtmpfs` **加上** `/dev/vda` 存在。
2. **"rootfs 就是 `RAMFS_MAGIC`" 不成立**。`init/do_mounts.c:init_rootfs()` 在命令行
   既没有 `root=` 也没有 `rootfstype=` 且 `CONFIG_TMPFS=y` 时走 `shmem_init_fs_context()`。
   `/proc/mounts` 里两种情况都写 `rootfs`，只有 `f_type` 能区分。

**不要用的判据**：目录里有没有文件（initramfs 里本来就放了程序）、`st_dev`（同一设备可挂多个
文件系统）、`du` 出来的用量（与文件系统类型无关）。

## 4. 接口卡片

### `mount(src, tgt, fstype, flags, data)`

- **`MS_MOVE`**：`fstype` 与 `data` 必须为 `NULL`，`src` 必须是已存在的挂载点，`tgt` 必须是已存在的目录。
  成功后**旧位置立刻不可访问**，不需要 `umount`。
- **返回/错误**：-1 置 `errno`；`ENOENT`（设备节点或目标目录不存在）、`EINVAL`（`src` 不是挂载点）、`EBUSY`。
- **误用**：拿 `MS_REMOUNT` 当 `MS_MOVE`；目标目录忘了建。

### `chroot(path)`

- **作用**：改本进程的根。**不改挂载表**，不影响其他进程，不卸载旧根。
- **误用**：`chroot` 之后不 `chdir("/")`（`getcwd()` 与相对路径仍指向旧根内部）；
  以为 `chroot` 等于"切根完成"。

### `execve(path, argv, envp)`

- **成功不返回**；失败返回 -1 并置 `errno`。
- **在切根流程里的作用**：释放旧 rootfs 的最后引用。`envp = { NULL }` 是合法的，
  但新进程就没有任何环境变量了——要么显式传，要么确认程序不读。

### `statfs(path, &buf)` / `reboot(cmd)`

- `statfs` 用于取 `f_type`；路径不存在时返回 -1，正可用来区分"不存在"与"存在但空"。
- `reboot(RB_POWER_OFF)` 需要 `CAP_SYS_BOOT`；PID 1 的失败路径用它收尾，
  避免 `return` 触发内核 panic 把证据淹没。

## 5. 常见错误与调试

| 现象 | 先查什么 | 判据 |
| --- | --- | --- |
| 没有取证行、串口一片空 | DTB / 串口是否初始化；`rdinit=` 路径 | 有没有 `Run /init` |
| `mount` 新根 `errno=2` | QEMU 有没有给盘；`/dev/vda` 是否存在 | `ls /dev` |
| 根换了但设备全丢 | 有没有做 `MS_MOVE` | `mounts_dev` 与 `dev_vda` |
| 第二阶段没接管 | `execve` 的 `errno`；镜像里有没有该文件 | `exec|fail` 行 |
| 读 `/proc/cmdline` 得 absent | 读的时候 `/proc` 挂了没有 | 该行必须在 `mount|proc|ok` 之后 |
| `root_before` 魔数与本表不符 | 命令行有没有 `root=`；`CONFIG_TMPFS` | `init_rootfs()` 的 `is_tmpfs` 条件 |
| QEMU 卡死不退出 | PID 1 是不是停在死循环；有没有 `reboot` | 最后一行是否 `done` / `skipped` |

## 6. 验证状态与边界

- **实测**：四步切根；`ok` / `rootfs-ramfs` / `no-move` / `noroot` / `no-stage2` 五组；
  判据的 6 条负向验证（含"日志不存在"返回 2）。
- **仅源码推断**：`init_rootfs()` 的 `is_tmpfs` 条件（但已用 `rootfs-ramfs` 组落到读数上）、
  `nullfs` 作为 VFS 真根的具体路径。
- **未验证**：`pivot_root` 对照、busybox `switch_root` 的实现差异、真板上的启动链与时序、
  u-boot 的 `extlinux`/`bootflow` 路径。
- **环境绑定**：所有 `f_type` 结论绑定本内核（7.0.0 树）。换内核对 `root_before` 与 `/dev`
  的魔数都可能不同——重跑 `./run.sh` 重新标定，不要照抄。

## 证据来源

- [`experiments/BSP_03_SwitchRoot`](../experiments/BSP_03_SwitchRoot/README.md)：四步切根、五组对照、
  判据与负向验证、两次失败复盘（`move_mount` 符号冲突、`/proc` 读太早）。
- [`experiments/BSP_01_Boot_Chain`](../experiments/BSP_01_Boot_Chain/README.md)：zImage / DTB / initramfs 的分工。
- [`experiments/BSP_02_UBoot_BlockRoot`](../experiments/BSP_02_UBoot_BlockRoot/README.md)：u-boot、块设备根、分区与整盘。
- 日志：`experiments/BSP_03_SwitchRoot/logs/{ok,rootfs-ramfs,no-move,noroot,no-stage2}.log`。
