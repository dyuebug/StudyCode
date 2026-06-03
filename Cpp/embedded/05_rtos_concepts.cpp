// ============================================================
// 嵌入式 RTOS 基础概念 - 超详细注释版
// 日期：2026-06-03（第57天）
// 难度：⭐⭐ 中级
// 编译：g++ -std=c++17 -Wall -pthread -o 05_rtos 05_rtos_concepts.cpp
// ============================================================

/*
【核心概念】

是什么？
- RTOS（Real-Time Operating System）：实时操作系统
- 管理多个任务的调度、同步、通信

裸机（Bare-metal）vs RTOS：
- 裸机：单一 while(1) 主循环 + 中断，结构简单，适合小项目
- RTOS：多任务并发，每个任务独立栈，适合复杂系统

核心特点：
- 任务（Task）：独立执行单元，有自己的栈
- 调度器（Scheduler）：决定哪个任务运行
- 同步原语：互斥量（Mutex）、信号量（Semaphore）、事件组
- 通信原语：消息队列（Queue）
*/

#include <iostream>
#include <thread>
#include <mutex>
#include <semaphore>
#include <queue>
#include <atomic>
#include <chrono>
#include <functional>
#include <string>
using namespace std;
using namespace chrono;


// ============================================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================================

void part1_concepts() {
    cout << string(60, '=') << "\n";
    cout << "第一部分：核心概念详解\n";
    cout << string(60, '=') << "\n";

    cout << R"(
【裸机 vs RTOS 对比】

裸机（Bare-metal）：
  while(true) {
      if (uart_flag) handle_uart();   // 轮询
      if (btn_flag)  handle_button();
      update_display();
  }
  优点：无额外开销，代码简单，确定性强
  缺点：任务耦合，优先级难以保证，实时性差

RTOS：
  task_uart()    { while(1) { wait_semaphore(); handle_uart(); } }
  task_button()  { while(1) { wait_semaphore(); handle_btn();  } }
  task_display() { while(1) { vTaskDelay(16); update(); } }
  优点：任务解耦，优先级调度，实时性好
  缺点：内存开销（每任务需要独立栈），调试复杂

【调度策略】
  抢占式（Preemptive）：高优先级任务就绪时立即抢占低优先级
  协作式（Cooperative）：任务主动让出CPU（yield/delay）
  FreeRTOS 默认：抢占式 + 时间片轮转（同优先级任务）

【RTOS 核心原语】
  Mutex（互斥量）   — 保护共享资源，同一时刻只有一个任务访问
  Semaphore（信号量）— 任务间同步，如"ISR通知任务有数据"
  Queue（消息队列） — 任务间传递数据，线程安全的 FIFO

)";
}


// ============================================================
// 第二部分：基础操作实践（用 C++ 标准库模拟 RTOS 原语）
// ============================================================

// ------------------------------------------------------------
// 2.1 互斥量（Mutex）——保护共享资源
// ------------------------------------------------------------

// 模拟共享资源：多个任务都要写的日志缓冲区
struct SharedLog {
    mutex mtx;            // 互斥量：同一时刻只有一个任务能写
    string buffer;        // 共享缓冲区
    int    entry_count = 0;

    // 线程安全的写入（任意任务调用）
    void append(const string& msg) {
        lock_guard<mutex> lock(mtx);  // 构造时加锁，析构时自动解锁（RAII）
        buffer += msg + "\n";
        ++entry_count;
    }

    void print() {
        lock_guard<mutex> lock(mtx);
        cout << "日志内容（共 " << entry_count << " 条）：\n" << buffer;
    }
};

SharedLog g_log;

void task_sensor(int id, int samples) {
    // 模拟传感器任务：每隔一段时间采样并记录
    for (int i = 0; i < samples; ++i) {
        this_thread::sleep_for(milliseconds(5));
        g_log.append("传感器" + to_string(id) + " 采样=" + to_string(id * 10 + i));
    }
}

void demo_mutex() {
    cout << string(60, '-') << "\n";
    cout << "2.1 互斥量演示（多任务写共享日志）\n";
    cout << string(60, '-') << "\n";

    // 启动两个"任务"（用线程模拟）
    thread t1(task_sensor, 1, 3);
    thread t2(task_sensor, 2, 3);
    t1.join();
    t2.join();

    g_log.print();
}


// ------------------------------------------------------------
// 2.2 二值信号量（Binary Semaphore）——ISR 通知任务
// ------------------------------------------------------------

// 嵌入式最典型模式：ISR 给信号量，任务等待信号量处理数据
// 对比 volatile 标志位：信号量由 RTOS 管理，任务等待时不占 CPU

// C++20 counting_semaphore<1> = 二值信号量
counting_semaphore<1> g_uart_sem{0};  // 初始计数=0（无数据）
atomic<uint8_t>       g_uart_byte{0};

// 模拟 UART ISR：给信号量（真实嵌入式：xSemaphoreGiveFromISR）
void simulate_uart_isr(uint8_t data) {
    g_uart_byte.store(data, memory_order_release);
    g_uart_sem.release();  // 信号量 +1，唤醒等待的任务
}

// UART 处理任务：阻塞等待信号量（不占 CPU）
void task_uart_handler() {
    for (int i = 0; i < 3; ++i) {
        g_uart_sem.acquire();  // 阻塞等待，直到 ISR 给信号量
        uint8_t data = g_uart_byte.load(memory_order_acquire);
        cout << "[UART任务] 收到数据：'" << (char)data << "'\n";
    }
}

void demo_semaphore() {
    cout << string(60, '-') << "\n";
    cout << "2.2 二值信号量演示（ISR → 任务）\n";
    cout << string(60, '-') << "\n";

    // 启动 UART 处理任务（阻塞等待）
    thread uart_task(task_uart_handler);

    // 模拟 ISR 触发（延迟一点，让 uart_task 先进入等待）
    this_thread::sleep_for(milliseconds(10));
    simulate_uart_isr('A');
    this_thread::sleep_for(milliseconds(10));
    simulate_uart_isr('B');
    this_thread::sleep_for(milliseconds(10));
    simulate_uart_isr('C');

    uart_task.join();
}


// ------------------------------------------------------------
// 2.3 消息队列（Message Queue）——任务间传递数据
// ------------------------------------------------------------

// 线程安全的消息队列（RTOS 中：xQueueSend / xQueueReceive）
template<typename T>
class SafeQueue {
public:
    // 生产者调用（可在 ISR 或任务中）
    void push(T val) {
        {
            lock_guard<mutex> lock(mtx_);
            q_.push(move(val));
        }
        sem_.release();  // 通知消费者有新数据
    }

    // 消费者调用（任务中，阻塞等待）
    T pop() {
        sem_.acquire();  // 等待数据
        lock_guard<mutex> lock(mtx_);
        T val = move(q_.front());
        q_.pop();
        return val;
    }

private:
    queue<T>                  q_;
    mutex                     mtx_;
    counting_semaphore<65535> sem_{0};  // 计数信号量，最大65535条消息
};

// 消息结构：任务间传递的数据包
struct SensorMsg {
    int    sensor_id;
    float  value;
    string timestamp;
};

SafeQueue<SensorMsg> g_sensor_queue;

// 生产者任务：采集传感器数据，放入队列
void task_producer() {
    const char* timestamps[] = {"10:00:01", "10:00:02", "10:00:03"};
    for (int i = 0; i < 3; ++i) {
        this_thread::sleep_for(milliseconds(10));
        SensorMsg msg{i + 1, 23.5f + i, timestamps[i]};
        cout << "[生产者] 发送传感器" << msg.sensor_id << " 数据\n";
        g_sensor_queue.push(msg);
    }
}

// 消费者任务：从队列取数据，处理（如上传、存储）
void task_consumer() {
    for (int i = 0; i < 3; ++i) {
        SensorMsg msg = g_sensor_queue.pop();  // 阻塞等待
        cout << "[消费者] 处理传感器" << msg.sensor_id
             << " 数据=" << msg.value
             << " @" << msg.timestamp << "\n";
    }
}

void demo_queue() {
    cout << string(60, '-') << "\n";
    cout << "2.3 消息队列演示（生产者-消费者）\n";
    cout << string(60, '-') << "\n";

    thread producer(task_producer);
    thread consumer(task_consumer);
    producer.join();
    consumer.join();
}


// ============================================================
// 🔍 深入理解：FreeRTOS API 对照（可跳过）
// ============================================================

/*
C++ 标准库     →  FreeRTOS API
─────────────────────────────────────────────────────
thread          →  xTaskCreate(func, name, stack, param, priority, handle)
mutex           →  xSemaphoreCreateMutex()
                   xSemaphoreTake(mtx, portMAX_DELAY)
                   xSemaphoreGive(mtx)
lock_guard      →  手动 Take/Give（FreeRTOS 无 RAII 封装）
counting_semaphore → xSemaphoreCreateCounting(max, init)
                     xSemaphoreGive / xSemaphoreTake
                     xSemaphoreGiveFromISR（ISR 中使用）
SafeQueue<T>    →  xQueueCreate(len, sizeof(T))
                   xQueueSend(q, &item, timeout)
                   xQueueReceive(q, &item, portMAX_DELAY)

sleep_for       →  vTaskDelay(pdMS_TO_TICKS(ms))
                   vTaskDelayUntil（绝对时间，用于周期任务）

【FreeRTOS 任务创建示例】
  void uart_task(void* param) {
      while (1) {
          if (xSemaphoreTake(uart_sem, portMAX_DELAY) == pdTRUE) {
              // 处理数据
          }
      }
  }
  xTaskCreate(uart_task, "UART", 128, NULL, 3, NULL);
  // 参数：函数, 名称, 栈大小(words), 参数, 优先级, 句柄

【优先级设计原则】
  - ISR 给信号量/队列，立刻唤醒对应任务
  - 关键实时任务（如电机控制）：高优先级（如 5）
  - 通信任务（UART 解析）：中优先级（如 3）
  - 显示/日志任务：低优先级（如 1）
  - Idle 任务：优先级 0（永远就绪，做垃圾回收）
*/


// ============================================================
// 第三部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================================

void part3_traps() {
    cout << string(60, '=') << "\n";
    cout << "第三部分：常见错误和陷阱\n";
    cout << string(60, '=') << "\n";

    cout << R"(
1. 死锁（Deadlock）
──────────────────────────────────────
❌ 任务A：lock(mtx1); lock(mtx2);
   任务B：lock(mtx2); lock(mtx1);
   → A 持有 mtx1 等 mtx2，B 持有 mtx2 等 mtx1，永久阻塞

✅ 统一加锁顺序：所有任务都按 mtx1 → mtx2 顺序加锁
✅ 使用 try_lock + 超时，加锁失败则释放已持有的锁重试
✅ 减少同时持有多个锁的情况（锁的粒度设计）

2. 在 ISR 中调用普通 RTOS API
──────────────────────────────────────
❌ void UART_ISR() { xSemaphoreGive(sem); }
   // xSemaphoreGive 不能在 ISR 中调用！

✅ void UART_ISR() {
       BaseType_t woken = pdFALSE;
       xSemaphoreGiveFromISR(sem, &woken);   // ISR 专用版本
       portYIELD_FROM_ISR(woken);            // 如有高优先级任务就绪则立即调度
   }

原因：普通 API 内部可能会关调度器或访问不可重入的数据结构。

3. 栈溢出（Stack Overflow）
──────────────────────────────────────
❌ xTaskCreate(my_task, "Task", 64, ...);  // 栈太小（64 words = 256字节）
   // my_task 内部调用了 sprintf，栈溢出，系统崩溃

✅ 使用 FreeRTOS 栈溢出检测：configCHECK_FOR_STACK_OVERFLOW = 2
   实现 vApplicationStackOverflowHook() 报告哪个任务溢出
✅ 调试阶段设置较大栈，用 uxTaskGetStackHighWaterMark() 查看最大使用量

4. 优先级反转（Priority Inversion）
──────────────────────────────────────
场景：低优先级任务持有互斥量，高优先级任务等待该互斥量，
      中优先级任务抢占了低优先级任务，导致高优先级任务迟迟得不到锁

✅ 使用优先级继承互斥量（Priority Inheritance Mutex）：
   FreeRTOS 默认互斥量支持优先级继承，低优先级任务持锁期间临时提升优先级

5. 消息队列满时阻塞 ISR
──────────────────────────────────────
❌ void ISR() { xQueueSend(q, &data, portMAX_DELAY); }
   // portMAX_DELAY 表示永久等待，ISR 中绝对不能阻塞！

✅ void ISR() {
       BaseType_t woken;
       xQueueSendFromISR(q, &data, &woken);  // 满时直接丢弃，不等待
       portYIELD_FROM_ISR(woken);
   }

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
【counting_semaphore<Max>】（C++20）
  counting_semaphore<1> sem{0};  — 二值信号量，初始计数0
  sem.release()                  — 给信号量（+1），对应 xSemaphoreGive
  sem.acquire()                  — 等信号量（-1），阻塞，对应 xSemaphoreTake
  sem.try_acquire()              — 非阻塞尝试

【mutex + lock_guard】
  mutex mtx;
  lock_guard<mutex> lock(mtx);   — RAII 加锁，作用域结束自动解锁
  unique_lock<mutex> ul(mtx);    — 可以手动 unlock/re-lock（条件变量用）

【SafeQueue<T>（本文实现）】
  q.push(val)                    — 生产者放入数据（线程安全）
  q.pop()                        → 消费者取数据（阻塞等待）

【FreeRTOS 常用 API 对照】
  xTaskCreate(fn, name, stack, param, prio, &handle)
  vTaskDelay(pdMS_TO_TICKS(ms))            — 相对延时
  vTaskDelayUntil(&last, pdMS_TO_TICKS(ms)) — 周期任务（精确）
  xSemaphoreCreateMutex()
  xSemaphoreCreateBinary()
  xSemaphoreGiveFromISR(sem, &woken)       — ISR 中给信号量
  xQueueCreate(length, item_size)
  xQueueSendFromISR(q, &item, &woken)      — ISR 中发消息

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
1. 实现周期任务（Periodic Task）
   要求：
   - 模拟 vTaskDelayUntil：任务每隔固定时间执行一次（精确周期）
   - 比较 sleep_for（相对延时）和 sleep_until（绝对时间）的差别
   提示：auto wake = steady_clock::now(); wake += 100ms; sleep_until(wake);

2. 模拟优先级调度
   要求：
   - 3个任务，优先级不同（高/中/低）
   - 用 SafeQueue 接收"就绪通知"，高优先级任务优先出队
   - priority_queue<Task, compare_by_priority> 实现
   提示：用 priority_queue 替代普通 queue

3. 用消息队列实现 UART 命令解析器
   要求：
   - ISR 线程：将收到的字节 push 到队列
   - 解析任务：从队列取字节，拼成命令字符串（遇'\n'完整命令）
   - 执行任务：解析命令字符串，分发处理（LED_ON / LED_OFF / STATUS）
   提示：两个队列：字节队列（ISR→解析）+ 命令队列（解析→执行）

4. 生产者-消费者限速
   要求：
   - 生产者速度 > 消费者速度时，SafeQueue 达到 max_size 后生产者等待
   - 添加 max_size 参数，push() 在队列满时阻塞（不是丢弃）
   提示：增加一个反向信号量（初始计数=max_size），push 前 acquire

)";
}


int main() {
    part1_concepts();
    demo_mutex();
    cout << "\n";
    demo_semaphore();
    cout << "\n";
    demo_queue();
    part3_traps();
    part4_cards();
    part5_exercises();
    return 0;
}
