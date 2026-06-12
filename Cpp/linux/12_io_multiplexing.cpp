// ============================================
// 12_io_multiplexing.cpp
// 日期：2026-06-10（第67天）
// 主题：I/O 多路复用 - select 单线程处理多连接
// 难度：⭐⭐⭐ 高级
// 注意：需在 Linux/WSL 环境编译运行
// 编译：g++ -std=c++17 -o 12_select 12_io_multiplexing.cpp
// 运行：./12_select server  然后用多个 nc 127.0.0.1 8890 连接测试
// ============================================

/*
【核心概念】简要说明

是什么？
- I/O 多路复用：用一个线程同时监视多个套接字的"就绪"状态
- select：最经典的多路复用函数，监视一组 fd，哪个有数据就处理哪个

为什么需要？
- 问题：服务端要同时服务1000个客户端怎么办？
  方案A：每个客户端一个线程 → 1000个线程，开销大、切换慢
  方案B：一个线程轮询所有连接 → 没数据时空转，浪费 CPU
  方案C：select 多路复用 → 一个线程"盯着"所有连接，谁来数据处理谁

核心特点？
- 单线程处理多连接（高并发服务器的基础）
- select 阻塞等待，直到至少一个 fd 就绪才返回
- 避免了"一连接一线程"的资源开销

【生活类比】select 像餐厅服务员：
  - 一个服务员（线程）盯着所有餐桌（连接）
  - 哪桌举手（数据就绪）就去服务哪桌
  - 不需要每桌配一个专属服务员（一连接一线程）

【与你学过的知识对照】
  第66天 多线程方案    → 一连接一线程（accept后pthread_create）
  本文件 select 方案   → 单线程处理所有连接（事件驱动）
  这是高并发服务器的两大流派
*/

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>  // select(), fd_set, FD_* 宏
#include <vector>
#include <algorithm>
using namespace std;

const int PORT = 8890;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

void showConcepts() {
    cout << R"(
============================================================
I/O 多路复用 (select) - 核心概念
============================================================

【三种并发服务器模型对比】⭐⭐⭐⭐⭐

  模型1：一连接一线程（第66天的方案）
    accept → pthread_create → 每个线程处理一个客户端
    优点：编程简单    缺点：1万连接=1万线程，资源爆炸

  模型2：select 多路复用（本文件）
    一个线程用 select 盯着所有 fd，谁就绪处理谁
    优点：单线程省资源  缺点：fd 数量有限(1024)，遍历开销

  模型3：epoll（Linux 高性能，了解即可）
    select 的升级版，支持百万连接，事件通知更高效
    （Nginx、Redis 都用 epoll）

【select 工作原理图】

  fd_set（一个 bitmap，记录要监视哪些 fd）
  ┌─┬─┬─┬─┬─┬─┬─┬─┐
  │0│1│1│0│1│0│0│1│  ← 第1,2,4,7位是1，表示监视 fd 1,2,4,7
  └─┴─┴─┴─┴─┴─┴─┴─┘
         │
         ▼  select(maxfd+1, &readfds, ...)
    阻塞等待，直到有 fd 就绪
         │
         ▼  返回后，fd_set 只剩"就绪"的位
  ┌─┬─┬─┬─┬─┬─┬─┬─┐
  │0│0│1│0│0│0│0│0│  ← 只有 fd 2 就绪，用 FD_ISSET 检查
  └─┴─┴─┴─┴─┴─┴─┴─┘

【select 编程套路】⭐⭐⭐⭐⭐
  while (true) {
      FD_ZERO(&readfds);              // 1. 清空集合
      FD_SET(listen_fd, &readfds);    // 2. 把要监视的fd加进去
      for (每个客户端 fd) FD_SET(...);

      select(maxfd+1, &readfds, ...); // 3. 阻塞等待就绪

      if (FD_ISSET(listen_fd, &readfds))  // 4. 检查哪个就绪
          accept 新连接;
      for (每个客户端 fd)
          if (FD_ISSET(fd, &readfds))
              recv 处理数据;
  }
  关键：每次循环都要重新 FD_SET（select 会修改 fd_set）
============================================================
)" << endl;
}

// ============================================
// 第一部分补充：核心函数卡片 ⭐⭐⭐⭐⭐
// ============================================

void showFunctionCards() {
    cout << R"(
============================================================
select 核心函数与宏卡片
============================================================

【函数：select() - 多路复用核心】⭐⭐⭐⭐⭐
语法：
  int select(int nfds, fd_set* readfds, fd_set* writefds,
             fd_set* exceptfds, struct timeval* timeout);
参数：
  1. nfds      - 要监视的最大 fd + 1（重点：是最大fd值加1！）
  2. readfds   - 监视"可读"的 fd 集合（有数据可读时就绪）
  3. writefds  - 监视"可写"的 fd 集合（不关心传 NULL）
  4. exceptfds - 监视"异常"的 fd 集合（不关心传 NULL）
  5. timeout   - 超时时间：
                 NULL=一直阻塞；{0,0}=立即返回；{秒,微秒}=等待指定时间
返回：就绪的 fd 数量；0=超时；-1=出错
重点：select 返回后会修改 readfds，只保留就绪的 fd

────────────────────────────────────

【fd_set 操作宏】⭐⭐⭐⭐⭐
  FD_ZERO(&set)          清空集合（每次循环开始必做）
  FD_SET(fd, &set)       把 fd 加入集合（标记要监视）
  FD_CLR(fd, &set)       把 fd 从集合移除
  FD_ISSET(fd, &set)     检查 fd 是否在集合里（就绪判断）

使用模式：
  fd_set readfds;
  FD_ZERO(&readfds);           // 清空
  FD_SET(listen_fd, &readfds); // 加入监听fd
  FD_SET(client_fd, &readfds); // 加入客户端fd
  select(maxfd+1, &readfds, NULL, NULL, NULL);  // 等待
  if (FD_ISSET(listen_fd, &readfds)) { ... }    // 检查就绪

────────────────────────────────────

【为什么 nfds 是"最大fd+1"？】
  select 内部从 fd 0 遍历到 nfds-1 检查每一位。
  如果最大 fd 是 7，要检查 0~7 共8个，所以传 7+1=8。
  漏加1会导致最大的那个 fd 检查不到！

────────────────────────────────────

【为什么每次循环要重新 FD_SET？】
  select 返回时会"修改" readfds，把没就绪的 fd 清掉。
  所以下次循环前必须重新 FD_ZERO + FD_SET 所有要监视的 fd，
  否则集合里只剩上次就绪的那些了。
)" << endl;
}

// ============================================
// 第二部分：基础操作实践 - select 多客户端回显服务器
// ============================================

void runServer() {
    cout << "\n[select服务端] 启动中..." << endl;

    // 创建监听套接字（和 TCP 服务端前几步一样）
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) { perror("socket"); return; }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_fd, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("bind"); close(listen_fd); return;
    }
    if (listen(listen_fd, 10) < 0) {
        perror("listen"); close(listen_fd); return;
    }
    cout << "[select服务端] 监听端口 " << PORT << endl;
    cout << "[提示] 另开终端用 nc 127.0.0.1 " << PORT
         << " 连接（可多开几个）" << endl;

    // 保存所有客户端 fd 的列表
    vector<int> clients;
    int maxfd = listen_fd;  // 当前最大 fd

    while (true) {
        // 步骤1：每次循环都要重新构建 fd_set（select 会修改它）
        fd_set readfds;
        FD_ZERO(&readfds);              // 清空
        FD_SET(listen_fd, &readfds);    // 监视监听套接字（新连接）
        maxfd = listen_fd;

        // 把所有客户端 fd 加入监视集合
        for (int fd : clients) {
            FD_SET(fd, &readfds);
            if (fd > maxfd) maxfd = fd;  // 更新最大 fd
        }

        // 步骤2：阻塞等待，直到有 fd 就绪（timeout=NULL 一直等）
        int ready = select(maxfd + 1, &readfds, nullptr, nullptr, nullptr);
        if (ready < 0) { perror("select"); break; }

        // 步骤3：检查监听套接字 —— 有新连接到来
        if (FD_ISSET(listen_fd, &readfds)) {
            sockaddr_in cliAddr;
            socklen_t cliLen = sizeof(cliAddr);
            int conn_fd = accept(listen_fd, (sockaddr*)&cliAddr, &cliLen);
            if (conn_fd >= 0) {
                clients.push_back(conn_fd);  // 加入客户端列表
                char ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &cliAddr.sin_addr, ip, sizeof(ip));
                cout << "[新连接] " << ip << ":" << ntohs(cliAddr.sin_port)
                     << " (fd=" << conn_fd << ")，当前 "
                     << clients.size() << " 个客户端" << endl;
            }
        }

        // 步骤4：检查每个客户端 fd —— 有数据可读
        for (auto it = clients.begin(); it != clients.end(); ) {
            int fd = *it;
            if (FD_ISSET(fd, &readfds)) {
                char buffer[1024];
                memset(buffer, 0, sizeof(buffer));
                ssize_t n = recv(fd, buffer, sizeof(buffer) - 1, 0);
                if (n > 0) {
                    cout << "[fd=" << fd << "] 收到：" << buffer;
                    // 回显
                    send(fd, buffer, n, 0);
                    ++it;
                } else {
                    // n==0 客户端关闭；n<0 出错 → 移除该客户端
                    cout << "[fd=" << fd << "] 断开连接" << endl;
                    close(fd);
                    it = clients.erase(it);  // 从列表删除并更新迭代器
                }
            } else {
                ++it;
            }
        }
    }

    close(listen_fd);
}

// ============================================
// 第三部分：深入理解（选学）🔍
// ============================================

/*
==============================================
🔍 深入理解1：select 为什么能"单线程处理多连接"？（选学）⭐⭐⭐⭐⭐
==============================================

核心思想：不要"傻等"任何一个连接，而是"谁有数据就处理谁"。

对比"一连接一线程"：
  线程A: recv(fd1)  ← 阻塞等 fd1 的数据
  线程B: recv(fd2)  ← 阻塞等 fd2 的数据
  ...1000个连接就要1000个线程在那阻塞等待

select 的做法：
  一个线程: select(所有fd)  ← 等待"任意一个"fd 就绪
  谁来数据 select 就返回，处理那个 fd，处理完继续 select
  → 一个线程就够了，因为它从不为单个连接傻等

这就是"事件驱动"：不主动轮询，而是被动等待事件（数据就绪）通知。

==============================================
🔍 深入理解2：select 的三大局限（选学）
==============================================

1. fd 数量限制：fd_set 默认最多 1024 个 fd（FD_SETSIZE）
   想监视上万连接，select 力不从心

2. 每次都要拷贝 fd_set：用户态↔内核态来回拷贝，连接多了开销大

3. 返回后要遍历所有 fd：select 只告诉你"有就绪的"，不告诉你"是哪个"，
   得 FD_ISSET 挨个检查，O(n) 复杂度

→ 这就是 epoll 诞生的原因：
  epoll 没有数量限制、不用每次拷贝、直接返回就绪的 fd 列表（O(1)）
  Linux 高性能服务器（Nginx/Redis）都用 epoll

==============================================
🔍 深入理解3：select 后为什么 fd_set 被改了？（选学）
==============================================

select 是"传入传出"参数：
  传入：你想监视哪些 fd（位图里对应位=1）
  传出：哪些 fd 就绪了（select 把没就绪的位清0，只留就绪的）

所以一次 select 后，readfds 已经不是你原来设置的了，
下次循环必须重新 FD_ZERO + FD_SET 重建。
（这也是 select 的一个不便之处，epoll 不需要这样反复重建）
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonMistakes() {
    cout << R"(
============================================================
常见错误和陷阱 ⭐⭐⭐⭐⭐
============================================================

【错误1：nfds 没有 +1】
❌ select(maxfd, &readfds, ...);      // 漏了+1
✅ select(maxfd + 1, &readfds, ...);  // 最大fd+1
预防：nfds = 所有被监视 fd 中的最大值 + 1

【错误2：循环里不重新 FD_SET】
❌ 在 while 外面 FD_SET 一次，循环里直接 select
   问题：select 会清掉没就绪的位，第二次循环集合就错了
✅ 每次循环开头 FD_ZERO + 重新 FD_SET 所有 fd
预防：fd_set 每轮重建（select 会修改它）

【错误3：忘记把新 accept 的 fd 加入监视】
❌ accept 后只通信一次，没加入 clients 列表
✅ accept 返回的 conn_fd 要存起来，下轮 FD_SET 进去
预防：新连接的 fd 要纳入监视集合

【错误4：客户端断开后不移除 fd】
❌ recv 返回 0（对方关闭）后还继续监视这个 fd
   问题：已关闭的 fd 会一直"就绪"，导致死循环
✅ recv 返回 <=0 时 close(fd) 并从列表移除
预防：处理好连接关闭，及时清理 fd

【错误5：遍历删除时迭代器失效】
❌ for(auto fd : clients) { ...; clients.erase(...); }  // 迭代器失效
✅ for(auto it=clients.begin(); it!=clients.end();){ ...; it=erase(it); }
预防：边遍历边删除要用 erase 返回的新迭代器

【错误6：maxfd 没及时更新】
❌ 加了新客户端 fd 但没更新 maxfd
   问题：select 检查不到大于 maxfd 的 fd
✅ 每次 FD_SET 时比较更新 maxfd
预防：maxfd 要随时反映当前最大 fd

【错误7：timeout 用法误解】
❌ 以为传 {0,0} 是"永久等待"
✅ NULL=永久阻塞；{0,0}=非阻塞立即返回；{n,0}=最多等n秒
预防：要一直等用 NULL，不是 {0,0}
)" << endl;
}

// ============================================
// 第五部分：函数卡片速查
// ============================================

void showQuickReference() {
    cout << R"(
============================================================
select 多路复用速查表
============================================================

【核心函数】
  select(maxfd+1, &readfds, &writefds, &exceptfds, timeout)
  返回：就绪fd数 / 0超时 / -1出错

【fd_set 四个宏】
  FD_ZERO(&set)        清空
  FD_SET(fd, &set)     加入
  FD_CLR(fd, &set)     移除
  FD_ISSET(fd, &set)   检查是否就绪

【标准编程套路】
  while (true) {
      FD_ZERO(&readfds);                 // 1.清空
      FD_SET(listen_fd, &readfds);       // 2.加监听fd
      for(c:clients) FD_SET(c,&readfds); //   加所有客户端fd
      select(maxfd+1, &readfds, ...);    // 3.等待就绪
      if(FD_ISSET(listen_fd)) accept;    // 4.新连接
      for(c:clients) if(FD_ISSET(c)) recv; //  处理数据
  }

【timeout 三种】
  NULL    一直阻塞等待
  {0,0}   非阻塞，立即返回
  {n,m}   最多等 n 秒 m 微秒

【select vs epoll】
  select  上限1024，每次拷贝+遍历O(n)，跨平台
  epoll   无上限，O(1)，仅Linux，高性能首选

【编译运行】
  g++ -std=c++17 -o 12_select 12_io_multiplexing.cpp
  ./12_select server
  nc 127.0.0.1 8890   (多开测试)
)" << endl;
}

// ============================================
// 第六部分：练习题（不提供答案）
// ============================================

void showExercises() {
    cout << R"(
============================================================
练习题（独立思考，不看答案）
============================================================

1. 【基础】统计在线人数
   要求：每当有客户端连接/断开，打印当前在线客户端总数
   提示：clients.size() 就是在线数

2. 【广播】群聊功能
   要求：一个客户端发的消息，转发给所有其他在线客户端
   提示：收到消息后遍历 clients，给除发送者外的所有 fd send

3. 【超时处理】带超时的 select
   要求：用 timeout={5,0}，5秒没任何活动就打印"服务器空闲中"
   提示：select 返回 0 表示超时，处理这种情况

4. 【命令处理】简单指令
   要求：客户端发 "list" 返回在线人数，发 "quit" 断开该客户端
   提示：recv 后判断 buffer 内容，分别处理

5. 【对比实验】select vs 多线程
   要求：回顾第66天的多线程方案，对比两种方式处理100个连接的差异
   提示：思考线程数、内存占用、CPU 切换开销
)" << endl;
}

int main(int argc, char* argv[]) {
    cout << string(60, '=') << endl;
    cout << "I/O 多路复用 (select)" << endl;
    cout << string(60, '=') << endl;

    if (argc >= 2 && string(argv[1]) == "server") {
        runServer();
    } else {
        showConcepts();
        showFunctionCards();
        showCommonMistakes();
        showQuickReference();
        showExercises();
        cout << "\n【如何测试多客户端】" << endl;
        cout << "  终端1：./12_select server" << endl;
        cout << "  终端2/3/4：nc 127.0.0.1 " << PORT
             << "（多开几个，输入文字看回显）" << endl;
    }

    return 0;
}
