# BSP_01 启动链与镜像组成

用四组对照实验回答一个问题：**从 QEMU 把 zImage / DTB / initramfs 交给内核，到 `/init` 被执行为 PID 1，
中间每一步各自依赖哪一件东西；少一件或改坏一件，现象有什么不同。**

本文所有数字都来自本机实测（QEMU 10.2.1 + Linux 7.0.14 ARMv7），原始日志在 `logs/`。

## 学习目标

- 说清 `zImage`、`DTB`、`initramfs` 三件套各自负责什么，以及边界在哪里。
- 能从串口日志里认出启动链的三个里程碑：`OF: fdt: Machine model`、`Unpacking initramfs`、`Run /init as init process`。
- 能用 `/sys/firmware/fdt`、`/sys/firmware/devicetree/base/`、`/proc/cmdline` 反查"内核看到的到底是哪份 DT"。
- 分清 `rdinit=` 与 `init=`，并知道 `rdinit` 写错时内核的真实行为（不是报"执行失败"）。
- 建立"现象相同、原因不同"的判断习惯：两种不同的缺件方式可以 panic 在同一行。

## 环境与前置条件

| 项 | 实测值 |
| --- | --- |
| QEMU | `qemu-system-arm` 10.2.1，机器 `vexpress-a9` |
| 内核 | Linux 7.0.14，`arm-linux-gnueabihf-gcc 15.2.0`，`#4 SMP`，内建 `vexpress_defconfig` + `ARCH_VIRT`/`GPIO_GENERIC_PLATFORM`/`SPI` |
| 内核树 | `~/kbuild/linux-source-7.0.0`（可用 `KERNEL_TREE` 覆盖） |
| 设备树 | 复用 `tests/qemu-vexpress/100ask-vexpress.dtb`（可用 `DTB` 覆盖） |
| 用户态 | 无 busybox；`init.c` 静态编译，只用系统调用 |
| 依赖命令 | `qemu-system-arm`、`arm-linux-gnueabihf-gcc`、`cpio`、`gzip` |

前置检查（缺任何一项 `run.sh` 会直接报错退出，不会猜路径）：

```bash
command -v qemu-system-arm arm-linux-gnueabihf-gcc cpio
ls -l ~/kbuild/linux-source-7.0.0/arch/arm/boot/zImage
ls -l ../tests/qemu-vexpress/100ask-vexpress.dtb
```

## 核心概念

### 三件套的分工

```text
QEMU -kernel zImage        自解压内核，跑到 C 代码并初始化串口
     -dtb  xxx.dtb         告诉内核"这块板子长什么样"：内存、串口、中断控制器
     -initrd xxx.cpio.gz   解包成 rootfs，作为"/"的初始内容
     -append "rdinit=/init" 指定 rootfs 里哪个文件当 PID 1
```

### 启动链上的边界

| 阶段 | 谁在做 | 失败时的现象 |
| --- | --- | --- |
| 解压 + 早期初始化 | `zImage` 自带解压头 | 串口完全静默 |
| 识别机器 | 内核读 DTB 根节点 | 串口完全静默（DT 缺失时连串口都配不出来） |
| 挂根 | `initramfs` 解包 / 否则挂真实块设备 | `VFS: Unable to mount root fs` panic |
| 执行 init | `init/main.c` 按 `rdinit` → `init=` → `/sbin/init`… 依次尝试 | 见下面的"陷阱 1" |

三件套里最容易误解的是 DTB：它不是"可选补充配置"，而是 ARM 内核知道自己在哪块板子上的**唯一**依据。

## 接口卡片

### `rdinit=`（内核命令行）

作用：指定 initramfs 里的哪个路径当 PID 1。

行为要点（`init/main.c:1658`，7.0.14 实测）：内核先用 `init_eaccess()` 检查这个路径；**不存在就丢弃整个参数**，
打印 `check access for rdinit=... failed: -2, ignoring`，然后转去 `prepare_namespace()` 挂真实根。
所以 rdinit 写错时看到的不是"执行 init 失败"，而是"挂不上根文件系统"。

常见误用：把 `rdinit=` 当成"init 路径覆盖"来写错路径排查问题——它会让内核去挂一个根本不存在的块设备。

### `-kernel / -dtb / -initrd`（QEMU）

作用：分别提供内核镜像、设备树二进制、初始内存盘。

注意：QEMU **会改写**你传的 DTB。vexpress 机器在把 DT 交给内核前会按 `-m` 等参数补齐节点，
因此 `/sys/firmware/fdt` 的大小和你磁盘上那个 `.dtb` 文件不一样（实测 49086 B vs 14543 B）。

### `/sys/firmware/fdt` 与 `/sys/firmware/devicetree/base/`

作用：前者是内核保留的**原始 DT 二进制**（交给内核的那一份），后者是它展开成的目录树。

用途：判断"跑的是哪份 DT"、读属性原值。展开树里的字符串以 NUL 结尾，多值属性用 NUL 分隔。

### `/proc/cmdline`

作用：内核实际收到的命令行（bootloader 或 QEMU `-append` 传入）。
用于确认 `rdinit`、`console`、`panic=` 这些参数真的到了内核，而不是写在了错误的文件里。

## 最小可运行实验

四组对照，每组只改一个变量：

| 用例 | 变量 | 预期 |
| --- | --- | --- |
| `normal` | 三件套齐全 | 正常启动到 `/init`，打印取证项后断电 |
| `bad-init` | `rdinit=/nosuch/init` | 内核丢弃 rdinit，转去挂根，VFS panic |
| `no-initrd` | 不给 `-initrd` | 无根可挂，VFS panic |
| `qemu-dtb` | 不给 `-dtb` | 内核识别不了机器，串口零输出 |

## 编译与运行

```bash
# 全部四组（约 60 s）
bash run.sh

# 单组
bash run.sh normal

# 指定别的内核树 / DTB
KERNEL_TREE=/path/to/linux-source-7.0.0 DTB=/path/to/board.dtb bash run.sh normal
```

用 `bash run.sh` 而不是 `./run.sh`：本仓库在 Windows 下开发（`core.filemode=false`），可执行位不随提交传播，
换到 Linux/WSL 克隆后 `./run.sh` 会报 permission denied。

`run.sh` 会：编译 `init` → 打包只含 `/init` 的 initramfs → 按用例拼 QEMU 参数 → 完整日志落 `logs/` → 回读并分类展示。

## 结果观察

### 镜像组成（实测字节数）

| 件 | 文件 | 大小 | 说明 |
| --- | --- | --- | --- |
| 内核 | `arch/arm/boot/zImage` | 6405304 B | 含自解压头 |
| 设备树 | `100ask-vexpress.dtb` | 14543 B | 磁盘上的原文 |
| 初始内存盘 | `build/initramfs.cpio.gz` | 267535 B | 只含一个静态 `init`（504576 B，压缩后 267 KB） |

### `normal` 用例

装置输出（原样节选 `logs/normal.log`）：

```text
BOOT|init|pid=1
BOOT|cmdline|console=ttyAMA0 rdinit=/init panic=1
BOOT|dt_model|V2P-CA9
BOOT|dt_compatible|arm,vexpress,v2p-ca9,arm,vexpress
BOOT|fdt_raw|size=49086
BOOT|dt_custom_node|present
BOOT|dt_memory|node=memory@60000000|base=0x60000000|size=0x20000000|mib=512
BOOT|rootfs_files|1
BOOT|root_fstype|rootfs
BOOT|init|done
```

内核侧（同一份日志）：

```text
Booting Linux on physical CPU 0x0
OF: fdt: Machine model: V2P-CA9
Unpacking initramfs...
Freeing unused kernel image (initmem) memory: 1024K
Run /init as init process
```

逐条对应关系：

| 取证项 | 证明的启动链环节 |
| --- | --- |
| `Booting Linux on physical CPU 0x0` | zImage 解压完成、CPU 早期初始化完成 |
| `OF: fdt: Machine model: V2P-CA9` + `BOOT\|dt_model` | DTB 被读取，根节点 model 生效 |
| `BOOT\|dt_custom_node\|present` | 跑的是我们传的那份 DTB（自定义节点在） |
| `Unpacking initramfs...` | initramfs 被解包 |
| `BOOT\|root_fstype\|rootfs` | `/` 的类型就是内存盘 `rootfs` |
| `BOOT\|rootfs_files\|1` | rootfs 里确实只有 `init` 一个文件（不是"挂上了但空的"） |
| `Run /init as init process` + `BOOT\|init\|pid=1` | 执行成功，PID 1 |

### 关键发现 1：QEMU 会改写你传的 DTB

| 项 | 值 |
| --- | --- |
| 磁盘上的 `.dtb` | 14543 B |
| `/sys/firmware/fdt`（内核收到的） | 49086 B |
| DTS 里 memory 节点声明 | `reg = <0x60000000 0x40000000>`，即 1 GiB |
| 运行期 DT 里 memory 节点 | `base=0x60000000 size=0x20000000`，即 512 MiB |

QEMU 以 `-m 512M` 启动，就把 DT 里的内存改了。结论：**"内核看到的 DT = 你传的文件"这个假设不成立**。
要查属性真值，只能读运行期的 `/sys/firmware/devicetree/base/`，不能拿磁盘文件推。

### 关键发现 2：`rdinit` 写错的真实行为

`bad-init` 日志里的两行说明了一切：

```text
check access for rdinit=/nosuch/init failed: -2, ignoring
Kernel panic - not syncing: VFS: Unable to mount root fs on unknown-block(0,0)
```

内核先检查路径是否存在（`init_eaccess`），不存在就**忽略参数**并转去挂真实根。
对照 `init/main.c:1658-1665` 源码，与实测一致。

### 关键发现 3：两种不同的缺失，panic 在同一行

| 用例 | `Unpacking initramfs...` | panic 消息 |
| --- | --- | --- |
| `bad-init` | 有 | `VFS: Unable to mount root fs on unknown-block(0,0)` |
| `no-initrd` | 无 | `VFS: Unable to mount root fs on unknown-block(0,0)` |

**只看最后一行会误判**。区分点在前面的 `Unpacking`：有它说明 initramfs 在、是 init 路径的问题；
没它说明根内存盘根本没进来。

### 关键发现 4：没有 DTB，一个字都不出

`qemu-dtb` 用例 25 秒零输出，`logs/qemu-dtb.log` 里只有 QEMU 自己的音频后端噪音和超时行。
ARM 内核在配置串口之前就要读 DT 找 UART，拿不到就死在最早期——**DTB 是必要输入，不是可选补充**。

## 常见错误和陷阱

1. **把 `rdinit` 写错当成"init 不存在"来查**：内核根本不报"执行失败"，它去挂根了，你查错方向。
2. **以为 DTB 文件 = 运行期 DT**：QEMU 会改（见关键发现 1）。属性真值只认 `/sys/firmware/devicetree/base/`。
3. **只看 panic 最后一行**：`bad-init` 与 `no-initrd` 消息完全相同（见关键发现 3）。
4. **用 `printf` 直接打设备树属性**：字符串以 NUL 结尾、多值用 NUL 分隔，不处理会只打出第一个值甚至打不出。
5. **以为 `-append` 里的参数一定生效**：`rdinit` 这种参数会被内核做可达性检查后静默丢弃。
6. **在 initramfs 里忘了建目录**：`mount proc /proc` 之前 `mkdir /proc`，否则挂载失败、后面全错。
7. **panic 后等满超时**：加 `panic=1` 配合 `-no-reboot`，内核 panic 后自动重启，QEMU 立即退出，日志完整且不用等 90 s。

## 故障排查方法

| 现象 | 先看什么 | 判据 |
| --- | --- | --- |
| 串口完全无输出 | 是否传了 `-dtb`；DTB 与机器是否匹配 | 零输出 → 死在内核最早期，与 init/rootfs 无关 |
| 有输出但停在 `Unpacking initramfs...` | `rdinit`/`init` 路径 | 后面应有 `Run /xxx as init process` |
| panic 在 `Unable to mount root fs` | 有没有 `Unpacking` 行 | 有 → init 路径问题；无 → 没给 initramfs |
| init 跑了但行为不对 | `/proc/cmdline` | 确认参数真的传到了 |
| DT 属性与 DTS 不一致 | `/sys/firmware/fdt` 大小、`dt_memory` | 大小或数值对不上 → DT 被 bootloader/QEMU 改写过 |
| 日志被截断 | 是否在管道里接了 `head` | SIGPIPE 会让 QEMU/tee 提前退出，先落盘再读 |

## 练习题

1. **基础题**：`normal` 用例里，哪一行证明"initramfs 被解包"、哪一行证明"根是内存盘"？两行分别属于内核还是用户态？
2. **改错题**：把 `rdinit=/init` 改成 `init=/init`，现象有区别吗？为什么（提示：`rdinit` 与 `init` 的查找顺序与适用对象不同）？
3. **改错题**：在 `normal` 里把 `-m 512M` 改成 `-m 256M`，`BOOT|dt_memory` 应该变成什么？先写预测，再跑验证。
4. **综合题**：设计一个用例，让"initramfs 在、init 存在、但 init 崩溃"发生，并说明它的日志与 `bad-init` / `no-initrd` 有什么区别。
5. **综合题**：不看源码，只用 `/sys/firmware/devicetree/base/` 推断这台机器有几个 UART、它们的基地址和中断号。

验收标准见下；练习只给题目与提示，参考实现按需再要。

## 验收标准

- [x] 三件套各自的字节数与作用能对上（实测表）
- [x] 能从日志中指出启动链三个里程碑
- [x] 能解释 `/sys/firmware/fdt` 与磁盘 `.dtb` 大小不同的原因，并给出证据（memory 节点 1 GiB → 512 MiB）
- [x] 能解释 `rdinit` 写错为什么不报"执行失败"，并有源码行号与实测日志互证
- [x] 能区分 `bad-init` 与 `no-initrd`（靠 `Unpacking` 行，不靠 panic 文案）
- [x] 四组用例都能一条命令复现

## 已知限制

- **只覆盖 QEMU 直接引导**：真实板子是 `BootROM → u-boot → kernel`，中间还多一层 bootloader 的参数传递和镜像加载，
  本实验没有这一层（`-kernel` 让 QEMU 跳过 u-boot）。
- **没有可写根文件系统**：`rootfs` 是内存盘，不涉及块设备、分区表、`root=/dev/xxx`、`rootwait` 这些真实启动参数。
- **没有 init 系统**：PID 1 是自撰的取证程序，不涉及 systemd/busybox init 的启动流程。
- **`panic=1` 是实验手段**：真实设备不会这么配，这里只是为了"panic 后自动重启以便 QEMU 退出"。
- **未验证项**：u-boot 阶段的镜像加载与 `bootm`/`bootz` 参数、真机 DDR 与时钟初始化、`/sys/firmware/fdt` 在非 QEMU 环境下的行为。

## 延伸阅读

- 内核源码 `init/main.c`：`kernel_init()` 里 `init_eaccess` → `prepare_namespace()` 的分支（本仓库实测对应 7.0.14 的 1658–1665 行）。
- 内核文档 `Documentation/devicetree/booting-without-of.rst`：DT 作为启动接口的约定。
- 内核文档 `Documentation/admin-guide/kernel-parameters.txt`：`rdinit`、`init`、`panic` 的行为定义。
- 同仓库 `../tests/qemu-vexpress/`：在这套启动链之上跑驱动的装置，本实验复用了它的内核树与 DTB。
