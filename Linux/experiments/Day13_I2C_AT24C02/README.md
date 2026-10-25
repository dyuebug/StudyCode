# Day13 实验：AT24C02 I2C EEPROM

## 接线

| AT24C02 | 板卡 |
| --- | --- |
| VCC | 3.3V |
| GND | GND |
| SDA | I2C1_SDA（需上拉） |
| SCL | I2C1_SCL（需上拉） |
| A0/A1/A2 | GND（地址 0x50） |

## 步骤

```bash
i2cdetect -y 1          # 确认 0x50

make KERN_DIR=~/100ask_imx6ull-sdk/Linux-4.9.88
insmod at24c02_drv.ko
ls -l /dev/at24c02

./at24c02_test

rmmod at24c02_drv
```

## 预期输出

```text
$ ./at24c02_test
write ok: 8 bytes
read  ok: 8 bytes
data match
```

## 关键实现点

1. **写**：一条消息 `[存储地址][数据...]`
2. **读**：两条消息，第二条 `I2C_M_RD`，由适配器产生重复起始
3. **等待**：写后 `msleep(10)` 等内部写周期
4. **校验**：`i2c_transfer` 返回消息数，必须等于 1 或 2

## 常见问题

| 现象 | 处理 |
| --- | --- |
| `i2cdetect` 看不到 0x50 | 检查上拉、接线、总线编号 |
| `-ENXIO` | 从机无应答，检查地址与供电 |
| 读回全 0/FF | 缺少地址写消息 |
| 写后立即读旧值 | 增加写周期等待 |
| `probe` 不执行 | `compatible` 或 `status` 不对 |

## 验证状态

未在本机验证。需要目标板与 AT24C02 模块。
