// ============================================
// 18_debug_techniques.cpp
// 日期：2026-06-06（第61天）
// 主题：嵌入式调试技术
// 难度：⭐⭐⭐ 高级
// 编译：g++ -std=c++17 -o 18_debug 18_debug_techniques.cpp
// ============================================

/*
【核心概念】

是什么？
- 嵌入式系统调试比PC调试更难：无操作系统保护、错误直接死机
- 常用手段：HardFault分析、断言宏、printf重定向、看门狗、Stack Canary

为什么需要？
- 嵌入式系统崩溃时没有栈回溯、没有core dump
- 需要主动埋点、保护、记录，才能定位问题

核心手段：
- HardFault Handler：从栈帧提取PC/LR，定位出错位置
- 断言宏：主动检测非法状态，打印文件/行号后挂起
- 看门狗：硬件定时器，防止程序卡死
- Stack Canary：栈底魔数，检测栈溢出
*/

#include <iostream>
#include <cstdint>
#include <cstring>
#include <cassert>
using namespace std;

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;

static u32 g_tick = 0;
u32 HAL_GetTick() { return g_tick; }

// ============================================
// 第一部分：断言宏 ⭐⭐⭐⭐⭐
// ============================================

/*
【函数卡片】ASSERT 宏

作用：主动检测非法条件，触发时打印位置信息并挂起
语法：ASSERT(condition)
参数：condition — 期望为真的条件
触发：condition 为 false 时触发
*/

// 嵌入式断言宏（真实项目中 while(1) 换成 NVIC_SystemReset()）
#define ASSERT(cond) \
    do { \
        if (!(cond)) { \
            cout << "[ASSERT] 断言失败！" \
                 << " 文件: " << __FILE__ \
                 << " 行号: " << __LINE__ \
                 << " 条件: " #cond << endl; \
            /* while(1); // 真实嵌入式：挂起等待调试器 */ \
        } \
    } while(0)

// 带消息的断言
#define ASSERT_MSG(cond, msg) \
    do { \
        if (!(cond)) { \
            cout << "[ASSERT] " << msg \
                 << " (" << __FILE__ << ":" << __LINE__ << ")" << endl; \
        } \
    } while(0)

void demoAssert() {
    cout << string(60, '-') << endl;
    cout << "1. 断言宏演示" << endl;
    cout << string(60, '-') << endl;

    int buf_size = 256;
    int write_pos = 300;

    // 检测缓冲区越界
    ASSERT(write_pos < buf_size);
    ASSERT_MSG(write_pos < buf_size, "写入位置超出缓冲区范围");

    // 检测空指针
    uint8_t* ptr = nullptr;
    ASSERT_MSG(ptr != nullptr, "指针不能为空");

    // 正常断言（不触发）
    int x = 42;
    ASSERT(x > 0);
    cout << "  正常断言通过" << endl;
}

// ============================================
// 第二部分：HardFault 分析 ⭐⭐⭐⭐
// ============================================

/*
【概念】HardFault 栈帧结构

ARM Cortex-M 发生异常时，CPU自动将以下寄存器压栈：
  sp[0] = R0    sp[1] = R1    sp[2] = R2    sp[3] = R3
  sp[4] = R12   sp[5] = LR    sp[6] = PC    sp[7] = xPSR

PC = 出错指令地址（最关键）
LR = 调用者返回地址（定位调用链）
*/

struct ExceptionFrame {
    u32 r0, r1, r2, r3;
    u32 r12;
    u32 lr;   // 调用者返回地址
    u32 pc;   // 出错指令地址
    u32 xpsr; // 程序状态寄存器
};

// 模拟HardFault分析（真实场景：在HardFault_Handler中调用）
void analyzeHardFault(const ExceptionFrame& frame) {
    cout << string(60, '-') << endl;
    cout << "2. HardFault 分析" << endl;
    cout << string(60, '-') << endl;

    cout << "  出错指令地址 PC  = 0x" << hex << frame.pc << endl;
    cout << "  调用者地址   LR  = 0x" << frame.lr << endl;
    cout << "  参数寄存器   R0  = 0x" << frame.r0 << endl;
    cout << dec;

    // 分析常见原因
    if (frame.pc == 0x00000000 || frame.pc == 0xFFFFFFFF) {
        cout << "  → 可能原因：调用了空指针或未初始化函数指针" << endl;
    } else if (frame.pc & 0x1) {
        cout << "  → 注意：PC最低位为1，ARM Thumb模式正常" << endl;
    }

    cout << "  → 下一步：用 arm-none-eabi-addr2line -e firmware.elf 0x"
         << hex << frame.pc << " 定位源码行号" << dec << endl;
}

void demoHardFault() {
    // 模拟一个HardFault场景
    ExceptionFrame frame = {
        0x00000001, 0x20000100, 0x00000000, 0x00000000,  // R0-R3
        0x00000000,       // R12
        0x08001234,       // LR：调用者在0x08001234
        0x08002ABC,       // PC：出错在0x08002ABC
        0x01000000        // xPSR
    };
    analyzeHardFault(frame);
}

// ============================================
// 第三部分：看门狗 ⭐⭐⭐⭐⭐
// ============================================

/*
【函数卡片】Watchdog

作用：硬件独立定时器，程序必须在超时前"喂狗"，否则自动复位
原理：启动后开始倒计时，喂狗重置计时器，超时触发系统复位
关键点：看门狗启动后无法关闭（IWDG），必须保证所有代码路径都能喂狗
*/

class Watchdog {
    u32 timeout_ms;
    u32 last_feed;
    bool triggered = false;

public:
    explicit Watchdog(u32 timeout = 2000) : timeout_ms(timeout), last_feed(0) {}

    void init() {
        last_feed = HAL_GetTick();
        cout << "[WDG] 看门狗启动，超时=" << timeout_ms << "ms" << endl;
    }

    // 喂狗：必须在超时前调用
    void feed() {
        last_feed = HAL_GetTick();
    }

    // 检查（模拟硬件行为）
    bool check() {
        u32 elapsed = HAL_GetTick() - last_feed;
        if (elapsed >= timeout_ms) {
            if (!triggered) {
                cout << "[WDG] ❌ 超时" << elapsed << "ms，系统复位！" << endl;
                triggered = true;
            }
            return false; // 触发复位
        }
        if (elapsed > timeout_ms * 3 / 4) {
            cout << "[WDG] ⚠ 警告：已" << elapsed << "ms未喂狗（超时"
                 << timeout_ms << "ms）" << endl;
        }
        return true;
    }
};

void demoWatchdog() {
    cout << string(60, '-') << endl;
    cout << "3. 看门狗演示" << endl;
    cout << string(60, '-') << endl;

    Watchdog wdg(1000); // 1秒超时
    wdg.init();

    // 正常喂狗
    for (int i = 0; i < 3; ++i) {
        g_tick += 400;
        wdg.feed();
        wdg.check();
        cout << "  tick=" << g_tick << " 正常喂狗" << endl;
    }

    // 模拟忘记喂狗（程序卡死）
    cout << "  模拟程序卡死，不喂狗..." << endl;
    g_tick += 800;  wdg.check();
    g_tick += 300;  wdg.check(); // 触发超时
}

// ============================================
// 第四部分：Stack Canary（栈溢出检测）⭐⭐⭐
// ============================================

/*
【概念】Stack Canary

原理：在栈底写入魔数（Canary值），定期检测魔数是否被覆盖
触发：栈溢出时局部变量会覆盖Canary → 检测到魔数被修改 → 报警
*/

constexpr u32 CANARY_VALUE = 0xDEADBEEF;
static u32 g_stack_canary = CANARY_VALUE; // 模拟栈底Canary

void checkStackCanary() {
    if (g_stack_canary != CANARY_VALUE) {
        cout << "[CANARY] ❌ 栈溢出检测！Canary被覆盖！原值=0xDEADBEEF"
             << " 当前值=0x" << hex << g_stack_canary << dec << endl;
        ASSERT(false); // 触发断言
    } else {
        cout << "[CANARY] ✅ 栈Canary正常（0xDEADBEEF）" << endl;
    }
}

void demoStackCanary() {
    cout << string(60, '-') << endl;
    cout << "4. Stack Canary 演示" << endl;
    cout << string(60, '-') << endl;

    checkStackCanary(); // 正常

    // 模拟栈溢出（覆盖Canary）
    cout << "  模拟栈溢出覆盖Canary..." << endl;
    g_stack_canary = 0xBAADF00D;
    checkStackCanary(); // 触发告警

    g_stack_canary = CANARY_VALUE; // 恢复
}

// ============================================
// 第五部分：printf 重定向
// ============================================

/*
【概念】printf 重定向

嵌入式中 printf 默认无输出，需要重定向到 UART
方法：重写 _write() 系统调用（GCC/newlib）
*/

void demoPrintfRedirect() {
    cout << string(60, '-') << endl;
    cout << "5. printf 重定向（概念说明）" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
// 真实嵌入式代码（重定向printf到UART）：
// ----------------------------------------
// #include <sys/unistd.h>
// extern UART_HandleTypeDef huart1;
//
// int _write(int file, char* data, int len) {
//     HAL_UART_Transmit(&huart1, (uint8_t*)data, len, HAL_MAX_DELAY);
//     return len;
// }
// ----------------------------------------
// 之后 printf("Hello %d\n", 42) 会通过UART1输出
)" << endl;
}

// ============================================
// 常见错误
// ============================================

void showMistakes() {
    cout << R"(
============================================================
常见错误和陷阱 ⭐⭐⭐⭐⭐
============================================================

❌ 错误1：断言宏在Release版本仍有效
  // Release版应禁用断言以节省Flash
  #ifdef NDEBUG
  #define ASSERT(c) ((void)0)  // ✅ Release版空宏
  #endif

❌ 错误2：看门狗超时时间设太短
  Watchdog wdg(50);  // ❌ 50ms太短，正常任务调度可能触发复位
  Watchdog wdg(2000); // ✅ 2~8秒是常见设置

❌ 错误3：在关中断期间忘记喂狗
  __disable_irq();
  // 长时间操作... 看门狗此时仍在计时！
  __enable_irq();
  // ✅ 关中断操作必须极短，或在操作前后各喂一次狗

❌ 错误4：HardFault中使用printf
  void HardFault_Handler() {
      printf("HardFault!\n");  // ❌ printf需要栈空间，HardFault时栈可能已损坏
      // ✅ 直接操作UART寄存器输出，不依赖运行时库
  }

❌ 错误5：Stack Canary只在初始化时检查一次
  checkStackCanary(); // ❌ 只检查一次没有意义
  // ✅ 在系统管理任务中每隔100~500ms定期检查
============================================================
)";
}

int main() {
    cout << string(60, '=') << endl;
    cout << "18_debug_techniques.cpp - 嵌入式调试技术" << endl;
    cout << string(60, '=') << endl;

    demoAssert();
    demoHardFault();
    demoWatchdog();
    demoStackCanary();
    demoPrintfRedirect();
    showMistakes();

    cout << R"(
练习题
============================================================
1. 实现一个带历史记录的断言：触发时将最近10次断言位置存入环形缓冲区
2. 扩展看门狗，支持"独立计数"：每个任务有自己的喂狗标志，只有所有任务都喂狗后才重置计时器
3. 实现一个崩溃日志系统：HardFault触发时将栈帧写入Flash，下次启动时读出并上报
============================================================
)";
    return 0;
}
