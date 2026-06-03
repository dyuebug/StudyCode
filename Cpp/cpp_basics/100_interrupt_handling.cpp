// 嵌入式中断处理 - 超详细注释版
// 日期：2026-06-03（第57天）
// 难度：⭐⭐ 中级

/*
【核心概念】

是什么？
- 中断（Interrupt）是硬件/软件向CPU发出的信号，要求立即暂停当前任务处理紧急事件
- ISR（Interrupt Service Routine，中断服务例程）是响应中断的特殊函数

为什么需要？
- 避免CPU不停轮询（polling）浪费资源
- 实现对实时事件的快速响应（如按键、串口数据到来）
- 是嵌入式实时系统的核心机制

核心特点：
- ISR必须短小精悍，不能阻塞
- 与主循环共享数据必须用volatile
- 临界区需要关中断保护
*/

#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>
#include <cstdint>

using namespace std;

// ============================================================
// 宏定义：模拟嵌入式特有语法（在PC上运行的模拟版本）
// ============================================================

// __attribute__((interrupt)) 告诉编译器这是一个中断处理函数
// 真实嵌入式中会保存/恢复寄存器，这里用宏模拟
#define ISR_HANDLER           // 实际嵌入式：__attribute__((interrupt))

// 关中断/开中断：嵌入式中用于保护临界区
// 真实实现：直接操作CPU状态寄存器
#define DISABLE_IRQ()   do { /* 模拟：关全局中断 */ } while(0)
#define ENABLE_IRQ()    do { /* 模拟：开全局中断 */ } while(0)

// 内存屏障：防止编译器/CPU重排序
#define MEMORY_BARRIER()  asm volatile("" ::: "memory")  // GCC内存屏障


// ============================================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================================

int main_part1() {
    cout << string(60, '=') << endl;
    cout << "第一部分：核心概念详解" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【中断处理基础概念】

1. 中断类型（嵌入式常见）：
   - 外部中断（EXTI）：GPIO引脚电平变化触发，如按键检测
   - 定时器中断（TIM）：定时时间到触发，如周期性任务
   - UART接收中断（USART）：串口收到数据触发，如命令解析
   - ADC转换完成中断：模数转换完成后触发

2. ISR设计黄金原则：
   ✅ 短小精悍：执行时间要极短（通常 < 1μs）
   ✅ 设置标志位：在ISR中设置volatile标志，主循环处理实际工作
   ✅ 不调用阻塞函数：禁止在ISR中调用printf、malloc、delay等
   ✅ 不加互斥锁：ISR中不能等待锁
   ❌ 禁止：复杂计算、内存分配、无限循环

3. volatile关键字的作用：
   - 告诉编译器"不要优化掉对该变量的读写"
   - 确保每次访问都从内存读取（不使用寄存器缓存）
   - ISR修改的变量、主循环读取时，必须加volatile

4. 竞争条件（Race Condition）：
   - 问题：主循环读取一个多字节变量，ISR中途修改，读到半新半旧的值
   - 解决：使用atomic（原子操作）或关中断保护临界区
)" << endl;

    return 0;
}


// ============================================================
// 第二部分：基础操作实践
// ============================================================

// ------------------------------------------------------------
// 2.1 volatile标志位通信模式（最常用的ISR通信方式）
// ------------------------------------------------------------

// volatile告诉编译器：这个变量可能在任何时刻被外部（ISR）修改
// 不加volatile时编译器可能将变量缓存在寄存器，导致主循环读不到ISR的修改
volatile bool g_uart_received = false;   // UART接收中断标志
volatile bool g_button_pressed = false;  // 外部中断（按键）标志
volatile bool g_timer_tick    = false;   // 定时器中断标志

// volatile数组：存储ISR中接收到的数据
volatile uint8_t g_uart_buffer[64] = {};  // 串口接收缓冲区
volatile uint8_t g_uart_buf_len    = 0;   // 已接收字节数

// ------------------------------------------------------------
// 2.2 模拟ISR函数（真实嵌入式中由向量表注册）
// ------------------------------------------------------------

// ISR_HANDLER标记这是中断服务例程
// 原则：只设置标志位，立刻返回，绝不阻塞
ISR_HANDLER void UART_RxISR() {
    // 模拟：从UART数据寄存器读取一字节数据
    // 真实代码：uint8_t data = USART1->DR;
    uint8_t data = 'A';  // 模拟收到字符'A'

    // 将数据存入缓冲区（ISR中可以做少量数据拷贝）
    if (g_uart_buf_len < 64) {
        g_uart_buffer[g_uart_buf_len] = data;  // 存入缓冲区
        g_uart_buf_len++;                       // 更新长度
    }

    // 设置标志位，通知主循环有数据待处理
    // 不在ISR中处理数据，主循环负责解析
    g_uart_received = true;

    // ISR立即返回，不做其他任何事
}

ISR_HANDLER void EXTI_ButtonISR() {
    // 外部中断：按键按下触发
    // 只设置标志，防抖（debounce）由主循环负责
    g_button_pressed = true;
}

ISR_HANDLER void TIM_TickISR() {
    // 定时器中断：每1ms触发一次
    // 只通知主循环时钟节拍到来
    g_timer_tick = true;
}

// ------------------------------------------------------------
// 2.3 主循环：轮询标志位处理事件
// ------------------------------------------------------------

void main_loop_demo() {
    cout << string(60, '=') << endl;
    cout << "第二部分：主循环轮询标志位模式" << endl;
    cout << string(60, '=') << endl;

    // 模拟触发中断（真实系统中由硬件自动触发）
    UART_RxISR();       // 模拟UART收到数据
    EXTI_ButtonISR();   // 模拟按键按下

    // 主循环：不停检查标志位
    // 这是嵌入式裸机（bare-metal）最典型的模式
    int loop_count = 0;
    while (loop_count < 3) {
        // --- 处理UART接收事件 ---
        if (g_uart_received) {
            // 清除标志位：必须在处理前清除，防止漏掉下一次中断
            g_uart_received = false;

            // 将volatile数组数据拷贝到普通局部变量处理
            // 原因：减少对volatile变量的访问，提高效率
            uint8_t len = g_uart_buf_len;
            cout << "主循环：处理UART数据，收到 " << (int)len << " 字节" << endl;

            // 重置缓冲区长度（临界区：需要关中断保护）
            DISABLE_IRQ();   // 关中断，防止此时ISR又修改g_uart_buf_len
            g_uart_buf_len = 0;
            ENABLE_IRQ();    // 开中断
        }

        // --- 处理按键事件 ---
        if (g_button_pressed) {
            g_button_pressed = false;  // 清除标志
            cout << "主循环：检测到按键按下，执行操作" << endl;
        }

        // --- 处理定时器节拍 ---
        if (g_timer_tick) {
            g_timer_tick = false;
            // 执行周期性任务（如传感器采样、LED闪烁）
        }

        loop_count++;
    }
}

// ------------------------------------------------------------
// 2.4 atomic原子操作：防止多字节变量的竞争条件
// ------------------------------------------------------------

// 错误示范：32位计数器在8位MCU上读写不是原子操作
volatile uint32_t g_tick_count_bad = 0;  // ❌ 可能读到中间状态

// 正确示范：使用atomic保证原子性
// atomic<uint32_t>保证读写操作不会被中断打断（硬件支持或编译器生成原子指令）
atomic<uint32_t> g_tick_count_good{0};   // ✅ 原子操作，安全

// 模拟定时器ISR（在PC上用线程模拟）
void timer_isr_thread() {
    for (int i = 0; i < 5; i++) {
        this_thread::sleep_for(chrono::milliseconds(10));
        g_tick_count_good.fetch_add(1, memory_order_relaxed);  // 原子自增
        // memory_order_relaxed：只保证原子性，不需要同步其他内存操作
        // 对于简单计数器够用；如果ISR同时修改其他变量，用memory_order_release
    }
}

void atomic_demo() {
    cout << string(60, '=') << endl;
    cout << "第二部分：atomic原子操作演示" << endl;
    cout << string(60, '=') << endl;

    // 启动模拟定时器线程（模拟硬件定时器中断）
    thread isr_sim(timer_isr_thread);

    // 主循环：读取原子计数器（安全，不会读到中间状态）
    uint32_t last_tick = 0;
    for (int i = 0; i < 3; i++) {
        this_thread::sleep_for(chrono::milliseconds(15));
        uint32_t current = g_tick_count_good.load(memory_order_relaxed);
        // load()：原子读取，memory_order_relaxed对计数器足够安全
        if (current != last_tick) {
            cout << "主循环：系统节拍更新为 " << current << endl;
            last_tick = current;
        }
    }

    isr_sim.join();  // 等待模拟ISR线程结束
    cout << "最终节拍计数：" << g_tick_count_good.load() << endl;
}

// ------------------------------------------------------------
// 2.5 临界区保护：关中断方式
// ------------------------------------------------------------

// 嵌入式中需要原子地读写一个多步骤的数据结构时
// 使用关中断保护临界区

struct RingBuffer {
    volatile uint8_t data[16];  // 环形缓冲区数据，volatile因为ISR会修改
    volatile uint8_t head;      // 读指针
    volatile uint8_t tail;      // 写指针（ISR写入，主循环读取）
    const uint8_t    size = 16; // 缓冲区大小
};

RingBuffer g_ring_buf = {};  // 全局环形缓冲区

// ISR写入一字节（中断上下文，不能等待）
ISR_HANDLER void ringbuf_isr_write(uint8_t byte) {
    uint8_t next_tail = (g_ring_buf.tail + 1) % g_ring_buf.size;
    if (next_tail != g_ring_buf.head) {  // 检查缓冲区未满
        g_ring_buf.data[g_ring_buf.tail] = byte;
        g_ring_buf.tail = next_tail;  // 更新写指针（原子操作，8位MCU安全）
    }
    // 注：tail是8位，8位MCU上赋值是原子的，不需要关中断
}

// 主循环读取一字节（主循环上下文）
bool ringbuf_read(uint8_t* out) {
    if (g_ring_buf.head == g_ring_buf.tail) {
        return false;  // 缓冲区空
    }
    *out = g_ring_buf.data[g_ring_buf.head];  // 读取数据
    // 关中断保护：防止ISR在更新head之前写入同一位置
    DISABLE_IRQ();
    g_ring_buf.head = (g_ring_buf.head + 1) % g_ring_buf.size;
    ENABLE_IRQ();
    return true;
}

void ring_buffer_demo() {
    cout << string(60, '=') << endl;
    cout << "第二部分：环形缓冲区演示" << endl;
    cout << string(60, '=') << endl;

    // 模拟ISR写入数据
    ringbuf_isr_write('H');
    ringbuf_isr_write('i');
    ringbuf_isr_write('!');

    // 主循环读取数据
    uint8_t byte;
    cout << "从环形缓冲区读取：";
    while (ringbuf_read(&byte)) {
        cout << (char)byte;  // 输出字符
    }
    cout << endl;
}


// ============================================================
// 第三部分：深入理解（可选）
// ============================================================

/*
🔍 深入理解：volatile vs atomic（可以跳过，不影响后续学习）

volatile的本质：
- 禁止编译器优化，强制每次从内存读/写
- 不保证原子性（多字节变量读写可能被打断）
- 不产生内存屏障（不阻止CPU乱序执行）
- 适用：单一标志位（bool/uint8_t），在8位MCU上天然原子

atomic的本质：
- 保证操作的原子性（不会被打断读到中间状态）
- 产生内存屏障（防止CPU/编译器重排序）
- 适用：32位/64位计数器、多字节数据结构

嵌入式实践建议：
- 简单标志位（bool）：volatile足够
- 32位计数器（ARM Cortex-M3+）：volatile可以（LDR/STR是原子的）
- 32位计数器（ARM Cortex-M0）：需要关中断或atomic
- 复杂数据结构：关中断保护临界区

ISR延迟（Latency）分析：
- 从中断触发到ISR执行：需要保存寄存器（Cortex-M约12个周期）
- ISR执行时间：越短越好（目标<1μs = <168个时钟周期@168MHz）
- 中断嵌套：高优先级中断可以打断低优先级ISR
*/


// ============================================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================================

void trap_demo() {
    cout << string(60, '=') << endl;
    cout << "第四部分：常见错误和陷阱" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
1. 忘记volatile导致编译器优化消除变量访问
──────────────────────────────────────────
❌ 错误代码：
bool flag = false;  // 没有volatile
while (!flag) { }   // 编译器优化为：while(true) { }（永远不检查flag）

✅ 正确代码：
volatile bool flag = false;  // volatile禁止优化
while (!flag) { }            // 每次循环都从内存读flag

原因：编译器看到flag在主循环内没有被修改（ISR修改不在编译器视野内），
      就将flag读取优化为常量，导致死循环。
预防措施：ISR中使用的所有共享变量都加volatile。

2. 在ISR中调用阻塞函数
──────────────────────────────────────────
❌ 错误代码：
void UART_ISR() {
    printf("收到数据\n");  // printf可能阻塞等待（涉及锁/缓冲）
    HAL_Delay(10);        // delay在ISR中会死锁（delay依赖SysTick中断）
}

✅ 正确代码：
void UART_ISR() {
    g_uart_received = true;  // 只设置标志
    g_uart_data = read_register();  // 最多读取硬件寄存器
}
// 在主循环中调用printf和delay

原因：ISR执行期间可能禁止了其他中断，调用依赖中断的函数会死锁。
预防措施：ISR只做"设置标志+读硬件寄存器"，其他全部在主循环处理。

3. 不清除标志位导致重复处理
──────────────────────────────────────────
❌ 错误代码：
if (g_button_pressed) {
    process_button();  // 处理完没有清除标志
    // g_button_pressed仍为true，下次循环继续触发！
}

✅ 正确代码：
if (g_button_pressed) {
    g_button_pressed = false;  // 先清除标志（防止遗漏下一次中断）
    process_button();
}

原因：标志不清除，每次主循环都会重复处理同一事件。
预防措施：进入if分支第一件事就是清除标志。

4. 多字节变量读写竞争条件
──────────────────────────────────────────
❌ 错误代码（8位MCU读32位变量）：
volatile uint32_t g_count = 0;
// ISR：g_count++;  （需要多条指令：读低16位→读高16位→加法→写回）
// 主循环读到：低16位是新值，高16位是旧值 → 读到错误数据！

✅ 正确代码：
DISABLE_IRQ();
uint32_t snapshot = g_count;  // 关中断期间原子读取
ENABLE_IRQ();

原因：32位操作在8位/16位MCU上需要多条指令，ISR可能在中间打断。
预防措施：关中断保护多字节变量的读写；或在支持原子操作的32位MCU上用atomic。

5. 环形缓冲区索引越界
──────────────────────────────────────────
❌ 错误代码：
uint8_t idx = 0;
void ISR_write(uint8_t data) {
    buf[idx] = data;
    idx++;  // 没有取模，溢出后idx=256回到0，但buf大小只有256，实际OK...
            // 但如果buf大小不是2的幂次，不能靠溢出回绕！
}

✅ 正确代码：
void ISR_write(uint8_t data) {
    uint8_t next = (idx + 1) % BUF_SIZE;  // 取模确保不越界
    if (next != read_idx) {               // 检查缓冲区未满
        buf[idx] = data;
        idx = next;
    }
}

原因：缓冲区大小不是2的幂次时，依赖整数溢出回绕会越界。
预防措施：始终用取模操作，始终检查缓冲区满/空状态。

6. ISR中使用C++异常或动态内存
──────────────────────────────────────────
❌ 错误代码：
void Timer_ISR() {
    auto ptr = new SensorData();  // malloc/new在ISR中极其危险！
    throw std::runtime_error("错误");  // 异常在ISR中行为未定义！
}

✅ 正确代码：
// ISR中只使用静态/全局/栈上局部变量
// 不使用动态内存分配，不抛出异常

原因：malloc内部有锁，ISR调用可能死锁；异常机制在中断上下文行为未定义。
预防措施：嵌入式ISR禁止动态内存分配和C++异常。
)" << endl;
}


// ============================================================
// 第五部分：函数卡片速查
// ============================================================

void function_cards() {
    cout << string(60, '=') << endl;
    cout << "第五部分：函数卡片速查" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【atomic<T>::load()】
语法：T load(memory_order order = memory_order_seq_cst) const noexcept
参数：
  - order：内存顺序语义
    * memory_order_relaxed：只保证原子性，不阻止重排序（计数器用）
    * memory_order_acquire：读屏障，配合release使用（标志位通信用）
    * memory_order_seq_cst：最强保证，顺序一致性（默认，性能最低）
返回：原子变量的当前值
示例：uint32_t val = g_tick.load(memory_order_relaxed);

────────────────────────────────────────────────────────────

【atomic<T>::store()】
语法：void store(T val, memory_order order = memory_order_seq_cst) noexcept
参数：
  - val：要存入的值
  - order：内存顺序（同load，写端通常用memory_order_release）
返回：无
示例：g_flag.store(true, memory_order_release);

────────────────────────────────────────────────────────────

【atomic<T>::fetch_add()】
语法：T fetch_add(T delta, memory_order order = memory_order_seq_cst) noexcept
参数：
  - delta：要增加的值
  - order：内存顺序
返回：加法之前的旧值（原子地执行"读旧值→加法→写回"）
示例：uint32_t old = g_count.fetch_add(1, memory_order_relaxed);

────────────────────────────────────────────────────────────

【DISABLE_IRQ / ENABLE_IRQ（嵌入式宏）】
用途：保护临界区，防止ISR打断多步骤操作
语法：
  DISABLE_IRQ();   // 关全局中断（真实：__disable_irq() 或 PRIMASK寄存器）
  // ... 临界区代码 ...
  ENABLE_IRQ();    // 开全局中断（真实：__enable_irq()）
注意：
  - 关中断时间必须极短（<1μs）
  - 不能在关中断状态下调用任何可能等待的函数
  - 嵌套关中断需要保存/恢复PRIMASK寄存器

────────────────────────────────────────────────────────────

【volatile关键字】
用途：防止编译器优化掉对共享变量的内存访问
语法：volatile 类型 变量名;
      volatile 类型* 指针名;  // 指向volatile数据的指针
适用场景：
  - ISR中修改的标志位、计数器、缓冲区索引
  - 内存映射寄存器（MMIO）：volatile uint32_t* const REG = (uint32_t*)0x40000000;
不适用：不能替代atomic的原子性保证
)" << endl;
}


// ============================================================
// 第六部分：练习题
// ============================================================

void exercises() {
    cout << string(60, '=') << endl;
    cout << "第六部分：练习题" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
1. 实现一个软件防抖（debounce）模块
   要求：
   - 按键ISR中设置volatile标志和时间戳
   - 主循环检查：距离上次按下超过20ms才视为有效按键
   - 用g_tick_count_good作为时间源（每ms加1）
   提示：记录上次有效按下的tick值，比较当前tick与上次tick之差

2. 扩展环形缓冲区为模板类
   要求：
   - template<typename T, size_t N> class RingBuffer
   - 成员：push()（ISR安全，不加锁）、pop()（主循环调用，关中断保护）
   - 满时push()返回false，空时pop()返回false
   提示：head和tail用atomic<size_t>或volatile size_t

3. 实现一个软件定时器（SoftTimer）
   要求：
   - 硬件定时器ISR每1ms调用一次 softimer_tick()
   - softimer_start(id, ms, callback)：注册定时器，ms毫秒后调用callback
   - softimer_tick()：在ISR中调用，倒计时，到期设置标志
   - 主循环调用 softimer_process()：检查到期定时器，执行callback
   提示：用数组存储定时器状态，ISR只修改计数，主循环调用回调

4. 设计一个UART命令解析器
   要求：
   - UART ISR将收到的字节写入环形缓冲区
   - 主循环从缓冲区读取字节，遇到'\n'则解析完整命令
   - 支持命令：LED_ON、LED_OFF、GET_STATUS
   - 每个命令对应一个回调函数
   提示：用状态机解析，避免在ISR中做字符串操作

5. 分析以下代码的竞争条件并修复：
   volatile uint16_t g_adc_value = 0;  // ADC结果（16位）
   // ISR（在16位MCU上运行）：
   void ADC_ISR() { g_adc_value = read_adc_register(); }
   // 主循环：
   uint16_t v = g_adc_value;
   要求：说明在哪种MCU上存在竞争条件，如何用关中断修复
)" << endl;
}


int main() {
    main_part1();
    main_loop_demo();
    atomic_demo();
    ring_buffer_demo();
    trap_demo();
    function_cards();
    exercises();
    return 0;
}

/*
编译命令：
  g++ -std=c++17 -Wall -o 100_interrupt_handling 100_interrupt_handling.cpp
  （Linux/Mac加 -pthread 参数，因为使用了std::thread）
  g++ -std=c++17 -Wall -pthread -o 100_interrupt_handling 100_interrupt_handling.cpp

运行：
  ./100_interrupt_handling   （Linux/Mac）
  100_interrupt_handling.exe （Windows）

注意：本文件在PC上模拟嵌入式中断行为，DISABLE_IRQ/ENABLE_IRQ为空宏。
      真实嵌入式环境（如STM32）中这些宏会替换为实际的ARM汇编指令。
*/
