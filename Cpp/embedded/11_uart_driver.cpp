// ============================================================
// 嵌入式 UART 驱动 - 超详细注释版
// 日期：2026-06-03（第59天）
// 难度：⭐⭐ 中级
// 编译：g++ -std=c++17 -Wall -pthread -o 11_uart 11_uart_driver.cpp
// ============================================================

/*
【核心概念】

是什么？
- UART（Universal Asynchronous Receiver-Transmitter）：异步串行通信
- 两根线：TX（发送）/ RX（接收），全双工
- 无时钟线，双方约定相同波特率

帧格式（8N1 最常见）：
  [起始位1bit][数据位8bit][停止位1bit] = 10bit/字节
  波特率 115200 → 每秒 11520 字节

为什么重要？
- 调试输出（printf → UART）
- 与 PC 通信（串口调试助手）
- 传感器/模块通信（GPS、蓝牙、Wi-Fi模块）
*/

#include <iostream>
#include <cstdint>
#include <cstring>
#include <atomic>
#include <thread>
#include <chrono>
#include <functional>
#include <semaphore>
using namespace std;
using namespace chrono;


// ============================================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================================

void part1_concepts() {
    cout << string(60, '=') << "\n";
    cout << "第一部分：UART 核心概念\n";
    cout << string(60, '=') << "\n";

    cout << R"(
【UART 帧格式（8N1）】

空闲状态：TX 线保持 HIGH（1）
发送一字节 0x41（'A' = 0b01000001）：

  ___     ___________     _____________
     |   |           |   |
     |___|           |___|
  起始  D0 D1 D2 D3 D4 D5 D6 D7  停止
   0    1  0  0  0  0  0  0  1    1

- 起始位（Start Bit）：LOW，告知接收方数据开始
- 数据位（Data Bits）：LSB 先发，通常8位
- 停止位（Stop Bit） ：HIGH，为下一帧做准备
- 无校验（N = None）

【波特率 vs 实际数据速率】
  波特率 9600   → 960 字节/秒（8N1：每字节10bit）
  波特率 115200 → 11520 字节/秒
  波特率 1M     → 100000 字节/秒

【三种接收方式】
  轮询（Polling）：主循环不断检查接收寄存器，CPU占用高
  中断（Interrupt）：收到字节触发ISR，存入缓冲区，主循环处理
  DMA：硬件自动搬运数据到内存，完成后中断通知，CPU零参与（最高效）

)";
}


// ============================================================
// 第二部分：UART 驱动实现
// ============================================================

// ------------------------------------------------------------
// 2.1 环形缓冲区（接收缓冲）
// ------------------------------------------------------------

// UART 接收：ISR 写入，主循环读取
// 使用环形缓冲区解耦生产者（ISR）和消费者（主循环）

constexpr size_t UART_RX_BUF_SIZE = 64;  // 必须是2的幂

struct UartRxBuf {
    volatile uint8_t data[UART_RX_BUF_SIZE];
    volatile uint8_t head = 0;  // 读指针（主循环移动）
    volatile uint8_t tail = 0;  // 写指针（ISR 移动）

    // ISR 调用：写入一字节
    bool push(uint8_t byte) {
        uint8_t next = (tail + 1) & (UART_RX_BUF_SIZE - 1);  // 位与替代取模
        if (next == head) return false;  // 缓冲区满，丢弃
        data[tail] = byte;
        tail = next;
        return true;
    }

    // 主循环调用：读取一字节
    bool pop(uint8_t* out) {
        if (head == tail) return false;  // 缓冲区空
        *out = data[head];
        head = (head + 1) & (UART_RX_BUF_SIZE - 1);
        return true;
    }

    uint8_t available() const {
        return (tail - head) & (UART_RX_BUF_SIZE - 1);
    }
};


// ------------------------------------------------------------
// 2.2 UART 抽象接口
// ------------------------------------------------------------

class IUart {
public:
    virtual ~IUart() = default;

    // 发送一字节（轮询，等待发送完成）
    virtual void sendByte(uint8_t byte) = 0;

    // 发送字节数组
    virtual void send(const uint8_t* data, size_t len) {
        for (size_t i = 0; i < len; ++i) sendByte(data[i]);
    }

    // 发送 C 字符串
    void print(const char* str) {
        while (*str) sendByte(static_cast<uint8_t>(*str++));
    }

    void println(const char* str) { print(str); sendByte('\n'); }

    // 接收缓冲区中可读字节数
    virtual uint8_t available() const = 0;

    // 读取一字节（需先检查 available()）
    virtual bool readByte(uint8_t* out) = 0;

    // 读取直到遇到终止字符（如 '\n'），填入 buf，返回实际读取字节数
    size_t readLine(char* buf, size_t max_len, char terminator = '\n') {
        size_t n = 0;
        uint8_t byte;
        while (n < max_len - 1 && readByte(&byte)) {
            if (byte == static_cast<uint8_t>(terminator)) break;
            buf[n++] = static_cast<char>(byte);
        }
        buf[n] = '\0';
        return n;
    }
};


// ------------------------------------------------------------
// 2.3 MockUart：PC 端模拟实现（带中断接收模拟）
// ------------------------------------------------------------

class MockUart : public IUart {
public:
    explicit MockUart(const char* name, uint32_t baudrate = 115200)
        : name_(name), baudrate_(baudrate) {
        cout << "[UART:" << name_ << "] 初始化，波特率=" << baudrate_ << "\n";
    }

    void sendByte(uint8_t byte) override {
        // 模拟发送：打印字符
        cout << "[UART:" << name_ << " TX] " << (char)byte;
    }

    uint8_t available() const override {
        return rx_buf_.available();
    }

    bool readByte(uint8_t* out) override {
        return rx_buf_.pop(out);
    }

    // 测试辅助：模拟 UART ISR 接收到数据
    // 真实嵌入式中，这部分由硬件中断自动完成
    void simulateRx(const char* data) {
        while (*data) {
            bool ok = rx_buf_.push(static_cast<uint8_t>(*data++));
            if (!ok) {
                cout << "[UART:" << name_ << "] ⚠️ RX缓冲区满！\n";
                break;
            }
        }
        // 通知主循环有数据（真实系统：中断标志 or 信号量）
        if (on_rx_) on_rx_();
    }

    // 注册接收回调（ISR 接收到数据后调用）
    void setRxCallback(function<void()> cb) { on_rx_ = cb; }

private:
    const char*      name_;
    uint32_t         baudrate_;
    UartRxBuf        rx_buf_;
    function<void()> on_rx_;
};


// ------------------------------------------------------------
// 2.4 UART 命令解析器（主循环处理接收数据）
// ------------------------------------------------------------

// 命令解析器：从 UART 逐字节读取，组装成命令字符串，解析执行
class CommandParser {
public:
    explicit CommandParser(IUart& uart) : uart_(uart) {}

    // 主循环调用：处理接收缓冲区中的数据
    void update() {
        uint8_t byte;
        while (uart_.readByte(&byte)) {
            if (byte == '\n' || byte == '\r') {
                // 收到换行，处理完整命令
                if (cmd_len_ > 0) {
                    cmd_buf_[cmd_len_] = '\0';
                    dispatch(cmd_buf_);
                    cmd_len_ = 0;
                }
            } else if (cmd_len_ < sizeof(cmd_buf_) - 1) {
                cmd_buf_[cmd_len_++] = static_cast<char>(byte);
            }
            // 超过缓冲区长度：丢弃（防止溢出）
        }
    }

    // 注册命令处理函数
    // cmd：命令前缀，handler：处理函数（参数为完整命令字符串）
    void on(const char* cmd, function<void(const char*)> handler) {
        if (handler_count_ < MAX_HANDLERS) {
            handlers_[handler_count_++] = {cmd, handler};
        }
    }

private:
    IUart& uart_;
    char   cmd_buf_[64] = {};
    size_t cmd_len_     = 0;

    struct Handler {
        const char*                  prefix;
        function<void(const char*)>  fn;
    };
    static constexpr size_t MAX_HANDLERS = 8;
    Handler handlers_[MAX_HANDLERS];
    size_t  handler_count_ = 0;

    void dispatch(const char* cmd) {
        cout << "\n[CMD] 收到命令：\"" << cmd << "\"\n";
        for (size_t i = 0; i < handler_count_; ++i) {
            // strncmp：比较前 n 个字符，匹配前缀
            if (strncmp(cmd, handlers_[i].prefix,
                        strlen(handlers_[i].prefix)) == 0) {
                handlers_[i].fn(cmd);
                return;
            }
        }
        cout << "[CMD] 未知命令\n";
    }
};


// ------------------------------------------------------------
// 2.5 完整演示
// ------------------------------------------------------------

void demo_uart() {
    cout << string(60, '=') << "\n";
    cout << "演示：UART 驱动 + 命令解析器\n";
    cout << string(60, '=') << "\n";

    MockUart uart("USART1", 115200);

    // 创建命令解析器
    CommandParser parser(uart);

    // 注册命令处理函数
    parser.on("LED_ON",  [](const char*) { cout << "  → LED 已打开\n"; });
    parser.on("LED_OFF", [](const char*) { cout << "  → LED 已关闭\n"; });
    parser.on("STATUS",  [](const char*) { cout << "  → 系统正常运行\n"; });
    parser.on("BAUD:",   [](const char* cmd) {
        // 解析波特率参数：BAUD:9600
        uint32_t baud = atoi(cmd + 5);  // 跳过 "BAUD:" 5个字符
        cout << "  → 设置波特率=" << baud << "\n";
    });

    // 设置接收回调（ISR 通知主循环）
    bool has_data = false;
    uart.setRxCallback([&]() { has_data = true; });

    // 模拟 UART 接收到命令（真实场景：由硬件中断触发）
    cout << "\n--- 模拟接收命令 ---\n";
    uart.simulateRx("LED_ON\n");
    uart.simulateRx("STATUS\n");
    uart.simulateRx("BAUD:9600\n");
    uart.simulateRx("LED_OFF\n");
    uart.simulateRx("UNKNOWN\n");

    // 主循环处理
    cout << "\n--- 主循环处理 ---\n";
    parser.update();

    // 演示发送
    cout << "\n--- 发送响应 ---\n";
    uart.println("OK");
    uart.print("Version:1.0\n");
    cout << "\n";
}


// ============================================================
// 🔍 深入理解：DMA 接收（可跳过）
// ============================================================

/*
DMA（Direct Memory Access）接收：
  - 配置 DMA：源地址=UART数据寄存器，目标地址=RAM缓冲区，长度=N
  - 硬件自动搬运，每收一字节 DMA 自动写入 RAM，无需 ISR
  - 传输完成（N字节）或半传输时触发 DMA 中断

优势：
  波特率 1Mbps，每字节10bit → 100000字节/s → 每 10μs 一字节
  用中断：100000次/秒中断 → CPU 压力大
  用 DMA：0次中断（传输期间）→ CPU 完全空闲，可做其他任务

STM32 HAL DMA 接收示例：
  HAL_UART_Receive_DMA(&huart1, rx_buf, BUF_SIZE);
  // DMA 中断回调：
  void HAL_UART_RxCpltCallback(UART_HandleTypeDef* h) {
      if (h == &huart1) process_rx_data(rx_buf, BUF_SIZE);
  }

空闲中断（IDLE Line Detection）：
  UART 检测到总线空闲（一帧时间无数据）→ 触发 IDLE 中断
  配合 DMA：不定长数据帧接收（不需要预先知道帧长度）
  void USART1_IRQHandler() {
      if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE)) {
          uint32_t received = BUF_SIZE - __HAL_DMA_GET_COUNTER(hdma);
          process(rx_buf, received);
      }
  }
*/


// ============================================================
// 第三部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================================

void part3_traps() {
    cout << string(60, '=') << "\n";
    cout << "第三部分：常见错误和陷阱\n";
    cout << string(60, '=') << "\n";

    cout << R"(
1. 双方波特率不匹配导致乱码
──────────────────────────────────────
❌ PC 串口工具设 115200，MCU 设 9600
   → 接收到的全是乱码（采样时机完全错误）

✅ 双方必须设置完全相同的：波特率、数据位、停止位、奇偶校验
   调试时先用 9600 确认通信，再提高到 115200

2. 接收缓冲区溢出（ISR 写入太快，主循环处理太慢）
──────────────────────────────────────
❌ 高速数据流（1Mbps），主循环处理慢
   → 环形缓冲区满，新数据被丢弃，帧数据残缺

✅ 1. 加大缓冲区（UART_RX_BUF_SIZE）
   2. 提高主循环频率，减少单次处理时间
   3. 使用 DMA，硬件搬运不漏数据

3. 在 ISR 中解析字符串
──────────────────────────────────────
❌ void UART_ISR() {
       char c = USART1->DR;
       if (strcmp(cmd_buf, "LED_ON") == 0) led_on();  // strcmp 在 ISR 中！
   }

✅ void UART_ISR() { rx_buf.push(USART1->DR); }
   // 主循环中调用 parser.update() 解析

4. readline 缓冲区无边界检查
──────────────────────────────────────
❌ void parse() {
       while(uart.readByte(&b)) cmd[i++] = b;  // i 没有上界检查！
   }
   // 超长命令导致栈溢出

✅ while(uart.readByte(&b) && i < sizeof(cmd)-1) cmd[i++] = b;
   cmd[i] = '\0';  // 强制终止

5. printf 重定向到 UART 后在 ISR 中调用
──────────────────────────────────────
❌ void ISR() { printf("收到数据\n"); }
   // printf 内部使用缓冲区和锁，ISR 中调用可能死锁或数据交织

✅ ISR 中只用轻量级日志（设置标志），主循环中 printf

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
【IUart 接口】
  sendByte(uint8_t)               — 发送一字节（轮询等待）
  send(const uint8_t*, size_t)    — 发送字节数组
  print(const char*)              — 发送 C 字符串
  println(const char*)            — 发送字符串 + 换行
  available() → uint8_t           — 接收缓冲区可读字节数
  readByte(uint8_t*) → bool       — 读一字节，空返回 false
  readLine(buf, max, '\n') → size — 读到换行符，返回字节数

【UartRxBuf（环形缓冲区）】
  push(uint8_t) → bool            — ISR 调用，满时返回 false
  pop(uint8_t*) → bool            — 主循环调用，空时返回 false
  available() → uint8_t           — 可读字节数

【CommandParser】
  on(prefix, handler)             — 注册命令前缀和处理函数
  update()                        — 主循环调用，驱动命令解析

【STM32 HAL UART API】
  HAL_UART_Transmit(&h, data, len, timeout)   — 轮询发送
  HAL_UART_Transmit_IT(&h, data, len)         — 中断发送
  HAL_UART_Transmit_DMA(&h, data, len)        — DMA 发送
  HAL_UART_Receive_IT(&h, buf, 1)             — 中断接收1字节
  HAL_UART_Receive_DMA(&h, buf, len)          — DMA 接收
  HAL_UART_RxCpltCallback(&h)                 — 接收完成回调（重写）

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
1. 实现 printf 风格的 UART 输出
   要求：
   - void uart_printf(IUart& uart, const char* fmt, ...)
   - 支持 %d、%s、%x 格式符
   - 内部用 char buf[128]，snprintf 格式化后 uart.send() 发送
   提示：#include <cstdarg>，va_list + vsnprintf

2. 实现 UART 数据包协议解析
   要求：
   - 帧格式：[AA 55][长度1B][命令1B][数据NB][校验1B]
   - CommandParser 改为状态机：WAIT_HEADER1 → WAIT_HEADER2 → WAIT_LEN → ...
   - 校验：所有字节异或（XOR）
   提示：enum class ParseState，update() 中 switch(state_) 处理每个字节

3. 实现超时接收
   要求：
   - readWithTimeout(buf, max_len, timeout_ms) → size_t
   - 距上次收到字节超过 timeout_ms 则认为一帧结束
   - 用 steady_clock::now() 计时
   提示：记录上次收到字节的时间点，循环中比较当前时间

4. 实现 UART 回环测试（Loopback）
   要求：
   - MockUart 支持：发送的数据自动进入接收缓冲区（TX→RX）
   - 验证：发送 "Hello" → readLine() 读到 "Hello"
   - 用于测试上层协议，无需真实硬件
   提示：sendByte() 中同时调用 rx_buf_.push(byte)

)";
}


int main() {
    part1_concepts();
    demo_uart();
    part3_traps();
    part4_cards();
    part5_exercises();
    return 0;
}
