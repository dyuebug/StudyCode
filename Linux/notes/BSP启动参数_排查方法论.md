# BSP 启动参数排查方法论：把"参数错"与"配置错"分开

**建立日期**：2026-10-28
**来源实验**：[`experiments/BSP_05_BadBootargs`](../experiments/BSP_05_BadBootargs/README.md)（六组对照，6/6 通过）
**记录性质**：判据分层与实测措辞来自日志；内核源码路径为**源码推断**，未逐函数验证。

---

## 1. 这份笔记解决什么问题

嵌入式板子起不来时，现场只有一句"串口没输出"或一段 panic。此时最常见的错误结论是
"内核配置有问题"，于是去重编内核、改 `CONFIG_*`、换 DTB——在参数写错的情况下，
这些动作全部无效，而且会掩盖真正的原因。

`BSP_05` 的做法是：**同一个内核、同一块镜像、只改命令行一个参数**，跑六组。
任何一组能完整跑到用户态 init，就证明内核侧（块设备驱动、ext4、VFS、init 查找）全都可用；
其余组的失败只能归因到命令行。这一步不是"顺便验证"，它是把两种解释**排除掉一种**的唯一手段。

六份日志的 `Linux version` 行逐字一致，就是"同一内核"这条前提的证据。

---

## 2. 前置概念：无 initramfs 时的四道门

```text
内核解压
  ├─ ① 串口注册        ← console=
  └─ prepare_namespace()
       ├─ ② 按 root= 找块设备
       ├─ ③ 按 rootfstype= 挂文件系统
       └─ ④ 按 init= exec 用户态
```

每一道门过了，日志里就多一条**唯一的成功行**。排查时不要问"哪一步错了"，
而要问"**最后出现的那条成功行是哪一条**"——它下面的第一道门就是嫌疑。

| 门 | 成功行（实测） | 失败行（实测） |
| --- | --- | --- |
| ① 串口 | `printk: console [ttyAMA0] enabled` | **没有失败行**（见 §5 坑 2） |
| ② 找设备 | （无独立成功行，隐含在 ③） | `VFS: Cannot open root device "/dev/vdz" or unknown-block(0,0): error -6` |
| ③ 挂载 | `VFS: Mounted root (ext4 filesystem) on device 254:0.` | `EXT2-fs (vda): error: ... unsupported optional features (22c4)` + `No filesystem could mount root, tried:` |
| ④ exec | `Run /sbin/init as init process` → 用户态第一行输出 | `Run /nope as init process` + `Requested init /nope failed (error -2)` |

`VFS: Mounted root ...` 出现即证明**②③ 全过**，包括块设备驱动和文件系统驱动都已编入并工作。
这条行是排查里最有价值的一条，因为它一次排除掉两大类原因。

---

## 3. 判据分层：两个 panic 长得像，含意不同

`root=` 错与 `rootfstype=` 错都会 panic 到 `VFS: Unable to mount root fs`。
两者的区别只在**设备找没找到**：

| 参数 | panic 文案里的设备字段 | 前面有没有 `EXT2-fs` 尝试行 |
| --- | --- | --- |
| `root=/dev/vdz`（设备不存在） | `unknown-block(0,0)` | 没有 |
| `rootfstype=ext2`（设备在、类型错） | `"/dev/vda" or unknown-block(254,0)` | **有**，且带 `unsupported optional features (22c4)` |

- 设备字段是 `unknown-block(0,0)` ⇒ **没找到设备**，去查设备名、驱动、分区。
- 设备字段带真实主次设备号 ⇒ **找到了设备**，失败在文件系统层，去查类型、`CONFIG_`、镜像内容。

`(254,0)` 是 virtio-blk 的主次设备号，说明"设备已经能被打开了"。这条算术比文字描述可靠。

---

## 4. 反向用法：`ok` 组才是关键证据

六组里最容易被视为"凑数"的 `ok` 组，实际承担两件事：

1. **内核可用性对照**：证明这套内核 + 这个 DTB + 这块镜像本来能启动。
   没有它，"其余五组失败"永远可以解释为环境坏了。
2. **判据基线**：`ok` 组日志里包含了所有成功行。判据的 `expect` 列表就是从它抄出来的，
   `refute` 列表（例如"失败用例里不得出现 `BADARGS|init|started`"）也靠它标定。

`ok-fixed` 组是**修复验证**：把坏参数改对，同一个用例必须转绿。
少了这一步，"参数改了"和"参数改对了"分不开。

---

## 5. 五个容易踩的坑（每个都有实测读数）

### 坑 1：用 initramfs 测 `root=`，参数根本不生效

有 initramfs 时内核先执行 `rdinit`，真实根由 initramfs 里的程序去挂，内核自己的
`prepare_namespace()` 与 `root=` 不参与。此时在命令行写坏 `root=` **什么都不会发生**。

`BSP_02` 已实测过这个对照：命令行含 `root=/dev/vda`，但日志里**没有任何 `EXT4-fs (vda)` 行**，
根仍是 `rootfs`。所以 `BSP_05` 全组**不带 initramfs**——这不是偷懒，是让内核自己挂根的前提。

### 坑 2：以为 `console=` 写错会启动失败

`console=` 是"注册候选控制台"，不是"必须用这个"。名字对不上就跳过，内核继续用已注册的串口打印。
`bad-console` 组实测：命令行 `console=ttyS9 console=ttyAMA0 ...`，系统**完整启动到用户态**，
`ttyS9` 一点痕迹都没有。

因此这条参数**没有失败模式**，只能正向确认"哪个赢了"：`printk: console [ttyAMA0] enabled`。
如果串口完全没输出，问题在 ① 之前（DTB、内存、时钟），不在 bootargs。

### 坑 3：以为 panic 会让 QEMU 返回非零退出码

`-no-reboot` 只阻止**重启**，不阻止**挂住**。panic 之后进程停在那里等，
`timeout` 到期返回 **124**。所以 124 是**预期的失败形态**，harness 不应该把它当成缺陷去修。

反过来：如果一个"应该 panic"的用例返回 0 或非 124，说明它其实没 panic 到——判据要能区分。

### 坑 4：`init=` 错被误读成"内核有问题"

恰恰相反。`bad-init` 组日志里有：

```text
EXT4-fs (vda): mounted filesystem ... r/w with ordered data mode
VFS: Mounted root (ext4 filesystem) on device 254:0.
VFS: Pivoted into new rootfs
Run /nope as init process
Kernel panic - not syncing: Requested init /nope failed (error -2).
```

这一组是**内核最健康**的一组失败：块设备、ext4、VFS 全部正常工作，只差用户态程序。
看到 `Requested init ... failed (error -2)`，`-2` 是 `ENOENT`，去查根文件系统里那个路径存不存在，
**不要**去重编内核。

### 坑 5：判据写成"日志里有 panic 就算失败"

三个 panic 用例的判据必须各自不同，否则无法定位：

- `bad-root`：**不得**出现 `VFS: Mounted root`
- `bad-type`：**必须**出现 `EXT2-fs` 尝试行，且**不得**出现 `VFS: Mounted root`
- `bad-init`：**必须**出现 `VFS: Mounted root` 与 `Requested init`，且**不得**出现用户态输出

只用"有 panic"一条判据，三组会被判成同一个结果，实验就退化成一次重复。

---

## 6. 判断顺序（现场可执行版）

```text
串口一个字都没有？
  └─ 是 → 查 DTB / DDR / 时钟，不要动 bootargs
  └─ 否 ↓

有 "VFS: Mounted root" 吗？
  ├─ 有 → 内核与文件系统都正常，问题在 rootfs 内容或 init=
  │        → "Requested init X failed (error -2)" ⇒ 路径不存在
  │        → 无任何 exec 相关行 ⇒ init= 参数或 /sbin/init 缺失
  └─ 没有 ↓

有 "Cannot open root device" 吗？
  ├─ 有，且 unknown-block(0,0) → 设备没找到：设备名 / 驱动 / 分区
  └─ 没有，但有 EXT2-fs 尝试行 → 设备找到了，文件系统类型或镜像不对

以上都对得上但两台机器行为不同 → 先确认是不是同一个内核（比对 Linux version 行）
```

---

## 7. 当前验证状态

| 结论 | 状态 | 证据 |
| --- | --- | --- |
| 四道门的分层与各自 panic 措辞 | **实测** | `BSP_05` 六份日志（`ok`/`bad-root`/`bad-type`/`bad-init`/`bad-console`/`ok-fixed`） |
| `console=` 无失败模式 | **实测** | `bad-console` 组完整启动 |
| panic 后退出码 124 | **实测** | `run.sh` 的 `case_timeout` 与日志生成路径 |
| 带 initramfs 时 `root=` 不参与 | **实测（他人实验）** | `BSP_02` 的 `uboot-initrd-vs-root` 组 |
| `prepare_namespace()` 的逐函数路径 | **源码推断** | 未逐函数验证，只由日志行反推 |
| 真板上的表现（BootROM/SPL、`rootwait`、异步探测） | **未验证** | 无真板 |

---

## 8. 待解决

- 真板上 `root=` 指向尚未枚举完成的 eMMC/SD 时，是否需要 `rootwait`——`BSP_02` 只提到未验证，
  没有实际读数。
- `rootfstype=` 留空时的自动探测顺序（`No filesystem could mount root, tried:` 后面那串列表）
  没有单独实验，目前只作为 `bad-type` 的副产物出现。
- 一个"改坏参数但设备仍能启动"的组合（坏 `console=` + 好 `root=`）已由 `bad-console` 覆盖，
  但坏 `root=` 且存在于多个候选设备的情况未覆盖。
