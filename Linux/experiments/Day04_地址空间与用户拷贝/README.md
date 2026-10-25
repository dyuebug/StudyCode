# Day04 实验：用户空间与内核空间数据拷贝

## 目的

验证 `copy_to_user` / `copy_from_user` 的正确用法，并对比课程示例中"忽略返回值、忽略 size"的缺陷。

## 步骤

```bash
make info
make

sudo insmod hello_copy.ko
sudo dmesg | tail -3
```

记下 `major = N`：

```bash
sudo mknod /dev/hello_copy c <N> 0
sudo chmod 666 /dev/hello_copy

./hello_copy_test -r
./hello_copy_test -w 456
./hello_copy_test -r
./hello_copy_test -s
sudo dmesg | tail -10
```

带参数加载：

```bash
sudo rmmod hello_copy
sudo insmod hello_copy.ko ker_val=999
./hello_copy_test -r
cat /sys/module/hello_copy/parameters/ker_val
```

清理：

```bash
sudo rm /dev/hello_copy
sudo rmmod hello_copy
```

## 预期输出

```text
$ ./hello_copy_test -r
read -> 4, value = 123

$ ./hello_copy_test -w 456
write -> 4, value = 456

$ ./hello_copy_test -s
read short -> 1, byte0 = 200
```

`-s` 只请求 1 字节，驱动也只拷贝 1 字节——这正是课程示例会越界的地方。

## 与课程示例的差异

| 项目 | 课程 `05_transfer_data` | 本实验 |
| --- | --- | --- |
| 拷贝长度 | 固定 4 字节 | `min(size, sizeof(ker_val))` |
| 返回值检查 | 无 | 失败返回 `-EFAULT` |
| 返回字节数 | 固定 4 | 实际拷贝长度 |
| 零长度请求 | 仍拷贝 4 字节 | 返回 0 |

## 验证状态

**已实测**（2026-10-25，QEMU 10.2.1 `vexpress-a9` + Linux 7.0.0 + arm-linux-gnueabihf-gcc 15.2.0）：

- 交叉编译：`hello_copy.ko` 为 ARM ELF
- 内核缓冲区初值 123 经 `copy_to_user` 正确读出
- `copy_from_user` 写入 456 后读回一致
- 1 字节短读只拿到 1 字节，无越界

**未验证**：非法用户指针（`-EFAULT` 路径）与并发读写。详见 [`../../tests/qemu-vexpress/RESULT.md`](../../tests/qemu-vexpress/RESULT.md)。
