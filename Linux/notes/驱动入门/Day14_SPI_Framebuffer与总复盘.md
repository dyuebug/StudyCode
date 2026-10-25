# Day14：SPI、Framebuffer 与 14 天总复盘

**验证状态**：编译实测，运行未验证。`oled_fb.ko` 与 `oled_test` 已交叉编译为 ARM 产物并通过 `modpost`（2026-10-25，Linux 7.0.0 内核树）；SPI 通信与屏幕显示需要目标板、SSD1306 模块与可加载的 dtb。

## 学习目标

学完本篇应能：

1. 画出 SPI 子系统分层，并说明 `spi_write` / `spi_transfer` 的区别。
2. 解释 SSD1306 的"命令/数据"选择为什么需要额外 GPIO。
3. 说明 `fb_info` 的生命周期与 `register_framebuffer` 的作用。
4. 指出课程 OLED Framebuffer 示例中的不一致之处。
5. 完成 14 天能力自评并选择下一阶段方向。

## 环境与前置条件

| 项目 | 要求 |
| --- | --- |
| 硬件 | i.MX6ULL ECSPI1 + SSD1306 OLED（128×64） |
| 内核配置 | `CONFIG_SPI`、`CONFIG_FB` |
| 前置 | Day9 GPIO、Day13 子系统分层 |

## 核心概念

### 1. SPI 子系统分层

```text
应用（fb 设备 /dev/fb0）
驱动 spi_driver（probe / remove）
  │ spi_write / spi_transfer
核心 SPI core
  │
控制器 spi_master（SoC 的 ECSPI）
  │
硬件 SCLK / MOSI / MISO / CS
```

与 I2C 的差异：

| 维度 | I2C | SPI |
| --- | --- | --- |
| 线数 | 2 根（SCL/SDA） | 4 根（CLK/MOSI/MISO/CS） |
| 寻址 | 7 位地址 | 片选 CS |
| 速率 | 100k～400k | 可达数十 MHz |
| 多设备 | 地址区分 | 每个设备一根 CS |

### 2. 设备树

```dts
&ecspi1 {
	pinctrl-names = "default";
	pinctrl-0 = <&pinctrl_ecspi1>;
	fsl,spi-num-chipselects = <2>;
	cs-gpios = <&gpio4 26 GPIO_ACTIVE_LOW>, <&gpio4 24 GPIO_ACTIVE_LOW>;
	status = "okay";

	oled: oled {
		compatible = "100ask,oled";
		reg = <0>;
		spi-max-frequency = <1000000>;
		dc-gpios = <&gpio4 20 GPIO_ACTIVE_LOW>;
	};
};
```

要点：

- `reg = <0>` 对应第 0 根片选；
- `spi-max-frequency` 必须不超过器件上限；
- `dc-gpios` 是 OLED 特有的"命令/数据"选择脚，SPI 本身不提供。

### 3. SSD1306 的命令与数据

同一根 MOSI 上传输两种内容，靠 DC 引脚区分：

```text
DC = 0  -> 命令（初始化、设置地址、开关显示）
DC = 1  -> 数据（显存字节）
```

课程驱动：

```c
static void oled_write_cmd_data(unsigned char data, unsigned char is_cmd)
{
	gpiod_set_value(oled_dc, is_cmd);
	spi_write(oled_dev, &data, 1);
}
```

### 4. Framebuffer 的四个要素

| 要素 | 作用 |
| --- | --- |
| `fb_info` | 帧缓冲对象，由 `framebuffer_alloc` 分配 |
| `var` | 可变参数：分辨率、bpp、虚拟分辨率 |
| `fix` | 固定参数：显存长度、行长度、类型 |
| `screen_base` | 显存虚拟地址，应用 `mmap` 后写入这里 |

注册后出现 `/dev/fb0`，应用可以直接 `mmap` 写入像素。

### 5. 课程示例的不一致处

`01_oled_framebuffer/oled_drv.c`：

```c
oled_fb_info->var.bits_per_pixel = 1;  /* rgb565 */
...
oled_fb_info->fix.visual = FB_VISUAL_MONO10;
```

注释写的是 `rgb565`（16bpp），实际设置是 1bpp 单色。这是注释与代码不一致，不是功能错误——SSD1306 本身就是单色屏。

阅读他人代码时，**以代码为准，注释只作参考**。

### 6. 显存布局

128×64 单色 = 1024 字节：

```text
每字节 8 个像素（纵向）
8 页（page 0..7）× 128 列
第 n 页第 c 列 -> 显存偏移 n*128 + c
```

`fix.line_length = 128 * 1 / 8 = 16` 字节/行——但 SSD1306 是按页组织的，行长度与显存布局并不等价。这是移植时最容易出错的地方。

## 系统调用/接口卡片

### `int spi_write(struct spi_device *spi, const void *buf, size_t len)`

作用：向 SPI 设备写数据（不关心返回值）。

返回值和错误：成功 0；失败返回负错误码。

最小示例：`spi_write(spi, buf, len);`

常见误用：不检查返回值；在原子上下文传输大块数据。

### `int spi_transfer(struct spi_device *spi, struct spi_transfer *xfer)`

作用：单次全双工传输。

最小示例：

```c
struct spi_transfer xfer = { .tx_buf = buf, .rx_buf = rbuf, .len = len };
ret = spi_transfer(spi, &xfer);
```

常见误用：`tx_buf` 与 `rx_buf` 指向同一缓冲区。

### `struct fb_info *framebuffer_alloc(size_t size, struct device *dev)`

作用：分配帧缓冲对象。

资源所有权：必须 `framebuffer_release()`。

最小示例：`info = framebuffer_alloc(0, &spi->dev);`

常见误用：注册失败后直接返回而不释放。

### `int register_framebuffer(struct fb_info *info)`

作用：注册帧缓冲，创建 `/dev/fbN`。

返回值和错误：成功返回次设备号（≥0）；失败返回负错误码。

资源所有权：必须 `unregister_framebuffer()`。

常见误用：未设置 `fix` / `var` 就注册；忘记反注册。

### `void *dma_alloc_wc(struct device *dev, size_t size, dma_addr_t *dma_handle, gfp_t flag)`

作用：分配可被设备访问的一致性内存。

资源所有权：必须 `dma_free_wc()`。

常见误用：用 `kmalloc` 分配需要 DMA 的显存。

## 最小可运行实验

实验目录：`Linux/experiments/Day14_SPI_Framebuffer/`

```text
oled_fb.c      最小 Framebuffer 驱动 + SSD1306 初始化 + 手动 flush
oled_test.c    用户态测试：checker / bars / fill / clear 四种图案
oled.dts       设备树片段
Makefile
README.md      接线、编译、写显存与超时排查
```

核心流程：

```text
probe
  -> 取 spi_device、dc gpio
  -> framebuffer_alloc
  -> 设置 var/fix/fbops
  -> dma_alloc_wc 分配 1024 字节显存
  -> register_framebuffer
  -> OLED 初始化（命令序列）
应用
  -> write() /dev/fb0 写入像素（走 fb_ops->fb_write）
  -> echo 1 > /sys/.../flush 触发整屏刷新
```

> `fb_ops` 是这个驱动能否工作的分界点：没有它，`register_framebuffer` 之后
> `/dev/fb0` 不能 `read`/`write`，fbcon 也不会接管。本驱动用
> `fb_sys_read` / `fb_sys_write` 加三个 `sys_*` 绘图函数，因为显存由 CPU 直接访问。

## 编译与运行

```bash
cd Linux/experiments/Day14_SPI_Framebuffer
make KDIR=~/100ask_imx6ull-sdk/Linux-4.9.88 ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
```

目标板：

```bash
insmod oled_fb.ko
ls -l /dev/fb*
cat /sys/class/graphics/fb0/virtual_size

# 写满一屏（走 fb_ops->fb_write）
./oled_test checker

# 或用 dd 手工写入
dd if=/dev/urandom of=/dev/fb0 bs=1024 count=1

# 触发刷新（路径以实际 class 名为准）
echo 1 > /sys/class/graphics/fb0/flush

rmmod oled_fb
```

## 结果观察

以下为**预期结果，未在本机实测**：

```text
$ cat /sys/class/graphics/fb0/virtual_size
128,64

$ ls -l /dev/fb0
crw-rw---- 1 root root 29, 0 ... /dev/fb0

$ echo 1 > /sys/class/graphics/fb0/flush
$ dmesg | tail -2
oled_fb: flush 1024 bytes
```

屏幕上出现随机噪点即表示显存到屏幕的链路已打通。

## 常见错误和陷阱

| 现象 | 原因 | 定位方法 |
| --- | --- | --- |
| 屏幕全黑 | 未执行初始化序列 | 用逻辑分析仪看 MOSI |
| 显示花屏 | 显存页/列地址设置错误 | 对照 SSD1306 手册 |
| `spi_write` 返回 `-EINVAL` | `spi-max-frequency` 或模式不匹配 | 检查 dts |
| `/dev/fb0` 不存在 | `register_framebuffer` 失败 | 看 `dmesg` |
| 写 `/dev/fb0` 失败 | 没有 `fb_ops` 或未设置 `fb_write` | 补 `fb_sys_write` |
| `mmap` 失败 | 未实现 `fb_mmap` | 本驱动用 `write()` 写显存 |
| `modpost: "__spi_register_driver" undefined` | 目标内核未开启 `CONFIG_SPI` | 内核配置中启用 SPI 后重编 |
| 卸载后 oops | 未 `unregister_framebuffer` / `dma_free_wc` | 检查 remove 顺序 |
| 屏幕闪烁 | 整屏刷新过于频繁 | 加脏区或降低刷新率 |

## 故障排查方法

1. **先确认 SPI 通信**：发一条命令后读回状态（部分 OLED 支持）。
2. **再确认 DC 极性**：`GPIO_ACTIVE_LOW` 是否与电路一致。
3. **再确认初始化序列**：逐条对照 SSD1306 手册。
4. **最后确认显存布局**：页/列地址是否与 `screen_base` 一致。

## 14 天总复盘

| 能力 | 对应 Day | 自评标准 |
| --- | --- | --- |
| 文件身份判断 | 1 | 能区分应用、模块、Makefile、设备树 |
| 模块生命周期 | 2 | 能独立完成 `make → insmod → dmesg → rmmod` |
| 字符设备与调用链 | 3 | 能画出 VFS → `file_operations` 路径 |
| 用户/内核拷贝 | 4 | 能写出带检查的 `copy_*_user` |
| 交叉编译 | 5 | 能配置 `ARCH` / `CROSS_COMPILE` / `KERN_DIR` |
| 总线设备模型 | 6–7 | 能解释 device → bus → driver → probe |
| 设备树 | 8 | 能写节点并让驱动匹配 |
| pinctrl / GPIO | 9 | 能用 gpiolib 取代裸寄存器 |
| 中断与等待 | 10 | 能实现阻塞、非阻塞、poll |
| 上下文与资源 | 11 | 能写出安全的卸载顺序 |
| 实战专题 | 12 | 能独立完成 SR501 数据流 |
| I2C 子系统 | 13 | 能写出两段式 EEPROM 读 |
| SPI / Framebuffer | 14 | 能说明显存与屏幕的关系 |

下一阶段可选方向：

1. **专题深入**：Input 子系统、IIO、SPI/I2C 复杂器件。
2. **BSP 方向**：启动链、设备树裁剪、镜像构建。
3. **工程化**：把 SR501 或 OLED 做成有测试与文档的完整项目。
4. **异构系统**：STM32MP157 A7/M4、RemoteProc、RPMsg。

## 练习题

### 基础题

题目：把实验的分辨率改为 64×48，并同步修改显存大小与页数。

提示：`xres/yres/smem_len` 三者必须一致。

验收标准：`/sys/class/graphics/fb0/virtual_size` 显示 `64,48`，刷新无越界。

### 改错题

题目：下面的 `remove` 有什么问题？

```c
static int oled_remove(struct spi_device *spi)
{
	framebuffer_release(oled_fb_info);
	return 0;
}
```

提示：注册与内存释放的顺序。

验收标准：能指出缺少 `unregister_framebuffer` 与 `dma_free_wc`。

### 综合题

题目：实现脏区刷新：记录应用写入的范围，只刷新变化的页。

提示：`fb_ops` 中拦截 `fb_imageblit` / `fb_fillrect`，或使用 `fb_deferred_io`。

验收标准：整屏刷新的闪烁明显降低，CPU 占用下降。

## 验收标准

- [ ] 能画出 SPI 子系统分层并与 I2C 对比。
- [ ] 能解释 DC 引脚的作用。
- [ ] 能说出 `fb_info` 的四个核心字段与生命周期。
- [ ] 能指出课程示例中注释与代码不一致之处。
- [ ] 完成 14 天自评并选定下一阶段方向。

## 延伸阅读

- `Linux_WEIDS/02_嵌入式Linux驱动入门直播/StudyPlan/DailyDocs/14_Day14_SPI与Framebuffer预览及总复盘_学习讲解.md`
- `Linux_WEIDS/02_嵌入式Linux驱动入门直播/27_SPI设备驱动开发1_OLED屏幕/source/`
- `Linux_WEIDS/02_嵌入式Linux驱动入门直播/29_Framebuffer驱动程序框架/`
- `Linux_WEIDS/02_嵌入式Linux驱动入门直播/30_使用Framebuffer改造SPI_OLED驱动/source/`

---

**验证记录**

| 项目 | 内容 |
| --- | --- |
| 日期 | 2026-10-25 |
| 环境 | WSL Ubuntu + arm-linux-gnueabihf-gcc 15.2.0 + Linux 7.0.0 内核树（`CONFIG_SPI=y`） |
| 已执行 | 交叉编译 `oled_fb.ko`、`oled_test`（均为 ARM ELF）、`modpost` 符号解析、源码阅读 |
| 未执行 | SPI 通信、屏幕显示、`fb_ops` 运行时行为 |
| 结论 | 编译链路已实测；运行行为未验证 |
