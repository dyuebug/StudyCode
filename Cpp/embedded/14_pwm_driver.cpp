// ============================================================
// 嵌入式 PWM 驱动 - 超详细注释版
// 日期：2026-06-03（第60天）
// 难度：⭐⭐ 中级
// 编译：g++ -std=c++17 -Wall -pthread -o 14_pwm 14_pwm_driver.cpp
// ============================================================

/*
【核心概念】

是什么？
- PWM（Pulse Width Modulation）：脉冲宽度调制
- 用数字信号模拟模拟量：通过控制高电平占比来控制"平均功率"

为什么重要？
- LED 调光：占空比 0%→熄灭，100%→最亮，50%→半亮
- 电机调速：占空比决定平均电压，控制转速
- 舵机控制：特定占空比对应特定角度（0°~180°）

两个关键参数：
- 频率（Hz）：PWM 信号每秒完整周期数，决定刷新速度
- 占空比（Duty Cycle %）：高电平时间 / 周期时间
*/

#include <iostream>
#include <cstdint>
#include <functional>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <cmath>
using namespace std;
using namespace chrono;


// ============================================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================================

void part1_concepts() {
    cout << string(60, '=') << "\n";
    cout << "第一部分：PWM 核心概念\n";
    cout << string(60, '=') << "\n";

    cout << R"(
【PWM 波形示意】

占空比 75%（高电平占 3/4 周期）：
  ___       ___       ___
 |   |     |   |     |   |
_|   |_____|   |_____|   |_____
 ←─────────────────────────────→ 时间
 ←─周期T─→

占空比 25%：
  _         _         _
 | |       | |       | |
_| |_______| |_______| |_______

【频率选择原则】
  LED 调光  ：>= 100Hz（避免人眼察觉闪烁），常用 1kHz
  DC 电机   ：1kHz ~ 20kHz（太低有噪音，太高效率下降）
  舵机      ：50Hz（20ms 周期，1~2ms 高电平对应 0°~180°）
  开关电源  ：几十 kHz ~ 几百 kHz

【舵机控制】
  周期：20ms（50Hz）
  高电平时间：
    1.0ms → 0°
    1.5ms → 90°（中位）
    2.0ms → 180°
  占空比：1.0/20 = 5% → 0°，2.0/20 = 10% → 180°

【硬件 PWM vs 软件 PWM】
  硬件 PWM：定时器自动产生，精确且不占 CPU（推荐）
  软件 PWM：GPIO 手动翻转，占 CPU，精度受中断影响（引脚不足时用）

)";
}


// ============================================================
// 第二部分：基础操作实践
// ============================================================

// ------------------------------------------------------------
// 2.1 PWM 抽象接口
// ------------------------------------------------------------

class IPwm {
public:
    virtual ~IPwm() = default;

    // 启动 PWM 输出
    virtual void start() = 0;

    // 停止 PWM 输出（输出低电平）
    virtual void stop() = 0;

    // 设置占空比（0~100）
    virtual void setDuty(uint8_t percent) = 0;

    // 设置频率（Hz）
    virtual void setFrequency(uint32_t hz) = 0;

    uint8_t  duty() const { return duty_; }
    uint32_t freq() const { return freq_hz_; }

protected:
    uint8_t  duty_   = 0;
    uint32_t freq_hz_= 1000;
};


// ------------------------------------------------------------
// 2.2 MockPwm：PC 端模拟（记录设置，打印状态）
// ------------------------------------------------------------

class MockPwm : public IPwm {
public:
    explicit MockPwm(const char* name) : name_(name) {}

    void start() override {
        running_ = true;
        cout << "[PWM:" << name_ << "] 启动，freq=" << freq_hz_
             << "Hz duty=" << (int)duty_ << "%\n";
    }

    void stop() override {
        running_ = false;
        cout << "[PWM:" << name_ << "] 停止\n";
    }

    void setDuty(uint8_t percent) override {
        duty_ = (percent > 100) ? 100 : percent;
        if (running_)
            cout << "[PWM:" << name_ << "] 占空比 → " << (int)duty_ << "%\n";
    }

    void setFrequency(uint32_t hz) override {
        freq_hz_ = hz;
        cout << "[PWM:" << name_ << "] 频率 → " << hz << " Hz\n";
    }

    bool isRunning() const { return running_; }

private:
    const char* name_;
    bool        running_ = false;
};


// ------------------------------------------------------------
// 2.3 LED 呼吸灯（基于 PWM）
// ------------------------------------------------------------

// 呼吸灯：占空比从 0→100→0 渐变，产生"呼吸"效果
class BreathingLed {
public:
    explicit BreathingLed(IPwm& pwm, uint32_t period_ms = 2000)
        : pwm_(pwm), period_ms_(period_ms) {}

    // 计算 tick_ms 时刻的占空比（0~100）
    // 使用正弦波实现平滑过渡（比线性更自然）
    uint8_t calcDuty(uint32_t tick_ms) const {
        // 将时间映射到 [0, 2π]
        float phase = 2.0f * 3.14159f * (tick_ms % period_ms_) / period_ms_;
        // sin 范围 [-1, 1] → 映射到 [0, 100]
        float duty  = (sinf(phase) + 1.0f) * 50.0f;
        return static_cast<uint8_t>(duty);
    }

    // 模拟运行 steps 帧
    void simulate(uint32_t steps = 8, uint32_t tick_step_ms = 250) {
        pwm_.start();
        for (uint32_t i = 0; i < steps; ++i) {
            uint32_t tick = i * tick_step_ms;
            pwm_.setDuty(calcDuty(tick));
        }
        pwm_.stop();
    }

private:
    IPwm&    pwm_;
    uint32_t period_ms_;
};


// ------------------------------------------------------------
// 2.4 舵机驱动（50Hz PWM，1~2ms 高电平）
// ------------------------------------------------------------

class ServoDriver {
public:
    // pwm：50Hz PWM 通道
    explicit ServoDriver(IPwm& pwm) : pwm_(pwm) {
        pwm_.setFrequency(50);  // 舵机固定 50Hz
        pwm_.start();
        setAngle(90);           // 初始中位
    }

    // 设置舵机角度（0°~180°）
    void setAngle(uint8_t degrees) {
        degrees = min(degrees, (uint8_t)180);
        // 线性映射：0° → 5%（1ms/20ms），180° → 10%（2ms/20ms）
        // duty = 5 + degrees × 5 / 180
        uint8_t duty = static_cast<uint8_t>(5.0f + degrees * 5.0f / 180.0f);
        pwm_.setDuty(duty);
        angle_ = degrees;
        cout << "  舵机角度：" << (int)degrees << "° → 占空比 " << (int)duty << "%\n";
    }

    uint8_t angle() const { return angle_; }

private:
    IPwm&   pwm_;
    uint8_t angle_ = 90;
};


// ------------------------------------------------------------
// 2.5 软件 PWM（用 GPIO 手动翻转，基于定时器回调）
// ------------------------------------------------------------

// 软件 PWM：每个 tick（通常 1ms）调用一次 update()
// 内部计数器达到高电平时间 → 拉低；达到周期 → 重置，拉高
class SoftPwm {
public:
    // gpio_write：写 GPIO 的回调，true=HIGH false=LOW
    // period_ticks：PWM 周期（tick 数，如 period=20 表示 20ms @ 1kHz tick）
    SoftPwm(function<void(bool)> gpio_write, uint32_t period_ticks)
        : write_(gpio_write), period_(period_ticks), high_ticks_(0), counter_(0) {}

    void setDuty(uint8_t percent) {
        high_ticks_ = period_ * percent / 100;
    }

    // 每个 tick 调用一次（在 SysTick ISR 或定时器中断中调用）
    void tick() {
        ++counter_;
        if (counter_ >= period_) {
            counter_ = 0;
            if (high_ticks_ > 0) write_(true);  // 新周期开始拉高
        }
        if (counter_ == high_ticks_) {
            write_(false);  // 高电平结束拉低
        }
    }

private:
    function<void(bool)> write_;
    uint32_t             period_;       // 周期 tick 数
    uint32_t             high_ticks_;   // 高电平 tick 数
    uint32_t             counter_;      // 当前 tick 计数
};

void demo_soft_pwm() {
    cout << string(60, '-') << "\n";
    cout << "软件 PWM 模拟（period=10 ticks，duty=30%）\n";
    cout << string(60, '-') << "\n";

    bool gpio_state = false;
    auto write = [&](bool high) {
        gpio_state = high;
        cout << (high ? "H" : "L");
    };

    SoftPwm spwm(write, 10);  // 10 tick 周期
    spwm.setDuty(30);          // 30% 占空比 = 3 tick 高电平

    cout << "30tick 输出：";
    for (int i = 0; i < 30; ++i) spwm.tick();
    cout << "\n";
}


// ============================================================
// 演示
// ============================================================

void demo_pwm() {
    // --- LED 呼吸灯 ---
    cout << string(60, '=') << "\n";
    cout << "演示：LED 呼吸灯（正弦波占空比）\n";
    cout << string(60, '=') << "\n";
    MockPwm led_pwm("TIM3_CH1");
    led_pwm.setFrequency(1000);
    BreathingLed breath(led_pwm, 2000);
    breath.simulate(8, 250);

    // --- 舵机 ---
    cout << "\n" << string(60, '=') << "\n";
    cout << "演示：舵机控制\n";
    cout << string(60, '=') << "\n";
    MockPwm servo_pwm("TIM4_CH1");
    ServoDriver servo(servo_pwm);
    servo.setAngle(0);
    servo.setAngle(90);
    servo.setAngle(180);
    servo.setAngle(45);

    // --- 软件 PWM ---
    cout << "\n";
    demo_soft_pwm();
}


// ============================================================
// 第三部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================================

void part3_traps() {
    cout << string(60, '=') << "\n";
    cout << "第三部分：常见错误和陷阱\n";
    cout << string(60, '=') << "\n";

    cout << R"(
1. 舵机频率设错（非 50Hz）
──────────────────────────────────────
❌ pwm.setFrequency(1000);  // 1kHz，舵机不识别
   → 舵机抖动、不响应或损坏

✅ pwm.setFrequency(50);    // 舵机固定 50Hz（20ms 周期）

2. 占空比超出舵机安全范围
──────────────────────────────────────
❌ servo.setAngle(200);  // 超出 0~180° 范围
   → 占空比超过 10%，舵机机械撞限位，堵转烧毁

✅ degrees = min(degrees, (uint8_t)180);  // 限幅保护
   初次使用先在 90° 中位测试，再逐步扩大范围

3. LED 调光频率太低引起闪烁
──────────────────────────────────────
❌ pwm.setFrequency(10);  // 10Hz，人眼明显看到闪烁

✅ LED 调光至少 100Hz，推荐 1kHz 以上
   摄像头拍摄场景需 > 帧率（如 60Hz 摄像用 120Hz+）

4. 软件 PWM tick 调用不均匀
──────────────────────────────────────
❌ 主循环中调用 spwm.tick()，但主循环耗时不均
   → PWM 频率不稳定，LED 亮度闪烁

✅ 在 SysTick ISR（每 1ms 固定触发）中调用 tick()
   保证时间基准精确

5. 定时器重装载值计算错误
──────────────────────────────────────
❌ 想要 1kHz PWM，直接设 ARR=1000
   → 忽略了定时器时钟频率和预分频

✅ 公式：频率 = 定时器时钟 / (PSC+1) / (ARR+1)
   例：72MHz 时钟，PSC=71，ARR=999 → 72M/72/1000 = 1kHz

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
【IPwm 接口（本文实现）】
  start()                     — 启动输出
  stop()                      — 停止输出
  setDuty(0~100)              — 设置占空比百分比
  setFrequency(hz)            — 设置频率

【BreathingLed】
  BreathingLed(pwm, period_ms) — 构造，默认2秒周期
  calcDuty(tick_ms) → uint8_t  — 计算当前占空比（正弦波）
  simulate(steps, step_ms)     — 模拟运行

【ServoDriver】
  ServoDriver(pwm)             — 构造，设置50Hz，中位90°
  setAngle(0~180)              — 设置角度（自动转换占空比）

【SoftPwm】
  SoftPwm(write_cb, period_ticks) — 构造
  setDuty(0~100)               — 设置占空比
  tick()                       — 每个时间基准调用一次（ISR中）

【STM32 HAL PWM API】
  HAL_TIM_PWM_Start(&htim, TIM_CHANNEL_1)   — 启动 PWM
  HAL_TIM_PWM_Stop(&htim, TIM_CHANNEL_1)    — 停止 PWM
  __HAL_TIM_SET_COMPARE(&htim, CH, value)   — 设置比较值（占空比）
  占空比% = value / (ARR+1) × 100

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
1. 实现 PWM 渐变（Fade）
   要求：
   - fadeTo(IPwm& pwm, uint8_t target, uint32_t duration_ms)
   - 从当前占空比线性渐变到 target，总时长 duration_ms
   - 每 10ms 更新一次占空比
   提示：步长 = (target - current) / (duration_ms / 10)

2. 实现多路软件 PWM 管理器
   要求：
   - class SoftPwmManager，管理最多8路 SoftPwm
   - addChannel(write_cb, period_ticks) → channel_id
   - setDuty(channel_id, percent)
   - tickAll()：一次调用更新所有通道
   提示：用数组存储 SoftPwm 实例，tickAll() 循环调用每个 tick()

3. 实现 PWM 捕获（输入捕获）模拟
   要求：
   - 模拟测量一个外部 PWM 信号的频率和占空比
   - measurePwm(vector<uint32_t> edge_times)：输入上升/下降沿时间戳（μs）
   - 返回：频率（Hz）、占空比（%）
   提示：相邻两个上升沿差 = 周期；上升沿到下降沿差 = 高电平时间

4. 实现 BLDC 电机三相 PWM 换相
   要求：
   - 三个 PWM 通道（A/B/C）
   - 6步换相：每步两路 PWM 有效，一路 PWM 关闭
   - commutate(step) 按步骤设置三路通道的占空比
   提示：建立换相表 commutation_table[6][3]，每格填充 0 或 duty

)";
}


int main() {
    part1_concepts();
    demo_pwm();
    part3_traps();
    part4_cards();
    part5_exercises();
    return 0;
}
