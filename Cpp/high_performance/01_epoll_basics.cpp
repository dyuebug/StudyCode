// ============================================
// 01_epoll_basics.cpp
// 日期：2026-06-15（第70天）
// 主题：epoll 基础 - 高性能 I/O 多路复用
// 难度：⭐⭐⭐⭐ 高级
// 注意：需在 Linux/WSL 环境编译运行
// 编译：g++ -std=c++17 -o 01_epoll 01_epoll_basics.cpp
// 运行：./01_epoll  然后用多个 telnet 127.0.0.1 8080 连接测试
// ============================================

/*
【核心概念】epoll - Linux 高性能 I/O 多路复用

是什么？
- epoll 是 Linux 独有的高性能 I/O 多路复用机制
- select 的升级版，解决了 select 的三大缺陷
- Nginx、Redis、Node.js 等高性能服务器的核心技术

为什么需要？
- select 的三大问题：
  1. O(n) 轮询：每次都要遍历所有 fd，连接越多越慢
  2. 1024 限制：FD_SETSIZE 硬编码为 1024
  3. 重复设置：每次调用都要重新设置 fd_set
- epoll 的解决方案：
  1. 事件通知：O(1) 获取就绪 fd，不需要轮询
  2. 无数量限制：受限于系统资源，没有硬编码
  3. 状态维护：内核维护 fd 状态，不需要每次重设

核心特点？
- 事件驱动架构（Event-Driven）
- 支持百万级并发连接（C10K 问题的解决方案）
- 性能不随连接数增加而下降（只跟活跃连接数相关）
*/

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>  // epoll API
#include <fcntl.h>
#include <errno.h>

using namespace std;

const int PORT = 8080;
const int MAX_EVENTS = 1024;  // epoll_wait 一次最多返回的事件数


// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

void showSelectVsEpoll() {
    cout << R"(
============================================================
select vs epoll 对比 ⭐⭐⭐⭐⭐
============================================================

【select 的三大缺陷】

1. 性能问题：O(n) 轮询
   - 每次调用 select，内核都要遍历所有 fd
   - 1000 个连接，只有 10 个活跃，也要检查 1000 次
   - 连接数 ↑，性能 ↓（线性下降）

2. 数量限制：最多 1024 个 fd
   - FD_SETSIZE 硬编码为 1024
   - 无法支持高并发（C10K 问题）

3. 重复设置：每次都要重设 fd_set
   - select 会修改 fd_set，下次调用需要重新设置
   - 用户态 ↔ 内核态频繁拷贝

【epoll 的三大改进】

1. 事件通知：O(1) 性能
   - 内核维护就绪列表，直接返回就绪的 fd
   - 不需要遍历，性能与总连接数无关
   - 只与活跃连接数相关

2. 无数量限制
   - 受限于系统资源（文件描述符上限）
   - 可以支持百万级并发

3. 状态维护：减少系统调用
   - 内核维护 fd 状态，不需要每次重设
   - 减少用户态 ↔ 内核态拷贝

【性能对比】

场景：10000 个连接，100 个活跃

select：
  - 每次 select() 遍历 10000 个 fd
  - 时间复杂度：O(10000)
  - 性能瓶颈：总连接数

epoll：
  - epoll_wait() 直接返回 100 个就绪 fd
  - 时间复杂度：O(100)
  - 性能瓶颈：活跃连接数

结论：连接数越多，epoll 优势越明显
============================================================
)" << endl;
}


void showEpollInternals() {
    cout << R"(
============================================================
epoll 内部原理 ⭐⭐⭐⭐⭐
============================================================

【核心数据结构】

1. 红黑树（RB-Tree）
   - 存储所有被监听的 fd
   - fd 作为 key，epoll_event 作为 value
   - 插入/删除/查找：O(log n)

   结构：
   ┌───────────────────────┐
   │    epoll 实例         │
   │  ┌─────────────────┐  │
   │  │   红黑树（RB-Tree）│  │
   │  │                 │  │
   │  │   fd=3 (listen) │  │
   │  │   fd=4 (client) │  │
   │  │   fd=5 (client) │  │
   │  │   ...           │  │
   │  └─────────────────┘  │
   └───────────────────────┘

2. 就绪列表（Ready List）
   - 双向链表，存储已就绪的 fd
   - epoll_wait 直接返回这个列表

   结构：
   ┌───────────────────────┐
   │  就绪列表（Ready List）│
   │  ┌─┐  ┌─┐  ┌─┐       │
   │  │4│→│7│→│9│  (已就绪的 fd) │
   │  └─┘  └─┘  └─┘       │
   └───────────────────────┘

【工作流程】

Step 1: epoll_create1()
  - 创建 epoll 实例
  - 初始化红黑树和就绪列表

Step 2: epoll_ctl(EPOLL_CTL_ADD)
  - 将 fd 插入红黑树
  - 向内核注册回调函数（callback）

Step 3: fd 就绪时
  - 内核调用回调函数
  - 将 fd 加入就绪列表

Step 4: epoll_wait()
  - 返回就绪列表中的 fd
  - O(1) 时间复杂度

【为什么 epoll 快？】

select 方式：
  while (还有 fd 未检查) {
      检查 fd 是否就绪;  // O(n) 遍历
  }

epoll 方式：
  直接返回就绪列表;  // O(1) 获取

本质区别：
- select：主动轮询（你去问每个 fd）
- epoll：被动通知（fd 就绪时主动告诉你）
============================================================
)" << endl;
}


void showEpollAPI() {
    cout << R"(
============================================================
epoll 三大 API ⭐⭐⭐⭐⭐
============================================================

【API 1：epoll_create1() - 创建 epoll 实例】

函数签名：
  int epoll_create1(int flags);

参数：
  - flags：0 或 EPOLL_CLOEXEC
    * 0：默认行为
    * EPOLL_CLOEXEC：exec 时关闭此 fd

返回值：
  - 成功：epoll 文件描述符
  - 失败：-1

示例：
  int epfd = epoll_create1(0);
  if (epfd < 0) {
      perror("epoll_create1");
      exit(1);
  }

────────────────────────────────────────────────────────

【API 2：epoll_ctl() - 管理监听的 fd】

函数签名：
  int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);

参数：
  - epfd：epoll 实例
  - op：操作类型
    * EPOLL_CTL_ADD：添加 fd 到 epoll
    * EPOLL_CTL_MOD：修改 fd 的事件
    * EPOLL_CTL_DEL：删除 fd
  - fd：要监听的文件描述符
  - event：事件结构体（DEL 时可为 NULL）

epoll_event 结构：
  struct epoll_event {
      uint32_t events;   // 事件类型
      epoll_data_t data; // 用户数据
  };

events 常用值：
  - EPOLLIN：可读（有数据到达）
  - EPOLLOUT：可写（可以发送数据）
  - EPOLLERR：错误
  - EPOLLHUP：挂断（对方关闭连接）

示例：
  struct epoll_event ev;
  ev.events = EPOLLIN;      // 监听可读事件
  ev.data.fd = client_fd;   // 存储 fd
  epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);

────────────────────────────────────────────────────────

【API 3：epoll_wait() - 等待事件就绪】

函数签名：
  int epoll_wait(int epfd, struct epoll_event *events,
                 int maxevents, int timeout);

参数：
  - epfd：epoll 实例
  - events：输出参数，返回就绪事件的数组
  - maxevents：events 数组大小
  - timeout：超时时间（毫秒）
    * -1：永久阻塞
    *  0：立即返回（非阻塞）
    * >0：超时毫秒数

返回值：
  - >0：就绪的 fd 数量
  -  0：超时
  - -1：出错

示例：
  struct epoll_event events[MAX_EVENTS];
  int n = epoll_wait(epfd, events, MAX_EVENTS, -1);

  for (int i = 0; i < n; i++) {
      int fd = events[i].data.fd;
      if (events[i].events & EPOLLIN) {
          // 可读事件
      }
  }

============================================================
)" << endl;
}


// ============================================
// 第二部分：epoll 回显服务器实战 ⭐⭐⭐⭐⭐
// ============================================

// 设置 fd 为非阻塞模式（ET 模式需要）
void setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    cout << "=========================================================" << endl;
    cout << "epoll 基础 - 高性能 I/O 多路复用" << endl;
    cout << "=========================================================" << endl;
    cout << endl;

    // 显示理论知识
    cout << "【理论学习】" << endl;
    showSelectVsEpoll();
    cout << endl;
    showEpollInternals();
    cout << endl;
    showEpollAPI();
    cout << endl;

    cout << "=========================================================" << endl;
    cout << "【实战项目】epoll 回显服务器" << endl;
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
    cout << "[1/5] socket 创建成功" << endl;

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
    cout << "[2/5] bind 成功，监听 0.0.0.0:" << PORT << endl;

    // 监听
    if (listen(listen_fd, 10) < 0) {
        perror("listen");
        close(listen_fd);
        return 1;
    }
    cout << "[3/5] listen 成功，开始创建 epoll..." << endl;


    // --------------------------------------------
    // Step 2: 创建 epoll 实例
    // --------------------------------------------
    int epfd = epoll_create1(0);
    if (epfd < 0) {
        perror("epoll_create1");
        close(listen_fd);
        return 1;
    }
    cout << "[4/5] epoll 实例创建成功（epfd=" << epfd << "）" << endl;

    // --------------------------------------------
    // Step 3: 将监听 socket 加入 epoll
    // --------------------------------------------
    struct epoll_event ev;
    ev.events = EPOLLIN;      // 监听可读事件（新连接到达）
    ev.data.fd = listen_fd;   // 存储 fd，方便后续识别

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev) < 0) {
        perror("epoll_ctl ADD listen_fd");
        close(listen_fd);
        close(epfd);
        return 1;
    }
    cout << "[5/5] 监听 socket 已加入 epoll" << endl;
    cout << "\n服务器启动成功！" << endl;
    cout << "提示：可用 telnet localhost " << PORT << " 或 nc localhost " << PORT << " 测试\n" << endl;

    // --------------------------------------------
    // Step 4: 事件循环
    // --------------------------------------------
    struct epoll_event events[MAX_EVENTS];  // 存储就绪事件
    int client_count = 0;  // 统计在线客户端数量

    while (true) {
        // epoll_wait 阻塞等待事件就绪
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
                struct sockaddr_in client_addr;
                socklen_t addr_len = sizeof(client_addr);
                int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &addr_len);

                if (client_fd < 0) {
                    perror("accept");
                    continue;
                }

                // 获取客户端地址
                char ip_str[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));
                int port = ntohs(client_addr.sin_port);

                cout << "[新连接] " << ip_str << ":" << port << " (fd=" << client_fd << ")" << endl;

                // 将新连接加入 epoll
                ev.events = EPOLLIN;
                ev.data.fd = client_fd;
                if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev) < 0) {
                    perror("epoll_ctl ADD client_fd");
                    close(client_fd);
                    continue;
                }

                client_count++;
                cout << "当前在线：" << client_count << " 人" << endl;
            }
            // --------------------------------------------
            // 情况2：客户端 socket 就绪 → 有数据到达
            // --------------------------------------------
            else {
                char buf[1024];
                memset(buf, 0, sizeof(buf));

                int len = recv(fd, buf, sizeof(buf) - 1, 0);

                if (len > 0) {
                    // 收到数据，回显
                    buf[len] = '\0';
                    cout << "[recv] fd=" << fd << ": " << buf;
                    send(fd, buf, len, 0);
                }
                else if (len == 0) {
                    // 客户端关闭连接
                    cout << "[关闭] fd=" << fd << " 断开连接" << endl;

                    // 从 epoll 删除
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);

                    client_count--;
                    cout << "当前在线：" << client_count << " 人" << endl;
                }
                else {
                    // 错误
                    perror("recv");
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                    client_count--;
                }
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

【epoll vs select 本质区别】

select：主动轮询
  - 你：select，1号fd准备好了吗？没有
  - 你：2号呢？没有
  - 你：3号呢？没有
  - ...（O(n) 遍历）

epoll：被动通知
  - 你：epoll_wait（等待）
  - 内核：3号和7号准备好了
  - 你：好的，处理3号和7号（O(1) 获取）

【epoll 三大 API 总结】

1. epoll_create1(0)
   - 创建 epoll 实例
   - 初始化红黑树和就绪列表

2. epoll_ctl(epfd, op, fd, &ev)
   - EPOLL_CTL_ADD：添加监听
   - EPOLL_CTL_MOD：修改事件
   - EPOLL_CTL_DEL：删除监听

3. epoll_wait(epfd, events, max, timeout)
   - 返回就绪的事件数组
   - O(1) 时间复杂度

【关键设计点】

1. 监听 socket 和客户端 socket 都要加入 epoll
2. 用 fd 区分是新连接还是数据到达
3. 客户端关闭时记得 EPOLL_CTL_DEL
4. epoll_wait 返回的是就绪事件数组，不是所有 fd

【性能优势】

- select：O(n) 遍历，1024 限制
- epoll：O(1) 获取，百万级支持
- 连接数越多，epoll 优势越明显

【下一步学习】

- 边缘触发（ET）vs 水平触发（LT）
- 非阻塞 I/O 配合 epoll
- epoll + 线程池架构

============================================================
*/
