// ============================================
// 协程调度策略：FIFO vs 优先级调度 - 超详细注释版
// 日期：2026-06-16（第83天）
// 难度：⭐⭐⭐ 中级进阶
// ============================================

/*
【核心概念】简要说明

是什么？
- FIFO调度：先进先出，所有任务按照添加顺序执行
- 优先级调度：根据任务的优先级决定执行顺序，高优先级任务优先执行

为什么需要？
- FIFO调度简单公平，但无法区分任务的重要性
- 实际场景中，有些任务更重要、更紧急，需要优先处理
- 优先级调度能够更好地满足业务需求

核心特点？
- FIFO：简单、公平、无法区分重要性
- 优先级：灵活、高效、可能导致饥饿问题
- 选择：根据业务场景选择合适的调度策略
*/

#include <iostream>
#include <coroutine>
#include <queue>
#include <vector>
#include <string>
#include <functional>
#include <iomanip>

using namespace std;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

/*
在使用任何代码之前，必须先详细讲解核心概念！
*/

void part1_concepts() {
    cout << string(60, '=') << endl;
    cout << "第一部分：核心概念详解 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 1.1 FIFO调度：先进先出
    // --------------------------------------------

    cout << "\n1.1 FIFO调度（First-In-First-Out）" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【FIFO调度】

是什么？
- FIFO = First-In-First-Out（先进先出）
- 按照任务添加的顺序执行
- 就像排队买票，先来先服务

核心特点：
- ✅ 简单易懂：实现简单，逻辑清晰
- ✅ 公平性好：所有任务平等对待
- ✅ 无饥饿：每个任务都能得到执行
- ❌ 无法区分重要性：紧急任务也要排队

适用场景：
- 所有任务优先级相同
- 追求公平性
- 简单的任务队列
)" << endl;

    // FIFO执行示例
    cout << "FIFO执行顺序示例：" << endl;
    cout << "添加顺序：Task1 → Task2 → Task3 → Task4" << endl;
    cout << "执行顺序：Task1 → Task2 → Task3 → Task4" << endl;
    cout << "特点：严格按照添加顺序执行" << endl;

    // --------------------------------------------
    // 1.2 优先级调度：按重要性排序
    // --------------------------------------------

    cout << "\n1.2 优先级调度（Priority Scheduling）" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【优先级调度】

是什么？
- 每个任务有一个优先级值（数字）
- 高优先级任务优先执行
- 就像医院急诊，重病患者优先

核心特点：
- ✅ 灵活：可以根据重要性调整
- ✅ 高效：重要任务快速响应
- ✅ 符合实际需求：业务场景往往有优先级
- ❌ 可能饥饿：低优先级任务可能永远得不到执行
- ❌ 实现复杂：需要维护优先级队列

适用场景：
- 任务有明确的重要性差异
- 需要快速响应重要任务
- 可以容忍低优先级任务延迟
)" << endl;

    // 优先级执行示例
    cout << "优先级执行顺序示例：" << endl;
    cout << "添加顺序：Task1(优先级3) → Task2(优先级1) → Task3(优先级5) → Task4(优先级2)" << endl;
    cout << "执行顺序：Task3(5) → Task1(3) → Task4(2) → Task2(1)" << endl;
    cout << "特点：优先级高的先执行，与添加顺序无关" << endl;

    // --------------------------------------------
    // 1.3 FIFO vs 优先级：对比分析
    // --------------------------------------------

    cout << "\n1.3 FIFO vs 优先级调度对比" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【对比表】

┌──────────┬──────────────┬──────────────┐
│  维度    │  FIFO调度    │  优先级调度  │
├──────────┼──────────────┼──────────────┤
│ 执行顺序 │ 添加顺序     │ 优先级顺序   │
│ 公平性   │ 完全公平     │ 不公平       │
│ 饥饿问题 │ 无           │ 可能有       │
│ 响应时间 │ 平均         │ 重要任务快   │
│ 实现复杂度│ 简单        │ 中等         │
│ 适用场景 │ 任务平等     │ 任务有优先级 │
└──────────┴──────────────┴──────────────┘

【选择指南】

使用FIFO调度：
- ✅ 所有任务同等重要
- ✅ 追求公平性
- ✅ 简单场景

使用优先级调度：
- ✅ 任务有明确的重要性差异
- ✅ 需要快速响应重要任务
- ✅ 可以接受低优先级任务延迟
)" << endl;

    // --------------------------------------------
    // 1.4 实际场景示例
    // --------------------------------------------

    cout << "\n1.4 实际场景示例" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【场景1：Web服务器】

任务类型：
- 用户请求（优先级：高）
- 日志写入（优先级：中）
- 缓存清理（优先级：低）

使用优先级调度：
- 用户请求快速响应
- 日志写入及时完成
- 缓存清理在空闲时执行

【场景2：游戏引擎】

任务类型：
- 渲染帧（优先级：高）
- 音效播放（优先级：中）
- 资源加载（优先级：低）

使用优先级调度：
- 保证60FPS流畅渲染
- 音效及时播放
- 资源后台加载

【场景3：批处理系统】

任务类型：
- 数据转换任务1
- 数据转换任务2
- 数据转换任务3

使用FIFO调度：
- 所有任务同等重要
- 按顺序依次处理
- 公平对待每个任务
)" << endl;

    // --------------------------------------------
    // 1.5 重要数据结构：std::priority_queue ⭐⭐⭐⭐⭐
    // --------------------------------------------

    cout << "\n1.5 核心数据结构：std::priority_queue" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【函数卡片】

容器名：std::priority_queue
作用：优先级队列，自动按优先级排序
类型：STL容器适配器

完整语法：
template<
    class T,                                    // 元素类型
    class Container = std::vector<T>,           // 底层容器（默认vector）
    class Compare = std::less<typename Container::value_type>  // 比较器（默认less，大顶堆）
> class priority_queue;

三个模板参数：
1. T：元素类型
   - 可以是任何类型
   - 示例：int, string, coroutine_handle

2. Container：底层容器（可选）
   - 默认：std::vector<T>
   - 也可以用：std::deque<T>

3. Compare：比较器（可选）
   - 默认：std::less<T>（大顶堆，最大元素在堆顶）
   - 也可以用：std::greater<T>（小顶堆，最小元素在堆顶）
   - 或自定义比较器

常用方法：
- push(value)：插入元素，自动排序
- pop()：删除堆顶元素（最高优先级）
- top()：访问堆顶元素（不删除）
- empty()：检查是否为空
- size()：获取元素个数

示例：
// 默认：大顶堆（最大值在堆顶）
std::priority_queue<int> pq1;
pq1.push(3); pq1.push(1); pq1.push(5);
pq1.top();  // 返回5（最大值）

// 小顶堆（最小值在堆顶）
std::priority_queue<int, vector<int>, greater<int>> pq2;
pq2.push(3); pq2.push(1); pq2.push(5);
pq2.top();  // 返回1（最小值）

// 自定义比较器
auto cmp = [](int a, int b) { return a > b; };
std::priority_queue<int, vector<int>, decltype(cmp)> pq3(cmp);
)" << endl;

    // priority_queue 实际演示
    cout << "\n实际演示：priority_queue的使用" << endl;

    // 大顶堆示例
    priority_queue<int> max_heap;
    cout << "\n大顶堆（默认）：" << endl;
    max_heap.push(3);
    max_heap.push(1);
    max_heap.push(5);
    max_heap.push(2);
    cout << "插入顺序：3, 1, 5, 2" << endl;
    cout << "弹出顺序：";
    while (!max_heap.empty()) {
        cout << max_heap.top() << " ";
        max_heap.pop();
    }
    cout << "（从大到小）" << endl;

    // 小顶堆示例
    priority_queue<int, vector<int>, greater<int>> min_heap;
    cout << "\n小顶堆（使用greater<int>）：" << endl;
    min_heap.push(3);
    min_heap.push(1);
    min_heap.push(5);
    min_heap.push(2);
    cout << "插入顺序：3, 1, 5, 2" << endl;
    cout << "弹出顺序：";
    while (!min_heap.empty()) {
        cout << min_heap.top() << " ";
        min_heap.pop();
    }
    cout << "（从小到大）" << endl;
}

// ============================================
// 第二部分：优先级调度器实现 ⭐⭐⭐⭐⭐
// ============================================

/*
基于昨天学习的简单调度器，今天实现优先级调度器
核心改进：使用 priority_queue 替代 queue
*/

// --------------------------------------------
// 2.1 任务结构：带优先级的协程
// --------------------------------------------

// Task结构：协程返回类型
struct Task {
    struct promise_type {
        int priority = 0;  // 任务优先级（数字越大优先级越高）
        int task_id = 0;   // 任务ID（用于标识）

        Task get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };

    std::coroutine_handle<promise_type> handle;

    // 获取优先级
    int priority() const {
        return handle.promise().priority;
    }

    // 获取任务ID
    int id() const {
        return handle.promise().task_id;
    }
};

// --------------------------------------------
// 2.2 优先级比较器：定义任务的优先级关系
// --------------------------------------------

/*
【核心概念】自定义比较器

是什么？
- 比较器是一个函数对象（仿函数）或lambda
- 定义两个元素的大小关系
- priority_queue 使用比较器来维护堆

为什么需要？
- priority_queue 默认是大顶堆（最大元素在堆顶）
- 但协程需要高优先级先执行
- 需要自定义比较器来实现我们的优先级逻辑

如何使用？
- 定义一个比较器类或lambda
- 作为 priority_queue 的第三个模板参数
- 比较器返回 true 表示第一个参数优先级"低于"第二个参数
*/

struct TaskComparator {
    // 比较两个任务的优先级
    // 注意：返回 true 表示 a 的优先级"低于" b（a 应该排在后面）
    // 这样高优先级的任务会在堆顶
    bool operator()(const Task& a, const Task& b) const {
        // 优先级高的任务优先（数字大的优先）
        return a.priority() < b.priority();

        // 如果优先级相同，可以考虑其他因素，例如：
        // return a.priority() < b.priority() ||
        //        (a.priority() == b.priority() && a.id() > b.id());
    }
};

// --------------------------------------------
// 2.3 优先级调度器：完整实现
// --------------------------------------------

class PriorityScheduler {
private:
    // 优先级队列：自动按优先级排序
    // 第一个参数：元素类型（Task）
    // 第二个参数：底层容器（vector<Task>）
    // 第三个参数：比较器（TaskComparator）
    std::priority_queue<Task, std::vector<Task>, TaskComparator> task_queue;

    bool running = false;
    int completed_count = 0;  // 完成的任务数量

public:
    // 调度协程（添加到优先级队列）
    void schedule(Task task) {
        if (task.handle) {
            cout << "[Scheduler] 调度任务 " << task.id()
                 << "（优先级 " << task.priority() << "）" << endl;
            task_queue.push(task);  // priority_queue 会自动排序
        }
    }

    // 运行调度器（事件循环）
    void run() {
        running = true;
        cout << "\n[Scheduler] 开始运行，队列中有 " << task_queue.size() << " 个任务" << endl;

        while (running && !task_queue.empty()) {
            // 1. 从队列取出最高优先级的任务
            Task task = task_queue.top();
            task_queue.pop();

            auto handle = task.handle;

            // 2. 检查协程是否完成
            if (handle.done()) {
                cout << "[Scheduler] 任务 " << task.id() << " 已完成，销毁" << endl;
                handle.destroy();
                completed_count++;
                continue;
            }

            // 3. 恢复协程
            cout << "[Scheduler] 执行任务 " << task.id()
                 << "（优先级 " << task.priority() << "）" << endl;
            handle.resume();

            // 4. 检查状态
            if (!handle.done()) {
                // 未完成：重新入队（优先级不变）
                cout << "[Scheduler] 任务 " << task.id() << " 暂停，重新入队" << endl;
                task_queue.push(task);
            } else {
                // 已完成：销毁
                cout << "[Scheduler] 任务 " << task.id() << " 完成，销毁" << endl;
                handle.destroy();
                completed_count++;
            }
        }

        running = false;
        cout << "\n[Scheduler] 调度器停止，共完成 " << completed_count << " 个任务" << endl;
    }

    // 停止调度器
    void stop() {
        running = false;
    }

    // 获取队列大小
    size_t size() const {
        return task_queue.size();
    }

    // 获取完成的任务数量
    int completed() const {
        return completed_count;
    }
};

// --------------------------------------------
// 2.4 协程任务创建函数
// --------------------------------------------

// 创建带优先级的任务（协程函数）
Task createPriorityTask(int id, int priority, const string& name) {
    cout << "  → 任务 " << id << "（" << name << "）开始执行" << endl;
    co_await std::suspend_always{};  // 第一次暂停

    cout << "  → 任务 " << id << "（" << name << "）继续执行" << endl;
    co_await std::suspend_always{};  // 第二次暂停

    cout << "  → 任务 " << id << "（" << name << "）完成" << endl;
    co_return;
}

// --------------------------------------------
// 2.5 使用示例：多个不同优先级的任务
// --------------------------------------------

void part2_priority_scheduler() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分：优先级调度器实现" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【核心改进】

昨天的FIFO调度器：
- 使用 std::queue<coroutine_handle<>>
- 按照添加顺序执行

今天的优先级调度器：
- 使用 std::priority_queue<Task, vector<Task>, TaskComparator>
- 按照优先级顺序执行

关键点：
1. Task 结构包含优先级信息
2. TaskComparator 定义优先级关系
3. priority_queue 自动维护优先级顺序
)" << endl;

    cout << "\n示例：Web服务器任务调度" << endl;
    cout << string(60, '-') << endl;

    PriorityScheduler scheduler;

    // 创建不同优先级的任务
    cout << "\n创建任务：" << endl;

    // 任务1：日志写入（低优先级）
    Task task1 = createPriorityTask(1, 1, "日志写入");
    task1.handle.promise().task_id = 1;
    task1.handle.promise().priority = 1;
    scheduler.schedule(task1);

    // 任务2：用户请求（高优先级）
    Task task2 = createPriorityTask(2, 5, "用户请求");
    task2.handle.promise().task_id = 2;
    task2.handle.promise().priority = 5;
    scheduler.schedule(task2);

    // 任务3：缓存清理（低优先级）
    Task task3 = createPriorityTask(3, 1, "缓存清理");
    task3.handle.promise().task_id = 3;
    task3.handle.promise().priority = 1;
    scheduler.schedule(task3);

    // 任务4：API调用（中优先级）
    Task task4 = createPriorityTask(4, 3, "API调用");
    task4.handle.promise().task_id = 4;
    task4.handle.promise().priority = 3;
    scheduler.schedule(task4);

    // 任务5：紧急用户请求（最高优先级）
    Task task5 = createPriorityTask(5, 10, "紧急用户请求");
    task5.handle.promise().task_id = 5;
    task5.handle.promise().priority = 10;
    scheduler.schedule(task5);

    cout << "\n执行顺序观察：" << endl;
    cout << "预期：任务5(10) → 任务2(5) → 任务4(3) → 任务1(1) → 任务3(1)" << endl;
    cout << string(60, '-') << endl;

    // 运行调度器
    scheduler.run();

    cout << "\n" << string(60, '-') << endl;
    cout << "观察到的执行顺序：" << endl;
    cout << "- 高优先级任务（5, 10）先执行" << endl;
    cout << "- 中优先级任务（4）其次" << endl;
    cout << "- 低优先级任务（1, 3）最后执行" << endl;
    cout << "- 与添加顺序无关，完全按优先级决定" << endl;
}

// ============================================
// 第三部分：FIFO vs 优先级对比实验 ⭐⭐⭐⭐
// ============================================

/*
通过概念对比说明FIFO和优先级调度的区别
*/

void part3_comparison() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第三部分：FIFO vs 优先级对比分析" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【实验目的】

对比两种调度策略的执行顺序：
1. FIFO调度：严格按照添加顺序执行
2. 优先级调度：按照优先级高低执行

【实验设置】

5个任务，添加顺序和优先级：
- 任务A：优先级3（中等）
- 任务B：优先级1（低）
- 任务C：优先级5（高）
- 任务D：优先级2（低）
- 任务E：优先级4（较高）

添加顺序：A → B → C → D → E
)" << endl;

    // 实验1：FIFO调度
    cout << "\n【实验1：FIFO调度】" << endl;
    cout << string(60, '-') << endl;

    cout << "添加顺序：A → B → C → D → E" << endl;
    cout << "执行顺序：A → B → C → D → E" << endl;
    cout << "结论：严格按照添加顺序执行" << endl;

    cout << R"(
特点：
- 公平：所有任务平等对待
- 可预测：执行顺序固定
- 简单：实现容易
- 局限：无法优先处理重要任务
)" << endl;

    // 实验2：优先级调度
    cout << "\n【实验2：优先级调度】" << endl;
    cout << string(60, '-') << endl;

    cout << "添加顺序：A(3) → B(1) → C(5) → D(2) → E(4)" << endl;
    cout << "执行顺序：C(5) → E(4) → A(3) → D(2) → B(1)" << endl;
    cout << "结论：按照优先级从高到低执行" << endl;

    cout << R"(
特点：
- 灵活：可以优先处理重要任务
- 高效：重要任务快速响应
- 符合实际：业务场景往往有优先级
- 风险：低优先级任务可能"饿死"
)" << endl;

    // 对比总结
    cout << "\n【对比总结】" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
┌──────────┬──────────────────┬──────────────────┐
│  维度    │  FIFO调度        │  优先级调度      │
├──────────┼──────────────────┼──────────────────┤
│ 执行顺序 │ A→B→C→D→E       │ C→E→A→D→B       │
│ 决定因素 │ 添加时间         │ 优先级           │
│ 公平性   │ 完全公平         │ 不公平           │
│ 响应时间 │ 平均             │ 高优先级快       │
│ 饥饿问题 │ 无               │ 可能有           │
│ 适用场景 │ 任务平等         │ 任务有优先级     │
└──────────┴──────────────────┴──────────────────┘

FIFO调度：
✅ 添加顺序 = 执行顺序
✅ 公平、可预测
❌ 无法优先处理重要任务

优先级调度：
✅ 重要任务优先执行
✅ 符合业务需求
❌ 低优先级任务可能延迟
❌ 可能出现饥饿问题

【实际场景示例】

Web服务器场景：
- 用户请求（优先级10）
- API调用（优先级5）
- 日志写入（优先级3）
- 缓存清理（优先级1）

FIFO调度：
如果日志写入先到达，用户请求也要等待
→ 用户体验差

优先级调度：
用户请求立即执行，日志写入可以等待
→ 用户体验好

结论：
- 简单场景用FIFO（公平、简单）
- 复杂场景用优先级（灵活、高效）
)" << endl;
}

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void part4_common_errors() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 4.1 错误1：比较器逻辑错误
    // --------------------------------------------

    cout << "\n4.1 错误1：比较器逻辑错误" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
struct WrongComparator {
    bool operator()(const Task& a, const Task& b) const {
        return a.priority() > b.priority();  // 错误：这样会导致低优先级先执行
    }
};

结果：低优先级任务先执行（与预期相反）

原因：
- priority_queue 是"大顶堆"
- 比较器返回 true 表示 a 的优先级"低于" b
- a.priority() > b.priority() 返回 true 时，说明 a 的优先级数字更大
- 但 priority_queue 会把 a 放在后面（因为认为 a 优先级更低）
- 结果：优先级数字大的反而后执行

✅ 正确代码：
struct CorrectComparator {
    bool operator()(const Task& a, const Task& b) const {
        return a.priority() < b.priority();  // 正确：优先级高的先执行
    }
};

结果：高优先级任务先执行（符合预期）

记忆技巧：
- priority_queue 的比较器定义"谁的优先级更低"
- 返回 true 表示第一个参数优先级更低（应该排后面）
- 所以：a.priority() < b.priority() 表示 a 优先级更低
- priority_queue 会把优先级高的放在堆顶
)" << endl;

    // --------------------------------------------
    // 4.2 错误2：优先级饥饿问题
    // --------------------------------------------

    cout << "\n4.2 错误2：优先级饥饿（Starvation）" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 问题场景：
// 不断有高优先级任务添加
while (true) {
    scheduler.schedule(createTask(++id, 10, "紧急任务"));
    // 低优先级任务永远得不到执行
}

结果：低优先级任务"饿死"，永远等待

原因：
- 高优先级任务源源不断
- 调度器总是优先执行高优先级任务
- 低优先级任务永远排在后面

✅ 解决方案1：优先级老化（Aging）
// 等待时间越长，优先级越高
int effective_priority = base_priority + (current_time - enqueue_time) / 100;

原理：
- 随着等待时间增加，优先级提升
- 最终低优先级任务也能得到执行

✅ 解决方案2：公平调度
// 定期检查低优先级任务
if (low_priority_wait_time > threshold) {
    force_execute_low_priority_task();
}

✅ 解决方案3：混合策略
// 80%时间优先级调度，20%时间FIFO
if (rand() % 100 < 80) {
    execute_by_priority();
} else {
    execute_by_fifo();
}

记忆技巧：
- 优先级调度要小心饥饿问题
- 生产环境必须有防护机制
- 考虑使用老化或混合策略
)" << endl;

    // --------------------------------------------
    // 4.3 错误3：优先级反转
    // --------------------------------------------

    cout << "\n4.3 错误3：优先级反转（Priority Inversion）" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 问题场景：
任务A（低优先级）：持有锁
任务B（中优先级）：不需要锁，正在执行
任务C（高优先级）：等待锁（被任务A阻塞）

执行顺序：B → A → C
问题：高优先级任务C被中优先级任务B延迟

原因：
- 任务C等待任务A释放锁
- 任务A被任务B抢占
- 结果：任务C反而最后执行

✅ 解决方案：优先级继承（Priority Inheritance）
// 持有锁的任务临时提升优先级
if (task_a.holds_lock && task_c.waiting_for_lock) {
    task_a.priority = max(task_a.priority, task_c.priority);
}

原理：
- 任务A继承任务C的高优先级
- 任务A优先于任务B执行
- 任务A释放锁后，任务C立即执行

记忆技巧：
- 优先级反转是实时系统的经典问题
- 涉及锁时要特别注意
- 使用优先级继承或优先级天花板协议
)" << endl;

    // --------------------------------------------
    // 4.4 错误4：忘记设置优先级
    // --------------------------------------------

    cout << "\n4.4 错误4：忘记设置优先级" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
Task task = createTask(1, "重要任务");
// 忘记设置优先级
scheduler.schedule(task);

结果：使用默认优先级（通常是0），可能不符合预期

原因：
- Task 结构的默认优先级是0
- 如果忘记设置，所有任务优先级相同
- 退化为FIFO调度

✅ 正确代码：
Task task = createTask(1, 10, "重要任务");  // 明确设置优先级
task.handle.resume();
task.handle.promise().priority = 10;  // 或者手动设置
scheduler.schedule(task);

预防措施：
- 封装创建函数，强制指定优先级
- 使用枚举定义优先级常量
- 添加断言检查优先级是否有效

enum class Priority {
    LOW = 1,
    NORMAL = 5,
    HIGH = 10,
    CRITICAL = 20
};
)" << endl;

    // --------------------------------------------
    // 4.5 错误5：优先级相同时的不确定性
    // --------------------------------------------

    cout << "\n4.5 错误5：优先级相同时的执行顺序不确定" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 问题场景：
// 三个任务，优先级都是5
scheduler.schedule(createTask(1, 5, "任务1"));
scheduler.schedule(createTask(2, 5, "任务2"));
scheduler.schedule(createTask(3, 5, "任务3"));

结果：执行顺序不确定（可能是1→2→3，也可能是其他顺序）

原因：
- priority_queue 不保证相同优先级的顺序
- 堆的内部结构决定了顺序
- 不同的实现可能有不同的结果

✅ 解决方案：添加次要排序条件
struct ImprovedComparator {
    bool operator()(const Task& a, const Task& b) const {
        if (a.priority() != b.priority()) {
            return a.priority() < b.priority();  // 按优先级排序
        }
        return a.id() > b.id();  // 优先级相同，按ID排序（先进先出）
    }
};

结果：优先级相同时，按添加顺序执行（FIFO）

记忆技巧：
- 优先级队列不保证相同优先级的顺序
- 如果需要确定性，添加次要排序条件
- 常用次要条件：ID、时间戳、序号
)" << endl;
}

// ============================================
// 第五部分：函数卡片速查
// ============================================

void part5_function_reference() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第五部分：函数卡片速查" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【std::priority_queue】

语法：
template<class T, class Container = vector<T>, class Compare = less<T>>
class priority_queue;

常用方法：
- push(value)：插入元素（O(log n)）
- pop()：删除堆顶（O(log n)）
- top()：访问堆顶（O(1)）
- empty()：是否为空（O(1)）
- size()：元素个数（O(1)）

示例：
priority_queue<int> pq;                          // 大顶堆
priority_queue<int, vector<int>, greater<int>> pq;  // 小顶堆
priority_queue<Task, vector<Task>, TaskComparator> pq;  // 自定义

────────────────────────────────────

【自定义比较器】

函数对象形式：
struct Comparator {
    bool operator()(const T& a, const T& b) const {
        return a.priority < b.priority;  // a优先级低于b
    }
};

Lambda形式：
auto cmp = [](const T& a, const T& b) {
    return a.priority < b.priority;
};
priority_queue<T, vector<T>, decltype(cmp)> pq(cmp);

注意事项：
- 返回 true 表示第一个参数优先级"低于"第二个参数
- 优先级高的会被放在堆顶
- 可以组合多个排序条件

────────────────────────────────────

【优先级调度器】

核心接口：
void schedule(Task task);  // 调度任务
void run();                // 运行调度器
void stop();               // 停止调度器
size_t size();             // 队列大小

使用示例：
PriorityScheduler scheduler;
scheduler.schedule(task1);
scheduler.schedule(task2);
scheduler.run();

────────────────────────────────────

【Task结构】

定义：
struct Task {
    struct promise_type { ... };
    coroutine_handle<promise_type> handle;
    int priority() const;
    int id() const;
};

创建任务：
Task task = createTask(id, priority, name);
task.handle.resume();  // 启动协程
scheduler.schedule(task);
)" << endl;
}

// ============================================
// 第六部分：练习题
// ============================================

void part6_exercises() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第六部分：练习题" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
1. 实现一个带优先级老化的调度器
   要求：等待时间越长，优先级越高
   提示：记录任务的入队时间，计算 effective_priority = base_priority + age

2. 实现一个优先级范围限制的调度器
   要求：优先级必须在1-10之间
   提示：在 schedule() 方法中检查优先级，无效则拒绝或调整

3. 实现一个支持动态优先级调整的调度器
   要求：可以在运行时改变任务的优先级
   提示：需要重新构建堆（remove + push with new priority）

4. 对比优先级调度和FIFO调度的平均等待时间
   要求：统计每个任务的等待时间，计算平均值
   提示：记录入队时间和开始执行时间，waiting_time = start_time - enqueue_time

5. 实现一个混合调度器
   要求：80%时间使用优先级调度，20%时间使用FIFO
   提示：随机数或计数器决定使用哪种策略

6. 解决饥饿问题
   要求：确保低优先级任务在一定时间内得到执行
   提示：检查任务等待时间，超过阈值则强制执行

7. 实现多级反馈队列调度器
   要求：根据任务的执行历史动态调整优先级
   提示：任务执行时间越长，优先级越低（防止长任务占用CPU）
)" << endl;
}

// ============================================
// 主函数
// ============================================

int main() {
    cout << "============================================" << endl;
    cout << "协程调度策略：FIFO vs 优先级调度" << endl;
    cout << "============================================" << endl;

    // 第一部分：核心概念详解
    part1_concepts();

    // 第二部分：优先级调度器实现
    part2_priority_scheduler();

    // 第三部分：FIFO vs 优先级对比
    part3_comparison();

    // 第四部分：常见错误和陷阱
    part4_common_errors();

    // 第五部分：函数卡片速查
    part5_function_reference();

    // 第六部分：练习题
    part6_exercises();

    cout << "\n" << string(60, '=') << endl;
    cout << "学习完成！" << endl;
    cout << string(60, '=') << endl;

    return 0;
}
