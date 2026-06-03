// ============================================================
// 嵌入式中断处理 - 超详细注释版
// 日期：2026-06-03（第57天）
// 难度：⭐⭐ 中级
// 编译：g++ -std=c++17 -Wall -pthread -o 03_interrupt 03_interrupt_handling.cpp
// ============================================================

/*
【核心概念】

是什么？
- 中断（Interrupt）：硬件/软件向CPU发出的紧急信号，打断当前执行流
- ISR（Interrupt Service Routine）：响应中断的特殊函数

为什么需要？
- 避免CPU轮询（polling）浪费资源
- 实现对实时事件的快速响应（按键、串口、ADC）

核心特点：
- ISR必须短小精悍，不能阻塞
- 与主循环共享的变量必须加 volatile
- 多字节操作需要关中断保护临界区
*/

#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>
#include <cstdint>
using namespace std;

// ============================================================
// 嵌入式宏模拟（PC端运行）
// ============================================================

// 真实嵌入式：__attribute__((interrupt)) 或向量表注册
#define ISR_HANDLER

// 真实嵌入式：操作PRIMASK寄存器（ARM Cortex-M）
// __disable_irq() / __enable_irq()
#define DISABLE_IRQ()  do {} while(0)
#define ENABLE_IRQ()   do {} while(0)


// ============================================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================================

void part1_concepts() {
    cout << string(60, '=') << "\n";
    cout << "第一部分：核心概念详解\n";
    cout << string(60, '=') << "\n";

    cout << R"(
【中断 vs 轮询】

轮询（Polling）：CPU不停检查状态寄存器
  while(true) { if (uart_rx_ready()) process(); }
  缺点：CPU 100%占用，其他任务无法执行

中断（Interrupt）：事件发生时硬件通知CPU
  CPU正常执行 → 中断触发 → 保存现场 → 执行ISR → 恢复现场 → 继续执行
  优点：CPU可以做其他事，实时性好

【ISR 黄金法则】
  ✅ 只设置标志位 / 读硬件寄存器
  ✅ 执行时间 < 1μs（< 168时钟周期 @ 168MHz）
  ❌ 禁止：printf / malloc / delay / 等待锁

【volatile 的作用】
  - 禁止编译器将变量缓存在寄存器
  - 确保每次都从内存读写
  - 不保证原子性（多字节变量仍需额外保护）

【临界区（Critical Section）】
  - 主循环读/写多字节共享变量时，关中断防止ISR中途修改
  - 关中断时间必须极短

)";
}


// ============================================================
// 第二部分：基础操作实践
// ============================================================

// ------------------------------------------------------------
// 2.1 volatile 标志位通信（最常用模式）
// ------------------------------------------------------------

// volatile：ISR 修改，主循环读取，必须加 volatile
// 不加 volatile 时编译器可能将 while(!flag) 优化为死循环
volatile bool g_uart_rx_flag   = false;  // UART 接收中断标志
volatile bool g_button_flag    = false;  // 外部中断（按键）标志
volatile uint8_t g_uart_data   = 0;      // ISR 暂存的接收字节

// ISR：只做两件事——读硬件寄存器 + 设置标志位
ISR_HANDLER void UART_RxISR() {
    g_uart_data    = 0x41;   // 模拟读 UART 数据寄存器，收到 'A'
    g_uart_rx_flag = true;   // 通知主循环
    // ISR 结束，立刻返回
}

ISR_HANDLER void EXTI_ButtonISR() {
    g_button_flag = true;  // 只设标志，防抖由主循环负责
}

void demo_flag_pattern() {
    cout << string(60, '-') << "\n";
    cout << "2.1 volatile 标志位通信\n";
    cout << string(60, '-') << "\n";

    // 模拟触发中断（真实系统由硬件触发）
    UART_RxISR();
    EXTI_ButtonISR();

    // 主循环：检查标志位
    if (g_uart_rx_flag) {
        g_uart_rx_flag = false;           // ① 先清标志（防漏下次中断）
        cout << "收到 UART 数据：" << (char)g_uart_data << "\n";
    }

    if (g_button_flag) {
        g_button_flag = false;
        cout << "检测到按键按下\n";
    }
}


// ------------------------------------------------------------
// 2.2 环形缓冲区（Ring Buffer）——ISR 高速写入，主循环慢速读取
// ------------------------------------------------------------

// 环形缓冲区：解决 ISR 和主循环速度不匹配的问题
// ISR 写入（tail 移动），主循环读取（head 移动）
// 关键：tail 由 ISR 写，head 由主循环写，互不干扰（8位MCU 单字节赋值原子）

constexpr uint8_t RING_SIZE = 16;  // 必须是2的幂次方（方便取模）

struct RingBuffer {
    volatile uint8_t buf[RING_SIZE];  // 数据缓冲区，ISR 会写，加 volatile
    volatile uint8_t head = 0;        // 读指针（主循环修改）
    volatile uint8_t tail = 0;        // 写指针（ISR 修改）
};

RingBuffer g_ring;

// ISR 调用：写入一字节，缓冲区满则丢弃（不能阻塞等待！）
ISR_HANDLER bool ringbuf_push(uint8_t byte) {
    uint8_t next_tail = (g_ring.tail + 1) % RING_SIZE;
    if (next_tail == g_ring.head) return false;  // 缓冲区满，丢弃
    g_ring.buf[g_ring.tail] = byte;
    g_ring.tail = next_tail;  // 8位赋值在大多数MCU上是原子的
    return true;
}

// 主循环调用：读取一字节
bool ringbuf_pop(uint8_t* out) {
    if (g_ring.head == g_ring.tail) return false;  // 缓冲区空
    *out = g_ring.buf[g_ring.head];
    DISABLE_IRQ();
    g_ring.head = (g_ring.head + 1) % RING_SIZE;  // 关中断保护 head 更新
    ENABLE_IRQ();
    return true;
}

void demo_ring_buffer() {
    cout << string(60, '-') << "\n";
    cout << "2.2 环形缓冲区\n";
    cout << string(60, '-') << "\n";

    // 模拟 UART ISR 连续写入
    const char* msg = "Hello!";
    for (const char* p = msg; *p; ++p) ringbuf_push(*p);

    // 主循环读取
    cout << "从环形缓冲区读取：";
    uint8_t byte;
    while (ringbuf_pop(&byte)) cout << (char)byte;
    cout << "\n";
}


// ------------------------------------------------------------
// 2.3 atomic——防止多字节变量竞争条件（C++11，ARM Cortex-M3+可用）
// ------------------------------------------------------------

// 错误示例（注释掉，不运行）：
//   volatile uint32_t g_ms = 0;  // 在8位MCU上，读写需要多条指令，ISR可能打断

// 正确示例：atomic 保证原子性
atomic<uint32_t> g_systick_ms{0};  // 系统节拍计数（毫秒）

// 模拟 SysTick ISR（每1ms触发）
void systick_isr_thread() {
    for (int i = 0; i < 5; ++i) {
        this_thread::sleep_for(chrono::milliseconds(10));
        // fetch_add：原子自增，返回旧值
        // memory_order_relaxed：只保证原子性，计数器够用
        g_systick_ms.fetch_add(1, memory_order_relaxed);
    }
}

void demo_atomic() {
    cout << string(60, '-') << "\n";
    cout << "2.3 atomic 原子操作\n";
    cout << string(60, '-') << "\n";

    thread isr(systick_isr_thread);

    uint32_t last = 0;
    for (int i = 0; i < 3; ++i) {
        this_thread::sleep_for(chrono::milliseconds(15));
        uint32_t cur = g_systick_ms.load(memory_order_relaxed);
        if (cur != last) {
            cout << "系统节拍：" << cur << " ms\n";
            last = cur;
        }
    }

    isr.join();
    cout << "最终节拍：" << g_systick_ms.load() << " ms\n";
}


// ============================================================
// 🔍 深入理解：volatile vs atomic（可跳过）
// ============================================================

/*
volatile：
  - 禁止编译器优化，强制访问内存
  - 不保证原子性，不产生内存屏障
  - 适用：单字节标志位；内存映射寄存器（MMIO）

atomic<T>：
  - 保证操作原子性（不会读到中间状态）
  - 产生内存屏障（防止CPU/编译器重排序）
  - 适用：32位计数器、需要多CPU核心可见的变量

嵌入式实践：
  - bool / uint8_t 标志位 → volatile 足够
  - uint32_t 计数器（Cortex-M3+，单核）→ volatile 也可以（LDR/STR原子）
  - uint32_t 计数器（8位MCU / 多核）→ 必须关中断或 atomic
  - 复杂数据结构 → 关中断保护临界区
*/


// ============================================================
// 第三部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================================

void part3_traps() {
    cout << string(60, '=') << "\n";
    cout << "第三部分：常见错误和陷阱\n";
    cout << string(60, '=') << "\n";

    cout << R"(
1. 忘记 volatile → 编译器优化消除循环
──────────────────────────────────────
❌ bool flag = false;
   while (!flag) { }  // 编译器优化为死循环！

✅ volatile bool flag = false;
   while (!flag) { }  // 每次都从内存读

原因：编译器不知道 ISR 会修改 flag，直接优化掉了内存读取。

2. ISR 中调用阻塞函数
──────────────────────────────────────
❌ void UART_ISR() { printf("收到\n"); }  // printf 有锁，可能死锁！
❌ void UART_ISR() { HAL_Delay(10); }     // delay 依赖 SysTick 中断，死锁！

✅ void UART_ISR() { g_flag = true; }      // 只设标志

3. 清标志位的顺序错误
──────────────────────────────────────
❌ if (g_flag) { process(); g_flag = false; }
   // process() 期间 ISR 可能又设置了 g_flag，清标志后丢失本次中断！

✅ if (g_flag) { g_flag = false; process(); }
   // 先清标志，process() 期间的新中断不会丢失

4. 多字节变量竞争（8位MCU）
──────────────────────────────────────
❌ volatile uint32_t cnt = 0;
   // 主循环读：低16位是新值，高16位是旧值 → 读到错误数据

✅ DISABLE_IRQ();
   uint32_t snap = cnt;  // 关中断期间原子读
   ENABLE_IRQ();

5. 环形缓冲区大小不是2的幂次
──────────────────────────────────────
❌ #define BUF_SIZE 100
   idx = (idx + 1) % BUF_SIZE;  // 取模操作慢（除法指令）

✅ #define BUF_SIZE 128  // 2的幂次
   idx = (idx + 1) & (BUF_SIZE - 1);  // 位与操作快（单条指令）

6. ISR 中使用动态内存 / C++ 异常
──────────────────────────────────────
❌ void ISR() { auto* p = new Data(); throw std::runtime_error(""); }
   // malloc 有锁会死锁；异常在中断上下文行为未定义

✅ ISR 中只使用全局/静态/栈上局部变量

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
【atomic<T>::load(memory_order)】
  作用：原子读取值
  常用 order：
    memory_order_relaxed  — 只保证原子性（计数器）
    memory_order_acquire  — 读屏障（读标志位后访问数据）
  示例：uint32_t v = g_ms.load(memory_order_relaxed);

【atomic<T>::store(val, memory_order)】
  作用：原子写入值
  常用 order：
    memory_order_relaxed  — 只保证原子性
    memory_order_release  — 写屏障（写数据后设置标志位）
  示例：g_flag.store(true, memory_order_release);

【atomic<T>::fetch_add(delta, memory_order)】
  作用：原子自增，返回旧值
  示例：uint32_t old = g_ms.fetch_add(1, memory_order_relaxed);

【volatile 变量】
  语法：volatile type var;
        volatile type* ptr;        // 指向 volatile 数据的指针
        volatile uint32_t* const REG = (volatile uint32_t*)0x40000000;
  注意：不保证原子性，不能替代 atomic 或关中断

【DISABLE_IRQ / ENABLE_IRQ】
  作用：保护临界区
  嵌入式真实实现（ARM Cortex-M）：
    __disable_irq();  // 设置 PRIMASK = 1
    // ... 临界区 ...
    __enable_irq();   // 清除 PRIMASK = 0
  注意：关中断时间必须 < 1μs

)";
}


// ============================================================
// 第五部分：练习题（不提供答案，独立思考）
// ============================================================

void part5_exercises() {
    cout << string(60, '=') << "\n";
    cout << "第五部分：练习题\n";
    cout << string(60, '=') << "\n";

    cout << R"(
1. 软件防抖（Debounce）
   要求：
   - 按键 ISR 设置标志位和时间戳（用 g_systick_ms）
   - 主循环：距上次按下超过 20ms 才视为有效按键
   提示：记录上次有效按下的 tick，比较差值

2. 模板化环形缓冲区
   要求：
   - template<typename T, size_t N> class RingBuffer
   - push()（ISR 安全）、pop()（主循环，关中断保护）
   - 满返回 false，空返回 false
   提示：N 用编译期检查确保是2的幂次（static_assert）

3. 软件定时器（SoftTimer）
   要求：
   - SysTick ISR 每 1ms 调用 softimer_tick()
   - softimer_start(id, ms, callback)：ms 后调用 callback
   - ISR 只做倒计时，主循环 softimer_process() 执行回调
   提示：用数组存定时器状态，避免在 ISR 中调用回调

4. 分析并修复竞争条件
   代码：
     volatile uint32_t g_adc = 0;
     void ADC_ISR()    { g_adc = read_reg(); }
     void main_loop()  { uint32_t v = g_adc; use(v); }
   问题：在哪种 MCU 上存在竞争？如何修复？

)";
}


int main() {
    part1_concepts();
    demo_flag_pattern();
    demo_ring_buffer();
    demo_atomic();
    part3_traps();
    part4_cards();
    part5_exercises();
    return 0;
}
