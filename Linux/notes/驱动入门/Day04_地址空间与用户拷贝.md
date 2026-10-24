# Day4：地址空间、模块信息与用户拷贝

**验证状态**：未验证。WSL2 缺少匹配内核头文件且不支持模块加载，本篇的编译与运行结果均为预期值，见文末"验证记录"。

## 学习目标

学完本篇应能：

1. 说明用户空间地址与内核空间地址为什么不能直接互相解引用。
2. 正确使用 `copy_to_user()` / `copy_from_user()`，并处理返回值。
3. 解释为什么内核不能直接用 `memcpy` 拷贝用户缓冲区。
4. 说明 `size` 参数与固定长度拷贝混用时的风险。
5. 知道模块信息可以从 `/sys/module/<name>/` 与 `modinfo` 获取。

## 环境与前置条件

| 项目 | 要求 |
| --- | --- |
| 内核头文件 | `/lib/modules/$(uname -r)/build` |
| 权限 | `insmod` / `mknod` 需要 root |
| 前置 | 已完成 Day3 的字符设备注册与调用链 |

本机 WSL2 不具备构建与加载条件，需 QEMU、虚拟机或开发板。

## 核心概念

### 1. 两套地址空间

```text
用户态进程地址空间          内核地址空间
  0x0000_0000 ~ ...           0xffff_8000_0000_0000 ~ ...
  栈、堆、.text、.data         直接映射区、vmalloc 区、模块区
        |                              |
        +---- 通过系统调用边界传递数据 ----+
```

用户指针在驱动里只是一个数值，**内核不能保证它指向的页当前有效**，直接解引用可能：

- 触发缺页异常且发生在内核态 → oops；
- 读到用户故意构造的非法地址 → 安全漏洞；
- 在非当前进程上下文访问时语义错误。

因此必须使用专门的拷贝接口，它们内部会做地址合法性检查与页锁定。

### 2. `copy_to_user` / `copy_from_user` 的方向

| 接口 | 方向 | 用在驱动的 |
| --- | --- | --- |
| `copy_to_user(to, from, n)` | 内核 → 用户 | `read()` |
| `copy_from_user(to, from, n)` | 用户 → 内核 | `write()` |

记忆方法：函数名中的 `to_user` / `from_user` 描述的是**用户侧是目的地还是来源**，不是数据流向的起点。

### 3. 返回值语义

两者返回**未能拷贝的字节数**：

```text
返回 0       全部成功
返回 n       全部失败
返回 0<r<n   部分失败
```

因此正确写法是：

```c
if (copy_to_user(buf, &ker_val, n))
	return -EFAULT;
```

课程示例 `05_transfer_data` 忽略返回值，属于工程缺陷。

### 4. `size` 参数不能忽略

课程示例固定拷贝 4 字节：

```c
copy_to_user(buf, &ker_val, 4);
return 4;
```

如果用户调用 `read(fd, buf, 1)`，用户只提供了 1 字节缓冲区，驱动却写 4 字节——**越界写用户内存**。

正确做法：

```c
size_t n = min(size, sizeof(ker_val));
if (copy_to_user(buf, &ker_val, n))
	return -EFAULT;
return n;
```

### 5. 模块信息从哪里看

| 来源 | 内容 |
| --- | --- |
| `modinfo xxx.ko` | 许可证、作者、描述、vermagic、依赖 |
| `/sys/module/<name>/` | 参数、引用计数、节区信息 |
| `/proc/modules` | 已加载模块与内存占用 |
| `lsmod` | 上述内容的格式化输出 |

模块参数（`module_param`）会在 `/sys/module/<name>/parameters/` 下出现，可用于运行期观察。

## 系统调用/接口卡片

### `unsigned long copy_to_user(void __user *to, const void *from, unsigned long n)`

作用：把内核缓冲区内容拷贝到用户缓冲区。

参数：`to` 必须是用户空间指针（带 `__user` 标注）；`n` 为字节数。

返回值和错误：返回未拷贝字节数；非 0 通常转换为 `-EFAULT` 返回给用户。

资源所有权：无。

线程/进程安全：只能访问调用它的进程的用户地址空间；在中断上下文或不相关进程上下文中调用会失败。

最小示例：

```c
size_t n = min(size, sizeof(ker_val));
if (copy_to_user(buf, &ker_val, n))
	return -EFAULT;
return n;
```

常见误用：忽略返回值；`n` 超过用户缓冲区；对内核指针使用该接口。

### `unsigned long copy_from_user(void *to, const void __user *from, unsigned long n)`

作用：把用户缓冲区内容拷贝到内核缓冲区。

参数：`from` 是用户空间指针；`to` 是内核缓冲区；`n` 为字节数。

返回值和错误：返回未拷贝字节数，语义同上。

线程/进程安全：同上，必须在能访问当前进程用户地址空间的上下文调用。

最小示例：

```c
size_t n = min(size, sizeof(ker_val));
if (copy_from_user(&ker_val, buf, n))
	return -EFAULT;
return n;
```

常见误用：先 `memcpy` 再检查，已经太晚；拷贝长度固定而忽略 `size`。

### `min(a, b)` / `max(a, b)` / `clamp(v, lo, hi)`

作用：内核提供的类型安全比较宏，避免手写三元表达式时的类型不一致。

最小示例：`size_t n = min(size, sizeof(ker_val));`

常见误用：对指针或不同类型直接使用导致编译警告。

### `module_param(name, type, perm)`

作用：声明模块参数，允许 `insmod xxx.ko name=value` 传入，并在 `/sys/module/xxx/parameters/` 暴露。

参数：`type` 常见为 `int`、`charp`、`bool`；`perm` 为 sysfs 权限（如 `0644`，`0` 表示不可见）。

最小示例：`static int ker_val = 123; module_param(ker_val, int, 0644);`

常见误用：权限写 `0` 后又在用户态找参数文件；类型与变量声明不匹配。

## 最小可运行实验

实验目录：`Linux/experiments/Day04_地址空间与用户拷贝/`

```text
hello_copy.c        修正版：按 size 截断、检查返回值
hello_copy_test.c   用户态读写测试，含短缓冲区场景
Makefile            模块与测试程序构建
README.md           步骤与预期结果
```

驱动核心：

```c
static int ker_val = 123;

static ssize_t hello_read(struct file *file, char __user *buf,
			  size_t size, loff_t *offset)
{
	size_t n = min(size, sizeof(ker_val));

	if (size == 0)
		return 0;
	if (copy_to_user(buf, &ker_val, n))
		return -EFAULT;

	pr_info("hello_copy: read %zu byte(s), ker_val=%d\n", n, ker_val);
	return n;
}
```

## 编译与运行

```bash
cd Linux/experiments/Day04_地址空间与用户拷贝
make

sudo insmod hello_copy.ko
sudo dmesg | tail -3          # 记录 major
sudo mknod /dev/hello_copy c <major> 0
sudo chmod 666 /dev/hello_copy

./hello_copy_test -r
./hello_copy_test -w 456
./hello_copy_test -r
./hello_copy_test -s          # 短缓冲区场景，验证不会越界
sudo dmesg | tail -10

sudo rm /dev/hello_copy
sudo rmmod hello_copy
```

## 结果观察

以下为**预期结果，未在本机实测**：

```text
$ ./hello_copy_test -r
read -> 4
value = 123

$ ./hello_copy_test -w 456
$ ./hello_copy_test -r
read -> 4
value = 456

$ ./hello_copy_test -s
read short -> 1          # 只请求 1 字节，驱动只拷 1 字节
```

对比课程 `05_transfer_data`：它固定拷贝 4 字节并返回 4，无论用户请求多少。

## 常见错误和陷阱

| 现象 | 原因 | 定位方法 |
| --- | --- | --- |
| 内核 oops，调用栈含 `hello_read` | 直接解引用用户指针 | 改为 `copy_to_user` |
| 用户缓冲区被写坏 / 堆损坏 | 固定长度拷贝超过 `size` | 用 `min(size, sizeof(...))` |
| 偶发 `-EFAULT` | 忽略返回值，用户地址非法时被当成成功 | 检查返回值并返回 `-EFAULT` |
| 在中断上下文拷贝失败 | 无当前进程地址空间 | 把拷贝移到进程上下文（如 workqueue） |
| `/sys/module/xxx/parameters` 不存在 | `module_param` 权限为 0 | 改为 `0644` |
| 编译报 `__user` 未识别 | 缺少 `<linux/uaccess.h>` | 补头文件；旧内核可能用 `<asm/uaccess.h>` |

## 故障排查方法

1. **看 oops 栈**：`dmesg` 中 `RIP` 指向哪个函数，能直接区分是拷贝接口用错还是指针解引用。
2. **加长度日志**：在回调里打印 `size` 与实际拷贝长度。
3. **用短缓冲区复现**：用户侧故意请求 1 字节，检验驱动是否越界。
4. **检查返回值路径**：确认每个 `copy_*_user` 都有分支处理。
5. **确认上下文**：中断上下文禁止睡眠，也不保证能访问用户地址空间。

## 练习题

### 基础题

题目：让驱动支持一个整数数组（4 个 `int`），`read` 时按 `size` 拷贝尽可能多的元素。

提示：`size_t n = min(size, sizeof(arr));`，返回实际字节数。

验收标准：用户请求 6 字节时返回 4（对齐到元素边界），请求 16 字节时返回 16。

### 改错题

题目：下面的 `write` 回调有什么问题？

```c
static ssize_t hello_write(struct file *f, const char __user *buf,
			   size_t size, loff_t *off)
{
	copy_from_user(&ker_val, buf, size);
	return size;
}
```

提示：考虑三个问题——返回值未检查、长度未限制、`size` 可能大于目标缓冲区。

验收标准：能指出三处问题并给出修正版。

### 综合题

题目：增加一个 `ioctl` 命令，用于在不解锁设备的情况下读取 `ker_val` 当前值，并在 `dmesg` 中记录调用次数。

提示：先查当前内核版本 `unlocked_ioctl` 的签名；使用 `_IOR` / `_IOW` 宏定义命令号。

验收标准：用户态 `ioctl` 能取回当前值；`dmesg` 中的计数随调用递增。

## 验收标准

- [ ] 能说明用户指针为什么不能直接解引用。
- [ ] 能写出带返回值检查的 `copy_to_user` / `copy_from_user` 代码。
- [ ] 能解释 `min(size, sizeof(...))` 的必要性。
- [ ] 能指出课程 `05_transfer_data` 的两个工程缺陷。
- [ ] 在具备条件的环境中完成一次读写验证（当前环境未完成）。

## 延伸阅读

- `Linux_WEIDS/02_嵌入式Linux驱动入门直播/StudyPlan/DailyDocs/04_Day4_辅助信息与地址空间_学习讲解.md`
- `Linux_WEIDS/02_嵌入式Linux驱动入门直播/02_彻底分析第1个驱动程序/source/05_transfer_data/`
- `Linux/knowledge_base/Linux驱动入门_接口与版本速查表.md`
- 下一篇：`Linux/notes/驱动入门/Day05_交叉编译与板级驱动.md`

---

**验证记录**

| 项目 | 内容 |
| --- | --- |
| 日期 | 2026-10-25 |
| 环境 | WSL Ubuntu 26.04，gcc 15.2.0 / make 4.4.1，内核头文件缺失 |
| 已执行 | 源码阅读：`05_transfer_data/hello_drv.c` 与测试程序 |
| 未执行 | 编译、加载、读写验证 |
| 结论 | 运行结果为预期值，未实测 |
