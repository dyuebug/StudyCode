# BSP_02 u-boot 阶段与块设备根

BSP_01 只走了 QEMU 直接引导（`-kernel`），启动链上缺两层：**bootloader 的装载与参数传递**，
以及**根文件系统来自块设备**。这一课把这两层补上，用八组对照回答：

> u-boot 到底替内核做了什么？换掉它（`-kernel` 直供）会少掉哪些环节？
> 根从 initramfs 换成 ext4 块设备，内核与 init 各自的行为又变了什么？
> initramfs 与 `root=` 同时出现时，谁说了算？

本文所有数字与日志都来自本机实测，原始串口日志分别在 `logs/`（八组正式用例）和
`probes/logs/`（写装置前的试探记录）。

## 学习目标

- 说清 `BootROM → u-boot → kernel` 三段式里 u-boot 的具体职责：**装镜像、准备 DT、传 bootargs、跳转**。
- 能用一条 `Kernel command line:` 与 `/proc/cmdline` 对齐，证明"参数确实是从 bootloader 传进来的"。
- 分清 `root=` / `rootfstype=` / `init=` / `rdinit=` 各自的适用对象，以及写错时的不同现象。
- 用 `mkimage -T ramdisk` 把 initramfs 包成 uImage，打通 u-boot 的 initrd 传参路径。
- 解释块设备根为什么是**只读挂载**，以及 init 为什么第一步要 remount。
- 从 `/proc/partitions` 判读分区表是否生效，而不是凭 `root=` 猜。

## 环境与前置条件

| 项 | 实测值 |
| --- | --- |
| QEMU | `qemu-system-arm` 10.2.1，机器 **`virt`**（见下方说明） |
| 内核 | Linux 7.0.14，`arm-linux-gnueabihf-gcc 15.2.0`，`#4 SMP`；`vexpress_defconfig` + `ARCH_VIRT`，`CONFIG_EXT4_FS=y`、`CONFIG_VIRTIO_MMIO=y`、`CONFIG_VIRTIO_BLK=y`、`# CONFIG_PCI is not set` |
| u-boot | 2025.10-0ubuntu2.2，`qemu_arm` 构建（`CONFIG_TARGET_QEMU_ARM_32BIT=y`），1294880 B |
| mkimage | u-boot-tools 2025.10，与 u-boot 同包源取出，用于打包 uImage ramdisk |
| 磁盘镜像 | 全部由 `mke2fs -d` / `sfdisk` / `dd` 生成，**不需要 root、不需要挂载** |
| 依赖命令 | `qemu-system-arm`、`arm-linux-gnueabihf-gcc`、`mke2fs`、`sfdisk`、`dd`、`cpio`、`mkimage` |

### 为什么是 `-M virt` 而不是 BSP_01 的 `vexpress-a9`

包里这份 u-boot 的构建配置写着：

```text
CONFIG_TARGET_QEMU_ARM_32BIT=y
# CONFIG_TARGET_VEXPRESS_CA9X4 is not set
CONFIG_DEFAULT_DEVICE_TREE="qemu-arm"
```

它是 QEMU **virt** 目标，不是 vexpress。实测 vexpress-a9 上 `-bios` 与 `-kernel` 两种装载都零输出
（`probes/logs/probe_load_*.log`、`probe_machine_vexpress.log`），virt 上一次成功
（`probes/logs/probe_machine_virt.log`）。要给 vexpress 配 u-boot 得自编源码，那是另一件事，本课不做。

**代价**：BSP_01 的设备树（`100ask-vexpress.dtb`）在这里用不上，virt 的 DT 由 QEMU 生成。
好处是 virt 原生支持 virtio-blk，块设备根这一半反而更干净。

### u-boot 的获取方式（不装系统包）

```bash
bash get_uboot.sh     # apt-get download u-boot-qemu + u-boot-tools，dpkg-deb -x 解到 ~/bsp02-tools/
```

下载物放在独立的空目录里，仓库只按绝对路径引用其中的 `u-boot.bin` 与 `mkimage`，不执行包内任何脚本。

## 核心概念

### 两条启动路径的差别

```text
BSP_01: QEMU -kernel zImage -initrd ... -append "..."
        └─ QEMU 直接按 ARM 引导协议装内核，没有 bootloader

BSP_02: QEMU -bios u-boot.bin
        ├─ u-boot 启动 → 从磁盘 ext4load 内核到 RAM → setenv bootargs → bootz
        │                └─ 内核启动 → 按 bootargs 里的 root= 去挂 ext4 块设备
        └─ 或：再 ext4load 一个 uImage ramdisk → bootz <内核> <initrd> <fdt>
                         └─ 内核把 cpio 解到 rootfs → 跑 initramfs 里的 /init
```

### u-boot 的四件事

| 动作 | 命令 | 少了它会怎样 |
| --- | --- | --- |
| 把内核从存储搬到 RAM | `ext4load virtio 0 0x40400000 /boot/zimage` | `bootz` 对着未装载的内存跑，报 `zimage: Bad magic!` |
| 准备设备树 | `bootz <k> - <fdt>` 的第三个参数 | 报 `FDT and ATAGS support not compiled in` 并复位，内核根本没启动 |
| 传内核命令行 | `setenv bootargs ...` | 内核拿不到 `console=` 就静默，拿不到 `root=` 就 panic |
| 跳转 | `bootz` | —— |

### 块设备根与 initramfs 根的行为差异

| 项 | initramfs 根 | 块设备根 |
| --- | --- | --- |
| 挂载方式 | 内存盘（ramfs），天然可写 | ext4，内核**先挂只读**，由 init remount |
| 指定入口 | `rdinit=/init` | `init=/init`（默认顺序是 `/sbin/init`、`/etc/init`、`/bin/init`、`/bin/sh`） |
| `/proc/mounts` 的设备字段 | `rootfs` | `/dev/root`（不是 `/dev/vda`，见关键发现 5） |
| 依赖 | 无 | 块设备驱动 + 分区识别 + 文件系统 |

### initramfs 与 `root=` 同时出现时谁说了算

内核解完 initramfs 后会先看 `rdinit=`（默认 `/init`）指向的文件在不在：
**在，就直接跑它，`root=` 根本不被使用；不在，才丢下 initramfs 去挂 `root=`。**
`uboot-initrd-vs-root` 就是这条规则的实测：命令行里明写 `root=/dev/vda rootfstype=ext4`，
`BOOT|cmdline` 也逐字含这段，但日志里**没有一行 `EXT4-fs (vda)`**，根仍是 `rootfs`
——块设备存在（`virtio_blk ... [vda]` 有），只是内核没有去挂它。

这也解释了真实发行版为什么要 initramfs：**initramfs 是一个"先跑起来"的 rootfs，
`root=` 是它内部脚本要挂的目标**，两者不是二选一，而是先后关系。

### uImage ramdisk 的三段结构

`mkimage -T ramdisk` 只加 64 B 的 legacy uImage 头，负载原样不动：

| 段 | 大小（本课实测） | 内容 |
| --- | --- | --- |
| `initramfs.cpio.gz` | 269941 B | newc cpio + gzip -9，内含 `/init` |
| uImage 头 | 64 B | magic `0x27051956`、`os=linux`、`arch=arm`、`type=ramdisk`、`comp=gzip` |
| `initrd.uimg` | 270005 B | 头 + 负载，`Image Type: ARM Linux RAMDisk Image (gzip compressed)` |

头里的 `comp=gzip` 只声明"负载是 gzip"，**不负责解压**；实测是内核解的
（`Freeing initrd memory: 264K` 对应的是**压缩后**的 269941 B，说明内核拿到的是压缩数据，
解压成 rootfs 后再释放这块 initrd 内存）。

## 接口卡片

### `bootz`（u-boot）

作用：启动 ARM zImage 内核。

形式：`bootz <kernel_addr> [<initrd_addr> [<fdt_addr>]]`

要点（实测）：**三个参数的完整形式才可靠**。只给内核地址时，u-boot 打印
`Working FDT set to 0` 与 `FDT and ATAGS support not compiled in`，然后复位。
默认环境里虽然有 `fdt_addr=0x40000000`，但 `bootz` **不会自动采用它**。
本例中 QEMU 传给 u-boot 的 DT 就位于 `0x40000000`，所以显式写这个值。

第二个参数是 initrd 地址，**必须是 uImage 或 FIT**，裸 gzip/cpio 会被判
`Wrong Ramdisk Image Format`（见关键发现 6）；不传 initrd 时用 `-` 占位。

### `mkimage -T ramdisk`（宿主侧）

作用：把 initramfs 包成 u-boot 能识别的 legacy uImage。

形式：`mkimage -A <arch> -O <os> -T ramdisk -C <comp> -n <name> -d <in> <out>`

| 参数 | 本例取值 | 说明 |
| --- | --- | --- |
| `-A` / `-O` | `arm` / `linux` | 写进 64 B 头，u-boot 按此校验 |
| `-T` | `ramdisk` | `Image Type: ARM Linux RAMDisk Image` |
| `-C` | `gzip` | 声明负载的压缩方式，实测取 `gzip`（与载荷实际一致） |
| `-d` / 输出 | `initramfs.cpio.gz` → `initrd.uimg` | 只加头，负载不改 |

要点：`-C` 只**声明**压缩方式，u-boot 不负责解压；`mkimage -l <file>` 可回读头部信息自检。

### `ext4load` / `fatload`（u-boot）

作用：把存储上的文件整块读到 RAM 指定地址。

形式：`ext4load <interface> <dev[:part]> <addr> <filename>`

要点：whole-disk 文件系统用 `virtio 0`，分区上的才写 `virtio 0:1`。
本例镜像没有分区表，所以是 `ext4load virtio 0 0x40400000 /boot/zimage`。

### `root=` / `rootfstype=` / `init=` / `rdinit=`（内核命令行）

| 参数 | 作用 | 写错时的现象 |
| --- | --- | --- |
| `root=` | 指定根设备（块设备必须） | `VFS: Cannot open root device ...` → panic |
| `rootfstype=` | 指定文件系统类型，省去内核逐个试探 | 一般只是变慢或选错驱动 |
| `init=` | 真实根上的 PID 1 路径 | 找不到就按默认列表继续试，最终 `No working init found` |
| `rdinit=` | **initramfs** 里的 PID 1 路径，默认 `/init` | 见 BSP_01 关键发现 2（参数被丢弃后去挂真实根） |

关键：`rdinit=` 有值时内核**先**试它，成功就不再管 `root=`；`rdinit` 未写时默认值同样是 `/init`
（源码结论，与本课实测一致：`uboot-initrd-vs-root` 没写 `rdinit=`，initramfs 里的 `/init` 照样跑起来）。

### `mke2fs -d` / `sfdisk` / `dd`（宿主侧造盘）

- `mke2fs -q -t ext4 -F -d <dir> <img>`：从目录直接生成 ext4 镜像，**不需要 root、不需要 loop 挂载**。
- `sfdisk <img> <<'EOF' ... EOF`：对普通文件写 MBR 分区表，同样不需要 root。
- `dd ... seek=2048 conv=notrunc`：把文件系统写进分区起点（2048 扇区 = 1 MiB）。

## 最小可运行实验

八组对照，每组只改一个变量：

| 用例 | 变量 | 预期 |
| --- | --- | --- |
| `uboot-boot` | u-boot 装载 + 显式 fdt + bootargs | 完整启动，`/proc/cmdline` == u-boot 的 `bootargs` |
| `uboot-nofdt` | 同上，但 `bootz` 不给 fdt | `FDT and ATAGS support not compiled in` 并复位，内核未启动 |
| `uboot-badmagic` | `bootz` 指向未装载的内存 | `zimage: Bad magic!` |
| `uboot-initrd` | u-boot 装载 + uImage ramdisk，`rdinit=/init` | `Unpacking initramfs...`，根是 `rootfs` |
| `uboot-initrd-vs-root` | 同上，但命令行**同时**写 `root=/dev/vda rootfstype=ext4` | 根仍是 `rootfs`，日志中无 `EXT4-fs (vda)` |
| `kernel-whole` | 无 u-boot，整盘 ext4，`root=/dev/vda` | 挂载成功，`partitions` 只有 `vda` |
| `kernel-part` | MBR + 分区内 ext4，`root=/dev/vda1` | 挂载成功，`partitions` 出现 `vda` 与 `vda1` |
| `kernel-nopart` | 同 `kernel-whole` 的盘，但写 `root=/dev/vda1` | `unknown-block(254,1)` panic |

## 编译与运行

```bash
# 取 u-boot 与 mkimage（一次）
bash get_uboot.sh

# 八组全部（每组的 QEMU 都能自行退出，约 3 分钟）
bash run.sh

# 单组
bash run.sh kernel-part
bash run.sh uboot-initrd

# 换内核树 / 换 u-boot / 换 mkimage
KERNEL_TREE=/path/to/linux-source UB=/path/to/u-boot.bin MKI=/path/to/mkimage bash run.sh
```

`run.sh` 会：编译并校验 `init` 是 ARM → 打包 initramfs 与 uImage → 造三块盘 → 逐组跑 QEMU
→ 完整日志落 `logs/` → 回读并打印证据行。

用 `bash run.sh` 而不是 `./run.sh`：本仓库在 Windows 下开发（`core.filemode=false`），
可执行位不随提交传播。

## 结果观察

### 镜像组成（实测字节数）

| 文件 | 大小 | 内容 |
| --- | --- | --- |
| `build/disk_uboot.ext4` | 25165824 B（vda 24576 块） | `/init` + `/boot/zimage`（6405304 B）+ `/boot/initrd.uimg`（270005 B） |
| `build/initramfs.cpio.gz` | 269941 B | newc cpio + gzip，仅含 `/init` |
| `build/initrd.uimg` | 270005 B | 上一行 + 64 B uImage 头 |
| `build/disk_whole.ext4` | 16777216 B | 整盘 ext4，无分区表，`/init` |
| `build/disk_part.img` | 33554432 B | MBR + 20 MiB 分区，ext4 写在分区里 |
| `init`（静态 ARM） | 约 508 KB | 取证程序，PID 1 |

### `uboot-boot`：完整链路

```text
U-Boot 2025.10-0ubuntu2.2 (Oct 07 2026 - 20:59:03 +0000)
=> ext4load virtio 0 0x40400000 /boot/zimage
6405304 bytes read in 25 ms (244.3 MiB/s)
=> setenv bootargs console=ttyAMA0 root=/dev/vda init=/init panic=1
=> bootz 0x40400000 - 0x40000000
   Booting using the fdt blob at 0x40000000
Starting kernel ...
Kernel command line: console=ttyAMA0 root=/dev/vda init=/init panic=1
```

装置侧（同一份日志）：

```text
BOOT|cmdline|console=ttyAMA0 root=/dev/vda init=/init panic=1
BOOT|root_mount|dev=/dev/root|fstype=ext4
BOOT|partitions|254:0:24576:vda,31:0:131072:mtdblock0
```

逐段对应：

| 证据 | 证明的环节 |
| --- | --- |
| `bytes read in ... /boot/zimage` | u-boot 从 ext4 读文件到 RAM |
| `Booting using the fdt blob at 0x40000000` | u-boot 把 DT 交给内核 |
| `Kernel command line:` 与 `BOOT\|cmdline` 一致 | **bootloader → 内核的参数传递成立**（两处独立读取） |
| `BOOT\|root_mount\|fstype=ext4` | 根来自块设备而不是内存盘 |

`partitions` 里的 `mtdblock0` 是本内核配置自带的 physmap flash 探到的，与本课无关。

### `uboot-initrd`：u-boot 把 uImage ramdisk 交给内核

```text
=> ext4load virtio 0 0x40400000 /boot/zimage
6405304 bytes read in 27 ms (226.2 MiB/s)
=> ext4load virtio 0 0x48000000 /boot/initrd.uimg
270005 bytes read in 2 ms (128.7 MiB/s)
=> setenv bootargs console=ttyAMA0 rdinit=/init panic=1
=> bootz 0x40400000 0x48000000 0x40000000
## Loading init Ramdisk from Legacy Image at 48000000 ...
   Image Type:   ARM Linux RAMDisk Image (gzip compressed)
   Booting using the fdt blob at 0x40000000
Kernel command line: console=ttyAMA0 rdinit=/init panic=1
Unpacking initramfs...
Freeing initrd memory: 264K
```

装置侧：

```text
BOOT|cmdline|console=ttyAMA0 rdinit=/init panic=1
BOOT|root_mount|dev=rootfs|fstype=rootfs
BOOT|partitions|254:0:24576:vda,31:0:131072:mtdblock0
BOOT|dev_vda|present
```

两个数字要一起看：`Freeing initrd memory: 264K` 对的是**压缩后**的 269941 B，
而 `BOOT|rootfs_files|1` 说明 initramfs 里确实只有 `/init` 一个文件——
所以内核拿到的是压缩数据、自己解开、再释放那块内存。

### 关键发现 1：`bootz` 不给 fdt，内核根本不会启动

`uboot-nofdt` 的全部有效输出只有三行：

```text
Working FDT set to 0
FDT and ATAGS support not compiled in
resetting ...
```

没有 `Starting kernel`，没有内核日志。默认环境里 `fdt_addr=0x40000000` **存在但不会被自动采用**。
这条与 BSP_01 关键发现 4（不给 `-dtb` 时零输出）是同一件事的两个位置：**DT 是必要输入**。

### 关键发现 2：DT 的字节数取决于"谁把它交给内核"

同一个内核，`/sys/firmware/fdt` 的大小随交付方式变化：

| 路径 | `fdt_raw` 大小 |
| --- | --- |
| u-boot `bootz` 显式 fdt | 8192 B |
| QEMU `-kernel` 直供（`virt`） | 1048576 B |
| QEMU `-kernel -dtb`（BSP_01 的 `vexpress-a9`） | 49086 B |

所以"内核里的 DT 长得和你手上的文件一样"从来都不成立；要判断"到底是谁给的 DT"，
只能看运行期 `/sys/firmware/devicetree/base/` 的内容与 `fdt_raw` 的大小。

### 关键发现 3：块设备根是只读挂载，init 必须 remount

`kernel-whole` 的内核侧：

```text
VFS: Mounted root (ext4 filesystem) readonly on device 254:0.
VFS: Pivoted into new rootfs
```

装置侧第一次进到这个环境时，三处 mount 全部 `errno=2`（ENOENT）——因为只读根上
`mkdir /proc` 失败了，后面去挂 `/proc`、`/sys`、`/dev` 自然找不到挂载点。
补上 `mount(NULL, "/", NULL, MS_REMOUNT, NULL)` 之后：

```text
EXT4-fs (vda): re-mounted ... r/w.
BOOT|remount_root|ok
```

initramfs 环境下同一个 remount 调用返回成功但无实际意义（ramfs 本来就可写）——
**这就是真实 init 系统（systemd/busybox）启动第一步要 remount 的原因**。

### 关键发现 4：没有分区表时，`root=/dev/vda1` 的报错和"没有块设备"不同

| 用例 | 关键行 |
| --- | --- |
| `kernel-nopart`（有 vda，无分区表） | `VFS: Cannot open root device "/dev/vda1" or unknown-block(254,1): error -6` |
| BSP_01 `no-initrd`（压根没有块设备） | `VFS: Unable to mount root fs on unknown-block(0,0)` |

括号里 `254:1` 说明内核已经认识了 virtio-blk（主设备号 254），缺的只是**第 1 号分区**。
`0:0` 才是"连设备都没有"。判读 panic 时要看这个括号。

### 关键发现 5：`/proc/mounts` 里根设备写的是 `/dev/root`

三个块设备用例的取值都是：

```text
BOOT|root_mount|dev=/dev/root|fstype=ext4
```

`/dev/root` 是内核为"当前根设备"保留的名字，**不是真实的设备路径**。
要确认真实设备与分区，看 `/proc/partitions`：

| 用例 | `BOOT\|partitions` |
| --- | --- |
| `kernel-whole` | `254:0:16384:vda,31:0:131072:mtdblock0` |
| `kernel-part` | `254:0:32768:vda,254:1:20480:vda1,31:0:131072:mtdblock0` |

### 关键发现 6：`bootz` 不认裸 gzip 的 initramfs，`mkimage -T ramdisk` 之后才通

先做过一版"u-boot 从盘上 `fatload` 内核与 gzip initramfs 再 `bootz`"，结果：

```text
=> fatload virtio 0 0x48000000 initrd
269943 bytes read in 6 ms (42.9 MiB/s)
=> bootz 0x40400000 0x48000000
Kernel image @ 0x40400000 [ 0x000000 - 0x61bcb8 ]
Wrong Ramdisk Image Format
Ramdisk image is corrupt or invalid
```

这份 u-boot 编入了 `CONFIG_GZIP=y`、`CONFIG_FIT=y`，但 `bootz` 的 ramdisk 校验只接受
uImage/FIT 形式。补上 `mkimage -A arm -O linux -T ramdisk -C gzip`（只加 64 B 头，负载一字不改）之后
同一条 `bootz` 立刻通过：

| 输入 | 大小 | `bootz` 结果 |
| --- | --- | --- |
| 裸 `initramfs.cpio.gz` | 269943 B（早期一次打包） | `Wrong Ramdisk Image Format`（`probes/logs/probe_uboot_load.log`） |
| `initrd.uimg` | 270005 B（本次打包，负载 269941 B） | `## Loading init Ramdisk from Legacy Image at 48000000 ...` → `Unpacking initramfs...`（`logs/uboot-initrd.log`） |

两次的负载都是"newc cpio + gzip 的同一份 `/init`"，差的只是那 64 B 头和 `Image Type` 字段
（两份 cpio 相差 2 B，未逐字节比对，按 newc 头字段差异推定，与本结论无关）。
`CONFIG_xxx=y` 只说明能力编进来了，不说明某条命令路径会自动使用它；
判据是命令自己打印的校验结果，不是配置文件。

### 关键发现 7：有 initramfs 时 `root=` 被跳过，不是被覆盖

`uboot-initrd-vs-root` 的命令行与块设备用例完全一致，只额外多传了一个 uImage：

```text
Kernel command line: console=ttyAMA0 root=/dev/vda rootfstype=ext4 init=/init panic=1
Unpacking initramfs...
BOOT|root_mount|dev=rootfs|fstype=rootfs
```

同一份日志里 `virtio_blk virtio0: [vda] 49152 512-byte logical blocks` 说明设备**在**，
但**没有任何一行 `EXT4-fs (vda)`**——内核根本没去挂它。对比块设备用例：

| 用例 | cmdline 中的 `root=` | `EXT4-fs (vda)` | `BOOT\|root_mount` |
| --- | --- | --- | --- |
| `uboot-initrd` | 无 | 无 | `dev=rootfs\|fstype=rootfs` |
| `uboot-initrd-vs-root` | `root=/dev/vda rootfstype=ext4` | **无** | `dev=rootfs\|fstype=rootfs` |
| `uboot-boot` | `root=/dev/vda` | 有（含 `re-mounted r/w`） | `dev=/dev/root\|fstype=ext4` |

判读方法：不要看 `root=` 写了什么，**看有没有 `EXT4-fs (xxx): mounted filesystem`**
——有才是真的挂了块设备。

## 常见错误和陷阱

1. **`bootz` 只给内核地址**：内核不会启动，只有 `FDT and ATAGS support not compiled in`。默认环境里的 `fdt_addr` 不会被自动采用。
2. **把 QEMU 的 `if=virtio` 当 MMIO 用**：本内核 `# CONFIG_PCI is not set`，`if=virtio` 建的是 PCI 设备，内核看不见（`root=/dev/vda` 报 `error -6`）。要写 `-device virtio-blk-device`。
3. **在只读根上直接 `mkdir /proc`**：返回 EROFS，后续 mount 全部 ENOENT，日志里只看到 `errno=2`，容易误判成"proc 没编进内核"。
4. **用 `CC ?=` 指定交叉编译器**：GNU make 内建 `CC=cc` 算"已定义"，`?=` 不生效，静默编出 x86-64 的 init，进内核报 `Failed to execute /init (error -8)`（ENOEXEC）。正确写法是 `ifeq ($(origin CC),default)`。
5. **块设备根仍用 `rdinit=/init`**：`rdinit` 只对 initramfs 生效，块设备根要 `init=/init`；不写就会被默认列表带到 `No working init found`。
6. **拿 `/proc/mounts` 的设备字段当真实设备名**：它是 `/dev/root`。看 `/proc/partitions`。
7. **用 `-bios` 装非目标机器的 u-boot**：零输出、无 panic，最容易误判成"QEMU 坏了"。先看包里的 `DEFAULT_DEVICE_TREE` 与 `TARGET_*`。
8. **把 `fat:ro:` 喂给 virtio-blk**：QEMU 直接拒绝启动（`Block node is read-only`），根本到不了 u-boot。
9. **直接把 `initramfs.cpio.gz` 交给 `bootz`**：`Wrong Ramdisk Image Format`。`bootz` 的 ramdisk 只认 uImage/FIT，先过一遍 `mkimage -T ramdisk`。
10. **看到 `bootargs` 里有 `root=/dev/vda` 就认为根在块设备上**：只要 initramfs 里有 `/init`，`root=` 会被整个跳过（关键发现 7），判据是有没有 `EXT4-fs (vda): mounted filesystem`。

## 故障排查方法

| 现象 | 先看什么 | 判据 |
| --- | --- | --- |
| u-boot 一点输出都没有 | 这份 u-boot 的目标机器与 `-M` 是否一致 | `strings`/配置文件里的 `TARGET_*`、`DEFAULT_DEVICE_TREE` |
| u-boot 出 prompt 但 `ext4load` 读不到文件 | `ls virtio 0 /` 先列目录 | 列不出 → 设备或文件系统没被识别；能列出 → 路径写错 |
| `bootz` 后立刻复位 | 是否给了 fdt 参数 | `Working FDT set to 0` → 没给 |
| `Bad magic!` | 内核地址是否真的装过 | 先 `ext4load`，再 `bootz` 同一地址 |
| `Wrong Ramdisk Image Format` | 第二参数指向的是不是 uImage/FIT | `mkimage -l <file>` 能读出 `Image Type` 才是；裸 cpio/gzip 一律不过 |
| 给了 initrd 但根不是块设备 | 有没有 `EXT4-fs (xxx): mounted filesystem` | 没有 → initramfs 接管，`root=` 被跳过 |
| `Cannot open root device ... unknown-block(254,1)` | `/proc/partitions`（panic 前打不出来就看 `vda: vda1` 那行） | 括号 `254:1` → 缺分区；`0:0` → 缺设备驱动 |
| init 起来了但所有 mount 都 ENOENT | 根是否只读 | 先 remount `r/w`，再建挂载点 |
| `No working init found` | `init=` 是否写了、路径是否存在 | 块设备根用 `init=`，initramfs 用 `rdinit=` |

## 练习题

1. **基础题**：`uboot-boot` 里哪两行分别证明"u-boot 装载了内核"和"参数传到了内核"？它们各自来自哪个软件的输出？
2. **改错题**：把 `uboot-boot` 的 `bootz 0x40400000 - 0x40000000` 改成 `bootz 0x40400000` 会怎样？先预测，再跑 `bash run.sh uboot-nofdt` 验证。
3. **改错题**：`kernel-whole` 的盘加上 `rootwait` 参数，输出有任何变化吗？为什么 virtio-blk 和 SD 卡在这点上不同？
4. **综合题**：给 `disk_part.img` 增加第二个分区并在 `root=` 里指向它，需要改哪几处（分区表、dd 偏移、`root=`）？
5. **综合题**：只用 `probes/logs/probe_uboot_load.log` 与 `logs/uboot-initrd.log`，说明"u-boot 认不认 gzip initramfs"这件事的判据是什么，以及为什么 `CONFIG_GZIP=y` 不足以让它成立。
6. **基础题**：`uboot-initrd.log` 里哪一行证明"内核拿到的是压缩数据、不是解压后的 cpio"？把它与 `initrd.uimg` 的字节数对上。
7. **改错题**：把 `uboot-initrd-vs-root` 的 initrd 参数改成 `-`（不传），根会变成什么？先预测，再跑 `bash run.sh uboot-initrd-vs-root` 对比——注意这时 cmdline 一个字都没变。
8. **综合题**：要让"块设备根 + initramfs 双阶段"真正成立（initramfs 里的脚本去挂 `/dev/vda`），需要 `init` 里补哪几步？对照 `uboot-initrd-vs-root` 缺了什么。

## 验收标准

- [x] 能说出 u-boot 的四项职责，并为每项给出一条日志证据
- [x] `bootargs` 与 `/proc/cmdline` 两处独立读取一致（参数传递成立）
- [x] 三种 DT 交付方式的 `fdt_raw` 字节数都有实测值
- [x] 能解释块设备根只读与 remount 的必要性，并有 `errno=2` 的传导链
- [x] 能从 panic 括号里的 `major:minor` 区分"缺分区"与"缺设备"
- [x] `mkimage -T ramdisk` 打出的 uImage 能被 `bootz` 接受，并有裸 gzip 被拒的对照日志
- [x] 能用一个用例证明 initramfs 接管后 `root=` 被跳过（有 `virtio_blk [vda]`、无 `EXT4-fs (vda)`）
- [x] 八组用例都能一条命令复现

## 已知限制

- **u-boot 是 virt 目标，不是 vexpress**：换机器换来的是"能跑"，代价是 BSP_01 的 DTB 与本课的 DT 不可比；真实 vexpress 需要自编 u-boot。
- **initramfs 只有 `/init` 一个文件**：只验证了"接管 PID 1"，没有 `busybox`、没有 `/sbin/init`，
  因此"initramfs 里再挂真实根"的两阶段流程（真正发行版的做法）**未覆盖**。
- **没有真实块硬件**：virtio-blk 是半虚拟化设备，探针顺序是确定的。真板的 SD/eMMC 需要 `rootwait`，本课**未验证**。
- **没有真实 BootROM 与二级引导**：QEMU `-bios` 直接把 u-boot 放进内存，真实的 BootROM → SPL → u-boot 三段式未覆盖。
- **`panic=1` 仍是实验手段**：只为让负向用例快速退出。
- **未验证**：FIT 形式的 ramdisk（`-T ramdisk` 之外）、`bootflow`/extlinux 引导、
  u-boot 侧显式解压 ramdisk 的路径（本课观察到的是内核解压，未测 `bootm` 的 `-C none` 变体）、
  `fdt print /model` 在 `0x40000000` 处返回 `FDT_ERR_NOTFOUND` 的原因（同一份 blob 却被内核正确解析出 `Machine model: linux,dummy-virt`，未深究）。

## 延伸阅读

- u-boot 文档 `doc/usage/cmd/bootz.rst`、`doc/usage/cmd/ext4load.rst`：命令语义与参数顺序。
- u-boot 文档 `doc/usage/mkimage.rst`：legacy uImage 头结构与 `-T ramdisk` 的字段含义。
- 内核文档 `Documentation/admin-guide/kernel-parameters.txt`：`root=`、`rootfstype=`、`rootwait`、`init=`、`rdinit=` 的准确定义。
- 内核源码 `init/do_mounts.c`：`rootwait` 与根设备探测的重试逻辑；`init/main.c`：`init=` 的默认搜索序列。
- u-boot 文档 `doc/develop/distro.rst`：`bootflow`/extlinux 的标准化引导路径（本课未覆盖）。
- 同仓库 `../BSP_01_Boot_Chain/`：三件套分工与 QEMU 直接引导；本课是它的下一层。
