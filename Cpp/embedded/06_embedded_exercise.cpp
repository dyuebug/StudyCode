// ============================================================
// 嵌入式综合练习 - 超详细注释版
// 日期：2026-06-03（第57天）
// 难度：⭐⭐⭐ 高级
// 编译：g++ -std=c++20 -Wall -pthread -o 06_exercise 06_embedded_exercise.cpp
// ============================================================

/*
【综合练习目标】
本文件将前三节的知识融合：
  - 中断处理（volatile 标志、环形缓冲区）
  - HAL 抽象层（IGpio、IUart、LedDriver）
  - RTOS 原语（信号量、消息队列）

实现一个完整的"传感器数据采集系统"模拟：
  ISR 模拟 → 环形缓冲区 → 解析任务 → 消息队列 → 显示任务
*/

#include <iostream>
#include <atomic>
#include <thread>
#include <mutex>
#include <semaphore>
#include <queue>
#include <cstdint>
#include <string>
#include <functional>
#include <chrono>
using namespace std;
using namespace chrono;


// ============================================================
// 第一部分：基础组件复用（来自前三节）
// ============================================================

// --- HAL：GPIO 抽象 ---
enum class GpioDir   { Input, Output };
enum class GpioLevel { Low = 0, High = 1 };

class IGpio {
public:
    virtual ~IGpio() = default;
    virtual void      setDir(GpioDir) = 0;
    virtual void      write(GpioLevel) = 0;
    virtual GpioLevel read() const = 0;
    virtual void      toggle() = 0;
};

class MockGpio : public IGpio {
public:
    explicit MockGpio(const char* name) : name_(name) {}
    void      setDir(GpioDir d) override { dir_ = d; }
    void      write(GpioLevel l) override { level_ = l; }
    GpioLevel read() const override { return level_; }
    void      toggle() override {
        level_ = (level_ == GpioLevel::High) ? GpioLevel::Low : GpioLevel::High;
    }
    const char* name() const { return name_; }
private:
    const char* name_;
    GpioDir     dir_   = GpioDir::Input;
    GpioLevel   level_ = GpioLevel::Low;
};

// LED 驱动
class LedDriver {
public:
    explicit LedDriver(IGpio& gpio, const char* name) : gpio_(gpio), name_(name) {
        gpio_.setDir(GpioDir::Output);
        gpio_.write(GpioLevel::Low);
    }
    void on()     { gpio_.write(GpioLevel::High); }
    void off()    { gpio_.write(GpioLevel::Low);  }
    void toggle() { gpio_.toggle(); }
    bool isOn() const { return gpio_.read() == GpioLevel::High; }
    const char* name() const { return name_; }
private:
    IGpio&      gpio_;
    const char* name_;
};

// 线程安全消息队列
template<typename T>
class SafeQueue {
public:
    void push(T val) {
        { lock_guard<mutex> lk(mtx_); q_.push(move(val)); }
        sem_.release();
    }
    T pop() {
        sem_.acquire();
        lock_guard<mutex> lk(mtx_);
        T v = move(q_.front()); q_.pop();
        return v;
    }
    bool empty() const {
        lock_guard<mutex> lk(mtx_);
        return q_.empty();
    }
private:
    queue<T>                  q_;
    mutable mutex             mtx_;
    counting_semaphore<65535> sem_{0};
};


// ============================================================
// 第二部分：状态机（State Machine）——嵌入式最常用设计模式
// ============================================================

/*
【状态机概念】
是什么？
  有限个状态 + 触发状态转换的事件 + 每个状态的行为

为什么嵌入式常用？
  - 代替复杂的 if/else 嵌套，逻辑清晰
  - 易于扩展新状态，不破坏已有逻辑
  - 适合描述设备工作模式（空闲/运行/错误/关机）

实现方式：enum class 枚举状态 + switch-case 处理转换
*/

enum class DeviceState {
    Idle,       // 空闲：等待命令
    Sampling,   // 采样中：定期读取传感器
    Error,      // 错误：故障处理
    Shutdown    // 关机：清理资源后停止
};

enum class DeviceEvent {
    StartCmd,   // 收到启动命令
    StopCmd,    // 收到停止命令
    SampleDone, // 采样完成
    FaultDetect,// 检测到故障
    Reset       // 复位
};

// 将枚举转为可读字符串（调试用）
const char* state_name(DeviceState s) {
    switch (s) {
        case DeviceState::Idle:     return "Idle";
        case DeviceState::Sampling: return "Sampling";
        case DeviceState::Error:    return "Error";
        case DeviceState::Shutdown: return "Shutdown";
    }
    return "Unknown";
}

class DeviceStateMachine {
public:
    // 处理事件：根据当前状态和事件决定下一个状态及动作
    void handleEvent(DeviceEvent evt) {
        DeviceState next = state_;  // 默认不转换

        switch (state_) {
            case DeviceState::Idle:
                if (evt == DeviceEvent::StartCmd)    next = DeviceState::Sampling;
                if (evt == DeviceEvent::StopCmd)     next = DeviceState::Shutdown;
                break;

            case DeviceState::Sampling:
                if (evt == DeviceEvent::StopCmd)     next = DeviceState::Idle;
                if (evt == DeviceEvent::FaultDetect) next = DeviceState::Error;
                // SampleDone：保持 Sampling 状态，只执行动作
                if (evt == DeviceEvent::SampleDone) {
                    onSampleDone();
                    return;  // 不转换状态
                }
                break;

            case DeviceState::Error:
                if (evt == DeviceEvent::Reset)       next = DeviceState::Idle;
                break;

            case DeviceState::Shutdown:
                break;  // 关机状态不响应任何事件
        }

        if (next != state_) {
            onExit(state_);   // 退出旧状态的清理动作
            state_ = next;
            onEnter(state_);  // 进入新状态的初始化动作
        }
    }

    DeviceState state() const { return state_; }

private:
    DeviceState state_ = DeviceState::Idle;
    int         sample_count_ = 0;

    // 进入新状态时执行
    void onEnter(DeviceState s) {
        cout << "[状态机] → 进入 " << state_name(s) << "\n";
        if (s == DeviceState::Sampling) sample_count_ = 0;
        if (s == DeviceState::Error)    cout << "[状态机] ⚠️ 错误！等待复位\n";
        if (s == DeviceState::Shutdown) cout << "[状态机] 系统关机\n";
    }

    // 退出旧状态时执行
    void onExit(DeviceState s) {
        if (s == DeviceState::Sampling)
            cout << "[状态机] 停止采样，共采 " << sample_count_ << " 次\n";
    }

    // 采样完成动作（不转换状态）
    void onSampleDone() {
        ++sample_count_;
        cout << "[状态机] 采样 #" << sample_count_ << " 完成\n";
    }
};

void demo_state_machine() {
    cout << string(60, '=') << "\n";
    cout << "演示：设备状态机\n";
    cout << string(60, '=') << "\n";

    DeviceStateMachine sm;
    cout << "初始状态：" << state_name(sm.state()) << "\n\n";

    sm.handleEvent(DeviceEvent::StartCmd);     // Idle → Sampling
    sm.handleEvent(DeviceEvent::SampleDone);   // 采样#1（保持Sampling）
    sm.handleEvent(DeviceEvent::SampleDone);   // 采样#2
    sm.handleEvent(DeviceEvent::FaultDetect);  // Sampling → Error
    sm.handleEvent(DeviceEvent::SampleDone);   // Error状态，无响应
    sm.handleEvent(DeviceEvent::Reset);        // Error → Idle
    sm.handleEvent(DeviceEvent::StartCmd);     // Idle → Sampling
    sm.handleEvent(DeviceEvent::SampleDone);   // 采样#1
    sm.handleEvent(DeviceEvent::StopCmd);      // Sampling → Idle
    sm.handleEvent(DeviceEvent::StopCmd);      // Idle → Shutdown
    cout << "最终状态：" << state_name(sm.state()) << "\n";
}


// ============================================================
// 第三部分：完整系统集成
// ============================================================

/*
系统架构：
  [模拟ADC ISR线程]
       ↓ 写入环形缓冲区 + 释放信号量
  [采样解析任务]
       ↓ 从缓冲区读取 + 解析 → 放入消息队列
  [显示任务]
       ↓ 从消息队列取数据 + 控制 LED 状态指示
*/

// --- 环形缓冲区（来自03_interrupt_handling.cpp）---
constexpr uint8_t RING_SIZE = 32;
struct RingBuf {
    volatile uint8_t data[RING_SIZE];
    volatile uint8_t head = 0;
    volatile uint8_t tail = 0;
};
RingBuf g_adc_ring;

bool ring_push(uint8_t byte) {
    uint8_t next = (g_adc_ring.tail + 1) % RING_SIZE;
    if (next == g_adc_ring.head) return false;  // 满
    g_adc_ring.data[g_adc_ring.tail] = byte;
    g_adc_ring.tail = next;
    return true;
}
bool ring_pop(uint8_t* out) {
    if (g_adc_ring.head == g_adc_ring.tail) return false;  // 空
    *out = g_adc_ring.data[g_adc_ring.head];
    g_adc_ring.head = (g_adc_ring.head + 1) % RING_SIZE;
    return true;
}

// --- 系统信号量 ---
counting_semaphore<RING_SIZE> g_adc_sem{0};  // ISR 触发采样任务
atomic<bool>                  g_running{true};

// --- 传感器消息 ---
struct SensorData { uint8_t raw; float voltage; };
SafeQueue<SensorData> g_data_queue;

// --- 模拟 ADC ISR 线程（每10ms触发一次，共5次）---
void adc_isr_thread() {
    for (int i = 0; i < 5; ++i) {
        this_thread::sleep_for(milliseconds(10));
        uint8_t val = static_cast<uint8_t>(50 + i * 20);  // 模拟 ADC 值
        ring_push(val);
        g_adc_sem.release();  // 通知采样任务
        cout << "[ADC ISR] 转换完成，原始值=" << (int)val << "\n";
    }
}

// --- 采样解析任务：从缓冲区读数据，转换为物理量，放入队列 ---
void sampling_task() {
    int processed = 0;
    while (processed < 5) {
        g_adc_sem.acquire();       // 等待 ISR 通知
        uint8_t raw;
        if (ring_pop(&raw)) {
            // ADC 值转电压：假设 3.3V / 255 分辨率
            float voltage = raw * 3.3f / 255.0f;
            cout << "[采样任务] 处理 raw=" << (int)raw
                 << " → " << voltage << "V\n";
            g_data_queue.push({raw, voltage});
            ++processed;
        }
    }
}

// --- 显示任务：从队列取数据，更新 LED 状态 ---
void display_task(LedDriver& led_ok, LedDriver& led_warn) {
    for (int i = 0; i < 5; ++i) {
        SensorData d = g_data_queue.pop();
        // 电压 > 2.5V：正常（绿灯亮）；否则告警（红灯亮）
        if (d.voltage > 2.5f) {
            led_ok.on();  led_warn.off();
            cout << "[显示任务] " << d.voltage << "V — 正常 ✅\n";
        } else {
            led_ok.off(); led_warn.on();
            cout << "[显示任务] " << d.voltage << "V — 告警 ⚠️\n";
        }
    }
    // 结束：熄灭所有指示灯
    led_ok.off(); led_warn.off();
}

void demo_integrated_system() {
    cout << string(60, '=') << "\n";
    cout << "演示：传感器数据采集系统（完整集成）\n";
    cout << string(60, '=') << "\n";

    // 创建 GPIO 和 LED 驱动
    MockGpio gpio_ok("PC13_GREEN");
    MockGpio gpio_warn("PA1_RED");
    LedDriver led_ok(gpio_ok, "绿灯");
    LedDriver led_warn(gpio_warn, "红灯");

    // 启动三个并发"任务"（模拟 RTOS 多任务）
    thread isr_t(adc_isr_thread);
    thread sample_t(sampling_task);
    thread display_t(display_task, ref(led_ok), ref(led_warn));

    isr_t.join();
    sample_t.join();
    display_t.join();

    cout << "\n系统运行完成。\n";
}


// ============================================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================================

void part4_traps() {
    cout << string(60, '=') << "\n";
    cout << "第四部分：常见错误和陷阱\n";
    cout << string(60, '=') << "\n";

    cout << R"(
1. 状态机中忘记处理所有状态的所有事件
──────────────────────────────────────
❌ 只处理了"正常"流程，未处理 Error 状态下的 StartCmd
   → 系统进入 Error 后永远卡住（若没有 Reset 事件路径）

✅ 每个状态都明确说明对每种事件的响应（转换 or 忽略）
✅ 用 default: break 明确表示"此状态忽略该事件"

2. 状态机 onEnter/onExit 中做耗时操作
──────────────────────────────────────
❌ onEnter(Sampling) { init_sensor(); delay(100); }
   // 状态转换时阻塞了整个系统

✅ onEnter(Sampling) { request_init = true; }  // 只设标志
   // 实际初始化在 Sampling 状态的第一次 update() 中完成

3. 集成系统中任务生命周期不匹配
──────────────────────────────────────
❌ thread display_t(display_task, led_ok, led_warn);
   // led_ok/led_warn 是局部变量，display_task 线程还在运行时可能已析构

✅ thread display_t(display_task, ref(led_ok), ref(led_warn));
   // 用 ref() 传引用，并确保 led 的生命周期覆盖线程运行期

4. 环形缓冲区 RING_SIZE 不是2的幂次
──────────────────────────────────────
❌ constexpr uint8_t RING_SIZE = 30;  // 取模用除法，慢
✅ constexpr uint8_t RING_SIZE = 32;  // 2的幂次，可用位与替代取模
   // 优化：idx = (idx + 1) & (RING_SIZE - 1);

5. 消息队列项目过大导致栈压力
──────────────────────────────────────
❌ SafeQueue<LargeStruct> q;  // LargeStruct 包含1KB数据
   // push/pop 每次都复制1KB，嵌入式内存吃紧

✅ 方案1：SafeQueue<unique_ptr<LargeStruct>>  // 只传指针
   方案2：使用内存池 + 索引队列，避免动态分配

6. 状态机状态枚举和事件枚举命名混淆
──────────────────────────────────────
❌ enum State { IDLE, RUNNING };
   enum Event { IDLE, START };  // IDLE 重名！

✅ enum class DeviceState { Idle, Sampling };   // enum class 有作用域
   enum class DeviceEvent { StartCmd, StopCmd };
   // 访问：DeviceState::Idle，永不与 DeviceEvent::Idle 冲突

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
1. 扩展状态机：添加低电量（LowBattery）状态
   要求：
   - 新增 DeviceEvent::BatteryLow
   - 任何状态收到 BatteryLow 都转到 LowBattery
   - LowBattery 下只响应 Reset（回到 Idle）
   提示：在每个 case 的最后加 if (evt == BatteryLow) next = LowBattery

2. 为 DeviceStateMachine 添加状态转换日志
   要求：
   - 记录所有转换：from_state, to_state, trigger_event, timestamp
   - vector<TransitionLog> history_
   - 提供 printHistory() 方法打印完整转换历史
   提示：TransitionLog 结构包含上面4个字段

3. 实现带超时的 SafeQueue::pop()
   要求：
   - pop(milliseconds timeout) → optional<T>
   - 超时返回 nullopt，未超时返回数据
   提示：C++20 sem_.try_acquire_for(timeout)

4. 完善集成系统：添加看门狗（Watchdog）
   要求：
   - WatchdogTimer：每500ms必须被"喂狗"一次
   - 采样任务每次处理完数据后调用 wdt.feed()
   - 超过500ms未喂狗，WatchdogTimer 打印告警并设置错误标志
   - 主函数检测到错误标志后向状态机发送 FaultDetect 事件
   提示：用 atomic<steady_clock::time_point> 记录上次喂狗时间

5. 将状态机改为表驱动（Table-Driven）
   要求：
   - 用 map<pair<State,Event>, pair<State,Action>> 存储转换表
   - handleEvent() 只需查表，不需要 switch-case
   - 添加新状态/事件只需在表中增加一行，无需修改核心逻辑
   提示：Action 用 function<void()> 存储

)";
}


int main() {
    demo_state_machine();
    cout << "\n";
    demo_integrated_system();
    part4_traps();
    part5_exercises();
    return 0;
}
