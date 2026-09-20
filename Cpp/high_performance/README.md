# high_performance - 高性能 I/O 学习

## 项目状态

> 学习材料完成，性能验证未完成。当前示例用于理解 epoll 和事件驱动 I/O，不应直接称为生产级服务器。

## 平台与构建

| 项目 | 内容 |
|---|---|
| 平台 | Linux/WSL-only |
| 标准 | C++17 |
| 构建 | 当前为单文件 g++ 命令；统一 CMake 待补充 |
| 测试 | 暂无自动化行为测试和固定压测基线 |
| 限制 | fd 生命周期、部分写、优雅关闭和性能数据仍需工程化验证 |

## 学习路线

1. `01_epoll_basics.cpp`：epoll API 和 LT 模式
2. `02_epoll_et.cpp`：ET、非阻塞 I/O 和 EAGAIN
3. `03_epoll_threadpool.cpp`：线程池和 EPOLLONESHOT
4. 阅读 `lt_vs_et_comparison.md` 与 `epoll_best_practices.md`

## 性能数据说明

文档中的 QPS、倍数和连接规模属于理论模型或历史估算，除非明确列出硬件、内核、编译器、优化级别、测试工具和原始结果，不应当作为通用实测结论。

## 后续工作

- 建立 Linux/WSL CMake 构建。
- 增加固定回显客户端和并发测试。
- 记录 LT、ET 和线程池版本的可重复基线。
- 再评估零拷贝、HTTP 和连接超时。
