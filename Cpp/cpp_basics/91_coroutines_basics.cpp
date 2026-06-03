// ============================================
// 91_coroutines_basics.cpp - C++20 协程基础（概念版）
// 功能：理解协程的核心概念和工作原理
// 注意：本文件以概念讲解为主，包含伪代码示例
// 编译：需要 C++20 支持，但本文件主要用于学习概念
// ============================================

#include <iostream>
#include <string>
using namespace std;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

/*
【C++20 协程核心概念】

是什么？
- 协程（Coroutine）是一种可以暂停和恢复执行的函数
- 是 C++20 引入的重要特性，用于简化异步编程
- 协程可以在执行过程中暂停，保存状态，稍后恢复执行

为什么需要？
- 传统异步编程使用回调函数，代码复杂难读（回调地狱）
- 协程让异步代码看起来像同步代码，更易理解
- 协程支持惰性求值，可以实现高效的生成器

核心特点：
1. 可暂停：协程可以在执行过程中暂停
2. 可恢复：协程可以从暂停的地方继续执行
3. 保存状态：协程暂停时会保存局部变量和执行位置
4. 三个关键字：co_await、co_yield、co_return

协程 vs 函数：
┌─────────────────┬──────────────────┬──────────────────┐
│     特性        │      函数        │      协程        │
├─────────────────┼──────────────────┼──────────────────┤
│ 执行方式        │ 一次性执行完毕   │ 可以暂停和恢复   │
│ 状态保存        │ 不保存状态       │ 保存局部变量     │
│ 返回方式        │ return 返回一次  │ 可以多次返回值   │
│ 调用开销        │ 低               │ 稍高（状态保存） │
│ 适用场景        │ 同步计算         │ 异步、生成器     │
└─────────────────┴──────────────────┴──────────────────┘
*/

// ============================================
// 常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonErrors() {
    cout << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
❌ 错误1：在不支持协程的编译器/标准下使用协程语法

   co_yield x;   // ❌ C++17 及以下不支持，编译报错

   ✅ 确认编译命令：g++ -std=c++20
   ✅ 确认编译器版本：GCC 10+，Clang 12+，MSVC 19.28+

------------------------------------------------------------

❌ 错误2：协程函数体没有 co_yield/co_await/co_return（不是协程）

   // 以下不是协程：
   Generator func() {
       return value;   // ❌ 普通 return，编译器不把它当协程
   }

   // 是协程：
   Generator func() {
       co_return value;   // ✅ co_return，才是协程
   }

   规则：函数体中至少有一个 co_yield、co_await 或 co_return，才是协程

------------------------------------------------------------

❌ 错误3：混淆 co_yield 和 co_return

   Generator<int> gen() {
       co_return 1;   // ❌ 协程结束，只产生一个值后立即终止
       co_return 2;   // 永远不会执行
   }

   ✅ 产生多个值用 co_yield，最后用 co_return（或省略）：
   Generator<int> gen() {
       co_yield 1;    // ✅ 暂停，产生 1
       co_yield 2;    // ✅ 恢复，产生 2
       co_return;     // ✅ 结束（可省略）
   }

------------------------------------------------------------

❌ 错误4：在协程中使用 std::this_thread::sleep_for 阻塞线程

   Task asyncOp() {
       std::this_thread::sleep_for(1s);   // ❌ 阻塞线程，失去异步意义
       co_return result;
   }

   ✅ 使用 co_await 异步等待定时器（框架提供）：
   Task asyncOp() {
       co_await timer.wait_for(1s);   // ✅ 挂起协程，线程可做其他事
       co_return result;
   }

------------------------------------------------------------

❌ 错误5：协程需要 promise_type 支持，忘记实现（直接编译报错）

   // 返回类型 Generator 必须有嵌套的 promise_type
   struct Generator {
       // ❌ 没有 promise_type → 编译报错："no member named promise_type"
   };

   ✅ C++20 协程框架（实际项目推荐使用 cppcoro 或 libcoro）：
   // 自己实现 promise_type 需要 get_return_object、initial_suspend、
   // final_suspend、unhandled_exception 等，较复杂
   // 学习阶段：理解概念，用现成框架

------------------------------------------------------------

❌ 错误6：协程帧生命周期管理错误（协程 handle 忘记 destroy）

   auto coro = gen();
   coro.resume();
   // ❌ 忘记 coro.destroy() → 内存泄漏（协程帧在堆上分配）

   ✅ RAII 包装自动管理协程帧：
   // 良好的 Generator 类应在析构函数中调用 handle.destroy()
)" << endl;
}

int main() {
    cout << string(60, '=') << endl;
    cout << "第一部分：核心概念详解" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 1.1 概念1：什么是协程？
    // --------------------------------------------

    cout << "\n1.1 什么是协程？" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【协程的定义】

协程是一种特殊的函数，具有以下特点：
1. 可以暂停执行（suspend）
2. 可以恢复执行（resume）
3. 可以多次返回值（yield）
4. 保存执行状态（局部变量、执行位置）

【形象比喻】

普通函数就像一次性火箭：
- 点火 → 飞行 → 爆炸（执行完毕）
- 不能暂停，不能重复使用

协程就像可回收火箭：
- 点火 → 飞行 → 暂停（悬停在空中）
- 继续 → 飞行 → 暂停
- 可以多次暂停和恢复

【实际例子】

想象你在读一本书：
- 普通函数：一口气读完整本书，不能停
- 协程：读一章 → 放下书（暂停）→ 做其他事 → 继续读下一章

【代码对比】

普通函数：
------------------------------------------------------------
vector<int> generate_numbers(int n) {
    vector<int> result;
    for (int i = 0; i < n; ++i) {
        result.push_back(i);  // 一次性生成所有数字
    }
    return result;  // 返回一次
}

// 使用：
auto numbers = generate_numbers(1000000);  // 立即生成100万个数字
// 占用大量内存！

协程（伪代码）：
------------------------------------------------------------
Generator<int> generate_numbers(int n) {
    for (int i = 0; i < n; ++i) {
        co_yield i;  // 返回一个数字，然后暂停
    }
}

// 使用：
auto gen = generate_numbers(1000000);
for (int num : gen) {  // 按需生成，一次一个
    cout << num;       // 只在需要时才生成下一个
}
// 内存占用小！
)" << endl;

    // --------------------------------------------
    // 1.2 概念2：三个关键字 ⭐⭐⭐⭐⭐
    // --------------------------------------------

    cout << "\n1.2 三个关键字" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【co_yield】- 生成值并暂停

作用：
- 返回一个值给调用者
- 暂停协程执行
- 等待下次调用时继续

使用场景：
- 生成器（Generator）
- 惰性求值
- 无限序列

伪代码示例：
------------------------------------------------------------
Generator<int> count_up() {
    int i = 0;
    while (true) {
        co_yield i;  // 返回 i，然后暂停
        ++i;         // 下次恢复时从这里继续
    }
}

// 使用：
auto counter = count_up();
cout << counter.next();  // 输出：0（协程暂停）
cout << counter.next();  // 输出：1（协程恢复，再次暂停）
cout << counter.next();  // 输出：2

执行流程：
1. 调用 count_up() → 创建协程，i=0
2. 执行到 co_yield i → 返回0，暂停
3. 调用 next() → 恢复，++i，i=1
4. 执行到 co_yield i → 返回1，暂停
5. 调用 next() → 恢复，++i，i=2
6. 执行到 co_yield i → 返回2，暂停
...

────────────────────────────────────

【co_await】- 异步等待

作用：
- 等待异步操作完成
- 暂停协程执行
- 操作完成后自动恢复

使用场景：
- 异步 I/O（文件读写、网络请求）
- 等待定时器
- 等待其他协程

伪代码示例：
------------------------------------------------------------
Task<string> read_file_async(string path) {
    auto content = co_await async_read(path);  // 等待读取完成
    co_return content;  // 返回结果
}

// 使用：
auto task = read_file_async("data.txt");
// 协程在等待文件读取时暂停，不阻塞主线程
string content = task.get();  // 获取结果

执行流程：
1. 调用 read_file_async() → 创建协程
2. 执行到 co_await async_read() → 开始异步读取，暂停协程
3. 文件读取中... 主线程可以做其他事
4. 文件读取完成 → 自动恢复协程
5. 执行 co_return content → 返回结果

对比回调函数：
------------------------------------------------------------
// 传统回调（回调地狱）：
async_read("file1.txt", [](string content1) {
    async_read("file2.txt", [content1](string content2) {
        async_read("file3.txt", [content1, content2](string content3) {
            // 嵌套太深，难以理解
        });
    });
});

// 协程（清晰易读）：
Task<void> read_files() {
    auto content1 = co_await async_read("file1.txt");
    auto content2 = co_await async_read("file2.txt");
    auto content3 = co_await async_read("file3.txt");
    // 看起来像同步代码，实际是异步执行
}

────────────────────────────────────

【co_return】- 返回最终结果

作用：
- 返回协程的最终结果
- 结束协程执行
- 类似普通函数的 return

使用场景：
- 协程执行完毕
- 返回计算结果
- 异步任务完成

伪代码示例：
------------------------------------------------------------
Task<int> calculate_async() {
    int result = 0;
    for (int i = 0; i < 10; ++i) {
        result += i;
        co_await sleep(100);  // 模拟耗时操作
    }
    co_return result;  // 返回最终结果
}

// 使用：
auto task = calculate_async();
int result = task.get();  // 获取结果：45

────────────────────────────────────

【三个关键字对比】

┌─────────────┬──────────────┬──────────────┬──────────────┐
│   关键字    │    作用      │   暂停？     │   返回？     │
├─────────────┼──────────────┼──────────────┼──────────────┤
│ co_yield    │ 生成值       │ 是（暂停）   │ 多次返回     │
│ co_await    │ 异步等待     │ 是（暂停）   │ 等待结果     │
│ co_return   │ 返回结果     │ 否（结束）   │ 一次返回     │
└─────────────┴──────────────┴──────────────┴──────────────┘

记忆技巧：
- co_yield：yield = 产出，生成器产出值
- co_await：await = 等待，等待异步操作
- co_return：return = 返回，返回最终结果
)" << endl;

    // --------------------------------------------
    // 1.3 概念3：协程的生命周期
    // --------------------------------------------

    cout << "\n1.3 协程的生命周期" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【协程的生命周期】

1. 创建（Create）
   - 调用协程函数
   - 分配协程帧（保存状态的内存）
   - 初始化局部变量

2. 暂停（Suspend）
   - 遇到 co_yield 或 co_await
   - 保存当前状态（局部变量、执行位置）
   - 返回控制权给调用者

3. 恢复（Resume）
   - 调用者请求下一个值或结果
   - 恢复协程状态
   - 从暂停的地方继续执行

4. 完成（Complete）
   - 遇到 co_return 或函数结束
   - 释放协程帧
   - 返回最终结果

【状态转换图】

    创建
     ↓
   [运行] ←──────┐
     ↓           │
  co_yield/     │
  co_await      │
     ↓           │
   [暂停] ───→ 恢复
     ↓
  co_return
     ↓
   [完成]

【示例：生成器的生命周期】

Generator<int> gen() {
    cout << "1. 协程创建" << endl;

    co_yield 1;  // 2. 暂停
    cout << "3. 协程恢复" << endl;

    co_yield 2;  // 4. 暂停
    cout << "5. 协程恢复" << endl;

    co_return;   // 6. 完成
}

// 使用：
auto g = gen();           // 1. 创建
cout << g.next();         // 2. 暂停，输出：1
cout << g.next();         // 3. 恢复，4. 暂停，输出：2
// g 销毁                 // 6. 完成

输出：
1. 协程创建
1
3. 协程恢复
2
5. 协程恢复
)" << endl;

    // --------------------------------------------
    // 1.4 概念4：协程的应用场景
    // --------------------------------------------

    cout << "\n1.4 协程的应用场景" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【应用场景1：生成器（Generator）】

问题：
- 需要生成大量数据，但不想一次性占用大量内存
- 需要按需生成数据，惰性求值

解决方案：
- 使用 co_yield 实现生成器
- 每次只生成一个值，节省内存

示例：
------------------------------------------------------------
// 生成斐波那契数列
Generator<int> fibonacci() {
    int a = 0, b = 1;
    while (true) {
        co_yield a;
        int temp = a + b;
        a = b;
        b = temp;
    }
}

// 使用：
auto fib = fibonacci();
for (int i = 0; i < 10; ++i) {
    cout << fib.next() << " ";  // 0 1 1 2 3 5 8 13 21 34
}

优势：
- 内存占用小（只保存 a、b 两个变量）
- 可以生成无限序列
- 按需计算，不浪费

────────────────────────────────────

【应用场景2：异步 I/O】

问题：
- 文件读写、网络请求等 I/O 操作耗时长
- 传统同步方式会阻塞线程
- 传统异步方式（回调）代码复杂

解决方案：
- 使用 co_await 实现异步等待
- 代码看起来像同步，实际是异步

示例：
------------------------------------------------------------
// 异步读取文件
Task<string> read_file(string path) {
    auto content = co_await async_read(path);
    co_return content;
}

// 异步写入文件
Task<void> write_file(string path, string content) {
    co_await async_write(path, content);
    co_return;
}

// 使用：
Task<void> process_files() {
    auto content = co_await read_file("input.txt");
    content = process(content);
    co_await write_file("output.txt", content);
}

优势：
- 不阻塞线程，提高并发性能
- 代码清晰，易于理解
- 避免回调地狱

────────────────────────────────────

【应用场景3：状态机】

问题：
- 需要实现复杂的状态机
- 传统方式使用 switch-case，代码复杂

解决方案：
- 使用协程实现状态机
- 每个状态对应一个 co_yield

示例：
------------------------------------------------------------
// 简单的状态机
Generator<string> state_machine() {
    co_yield "State 1: Initializing";
    co_yield "State 2: Processing";
    co_yield "State 3: Finalizing";
    co_return;
}

// 使用：
auto sm = state_machine();
cout << sm.next() << endl;  // State 1: Initializing
cout << sm.next() << endl;  // State 2: Processing
cout << sm.next() << endl;  // State 3: Finalizing

────────────────────────────────────

【应用场景4：协作式多任务】

问题：
- 需要多个任务协作执行
- 不想使用多线程（开销大）

解决方案：
- 使用协程实现协作式多任务
- 任务主动让出 CPU（co_yield）

示例：
------------------------------------------------------------
// 任务1
Generator<void> task1() {
    for (int i = 0; i < 5; ++i) {
        cout << "Task 1: " << i << endl;
        co_yield;  // 让出 CPU
    }
}

// 任务2
Generator<void> task2() {
    for (int i = 0; i < 5; ++i) {
        cout << "Task 2: " << i << endl;
        co_yield;  // 让出 CPU
    }
}

// 调度器
void scheduler() {
    auto t1 = task1();
    auto t2 = task2();

    while (!t1.done() || !t2.done()) {
        if (!t1.done()) t1.next();
        if (!t2.done()) t2.next();
    }
}

输出：
Task 1: 0
Task 2: 0
Task 1: 1
Task 2: 1
...

────────────────────────────────────

【应用场景总结】

┌──────────────┬────────────────┬────────────────┐
│   场景       │   使用关键字   │   优势         │
├──────────────┼────────────────┼────────────────┤
│ 生成器       │ co_yield       │ 节省内存       │
│ 异步 I/O     │ co_await       │ 不阻塞线程     │
│ 状态机       │ co_yield       │ 代码清晰       │
│ 协作式多任务 │ co_yield       │ 轻量级         │
└──────────────┴────────────────┴────────────────┘
)" << endl;

    cout << "\n程序结束" << endl;
    return 0;
}
