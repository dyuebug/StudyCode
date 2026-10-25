# Day13：I2C 驱动与总线分层（AT24C02）

**验证状态**：未验证。需要目标板、I2C 总线与 AT24C02 器件，当前环境不具备。

## 学习目标

学完本篇应能：

1. 画出 I2C 子系统的分层：i2c_adapter → i2c_client → i2c_driver。
2. 说明 `i2c_msg`、`i2c_transfer` 的作用。
3. 写出 EEPROM 的"写地址 + 读数据"两段式传输。
4. 解释课程"错误版"与"正确版"的差异。
5. 判断什么时候该用子系统接口，而不是自己写字符设备。

## 环境与前置条件

| 项目 | 要求 |
| --- | --- |
| 硬件 | i.MX6ULL I2C1（或 STM32MP157 对应总线）+ AT24C02 |
| 内核配置 | `CONFIG_I2C`、对应 SoC 的 I2C 控制器驱动 |
| 前置 | Day8 设备树、Day10 阻塞与 ioctl |
| 工具 | `i2cdetect`（i2c-tools 包） |

## 核心概念

### 1. I2C 子系统分层

```text
应用
  │  /dev/at24c02（本实验自建的字符设备）
驱动 i2c_driver（probe / remove）
  │  i2c_transfer(adapter, msgs, n)
核心 i2c_core
  │
适配器 i2c_adapter（SoC 的 I2C 控制器）
  │
硬件 SCL / SDA
```

关键认识：**驱动不直接操作 SCL/SDA**，只提交消息给适配器。

### 2. 设备树的作用

```dts
&i2c1 {
	at24c02 {
		compatible = "100ask,at24c02";
		reg = <0x50>;
	};
};
```

- 节点挂在 `&i2c1` 下，内核会自动为其创建 `i2c_client`；
- `reg = <0x50>` 是 7 位从机地址；
- 驱动通过 `i2c_driver.of_match_table` 匹配，probe 收到可直接使用的 `client`。

**不需要**自己调用 `i2c_new_client_device`，也不需要写控制器寄存器。

### 3. EEPROM 的读写时序

写（字节写）：

```text
START → 从机地址+W → ACK → 存储地址 → ACK → 数据 → ACK → STOP
```

读（随机读）：

```text
START → 从机地址+W → ACK → 存储地址 → ACK
      → 重复 START → 从机地址+R → ACK → 数据 → NACK → STOP
```

第二段必须用**重复起始**（repeated start）。这就是 `i2c_msg msgs[2]` 的意义：

```c
struct i2c_msg msgs[2] = {
	{ .addr = client->addr, .flags = 0,        .len = 1,   .buf = &mem_addr },
	{ .addr = client->addr, .flags = I2C_M_RD, .len = len, .buf = buf      },
};
ret = i2c_transfer(client->adapter, msgs, 2);
```

### 4. 课程"错误版"与"正确版"

目录 `02_at24c02_read_write_err` 与 `03_at24c02_read_write_ok` 的对比是本日核心：

| 常见错误 | 现象 |
| --- | --- |
| 读操作只发一个 `I2C_M_RD` 消息 | 读不到正确数据，因为没先设置存储地址 |
| 把存储地址和数据放在同一个消息 | 总线时序错误 |
| 未检查 `i2c_transfer` 返回值 | 失败时当成成功 |
| EEPROM 写后立即读 | 器件内部写周期未完成，读到旧值 |

`i2c_transfer` 返回**成功传输的消息数**，不是字节数。必须与预期消息数比较。

### 5. EEPROM 写周期

AT24C02 每次写后需要约 5ms 内部写周期。连续写需等待，或轮询 ACK：

```c
/* 简单做法 */
msleep(10);

/* 更严谨：轮询直到器件应答 */
```

### 6. 什么时候用子系统

| 场景 | 做法 |
| --- | --- |
| 标准 EEPROM | 直接使用内核自带的 `at24` 驱动 |
| 自定义协议芯片 | 自己写 `i2c_driver` |
| 需要暴露给应用 | 在上层加字符设备、sysfs 或 IIO |

本实验为了教学，自己写了一个字符设备；工程中优先使用内核已有驱动。

## 系统调用/接口卡片

### `int i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)`

作用：一次性提交一组 I2C 消息。

参数：`msgs` 数组、消息数量。

返回值和错误：成功返回实际传输的消息数；失败返回负错误码。

资源所有权：无；`msgs` 与缓冲区由调用方管理。

线程/进程安全：适配器内部有锁；同一 client 的并发访问建议加 mutex。

最小示例：

```c
ret = i2c_transfer(client->adapter, msgs, 2);
if (ret != 2)
	return ret < 0 ? ret : -EIO;
```

常见误用：把返回值当成字节数；失败时继续使用缓冲区数据。

### `int i2c_smbus_read_byte_data(struct i2c_client *client, u8 command)`

作用：SMBus 协议的简化读写，适用于兼容器件。

返回值和错误：成功返回读到的字节；失败返回负错误码。

常见误用：对只支持 I2C 原始协议的器件使用 SMBus 接口。

### `static int probe(struct i2c_client *client, const struct i2c_device_id *id)`

作用：匹配成功后的初始化。

注意：新内核中第二个参数已移除（`probe(struct i2c_client *client)`），可用 `i2c_get_clientdata()` 获取数据。

常见误用：沿用旧签名导致编译失败。

### `struct i2c_msg`

| 字段 | 含义 |
| --- | --- |
| `addr` | 7 位从机地址 |
| `flags` | `0` 表示写，`I2C_M_RD` 表示读 |
| `len` | 数据长度 |
| `buf` | 数据缓冲区 |

常见误用：`flags` 写成 `I2C_M_RD | 0` 之外的组合；`buf` 指向栈上已失效的内存。

## 最小可运行实验

实验目录：`Linux/experiments/Day13_I2C_AT24C02/`

```text
at24c02_drv.c     I2C 驱动：地址写入 + 重复起始读
at24c02_test.c    写入 8 字节、读回并比对
at24c02.dts       设备树片段
Makefile
README.md         接线、i2cdetect、验证步骤
```

核心：

```c
static int at24c02_read(struct i2c_client *c, u8 addr, u8 *buf, int len)
{
	struct i2c_msg msgs[2] = {
		{ .addr = c->addr, .flags = 0,        .len = 1,   .buf = &addr },
		{ .addr = c->addr, .flags = I2C_M_RD, .len = len, .buf = buf   },
	};
	int ret = i2c_transfer(c->adapter, msgs, 2);

	return ret == 2 ? 0 : (ret < 0 ? ret : -EIO);
}
```

## 编译与运行

```bash
cd Linux/experiments/Day13_I2C_AT24C02
make KERN_DIR=~/100ask_imx6ull-sdk/Linux-4.9.88
```

目标板：

```bash
i2cdetect -y 1          # 应看到 0x50
insmod at24c02_drv.ko
ls /dev/at24c02
./at24c02_test
dmesg | tail -10
rmmod at24c02_drv
```

## 结果观察

以下为**预期结果，未在本机实测**：

```text
$ i2cdetect -y 1
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
50: 50 -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

$ ./at24c02_test
write ok: 8 bytes
read  ok: 8 bytes
data match
```

## 常见错误和陷阱

| 现象 | 原因 | 定位方法 |
| --- | --- | --- |
| `i2cdetect` 看不到 0x50 | 接线、上拉电阻、总线编号 | 用示波器或换总线 |
| `i2c_transfer` 返回 `-ENXIO` | 从机无应答 | 检查地址与电源 |
| 读回全 0 或全 FF | 未先写存储地址 | 检查两段式消息 |
| 写后立即读得到旧值 | 未等待写周期 | 加 `msleep(10)` |
| 多进程读写错乱 | 未加锁 | 用 `mutex` 保护 EEPROM 访问 |
| `probe` 不执行 | `compatible` 不一致 | 检查 dts 与 `of_match_table` |

## 故障排查方法

1. **先确认总线**：`i2cdetect -y <bus>`。
2. **再确认节点**：`ls /sys/bus/i2c/devices/`。
3. **再确认传输**：在驱动中打印 `i2c_transfer` 返回值。
4. **最后确认数据**：用 `i2cget` / `i2cset` 手工读写对照。

## 练习题

### 基础题

题目：把固定地址 0 改为可配置的起始地址，支持写入 16 字节。

提示：AT24C02 页大小为 8 字节，跨页写需要拆分。

验收标准：写入 16 字节后逐字节读回一致。

### 改错题

题目：下面的读函数有什么问题？

```c
static int at24c02_read(struct i2c_client *c, u8 *buf, int len)
{
	struct i2c_msg msg = {
		.addr = c->addr, .flags = I2C_M_RD, .len = len, .buf = buf,
	};
	return i2c_transfer(c->adapter, &msg, 1);
}
```

提示：随机读需要先指定存储地址。

验收标准：能指出缺少地址写消息，并给出两段式实现。

### 综合题

题目：改用内核自带的 `at24` 驱动，通过 sysfs 读写 EEPROM，并对比自写驱动的差异。

提示：设备树 `compatible = "atmel,24c02"`，节点路径在 `/sys/bus/i2c/devices/1-0050/`。

验收标准：能用 `dd` 或 sysfs 完成读写，并说明自写驱动的额外价值。

## 验收标准

- [ ] 能画出 I2C 子系统分层。
- [ ] 能解释 `i2c_msg` 各字段与 `i2c_transfer` 返回值语义。
- [ ] 能写出重复起始的两段式读。
- [ ] 能说出至少三个 EEPROM 访问的常见错误。
- [ ] 在目标板完成一次读写比对（当前环境未完成）。

## 延伸阅读

- `Linux_WEIDS/02_嵌入式Linux驱动入门直播/StudyPlan/DailyDocs/13_Day13_总线或子系统专题_学习讲解.md`
- `Linux_WEIDS/02_嵌入式Linux驱动入门直播/24_I2C设备驱动程序开发1_AT24C02程序框架/source/`
- `Linux_WEIDS/02_嵌入式Linux驱动入门直播/25_I2C设备驱动程序开发1_AT24C02编程与上机/source/`
- 下一篇：`Linux/notes/驱动入门/Day14_SPI_Framebuffer与总复盘.md`

---

**验证记录**

| 项目 | 内容 |
| --- | --- |
| 日期 | 2026-10-25 |
| 已执行 | 源码阅读：`03_at24c02_read_write_ok` 的驱动结构、`at24c02.dts` |
| 未执行 | 编译、`i2cdetect`、读写比对 |
| 结论 | 运行结果为预期值，未实测 |
