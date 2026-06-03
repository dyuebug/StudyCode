// ============================================
// 嵌入式 C/C++ 编程特点 - 超详细注释版
// 日期：2026-06-03（第56天）
// 难度：⭐⭐ 入门
// ============================================

/*
【核心概念】嵌入式 C/C++ 编程特点

是什么？
- 嵌入式编程是在资源受限的硬件上编写程序
- 需要直接与硬件交互（寄存器操作）
- 对代码大小、执行速度、内存使用有严格要求

为什么需要了解这些特点？
- 嵌入式编程与桌面编程思维方式不同
- 很多桌面编程的"好习惯"在嵌入式中可能是"坏习惯"
- 理解这些特点才能写出高质量的嵌入式代码

核心特点：
- 资源受限（内存、Flash、CPU 速度）
- 实时性要求（中断响应时间）
- 硬件相关性（寄存器操作）
- 可靠性要求（不能崩溃重启）
*/

#include <iostream>
#include <cstdint>
#include <cstring>  // memset、memcpy
using namespace std;

// ============================================
// 第一部分：嵌入式 C++ 特性取舍
// ============================================

/*
────────────────────────────────────────────
1.1 哪些 C++ 特性在嵌入式中要谨慎使用？
────────────────────────────────────────────

【可以使用的 C++ 特性】✅
- 类和对象（封装硬件驱动）
- 模板（零开销抽象）
- 内联函数（替代宏）
- constexpr（编译期计算）
- 引用（避免拷贝）
- 命名空间（避免命名冲突）
- enum class（类型安全的枚举）

【需要谨慎使用的 C++ 特性】⚠️
- 虚函数（有虚函数表开销，但可接受）
- 模板（可能增加代码大小）
- 内联函数（可能增加代码大小）

【应该避免的 C++ 特性】❌
- 动态内存分配（new/delete/malloc/free）
  原因：内存碎片化、不确定性、可能失败
- 异常处理（try/catch/throw）
  原因：增加代码大小，执行时间不确定
- STL 容器（vector、map、string 等）
  原因：依赖动态内存分配
- RTTI（运行时类型信息）
  原因：增加代码大小
- 全局对象的构造函数（复杂初始化）
  原因：启动时间不确定
*/

// ============================================
// 第二部分：嵌入式编程核心技巧
// ============================================

// --------------------------------------------
// 2.1 使用 constexpr 替代宏定义
// --------------------------------------------

/*
【为什么用 constexpr 替代宏？】

❌ 宏定义的问题：
- 没有类型检查
- 调试困难
- 可能产生意外的副作用

✅ constexpr 的优点：
- 有类型检查
- 调试友好
- 编译期计算，零运行时开销
*/

// ❌ 旧式宏定义（不推荐）
#define LED_PIN_OLD    5
#define CPU_FREQ_OLD   72000000

// ✅ 现代 constexpr（推荐）
constexpr uint8_t  LED_PIN    = 5;           // LED 连接的引脚号
constexpr uint32_t CPU_FREQ   = 72000000;    // CPU 频率：72MHz
constexpr uint32_t BAUD_RATE  = 115200;      // 串口波特率
constexpr uint16_t ADC_MAX    = 4095;        // 12位ADC最大值
constexpr float    VREF       = 3.3f;        // 参考电压

// constexpr 函数（编译期计算）
constexpr uint32_t calc_baud_divider(uint32_t cpu_freq, uint32_t baud)
{
    return cpu_freq / baud;
}

// 编译期计算波特率分频值
constexpr uint32_t BAUD_DIV = calc_baud_divider(CPU_FREQ, BAUD_RATE);

void demo_constexpr()
{
    cout << "=== constexpr 演示 ===" << endl;
    cout << "LED 引脚：" << (int)LED_PIN << endl;
    cout << "CPU 频率：" << CPU_FREQ << " Hz" << endl;
    cout << "波特率分频值：" << BAUD_DIV << endl;

    // constexpr 在编译期计算，运行时零开销
    constexpr float voltage_per_bit = VREF / ADC_MAX;
    cout << "每个 ADC 位对应电压：" << voltage_per_bit << " V" << endl;
}

// --------------------------------------------
// 2.2 内联函数替代宏（嵌入式常用）
// --------------------------------------------

/*
【内联函数 vs 宏】

❌ 宏的问题：
#define MAX(a, b) ((a) > (b) ? (a) : (b))
// 调用 MAX(x++, y++) 会导致 x 或 y 被递增两次！

✅ 内联函数的优点：
- 有类型检查
- 没有宏的副作用
- 调试友好
- 编译器可以优化
*/

// 内联函数：限制值在范围内
inline uint16_t clamp(uint16_t value, uint16_t min_val, uint16_t max_val)
{
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

// 内联函数：将 ADC 值转换为电压
inline float adc_to_voltage(uint16_t adc_value)
{
    return (adc_value * VREF) / ADC_MAX;
}

// 内联函数：将电压转换为温度（假设线性传感器）
inline float voltage_to_temperature(float voltage)
{
    // 假设：0V = -40°C，3.3V = 125°C
    return (voltage / VREF) * 165.0f - 40.0f;
}

void demo_inline_functions()
{
    cout << "\n=== 内联函数演示 ===" << endl;

    uint16_t raw_adc = 2048;  // ADC 采样值（12位，0-4095）
    float voltage = adc_to_voltage(raw_adc);
    float temperature = voltage_to_temperature(voltage);

    cout << "ADC 原始值：" << raw_adc << endl;
    cout << "对应电压：" << voltage << " V" << endl;
    cout << "对应温度：" << temperature << " °C" << endl;

    // 限制 PWM 占空比在 0-1000 之间
    uint16_t pwm_duty = clamp(1500, 0, 1000);
    cout << "PWM 占空比（限制后）：" << pwm_duty << endl;
}

// --------------------------------------------
// 2.3 使用 enum class 替代普通枚举
// --------------------------------------------

/*
【enum class 的优点】
- 类型安全（不会隐式转换为 int）
- 有作用域（不会污染命名空间）
- 适合嵌入式中的状态机
*/

// ❌ 旧式枚举（不推荐）
enum LED_State_Old { OFF, ON, BLINK };  // 可能与其他枚举冲突

// ✅ 现代 enum class（推荐）
enum class LedState : uint8_t {
    Off   = 0,
    On    = 1,
    Blink = 2
};

enum class GpioMode : uint8_t {
    Input        = 0,
    OutputPushPull = 1,
    OutputOpenDrain = 2,
    AlternateFunction = 3
};

enum class UartParity : uint8_t {
    None = 0,
    Even = 1,
    Odd  = 2
};

void demo_enum_class()
{
    cout << "\n=== enum class 演示 ===" << endl;

    LedState led = LedState::Off;

    // 状态机
    switch (led) {
    case LedState::Off:
        cout << "LED 状态：关闭" << endl;
        led = LedState::On;
        break;
    case LedState::On:
        cout << "LED 状态：开启" << endl;
        break;
    case LedState::Blink:
        cout << "LED 状态：闪烁" << endl;
        break;
    }

    // enum class 需要显式转换为整数
    uint8_t led_value = static_cast<uint8_t>(led);
    cout << "LED 状态值：" << (int)led_value << endl;
}

// --------------------------------------------
// 2.4 嵌入式中的类设计（硬件驱动封装）
// --------------------------------------------

/*
【嵌入式中如何用类封装硬件驱动？】

原则：
1. 类代表一个硬件外设（如 GPIO、UART、SPI）
2. 构造函数初始化硬件
3. 成员函数提供操作接口
4. 避免动态内存分配
5. 使用 constexpr 和 inline 优化性能
*/

// 模拟 GPIO 引脚类（简化版）
class GpioPin {
public:
    // 构造函数：初始化引脚
    // 参数：port - 端口号（0=A, 1=B, 2=C）
    //       pin  - 引脚号（0-15）
    //       mode - 引脚模式
    GpioPin(uint8_t port, uint8_t pin, GpioMode mode)
        : m_port(port)
        , m_pin(pin)
        , m_mode(mode)
        , m_state(false)
    {
        // 在真实嵌入式中，这里会初始化硬件寄存器
        cout << "GPIO 初始化：P" << (char)('A' + port) << (int)pin << endl;
    }

    // 设置引脚为高电平
    inline void set()
    {
        m_state = true;
        // 真实嵌入式：GPIOA->BSRR = (1 << m_pin);
        cout << "P" << (char)('A' + m_port) << (int)m_pin << " = 高电平" << endl;
    }

    // 设置引脚为低电平
    inline void reset()
    {
        m_state = false;
        // 真实嵌入式：GPIOA->BSRR = (1 << (m_pin + 16));
        cout << "P" << (char)('A' + m_port) << (int)m_pin << " = 低电平" << endl;
    }

    // 翻转引脚状态
    inline void toggle()
    {
        m_state = !m_state;
        // 真实嵌入式：GPIOA->ODR ^= (1 << m_pin);
        cout << "P" << (char)('A' + m_port) << (int)m_pin
             << " 翻转 → " << (m_state ? "高" : "低") << "电平" << endl;
    }

    // 读取引脚状态
    inline bool read() const
    {
        // 真实嵌入式：return (GPIOA->IDR >> m_pin) & 1;
        return m_state;
    }

private:
    uint8_t  m_port;    // 端口号
    uint8_t  m_pin;     // 引脚号
    GpioMode m_mode;    // 引脚模式
    bool     m_state;   // 当前状态（模拟）
};

void demo_gpio_class()
{
    cout << "\n=== GPIO 类演示 ===" << endl;

    // 创建 LED 引脚（PA5，输出推挽模式）
    GpioPin led(0, 5, GpioMode::OutputPushPull);

    // 控制 LED
    led.set();      // 点亮 LED
    led.toggle();   // 翻转（熄灭）
    led.toggle();   // 翻转（点亮）
    led.reset();    // 熄灭 LED

    cout << "LED 当前状态：" << (led.read() ? "亮" : "灭") << endl;
}

// --------------------------------------------
// 2.5 嵌入式中的内存优化技巧
// --------------------------------------------

void demo_memory_optimization()
{
    cout << "\n=== 内存优化技巧 ===" << endl;

    // ----------------------------------------
    // 技巧1：使用位域（Bit Fields）节省内存
    // ----------------------------------------
    cout << "--- 位域节省内存 ---" << endl;

    // 不使用位域：每个标志占 1 字节，共 8 字节
    struct FlagsNormal {
        bool flag0;
        bool flag1;
        bool flag2;
        bool flag3;
        bool flag4;
        bool flag5;
        bool flag6;
        bool flag7;
    };

    // 使用位域：8个标志只占 1 字节
    struct FlagsBitField {
        uint8_t flag0 : 1;  // 1位
        uint8_t flag1 : 1;
        uint8_t flag2 : 1;
        uint8_t flag3 : 1;
        uint8_t flag4 : 1;
        uint8_t flag5 : 1;
        uint8_t flag6 : 1;
        uint8_t flag7 : 1;
    };

    cout << "普通结构体大小：" << sizeof(FlagsNormal) << " 字节" << endl;
    cout << "位域结构体大小：" << sizeof(FlagsBitField) << " 字节" << endl;
    cout << "节省：" << sizeof(FlagsNormal) - sizeof(FlagsBitField) << " 字节" << endl;

    // ----------------------------------------
    // 技巧2：使用 __attribute__((packed)) 消除填充
    // ----------------------------------------
    cout << "\n--- 消除结构体填充 ---" << endl;

    struct SensorDataNormal {
        uint8_t  id;          // 1 字节
        // 编译器可能在这里填充 1 字节
        uint16_t temperature; // 2 字节
        uint32_t timestamp;   // 4 字节
    };

    // 使用 packed 属性消除填充（GCC/Clang）
    struct __attribute__((packed)) SensorDataPacked {
        uint8_t  id;          // 1 字节
        uint16_t temperature; // 2 字节（紧接在 id 后面）
        uint32_t timestamp;   // 4 字节
    };

    cout << "普通结构体大小：" << sizeof(SensorDataNormal) << " 字节" << endl;
    cout << "packed 结构体大小：" << sizeof(SensorDataPacked) << " 字节" << endl;

    // ----------------------------------------
    // 技巧3：使用 memset/memcpy 替代循环
    // ----------------------------------------
    cout << "\n--- 使用 memset/memcpy ---" << endl;

    uint8_t buffer[64];

    // 清零缓冲区
    memset(buffer, 0, sizeof(buffer));
    cout << "缓冲区已清零（" << sizeof(buffer) << " 字节）" << endl;

    // 复制数据
    uint8_t source[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    memcpy(buffer, source, sizeof(source));
    cout << "数据已复制（" << sizeof(source) << " 字节）" << endl;
    cout << "buffer[0] = " << (int)buffer[0] << ", buffer[15] = " << (int)buffer[15] << endl;
}

// ============================================
// 第三部分：深入理解（可选）
// ============================================

/*
==============================================
🔍 深入理解：嵌入式中的 C++ 零开销抽象（选学）
==============================================

【什么是零开销抽象？】
C++ 的设计哲学之一：
"你不使用的东西，你不需要为它付出代价"
"你使用的东西，你无法手写出更好的代码"

【嵌入式中的零开销抽象示例】

1. 模板（Templates）
   - 编译期展开，运行时零开销
   - 适合嵌入式中的通用驱动

2. constexpr
   - 编译期计算，运行时零开销
   - 适合常量计算

3. 内联函数
   - 编译期展开，无函数调用开销
   - 适合频繁调用的小函数

4. 引用
   - 编译期优化，无额外内存开销
   - 适合传递大型结构体

【示例：模板实现通用 GPIO 驱动】
template<uint8_t PORT, uint8_t PIN>
class GpioPinTemplate {
public:
    static inline void set() {
        // 编译期确定端口和引脚，生成最优代码
        GPIO_PORT[PORT]->BSRR = (1 << PIN);
    }
    static inline void reset() {
        GPIO_PORT[PORT]->BSRR = (1 << (PIN + 16));
    }
};

// 使用：
using LedPin = GpioPinTemplate<0, 5>;  // PA5
LedPin::set();   // 编译后等同于直接操作寄存器

==============================================
*/

// ============================================
// 第四部分：练习题
// ============================================

/*
练习1：GPIO 驱动扩展
要求：
- 扩展 GpioPin 类，添加以下功能：
  * 读取输入引脚状态
  * 设置引脚为输入模式
  * 添加上拉/下拉电阻配置
- 创建一个按键类（Button），使用 GpioPin 作为成员

提示：
- 按键类需要检测按键按下和释放
- 考虑按键消抖（debounce）

────────────────────────────────────

练习2：传感器数据结构
要求：
- 设计一个传感器数据结构，包含：
  * 传感器 ID（uint8_t）
  * 温度（int16_t，单位 0.1°C）
  * 湿度（uint8_t，单位 1%）
  * 气压（uint16_t，单位 1hPa）
  * 时间戳（uint32_t，单位 ms）
- 使用 packed 属性
- 计算结构体大小
- 编写序列化函数（转换为字节数组）

提示：
- 序列化用于通过串口发送数据
- 使用 memcpy 复制数据

────────────────────────────────────

练习3：状态机实现
要求：
- 使用 enum class 定义 LED 状态机
- 状态：关闭、常亮、慢闪（1Hz）、快闪（5Hz）
- 实现状态转换函数
- 实现状态更新函数（模拟定时器中断）

提示：
- 使用 switch-case 实现状态机
- 使用计数器模拟定时器
*/

// ============================================
// 主函数
// ============================================

int main()
{
    cout << "========================================" << endl;
    cout << "嵌入式 C/C++ 编程特点 - 第56天" << endl;
    cout << "========================================" << endl;

    demo_constexpr();
    demo_inline_functions();
    demo_enum_class();
    demo_gpio_class();
    demo_memory_optimization();

    cout << "\n========================================" << endl;
    cout << "今日学习完成！" << endl;
    cout << "核心要点：" << endl;
    cout << "1. 用 constexpr 替代宏定义" << endl;
    cout << "2. 用内联函数替代宏函数" << endl;
    cout << "3. 用 enum class 替代普通枚举" << endl;
    cout << "4. 用类封装硬件驱动" << endl;
    cout << "5. 使用位域和 packed 节省内存" << endl;
    cout << "========================================" << endl;

    return 0;
}
