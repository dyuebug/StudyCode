# C++ 学习资料总索引

> 状态基准：2026-09-19。实际构建和测试结果优先于历史统计。

## 学习主线

```text
C++ 基础
  → 面向对象与资源管理
  → STL 与现代 C++
  → Qt 桌面开发
  → 嵌入式系统概念
  → Linux 系统编程
  → epoll
  → Reactor
  → C++20 协程
  → HTTP 服务器工程化
```

## 模块索引

| 模块 | 入口 | 当前状态 | 平台/标准 | 主要限制 |
|---|---|---|---|---|
| 基础 C++ | `cpp_basics/README.md` | 学习完成，工程化中 | Windows；C++11/17/20 | 缺统一构建和自动测试 |
| 学生管理系统 | `student_management/README.md` | 项目完成，工程化中 | Windows/Linux；C++17 | 测试和构建需统一 |
| Qt 学习 | `qt_basics/README.md` | 学习完成，工程化中 | Qt；C++11+ | Qt 版本和构建矩阵待补 |
| TaskManager | `TaskManager/README.md` | 学习项目完成，工程化中 | Qt/qmake；C++11 | 缺单元测试和数据库隔离 |
| 嵌入式 | `embedded/README.md` | 主机模拟学习材料完成 | 主机 C++17 | 非真实固件工程 |
| Linux | `linux/README.md` | 学习完成，工程化中 | Linux/WSL；C++17 | 缺统一构建和行为测试 |
| epoll | `high_performance/README.md` | 学习完成，性能验证未完成 | Linux/WSL；C++17 | 缺真实压测和生命周期审查 |
| Reactor | `reactor/README.md` | 学习完成，工程化中 | Linux/WSL；C++17 | fd 和线程任务生命周期待验证 |
| 协程 | `coroutine/README.md` | 概念材料完成，工程化中 | C++20 | 缺生命周期、取消和超时测试 |
| 知识库 | `knowledge_base/README.md` | 索引材料完成，工程化中 | 与来源模块一致 | 历史快照和来源同步待补 |

## 阶段13资料

- 计划：`PHASE_13_PLAN.md`
- epoll：`high_performance/`
- Reactor：`reactor/`
- 协程：`coroutine/`
- 速查表：`epoll速查表.md`、`Reactor模式速查表.md`

阶段13目前完成了 epoll、Reactor 和协程学习材料，HTTP 服务器、零拷贝、内存池、定时器和真实压测仍属于后续工程化任务。

## 项目入口

- 学生管理系统：`student_management/README.md`
- Qt 任务管理器：`TaskManager/README.md`
- Qt 综合规划：`qt_basics/qt_capstone_project_plan.md`

## 历史资料

`学习历程总结_截至第54天.md` 是截至第54天的历史总结，不代表当前完整状态。文件名已带截止日期，保留其历史内容但不作为当前状态依据。

## 维护方式

新增模块或修改设计时，同步更新：

1. 模块 README；
2. 模块 DESIGN；
3. 本索引；
4. 对应学习日志或变更记录。
