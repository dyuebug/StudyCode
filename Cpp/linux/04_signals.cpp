// ============================================
// 04_signals.cpp
// 日期：2026-06-07（第65天）
// 主题：Linux 信号机制
// 难度：⭐⭐ 中级
// 注意：需在 Linux/WSL 环境编译运行
// 编译：g++ -std=c++17 -o 04_signals 04_signals.cpp
// ============================================

/*
【核心概念】

是什么？
- 信号（Signal）：内核向进程发送的异步通知，类似"软件中断"
- 进程收到信号后中断当前执行，跳转到信号处理函数

为什么需要？
- 进程间通信的最简单方式（无需共享内存/管道）
- 处理异常事件：Ctrl+C（SIGINT）、程序崩溃（SIGSEGV）
- 优雅退出：收到SIGTERM后清理资源再退出

核心特点：
- 异步：信号可在任意时刻到达
- 处理函数中只能调用 async-signal-safe 函数
- 信号不排队（同一信号多次到达只处理一次，SIGRTMIN以上实时信号除外）
*/

#include <iostream>
#include <string>
using namespace std;

// ============================================
// 第一部分：核心概念详解
// ============================================

void showConcepts() {
    cout << R"(
============================================================
信号（Signal）核心概念
============================================================

【常用信号速查表】

  信号名      编号  默认行为    触发场景
  ─────────────────────────────────────────────
  SIGINT       2   终止进程   Ctrl+C
  SIGTERM     15   终止进程   kill命令默认发送
  SIGKILL      9   强制终止   kill -9（不可捕获/忽略）
  SIGQUIT      3   终止+core  Ctrl+\
  SIGSEGV     11   终止+core  非法内存访问
  SIGFPE       8   终止+core  除零等浮点错误
  SIGPIPE     13   终止进程   写已关闭读端的管道
  SIGCHLD     17   忽略       子进程状态改变
  SIGUSR1     10   终止进程   用户自定义
  SIGUSR2     12   终止进程   用户自定义
  SIGALRM     14   终止进程   alarm()定时器到期
  SIGHUP       1   终止进程   终端断开/守护进程重载配置

【三种处理方式】
  SIG_DFL — 默认行为（终止/忽略/core dump）
  SIG_IGN — 忽略信号
  自定义函数 — 执行自定义处理逻辑

【signal() vs sigaction()】
  signal()   — 简单，但行为在不同系统上不一致（不推荐）
  sigaction() — POSIX标准，行为明确，推荐使用

============================================================
)";
}

// ============================================
// 第二部分：代码示例
// ============================================

void showSigaction() {
    cout << R"(
------------------------------------------------------------
sigaction() 注册信号处理函数（推荐方式）
------------------------------------------------------------

// ---- 真实Linux代码 ----
#include <signal.h>
#include <unistd.h>

volatile sig_atomic_t g_running = 1;  // ★ 信号处理函数与主循环共享的标志
                                       //   必须用 volatile sig_atomic_t

void handleSIGTERM(int signo) {
    // ⚠ 只能调用 async-signal-safe 函数！
    // ✅ write()、_exit()、sig_atomic_t操作
    // ❌ printf()、malloc()、cout（有锁，不安全）
    g_running = 0;
    // 用write而非printf（write是async-signal-safe）
    const char msg[] = "收到SIGTERM，准备退出...\n";
    write(STDOUT_FILENO, msg, sizeof(msg)-1);
}

int main() {
    struct sigaction sa{};
    sa.sa_handler = handleSIGTERM;   // 处理函数
    sigemptyset(&sa.sa_mask);        // 处理期间不额外屏蔽信号
    sa.sa_flags = SA_RESTART;        // 被信号中断的系统调用自动重启

    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT,  &sa, nullptr);  // Ctrl+C 也优雅退出

    // 主循环
    while (g_running) {
        // 正常工作...
        sleep(1);
    }

    // 清理资源
    cout << "清理资源，正常退出" << endl;
    return 0;
}
)";
}

void showKillAndAlarm() {
    cout << R"(
------------------------------------------------------------
kill() 发送信号 / alarm() 定时器
------------------------------------------------------------

#include <signal.h>

// kill() 发送信号
kill(pid, SIGTERM);   // 向指定进程发SIGTERM
kill(pid, SIGUSR1);   // 发用户自定义信号1
kill(0,   SIGTERM);   // 向同进程组所有进程发送
kill(-1,  SIGTERM);   // 向所有有权限的进程发送（慎用！）

// raise() 向自身发信号
raise(SIGUSR1);       // 等价于 kill(getpid(), SIGUSR1)

// alarm() 定时器（单次，精度秒级）
signal(SIGALRM, [](int){ cout << "超时！" << endl; });
alarm(5);    // 5秒后发送SIGALRM
// ... 等待操作 ...
alarm(0);    // 取消定时器

// 忽略SIGPIPE（写管道时对端已关闭，默认终止进程）
signal(SIGPIPE, SIG_IGN);  // 改为忽略，write()返回-1/errno=EPIPE
)";
}

void showSigprocmask() {
    cout << R"(
------------------------------------------------------------
信号屏蔽（临界区保护）
------------------------------------------------------------

// 临时屏蔽SIGINT，保护临界区
sigset_t mask, old_mask;
sigemptyset(&mask);
sigaddset(&mask, SIGINT);
sigaddset(&mask, SIGTERM);

sigprocmask(SIG_BLOCK, &mask, &old_mask);  // 屏蔽
// --- 临界区：不会被信号中断 ---
update_critical_data();
// --- 临界区结束 ---
sigprocmask(SIG_SETMASK, &old_mask, nullptr);  // 恢复

// 注意：被屏蔽的信号"挂起"，解除屏蔽后立即递送
)";
}

// ============================================
// 第三部分：深入理解
// ============================================

void showDeepDive() {
    cout << R"(
============================================================
🔍 深入理解：async-signal-safe 函数（选学）
============================================================

信号处理函数可在任意时刻打断主程序，包括主程序正在执行
malloc()、printf()内部的时候。这些函数内部有锁，若信号
处理函数再次调用同一函数，会死锁。

async-signal-safe 函数（可在信号处理函数中安全调用）：
  ✅ write()、read()、_exit()、kill()、signal()
  ✅ open()、close()、waitpid()、sleep()
  ❌ printf()、malloc()/free()、cout、fopen()
  ❌ exit()（会调用atexit回调）、任何C++标准库函数

最佳实践：
  信号处理函数只做一件事：设置 volatile sig_atomic_t 标志
  → 主循环检测标志 → 在主循环中做清理工作

============================================================
)";
}

// ============================================
// 第四部分：常见错误
// ============================================

void showMistakes() {
    cout << R"(
============================================================
常见错误和陷阱 ⭐⭐⭐⭐⭐
============================================================

❌ 错误1：在信号处理函数中调用printf/cout
  void handler(int sig) {
      printf("收到信号%d\n", sig);  // ❌ 可能死锁
  }
  // ✅ 用 write(STDOUT_FILENO, msg, len)

❌ 错误2：共享变量不用volatile sig_atomic_t
  bool g_stop = false;  // ❌ 编译器可能优化掉对它的读取
  volatile sig_atomic_t g_stop = 0;  // ✅

❌ 错误3：用signal()代替sigaction()
  signal(SIGCHLD, SIG_IGN);  // ❌ 不同系统行为不一致
  // ✅ 始终使用sigaction()

❌ 错误4：忘记处理EINTR错误码
  n = read(fd, buf, size);
  if (n < 0) { /* 直接报错 */ }  // ❌ 若被信号中断，errno=EINTR，应重试
  // ✅ while((n=read(fd,buf,size))<0 && errno==EINTR); // 或设置SA_RESTART

❌ 错误5：试图捕获SIGKILL/SIGSTOP
  signal(SIGKILL, handler);  // ❌ 编译不报错但无效，内核强制执行
  // SIGKILL和SIGSTOP永远不能被捕获、忽略或屏蔽

============================================================
)";
}

int main() {
    cout << string(60, '=') << endl;
    cout << "04_signals.cpp - Linux信号机制" << endl;
    cout << "（概念演示版，完整代码需在Linux/WSL中运行）" << endl;
    cout << string(60, '=') << endl;

    showConcepts();
    showSigaction();
    showKillAndAlarm();
    showSigprocmask();
    showDeepDive();
    showMistakes();

    cout << R"(
============================================================
练习题
============================================================
1. 用sigaction实现优雅退出守护进程：
   收到SIGTERM时设置标志，主循环完成当前任务后清理退出

2. 实现父进程监控子进程：注册SIGCHLD处理函数，
   子进程退出时自动调用waitpid回收，避免僵尸进程

3. 用SIGUSR1/SIGUSR2实现进程间简单通信：
   进程A收到SIGUSR1时打印"收到命令1"，收到SIGUSR2时打印"收到命令2"

4. 用alarm()实现操作超时：
   等待用户输入时设置10秒alarm，超时则打印"超时"并退出
============================================================
)";
    return 0;
}
