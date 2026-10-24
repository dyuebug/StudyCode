# Day03 实验：字符设备调用链

## 目的

观察 `open/read/write/close` 如何经过 VFS 进入驱动的 `file_operations`，并理解主设备号的作用。

## 文件

| 文件 | 用途 |
| --- | --- |
| `hello_chrdev.c` | 字符设备驱动，注册动态主设备号 |
| `hello_chrdev_test.c` | 用户态测试程序 |
| `Makefile` | 驱动与测试程序构建 |

## 步骤

```bash
make info

make

sudo insmod hello_chrdev.ko
sudo dmesg | tail -5
```

记下 `dmesg` 中的 `major = N`，然后：

```bash
cat /proc/devices | grep hello_chrdev

sudo mknod /dev/hello_drv c <N> 0
sudo chmod 666 /dev/hello_drv

./hello_chrdev_test -w abc
./hello_chrdev_test -r
sudo dmesg | tail -20
```

清理：

```bash
sudo rm /dev/hello_drv
sudo rmmod hello_chrdev
sudo dmesg | tail -5
```

## 预期输出

```text
[  ...] hello_chrdev: init, major = 240
[  ...] hello_chrdev: open
[  ...] hello_chrdev: write size=4
[  ...] hello_chrdev: release
```

`-r` 会得到 `read -> 1`，但 `buffer` 为空——本日驱动没有 `copy_to_user`，这是刻意保留的现象。

## 常见问题

| 现象 | 处理 |
| --- | --- |
| `open failed: No such device` | `mknod` 的 major 与 `dmesg` 不一致 |
| `open failed: Permission denied` | 未执行 `chmod 666` |
| `read -> -1` | 驱动未实现 `.read`，检查 `file_operations` |
| `rmmod: Module is in use` | 仍有进程占用，先 `lsof /dev/hello_drv` |

## 验证状态

未在本机验证（WSL2 无法构建和加载内核模块）。请在 QEMU、虚拟机或开发板中执行。
