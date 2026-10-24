# Day1：Linux 应用与驱动的整体认知

**验证状态**：用户态实验已实测（Windows MinGW gcc 15.1.0，2026-10-25）；内核相关结论为源码分析，未在目标环境验证。

## 学习目标

学完本篇应能用代码证据回答：

1. 单片机程序、Linux 用户态应用、内核模块三者的运行环境有什么不同。
2. Linux 应用访问硬件时，从应用到硬件要经过哪几层。
3. 面对一个陌生 `.c` 文件，如何判断它是应用、驱动还是硬件描述。
4. `main(int argc, char **argv)` 的 `argc` 和 `argv` 分别表示什么。

## 环境与前置条件

| 项目 | 要求 |
| --- | --- |
| 编译器 | 任意 C11 编译器（gcc / clang / MinGW） |
| 操作系统 | Linux、WSL、macOS 或 Windows 均可（本篇只涉及用户态） |
| 前置知识 | 会使用命令行、会写最基础的 C 程序 |
| 不需要 | 内核源码、内核头文件、开发板 |

本材料实测环境：

```text
Windows 11 + Git Bash + gcc.exe (MinGW-Builds) 15.1.0
命令: gcc -Wall -Wextra -std=c11 hello_arg.c -o hello_arg.exe
```

## 核心概念

### 1. 三种程序的运行模型

```text
单片机裸机：  应用逻辑 -> 寄存器 / HAL -> 硬件
RTOS 任务：   任务 -> RTOS API -> HAL / 寄存器 -> 硬件
Linux 应用：  应用逻辑 -> C 库 / 系统调用 -> 内核 -> 驱动 -> 硬件
Linux 驱动：  内核模块 -> 内核子系统 / 寄存器 -> 硬件
```

关键差别不在语言，而在**谁能直接触碰硬件**：

- 用户态应用没有权限直接访问物理地址，只能通过系统调用请求内核。
- 内核模块运行在内核空间，可以注册 `file_operations`、访问 `ioremap` 后的地址、响应中断。
- 设备树不是代码，是给内核读的硬件描述数据。

### 2. 判断文件身份的证据链

```text
先找入口函数
-> 看使用的 API
-> 看编译产物
-> 看启动方式
-> 再判断属于用户态、内核态还是硬件描述层
```

| 证据 | 用户态应用 | 内核模块 |
| --- | --- | --- |
| 入口 | `main()` | `module_init()` / `init_module()` |
| 输出 | `printf()` / `fprintf()` | `printk()` / `pr_info()` |
| 头文件 | `<stdio.h>`、`<unistd.h>` | `<linux/module.h>`、`<linux/fs.h>` |
| 产物 | 可执行文件（ELF / `.exe`） | `.ko` 内核模块 |
| 启动 | 直接执行 | `insmod` 由内核加载 |
| 结束 | `return` / `exit()` | `rmmod` 触发 `module_exit()` |

`return 0` 是辅助证据，不能单独作为身份判据——内核模块的初始化函数同样返回 0。

### 3. 本篇示例在整体中的位置

```text
hello.c       用户态应用，只演示参数与输出，不接触硬件
hello_drv.c   内核模块，Day2 起才出现
```

## 系统调用/接口卡片

### `int main(int argc, char **argv)`

作用：C 程序的入口，接收命令行参数。

参数：

- `argc`：参数个数，包含程序自身，最小为 1。
- `argv`：字符串指针数组，`argv[0]` 是程序名或路径，`argv[argc]` 为 `NULL`。

返回值和错误：返回 `int` 给运行环境作为退出码，`0` 表示成功。

资源所有权：`argv` 由运行环境分配，程序不应 `free`，也不应修改其指向的字符串内容。

线程/进程安全：每个进程有独立的一份。

最小示例：

```c
#include <stdio.h>

int main(int argc, char **argv)
{
    if (argc >= 2)
        printf("Hello, %s!\n", argv[1]);
    else
        printf("Hello, world!\n");
    return 0;
}
```

常见误用：未判断 `argc` 就访问 `argv[1]`，参数缺失时得到空指针。

### `int printf(const char *format, ...)`

作用：向标准输出写格式化文本，内部最终走 `write(1, ...)`。

参数：格式串 + 可变参数。

返回值和错误：成功返回写入字符数，失败返回负数并设置 `errno`。

资源所有权：不分配需要调用方释放的资源；标准输出缓冲区由 C 库管理。

线程/进程安全：单次调用是线程安全的，但多线程交错输出的顺序不保证。

最小示例：`printf("argc = %d\n", argc);`

常见误用：函数名拼写为 `prinf`；忘记 `\n` 导致输出未及时刷新；格式串与参数类型不匹配。

## 最小可运行实验

实验目录：`Linux/experiments/Day01_应用与驱动认知/`

```text
hello_arg.c   打印 argc / argv，并按参数个数输出不同内容
Makefile      标准编译规则
run.sh        一次性编译并运行三种参数场景
README.md     运行说明与预期输出
```

核心代码：

```c
#include <stdio.h>

int main(int argc, char **argv)
{
    int i;

    printf("argc = %d\n", argc);
    for (i = 0; i < argc; i++)
        printf("argv[%d] = %s\n", i, argv[i]);

    if (argc >= 2)
        printf("Hello, %s!\n", argv[1]);
    else
        printf("Hello, world!\n");

    return 0;
}
```

## 编译与运行

Linux / WSL：

```bash
cd Linux/experiments/Day01_应用与驱动认知
make
./hello_arg
./hello_arg Linux
./hello_arg Linux driver
```

Windows（Git Bash + MinGW）：

```bash
gcc -Wall -Wextra -std=c11 hello_arg.c -o hello_arg.exe
./hello_arg.exe Linux
```

## 结果观察

实测输出（2026-10-25，MinGW gcc 15.1.0）：

```text
$ ./hello_arg.exe
Hello, world!

$ ./hello_arg.exe Linux
Hello, Linux!

$ ./hello_arg.exe; echo "exit=$?"
Hello, world!
exit=0
```

加上 `hello_arg.c` 的打印后，可观察到：

```text
argc = 2
argv[0] = ./hello_arg.exe
argv[1] = Linux
```

需要理解的两点：

1. `argv[0]` 是调用时写的路径，不是程序内部的名字。
2. 参数以空格分隔，带空格的参数需要引号包裹。

## 常见错误和陷阱

| 现象 | 原因 | 定位方法 |
| --- | --- | --- |
| 段错误 / 程序崩溃 | 未判断 `argc` 就访问 `argv[1]` | 在访问前打印 `argc` |
| 输出乱码或为空 | 把 `argc` 当成字符串使用 | 检查类型：`argc` 是整数 |
| 编译报 implicit declaration | 忘记 `#include <stdio.h>` | 看警告中的函数名 |
| 输出粘在同一行 | 格式串缺少 `\n` | 检查所有 `printf` 的格式串 |
| 把 `printf` 写成 `prinf` | 拼写错误 | 编译器会报未声明 |

## 故障排查方法

1. **先看编译输出**：`-Wall -Wextra` 打开后，警告基本能定位大部分低级错误。
2. **先打印再访问**：不确定参数时，先输出 `argc` 和每个 `argv[i]`。
3. **检查退出码**：`echo $?`（Linux）或 `echo %ERRORLEVEL%`（cmd）。
4. **区分编译期与运行期**：编译错误是语法或声明问题；段错误是运行期内存访问问题。

## 练习题

### 基础题

题目：修改程序，使其在参数少于 2 个时打印用法说明并返回非零退出码。

提示：判断 `argc < 3`，使用 `fprintf(stderr, ...)` 输出错误信息。

验收标准：无参数运行时退出码不为 0，且标准错误有 usage 提示。

### 改错题

题目：下面的代码有什么问题？

```c
int main(int argc, char **argv)
{
    printf("Hello, %s!\n", argv[1]);
    return 0;
}
```

提示：考虑无参数运行的情况。

验收标准：能指出崩溃原因，并给出加入 `argc` 判断后的修正版本。

### 综合题

题目：写一个程序，把传入的所有参数按 `argv[i]` 逐行打印，并统计参数总长度（不含 `argv[0]`）。

提示：使用 `strlen`，注意包含 `<string.h>`；累加时使用 `size_t` 避免符号问题。

验收标准：`./prog a bb ccc` 输出三行且总长度为 6；无参数时总长度为 0 且不崩溃。

## 验收标准

- [ ] 能说出用户态应用与内核模块在入口、输出、产物、启动方式上的四点差别。
- [ ] 能解释 `argc` 与 `argv` 的区别，并手算出 `./hello Linux driver` 的 `argc`。
- [ ] 能独立编译并运行实验，得到与本文一致的三组输出。
- [ ] 能指出至少三个本篇列出的常见错误，并说明定位方法。

## 延伸阅读

- `Linux_WEIDS/02_嵌入式Linux驱动入门直播/StudyPlan/DailyDocs/01_Day1_建立整体认知_学习讲解.md`
- `Linux/experiments/驱动入门_环境与验证指南.md`
- `Linux/knowledge_base/Linux驱动入门_接口与版本速查表.md`
- 下一篇：`Linux/notes/驱动入门/Day02_第一个内核模块.md`

---

**验证记录**

| 项目 | 内容 |
| --- | --- |
| 日期 | 2026-10-25 |
| 环境 | Windows 11 + Git Bash + MinGW gcc 15.1.0 |
| 命令 | `gcc -Wall -Wextra -std=c11 hello.c -o hello.exe` |
| 结果 | `Hello, world!` / `Hello, Linux!`，退出码 0 |
| 未覆盖 | Linux 原生编译、WSL 编译（WSL 缺 gcc） |
