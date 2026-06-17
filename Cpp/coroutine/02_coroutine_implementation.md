# 协程实战 - Promise 与 Awaitable

> **学习日期**：2026-06-24（第79天）  
> **主题**：协程实现原理与核心组件

---

## 🎯 核心概念

**C++20 协程的三大组件**：

1. **Promise 类型**：配置协程行为
2. **coroutine_handle**：协程句柄（操作协程）
3. **Awaitable 对象**：定义暂停和恢复行为

---

## 📊 协程运行流程

```
1. 调用协程函数
   ↓
2. 编译器创建协程帧（Coroutine Frame）
   - 分配内存
   - 创建 Promise 对象
   - 保存局部变量
   ↓
3. 调用 promise.get_return_object()
   - 返回协程对象（如 Generator、Task）
   ↓
4. 调用 promise.initial_suspend()
   - 决定是否立即开始执行
   ↓
5. 执行协程体
   ↓
6. 遇到 co_yield / co_await
   - 调用 promise.yield_value() 或 awaitable.await_suspend()
   - 保存状态
   - 暂停协程
   ↓
7. 外部调用 handle.resume()
   - 从暂停处恢复执行
   ↓
8. co_return 或执行到末尾
   - 调用 promise.return_value() 或 promise.return_void()
   - 调用 promise.final_suspend()
   ↓
9. 销毁协程帧
   - 释放内存
   - 销毁 Promise 对象
```

---

## 💻 实现 Generator

### 完整实现

```cpp
#include <coroutine>
#include <iostream>
#include <exception>

// ============================================
// Generator 类定义
// ============================================

template<typename T>
class Generator {
public:
    // ----------------------------------------
    // Promise 类型（协程的核心配置）
    // ----------------------------------------
    struct promise_type {
        T current_value;              // 当前 co_yield 的值
        std::exception_ptr exception; // 异常指针
        
        // 1. 协程创建时调用，返回 Generator 对象
        Generator get_return_object() {
            return Generator{
                std::coroutine_handle<promise_type>::from_promise(*this)
            };
        }
        
        // 2. 协程开始时调用
        // suspend_never：立即开始执行
        // suspend_always：延迟执行（需要手动 resume）
        std::suspend_always initial_suspend() { 
            return ; 
        }
        
        // 3. 协程结束时调用
        // suspend_always：保持协程状态，不立即销毁
        std::suspend_always final_suspend() noexcept { 
            return {}; 
        }
        
        // 4. co_yield 调用时
        // 保存值，并暂停协程
        std::suspend_always yield_value(T value) {
            current_value = value;  // 保存值
            return {};              // 返回 suspend_always，暂停协程
        }
        
        // 5. Generator 不使用 co_return
        void return_void() {}
        
        // 6. 异常处理
        void unhandled_exception() {
            exception = std::current_exception();
        }
    };
    
    // ----------------------------------------
    // Generator 类成员
    // ----------------------------------------
private:
    std::coroutine_handle<promise_type> handle;
    
public:
    // 构造函数
    explicit Generator(std::coroutine_handle<promise_type> h)
        : handle(h) {}
    
    // 析构函数：销毁协程
    ~Generator() {
        if (handle) {
            handle.destroy();
        }
    }
    
    // 禁止拷贝
    Generator(const Generator&) = delete;
    Generator& operator=(const Generator&) = delete;
    
    // 支持移动
    Generator(Generator&& other) noexcept
        : handle(other.handle) {
        other.handle = nullptr;
    }
    
    Generator& operator=(Generator&& other) noexcept {
        if (this != &other) {
            if (handle) {
                handle.destroy();
            }
            handle = other.handle;
            other.handle = nullptr;
        }
        return *this;
    }
    
    // ----------------------------------------
    // 核心方法
    // ----------------------------------------
    
    // 获取下一个值
    T next() {
        // 恢复协程执行
        handle.resume();
        
        // 检查异常
        if (handle.promise().exception) {
            std::rethrow_exception(handle.promise().exception);
        }
        
        // 返回当前值
        return handle.promise().current_value;
    }
    
    // 是否完成
    bool done() const {
        return handle.done();
    }
};

// ============================================
// 使用示例
// ============================================

// 示例1：斐波那契数列生成器
Generator<int> fibonacci() {
    int a = 0, b = 1;
    
    while (true) {
        co_yield a;  // 暂停并返回 a
        
        int next = a + b;
        a = b;
        b = next;
    }
}

// 示例2：范围生成器
Generator<int> range(int start, int end) {
    for (int i = start; i < end; i++) {
        co_yield i;
    }
}

// 主函数
int main() {
    std::cout << "=== Generator 示例 ===" << std::endl;
    
    // 测试斐波那契
    std::cout << "\n斐波那契数列：" << std::endl;
    auto fib = fibonacci();
    for (int i = 0; i < 10; i++) {
        std::cout << fib.next() << " ";
    }
    std::cout << std::endl;
    
    // 测试范围
    std::cout << "\n范围 [0, 10)：" << std::endl;
    auto r = range(0, 10);
    while (!r.done()) {
        std::cout << r.next() << " ";
    }
    std::cout << std::endl;
    
    return 0;
}

/*
输出：
=== Generator 示例 ===

斐波那契数列：
0 1 1 2 3 5 8 13 21 34

范围 [0, 10)：
0 1 2 3 4 5 6 7 8 9
*/
```

---

### Generator 关键点解析

**1. Promise 类型**：
```cpp
struct promise_type {
    // co_yield value 时调用
    std::suspend_always yield_value(T value) {
        current_value = value;  // 保存值
        return {};              // 暂停协程
    }
};
```

**作用**：
- `co_yield value` → 编译器调用 `promise.yield_value(value)`
- 保存值到 `current_value`
- 返回 `suspend_always`，暂停协程

**2. next() 方法**：
```cpp
T next() {
    handle.resume();  // 恢复协程执行
    return handle.promise().current_value;  // 获取值
}
```

**执行流程**：
```
调用 next()
  ↓
resume()：从上次暂停处继续执行
  ↓
执行到下一个 co_yield value
  ↓
调用 yield_value(value)，保存值并暂停
  ↓
返回 current_value
```

**3. 生命周期**：
```cpp
auto gen = fibonacci();  // 创建协程，但未开始执行（initial_suspend 返回 suspend_always）
gen.next();              // 第1次 resume，执行到第1个 co_yield
gen.next();              // 第2次 resume，从第1个 co_yield 恢复，执行到第2个 co_yield
...
~Generator();            // 销毁时调用 handle.destroy()
```

---

## 💻 实现 Task

### 完整实现

```cpp
#include <coroutine>
#include <iostream>
#include <exception>
#include <optional>

// ============================================
// Task 类定义
// ============================================

template<typename T>
class Task {
public:
    // ----------------------------------------
    // Promise 类型
    // ----------------------------------------
    struct promise_type {
        std::optional<T> result_value;  // 结果值（可能未设置）
        std::exception_ptr exception;    // 异常指针
        
        // 1. 协程创建时调用
        Task get_return_object() {
            return Task{
                std::coroutine_handle<promise_type>::from_promise(*this)
            };
        }
        
        // 2. 协程开始时：立即开始执行
        std::suspend_never initial_suspend() { 
            return {}; 
        }
        
        // 3. 协程结束时：保持状态
        std::suspend_always final_suspend() noexcept { 
            return {}; 
        }
        
        // 4. co_return value 调用时
        void return_value(T value) {
            result_value = value;
        }
        
        // 5. 异常处理
        void unhandled_exception() {
            exception = std::current_exception();
        }
    };
    
    // ----------------------------------------
    // Task 类成员
    // ----------------------------------------
private:
    std::coroutine_handle<promise_type> handle;
    
public:
    // 构造函数
    explicit Task(std::coroutine_handle<promise_type> h)
        : handle(h) {}
    
    // 析构函数
    ~Task() {
        if (handle) {
            handle.destroy();
        }
    }
    
    // 禁止拷贝
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
    
    // 支持移动
    Task(Task&& other) noexcept
        : handle(other.handle) {
        other.handle = nullptr;
    }
    
    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            if (handle) {
                handle.destroy();
            }
            handle = other.handle;
            other.handle = nullptr;
        }
        return *this;
    }
    
    // ----------------------------------------
    // 核心方法
    // ----------------------------------------
    
    // 恢复协程
    void resume() {
        if (handle && !handle.done()) {
            handle.resume();
        }
    }
    
    // 是否完成
    bool done() const {
        return handle.done();
    }
    
    // 获取结果
    T result() {
        // 检查异常
        if (handle.promise().exception) {
            std::rethrow_exception(handle.promise().exception);
        }
        
        // 检查结果
        if (!handle.promise().result_value.has_value()) {
            throw std::runtime_error("协程未完成或未返回值");
        }
        
        return handle.promise().result_value.value();
    }
};

// ============================================
// 使用示例
// ============================================

// 示例1：简单计算任务
Task<int> calculate() {
    int sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += i;
        co_await std::suspend_always{};  // 让出 CPU
    }
    co_return sum;  // 返回结果
}

// 示例2：阶乘任务
Task<int> factorial(int n) {
    int result = 1;
    for (int i = 1; i <= n; i++) {
        result *= i;
        co_await std::suspend_always{};
    }
    co_return result;
}

// 主函数
int main() {
    std::cout << "=== Task 示例 ===" << std::endl;
    
    // 测试计算任务
    std::cout << "\n计算 0+1+...+9：" << std::endl;
    auto task1 = calculate();
    
    // 手动驱动协程
    while (!task1.done()) {
        std::cout << "执行一步..." << std::endl;
        task1.resume();
    }
    
    std::cout << "结果：" << task1.result() << std::endl;  // 45
    
    // 测试阶乘
    std::cout << "\n计算 5!：" << std::endl;
    auto task2 = factorial(5);
    
    while (!task2.done()) {
        task2.resume();
    }
    
    std::cout << "结果：" << task2.result() << std::endl;  // 120
    
    return 0;
}

/*
输出：
=== Task 示例 ===

计算 0+1+...+9：
执行一步...
执行一步...
执行一步...
执行一步...
执行一步...
执行一步...
执行一步...
执行一步...
执行一步...
执行一步...
结果：45

计算 5!：
结果：120
*/
```

---

### Task 关键点解析

**1. Promise 类型**：
```cpp
struct promise_type {
    // co_return value 时调用
    void return_value(T value) {
        result_value = value;  // 保存结果
    }
};
```

**作用**：
- `co_return value` → 编译器调用 `promise.return_value(value)`
- 保存结果到 `result_value`

**2. result() 方法**：
```cpp
T result() {
    if (!handle.promise().result_value.has_value()) {
        throw std::runtime_error("协程未完成");
    }
    return handle.promise().result_value.value();
}
```

**使用**：
- 协程完成后调用
- 获取 co_return 的值

**3. initial_suspend()**：
```cpp
std::suspend_never initial_suspend() { 
    return {};  // 立即开始执行
}
```

**对比**：
- Generator：`suspend_always`（延迟执行）
- Task：`suspend_never`（立即执行）

---

## 🔧 Awaitable 接口

### Awaitable 的定义

```cpp
struct Awaitable {
    // 1. 是否需要暂停？
    //    返回 true：不暂停，立即继续
    //    返回 false：暂停协程
    bool await_ready();
    
    // 2. 暂停时调用
    //    参数：当前协程的 handle
    //    可以保存 handle，稍后恢复
    void await_suspend(std::coroutine_handle<> handle);
    
    // 3. 恢复时调用
    //    返回值：co_await 表达式的结果
    auto await_resume();
};
```

---

### 内置 Awaitable

**1. suspend_never**：
```cpp
struct suspend_never {
    bool await_ready() const noexcept { 
        return true;  // 不需要暂停
    }
    
    void await_suspend(std::coroutine_handle<>) const noexcept {}
    
    void await_resume() const noexcept {}
};

// 使用
co_await std::suspend_never{};  // 不暂停，立即继续
```

**2. suspend_always**：
```cpp
struct suspend_always {
    bool await_ready() const noexcept { 
        return false;  // 需要暂停
    }
    
    void await_suspend(std::coroutine_handle<>) const noexcept {}
    
    void await_resume() const noexcept {}
};

// 使用
co_await std::suspend_always{};  // 暂停，等待 resume
```

---

### 自定义 Awaitable

**示例：带返回值的 Awaitable**：
```cpp
struct IntAwaitable {
    int value;
    
    // 总是暂停
    bool await_ready() { 
        return false; 
    }
    
    // 暂停时什么都不做
    void await_suspend(std::coroutine_handle<>) {}
    
    // 恢复时返回值
    int await_resume() { 
        return value; 
    }
};

// 使用
Task<void> test() {
    int x = co_await IntAwaitable{42};
    std::cout << x << std::endl;  // 42
}
```

---

## 📊 Generator vs Task

| 维度 | Generator | Task |
|------|-----------|------|
| **关键字** | co_yield | co_return |
| **返回值** | 多次（每次 yield） | 一次（return） |
| **Promise 方法** | yield_value() | return_value() |
| **initial_suspend** | suspend_always（延迟） | suspend_never（立即） |
| **使用场景** | 生成序列 | 异步任务 |
| **示例** | fibonacci() | calculate() |

---

## 🎓 总结

### 核心组件

**1. Promise 类型**：
- 配置协程行为
- 定义 yield_value() / return_value()
- 定义 initial_suspend() / final_suspend()

**2. coroutine_handle**：
- 操作协程（resume、destroy）
- 访问 Promise（promise()）
- 检查状态（done()）

**3. Awaitable**：
- 定义暂停和恢复行为
- await_ready() / await_suspend() / await_resume()
- 内置：suspend_never、suspend_always

---

### 实现流程

**Generator**：
```
1. co_yield value
   ↓
2. 调用 promise.yield_value(value)
   ↓
3. 保存 value，返回 suspend_always
   ↓
4. 暂停协程
   ↓
5. 外部调用 next()
   ↓
6. 调用 handle.resume()
   ↓
7. 从暂停处恢复
```

**Task**：
```
1. co_return value
   ↓
2. 调用 promise.return_value(value)
   ↓
3. 保存 value
   ↓
4. 协程结束
   ↓
5. 外部调用 result()
   ↓
6. 返回保存的 value
```

---

> **协程实现完成！** 🎉  
> 从原理到代码，深入理解协程！
