// ============================================================
// SPI / I2C 协议概述 - 超详细注释版
// 日期：2026-06-03（第59天）
// 难度：⭐⭐ 中级
// 编译：g++ -std=c++17 -Wall -o 12_spi_i2c 12_spi_i2c_overview.cpp
// ============================================================

/*
【核心概念】

SPI（Serial Peripheral Interface）：同步串行，高速，主从模式
I2C（Inter-Integrated Circuit）：同步串行，低速，多主多从，只需2根线

选择原则：
- 高速（ADC/显示屏/SD卡）→ SPI
- 多设备少引脚（温湿度/RTC/EEPROM）→ I2C
*/

#include <iostream>
#include <cstdint>
#include <vector>
#include <functional>
#include <cstring>
#include <map>
using namespace std;


// ============================================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================================

void part1_concepts() {
    cout << string(60, '=') << "\n";
    cout << "第一部分：SPI vs I2C 对比\n";
    cout << string(60, '=') << "\n";

    cout << R"(
【SPI 总线】

引脚：
  MOSI（Master Out Slave In）：主机发，从机收
  MISO（Master In Slave Out）：从机发，主机收
  SCK （Serial Clock）       ：主机提供时钟
  CS/SS（Chip Select）       ：片选，低有效，每个从设备独占一根

特点：
  ✅ 全双工（同时收发）
  ✅ 速度快（几MHz~几十MHz）
  ✅ 无地址，片选线直接选从设备
  ❌ 多从设备需要多根 CS 线（引脚消耗大）
  ❌ 无应答机制（不知道从设备是否收到）

时序（Mode 0：CPOL=0 CPHA=0，最常用）：
  CS  ____↓_________↑____
  SCK ____⌐‾⌐‾⌐‾⌐‾⌐‾____
  MOSI ───D7 D6 D5 D4 D3 D2 D1 D0───
  MISO ───Q7 Q6 Q5 Q4 Q3 Q2 Q1 Q0───
  （时钟上升沿采样，下降沿切换）

典型应用：
  - W25Q128 Flash 存储（SPI，80MHz）
  - ST7789 液晶屏（SPI，80MHz）
  - MPU6050 六轴传感器（支持SPI）
  - SD 卡（SPI 模式，25MHz）

────────────────────────────────────────────────────────────

【I2C 总线】

引脚：
  SDA（Serial Data） ：数据线（开漏，需4.7kΩ上拉）
  SCL（Serial Clock）：时钟线（开漏，需4.7kΩ上拉）

特点：
  ✅ 只需2根线，多设备共享（节省引脚）
  ✅ 有设备地址（7位，最多128个设备）
  ✅ 有应答机制（ACK/NAK，确认收到）
  ❌ 半双工（不能同时收发）
  ❌ 速度较慢（标准100kHz，快速400kHz，高速3.4MHz）
  ❌ 地址冲突（同型号传感器默认地址相同）

帧格式：
  [START][设备地址7bit][R/W 1bit][ACK][数据8bit][ACK]...[STOP]

  START：SCL高时SDA下降沿
  STOP ：SCL高时SDA上升沿
  ACK  ：接收方拉低SDA一个周期（确认）
  NAK  ：SDA保持高（否认/结束）

典型应用：
  - DHT20 温湿度传感器（I2C，0x38）
  - DS3231 RTC 实时时钟（I2C，0x68）
  - AT24C32 EEPROM（I2C，0x50~0x57）
  - OLED 显示屏 0.96"（I2C，0x3C/0x3D）

────────────────────────────────────────────────────────────

【SPI vs I2C 选择决策树】

需要高速（>400kHz）？
  是 → SPI

多个同类型传感器（地址冲突）？
  是 → SPI（用 CS 区分）

引脚资源紧张？
  是 → I2C（2根线搞定所有从设备）

需要确认数据收到？
  是 → I2C（有 ACK）

默认选择：I2C（简单，够用）

)";
}


// ============================================================
// 第二部分：SPI 驱动模拟实现
// ============================================================

// ------------------------------------------------------------
// 2.1 SPI 抽象接口
// ------------------------------------------------------------

class ISpi {
public:
    virtual ~ISpi() = default;

    // 片选操作
    virtual void csLow()  = 0;  // 选中从设备（CS 拉低）
    virtual void csHigh() = 0;  // 取消选中（CS 拉高）

    // 全双工传输：发送 tx，同时接收并返回 rx
    // SPI 特性：每发一字节同时收一字节
    virtual uint8_t transfer(uint8_t tx) = 0;

    // 批量传输（默认实现：逐字节）
    virtual void transfer(const uint8_t* tx, uint8_t* rx, size_t len) {
        for (size_t i = 0; i < len; ++i)
            rx[i] = transfer(tx[i]);
    }

    // 只发不收（rx 丢弃）
    void write(uint8_t byte) { transfer(byte); }

    // 只收不发（tx 发 0xFF，SPI 空闲时主机发高电平）
    uint8_t read() { return transfer(0xFF); }
};


// ------------------------------------------------------------
// 2.2 MockSpi：PC 端模拟
// ------------------------------------------------------------

class MockSpi : public ISpi {
public:
    void csLow()  override { cs_ = false; }
    void csHigh() override { cs_ = true;  }

    uint8_t transfer(uint8_t tx) override {
        // 模拟：将发送的字节记录，返回预设的响应
        tx_log_.push_back(tx);
        uint8_t rx = rx_preset_.empty() ? 0x00 : rx_preset_[rx_idx_++ % rx_preset_.size()];
        return rx;
    }

    // 测试辅助：预设从设备响应数据
    void setRxData(vector<uint8_t> data) {
        rx_preset_ = move(data);
        rx_idx_    = 0;
    }

    // 查看发送记录
    const vector<uint8_t>& txLog() const { return tx_log_; }
    void clearLog() { tx_log_.clear(); rx_idx_ = 0; }

private:
    bool            cs_     = true;
    vector<uint8_t> tx_log_;
    vector<uint8_t> rx_preset_;
    size_t          rx_idx_ = 0;
};


// ------------------------------------------------------------
// 2.3 基于 SPI 的 W25Q128 Flash 驱动（简化）
// ------------------------------------------------------------

/*
W25Q128 常用命令：
  0x9F：读设备 ID（返回 EF 40 18）
  0x03：读数据（发地址24bit，返回数据）
  0x02：页写（发地址+数据，最多256字节）
  0x06：写使能
  0x05：读状态寄存器（bit0=BUSY）
*/

class W25Q128 {
public:
    explicit W25Q128(ISpi& spi) : spi_(spi) {}

    // 读设备 ID（验证通信是否正常）
    // 返回：[ManufacturerID, MemType, Capacity]
    void readId(uint8_t id[3]) {
        spi_.csLow();
        spi_.write(0x9F);       // 命令：JEDEC ID
        id[0] = spi_.read();    // 制造商 ID（W25Q：0xEF）
        id[1] = spi_.read();    // 存储类型（0x40）
        id[2] = spi_.read();    // 容量（128Mbit=0x18）
        spi_.csHigh();
    }

    // 读取数据
    // addr：24位地址（W25Q128：16MB寻址空间）
    // buf：接收缓冲区，len：读取字节数
    void readData(uint32_t addr, uint8_t* buf, size_t len) {
        spi_.csLow();
        spi_.write(0x03);                      // 命令：Read Data
        spi_.write((addr >> 16) & 0xFF);       // 地址高8位
        spi_.write((addr >>  8) & 0xFF);       // 地址中8位
        spi_.write((addr >>  0) & 0xFF);       // 地址低8位
        for (size_t i = 0; i < len; ++i)
            buf[i] = spi_.read();              // 读取数据
        spi_.csHigh();
    }

private:
    ISpi& spi_;
};


// ------------------------------------------------------------
// 2.4 I2C 抽象接口
// ------------------------------------------------------------

class II2c {
public:
    virtual ~II2c() = default;

    // 向指定地址的从设备写入数据
    // addr：7位从设备地址，data：数据，len：长度
    // 返回：true=ACK（成功），false=NAK（设备不存在/忙）
    virtual bool write(uint8_t addr, const uint8_t* data, size_t len) = 0;

    // 从指定地址的从设备读取数据
    virtual bool read(uint8_t addr, uint8_t* buf, size_t len) = 0;

    // 先写寄存器地址，再读数据（最常用操作：读寄存器）
    // reg_addr：寄存器地址（1字节），buf：接收缓冲区
    bool readReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t* buf, size_t len) {
        if (!write(dev_addr, &reg_addr, 1)) return false;  // 先写寄存器地址
        return read(dev_addr, buf, len);                    // 再读数据
    }

    // 写寄存器
    bool writeReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t value) {
        uint8_t buf[2] = {reg_addr, value};
        return write(dev_addr, buf, 2);
    }
};


// ------------------------------------------------------------
// 2.5 MockI2c：PC 端模拟
// ------------------------------------------------------------

class MockI2c : public II2c {
public:
    bool write(uint8_t addr, const uint8_t* data, size_t len) override {
        if (!device_exists(addr)) return false;  // NAK：设备不在线
        last_write_addr_ = addr;
        write_log_.insert(write_log_.end(), data, data + len);
        return true;
    }

    bool read(uint8_t addr, uint8_t* buf, size_t len) override {
        if (!device_exists(addr)) return false;
        auto it = rx_data_.find(addr);
        if (it == rx_data_.end()) { memset(buf, 0, len); return true; }
        auto& vec = it->second;
        for (size_t i = 0; i < len; ++i)
            buf[i] = (i < vec.size()) ? vec[i] : 0x00;
        return true;
    }

    // 测试辅助：注册在线设备
    void addDevice(uint8_t addr) { online_devices_.push_back(addr); }

    // 预设设备响应数据
    void setDeviceData(uint8_t addr, vector<uint8_t> data) {
        rx_data_[addr] = move(data);
    }

    const vector<uint8_t>& writeLog() const { return write_log_; }

private:
    vector<uint8_t> online_devices_;
    vector<uint8_t> write_log_;
    map<uint8_t, vector<uint8_t>> rx_data_;
    uint8_t last_write_addr_ = 0;

    bool device_exists(uint8_t addr) const {
        for (auto a : online_devices_) if (a == addr) return true;
        return false;
    }
};


// ------------------------------------------------------------
// 2.6 基于 I2C 的 DHT20 温湿度传感器驱动（简化）
// ------------------------------------------------------------

class DHT20 {
public:
    static constexpr uint8_t ADDR = 0x38;  // 固定 I2C 地址

    explicit DHT20(II2c& i2c) : i2c_(i2c) {}

    // 触发一次测量
    bool triggerMeasure() {
        // 发送触发命令：0xAC 0x33 0x00
        uint8_t cmd[] = {0xAC, 0x33, 0x00};
        return i2c_.write(ADDR, cmd, 3);
    }

    // 读取测量结果（等待 80ms 后调用）
    // temp_x10：温度×10（如 235 = 23.5℃）
    // humi_x10：湿度×10（如 650 = 65.0%）
    bool readResult(int16_t* temp_x10, uint16_t* humi_x10) {
        uint8_t buf[7];
        if (!i2c_.read(ADDR, buf, 7)) return false;
        if (buf[0] & 0x80) return false;  // bit7=1：测量未完成

        // 解析湿度（20位，buf[1]高8位 + buf[2]低8位 + buf[3]高4位）
        uint32_t raw_humi = ((uint32_t)buf[1] << 12)
                          | ((uint32_t)buf[2] <<  4)
                          | (buf[3] >> 4);
        *humi_x10 = (uint16_t)(raw_humi * 1000 / 1048576);  // 转换为 %×10

        // 解析温度（20位，buf[3]低4位 + buf[4] + buf[5]）
        uint32_t raw_temp = ((uint32_t)(buf[3] & 0x0F) << 16)
                          | ((uint32_t)buf[4] << 8)
                          |  buf[5];
        *temp_x10 = (int16_t)(raw_temp * 2000 / 1048576 - 500);  // 转换为 ℃×10

        return true;
    }

private:
    II2c& i2c_;
};


// ============================================================
// 演示
// ============================================================

void demo_spi() {
    cout << string(60, '=') << "\n";
    cout << "演示：SPI → W25Q128 Flash\n";
    cout << string(60, '=') << "\n";

    MockSpi spi;
    // 预设 W25Q128 的 JEDEC ID 响应
    spi.setRxData({0xEF, 0x40, 0x18});
    W25Q128 flash(spi);

    uint8_t id[3];
    flash.readId(id);
    cout << "Flash ID: "
         << hex << (int)id[0] << " "
         << (int)id[1] << " "
         << (int)id[2] << dec << "\n";
    cout << "发送命令字节：";
    for (auto b : spi.txLog()) cout << hex << (int)b << " ";
    cout << dec << "\n";
}

void demo_i2c() {
    cout << string(60, '=') << "\n";
    cout << "演示：I2C → DHT20 温湿度传感器\n";
    cout << string(60, '=') << "\n";

    MockI2c i2c;
    i2c.addDevice(DHT20::ADDR);

    // 预设传感器返回数据（模拟 23.5℃，65.0% 湿度）
    // 实际编码参考 DHT20 datasheet
    i2c.setDeviceData(DHT20::ADDR, {
        0x00,                    // 状态字节：bit7=0 测量完成
        0x6B, 0x5A, 0x01,        // 湿度原始数据（约65%）
        0x80, 0x9A, 0x50,        // 温度原始数据（约23.5℃）
    });

    DHT20 sensor(i2c);
    sensor.triggerMeasure();

    // 模拟等待 80ms（PC 上跳过）

    int16_t  temp;
    uint16_t humi;
    if (sensor.readResult(&temp, &humi)) {
        cout << "温度：" << temp / 10.0f << " ℃\n";
        cout << "湿度：" << humi / 10.0f << " %\n";
    } else {
        cout << "读取失败\n";
    }
}


// ============================================================
// 第三部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================================

void part3_traps() {
    cout << string(60, '=') << "\n";
    cout << "第三部分：常见错误和陷阱\n";
    cout << string(60, '=') << "\n";

    cout << R"(
1. SPI 模式配置错误（CPOL/CPHA）
──────────────────────────────────────
❌ 从设备要求 Mode1（CPOL=0 CPHA=1），主机配了 Mode0
   → 数据采样时机错误，读到全是 0x00 或 0xFF

✅ 查数据手册确认从设备的 SPI Mode
   常见：W25Q Flash=Mode0，SD卡=Mode0，部分ADC=Mode1/Mode3

2. I2C 缺少上拉电阻
──────────────────────────────────────
❌ SDA/SCL 没有接上拉电阻
   → 总线无法拉高，通信失败，逻辑分析仪看到波形不干净

✅ SDA 和 SCL 各接 4.7kΩ 到 VCC（3.3V 系统）
   高速 I2C（400kHz+）用 2.2kΩ

3. I2C 地址写错（7位地址 vs 8位地址）
──────────────────────────────────────
❌ 数据手册写 0x70，代码写 0x70
   部分手册给的是 8 位地址（含 R/W 位）
   实际 7 位地址 = 0x70 >> 1 = 0x38

✅ 确认数据手册的地址是7位还是8位
   HAL_I2C_Master_Transmit 需要7位地址左移1位：addr << 1

4. SPI 传输完未拉高 CS
──────────────────────────────────────
❌ flash.readId(id);
   // 忘记在 readId 末尾 csHigh()
   // → 下次通信从设备仍认为在接收，数据错乱

✅ 每次传输必须 csLow() → transfer() → csHigh() 成对出现
   用 RAII 包装：SpiTransaction 构造时 csLow，析构时 csHigh

5. I2C 总线锁死（Bus Hang）
──────────────────────────────────────
❌ 通信中途断电，SCL 或 SDA 卡在低电平
   下次上电后 I2C 总线仍被锁住，无法通信

✅ 复位后手动给 SCL 9个时钟脉冲（让从设备完成当前字节传输）
   STM32 HAL 提供 HAL_I2C_DeInit() + 手动时钟脉冲恢复

)";
}


// ============================================================
// 第四部分：函数卡片速查
// ============================================================

void part4_cards() {
    cout << string(60, '=') << "\n";
    cout << "第四部分：函数卡片速查\n";
    cout << string(60, '=') << "\n";

    cout << R"(
【ISpi 接口】
  csLow() / csHigh()              — 片选操作
  transfer(tx) → rx               — 全双工传输一字节
  write(byte)                     — 只发（rx 丢弃）
  read() → byte                   — 只收（发 0xFF）

【II2c 接口】
  write(addr, data, len) → bool   — 写从设备（返回 ACK 状态）
  read(addr, buf, len)  → bool    — 读从设备
  readReg(dev, reg, buf, len)     — 先写寄存器地址再读（最常用）
  writeReg(dev, reg, val)         — 写寄存器

【STM32 HAL SPI API】
  HAL_SPI_Transmit(&h, data, len, timeout)
  HAL_SPI_Receive(&h, buf, len, timeout)
  HAL_SPI_TransmitReceive(&h, tx, rx, len, timeout)  — 全双工

【STM32 HAL I2C API】
  HAL_I2C_Master_Transmit(&h, addr<<1, data, len, timeout)
  HAL_I2C_Master_Receive(&h, addr<<1, buf, len, timeout)
  HAL_I2C_Mem_Write(&h, addr<<1, reg, 1, data, len, timeout)  — 写寄存器
  HAL_I2C_Mem_Read(&h, addr<<1, reg, 1, buf, len, timeout)    — 读寄存器

)";
}


// ============================================================
// 第五部分：练习题（不提供答案）
// ============================================================

void part5_exercises() {
    cout << string(60, '=') << "\n";
    cout << "第五部分：练习题\n";
    cout << string(60, '=') << "\n";

    cout << R"(
1. 实现 SpiTransaction RAII 包装
   要求：
   - class SpiTransaction，构造时 csLow，析构时 csHigh
   - W25Q128::readId 改用 SpiTransaction 自动管理 CS
   提示：持有 ISpi& 引用，构造/析构调用 csLow/csHigh

2. 实现 I2C 总线扫描（Bus Scan）
   要求：
   - scanBus(II2c& i2c)：扫描 0x08~0x77 所有地址
   - 对每个地址发送0字节 write，ACK 则打印地址
   - MockI2c 预设几个在线设备，验证扫描结果
   提示：write(addr, nullptr, 0) 只发地址不发数据

3. 基于 I2C 实现 AT24C32 EEPROM 驱动
   要求：
   - write_page(addr, data, len)：页写（最大32字节）
   - read(addr, buf, len)：随机读
   - 地址：0x50（A0/A1/A2=0）
   提示：写操作后需等待 5ms（EEPROM 写入时间）

4. 实现 SPI 软件模拟（Bit-Bang）
   要求：
   - class BitBangSpi : public ISpi
   - 使用两个 IGpio 引脚（MOSI/SCK）模拟 SPI 时序
   - Mode 0：SCK 空闲低，上升沿采样
   提示：transfer() 中手动控制 SCK 和 MOSI 引脚电平，循环8次

)";
}


int main() {
    part1_concepts();
    demo_spi();
    cout << "\n";
    demo_i2c();
    part3_traps();
    part4_cards();
    part5_exercises();
    return 0;
}
