// ============================================
// 03_epoll_threadpool.cpp
// 日期：2026-06-17（第72天）
// 主题：epoll + 线程池 + EPOLLONESHOT
// 难度：⭐⭐⭐⭐⭐ 高级
// 注意：需在 Linux/WSL 环境编译运行
// 编译：g++ -std=c++17 -pthread -o 03_epoll_threadpool 03_epoll_threadpool.cpp
// 运行：./03_epoll_threadpool
// ============================================

/*
【核心概念】epoll + 线程池架构

是什么？
- epoll 负责 I/O 事件监听（快）
- 线程池负责业务逻辑处理（可以慢）
- 职责分离，提升并发能力

为什么需要？
- 单线程 epoll 的问题：业务逻辑慢会阻塞 epoll_wait
- 解决方案：主线程只监听，工作线程处理业务
- 这是 Nginx、Redis 等高性能服务器的标准架构

核心特点？
- EPOLLONESHOT：防止同一 fd 被多个线程同时处理
- 任务队列：连接主线程和工作线程
- EPOLL_CTL_MOD：处理完成后重新注册
*/

#include <iostream>
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

const int PORT = 8082;
const int MAX_EVENTS = 1024;
const int THREAD_POOL_SIZE = 4;  // 线程池大小


// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

void showArchitecture() {
    cout << R"(
============================================================
epoll + 线程池架构 ⭐⭐⭐⭐⭐
============================================================

【单线程 epoll 的瓶颈】

问题场景：
  主线程（epoll 事件循环）：
    epoll_wait() → 返回就绪的 fd
    ↓
    recv 接收数据
    ↓
    处理业务逻辑（可能很慢）← 阻塞在这里
      - 数据库查询（100ms）
      - 文件操作（50ms）
      - 复杂计算（200ms）
    ↓
    send 发送结果
    ↓
    继续 epoll_wait()

瓶颈：
  - 如果业务逻辑慢（300ms），其他就绪的 fd 无法及时处理
  - 即使有 1000 个连接，同一时刻只能处理 1 个
  - CPU 多核利用率低（单线程只用 1 个核）

【epoll + 线程池的解决方案】

架构设计：
  主线程（只负责监听）：
    epoll_wait() → 返回就绪的 fd
    ↓
    将任务放入队列 ← 很快（微秒级）
    ↓
    立即继续 epoll_wait() ← 不阻塞

  工作线程池（负责处理）：
    从队列取任务
    ↓
    处理业务逻辑 ← 可以慢（毫秒级）
    ↓
    完成后通知主线程

优势：
  - 主线程专注监听，响应快
  - 工作线程并发处理，吞吐量高
  - 充分利用多核 CPU

【职责划分】

┌─────────────────────────────────────────┐
│          主线程（epoll 事件循环）          │
│  职责：I/O 事件监听                      │
│  特点：轻量、快速、不能阻塞              │
│                                         │
│  epoll_wait()                           │
│    ↓                                    │
│  接收到 fd 就绪                          │
│    ↓                                    │
│  创建任务，放入队列                      │
│    ↓                                    │
│  立即继续 epoll_wait()                   │
└─────────────────────────────────────────┘
              ↓ 任务队列
┌─────────────────────────────────────────┐
│      工作线程池（4个线程）                │
│  职责：业务逻辑处理                      │
│  特点：可以慢、可以阻塞                  │
│                                         │
│  线程1：处理 fd=5                        │
│  线程2：处理 fd=8                        │
│  线程3：处理 fd=12                       │
│  线程4：空闲，等待任务                   │
└─────────────────────────────────────────┘

【性能对比】

场景：1000 个连接，每个请求需要 100ms 处理时间

单线程 epoll：
  - 同时只能处理 1 个请求
  - 1000 个请求需要：1000 × 100ms = 100秒
  - QPS = 10

epoll + 4 线程池：
  - 同时可以处理 4 个请求
  - 1000 个请求需要：1000 × 100ms / 4 = 25秒
  - QPS = 40

性能提升：4 倍（线程数决定）

============================================================
)" << endl;
}


void showRaceCondition() {
    cout << R"(
============================================================
竞态问题与 EPOLLONESHOT ⭐⭐⭐⭐⭐
============================================================

【问题场景：同一 fd 被多个线程同时处理】

时间线：
  时刻1：客户端发送数据1到 fd=5
  时刻2：epoll_wait 返回 fd=5（数据1就绪）
  时刻3：主线程将 fd=5 的任务放入队列
  时刻4：工作线程 A 从队列取出 fd=5，开始处理数据1
  时刻5：客户端又发送数据2到 fd=5
  时刻6：epoll_wait 又返回 fd=5（数据2就绪）← 问题！
  时刻7：主线程又将 fd=5 的任务放入队列
  时刻8：工作线程 B 从队列取出 fd=5，开始处理数据2

结果：
  - 线程 A 和线程 B 同时处理 fd=5
  - recv 可能读到对方的数据
  - send 可能互相覆盖
  - 数据乱序、程序崩溃

【EPOLLONESHOT 的解决方案】

含义：
  - One Shot = 一次性触发
  - fd 就绪后，epoll_wait 返回一次
  - 之后 epoll 自动将该 fd 移除
  - 直到你用 EPOLL_CTL_MOD 重新注册

工作流程（使用 EPOLLONESHOT）：
  时刻1：客户端发送数据1到 fd=5
  时刻2：epoll_wait 返回 fd=5
  时刻3：epoll 自动移除 fd=5（不再监听）← 关键
  时刻4：主线程将 fd=5 的任务放入队列
  时刻5：工作线程 A 从队列取出 fd=5，开始处理
  时刻6：客户端又发送数据2到 fd=5
  时刻7：epoll_wait 不会返回 fd=5（已移除）← 避免竞态
  时刻8：工作线程 A 处理完成
  时刻9：工作线程 A 用 EPOLL_CTL_MOD 重新注册 fd=5
  时刻10：epoll_wait 返回 fd=5（数据2就绪）
  时刻11：重复流程

保证：
  - 同一时刻，只有一个线程处理一个 fd
  - 避免竞态条件
  - 数据顺序正确

【代码对比】

不使用 EPOLLONESHOT（有竞态）：
  // 注册 fd
  ev.events = EPOLLIN | EPOLLET;  // 没有 EPOLLONESHOT
  epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);

  // 可能被多次触发，导致竞态

使用 EPOLLONESHOT（无竞态）：
  // 注册 fd
  ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;  // 一次性触发
  epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);

  // 工作线程处理完成后，重新注册
  ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
  epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);  // 重新监听

【关键理解】⭐⭐⭐⭐⭐

EPOLLONESHOT 的本质：
  - 触发后自动"下线"
  - 处理完成后手动"上线"
  - 保证同一时刻只有一个线程持有 fd

类比：
  - 像借书：借出后从书架移除，还回后重新上架
  - 像排队：被叫到号的人离开队列，办完事再重新排队

注意事项：
  1. 处理完成后必须 EPOLL_CTL_MOD 重新注册
  2. 如果忘记重新注册，fd 永远不会再触发
  3. 如果连接关闭，不需要重新注册（直接 DEL）

============================================================
)" << endl;
}


// ============================================
// 第二部分：简化版线程池实现 ⭐⭐⭐⭐⭐
// ============================================

// 简化版线程池（参考阶段12的实现）
class ThreadPool {
private:
    vector<thread> workers;           // 工作线程
    queue<function<void()>> tasks;    // 任务队列
    mutex queue_mutex;                // 队列互斥锁
    condition_variable cv;            // 条件变量
    bool stop;                        // 停止标志

public:
    // 构造函数：创建指定数量的工作线程
    ThreadPool(int num_threads) : stop(false) {
        for (int i = 0; i < num_threads; i++) {
            workers.emplace_back([this]() {
                // 工作线程的工作循环
                while (true) {
                    function<void()> task;

                    {
                        unique_lock<mutex> lock(queue_mutex);

                        // 等待任务或停止信号
                        cv.wait(lock, [this]() {
                            return stop || !tasks.empty();
                        });

                        if (stop && tasks.empty()) {
                            return;  // 线程退出
                        }

                        // 取出任务
                        task = move(tasks.front());
                        tasks.pop();
                    }

                    // 执行任务（在锁外执行）
                    task();
                }
            });
        }
    }

    // 添加任务到队列
    void enqueue(function<void()> task) {
        {
            unique_lock<mutex> lock(queue_mutex);
            tasks.push(move(task));
        }
        cv.notify_one();  // 通知一个工作线程
    }

    // 析构函数：停止所有线程
    ~ThreadPool() {
        {
            unique_lock<mutex> lock(queue_mutex);
            stop = true;
        }
        cv.notify_all();  // 唤醒所有线程

        for (thread& worker : workers) {
            worker.join();
        }
    }
};


// ============================================
// 第三部分：辅助函数
// ============================================

// 设置 fd 为非阻塞模式
void setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        return;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL");
    }
}

// 工作线程的处理函数
// 参数：fd（客户端连接）、epfd（epoll 实例）
void handleClient(int fd, int epfd) {
    cout << "[工作线程 " << this_thread::get_id() << "] 开始处理 fd=" << fd << endl;

    // ⭐ ET 模式：必须循环读取直到 EAGAIN
    bool connection_closed = false;
    bool error_occurred = false;

    while (true) {
        char buf[1024];
        memset(buf, 0, sizeof(buf));

        int len = recv(fd, buf, sizeof(buf) - 1, 0);

        if (len > 0) {
            // 收到数据
            buf[len] = '\0';
            cout << "[工作线程 " << this_thread::get_id() << "] fd=" << fd << " 收到: " << buf;

            // 模拟业务逻辑处理（可以很慢）
            // this_thread::sleep_for(chrono::milliseconds(100));

            // 回显数据
            int sent = 0;
            while (sent < len) {
                int n = send(fd, buf + sent, len - sent, 0);
                if (n > 0) {
                    sent += n;
                } else if (n < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        // 发送缓冲区满，等待下次
                        break;
                    } else {
                        perror("send");
                        error_occurred = true;
                        break;
                    }
                }
            }
        }
        else if (len == 0) {
            // 客户端关闭连接
            cout << "[工作线程 " << this_thread::get_id() << "] fd=" << fd << " 连接关闭" << endl;
            connection_closed = true;
            break;
        }
        else {
            // len < 0
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // ⭐ 数据读完了，正常退出
                break;
            } else {
                perror("recv");
                error_occurred = true;
                break;
            }
        }
    }

    // 处理连接关闭或错误
    if (connection_closed || error_occurred) {
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
        close(fd);
        cout << "[工作线程 " << this_thread::get_id() << "] fd=" << fd << " 已关闭" << endl;
    } else {
        // ⭐⭐⭐ 关键：处理完成后，重新注册 fd（EPOLLONESHOT）
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
        ev.data.fd = fd;
        if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) < 0) {
            perror("epoll_ctl MOD");
        } else {
            cout << "[工作线程 " << this_thread::get_id() << "] fd=" << fd << " 重新注册成功" << endl;
        }
    }
}


// ============================================
// 第四部分：主程序 - epoll + 线程池服务器
// ============================================

int main() {
    cout << "=========================================================" << endl;
    cout << "epoll + 线程池 + EPOLLONESHOT" << endl;
    cout << "=========================================================" << endl;
    cout << endl;

    // 显示理论知识
    cout << "【理论学习】" << endl;
    showArchitecture();
    cout << endl;
    showRaceCondition();
    cout << endl;

    cout << "=========================================================" << endl;
    cout << "【实战项目】epoll + 线程池服务器" << endl;
    cout << "=========================================================" << endl;
    cout << endl;

    // --------------------------------------------
    // Step 1: 创建线程池
    // --------------------------------------------
    ThreadPool pool(THREAD_POOL_SIZE);
    cout << "[1/6] 线程池创建成功（" << THREAD_POOL_SIZE << " 个工作线程）" << endl;

    // --------------------------------------------
    // Step 2: 创建监听 socket
    // --------------------------------------------
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return 1;
    }
    cout << "[2/6] socket 创建成功" << endl;

    // 设置地址复用
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 设置非阻塞
    setNonBlocking(listen_fd);
    cout << "[3/6] listen_fd 设置为非阻塞模式" << endl;

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
    cout << "[4/6] bind 成功，监听 0.0.0.0:" << PORT << endl;

    if (listen(listen_fd, 10) < 0) {
        perror("listen");
        close(listen_fd);
        return 1;
    }

    // --------------------------------------------
    // Step 3: 创建 epoll 实例
    // --------------------------------------------
    int epfd = epoll_create1(0);
    if (epfd < 0) {
        perror("epoll_create1");
        close(listen_fd);
        return 1;
    }
    cout << "[5/6] epoll 实例创建成功（epfd=" << epfd << "）" << endl;

    // 将监听 socket 加入 epoll（不需要 EPOLLONESHOT）
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;  // listen_fd 不需要 EPOLLONESHOT
    ev.data.fd = listen_fd;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev) < 0) {
        perror("epoll_ctl ADD listen_fd");
        close(listen_fd);
        close(epfd);
        return 1;
    }
    cout << "[6/6] 监听 socket 已加入 epoll" << endl;
    cout << "\n服务器启动成功！（epoll + 线程池 + EPOLLONESHOT）" << endl;
    cout << "提示：可用 telnet localhost " << PORT << " 或 nc localhost " << PORT << " 测试\n" << endl;

    // --------------------------------------------
    // Step 4: 主线程 epoll 事件循环
    // --------------------------------------------
    struct epoll_event events[MAX_EVENTS];
    int client_count = 0;

    while (true) {
        int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (n < 0) {
            perror("epoll_wait");
            break;
        }

        // 处理所有就绪的事件
        for (int i = 0; i < n; i++) {
            int fd = events[i].data.fd;

            // --------------------------------------------
            // 情况1：监听 socket 就绪 → 有新连接
            // --------------------------------------------
            if (fd == listen_fd) {
                // accept 也要循环（ET 模式）
                while (true) {
                    struct sockaddr_in client_addr;
                    socklen_t addr_len = sizeof(client_addr);
                    int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &addr_len);

                    if (client_fd < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;  // 所有新连接都处理完了
                        } else {
                            perror("accept");
                            break;
                        }
                    }

                    // 获取客户端地址
                    char ip_str[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));
                    int port = ntohs(client_addr.sin_port);

                    cout << "[主线程] 新连接 " << ip_str << ":" << port << " (fd=" << client_fd << ")" << endl;

                    // 设置非阻塞
                    setNonBlocking(client_fd);

                    // ⭐⭐⭐ 关键：加入 epoll，使用 EPOLLONESHOT
                    ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
                    ev.data.fd = client_fd;
                    if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev) < 0) {
                        perror("epoll_ctl ADD client_fd");
                        close(client_fd);
                        continue;
                    }

                    client_count++;
                    cout << "[主线程] 当前在线：" << client_count << " 人" << endl;
                }
            }
            // --------------------------------------------
            // 情况2：客户端 socket 就绪 → 有数据到达
            // --------------------------------------------
            else {
                // ⭐⭐⭐ 关键：将任务交给线程池处理（不在主线程处理）
                cout << "[主线程] fd=" << fd << " 有数据，交给线程池处理" << endl;

                // 注意：需要捕获 epfd 的值
                int epfd_copy = epfd;
                pool.enqueue([fd, epfd_copy]() {
                    handleClient(fd, epfd_copy);
                });

                // 主线程立即继续 epoll_wait，不阻塞
            }
        }
    }

    // 清理资源
    close(listen_fd);
    close(epfd);

    return 0;
}

/*
============================================================
🎓 学习总结
============================================================

【epoll + 线程池架构】

职责分离：
  - 主线程：epoll_wait 监听事件（快）
  - 工作线程：处理业务逻辑（可以慢）

优势：
  - 主线程不阻塞，响应快
  - 工作线程并发处理，吞吐量高
  - 充分利用多核 CPU

【EPOLLONESHOT 的作用】

问题：
  - 同一 fd 可能被多个线程同时处理
  - 导致竞态条件、数据乱序

解决：
  - EPOLLONESHOT：触发后自动移除
  - 处理完成后 EPOLL_CTL_MOD 重新注册
  - 保证同一时刻只有一个线程持有 fd

【关键代码】

注册时设置 EPOLLONESHOT：
  ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
  epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);

处理完成后重新注册：
  ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
  epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);

主线程分发任务：
  pool.enqueue([fd, epfd]() {
      handleClient(fd, epfd);
  });

【注意事项】⚠️

1. listen_fd 不需要 EPOLLONESHOT
   - 因为 accept 不涉及竞态问题
   - 主线程循环 accept 即可

2. client_fd 必须 EPOLLONESHOT
   - 防止多个工作线程同时处理

3. 处理完成后必须重新注册
   - 否则 fd 永远不会再触发

4. 连接关闭时不需要重新注册
   - 直接 EPOLL_CTL_DEL 即可

【工业级架构】

这就是 Nginx、Redis 的核心架构：
  - Nginx：epoll + 线程池 + EPOLLONESHOT
  - Redis：epoll（单线程，不需要 EPOLLONESHOT）

【下一步学习】

- epoll 的常见陷阱和最佳实践
- epoll 性能调优技巧
- Reactor 模式（事件驱动框架）

============================================================
*/
