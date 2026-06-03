// ============================================
// 93_async_concept.cpp - 异步协程概念示例
// 功能：通过模拟代码理解异步协程的工作原理
// 注意：本文件不使用真实的协程，而是用类模拟异步行为
// 编译：g++ -std=c++20 -o 93_async_concept.exe 93_async_concept.cpp
// ============================================

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <thread>
using namespace std;

// ============================================
// 第一部分：异步编程核心概念
// ============================================

/*
【异步编程的核心思想】

同步 vs 异步：
- 同步：一个任务执行完才能执行下一个（阻塞）
- 异步：任务可以并发执行，不互相阻塞

传统异步方式的问题：
1. 回调地狱：嵌套的回调函数，代码难以理解
2. 错误处理复杂：每层回调都要处理错误
3. 代码不直观：执行顺序不清晰

协程的优势：
1. 代码看起来像同步，实际是异步
2. 错误处理简单：使用 try-catch
3. 执行顺序清晰：从上到下阅读
*/

// ============================================
// 第二部分：模拟异步任务
// ============================================

// 模拟异步任务的状态
enum class TaskState {
    PENDING,    // 等待中
    RUNNING,    // 运行中
    COMPLETED,  // 已完成
    FAILED      // 失败
};

// 模拟异步任务
template<typename T>
class AsyncTask {
private:
    TaskState state;
    T result;
    string error_message;
    function<T()> task_func;  // 任务函数

public:
    // 构造函数：接受一个任务函数
    AsyncTask(function<T()> func)
        : state(TaskState::PENDING), task_func(func) {
        cout << "[异步任务创建] 状态：PENDING" << endl;
    }

    // 启动任务（模拟异步执行）
    void start() {
        cout << "[异步任务启动] 状态：RUNNING" << endl;
        state = TaskState::RUNNING;

        try {
            // 模拟耗时操作
            this_thread::sleep_for(chrono::milliseconds(100));

            // 执行任务
            result = task_func();

            state = TaskState::COMPLETED;
            cout << "[异步任务完成] 状态：COMPLETED" << endl;
        } catch (const exception& e) {
            state = TaskState::FAILED;
            error_message = e.what();
            cout << "[异步任务失败] 错误：" << error_message << endl;
        }
    }

    // 等待任务完成（模拟 co_await）
    T await() {
        cout << "[等待任务完成] 当前状态：";
        switch (state) {
            case TaskState::PENDING:
                cout << "PENDING，开始执行..." << endl;
                start();
                break;
            case TaskState::RUNNING:
                cout << "RUNNING，等待中..." << endl;
                // 模拟等待
                while (state == TaskState::RUNNING) {
                    this_thread::sleep_for(chrono::milliseconds(10));
                }
                break;
            case TaskState::COMPLETED:
                cout << "COMPLETED" << endl;
                break;
            case TaskState::FAILED:
                cout << "FAILED" << endl;
                throw runtime_error(error_message);
        }

        return result;
    }

    // 检查任务是否完成
    bool is_completed() const {
        return state == TaskState::COMPLETED;
    }

    // 获取任务状态
    TaskState get_state() const {
        return state;
    }
};

// ============================================
// 第三部分：异步协程 vs 传统方法对比
// ============================================

void part1_async_comparison() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第一部分：异步协程 vs 传统方法对比" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 1.1 传统同步方式（阻塞）
    // --------------------------------------------

    cout << "\n【传统同步方式】阻塞执行" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
代码示例：
------------------------------------------------------------
string read_file(string path) {
    // 阻塞，直到文件读取完成
    return file_content;
}

string content1 = read_file("file1.txt");  // 阻塞1秒
string content2 = read_file("file2.txt");  // 阻塞1秒
string content3 = read_file("file3.txt");  // 阻塞1秒

总耗时：3秒（串行执行）

问题：
- 主线程被阻塞，无法做其他事
- 无法并发执行多个任务
- 性能差，用户体验不好
)" << endl;

    // --------------------------------------------
    // 1.2 传统回调方式（回调地狱）
    // --------------------------------------------

    cout << "\n【传统回调方式】回调地狱" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
代码示例：
------------------------------------------------------------
read_file_async("file1.txt", [](string content1) {
    // 读取文件1完成
    read_file_async("file2.txt", [content1](string content2) {
        // 读取文件2完成
        read_file_async("file3.txt", [content1, content2](string content3) {
            // 读取文件3完成
            // 嵌套太深，难以理解！
            process(content1, content2, content3);
        });
    });
});

总耗时：1秒（并发执行）

问题：
- 代码嵌套太深，难以阅读
- 错误处理复杂（每层都要处理）
- 变量捕获复杂（需要捕获外层变量）
- 调试困难
)" << endl;

    // --------------------------------------------
    // 1.3 协程方式（清晰易读）
    // --------------------------------------------

    cout << "\n【协程方式】清晰易读" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
代码示例（伪代码）：
------------------------------------------------------------
Task<void> process_files() {
    // 看起来像同步代码，实际是异步执行
    string content1 = co_await read_file_async("file1.txt");
    string content2 = co_await read_file_async("file2.txt");
    string content3 = co_await read_file_async("file3.txt");

    // 处理结果
    process(content1, content2, content3);
}

总耗时：1秒（并发执行）

优势：
- 代码清晰，从上到下阅读
- 错误处理简单（使用 try-catch）
- 不需要捕获变量
- 调试容易
)" << endl;

    // --------------------------------------------
    // 1.4 对比总结
    // --------------------------------------------

    cout << "\n【对比总结】" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
┌──────────────┬────────────┬────────────┬────────────┐
│   特性       │  同步方式  │  回调方式  │  协程方式  │
├──────────────┼────────────┼────────────┼────────────┤
│ 代码可读性   │ 高         │ 低         │ 高         │
│ 执行效率     │ 低（阻塞） │ 高（并发） │ 高（并发） │
│ 错误处理     │ 简单       │ 复杂       │ 简单       │
│ 调试难度     │ 简单       │ 困难       │ 简单       │
│ 学习曲线     │ 平缓       │ 陡峭       │ 平缓       │
└──────────────┴────────────┴────────────┴────────────┘

结论：协程结合了同步代码的可读性和异步代码的高效性！
)" << endl;
}

// ============================================
// 第四部分：异步任务实践示例
// ============================================

void part2_async_examples() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分：异步任务实践示例" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 2.1 示例1：模拟异步文件读取
    // --------------------------------------------

    cout << "\n【示例1】模拟异步文件读取" << endl;
    cout << string(60, '-') << endl;

    {
        // 创建异步任务：读取文件
        AsyncTask<string> task([](){
            cout << "  [模拟] 正在读取文件..." << endl;
            return "文件内容：Hello, Coroutines!";
        });

        cout << "\n主线程：创建异步任务" << endl;
        cout << "主线程：可以做其他事情..." << endl;

        // 等待任务完成（模拟 co_await）
        cout << "\n主线程：现在需要文件内容，等待任务完成" << endl;
        string content = task.await();

        cout << "\n主线程：获取到结果：" << content << endl;
    }

    // --------------------------------------------
    // 2.2 示例2：模拟异步网络请求
    // --------------------------------------------

    cout << "\n【示例2】模拟异步网络请求" << endl;
    cout << string(60, '-') << endl;

    {
        // 创建异步任务：网络请求
        AsyncTask<int> task([](){
            cout << "  [模拟] 正在发送网络请求..." << endl;
            return 200;  // HTTP 状态码
        });

        cout << "\n主线程：发起网络请求" << endl;
        cout << "主线程：可以更新UI..." << endl;

        // 等待请求完成
        cout << "\n主线程：等待网络响应" << endl;
        int status_code = task.await();

        cout << "\n主线程：收到响应，状态码：" << status_code << endl;
    }

    // --------------------------------------------
    // 2.3 示例3：多个异步任务
    // --------------------------------------------

    cout << "\n【示例3】多个异步任务" << endl;
    cout << string(60, '-') << endl;

    {
        cout << "\n创建3个异步任务..." << endl;

        // 创建多个异步任务
        AsyncTask<string> task1([](){
            return "任务1结果";
        });

        AsyncTask<string> task2([](){
            return "任务2结果";
        });

        AsyncTask<string> task3([](){
            return "任务3结果";
        });

        cout << "\n主线程：等待所有任务完成..." << endl;

        // 依次等待（模拟 co_await）
        string result1 = task1.await();
        string result2 = task2.await();
        string result3 = task3.await();

        cout << "\n所有任务完成：" << endl;
        cout << "  " << result1 << endl;
        cout << "  " << result2 << endl;
        cout << "  " << result3 << endl;

        cout << "\n说明：" << endl;
        cout << "- 真实协程可以并发执行多个任务" << endl;
        cout << "- 使用 co_await 等待每个任务完成" << endl;
        cout << "- 代码清晰，易于理解" << endl;
    }
}

// ============================================
// 第五部分：异步协程的应用场景
// ============================================

void part3_async_scenarios() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第三部分：异步协程的应用场景" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【应用场景1：文件 I/O】

问题：
- 文件读写耗时长，阻塞主线程
- 需要读取多个文件时，串行执行效率低

解决方案（伪代码）：
------------------------------------------------------------
Task<void> process_files() {
    // 并发读取多个文件
    auto task1 = read_file_async("file1.txt");
    auto task2 = read_file_async("file2.txt");
    auto task3 = read_file_async("file3.txt");

    // 等待所有文件读取完成
    string content1 = co_await task1;
    string content2 = co_await task2;
    string content3 = co_await task3;

    // 处理文件内容
    process(content1, content2, content3);
}

优势：
- 并发读取，提高效率
- 代码清晰，易于维护
- 不阻塞主线程

────────────────────────────────────

【应用场景2：网络请求】

问题：
- 网络请求延迟高，阻塞主线程
- 多个请求需要串行执行，效率低

解决方案（伪代码）：
------------------------------------------------------------
Task<void> fetch_data() {
    // 并发发起多个请求
    auto task1 = http_get_async("https://api1.com/data");
    auto task2 = http_get_async("https://api2.com/data");

    // 等待所有请求完成
    string data1 = co_await task1;
    string data2 = co_await task2;

    // 处理数据
    process(data1, data2);
}

优势：
- 并发请求，减少总耗时
- 避免回调地狱
- 错误处理简单

────────────────────────────────────

【应用场景3：数据库操作】

问题：
- 数据库查询耗时长
- 多个查询需要串行执行

解决方案（伪代码）：
------------------------------------------------------------
Task<void> query_database() {
    // 异步查询
    auto users = co_await db.query_async("SELECT * FROM users");
    auto orders = co_await db.query_async("SELECT * FROM orders");

    // 处理结果
    process(users, orders);
}

优势：
- 不阻塞主线程
- 代码清晰
- 易于组合多个查询

────────────────────────────────────

【应用场景4：定时器】

问题：
- 需要延迟执行某些操作
- 传统方式使用回调，代码复杂

解决方案（伪代码）：
------------------------------------------------------------
Task<void> delayed_task() {
    cout << "开始任务" << endl;

    // 等待1秒
    co_await sleep_async(1000);

    cout << "1秒后执行" << endl;

    // 再等待2秒
    co_await sleep_async(2000);

    cout << "再过2秒执行" << endl;
}

优势：
- 代码顺序清晰
- 不需要嵌套回调
- 易于理解

────────────────────────────────────

【应用场景5：游戏开发】

问题：
- 游戏中有很多异步操作（动画、音效、网络同步）
- 传统方式使用状态机，代码复杂

解决方案（伪代码）：
------------------------------------------------------------
Task<void> player_attack() {
    // 播放攻击动画
    co_await play_animation_async("attack");

    // 播放音效
    co_await play_sound_async("sword.wav");

    // 计算伤害
    int damage = calculate_damage();

    // 同步到服务器
    co_await send_to_server_async(damage);

    cout << "攻击完成" << endl;
}

优势：
- 代码清晰，易于理解
- 不需要复杂的状态机
- 易于调试和维护

────────────────────────────────────

【应用场景总结】

┌──────────────┬────────────────┬────────────────┐
│   场景       │   传统方式     │   协程方式     │
├──────────────┼────────────────┼────────────────┤
│ 文件 I/O     │ 阻塞或回调     │ co_await       │
│ 网络请求     │ 回调地狱       │ co_await       │
│ 数据库操作   │ 阻塞或回调     │ co_await       │
│ 定时器       │ 嵌套回调       │ co_await       │
│ 游戏开发     │ 状态机         │ co_await       │
└──────────────┴────────────────┴────────────────┘

核心优势：
1. 代码清晰：看起来像同步，实际是异步
2. 性能高：不阻塞线程，充分利用资源
3. 易维护：不需要复杂的回调或状态机
4. 易调试：执行流程清晰
)" << endl;
}

// ============================================
// 第六部分：真实协程 vs 模拟异步
// ============================================

void part4_real_vs_simulated() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第四部分：真实协程 vs 模拟异步" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【真实协程（C++20）】

优势：
- 编译器自动管理状态
- 语法简洁，使用 co_await
- 性能优化（编译器优化）
- 标准化，跨平台

劣势：
- 需要复杂的样板代码（promise_type、awaiter）
- 编译器支持要求高（GCC 10+、Clang 10+）
- 学习曲线陡峭
- 调试困难

示例（伪代码）：
------------------------------------------------------------
Task<string> read_file_async(string path) {
    auto content = co_await async_read(path);
    co_return content;
}

// 使用
Task<void> process() {
    string content = co_await read_file_async("data.txt");
    cout << content;
}

────────────────────────────────────

【模拟异步（本文件）】

优势：
- 不需要编译器支持
- 容易理解，适合学习
- 可以自定义行为
- 调试简单

劣势：
- 需要手动管理状态
- 代码冗长（需要写类）
- 性能稍差（没有编译器优化）
- 不是标准化的

示例：
------------------------------------------------------------
AsyncTask<string> read_file_async(string path) {
    return AsyncTask<string>([path](){
        // 读取文件
        return file_content;
    });
}

// 使用
auto task = read_file_async("data.txt");
string content = task.await();
cout << content;

────────────────────────────────────

【对比总结】

┌──────────────┬────────────────┬────────────────┐
│   特性       │   真实协程     │   模拟异步     │
├──────────────┼────────────────┼────────────────┤
│ 语法简洁度   │ 高（co_await） │ 低（需要写类） │
│ 编译器要求   │ 高（C++20）    │ 低（C++11）    │
│ 学习难度     │ 高             │ 低             │
│ 性能         │ 高             │ 中             │
│ 适用场景     │ 生产环境       │ 学习、原型     │
└──────────────┴────────────────┴────────────────┘

【学习建议】

1. 先理解异步编程的概念（本文件）
2. 理解协程如何简化异步代码
3. 再学习真实协程的语法
4. 最后实践真实协程的应用

【下一步】

- 如果你的编译器支持 C++20 协程，可以尝试真实协程
- 如果不支持，可以使用第三方库（如 cppcoro、Boost.Asio）
- 或者继续使用传统异步方式（回调、Promise/Future）
)" << endl;
}

// ============================================
// 常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonErrors() {
    cout << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
❌ 错误1：误以为异步就是多线程（概念混淆）

   // 异步 ≠ 多线程：
   // 单线程也可以异步（事件循环、协程）
   // 多线程也可以同步（阻塞等待）

   协程异步：单线程内挂起/恢复，切换开销极小（纳秒级）
   线程并行：真正的并行计算，切换开销大（微秒级），适合 CPU 密集型任务

   ✅ 规则：I/O 密集型用异步协程；CPU 密集型用多线程

------------------------------------------------------------

❌ 错误2：在异步任务中做阻塞操作（破坏异步效果）

   Task fetchData() {
       auto data = httpGet(url);   // ❌ 同步阻塞 HTTP 请求，线程卡住
       co_return data;
   }

   ✅ 用异步版本的 I/O 操作：
   Task fetchData() {
       auto data = co_await asyncHttpGet(url);  // ✅ 挂起协程，线程继续处理其他任务
       co_return data;
   }

------------------------------------------------------------

❌ 错误3：回调地狱没有用协程改善（仍然嵌套回调）

   fetchA([](ResultA a) {
       fetchB(a, [](ResultB b) {
           fetchC(b, [](ResultC c) {
               process(c);          // ❌ 三层嵌套，难以阅读
           });
       });
   });

   ✅ 协程让异步代码像同步代码一样线性：
   Task task() {
       auto a = co_await fetchA();
       auto b = co_await fetchB(a);
       auto c = co_await fetchC(b);
       process(c);                  // ✅ 线性，清晰
   }

------------------------------------------------------------

❌ 错误4：忘记 co_await 导致任务未被等待就析构

   Task main() {
       auto t = slowOperation();   // ❌ 忘记 co_await，t 立即析构，操作可能未完成
       useResult();
   }

   ✅ 异步任务必须 co_await：
   Task main() {
       auto result = co_await slowOperation();  // ✅ 等待完成
       useResult(result);
   }

------------------------------------------------------------

❌ 错误5：模拟异步类没有处理异常（异步异常传播）

   class AsyncTask {
       void execute() {
           throw runtime_error("fail");  // ❌ 异常在后台，调用方不知道
       }
   };

   ✅ 捕获异常并存储，在 get()/co_await 时重新抛出：
   try { execute(); }
   catch (...) { exception = current_exception(); }
   // 在 get() 中：if (exception) rethrow_exception(exception);
)" << endl;
}

int main() {
    cout << "=== 异步协程概念示例 ===" << endl;

    part1_async_comparison();
    part2_async_examples();
    part3_async_scenarios();
    part4_real_vs_simulated();

    cout << "\n程序结束" << endl;
    return 0;
}
