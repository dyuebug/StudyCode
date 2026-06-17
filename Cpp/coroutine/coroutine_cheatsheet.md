# C++20 协程速查表

**版本**：v1.0
**最后更新**：2026-06-26
**适用范围**：C++20及以上

---

## 📋 目录

1. [协程核心概念](#协程核心概念)
2. [Promise速查](#promise速查)
3. [Awaitable速查](#awaitable速查)
4. [coroutine_handle速查](#coroutine_handle速查)
5. [协程+Reactor模式](#协程reactor模式)
6. [代码模板](#代码模板)
7. [常见问题FAQ](#常见问题faq)
8. [性能对比表](#性能对比表)
9. [调试技巧](#调试技巧)
10. [常见陷阱](#常见陷阱)

---

## 协程核心概念

### 基本定义

```
协程（Coroutine）= 可暂停和恢复的函数
```

| 特性 | 说明 |
|------|------|
| 暂停 | co_await、co_yield |
| 恢复 | handle.resume() |
| 状态保存 | 局部变量、执行位置 |

---

### 三个关键字

| 关键字 | 作用 | 使用场景 | 示例 |
|--------|------|---------|------|
| `co_await` | 等待异步操作 | 异步I/O、网络请求 | `auto data = co_await asyncRead();` |
| `co_yield` | 生成值并暂停 | 生成器、序列 | `co_yield fibonacci_value;` |
| `co_return` | 协程返回 | 异步任务 | `co_return result;` |

**注意**：一个协程函数必须包含至少一个关键字

---

### 协程生命周期

```
创建 → 执行 → 暂停 → 恢复 → 完成 → 销毁
  ↓      ↓      ↓      ↓      ↓      ↓
调用   开始   co_xxx  resume  co_ret destroy
```

---

## Promise速查

### Promise类型模板

```cpp
template<typename T>
struct promise_type {
    // 1. 必需方法（5个）
    ReturnType get_return_object();
    suspend_xxx initial_suspend();
    suspend_always final_suspend() noexcept;
    void unhandled_exception();
    
    // 2. 返回方式（二选一）
    void return_void();              // co_return; 无返回值
    void return_value(T value);      // co_return value; 有返回值
    
    // 3. yield方式（可选）
    suspend_always yield_value(T value);  // co_yield value;
};
```

---

### Promise方法详解

| 方法 | 返回类型 | 调用时机 | 作用 |
|------|---------|---------|------|
| `get_return_object()` | 协程对象 | 创建协程时 | 返回给调用者的对象 |
| `initial_suspend()` | suspend_xxx | 协程开始前 | 控制是否立即执行 |
| `final_suspend()` | suspend_always | 协程结束后 | 控制是否保持状态 |
| `unhandled_exception()` | void | 异常发生时 | 处理未捕获异常 |
| `return_void()` | void | co_return; | 无返回值 |
| `return_value(T)` | void | co_return value; | 有返回值 |
| `yield_value(T)` | suspend_always | co_yield value; | 生成值 |

---

### initial_suspend() 决策

| 返回值 | 行为 | 适用场景 |
|--------|------|---------|
| `suspend_never` | 立即执行 | Task（异步任务） |
| `suspend_always` | 延迟执行 | Generator（生成器） |

**示例**：
```cpp
// Generator：延迟执行，需要next()驱动
std::suspend_always initial_suspend() { return {}; }

// Task：立即执行，创建即开始
std::suspend_never initial_suspend() { return {}; }
```

---

### final_suspend() 决策

**推荐**：总是返回 `suspend_always`

**原因**：
- ✅ 保持协程状态，可以获取结果
- ✅ 避免悬垂指针
- ✅ 手动控制销毁时机

```cpp
std::suspend_always final_suspend() noexcept { 
    return {}; 
}
```

---

## Awaitable速查

### Awaitable接口

```cpp
struct Awaitable {
    // 1. 是否需要暂停？
    bool await_ready();
    
    // 2. 暂停时调用（可选返回类型）
    void/bool/coroutine_handle<> await_suspend(std::coroutine_handle<>);
    
    // 3. 恢复时调用，返回co_await的结果
    T await_resume();
};
```

---

### 三大方法详解

| 方法 | 返回值 | 含义 | 典型实现 |
|------|--------|------|---------|
| `await_ready()` | bool | true=立即完成，false=需要暂停 | 尝试立即完成操作 |
| `await_suspend(handle)` | void/bool/handle | void=总是暂停，false=不暂停 | 注册到事件循环 |
| `await_resume()` | T | co_await的返回值 | 返回操作结果 |

---

### await_suspend() 返回值

| 返回类型 | 行为 | 使用场景 |
|---------|------|---------|
| `void` | 总是暂停 | 标准异步操作 |
| `bool` | false=不暂停，true=暂停 | 可能立即完成的操作 |
| `coroutine_handle<>` | 恢复指定协程（对称转移） | 协程调度器 |

---

### 内置Awaitable类型

```cpp
// 1. suspend_never - 永不暂停
struct suspend_never {
    bool await_ready() const noexcept { return true; }
    void await_suspend(std::coroutine_handle<>) const noexcept {}
    void await_resume() const noexcept {}
};

// 2. suspend_always - 总是暂停
struct suspend_always {
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<>) const noexcept {}
    void await_resume() const noexcept {}
};
```

**使用**：
```cpp
co_await std::suspend_never{};   // 不暂停，立即继续
co_await std::suspend_always{};  // 暂停，等待resume()
```

---

## coroutine_handle速查

### 基本操作

```cpp
#include <coroutine>

std::coroutine_handle<promise_type> handle;
```

| 方法 | 作用 | 返回值 | 注意事项 |
|------|------|--------|---------|
| `handle.resume()` | 恢复协程 | void | 协程未完成时才能调用 |
| `handle.destroy()` | 销毁协程 | void | 只能调用一次 |
| `handle.done()` | 是否完成 | bool | 检查状态 |
| `handle.promise()` | 获取Promise | promise_type& | 访问协程状态 |
| `handle.address()` | 获取地址 | void* | 用于序列化 |

---

### 创建handle

```cpp
// 1. 从Promise创建
auto handle = std::coroutine_handle<promise_type>::from_promise(promise);

// 2. 从地址创建（反序列化）
auto handle = std::coroutine_handle<promise_type>::from_address(addr);
```

---

### 典型用法

```cpp
class Task {
    std::coroutine_handle<promise_type> handle;
    
public:
    // 恢复协程
    void resume() {
        if (handle && !handle.done()) {
            handle.resume();
        }
    }
    
    // 析构时销毁
    ~Task() {
        if (handle) {
            handle.destroy();
        }
    }
};
```

---

## 协程+Reactor模式

### 架构图

```
用户代码（协程）
    ↓ co_await asyncRead()
ReadAwaitable
    ↓ await_suspend(handle)
Reactor
    ↓ registerRead(fd, handle)
epoll
    ↓ 检测到可读
Reactor
    ↓ handle.resume()
ReadAwaitable
    ↓ await_resume()
用户代码（协程）
    ↓ 继续执行
```

---

### 核心流程

| 步骤 | 组件 | 操作 |
|------|------|------|
| 1 | 用户代码 | `co_await asyncRead(fd)` |
| 2 | ReadAwaitable | `await_ready()` 尝试立即读取 |
| 3 | ReadAwaitable | `await_suspend(handle)` 注册到epoll |
| 4 | Reactor | 保存 `fd → handle` 映射 |
| 5 | epoll | 等待事件 |
| 6 | Reactor | 检测到fd可读 |
| 7 | Reactor | `handle.resume()` 恢复协程 |
| 8 | ReadAwaitable | `await_resume()` 返回数据 |
| 9 | 用户代码 | 继续执行 |

---

### ReadAwaitable模板

```cpp
struct ReadAwaitable {
    int fd;
    Reactor* reactor;
    char* buffer;
    size_t size;
    ssize_t result;
    
    // 1. 尝试立即读取
    bool await_ready() {
        result = recv(fd, buffer, size, MSG_DONTWAIT);
        if (result >= 0) return true;      // 成功，不暂停
        if (errno == EAGAIN) return false; // 未就绪，暂停
        return true;                        // 错误，不暂停
    }
    
    // 2. 暂停时注册到epoll
    void await_suspend(std::coroutine_handle<> handle) {
        reactor->registerRead(fd, handle);
    }
    
    // 3. 恢复时返回结果
    ssize_t await_resume() {
        if (result >= 0) return result;
        result = recv(fd, buffer, size, MSG_DONTWAIT);
        return result;
    }
};
```

---

## 代码模板

### Generator模板

```cpp
template<typename T>
class Generator {
public:
    struct promise_type {
        T current_value;
        
        Generator get_return_object() {
            return Generator{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        
        std::suspend_always yield_value(T value) {
            current_value = value;
            return {};
        }
        
        void return_void() {}
        void unhandled_exception() {}
    };
    
private:
    std::coroutine_handle<promise_type> handle;
    
public:
    Generator(std::coroutine_handle<promise_type> h) : handle(h) {}
    ~Generator() { if (handle) handle.destroy(); }
    
    T next() {
        handle.resume();
        return handle.promise().current_value;
    }
    
    bool done() const { return handle.done(); }
};
```

**使用示例**：
```cpp
Generator<int> fibonacci() {
    int a = 0, b = 1;
    while (true) {
        co_yield a;
        int next = a + b;
        a = b;
        b = next;
    }
}

auto gen = fibonacci();
cout << gen.next();  // 0
cout << gen.next();  // 1
cout << gen.next();  // 1
```

---

### Task模板

```cpp
template<typename T>
class Task {
public:
    struct promise_type {
        std::optional<T> result_value;
        
        Task get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        
        void return_value(T value) {
            result_value = value;
        }
        
        void unhandled_exception() {}
    };
    
private:
    std::coroutine_handle<promise_type> handle;
    
public:
    Task(std::coroutine_handle<promise_type> h) : handle(h) {}
    ~Task() { if (handle) handle.destroy(); }
    
    void resume() {
        if (handle && !handle.done()) {
            handle.resume();
        }
    }
    
    bool done() const { return handle.done(); }
    
    T result() {
        return handle.promise().result_value.value();
    }
};
```

**使用示例**：
```cpp
Task<int> calculate() {
    int sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += i;
        co_await std::suspend_always{};
    }
    co_return sum;
}

auto task = calculate();
while (!task.done()) {
    task.resume();
}
int result = task.result();  // 45
```

---

### 协程版handleClient模板

```cpp
Task<void> handleClient(int fd, Reactor* reactor) {
    char buffer[1024];
    
    while (true) {
        // 异步读取
        ssize_t n = co_await reactor->asyncRead(fd, buffer, sizeof(buffer));
        if (n <= 0) break;
        
        // 业务逻辑
        process(buffer, n);
        
        // 异步写入
        co_await reactor->asyncWrite(fd, buffer, n);
    }
    
    close(fd);
    co_return;
}
```

---

## 常见问题FAQ

### Q1: 何时使用协程？

**适合**：
- ✅ I/O密集型应用（网络、文件）
- ✅ 高并发场景（>10000连接）
- ✅ 需要清晰代码逻辑
- ✅ 异步操作较多

**不适合**：
- ❌ CPU密集型计算（用线程池）
- ❌ 简单同步程序
- ❌ 编译器不支持C++20

---

### Q2: 协程 vs 回调如何选择？

| 场景 | 推荐方案 | 理由 |
|------|---------|------|
| 新项目 | 协程 | 代码清晰，易维护 |
| 旧项目改造 | 渐进式 | 逐步引入协程 |
| 简单异步 | 回调 | 学习成本低 |
| 复杂逻辑 | 协程 | 避免回调地狱 |
| 对接C库 | 回调 | C不支持协程 |

---

### Q3: 协程性能开销多大？

**开销对比**：

| 操作 | 线程 | 协程 | 提升 |
|------|------|------|------|
| 创建 | 1ms | 0.1μs | 10000倍 |
| 切换 | 1-10μs | 10-100ns | 100倍 |
| 内存 | 1MB | 0.5KB | 2000倍 |

**结论**：协程几乎无开销

---

### Q4: 如何调试协程？

**方法1：日志输出**
```cpp
Task<void> func() {
    cout << "Start" << endl;
    co_await something();
    cout << "After await" << endl;
    co_return;
}
```

**方法2：GDB断点**
```bash
# 在协程函数设置断点
break func
# 查看协程状态
info locals
```

**方法3：状态追踪**
```cpp
struct promise_type {
    void return_value(T value) {
        cout << "Coroutine returning: " << value << endl;
        result_value = value;
    }
};
```

---

### Q5: 协程内存如何管理？

**协程帧分配**：
```cpp
// 默认：在堆上分配
operator new

// 优化：自定义分配器
void* operator new(size_t size) {
    return memory_pool.allocate(size);
}
```

**生命周期**：
- 创建：调用协程函数时分配
- 暂停：保持协程帧
- 销毁：调用handle.destroy()时释放

**注意**：
- ⚠️ final_suspend()返回suspend_always，需要手动destroy()
- ⚠️ 不要在协程完成前删除协程对象

---

### Q6: 协程可以嵌套吗？

**可以！** 协程可以调用其他协程。

```cpp
Task<int> innerTask() {
    co_await std::suspend_always{};
    co_return 42;
}

Task<void> outerTask() {
    int result = co_await innerTask();  // 调用另一个协程
    cout << result << endl;
    co_return;
}
```

---

### Q7: 协程如何处理异常？

**方法1：try-catch**
```cpp
Task<void> func() {
    try {
        auto data = co_await asyncRead();
        co_await asyncWrite(data);
    } catch (const std::exception& e) {
        cout << "Error: " << e.what() << endl;
    }
    co_return;
}
```

**方法2：Promise的unhandled_exception()**
```cpp
struct promise_type {
    std::exception_ptr exception;
    
    void unhandled_exception() {
        exception = std::current_exception();
    }
};

// 使用时检查
if (task.has_exception()) {
    std::rethrow_exception(task.get_exception());
}
```

---

### Q8: 协程和std::future有什么区别？

| 特性 | 协程 | std::future |
|------|------|------------|
| 语法 | co_await | .get() |
| 阻塞 | 不阻塞 | 阻塞 |
| 开销 | 小 | 大 |
| 灵活性 | 高 | 低 |
| 适用场景 | 高并发 | 简单异步 |

---

## 性能对比表

### 协程 vs 线程 vs 回调

| 维度 | 线程 | 回调 | 协程 | 最佳 |
|------|------|------|------|------|
| **创建开销** | 1ms | 0 | 0.1μs | 回调 |
| **切换开销** | 1-10μs | 函数调用 | 10-100ns | 回调 |
| **内存占用** | 1MB | 0 | 0.5KB | 回调 |
| **并发数量** | ~1000 | 无限 | ~100万 | 回调 |
| **代码可读性** | 好 | 差 | 好 | 协程 |
| **维护成本** | 中 | 高 | 低 | 协程 |
| **学习成本** | 低 | 低 | 高 | 回调 |

**结论**：协程 = 线程的编程模型 + 回调的性能

---

### 实际场景对比（10000并发连接）

| 方案 | 内存占用 | 创建时间 | 代码行数 | 可读性 |
|------|---------|---------|---------|--------|
| 多线程 | 10GB | 10s | 50 | ⭐⭐⭐⭐ |
| Reactor+回调 | 10MB | 1ms | 100 | ⭐⭐ |
| 协程+Reactor | 5MB | 1ms | 30 | ⭐⭐⭐⭐⭐ |

---

## 调试技巧

### 1. 打印协程状态

```cpp
struct promise_type {
    promise_type() {
        cout << "Coroutine created" << endl;
    }
    
    ~promise_type() {
        cout << "Coroutine destroyed" << endl;
    }
    
    std::suspend_always yield_value(T value) {
        cout << "Yielding: " << value << endl;
        current_value = value;
        return {};
    }
};
```

---

### 2. 使用静态分析工具

```bash
# Clang Tidy检查协程
clang-tidy --checks='-*,cppcoreguidelines-*' file.cpp

# AddressSanitizer检测内存问题
g++ -fsanitize=address -g file.cpp
```

---

### 3. GDB调试协程

```bash
# 设置断点
(gdb) break coroutine_function

# 查看协程帧
(gdb) info frame

# 查看局部变量
(gdb) info locals

# 单步执行
(gdb) next
```

---

### 4. 可视化协程执行流

```cpp
#define LOG_CORO(msg) \
    cout << "[Coro " << this << "] " << msg << endl;

Task<void> func() {
    LOG_CORO("Start");
    co_await something();
    LOG_CORO("After await");
    co_return;
}
```

---

## 常见陷阱

### ⚠️ 陷阱1：忘记destroy()

```cpp
// ❌ 错误：内存泄漏
Generator<int> gen = fibonacci();
// 协程帧永远不会释放

// ✅ 正确：在析构函数中destroy()
~Generator() {
    if (handle) {
        handle.destroy();
    }
}
```

---

### ⚠️ 陷阱2：协程完成后继续resume()

```cpp
// ❌ 错误：未定义行为
auto task = calculate();
while (true) {
    task.resume();  // task完成后继续调用
}

// ✅ 正确：检查done()
while (!task.done()) {
    task.resume();
}
```

---

### ⚠️ 陷阱3：final_suspend()返回suspend_never

```cpp
// ❌ 错误：协程帧立即销毁，无法获取结果
std::suspend_never final_suspend() noexcept {
    return {};
}

// ✅ 正确：返回suspend_always，保持状态
std::suspend_always final_suspend() noexcept {
    return {};
}
```

---

### ⚠️ 陷阱4：悬垂引用

```cpp
// ❌ 错误：局部变量的引用
Task<void> func() {
    int x = 42;
    co_await asyncOp(&x);  // asyncOp保存了x的指针
}  // x销毁，但asyncOp可能还在使用

// ✅ 正确：使用值传递或堆分配
Task<void> func() {
    auto x = std::make_shared<int>(42);
    co_await asyncOp(x);
}
```

---

### ⚠️ 陷阱5：在协程中使用thread_local

```cpp
// ⚠️ 危险：协程可能在不同线程恢复
thread_local int counter = 0;

Task<void> func() {
    counter++;  // 可能在线程A
    co_await something();
    counter++;  // 可能在线程B（counter是B的thread_local）
}
```

---

### ⚠️ 陷阱6：循环引用

```cpp
// ❌ 错误：Task持有自己的handle
class Task {
    std::coroutine_handle<promise_type> handle;
    Task* self;  // 循环引用
};

// ✅ 正确：避免循环引用
class Task {
    std::coroutine_handle<promise_type> handle;
    // 不需要self指针
};
```

---

## 编译器支持

| 编译器 | 版本要求 | 支持程度 | 备注 |
|--------|---------|---------|------|
| GCC | ≥ 11 | 完整支持 | 推荐 |
| Clang | ≥ 14 | 完整支持 | 推荐 |
| MSVC | ≥ 19.28 | 完整支持 | VS 2019 16.8+ |
| Apple Clang | ≥ 13 | 部分支持 | macOS 12+ |

**编译选项**：
```bash
# GCC/Clang
g++ -std=c++20 -fcoroutines file.cpp

# MSVC
cl /std:c++20 /await file.cpp
```

---

## 参考资料

### 官方文档
- [C++20 Coroutines (cppreference)](https://en.cppreference.com/w/cpp/language/coroutines)
- [C++ Core Guidelines - Coroutines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)

### 开源库
- **asio**：C++网络编程库，支持协程
- **libco**：腾讯开源的协程库（C++98）
- **cppcoro**：Lewis Baker的协程库

### 学习资源
- 《C++20 in Detail》- Bartłomiej Filipek
- 《C++ High Performance》- Björn Andrist
- CppCon演讲：Coroutines相关主题

---

> **速查表完成！** 🎉  
> 随时查阅，快速上手协程！  
> 从概念到实践，从模板到陷阱！  
> 保存收藏，终身受用！💪🚀




