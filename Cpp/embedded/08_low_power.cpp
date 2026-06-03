// ============================================================
// 嵌入式低功耗编程 - 超详细注释版
// 日期：2026-06-03（第58天）
// 难度：⭐⭐ 中级
// 编译：g++ -std=c++17 -Wall -pthread -o 08_lp 08_low_power.cpp
// ============================================================

/*
【核心概念】

是什么？
- 低功耗模式：让 CPU 和外设在空闲时停止运行，降低电流消耗
- 典型场景：电池供电设备（IoT传感器、可穿戴）

为什么重要？
- 活跃模式：~50mA；深度睡眠：~5μA → 差距 10000 倍
- 纽扣电池（225mAh）：全速运行 4.5小时 vs 睡眠唤醒 5年+

核心特点：
- 三种主要模式：Sleep / Stop / Standby（越深越省电，唤醒越慢）
- WFI 指令：Wait For Interrupt，CPU 停止执行直到中断唤醒
*/

#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>
#include <functional>
#include <cstdint>
using namespace std;
using namespace chrono;

// ============================================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================================

void part1_concepts() {
    cout << string(60, '=') << "\n";
    cout << "第一部分：低功耗模式概念\n";
    cout << string(60, '=') << "\n";

    cout << R"(
【STM32 三种低功耗模式对比】

模式       电流    唤醒时间   保留内容          唤醒源
──────────────────────────────────────────────────────────
Sleep      1mA    ~1μs      RAM/寄存器全保留  任意中断
Stop       50μA   ~5μs      RAM/部分寄存器   外部中断/RTC
Standby    5μA    ~50μs     仅备份寄存器     WKUP引脚/RTC

【WFI 指令（Wait For Interrupt）】
  汇编：WFI
  效果：CPU 立即停止执行，进入低功耗状态
  唤醒：任意中断发生时，CPU 从 WFI 下一条指令继续执行

【低功耗编程策略】
  1. 最小化活跃时间：事件驱动，不用轮询
  2. 关闭不用的外设时钟（GPIO/ADC/UART 各有时钟门控）
  3. 降低 CPU 频率（不需要高速时用低速）
  4. 用 RTC 定时唤醒（而非定时器中断保持 CPU 运行）
  5. 数据批处理（攒够再发送，减少无线收发次数）

【典型工作模式：事件驱动 + 睡眠】
  while(true) {
      process_pending_events();   // 处理所有待处理事件
      if (no_pending_events()) {
          WFI();                  // 无事可做，睡眠等中断
      }
  }

)";
}

// ============================================================
// 第二部分：基础操作实践（PC 端模拟）
// ============================================================

// ------------------------------------------------------------
// 2.1 事件驱动主循环（模拟 WFI 等待）
// ------------------------------------------------------------

// 模拟系统滴答（毫秒）
atomic<uint32_t> g_tick_ms{0};
// 模拟待处理事件标志
atomic<bool> g_uart_event{false};
atomic<bool> g_timer_event{false};
atomic<bool> g_button_event{false};
atomic<bool> g_stop_flag{false};    // 控制演示停止

// 模拟"硬件事件"产生线程（代替真实中断）
void hw_event_simulator() {
    for (int i = 0; i < 6 && !g_stop_flag.load(); ++i) {
        this_thread::sleep_for(milliseconds(30));
        g_tick_ms.fetch_add(30, memory_order_relaxed);

        // 模拟不同中断在不同时刻触发
        if (i % 3 == 0) g_uart_event.store(true, memory_order_release);
        if (i % 2 == 0) g_timer_event.store(true, memory_order_release);
        if (i == 4)     g_button_event.store(true, memory_order_release);
    }
    g_stop_flag.store(true, memory_order_release);
}

// 模拟 WFI：无事件时"休眠"（PC 上用 sleep 模拟）
void wfi_sleep(uint32_t max_ms = 100) {
    // 真实嵌入式：执行 WFI 汇编指令，CPU 停止
    // PC 模拟：短暂 sleep，期间其他线程可触发"中断"
    this_thread::sleep_for(milliseconds(max_ms / 10));
}

// 检查是否有任何待处理事件
bool has_pending_events() {
    return g_uart_event.load(memory_order_acquire)
        || g_timer_event.load(memory_order_acquire)
        || g_button_event.load(memory_order_acquire);
}

void demo_event_driven_loop() {
    cout << string(60, '-') << "\n";
    cout << "2.1 事件驱动主循环（WFI 模拟）\n";
    cout << string(60, '-') << "\n";

    thread hw_sim(hw_event_simulator);

    int loop_count = 0;
    int sleep_count = 0;

    while (!g_stop_flag.load(memory_order_acquire) && loop_count < 20) {
        ++loop_count;

        // 处理所有待处理事件
        if (g_uart_event.exchange(false, memory_order_acq_rel)) {
            cout << "[t=" << g_tick_ms.load() << "ms] 处理 UART 事件\n";
        }
        if (g_timer_event.exchange(false, memory_order_acq_rel)) {
            cout << "[t=" << g_tick_ms.load() << "ms] 处理定时器事件\n";
        }
        if (g_button_event.exchange(false, memory_order_acq_rel)) {
            cout << "[t=" << g_tick_ms.load() << "ms] 处理按键事件\n";
        }

        // 无事件：进入睡眠（WFI）
        if (!has_pending_events() && !g_stop_flag.load()) {
            ++sleep_count;
            wfi_sleep();
        }
    }

    hw_sim.join();
    cout << "主循环共执行 " << loop_count << " 次，睡眠 " << sleep_count << " 次\n";
}

// ------------------------------------------------------------
// 2.2 低功耗状态机：管理不同功耗模式的切换
// ------------------------------------------------------------

enum class PowerMode {
    Active,    // 活跃：CPU 全速，所有外设开启
    Sleep,     // 睡眠：CPU 停止，外设时钟保持
    Stop,      // 停止：CPU+大部分外设停止，RAM 保持
    Standby    // 待机：几乎全关，只有备份域
};

const char* mode_name(PowerMode m) {
    switch (m) {
        case PowerMode::Active:  return "Active";
        case PowerMode::Sleep:   return "Sleep";
        case PowerMode::Stop:    return "Stop";
        case PowerMode::Standby: return "Standby";
    }
    return "?";
}

// 各模式"模拟电流"（μA）
uint32_t mode_current_ua(PowerMode m) {
    switch (m) {
        case PowerMode::Active:  return 50000;  // 50mA
        case PowerMode::Sleep:   return 1000;   // 1mA
        case PowerMode::Stop:    return 50;     // 50μA
        case PowerMode::Standby: return 5;      // 5μA
    }
    return 0;
}

class PowerManager {
public:
    PowerMode mode() const { return mode_; }

    // 进入睡眠（有外设中断可唤醒）
    void enterSleep() {
        if (mode_ == PowerMode::Active) transition(PowerMode::Sleep);
    }

    // 进入停止模式（深度省电，只有外部中断或RTC唤醒）
    void enterStop() {
        transition(PowerMode::Stop);
    }

    // 唤醒（任何中断事件触发）
    void wakeup(const char* reason) {
        if (mode_ != PowerMode::Active) {
            cout << "[PowerMgr] 唤醒，原因：" << reason << "\n";
            transition(PowerMode::Active);
        }
    }

    // 记录在某模式下运行了 ms 毫秒，累计电量消耗（nAh）
    void recordTime(uint32_t ms) {
        // 电量 = 电流(μA) × 时间(ms) / 3600000 → nAh
        total_charge_nah_ += (uint64_t)mode_current_ua(mode_) * ms / 3600;
    }

    void printStats() const {
        cout << "累计消耗电量：" << total_charge_nah_ << " nAh"
             << "（≈" << total_charge_nah_ / 1000 << " μAh）\n";
    }

private:
    PowerMode mode_ = PowerMode::Active;
    uint64_t  total_charge_nah_ = 0;

    void transition(PowerMode next) {
        cout << "[PowerMgr] " << mode_name(mode_)
             << " → " << mode_name(next)
             << "（" << mode_current_ua(next) << " μA）\n";
        mode_ = next;
    }
};

void demo_power_manager() {
    cout << string(60, '-') << "\n";
    cout << "2.2 低功耗状态机演示\n";
    cout << string(60, '-') << "\n";

    PowerManager pm;

    // 模拟典型 IoT 传感器工作周期（每10秒采集一次）
    struct Scenario { const char* desc; PowerMode mode; uint32_t duration_ms; };
    Scenario scenarios[] = {
        {"初始化外设",      PowerMode::Active,  50},    // 50ms 初始化
        {"进入睡眠等待",    PowerMode::Sleep,   0},     // 睡眠（由enterSleep设置）
        {"定时器唤醒",      PowerMode::Active,  0},     // 由wakeup恢复
        {"ADC采样",         PowerMode::Active,  10},    // 10ms 采样
        {"数据处理",        PowerMode::Active,  5},     // 5ms 处理
        {"进入停止模式",    PowerMode::Stop,    0},
        {"RTC 10秒后唤醒",  PowerMode::Active,  0},
        {"无线发送数据",    PowerMode::Active,  20},    // 20ms 发送
        {"再次进入停止",    PowerMode::Stop,    0},
    };

    pm.recordTime(50);   // Active 50ms
    pm.enterSleep();
    pm.recordTime(9935); // Sleep ~10s
    pm.wakeup("定时器中断");
    pm.recordTime(15);   // Active 15ms（采样+处理）
    pm.enterStop();
    pm.recordTime(9950); // Stop ~10s
    pm.wakeup("RTC闹钟");
    pm.recordTime(20);   // Active 20ms（发送）
    pm.enterStop();

    pm.printStats();

    // 对比：如果全程 Active
    uint64_t always_active = (uint64_t)50000 * (50 + 9935 + 15 + 9950 + 20) / 3600;
    cout << "对比全程 Active：" << always_active << " nAh\n";
    cout << "节省比例：" << (1.0 - (double)10 / always_active) * 100 << "%+\n";

    (void)scenarios;
}

// ============================================================
// 🔍 深入理解：WFI vs WFE（可跳过）
// ============================================================

/*
WFI（Wait For Interrupt）：
  - 等待任意中断（包括未屏蔽中断）
  - 中断发生 → CPU 执行 ISR → 从 WFI 后继续
  - 最常用

WFE（Wait For Event）：
  - 等待事件（EVENT 信号或中断）
  - 多核系统中一个核发 SEV（Send Event），另一个核的 WFE 唤醒
  - 单核系统中不常用

低功耗典型代码（ARM Cortex-M）：
  while (1) {
      process_events();
      __WFI();   // CMSIS 宏，展开为 WFI 汇编指令
  }

时钟门控（Clock Gating）——降低动态功耗：
  RCC->APB1ENR &= ~RCC_APB1ENR_USART2EN;  // 关闭 UART2 时钟
  // 不使用的外设关闭时钟，即使 CPU 运行也不消耗动态功耗
*/

// ============================================================
// 第三部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================================

void part3_traps() {
    cout << string(60, '=') << "\n";
    cout << "第三部分：常见错误和陷阱\n";
    cout << string(60, '=') << "\n";

    cout << R"(
1. WFI 前未清除中断标志，立刻被唤醒
──────────────────────────────────────
❌ // UART 中断标志未清除
   __WFI();  // 立刻被唤醒，进入死循环
   // 原因：中断标志还在，CPU 刚进 WFI 就立刻唤醒

✅ // 处理事件时清除中断标志
   if (uart_flag) { uart_flag = false; process_uart(); }
   if (!any_event()) __WFI();  // 确认无未处理事件再睡眠

2. Stop 模式后外设时钟需重新初始化
──────────────────────────────────────
❌ 从 Stop 模式唤醒后直接使用 UART 发数据
   → UART 时钟未恢复，数据乱码或无输出

✅ 从 Stop 唤醒后，先恢复系统时钟（SystemClock_Config()）
   再使用任何外设

3. 在 ISR 中执行耗时操作延误低功耗
──────────────────────────────────────
❌ void RTC_IRQHandler() { send_data_via_wifi(); }
   // Wi-Fi 发送耗时几十毫秒，ISR 期间系统不能再睡眠

✅ void RTC_IRQHandler() { g_rtc_wakeup = true; }
   // 主循环处理：wakeup → send → 再次 Stop

4. 忘记关闭调试接口（SWD/JTAG）耗电
──────────────────────────────────────
❌ 进入 Standby 前未关闭 SWD 调试口
   → 调试接口保持供电，漏电流 ~100μA

✅ 量产固件关闭不用的调试接口
   调试阶段可保留，量产阶段用编译宏关闭

5. 轮询替代事件驱动
──────────────────────────────────────
❌ while(1) { if(sensor_ready()) read_sensor(); }
   // CPU 100% 占用等待传感器，无法进入任何低功耗模式

✅ 配置传感器就绪引脚为外部中断
   ISR 设置标志 → 主循环处理后 WFI 睡眠

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
【WFI / WFE（ARM CMSIS）】
  __WFI()   — 等待中断，CPU 停止，有中断即唤醒
  __WFE()   — 等待事件（多核/SEV配合）
  __SEV()   — 发送事件（唤醒其他核的 WFE）
  用法：while(1) { process(); if(!events) __WFI(); }

【STM32 HAL 低功耗 API】
  HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI)
  HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI)
  HAL_PWR_EnterSTANDBYMode()
  HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1)  — 配置唤醒引脚

【时钟门控（RCC）】
  __HAL_RCC_USART2_CLK_DISABLE()   — 关闭 UART2 时钟
  __HAL_RCC_GPIOA_CLK_DISABLE()    — 关闭 GPIOA 时钟
  __HAL_RCC_USART2_CLK_ENABLE()    — 重新开启

【PowerManager（本文实现）】
  enterSleep()              — Active → Sleep
  enterStop()               — → Stop
  wakeup(reason)            — 任意模式 → Active
  recordTime(ms)            — 累计电量
  printStats()              — 打印统计

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
1. 实现电池寿命估算器
   要求：
   - 输入：电池容量（mAh）、各模式时间占比
   - 计算：平均电流 = Σ(模式电流 × 时间占比)
   - 输出：估算寿命（小时/天）
   提示：life_hours = capacity_mah / avg_current_ma

2. 为 PowerManager 添加功耗预算检查
   要求：
   - 设置每小时最大允许消耗（如 50μAh）
   - recordTime() 累计时检查是否超预算
   - 超预算时强制进入 Stop 模式并打印告警
   提示：将 total_charge_nah_ 转换为 μAh 与预算比较

3. 实现自适应睡眠深度
   要求：
   - 活动频繁（<1秒内有事件）→ 只进 Sleep
   - 活动稀少（>5秒无事件）→ 进 Stop
   - 极少活动（>30秒无事件）→ 进 Standby
   提示：记录上次事件时间戳，与当前时间比较决定模式

4. 模拟 IoT 设备完整工作周期
   要求：
   - 每10秒：唤醒 → 采集温湿度 → 压缩数据 → BLE发送 → 睡眠
   - 记录每个步骤的时间和电量
   - 估算 2000mAh 电池的寿命
   提示：用 chrono 计时每步骤，PowerManager.recordTime() 累计

)";
}

int main() {
    part1_concepts();
    demo_event_driven_loop();
    cout << "\n";
    demo_power_manager();
    part3_traps();
    part4_cards();
    part5_exercises();
    return 0;
}
