// ============================================
// 21_freertos_advanced.cpp
// 日期：2026-06-06（第62天）
// 主题：FreeRTOS 进阶 - 事件组、任务通知、软件定时器
// 难度：⭐⭐⭐ 高级
// 编译：g++ -std=c++17 -o 21_freertos 21_freertos_advanced.cpp
// ============================================

/*
【核心概念】

是什么？
- FreeRTOS进阶同步原语：比互斥锁/信号量更灵活的多任务协调机制
- 事件组（EventGroup）：32位标志，支持等待多个事件的AND/OR组合
- 任务通知（Task Notification）：轻量级任务间通信，无需创建额外对象
- 软件定时器（Software Timer）：基于RTOS时基的定时回调

为什么需要？
- 事件组：多个条件都满足才能继续（如：传感器就绪 AND 网络就绪 → 上报）
- 任务通知：替代只需1对1通信的信号量，节省内存和CPU
- 软件定时器：不需要硬件定时器资源，数量灵活

核心对比：
  信号量    → 1个条件，1:N通知
  事件组    → N个条件，AND/OR等待
  任务通知  → 1:1通知，最轻量
  软件定时器→ 周期/单次回调，不占任务栈
*/

#include <iostream>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <map>
using namespace std;

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;

static u32 g_tick = 0;
u32 xTaskGetTickCount() { return g_tick; }

// ============================================
// 第一部分：事件组 ⭐⭐⭐⭐⭐
// ============================================

/*
【函数卡片】EventGroup

FreeRTOS API：
  EventGroupHandle_t xEventGroupCreate()
  EventBits_t xEventGroupSetBits(eg, bits)
  EventBits_t xEventGroupClearBits(eg, bits)
  EventBits_t xEventGroupWaitBits(eg, bitsToWait, clearOnExit, waitForAll, timeout)

参数详解（xEventGroupWaitBits）：
  eg          — 事件组句柄
  bitsToWait  — 要等待的位掩码
  clearOnExit — pdTRUE：返回时自动清除等待的位
  waitForAll  — pdTRUE：所有位都置位才返回；pdFALSE：任一位置位即返回
  timeout     — 等待超时（portMAX_DELAY = 永久等待）
返回：触发返回时的事件组值
*/

class EventGroup {
    u32 bits = 0;

public:
    static constexpr u32 MAX_DELAY = 0xFFFFFFFF;

    void setBits(u32 mask) {
        bits |= mask;
        cout << "  [EventGroup] 置位 0x" << hex << mask
             << " 当前=0x" << bits << dec << endl;
    }

    void clearBits(u32 mask) { bits &= ~mask; }

    // 模拟等待（同步场景下直接检查）
    bool waitBits(u32 mask, bool clearOnExit, bool waitForAll) {
        bool ok = waitForAll ? ((bits & mask) == mask)
                             : ((bits & mask) != 0);
        if (ok && clearOnExit) bits &= ~mask;
        return ok;
    }

    u32 getBits() const { return bits; }
};

// 系统事件位定义（每一位代表一个事件）
constexpr u32 BIT_SENSOR_READY   = (1 << 0); // 传感器数据就绪
constexpr u32 BIT_NETWORK_READY  = (1 << 1); // 网络连接就绪
constexpr u32 BIT_CMD_RECEIVED   = (1 << 2); // 收到命令
constexpr u32 BIT_OTA_TRIGGERED  = (1 << 3); // OTA触发

void demoEventGroup() {
    cout << string(60, '-') << endl;
    cout << "1. 事件组演示" << endl;
    cout << string(60, '-') << endl;

    EventGroup eg;

    // 场景：只有传感器就绪 AND 网络就绪，才上报数据
    cout << "\n场景：等待（传感器就绪 AND 网络就绪）才上报数据" << endl;

    // 模拟两个任务分别置位
    cout << "\n传感器任务完成采集：" << endl;
    eg.setBits(BIT_SENSOR_READY);

    bool ready = eg.waitBits(BIT_SENSOR_READY | BIT_NETWORK_READY,
                             false, true); // AND等待
    cout << "  等待（AND）结果: " << (ready ? "就绪" : "未就绪") << endl;

    cout << "\n网络任务连接成功：" << endl;
    eg.setBits(BIT_NETWORK_READY);

    ready = eg.waitBits(BIT_SENSOR_READY | BIT_NETWORK_READY,
                        true, true); // AND等待，成功后清除
    cout << "  等待（AND）结果: " << (ready ? "✅ 两个条件都满足，开始上报！" : "未就绪") << endl;

    // 场景：等待任一事件（OR）
    cout << "\n场景：等待（命令收到 OR OTA触发）任一即处理" << endl;
    eg.setBits(BIT_OTA_TRIGGERED);
    bool any = eg.waitBits(BIT_CMD_RECEIVED | BIT_OTA_TRIGGERED,
                           true, false); // OR等待
    cout << "  等待（OR）结果: " << (any ? "✅ 有事件触发，进行处理" : "无事件") << endl;
}

// ============================================
// 第二部分：任务通知 ⭐⭐⭐⭐
// ============================================

/*
【概念】任务通知 vs 信号量

信号量：
  - 需要创建独立的信号量对象（消耗内存）
  - 适合多任务共享一个信号量（N:1场景）

任务通知：
  - 每个任务内置32位通知值，无需创建额外对象
  - 节省约20字节内存 + 更快的执行速度
  - 适合1:1通知场景

FreeRTOS API：
  xTaskNotify(taskHandle, value, eAction)  // 发送通知
  xTaskNotifyWait(ulBitsToClear, ulBitsToClear2, pulValue, timeout) // 等待

eAction 选项：
  eNoAction            → 仅通知，不修改通知值
  eSetBits             → 通知值 |= value
  eIncrement           → 通知值++
  eSetValueWithOverwrite → 直接覆写通知值
*/

struct TaskHandle {
    string name;
    u32    notification_value = 0;
    bool   notified = false;
};

// 模拟 xTaskNotify
void xTaskNotify(TaskHandle& task, u32 value, const string& action) {
    if (action == "SetBits") {
        task.notification_value |= value;
    } else if (action == "Overwrite") {
        task.notification_value = value;
    } else if (action == "Increment") {
        task.notification_value++;
    }
    task.notified = true;
    cout << "  [通知] → 任务'" << task.name
         << "' 通知值=0x" << hex << task.notification_value << dec << endl;
}

// 模拟 xTaskNotifyWait
bool xTaskNotifyWait(TaskHandle& task, u32& out_value) {
    if (!task.notified) return false;
    out_value = task.notification_value;
    task.notification_value = 0;
    task.notified = false;
    return true;
}

void demoTaskNotify() {
    cout << string(60, '-') << endl;
    cout << "2. 任务通知演示" << endl;
    cout << string(60, '-') << endl;

    TaskHandle worker{"工作任务"};

    // 场景1：用通知值传递状态位（替代事件组，适合1:1场景）
    cout << "\n场景1：用通知位传递多个事件（1:1场景）" << endl;
    xTaskNotify(worker, 0x01, "SetBits"); // 事件1
    xTaskNotify(worker, 0x04, "SetBits"); // 事件3
    u32 val = 0;
    if (xTaskNotifyWait(worker, val)) {
        cout << "  工作任务收到通知值=0x" << hex << val << dec << endl;
        if (val & 0x01) cout << "  → 处理事件1" << endl;
        if (val & 0x04) cout << "  → 处理事件3" << endl;
    }

    // 场景2：用通知值传递数据（替代只传1个值的消息队列）
    cout << "\n场景2：用通知值传递ADC采样结果" << endl;
    u32 adc_result = 2048;
    xTaskNotify(worker, adc_result, "Overwrite");
    if (xTaskNotifyWait(worker, val)) {
        cout << "  工作任务收到ADC值=" << val
             << " 电压=" << (val * 3300 / 4095) << "mV" << endl;
    }

    // 场景3：计数信号量替代
    cout << "\n场景3：任务通知替代计数信号量（中断每触发一次+1）" << endl;
    for (int i = 0; i < 3; ++i)
        xTaskNotify(worker, 0, "Increment");
    if (xTaskNotifyWait(worker, val))
        cout << "  积累了" << val << "次中断需要处理" << endl;
}

// ============================================
// 第三部分：软件定时器 ⭐⭐⭐⭐
// ============================================

/*
【函数卡片】软件定时器

FreeRTOS API：
  xTimerCreate(name, period, autoReload, id, callback)
  xTimerStart(timer, timeout)
  xTimerStop(timer, timeout)
  xTimerChangePeriod(timer, newPeriod, timeout)
  pvTimerGetTimerID(timer) → 获取创建时传入的id

参数说明：
  period     — 定时周期（单位：tick）
  autoReload — pdTRUE=周期定时器；pdFALSE=单次定时器
  id         — 用户数据，可在回调中通过pvTimerGetTimerID获取
  callback   — 定时回调函数（在定时器服务任务中执行，非ISR、非调用者任务）

⚠ 重要：回调函数不能阻塞（不能调用 vTaskDelay 等阻塞API）
*/

struct SoftTimer {
    string   name;
    u32      period;     // 周期（tick）
    bool     auto_reload;
    u32      next_tick;
    bool     running = false;
    function<void(SoftTimer&)> callback;

    SoftTimer(const string& n, u32 p, bool ar, function<void(SoftTimer&)> cb)
        : name(n), period(p), auto_reload(ar), next_tick(0), callback(cb) {}

    void start() {
        running   = true;
        next_tick = xTaskGetTickCount() + period;
        cout << "  [定时器'" << name << "'] 启动，周期=" << period << "ms" << endl;
    }

    void stop() { running = false; }

    // 在主循环中调用
    void tick() {
        if (!running) return;
        if (xTaskGetTickCount() >= next_tick) {
            callback(*this);
            if (auto_reload)
                next_tick += period;  // 绝对时间推进（防累积误差）
            else
                running = false;
        }
    }
};

void demoSoftTimer() {
    cout << string(60, '-') << endl;
    cout << "3. 软件定时器演示" << endl;
    cout << string(60, '-') << endl;

    int heartbeat_count = 0;
    int report_count    = 0;

    // 心跳定时器：每500ms触发（周期）
    SoftTimer heartbeat("心跳", 500, true, [&](SoftTimer&) {
        ++heartbeat_count;
        cout << "  [心跳] tick=" << xTaskGetTickCount()
             << " 第" << heartbeat_count << "次" << endl;
    });

    // 上报定时器：每1000ms触发（周期）
    SoftTimer report("上报", 1000, true, [&](SoftTimer&) {
        ++report_count;
        cout << "  [上报] tick=" << xTaskGetTickCount()
             << " 第" << report_count << "次数据上报" << endl;
    });

    // 单次定时器：3000ms后触发一次OTA检查
    SoftTimer ota_check("OTA检查", 3000, false, [](SoftTimer&) {
        cout << "  [OTA检查] 单次触发，检查服务器版本..." << endl;
    });

    heartbeat.start();
    report.start();
    ota_check.start();

    // 模拟主循环运行4秒
    cout << "\n模拟主循环（4秒）：" << endl;
    for (int i = 0; i < 40; ++i) {
        g_tick += 100; // 每次推进100ms
        heartbeat.tick();
        report.tick();
        ota_check.tick();
    }
}

// ============================================
// 第四部分：三种机制对比
// ============================================

void showComparison() {
    cout << R"(
============================================================
事件组 vs 任务通知 vs 信号量 对比
============================================================

┌─────────────┬──────────┬──────────┬──────────┐
│             │ 事件组   │ 任务通知  │ 信号量   │
├─────────────┼──────────┼──────────┼──────────┤
│ 等待多条件  │ ✅ AND/OR │ ✅(位操作)│ ❌ 只有1个│
│ 1:N广播     │ ✅        │ ❌ 1:1   │ ✅        │
│ 传递数值    │ ❌        │ ✅ 32位值 │ ❌        │
│ 内存开销    │ 中        │ 最小(内置)│ 小       │
│ 速度        │ 中        │ 最快      │ 快       │
│ ISR支持     │ ✅        │ ✅        │ ✅        │
└─────────────┴──────────┴──────────┴──────────┘

选择原则：
  多条件AND/OR等待      → 事件组
  1:1通知 + 传值        → 任务通知
  多任务竞争保护共享资源 → 互斥锁（Mutex）
  计数资源（缓冲区槽位）→ 计数信号量
============================================================
)";
}

// ============================================
// 常见错误
// ============================================

void showMistakes() {
    cout << R"(
============================================================
常见错误和陷阱
============================================================

❌ 错误1：在软件定时器回调中阻塞
  void timerCallback(TimerHandle_t xTimer) {
      vTaskDelay(100);   // ❌ 定时器回调不能阻塞！
      xQueueSend(...);   // ❌ 可能阻塞的API也不能用
  }
  // ✅ 回调中只能做快速操作：置标志、发非阻塞通知、写队列（timeout=0）

❌ 错误2：事件组在ISR中使用非ISR API
  xEventGroupSetBits(eg, bit);           // ❌ ISR中禁止
  xEventGroupSetBitsFromISR(eg, bit, &hp); // ✅ ISR版本

❌ 错误3：任务通知发送后立即覆写
  xTaskNotify(task, 0x01, eSetBits);  // 事件1
  xTaskNotify(task, 0x02, eSetBits);  // 事件2（此时事件1还未处理）
  // 如果接收任务还未处理，两次通知会合并 → ✅ 正常行为，用SetBits
  // 如果用eSetValueWithOverwrite → ❌ 事件1会被覆盖丢失

❌ 错误4：软件定时器周期用相对时间推进
  next_tick = HAL_GetTick() + period;  // ❌ 执行时间会累积进误差
  next_tick += period;                 // ✅ 绝对时间推进，误差不累积

❌ 错误5：混淆事件组的clearOnExit语义
  // clearOnExit=pdTRUE：成功等到后清除"你等待的位"
  // 不是清除整个事件组！其他任务等待的位不受影响
============================================================
)";
}

int main() {
    cout << string(60, '=') << endl;
    cout << "21_freertos_advanced.cpp - FreeRTOS进阶" << endl;
    cout << string(60, '=') << endl;

    demoEventGroup();
    demoTaskNotify();
    demoSoftTimer();
    showComparison();
    showMistakes();

    cout << R"(
练习题
============================================================
1. 用事件组实现"5个任务都完成初始化后，系统才开始运行"的栅栏同步
   提示：每个任务完成初始化后置对应位，主任务等待所有5位都置位

2. 用任务通知替代一个1:1的消息队列，传递结构体数据
   提示：通知值只有32位，需要先把数据写入共享内存，通知值用作"数据就绪"信号

3. 实现一个定时器管理器，支持动态创建/删除软件定时器，最多支持8个
   提示：用数组存储定时器，用链表管理活跃定时器
============================================================
)";
    return 0;
}
