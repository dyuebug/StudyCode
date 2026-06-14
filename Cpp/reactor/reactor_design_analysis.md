# Reactor 模式设计分析

> **学习日期**：2026-06-19（第74天）  
> **主题**：Reactor 单线程模型设计模式分析

---

## 📊 Reactor vs 原始 epoll 对比

### 代码结构对比

**原始 epoll**（第1周的代码）：
```cpp
while (true) {
    int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
    
    for (int i = 0; i < n; i++) {
        int fd = events[i].data.fd;
        
        if (fd == listen_fd) {
            // ❌ accept 逻辑混在这里
            int client_fd = accept(listen_fd, ...);
            setNonBlocking(client_fd);
            // ...
        } else {
            // ❌ recv 逻辑混在这里
            while (true) {
                char buf[1024];
                int n = recv(fd, buf, sizeof(buf), 0);
                // ...
            }
        }
    }
}
```

**Reactor 模式**（今天的代码）：
```cpp
// ✅ 事件循环（清晰）
class Reactor {
    void run() {
        while (true) {
            int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
            for (int i = 0; i < n; i++) {
                Handler* h = getHandler(events[i].data.fd);
                h->handleEvent();  // 分发
            }
        }
    }
};

// ✅ 事件处理（独立）
class AcceptHandler {
    void handleEvent() { /* accept 逻辑 */ }
};

class ReadHandler {
    void handleEvent() { /* recv 逻辑 */ }
};
```

---

### 详细对比表

| 维度 | 原始 epoll | Reactor 模式 |
|------|-----------|-------------|
| **代码组织** | 所有逻辑混在事件循环中 | 事件循环 + Handler 分离 |
| **职责划分** | 不清晰 | 清晰（Reactor 负责分发，Handler 负责处理） |
| **扩展性** | 低（添加新事件类型要修改事件循环） | 高（添加新 Handler，不修改 Reactor） |
| **可维护性** | 低（逻辑耦合） | 高（职责单一） |
| **可测试性** | 低（无法独立测试） | 高（Handler 可独立测试） |
| **代码行数** | 约 300 行（所有逻辑混在一起） | 约 350 行（结构清晰） |
| **复杂度** | 简单（直接） | 稍复杂（需要设计接口） |
| **性能** | 基准 | 略低（函数调用开销，可忽略） |
| **设计模式** | 无 | 符合开闭原则、单一职责原则 |

---

## 🎯 Reactor 的优缺点

### ✅ 优点

#### 1. 代码结构清晰（职责分离）

**原始 epoll**：
```cpp
// 所有逻辑混在一起，难以阅读
if (fd == listen_fd) {
    // 100 行 accept 逻辑
} else {
    // 100 行 recv 逻辑
}
```

**Reactor**：
```cpp
// 结构清晰，一目了然
class AcceptHandler { /* 50 行 */ };
class ReadHandler { /* 50 行 */ };
class Reactor { /* 100 行 */ };
```

---

#### 2. 易于扩展（开闭原则）

**原始 epoll**：
- 添加新事件类型 → 修改事件循环
- 违反开闭原则（对扩展开放，对修改关闭）

**Reactor**：
- 添加新事件类型 → 创建新 Handler
- 不需要修改 Reactor
- 符合开闭原则

**示例**：添加写事件处理
```cpp
// 原始 epoll：修改事件循环
if (fd == listen_fd) {
    // ...
} else if (events[i].events & EPOLLIN) {
    // ...
} else if (events[i].events & EPOLLOUT) {  // ❌ 修改原有代码
    // 写事件逻辑
}

// Reactor：添加新 Handler
class WriteHandler : public Handler {  // ✅ 不修改原有代码
    void handleEvent() { /* 写事件逻辑 */ }
};
```

---

#### 3. 易于维护（单一职责）

**每个 Handler 只负责一种事件**：
- AcceptHandler：只处理新连接
- ReadHandler：只处理读事件
- WriteHandler：只处理写事件

**好处**：
- 修改某种事件处理 → 只修改对应 Handler
- 不影响其他 Handler
- Bug 隔离，易于定位

---

#### 4. 易于测试（可测试性）

**原始 epoll**：
```cpp
// 无法独立测试 accept 逻辑
// 必须运行整个服务器
```

**Reactor**：
```cpp
// 可以独立测试 AcceptHandler
TEST(AcceptHandlerTest, HandleEvent) {
    MockReactor reactor;
    AcceptHandler handler(listen_fd, &reactor);
    handler.handleEvent();
    // 验证逻辑
}
```

---

### ❌ 缺点

#### 1. 复杂度增加

**需要设计接口**：
- Handler 抽象接口
- Reactor 和 Handler 的交互
- 需要理解设计模式

**学习成本**：
- 原始 epoll：200 行代码，易理解
- Reactor：350 行代码，需要理解架构

---

#### 2. 性能略有损失

**函数调用开销**：
```cpp
// 原始 epoll：直接调用
recv(fd, buf, sizeof(buf), 0);

// Reactor：多一层间接调用
handler->handleEvent() {
    recv(fd, buf, sizeof(buf), 0);
}
```

**实际影响**：
- 虚函数调用开销：约 5-10 纳秒
- 相比网络 I/O（毫秒级），可忽略不计

---

#### 3. 单线程模型的固有问题

**业务逻辑慢会阻塞**：
```cpp
class ReadHandler {
    void handleEvent() {
        // 业务逻辑（假设需要 100ms）
        processBusinessLogic();  // ← 阻塞整个 Reactor
    }
};
```

**解决方案**：
- Reactor 多线程模型（明天学习）
- 主从 Reactor（后天学习）

---

## 🎯 适用场景

### ✅ Reactor 单线程适合

#### 1. I/O 密集型应用

**特点**：
- 大量网络 I/O 操作
- 业务逻辑简单且快（微秒级）
- CPU 利用率低

**示例**：
- 简单的代理服务器
- 消息转发服务
- 协议网关

---

#### 2. 需要清晰代码结构的项目

**特点**：
- 长期维护的项目
- 多人协作开发
- 需要频繁添加新功能

**好处**：
- 代码易读易维护
- 职责清晰
- 易于 Code Review

---

#### 3. 原型开发和学习

**特点**：
- 快速验证想法
- 学习事件驱动架构
- 为多线程 Reactor 打基础

---

### ❌ 不适合的场景

#### 1. CPU 密集型应用

**问题**：
- 业务逻辑复杂（需要大量计算）
- 单线程无法利用多核
- 性能瓶颈在 CPU

**解决方案**：
- Reactor + 线程池
- Reactor 多线程模型

---

#### 2. 业务逻辑慢的应用

**问题**：
- 数据库查询（10-100ms）
- 文件操作（1-10ms）
- 复杂计算（>1ms）

**影响**：
- 阻塞整个 Reactor
- 其他连接无法及时处理

**解决方案**：
- 异步回调
- Reactor + 线程池

---

#### 3. 需要极致性能的场景

**问题**：
- 虚函数调用开销
- Handler 查找开销
- 不如原始 epoll 直接

**适用**：
- 超高性能要求（>100万 QPS）
- 对延迟极其敏感（<1微秒）

---

## 📊 性能分析

### 理论性能

**Reactor vs 原始 epoll**：

| 操作 | 原始 epoll | Reactor | 差异 |
|------|-----------|---------|------|
| **epoll_wait** | 1 次 | 1 次 | 相同 |
| **事件分发** | 直接 | 查找 Handler + 虚函数调用 | +10ns |
| **事件处理** | 直接 | 相同 | 相同 |

**结论**：
- 性能差异可忽略（+10ns vs 网络 I/O 1ms+）
- 代码质量提升显著

---

### 实际场景

**场景**：1000 个并发连接，每秒 10000 个请求

**原始 epoll**：
- QPS：10000
- 延迟：平均 100ms

**Reactor**：
- QPS：9990（-0.1%）
- 延迟：平均 100.01ms（+0.01%）

**结论**：性能差异在误差范围内

---

## 🎓 设计模式分析

### 符合的设计原则

#### 1. 单一职责原则（SRP）

**Reactor**：只负责事件循环和分发  
**Handler**：只负责具体事件处理

---

#### 2. 开闭原则（OCP）

**对扩展开放**：添加新 Handler  
**对修改关闭**：不修改 Reactor

---

#### 3. 依赖倒置原则（DIP）

**Reactor 依赖 Handler 抽象接口**：
```cpp
class Reactor {
    map<int, Handler*> handlers;  // 依赖抽象
};
```

---

### 设计模式

**Reactor 模式本身**：
- 事件驱动架构的经典模式
- GOF 设计模式之外的并发模式

**使用的其他模式**：
- 策略模式（Handler 是策略）
- 模板方法模式（Handler 接口）

---

## 🔜 下一步学习

**明天（第75天）**：Reactor 多线程模型
- 单 Reactor + 线程池
- 解决业务逻辑慢的问题

**后天（第76天）**：主从 Reactor
- 多个 Reactor
- Nginx 的架构

---

> **Reactor 模式是从"技术"到"架构"的跨越！** 🚀  
> 不只是用 epoll，而是设计一个可扩展的框架！
