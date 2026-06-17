# 协程核心概念速查

> **学习日期**：2026-06-24（第79天）  
> **主题**：Promise、Awaitable、coroutine_handle 核心概念

---

## 🎯 三大核心组件

### 1. Promise 类型

**作用**：配置协程行为

**必需方法**：
```cpp
struct promise_type {
    // 1. 返回协程对象
    Generator get_return_object();
    
    // 2. 协程开始时的行为
    suspend_never/suspend_always initial_suspend();
    
    // 3. 协程结束时的行为
    suspend_always final_suspend() noexcept;
    
    // 4. 异常处理
    void unhandled_exception();
    
    // 5. co_yield 或 co_return（二选一或都有）
    suspend_always yield_value(T value);  // co_yield
    void return_value(T value);           // co_return
    void return_void();                   // co_return（无返回值）
};
```

**关键理解**：
- Promise 是协程的"配置文件"
- 编译器根据 Promise 生成协程代码
- 不同的 Promise → 不同的协程行为

---

### 2. coroutine_handle

**作用**：操作协程

**常用方法**：
```cpp
std::coroutine_handle<promise_type> handle;

// 恢复协程
handle.resume();

// 销毁协程
handle.destroy();

// 检查是否完成
bool done = handle.done();

// 访问 Promise
promise_type& p = handle.promise();

// 从 Promise 创建 handle
auto h = std::coroutine_handle<promise_type>::from_promise(promise);
```

**关键理解**：
- handle 是协程的"遥控器"
- 通过 handle 控制协程的暂停和恢复
- 协程对象通常持有 handle

---

### 3. Awaitable 对象

**作用**：定义 co_await 的行为

**接口**：
```cpp
struct Awaitable {
    // 是否需要暂停？
    bool await_ready();
    
    // 暂停时调用
    void await_suspend(std::coroutine_handle<>);
    
    // 恢复时调用，返回 co_await 的结果
    auto await_resume();
};
```

**内置类型**：
```cpp
// 永不暂停
std::suspend_never

// 总是暂停
std::suspend_always
```

**关键理解**：
- Awaitable 控制"何时暂停、何时恢复"
- co_await obj → 调用 obj 的三个方法
- 可以自定义 Awaitable 实现复杂逻辑

---

## 📊 协程执行流程

### 完整流程

```
1. 调用协程函数 myCoroutine()
   ↓
2. 编译器创建协程帧
   - 分配内存
   - 创建 Promise 对象
   ↓
3. promise.get_return_object()
   - 返回协程对象
   ↓
4. promise.initial_suspend()
   - suspend_never → 立即开始
   - suspend_always → 延迟执行
   ↓
5. 执行协程体
   ↓
6. 遇到 co_yield / co_await
   - co_yield value → promise.yield_value(value)
   - co_await obj → obj.await_ready/suspend/resume()
   - 保存状态，暂停
   ↓
7. 外部调用 handle.resume()
   - 从暂停处恢复
   ↓
8. co_return value
   - promise.return_value(value)
   ↓
9. promise.final_suspend()
   - 返回 suspend_always → 保持状态
   ↓
10. 外部调用 handle.destroy()
    - 销毁协程帧
```

---

### Generator 流程

```cpp
Generator<int> gen = fibonacci();  // 步骤 1-4
int val1 = gen.next();             // 步骤 5-6（第1次）
int val2 = gen.next();             // 步骤 7, 5-6（第2次）
// ~Generator()                    // 步骤 10
```

**详细**：
```
gen.next() 第1次：
  → handle.resume()
  → 执行协程体，遇到第1个 co_yield 0
  → promise.yield_value(0)，保存 0，暂停
  → 返回 promise.current_value = 0

gen.next() 第2次：
  → handle.resume()
  → 从第1个 co_yield 后继续
  → 执行到第2个 co_yield 1
  → promise.yield_value(1)，保存 1，暂停
  → 返回 promise.current_value = 1
```

---

### Task 流程

```cpp
Task<int> task = calculate();  // 步骤 1-5（立即开始）
task.resume();                 // 步骤 7（多次）
int result = task.result();    // 获取结果
```

**详细**：
```
创建 task：
  → promise.initial_suspend() 返回 suspend_never
  → 立即开始执行
  → 遇到 co_await suspend_always{}，暂停

task.resume()：
  → 从暂停处恢复
  → 继续执行，遇到下一个 co_await，再次暂停

最后一次 resume()：
  → 执行到 co_return 45
  → promise.return_value(45)，保存结果
  → promise.final_suspend()

task.result()：
  → 返回 promise.result_value = 45
```

---

## 🔑 关键概念对比

### initial_suspend

| 返回值 | 行为 | 适用 |
|--------|------|------|
| suspend_never | 立即开始执行 | Task（异步任务） |
| suspend_always | 延迟执行，需 resume | Generator（生成器） |

**示例**：
```cpp
// Generator：延迟执行
std::suspend_always initial_suspend() { return {}; }

auto gen = fibonacci();  // 创建，但未执行
gen.next();              // 第1次 resume，开始执行

// Task：立即执行
std::suspend_never initial_suspend() { return {}; }

auto task = calculate();  // 创建并开始执行
task.resume();            // 从第1个暂停处恢复
```

---

### final_suspend

| 返回值 | 行为 |
|--------|------|
| suspend_always | 保持协程状态，不自动销毁 |
| suspend_never | 立即销毁（危险，不推荐） |

**推荐**：
```cpp
std::suspend_always final_suspend() noexcept { 
    return {}; 
}
```

**原因**：
- 保持协程状态，可以获取结果
- 避免悬垂指针
- 手动调用 destroy() 销毁

---

### yield_value vs return_value

| 方法 | 关键字 | 调用时机 | 行为 |
|------|--------|----------|------|
| yield_value(T) | co_yield | 多次 | 保存值，暂停 |
| return_value(T) | co_return | 一次 | 保存值，结束 |
| return_void() | co_return | 一次 | 结束（无值） |

**Generator**：
```cpp
suspend_always yield_value(T value) {
    current_value = value;  // 保存
    return {};              // 暂停
}

void return_void() {}  // Generator 不 co_return
```

**Task**：
```cpp
void return_value(T value) {
    result_value = value;  // 保存
}
// 不需要 yield_value
```

---

## 💡 常见模式

### 模式1：惰性生成（Generator）

**特点**：
- 按需生成
- co_yield 多次
- initial_suspend 返回 suspend_always

**使用**：
```cpp
Generator<int> range(int n) {
    for (int i = 0; i < n; i++) {
        co_yield i;
    }
}

auto gen = range(100);
while (!gen.done()) {
    int val = gen.next();
    // 只生成需要的值
}
```

---

### 模式2：异步任务（Task）

**特点**：
- 返回结果
- co_return 一次
- initial_suspend 返回 suspend_never

**使用**：
```cpp
Task<int> asyncTask() {
    int result = 0;
    // 异步操作
    co_await someOperation();
    co_return result;
}

auto task = asyncTask();  // 立即开始
// ... 做其他事
int result = task.result();  // 获取结果
```

---

### 模式3：协程链（Awaitable）

**特点**：
- 一个协程等待另一个协程
- 实现 Awaitable 接口

**示例**（简化）：
```cpp
template<typename T>
struct TaskAwaitable {
    Task<T>& task;
    
    bool await_ready() { 
        return task.done(); 
    }
    
    void await_suspend(std::coroutine_handle<> h) {
        // 保存 h，task 完成时恢复 h
    }
    
    T await_resume() { 
        return task.result(); 
    }
};

// 使用
Task<int> caller() {
    Task<int> child = asyncTask();
    int result = co_await TaskAwaitable{child};
    co_return result;
}
```

---

## ⚠️ 常见陷阱

### 1. 忘记 destroy

**问题**：
```cpp
{
    auto gen = fibonacci();
    gen.next();
}  // 析构，但没有 destroy，内存泄漏
```

**解决**：
```cpp
~Generator() {
    if (handle) {
        handle.destroy();  // 必须手动销毁
    }
}
```

---

### 2. 使用已销毁的协程

**问题**：
```cpp
auto gen = fibonacci();
gen.next();
gen.~Generator();  // 手动销毁
gen.next();        // ❌ 使用已销毁的协程
```

**解决**：
- 不要手动调用析构函数
- 或者设置 handle = nullptr

---

### 3. final_suspend 返回 suspend_never

**问题**：
```cpp
std::suspend_never final_suspend() noexcept { 
    return {};  // ❌ 立即销毁
}

// 协程完成后立即销毁
int result = task.result();  // ❌ 悬垂指针
```

**解决**：
```cpp
std::suspend_always final_suspend() noexcept { 
    return {};  // ✅ 保持状态
}
```

---

### 4. 异常未处理

**问题**：
```cpp
void unhandled_exception() {
    // 什么都不做
}

// 协程抛出异常，但未处理，程序可能崩溃
```

**解决**：
```cpp
void unhandled_exception() {
    exception = std::current_exception();
}

// 在 next() 或 result() 中重新抛出
if (promise().exception) {
    std::rethrow_exception(promise().exception);
}
```

---

## 🎓 总结

**核心三要素**：
```
Promise：配置协程行为
coroutine_handle：操作协程
Awaitable：定义暂停和恢复
```

**Generator 关键**：
```
co_yield → yield_value() → 暂停
next() → resume() → 恢复
```

**Task 关键**：
```
co_return → return_value() → 保存结果
result() → 获取结果
```

**Awaitable 关键**：
```
co_await obj →
  obj.await_ready()
  obj.await_suspend(handle)
  obj.await_resume()
```

---

> **协程核心概念掌握！** 🎉  
> Promise、Awaitable、coroutine_handle 三大组件！
