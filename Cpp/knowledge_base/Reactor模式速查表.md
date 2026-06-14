# Reactor 模式速查表

> **创建日期**：2026-06-22（第77天）  
> **用途**：快速查阅 Reactor 架构、快速选择合适的模型

---

## 📊 三种 Reactor 架构对比

### 架构图速查

#### 1. 单线程 Reactor

```
┌─────────────────────────────────┐
│    Reactor（主线程，1个）        │
│                                 │
│  while (true) {                 │
│    events = epoll_wait();       │
│    for (event in events) {      │
│      handler = getHandler(fd);  │
│      handler->handleEvent();    │
│    }                            │
│  }                              │
│                                 │
│  Handler::handleEvent() {       │
│    recv(fd, buf, ...);          │
│    processBusinessLogic(buf);   │
│    send(fd, result, ...);       │
│  }                              │
└─────────────────────────────────┘

特点：所有工作在单线程
适用：<100 连接，业务快（<1ms）
代表：Redis
```

---

#### 2. Reactor + 线程池

```
┌─────────────────────────────────┐
│    Reactor（主线程，1个）        │
│                                 │
│  while (true) {                 │
│    events = epoll_wait();       │
│    for (event in events) {      │
│      recv(fd, buf, ...);        │
│      pool.enqueue([buf, fd]() { │
│        process(buf);            │
│        send(fd, result, ...);   │
│      });                        │
│    }                            │
│  }                              │
└─────────────────────────────────┘
              ↓ 任务队列
┌─────────────────────────────────┐
│   工作线程池（M 个线程，如 4）   │
│                                 │
│  线程 1：[业务1][业务2]...       │
│  线程 2：[业务3][业务4]...       │
│  线程 3：[业务5][业务6]...       │
│  线程 4：[业务7][业务8]...       │
└─────────────────────────────────┘

特点：主线程 I/O，工作线程业务
适用：100-1000 连接，业务慢（>100ms）
```

---

#### 3. 主从 Reactor

```
┌─────────────────────────────────┐
│   主 Reactor（1 个线程）         │
│                                 │
│  while (true) {                 │
│    client_fd = accept(...);     │
│    subReactors[idx]             │
│      .addConnection(client_fd); │
│  }                              │
└─────────────────────────────────┘
              ↓ 轮询分配
┌─────────────────────────────────┐
│   从 Reactor 1（N 个线程之一）   │
│   管理：2500 个连接（假设 10000）│
│                                 │
│  while (true) {                 │
│    events = epoll_wait();       │
│    for (event in events) {      │
│      recv(fd, buf, ...);        │
│      pool.enqueue([buf, fd]() { │
│        process(buf);            │
│        send(fd, result, ...);   │
│      });                        │
│    }                            │
│  }                              │
└─────────────────────────────────┘

从 Reactor 2、3、4（同上）

              ↓ 任务队列
┌─────────────────────────────────┐
│   工作线程池（M 个线程，如 4）   │
└─────────────────────────────────┘

特点：多个 Reactor 并行 I/O
适用：>10000 连接，业务快（<10ms）
代表：Nginx、muduo、Netty
```

---

## 📋 详细对比表

| 维度 | 单线程 Reactor | Reactor + 线程池 | 主从 Reactor |
|------|---------------|-----------------|-------------|
| **线程数** | 1 | 1 + M | 1 + N + M |
| **I/O 线程** | 1 | 1 | 1 + N |
| **accept** | 主线程 | 主线程 | 主 Reactor（1 线程） |
| **recv** | 主线程 | 主线程 | 从 Reactor（N 线程） |
| **业务逻辑** | 主线程 | 工作线程（M） | 工作线程（M） |
| **send** | 主线程 | 工作线程 | 工作线程 |
| **I/O 瓶颈** | ✅ 存在 | ✅ 存在 | ❌ 不存在 |
| **业务瓶颈** | ✅ 存在 | ❌ 不存在 | ❌ 不存在 |
| **多核利用** | ❌ 差（0%） | ⚠️ 部分（50%） | ✅ 充分（100%） |
| **适用连接数** | <100 | 100-1000 | >10000 |
| **业务逻辑** | 必须快 | 可以慢 | 最好快 |
| **复杂度** | 低 | 中 | 高 |
| **代码行数** | ~400 | ~450 | ~600+ |
| **QPS**（示例） | 10 | 40 | 4000 |

**示例场景**：10000 连接，业务逻辑 1ms

---

## 🎯 快速选择决策树

```
┌─ 开始 ─┐
    ↓
连接数多少？
    ├─ <100
    │    └─→ 单线程 Reactor ✅
    │
    ├─ 100-1000
    │    ├─ 业务逻辑快（<10ms）？
    │    │    ├─ 是 → 单线程 Reactor ✅
    │    │    └─ 否 → Reactor + 线程池 ✅
    │    │
    │    └─ 业务逻辑慢（>100ms）？
    │         └─ 是 → Reactor + 线程池 ✅
    │
    └─ >10000
         ├─ 业务逻辑快（<10ms）？
         │    └─ 是 → 主从 Reactor ✅
         │
         └─ 业务逻辑慢（>100ms）？
              └─ 是 → 主从 Reactor + 线程池 ✅
```

**简化原则**：
- **连接数少** → 单线程
- **业务慢** → 加线程池
- **连接数多 + 业务快** → 主从 Reactor

---

## 💡 常见问题解答（FAQ）

### Q1：为什么需要 Reactor 模式？

**A**：
- **原始 epoll**：所有逻辑混在事件循环中
- **问题**：难以扩展、难以维护
- **Reactor**：职责分离（事件循环 vs 事件处理）
- **好处**：代码清晰、易于扩展、符合开闭原则

---

### Q2：什么时候需要线程池？

**A**：
- **问题**：业务逻辑慢（>100ms）会阻塞 Reactor
- **解决**：线程池处理业务逻辑
- **效果**：性能提升 N 倍（N = 线程数）

**例子**：
- 数据库查询（10-100ms）
- 文件操作（1-10ms）
- 复杂计算（>100ms）

---

### Q3：什么时候需要主从 Reactor？

**A**：
- **问题**：单 Reactor 主线程 I/O 成为瓶颈
- **条件**：连接数多（>10000）+ 业务快（<10ms）
- **解决**：多个 Reactor 并行处理 I/O
- **效果**：性能再提升 N 倍（N = 从 Reactor 数量）

**注意**：如果业务逻辑慢（>100ms），主从 Reactor 无优势

---

### Q4：Nginx 为什么不用线程池？

**A**：
- Nginx 的业务逻辑极快（<1ms）
- 只做转发、代理，不需要复杂计算
- 单线程事件循环足够
- 多进程（Worker）已充分利用多核

---

### Q5：Redis 为什么用单线程？

**A**：
- Redis 的业务逻辑极快（<1微秒）
- 内存操作，无阻塞
- 连接数不多（通常 <1000）
- 单线程简单、无锁、高效

---

### Q6：如何判断是 I/O 瓶颈还是业务瓶颈？

**A**：
- **方法**：计算各步骤耗时，找到 max
- **I/O 瓶颈**：recv/send 耗时 > 业务逻辑耗时
- **业务瓶颈**：业务逻辑耗时 > recv/send 耗时

**例子**：
```
场景1：recv 1ms，业务 100ms → 业务瓶颈 → 加线程池
场景2：recv 10ms，业务 1ms → I/O 瓶颈 → 主从 Reactor
```

---

### Q7：最优线程数如何选择？

**A**：
- **CPU 密集型**：线程数 = CPU 核数
- **I/O 密集型**：线程数 = CPU 核数 × 2
- **混合型**：根据实际测试调整

**从 Reactor 数量**：
- 推荐：CPU 核数
- 4 核 CPU：4 个从 Reactor

**工作线程池大小**：
- I/O 密集型：CPU 核数 × 2
- 4 核 CPU：8 个工作线程

---

### Q8：三种 Reactor 如何演进？

**A**：
```
单线程 Reactor
  ↓ 问题：业务逻辑慢会阻塞
  ↓ 解决：引入线程池
Reactor + 线程池
  ↓ 问题：主线程 I/O 成为瓶颈
  ↓ 解决：多个 Reactor 并行
主从 Reactor
  ↓ 完美
```

**每一步都解决上一步的瓶颈**

---

## 🏆 工业级实现参考

### Nginx

**模型**：多进程 + 主从 Reactor

**架构**：
- Master 进程：管理 Worker
- Worker 进程：每个独立运行
- Worker 内部：事件循环（单线程）
- Worker 数量：CPU 核数

**特点**：
- 业务逻辑极快（只转发）
- 不需要线程池

---

### muduo

**模型**：主从 Reactor + One Loop Per Thread

**架构**：
- 主 Reactor：accept
- 从 Reactor：每个线程一个
- 业务逻辑：在 Reactor 线程或线程池

**特点**：
- C++ 网络库
- 高性能、易用

---

### Redis

**模型**：单线程 Reactor

**架构**：
- 单线程事件循环
- 非阻塞 I/O
- 业务逻辑极快（内存操作）

**特点**：
- 简单、无锁
- 单线程足够

---

### Netty（Java）

**模型**：主从 Reactor

**架构**：
- Boss Group（主 Reactor）：accept
- Worker Group（从 Reactor）：I/O
- 业务线程池：业务逻辑

**特点**：
- Java NIO 框架
- 工业级标准

---

## 🔧 实现要点

### 单线程 Reactor

**关键代码**：
```cpp
class Handler {
    virtual void handleEvent() = 0;
    virtual int getFd() const = 0;
};

class Reactor {
    void run() {
        while (true) {
            int n = epoll_wait(epfd, events, ...);
            for (int i = 0; i < n; i++) {
                Handler* h = getHandler(events[i].data.fd);
                h->handleEvent();
            }
        }
    }
};
```

**要点**：
- Handler 抽象接口（多态）
- Reactor 只负责分发
- 所有工作在主线程

---

### Reactor + 线程池

**关键代码**：
```cpp
class ReadHandler : public Handler {
    void handleEvent() {
        // 主线程：recv
        recv(fd, buf, sizeof(buf), 0);
        
        // 交给线程池
        pool->enqueue([buf, fd]() {
            // 工作线程：业务逻辑
            process(buf);
            send(fd, result, ...);
        });
    }
};
```

**要点**：
- 主线程只负责 I/O（recv）
- 工作线程处理业务
- Lambda 捕获（值捕获，避免悬垂引用）

---

### 主从 Reactor

**关键代码**：
```cpp
class MainReactor {
    void run() {
        while (true) {
            int client_fd = accept(listen_fd, ...);
            // 轮询分配
            subReactors[nextIdx]->addConnection(client_fd);
            nextIdx = (nextIdx + 1) % N;
        }
    }
};

class SubReactor {
    void run() {
        while (true) {
            int n = epoll_wait(epfd, events, ...);
            for (int i = 0; i < n; i++) {
                recv(fd, buf, ...);
                pool->enqueue([buf, fd]() {
                    process(buf);
                    send(fd, result, ...);
                });
            }
        }
    }
};
```

**要点**：
- 主 Reactor 只负责 accept
- 从 Reactor 负责各自连接的 I/O
- 连接分配策略（轮询、最少连接、哈希）

---

## ⚠️ 常见陷阱

### 1. Lambda 捕获悬垂引用

**错误**：
```cpp
char buf[1024];
recv(fd, buf, sizeof(buf), 0);
pool->enqueue([&buf, fd]() {  // ❌ 引用捕获
    process(buf);  // buf 可能已被覆盖
});
```

**正确**：
```cpp
char buf[1024];
int len = recv(fd, buf, sizeof(buf), 0);
string data(buf, len);  // 拷贝到 string
pool->enqueue([data, fd]() {  // ✅ 值捕获
    process(data);
});
```

---

### 2. 忘记设置非阻塞（ET 模式）

**错误**：
```cpp
// ET 模式但 fd 是阻塞的
epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
recv(fd, buf, sizeof(buf), 0);  // ❌ 可能阻塞
```

**正确**：
```cpp
// 设置非阻塞
int flags = fcntl(fd, F_GETFL, 0);
fcntl(fd, F_SETFL, flags | O_NONBLOCK);

// 循环读取直到 EAGAIN
while (true) {
    int n = recv(fd, buf, sizeof(buf), 0);
    if (n < 0 && errno == EAGAIN) break;  // ✅
}
```

---

### 3. 线程池大小不合理

**问题**：
- 太小：无法充分利用多核
- 太大：上下文切换开销大

**推荐**：
```cpp
// I/O 密集型
int num_threads = std::thread::hardware_concurrency() * 2;

// CPU 密集型
int num_threads = std::thread::hardware_concurrency();
```

---

### 4. 主从 Reactor 负载不均

**问题**：
- 简单轮询可能导致某些 Reactor 过载

**改进**：
```cpp
// 最少连接策略
int minIdx = 0;
int minCount = subReactors[0]->getConnectionCount();
for (int i = 1; i < N; i++) {
    int count = subReactors[i]->getConnectionCount();
    if (count < minCount) {
        minCount = count;
        minIdx = i;
    }
}
subReactors[minIdx]->addConnection(client_fd);
```

---

## 📚 学习资源

**推荐阅读**：
- 《Unix 网络编程》卷1 - Reactor 模式
- 《Linux 高性能服务器编程》
- muduo 网络库源码
- Nginx 架构设计

**在线资源**：
- muduo GitHub：https://github.com/chenshuo/muduo
- Nginx 官方文档

---

> **Reactor 模式速查表完成！** 📋  
> 快速查阅架构，快速选择模型！  
> 收藏本文档，随时参考！💪
