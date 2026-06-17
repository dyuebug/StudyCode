# 协程 vs 线程 vs 回调 - 深度对比

> **学习日期**：2026-06-23（第78天）  
> **主题**：协程与其他并发方案的详细对比

---

## 📊 实战对比：Web 服务器实现

### 场景设置

**需求**：
- Web 服务器
- 处理 HTTP 请求
- 支持 10000 个并发连接
- 每个请求：读取请求 → 查询数据库 → 返回响应

---

### 方案1：多线程

```cpp
void handleClient(int client_fd) {
    while (true) {
        // 阻塞读取
        char buf[1024];
        int n = recv(client_fd, buf, sizeof(buf), 0);
        if (n <= 0) break;
        
        // 解析请求（快，1ms）
        Request req = parseRequest(buf);
        
        // 查询数据库（慢，100ms）
        Data data = queryDatabase(req);  // ← 阻塞等待
        
        // 生成响应（快，1ms）
        string response = generateResponse(data);
        
        // 发送响应（快，1ms）
        send(client_fd, response.c_str(), response.size(), 0);
    }
}

// 主线程
void main() {
    int listen_fd = socket(...);
    bind(listen_fd, ...);
    listen(listen_fd, ...);
    
    while (true) {
        int client_fd = accept(listen_fd, ...);
        // 每个客户端一个线程
        std::thread(handleClient, client_fd).detach();
    }
}
```

**分析**：

**资源占用**：
- 10000 个连接 = 10000 个线程
- 内存：10000 × 1MB（栈）= 10GB
- 创建时间：10000 × 1ms = 10 秒

**性能**：
- 每个请求：1 + 100 + 1 + 1 = 103ms
- 吞吐量：10000 / 0.103 ≈ 97000 QPS

**问题**：
- ❌ 内存占用巨大（10GB）
- ❌ 创建开销大（10秒）
- ❌ 上下文切换频繁
- ❌ 线程数受限（通常 <5000）

---

### 方案2：Reactor + 回调

```cpp
class HttpHandler {
    void handleClient(int fd) {
        // 异步读取
        asyncRead(fd, [this, fd](char* buf, int n) {
            if (n <= 0) {
                close(fd);
                return;
            }
            
            // 解析请求
            Request req = parseRequest(buf);
            
            // 异步查询数据库
            asyncQueryDB(req, [this, fd](Data data) {
                // 生成响应
                string response = generateResponse(data);
                
                // 异步发送
                asyncWrite(fd, response, [this, fd](int n) {
                    // 继续下一个请求
                    handleClient(fd);
                });
            });
        });
    }
};

// Reactor 事件循环
void Reactor::run() {
    while (true) {
        int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
        for (int i = 0; i < n; i++) {
            Handler* h = getHandler(events[i].data.fd);
            h->handleEvent();  // 调用回调
        }
    }
}
```

**分析**：

**资源占用**：
- 10000 个连接 = 1 个线程（Reactor）+ 回调
- 内存：~10MB（每个连接 1KB 状态）

**性能**：
- 单线程处理所有 I/O
- 吞吐量：受单线程限制

**问题**：
- ❌ 回调地狱（嵌套 3 层）
- ❌ 逻辑分散（难以理解）
- ❌ 错误处理困难
- ❌ 状态管理复杂

---

### 方案3：协程 + Reactor

```cpp
Task<void> handleClient(int fd, Reactor* reactor) {
    while (true) {
        // 异步读取（同步写法）
        auto [buf, n] = co_await reactor->asyncRead(fd);
        if (n <= 0) {
            co_return;
        }
        
        // 解析请求
        Request req = parseRequest(buf);
        
        // 异步查询数据库（同步写法）
        Data data = co_await asyncQueryDB(req);
        
        // 生成响应
        string response = generateResponse(data);
        
        // 异步发送（同步写法）
        co_await reactor->asyncWrite(fd, response);
    }
}

// 主函数
Task<void> main() {
    Reactor reactor;
    int listen_fd = socket(...);
    bind(listen_fd, ...);
    listen(listen_fd, ...);
    
    while (true) {
        int client_fd = co_await reactor.asyncAccept(listen_fd);
        // 创建协程（轻量级）
        handleClient(client_fd, &reactor);
    }
}
```

**分析**：

**资源占用**：
- 10000 个连接 = 10000 个协程
- 内存：10000 × 0.5KB = 5MB
- 创建时间：10000 × 0.0001ms = 1ms

**性能**：
- 单线程处理所有协程
- 吞吐量：与 Reactor 相当

**优势**：
- ✅ 内存占用小（5MB）
- ✅ 创建快（1ms）
- ✅ 代码清晰（线性逻辑）
- ✅ 错误处理简单（try-catch）

---

## 📈 性能对比

### 内存占用

| 方案 | 10000 连接 | 100000 连接 |
|------|-----------|------------|
| 多线程 | 10GB | 100GB（不可行） |
| Reactor + 回调 | 10MB | 100MB |
| 协程 + Reactor | 5MB | 50MB |

**结论**：协程内存占用最小

---

### 延迟对比

| 操作 | 多线程 | 协程 | 差异 |
|------|--------|------|------|
| 创建 | 1ms | 0.1μs | 快 10000 倍 |
| 切换 | 1-10μs | 10-100ns | 快 100 倍 |
| 销毁 | 1ms | 0.1μs | 快 10000 倍 |

**结论**：协程延迟远低于线程

---

### 并发数量

| 方案 | 理论极限 | 实际极限 |
|------|---------|---------|
| 多线程 | 受线程数限制 | ~1000-5000 |
| Reactor + 回调 | 受 fd 限制 | ~100万 |
| 协程 + Reactor | 受内存限制 | ~100万 |

**结论**：协程和 Reactor 支持高并发

---

### 代码可读性

**多线程**（好）：
```cpp
// 线性逻辑，易读
auto data = queryDB(req);
send(fd, response);
```

**Reactor + 回调**（差）：
```cpp
// 嵌套回调，难读
asyncQueryDB(req, [](data) {
    asyncWrite(fd, response, [](n) {
        // ...
    });
});
```

**协程**（好）：
```cpp
// 线性逻辑，易读
auto data = co_await asyncQueryDB(req);
co_await asyncWrite(fd, response);
```

**结论**：协程 = 多线程的可读性 + Reactor 的性能

---

## 🎯 选择指南

### 决策树

```
并发数量？
  ├─ <1000
  │   └─ 业务逻辑简单？
  │       ├─ 是 → 多线程（简单）
  │       └─ 否 → 协程（高性能）
  │
  ├─ 1000-10000
  │   └─ 代码可读性重要？
  │       ├─ 是 → 协程（推荐）
  │       └─ 否 → Reactor + 回调
  │
  └─ >10000
      └─ 协程 + Reactor（唯一选择）
```

---

### 适用场景

**多线程**：
- ✅ 并发数少（<1000）
- ✅ CPU 密集型
- ✅ 需要简单实现
- ❌ 不适合高并发

**Reactor + 回调**：
- ✅ 并发数多（>10000）
- ✅ I/O 密集型
- ❌ 代码难以维护
- ❌ 回调地狱

**协程 + Reactor**（推荐）：
- ✅ 并发数多（>10000）
- ✅ I/O 密集型
- ✅ 代码易读易维护
- ✅ 兼得线程和回调的优点

---

## 💡 核心结论

**协程 = 多线程的编程模型 + Reactor 的性能**

**优势**：
- 高并发（100万协程）
- 低延迟（快100倍）
- 易读易维护（同步写法）
- 简单错误处理（try-catch）

**适用**：
- I/O 密集型应用
- 高并发场景
- 需要清晰代码

**不适用**：
- CPU 密集型（用线程池）
- 低并发简单场景（用多线程）

---

> **协程是现代高性能服务器的标准方案！** 🏆
