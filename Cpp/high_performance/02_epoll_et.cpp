// ============================================
// 02_epoll_et.cpp
// 日期：2026-06-16（第71天）
// 主题：epoll 边缘触发（ET）模式 + 非阻塞 I/O
// 难度：⭐⭐⭐⭐ 高级
// 注意：需在 Linux/WSL 环境编译运行
// 编译：g++ -std=c++17 -o 02_epoll_et 02_epoll_et.cpp
// 运行：./02_epoll_et  然后用 telnet localhost 8081 测试
// ============================================

/*
【核心概念】边缘触发（Edge Triggered, ET）

是什么？
- epoll 的高性能模式
- 只在 fd 状态从"未就绪"变为"就绪"时通知一次
- Nginx、Redis 等高性能服务器的选择

为什么需要？
- LT 模式的问题：只要有数据，epoll_wait 就会反复返回（额外的系统调用）
- ET 模式的优势：只通知一次，减少系统调用次数
- 性能提升：在高并发场景下，ET 比 LT 快 20-30%

核心特点？
- 必须配合非阻塞 I/O（否则最后一次 recv 会永久阻塞）
- 必须循环读取直到 EAGAIN（否则会丢失数据）
- 编程复杂度更高，但性能更好
*/

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>

using namespace std;

const int PORT = 8081;  // 使用不同端口，方便与 LT 版本对比
const int MAX_EVENTS = 1024;


// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

void showLT_vs_ET() {
    cout << R"(
============================================================
LT（水平触发）vs ET（边缘触发）⭐⭐⭐⭐⭐
============================================================

【概念对比】

水平触发（Level Triggered, LT）：
  - 触发条件：只要 fd 就绪，epoll_wait 就返回
  - 行为：持续通知（像电灯开关一直按着，灯一直亮）
  - 特点：安全，不容易漏数据
  - 缺点：系统调用多，性能较低

边缘触发（Edge Triggered, ET）：
  - 触发条件：只在状态变化时通知一次
  - 行为：通知一次（像门铃，按一次响一次）
  - 特点：高效，系统调用少
  - 缺点：容易漏数据，编程复杂

【生活类比】

LT 模式：水龙头
  - 只要有水（数据），就持续流出（通知）
  - 你可以慢慢接水（分多次读取）
  - 不用担心错过水（安全）

ET 模式：门铃
  - 客人按一次门铃（状态变化）
  - 门铃只响一次（通知一次）
  - 你必须立即去开门（一次性处理完）
  - 如果没听到，客人就走了（数据丢失）

【工作流程对比】

假设：客户端发送 1000 字节数据

LT 模式：
  1. 数据到达，epoll_wait 返回（通知你）
  2. 你读了 300 字节
  3. 还有 700 字节未读
  4. epoll_wait 再次返回（又通知你）← 额外的系统调用
  5. 你读了 300 字节
  6. 还有 400 字节未读
  7. epoll_wait 再次返回（又通知你）← 额外的系统调用
  8. 你读完剩余的 400 字节

ET 模式：
  1. 数据到达，epoll_wait 返回（通知你一次）
  2. 你必须循环读取：
     - 读 300 字节
     - 读 300 字节
     - 读 300 字节
     - 读 100 字节
     - recv 返回 EAGAIN（数据读完）
  3. 所有数据一次性处理完

结论：ET 减少了 epoll_wait 的返回次数（减少系统调用）

【性能对比】

场景：1000 个连接，每个连接有 10KB 数据

LT 模式：
  - 每个连接可能触发 10 次 epoll_wait（分多次读取）
  - 总系统调用：1000 × 10 = 10000 次

ET 模式：
  - 每个连接只触发 1 次 epoll_wait（一次性读完）
  - 总系统调用：1000 × 1 = 1000 次

性能提升：10 倍（在高并发场景下）

【代价对比】

LT 模式（简单但慢）：
  ✅ 编程简单：不需要循环读取
  ✅ 安全：不会丢数据
  ❌ 性能较低：系统调用多
  ❌ 不需要非阻塞 I/O

ET 模式（复杂但快）：
  ✅ 性能高：系统调用少
  ❌ 编程复杂：必须循环读取
  ❌ 容易出错：可能丢数据
  ❌ 必须非阻塞 I/O

============================================================
)" << endl;
}


void showET_Requirements() {
    cout << R"(
============================================================
ET 模式的两大要求 ⭐⭐⭐⭐⭐
============================================================

【要求1：必须使用非阻塞 I/O】

为什么？
  - ET 模式下，必须循环读取直到数据读完
  - 如果使用阻塞 I/O，最后一次 recv 会永久阻塞
  - 因为没有数据时，阻塞 I/O 会等待更多数据到来

示例：阻塞 I/O 的问题
  while (true) {
      int n = recv(fd, buf, 1024, 0);  // 阻塞模式
      if (n > 0) {
          // 处理数据
      } else if (n == 0) {
          break;  // 连接关闭
      }
  }
  // 问题：当读完所有数据后，recv 会阻塞等待
  // → 整个事件循环卡住
  // → 其他连接无法处理

非阻塞 I/O 的解决方案：
  while (true) {
      int n = recv(fd, buf, 1024, 0);  // 非阻塞模式
      if (n > 0) {
          // 处理数据
      } else if (n == 0) {
          break;  // 连接关闭
      } else {  // n < 0
          if (errno == EAGAIN || errno == EWOULDBLOCK) {
              break;  // 数据读完了，正常退出
          } else {
              // 真正的错误
              perror("recv");
              break;
          }
      }
  }

设置非阻塞的方法：
  int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);

【要求2：必须循环读取直到 EAGAIN】

为什么？
  - ET 模式只通知一次
  - 如果你不一次性读完，剩余数据不会再通知
  - 剩余数据会"卡"在内核缓冲区，永远读不到

错误示例（只读一次）：
  // ❌ 错误：ET 模式下只读一次
  char buf[1024];
  int n = recv(fd, buf, sizeof(buf), 0);
  if (n > 0) {
      send(fd, buf, n, 0);
  }
  // 问题：如果数据超过 1024 字节，剩余部分丢失

正确示例（循环读取）：
  // ✅ 正确：循环读取直到 EAGAIN
  while (true) {
      char buf[1024];
      int n = recv(fd, buf, sizeof(buf), 0);
      if (n > 0) {
          send(fd, buf, n, 0);
      } else if (n == 0) {
          // 连接关闭
          break;
      } else {
          if (errno == EAGAIN) {
              // 数据读完了，正常
              break;
          } else {
              // 真正的错误
              perror("recv");
              break;
          }
      }
  }

【EAGAIN 的含义】⭐⭐⭐⭐⭐

EAGAIN 不是错误！
  - 含义：暂时没有数据可读
  - 在非阻塞模式下，这是正常的
  - 表示：内核缓冲区的数据已经读完了

错误理解：
  ❌ "EAGAIN 是错误，应该关闭连接"
  ❌ "EAGAIN 表示连接有问题"

正确理解：
  ✅ "EAGAIN 表示数据读完了，可以退出循环"
  ✅ "EAGAIN 是 ET 模式的正常信号"

【ET 模式的陷阱】⚠️

陷阱1：忘记循环读取
  → 数据丢失

陷阱2：使用阻塞 I/O
  → 程序卡死

陷阱3：把 EAGAIN 当作错误
  → 错误地关闭连接

陷阱4：只设置 client_fd 为非阻塞，忘记 listen_fd
  → accept 可能阻塞

============================================================
)" << endl;
}


// ============================================
// 第二部分：ET 模式回显服务器实战 ⭐⭐⭐⭐⭐
// ============================================

// 设置 fd 为非阻塞模式（ET 模式必须）
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

int main() {
    cout << "=========================================================" << endl;
    cout << "epoll 边缘触发（ET）模式 + 非阻塞 I/O" << endl;
    cout << "=========================================================" << endl;
    cout << endl;

    // 显示理论知识
    cout << "【理论学习】" << endl;
    showLT_vs_ET();
    cout << endl;
    showET_Requirements();
    cout << endl;

    cout << "=========================================================" << endl;
    cout << "【实战项目】ET 模式 epoll 回显服务器" << endl;
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
    cout << "[1/6] socket 创建成功" << endl;

    // 设置地址复用
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // ⭐ 关键：设置 listen_fd 为非阻塞
    setNonBlocking(listen_fd);
    cout << "[2/6] listen_fd 设置为非阻塞模式" << endl;

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
    cout << "[3/6] bind 成功，监听 0.0.0.0:" << PORT << endl;

    if (listen(listen_fd, 10) < 0) {
        perror("listen");
        close(listen_fd);
        return 1;
    }
    cout << "[4/6] listen 成功，开始创建 epoll..." << endl;

    // --------------------------------------------
    // Step 2: 创建 epoll 实例
    // --------------------------------------------
    int epfd = epoll_create1(0);
    if (epfd < 0) {
        perror("epoll_create1");
        close(listen_fd);
        return 1;
    }
    cout << "[5/6] epoll 实例创建成功（epfd=" << epfd << "）" << endl;

    // --------------------------------------------
    // Step 3: 将监听 socket 加入 epoll（ET 模式）
    // --------------------------------------------
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;  // ⭐ 关键：添加 EPOLLET 标志
    ev.data.fd = listen_fd;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev) < 0) {
        perror("epoll_ctl ADD listen_fd");
        close(listen_fd);
        close(epfd);
        return 1;
    }
    cout << "[6/6] 监听 socket 已加入 epoll（ET 模式）" << endl;
    cout << "\n服务器启动成功！（ET 模式）" << endl;
    cout << "提示：可用 telnet localhost " << PORT << " 或 nc localhost " << PORT << " 测试\n" << endl;


    // --------------------------------------------
    // Step 4: 事件循环
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
                // ⭐ ET 模式：必须循环 accept 直到 EAGAIN
                while (true) {
                    struct sockaddr_in client_addr;
                    socklen_t addr_len = sizeof(client_addr);
                    int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &addr_len);

                    if (client_fd < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            // 所有新连接都处理完了，正常退出
                            break;
                        } else {
                            perror("accept");
                            break;
                        }
                    }

                    // 获取客户端地址
                    char ip_str[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));
                    int port = ntohs(client_addr.sin_port);

                    cout << "[新连接] " << ip_str << ":" << port << " (fd=" << client_fd << ")" << endl;

                    // ⭐ 关键：设置 client_fd 为非阻塞
                    setNonBlocking(client_fd);

                    // 将新连接加入 epoll（ET 模式）
                    ev.events = EPOLLIN | EPOLLET;  // ET 模式
                    ev.data.fd = client_fd;
                    if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev) < 0) {
                        perror("epoll_ctl ADD client_fd");
                        close(client_fd);
                        continue;
                    }

                    client_count++;
                    cout << "当前在线：" << client_count << " 人" << endl;
                }
            }
            // --------------------------------------------
            // 情况2：客户端 socket 就绪 → 有数据到达
            // --------------------------------------------
            else {
                // ⭐ ET 模式：必须循环读取直到 EAGAIN
                bool connection_closed = false;
                bool error_occurred = false;

                while (true) {
                    char buf[1024];
                    memset(buf, 0, sizeof(buf));

                    int len = recv(fd, buf, sizeof(buf) - 1, 0);

                    if (len > 0) {
                        // 收到数据，回显
                        buf[len] = '\0';
                        cout << "[recv] fd=" << fd << ": " << buf;

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
                        cout << "[关闭] fd=" << fd << " 断开连接" << endl;
                        connection_closed = true;
                        break;
                    }
                    else {
                        // len < 0
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            // ⭐ 数据读完了，正常退出（不是错误）
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
                    client_count--;
                    cout << "当前在线：" << client_count << " 人" << endl;
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

【LT vs ET 本质区别】

LT（水平触发）：
  - 持续通知：只要有数据就通知
  - 就像水龙头：只要开着就持续流水
  - 安全但慢：不会丢数据，但系统调用多

ET（边缘触发）：
  - 通知一次：状态变化时通知一次
  - 就像门铃：按一次响一次
  - 快但复杂：系统调用少，但容易出错

【ET 模式的两大要求】

1. 必须使用非阻塞 I/O
   - 原因：需要循环读取直到数据读完
   - 如果阻塞：最后一次 recv 会永久阻塞

2. 必须循环读取直到 EAGAIN
   - 原因：ET 只通知一次
   - 如果不读完：剩余数据会丢失

【EAGAIN 的含义】⭐⭐⭐⭐⭐

EAGAIN 不是错误！
  - 含义：暂时没有数据可读
  - 在 ET 模式下：表示数据读完了，正常退出
  - 处理方式：break（退出循环）

【ET 模式的关键代码】

设置 ET 模式：
  ev.events = EPOLLIN | EPOLLET;

设置非阻塞：
  int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);

循环读取：
  while (true) {
      int n = recv(fd, buf, sizeof(buf), 0);
      if (n > 0) {
          // 处理数据
      } else if (n == 0) {
          // 连接关闭
          break;
      } else {
          if (errno == EAGAIN) {
              break;  // 数据读完，正常
          } else {
              // 真正的错误
              break;
          }
      }
  }

【性能对比】

LT 模式：
  - 1000 个连接，每个 10KB 数据
  - 可能触发 10000 次 epoll_wait（每个连接 10 次）

ET 模式：
  - 1000 个连接，每个 10KB 数据
  - 只触发 1000 次 epoll_wait（每个连接 1 次）
  - 性能提升：10 倍

【适用场景】

LT 模式适合：
  - 开发阶段：简单，易调试
  - 低并发场景：<100 连接
  - 安全要求高：不能丢数据

ET 模式适合：
  - 高并发场景：>1000 连接
  - 性能要求高：Nginx、Redis
  - 生产环境：追求极致性能

【下一步学习】

- epoll + 线程池
- EPOLLONESHOT（确保同一 fd 只被一个线程处理）
- epoll 实战陷阱与最佳实践

============================================================
*/
