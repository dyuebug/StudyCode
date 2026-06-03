// ============================================================
// 嵌入式定时器综合练习 - 超详细注释版
// 日期：2026-06-03（第60天）
// 难度：⭐⭐⭐ 高级
// 编译：g++ -std=c++17 -Wall -pthread -o 15_timer 15_timer_exercise.cpp
// ============================================================

/*
【核心概念】

是什么？
- 软件定时器：基于系统 SysTick 滴答，在应用层实现多路定时
- 周期任务调度器：让多个任务以各自周期精确执行，不互相阻塞

为什么需要？
- 硬件定时器数量有限（STM32 通常8个）
- 用一个硬件定时器（SysTick 1ms）驱动 N 个软件定时器
- 嵌入式替代 RTOS 的轻量方案：超级循环 + 软件定时器

核心设计：
- SysTick ISR 每 1ms 递增全局 tick 计数
- 软件定时器：记录到期 tick，到期时执行回调，重置到期时间
- 调度器：主循环检查所有定时器是否到期
*/

#include <iostream>
#include <functional>
#include <array>
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
    cout << "第一部分：软件定时器与任务调度概念\n";
    cout << string(60, '=') << "\n";

    cout << R"(
【超级循环 + 软件定时器架构】

SysTick ISR（每 1ms）：
  ++g_tick;  // 仅递增计数，不做任何业务

主循环：
  while(true) {
      scheduler.run(g_tick);   // 检查所有定时器，到期则执行回调
      // 可选：__WFI()         // 无到期任务则睡眠
  }

软件定时器状态：
  [创建] → start() → [运行中] → 到期 → callback() → [重置/停止]
                              ↑                         |
                              └─────────────────────────┘ (周期定时器)

【绝对时间调度（vTaskDelayUntil 风格）】
  // 错误方式（累积偏差）：
  while(true) { task(); sleep(100ms); }  // task耗时算入下次间隔，越来越慢

  // 正确方式（固定周期）：
  auto next = now();
  while(true) {
      task();
      next += 100ms;
      sleep_until(next);   // 下次唤醒时间固定，不受 task 耗时影响
  }

【软件定时器 vs 硬件定时器】
  软件定时器：精度受主循环影响（±1 tick），数量无限制，无需配置寄存器
  硬件定时器：精度高（纳秒级），数量有限，配置复杂，适合高精度 PWM/捕获

)";
}


// ============================================================
// 第二部分：软件定时器实现
// ============================================================

// 全局系统 tick（模拟 SysTick，每 1ms +1）
atomic<uint32_t> g_tick_ms{0};

// ------------------------------------------------------------
// 2.1 单个软件定时器
// ------------------------------------------------------------

class SoftTimer {
public:
    using Callback = function<void()>;

    // 创建定时器
    // interval_ms：触发间隔（ms）
    // cb：到期回调
    // repeat：true=周期定时器，false=单次定时器
    SoftTimer(uint32_t interval_ms, Callback cb, bool repeat = true)
        : interval_(interval_ms), cb_(cb), repeat_(repeat) {}

    // 启动定时器：记录下次到期的绝对 tick
    void start(uint32_t current_tick) {
        next_tick_ = current_tick + interval_;
        active_    = true;
    }

    void stop() { active_ = false; }

    bool isActive() const { return active_; }

    // 主循环中调用：检查是否到期，到期则执行回调
    // 返回 true 表示本次触发了回调
    bool update(uint32_t current_tick) {
        if (!active_ || current_tick < next_tick_) return false;

        cb_();  // 执行回调

        if (repeat_) {
            // 周期定时器：基于上次到期时间推进（防止累积误差）
            next_tick_ += interval_;
        } else {
            active_ = false;  // 单次定时器：执行后停止
        }
        return true;
    }

private:
    uint32_t interval_;
    Callback cb_;
    bool     repeat_;
    uint32_t next_tick_ = 0;
    bool     active_    = false;
};


// ------------------------------------------------------------
// 2.2 软件定时器调度器（管理多个定时器）
// ------------------------------------------------------------

template<size_t MAX_TIMERS = 8>
class TimerScheduler {
public:
    // 添加定时器并自动启动
    // 返回定时器 ID（用于后续 stop/restart）
    int add(uint32_t interval_ms, SoftTimer::Callback cb, bool repeat = true) {
        if (count_ >= MAX_TIMERS) return -1;  // 已满
        int id = count_++;
        timers_[id] = SoftTimer(interval_ms, cb, repeat);
        timers_[id].start(g_tick_ms.load());
        return id;
    }

    void stop(int id) {
        if (id >= 0 && (size_t)id < count_) timers_[id].stop();
    }

    // 主循环中调用，驱动所有定时器
    void run() {
        uint32_t now = g_tick_ms.load(memory_order_relaxed);
        for (size_t i = 0; i < count_; ++i)
            timers_[i].update(now);
    }

    size_t activeCount() const {
        size_t n = 0;
        for (size_t i = 0; i < count_; ++i)
            if (timers_[i].isActive()) ++n;
        return n;
    }

private:
    array<SoftTimer, MAX_TIMERS> timers_;
    size_t                       count_ = 0;
};


// ------------------------------------------------------------
// 2.3 周期任务调度器（固定周期，防累积误差）
// ------------------------------------------------------------

// 任务结构体：任务函数 + 执行周期 + 上次执行的绝对时间
struct PeriodicTask {
    const char*              name;
    uint32_t                 period_ms;
    function<void(uint32_t)> fn;      // 参数：当前 tick
    uint32_t                 next_ms = 0;
    bool                     active  = true;
};

class TaskScheduler {
public:
    void addTask(const char* name, uint32_t period_ms,
                 function<void(uint32_t)> fn) {
        uint32_t now = g_tick_ms.load();
        tasks_.push_back({name, period_ms, fn, now + period_ms, true});
    }

    // 主循环调用：执行所有到期任务
    void run() {
        uint32_t now = g_tick_ms.load(memory_order_relaxed);
        for (auto& t : tasks_) {
            if (!t.active || now < t.next_ms) continue;
            t.fn(now);
            // 关键：基于上次到期时间推进，而非当前时间
            // 防止任务执行时间影响下次触发间隔
            t.next_ms += t.period_ms;
            // 如果落后多个周期（如调试暂停），追赶到最近的到期时间
            while (t.next_ms <= now) t.next_ms += t.period_ms;
        }
    }

    void stopTask(const char* name) {
        for (auto& t : tasks_)
            if (t.name == name) t.active = false;
    }

private:
    vector<PeriodicTask> tasks_;
};


// ============================================================
// 第三部分：完整演示
// ============================================================

void demo_timers() {
    cout << string(60, '=') << "\n";
    cout << "演示：软件定时器调度器\n";
    cout << string(60, '=') << "\n";

    // 模拟 SysTick 线程（每 10ms 推进 10 个 tick，共 200ms）
    thread systick([]() {
        for (int i = 0; i < 20; ++i) {
            this_thread::sleep_for(milliseconds(10));
            g_tick_ms.fetch_add(10, memory_order_relaxed);
        }
    });

    // 创建调度器，注册任务
    TaskScheduler scheduler;

    int led_toggle_count = 0;
    scheduler.addTask("LED闪烁", 50, [&](uint32_t tick) {
        ++led_toggle_count;
        cout << "[t=" << tick << "ms] LED 切换（第" << led_toggle_count << "次）\n";
    });

    scheduler.addTask("传感器采样", 100, [](uint32_t tick) {
        cout << "[t=" << tick << "ms] 采集温度传感器\n";
    });

    scheduler.addTask("心跳上报", 200, [](uint32_t tick) {
        cout << "[t=" << tick << "ms] 发送心跳包\n";
    });

    // 主循环（运行 200ms）
    auto end = steady_clock::now() + milliseconds(220);
    while (steady_clock::now() < end) {
        scheduler.run();
        this_thread::sleep_for(milliseconds(1));  // 模拟 WFI
    }

    systick.join();
}


// ============================================================
// 🔍 深入理解：时间轮（Timer Wheel）（可跳过）
// ============================================================

/*
上面的调度器每次 run() 遍历所有任务（O(N)）。
任务很多（>100）时性能下降。

时间轮（Hashed Timing Wheel）优化：
  - 将时间分成 256 个槽（slot），每个槽对应 1ms
  - 定时器注册时：放入 (current + interval) % 256 的槽
  - SysTick 每 1ms：只处理当前槽的定时器（O(1) 期望复杂度）

适用场景：几百个软件定时器，且大多数定时器间隔 < 256ms

简化实现思路：
  array<vector<SoftTimer*>, 256> slots;
  void tick() {
      uint8_t slot = ++current_tick & 0xFF;  // 取低8位作槽索引
      for (auto* t : slots[slot]) t->fire();
  }
*/


// ============================================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================================

void part4_traps() {
    cout << string(60, '=') << "\n";
    cout << "第四部分：常见错误和陷阱\n";
    cout << string(60, '=') << "\n";

    cout << R"(
1. 周期任务用 sleep 而非绝对时间（累积误差）
──────────────────────────────────────
❌ while(true) { sample(); vTaskDelay(100); }
   // sample() 耗时 10ms → 实际间隔 110ms，越来越慢

✅ while(true) {
       vTaskDelayUntil(&last, 100);  // 绝对时间，间隔精确 100ms
       sample();
   }
   // 本文 TaskScheduler 的 next_ms += period_ms 也是同样原理

2. 软件定时器在 ISR 中执行回调
──────────────────────────────────────
❌ // SysTick ISR 中调用 scheduler.run()
   void SysTick_Handler() { scheduler.run(); }
   // 回调可能耗时长，阻塞系统！

✅ ISR 只做 ++g_tick
   scheduler.run() 在主循环中调用

3. tick 溢出导致定时器失效
──────────────────────────────────────
❌ uint16_t g_tick = 0;  // 16位，65535ms ≈ 65秒后溢出
   // 溢出后 next_tick = 50000，current = 100，50000 > 100 → 永不触发！

✅ 使用 uint32_t（约49天溢出），且比较时用有符号差：
   int32_t diff = (int32_t)(current - next_tick);
   if (diff >= 0) fire();  // 正确处理溢出绕回

4. 回调中修改调度器（迭代器失效）
──────────────────────────────────────
❌ scheduler.addTask("新任务", 50, [&](uint32_t) {
       scheduler.addTask("嵌套", 100, ...);  // 修改 tasks_ vector 触发重新分配
   });

✅ 回调中只设置标志，主循环检测标志后再 addTask

5. 多个任务同时到期引起延迟
──────────────────────────────────────
❌ 5个任务同时到期，每个回调耗时 10ms → 后面的任务延迟 40ms

✅ 回调保持简短（< 1ms），耗时操作只设标志
   或使用 RTOS 多任务，避免串行执行

)";
}


// ============================================================
// 第五部分：函数卡片速查
// ============================================================

void part5_cards() {
    cout << string(60, '=') << "\n";
    cout << "第五部分：函数卡片速查\n";
    cout << string(60, '=') << "\n";

    cout << R"(
【SoftTimer】
  SoftTimer(interval_ms, callback, repeat=true)
  start(current_tick)      — 启动，记录下次到期 tick
  stop()                   — 停止
  update(current_tick)     — 主循环调用，到期则执行回调，返回是否触发

【TimerScheduler<N>】
  add(interval_ms, cb, repeat) → id   — 添加并自动启动
  stop(id)                             — 停止指定定时器
  run()                                — 主循环调用，驱动所有定时器

【TaskScheduler】
  addTask(name, period_ms, fn)         — 注册周期任务
  run()                                — 主循环调用（固定周期，防累积误差）
  stopTask(name)                       — 停止任务

【FreeRTOS 对照】
  xTimerCreate(name, period, repeat, id, cb) — 创建软件定时器
  xTimerStart(timer, timeout)               — 启动
  xTimerStop(timer, timeout)                — 停止
  xTimerChangePeriod(timer, new_period, 0)  — 修改周期
  vTaskDelayUntil(&last, period)            — 周期任务精确延时

)";
}


// ============================================================
// 第六部分：练习题（不提供答案）
// ============================================================

void part6_exercises() {
    cout << string(60, '=') << "\n";
    cout << "第六部分：练习题\n";
    cout << string(60, '=') << "\n";

    cout << R"(
1. 实现单次延时定时器（One-shot）
   要求：
   - delayMs(uint32_t ms, function<void()> cb)：ms 后执行一次 cb
   - 内部用 repeat=false 的 SoftTimer
   - 验证：t=0 注册100ms单次定时器，t=100ms 触发，t=200ms 不再触发
   提示：TimerScheduler::add 中 repeat=false 即可

2. 实现看门狗定时器（Watchdog）
   要求：
   - WatchdogTimer(uint32_t timeout_ms, function<void()> bite_cb)
   - feed()：重置定时器（"喂狗"）
   - 内部用单次定时器；每次 feed() 调用 stop + start 重置
   - timeout_ms 内未 feed → 执行 bite_cb（告警/复位）
   提示：bite_cb 中可以打印"系统超时，执行复位"

3. 实现任务执行统计
   要求：
   - TaskScheduler 记录每个任务的：执行次数、最大耗时（μs）、平均耗时
   - run() 中用 steady_clock 计时每次执行
   - printStats() 打印统计报告
   提示：chrono::steady_clock::now() 计时，duration_cast<microseconds>

4. 实现优先级任务调度
   要求：
   - PeriodicTask 增加 uint8_t priority 字段（1=高，2=中，3=低）
   - 同一时刻多任务到期时，先执行高优先级任务
   - run() 中先收集所有到期任务，按优先级排序后依次执行
   提示：sort(due_tasks, by_priority)，用临时 vector 收集到期任务

)";
}


int main() {
    part1_concepts();
    demo_timers();
    part4_traps();
    part5_cards();
    part6_exercises();
    return 0;
}
