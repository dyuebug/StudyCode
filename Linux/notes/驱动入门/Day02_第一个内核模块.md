# Day2：第一个内核模块

**验证状态**：未验证。当前环境的 WSL2 缺少内核头文件且不支持加载自定义模块，本材料的编译、加载、日志观察均未实测，见文末"验证记录"。

## 学习目标

学完本篇应能：

1. 说明内核模块为什么没有 `main()`。
2. 指出 `module_init()`、`module_exit()`、`__init`、`__exit`、`printk()` 的作用。
3. 画出 `make -> insmod -> dmesg -> lsmod -> rmmod` 的模块生命周期。
4. 区分驱动源码、内核模块 Makefile 和用户态测试程序。
5. 指出课程示例在工程化上的至少三个缺口。

## 环境与前置条件

| 项目 | 要求 |
| --- | --- |
| 内核源码或头文件 | `/lib/modules/$(uname -r)/build` 必须存在 |
| 编译器 | 与目标内核匹配的 gcc |
| 权限 | `insmod` / `rmmod` 需要 root |
| 可选 | QEMU + 100ask_imx6ull 镜像，或 Ubuntu 虚拟机 |

本机现状（2026-10-25 实测）：

```text
WSL Ubuntu 26.04 / 内核 6.6.87.2-microsoft-standard-WSL2
gcc: 15.2.0（已安装）
make: 4.4.1（已安装）
/lib/modules/6.6.87.2-microsoft-standard-WSL2/build: 不存在
apt 源中无匹配该内核的 linux-headers 包
结论: 工具链可用，但仍无法在 WSL 中编译或加载内核模块
```

可选验证路径：

1. **QEMU**：使用资料中的 `100ask_imx6ull-qemu` 环境（推荐，符合虚拟化学习原则）。
2. **Ubuntu 虚拟机**（VMware / VirtualBox）：安装 `build-essential` 与对应内核头文件。
3. **真实开发板**：交叉编译后通过 NFS / SD 卡部署。

## 核心概念

### 1. 模块没有 `main()`

用户态程序由运行环境从 `main()` 开始执行；内核模块由内核在 `insmod` 时调用注册的初始化函数。

```text
用户态:  shell -> execve -> C 运行时 -> main()
内核态:  insmod -> 内核加载器 -> module_init 注册的函数
```

因此模块的"入口"是**被注册给内核的函数指针**，而不是固定符号名。

### 2. 两套写法

| 写法 | 形式 | 说明 |
| --- | --- | --- |
| 现代写法 | `module_init(hello_init);` | 宏，内部生成 `init_module` 别名 |
| 传统写法 | 直接定义 `int init_module(void)` | 内核按固定符号名查找 |

课程 `01_simple` 用的是传统写法，`02_printk` 用的是 `module_init()`。两者等价，现代代码应使用宏，因为它能在内置编译与模块编译两种场景下都正确工作。

### 3. `__init` 与 `__exit`

- `__init`：提示链接器把函数放入初始化段，内置编译时初始化完成后可释放该段内存。
- `__exit`：标记只在内核退出或模块卸载时使用的函数；内置编译时可被丢弃。

对模块而言，这两个属性主要影响编译产物布局，不改变调用时机。

### 4. 模块生命周期

```text
源码 .c
  -> make（调用内核构建系统）
  -> hello_drv.ko
  -> insmod（内核校验 vermagic、解析符号、调用 init）
  -> lsmod 可见 / dmesg 有日志
  -> rmmod（调用 exit，释放资源）
  -> dmesg 记录退出
```

### 5. 内核日志与用户态输出的区别

`printk()` 写入内核日志缓冲区，由 `dmesg` 或 `/dev/kmsg` 读取；`printf()` 写入进程的标准输出。二者不在同一个通道，所以 `insmod` 后终端看不到打印，必须看 `dmesg`。

## 系统调用/接口卡片

### `module_init(fn)` / `module_exit(fn)`

作用：向内核注册模块的初始化与清理函数。

原型：

```c
int __init fn(void);
void __exit fn(void);
module_init(fn);
module_exit(fn);
```

参数：函数指针，签名必须与上述一致。

返回值和错误：初始化函数返回 0 表示加载成功；返回负值（如 `-ENOMEM`）表示失败，模块不会被保留。

资源所有权：初始化中申请的资源必须在清理函数中释放，否则模块无法干净卸载。

线程/进程安全：初始化在加载时单次执行；清理在卸载时单次执行，但需与已打开的设备、并发回调协调。

最小示例：

```c
static int __init hello_init(void) { pr_info("init\n"); return 0; }
static void __exit hello_exit(void) { pr_info("exit\n"); }
module_init(hello_init);
module_exit(hello_exit);
```

常见误用：初始化失败却返回 0；忘记 `module_exit`；初始化中申请资源后失败返回却不回滚。

### `printk(fmt, ...)` / `pr_info(fmt, ...)`

作用：向内核日志缓冲区输出。

参数：格式串 + 可变参数，语义与 `printf` 类似但不支持浮点。

返回值和错误：返回写入的字节数；内核日志缓冲区满时可能丢弃。

资源所有权：无。

线程/进程安全：可在中断上下文调用（受上下文限制），但有速率与缓冲区约束。

最小示例：`pr_info("hello: init\n");`

常见误用：把 `printf` 的浮点格式用于 `printk`；在中断上下文做长时间输出；忘记日志级别导致控制台被刷屏。

### `MODULE_LICENSE("GPL")`

作用：声明模块许可证，影响内核是否允许其使用 GPL-only 符号。

参数：许可证字符串，如 `"GPL"`、`"Dual BSD/GPL"`。

返回值和错误：无返回值；缺失时加载会打印 tainted 警告，且部分符号不可用。

资源所有权：无。

最小示例：`MODULE_LICENSE("GPL");`

常见误用：漏写导致 `insmod` 后内核被标记为污染，或无法解析 GPL-only 符号。

### `insmod` / `rmmod` / `lsmod` / `dmesg` / `modinfo`

| 命令 | 作用 | 典型用法 |
| --- | --- | --- |
| `insmod` | 加载指定 `.ko` | `sudo insmod hello_drv.ko` |
| `rmmod` | 卸载已加载模块 | `sudo rmmod hello_drv` |
| `lsmod` | 列出已加载模块 | `lsmod \| grep hello` |
| `dmesg` | 读取内核日志 | `sudo dmesg \| tail -20` |
| `modinfo` | 查看模块元信息 | `modinfo hello_drv.ko` |

## 最小可运行实验

实验目录：`Linux/experiments/Day02_首次模块加载/`

```text
hello_module.c   最小模块：只打印，不接硬件
Makefile         kbuild 外部模块构建规则
README.md        构建、加载、卸载与验证步骤
```

核心代码：

```c
#include <linux/module.h>
#include <linux/kernel.h>

static int __init hello_init(void)
{
	pr_info("hello_module: init\n");
	return 0;
}

static void __exit hello_exit(void)
{
	pr_info("hello_module: exit\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Day2 minimal kernel module");
```

## 编译与运行

在具备内核头文件的 Linux 中：

```bash
cd Linux/experiments/Day02_首次模块加载
make

sudo insmod hello_module.ko
lsmod | grep hello_module
sudo dmesg | tail -5

sudo rmmod hello_module
sudo dmesg | tail -5

make clean
```

Makefile 的关键一行：

```make
obj-m += hello_module.o
$(MAKE) -C $(KDIR) M=$(PWD) modules
```

`-C $(KDIR)` 表示进入内核构建目录执行，`M=$(PWD)` 告诉内核"模块源码在当前目录"。

## 结果观察

以下为**预期结果，未在本机实测**：

```text
$ sudo insmod hello_module.ko
$ sudo dmesg | tail -2
[ 1234.567890] hello_module: init

$ lsmod | grep hello_module
hello_module            16384  0

$ sudo rmmod hello_module
$ sudo dmesg | tail -2
[ 1234.678901] hello_module: exit
```

需要核对的三件事：

1. `lsmod` 的第三列（被引用计数）为 0 才可卸载。
2. `dmesg` 中出现 init / exit 两条记录。
3. 卸载后 `lsmod` 不再显示该模块。

## 常见错误和陷阱

| 现象 | 原因 | 定位方法 |
| --- | --- | --- |
| `make` 报 `No such file or directory` 指向 `/lib/modules/.../build` | 未安装内核头文件 | `ls /lib/modules/$(uname -r)/build` |
| `insmod: ERROR: could not insert module: Invalid module format` | vermagic 不匹配，或用错内核构建 | `modinfo hello_module.ko \| grep vermagic`，与 `uname -r` 对比 |
| `insmod: ERROR: ... Unknown symbol` | 使用了未导出或 GPL-only 的符号 | 看 `dmesg` 中的符号名 |
| `insmod` 成功但看不到输出 | 打印到了内核日志，不在终端 | `sudo dmesg \| tail` |
| 内核被标记为 tainted | 缺少 `MODULE_LICENSE("GPL")` | `dmesg` 中查找 `module verification failed` |
| `rmmod: ERROR: Module is in use` | 有进程打开模块提供的设备 | `lsmod` 看引用计数，`lsof` 查占用进程 |

## 故障排查方法

按顺序排查：

1. **编译阶段**：先确认 `KDIR` 是否存在；再确认 `make -C $(KDIR) M=$(PWD) modules` 是否单独可执行。
2. **加载阶段**：看完整错误信息，不要只看最后一行；`dmesg` 通常有更具体的原因。
3. **版本匹配**：对比 `modinfo hello_module.ko` 的 `vermagic` 与 `uname -r`。
4. **权限**：`insmod` 与 `rmmod` 必须 root。
5. **残留状态**：加载失败后检查 `lsmod`，避免误以为已加载。

## 练习题

### 基础题

题目：把 `hello_module.c` 的 `pr_info` 改为 `printk`，并在 init 与 exit 中分别打印不同的自定义文本，观察 `dmesg` 输出。

提示：`printk` 需要显式级别前缀，例如 `printk(KERN_INFO "..." )`。

验收标准：`insmod` 与 `rmmod` 后 `dmesg` 各出现一条自定义文本。

### 改错题

题目：下面的模块有什么问题？

```c
#include <linux/module.h>

int __init hello_init(void)
{
    printk("init\n");
}

void __exit hello_exit(void)
{
    printk("exit\n");
}

module_init(hello_init);
module_exit(hello_exit);
```

提示：检查初始化函数的返回类型、返回值与许可证声明。

验收标准：能指出三处问题（缺返回值、缺 `MODULE_LICENSE`、`printk` 无级别），并给出修正。

### 综合题

题目：在初始化函数中模拟一次失败：当模块参数 `fail=1` 时返回 `-ENOMEM`，否则返回 0。验证两种加载结果。

提示：使用 `module_param(fail, int, 0644);` 定义参数，加载时写 `sudo insmod hello_module.ko fail=1`。

验收标准：`fail=1` 时 `insmod` 报错且 `lsmod` 无该模块；`fail=0` 时加载成功。

## 验收标准

- [ ] 能解释模块为什么没有 `main()`，以及 `module_init()` 的作用。
- [ ] 能写出 `make -> insmod -> dmesg -> lsmod -> rmmod` 的完整流程与每步命令。
- [ ] 能区分驱动源码、Makefile、测试程序三者的角色。
- [ ] 能列出至少三个常见加载失败原因及定位方法。
- [ ] 在具备条件的环境中完成一次真实加载与卸载（当前环境未完成）。

## 延伸阅读

- `Linux_WEIDS/02_嵌入式Linux驱动入门直播/StudyPlan/DailyDocs/02_Day2_认识第一个驱动_学习讲解.md`
- `Linux_WEIDS/02_嵌入式Linux驱动入门直播/02_彻底分析第1个驱动程序/source/02_printk/`
- `Linux/experiments/驱动入门_环境与验证指南.md`
- 下一篇：`Linux/notes/驱动入门/Day03_驱动结构与调用链.md`

---

**验证记录**

| 项目 | 内容 |
| --- | --- |
| 日期 | 2026-10-25 |
| 环境 | WSL Ubuntu 26.04，gcc 15.2.0 + make 4.4.1 已安装，内核头文件缺失 |
| 已执行 | 工具链安装与验证；`/lib/modules/.../build` 确认不存在 |
| 未执行 | 编译、`insmod`、`dmesg`、`rmmod` |
| 结论 | 本材料中的运行结果均为预期值，不作实测结论 |
