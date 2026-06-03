// ============================================================
// 嵌入式内存管理 + 低功耗综合练习
// 日期：2026-06-03（第58天）
// 难度：⭐⭐⭐ 高级
// 编译：g++ -std=c++17 -Wall -pthread -o 09_exercise 09_memory_power_exercise.cpp
// ============================================================

/*
【综合练习目标】
融合本节两大主题：
  - 静态内存池（替代 malloc，无碎片）
  - 低功耗状态机（事件驱动 + WFI 睡眠）

实现：IoT 传感器节点模拟
  静态内存池管理数据包 → 低功耗主循环 → 批处理发送 → 深度睡眠
*/

#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>
#include <cstdint>
#include <cassert>
#include <type_traits>
using namespace std;
using namespace chrono;

// ============================================================
// 第一部分：静态内存池（复用 + 扩展）
// ============================================================

// 编译期检查：N 必须是2的幂次（环形缓冲区索引优化）
template<size_t N>
struct IsPowerOfTwo {
    static_assert(N > 0 && (N & (N - 1)) == 0, "N 必须是2的幂次！");
};

template<typename T, size_t N>
class StaticPool : IsPowerOfTwo<N> {
public:
    StaticPool() {
        for (size_t i = 0; i < N - 1; ++i) next_[i] = uint8_t(i + 1);
        next_[N - 1] = NONE;
        free_head_   = 0;
        free_count_  = N;
        alloc_fail_  = 0;
        high_water_  = 0;  // 历史最高使用量
    }

    T* alloc() {
        if (free_head_ == NONE) { ++alloc_fail_; return nullptr; }
        uint8_t idx = free_head_;
        free_head_  = next_[idx];
        --free_count_;
        // 更新高水位线
        size_t used = N - free_count_;
        if (used > high_water_) high_water_ = used;
        return reinterpret_cast<T*>(&storage_[idx]);
    }

    void free(T* ptr) {
        if (!ptr) return;
        size_t idx = reinterpret_cast<AlignedBlock*>(ptr) - &storage_[0];
        assert(idx < N);
        next_[idx] = free_head_;
        free_head_ = uint8_t(idx);
        ++free_count_;
    }

    // 统计信息
    size_t freeCount()    const { return free_count_; }
    size_t usedCount()    const { return N - free_count_; }
    size_t highWaterMark()const { return high_water_; }
    size_t allocFails()   const { return alloc_fail_; }

    void printStats(const char* name) const {
        cout << "[Pool:" << name << "] "
             << "总量=" << N
             << " 使用=" << usedCount()
             << " 空闲=" << freeCount()
             << " 高水位=" << high_water_
             << " 分配失败=" << alloc_fail_ << "\n";
    }

private:
    using AlignedBlock = aligned_storage_t<sizeof(T), alignof(T)>;
    static constexpr uint8_t NONE = 0xFF;

    AlignedBlock storage_[N];
    uint8_t      next_[N];
    uint8_t      free_head_;
    size_t       free_count_;
    size_t       alloc_fail_;
    size_t       high_water_;
};

// ============================================================
// 第二部分：IoT 传感器节点数据结构
// ============================================================

// 传感器数据包
struct SensorPacket {
    uint8_t  node_id;         // 节点编号
    uint8_t  sensor_type;     // 传感器类型：0=温度 1=湿度 2=气压
    int16_t  value_x10;       // 值×10（避免浮点）例：235 = 23.5℃
    uint32_t timestamp_ms;    // 采集时间戳
};

// 发送队列节点（链表节点，同样用内存池管理）
struct TxQueueNode {
    SensorPacket pkt;
    TxQueueNode* next = nullptr;
};

// 全局内存池（静态，放 .bss，不占堆）
StaticPool<SensorPacket, 16> g_pkt_pool;    // 16个数据包缓冲
StaticPool<TxQueueNode,   8> g_txq_pool;    // 8个发送队列节点

// ============================================================
// 第三部分：低功耗主循环
// ============================================================

enum class PowerMode { Active, Sleep, Stop };

const char* mode_name(PowerMode m) {
    switch (m) {
        case PowerMode::Active: return "Active";
        case PowerMode::Sleep:  return "Sleep";
        case PowerMode::Stop:   return "Stop";
    }
    return "?";
}

// 电流消耗（μA）
uint32_t mode_ua(PowerMode m) {
    switch (m) {
        case PowerMode::Active: return 50000;
        case PowerMode::Sleep:  return 1000;
        case PowerMode::Stop:   return 50;
    }
    return 0;
}

// 事件标志（由"ISR线程"设置）
atomic<bool> g_sensor_ready{false};   // ADC 采集完成
atomic<bool> g_rtc_wakeup{false};     // RTC 定时唤醒
atomic<bool> g_tx_done{false};        // 发送完成
atomic<bool> g_sim_done{false};       // 演示结束标志
atomic<uint32_t> g_tick_ms{0};

// 模拟 WFI（PC 端）
void wfi() { this_thread::sleep_for(milliseconds(5)); }

// 主循环核心：事件驱动 + 功耗管理
class IotNode {
public:
    void run() {
        // 启动"硬件模拟"线程
        thread hw(hw_simulator_thread);

        int cycle = 0;
        while (!g_sim_done.load() && cycle < 12) {
            ++cycle;

            // --- 处理所有待处理事件 ---
            bool did_work = false;

            if (g_rtc_wakeup.exchange(false)) {
                setMode(PowerMode::Active);
                recordTime(0);
                cout << "[Node t=" << g_tick_ms.load() << "ms] RTC唤醒，请求采样\n";
                did_work = true;
            }

            if (g_sensor_ready.exchange(false)) {
                // 从内存池分配包，填写数据
                SensorPacket* pkt = g_pkt_pool.alloc();
                if (pkt) {
                    pkt->node_id     = 1;
                    pkt->sensor_type = 0;    // 温度
                    pkt->value_x10   = 235;  // 23.5℃
                    pkt->timestamp_ms = g_tick_ms.load();
                    cout << "[Node] 采集完成，温度=" << pkt->value_x10 / 10.0f << "℃\n";
                    enqueue(pkt);
                } else {
                    cout << "[Node] ⚠️ 内存池满，丢弃本次采样\n";
                }
                did_work = true;
            }

            if (g_tx_done.exchange(false)) {
                flush_tx_queue();  // 发送完成，释放已发送的包
                cout << "[Node] 发送完成，进入 Stop 模式\n";
                setMode(PowerMode::Stop);
                did_work = true;
            }

            // --- 无事件：选择合适的低功耗模式 ---
            if (!did_work) {
                uint32_t idle_ms = g_tick_ms.load() - last_active_ms_;
                if (idle_ms > 50) {
                    setMode(PowerMode::Stop);
                } else {
                    setMode(PowerMode::Sleep);
                }
                recordTime(5);  // 记录睡眠时间
                wfi();
            } else {
                recordTime(10);
                last_active_ms_ = g_tick_ms.load();
            }
        }

        g_sim_done.store(true);
        hw.join();

        // 打印最终统计
        cout << "\n" << string(60, '=') << "\n";
        cout << "IoT 节点运行统计\n";
        cout << string(60, '=') << "\n";
        g_pkt_pool.printStats("SensorPacket");
        g_txq_pool.printStats("TxQueueNode");
        cout << "累计消耗：" << total_charge_nah_ / 1000 << " μAh\n";
    }

private:
    PowerMode mode_          = PowerMode::Active;
    uint32_t  last_active_ms_= 0;
    uint64_t  total_charge_nah_ = 0;

    // 发送队列（链表，节点由 g_txq_pool 管理）
    TxQueueNode* tx_head_ = nullptr;
    TxQueueNode* tx_tail_ = nullptr;

    void setMode(PowerMode m) {
        if (mode_ != m) {
            cout << "[PowerMgr] " << mode_name(mode_) << " → " << mode_name(m)
                 << " (" << mode_ua(m) << "μA)\n";
            mode_ = m;
        }
    }

    void recordTime(uint32_t ms) {
        total_charge_nah_ += (uint64_t)mode_ua(mode_) * ms / 3600;
    }

    // 将数据包入发送队列
    void enqueue(SensorPacket* pkt) {
        TxQueueNode* node = g_txq_pool.alloc();
        if (!node) { g_pkt_pool.free(pkt); return; }
        node->pkt  = *pkt;   // 拷贝数据
        node->next = nullptr;
        g_pkt_pool.free(pkt);  // 立刻归还数据包内存

        if (!tx_tail_) { tx_head_ = tx_tail_ = node; }
        else           { tx_tail_->next = node; tx_tail_ = node; }

        cout << "[Node] 数据包入队，队列长度=" << queue_len() << "\n";
    }

    // 清空发送队列，释放所有节点内存
    void flush_tx_queue() {
        while (tx_head_) {
            TxQueueNode* next = tx_head_->next;
            g_txq_pool.free(tx_head_);
            tx_head_ = next;
        }
        tx_tail_ = nullptr;
    }

    size_t queue_len() const {
        size_t n = 0;
        for (auto* p = tx_head_; p; p = p->next) ++n;
        return n;
    }

    // 硬件模拟线程：产生 RTC 唤醒 / 传感器就绪 / 发送完成事件
    static void hw_simulator_thread() {
        auto tick = [](uint32_t ms) {
            this_thread::sleep_for(milliseconds(ms));
            g_tick_ms.fetch_add(ms, memory_order_relaxed);
        };

        for (int i = 0; i < 3 && !g_sim_done.load(); ++i) {
            tick(30);
            g_rtc_wakeup.store(true, memory_order_release);    // RTC 唤醒
            tick(10);
            g_sensor_ready.store(true, memory_order_release);  // 采样完成
            tick(20);
            g_tx_done.store(true, memory_order_release);       // 发送完成
        }
        tick(10);
        g_sim_done.store(true, memory_order_release);
    }
};

// ============================================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================================

void part4_traps() {
    cout << string(60, '=') << "\n";
    cout << "第四部分：常见错误和陷阱\n";
    cout << string(60, '=') << "\n";

    cout << R"(
1. 数据包未归还内存池导致泄漏
──────────────────────────────────────
❌ SensorPacket* p = g_pkt_pool.alloc();
   if (error) return;  // 直接返回，p 未 free → 内存泄漏，池逐渐耗尽

✅ SensorPacket* p = g_pkt_pool.alloc();
   if (error) { g_pkt_pool.free(p); return; }  // 错误路径也要 free
   // 或者用 RAII 包装：析构时自动 free

2. 队列节点和数据包生命周期混乱
──────────────────────────────────────
❌ enqueue(pkt);
   // 之后继续使用 pkt 指针
   pkt->value_x10 = 999;  // pkt 已经 free 了，写入悬空内存！

✅ enqueue() 中先拷贝数据到 TxQueueNode，再 free 原始 pkt
   调用者不再访问已 free 的指针

3. 内存池 N 超过 uint8_t 范围
──────────────────────────────────────
❌ StaticPool<T, 300> pool;  // N=300，但 next_ 是 uint8_t（最大255）
   // 链表索引溢出，行为未定义

✅ 本实现 N ≤ 254（255 = NONE 保留）
   大池子换 uint16_t 索引（NONE = 0xFFFF）

4. 低功耗模式切换过于频繁
──────────────────────────────────────
❌ 每次主循环都 enterSleep() → 处理事件 → enterSleep()
   // 频繁切换本身有时间/电量开销（~5μs × 1000次/s = 5ms额外开销）

✅ 只在"确认无待处理事件"后才切换到低功耗
   设置最小活跃时间（如 idle_ms > 50ms 才进 Stop）

5. WFI 前未禁用不必要的中断源
──────────────────────────────────────
❌ 定时器以 1kHz 运行，每 1ms 唤醒一次 → CPU 几乎无法睡眠

✅ 仅保留必要的唤醒源（RTC、外部中断）
   降低定时器频率或在睡眠前关闭定时器中断

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
1. 为 StaticPool 实现 RAII 包装器
   要求：
   - template<typename T, size_t N>
     class PoolPtr（类似 unique_ptr）
   - 构造：从池中 alloc()
   - 析构：自动 free() 归还池
   - 支持 operator-> 和 operator* 访问数据
   提示：持有 StaticPool<T,N>& 引用和 T* 指针

2. 实现带优先级的发送队列
   要求：
   - SensorPacket 增加 uint8_t priority 字段（0=低，1=高）
   - enqueue() 按优先级插入：高优先级在低优先级前
   - 内存池不变，只改链表插入逻辑
   提示：遍历链表找到第一个优先级更低的节点，在其前插入

3. 实现编译期内存预算验证
   要求：
   - 计算 g_pkt_pool + g_txq_pool 的总占用字节
   - static_assert 确保不超过 4KB
   - 打印各池的 sizeof
   提示：sizeof(StaticPool<T,N>) 即包含 storage_ 在内的总大小

4. 实现自适应采样频率
   要求：
   - 内存池使用率 > 75%：降低采样频率（延长 RTC 唤醒间隔）
   - 内存池使用率 < 25%：恢复正常频率
   - IotNode::run() 中根据 g_pkt_pool.usedCount() / N 决策
   提示：每次 RTC 唤醒后检查内存池状态，动态调整下次 RTC 唤醒时间

)";
}

int main() {
    cout << string(60, '=') << "\n";
    cout << "IoT 传感器节点模拟（内存池 + 低功耗）\n";
    cout << string(60, '=') << "\n\n";

    IotNode node;
    node.run();

    part4_traps();
    part5_exercises();
    return 0;
}
