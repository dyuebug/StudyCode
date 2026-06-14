// ============================================
// 01_reactor_single_thread.cpp
// 日期：2026-06-19（第74天）
// 主题：Reactor 单线程模型
// 难度：⭐⭐⭐⭐ 高级
// 注意：需在 Linux/WSL 环境编译运行
// 编译：g++ -std=c++17 -o 01_reactor 01_reactor_single_thread.cpp
// 运行：./01_reactor
// ============================================

/*
【核心概念】Reactor 模式

是什么？
- Reactor（反应器）是一种事件驱动架构模式
- 用于处理并发 I/O 事件
- 核心思想：事件分离与分发

为什么需要？
- 原始 epoll：所有逻辑混在事件循环中
- 问题：难以扩展、难以维护、逻辑耦合
- Reactor：事件循环 + Handler 分离
- 优势：清晰、易扩展、易维护

核心特点？
- 三大组件：Reactor、Demultiplexer、Handler
- 职责分离：事件循环 vs 事件处理
- 开闭原则：易于添加新的 Handler
*/

#include <iostream>
#include <map>
#include <memory>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>

using namespace std;

const int PORT = 8090;
const int MAX_EVENTS = 1024;

// 前向声明
class Reactor;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

void showReactorConcept() {
    cout << R"(
============================================================
Reactor 模式核心概念 ⭐⭐⭐⭐⭐
============================================================

【什么是 Reactor？】

Reactor（反应器）= 事件驱动架构模式

核心思想：
  事件分离（Separation） + 事件分发（Dispatching）

【为什么需要 Reactor？】

原始 epoll 的问题：
  while (true) {
      events = epoll_wait(...);
      for (event in events) {
          if (fd == listen_fd) {
              // accept 逻辑（混在一起）
              int client_fd = accept(...);
              // ...
          } else {
              // recv 逻辑（混在一起）
              char buf[1024];
              int n = recv(...);
              // ...
          }
      }
  }

问题：
  ❌ 所有逻辑混在事件循环中
  ❌ 难以扩展（添加新事件类型要修改事件循环）
  ❌ 难以维护（逻辑耦合）
  ❌ 难以测试（无法独立测试各种事件处理）

Reactor 的解决方案：
  class Reactor {
      void run() {
          while (true) {
              events = epoll_wait(...);
              for (event in events) {
                  Handler* h = getHandler(event.fd);
                  h->handleEvent();  // 分发
              }
          }
      }
  };

  class AcceptHandler {  // 专门处理新连接
      void handleEvent() { /* accept 逻辑 */ }
  };

  class ReadHandler {    // 专门处理读事件
      void handleEvent() { /* recv 逻辑 */ }
  };

优势：
  ✅ 事件循环和事件处理分离
  ✅ 每种事件有独立的 Handler
  ✅ 易于扩展（添加新 Handler，不修改 Reactor）
  ✅ 易于维护（职责单一）
  ✅ 易于测试（Handler 可独立测试）

【Reactor 三大组件】

1. Reactor（反应器）
   - 职责：事件循环 + 事件分发
   - 核心方法：run() - 运行事件循环
   - 包含：Demultiplexer + Handler 映射表

2. Demultiplexer（事件分离器）
   - 职责：I/O 多路复用
   - 实现：epoll_wait / select / poll
   - 作用：监听多个 fd，返回就绪的事件

3. Handler（事件处理器）
   - 职责：处理具体的事件
   - 类型：AcceptHandler、ReadHandler、WriteHandler
   - 接口：handleEvent() - 纯虚函数

【Reactor 工作流程】

1. 应用启动
   ↓
2. 创建 Reactor
   ↓
3. 注册 Handler
   - AcceptHandler（listen_fd）
   - ReadHandler（client_fd）
   ↓
4. Reactor.run() 启动事件循环
   ↓
5. Demultiplexer.select()（epoll_wait）
   ↓ 返回就绪事件
6. Reactor 分发事件
   - 查找对应的 Handler
   - 调用 Handler.handleEvent()
   ↓
7. Handler 处理事件
   - AcceptHandler：accept + 注册新 ReadHandler
   - ReadHandler：recv + send
   ↓
8. 返回步骤 5（循环）

【Reactor 单线程模型】

架构：
  ┌────────────────────────────────────┐
  │      单线程 Reactor                 │
  │                                    │
  │  ┌──────────────────────────────┐ │
  │  │   Reactor（事件循环）         │ │
  │  │                              │ │
  │  │  while (true) {              │ │
  │  │    events = epoll_wait()     │ │
  │  │    dispatch(events)          │ │
  │  │  }                           │ │
  │  └──────────────────────────────┘ │
  │                                    │
  │  ┌──────────┐  ┌──────────┐      │
  │  │ Accept   │  │  Read    │      │
  │  │ Handler  │  │ Handler  │ ...  │
  │  └──────────┘  └──────────┘      │
  └────────────────────────────────────┘

特点：
  - 所有逻辑在单线程中执行
  - Reactor + 所有 Handler 都在同一个线程
  - 优点：简单，无竞态问题
  - 缺点：业务逻辑慢会阻塞整个 Reactor

============================================================
)" << endl;
}


// ============================================
// 第二部分：Handler 抽象接口设计 ⭐⭐⭐⭐⭐
// ============================================

// Handler 抽象接口（所有事件处理器的基类）
class Handler {
public:
    virtual ~Handler() = default;

    // 纯虚函数：处理事件（子类必须实现）
    virtual void handleEvent() = 0;

    // 获取文件描述符
    virtual int getFd() const = 0;
};

// ============================================
// 第三部分：Reactor 反应器实现 ⭐⭐⭐⭐⭐
// ============================================

class Reactor {
private:
    int epfd;                              // epoll 文件描述符
    map<int, shared_ptr<Handler>> handlers; // fd → Handler 映射表
    bool running;                          // 运行标志

public:
    Reactor() : running(false) {
        // 创建 epoll 实例
        epfd = epoll_create1(0);
        if (epfd < 0) {
            perror("epoll_create1");
            exit(1);
        }
        cout << "[Reactor] 创建成功（epfd=" << epfd << "）" << endl;
    }

    ~Reactor() {
        stop();
        close(epfd);
        cout << "[Reactor] 销毁" << endl;
    }

    // 注册 Handler
    void registerHandler(shared_ptr<Handler> handler, uint32_t events) {
        int fd = handler->getFd();

        struct epoll_event ev;
        ev.events = events;
        ev.data.fd = fd;

        if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) < 0) {
            perror("epoll_ctl ADD");
            return;
        }

        handlers[fd] = handler;
        cout << "[Reactor] 注册 Handler（fd=" << fd << "）" << endl;
    }

    // 移除 Handler
    void removeHandler(int fd) {
        if (handlers.find(fd) == handlers.end()) {
            return;
        }

        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
        handlers.erase(fd);
        cout << "[Reactor] 移除 Handler（fd=" << fd << "）" << endl;
    }

    // 运行事件循环
    void run() {
        running = true;
        cout << "[Reactor] 开始事件循环..." << endl;

        struct epoll_event events[MAX_EVENTS];

        while (running) {
            // ⭐ Demultiplexer（事件分离）
            int n = epoll_wait(epfd, events, MAX_EVENTS, -1);

            if (n < 0) {
                if (errno == EINTR) continue;  // 被信号中断，继续
                perror("epoll_wait");
                break;
            }

            // ⭐ 事件分发
            for (int i = 0; i < n; i++) {
                int fd = events[i].data.fd;

                // 查找对应的 Handler
                auto it = handlers.find(fd);
                if (it != handlers.end()) {
                    // 调用 Handler 处理事件
                    it->second->handleEvent();
                }
            }
        }

        cout << "[Reactor] 事件循环结束" << endl;
    }

    // 停止事件循环
    void stop() {
        running = false;
    }

    // 获取 Reactor 指针（供 Handler 使用）
    int getEpfd() const { return epfd; }
};

// ============================================
// 第四部分：AcceptHandler 实现 ⭐⭐⭐⭐⭐
// ============================================

class AcceptHandler : public Handler {
private:
    int listen_fd;
    Reactor* reactor;

public:
    AcceptHandler(int fd, Reactor* r) : listen_fd(fd), reactor(r) {}

    int getFd() const override {
        return listen_fd;
    }

    void handleEvent() override;  // 前向声明（需要 ReadHandler）
};

// ============================================
// 第五部分：ReadHandler 实现 ⭐⭐⭐⭐⭐
// ============================================

class ReadHandler : public Handler {
private:
    int client_fd;
    Reactor* reactor;

public:
    ReadHandler(int fd, Reactor* r) : client_fd(fd), reactor(r) {}

    ~ReadHandler() {
        close(client_fd);
    }

    int getFd() const override {
        return client_fd;
    }

    void handleEvent() override {
        char buf[1024];
        memset(buf, 0, sizeof(buf));

        int len = recv(client_fd, buf, sizeof(buf) - 1, 0);

        if (len > 0) {
            // 收到数据，回显
            buf[len] = '\0';
            cout << "[ReadHandler] fd=" << client_fd << " 收到: " << buf;

            send(client_fd, buf, len, 0);
        }
        else if (len == 0) {
            // 客户端关闭连接
            cout << "[ReadHandler] fd=" << client_fd << " 连接关闭" << endl;
            reactor->removeHandler(client_fd);
        }
        else {
            // 错误
            perror("recv");
            reactor->removeHandler(client_fd);
        }
    }
};

// AcceptHandler::handleEvent() 的实现（需要在 ReadHandler 定义之后）
void AcceptHandler::handleEvent() {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &addr_len);

    if (client_fd < 0) {
        perror("accept");
        return;
    }

    // 获取客户端地址
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));
    int port = ntohs(client_addr.sin_port);

    cout << "[AcceptHandler] 新连接 " << ip_str << ":" << port
         << " (fd=" << client_fd << ")" << endl;

    // ⭐ 创建 ReadHandler 并注册到 Reactor
    auto read_handler = make_shared<ReadHandler>(client_fd, reactor);
    reactor->registerHandler(read_handler, EPOLLIN);
}


// ============================================
// 第六部分：主程序 - Reactor 单线程服务器
// ============================================

int main() {
    cout << "=========================================================" << endl;
    cout << "Reactor 单线程模型" << endl;
    cout << "=========================================================" << endl;
    cout << endl;

    // 显示理论知识
    cout << "【理论学习】" << endl;
    showReactorConcept();
    cout << endl;

    cout << "=========================================================" << endl;
    cout << "【实战项目】Reactor 单线程服务器" << endl;
    cout << "=========================================================" << endl;
    cout << endl;

    // --------------------------------------------
    // Step 1: 创建监听 socket
    // --------------------------------------------
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return 1;
    }
    cout << "[1/4] socket 创建成功" << endl;

    // 设置地址复用
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 绑定地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(listen_fd);
        return 1;
    }
    cout << "[2/4] bind 成功，监听 0.0.0.0:" << PORT << endl;

    if (listen(listen_fd, 10) < 0) {
        perror("listen");
        close(listen_fd);
        return 1;
    }

    // --------------------------------------------
    // Step 2: 创建 Reactor
    // --------------------------------------------
    Reactor reactor;
    cout << "[3/4] Reactor 创建成功" << endl;

    // --------------------------------------------
    // Step 3: 创建并注册 AcceptHandler
    // --------------------------------------------
    auto accept_handler = make_shared<AcceptHandler>(listen_fd, &reactor);
    reactor.registerHandler(accept_handler, EPOLLIN);
    cout << "[4/4] AcceptHandler 注册成功" << endl;

    cout << "\n服务器启动成功！（Reactor 单线程模型）" << endl;
    cout << "提示：可用 telnet localhost " << PORT << " 或 nc localhost " << PORT << " 测试\n" << endl;

    // --------------------------------------------
    // Step 4: 运行 Reactor 事件循环
    // --------------------------------------------
    reactor.run();

    // 清理资源
    close(listen_fd);

    return 0;
}

/*
============================================================
🎓 学习总结
============================================================

【Reactor 模式核心】

三大组件：
  1. Reactor（反应器）：事件循环 + 事件分发
  2. Demultiplexer（事件分离器）：epoll_wait
  3. Handler（事件处理器）：AcceptHandler、ReadHandler

工作流程：
  注册 Handler → 事件循环 → 事件分离 → 事件分发 → Handler 处理

【Reactor vs 原始 epoll】

原始 epoll：
  - 所有逻辑混在事件循环中
  - 难以扩展和维护

Reactor：
  - 事件循环和事件处理分离
  - 每种事件有独立的 Handler
  - 易于扩展（添加新 Handler）

【关键设计】

Handler 抽象接口：
  class Handler {
      virtual void handleEvent() = 0;  // 纯虚函数
      virtual int getFd() const = 0;
  };

Reactor 核心方法：
  - registerHandler()：注册 Handler
  - removeHandler()：移除 Handler
  - run()：事件循环

Handler 实现：
  - AcceptHandler：accept + 创建 ReadHandler
  - ReadHandler：recv + send

【Reactor 单线程模型】

优点：
  ✅ 代码结构清晰（职责分离）
  ✅ 易于扩展（添加新 Handler）
  ✅ 易于测试（Handler 可独立测试）
  ✅ 无竞态问题（单线程）

缺点：
  ❌ 业务逻辑慢会阻塞整个 Reactor
  ❌ 无法充分利用多核

适用场景：
  ✅ I/O 密集型应用
  ✅ 业务逻辑简单且快
  ✅ 需要清晰的代码结构

【下一步学习】

- Reactor 多线程模型（解决单线程瓶颈）
- 主从 Reactor（Nginx 的架构）
- One Loop Per Thread（muduo 的架构）

============================================================
*/
