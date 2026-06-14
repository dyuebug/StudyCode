// ============================================
// 多线程 TCP 回显服务器 - 综合项目
// 日期：2026-06-13（第68天）
// 难度：⭐⭐⭐ 综合实战
// 技术：socket + pthread + 互斥锁
// ============================================

/*
【项目功能】
- TCP 服务器监听 8080 端口
- 接受多个客户端连接
- 每个客户端分配独立线程处理
- 回显功能：客户端发什么，服务器返回什么
- 维护在线客户端列表（加锁保护）
- 优雅关闭：客户端断开时自动清理资源

【技术串联】
1. socket：TCP 服务端五步流程
2. pthread：每连接一线程（accept 后 pthread_create）
3. 互斥锁：保护共享数据（在线客户端列表）
4. 资源管理：线程结束时关闭 fd、从列表移除

【编译运行】
编译：g++ -o server 13_mini_server.cpp -pthread
运行：./server
测试：telnet localhost 8080  或  nc localhost 8080
*/

#include <iostream>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

using namespace std;

// --------------------------------------------
// 全局共享数据：在线客户端列表
// --------------------------------------------

struct ClientInfo {
    int fd;           // 客户端 socket fd
    pthread_t tid;    // 线程 ID
    string addr;      // 客户端地址
};

vector<ClientInfo> online_clients;  // 在线客户端列表（共享数据）
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;  // 保护列表的互斥锁


// --------------------------------------------
// 工具函数：添加客户端到在线列表
// --------------------------------------------
void add_client(int fd, pthread_t tid, const string& addr) {
    pthread_mutex_lock(&clients_mutex);  // 加锁

    ClientInfo info;
    info.fd = fd;
    info.tid = tid;
    info.addr = addr;
    online_clients.push_back(info);

    cout << "[INFO] 客户端 " << addr << " 已连接，当前在线："
         << online_clients.size() << " 人" << endl;

    pthread_mutex_unlock(&clients_mutex);  // 解锁
}

// --------------------------------------------
// 工具函数：从在线列表移除客户端
// --------------------------------------------
void remove_client(int fd) {
    pthread_mutex_lock(&clients_mutex);

    for (auto it = online_clients.begin(); it != online_clients.end(); ++it) {
        if (it->fd == fd) {
            cout << "[INFO] 客户端 " << it->addr << " 已断开，当前在线："
                 << (online_clients.size() - 1) << " 人" << endl;
            online_clients.erase(it);
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

// --------------------------------------------
// 工作线程函数：处理单个客户端
// --------------------------------------------
void* client_handler(void* arg) {
    int client_fd = *(int*)arg;  // 获取客户端 fd
    delete (int*)arg;            // 释放动态分配的内存

    char buffer[1024];

    while (true) {
        memset(buffer, 0, sizeof(buffer));

        // 接收数据
        int n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

        if (n > 0) {
            // 收到数据，回显
            buffer[n] = '\0';
            cout << "[RECV] fd=" << client_fd << ": " << buffer;

            send(client_fd, buffer, n, 0);  // 回显

        } else if (n == 0) {
            // 客户端关闭连接
            cout << "[INFO] 客户端 fd=" << client_fd << " 关闭连接" << endl;
            break;
        } else {
            // 错误
            perror("recv");
            break;
        }
    }

    // 清理资源
    remove_client(client_fd);  // 从在线列表移除
    close(client_fd);          // 关闭 socket

    return NULL;
}


// ============================================
// 主函数：TCP 服务端主循环
// ============================================
int main() {
    cout << "==========================================================\n";
    cout << "多线程 TCP 回显服务器\n";
    cout << "==========================================================\n\n";

    // --------------------------------------------
    // 第1步：创建 socket
    // --------------------------------------------
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return 1;
    }
    cout << "[1/5] socket 创建成功" << endl;

    // 设置地址复用（避免重启时"地址已被占用"）
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // --------------------------------------------
    // 第2步：绑定地址
    // --------------------------------------------
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);           // 端口 8080
    server_addr.sin_addr.s_addr = INADDR_ANY;     // 监听所有网卡

    if (bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(listen_fd);
        return 1;
    }
    cout << "[2/5] bind 成功，监听 0.0.0.0:8080" << endl;

    // --------------------------------------------
    // 第3步：监听
    // --------------------------------------------
    if (listen(listen_fd, 10) < 0) {
        perror("listen");
        close(listen_fd);
        return 1;
    }
    cout << "[3/5] listen 成功，等待客户端连接..." << endl;
    cout << "\n提示：可用 telnet localhost 8080 或 nc localhost 8080 测试\n" << endl;


    // --------------------------------------------
    // 第4步：主循环 - 接受连接并创建线程
    // --------------------------------------------
    while (true) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        // accept 阻塞等待新连接
        int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        // 获取客户端地址字符串
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));
        int port = ntohs(client_addr.sin_port);

        string client_addr_str = string(ip_str) + ":" + to_string(port);
        cout << "\n[4/5] accept 成功，客户端：" << client_addr_str << endl;

        // --------------------------------------------
        // 第5步：创建新线程处理该客户端
        // --------------------------------------------
        pthread_t tid;
        int* fd_ptr = new int(client_fd);  // 动态分配，避免栈变量被覆盖

        if (pthread_create(&tid, NULL, client_handler, fd_ptr) != 0) {
            perror("pthread_create");
            close(client_fd);
            delete fd_ptr;
            continue;
        }

        cout << "[5/5] 线程 " << tid << " 已创建，处理客户端" << endl;

        // 分离线程（自动回收资源）
        pthread_detach(tid);

        // 添加到在线列表
        add_client(client_fd, tid, client_addr_str);
    }

    // 关闭监听 socket（实际不会执行到这里，因为是无限循环）
    close(listen_fd);
    pthread_mutex_destroy(&clients_mutex);

    return 0;
}

/*
==============================================
🎓 学习总结
==============================================

【技术串联】
1. socket 五步：socket → bind → listen → accept → recv/send
2. 多线程：accept 后立即 pthread_create，主线程继续 accept
3. 线程安全：online_clients 是共享数据，增删必须加锁
4. 资源管理：线程结束时 close(fd)、从列表移除、自动回收

【关键设计】
1. 动态分配 fd：new int(client_fd)，避免栈变量被下次循环覆盖
2. pthread_detach：线程分离，退出时自动回收资源
3. SO_REUSEADDR：允许重启服务器立即绑定端口
4. 互斥锁：保护共享的 vector，防止数据竞争

【测试方法】
终端1：./server
终端2：telnet localhost 8080（输入任意文本，服务器回显）
终端3：nc localhost 8080（再开一个客户端，测试多连接）

【扩展思路】
- 聊天室：收到消息后广播给所有在线客户端
- 心跳检测：定时发送 ping，检测客户端存活
- 优雅关闭：捕获 SIGINT，关闭所有连接后退出
- 线程池：预创建线程，避免频繁 create/destroy

==============================================
*/
