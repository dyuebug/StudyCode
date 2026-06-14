// ============================================
// 02_reactor_threadpool.cpp
// 日期：2026-06-20（第75天）
// 主题：Reactor 多线程模型（Reactor + 线程池）
// 难度：⭐⭐⭐⭐⭐ 高级
// 注意：需在 Linux/WSL 环境编译运行
// 编译：g++ -std=c++17 -pthread -o 02_reactor_threadpool 02_reactor_threadpool.cpp
// 运行：./02_reactor_threadpool
// ============================================

/*
【核心概念】Reactor 多线程模型

是什么？
- Reactor（主线程）+ 线程池（工作线程）
- 主线程负责 I/O 事件处理
- 工作线程负责业务逻辑处理

为什么需要？
- 单线程 Reactor 的瓶颈：业务逻辑慢会阻塞整个 Reactor
- 解决方案：职责再分离（I/O vs 业务）
- 充分利用多核 CPU

核心特点？
- 主线程：recv（快）
- 工作线程：processBusinessLogic（可以慢）
- 性能提升：4 线程 = 4 倍
*/

#include <iostream>
#include <map>
#include <memory>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>

using namespace std;

const int PORT = 8091;
const int MAX_EVENTS = 1024;
const int THREAD_POOL_SIZE = 4;  // 线程池大小

// 前向声明
class Reactor;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

void showMultiThreadReactorConcept() {
    cout << R"(
============================================================
Reactor 多线程模型核心概念 ⭐⭐⭐⭐⭐
============================================================

【单线程 Reactor 的瓶颈】

问题场景：
  class ReadHandler {
      void handleEvent() {
          recv(fd, buf, sizeof(buf), 0);

          // ❌ 业务逻辑（假设需要 100ms）
          processBusinessLogic();  // 阻塞整个 Reactor

          send(fd, result, len, 0);
      }
  };

问题：
  - 业务逻辑慢（数据库查询、文件操作、复杂计算）
  - 阻塞整个 Reactor 事件循环
  - 其他就绪的 fd 无法及时处理
  - 无法充分利用多核 CPU

影响：
  场景：1000 个连接，业务逻辑 100ms
  单线程：1000 × 100ms = 100 秒
  QPS = 10（非常低）

【Reactor + 线程池架构】

解决方案：职责再分离

架构：
  ┌────────────────────────────────────┐
  │   主线程（Reactor）                 │
  │                                    │
  │  Reactor 事件循环                   │
  │    ↓                               │
  │  Handler::handleEvent() {          │
  │    recv(fd, buf, ...);  ← I/O（快）│
  │                                    │
  │    // ✅ 交给线程池处理              │
  │    pool.enqueue([buf, fd]() {     │
  │      process(buf);                │
  │      send(fd, result, ...);       │
  │    });                            │
  │  }                                │
  └────────────────────────────────────┘
                ↓ 任务队列
  ┌────────────────────────────────────┐
  │   工作线程池（4 个线程）            │
  │                                    │
  │  线程1：处理业务逻辑 1              │
  │  线程2：处理业务逻辑 2              │
  │  线程3：处理业务逻辑 3              │
  │  线程4：空闲，等待任务              │
  └────────────────────────────────────┘

优势：
  ✅ Reactor 主线程不阻塞（只负责 I/O）
  ✅ 工作线程并发处理业务逻辑
  ✅ 充分利用多核 CPU

性能提升：
  场景：1000 个连接，业务逻辑 100ms，4 个工作线程
  多线程：1000 × 100ms / 4 = 25 秒
  QPS = 40
  性能提升 4 倍

【Reactor + 线程池 vs epoll + 线程池】

第1周的 epoll + 线程池（第72天）：
  while (true) {
      events = epoll_wait(...);
      for (event in events) {
          if (fd == listen_fd) {
              // 主线程处理 accept
          } else {
              // ❌ 整个事件处理交给线程池
              pool.enqueue([fd]() {
                  handleClient(fd);  // recv + 业务 + send
              });
          }
      }
  }

今天的 Reactor + 线程池：
  class ReadHandler {
      void handleEvent() {
          // ✅ 主线程只负责 recv
          recv(fd, buf, sizeof(buf), 0);

          // ✅ 只把业务逻辑交给线程池
          pool.enqueue([buf]() {
              processBusinessLogic(buf);
          });
      }
  };

核心区别：
  | 维度         | epoll + 线程池    | Reactor + 线程池 |
  |-------------|------------------|------------------|
  | 架构         | 原始 epoll       | Reactor 框架     |
  | 主线程       | epoll_wait + 分发| Reactor 事件循环 |
  | 线程池       | recv + 业务 + send| 只处理业务逻辑   |
  | recv         | 工作线程         | 主线程           |
  | send         | 工作线程         | 工作线程         |
  | 代码组织     | 混在一起         | Handler 分离     |
  | 扩展性       | 低               | 高               |

关键理解：
  - 第1周：整个事件处理交给线程池（粗粒度）
  - 今天：只把业务逻辑交给线程池（细粒度）
  - Reactor 提供了更清晰的架构

【职责划分】

主线程（Reactor）：
  - epoll_wait 监听事件
  - recv 接收数据
  - 创建任务，放入队列
  - 立即继续事件循环（不阻塞）

工作线程（线程池）：
  - 从队列取任务
  - 处理业务逻辑（可以慢）
  - send 发送结果

============================================================
)" << endl;
}


// ============================================
// 第二部分：简化版线程池 ⭐⭐⭐⭐⭐
// ============================================

class ThreadPool {
private:
    vector<thread> workers;
    queue<function<void()>> tasks;
    mutex queue_mutex;
    condition_variable cv;
    bool stop;

public:
    ThreadPool(int num_threads) : stop(false) {
        for (int i = 0; i < num_threads; i++) {
            workers.emplace_back([this]() {
                while (true) {
                    function<void()> task;

                    {
                        unique_lock<mutex> lock(queue_mutex);
                        cv.wait(lock, [this]() {
                            return stop || !tasks.empty();
                        });

                        if (stop && tasks.empty()) {
                            return;
                        }

                        task = move(tasks.front());
                        tasks.pop();
                    }

                    task();
                }
            });
        }
        cout << "[ThreadPool] 创建成功（" << num_threads << " 个工作线程）" << endl;
    }

    void enqueue(function<void()> task) {
        {
            unique_lock<mutex> lock(queue_mutex);
            tasks.push(move(task));
        }
        cv.notify_one();
    }

    ~ThreadPool() {
        {
            unique_lock<mutex> lock(queue_mutex);
            stop = true;
        }
        cv.notify_all();

        for (thread& worker : workers) {
            worker.join();
        }
        cout << "[ThreadPool] 销毁" << endl;
    }
};

// ============================================
// 第三部分：Handler 抽象接口
// ============================================

class Handler {
public:
    virtual ~Handler() = default;
    virtual void handleEvent() = 0;
    virtual int getFd() const = 0;
};

// ============================================
// 第四部分：Reactor 反应器
// ============================================

class Reactor {
private:
    int epfd;
    map<int, shared_ptr<Handler>> handlers;
    bool running;
    shared_ptr<ThreadPool> thread_pool;

public:
    Reactor(shared_ptr<ThreadPool> pool) : running(false), thread_pool(pool) {
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

    void removeHandler(int fd) {
        if (handlers.find(fd) == handlers.end()) {
            return;
        }

        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
        handlers.erase(fd);
        cout << "[Reactor] 移除 Handler（fd=" << fd << "）" << endl;
    }

    void run() {
        running = true;
        cout << "[Reactor] 开始事件循环..." << endl;

        struct epoll_event events[MAX_EVENTS];

        while (running) {
            int n = epoll_wait(epfd, events, MAX_EVENTS, -1);

            if (n < 0) {
                if (errno == EINTR) continue;
                perror("epoll_wait");
                break;
            }

            for (int i = 0; i < n; i++) {
                int fd = events[i].data.fd;

                auto it = handlers.find(fd);
                if (it != handlers.end()) {
                    it->second->handleEvent();
                }
            }
        }

        cout << "[Reactor] 事件循环结束" << endl;
    }

    void stop() {
        running = false;
    }

    shared_ptr<ThreadPool> getThreadPool() const {
        return thread_pool;
    }
};

// ============================================
// 第五部分：AcceptHandler 实现
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

    void handleEvent() override;
};

// ============================================
// 第六部分：ReadHandler 实现（多线程版）⭐⭐⭐⭐⭐
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
        // ⭐ 主线程：recv 接收数据
        char buf[1024];
        memset(buf, 0, sizeof(buf));

        int len = recv(client_fd, buf, sizeof(buf) - 1, 0);

        if (len > 0) {
            buf[len] = '\0';
            cout << "[ReadHandler 主线程] fd=" << client_fd << " 收到: " << buf;

            // ⭐⭐⭐ 关键：交给线程池处理业务逻辑
            // 捕获必要的变量
            int fd_copy = client_fd;
            string data(buf, len);

            reactor->getThreadPool()->enqueue([fd_copy, data]() {
                // ⭐ 工作线程：处理业务逻辑
                cout << "[工作线程 " << this_thread::get_id() << "] fd=" << fd_copy << " 开始处理" << endl;

                // 模拟业务逻辑（例如：数据库查询、复杂计算）
                // this_thread::sleep_for(chrono::milliseconds(100));

                // 简单的回显
                string result = data;

                // ⭐ 工作线程：send 发送结果
                send(fd_copy, result.c_str(), result.length(), 0);

                cout << "[工作线程 " << this_thread::get_id() << "] fd=" << fd_copy << " 处理完成" << endl;
            });
        }
        else if (len == 0) {
            // 客户端关闭连接
            cout << "[ReadHandler 主线程] fd=" << client_fd << " 连接关闭" << endl;
            reactor->removeHandler(client_fd);
        }
        else {
            perror("recv");
            reactor->removeHandler(client_fd);
        }
    }
};

// AcceptHandler::handleEvent() 的实现
void AcceptHandler::handleEvent() {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &addr_len);

    if (client_fd < 0) {
        perror("accept");
        return;
    }

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));
    int port = ntohs(client_addr.sin_port);

    cout << "[AcceptHandler] 新连接 " << ip_str << ":" << port
         << " (fd=" << client_fd << ")" << endl;

    auto read_handler = make_shared<ReadHandler>(client_fd, reactor);
    reactor->registerHandler(read_handler, EPOLLIN);
}


// ============================================
// 第七部分：主程序 - Reactor 多线程服务器
// ============================================

int main() {
    cout << "=========================================================" << endl;
    cout << "Reactor 多线程模型（Reactor + 线程池）" << endl;
    cout << "=========================================================" << endl;
    cout << endl;

    // 显示理论知识
    cout << "【理论学习】" << endl;
    showMultiThreadReactorConcept();
    cout << endl;

    cout << "=========================================================" << endl;
    cout << "【实战项目】Reactor + 线程池服务器" << endl;
    cout << "=========================================================" << endl;
    cout << endl;

    // --------------------------------------------
    // Step 1: 创建线程池
    // --------------------------------------------
    auto thread_pool = make_shared<ThreadPool>(THREAD_POOL_SIZE);
    cout << "[1/5] 线程池创建成功" << endl;

    // --------------------------------------------
    // Step 2: 创建监听 socket
    // --------------------------------------------
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return 1;
    }
    cout << "[2/5] socket 创建成功" << endl;

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

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
    cout << "[3/5] bind 成功，监听 0.0.0.0:" << PORT << endl;

    if (listen(listen_fd, 10) < 0) {
        perror("listen");
        close(listen_fd);
        return 1;
    }

    // --------------------------------------------
    // Step 3: 创建 Reactor
    // --------------------------------------------
    Reactor reactor(thread_pool);
    cout << "[4/5] Reactor 创建成功" << endl;

    // --------------------------------------------
    // Step 4: 创建并注册 AcceptHandler
    // --------------------------------------------
    auto accept_handler = make_shared<AcceptHandler>(listen_fd, &reactor);
    reactor.registerHandler(accept_handler, EPOLLIN);
    cout << "[5/5] AcceptHandler 注册成功" << endl;

    cout << "\n服务器启动成功！（Reactor + 线程池模型）" << endl;
    cout << "线程池大小：" << THREAD_POOL_SIZE << " 个工作线程" << endl;
    cout << "提示：可用 telnet localhost " << PORT << " 或 nc localhost " << PORT << " 测试\n" << endl;

    // --------------------------------------------
    // Step 5: 运行 Reactor 事件循环
    // --------------------------------------------
    reactor.run();

    close(listen_fd);

    return 0;
}

/*
============================================================
🎓 学习总结
============================================================

【Reactor 多线程模型】

架构：
  主线程（Reactor）：I/O 事件处理
  工作线程池：业务逻辑处理

职责划分：
  主线程：
    - epoll_wait 监听事件
    - recv 接收数据
    - 创建任务，放入队列
    - 立即继续事件循环（不阻塞）

  工作线程：
    - 从队列取任务
    - 处理业务逻辑（可以慢）
    - send 发送结果

【关键代码】

ReadHandler::handleEvent()：
  // ⭐ 主线程：recv
  recv(fd, buf, sizeof(buf), 0);

  // ⭐ 交给线程池处理
  pool->enqueue([fd, buf]() {
      // ⭐ 工作线程：业务逻辑
      processBusinessLogic(buf);

      // ⭐ 工作线程：send
      send(fd, result, len, 0);
  });

【Reactor + 线程池 vs epoll + 线程池】

核心区别：
  - epoll + 线程池：整个事件处理交给线程池（粗粒度）
  - Reactor + 线程池：只把业务逻辑交给线程池（细粒度）

优势：
  - Reactor 提供了更清晰的架构
  - Handler 职责分离
  - 易于扩展和维护

【性能提升】

单线程 Reactor：
  场景：1000 个连接，业务逻辑 100ms
  时间：1000 × 100ms = 100 秒
  QPS = 10

Reactor + 4 线程池：
  场景：1000 个连接，业务逻辑 100ms
  时间：1000 × 100ms / 4 = 25 秒
  QPS = 40
  性能提升：4 倍

【优点】

✅ Reactor 主线程不阻塞（只负责 I/O）
✅ 工作线程并发处理业务逻辑
✅ 充分利用多核 CPU
✅ 代码结构清晰（Reactor 框架）

【缺点】

❌ 复杂度增加（需要管理线程池）
❌ 线程安全问题（主线程和工作线程访问 fd）
❌ 任务队列开销

【适用场景】

✅ 业务逻辑复杂且慢
✅ 需要充分利用多核
✅ 并发连接多（>1000）

【下一步学习】

- 主从 Reactor 模式（明天）
- 多个 Reactor（One Loop Per Thread）
- Nginx、muduo 的架构

============================================================
*/
