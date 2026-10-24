# Day05 实验：交叉编译与板级 LED 驱动

## 目的

完成"主机编译 → 目标板加载 → 硬件现象"的完整链路，并理解 `ARCH` / `CROSS_COMPILE` / `KERN_DIR` 的作用。

## 硬件假设

本驱动针对 **i.MX6ULL，LED 接 GPIO5_IO03**：

| 寄存器 | 地址 |
| --- | --- |
| SNVS_TAMPER3 复用 | `0x02290000 + 0x14` |
| GPIO5_GDIR | `0x020AC004` |
| GPIO5_DR | `0x020AC000` |

其他板卡地址不同，直接使用会写错寄存器。

## 步骤

### 1. 检查环境

```bash
make info
# 期望：cross gcc: OK / Module.symvers: OK
```

### 2. 编译

```bash
export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-
export KERN_DIR=~/100ask_imx6ull-sdk/Linux-4.9.88

make
file led_drv.ko
```

期望看到 `ELF 32-bit LSB relocatable, ARM`。

### 3. 部署

```bash
# NFS
cp led_drv.ko ledtest /home/book/nfs_rootfs/

# 或 scp
scp led_drv.ko ledtest root@<board-ip>:/root/
```

### 4. 在目标板运行

```bash
insmod led_drv.ko
ls -l /dev/myled
./ledtest on
./ledtest off
dmesg | tail -10
rmmod led_drv
```

## 预期现象

- `/dev/myled` 由 `device_create` 自动生成
- `./ledtest on` 点亮 LED，`off` 熄灭
- `dmesg` 显示 `init, major = N`

## 常见问题

| 现象 | 处理 |
| --- | --- |
| `cross gcc: MISSING` | 安装工具链并加入 `PATH` |
| `Module.symvers: MISSING` | 在 `KERN_DIR` 执行 `make modules_prepare` |
| `insmod: Invalid module format` | 核对 `modinfo led_drv.ko` 的 vermagic |
| LED 无反应 | 对照原理图确认引脚与 GPIO 编号 |
| `class_create` 参数错误 | 内核 ≥6.4 使用 `class_create("myled")`，本驱动已做版本分支 |

## 验证状态

未在本机验证。当前环境无 ARM 交叉工具链、无目标内核树、无开发板。
