// ============================================
// 10_tcp_socket.cpp
// 日期：2026-06-10（第67天）
// 主题：TCP Socket 编程 - 服务端/客户端
// 难度：⭐⭐⭐ 高级
// 注意：需在 Linux/WSL 环境编译运行
// 编译：g++ -std=c++17 -o 10_tcp 10_tcp_socket.cpp
// 运行：先开服务端 ./10_tcp server，再开客户端 ./10_tcp client
// ============================================

/*
【核心概念】简要说明

是什么？
- Socket（套接字）：网络通信的端点，是进程间跨网络通信的接口
- TCP：面向连接、可靠传输的协议（保证数据不丢、不乱序）

为什么需要？
- 进程间通信（IPC）只能在同一台机器，Socket 能跨机器通信
- TCP 提供"可靠的字节流"，像打电话（先建立连接，再通话）

核心特点？
- 面向连接：通信前必须先"三次握手"建立连接
- 可靠传输：丢包自动重传，保证顺序
- 字节流：数据像水流一样连续，没有消息边界（需自己分帧）

【生活类比】TCP 像打电话：
  1. 拨号（connect）→ 对方接听（accept）→ 建立连接
  2. 双方通话（send/recv）
  3. 挂电话（close）

【与你学过的知识对照】
  Qt 的 QNetworkAccessManager → 高层封装，底层就是 socket
  第02天的管道(pipe)          → 同机器进程通信；socket 能跨机器
*/

#include <iostream>
#include <cstring>
#include <unistd.h>      // close(), read(), write()
#include <sys/socket.h>  // socket(), bind(), listen(), accept(), connect()
#include <netinet/in.h>  // sockaddr_in 结构体
#include <arpa/inet.h>   // htons(), inet_addr(), inet_pton()
using namespace std;

const int PORT = 8888;           // 服务端监听端口
const char* SERVER_IP = "127.0.0.1";  // 本机回环地址

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

void showConcepts() {
    cout << R"(
============================================================
TCP Socket - 核心概念
============================================================

【TCP 通信完整流程图】

   服务端 (Server)              客户端 (Client)
  ┌──────────────┐
  │ socket()     │ 创建套接字
  ├──────────────┤
  │ bind()       │ 绑定IP和端口
  ├──────────────┤
  │ listen()     │ 开始监听
  ├──────────────┤
  │ accept()     │ 等待连接（阻塞）   ┌──────────────┐
  │      ◄───────┼───────────────────│ socket()     │ 创建套接字
  │              │                   ├──────────────┤
  │              │   三次握手         │ connect()    │ 发起连接
  │      ◄───────┼═══════════════════┤              │
  ├──────────────┤                   ├──────────────┤
  │ recv()/send()│ ◄═══数据收发═════► │ send()/recv()│
  ├──────────────┤                   ├──────────────┤
  │ close()      │                   │ close()      │
  └──────────────┘                   └──────────────┘

【服务端五大步骤】⭐⭐⭐⭐⭐
  1. socket()  创建套接字（拿到一个文件描述符 fd）
  2. bind()    把套接字绑定到 IP+端口（"我在哪个门牌号收信"）
  3. listen()  开始监听（"我准备好接客了"）
  4. accept()  接受连接（阻塞等待客户端，返回新的通信 fd）
  5. recv/send 收发数据，结束后 close()

【客户端三大步骤】
  1. socket()  创建套接字
  2. connect() 连接服务端（指定服务端 IP+端口）
  3. send/recv 收发数据，结束后 close()

【关键：accept 返回新 fd】
  - 监听 fd（listen_fd）：专门用来"接客"，一直监听
  - 通信 fd（conn_fd）：accept 返回的，用来和某个客户端"对话"
  - 这样服务端能同时和多个客户端通信（每个客户端一个 conn_fd）
// ============================================
// 第一部分补充：核心函数卡片 ⭐⭐⭐⭐⭐
// ============================================

void showFunctionCards() {
    cout << R"(
============================================================
TCP Socket 核心函数卡片
============================================================

【函数1：socket() - 创建套接字】
语法：int socket(int domain, int type, int protocol);
参数：
  1. domain   - 地址族：AF_INET(IPv4) / AF_INET6(IPv6)
  2. type     - 类型：SOCK_STREAM(TCP) / SOCK_DGRAM(UDP)
  3. protocol - 协议：通常传 0（自动选择）
返回：成功返回套接字文件描述符(fd)，失败返回 -1
示例：int fd = socket(AF_INET, SOCK_STREAM, 0);  // 创建TCP套接字

────────────────────────────────────

【函数2：bind() - 绑定地址】
语法：int bind(int fd, const sockaddr* addr, socklen_t addrlen);
参数：
  1. fd      - socket() 返回的套接字
  2. addr    - 指向 sockaddr_in 结构（含IP和端口）
  3. addrlen - 地址结构大小：sizeof(sockaddr_in)
返回：成功返回 0，失败返回 -1
作用：告诉系统"这个套接字使用哪个 IP 和端口"

────────────────────────────────────

【函数3：listen() - 开始监听】
语法：int listen(int fd, int backlog);
参数：
  1. fd      - 已 bind 的套接字
  2. backlog - 等待连接队列的最大长度（如 5）
返回：成功返回 0，失败返回 -1
作用：把套接字变成"被动监听"状态，准备接受连接

────────────────────────────────────

【函数4：accept() - 接受连接】⭐⭐⭐⭐⭐
语法：int accept(int fd, sockaddr* addr, socklen_t* addrlen);
参数：
  1. fd      - 监听套接字
  2. addr    - 输出参数：接收客户端地址（不关心可传 NULL）
  3. addrlen - 输入输出参数：地址结构大小
返回：成功返回【新的通信套接字 fd】，失败返回 -1
作用：阻塞等待客户端连接，连上后返回专用通信 fd
重点：返回的是新 fd，监听 fd 继续监听别的客户端！

────────────────────────────────────

【函数5：connect() - 发起连接（客户端）】
语法：int connect(int fd, const sockaddr* addr, socklen_t addrlen);
参数：
  1. fd      - 客户端套接字
  2. addr    - 服务端地址（IP+端口）
  3. addrlen - 地址结构大小
返回：成功返回 0，失败返回 -1
作用：客户端主动连接服务端，触发三次握手

────────────────────────────────────

【函数6：send()/recv() - 收发数据】
语法：
  ssize_t send(int fd, const void* buf, size_t len, int flags);
  ssize_t recv(int fd, void* buf, size_t len, int flags);
参数：
  1. fd    - 通信套接字
  2. buf   - 数据缓冲区
  3. len   - 数据长度
  4. flags - 通常传 0
返回：实际发送/接收的字节数；recv 返回 0 表示对方关闭连接
说明：也可以用 write()/read()（socket fd 本质是文件描述符）

────────────────────────────────────

【函数7：字节序转换】⭐⭐⭐⭐⭐
  htons(x)  host to network short  主机序→网络序（16位，端口用）
  htonl(x)  host to network long   主机序→网络序（32位，IP用）
  ntohs(x)  network to host short  网络序→主机序（16位）
  ntohl(x)  network to host long   网络序→主机序（32位）
为什么需要：网络统一用"大端序"，但你的机器可能是小端序，
           不转换会导致端口/IP 解析错乱（详见深入理解）

【地址转换】
  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr)  字符串IP→二进制
  inet_ntop(AF_INET, &addr.sin_addr, buf, len)     二进制→字符串IP
)" << endl;
}

// ============================================
// 第二部分：基础操作实践 - TCP 服务端/客户端
// ============================================

// --------------------------------------------
// TCP 服务端：完整五步流程
// --------------------------------------------
void runServer() {
    cout << "\n[服务端] 启动中..." << endl;

    // 步骤1：创建套接字
    // AF_INET=IPv4, SOCK_STREAM=TCP, 0=自动选协议
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket 失败");
        return;
    }

    // 设置端口复用：避免程序重启时报"地址已被占用"
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 步骤2：准备地址结构并 bind
    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));  // 清零，避免脏数据
    serverAddr.sin_family = AF_INET;             // IPv4
    serverAddr.sin_port = htons(PORT);           // 端口（主机序→网络序）
    serverAddr.sin_addr.s_addr = INADDR_ANY;     // 监听本机所有网卡

    if (bind(listen_fd, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("bind 失败");
        close(listen_fd);
        return;
    }

    // 步骤3：开始监听，backlog=5（等待队列最多5个）
    if (listen(listen_fd, 5) < 0) {
        perror("listen 失败");
        close(listen_fd);
        return;
    }
    cout << "[服务端] 监听端口 " << PORT << "，等待客户端连接..." << endl;

    // 步骤4：接受连接（阻塞，直到客户端连上）
    sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    int conn_fd = accept(listen_fd, (sockaddr*)&clientAddr, &clientLen);
    if (conn_fd < 0) {
        perror("accept 失败");
        close(listen_fd);
        return;
    }

    // 打印客户端地址（二进制IP→字符串）
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
    cout << "[服务端] 客户端 " << clientIP << ":"
         << ntohs(clientAddr.sin_port) << " 已连接" << endl;

    // 步骤5：收发数据
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    ssize_t n = recv(conn_fd, buffer, sizeof(buffer) - 1, 0);
    if (n > 0) {
        cout << "[服务端] 收到消息：" << buffer << endl;
        // 回复客户端
        const char* reply = "你好，客户端！我收到了你的消息。";
        send(conn_fd, reply, strlen(reply), 0);
        cout << "[服务端] 已回复" << endl;
    }

    // 关闭两个套接字
    close(conn_fd);    // 关闭通信套接字
    close(listen_fd);  // 关闭监听套接字
    cout << "[服务端] 连接关闭" << endl;
}

// --------------------------------------------
// TCP 客户端：三步流程
// --------------------------------------------
void runClient() {
    cout << "\n[客户端] 启动中..." << endl;

    // 步骤1：创建套接字
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket 失败");
        return;
    }

    // 准备服务端地址
    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    // 把字符串IP "127.0.0.1" 转成二进制写入 sin_addr
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

    // 步骤2：连接服务端
    if (connect(sock_fd, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("connect 失败（确认服务端已启动？）");
        close(sock_fd);
        return;
    }
    cout << "[客户端] 已连接到服务端 " << SERVER_IP << ":" << PORT << endl;

    // 步骤3：发送数据并接收回复
    const char* msg = "Hello Server, 我是客户端！";
    send(sock_fd, msg, strlen(msg), 0);
    cout << "[客户端] 已发送：" << msg << endl;

    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    ssize_t n = recv(sock_fd, buffer, sizeof(buffer) - 1, 0);
    if (n > 0) {
        cout << "[客户端] 收到回复：" << buffer << endl;
    }

    close(sock_fd);
    cout << "[客户端] 连接关闭" << endl;
}

// ============================================
// 第三部分：深入理解（选学）🔍
// ============================================

/*
==============================================
🔍 深入理解1：为什么需要字节序转换？（选学）⭐⭐⭐⭐⭐
==============================================

【什么是字节序】
  多字节数据（如端口 8888 占2字节）在内存里有两种存放顺序：
    大端序(Big-Endian)：高位字节在前   [0x22][0xB8]
    小端序(Little-Endian)：低位字节在前 [0xB8][0x22]
  (8888 = 0x22B8)

  x86/ARM 大多是小端序，但网络协议规定统一用大端序（网络字节序）。

【不转换的后果】
  小端机器上 port=8888 直接发出去 → 对方按大端解析 → 变成 47138！
  连接到了错误的端口，通信失败。

【解决】
  发送前：htons(8888)  把主机序转成网络序
  接收后：ntohs(...)   把网络序转回主机序
  口诀：to network 用 hton*，to host 用 ntoh*；
       s=short(端口16位)，l=long(IP 32位)

==============================================
🔍 深入理解2：为什么 accept 要返回新 fd？（选学）
==============================================

监听套接字(listen_fd)的职责是"接客"，它要一直监听新连接。
如果用它直接通信，就没法同时接待其他客户端了。

所以 accept 返回一个全新的 conn_fd 专门和这个客户端对话，
listen_fd 继续监听。这样服务端可以：
  while (true) {
      int conn_fd = accept(listen_fd, ...);  // 接一个新客户端
      // 用 conn_fd 和这个客户端通信（可以交给新线程/进程处理）
  }
  一个 listen_fd，无数个 conn_fd → 支持多客户端。

==============================================
🔍 深入理解3：TCP 是字节流，没有消息边界（选学）⭐⭐⭐⭐
==============================================

TCP 把数据当成连续的字节流，不保证"一次 send 对应一次 recv"：
  发送方：send("Hello"); send("World");
  接收方：可能一次 recv 收到 "HelloWorld"（粘包）
         也可能分两次收到 "Hel" 和 "loWorld"（拆包）

这就是著名的"粘包/拆包"问题。解决方法：
  1. 固定长度消息
  2. 消息前加长度头（先读4字节长度，再读对应长度的数据）
  3. 特殊分隔符（如 \n 分隔）
  （UDP 没有这个问题，因为它是"数据报"，保留消息边界）
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonMistakes() {
    cout << R"(
============================================================
常见错误和陷阱 ⭐⭐⭐⭐⭐
============================================================

【错误1：端口/IP 忘记字节序转换】
❌ serverAddr.sin_port = PORT;          // 直接赋值
✅ serverAddr.sin_port = htons(PORT);   // 转网络序
预防：端口用 htons，IP 用 inet_pton（内部已处理字节序）

【错误2：sockaddr_in 没有清零】
❌ sockaddr_in addr; addr.sin_family=...;  // 其他字段是脏数据
✅ memset(&addr, 0, sizeof(addr)); 再赋值
预防：地址结构使用前必须 memset 清零

【错误3：bind 报"Address already in use"】
❌ 程序刚关闭重启，端口处于 TIME_WAIT 状态，bind 失败
✅ setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
预防：bind 前设置 SO_REUSEADDR 端口复用

【错误4：用 listen_fd 收发数据】
❌ recv(listen_fd, ...);  // listen_fd 是监听用的，不能通信
✅ recv(conn_fd, ...);    // 用 accept 返回的 conn_fd
预防：分清监听 fd 和通信 fd 的职责

【错误5：忘记 recv 返回 0 表示对方关闭】
❌ 不判断返回值，对方关闭后死循环读到空数据
✅ ssize_t n = recv(...); if (n == 0) 对方关闭; if (n < 0) 出错;
预防：recv 返回 0 = 连接正常关闭，<0 = 出错，>0 = 收到n字节

【错误6：客户端先启动，连不上】
❌ 先开客户端 → connect 失败（服务端还没监听）
✅ 必须先启动服务端 listen，再启动客户端 connect
预防：服务端先就绪，客户端后连接

【错误7：地址结构指针类型未强转】
❌ bind(fd, &serverAddr, ...);  // 类型不匹配编译错误
✅ bind(fd, (sockaddr*)&serverAddr, ...);  // 强转为通用 sockaddr*
预防：sockaddr_in* 要强转成 sockaddr* 传入
)" << endl;
}

// ============================================
// 第五部分：函数卡片速查
// ============================================

void showQuickReference() {
    cout << R"(
============================================================
TCP Socket 速查表
============================================================

【服务端五步】
  socket(AF_INET, SOCK_STREAM, 0)        创建
  bind(fd, (sockaddr*)&addr, len)        绑定IP+端口
  listen(fd, backlog)                    监听
  accept(fd, &cliaddr, &len)             接受连接→返回conn_fd
  recv/send(conn_fd, ...)  →  close()    收发→关闭

【客户端三步】
  socket(AF_INET, SOCK_STREAM, 0)        创建
  connect(fd, (sockaddr*)&addr, len)     连接服务端
  send/recv(fd, ...)  →  close()         收发→关闭

【地址结构 sockaddr_in】
  memset(&addr, 0, sizeof(addr));        清零
  addr.sin_family = AF_INET;             IPv4
  addr.sin_port = htons(PORT);           端口（转网络序）
  addr.sin_addr.s_addr = INADDR_ANY;     服务端：所有网卡
  inet_pton(AF_INET, "1.2.3.4", &addr.sin_addr);  客户端：指定IP

【字节序】
  htons/htonl  主机→网络   ntohs/ntohl  网络→主机
  s=端口(16位)  l=IP(32位)

【recv 返回值】
  >0 收到字节数   =0 对方关闭   <0 出错

【编译运行】
  g++ -std=c++17 -o 10_tcp 10_tcp_socket.cpp
  ./10_tcp server   (终端1)
  ./10_tcp client   (终端2)
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

1. 【基础】修改回显内容
   要求：让服务端把客户端发来的消息原样返回（echo），
        客户端打印收到的回显
   提示：服务端 recv 后直接 send 收到的 buffer

2. 【循环服务】持续接受多个客户端
   要求：服务端用 while 循环不断 accept，
        处理完一个客户端再接待下一个
   提示：accept 放在 while 里，每次处理完 close(conn_fd) 不关 listen_fd

3. 【多轮对话】聊天模式
   要求：客户端和服务端能多次往返收发（不是一次就关闭），
        客户端输入 "quit" 时双方关闭连接
   提示：用 while 循环收发，检测到 "quit" 退出

4. 【并发服务】多线程处理客户端
   要求：结合第66天 pthread，每 accept 一个客户端就开一个线程处理，
        实现同时服务多个客户端
   提示：accept 返回 conn_fd 后 pthread_create，conn_fd 作为参数传入

5. 【粘包处理】带长度头的消息协议
   要求：发送时先发4字节长度，再发数据；接收时先读长度再读数据
   提示：解决 TCP 字节流没有消息边界的问题
)" << endl;
}

int main(int argc, char* argv[]) {
    cout << string(60, '=') << endl;
    cout << "TCP Socket 编程" << endl;
    cout << string(60, '=') << endl;

    // 根据命令行参数决定运行服务端还是客户端
    if (argc >= 2 && string(argv[1]) == "server") {
        runServer();
    } else if (argc >= 2 && string(argv[1]) == "client") {
        runClient();
    } else {
        // 没有参数：只展示概念，不实际运行网络
        showConcepts();
        showFunctionCards();
        showCommonMistakes();
        showQuickReference();
        showExercises();
        cout << "\n【如何运行实际网络通信】" << endl;
        cout << "  终端1（先启动服务端）：./10_tcp server" << endl;
        cout << "  终端2（再启动客户端）：./10_tcp client" << endl;
    }

    return 0;
}
