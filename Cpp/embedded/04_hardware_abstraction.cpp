// ============================================================
// 嵌入式硬件抽象层（HAL）- 超详细注释版
// 日期：2026-06-03（第57天）
// 难度：⭐⭐ 中级
// 编译：g++ -std=c++17 -Wall -o 04_hal 04_hardware_abstraction.cpp
// ============================================================

/*
【核心概念】

是什么？
- HAL（Hardware Abstraction Layer）：用抽象接口隔离硬件细节
- 上层业务逻辑只依赖接口，不依赖具体硬件寄存器

为什么需要？
- 同一份业务代码可以在 STM32 / ESP32 / PC 模拟器上运行
- 换硬件只需换 HAL 实现，不改业务逻辑

核心特点：
- 纯虚基类定义接口（抽象），派生类实现具体硬件
- 依赖抽象（DIP 原则），不依赖具体实现
- 嵌入式常见外设：GPIO / UART / SPI / I2C / Timer
*/

#include <iostream>
#include <cstdint>
#include <array>
#include <functional>
using namespace std;


// ============================================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================================

/*
【HAL 设计三层结构】

┌─────────────────────────────────────┐
│  应用层 (Application)               │  ← 不知道硬件细节
│  如：LED闪烁、传感器读取、命令解析  │
├─────────────────────────────────────┤
│  HAL 接口层 (Interface)             │  ← 纯虚基类 / 抽象接口
│  如：IGpio / IUart / ISpi           │
├─────────────────────────────────────┤
│  BSP / 驱动层 (Implementation)     │  ← 具体硬件实现
│  如：Stm32Gpio / EspUart / MockGpio │
└─────────────────────────────────────┘

【GPIO 抽象】
  GPIO（General Purpose I/O）：通用输入输出引脚
  操作：设置方向（输入/输出）、读电平、写电平、切换电平

【UART 抽象】
  UART（Universal Asynchronous Receiver-Transmitter）：串行通信
  操作：发送字节、接收字节、检查是否有数据

【HAL 好处举例】
  测试时用 MockGpio，不需要真实硬件
  换MCU时只换 Stm32Gpio → EspGpio，业务代码不变
*/


// ============================================================
// 第二部分：基础操作实践
// ============================================================

// ------------------------------------------------------------
// 2.1 GPIO 抽象接口
// ------------------------------------------------------------

// GPIO 方向枚举：使用 enum class 防止命名冲突
enum class GpioDir  { Input, Output };
// GPIO 电平枚举
enum class GpioLevel{ Low = 0, High = 1 };

// IGpio：GPIO 抽象基类（接口）
// 纯虚函数 = 0，强制派生类实现
class IGpio {
public:
    virtual ~IGpio() = default;

    // 设置引脚方向：输入/输出
    virtual void setDir(GpioDir dir) = 0;

    // 写引脚电平（仅输出模式有效）
    virtual void write(GpioLevel level) = 0;

    // 读引脚电平（输入/输出均可读）
    virtual GpioLevel read() const = 0;

    // 切换引脚电平（输出模式，High↔Low）
    virtual void toggle() = 0;
};


// ------------------------------------------------------------
// 2.2 PC 模拟 GPIO 实现（用于测试，不依赖真实硬件）
// ------------------------------------------------------------

// MockGpio：在 PC 上模拟 GPIO，用于单元测试和开发调试
class MockGpio : public IGpio {
public:
    // 构造函数：引脚名称（方便打印调试信息）
    explicit MockGpio(const char* name) : name_(name) {}

    void setDir(GpioDir dir) override {
        dir_ = dir;
        cout << "[GPIO:" << name_ << "] 方向设置为 "
             << (dir == GpioDir::Output ? "Output" : "Input") << "\n";
    }

    void write(GpioLevel level) override {
        level_ = level;
        cout << "[GPIO:" << name_ << "] 写入 "
             << (level == GpioLevel::High ? "HIGH" : "LOW") << "\n";
    }

    GpioLevel read() const override {
        return level_;
    }

    void toggle() override {
        // 三元表达式：High 变 Low，Low 变 High
        level_ = (level_ == GpioLevel::High) ? GpioLevel::Low : GpioLevel::High;
        cout << "[GPIO:" << name_ << "] 切换为 "
             << (level_ == GpioLevel::High ? "HIGH" : "LOW") << "\n";
    }

private:
    const char* name_;                     // 引脚名称（调试用）
    GpioDir     dir_   = GpioDir::Input;   // 当前方向，默认输入
    GpioLevel   level_ = GpioLevel::Low;   // 当前电平，默认低电平
};


// ------------------------------------------------------------
// 2.3 UART 抽象接口
// ------------------------------------------------------------

class IUart {
public:
    virtual ~IUart() = default;

    // 发送一字节
    virtual void sendByte(uint8_t byte) = 0;

    // 发送字节数组：默认实现（逐字节调用 sendByte）
    // 派生类可以重写为 DMA 批量发送
    virtual void send(const uint8_t* data, size_t len) {
        for (size_t i = 0; i < len; ++i) sendByte(data[i]);
    }

    // 发送 C 字符串（嵌入式调试常用）
    void print(const char* str) {
        while (*str) sendByte(static_cast<uint8_t>(*str++));
    }

    // 检查是否有待读取的数据
    virtual bool available() const = 0;

    // 读取一字节（调用前先检查 available()）
    virtual uint8_t readByte() = 0;
};


// ------------------------------------------------------------
// 2.4 PC 模拟 UART 实现
// ------------------------------------------------------------

class MockUart : public IUart {
public:
    void sendByte(uint8_t byte) override {
        // 模拟：打印发送的字节
        cout << "[UART TX] " << (char)byte;
    }

    bool available() const override {
        // 模拟：总有数据可读（返回预设字节）
        return rx_ready_;
    }

    uint8_t readByte() override {
        rx_ready_ = false;
        return rx_data_;
    }

    // 测试辅助：模拟接收到数据
    void simulateRx(uint8_t byte) {
        rx_data_  = byte;
        rx_ready_ = true;
    }

private:
    bool    rx_ready_ = false;
    uint8_t rx_data_  = 0;
};


// ------------------------------------------------------------
// 2.5 LED 驱动：依赖 IGpio 接口，不关心具体硬件
// ------------------------------------------------------------

// LedDriver：LED 驱动，只依赖 IGpio 抽象
// 同一个 LedDriver 可以驱动 STM32 GPIO 或 ESP32 GPIO，只需换实现
class LedDriver {
public:
    // 构造函数：接收 IGpio 引用（依赖注入）
    // 注意：gpio 的生命周期必须比 LedDriver 长
    explicit LedDriver(IGpio& gpio) : gpio_(gpio) {
        gpio_.setDir(GpioDir::Output);  // LED 引脚配置为输出
        gpio_.write(GpioLevel::Low);    // 初始熄灭
    }

    void on()     { gpio_.write(GpioLevel::High); }  // 点亮
    void off()    { gpio_.write(GpioLevel::Low);  }  // 熄灭
    void toggle() { gpio_.toggle();               }  // 切换

    bool isOn() const {
        return gpio_.read() == GpioLevel::High;
    }

private:
    IGpio& gpio_;  // 引用，不持有所有权
};


// ------------------------------------------------------------
// 2.6 按键驱动：带软件防抖
// ------------------------------------------------------------

class ButtonDriver {
public:
    // gpio：按键引脚；debounce_ms：防抖时间（毫秒）
    explicit ButtonDriver(IGpio& gpio, uint32_t debounce_ms = 20)
        : gpio_(gpio), debounce_ms_(debounce_ms) {
        gpio_.setDir(GpioDir::Input);
    }

    // 轮询检测按键（主循环调用）
    // tick_ms：当前系统时间（毫秒），由调用方传入（依赖注入时间源）
    bool isPressed(uint32_t tick_ms) {
        GpioLevel cur = gpio_.read();

        // 电平变化：开始计时（等待稳定）
        if (cur != last_raw_level_) {
            last_change_tick_ = tick_ms;
            last_raw_level_   = cur;
        }

        // 稳定超过防抖时间，且当前是按下状态（低电平有效，通常按键按下拉低）
        bool stable_pressed = (cur == GpioLevel::Low)
                           && (tick_ms - last_change_tick_ >= debounce_ms_);

        // 边沿检测：只在按下瞬间返回 true（不重复触发）
        if (stable_pressed && !last_stable_state_) {
            last_stable_state_ = true;
            return true;   // 触发一次
        }
        if (!stable_pressed) {
            last_stable_state_ = false;
        }
        return false;
    }

private:
    IGpio&    gpio_;
    uint32_t  debounce_ms_;
    GpioLevel last_raw_level_   = GpioLevel::High;  // 默认高电平（未按下）
    uint32_t  last_change_tick_ = 0;
    bool      last_stable_state_= false;
};


// ============================================================
// 🔍 深入理解：依赖注入与可测试性（可跳过）
// ============================================================

/*
传统嵌入式代码（难以测试）：
  void led_on() { GPIOA->ODR |= (1 << 5); }  // 直接写寄存器
  // 在 PC 上无法运行，无法单元测试

HAL 模式（可测试）：
  void LedDriver::on() { gpio_.write(GpioLevel::High); }
  // 测试时传入 MockGpio，在 PC 上验证逻辑

依赖注入（Dependency Injection）：
  - 不在类内部创建硬件对象
  - 通过构造函数/方法参数传入接口引用
  - 测试时传 Mock，生产时传真实实现

工厂函数模式（为不同平台创建正确实现）：
  IGpio* create_led_gpio() {
  #ifdef STM32
      return new Stm32Gpio(GPIOA, 5);
  #elif defined(ESP32)
      return new EspGpio(GPIO_NUM_2);
  #else
      return new MockGpio("LED");
  #endif
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
1. HAL 接口忘记虚析构函数
──────────────────────────────────────
❌ class IGpio { public: virtual void write(GpioLevel) = 0; };
   delete igpio_ptr;  // 实际调用 IGpio::~IGpio()（非虚），MockGpio 析构不执行！

✅ class IGpio { public: virtual ~IGpio() = default; ... };

原因：通过基类指针删除派生类对象，非虚析构导致资源泄漏。

2. 接口引用悬空（生命周期问题）
──────────────────────────────────────
❌ LedDriver create_led() {
       MockGpio gpio("LED");   // gpio 是局部变量
       return LedDriver(gpio); // LedDriver 持有 gpio 的引用
   }                           // gpio 析构！LedDriver 持有悬空引用！

✅ // gpio 的生命周期必须 >= LedDriver 的生命周期
   MockGpio gpio("LED");    // 在更外层作用域声明
   LedDriver led(gpio);

3. 在 HAL 接口中暴露硬件细节
──────────────────────────────────────
❌ class IGpio {
       virtual void setRegister(uint32_t* reg, uint8_t pin) = 0;  // 暴露寄存器！
   };

✅ class IGpio {
       virtual void write(GpioLevel level) = 0;  // 只暴露行为
   };

原因：接口泄露实现细节，其他平台无法实现该接口。

4. send() 默认实现效率低但忘记重写
──────────────────────────────────────
默认 send() 逐字节调用 sendByte()，每字节都有函数调用开销。
高性能场景（DMA 传输）应重写 send() 为一次性 DMA 请求。

5. 防抖时间源依赖全局变量
──────────────────────────────────────
❌ bool ButtonDriver::isPressed() { return millis() - last > 20; }
   // millis() 全局函数，难以在测试中控制时间

✅ bool ButtonDriver::isPressed(uint32_t tick_ms) { ... }
   // 调用方传入时间，测试时可以传任意值模拟时间流逝

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
【IGpio 接口】
  setDir(GpioDir::Input / Output)  — 设置方向
  write(GpioLevel::High / Low)     — 写电平
  read() → GpioLevel               — 读电平
  toggle()                         — 切换电平

【IUart 接口】
  sendByte(uint8_t)                — 发送一字节
  send(const uint8_t*, size_t)     — 发送字节数组（默认逐字节）
  print(const char*)               — 发送 C 字符串
  available() → bool               — 是否有待读取数据
  readByte() → uint8_t             — 读取一字节

【LedDriver】
  LedDriver(IGpio& gpio)           — 构造，gpio 配置为输出
  on() / off() / toggle()          — 控制 LED
  isOn() → bool                    — 查询状态

【ButtonDriver】
  ButtonDriver(IGpio&, uint32_t debounce_ms=20)
  isPressed(uint32_t tick_ms) → bool   — 检测按键（带防抖，边沿触发）

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
1. 实现 ISpi 接口（SPI 总线抽象）
   要求：
   - 纯虚函数：chipSelect(bool)、transferByte(uint8_t) → uint8_t
   - MockSpi 实现：记录所有传输的字节，方便断言测试
   提示：SPI 是全双工，发送同时接收，用 transferByte 同时处理

2. 实现 PWM 驱动（基于 IGpio 的软件 PWM）
   要求：
   - class SoftPwm，构造接受 IGpio& 和频率
   - setDutyCycle(uint8_t percent)：0-100
   - tick()：每 1ms 调用一次，控制 GPIO 翻转实现 PWM
   提示：计算高电平时间 = period_ms * duty / 100

3. 为 LedDriver 添加闪烁功能
   要求：
   - blink(uint32_t on_ms, uint32_t off_ms)：设置闪烁参数
   - update(uint32_t tick_ms)：主循环调用，根据时间控制亮灭
   提示：记录上次切换时间，比较差值决定是否切换

4. 实现带回调的 GPIO 中断接口
   要求：
   - IGpio 增加：attachInterrupt(function<void()> cb, EdgeType edge)
   - MockGpio 实现：simulate_edge() 触发回调
   - EdgeType：Rising / Falling / Both
   提示：用 std::function 存储回调

)";
}


// ============================================================
// 主函数：运行所有演示
// ============================================================

int main() {
    // 概念讲解（注释形式，见文件顶部）

    // --- 演示 2.5：LedDriver ---
    cout << string(60, '=') << "\n";
    cout << "演示：LedDriver\n";
    cout << string(60, '=') << "\n";

    MockGpio led_gpio("PA5");          // 模拟 STM32 PA5 引脚（LED）
    LedDriver led(led_gpio);           // 创建 LED 驱动，内部调用 setDir + write

    led.on();
    cout << "LED 状态：" << (led.isOn() ? "亮" : "灭") << "\n";
    led.toggle();
    cout << "LED 状态：" << (led.isOn() ? "亮" : "灭") << "\n";
    led.off();

    // --- 演示 2.6：ButtonDriver ---
    cout << "\n" << string(60, '=') << "\n";
    cout << "演示：ButtonDriver（软件防抖）\n";
    cout << string(60, '=') << "\n";

    MockGpio btn_gpio("PA0");
    ButtonDriver btn(btn_gpio, 20);    // 20ms 防抖

    // 模拟：t=0 按键按下（电平变低）
    btn_gpio.write(GpioLevel::Low);
    bool triggered = btn.isPressed(0);
    cout << "t=0ms，按键按下，触发=" << triggered << "（防抖中）\n";

    triggered = btn.isPressed(10);
    cout << "t=10ms，触发=" << triggered << "（防抖中）\n";

    triggered = btn.isPressed(25);
    cout << "t=25ms，触发=" << triggered << "（应为1，防抖完成）\n";

    triggered = btn.isPressed(30);
    cout << "t=30ms，触发=" << triggered << "（应为0，边沿检测）\n";

    // --- 演示 2.4：MockUart ---
    cout << "\n" << string(60, '=') << "\n";
    cout << "演示：MockUart\n";
    cout << string(60, '=') << "\n";

    MockUart uart;
    uart.print("Hello Embedded!\n");

    uart.simulateRx('Z');
    if (uart.available()) {
        cout << "\n[UART RX] 收到：" << (char)uart.readByte() << "\n";
    }

    part3_traps();
    part4_cards();
    part5_exercises();

    return 0;
}
