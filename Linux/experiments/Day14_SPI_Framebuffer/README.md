# Day14 实验：SPI OLED 最小 Framebuffer

## 接线

| OLED | 板卡 |
| --- | --- |
| VCC | 3.3V |
| GND | GND |
| SCL/D0 | ECSPI1_SCLK |
| SDA/D1 | ECSPI1_MOSI |
| RES | 3.3V 或复位 GPIO |
| DC | GPIO4_IO20 |
| CS | ECSPI1_SS0 |

## 步骤

```bash
make KDIR=~/100ask_imx6ull-sdk/Linux-4.9.88 ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
insmod oled_fb.ko

ls -l /dev/fb*
cat /sys/class/graphics/fb0/virtual_size

# 写入显存（走 fb_ops->fb_write）
./oled_test checker
# 等价手工操作：
# dd if=/dev/urandom of=/dev/fb0 bs=1024 count=1

# 推送到屏幕
echo 1 > /sys/class/graphics/fb0/flush

rmmod oled_fb
```

`oled_test` 支持 `checker` / `bars` / `fill` / `clear` 四种图案，写入后会自己触发一次 flush。

## 预期输出

```text
$ cat /sys/class/graphics/fb0/virtual_size
128,64

$ echo 1 > /sys/class/graphics/fb0/flush
$ dmesg | tail -2
oled_fb spi0.0: flush 1024 bytes
```

屏幕上出现随机噪点，说明"显存 → SPI → 屏幕"链路打通。

## 设计说明

| 选择 | 原因 |
| --- | --- |
| 1bpp、1024 字节 | SSD1306 原生单色显存 |
| page-major 布局 | 与 SSD1306 页寻址一致，刷新无需重排 |
| sysfs flush | 避开 `fb_deferred_io` 的复杂度，便于先验证链路 |
| `fb_ops` + `fb_sys_read/write` | 没有 `fb_ops` 时 `/dev/fb0` 无法读写，fbcon 也会拒绝该设备 |
| `dma_alloc_wc(&spi->dev, ...)` | 保证 SPI DMA 可用的缓冲区；传 NULL 会在内部解引用而崩溃 |

工程化改进方向：

1. 用 `fb_deferred_io` 或 `fb_ops` 拦截写入，自动局部刷新。
2. 增加脏页标记，减少闪烁。
3. 加入 `RES` 复位脚控制。
4. 支持 `FBIOGET_VSCREENINFO` 之外的 ioctl 扩展。

## 常见问题

| 现象 | 处理 |
| --- | --- |
| 全黑 | 检查初始化序列与 DC 极性，用逻辑分析仪看 MOSI |
| 花屏 | 检查页/列地址与显存布局 |
| `/dev/fb0` 不存在 | `register_framebuffer` 失败，看 dmesg |
| 写 `/dev/fb0` 返回 `EINVAL` | 缺 `fb_ops` 或未设置 `fb_write` |
| `mmap` 失败 | 本驱动未实现 `fb_mmap`，用 `write()` 写显存 |
| 卸载 oops | 检查 `device_remove_file` → `unregister` → `dma_free` → `release` 顺序 |

## 验证状态

**编译实测**（2026-10-25，WSL + arm-linux-gnueabihf-gcc 15.2.0 + Linux 7.0.0 内核树）：
`oled_fb.ko` 与 `oled_test` 均为 ARM ELF 产物，`modpost` 通过。

**未验证**：SPI 通信、屏幕显示、`fb_ops` 运行时行为，需要目标板与 SSD1306 模块。

> 编译该驱动要求目标内核开启 `CONFIG_SPI`（新内核中为 bool）。未开启时报
> `modpost: "__spi_register_driver" [oled_fb.ko] undefined!`。
