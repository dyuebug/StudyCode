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

当前 WSL2 环境（内核 `6.6.87.2-microsoft-standard-WSL2`）：

- 未安装 gcc / make
- `/lib/modules/$(uname -r)/build` 不存在
- WSL2 不支持加载自定义内核模块

因此本实验**未在本机验证**。可用替代环境：

1. QEMU + 100ask_imx6ull 镜像（资料内已有）
2. Ubuntu 虚拟机（VMware / VirtualBox）
3. 真实开发板

## 验证状态

- 已执行：环境能力检查（确认不可用）
- 未执行：编译、加载、日志观察、卸载
