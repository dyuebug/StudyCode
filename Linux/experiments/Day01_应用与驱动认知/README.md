# Day01 实验：argc 与 argv

## 目的

用最小程序观察命令行参数，建立"用户态应用"的身份判断依据：有 `main()`、用 `printf()`、有返回值。

## 文件

| 文件 | 用途 |
| --- | --- |
| `hello_arg.c` | 打印 `argc` / `argv`，并按参数个数输出不同内容 |
| `Makefile` | 编译与清理规则 |
| `run.sh` | 一次性运行三种参数场景 |

## 编译与运行

Linux / WSL（需先安装 gcc 与 make）：

```bash
make
./hello_arg Linux driver
```

一键运行：

```bash
bash run.sh
```

Windows（Git Bash + MinGW，无 make 时）：

```bash
gcc -Wall -Wextra -std=c11 hello_arg.c -o hello_arg.exe
./hello_arg.exe Linux
```

## 预期输出

```text
argc = 3
argv[0] = ./hello_arg
argv[1] = Linux
argv[2] = driver
Hello, Linux!
```

`argv[0]` 的实际内容取决于调用时写的路径。

## 验证状态

- 已实测：Windows 11 + MinGW gcc 15.1.0，输出与上例一致。
- 未覆盖：Linux 原生编译、WSL 编译（当前 WSL 未安装 gcc）。
