# 协程基础 - 异步编程的新范式

> **学习日期**：2026-06-23（第78天）  
> **主题**：协程（Coroutine）基础概念与 C++20 Coroutines

---

## 🎯 核心概念

**协程（Coroutine）**：
- 可以暂停和恢复执行的函数
- 协作式多任务（Cooperative Multitasking）
- 用户态轻量级"线程"

**核心特性**：
- **暂停（Suspend）**：co_await、co_yield
- **恢复（Resume）**：继续执行
- **状态保存**：局部变量、执行位置

---

## 📊 协程 vs 普通函数

### 普通函数

```cpp
int add(int a, int b) {
    return a + b;  // 执行完就返回
}

// 执行流程
调用 add(1, 2)
  ↓
执行 return a + b
  ↓
返回 3
  ↓
函数结束（一次性）
```

**特点**：
- 一次性执行
- 无法暂停
- 无法恢复
- 无状态保存

---

### 协程函数

```cpp
Generator<int> count() {
    int i = 0;
    while (true) {
        co_yield i++;  // 暂停并返回
    }
}

// 执行流程
auto gen = count()
  ↓
gen.next()  // 第1次调用
  ↓ 执行到 co_yield 0
暂停，返回 0（保存状态：i=1）
  ↓
gen.next()  // 第2次调用
  ↓ 从暂停处恢复
执行到 co_yield 1
暂停，返回 1（保存状态：i=2）
  ↓
...（可以无限次）
```

**特点**：
- 可多次执行
- 可以暂停（co_yield）
- 可以恢复（next）
- 保存状态（i 的值）

---

## 🆚 协程 vs 线程 vs 回调

### 详细对比

| 维度 | 线程（Thread） | 协程（Coroutine） | 回调（Callback） |
|------|---------------|------------------|-----------------|
| **调度方式** | 抢占式（内核） | 协作式（用户） | 事件驱动 |
| **调度器** | 操作系统内核 | 用户程序 | 事件循环 |
| **创建开销** | 大（~1MB 栈） | 小（几百字节） | 无（只是函数） |
| **切换开销** | 慢（1-10μs） | 快（10-100ns） | 很快（函数调用） |
| **并发数量** | 受限（~1000） | 很大（~100万） | 很大 |
| **内存占用** | 大（每个 1MB） | 小（几百字节） | 小 |
| **编程模型** | 同步 | 同步写法，异步执行 | 异步（嵌套） |
| **代码可读性** | 好 | 好 | 差（回调地狱） |
| **错误处理** | try-catch | try-catch | 困难 |
| **同步机制** | 锁（复杂） | 无需（单线程） | 无需 |
| **适用场景** | CPU 密集 | I/O 密集 | I/O 密集 |

---

### 线程（Thread）

**示例**：
```cpp
void handleClient(int fd) {
    while (true) {
        char buf[1024];
        recv(fd, buf, sizeof(buf), 0);  // 阻塞
        process(buf);
        send(fd, result, ...);          // 阻塞
    }
}

// 每个客户端一个线程
for (int fd : clients) {
    std::thread(handleClient, fd).detach();
}
```

**问题**：
- 10000 个客户端 = 10000 个线程
- 内存占用：10000 × 1MB = ~10GB
- 上下文切换开销大

---

### 协程（Coroutine）

**示例**：
```cpp
Task<void> handleClient(int fd) {
    while (true) {
        auto data = co_await asyncRead(fd);  // 暂停，不阻塞
        auto result = process(data);
        co_await asyncWrite(fd, result);     // 暂停，不阻塞
    }
}

// 10000 个协程
for (int fd : clients) {
    handleClient(fd);  // 创建协程
}
// 所有协程在单线程中执行
```

**优势**：
- 10000 个客户端 = 10000 个协程
- 内存占用：10000 × 0.5KB = ~5MB（小 2000 倍）
- 切换快（100 倍）
- 无锁（单线程）

---

### 回调（Callback）

**示例**：
```cpp
void handleClient(int fd) {
    asyncRead(fd, [fd](char* data1) {
        auto result1 = process(data1);
        asyncWrite(fd, result1, [fd](int n1) {
            asyncRead(fd, [fd](char* data2) {
                auto result2 = process(data2);
                asyncWrite(fd, result2, [](int n2) {
                    // 嵌套4层，回调地狱
                });
            });
        });
    });
}
```

**问题**：
- 嵌套深（回调地狱）
- 逻辑分散
- 错误处理困难
- 难以理解和维护

**协程改写**：
```cpp
Task<void> handleClient(int fd) {
    auto data1 = co_await asyncRead(fd);
    auto result1 = process(data1);
    co_await asyncWrite(fd, result1);
    
    auto data2 = co_await asyncRead(fd);
    auto result2 = process(data2);
    co_await asyncWrite(fd, result2);
    // 线性逻辑，清晰易懂
}
```

---

## ⭐ 协程的优势

### 1. 高并发

**线程方式**：
- 1 万个连接 = 1 万个线程
- 内存：1 万 × 1MB = 10GB
- 极限：~1000 个线程（受内存和调度限制）

**协程方式**：
- 1 万个连接 = 1 万个协程
- 内存：1 万 × 0.5KB = 5MB
- 极限：~100 万个协程（只受内存限制）

**提升**：1000 倍

---

### 2. 低延迟

**线程切换**：
- 保存寄存器、栈
- 切换页表
- 内核调度
- 耗时：1-10 微秒

**协程切换**：
- 保存少量状态
- 用户态调度
- 无内核参与
- 耗时：10-100 纳秒

**提升**：100 倍

---

### 3. 简化异步编程

**回调方式**（难以理解）：
```cpp
asyncOp1([](result1) {
    asyncOp2(result1, [](result2) {
        asyncOp3(result2, [](result3) {
            // 使用 result3
        });
    });
});
```

**协程方式**（易于理解）：
```cpp
auto result1 = co_await asyncOp1();
auto result2 = co_await asyncOp2(result1);
auto result3 = co_await asyncOp3(result2);
// 使用 result3
```

**改进**：
- 代码线性（从上到下）
- 逻辑集中（在同一函数）
- 易于理解（同步写法）

---

### 4. 无锁编程

**多线程**（需要锁）：
```cpp
std::mutex mtx;
int shared_data = 0;

void thread1() {
    std::lock_guard<std::mutex> lock(mtx);
    shared_data++;  // 需要锁保护
}

void thread2() {
    std::lock_guard<std::mutex> lock(mtx);
    shared_data++;  // 需要锁保护
}
```

**协程**（无需锁）：
```cpp
int shared_data = 0;

Task<void> coroutine1() {
    shared_data++;  // 无需锁（单线程内执行）
    co_await something();
    shared_data++;  // 无需锁
}

Task<void> coroutine2() {
    shared_data++;  // 无需锁
    co_await something();
    shared_data++;  // 无需锁
}
```

**原因**：
- 协程在单线程内执行
- 协作式调度（主动让出 CPU）
- 只在 co_await 处切换
- 无竞态条件

---

## 🔧 C++20 Coroutines 基础

### 三个关键字

#### 1. co_await - 等待异步操作

**作用**：
- 暂停当前协程
- 等待异步操作完成
- 操作完成后恢复执行

**语法**：
```cpp
result = co_await awaitable_object;
```

**示例**：
```cpp
Task<string> fetchUrl(string url) {
    // 暂停，等待 HTTP 请求完成
    auto data = co_await httpGet(url);
    
    // 请求完成后恢复，继续执行
    co_return data;
}
```

**类似**：
- JavaScript 的 `await`
- Python 的 `await`
- C# 的 `await`

---

#### 2. co_yield - 生成值并暂停

**作用**：
- 暂停协程
- 返回一个值给调用者
- 下次调用时从此处恢复

**语法**：
```cpp
co_yield value;
```

**示例**：
```cpp
Generator<int> fibonacci() {
    int a = 0, b = 1;
    while (true) {
        co_yield a;  // 返回 a，然后暂停
        
        int next = a + b;
        a = b;
        b = next;
    }
}

// 使用
auto gen = fibonacci();
cout << gen.next();  // 0（第1次调用，执行到第1个 co_yield）
cout << gen.next();  // 1（第2次调用，从第1个 co_yield 恢复）
cout << gen.next();  // 1
cout << gen.next();  // 2
cout << gen.next();  // 3
```

**类似**：
- Python 的 `yield`
- JavaScript 的 `yield`

---

#### 3. co_return - 协程返回

**作用**：
- 协程返回结果
- 协程结束

**语法**：
```cpp
co_return value;
co_return;  // 无返回值
```

**示例**：
```cpp
Task<int> calculate() {
    int result = 0;
    for (int i = 0; i < 10; i++) {
        result += i;
        co_await yield();  // 让出 CPU
    }
    co_return result;  // 返回结果，协程结束
}

// 使用
auto task = calculate();
int result = co_await task;  // 等待协程完成，获取返回值
cout << result;  // 45
```

---

### 协程的生命周期

```
1. 创建（Create）
   ↓
   调用协程函数
   ↓
   auto task = myCoroutine();
   ↓
2. 执行（Execute）
   ↓
   开始执行协程体
   ↓
3. 暂停（Suspend）
   ↓
   遇到 co_await 或 co_yield
   ↓
   保存状态（局部变量、执行位置）
   ↓
   返回给调用者
   ↓
4. 恢复（Resume）
   ↓
   调用 resume() 或 next()
   ↓
   从暂停处继续执行
   ↓
   （可能多次暂停和恢复）
   ↓
5. 完成（Complete）
   ↓
   co_return 或执行到末尾
   ↓
6. 销毁（Destroy）
   ↓
   协程对象销毁
   ↓
   释放协程帧（Coroutine Frame）
```

---

## 🎯 协程应用场景

### 场景1：异步 I/O

**问题**：
- I/O 操作慢（网络、磁盘）
- 阻塞等待浪费 CPU
- 多线程开销大

**协程解决**：
```cpp
Task<void> handleClient(int fd) {
    while (true) {
        // 暂停，等待数据到达
        auto data = co_await asyncRead(fd);
        
        // 处理数据（CPU 工作）
        auto result = process(data);
        
        // 暂停，等待发送完成
        co_await asyncWrite(fd, result);
    }
}

// 1 万个客户端
for (int fd : clients) {
    handleClient(fd);  // 创建 1 万个协程
}
// 所有协程在单线程中交替执行
```

**好处**：
- 等待 I/O 时暂停，让出 CPU
- 其他协程继续执行
- 充分利用 CPU
- 高并发（1 万个协程，5MB 内存）

---

### 场景2：生成器（Generator）

**问题**：
- 生成大量数据占用内存
- 需要惰性求值（Lazy Evaluation）
- 只在需要时生成

**协程解决**：
```cpp
Generator<int> range(int start, int end) {
    for (int i = start; i < end; i++) {
        co_yield i;  // 按需生成，不占用内存
    }
}

// 使用
for (int i : range(0, 1000000)) {
    cout << i << endl;
    if (i > 10) break;
    // 只生成了 11 个数，不是 100 万
}
```

**好处**：
- 惰性求值（按需生成）
- 节省内存（不预先生成）
- 无限序列（可以生成无限个数）

---

### 场景3：协程 + Reactor

**传统 Reactor**（回调）：
```cpp
class ReadHandler {
    void handleEvent() {
        recv(fd, buf, ...);
        pool->enqueue([buf, fd]() {
            auto result = process(buf);
            send(fd, result, ...);
        });
    }
};
```

**协程 + Reactor**（同步写法）：
```cpp
Task<void> handleClient(int fd, Reactor* reactor) {
    while (true) {
        // 同步写法，异步执行
        auto data = co_await reactor->asyncRead(fd);
        auto result = process(data);
        co_await reactor->asyncWrite(fd, result);
    }
}

// Reactor 的 asyncRead 实现
Awaitable<string> Reactor::asyncRead(int fd) {
    // 注册 fd 到 epoll
    // 返回 Awaitable 对象
    // 数据到达时恢复协程
}
```

**好处**：
- 代码更清晰（线性逻辑）
- 逻辑更集中（在同一函数）
- 易于理解和维护
- 避免回调地狱

---

## 🏆 工业级协程库

### 1. libco（腾讯）

**特点**：
- C 语言实现
- 微信后台广泛使用
- 支持百万级并发
- Hook 系统调用（透明协程化）

**示例**：
```cpp
void* routine(void* arg) {
    // 自动协程化的 I/O
    read(fd, buf, len);   // 阻塞式写法，协程式执行
    write(fd, buf, len);
}

// 创建协程
co_create(&co, routine, arg);
co_resume(co);
```

---

### 2. Boost.Coroutine2

**特点**：
- C++ 实现
- Boost 库的一部分
- 跨平台
- 支持 C++11

---

### 3. cppcoro（Lewis Baker）

**特点**：
- C++20 Coroutines 的参考实现
- 现代 C++ 风格
- 异步 I/O 库

---

## 📊 协程 vs Reactor

| 维度 | Reactor | 协程 + Reactor |
|------|---------|---------------|
| **编程模型** | 回调 | 同步写法 |
| **代码可读性** | 中（回调嵌套） | 高（线性逻辑） |
| **错误处理** | 困难 | 简单（try-catch） |
| **学习曲线** | 中 | 低 |
| **性能** | 高 | 高（相当） |
| **内存占用** | 低 | 低（相当） |
| **并发数量** | 高 | 更高 |

**结论**：
- 协程是 Reactor 的更高层抽象
- 保留 Reactor 的性能
- 提升代码可读性

---

## 🎓 总结

### 核心要点

**协程 = 可暂停和恢复的函数**

**三个关键字**：
- `co_await`：等待异步操作
- `co_yield`：生成值并暂停
- `co_return`：协程返回

**优势**：
- 高并发（100万协程）
- 低延迟（快100倍）
- 简化异步编程
- 无锁编程

**应用场景**：
- 异步 I/O
- 生成器
- 协程 + Reactor

---

> **协程是异步编程的未来！** 🚀  
> 同步写法，异步执行，兼得两者之长！
