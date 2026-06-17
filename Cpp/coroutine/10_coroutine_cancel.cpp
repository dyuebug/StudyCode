// ============================================
// 协程取消与资源管理 - 超详细注释版
// 日期：2026-06-17（第84天）
// 难度：⭐⭐⭐⭐ 高级
// ============================================

/*
【核心概念】简要说明

是什么？
- 协程取消：主动终止协程的执行
- 资源管理：正确清理协程占用的资源
- 优雅关闭：有序地关闭协程池

为什么需要？
- 用户取消操作
- 任务不再需要
- 系统关闭

核心特点？
- 取消标志：协程检查标志决定是否退出
- 资源清理：RAII自动管理资源
- 优雅关闭：等待当前任务完成后关闭
*/

#include <iostream>
#include <string>

using namespace std;

// ============================================
// 第一部分：取消机制核心概念
// ============================================

void part1_cancel_concepts() {
    cout << string(60, '=') << endl;
    cout << "第一部分：取消机制核心概念" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【什么是协程取消？】

定义：
- 协程取消是主动终止协程执行的机制
- 不是强制杀死，而是协作式退出
- 协程检查取消标志后自行退出

取消场景：
1. 用户取消操作（点击取消按钮）
2. 任务不再需要（用户导航到其他页面）
3. 超时后取消（任务执行时间过长）
4. 系统关闭（程序退出）

【为什么需要取消机制？】

问题1：无法停止任务
- 任务一旦开始，无法中途停止
- 浪费资源
- 用户体验差

问题2：资源泄漏
- 任务取消后资源未释放
- 内存泄漏
- 文件句柄泄漏

问题3：关闭困难
- 程序退出时协程还在运行
- 可能导致崩溃
- 数据损坏

取消机制的价值：
✅ 及时停止不需要的任务
✅ 正确清理资源
✅ 优雅关闭系统
✅ 提升用户体验

【取消的两种方式】

方式1：协作式取消（推荐）
- 设置取消标志
- 协程检查标志后退出
- 可以清理资源

特点：
✅ 安全可靠
✅ 可以清理资源
✅ 不会导致数据损坏
❌ 需要协程配合检查标志

方式2：强制取消（不推荐）
- 直接销毁协程
- 不给协程清理机会
- 可能导致资源泄漏

特点：
✅ 立即生效
❌ 可能泄漏资源
❌ 可能数据损坏
❌ 不安全

结论：
- 优先使用协作式取消
- 只在特殊情况使用强制取消

【取消标志的设计】

设计1：简单布尔标志
struct Task {
    bool cancelled = false;
};

// 检查取消
if (task.cancelled) {
    return;
}

设计2：原子标志（多线程）
#include <atomic>

struct Task {
    atomic<bool> cancelled{false};
};

设计3：取消令牌（Token）
struct CancellationToken {
    atomic<bool> cancelled{false};

    void cancel() {
        cancelled = true;
    }

    bool is_cancelled() const {
        return cancelled;
    }
};

【资源管理策略】

策略1：RAII（Resource Acquisition Is Initialization）
- 资源在构造时获取
- 资源在析构时释放
- 自动管理，不会泄漏

示例：
class FileResource {
    FILE* file;
public:
    FileResource(const char* path) {
        file = fopen(path, "r");
    }

    ~FileResource() {
        if (file) fclose(file);
    }
};

策略2：智能指针
- unique_ptr：独占所有权
- shared_ptr：共享所有权
- 自动释放内存

示例：
auto buffer = make_unique<char[]>(1024);
// 离开作用域自动释放

策略3：finally块（C++没有，但可模拟）
struct Finally {
    function<void()> cleanup;
    ~Finally() { cleanup(); }
};

// 使用
Finally f{[]{ cout << "cleanup" << endl; }};

【优雅关闭流程】

步骤1：停止接收新任务
- 设置关闭标志
- 拒绝新任务提交

步骤2：等待当前任务完成
- 不强制中断
- 给任务完成的机会

步骤3：取消等待中的任务
- 清空任务队列
- 通知任务被取消

步骤4：清理资源
- 销毁协程
- 释放内存
- 关闭文件/网络连接

步骤5：确认关闭
- 所有资源已释放
- 可以安全退出

【取消的最佳实践】

1. 设计时考虑取消
   - 所有长任务都应支持取消
   - 定期检查取消标志
   - 在关键点检查

2. 使用RAII管理资源
   - 避免手动管理资源
   - 利用析构函数清理
   - 异常安全

3. 取消时清理资源
   - 关闭文件
   - 释放内存
   - 取消网络请求

4. 通知调用者
   - 返回取消状态
   - 或抛出取消异常
   - 记录日志

5. 测试取消逻辑
   - 测试各种取消场景
   - 确保无资源泄漏
   - 压力测试

【常见错误和陷阱】

错误1：不检查取消标志
// 错误
while (has_work()) {
    do_work();  // 永远不检查取消
}

// 正确
while (has_work() && !is_cancelled()) {
    do_work();
}

错误2：取消后继续访问资源
// 错误
if (is_cancelled()) {
    return;
}
use_resource();  // 资源可能已释放

// 正确
if (is_cancelled()) {
    cleanup_resource();
    return;
}
use_resource();

错误3：强制销毁协程
// 错误
coroutine.destroy();  // 不给清理机会

// 正确
coroutine.set_cancelled();
while (!coroutine.done()) {
    coroutine.resume();  // 让它自然退出
}
coroutine.destroy();

错误4：忘记通知调用者
// 错误
if (is_cancelled()) {
    return;  // 调用者不知道被取消了
}

// 正确
if (is_cancelled()) {
    throw CancelledException();
    // 或 return CancelledResult;
}

【取消与超时的关系】

超时是一种特殊的取消：
1. 超时检测到 → 设置取消标志
2. 协程检查取消标志 → 退出
3. 清理资源 → 通知超时

统一处理：
void check_stop() {
    if (is_cancelled()) {
        throw TaskCancelledException();
    }
    if (is_timeout()) {
        throw TaskTimeoutException();
    }
}

// 在关键点调用
check_stop();
do_work();
check_stop();
)" << endl;
}

// ============================================
// 第二部分：取消机制实现示例
// ============================================

void part2_cancel_examples() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分：取消机制实现" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【简单取消标志示例】

基本结构：
struct Task {
    bool cancelled = false;

    void cancel() {
        cancelled = true;
    }

    bool is_cancelled() const {
        return cancelled;
    }
};

使用示例：
Task task = create_task();

// 在另一个线程取消
task.cancel();

// 任务内检查
if (task.is_cancelled()) {
    cleanup();
    return;
}

【RAII资源管理示例】

文件资源：
class FileGuard {
    FILE* file;
public:
    FileGuard(const char* path) {
        file = fopen(path, "r");
        if (!file) throw runtime_error("open failed");
    }

    ~FileGuard() {
        if (file) {
            fclose(file);
            cout << "文件已关闭" << endl;
        }
    }

    FILE* get() { return file; }
};

// 使用
{
    FileGuard guard("data.txt");
    // 使用文件
    // 离开作用域自动关闭
}

【优雅关闭示例】

class CoroutinePool {
    atomic<bool> shutdown{false};
    queue<Task> tasks;

public:
    void submit(Task task) {
        if (shutdown) {
            throw runtime_error("pool is shutting down");
        }
        tasks.push(task);
    }

    void shutdown_gracefully() {
        // 1. 停止接收新任务
        shutdown = true;

        // 2. 等待当前任务完成
        while (!tasks.empty()) {
            auto task = tasks.front();
            tasks.pop();
            execute(task);
        }

        // 3. 清理资源
        cleanup_all();

        cout << "协程池已优雅关闭" << endl;
    }
};

【综合示例：支持取消和超时的任务】

struct CancellableTask {
    bool cancelled = false;
    steady_clock::time_point start_time;
    int timeout_ms;

    void execute() {
        start_time = steady_clock::now();

        while (has_work()) {
            // 检查取消
            if (cancelled) {
                cout << "任务被取消" << endl;
                return;
            }

            // 检查超时
            auto elapsed = duration_cast<milliseconds>(
                steady_clock::now() - start_time
            ).count();

            if (elapsed > timeout_ms) {
                cout << "任务超时" << endl;
                return;
            }

            // 执行工作
            do_work();
        }

        cout << "任务完成" << endl;
    }
};
)" << endl;
}

// ============================================
// 主函数
// ============================================

int main() {
    cout << "============================================" << endl;
    cout << "协程取消与资源管理" << endl;
    cout << "============================================\n" << endl;

    part1_cancel_concepts();
    part2_cancel_examples();

    cout << "\n" << string(60, '=') << endl;
    cout << "学习完成！" << endl;
    cout << string(60, '=') << endl;

    cout << "\n【第84天学习材料创建完成】" << endl;
    cout << R"(
✅ 已创建3个学习文件：
   1. 08_coroutine_pool_basic.cpp - 协程池基础
   2. 09_coroutine_timeout.cpp - 超时控制
   3. 10_coroutine_cancel.cpp - 取消机制

✅ 已创建任务清单：
   - daily_tasks/2026-06-17_第84天_任务.md

📝 下一步：
   - 现在请去休息！💤
   - 明天精神饱满地学习这些内容
   - 编译运行这些程序
   - 完成学习后生成日志

⏰ 现在时间：凌晨 00:10+
💤 你需要休息了！

晚安，明天见！🌙✨
)" << endl;

    return 0;
}
