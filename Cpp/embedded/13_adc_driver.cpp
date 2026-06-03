// ============================================================
// 嵌入式 ADC 驱动 - 超详细注释版
// 日期：2026-06-03（第60天）
// 难度：⭐⭐ 中级
// 编译：g++ -std=c++17 -Wall -o 13_adc 13_adc_driver.cpp
// ============================================================

/*
【核心概念】

是什么？
- ADC（Analog-to-Digital Converter）：模数转换器
- 将连续的模拟信号（电压）转换为离散的数字值

为什么重要？
- 读取传感器：温度、光照、压力、电池电压等都是模拟信号
- MCU 只能处理数字信号，ADC 是连接物理世界的桥梁

核心参数：
- 分辨率：决定转换精度（12位 = 4096级）
- 采样率：每秒能采多少次（影响能跟踪多快的信号）
- 参考电压：决定满量程（通常 = VCC = 3.3V）
*/

#include <iostream>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>
using namespace std;


// ============================================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================================

void part1_concepts() {
    cout << string(60, '=') << "\n";
    cout << "第一部分：ADC 核心概念\n";
    cout << string(60, '=') << "\n";

    cout << R"(
【分辨率与精度】

12位 ADC：
  数字输出范围：0 ~ 4095（2^12 - 1）
  电压分辨率  ：3.3V / 4096 ≈ 0.8mV（能分辨最小电压变化）
  换算公式    ：电压(V) = ADC值 × Vref / (2^bits - 1)
               电压 = 2048 × 3.3 / 4095 ≈ 1.65V（中间值）

10位 ADC（如 Arduino UNO）：
  数字输出范围：0 ~ 1023
  电压分辨率  ：5V / 1024 ≈ 4.9mV

【采样率】
  采样定理（奈奎斯特）：采样率 ≥ 2 × 信号最高频率
  音频（20kHz）：采样率至少 40kHz（通常用 44.1kHz）
  工频干扰（50Hz）：采样率至少 100Hz
  MCU ADC 典型采样率：100kSPS ~ 5MSPS

【参考电压（Vref）】
  内部参考：MCU 内置稳定电压（如 1.2V），不受 VCC 波动影响
  外部参考：接精密基准芯片（如 REF3033），精度更高
  VCC 参考：直接用电源电压，简单但受电源噪声影响

【采样模式】
  单次（One-Shot）：触发一次，采样一次，节省功耗
  连续（Continuous）：不断采样，适合实时监控
  扫描（Scan）：自动循环采样多个通道，配合 DMA

)";

    // 演示换算
    cout << string(60, '-') << "\n";
    cout << "ADC 值换算示例（12位，Vref=3.3V）：\n";
    const float VREF = 3.3f;
    const int   BITS = 12;
    const int   MAX  = (1 << BITS) - 1;  // 4095

    for (int raw : {0, 1024, 2048, 3072, 4095}) {
        float voltage = (float)raw * VREF / MAX;
        cout << "  ADC=" << raw << " → " << voltage << " V\n";
    }
}


// ============================================================
// 第二部分：基础操作实践
// ============================================================

// ------------------------------------------------------------
// 2.1 ADC 抽象接口
// ------------------------------------------------------------

class IAdc {
public:
    virtual ~IAdc() = default;

    // 触发单次采样并等待完成（轮询模式）
    // 返回原始 ADC 值（0 ~ 2^bits-1）
    virtual uint16_t readRaw() = 0;

    // 读取电压值（单位：V）
    float readVoltage(float vref = 3.3f, uint8_t bits = 12) {
        return (float)readRaw() * vref / ((1 << bits) - 1);
    }

    // 均值滤波：采样 n 次取平均，消除随机噪声
    uint16_t readAverage(uint8_t n = 8) {
        uint32_t sum = 0;
        for (uint8_t i = 0; i < n; ++i) sum += readRaw();
        return static_cast<uint16_t>(sum / n);
    }

    // 中值滤波：采样 n 次取中位数，消除脉冲干扰
    uint16_t readMedian(uint8_t n = 7) {
        vector<uint16_t> samples(n);
        for (uint8_t i = 0; i < n; ++i) samples[i] = readRaw();
        sort(samples.begin(), samples.end());
        return samples[n / 2];  // 取中间值
    }
};


// ------------------------------------------------------------
// 2.2 MockAdc：PC 端模拟（可注入测试数据）
// ------------------------------------------------------------

class MockAdc : public IAdc {
public:
    // 设置固定返回值（模拟稳定信号）
    void setFixed(uint16_t val) { fixed_ = val; use_seq_ = false; }

    // 设置序列返回值（模拟变化信号）
    void setSequence(vector<uint16_t> seq) {
        seq_     = move(seq);
        idx_     = 0;
        use_seq_ = true;
    }

    uint16_t readRaw() override {
        if (use_seq_ && !seq_.empty()) {
            uint16_t val = seq_[idx_];
            idx_ = (idx_ + 1) % seq_.size();  // 循环序列
            return val;
        }
        return fixed_;
    }

private:
    uint16_t         fixed_   = 2048;   // 默认中间值（约1.65V）
    vector<uint16_t> seq_;
    size_t           idx_     = 0;
    bool             use_seq_ = false;
};


// ------------------------------------------------------------
// 2.3 多通道 ADC 扫描（模拟 DMA 扫描模式）
// ------------------------------------------------------------

// 嵌入式 ADC 扫描：配置多个通道，DMA 自动依次采样写入数组
// 真实 STM32：配置 ADC1 扫描 CH0/CH1/CH2，DMA 填充 adc_buf[3]

struct AdcChannel {
    const char* name;    // 通道名称
    IAdc*       adc;     // 对应 ADC 实例（模拟每通道独立 ADC）
    float       vref;    // 该通道参考电压
    uint8_t     bits;    // 分辨率
};

class AdcScanner {
public:
    void addChannel(const char* name, IAdc* adc,
                    float vref = 3.3f, uint8_t bits = 12) {
        channels_.push_back({name, adc, vref, bits});
    }

    // 扫描所有通道，打印结果
    void scan() {
        cout << string(60, '-') << "\n";
        cout << "ADC 多通道扫描结果：\n";
        for (auto& ch : channels_) {
            uint16_t raw  = ch.adc->readAverage(4);  // 4次均值
            float    volt = (float)raw * ch.vref / ((1 << ch.bits) - 1);
            cout << "  [" << ch.name << "] raw=" << raw
                 << " → " << volt << " V\n";
        }
    }

private:
    vector<AdcChannel> channels_;
};


// ------------------------------------------------------------
// 2.4 完整演示
// ------------------------------------------------------------

void demo_adc() {
    cout << string(60, '=') << "\n";
    cout << "演示：ADC 采样 + 滤波\n";
    cout << string(60, '=') << "\n";

    // --- 单通道采样 ---
    MockAdc adc_temp;
    // 模拟带噪声的温度传感器信号（约 2000 ± 50）
    adc_temp.setSequence({2010, 1980, 2050, 1990, 2020, 1970, 2030, 2000,
                          2015, 1985, 100,  2005, 2025, 1995, 2010, 2000});

    cout << "原始采样（前8次）：";
    for (int i = 0; i < 8; ++i) cout << adc_temp.readRaw() << " ";
    cout << "\n";

    // 重置序列重新采样
    adc_temp.setSequence({2010, 1980, 2050, 1990, 2020, 1970, 2030, 2000,
                          2015, 1985, 100,  2005, 2025, 1995, 2010, 2000});
    cout << "均值滤波（8次平均）：" << adc_temp.readAverage(8) << "\n";

    adc_temp.setSequence({2010, 1980, 2050, 1990, 2020, 1970, 2030, 2000,
                          2015, 1985, 100,  2005, 2025, 1995, 2010, 2000});
    cout << "中值滤波（7次中位）：" << adc_temp.readMedian(7)
         << "（剔除了脉冲干扰 100）\n";

    // --- 多通道扫描 ---
    cout << "\n";
    MockAdc adc_battery;  adc_battery.setFixed(3600);   // 电池电压通道
    MockAdc adc_light;    adc_light.setFixed(1200);      // 光照传感器
    MockAdc adc_ntc;      adc_ntc.setFixed(2048);        // NTC温度传感器

    AdcScanner scanner;
    scanner.addChannel("电池(÷2分压)", &adc_battery);
    scanner.addChannel("光照传感器",   &adc_light);
    scanner.addChannel("NTC温度",      &adc_ntc);
    scanner.scan();
}


// ============================================================
// 🔍 深入理解：NTC 热敏电阻换算温度（可跳过）
// ============================================================

/*
NTC（Negative Temperature Coefficient）热敏电阻：
  温度↑ → 电阻↓ → 分压电路输出电压变化 → ADC 读取 → 换算温度

电路：VCC → R_fixed(10kΩ) → 节点 → NTC → GND
  V_adc = VCC × NTC / (R_fixed + NTC)
  NTC = R_fixed × V_adc / (VCC - V_adc)

Steinhart-Hart 方程（B参数近似）：
  1/T = 1/T0 + (1/B) × ln(R/R0)
  T0 = 298.15K（25℃），R0 = 10000Ω，B ≈ 3950

float ntc_to_celsius(uint16_t adc_raw) {
    float v   = adc_raw * 3.3f / 4095;
    float ntc = 10000.0f * v / (3.3f - v);       // 计算 NTC 阻值
    float t   = 1.0f / (log(ntc / 10000.0f) / 3950.0f + 1.0f / 298.15f);
    return t - 273.15f;                           // 开尔文转摄氏度
}
*/


// ============================================================
// 第三部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================================

void part3_traps() {
    cout << string(60, '=') << "\n";
    cout << "第三部分：常见错误和陷阱\n";
    cout << string(60, '=') << "\n";

    cout << R"(
1. ADC 通道引脚配置了上下拉
──────────────────────────────────────
❌ GPIO 配置为上拉输入，然后连接 ADC 通道
   → 上拉电阻影响传感器分压，读数偏高

✅ ADC 通道引脚必须配置为模拟模式（Analog）
   HAL：init.Mode = GPIO_MODE_ANALOG; init.Pull = GPIO_NOPULL;

2. 忘记等待采样完成就读值
──────────────────────────────────────
❌ HAL_ADC_Start(&hadc1);
   uint32_t val = HAL_ADC_GetValue(&hadc1);  // 转换还没完成！

✅ HAL_ADC_Start(&hadc1);
   HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);  // 等待完成
   uint32_t val = HAL_ADC_GetValue(&hadc1);

3. 整数换算溢出
──────────────────────────────────────
❌ uint16_t mv = adc_raw * 3300 / 4095;
   // adc_raw=4095，4095×3300=13513500 > uint16_t 最大值 65535? 实际不溢出
   // 但 adc_raw=4095，4095×33000=135135000 > uint32_t? 超过 uint16_t

✅ uint32_t mv = (uint32_t)adc_raw * 3300 / 4095;  // 先强制转 32位再乘

4. 参考电压不准导致系统误差
──────────────────────────────────────
❌ 代码写死 Vref = 3.3V，实际电路 VCC = 3.27V
   → 全量程有 0.9% 系统误差（不可校准）

✅ 方案1：测量实际 Vref，代入换算
   方案2：使用 MCU 内部基准（Vrefint）校准 Vref：
          实际Vref = 1.2V × 4095 / ADC(VREFINT通道)

5. 高速连续采样占满 CPU
──────────────────────────────────────
❌ while(1) { HAL_ADC_Start(); PollForConversion(); GetValue(); }
   // CPU 100% 在等待 ADC，无法执行其他任务

✅ 使用 ADC + DMA：HAL_ADC_Start_DMA(&hadc1, buf, len)
   DMA 自动搬运，完成后回调，CPU 期间完全空闲

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
【IAdc 接口（本文实现）】
  readRaw() → uint16_t         — 单次采样，返回原始值
  readVoltage(vref, bits)      — 直接返回电压（V）
  readAverage(n=8)             — n 次均值滤波
  readMedian(n=7)              — n 次中值滤波

【STM32 HAL ADC API】
  HAL_ADC_Start(&hadc)                        — 启动单次转换
  HAL_ADC_PollForConversion(&hadc, timeout)   — 等待转换完成
  HAL_ADC_GetValue(&hadc) → uint32_t          — 读取转换结果
  HAL_ADC_Stop(&hadc)                         — 停止转换

  HAL_ADC_Start_IT(&hadc)                     — 中断模式启动
  HAL_ADC_ConvCpltCallback(&hadc)             — 转换完成回调（重写）

  HAL_ADC_Start_DMA(&hadc, buf, len)          — DMA 模式（连续/扫描）
  HAL_ADC_ConvCpltCallback(&hadc)             — DMA 传输完成回调

【换算公式】
  电压 = raw × Vref / (2^bits - 1)
  12位：电压 = raw × 3.3 / 4095
  百分比（如电位器）：percent = raw × 100 / 4095

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
1. 实现 NTC 温度换算
   要求：
   - float ntc_celsius(uint16_t raw, float vref=3.3f, float r_fixed=10000.0f, float b=3950.0f)
   - 先换算 NTC 阻值，再用 B 参数公式换算温度
   - 测试：raw=2048 → NTC=10kΩ → 约25℃
   提示：需要 #include <cmath>，用 logf 和温度开尔文转摄氏度

2. 实现滑动平均滤波器（Moving Average）
   要求：
   - class MovingAverage，构造参数 window_size
   - update(uint16_t raw) → uint16_t：加入新值，返回当前均值
   - 内部用固定大小循环缓冲区（不用 vector 动态分配）
   提示：static uint16_t buf[N]，维护 sum 和 oldest_idx

3. 实现电池电量百分比估算
   要求：
   - 电池满电 4.2V，截止电压 3.0V（ADC 通过分压电路读取）
   - adcToPercent(uint16_t raw) → uint8_t（0~100）
   - 超出范围时截断（< 0 → 0，> 100 → 100）
   提示：先换算电压，再线性映射到 0~100%

4. 实现过采样提升分辨率（Oversampling）
   要求：
   - 采样 4^n 次后右移 n 位，将 12 位 ADC 提升到 (12+n) 位有效位
   - oversample(IAdc& adc, uint8_t n) → uint32_t
   - n=2：采样16次，提升到14位分辨率
   提示：sum 需要 uint32_t，防止溢出；结果右移 n 位

)";
}


int main() {
    part1_concepts();
    demo_adc();
    part3_traps();
    part4_cards();
    part5_exercises();
    return 0;
}
