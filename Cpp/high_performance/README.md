# high_performance - 高性能服务器开发

> **学习阶段**：阶段13 - 高性能服务器开发  
> **学习周期**：第70-72天（2026-06-15 至 2026-06-17）  
> **核心主题**：epoll 高性能 I/O 多路复用

---

## 📚 目录说明

本目录包含 epoll 高性能服务器开发的完整学习材料，从基础到工业级，循序渐进。

**学习路线**：
```
第70天：epoll 基础 → LT 模式
第71天：epoll 进阶 → ET 模式
第72天：epoll 实战 → 线程池 + EPOLLONESHOT
```

---

## 📁 文件列表

### 代码文件（3 个）

#### 1. `01_epoll_basics.cpp`（第70天）

**主题**：epoll 基础与原理 - LT 模式

**内容**：
- select vs epoll 对比
- epoll 内部原理（红黑树 + 就绪列表）
- epoll 三大 API（create/ctl/wait）
- LT 模式回显服务器实现

**关键知识点**：
- epoll 为什么比 select 快？O(1) vs O(n)
- 红黑树管理 fd，就绪列表返回就绪 fd
- LT 模式：只要就绪就通知（持续通知）

**编译运行**：
```bash
g++ -std=c++17 -o 01_epoll 01_epoll_basics.cpp
./01_epoll
# 测试：telnet localhost 8080
```

**代码行数**：约 300 行

---

#### 2. `02_epoll_et.cpp`（第71天）

**主题**：epoll 边缘触发（ET）模式 + 非阻塞 I/O

**内容**：
- LT vs ET 本质区别（水龙头 vs 门铃）
- ET 性能提升原理（减少系统调用 10 倍）
- ET 两大要求（非阻塞 I/O + 循环读取）
- EAGAIN 的正确理解
- ET 模式回显服务器实现

**关键知识点**：
- ET 模式：状态变化时通知一次
- 为什么必须非阻塞？避免最后一次 recv 阻塞
- EAGAIN = "数据读完了"，不是错误

**编译运行**：
```bash
g++ -std=c++17 -o 02_epoll_et 02_epoll_et.cpp
./02_epoll_et
# 测试：telnet localhost 8081
```

**代码行数**：约 350 行

---

#### 3. `03_epoll_threadpool.cpp`（第72天）

**主题**：epoll + 线程池 + EPOLLONESHOT

**内容**：
- 单线程 epoll 的瓶颈
- epoll + 线程池架构（职责分离）
- 竞态问题与 EPOLLONESHOT 解决方案
- 工业级 epoll 服务器实现

**关键知识点**：
- 主线程监听，工作线程处理业务
- EPOLLONESHOT：触发后自动移除，处理完手动注册
- 处理完必须 EPOLL_CTL_MOD 重新注册

**编译运行**：
```bash
g++ -std=c++17 -pthread -o 03_epoll_threadpool 03_epoll_threadpool.cpp
./03_epoll_threadpool
# 测试：telnet localhost 8082
```

**代码行数**：约 400 行

---

### 文档文件（4 个）

#### 1. `performance_test.md`

**内容**：epoll vs select 性能对比实验

**包含**：
- 实验设计
- 测试步骤
- 性能分析

---

#### 2. `lt_vs_et_comparison.md`

**内容**：LT vs ET 详细对比

**包含**：
- 理论对比表
- 工作流程对比
- 性能分析
- 适用场景

---

#### 3. `epoll_best_practices.md`

**内容**：epoll 最佳实践与常见陷阱

**包含**：
- 10+ 个常见陷阱
- 最佳实践建议
- 调试技巧
- 性能优化建议

---

#### 4. `epoll_knowledge_summary.md`

**内容**：epoll 完整知识体系总结

**包含**：
- epoll 知识地图
- 核心概念对比
- 三天学习脉络
- 本周成果总结

---

## 🎯 学习路线推荐

### 适合初学者

**Day 1**：学习 `01_epoll_basics.cpp`
- 理解 epoll 的基本原理
- 掌握 epoll 三大 API
- 完成 LT 模式服务器

**Day 2**：学习 `02_epoll_et.cpp`
- 理解 LT vs ET 的区别
- 掌握非阻塞 I/O 编程
- 完成 ET 模式服务器

**Day 3**：学习 `03_epoll_threadpool.cpp`
- 理解 epoll + 线程池架构
- 掌握 EPOLLONESHOT
- 完成工业级服务器

---

### 快速参考

**开发时**：查阅 `knowledge_base/epoll速查表.md`
- 快速查阅 API
- 查看常用模式
- 检查陷阱清单

**遇到问题**：查阅 `epoll_best_practices.md`
- 查找常见错误
- 查看解决方案
- 使用调试工具

---

## 📊 统计数据

**总代码行数**：约 1050 行  
**总文档行数**：约 1200 行  
**学习时长**：7.5 小时  
**完成文件数**：7 个

---

## 🏆 核心收获

通过本目录的学习，你将掌握：

### 基础层（第70天）
- ✅ epoll 的内部原理
- ✅ epoll 三大 API 的使用
- ✅ LT 模式的 epoll 服务器

### 进阶层（第71天）
- ✅ LT vs ET 的本质区别
- ✅ 非阻塞 I/O 编程
- ✅ ET 模式的 epoll 服务器

### 高级层（第72天）
- ✅ epoll + 线程池架构
- ✅ EPOLLONESHOT 防止竞态
- ✅ 工业级 epoll 服务器

---

## 🚀 性能对比

### select vs epoll

| 场景 | select | epoll LT | epoll ET |
|------|--------|----------|----------|
| 1000 连接 | 基准 | 10 倍 | 10 倍 |
| 10000 连接 | 基准 | 100 倍 | 100 倍 |

---

### 单线程 vs 多线程

| 场景 | 单线程 | 4 线程池 |
|------|--------|---------|
| I/O 密集 | 基准 | 1.5 倍 |
| CPU + I/O | 基准 | 4 倍 |

---

## 💡 常见问题

### Q1：什么时候用 LT，什么时候用 ET？

**A**：
- 开发阶段、<100 连接 → LT（简单）
- 生产环境、>1000 连接 → ET（高性能）

---

### Q2：为什么 ET 必须非阻塞？

**A**：
- ET 需要循环读取直到 EAGAIN
- 如果阻塞，最后一次 recv 会永久阻塞
- 非阻塞在没有数据时返回 EAGAIN

---

### Q3：EPOLLONESHOT 什么时候用？

**A**：
- 多线程 + epoll 必须用
- 防止同一 fd 被多个线程同时处理
- 单线程不需要

---

### Q4：处理完为什么要重新注册？

**A**：
- EPOLLONESHOT 触发后自动移除 fd
- 如果不重新注册，fd 永远不会再触发
- 用 EPOLL_CTL_MOD 重新注册

---

## 🔗 相关资源

**速查表**：
- `knowledge_base/epoll速查表.md` - 快速参考

**学习日志**：
- `daily_logs/2026-06-15_第70天_日志.md`
- `daily_logs/2026-06-16_第71天_日志.md`
- `daily_logs/2026-06-17_第72天_日志.md`

**推荐阅读**：
- 《Unix 网络编程》卷1 - I/O 多路复用
- 《Linux 高性能服务器编程》- epoll 章节

---

## 🎓 下一步学习

**第2周：Reactor 模式**
- Reactor 单线程/多线程模型
- 事件循环与事件分发
- 完整 Reactor 框架实现

---

> **恭喜你完成 epoll 的学习！** 🎉  
> 你已经掌握了 Nginx、Redis 高性能的核心秘密！  
> 下一步：学习 Reactor 事件驱动架构！💪
