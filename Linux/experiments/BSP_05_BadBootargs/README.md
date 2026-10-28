# BSP_05 坏 bootargs：区分"参数错"与"内核配置错"

**平台**：QEMU `-M virt`（ARMv7）+ Linux 7.0.14
**验证状态**：六组用例机器判定通过（6/6），七组负向验证全部按预期拒绝（7/7）
**证据**：`logs/*.log`（六份，即 `run.sh` 本次运行的原始输出）

## 学习目标

一个系统起不来时，最常见的第一反应是"内核没配好"。但**同样的现象**也可能只是
bootloader 传错了一个参数。这两种原因的处理方式完全不同：

- 参数错 → 改 `bootargs`/`cmdline`，重新引导，不需要重编内核；
- 内核配置错 → 改 `.config`、重编内核，改参数无效。

本实验用**同一份 `zImage`、同一份 ext4 镜像**，只改 QEMU 的 `-append`，构造出
"失败 → 改参数 → 成功"的对照，从而把两者分开。

学完应当能回答：

- `root=`、`rootfstype=`、`init=`、`console=` 分别在哪一步被内核使用，失败时各自打印什么；
- 为什么 `bad-root` 与 `bad-type` 都 panic，但 panic 前的读数完全不同；
- 为什么"坏 console 仍能引导"是区分参数错与内核配置错的关键证据；
- 为什么 panic 用例的 QEMU 退出码是 124 而不是 0。

## 环境与前置条件

| 项 | 值 |
| --- | --- |
| QEMU | `qemu-system-arm`，`-M virt` |
| 内核 | `$HOME/kbuild/linux-source-7.0.0/arch/arm/boot/zImage`（`KERNEL_TREE` 可覆盖） |
| 交叉编译器 | `arm-linux-gnueabihf-gcc` |
| 其他 | `mke2fs`、`file`、`bash` 4+ |

内核必须内置：`CONFIG_VIRTIO_BLK`、`CONFIG_EXT4_FS`、`CONFIG_EXT2_FS`、`CONFIG_SERIAL_AMBA_PL011`。
本实验的 `bad-type` 用例还要求 `CONFIG_EXT2_FS` 存在，否则"按 ext2 去挂"会因为"没有 ext2 驱动"
而失败，那就测不到"特性位不兼容"这条路径了。

**与 BSP_03/04 的关键差别：本实验不使用 initramfs。** 只要命令行给了 `-initrd`，
内核会优先执行 initramfs 里的 `/init`（`ramdisk_execute_command` 默认为 `/init`），
`root=` 根本不会被尝试——那样就测不到内核层的 `root=` 失败。所以这里只挂一个 ext4 盘，
由内核自己 `prepare_namespace()` 挂根。

## 核心概念

### 1. 启动链上参数被使用的顺序

```text
内核解压 → start_kernel → kernel_init()
                              │
                              ├─ ramdisk_execute_command（rdinit=，默认 /init）
                              │    有 initramfs 且 /init 存在 → 执行它，root= 不参与
                              │
                              ├─ prepare_namespace() ← root= / rootfstype= 在这里生效
                              │    设备不存在  → VFS: Cannot open root device
                              │    超级块读不到 → VFS: Unable to mount root fs
                              │
                              └─ init= （execute_command）→ 执行 /sbin/init 或指定路径
                                   路径不存在 → Requested init /xxx failed
```

三种失败分别落在三个不同位置，所以日志措辞不同。判据必须按位置区分，不能笼统地报"启动失败"。

### 2. panic 之后 QEMU 不会自己退出

`-no-reboot` 只禁止重启，不禁止停住。内核 panic 后 CPU 停在 `panic()` 里，
QEMU 进程继续运行，直到 `timeout` 把它杀掉（退出码 **124**）。这是预期行为，不是挂死。
成功用例由 init 主动 `reboot(RB_POWER_OFF)`，退出码 0。

所以 `run.sh` 给 panic 用例 20 s 超时、给成功用例 60 s：两者的退出码本来就不一样。

### 3. 为什么"坏 console 仍能引导"最有价值

如果所有坏参数都导致 panic，那么"起不来"仍然无法区分是参数错还是内核坏。
`bad-console` 反其道而行：命令行里放一个不存在的 `console=ttyS9`，内核**忽略它**、
继续用 `console=ttyAMA0`，系统完整启动并执行 init。

这条证明：**内核能正确处理命令行里的坏值**。于是当 `root=` 出错导致 panic 时，
"内核连参数都处理不了"这个替代解释就被排除了。

## 接口卡片

### `root=`

作用：指定真实根文件系统所在的设备。
内核侧：`prepare_namespace()` → `mount_block_root()`。
失败读数：设备名不存在时 `VFS: Cannot open root device "<name>" or unknown-block(0,0): error -6`；
随后 panic `VFS: Unable to mount root fs on unknown-block(0,0)`。
常见误用：在有 initramfs 的情况下以为 `root=` 会生效——实际被 `/init` 抢先，需要 init 自己挂。

### `rootfstype=`

作用：跳过文件系统探测，直接用指定类型挂载。
失败读数：类型不匹配时由该类型的驱动报错（本例 `EXT2-fs (vda): error: couldn't mount because of
unsupported optional features (22c4)`），随后 `No filesystem could mount root, tried: ext2`。
注意：给了 `rootfstype=` 就不会再尝试其他类型；不给则会遍历内核支持的类型。

### `init=`

作用：覆盖默认的 `/sbin/init`。在根**挂载成功之后**使用。
失败读数：`Run /nope as init process` → `Kernel panic - not syncing: Requested init /nope failed (error -2).`
价值：这一组能证明 `root=` 是对的、错只在 `init=` 一个参数上。

### `console=`

作用：选择 printk 输出与 `/dev/console`。可重复给出，最后一个成为 `/dev/console`；
不存在的设备会被忽略，日志里可见 `printk: console [ttyAMA0] enabled`。
常见误用：只给一个不存在的 console → 系统可能正常启动但**看不到任何输出**，
容易误判为"没起来"。本实验的做法是保留一个有效 console 做对照。

## 六组用例与实测读数

| 用例 | 命令行差异 | 退出码 | 关键读数 | 判定 |
| --- | --- | --- | --- | --- |
| `ok` | `root=/dev/vda rootfstype=ext4` | 0 | init 跑起来、`root_fs magic=0xef53`、marker 命中 | PASS |
| `bad-root` | `root=/dev/vdz` | 124 | `Cannot open root device "/dev/vdz" ... error -6` → panic | PASS |
| `bad-type` | `rootfstype=ext2`（镜像仍是 ext4） | 124 | `EXT2-fs (vda): error: ... unsupported optional features (22c4)` → panic | PASS |
| `bad-init` | `init=/nope` | 124 | `EXT4-fs (vda): mounted filesystem` → `Requested init /nope failed (error -2)` | PASS |
| `bad-console` | `console=ttyS9 console=ttyAMA0` | 0 | init 跑起来且 `printk: console [ttyAMA0] enabled` | PASS |
| `ok-fixed` | 与 `bad-root` 只差设备名 | 0 | 与 `ok` 相同 | PASS |

两组对照的意义：

- `bad-root` vs `ok-fixed`：**只差一个字符串**（`vdz` → `vda`）。前者 panic，后者完整启动。
  这直接证明失败来自参数，不来自内核或镜像。
- `bad-type` vs `bad-init`：两者都 panic，但前者连根都没挂上，后者根挂上了、只差 init 程序。
  panic 消息相同（都是 `Kernel panic - not syncing`），能区分它们的只有 panic 之前的读数。

同一内核的证明：六份日志的 `Linux version 7.0.14 ... #4 SMP Sun Oct 25 17:02:10 CST 2026`
完全一致，由 `judge_same_kernel` 逐份核对。

## 判定器

`run.sh` 对每组做 `expect`（必须出现的串）与 `refute`（不得出现的串）两类检查：

| 用例 | 核心判据 |
| --- | --- |
| `ok` / `ok-fixed` | `BADARGS|init|started` + `root_fs|magic=0xef53` + marker + `init|done` |
| `bad-root` | `VFS: Unable to mount root fs` + `Cannot open root device "/dev/vdz"` + `Please append a correct "root=" boot option` + 内核文件系统清单含 `ext4` + **无** init 行 |
| `bad-type` | `EXT2-fs (vda): error` + `No filesystem could mount root, tried:` + `ext2` + **无** init 行 |
| `bad-init` | `EXT4-fs (vda): mounted filesystem` + `VFS: Mounted root (ext4 filesystem)` + `Run /nope as init process` + `Requested init /nope failed` + **无** init 行 |
| `bad-console` | 同 `ok` + `printk: console [ttyAMA0] enabled` |

`bad-root` 里的"内核文件系统清单含 `ext4`"这一条不是装饰：它证明 ext4 驱动在核心里，
所以挂不上根只能是参数问题。没有这条，"内核缺 ext4"这个解释无法反驳。

## 负向验证

在同一份日志的临时副本上做，不重跑 QEMU（`run.sh --judge <用例> <日志>`）：

| 变异 | 期望 | 实测 |
| --- | --- | --- |
| 删掉 `Please append a correct "root=" boot option` | 失败 | 拒绝 |
| 删掉 `EXT2-fs (vda)` 报错行 | 失败 | 拒绝 |
| 用 `ok.log` 冒充 `bad-root.log` | 失败（`refute` 生效） | 拒绝 |
| 删掉 `printk: console [ttyAMA0] enabled` | 失败 | 拒绝 |
| 删掉 `Requested init /nope failed` | 失败 | 拒绝 |
| 把 `magic=0xef53` 改成 `magic=0xdead` | 失败 | 拒绝 |
| 日志文件不存在 | 失败 | 拒绝 |

7/7 按预期拒绝。特别地，"用成功日志冒充失败用例"这一条验证的是 `refute`：
只靠"该出现的出现了"不够，失败用例还必须证明"init 没有跑起来"。

## 常见错误与陷阱

1. **带着 initramfs 测 `root=`**：`/init` 会被优先执行，`root=` 从没被尝试，
   于是"坏 root 居然也能起来"——这不是内核宽容，是参数压根没用上。
2. **用退出码判断成败**：panic 用例的退出码是 124（被 timeout 杀掉），不是非零错误码；
   成功用例才是 0。拿 124 当"程序崩溃"会得出相反结论。
3. **只给一个不存在的 console**：系统可能完全正常，只是输出全丢。必须保留一个有效 console。
4. **把 `rootfstype=` 写错当成本质区别**：`bad-type` 里镜像**能**被正确识别为 ext4，
   是"强制按 ext2 解析"失败，不能用它证明"镜像坏了"。
5. **`init=` 失败当成 `root=` 失败**：`bad-init` 日志里有 `EXT4-fs (vda): mounted filesystem`，
   根是好的。只看最后的 panic 行会把两者混为一谈。

## 故障排查方法

1. 先看 `Kernel command line:` 确认内核收到的参数与你以为的一致。
2. 找 `VFS: Cannot open root device`（设备问题）与 `VFS: Unable to mount root fs`（超级块问题）的分界。
3. 看 `List of all bdev filesystems:`：内核支持哪些文件系统；没有目标类型就是配置问题。
4. 看 `List of all partitions:`：设备到底有没有被识别。
5. 有 initramfs 时先确认 `rdinit=`/`/init` 是否抢在前面。

## 练习题

1. 增加一组 `bad-rootwait`：`root=/dev/vdz rootwait`。预测它与 `bad-root` 的行为差别，
   实测验证，并解释为什么两者最终都 panic 但耗时不同。
2. 去掉 `bad-init` 的 `refute 'BADARGS|init|started'` 判据，用哪种变异能证明这条判据是必要的？
3. 构造一组"内核真的配置错"的对照（例如用一个不含 ext4 的内核），
   证明现有判据能把两种失败区分开——注意这需要另一个内核镜像，不能只改命令行。

## 验收标准

- [ ] 能解释 `root=`、`rootfstype=`、`init=`、`console=` 各自在内核启动链的哪一步生效
- [ ] 能说出 `bad-root` 与 `bad-type` 在 panic 之前的读数差别
- [ ] 能解释为什么本实验不能带 initramfs
- [ ] 能解释 panic 用例退出码 124 的原因
- [ ] 能用 `run.sh --judge` 单独复判任意一份日志

## 延伸阅读

- `../BSP_03_SwitchRoot/`：有 initramfs 的两阶段启动与 `switch_root`
- `../BSP_04_PivotRoot/`：`pivot_root` 与 `chroot` 的同口径对照
- `../../knowledge_base/BSP启动链与切根_速查表.md`：启动链阶段的速查表
