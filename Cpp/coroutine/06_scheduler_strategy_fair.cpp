// ============================================
// 协程调度策略：公平调度 - 超详细注释版
// 日期：2026-06-16（第83天）
// 难度：⭐⭐⭐ 中级进阶
// ============================================

/*
【核心概念】简要说明

是什么？
- 公平调度：确保所有任务都有平等的执行机会
- 轮询调度（Round-Robin）：按顺序轮流执行每个任务
- 时间片轮转（Time Slicing）：每个任务执行固定次数后切换

为什么需要？
- 优先级调度可能导致低优先级任务"饥饿"（永远得不到执行）
- 公平调度保证每个任务都有机会执行
- 平衡效率和公平性

核心特点？
- 公平性：所有任务平等对待
- 防饥饿：每个任务都能得到执行
- 响应性：任务不会等待太久
*/

#include <iostream>
#include <coroutine>
#include <queue>
#include <deque>
#include <vector>
#include <string>
#include <chrono>
#include <iomanip>

using namespace std;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

void part1_concepts() {
    cout << string(60, '=') << endl;
    cout << "第一部分：公平调度核心概念 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 1.1 什么是公平调度
    // --------------------------------------------

    cout << "\n1.1 公平调度（Fair Scheduling）" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【公平调度】

是什么？
- 公平调度确保所有任务都有平等的执行机会
- 不论任务的优先级，都能得到执行
- 类似于"雨露均沾"，每个任务都分配到资源

核心目标：
- ✅ 防止饥饿：低优先级任务也能执行
- ✅ 公平性：所有任务平等对待
- ✅ 响应性：任务不会等待太久
- ✅ 可预测：执行顺序可预期

适用场景：
- 所有任务都很重要（无明显优先级差异）
- 不能容忍任务"饿死"
- 需要保证响应时间
- 交互式系统、实时系统
)" << endl;

    // --------------------------------------------
    // 1.2 轮询调度（Round-Robin）
    // --------------------------------------------

    cout << "\n1.2 轮询调度（Round-Robin, RR）" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【轮询调度】

是什么？
- 按照固定顺序轮流执行每个任务
- 每个任务执行一次后，移到队列末尾
- 就像轮流发言，每人说一句

工作流程：
1. 从队列头取出任务
2. 执行任务一次（或一个时间片）
3. 如果任务未完成，放到队列尾
4. 继续处理下一个任务

示例：
任务队列：[A, B, C]
执行顺序：A → B → C → A → B → C → A → B → C → ...
特点：循环往复，每个任务轮流执行

优点：
- ✅ 简单易懂
- ✅ 公平性好
- ✅ 无饥饿问题
- ✅ 响应时间可预测

缺点：
- ❌ 无法区分任务优先级
- ❌ 长任务影响短任务的响应时间
- ❌ 上下文切换开销
)" << endl;

    // 轮询调度示意图
    cout << "\n轮询调度示意图：" << endl;
    cout << R"(
初始队列：[A, B, C]

第1轮：
  执行A → 队列：[B, C, A]
  执行B → 队列：[C, A, B]
  执行C → 队列：[A, B, C]

第2轮：
  执行A → 队列：[B, C, A]
  执行B → 队列：[C, A, B]
  执行C → 队列：[A, B, C]

规律：每个任务轮流执行，循环往复
)" << endl;

    // --------------------------------------------
    // 1.3 时间片轮转（Time Slicing）
    // --------------------------------------------

    cout << "\n1.3 时间片轮转（Time Slicing）" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【时间片轮转】

是什么？
- 每个任务分配固定的执行时间（时间片）
- 时间片用完后，强制切换到下一个任务
- 类似于限时发言，时间到了就轮到下一个人

核心概念：
1. 时间片（Time Quantum/Time Slice）
   - 任务连续执行的最大时间
   - 典型值：10ms、20ms、50ms
   - 或者：执行N次resume()

2. 上下文切换（Context Switch）
   - 保存当前任务的状态
   - 加载下一个任务的状态
   - 协程的暂停/恢复机制

工作流程：
1. 任务开始执行
2. 执行时间片（例如：10次resume）
3. 时间片用完 → 强制暂停
4. 切换到下一个任务
5. 重复上述过程

示例（时间片=2次）：
任务A需要6次resume完成
任务B需要4次resume完成
任务C需要2次resume完成

执行顺序：
  A(2次) → B(2次) → C(2次,完成) → A(2次) → B(2次,完成) → A(2次,完成)

特点：
- 每个任务最多连续执行2次
- 长任务被分割成多段
- 短任务可以快速完成

优点：
- ✅ 响应性好（短任务不会等太久）
- ✅ 公平性好（每个任务分配到时间）
- ✅ 适合交互式系统

缺点：
- ❌ 时间片太小 → 切换开销大
- ❌ 时间片太大 → 响应时间长
- ❌ 需要调优时间片大小
)" << endl;

    // 时间片大小的影响
    cout << "\n时间片大小的影响：" << endl;
    cout << R"(
时间片太小（例如：1次resume）
- 优点：响应快
- 缺点：切换频繁，开销大
- 极端情况：退化为逐个执行

时间片太大（例如：100次resume）
- 优点：切换少，开销小
- 缺点：响应慢
- 极端情况：退化为FIFO

合适的时间片（例如：5-10次resume）
- 平衡响应性和开销
- 根据任务特点调整
- 经验法则：80-90%时间执行任务，10-20%时间切换
)" << endl;

    // --------------------------------------------
    // 1.4 公平调度 vs 优先级调度
    // --------------------------------------------

    cout << "\n1.4 公平调度 vs 优先级调度对比" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【对比表】

┌──────────┬──────────────┬──────────────┐
│  维度    │  公平调度    │  优先级调度  │
├──────────┼──────────────┼──────────────┤
│ 执行顺序 │ 轮流执行     │ 优先级顺序   │
│ 公平性   │ 完全公平     │ 不公平       │
│ 饥饿问题 │ 无           │ 可能有       │
│ 响应时间 │ 可预测       │ 高优先级快   │
│ 适用场景 │ 所有任务重要 │ 任务有优先级 │
└──────────┴──────────────┴──────────────┘

【选择指南】

使用公平调度：
- ✅ 所有任务同等重要
- ✅ 不能容忍饥饿
- ✅ 需要可预测的响应时间
- ✅ 交互式系统

使用优先级调度：
- ✅ 任务有明确优先级
- ✅ 可以接受低优先级延迟
- ✅ 追求高优先级快速响应

混合策略：
- 多级队列：高优先级队列 + 低优先级队列
- 优先级+时间片：高优先级任务优先，但限制连续执行时间
- 公平+权重：按权重分配时间片
)" << endl;

    // --------------------------------------------
    // 1.5 实际场景示例
    // --------------------------------------------

    cout << "\n1.5 实际场景示例" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【场景1：多用户系统】

任务类型：
- 用户A的请求
- 用户B的请求
- 用户C的请求

使用公平调度：
- 每个用户轮流获得CPU时间
- 保证每个用户都有响应
- 避免某个用户独占资源

效果：
- 用户A感觉系统响应快
- 用户B感觉系统响应快
- 用户C感觉系统响应快
- 所有用户满意度高

【场景2：游戏服务器】

任务类型：
- 玩家1的操作
- 玩家2的操作
- 玩家3的操作
- NPC AI计算

使用时间片轮转：
- 每个玩家的操作都能及时响应
- NPC AI不会阻塞玩家操作
- 保证游戏流畅性

效果：
- 延迟低：每个操作快速响应
- 流畅性好：没有卡顿
- 公平性好：每个玩家平等对待

【场景3：批处理系统】

任务类型：
- 数据导入任务（长任务）
- 报表生成任务（中等任务）
- 数据校验任务（短任务）

问题：
- 如果用FIFO，长任务会阻塞短任务
- 短任务要等很久

解决方案：
- 使用时间片轮转
- 长任务被分割成多段
- 短任务可以快速完成

效果：
- 短任务响应快
- 长任务仍能完成
- 总体吞吐量不变
)" << endl;
}

// ============================================
// 第二部分：轮询调度器实现 ⭐⭐⭐⭐⭐
// ============================================

// Task结构：协程返回类型
struct Task {
    struct promise_type {
        int task_id = 0;
        string task_name;
        int execute_count = 0;  // 已执行次数

        Task get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };

    std::coroutine_handle<promise_type> handle;

    int id() const { return handle.promise().task_id; }
    string name() const { return handle.promise().task_name; }
    int execute_count() const { return handle.promise().execute_count; }
};

// 轮询调度器（Round-Robin Scheduler）
class RoundRobinScheduler {
private:
    std::deque<Task> task_queue;  // 使用deque，方便头尾操作
    bool running = false;
    int total_executions = 0;  // 总执行次数

public:
    // 调度任务
    void schedule(Task task) {
        if (task.handle) {
            cout << "[Scheduler] 调度任务 " << task.id()
                 << "（" << task.name() << "）" << endl;
            task_queue.push_back(task);
        }
    }

    // 运行调度器（轮询模式）
    void run() {
        running = true;
        int round = 1;

        cout << "\n[Scheduler] 开始轮询调度，队列中有 "
             << task_queue.size() << " 个任务" << endl;

        while (running && !task_queue.empty()) {
            cout << "\n--- 第 " << round << " 轮 ---" << endl;

            size_t queue_size = task_queue.size();

            // 轮询队列中的每个任务
            for (size_t i = 0; i < queue_size; ++i) {
                // 从队列头取出任务
                Task task = task_queue.front();
                task_queue.pop_front();

                auto handle = task.handle;

                // 检查是否完成
                if (handle.done()) {
                    cout << "[Scheduler] 任务 " << task.id() << " 已完成，销毁" << endl;
                    handle.destroy();
                    continue;
                }

                // 执行任务一次
                cout << "[Scheduler] 执行任务 " << task.id()
                     << "（" << task.name() << "）";
                handle.resume();
                handle.promise().execute_count++;
                total_executions++;

                cout << " [执行 " << handle.promise().execute_count << " 次]" << endl;

                // 检查是否完成
                if (!handle.done()) {
                    // 未完成：放到队列尾
                    task_queue.push_back(task);
                } else {
                    // 已完成：销毁
                    cout << "[Scheduler] 任务 " << task.id() << " 完成，销毁" << endl;
                    handle.destroy();
                }
            }

            round++;
        }

        running = false;
        cout << "\n[Scheduler] 调度器停止，总共执行 " << total_executions << " 次" << endl;
    }

    // 停止调度器
    void stop() {
        running = false;
    }

    // 获取队列大小
    size_t size() const {
        return task_queue.size();
    }
};

// 创建任务（协程函数）
Task createRoundRobinTask(int id, const string& name, int steps) {
    for (int i = 1; i <= steps; ++i) {
        cout << "    → 任务 " << id << " 执行步骤 " << i << "/" << steps << endl;
        co_await std::suspend_always{};
    }
    co_return;
}

void part2_round_robin() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分：轮询调度器实现" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【轮询调度器设计】

核心数据结构：
- std::deque<Task> task_queue
- deque支持头尾高效操作

工作流程：
1. 从队列头取出任务
2. 执行任务一次（resume）
3. 放到队列尾（如果未完成）
4. 继续处理下一个任务

关键点：
- 每个任务每轮只执行一次
- 任务按顺序轮流执行
- 完全公平，无优先级
)" << endl;

    cout << "\n示例：多个任务轮询执行" << endl;
    cout << string(60, '-') << endl;

    RoundRobinScheduler scheduler;

    // 创建不同步骤的任务
    cout << "\n创建任务：" << endl;

    Task task1 = createRoundRobinTask(1, "任务A", 3);
    task1.handle.promise().task_id = 1;
    task1.handle.promise().task_name = "任务A";
    scheduler.schedule(task1);

    Task task2 = createRoundRobinTask(2, "任务B", 2);
    task2.handle.promise().task_id = 2;
    task2.handle.promise().task_name = "任务B";
    scheduler.schedule(task2);

    Task task3 = createRoundRobinTask(3, "任务C", 4);
    task3.handle.promise().task_id = 3;
    task3.handle.promise().task_name = "任务C";
    scheduler.schedule(task3);

    cout << "\n预期执行顺序：" << endl;
    cout << "第1轮：A → B → C" << endl;
    cout << "第2轮：A → B → C" << endl;
    cout << "第3轮：A → C（B已完成）" << endl;
    cout << "第4轮：C（A已完成）" << endl;
    cout << string(60, '-') << endl;

    // 运行调度器
    scheduler.run();

    cout << "\n" << string(60, '-') << endl;
    cout << "观察到的特点：" << endl;
    cout << "- 每个任务轮流执行，完全公平" << endl;
    cout << "- 短任务（B）优先完成" << endl;
    cout << "- 长任务（C）最后完成" << endl;
    cout << "- 所有任务都有机会执行" << endl;
}

// ============================================
// 第三部分：时间片调度器实现 ⭐⭐⭐⭐⭐
// ============================================

// 时间片调度器（Time Slice Scheduler）
class TimeSliceScheduler {
private:
    std::deque<Task> task_queue;
    bool running = false;
    int time_slice = 2;  // 时间片大小（每个任务连续执行的次数）
    int total_executions = 0;
    int context_switches = 0;  // 上下文切换次数

public:
    // 构造函数：指定时间片大小
    TimeSliceScheduler(int slice = 2) : time_slice(slice) {}

    // 调度任务
    void schedule(Task task) {
        if (task.handle) {
            cout << "[Scheduler] 调度任务 " << task.id()
                 << "（" << task.name() << "）" << endl;
            task_queue.push_back(task);
        }
    }

    // 运行调度器（时间片模式）
    void run() {
        running = true;
        int step = 1;

        cout << "\n[Scheduler] 开始时间片调度，时间片 = " << time_slice
             << "，队列中有 " << task_queue.size() << " 个任务" << endl;

        while (running && !task_queue.empty()) {
            // 从队列头取出任务
            Task task = task_queue.front();
            task_queue.pop_front();

            auto handle = task.handle;

            // 检查是否完成
            if (handle.done()) {
                cout << "[步骤 " << step++ << "] 任务 " << task.id() << " 已完成，销毁" << endl;
                handle.destroy();
                context_switches++;
                continue;
            }

            // 执行时间片（连续执行多次）
            cout << "[步骤 " << step++ << "] 任务 " << task.id()
                 << "（" << task.name() << "）获得时间片 " << time_slice << endl;

            int executed = 0;
            for (int i = 0; i < time_slice && !handle.done(); ++i) {
                handle.resume();
                handle.promise().execute_count++;
                total_executions++;
                executed++;

                cout << "    → 执行第 " << executed << "/" << time_slice << " 次"
                     << " [总共 " << handle.promise().execute_count << " 次]" << endl;
            }

            // 上下文切换
            context_switches++;

            // 检查是否完成
            if (!handle.done()) {
                // 未完成：放到队列尾
                cout << "    → 时间片用完，任务 " << task.id() << " 重新入队" << endl;
                task_queue.push_back(task);
            } else {
                // 已完成：销毁
                cout << "    → 任务 " << task.id() << " 在时间片内完成，销毁" << endl;
                handle.destroy();
            }
        }

        running = false;
        cout << "\n[Scheduler] 调度器停止" << endl;
        cout << "  总执行次数：" << total_executions << endl;
        cout << "  上下文切换：" << context_switches << " 次" << endl;
        cout << "  平均执行次数/切换：" << (double)total_executions / context_switches << endl;
    }

    // 设置时间片大小
    void set_time_slice(int slice) {
        time_slice = slice;
    }
};

// 创建任务（用于时间片调度）
Task createTimeSliceTask(int id, const string& name, int steps) {
    for (int i = 1; i <= steps; ++i) {
        cout << "        [任务 " << id << " 步骤 " << i << "/" << steps << "]" << endl;
        co_await std::suspend_always{};
    }
    co_return;
}

void part3_time_slice() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第三部分：时间片调度器实现" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【时间片调度器设计】

核心思想：
- 每个任务分配固定的执行时间（时间片）
- 时间片用完后，强制切换到下一个任务
- 即使任务未完成，也要让出CPU

关键参数：
- time_slice：时间片大小（连续执行的次数）
- 典型值：2、5、10

优点：
- 响应性好：短任务不会等太久
- 公平性好：每个任务分配到时间
- 适合交互式系统

缺点：
- 时间片太小 → 切换频繁
- 时间片太大 → 响应慢
)" << endl;

    // 实验1：时间片=2
    cout << "\n【实验1：时间片 = 2】" << endl;
    cout << string(60, '-') << endl;

    TimeSliceScheduler scheduler1(2);

    Task task1a = createTimeSliceTask(1, "短任务", 3);
    task1a.handle.promise().task_id = 1;
    task1a.handle.promise().task_name = "短任务";
    scheduler1.schedule(task1a);

    Task task1b = createTimeSliceTask(2, "长任务", 7);
    task1b.handle.promise().task_id = 2;
    task1b.handle.promise().task_name = "长任务";
    scheduler1.schedule(task1b);

    cout << "\n任务：短任务(3步) + 长任务(7步)" << endl;
    cout << "预期：每个任务连续执行2次，然后切换" << endl;
    cout << string(60, '-') << endl;

    scheduler1.run();

    // 实验2：时间片=5
    cout << "\n\n【实验2：时间片 = 5】" << endl;
    cout << string(60, '-') << endl;

    TimeSliceScheduler scheduler2(5);

    Task task2a = createTimeSliceTask(3, "短任务", 3);
    task2a.handle.promise().task_id = 3;
    task2a.handle.promise().task_name = "短任务";
    scheduler2.schedule(task2a);

    Task task2b = createTimeSliceTask(4, "长任务", 7);
    task2b.handle.promise().task_id = 4;
    task2b.handle.promise().task_name = "长任务";
    scheduler2.schedule(task2b);

    cout << "\n任务：短任务(3步) + 长任务(7步)" << endl;
    cout << "预期：每个任务连续执行5次，然后切换" << endl;
    cout << string(60, '-') << endl;

    scheduler2.run();

    // 对比总结
    cout << "\n【对比总结】" << endl;
    cout << string(60, '-') << endl;
    cout << R"(
时间片 = 2：
- 短任务完成快（第2轮完成）
- 切换频繁（更公平）
- 响应性好

时间片 = 5：
- 短任务在第1轮完成
- 切换少（开销小）
- 长任务连续执行时间长

选择建议：
- 交互式系统：时间片 2-5
- 批处理系统：时间片 5-10
- 根据任务特点调整
)" << endl;
}

// ============================================
// 第四部分：公平性验证与统计 ⭐⭐⭐⭐
// ============================================

void part4_fairness_analysis() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第四部分：公平性验证与统计" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【公平性指标】

如何衡量调度器的公平性？

1. 等待时间（Waiting Time）
   - 任务从创建到开始执行的时间
   - 理想：所有任务等待时间相近

2. 响应时间（Response Time）
   - 任务从创建到第一次执行的时间
   - 理想：响应时间短且均匀

3. 完成时间（Turnaround Time）
   - 任务从创建到完成的总时间
   - 理想：短任务快速完成

4. 执行次数分布
   - 每个任务被执行的次数
   - 理想：相近（在任务未完成时）

5. 饥饿检测
   - 是否有任务长时间未执行
   - 理想：无任务饥饿
)" << endl;

    cout << "\n【三种调度策略的公平性对比】" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
假设3个任务：A(3步)、B(2步)、C(4步)

┌──────────┬──────────────┬──────────────┬──────────────┐
│  策略    │  FIFO        │  轮询        │  时间片(2)   │
├──────────┼──────────────┼──────────────┼──────────────┤
│ 执行顺序 │ A A A B B C C C C │ A B C A B C A C C │ A A B B C C A C C │
│          │                   │                   │                   │
│ A完成轮  │ 第1轮            │ 第3轮            │ 第2轮            │
│ B完成轮  │ 第1轮            │ 第2轮            │ 第1轮            │
│ C完成轮  │ 第1轮            │ 第4轮            │ 第2轮            │
│          │                   │                   │                   │
│ 公平性   │ 差（A先完成）    │ 好（轮流执行）    │ 好（短任务优先） │
│ 响应时间 │ B和C等待久       │ 所有任务快速响应  │ B快速完成        │
└──────────┴──────────────┴──────────────┴──────────────┘

分析：

FIFO调度：
- A完全执行完才轮到B
- B完全执行完才轮到C
- 后面的任务等待时间长
- 公平性差

轮询调度：
- 每个任务轮流执行1次
- 所有任务都能快速响应
- 公平性最好
- 但完成时间可能较长

时间片调度（时间片=2）：
- 短任务（B）快速完成
- 长任务被分割
- 平衡公平性和响应性
- 综合效果最好
)" << endl;

    cout << "\n【实际数据对比】" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
假设任务A(10步)、B(5步)、C(20步)

FIFO调度：
- A: 等待0，响应0，完成10
- B: 等待10，响应10，完成15
- C: 等待15，响应15，完成35
- 平均等待：8.3
- 平均响应：8.3

轮询调度：
- A: 等待0，响应0，完成30
- B: 等待1，响应1，完成14
- C: 等待2，响应2，完成35
- 平均等待：1.0
- 平均响应：1.0

时间片调度（时间片=5）：
- A: 等待0，响应0，完成25
- B: 等待5，响应5，完成10
- C: 等待10，响应10，完成35
- 平均等待：5.0
- 平均响应：5.0

结论：
- 轮询调度响应时间最短（最公平）
- 时间片调度是平衡选择
- FIFO调度响应时间最长（最不公平）
)" << endl;
}

// ============================================
// 第五部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void part5_common_errors() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第五部分：常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 5.1 错误1：时间片设置不当
    // --------------------------------------------

    cout << "\n5.1 错误1：时间片设置不当" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误场景1：时间片太小（time_slice = 1）

问题：
- 每个任务只执行1次就切换
- 上下文切换非常频繁
- CPU大部分时间花在切换上

结果：
- 吞吐量下降
- 整体性能差

示例：
10个任务，每个10步 = 100次执行
时间片=1 → 100次切换
时间片=5 → 20次切换
差异：5倍的切换开销！

❌ 错误场景2：时间片太大（time_slice = 100）

问题：
- 任务长时间独占CPU
- 其他任务等待时间长
- 响应时间差

结果：
- 退化为FIFO调度
- 失去公平性

✅ 正确做法：根据任务特点选择时间片

经验法则：
- 短任务为主：time_slice = 2-5
- 长任务为主：time_slice = 5-10
- 混合任务：time_slice = 5

调优方法：
1. 测量平均任务长度
2. 时间片 = 平均长度 / 2 到 平均长度
3. 观察响应时间和吞吐量
4. 调整时间片

记忆技巧：
- 时间片太小 = 频繁切换 = 开销大
- 时间片太大 = 长时间独占 = 响应慢
- 合适的时间片 = 平衡响应和开销
)" << endl;

    // --------------------------------------------
    // 5.2 错误2：忘记检查任务是否完成
    // --------------------------------------------

    cout << "\n5.2 错误2：忘记检查任务是否完成" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
for (int i = 0; i < time_slice; ++i) {
    handle.resume();  // 没有检查done()
}

问题：
- 任务已经完成，还继续resume
- 导致未定义行为
- 可能崩溃

✅ 正确代码：
for (int i = 0; i < time_slice && !handle.done(); ++i) {
    handle.resume();  // 每次都检查done()
}

原因：
- 任务可能在时间片内完成
- 必须及时检查并停止

记忆技巧：
- 时间片循环必须检查done()
- 就像开车踩刹车前要看前方
- 防止"追尾"崩溃
)" << endl;

    // --------------------------------------------
    // 5.3 错误3：队列选择不当
    // --------------------------------------------

    cout << "\n5.3 错误3：队列选择不当" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
std::queue<Task> task_queue;  // 只能从尾部添加，头部删除

问题：
- queue不支持从头部添加
- 轮询调度需要从头取、从尾加
- 实现困难

✅ 正确代码：
std::deque<Task> task_queue;  // 双端队列，头尾都支持

操作：
- task_queue.front()  // 访问头部
- task_queue.pop_front()  // 从头部删除
- task_queue.push_back()  // 从尾部添加

对比：
std::queue：
- 只能从尾部添加
- 只能从头部删除
- 不适合轮询调度

std::deque：
- 头尾都支持添加/删除
- 完美适合轮询调度
- 性能好（O(1)）

记忆技巧：
- 轮询调度 = 头取尾放 = deque
- FIFO调度 = 头取尾放 = queue或deque
- 优先级调度 = priority_queue
)" << endl;

    // --------------------------------------------
    // 5.4 错误4：没有统计执行次数
    // --------------------------------------------

    cout << "\n5.4 错误4：没有统计执行次数" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 问题：
- 无法验证公平性
- 无法调优时间片
- 无法发现异常

✅ 解决方案：统计关键指标

需要统计的指标：
1. 每个任务的执行次数
   - 验证公平性
   - 检测饥饿

2. 总执行次数
   - 计算吞吐量
   - 评估性能

3. 上下文切换次数
   - 评估开销
   - 优化时间片

4. 等待时间、响应时间
   - 评估用户体验
   - 调优参数

实现方法：
struct Task {
    int execute_count = 0;  // 任务执行次数
    chrono::time_point<> enqueue_time;  // 入队时间
    chrono::time_point<> first_run_time;  // 首次运行时间
    chrono::time_point<> complete_time;  // 完成时间
};

记忆技巧：
- 没有度量就无法改进
- 统计是调优的基础
- 数据驱动决策
)" << endl;

    // --------------------------------------------
    // 5.5 错误5：混淆公平性和效率
    // --------------------------------------------

    cout << "\n5.5 错误5：混淆公平性和效率" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误认识：
"公平调度最好，应该总是使用"

问题：
- 公平性和效率有时矛盾
- 盲目追求公平可能降低效率

✅ 正确认识：
根据场景选择合适的策略

场景1：所有任务同等重要
→ 使用公平调度（轮询、时间片）

场景2：任务有明确优先级
→ 使用优先级调度

场景3：混合场景
→ 使用混合策略
  - 多级队列
  - 优先级+时间片
  - 动态调整

权衡因素：
- 公平性 vs 效率
- 响应时间 vs 吞吐量
- 简单性 vs 灵活性

示例：
Web服务器：
- 用户请求：优先级调度（快速响应）
- 后台任务：公平调度（防止饥饿）

游戏引擎：
- 渲染：最高优先级（保证帧率）
- AI计算：时间片调度（不阻塞渲染）
- 资源加载：最低优先级（可以等待）

记忆技巧：
- 没有最好的策略，只有最合适的策略
- 理解业务需求
- 选择合适的策略
)" << endl;
}

// ============================================
// 第六部分：函数卡片速查
// ============================================

void part6_function_reference() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第六部分：函数卡片速查" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【std::deque】

语法：
template<class T>
class deque;

常用方法：
- push_back(value)：尾部添加（O(1)）
- push_front(value)：头部添加（O(1)）
- pop_back()：尾部删除（O(1)）
- pop_front()：头部删除（O(1)）
- front()：访问头部（O(1)）
- back()：访问尾部（O(1)）
- size()：元素个数（O(1)）
- empty()：是否为空（O(1)）

示例：
deque<int> dq;
dq.push_back(1);   // [1]
dq.push_back(2);   // [1, 2]
dq.push_front(0);  // [0, 1, 2]
dq.pop_front();    // [1, 2]

────────────────────────────────────

【轮询调度器】

核心接口：
void schedule(Task task);  // 调度任务
void run();                // 运行调度器（轮询模式）

使用示例：
RoundRobinScheduler scheduler;
scheduler.schedule(task1);
scheduler.schedule(task2);
scheduler.run();

特点：
- 每个任务每轮执行一次
- 完全公平
- 无饥饿

────────────────────────────────────

【时间片调度器】

核心接口：
TimeSliceScheduler(int time_slice);  // 构造函数，指定时间片
void schedule(Task task);            // 调度任务
void run();                          // 运行调度器（时间片模式）
void set_time_slice(int slice);      // 设置时间片大小

使用示例：
TimeSliceScheduler scheduler(5);  // 时间片=5
scheduler.schedule(task1);
scheduler.schedule(task2);
scheduler.run();

特点：
- 每个任务连续执行time_slice次
- 平衡响应性和开销
- 适合交互式系统

────────────────────────────────────

【公平性指标】

等待时间（Waiting Time）：
- 任务创建到开始执行的时间
- waiting_time = first_run_time - enqueue_time

响应时间（Response Time）：
- 任务创建到第一次执行的时间
- response_time = first_run_time - enqueue_time

完成时间（Turnaround Time）：
- 任务创建到完成的总时间
- turnaround_time = complete_time - enqueue_time

执行次数：
- 任务被执行的总次数
- execute_count++
)" << endl;
}

// ============================================
// 第七部分：练习题
// ============================================

void part7_exercises() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第七部分：练习题" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
1. 实现一个加权轮询调度器
   要求：根据任务权重分配执行次数
   提示：权重高的任务每轮执行多次

2. 实现一个自适应时间片调度器
   要求：根据任务执行历史动态调整时间片
   提示：长任务减少时间片，短任务增加时间片

3. 统计并输出公平性指标
   要求：计算每个任务的等待时间、响应时间、完成时间
   提示：使用chrono库记录时间戳

4. 对比不同时间片大小的性能
   要求：测试时间片=1, 2, 5, 10, 20的效果
   提示：统计总执行时间、切换次数、响应时间

5. 实现一个多级反馈队列调度器
   要求：新任务高优先级，执行时间长则降低优先级
   提示：维护多个队列，任务在队列间移动

6. 检测并报告任务饥饿
   要求：如果任务超过N轮未执行，发出警告
   提示：记录每个任务的上次执行轮数

7. 实现一个公平性可视化工具
   要求：用文本图表展示任务执行的时间线
   提示：用字符表示任务，用空格表示等待
)" << endl;
}

// ============================================
// 主函数
// ============================================

int main() {
    cout << "============================================" << endl;
    cout << "协程调度策略：公平调度" << endl;
    cout << "============================================" << endl;

    // 第一部分：核心概念详解
    part1_concepts();

    // 第二部分：轮询调度器实现
    part2_round_robin();

    // 第三部分：时间片调度器实现
    part3_time_slice();

    // 第四部分：公平性验证与统计
    part4_fairness_analysis();

    // 第五部分：常见错误和陷阱
    part5_common_errors();

    // 第六部分：函数卡片速查
    part6_function_reference();

    // 第七部分：练习题
    part7_exercises();

    cout << "\n" << string(60, '=') << endl;
    cout << "学习完成！" << endl;
    cout << string(60, '=') << endl;

    return 0;
}