// ============================================================
// 嵌入式内存管理 - 超详细注释版
// 日期：2026-06-03（第58天）
// 难度：⭐⭐ 中级
// 编译：g++ -std=c++17 -Wall -o 07_mem 07_memory_management.cpp
// ============================================================

/*
【核心概念】

是什么？
- 嵌入式系统内存资源极其有限（典型：64KB~512KB RAM）
- 必须精确控制每个字节的使用方式

为什么不用 malloc？
- malloc 内部有锁（非实时性）
- 堆碎片化（长时间运行后无法分配大块内存）
- 分配失败难以预测（实时系统不可接受）
- 代码体积增大

解决方案：静态分配 + 内存池
*/

#include <iostream>
#include <cstdint>
#include <cstring>
#include <array>
#include <cassert>
using namespace std;

// ============================================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================================

void part1_concepts() {
    cout << string(60, '=') << "\n";
    cout << "第一部分：嵌入式内存布局\n";
    cout << string(60, '=') << "\n";

    cout << R"(
【嵌入式内存四大区域】

高地址 ┌─────────────────┐
       │   Stack（栈）    │ ← 向下增长，局部变量、函数调用帧
       │        ↓        │   大小固定（链接脚本指定），溢出直接崩溃
       ├─────────────────┤
       │   （空闲区）     │
       ├─────────────────┤
       │   Heap（堆）     │ ← 向上增长，malloc/new（嵌入式慎用）
       ├─────────────────┤
       │  .bss（未初始化）│ ← 全局/静态变量，启动时清零（RAM）
       ├─────────────────┤
       │  .data（已初始化）│← 全局/静态变量有初值（RAM，从Flash复制）
低地址 └─────────────────┘ （以上是 RAM）

       ┌─────────────────┐
       │  .text（代码段）  │← 程序指令（Flash，只读）
       │  .rodata（常量）  │← const 字符串、常量表（Flash）
       └─────────────────┘

【各区域特点】
  Stack：快速，自动管理；溢出无保护则覆盖相邻内存
  Heap ：灵活，但碎片化；嵌入式通常禁用或严格限制
  .bss ：程序启动前由启动代码（startup.s）清零
  .data：启动代码从 Flash 中的镜像复制到 RAM

【嵌入式实践原则】
  ✅ 优先静态分配（全局变量、static局部变量）
  ✅ 使用内存池替代动态分配
  ✅ 在链接脚本中限制堆大小（甚至设为0）
  ❌ 避免运行时 malloc/new（非RTOS场景）

)";
}

// ============================================================
// 第二部分：基础操作实践
// ============================================================

// ------------------------------------------------------------
// 2.1 各内存区域的变量声明示例
// ------------------------------------------------------------

// .bss 段：未初始化全局变量（程序启动自动清零）
uint8_t  g_rx_buffer[256];      // 全局数组，256字节，在 .bss
uint32_t g_error_count;         // 全局变量，在 .bss，初值=0

// .data 段：有初值的全局变量（从 Flash 复制到 RAM）
uint8_t  g_device_id   = 0xAB;  // 有初值，在 .data
uint32_t g_baudrate    = 115200;

// .rodata 段（Flash）：const 全局变量
const uint8_t PROTOCOL_HEADER[] = {0xAA, 0x55, 0x01};  // 放 Flash，不占 RAM
const char*   DEVICE_NAME       = "EmbeddedSensor_v1";

void demo_memory_sections() {
    cout << string(60, '-') << "\n";
    cout << "2.1 内存区域演示\n";
    cout << string(60, '-') << "\n";

    // Stack：局部变量，函数返回后自动释放
    uint8_t  local_buf[64];        // 栈上64字节，注意不要声明过大
    uint32_t local_val = 42;

    // static 局部变量：在 .bss/.data，不在栈上，函数调用间保持值
    static uint32_t call_count = 0;  // 只初始化一次，保存在 .data
    ++call_count;

    cout << "全局变量 g_device_id = 0x" << hex << (int)g_device_id << dec << "\n";
    cout << "常量（Flash）DEVICE_NAME = " << DEVICE_NAME << "\n";
    cout << "static 局部变量 call_count = " << call_count << "\n";
    cout << "栈变量 local_val = " << local_val << "\n";

    (void)local_buf;  // 消除未使用警告
}

// ------------------------------------------------------------
// 2.2 固定大小内存池（静态分配，替代 malloc）
// ------------------------------------------------------------

/*
【内存池概念】
  预先分配一大块静态内存，切割成等大小的块
  分配：从空闲链表取一块（O(1)，无碎片）
  释放：归还到空闲链表（O(1)）
  优点：确定性、无碎片、无堆依赖
*/

// 模板内存池：N 个大小为 sizeof(T) 的块
template<typename T, size_t N>
class StaticPool {
public:
    StaticPool() {
        // 初始化空闲链表：所有块串联起来
        // 每个块的前4字节存储"下一个空闲块"的索引
        for (size_t i = 0; i < N - 1; ++i)
            next_[i] = static_cast<uint16_t>(i + 1);
        next_[N - 1] = NONE;   // 链表末尾
        free_head_   = 0;      // 空闲链表头指向第0块
        free_count_  = N;
    }

    // 分配一块（返回指针，失败返回 nullptr）
    // 时间复杂度 O(1)，无系统调用
    T* alloc() {
        if (free_head_ == NONE) return nullptr;  // 池已满
        uint16_t idx  = free_head_;
        free_head_    = next_[idx];  // 链表头移到下一个空闲块
        --free_count_;
        return reinterpret_cast<T*>(&storage_[idx]);
    }

    // 释放一块（归还到空闲链表头）
    void free(T* ptr) {
        if (!ptr) return;
        // 计算块索引：指针地址 - 存储基地址，除以块大小
        size_t idx = reinterpret_cast<AlignedBlock*>(ptr) - &storage_[0];
        assert(idx < N);           // 防止释放不属于本池的指针
        next_[idx] = free_head_;   // 新块插入链表头
        free_head_ = static_cast<uint16_t>(idx);
        ++free_count_;
    }

    size_t freeCount()  const { return free_count_; }
    size_t totalCount() const { return N; }

private:
    // 对齐存储块：保证 T 的对齐要求满足
    using AlignedBlock = aligned_storage_t<sizeof(T), alignof(T)>;

    static constexpr uint16_t NONE = 0xFFFF;  // 链表末尾标志

    AlignedBlock storage_[N];   // 静态存储区（.bss 段，不用堆）
    uint16_t     next_[N];      // 空闲链表数组
    uint16_t     free_head_;    // 空闲链表头索引
    size_t       free_count_;   // 当前空闲块数量
};

// 传感器数据结构（内存池管理的对象）
struct SensorPacket {
    uint8_t  sensor_id;
    uint16_t raw_value;
    uint32_t timestamp_ms;
};

// 全局静态内存池：8个 SensorPacket 块（在 .bss，不占堆）
StaticPool<SensorPacket, 8> g_packet_pool;

void demo_static_pool() {
    cout << string(60, '-') << "\n";
    cout << "2.2 静态内存池演示\n";
    cout << string(60, '-') << "\n";

    cout << "池总量=" << g_packet_pool.totalCount()
         << " 空闲=" << g_packet_pool.freeCount() << "\n";

    // 分配3个包
    SensorPacket* p1 = g_packet_pool.alloc();
    SensorPacket* p2 = g_packet_pool.alloc();
    SensorPacket* p3 = g_packet_pool.alloc();

    if (p1) { p1->sensor_id = 1; p1->raw_value = 512; p1->timestamp_ms = 1000; }
    if (p2) { p2->sensor_id = 2; p2->raw_value = 768; p2->timestamp_ms = 1010; }

    cout << "分配3个后，空闲=" << g_packet_pool.freeCount() << "\n";

    // 使用数据
    if (p1) cout << "p1: sensor=" << (int)p1->sensor_id
                 << " raw=" << p1->raw_value << "\n";

    // 释放
    g_packet_pool.free(p1);
    g_packet_pool.free(p2);
    g_packet_pool.free(p3);
    cout << "释放后，空闲=" << g_packet_pool.freeCount() << "\n";
}

// ------------------------------------------------------------
// 2.3 栈溢出检测（Stack Canary）
// ------------------------------------------------------------

/*
【栈溢出 Canary 原理】
  在栈底放一个已知值（"金丝雀"），定期检查是否被覆盖
  被覆盖说明栈溢出了
  FreeRTOS 的 configCHECK_FOR_STACK_OVERFLOW 就用此原理
*/

constexpr uint32_t STACK_CANARY_VALUE = 0xDEADBEEF;

// 模拟任务栈（静态分配，真实嵌入式中由链接脚本定义）
struct TaskStack {
    uint32_t canary;         // 栈底 Canary
    uint8_t  stack[512];     // 实际栈空间
};

TaskStack g_task_stack;

void stack_canary_init() {
    g_task_stack.canary = STACK_CANARY_VALUE;  // 初始化 Canary
}

bool stack_canary_check() {
    return g_task_stack.canary == STACK_CANARY_VALUE;  // 检查是否被覆盖
}

void demo_stack_canary() {
    cout << string(60, '-') << "\n";
    cout << "2.3 栈溢出检测（Canary）\n";
    cout << string(60, '-') << "\n";

    stack_canary_init();
    cout << "初始化 Canary=0x" << hex << STACK_CANARY_VALUE << dec << "\n";
    cout << "检查结果：" << (stack_canary_check() ? "正常✅" : "溢出❌") << "\n";

    // 模拟栈溢出（破坏 Canary）
    g_task_stack.canary = 0x12345678;
    cout << "模拟溢出后检查：" << (stack_canary_check() ? "正常✅" : "溢出❌") << "\n";

    // 恢复
    stack_canary_init();
}

// ============================================================
// 🔍 深入理解：链接脚本与内存布局（可跳过）
// ============================================================

/*
链接脚本（.ld 文件）控制各段在内存中的位置：

MEMORY {
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

SECTIONS {
  .text  : { *(.text*)  } > FLASH   // 代码放 Flash
  .data  : { *(.data*)  } > RAM AT > FLASH  // 数据放 RAM，初值存 Flash
  .bss   : { *(.bss*)   } > RAM     // 未初始化放 RAM
  ._heap : { . = ALIGN(8); _heap_start = .; . += 4K; } > RAM
  ._stack: { . = ALIGN(8); . += 8K; _stack_top = .; } > RAM
}

禁用堆（防止意外 malloc）：
  ._heap : { . = ALIGN(8); _heap_start = .; . += 0; }  // 堆大小设为0
  // 此时 malloc 会立刻返回 NULL
*/

// ============================================================
// 第三部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================================

void part3_traps() {
    cout << string(60, '=') << "\n";
    cout << "第三部分：常见错误和陷阱\n";
    cout << string(60, '=') << "\n";

    cout << R"(
1. 栈上分配过大数组
──────────────────────────────────────
❌ void task() { uint8_t buf[4096]; process(buf); }
   // 栈大小只有2KB，直接溢出覆盖相邻内存，系统崩溃

✅ static uint8_t buf[4096];  // static：放 .bss，不占栈
   void task() { process(buf); }

2. 忘记检查内存池分配失败
──────────────────────────────────────
❌ SensorPacket* p = g_packet_pool.alloc();
   p->sensor_id = 1;  // 池满时 p==nullptr，解引用崩溃！

✅ SensorPacket* p = g_packet_pool.alloc();
   if (!p) { log_error("内存池耗尽"); return; }
   p->sensor_id = 1;

3. 释放不属于内存池的指针
──────────────────────────────────────
❌ SensorPacket local_pkt;
   g_packet_pool.free(&local_pkt);  // local_pkt 是栈变量，不在池中！

✅ 只释放从 alloc() 获得的指针
   用 assert(idx < N) 在调试模式检测越界释放

4. const 变量放 RAM 浪费内存
──────────────────────────────────────
❌ uint8_t lookup_table[256] = { 0, 1, 2, ... };  // 放 .data，占 RAM

✅ const uint8_t lookup_table[256] = { ... };      // 放 .rodata（Flash），不占 RAM
   // 嵌入式 Flash 通常比 RAM 大 4~8 倍，优先使用 Flash 存常量

5. 内存池块大小不考虑对齐
──────────────────────────────────────
❌ struct Pkt { uint8_t id; uint32_t ts; };  // 实际大小8字节（有填充）
   char pool[N * 5];  // 按5字节算，实际访问时地址未对齐 → 硬件 fault

✅ 使用 aligned_storage_t<sizeof(T), alignof(T)> 自动处理对齐

6. 局部 static 变量线程安全问题（RTOS 中）
──────────────────────────────────────
❌ void task() { static bool initialized = false; if (!initialized) { init(); initialized = true; } }
   // 两个任务同时调用：都看到 initialized==false，都执行 init()！

✅ 用 mutex 保护，或在任务启动前（main 中）完成初始化

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
【StaticPool<T, N>（本文实现）】
  alloc() → T*     — 从池中分配一块，池满返回 nullptr，O(1)
  free(T* ptr)     — 归还到池，O(1)
  freeCount()      — 当前空闲块数
  totalCount()     — 池总块数

【aligned_storage_t<Size, Align>】（C++11）
  作用：提供满足对齐要求的原始存储空间
  语法：aligned_storage_t<sizeof(T), alignof(T)> storage;
  用途：内存池的底层存储，避免对齐 fault

【内存段关键字（GCC/嵌入式）】
  __attribute__((section(".ccmram")))  — 指定变量放到特定段
  static                               — 放 .bss/.data，不占栈
  const                                — 尽量放 .rodata（Flash）
  volatile                             — 禁止编译器优化（MMIO/ISR共享）

【栈使用量查询（FreeRTOS）】
  uxTaskGetStackHighWaterMark(NULL)    — 返回当前任务栈剩余最小值（words）
  建议：剩余 < 20% 时扩大栈

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
1. 实现变长块内存池（Slab 分配器简化版）
   要求：
   - 支持 3 种固定大小：16B / 64B / 256B
   - alloc(size_t n) 自动选择最合适的池
   - 超过 256B 返回 nullptr（不使用堆）
   提示：三个 StaticPool 实例 + if/else 路由

2. 为 StaticPool 添加内存使用统计
   要求：
   - 记录历史最高使用量（highWaterMark）
   - 记录分配失败次数（allocFailCount）
   - printStats() 打印报告
   提示：在 alloc() 中更新统计

3. 实现编译期内存预算检查
   要求：
   - 计算所有全局变量占用的总 RAM
   - 用 static_assert 确保不超过目标平台 RAM（如 64KB）
   提示：sizeof 各全局结构体求和，static_assert(total < 65536, "RAM超限")

4. 实现双缓冲区（Ping-Pong Buffer）
   要求：
   - 两个静态缓冲区 buf_a[256] 和 buf_b[256]
   - ISR 写当前缓冲区，主循环处理另一个
   - swap()：原子切换两个缓冲区的角色
   提示：用 volatile uint8_t* 指针和 atomic<bool> 标记当前写缓冲区

)";
}

int main() {
    part1_concepts();
    demo_memory_sections();
    demo_static_pool();
    demo_stack_canary();
    part3_traps();
    part4_cards();
    part5_exercises();
    return 0;
}
