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
make KERN_DIR=~/100ask_imx6ull-sdk/Linux-4.9.88
insmod oled_fb.ko

ls -l /dev/fb*
cat /sys/class/graphics/fb0/virtual_size

# 写入显存
dd if=/dev/urandom of=/dev/fb0 bs=1024 count=1

# 推送到屏幕
echo 1 > /sys/class/graphics/fb0/flush

rmmod oled_fb
```

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
| `dma_alloc_wc` | 保证 SPI DMA 可用的缓冲区 |

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
| `mmap` 失败 | `fix.smem_len` 或 `line_length` 不正确 |
| 卸载 oops | 检查 `device_remove_file` → `unregister` → `dma_free` → `release` 顺序 |

## 验证状态

未在本机验证。需要目标板与 SSD1306 模块。
