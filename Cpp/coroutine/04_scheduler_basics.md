# 协程调度器基础

**日期**：2026-06-27（第82天）
**主题**：协程调度器概念与设计
**难度**：⭐⭐⭐ 中级

---

## 📋 目录

1. [为什么需要调度器](#为什么需要调度器)
2. [调度器的核心作用](#调度器的核心作用)
3. [调度器的核心组件](#调度器的核心组件)
4. [调度器与协程的交互](#调度器与协程的交互)
5. [任务队列设计](#任务队列设计)
6. [简单调度器实现](#简单调度器实现)
7. [使用示例](#使用示例)
8. [常见问题](#常见问题)

---

## 为什么需要调度器

### 问题场景：手动管理协程

```cpp
#include <iostream>
#include <coroutine>
#include <vector>

// 假设我们有多个协程
Task<void> task1 = asyncOperation1();
Task<void> task2 = asyncOperation2();
Task<void> task3 = asyncOperation3();

// 问题：如何管理这些协程？

// 方案1：逐个手动调用（串行）
task1.resume();
while (!task1.done()) {
    task1.resume();
}

task2.resume();
while (!task2.done()) {
    task2.resume();
}

task3.resume();
while (!task3.done()) {
    task3.resume();
}

// 问题：
// ❌ 串行执行，无法并发
// ❌ 代码重复
// ❌ 不灵活
```

```cpp
// 方案2：手动轮询（并发）
std::vector<Task<void>> tasks = {task1, task2, task3};

while (true) {
    bool all_done = true;
    
    for (auto& task : tasks) {
        if (!task.done()) {
            task.resume();
            all_done = false;
        }
    }
    
    if (all_done) break;
}

// 问题：
// ❌ 代码复杂
// ❌ 容易出错
// ❌ 难以扩展（如何添加优先级？超时控制？）
// ❌ 性能不佳（空轮询）
```

---

### 调度器解决方案

```cpp
// 有调度器：简单优雅
Scheduler scheduler;

// 添加协程到调度器
scheduler.schedule(asyncOperation1());
scheduler.schedule(asyncOperation2());
scheduler.schedule(asyncOperation3());

// 运行调度器（自动管理所有协程）
scheduler.run();

// 优势：
// ✅ 代码简洁
// ✅ 自动管理
// ✅ 易于扩展
// ✅ 性能优化
```

---

### 调度器的价值

| 维度 | 手动管理 | 调度器管理 |
|------|---------|-----------|
| **代码量** | 多（每个协程都要写循环） | 少（一行schedule） |
| **复杂度** | 高（手动轮询） | 低（自动调度） |
| **并发性** | 难实现 | 内置支持 |
| **优先级** | 难实现 | 易于添加 |
| **超时控制** | 难实现 | 易于添加 |
| **错误处理** | 分散 | 集中 |
| **可维护性** | 差 | 好 |

**结论**：调度器是管理协程的必备工具！

---

## 调度器的核心作用

### 作用1：协程生命周期管理

```cpp
class Scheduler {
public:
    // 1. 创建：接收协程
    void schedule(std::coroutine_handle<> handle) {
        // 添加到任务队列
    }
    
    // 2. 暂停：协程主动暂停
    // （协程内部 co_await 时自动暂停）
    
    // 3. 恢复：在合适的时机恢复
    void resume_task(std::coroutine_handle<> handle) {
        handle.resume();
    }
    
    // 4. 销毁：协程完成后销毁
    void destroy_task(std::coroutine_handle<> handle) {
        if (handle.done()) {
            handle.destroy();
        }
    }
};
```

**生命周期**：
```
schedule() → 创建
  ↓
run() → 开始执行
  ↓
co_await → 暂停（自动）
  ↓
resume() → 恢复（调度器决定）
  ↓
co_return → 完成
  ↓
destroy() → 销毁
```

---

### 作用2：任务调度

**调度决策**：

```cpp
class Scheduler {
private:
    TaskQueue queue;
    
public:
    void run() {
        while (!queue.empty()) {
            // 决策1：选择哪个协程执行
            auto handle = selectNextTask();
            
            // 决策2：何时切换协程
            if (shouldSwitch(handle)) {
                // 暂停当前协程，切换到下一个
            }
            
            // 决策3：如何分配资源
            allocateResources(handle);
            
            // 执行协程
            handle.resume();
        }
    }
    
private:
    // 选择下一个任务
    std::coroutine_handle<> selectNextTask() {
        // 策略1：FIFO（先进先出）
        // 策略2：优先级（高优先级优先）
        // 策略3：公平调度（轮询）
        return queue.dequeue();
    }
    
    // 是否应该切换
    bool shouldSwitch(std::coroutine_handle<> handle) {
        // 条件1：协程完成
        // 条件2：协程主动让出（co_await）
        // 条件3：时间片用完（抢占式）
        return handle.done();
    }
};
```

**调度策略**：
- **FIFO**：先进先出（最简单）
- **优先级**：高优先级优先（常用）
- **公平调度**：每个协程平等机会（公平）
- **抢占式**：时间片轮转（类似线程）

---

### 作用3：状态追踪

```cpp
enum class TaskState {
    READY,      // 就绪（在队列中）
    RUNNING,    // 运行中
    SUSPENDED,  // 暂停（等待事件）
    COMPLETED   // 已完成
};

class Scheduler {
private:
    // 状态管理
    std::map<std::coroutine_handle<>, TaskState> task_states;
    
    // 依赖管理
    std::map<std::coroutine_handle<>, std::vector<std::coroutine_handle<>>> dependencies;
    
public:
    // 跟踪状态
    void setState(std::coroutine_handle<> handle, TaskState state) {
        task_states[handle] = state;
    }
    
    TaskState getState(std::coroutine_handle<> handle) {
        return task_states[handle];
    }
    
    // 添加依赖
    void addDependency(std::coroutine_handle<> task, std::coroutine_handle<> depends_on) {
        dependencies[task].push_back(depends_on);
    }
    
    // 检查是否可以运行（所有依赖都完成）
    bool canRun(std::coroutine_handle<> handle) {
        for (auto dep : dependencies[handle]) {
            if (!dep.done()) {
                return false;
            }
        }
        return true;
    }
};
```

**状态转换**：
```
READY（就绪）
  ↓ selectNextTask()
RUNNING（运行中）
  ↓ co_await
SUSPENDED（暂停）
  ↓ 事件就绪
READY（就绪）
  ↓ co_return
COMPLETED（完成）
```

---

## 调度器的核心组件

### 组件结构图

```
┌────────────────────────────────────────┐
│        Scheduler（调度器）              │
├────────────────────────────────────────┤
│                                        │
│  ┌──────────────────────────────────┐ │
│  │   Task Queue（任务队列）          │ │
│  │   - 存储待执行的协程               │ │
│  │   - FIFO / 优先级队列             │ │
│  │   - 线程安全（可选）               │ │
│  └──────────────────────────────────┘ │
│                                        │
│  ┌──────────────────────────────────┐ │
│  │  Running Set（运行集合）          │ │
│  │   - 当前运行的协程                 │ │
│  │   - handle → state 映射           │ │
│  │   - handle → priority 映射        │ │
│  └──────────────────────────────────┘ │
│                                        │
│  ┌──────────────────────────────────┐ │
│  │ Event Loop（事件循环）            │ │
│  │   - 主调度逻辑                     │ │
│  │   - 协程选择                       │ │
│  │   - 协程切换                       │ │
│  │   - 资源管理                       │ │
│  └──────────────────────────────────┘ │
│                                        │
└────────────────────────────────────────┘
```

---

### 组件1：任务队列（Task Queue）

**作用**：存储待执行的协程

**接口**：
```cpp
class TaskQueue {
public:
    virtual void enqueue(std::coroutine_handle<>) = 0;  // 入队
    virtual std::coroutine_handle<> dequeue() = 0;      // 出队
    virtual bool empty() const = 0;                      // 是否为空
    virtual size_t size() const = 0;                     // 队列大小
};
```

**实现方式**：
- FIFO队列：`std::queue`
- 优先级队列：`std::priority_queue`
- 无锁队列：`boost::lockfree::queue`

---

### 组件2：运行集合（Running Set）

**作用**：跟踪当前运行的协程

**数据结构**：
```cpp
class RunningSet {
private:
    // handle → state
    std::map<std::coroutine_handle<>, TaskState> states;
    
    // handle → priority
    std::map<std::coroutine_handle<>, int> priorities;
    
public:
    void add(std::coroutine_handle<> handle, int priority = 0) {
        states[handle] = TaskState::READY;
        priorities[handle] = priority;
    }
    
    void remove(std::coroutine_handle<> handle) {
        states.erase(handle);
        priorities.erase(handle);
    }
    
    void setState(std::coroutine_handle<> handle, TaskState state) {
        states[handle] = state;
    }
    
    TaskState getState(std::coroutine_handle<> handle) {
        return states[handle];
    }
};
```

---

### 组件3：事件循环（Event Loop）

**作用**：调度器的核心逻辑

**伪代码**：
```cpp
void Scheduler::run() {
    while (has_tasks()) {
        // 1. 选择下一个任务
        auto handle = select_next_task();
        
        // 2. 检查是否可以运行
        if (!can_run(handle)) {
            continue;
        }
        
        // 3. 更新状态
        set_state(handle, TaskState::RUNNING);
        
        // 4. 恢复协程
        handle.resume();
        
        // 5. 检查状态
        if (handle.done()) {
            // 完成：销毁
            handle.destroy();
            remove_task(handle);
        } else {
            // 未完成：重新入队
            set_state(handle, TaskState::READY);
            enqueue(handle);
        }
    }
}
```

---

## 调度器与协程的交互

### 完整交互流程

```
┌─────────────┐
│  用户代码    │
└──────┬──────┘
       │ scheduler.schedule(coro)
       ↓
┌─────────────────────────────────┐
│  Scheduler                      │
│  - 接收协程handle                │
│  - 添加到任务队列                 │
└──────┬──────────────────────────┘
       │ scheduler.run()
       ↓
┌─────────────────────────────────┐
│  Event Loop（事件循环）          │
│  - while (!queue.empty())       │
└──────┬──────────────────────────┘
       │ 从队列取出协程
       ↓
┌─────────────────────────────────┐
│  Coroutine（协程开始执行）       │
│  - handle.resume()              │
└──────┬──────────────────────────┘
       │ co_await（暂停）
       ↓
┌─────────────────────────────────┐
│  Scheduler（协程暂停）           │
│  - 保存协程状态                  │
│  - 切换到下一个协程               │
└──────┬──────────────────────────┘
       │ 事件就绪（或轮询到）
       ↓
┌─────────────────────────────────┐
│  Scheduler（恢复协程）           │
│  - 找到暂停的协程                │
│  - handle.resume()              │
└──────┬──────────────────────────┘
       │
       ↓
┌─────────────────────────────────┐
│  Coroutine（继续执行）           │
│  - 从暂停处继续                  │
└──────┬──────────────────────────┘
       │ co_return（完成）
       ↓
┌─────────────────────────────────┐
│  Scheduler（清理）               │
│  - handle.destroy()             │
│  - 从运行集合移除                │
└─────────────────────────────────┘
```

---

### 详细步骤说明

**步骤1：用户调度协程**
```cpp
Scheduler scheduler;
auto coro = asyncOperation();
scheduler.schedule(coro);  // 添加到调度器
```

**步骤2：调度器接收协程**
```cpp
void Scheduler::schedule(std::coroutine_handle<> handle) {
    task_queue.enqueue(handle);  // 入队
    running_set.add(handle);      // 添加到运行集合
}
```

**步骤3：运行事件循环**
```cpp
void Scheduler::run() {
    while (!task_queue.empty()) {
        // 步骤4：取出协程
        auto handle = task_queue.dequeue();
        
        // 步骤5：恢复协程
        handle.resume();
        
        // 步骤6：检查状态
        if (handle.done()) {
            handle.destroy();
        } else {
            task_queue.enqueue(handle);  // 重新入队
        }
    }
}
```

**步骤4-6：协程执行**
```cpp
Task<void> asyncOperation() {
    // 执行一些工作
    doWork1();
    
    // 步骤7：协程暂停
    co_await std::suspend_always{};
    
    // 步骤8：协程恢复后继续
    doWork2();
    
    // 步骤9：协程完成
    co_return;
}
```

---

### 关键点

**1. 协程的暂停和恢复**

协程暂停：
```cpp
co_await std::suspend_always;
// 此时：
// - 协程状态被保存
// - 控制权返回给调度器
// - 调度器切换到下一个协程
```

协程恢复：
```cpp
handle.resume();
// 此时：
// - 从暂停点继续执行
// - 局部变量保持不变
// - 继续执行后续代码
```

**2. 调度器的职责**

- ✅ 管理所有协程
- ✅ 决定执行顺序
- ✅ 在合适的时机切换
- ✅ 销毁完成的协程

**3. 协程与调度器的配合**

协程：
- 主动暂停（co_await）
- 保存状态
- 等待恢复

调度器：
- 选择协程
- 恢复协程
- 管理生命周期

---

## 任务队列设计

### FIFO队列实现（最简单）

```cpp
#include <queue>
#include <coroutine>

class FIFOQueue {
private:
    std::queue<std::coroutine_handle<>> tasks;
    
public:
    // 入队
    void enqueue(std::coroutine_handle<> handle) {
        if (handle) {
            tasks.push(handle);
        }
    }
    
    // 出队
    std::coroutine_handle<> dequeue() {
        if (tasks.empty()) {
            return nullptr;
        }
        auto handle = tasks.front();
        tasks.pop();
        return handle;
    }
    
    // 是否为空
    bool empty() const {
        return tasks.empty();
    }
    
    // 队列大小
    size_t size() const {
        return tasks.size();
    }
};
```

**特点**：
- ✅ 简单易懂
- ✅ 公平（先进先出）
- ✅ 性能好（O(1)）
- ❌ 没有优先级

**适用场景**：
- 单线程调度
- 不需要优先级
- 简单的协程管理

---

### 线程安全的队列

```cpp
#include <queue>
#include <mutex>
#include <coroutine>

class ThreadSafeQueue {
private:
    std::queue<std::coroutine_handle<>> tasks;
    mutable std::mutex mtx;
    
public:
    // 入队（线程安全）
    void enqueue(std::coroutine_handle<> handle) {
        std::lock_guard<std::mutex> lock(mtx);
        if (handle) {
            tasks.push(handle);
        }
    }
    
    // 出队（线程安全）
    std::coroutine_handle<> dequeue() {
        std::lock_guard<std::mutex> lock(mtx);
        if (tasks.empty()) {
            return nullptr;
        }
        auto handle = tasks.front();
        tasks.pop();
        return handle;
    }
    
    // 是否为空（线程安全）
    bool empty() const {
        std::lock_guard<std::mutex> lock(mtx);
        return tasks.empty();
    }
    
    // 队列大小（线程安全）
    size_t size() const {
        std::lock_guard<std::mutex> lock(mtx);
        return tasks.size();
    }
};
```

**特点**：
- ✅ 线程安全
- ✅ 支持多线程调度
- ❌ 有锁开销
- ❌ 性能略低

**适用场景**：
- 多线程调度
- 工作窃取
- 并行执行

---

### 队列选择指南

| 场景 | 推荐队列 | 理由 |
|------|---------|------|
| 单线程调度 | FIFO队列 | 简单高效，无锁开销 |
| 多线程调度 | 线程安全队列 | 避免竞态条件 |
| 需要优先级 | 优先级队列 | 支持调度策略 |
| 高性能要求 | 无锁队列 | 减少锁开销 |
| 简单应用 | FIFO队列 | 够用且简单 |

**建议**：
- 从FIFO队列开始
- 需要时再升级
- 不要过早优化

---

## 简单调度器实现

### Scheduler类设计

```cpp
#include <queue>
#include <coroutine>
#include <iostream>

class Scheduler {
private:
    std::queue<std::coroutine_handle<>> task_queue;
    bool running = false;
    
public:
    // 调度协程（添加到队列）
    void schedule(std::coroutine_handle<> handle);
    
    // 运行调度器（事件循环）
    void run();
    
    // 停止调度器
    void stop();
    
    // 获取队列大小
    size_t size() const;
};
```

---

### 完整实现

```cpp
#include <queue>
#include <coroutine>
#include <iostream>

class Scheduler {
private:
    std::queue<std::coroutine_handle<>> task_queue;
    bool running = false;
    
public:
    // ----------------------------------------
    // 调度协程（添加到队列）
    // ----------------------------------------
    void schedule(std::coroutine_handle<> handle) {
        if (handle) {
            task_queue.push(handle);
            std::cout << "[Scheduler] Task scheduled, queue size: " 
                      << task_queue.size() << std::endl;
        }
    }
    
    // ----------------------------------------
    // 运行调度器（事件循环）
    // ----------------------------------------
    void run() {
        running = true;
        std::cout << "[Scheduler] Starting event loop..." << std::endl;
        
        while (running && !task_queue.empty()) {
            // 1. 从队列取出协程
            auto handle = task_queue.front();
            task_queue.pop();
            
            // 2. 检查协程是否完成
            if (handle.done()) {
                std::cout << "[Scheduler] Task already done, destroying..." << std::endl;
                handle.destroy();
                continue;
            }
            
            // 3. 恢复协程
            std::cout << "[Scheduler] Resuming task..." << std::endl;
            handle.resume();
            
            // 4. 检查协程状态
            if (!handle.done()) {
                // 未完成：重新加入队列
                std::cout << "[Scheduler] Task suspended, re-queuing..." << std::endl;
                task_queue.push(handle);
            } else {
                // 已完成：销毁
                std::cout << "[Scheduler] Task completed, destroying..." << std::endl;
                handle.destroy();
            }
        }
        
        running = false;
        std::cout << "[Scheduler] Event loop ended." << std::endl;
    }
    
    // ----------------------------------------
    // 停止调度器
    // ----------------------------------------
    void stop() {
        running = false;
        std::cout << "[Scheduler] Stopping..." << std::endl;
    }
    
    // ----------------------------------------
    // 获取队列大小
    // ----------------------------------------
    size_t size() const {
        return task_queue.size();
    }
};
```

---

### 核心逻辑解析

**schedule方法**：
```cpp
void schedule(std::coroutine_handle<> handle) {
    if (handle) {
        task_queue.push(handle);  // 加入队列
    }
}
```
- 接收协程handle
- 添加到任务队列
- 简单直接

**run方法（事件循环）**：
```cpp
void run() {
    while (running && !task_queue.empty()) {
        auto handle = task_queue.front();  // 取出
        task_queue.pop();
        
        if (handle.done()) {
            handle.destroy();  // 已完成：销毁
            continue;
        }
        
        handle.resume();  // 恢复协程
        
        if (!handle.done()) {
            task_queue.push(handle);  // 未完成：重新入队
        } else {
            handle.destroy();  // 已完成：销毁
        }
    }
}
```
- while循环：事件循环
- dequeue：取出协程
- resume：恢复协程
- 根据状态决定：重新入队 or 销毁

---

## 使用示例

### 示例1：简单的Task类型

```cpp
#include <coroutine>
#include <iostream>

// 简单的Task类型
template<typename T = void>
struct Task {
    struct promise_type {
        T value;
        
        Task get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        
        void return_value(T v) { value = v; }
        void unhandled_exception() {}
    };
    
    std::coroutine_handle<promise_type> handle;
    
    Task(std::coroutine_handle<promise_type> h) : handle(h) {}
    ~Task() {
        // 注意：调度器会销毁，这里不需要
    }
    
    // 转换为通用handle
    operator std::coroutine_handle<>() const {
        return handle;
    }
};
```

---

### 示例2：使用调度器管理多个协程

```cpp
#include <iostream>
#include <coroutine>

// 协程函数1：计算
Task<int> calculate(int n) {
    std::cout << "  [Task " << n << "] Start calculating..." << std::endl;
    
    // 第1个暂停点
    co_await std::suspend_always{};
    std::cout << "  [Task " << n << "] Continue calculating..." << std::endl;
    
    // 第2个暂停点
    co_await std::suspend_always{};
    std::cout << "  [Task " << n << "] Final calculation..." << std::endl;
    
    int result = n * 2;
    std::cout << "  [Task " << n << "] Result: " << result << std::endl;
    
    co_return result;
}

// 主函数
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "协程调度器示例" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 创建调度器
    Scheduler scheduler;
    
    // 调度多个协程
    std::cout << "\n【阶段1：调度协程】" << std::endl;
    scheduler.schedule(calculate(1));
    scheduler.schedule(calculate(2));
    scheduler.schedule(calculate(3));
    
    // 运行调度器
    std::cout << "\n【阶段2：运行调度器】" << std::endl;
    scheduler.run();
    
    std::cout << "\n【阶段3：所有协程完成】" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}
```

**输出**：
```
========================================
协程调度器示例
========================================

【阶段1：调度协程】
[Scheduler] Task scheduled, queue size: 1
[Scheduler] Task scheduled, queue size: 2
[Scheduler] Task scheduled, queue size: 3

【阶段2：运行调度器】
[Scheduler] Starting event loop...
[Scheduler] Resuming task...
  [Task 1] Start calculating...
[Scheduler] Task suspended, re-queuing...
[Scheduler] Resuming task...
  [Task 2] Start calculating...
[Scheduler] Task suspended, re-queuing...
[Scheduler] Resuming task...
  [Task 3] Start calculating...
[Scheduler] Task suspended, re-queuing...
[Scheduler] Resuming task...
  [Task 1] Continue calculating...
[Scheduler] Task suspended, re-queuing...
[Scheduler] Resuming task...
  [Task 2] Continue calculating...
[Scheduler] Task suspended, re-queuing...
[Scheduler] Resuming task...
  [Task 3] Continue calculating...
[Scheduler] Task suspended, re-queuing...
[Scheduler] Resuming task...
  [Task 1] Final calculation...
  [Task 1] Result: 2
[Scheduler] Task completed, destroying...
[Scheduler] Resuming task...
  [Task 2] Final calculation...
  [Task 2] Result: 4
[Scheduler] Task completed, destroying...
[Scheduler] Resuming task...
  [Task 3] Final calculation...
  [Task 3] Result: 6
[Scheduler] Task completed, destroying...
[Scheduler] Event loop ended.

【阶段3：所有协程完成】
========================================
```

**关键观察**：
1. 三个协程交替执行（并发）
2. 每个协程在暂停点让出控制权
3. 调度器自动管理所有协程
4. 完成的协程自动销毁

---

### 示例3：协程之间的交互

```cpp
#include <iostream>
#include <string>

// 生产者协程
Task<void> producer(int id) {
    for (int i = 1; i <= 3; i++) {
        std::cout << "  [Producer " << id << "] Producing item " << i << std::endl;
        co_await std::suspend_always{};
    }
    std::cout << "  [Producer " << id << "] Done" << std::endl;
    co_return;
}

// 消费者协程
Task<void> consumer(int id) {
    for (int i = 1; i <= 3; i++) {
        std::cout << "  [Consumer " << id << "] Consuming item " << i << std::endl;
        co_await std::suspend_always{};
    }
    std::cout << "  [Consumer " << id << "] Done" << std::endl;
    co_return;
}

int main() {
    Scheduler scheduler;
    
    std::cout << "生产者-消费者模式" << std::endl;
    std::cout << "--------------------------------" << std::endl;
    
    // 调度生产者和消费者
    scheduler.schedule(producer(1));
    scheduler.schedule(consumer(1));
    scheduler.schedule(producer(2));
    scheduler.schedule(consumer(2));
    
    // 运行
    scheduler.run();
    
    return 0;
}
```

---

## 常见问题

### Q1: 调度器和Reactor有什么区别？

**Reactor**：
- 事件驱动（I/O事件）
- 调度Handler（事件处理器）
- 基于epoll等I/O多路复用

**调度器**：
- 协程驱动（协程暂停/恢复）
- 调度协程（coroutine）
- 基于协程的暂停点

**关系**：
- Reactor可以与调度器结合
- 调度器管理协程
- Reactor管理I/O事件
- 协程可以在Reactor中使用

---

### Q2: 为什么协程未完成要重新入队？

**原因**：
```cpp
Task<void> task() {
    doWork1();
    co_await std::suspend_always{};  // 暂停点1
    doWork2();
    co_await std::suspend_always{};  // 暂停点2
    doWork3();
    co_return;
}
```

协程有多个暂停点：
- 第1次resume：执行到暂停点1，暂停
- 第2次resume：从暂停点1继续，执行到暂停点2，暂停
- 第3次resume：从暂停点2继续，执行到完成

**重新入队**：确保协程能被再次调度

---

### Q3: 如何避免协程无限循环？

**问题代码**：
```cpp
Task<void> infiniteLoop() {
    while (true) {
        doWork();
        // 没有暂停点！
    }
    co_return;  // 永远不会到达
}
```

**问题**：
- 协程不暂停
- 调度器无法切换
- 其他协程饿死

**解决方案**：
```cpp
Task<void> goodLoop() {
    while (true) {
        doWork();
        co_await std::suspend_always{};  // 定期让出控制权
    }
    co_return;
}
```

**建议**：
- 循环中必须有暂停点
- 定期让出控制权
- 避免长时间占用CPU

---

### Q4: 调度器的性能如何？

**时间复杂度**：
- schedule: O(1) - 入队
- run（单次循环）: O(1) - 出队+resume
- 总体：O(n) - n是协程总的resume次数

**空间复杂度**：
- O(m) - m是协程数量

**性能对比**：

| 方案 | 创建开销 | 切换开销 | 内存占用 |
|------|---------|---------|---------|
| 线程 | 1ms | 1-10μs | 1MB/线程 |
| 协程+调度器 | 0.1μs | 10-100ns | 0.5KB/协程 |
| **提升** | **10000倍** | **100倍** | **2000倍** |

**结论**：协程+调度器性能远超线程

---

### Q5: 调度器是线程安全的吗？

**当前实现**：
```cpp
class Scheduler {
private:
    std::queue<std::coroutine_handle<>> task_queue;  // 非线程安全
    // ...
};
```

**不是线程安全的！**

**如果需要多线程**：
```cpp
class ThreadSafeScheduler {
private:
    std::queue<std::coroutine_handle<>> task_queue;
    std::mutex mtx;  // 添加锁
    
public:
    void schedule(std::coroutine_handle<> handle) {
        std::lock_guard<std::mutex> lock(mtx);
        task_queue.push(handle);
    }
    
    // run方法也要加锁
};
```

**建议**：
- 单线程：使用简单版本
- 多线程：使用线程安全版本
- 根据需求选择

---

## 核心总结

### 今日学习要点

**1. 调度器的价值**
```
手动管理协程：复杂、容易出错、不灵活
调度器管理：简单、自动化、易扩展
```

**2. 调度器的三大作用**
- 协程生命周期管理（创建→暂停→恢复→销毁）
- 任务调度（选择、切换、资源分配）
- 状态追踪（状态、依赖、异常）

**3. 调度器的三大组件**
- 任务队列（存储协程）
- 运行集合（跟踪状态）
- 事件循环（调度逻辑）

**4. 简单调度器实现**
```cpp
class Scheduler {
    std::queue<std::coroutine_handle<>> task_queue;
    
    void schedule(handle) { task_queue.push(handle); }
    
    void run() {
        while (!task_queue.empty()) {
            auto handle = task_queue.front();
            task_queue.pop();
            handle.resume();
            if (!handle.done()) {
                task_queue.push(handle);
            } else {
                handle.destroy();
            }
        }
    }
};
```

---

### 与前三周的联系

**epoll（第1周）**：
- epoll_wait循环 = 事件循环
- I/O事件队列 = 任务队列

**Reactor（第2周）**：
- Handler队列 = 任务队列
- 事件分发 = 任务调度

**协程（第3周）**：
- co_await = 暂停点（调度时机）
- handle.resume() = 恢复机制

**今天的调度器**：
- 综合了前三周的思想
- 为协程提供自动管理
- 是协程的"管家"

---

### 明天预告

**第83天：协程调度策略**

**学习内容**：
- FIFO vs 优先级
- 公平调度
- 负载均衡
- 抢占式调度

**今天是基础，明天是进阶！**

---

> **第82天，协程调度器基础学习完成！** 🎉  
> 理解了为什么需要调度器！  
> 掌握了调度器的三大作用和三大组件！  
> 实现了简单的调度器！  
> 明天将学习更高级的调度策略！  
> 休息好，明天见！💪🚀


