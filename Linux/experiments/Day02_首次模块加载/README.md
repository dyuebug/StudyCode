# Day02 实验：第一个内核模块

## 目的

验证 `make -> insmod -> dmesg -> lsmod -> rmmod` 闭环，并确认模块没有 `main()`。

## 前置条件

```bash
# 检查内核头文件
make info
# 期望输出：kernel headers: OK
```

缺失时先安装：

```bash
# Ubuntu / Debian
sudo apt update
sudo apt install -y build-essential linux-headers-$(uname -r)
```

## 步骤

```bash
make
sudo insmod hello_module.ko
lsmod | grep hello_module
sudo dmesg | tail -5

sudo rmmod hello_module
sudo dmesg | tail -5

make clean
```

## 预期输出

```text
[ 1234.567890] hello_module: init
...
[ 1234.678901] hello_module: exit
```

## 本机限制

初次记录时的 WSL2 环境（内核 `6.6.87.2-microsoft-standard-WSL2`）：

- 未安装 gcc / make
- `/lib/modules/$(uname -r)/build` 不存在
- WSL2 不支持加载自定义内核模块

因此无法在 WSL2 本机直接加载模块。本实验已在 QEMU vexpress-a9 中完成验证；若要在真实板卡上复现，可用：

1. QEMU + 100ask_imx6ull 镜像（资料内已有）
2. Ubuntu 虚拟机（VMware / VirtualBox）
3. 真实开发板

## 验证状态

**已实测**（2026-10-25，QEMU 10.2.1 `vexpress-a9` + Linux 7.0.0 + arm-linux-gnueabihf-gcc 15.2.0）：

- 交叉编译：`hello_module.ko` 为 ARM ELF
- `finit_module` 加载成功，`hello_module: init` 打印到控制台
- 卸载走 `SYS_delete_module`，`hello_module: exit` 正常打印

**未验证**：`insmod` / `rmmod` 命令行工具路径（QEMU 内由 PID 1 init 直接调用系统调用）；WSL2 本机加载。详见 [`../../tests/qemu-vexpress/RESULT.md`](../../tests/qemu-vexpress/RESULT.md)。
