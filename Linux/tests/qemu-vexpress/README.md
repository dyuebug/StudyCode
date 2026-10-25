# QEMU vexpress-a9 自动验证

## 目的

在没有开发板的条件下，用 QEMU 跑通内核模块的完整闭环：

```text
交叉编译 .ko -> 打包 initramfs -> QEMU 启动 -> init 加载模块 -> 读写验证 -> 打印结果
```

覆盖范围：

| Day | 验证内容 | 依赖硬件 |
| --- | --- | --- |
| 2 | 模块加载与 printk | 无 |
| 3 | 字符设备注册与调用链 | 无 |
| 4 | `copy_to_user` / `copy_from_user` | 无 |
| 5 | 交叉编译、加载、设备节点、卸载 | 板级寄存器不访问 |
| 6 | platform device/driver 匹配 | 无 |
| 7 | 多实例、次设备号、drvdata | 无 |
| 8 | 自定义设备树节点、`compatible` 匹配 | 无（需自定义 dtb） |
| 9 | `devm_gpiod_get`、GPIO 写入与读回 | `basic-mmio-gpio` |
| 10 | 等待队列、非阻塞、poll、ioctl | 定时器替代硬件中断（运行验证用 `event_demo`） |
| 11 | timer / tasklet / workqueue / 卸载清理 | 无（运行验证用 `event_demo`） |
| 12–14 | 编译验证（ARM ELF） | 传感器、I2C、SPI 外设未建模，只编译不运行 |

## 前置

```bash
sudo apt install qemu-system-arm gcc-arm-linux-gnueabihf linux-source \
                 libncurses-dev flex bison bc libssl-dev libelf-dev cpio

# 一次性构建 ARM 内核
mkdir -p ~/kbuild && cd ~/kbuild
tar xf /usr/src/linux-source-7.0.0.tar.bz2
cd linux-source-7.0.0
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- vexpress_defconfig

# Day8/Day9 需要 vexpress sysreg GPIO（basic-mmio-gpio）
./scripts/config --enable ARCH_VIRT
./scripts/config --enable GPIO_GENERIC_PLATFORM
./scripts/config --enable SPI                    # 仅 Day14 编译检查需要
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- olddefconfig

make -j$(nproc) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- zImage dtbs modules
```

> `olddefconfig` 必须带 `ARCH=arm`。漏掉会把配置重算成主机 x86，导致外部模块结构体布局错误，加载时内核 oops。

## 运行

```bash
make            # 编译 init 与 11 个内核模块，打包 initramfs
make dtb        # 编译自定义 vexpress 设备树
bash run.sh     # 启动 QEMU，自动执行并退出
```

## 预期输出

```text
===== qemu-vexpress verification =====
[ OK ] loaded /hello_module.ko
[ OK ] loaded /hello_chrdev.ko
[ OK ] mknod /dev/hello_chrdev major=...
[ OK ] Day3 open /dev/hello_chrdev
[ OK ] Day3 write returns 1 (callback stub)
...
===== result: PASSED (failures=0) =====
```

同时可在控制台看到驱动自身的 `pr_info` 输出。

## 已知限制

- vexpress-a9 没有 SR501、AT24C02、SPI OLED，Day12–Day14 未覆盖；Day5 的 i.MX6ULL 寄存器访问同样无对应硬件。
- Day10/11 用定时器事件替代硬件中断，`request_irq` 的真实投递未验证。
- 模块必须用 `KDIR` 指向的同一棵内核树构建，否则 vermagic 不匹配。
- init 以 PID 1 运行，`init_module` 直接使用系统调用，不依赖 kmod。
