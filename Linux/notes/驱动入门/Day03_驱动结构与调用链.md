# Day3：驱动结构、设备文件与读写调用链

**验证状态**：未验证。当前环境无法构建或加载内核模块，本篇的 `mknod`、`open/read/write` 调用链均为源码分析结论，见文末"验证记录"。

## 学习目标

学完本篇应能：

1. 说明 `register_chrdev()` 与 `unregister_chrdev()` 做了什么。
2. 解释主设备号与次设备号分别标识什么。
3. 说清 `/dev/xyz` 这个设备文件如何与驱动的 `file_operations` 关联。
4. 画出应用 `open/read/write` 到驱动回调的完整调用链。
5. 解释为什么本日示例的 `read/write` 返回 1 却不是"传输了 1 字节"。

## 环境与前置条件

| 项目 | 要求 |
| --- | --- |
| 内核头文件 | `/lib/modules/$(uname -r)/build` |
| root 权限 | 加载模块、创建设备节点 |
| 工具 | `mknod`、`dmesg`、`lsmod`、`cat /proc/devices` |

本机 WSL2 不具备上述条件（无头文件、不支持模块加载），需改用 QEMU、虚拟机或真实开发板。

## 核心概念

### 1. 三个对象的关系

```text
设备文件 /dev/xyz  --(主设备号)-->  驱动 hello_drv  --(file_operations)-->  函数 hello_open/read/write
```

- **设备文件**是用户态可见的入口，本身不含代码。
- **主设备号**告诉内核"这个设备归哪个驱动管"。
- **次设备号**由驱动自己解释，用于区分同一驱动下的多个实例。
- **file_operations** 是驱动向内核注册的函数表。

### 2. 注册流程

```text
module_init
  -> register_chrdev(0, "hello_drv", &hello_fops)
     0 表示请内核动态分配主设备号
     返回值 = 主设备号（>= 0）或负错误码
  -> 打印/记录 major
用户态
  -> mknod /dev/xyz c <major> 0
  -> open("/dev/xyz") 时内核按 major 找到 hello_fops
```

### 3. 两种创建设备节点的方式

| 方式 | 命令/接口 | 适用 |
| --- | --- | --- |
| 手动 | `mknod /dev/xyz c <major> 0` | 教学、调试 |
| 自动 | `class_create()` + `device_create()` | 工程实践，由 udev/mdev 自动建节点 |

本日课程示例只用了手动 `mknod`，设备节点在重启或 `rmmod` 后不会自动消失。自动创建属于后续工程化内容。

### 4. 调用链

```text
应用:  fd = open("/dev/xyz", O_RDWR)
内核:  sys_openat -> do_filp_open -> VFS -> chrdev_open -> hello_fops.open
应用:  write(fd, buf, len)
内核:  sys_write -> vfs_write -> hello_fops.write
应用:  read(fd, buf, len)
内核:  sys_read -> vfs_read -> hello_fops.read
应用:  close(fd)
内核:  filp_close -> hello_fops.release
```

VFS 是统一入口，驱动只提供回调。**驱动不决定何时被调用，只实现被调用时做什么。**

### 5. 为什么示例的 read/write 返回 1 不是传输 1 字节

课程 `04_call` 的回调只做 `printk` 后 `return 1`：

```c
static ssize_t hello_read(struct file *file, char __user *buf, size_t size, loff_t *offset)
{
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	return 1;
}
```

它没有调用 `copy_to_user()`，所以用户态缓冲区内容没有被写入。返回值 1 只表示"内核认为处理了 1 字节"。

这引出一个关键规则：**返回值必须与实际发生的传输一致**，否则用户态会读到未初始化数据。

真正的数据拷贝在 Day4 讲解（`copy_to_user` / `copy_from_user`）。

## 系统调用/接口卡片

### `int register_chrdev(unsigned int major, const char *name, const struct file_operations *fops)`

作用：注册一个字符设备驱动，分配主设备号。

参数：

- `major`：0 表示动态分配；非 0 表示指定主设备号。
- `name`：设备名，出现在 `/proc/devices`，长度有限制。
- `fops`：函数表指针。

返回值和错误：成功返回主设备号（>= 0）；失败返回负错误码（如 `-EBUSY`、`-ENOMEM`）。

资源所有权：成功后必须用同一 `major` 调用 `unregister_chrdev()`，否则主设备号泄漏。

线程/进程安全：注册/注销应在模块初始化与清理中执行，不与并发 I/O 同时进行。

最小示例：

```c
static int major;
major = register_chrdev(0, "hello_drv", &hello_fops);
if (major < 0)
	return major;
```

常见误用：把返回值当"0 成功"判断；失败时不回滚；忘记注销。

### `void unregister_chrdev(unsigned int major, const char *name)`

作用：注销字符设备，释放主设备号。

参数：必须与注册时一致。

返回值和错误：无返回值。

资源所有权：注销后不应再让应用访问该设备文件。

最小示例：`unregister_chrdev(major, "hello_drv");`

常见误用：`name` 与注册时不一致，导致注销失败或警告。

### `struct file_operations`

作用：驱动向 VFS 注册的回调表。

常用成员：

| 成员 | 触发时机 |
| --- | --- |
| `.owner` | 固定填 `THIS_MODULE`，用于引用计数 |
| `.open` | 应用 `open()` |
| `.release` | 应用 `close()` |
| `.read` | 应用 `read()` |
| `.write` | 应用 `write()` |
| `.unlocked_ioctl` | 应用 `ioctl()` |
| `.poll` | 应用 `select()/poll()/epoll()` |

资源所有权：函数表通常是 `static` 全局对象，生命周期与模块一致。

常见误用：漏写 `.owner` 导致模块可在仍被打开时卸载；回调签名与内核版本不匹配。

### `mknod <path> c <major> <minor>`

作用：创建字符设备节点。

参数：`c` 表示字符设备；`major` 必须与驱动注册值一致；`minor` 通常为 0。

返回值和错误：失败时返回非 0，常见原因是没有权限或路径已存在。

最小示例：`sudo mknod /dev/xyz c 240 0`

常见误用：`major` 写错导致打开的是别的驱动或得到 `ENODEV`；忘记 `chmod` 导致普通用户无权访问。

### 用户态 `open` / `read` / `write` / `close`

| 接口 | 原型要点 | 与驱动的关系 |
| --- | --- | --- |
| `open` | `int open(const char *path, int flags)` | 触发 `fops->open` |
| `read` | `ssize_t read(int fd, void *buf, size_t count)` | 触发 `fops->read` |
| `write` | `ssize_t write(int fd, const void *buf, size_t count)` | 触发 `fops->write` |
| `close` | `int close(int fd)` | 触发 `fops->release` |

返回值和错误：成功返回实际字节数；失败返回 -1 并设置 `errno`。

## 最小可运行实验

实验目录：`Linux/experiments/Day03_字符设备调用链/`

```text
hello_chrdev.c        字符设备驱动：注册 + open/read/write/release 回调
hello_chrdev_test.c   用户态测试程序：open / read / write / close
Makefile              模块构建规则
README.md             构建、mknod、测试、观察步骤
```

驱动核心：

```c
static int major;
static struct file_operations hello_fops = {
	.owner   = THIS_MODULE,
	.open    = hello_open,
	.release = hello_release,
	.read    = hello_read,
	.write   = hello_write,
};

static int __init hello_init(void)
{
	major = register_chrdev(0, "hello_chrdev", &hello_fops);
	pr_info("hello_chrdev: major = %d\n", major);
	return major < 0 ? major : 0;
}
```

## 编译与运行

```bash
cd Linux/experiments/Day03_字符设备调用链
make

sudo insmod hello_chrdev.ko
sudo dmesg | tail -5          # 记录 major
cat /proc/devices | grep hello_chrdev

sudo mknod /dev/hello_drv c <major> 0
sudo chmod 666 /dev/hello_drv

./hello_chrdev_test -w abc
./hello_chrdev_test -r
sudo dmesg | tail -20

sudo rm /dev/hello_drv
sudo rmmod hello_chrdev
sudo dmesg | tail -5
```

注意：`<major>` 必须替换为 `dmesg` 中打印的实际值。

## 结果观察

以下为**预期结果，未在本机实测**：

```text
$ sudo dmesg | tail -2
[  ...] hello_chrdev: major = 240
[  ...] hello_chrdev: init

$ ./hello_chrdev_test -w abc
open /dev/hello_drv ok
write -> 1

$ sudo dmesg | tail -4
[  ...] hello_chrdev: open
[  ...] hello_chrdev: write size=4
[  ...] hello_chrdev: release
```

要确认的是**调用顺序**，而不是数据内容：本日没有 `copy_to_user`，所以 `-r` 读到的缓冲区是未初始化的。

## 常见错误和陷阱

| 现象 | 原因 | 定位方法 |
| --- | --- | --- |
| `open` 返回 `ENODEV` | `mknod` 的 major 与驱动不一致 | `cat /proc/devices` 对比 |
| `open` 返回 `EACCES` | 设备节点权限不足 | `ls -l /dev/xxx` |
| `read/write` 返回 `-1 EINVAL` | `fops` 未实现对应回调 | 检查 `file_operations` 成员 |
| `rmmod` 报 `Module is in use` | 引用计数非 0 或有进程占用 | `lsmod` 第三列、`lsof /dev/xxx` |
| 卸载后 `/proc/devices` 仍有记录 | 忘记 `unregister_chrdev` | 对比 `register` 与 `unregister` |
| 返回值与 `size` 不一致 | 回调返回固定值 | 检查 `return` 语句 |
| 模块可被卸载但设备仍被打开 | `fops` 漏写 `.owner` | 检查函数表初始化 |

## 故障排查方法

1. **先确认 major**：`cat /proc/devices | grep <name>`，再核对 `mknod` 参数。
2. **再看 dmesg**：驱动内的 `pr_info` 是最直接的调用证据。
3. **分层定位**：应用侧错误码 → VFS 是否进入驱动 → 驱动回调是否执行。
4. **检查引用计数**：`lsmod` 第三列为 0 才能 `rmmod`。
5. **清理残留**：卸载后确认设备节点与 `/proc/devices` 都恢复。

## 练习题

### 基础题

题目：给驱动增加 `.release` 回调，并在其中打印 `pr_info`，验证 `close()` 是否触发。

提示：应用侧必须在 `open` 成功后调用 `close`；提前 `return` 会跳过。

验收标准：`dmesg` 中出现与 open 一一对应的 release 记录。

### 改错题

题目：下面的初始化函数有什么问题？

```c
static int __init hello_init(void)
{
	int major = register_chrdev(0, "hello_drv", &hello_fops);
	pr_info("major = %d\n", major);
	return 0;
}
```

提示：考虑注册失败的情况。

验收标准：能指出失败时仍返回 0 且未注销的问题，并给出正确写法。

### 综合题

题目：让驱动支持次设备号：注册时申请 4 个次设备号，`read` 回调中根据 `MINOR(file->f_inode->i_rdev)` 返回不同的值。

提示：使用 `register_chrdev_region()` / `alloc_chrdev_region()` 配合 `cdev`，而不是 `register_chrdev()`；先查当前内核版本中 `file->f_inode` 的访问方式。

验收标准：创建 `/dev/hello0`～`/dev/hello3` 四个节点，读取各自得到不同结果。

## 验收标准

- [ ] 能画出 `/dev 节点 → major → file_operations → 回调` 的关联图。
- [ ] 能解释 `register_chrdev(0, ...)` 中 0 的含义与返回值语义。
- [ ] 能说明修改 `fops` 后为什么不需要重新 `mknod`。
- [ ] 能指出 `read` 返回 1 与"传输 1 字节"的区别。
- [ ] 在具备条件的环境中完成一次完整调用链验证（当前环境未完成）。

## 延伸阅读

- `Linux_WEIDS/02_嵌入式Linux驱动入门直播/StudyPlan/DailyDocs/03_Day3_彻底分析第一个驱动_学习讲解.md`
- `Linux_WEIDS/02_嵌入式Linux驱动入门直播/02_彻底分析第1个驱动程序/source/03_test/`
- `Linux_WEIDS/02_嵌入式Linux驱动入门直播/02_彻底分析第1个驱动程序/source/04_call/`
- `Linux/knowledge_base/Linux驱动入门_接口与版本速查表.md`
- 下一篇：`Linux/notes/驱动入门/Day04_地址空间与用户拷贝.md`

---

**验证记录**

| 项目 | 内容 |
| --- | --- |
| 日期 | 2026-10-25 |
| 环境 | WSL Ubuntu 26.04（不可构建模块） |
| 已执行 | 源码阅读：`03_test/`、`04_call/`、Makefile |
| 未执行 | 编译、`insmod`、`mknod`、运行测试程序、`dmesg` 观察 |
| 结论 | 调用链结论来自源码与内核 VFS 语义，未实测 |
