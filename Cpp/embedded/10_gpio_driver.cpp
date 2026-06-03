// ============================================================
// 嵌入式 GPIO 驱动 - 超详细注释版
// 日期：2026-06-03（第59天）
// 难度：⭐⭐ 中级
// 编译：g++ -std=c++17 -Wall -pthread -o 10_gpio 10_gpio_driver.cpp
// ============================================================

/*
【核心概念】

是什么？
- GPIO（General Purpose Input/Output）：通用输入输出引脚
- 每个引脚可独立配置为：输入、输出、复用功能（UART/SPI/I2C）

为什么重要？
- 几乎所有外设连接都从 GPIO 开始：LED、按键、传感器使能、片选信号
- GPIO 是嵌入式开发的"基础设施"

核心操作：
- 方向配置（输入/输出）
- 输出模式：推挽（Push-Pull）/ 开漏（Open-Drain）
- 输入模式：上拉/下拉/浮空
- 外部中断：边沿触发（上升沿/下降沿/双边沿）
*/

#include <iostream>
#include <functional>
#include <cstdint>
#include <atomic>
#include <thread>
#include <chrono>
using namespace std;
using namespace chrono;


// ============================================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================================

void part1_concepts() {
    cout << string(60, '=') << "\n";
    cout << "第一部分：GPIO 核心概念\n";
    cout << string(60, '=') << "\n";

    cout << R"(
【GPIO 输出模式】

推挽输出（Push-Pull）：
  - 输出 HIGH：P-MOS 导通，引脚强制拉到 VCC（3.3V）
  - 输出 LOW ：N-MOS 导通，引脚强制拉到 GND
  - 特点：驱动能力强，直接驱动 LED
  - 典型场景：LED、数字信号输出

开漏输出（Open-Drain）：
  - 输出 HIGH：MOS 关断，引脚"悬空"（需外部上拉电阻）
  - 输出 LOW ：N-MOS 导通，引脚拉到 GND
  - 特点：多个设备可共享一根线（线与逻辑）
  - 典型场景：I2C 总线、电平转换

【GPIO 输入模式】

上拉输入（Pull-Up）：
  - 内部接一个上拉电阻到 VCC
  - 引脚悬空时读到 HIGH；外部按键按下拉低读到 LOW
  - 典型场景：按键（低电平有效）

下拉输入（Pull-Down）：
  - 内部接一个下拉电阻到 GND
  - 引脚悬空时读到 LOW；外部信号拉高读到 HIGH
  - 典型场景：信号检测（高电平有效）

浮空输入（Floating）：
  - 无内部上下拉，引脚悬空时值不确定
  - 必须有外部确定电平，否则读到随机值
  - 典型场景：ADC 输入（不允许有上下拉干扰）

【外部中断触发方式】
  上升沿（Rising） ：LOW→HIGH 时触发（如按键松开）
  下降沿（Falling）：HIGH→LOW 时触发（如按键按下，低电平有效）
  双边沿（Both）   ：任何电平变化都触发

)";
}


// ============================================================
// 第二部分：GPIO 驱动实现
// ============================================================

// ------------------------------------------------------------
// 2.1 枚举定义
// ------------------------------------------------------------

enum class GpioDir    { Input, Output };
enum class GpioLevel  { Low = 0, High = 1 };
enum class GpioMode   { PushPull, OpenDrain };   // 输出模式
enum class GpioPull   { None, Up, Down };         // 输入上下拉
enum class EdgeType   { Rising, Falling, Both };  // 中断触发边沿


// ------------------------------------------------------------
// 2.2 GPIO 抽象接口（扩展版，含中断）
// ------------------------------------------------------------

class IGpio {
public:
    virtual ~IGpio() = default;

    // 配置为输出
    virtual void configOutput(GpioMode mode = GpioMode::PushPull) = 0;

    // 配置为输入
    virtual void configInput(GpioPull pull = GpioPull::Up) = 0;

    // 写电平（输出模式）
    virtual void write(GpioLevel level) = 0;

    // 读电平（输入/输出均可）
    virtual GpioLevel read() const = 0;

    // 切换电平
    void toggle() { write(read() == GpioLevel::High ? GpioLevel::Low : GpioLevel::High); }

    // 注册外部中断回调（边沿触发）
    // callback：中断发生时调用，参数为当前电平
    virtual void attachInterrupt(EdgeType edge, function<void(GpioLevel)> callback) = 0;

    // 注销中断
    virtual void detachInterrupt() = 0;
};


// ------------------------------------------------------------
// 2.3 MockGpio：PC 端模拟实现
// ------------------------------------------------------------

class MockGpio : public IGpio {
public:
    explicit MockGpio(const char* name) : name_(name) {}

    void configOutput(GpioMode mode) override {
        dir_  = GpioDir::Output;
        mode_ = mode;
        cout << "[GPIO:" << name_ << "] 配置为输出/"
             << (mode == GpioMode::PushPull ? "推挽" : "开漏") << "\n";
    }

    void configInput(GpioPull pull) override {
        dir_  = GpioDir::Input;
        pull_ = pull;
        // 上拉输入：默认电平 HIGH；下拉/浮空：默认 LOW
        level_ = (pull == GpioPull::Up) ? GpioLevel::High : GpioLevel::Low;
        cout << "[GPIO:" << name_ << "] 配置为输入/"
             << (pull == GpioPull::Up ? "上拉" : pull == GpioPull::Down ? "下拉" : "浮空")
             << " 默认电平=" << (level_ == GpioLevel::High ? "H" : "L") << "\n";
    }

    void write(GpioLevel level) override {
        level_ = level;
        // 开漏输出 HIGH 时实际是高阻，需外部上拉，这里仅模拟逻辑
        cout << "[GPIO:" << name_ << "] 写 " << (level == GpioLevel::High ? "H" : "L") << "\n";
    }

    GpioLevel read() const override { return level_; }

    void attachInterrupt(EdgeType edge, function<void(GpioLevel)> cb) override {
        edge_type_ = edge;
        callback_  = cb;
        irq_enabled_ = true;
        cout << "[GPIO:" << name_ << "] 注册中断，触发方式="
             << (edge == EdgeType::Rising ? "上升沿" :
                 edge == EdgeType::Falling ? "下降沿" : "双边沿") << "\n";
    }

    void detachInterrupt() override {
        irq_enabled_ = false;
        callback_    = nullptr;
    }

    // 测试辅助：模拟外部电平变化（触发中断）
    void simulateEdge(GpioLevel new_level) {
        GpioLevel old = level_;
        level_ = new_level;

        if (!irq_enabled_ || !callback_) return;

        bool rising  = (old == GpioLevel::Low  && new_level == GpioLevel::High);
        bool falling = (old == GpioLevel::High && new_level == GpioLevel::Low);

        bool triggered =
            (edge_type_ == EdgeType::Rising  && rising)  ||
            (edge_type_ == EdgeType::Falling && falling) ||
            (edge_type_ == EdgeType::Both    && (rising || falling));

        if (triggered) {
            cout << "[GPIO:" << name_ << "] 中断触发！电平="
                 << (new_level == GpioLevel::High ? "H" : "L") << "\n";
            callback_(new_level);
        }
    }

private:
    const char*            name_;
    GpioDir                dir_     = GpioDir::Input;
    GpioMode               mode_    = GpioMode::PushPull;
    GpioPull               pull_    = GpioPull::Up;
    GpioLevel              level_   = GpioLevel::High;
    bool                   irq_enabled_ = false;
    EdgeType               edge_type_   = EdgeType::Falling;
    function<void(GpioLevel)> callback_;
};


// ------------------------------------------------------------
// 2.4 LED 驱动（基于 IGpio）
// ------------------------------------------------------------

class LedDriver {
public:
    explicit LedDriver(IGpio& gpio, const char* name)
        : gpio_(gpio), name_(name) {
        gpio_.configOutput(GpioMode::PushPull);
        gpio_.write(GpioLevel::Low);  // 初始熄灭
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


// ------------------------------------------------------------
// 2.5 按键驱动（带软件防抖 + 中断）
// ------------------------------------------------------------

class ButtonDriver {
public:
    // gpio：按键引脚（低电平有效，配上拉输入）
    // on_press：按下回调（防抖后触发）
    ButtonDriver(IGpio& gpio, function<void()> on_press, uint32_t debounce_ms = 20)
        : gpio_(gpio), on_press_(on_press), debounce_ms_(debounce_ms) {

        gpio_.configInput(GpioPull::Up);

        // 注册下降沿中断（按键按下拉低）
        gpio_.attachInterrupt(EdgeType::Falling, [this](GpioLevel) {
            // ISR：仅记录"疑似按下"时间，主循环做防抖确认
            pending_press_tick_ = press_tick_counter_.load(memory_order_relaxed);
            has_pending_.store(true, memory_order_release);
        });
    }

    // 主循环调用：传入当前 tick（ms），执行防抖确认
    void update(uint32_t tick_ms) {
        press_tick_counter_.store(tick_ms, memory_order_relaxed);

        if (!has_pending_.load(memory_order_acquire)) return;

        // 防抖：距中断发生超过 debounce_ms，且引脚仍为低电平（确认按下）
        if (tick_ms - pending_press_tick_ >= debounce_ms_
            && gpio_.read() == GpioLevel::Low) {
            has_pending_.store(false, memory_order_release);
            if (on_press_) on_press_();
        }
        // 抖动：电平已回到高（虚假触发），丢弃
        else if (gpio_.read() == GpioLevel::High) {
            has_pending_.store(false, memory_order_release);
        }
    }

private:
    IGpio&            gpio_;
    function<void()>  on_press_;
    uint32_t          debounce_ms_;
    atomic<uint32_t>  press_tick_counter_{0};
    uint32_t          pending_press_tick_ = 0;
    atomic<bool>      has_pending_{false};
};


// ------------------------------------------------------------
// 2.6 完整演示：LED + 按键 + 外部中断
// ------------------------------------------------------------

void demo_gpio() {
    cout << string(60, '=') << "\n";
    cout << "演示：LED 驱动 + 按键中断\n";
    cout << string(60, '=') << "\n";

    MockGpio led_gpio("PC13");
    MockGpio btn_gpio("PA0");

    LedDriver led(led_gpio, "状态灯");

    int press_count = 0;
    ButtonDriver btn(btn_gpio, [&]() {
        ++press_count;
        led.toggle();
        cout << "→ 按键有效触发！LED=" << (led.isOn() ? "亮" : "灭")
             << " 累计=" << press_count << "\n";
    });

    // 模拟主循环 + 外部按键事件
    auto simulate = [&](uint32_t tick, bool edge_low) {
        if (edge_low) btn_gpio.simulateEdge(GpioLevel::Low);   // 按下
        btn.update(tick);
    };

    cout << "\n--- 模拟时序 ---\n";
    simulate(0,   true);   // t=0:  下降沿（按下）
    simulate(5,   false);  // t=5:  防抖中
    simulate(15,  false);  // t=15: 防抖中
    simulate(25,  false);  // t=25: 防抖完成，确认按下
    btn_gpio.simulateEdge(GpioLevel::High);  // 松开
    simulate(50,  true);   // t=50: 第二次按下
    simulate(75,  false);  // t=75: 防抖完成
}


// ============================================================
// 🔍 深入理解：GPIO 寄存器操作（可跳过）
// ============================================================

/*
STM32 GPIO 寄存器（以 GPIOA 为例，基地址 0x40020000）：

MODER  (0x00)：模式寄存器，每引脚2位（00输入/01输出/10复用/11模拟）
OTYPER (0x04)：输出类型（0推挽/1开漏），每引脚1位
OSPEEDR(0x08)：输出速度（00低/01中/10高/11极高）
PUPDR  (0x0C)：上下拉（00无/01上拉/10下拉）
IDR    (0x10)：输入数据寄存器（只读）
ODR    (0x14)：输出数据寄存器
BSRR   (0x18)：位操作寄存器（高16位清零/低16位置1，原子操作）

用 BSRR 设置/清除引脚（原子操作，比直接写 ODR 更安全）：
  GPIOA->BSRR = (1 << 5);        // 置位 PA5（HIGH）
  GPIOA->BSRR = (1 << (5 + 16)); // 清位 PA5（LOW）

外部中断配置（STM32 HAL）：
  GPIO_InitTypeDef init = {
      .Pin  = GPIO_PIN_0,
      .Mode = GPIO_MODE_IT_FALLING,  // 下降沿中断
      .Pull = GPIO_PULLUP,
  };
  HAL_GPIO_Init(GPIOA, &init);
  HAL_NVIC_SetPriority(EXTI0_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);
  // ISR：void EXTI0_IRQHandler() { HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0); }
*/


// ============================================================
// 第三部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================================

void part3_traps() {
    cout << string(60, '=') << "\n";
    cout << "第三部分：常见错误和陷阱\n";
    cout << string(60, '=') << "\n";

    cout << R"(
1. 输入引脚不配上下拉，悬空读到随机值
──────────────────────────────────────
❌ gpio.configInput(GpioPull::None);  // 浮空
   while(true) { if(gpio.read() == High) do_something(); }
   // 未连接任何信号时，读到的值随环境干扰随机变化

✅ 按键（低有效）：configInput(GpioPull::Up)   // 未按下时稳定 HIGH
   信号（高有效）：configInput(GpioPull::Down) // 无信号时稳定 LOW
   ADC：configInput(GpioPull::None) + 配置为模拟模式（不能有上下拉）

2. 在 ISR 中做防抖（错误）
──────────────────────────────────────
❌ void EXTI_ISR() {
       HAL_Delay(20);          // ISR 中 delay！HAL_Delay 依赖 SysTick，死锁！
       if(gpio.read() == Low) process();
   }

✅ ISR 只记录时间戳：pending_tick = systick;
   主循环 update() 检查：当前 tick - pending_tick >= 20ms 才确认

3. 直接读写 ODR 不是原子操作（多中断场景）
──────────────────────────────────────
❌ GPIOA->ODR |= (1 << 5);   // 读-改-写，三步操作，中断可能打断
   // 如果中断也修改 GPIOA->ODR，可能导致修改丢失

✅ GPIOA->BSRR = (1 << 5);           // 置1：单条写指令，原子
   GPIOA->BSRR = (1 << (5 + 16));   // 清0：单条写指令，原子

4. 开漏输出忘记外部上拉电阻
──────────────────────────────────────
❌ 配置开漏输出，输出 HIGH，用示波器测引脚
   → 电平缓慢上升（RC 充电），边沿不干净

✅ 开漏输出必须配外部上拉电阻（如 4.7kΩ 到 VCC）
   I2C 总线标准：SCL/SDA 均为开漏 + 4.7kΩ 上拉

5. GPIO 时钟未使能
──────────────────────────────────────
❌ GPIO_InitTypeDef init = {...};
   HAL_GPIO_Init(GPIOA, &init);  // 配置无效！GPIOA 时钟未开启

✅ __HAL_RCC_GPIOA_CLK_ENABLE();   // 先使能时钟
   HAL_GPIO_Init(GPIOA, &init);   // 再配置 GPIO
   // STM32 所有外设使用前必须先使能对应时钟

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
【IGpio 接口（本文扩展版）】
  configOutput(GpioMode)                    — 配置为输出（推挽/开漏）
  configInput(GpioPull)                     — 配置为输入（上拉/下拉/浮空）
  write(GpioLevel::High / Low)              — 写电平
  read() → GpioLevel                        — 读电平
  toggle()                                  — 切换电平
  attachInterrupt(EdgeType, callback)       — 注册中断回调
  detachInterrupt()                         — 注销中断

【MockGpio 扩展方法】
  simulateEdge(GpioLevel)                   — 模拟外部电平变化（触发中断）

【ButtonDriver】
  ButtonDriver(gpio, on_press, debounce_ms) — 构造，注册下降沿中断
  update(tick_ms)                           — 主循环调用，执行防抖确认

【STM32 HAL 常用 GPIO API】
  __HAL_RCC_GPIOx_CLK_ENABLE()              — 使能 GPIO 时钟（必须先调）
  HAL_GPIO_Init(GPIOx, &init)               — 初始化 GPIO
  HAL_GPIO_WritePin(GPIOx, Pin, State)      — 写引脚
  HAL_GPIO_ReadPin(GPIOx, Pin) → State      — 读引脚
  HAL_GPIO_TogglePin(GPIOx, Pin)            — 切换引脚

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
1. 实现流水灯控制器
   要求：
   - 4个 MockGpio 模拟 4颗 LED
   - LedController 类：start(interval_ms) 按顺序循环点亮
   - 每次只亮一颗，其余熄灭
   提示：用数组存 LedDriver，index = tick / interval % 4

2. 实现 GPIO 寄存器位操作封装
   要求：
   - template<uint32_t Addr, uint8_t Pin> class GpioReg
   - set()：对应 BSRR 置1
   - clear()：对应 BSRR 清0
   - read()：读 IDR 对应位
   提示：用 volatile uint32_t* 指向寄存器地址，位操作即可

3. 为 ButtonDriver 添加长按检测
   要求：
   - 按住超过 1000ms 触发 on_long_press 回调
   - 短按（< 1000ms）触发 on_press 回调
   - 同一次按下只触发一种回调
   提示：记录按下时间，update() 中判断是否超过 1000ms

4. 实现 GpioExpander（GPIO 扩展器模拟）
   要求：
   - 通过 SPI/I2C 控制的芯片（如 MCP23017）提供额外16个 GPIO
   - class GpioExpander : public IGpio，接受 ISpi& 接口
   - 内部通过 SPI 写寄存器实现 write/read
   提示：维护本地"影子寄存器"缓存，write 时更新缓存并通过 SPI 发送

)";
}


int main() {
    part1_concepts();
    demo_gpio();
    part3_traps();
    part4_cards();
    part5_exercises();
    return 0;
}
