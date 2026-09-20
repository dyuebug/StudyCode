# reactor

## 定位

学习 Reactor 事件驱动架构，从单线程事件循环逐步演进到线程池和主从 Reactor。

## 内容范围

- 单线程 Reactor、Handler 和事件分发
- Reactor + 线程池
- 主从 Reactor、负载分配和架构对比
- 性能分析和工业实现参考

## 状态与限制

| 项目 | 内容 |
|---|---|
| 状态 | 学习材料完成，工程化中 |
| 平台 | Linux/WSL-only，依赖 epoll、Socket 和 pthread |
| 标准 | 主要为 C++17 |
| 构建 | 当前为单文件命令，统一 CMake 待补充 |
| 测试 | 暂无自动化行为测试和压测基线 |
| 限制 | fd 所有权、部分写、关闭流程、线程任务生命周期和性能数字仍需审查 |

## 阅读顺序

`01_reactor_single_thread.cpp` → `02_reactor_threadpool.cpp` → `master_slave_reactor.md` → `reactor_evolution_summary.md`。

文档中的 QPS 仅在明确给出假设时作为理论估算，不能直接作为跨平台性能结论。
