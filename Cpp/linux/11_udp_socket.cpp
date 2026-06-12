// ============================================
// 11_udp_socket.cpp
// 日期：2026-06-10（第67天）
// 主题：UDP Socket 编程 - 无连接通信
// 难度：⭐⭐⭐ 高级
// 注意：需在 Linux/WSL 环境编译运行
// 编译：g++ -std=c++17 -o 11_udp 11_udp_socket.cpp
// 运行：先开服务端 ./11_udp server，再开客户端 ./11_udp client
// ============================================

/*
【核心概念】简要说明

是什么？
- UDP：用户数据报协议，无连接、不可靠但高效的传输协议
- 不需要建立连接，直接发送数据报（datagram）

为什么需要？
- TCP 可靠但开销大（握手、确认、重传）
- 某些场景"快"比"可靠"更重要：视频直播、游戏、DNS、语音通话
- 丢一两帧无所谓，但不能卡顿

核心特点？
- 无连接：不需要 connect/accept，直接 sendto/recvfrom
- 不可靠：不保证送达、不保证顺序、不重传
- 保留消息边界：一次 sendto 对应一次 recvfrom（无粘包）

【生活类比】UDP 像寄明信片：
  - 写上地址直接投递（不需先建立连接）
  - 可能丢失、可能乱序到达
  - 但简单快速，不用打电话确认

【TCP vs UDP 一句话】
  TCP = 打电话（先拨号建立连接，保证听清每个字）
  UDP = 寄明信片（直接寄，可能丢，但快）

【与你学过的知识对照】
  第10天 TCP socket  → 面向连接、可靠
  本文件 UDP socket  → 无连接、高效，少了 listen/accept/connect
*/

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
using namespace std;

const int PORT = 9999;
const char* SERVER_IP = "127.0.0.1";

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

void showConcepts() {
    cout << R"(
============================================================
UDP Socket - 核心概念
============================================================

【UDP 通信流程图（对比 TCP，简单很多）】

   服务端 (Server)              客户端 (Client)
  ┌──────────────┐
  │ socket()     │ 创建(SOCK_DGRAM)
  ├──────────────┤
  │ bind()       │ 绑定IP和端口      ┌──────────────┐
  ├──────────────┤                   │ socket()     │ 创建
  │ recvfrom()   │ 等待数据报        │              │
  │      ◄───────┼───────────────────│ sendto()     │ 直接发送
  │ sendto()     │ ──────────────────►│ recvfrom()   │ 接收回复
  ├──────────────┤                   ├──────────────┤
  │ close()      │                   │ close()      │
  └──────────────┘                   └──────────────┘

  注意：没有 listen / accept / connect！
       UDP 不建立连接，直接收发数据报。

【TCP vs UDP 全面对比】⭐⭐⭐⭐⭐
┌──────────────┬──────────────────┬──────────────────┐
│ 对比项       │ TCP              │ UDP              │
├──────────────┼──────────────────┼──────────────────┤
│ 连接         │ 面向连接(握手)   │ 无连接           │
│ 可靠性       │ 可靠(重传)       │ 不可靠(可能丢)   │
│ 顺序         │ 保证顺序         │ 不保证顺序       │
│ 速度         │ 较慢(开销大)     │ 快(开销小)       │
│ 消息边界     │ 字节流(会粘包)   │ 数据报(有边界)   │
│ 套接字类型   │ SOCK_STREAM      │ SOCK_DGRAM       │
│ 收发函数     │ send/recv        │ sendto/recvfrom  │
│ 适用场景     │ 文件/网页/邮件   │ 视频/游戏/DNS    │
└──────────────┴──────────────────┴──────────────────┘

【关键：sendto/recvfrom 每次都带地址】
  TCP 建立连接后，send/recv 不需要再指定对方地址（连接里有）
  UDP 无连接，所以每次 sendto 都要指定"发给谁"，
       recvfrom 也会告诉你"这个数据报是谁发来的"
============================================================
)" << endl;
}

// ============================================
// 第一部分补充：核心函数卡片 ⭐⭐⭐⭐⭐
// ============================================

void showFunctionCards() {
    cout << R"(
============================================================
UDP Socket 核心函数卡片
============================================================

【函数1：socket() - 创建 UDP 套接字】
语法：int socket(int domain, int type, int protocol);
区别：type 用 SOCK_DGRAM（数据报），不是 SOCK_STREAM
示例：int fd = socket(AF_INET, SOCK_DGRAM, 0);  // UDP！

────────────────────────────────────

【函数2：sendto() - 发送数据报】⭐⭐⭐⭐⭐
语法：
  ssize_t sendto(int fd, const void* buf, size_t len, int flags,
                 const sockaddr* dest_addr, socklen_t addrlen);
参数：
  1. fd        - UDP 套接字
  2. buf       - 要发送的数据
  3. len       - 数据长度
  4. flags     - 通常 0
  5. dest_addr - 【目标地址】发给谁（关键！TCP的send没有这个）
  6. addrlen   - 地址结构大小
返回：发送的字节数，失败 -1
重点：每次发送都要指定目标地址（因为无连接）

────────────────────────────────────

【函数3：recvfrom() - 接收数据报】⭐⭐⭐⭐⭐
语法：
  ssize_t recvfrom(int fd, void* buf, size_t len, int flags,
                   sockaddr* src_addr, socklen_t* addrlen);
参数：
  1. fd       - UDP 套接字
  2. buf      - 接收缓冲区
  3. len      - 缓冲区大小
  4. flags    - 通常 0
  5. src_addr - 【输出】数据报是谁发来的（可用于回复）
  6. addrlen  - 输入输出：地址结构大小
返回：接收的字节数，失败 -1
重点：src_addr 会被填上发送方地址，可以直接拿来回复

────────────────────────────────────

【UDP vs TCP 函数对照】
  TCP                      UDP
  socket(SOCK_STREAM)      socket(SOCK_DGRAM)
  bind()                   bind()           (服务端都要)
  listen()                 （无）
  accept()                 （无）
  connect()                （无，可选）
  send(fd,...)             sendto(fd,...,addr)   ← 多了地址
  recv(fd,...)             recvfrom(fd,...,addr) ← 多了地址
  close()                  close()
)" << endl;
}

// ============================================
// 第二部分：基础操作实践 - UDP 回显(echo)
// ============================================

// --------------------------------------------
// UDP 服务端：接收数据报并原样回显
// --------------------------------------------
void runServer() {
    cout << "\n[UDP服务端] 启动中..." << endl;

    // 步骤1：创建 UDP 套接字（SOCK_DGRAM）
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) { perror("socket 失败"); return; }

    // 步骤2：绑定地址（服务端必须 bind，客户端才知道发到哪）
    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(fd, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("bind 失败");
        close(fd);
        return;
    }
    cout << "[UDP服务端] 监听端口 " << PORT << "，等待数据报..." << endl;

    // 步骤3：循环接收并回显（注意：没有 listen/accept！）
    char buffer[1024];
    sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    memset(buffer, 0, sizeof(buffer));
    // recvfrom 会把发送方地址填进 clientAddr
    ssize_t n = recvfrom(fd, buffer, sizeof(buffer) - 1, 0,
                         (sockaddr*)&clientAddr, &clientLen);
    if (n > 0) {
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
        cout << "[UDP服务端] 收到来自 " << clientIP << ":"
             << ntohs(clientAddr.sin_port) << " 的消息：" << buffer << endl;

        // 原样回显：用 recvfrom 得到的 clientAddr 作为目标地址
        sendto(fd, buffer, n, 0, (sockaddr*)&clientAddr, clientLen);
        cout << "[UDP服务端] 已回显" << endl;
    }

    close(fd);
    cout << "[UDP服务端] 关闭" << endl;
}

// --------------------------------------------
// UDP 客户端：发送数据报并接收回显
// --------------------------------------------
void runClient() {
    cout << "\n[UDP客户端] 启动中..." << endl;

    // 步骤1：创建 UDP 套接字（客户端不需要 bind）
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) { perror("socket 失败"); return; }

    // 准备服务端地址
    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

    // 步骤2：直接 sendto 发送（不需要 connect！）
    const char* msg = "Hello UDP Server, 这是数据报！";
    sendto(fd, msg, strlen(msg), 0,
           (sockaddr*)&serverAddr, sizeof(serverAddr));
    cout << "[UDP客户端] 已发送：" << msg << endl;

    // 步骤3：接收服务端回显
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    sockaddr_in fromAddr;
    socklen_t fromLen = sizeof(fromAddr);
    ssize_t n = recvfrom(fd, buffer, sizeof(buffer) - 1, 0,
                        (sockaddr*)&fromAddr, &fromLen);
    if (n > 0) {
        cout << "[UDP客户端] 收到回显：" << buffer << endl;
    }

    close(fd);
    cout << "[UDP客户端] 关闭" << endl;
}

// ============================================
// 第三部分：深入理解（选学）🔍
// ============================================

/*
==============================================
🔍 深入理解1：UDP 为什么"不可靠"却被广泛使用？（选学）
==============================================

"不可靠"听起来像缺点，但在某些场景反而是优点：

【视频直播/语音通话】
  假设第100帧丢了，TCP 会停下来重传第100帧，导致后面所有帧卡住等待。
  但对直播来说，第100帧的内容已经过时了，重传它毫无意义！
  UDP 直接跳过丢失的帧，继续播放后面的，宁可花屏一下也不卡顿。

【在线游戏】
  玩家位置每秒更新60次，丢一两个位置包无所谓（下一个包马上来）。
  追求的是"最新"而不是"完整"。

【DNS 查询】
  一问一答，数据小，UDP 一个包搞定。失败了重新问一次即可，
  不值得为单次查询建立 TCP 连接（握手开销比查询本身还大）。

结论：UDP 把"可靠性"的控制权交给应用层，需要可靠就自己实现，
     不需要就享受高效。

==============================================
🔍 深入理解2：UDP 服务端为什么不需要 accept？（选学）
==============================================

TCP 是面向连接的，accept 的本质是"完成握手、建立一条连接通道"。
UDP 无连接，每个数据报都是独立的，自带发送方地址，
所以不需要"建立连接"这个步骤，recvfrom 直接收包即可。

一个 UDP 套接字就能接收来自任意客户端的数据报，
通过 recvfrom 的 src_addr 区分是谁发来的。

==============================================
🔍 深入理解3：客户端为什么不用 bind？（选学）
==============================================

服务端必须 bind 固定端口（如 9999），因为客户端要知道"往哪发"。
客户端不 bind，系统会在第一次 sendto 时自动分配一个临时端口
（ephemeral port，通常 32768~60999），用完即弃。
服务端通过 recvfrom 的 src_addr 就能知道这个临时端口，用来回复。
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonMistakes() {
    cout << R"(
============================================================
常见错误和陷阱 ⭐⭐⭐⭐⭐
============================================================

【错误1：socket 类型用错】
❌ socket(AF_INET, SOCK_STREAM, 0);  // 这是 TCP！
✅ socket(AF_INET, SOCK_DGRAM, 0);   // UDP 用 SOCK_DGRAM
预防：UDP = SOCK_DGRAM，记住"数据报 datagram"

【错误2：sendto 忘记传目标地址】
❌ send(fd, buf, len, 0);              // UDP 不能用 send（没连接）
✅ sendto(fd, buf, len, 0, &addr, len); // 必须指定发给谁
预防：UDP 用 sendto/recvfrom，每次带地址

【错误3：服务端不 bind】
❌ 服务端创建 socket 后直接 recvfrom（没绑定端口）
✅ 服务端必须 bind 固定端口，客户端才知道往哪发
预防：UDP 服务端要 bind，客户端可以不 bind

【错误4：recvfrom 的 addrlen 没初始化】
❌ socklen_t len; recvfrom(..., &len);  // len 是随机值
✅ socklen_t len = sizeof(addr); recvfrom(..., &len);
预防：addrlen 作为输入要先设成结构体大小

【错误5：以为 UDP 能保证送达】
❌ sendto 后就认为对方一定收到了
✅ UDP 不保证送达，需要可靠性就自己加确认/重传机制
预防：UDP 不可靠，关键数据要应用层确认

【错误6：缓冲区太小导致数据报被截断】
❌ char buf[10]; recvfrom(fd, buf, 10, ...);  // 大数据报被截断丢失
✅ 缓冲区要足够大（如 1024 或更大），UDP 超出部分会被丢弃
预防：UDP 数据报要一次性读完，缓冲区开大些

【错误7：回复时用错地址】
❌ 用预设的某个地址回复，而不是 recvfrom 得到的发送方地址
✅ 用 recvfrom 填充的 clientAddr 作为 sendto 的目标
预防：回复要发回"刚才是谁发来的"那个地址
)" << endl;
}

// ============================================
// 第五部分：函数卡片速查
// ============================================

void showQuickReference() {
    cout << R"(
============================================================
UDP Socket 速查表
============================================================

【服务端流程】
  socket(AF_INET, SOCK_DGRAM, 0)         创建UDP套接字
  bind(fd, (sockaddr*)&addr, len)        绑定端口
  recvfrom(fd, buf, len, 0, &cli, &clen) 收数据报+获取发送方
  sendto(fd, buf, n, 0, &cli, clen)      回复发送方
  close(fd)

【客户端流程】
  socket(AF_INET, SOCK_DGRAM, 0)         创建（不用bind）
  sendto(fd, buf, len, 0, &srv, slen)    直接发送（不用connect）
  recvfrom(fd, buf, len, 0, &from, &fl)  收回复
  close(fd)

【sendto/recvfrom 比 send/recv 多了什么】
  多了"地址参数"——因为无连接，每次都要指定/获取对方地址

【TCP vs UDP 选型】
  要可靠、有序、传文件/网页    → TCP
  要快、容忍丢包、实时音视频   → UDP

【编译运行】
  g++ -std=c++17 -o 11_udp 11_udp_socket.cpp
  ./11_udp server   (终端1)
  ./11_udp client   (终端2)
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

1. 【基础】大小写转换服务
   要求：客户端发送小写字符串，服务端转成大写回显
   提示：服务端收到后用循环或 toupper 转换再 sendto

2. 【循环服务】持续接收多个数据报
   要求：服务端用 while 循环不断 recvfrom，能服务多个客户端
   提示：UDP 一个套接字就能收所有客户端，recvfrom 放 while 里

3. 【多轮通信】UDP 聊天
   要求：客户端能多次发送消息，输入 "quit" 退出
   提示：客户端用 while 循环 sendto + recvfrom

4. 【丢包模拟】观察不可靠性
   要求：客户端连续 sendto 1000 个编号数据报，服务端统计实际收到几个
   提示：本机回环一般不丢，可思考跨网络/高负载时会丢包

5. 【对比实验】TCP vs UDP 性能
   要求：分别用 TCP 和 UDP 发送 10000 条小消息，对比耗时
   提示：UDP 通常更快（没有握手和确认开销）
)" << endl;
}

int main(int argc, char* argv[]) {
    cout << string(60, '=') << endl;
    cout << "UDP Socket 编程" << endl;
    cout << string(60, '=') << endl;

    if (argc >= 2 && string(argv[1]) == "server") {
        runServer();
    } else if (argc >= 2 && string(argv[1]) == "client") {
        runClient();
    } else {
        showConcepts();
        showFunctionCards();
        showCommonMistakes();
        showQuickReference();
        showExercises();
        cout << "\n【如何运行实际通信】" << endl;
        cout << "  终端1（先启动服务端）：./11_udp server" << endl;
        cout << "  终端2（再启动客户端）：./11_udp client" << endl;
    }

    return 0;
}
