# QEMU 验证结果

**日期**：2026-10-25
**环境**：QEMU 10.2.1 `vexpress-a9`，Linux 7.0.0（Ubuntu `linux-source-7.0.0`，`vexpress_defconfig`）
**工具链**：arm-linux-gnueabihf-gcc 15.2.0
**内核镜像**：`arch/arm/boot/zImage`（6.1 MB）+ `vexpress-v2p-ca9.dtb`
**载荷**：initramfs 635 KB，含静态 init 与 7 个外部模块

## 命令

```bash
make initramfs
bash run.sh
```

## 实测输出（原文）

```text
===== qemu-vexpress verification =====
hello_module: loading out-of-tree module taints kernel.
hello_module: init
[ OK ] loaded /hello_module.ko
[ OK ] loaded /hello_chrdev.ko
hello_chrdev: init, major = 248
hello_chrdev: next: sudo mknod /dev/hello_drv c 248 0
[ OK ] mknod /dev/hello_chrdev major=248
[ OK ] Day3 open /dev/hello_chrdev
[ OK ] Day3 write returns 1 (callback stub)
[ OK ] Day3 read returns 1 (no data copied)
[ OK ] loaded /hello_copy.ko
[ OK ] mknod /dev/hello_copy major=247
[ OK ] Day4 open /dev/hello_copy
[ OK ] Day4 read initial value = 123
[ OK ] Day4 write 456
[ OK ] Day4 read back 456
[ OK ] Day4 short read returns 1 (no overflow)
hello_chrdev: open
hello_chrdev: write size=3
hello_chrdev: read size=8
hello_chrdev: release
hello_copy: init, major = 247
hello_copy: next: sudo mknod /dev/hello_copy c 247 0
hello_copy: read 4 byte(s), ker_val=123
hello_copy: write 4 byte(s), ker_val=456
hello_copy: read 4 byte(s), ker_val=456
hello_copy: read 1 byte(s), ker_val=456
[ OK ] loaded /hello_platform_dev.ko
[ OK ] loaded /hello_platform_drv.ko
[ OK ] Day6 platform driver registered
[ OK ] loaded /led_platform_dev.ko
hello_platform: device register -> 0
hello_platform hello_platform: probe matched: hello_platform
hello_platform: driver register -> 0
led_multi: 2 device(s) registered
[ OK ] loaded /led_platform_drv.ko
[ OK ] Day7 /dev/led0 created
[ OK ] Day7 /dev/led1 created
[ OK ] Day7 write to /dev/led0
led_multi led_multi.0: probe: minor=0 pin=3
led_multi led_multi.1: probe: minor=1 pin=5
led_multi: minor=0 pin=3 value=1
===== result: PASSED (failures=0) =====
```

## 结论

| Day | 验证项 | 结果 |
| --- | --- | --- |
| 2 | 交叉编译、模块加载、printk | 通过 |
| 3 | `register_chrdev`、`mknod`、open/write/read/release | 通过 |
| 4 | `copy_to_user` / `copy_from_user`、短读边界 | 通过 |
| 6 | platform 设备与驱动匹配、probe | 通过 |
| 7 | 多实例、次设备号、drvdata、设备节点 | 通过 |

## 未覆盖

- Day5/8/9/10/11/12/13/14：需要 GPIO 按键、设备树节点、I2C/SPI 外设或真实传感器，`vexpress-a9` 不具备。
- `rmmod` 卸载路径：init 以系统调用加载，未执行卸载；可在 QEMU 交互 shell 中另行验证。

## 过程中发现并修复的问题

1. **Day7 驱动缺少 `.remove` 版本分支**：Linux 6.11 起 `platform_driver.remove` 返回 `void`，编译报 `incompatible pointer type`。已加入 `LINUX_VERSION_CODE` 分支。
2. **initramfs 缺少 `/proc`、`/sys` 目录**：导致挂载失败、`/proc/devices` 解析不到主设备号。已在打包时创建目录。
3. **`init_module` 返回 `EFAULT`**：改用 `finit_module` 由内核直接读取模块文件，加载成功。
