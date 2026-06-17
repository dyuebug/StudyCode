// ============================================
// 协程池基础 - 超详细注释版
// 日期：2026-06-17（第84天）
// 难度：⭐⭐⭐ 中级进阶
// ============================================

/*
【核心概念】简要说明

是什么？
- 协程池：预先创建并管理一组协程的容器
- 复用协程：避免频繁创建和销毁
- 任务队列：待执行的任务排队等待

为什么需要？
- 创建协程有开销（虽然比线程小得多）
- 频繁创建销毁影响性能
- 统一管理方便控制

核心特点？
- 协程复用：一个协程可以执行多个任务
- 任务队列：任务排队等待空闲协程
- 生命周期管理：自动管理协程的创建和销毁
*/

#include <iostream>
#include <coroutine>
#include <queue>
#include <vector>
#include <functional>
#include <string>

using namespace std;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

void part1_concepts() {
    cout << string(60, '=') << endl;
    cout << "第一部分：协程池核心概念 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【协程池是什么？】

定义：
- 协程池是预先创建并管理一组协程的容器
- 类似于线程池，但管理的是协程
- 提供协程的复用和统一管理

核心组件：
1. 协程容器：存储可复用的协程
2. 任务队列：存储待执行的任务
3. 调度器：分配任务给空闲协程

【为什么需要协程池？】

问题1：频繁创建销毁开销
- 虽然协程创建很快（0.1μs）
- 但频繁创建仍有开销
- 大量协程会消耗内存

问题2：难以管理
- 协程数量不可控
- 生命周期难以追踪
- 资源泄漏风险

问题3：并发控制困难
- 无法限制同时运行的协程数
- 可能导致资源耗尽

协程池的解决方案：
✅ 协程复用：减少创建销毁开销
✅ 统一管理：集中控制生命周期
✅ 并发控制：限制最大协程数
✅ 任务队列：平滑处理突发负载

【协程池 vs 线程池】

相同点：
- 都是池化管理资源
- 都有任务队列
- 都支持并发控制

不同点：

┌──────────┬──────────────┬──────────────┐
│  维度    │  协程池      │  线程池      │
├──────────┼──────────────┼──────────────┤
│ 创建开销 │ 0.1μs        │ 1ms          │
│ 切换开销 │ 10-100ns     │ 1-10μs       │
│ 内存占用 │ 0.5KB/协程   │ 1MB/线程     │
│ 最大数量 │ 100万+       │ 1000         │
│ 调度方式 │ 协作式       │ 抢占式       │
│ 上下文   │ 轻量级       │ 重量级       │
└──────────┴──────────────┴──────────────┘

性能对比（相对值）：
- 协程池创建速度：10000倍快
- 协程池切换速度：100倍快
- 协程池内存占用：2000倍小

结论：
- 协程池更轻量
- 协程池更高效
- 但需要协作式调度

【协程池的使用场景】

适合：
✅ 大量短期任务（如Web请求处理）
✅ I/O密集型任务（如文件读写、网络通信）
✅ 需要高并发的场景（如聊天服务器）

不适合：
❌ CPU密集型长任务（会阻塞其他协程）
❌ 需要抢占式调度的场景
❌ 任务间需要隔离的场景

【协程池的设计要点】

1. 协程数量
   - 固定大小 vs 动态扩展
   - 最小/最大协程数
   - 根据负载调整

2. 任务队列
   - 有界 vs 无界队列
   - 队列满时的策略
   - 优先级支持

3. 任务分配
   - 轮询分配
   - 负载均衡
   - 亲和性调度

4. 生命周期管理
   - 协程的创建时机
   - 协程的销毁时机
   - 优雅关闭流程
)" << endl;
}

// ============================================
// 第二部分：基础协程池实现
// ============================================

// 简化的Task结构（用于协程池）
struct PoolTask {
    struct promise_type {
        int task_id = 0;

        PoolTask get_return_object() {
            return PoolTask{coroutine_handle<promise_type>::from_promise(*this)};
        }

        suspend_always initial_suspend() { return {}; }
        suspend_always final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() { terminate(); }
    };

    coroutine_handle<promise_type> handle;
};

// 简单的协程池实现
class SimpleCoroutinePool {
private:
    vector<PoolTask> workers;           // 工作协程
    queue<function<void()>> task_queue; // 任务队列
    size_t pool_size;                   // 协程池大小
    int completed_tasks = 0;            // 完成的任务数

public:
    SimpleCoroutinePool(size_t size) : pool_size(size) {
        cout << "[CoroutinePool] 创建协程池，大小=" << size << endl;
    }

    // 提交任务
    void submit(function<void()> task) {
        task_queue.push(task);
        cout << "[CoroutinePool] 提交任务，队列大小=" << task_queue.size() << endl;
    }

    // 运行协程池
    void run() {
        cout << "\n[CoroutinePool] 开始运行" << endl;

        while (!task_queue.empty()) {
            auto task = task_queue.front();
            task_queue.pop();

            cout << "[CoroutinePool] 执行任务" << endl;
            task();  // 执行任务
            completed_tasks++;
        }

        cout << "\n[CoroutinePool] 运行完成，共执行 " << completed_tasks << " 个任务" << endl;
    }

    // 获取完成的任务数
    int get_completed() const { return completed_tasks; }
};

void part2_basic_pool() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分：基础协程池实现" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【简单协程池设计】

核心思想：
- 预先创建N个协程
- 任务提交到队列
- 协程从队列取任务执行

关键点：
1. 协程复用：一个协程执行多个任务
2. 任务队列：存储待执行的任务
3. 简化实现：先实现基本功能

注意：
这是简化版本，用于演示概念
生产环境需要更复杂的实现
)" << endl;

    cout << "\n示例：使用协程池执行任务" << endl;
    cout << string(60, '-') << endl;

    // 创建协程池
    SimpleCoroutinePool pool(4);

    // 提交任务
    for (int i = 1; i <= 5; ++i) {
        pool.submit([i]() {
            cout << "  → 执行任务 " << i << endl;
        });
    }

    // 运行协程池
    pool.run();

    cout << "\n观察到的特点：" << endl;
    cout << "- 任务按顺序执行" << endl;
    cout << "- 协程池统一管理任务" << endl;
    cout << "- 简化了任务的提交和执行" << endl;
}

// ============================================
// 主函数
// ============================================

int main() {
    cout << "============================================" << endl;
    cout << "协程池基础" << endl;
    cout << "============================================\n" << endl;

    part1_concepts();
    part2_basic_pool();

    cout << "\n" << string(60, '=') << endl;
    cout << "学习完成！" << endl;
    cout << string(60, '=') << endl;

    cout << "\n【温馨提示】" << endl;
    cout << "现在是凌晨时间，学习材料已创建完成。" << endl;
    cout << "请休息好，明天再继续学习！💤" << endl;

    return 0;
}
