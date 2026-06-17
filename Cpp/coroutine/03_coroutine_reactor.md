# 协程 + Reactor - 知识融合

> **学习日期**：2026-06-25（第80天）  
> **主题**：协程化 Reactor，用协程简化异步编程

---

## 🎯 核心目标

**用协程简化 Reactor 的异步编程**：
- 把 I/O 操作变成 Awaitable
- 用 co_await 等待 I/O 完成
- 同步写法，异步执行

---

## 📊 为什么要协程化 Reactor？

### 传统 Reactor（回调）的问题

```cpp
class ReadHandler {
    void handleEvent() {
        // 读取数据
        recv(fd, buf, ...);
        
        // 业务逻辑（假设需要异步）
        asyncProcess(buf, [this](Result result1) {
            // 发送结果
            asyncWrite(fd, result1, [this](int n) {
                // 继续读取
                asyncRead(fd, [this](char* data2) {
                    asyncProcess(data2, [this](Result result2) {
                        asyncWrite(fd, result2, [this](int n) {
                            // 嵌套 5 层，回调地狱
                        });
                    });
                });
            });
        });
    }
};
```

**问题**：
- ❌ 嵌套深（回调地狱）
- ❌ 逻辑分散（难以理解）
- ❌ 错误处理困难
- ❌ 状态管理复杂

---

### 协程化的目标

```cpp
Task<void> handleClient(int fd) {
    while (true) {
        // 同步写法，异步执行
        auto data = co_await asyncRead(fd);
        if (data.empty()) break;
        
        auto result = process(data);
        co_await asyncWrite(fd, result);
    }
}
```

**好处**：
- ✅ 代码线性（从上到下）
- ✅ 逻辑集中（在同一函数）
- ✅ 错误处理简单（try-catch）
- ✅ 易于理解和维护

---

## 🔧 核心思想：I/O 操作变成 Awaitable

### 关键设计

**把异步 I/O 封装成 Awaitable**：
```cpp
// co_await asyncRead(fd) 
//   → 创建 ReadAwaitable
//   → co_await ReadAwaitable
//   → 调用三个方法
```

---

### ReadAwaitable 完整实现

```cpp
#include <coroutine>
#include <sys/socket.h>
#include <errno.h>

// 前向声明
class Reactor;

// ============================================
// ReadAwaitable - 异步读取
// ============================================

struct ReadAwaitable {
    int fd;                  // 文件描述符
    Reactor* reactor;        // Reactor 指针
    char* buffer;            // 读取缓冲区
    size_t size;             // 缓冲区大小
    ssize_t result;          // 读取结果
    
    // 构造函数
    ReadAwaitable(int fd, Reactor* r, char* buf, size_t sz)
        : fd(fd), reactor(r), buffer(buf), size(sz), result(0) {}
    
    // ----------------------------------------
    // 1. 是否需要暂停？
    // ----------------------------------------
    bool await_ready() {
        // 尝试立即读取（非阻塞）
        result = recv(fd, buffer, size, MSG_DONTWAIT);
        
        if (result >= 0) {
            // 读取成功（包括 EOF）
            return true;  // 不暂停，立即继续
        }
        
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // 数据未就绪
            return false;  // 暂停，等待数据
        }
        
        // 真正的错误
        return true;  // 不暂停，返回错误
    }
    
    // ----------------------------------------
    // 2. 暂停时：注册到 epoll
    // ----------------------------------------
    void await_suspend(std::coroutine_handle<> handle) {
        // 注册 fd 到 epoll，保存协程 handle
        // 数据就绪时，Reactor 会调用 handle.resume()
        reactor->registerRead(fd, handle);
    }
    
    // ----------------------------------------
    // 3. 恢复时：再次读取并返回结果
    // ----------------------------------------
    ssize_t await_resume() {
        if (result >= 0) {
            // await_ready 时已读取成功
            return result;
        }
        
        // 被 epoll 唤醒，再次读取
        result = recv(fd, buffer, size, MSG_DONTWAIT);
        return result;
    }
};

// ============================================
// WriteAwaitable - 异步写入
// ============================================

struct WriteAwaitable {
    int fd;
    Reactor* reactor;
    const char* buffer;
    size_t size;
    ssize_t result;
    
    WriteAwaitable(int fd, Reactor* r, const char* buf, size_t sz)
        : fd(fd), reactor(r), buffer(buf), size(sz), result(0) {}
    
    bool await_ready() {
        // 尝试立即写入
        result = send(fd, buffer, size, MSG_DONTWAIT);
        
        if (result >= 0) {
            return true;  // 写入成功
        }
        
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return false;  // 发送缓冲区满，暂停
        }
        
        return true;  // 错误
    }
    
    void await_suspend(std::coroutine_handle<> handle) {
        // 注册写事件
        reactor->registerWrite(fd, handle);
    }
    
    ssize_t await_resume() {
        if (result >= 0) {
            return result;
        }
        
        // 再次写入
        result = send(fd, buffer, size, MSG_DONTWAIT);
        return result;
    }
};
```

---

## 🏗️ 协程版 Reactor

### Reactor 改造

```cpp
#include <coroutine>
#include <map>
#include <sys/epoll.h>

class Reactor {
private:
    int epfd;
    
    // fd → 协程 handle 的映射
    std::map<int, std::coroutine_handle<>> read_handles;
    std::map<int, std::coroutine_handle<>> write_handles;
    
public:
    Reactor() {
        epfd = epoll_create1(0);
    }
    
    ~Reactor() {
        close(epfd);
    }
    
    // ----------------------------------------
    // 注册读事件
    // ----------------------------------------
    void registerRead(int fd, std::coroutine_handle<> handle) {
        read_handles[fd] = handle;  // 保存协程 handle
        
        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = fd;
        
        // 添加或修改
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) < 0) {
            if (errno == EEXIST) {
                ev.events |= EPOLLOUT;  // 已存在，添加读事件
                epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
            }
        }
    }
    
    // ----------------------------------------
    // 注册写事件
    // ----------------------------------------
    void registerWrite(int fd, std::coroutine_handle<> handle) {
        write_handles[fd] = handle;
        
        struct epoll_event ev;
        ev.events = EPOLLOUT;
        ev.data.fd = fd;
        
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) < 0) {
            if (errno == EEXIST) {
                ev.events |= EPOLLIN;
                epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
            }
        }
    }
    
    // ----------------------------------------
    // 事件循环
    // ----------------------------------------
    void run() {
        struct epoll_event events[1024];
        
        while (true) {
            int n = epoll_wait(epfd, events, 1024, -1);
            
            for (int i = 0; i < n; i++) {
                int fd = events[i].data.fd;
                
                // 处理读事件
                if (events[i].events & EPOLLIN) {
                    auto it = read_handles.find(fd);
                    if (it != read_handles.end()) {
                        auto handle = it->second;
                        read_handles.erase(it);  // 移除
                        
                        // ⭐ 恢复协程
                        handle.resume();
                    }
                }
                
                // 处理写事件
                if (events[i].events & EPOLLOUT) {
                    auto it = write_handles.find(fd);
                    if (it != write_handles.end()) {
                        auto handle = it->second;
                        write_handles.erase(it);
                        
                        // ⭐ 恢复协程
                        handle.resume();
                    }
                }
            }
        }
    }
    
    // ----------------------------------------
    // 便捷方法：返回 Awaitable
    // ----------------------------------------
    ReadAwaitable asyncRead(int fd, char* buf, size_t size) {
        return ReadAwaitable{fd, this, buf, size};
    }
    
    WriteAwaitable asyncWrite(int fd, const char* buf, size_t size) {
        return WriteAwaitable{fd, this, buf, size};
    }
};
```

---

## 💻 协程版 handleClient

### 完整实现

```cpp
// ============================================
// 协程版客户端处理
// ============================================

Task<void> handleClient(int fd, Reactor* reactor) {
    char buffer[1024];
    
    while (true) {
        // ⭐ 异步读取（同步写法）
        ssize_t n = co_await reactor->asyncRead(fd, buffer, sizeof(buffer));
        
        if (n <= 0) {
            // 连接关闭或错误
            break;
        }
        
        // 业务逻辑（处理数据）
        // 这里简单回显
        
        // ⭐ 异步写入（同步写法）
        co_await reactor->asyncWrite(fd, buffer, n);
    }
    
    // 关闭连接
    close(fd);
    
    co_return;
}

// ============================================
// 主函数
// ============================================

Task<void> server(Reactor* reactor) {
    // 创建监听 socket
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    // bind, listen...
    
    while (true) {
        // 异步 accept（也可以协程化）
        int client_fd = accept(listen_fd, ...);
        
        if (client_fd >= 0) {
            // ⭐ 为每个客户端创建协程
            handleClient(client_fd, reactor);
            // 注意：协程会自动管理，不需要手动驱动
        }
    }
}

int main() {
    Reactor reactor;
    
    // 启动服务器协程
    server(&reactor);
    
    // 运行事件循环
    reactor.run();
    
    return 0;
}
```

---

## 📊 完整执行流程

### 协程 + Reactor 流程

```
1. handleClient(fd) 创建协程
   ↓
2. 执行 co_await asyncRead(fd)
   ↓
3. 创建 ReadAwaitable
   ↓
4. 调用 await_ready()
   ├─ 数据就绪 → 返回 true → 直接 await_resume()
   └─ 数据未就绪 → 返回 false → 暂停
        ↓
5. 调用 await_suspend(handle)
   ↓
6. reactor->registerRead(fd, handle)
   - 保存 handle 到 read_handles[fd]
   - 添加 fd 到 epoll
   ↓
7. 协程暂停，控制权回到 Reactor
   ↓
8. Reactor.run() 事件循环
   - epoll_wait 等待事件
   ↓
9. epoll 检测到 fd 可读
   ↓
10. Reactor 找到 read_handles[fd] = handle
    ↓
11. 调用 handle.resume()
    ↓
12. 协程恢复，调用 await_resume()
    - 再次 recv 读取数据
    - 返回读取结果
    ↓
13. 协程从 co_await 处继续执行
    - 处理数据
    - co_await asyncWrite(fd, result)
    ↓
14. 重复上述流程
```

---

## 🆚 回调 vs 协程对比

### 代码对比

#### 回调版

```cpp
class ReadHandler {
    int fd;
    char buffer[1024];
    
    void handleEvent() {
        // 读取
        asyncRead(fd, buffer, [this](ssize_t n) {
            if (n <= 0) {
                close(fd);
                return;
            }
            
            // 处理
            auto result = process(buffer, n);
            
            // 写入
            asyncWrite(fd, result, [this](ssize_t n) {
                if (n < 0) {
                    close(fd);
                    return;
                }
                
                // 继续读取（递归回调）
                handleEvent();
            });
        });
    }
};
```

**问题**：
- 嵌套 2 层
- 逻辑分散在多个回调中
- 状态（buffer）需要保存在类中
- 错误处理重复

---

#### 协程版

```cpp
Task<void> handleClient(int fd, Reactor* reactor) {
    char buffer[1024];
    
    while (true) {
        // 读取
        ssize_t n = co_await reactor->asyncRead(fd, buffer, sizeof(buffer));
        if (n <= 0) break;
        
        // 处理
        auto result = process(buffer, n);
        
        // 写入
        co_await reactor->asyncWrite(fd, result.data(), result.size());
    }
    
    close(fd);
}
```

**优势**：
- 无嵌套（线性）
- 逻辑集中在一个函数
- 状态（buffer）是局部变量
- 错误处理统一（try-catch）

---

### 详细对比

| 维度 | 回调版 | 协程版 |
|------|--------|--------|
| **代码行数** | 多（回调样板） | 少（线性） |
| **嵌套层数** | 深（2-5 层） | 无（0 层） |
| **逻辑组织** | 分散 | 集中 |
| **状态管理** | 类成员变量 | 局部变量 |
| **错误处理** | 每个回调处理 | 统一 try-catch |
| **可读性** | 差 | 好 |
| **可维护性** | 差 | 好 |
| **调试难度** | 高（跳转回调） | 低（线性执行） |
| **性能** | 高 | 高（相当） |
| **内存占用** | 低 | 低（相当） |

---

### 错误处理对比

**回调版**（错误处理分散）：
```cpp
asyncRead(fd, [](ssize_t n) {
    if (n < 0) {
        // 处理读错误
        handleError();
        return;
    }
    
    asyncWrite(fd, data, [](ssize_t n) {
        if (n < 0) {
            // 处理写错误（重复）
            handleError();
            return;
        }
    });
});
```

**协程版**（统一错误处理）：
```cpp
Task<void> handleClient(int fd) {
    try {
        auto data = co_await asyncRead(fd);
        co_await asyncWrite(fd, data);
    } catch (const std::exception& e) {
        // 统一处理所有错误
        handleError(e);
    }
}
```

---

## 🎯 协程化的优势总结

### 1. 代码可读性

**回调**：
```
读取 → 回调1 → 处理 → 回调2 → 写入 → 回调3 → ...
（跳来跳去，难以跟踪）
```

**协程**：
```
读取 → 处理 → 写入 → 读取 → ...
（线性执行，一目了然）
```

---

### 2. 逻辑集中

**回调**：
- 一个完整逻辑分散在多个回调
- 状态需要在回调间传递
- 难以理解整体流程

**协程**：
- 完整逻辑在一个函数
- 状态是局部变量
- 整体流程清晰

---

### 3. 错误处理

**回调**：
- 每个回调单独处理错误
- 代码重复
- 容易遗漏

**协程**：
- try-catch 统一处理
- 代码简洁
- 不易遗漏

---

### 4. 性能相当

**关键**：协程不牺牲性能
- 协程切换：~10-100ns
- 与 Reactor 回调相当
- 但代码可读性大幅提升

---

## 🏆 工业级实现

### 主流协程网络库

**1. asio（C++）**：
- Boost.Asio 支持协程
- co_await async_read/async_write
- 工业级标准

**2. libco（腾讯）**：
- C 语言协程库
- Hook 系统调用
- 微信后台使用

**3. cppcoro（Lewis Baker）**：
- C++20 协程库
- 现代 C++ 风格

**4. Folly（Facebook）**：
- C++ 协程支持
- 高性能

---

## 🎓 总结

### 核心思想

**协程 + Reactor = 同步写法 + 异步执行**

**关键**：
- I/O 操作 → Awaitable
- co_await 等待 I/O
- Reactor 恢复协程

---

### 实现要点

**1. ReadAwaitable**：
```cpp
await_ready()    // 尝试立即读取
await_suspend()  // 注册到 epoll
await_resume()   // 返回结果
```

**2. Reactor 改造**：
```cpp
registerRead(fd, handle)  // 保存协程 handle
run()                     // epoll 检测到事件，resume 协程
```

**3. 协程版 handleClient**：
```cpp
auto data = co_await asyncRead(fd);
co_await asyncWrite(fd, result);
```

---

### 优势

- ✅ 代码可读（线性逻辑）
- ✅ 逻辑集中（一个函数）
- ✅ 错误处理简单（try-catch）
- ✅ 性能相当（不牺牲性能）

---

> **协程 + Reactor 完美融合！** 🎉  
> 同步写法，异步执行，兼得两者之长！
