# 阶段13学习计划：高性能服务器开发

> **学习周期**：4-6周（第69-95天）  
> **难度等级**：⭐⭐⭐⭐  
> **前置要求**：阶段12 Linux 系统编程（已完成✅）  
> **最终目标**：实现一个工业级高性能 HTTP 服务器

---

## 🎯 学习目标

**技术能力**：
- ✅ 掌握 epoll 高性能 I/O 机制
- ✅ 理解 Reactor 事件驱动模式
- ✅ 掌握零拷贝、内存池等优化技术
- ✅ 能够进行性能分析与调优
- ✅ 理解 Nginx/Redis 的核心架构

**实战成果**：
- ✅ 完成一个高性能 HTTP 服务器
- ✅ 性能达到 Nginx 50% 以上
- ✅ 掌握工业级服务器开发技能

---

## 📅 学习路线图

```
第1周：epoll 深入
  ├─ epoll 原理（红黑树 + 就绪列表）
  ├─ 边缘触发 vs 水平触发
  ├─ 非阻塞 I/O 实战
  └─ 项目：epoll 回显服务器

第2周：Reactor 模式
  ├─ 单 Reactor 单线程
  ├─ 单 Reactor 多线程
  ├─ 主从 Reactor 多线程
  └─ 项目：Reactor 框架实现

第3周：高级特性
  ├─ 零拷贝（sendfile、mmap）
  ├─ 内存池技术
  ├─ 定时器管理
  └─ 项目：HTTP 静态文件服务器

第4周：性能优化
  ├─ CPU 亲和性绑定
  ├─ 无锁编程（原子操作）
  ├─ 压测与性能分析
  └─ 项目：性能调优实战

第5-6周：综合项目
  ├─ 完整 HTTP 服务器
  ├─ epoll + Reactor + 线程池
  ├─ 零拷贝 + 内存池
  └─ 压测对比 Nginx
```

---

## 📚 第1周详细计划：epoll 深入

### 第70天：epoll 基础与原理

**学习目标**：
- 理解 epoll 为什么比 select 快
- 掌握 epoll 三个核心 API
- 理解 epoll 内部实现（红黑树 + 就绪列表）

**学习内容**：
1. **epoll vs select 对比**
   - select 的缺陷（轮询、1024限制、每次都要重设 fd_set）
   - epoll 的优势（事件通知、无上限、fd 状态由内核维护）
   - 性能对比实验

2. **epoll 三大 API**
   - `epoll_create1(0)`：创建 epoll 实例
   - `epoll_ctl()`：添加/修改/删除监听的 fd
   - `epoll_wait()`：等待事件就绪

3. **epoll 内部原理**
   - 红黑树管理所有监听的 fd
   - 就绪列表（ready list）存放已就绪的 fd
   - 事件通知机制（回调函数）

**实战项目**：
- 文件：`high_performance/01_epoll_basics.cpp`
- 功能：用 epoll 改写阶段12的 TCP 回显服务器
- 对比：epoll vs select 性能测试

**预计时长**：2.0 小时

---

### 第71天：边缘触发与水平触发

**学习目标**：
- 理解 ET（边缘触发）和 LT（水平触发）的区别
- 掌握非阻塞 I/O 编程
- 处理 EAGAIN 错误

**学习内容**：
1. **水平触发（LT）**
   - 默认模式
   - 只要 fd 可读/可写，epoll_wait 就会返回
   - 类似 select 行为

2. **边缘触发（ET）**
   - 高性能模式
   - 只在状态变化时通知一次
   - 必须配合非阻塞 I/O
   - 必须一次性读/写完所有数据（循环直到 EAGAIN）

3. **非阻塞 I/O**
   - fcntl 设置 O_NONBLOCK
   - 处理 EAGAIN/EWOULDBLOCK
   - 避免阻塞导致饿死其他连接

**实战项目**：
- 文件：`high_performance/02_epoll_et.cpp`
- 功能：ET 模式 + 非阻塞 I/O 的回显服务器
- 测试：大量并发连接（1000+）

**预计时长**：2.0 小时

---

### 第72天：epoll 实战与陷阱

**学习目标**：
- 掌握 epoll 的最佳实践
- 避免常见陷阱
- 实现一个完整的 epoll 服务器

**学习内容**：
1. **常见陷阱**
   - 忘记 epoll_ctl 添加新连接的 fd
   - ET 模式下没有循环读完所有数据
   - 忘记处理 EPOLLHUP/EPOLLERR
   - 监听 fd 使用 LT，连接 fd 使用 ET

2. **最佳实践**
   - EPOLLONESHOT：确保同一 fd 同一时刻只被一个线程处理
   - EPOLLRDHUP：对方关闭连接的优雅检测
   - 错误处理：区分 EAGAIN、EINTR、其他错误

3. **epoll + 线程池**
   - epoll 负责 I/O 事件监听
   - 线程池负责业务处理
   - 避免业务逻辑阻塞事件循环

**实战项目**：
- 文件：`high_performance/03_epoll_threadpool.cpp`
- 功能：epoll + 线程池的完整服务器
- 测试：wrk 压测，QPS 统计

**预计时长**：2.0 小时

---

### 第73天：epoll 性能测试与总结

**学习目标**：
- 对比 select、epoll LT、epoll ET 的性能
- 理解性能差异的本质原因
- 总结 epoll 最佳实践

**学习内容**：
1. **性能对比实验**
   - 同样的回显服务器，三种实现
   - 压测工具：wrk、ab
   - 指标：QPS、延迟、CPU 使用率

2. **瓶颈分析**
   - select：O(n) 轮询
   - epoll LT：减少系统调用，但仍有额外通知
   - epoll ET：最少的系统调用和通知

3. **epoll 速查表**
   - API 快速参考
   - 常见模式代码片段
   - 陷阱检查清单

**实战项目**：
- 文件：`knowledge_base/epoll速查表.md`
- 内容：epoll 完整知识体系总结

**预计时长**：2.0 小时

---

## 📚 第2周详细计划：Reactor 模式

### 第74天：Reactor 单线程模型

**学习目标**：
- 理解 Reactor 模式的核心思想
- 掌握事件循环（Event Loop）设计
- 实现一个单线程 Reactor

**学习内容**：
1. **Reactor 核心概念**
   - Reactor 反应堆模式
   - 事件驱动架构
   - 同步非阻塞 I/O

2. **单线程 Reactor 组件**
   - Reactor：事件循环，调用 epoll_wait
   - EventHandler：事件处理器接口
   - Acceptor：处理新连接
   - Connection：处理已连接的读写

3. **事件循环设计**
   ```cpp
   while (!stop) {
       events = epoll_wait();
       for (event : events) {
           handler = find_handler(event.fd);
           handler->handle_event(event);
       }
   }
   ```

**实战项目**：
- 文件：`high_performance/04_single_reactor.cpp`
- 功能：单线程 Reactor 回显服务器
- 优点：简单、无锁
- 缺点：无法利用多核

**预计时长**：2.0 小时

---

### 第75天：Reactor 多线程模型

**学习目标**：
- 掌握单 Reactor + 线程池模型
- 理解 I/O 线程和工作线程的职责划分
- 实现一个多线程 Reactor

**学习内容**：
1. **单 Reactor + 线程池**
   - 主线程（Reactor）：epoll 监听，分发事件
   - 工作线程（ThreadPool）：处理业务逻辑
   - 适合：计算密集型业务

2. **职责划分**
   - Reactor：只做 I/O（accept、read、write）
   - Worker：只做计算（解析请求、业务逻辑）
   - 避免：业务逻辑阻塞 Reactor

3. **线程间通信**
   - eventfd：线程间事件通知
   - 任务队列：Worker 完成后通知 Reactor

**实战项目**：
- 文件：`high_performance/05_reactor_threadpool.cpp`
- 功能：Reactor + 线程池的回显服务器
- 测试：模拟计算密集型业务（sha256 哈希）

**预计时长**：2.0 小时

---

### 第76天：主从 Reactor 模型

**学习目标**：
- 理解主从 Reactor（Netty/muduo 架构）
- 掌握多 Reactor 的负载均衡
- 实现一个主从 Reactor

**学习内容**：
1. **主从 Reactor 架构**
   - MainReactor：只负责 accept 新连接
   - SubReactor：每个线程一个，负责已连接的 I/O
   - 适合：高并发、I/O 密集型

2. **Reactor 线程池**
   - 预创建 N 个 SubReactor 线程（N = CPU 核心数）
   - 每个 SubReactor 有独立的 epoll
   - 新连接通过轮询分配给 SubReactor

3. **One Loop Per Thread**
   - 每个线程跑一个独立的事件循环
   - 减少锁竞争
   - 更好的 CPU 缓存局部性

**实战项目**：
- 文件：`high_performance/06_main_sub_reactor.cpp`
- 功能：主从 Reactor 架构服务器
- 测试：1 万并发连接

**预计时长**：2.5 小时

---

### 第77天：Reactor 总结与 muduo 源码阅读

**学习目标**：
- 对比三种 Reactor 模型的适用场景
- 阅读 muduo 源码，学习工业级实现
- 总结 Reactor 最佳实践

**学习内容**：
1. **三种模型对比**
   - 单线程：简单，适合低并发
   - 单 Reactor + 线程池：适合计算密集
   - 主从 Reactor：适合高并发 I/O 密集

2. **muduo 源码学习**
   - EventLoop 事件循环
   - Channel 事件分发
   - TcpServer/TcpConnection 封装
   - 定时器实现

3. **Reactor 速查表**
   - 设计模式对比
   - 常见实现方案
   - 性能优化建议

**实战项目**：
- 文件：`knowledge_base/Reactor模式速查表.md`
- 内容：Reactor 完整知识体系

**预计时长**：2.0 小时

---

## 📚 第3-4周：高级特性与性能优化

（内容包括：零拷贝、内存池、定时器、HTTP 协议、压测调优等）

**详细计划将在第2周结束前制定**

---

## 📚 第5-6周：综合项目

**最终项目**：高性能 HTTP 静态文件服务器

**功能要求**：
- epoll + 主从 Reactor 架构
- 零拷贝（sendfile）
- 内存池优化
- HTTP/1.1 协议（GET、HEAD、Range）
- 定时器（Keep-Alive 超时）
- 日志系统
- 配置文件（端口、线程数、根目录）

**性能目标**：
- QPS：达到 Nginx 的 50% 以上
- 并发：支持 1 万并发连接
- 延迟：P99 延迟 < 10ms

**压测对比**：
- 你的服务器 vs Nginx
- wrk 压测报告
- 性能分析与优化记录

---

## 📖 学习资源

**推荐书籍**：
- 《Linux 高性能服务器编程》- 游双（必读）
- 《Unix 网络编程》卷1 - Stevens（进阶）
- 《深入理解 Nginx》- 陶辉（工业级参考）

**开源项目**：
- muduo：陈硕的 C++ 网络库（必学）
- libevent：经典事件驱动库
- libev：高性能事件库

**在线资源**：
- muduo 源码解析（陈硕博客）
- Nginx 架构解析
- Redis 网络模型

---

## ✅ 学习建议

1. **理论与实践结合**：先理解原理，再写代码验证
2. **渐进式优化**：先跑通，再优化，对比每次优化的效果
3. **读源码**：muduo 的代码非常值得细读
4. **性能测试**：每个版本都要压测，数据说话
5. **写文档**：记录设计思路、性能数据、优化过程

---

## 🎯 成功标准

- ✅ 能独立实现 epoll + Reactor 架构的服务器
- ✅ 理解 Nginx/Redis 的核心架构
- ✅ 掌握性能分析与调优方法
- ✅ 最终项目的 QPS 达到 Nginx 50% 以上
- ✅ 能够清晰解释每个设计决策的原因

---

> **阶段13启航！** 🚀 这是一段充满挑战的旅程，但你将掌握工业级服务器开发的核心技能！
