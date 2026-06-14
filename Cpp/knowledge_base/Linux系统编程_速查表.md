# Linux 系统编程速查表

> **适用环境**：Linux/Unix/WSL  
> **编译器**：g++ (需要 `-pthread` 链接线程库)  
> **学习日期**：第64-68天  
> **知识范围**：进程、IPC、线程、网络编程

---

## 📑 目录

1. [进程管理](#1-进程管理)
2. [IPC 进程间通信](#2-ipc-进程间通信)
3. [多线程编程](#3-多线程编程)
4. [网络编程](#4-网络编程)

---

## 1. 进程管理

### 1.1 fork() - 创建子进程

```cpp
#include <unistd.h>
#include <sys/types.h>

pid_t pid = fork();

if (pid < 0) {
    // fork 失败
    perror("fork");
} else if (pid == 0) {
    // 子进程代码
    cout << "我是子进程，PID=" << getpid() << endl;
} else {
    // 父进程代码，pid 是子进程的 PID
    cout << "我是父进程，子进程PID=" << pid << endl;
}
```

**关键点**：
- fork() 返回两次：父进程返回子进程PID，子进程返回0
- 子进程复制父进程的内存空间（写时复制 COW）
- 父子进程独立运行，执行顺序不确定


### 1.2 exec() 系列 - 替换进程映像

```cpp
#include <unistd.h>

// execl - 逐个列出参数
execl("/bin/ls", "ls", "-l", NULL);

// execlp - 从 PATH 查找
execlp("ls", "ls", "-l", NULL);

// execv - 参数数组
char* args[] = {"ls", "-l", NULL};
execv("/bin/ls", args);
```

**关键点**：
- exec 系列成功调用后不会返回，当前进程被新程序替换
- 失败返回 -1，原进程继续运行
- 常见模式：fork + exec（父进程fork子进程，子进程exec新程序）

### 1.3 wait() / waitpid() - 等待子进程

```cpp
#include <sys/wait.h>

// 等待任意子进程
int status;
pid_t pid = wait(&status);

// 等待指定子进程
pid_t pid = waitpid(child_pid, &status, 0);

// 检查退出状态
if (WIFEXITED(status)) {
    cout << "子进程正常退出，返回值=" << WEXITSTATUS(status) << endl;
}
```

**关键点**：
- wait() 阻塞等待任意子进程退出
- waitpid() 可指定等待哪个子进程，options 可设置非阻塞
- 不 wait 会产生僵尸进程（zombie）


---

## 2. IPC 进程间通信

### 2.1 管道 (Pipe) - 半双工通信

```cpp
#include <unistd.h>

int pipefd[2];  // pipefd[0] 读端，pipefd[1] 写端
pipe(pipefd);

if (fork() == 0) {
    // 子进程：写入管道
    close(pipefd[0]);  // 关闭读端
    write(pipefd[1], "Hello", 5);
    close(pipefd[1]);
} else {
    // 父进程：读取管道
    close(pipefd[1]);  // 关闭写端
    char buf[128];
    read(pipefd[0], buf, sizeof(buf));
    close(pipefd[0]);
}
```

**关键点**：
- 单向通信：一端写，另一端读
- 用完必须关闭不用的端（否则 read 永远阻塞）
- 适合父子进程通信

### 2.2 共享内存 (Shared Memory) - 最快 IPC

```cpp
#include <sys/shm.h>

// 创建共享内存
int shmid = shmget(IPC_PRIVATE, 1024, IPC_CREAT | 0666);

// 映射到进程地址空间
void* shmaddr = shmat(shmid, NULL, 0);

// 使用共享内存（当作普通指针）
strcpy((char*)shmaddr, "共享数据");

// 解除映射
shmdt(shmaddr);

// 删除共享内存
shmctl(shmid, IPC_RMID, NULL);
```

**关键点**：
- 最快的 IPC 方式（直接内存访问）
- 需要同步机制（信号量）防止竞态条件
- 记得删除，否则系统重启前一直占用


### 2.3 消息队列 (Message Queue) - 结构化通信

```cpp
#include <sys/msg.h>

struct msg_buffer {
    long msg_type;
    char msg_text[100];
};

// 创建消息队列
int msgid = msgget(IPC_PRIVATE, IPC_CREAT | 0666);

// 发送消息
struct msg_buffer msg;
msg.msg_type = 1;
strcpy(msg.msg_text, "Hello");
msgsnd(msgid, &msg, sizeof(msg.msg_text), 0);

// 接收消息
msgrcv(msgid, &msg, sizeof(msg.msg_text), 1, 0);

// 删除消息队列
msgctl(msgid, IPC_RMID, NULL);
```

**关键点**：
- 消息带类型，可选择性接收
- 消息有边界（不会粘包）
- 适合多对多通信

---

## 3. 多线程编程

### 3.1 pthread_create() - 创建线程

```cpp
#include <pthread.h>

void* thread_func(void* arg) {
    int* num = (int*)arg;
    cout << "线程参数：" << *num << endl;
    return NULL;
}

int main() {
    pthread_t tid;
    int arg = 42;
    pthread_create(&tid, NULL, thread_func, &arg);
    pthread_join(tid, NULL);  // 等待线程结束
    return 0;
}
```

**编译**：`g++ -o prog prog.cpp -pthread`

**关键点**：
- 线程函数签名：`void* func(void* arg)`
- pthread_join() 回收线程资源，类似进程的 wait()
- pthread_detach() 分离线程，自动回收


### 3.2 pthread_mutex_t - 互斥锁

```cpp
#include <pthread.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// 加锁
pthread_mutex_lock(&mutex);

// 临界区：访问共享数据
shared_data++;

// 解锁
pthread_mutex_unlock(&mutex);

// 销毁
pthread_mutex_destroy(&mutex);
```

**关键点**：
- 保护共享数据，防止数据竞争
- lock() 阻塞等待，trylock() 立即返回
- 必须配对：lock → unlock，否则死锁

### 3.3 pthread_cond_t - 条件变量

```cpp
#include <pthread.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// 消费者：等待条件
pthread_mutex_lock(&mutex);
while (!condition) {  // 必须用 while，防止虚假唤醒
    pthread_cond_wait(&cond, &mutex);  // 原子：解锁 → 等待 → 加锁
}
// 条件满足，处理数据
pthread_mutex_unlock(&mutex);

// 生产者：通知条件
pthread_mutex_lock(&mutex);
// 改变条件
condition = true;
pthread_cond_signal(&cond);  // 唤醒一个等待线程
pthread_mutex_unlock(&mutex);
```

**关键点**：
- 配合互斥锁使用
- wait() 必须用 while 循环包围（防止虚假唤醒）
- signal() 唤醒一个，broadcast() 唤醒全部


---

## 4. 网络编程

### 4.1 TCP 服务端 - 五步流程

```cpp
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

// 1. 创建 socket
int listen_fd = socket(AF_INET, SOCK_STREAM, 0);

// 2. 绑定地址
struct sockaddr_in addr;
memset(&addr, 0, sizeof(addr));
addr.sin_family = AF_INET;
addr.sin_port = htons(8080);  // 主机字节序 → 网络字节序
addr.sin_addr.s_addr = INADDR_ANY;
bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr));

// 3. 监听
listen(listen_fd, 5);  // backlog=5

// 4. 接受连接
struct sockaddr_in client_addr;
socklen_t len = sizeof(client_addr);
int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &len);

// 5. 收发数据
char buf[1024];
int n = recv(client_fd, buf, sizeof(buf), 0);
send(client_fd, buf, n, 0);

// 关闭
close(client_fd);
close(listen_fd);
```

**关键点**：
- accept() 返回新的 fd 用于通信，listen_fd 继续监听
- recv() 返回 0 表示对方关闭连接，-1 表示出错
- TCP 是字节流，无消息边界，需要自己分帧


### 4.2 TCP 客户端 - 三步流程

```cpp
// 1. 创建 socket
int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

// 2. 连接服务器
struct sockaddr_in server_addr;
memset(&server_addr, 0, sizeof(server_addr));
server_addr.sin_family = AF_INET;
server_addr.sin_port = htons(8080);
inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));

// 3. 收发数据
send(sock_fd, "Hello", 5, 0);
char buf[1024];
recv(sock_fd, buf, sizeof(buf), 0);

close(sock_fd);
```

**关键点**：
- connect() 成功返回 0，失败返回 -1
- inet_pton() 将 IP 字符串转为二进制

### 4.3 UDP Socket - 无连接

```cpp
// 服务端
int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
bind(sock_fd, ...);

struct sockaddr_in client_addr;
socklen_t len = sizeof(client_addr);
recvfrom(sock_fd, buf, sizeof(buf), 0, (struct sockaddr*)&client_addr, &len);
sendto(sock_fd, buf, n, 0, (struct sockaddr*)&client_addr, len);

// 客户端
int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
sendto(sock_fd, "Hello", 5, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
recvfrom(sock_fd, buf, sizeof(buf), 0, NULL, NULL);
```

**关键点**：
- 无需 listen、accept、connect
- 每次收发都带地址信息
- 保留消息边界，无粘包


### 4.4 select() - I/O 多路复用

```cpp
#include <sys/select.h>

fd_set read_fds;
FD_ZERO(&read_fds);         // 清空集合
FD_SET(listen_fd, &read_fds);  // 添加 fd
FD_SET(client_fd, &read_fds);

int max_fd = max(listen_fd, client_fd);

// 等待事件（阻塞）
int ret = select(max_fd + 1, &read_fds, NULL, NULL, NULL);

// 检查哪个 fd 就绪
if (FD_ISSET(listen_fd, &read_fds)) {
    // 有新连接
    int new_fd = accept(listen_fd, ...);
}
if (FD_ISSET(client_fd, &read_fds)) {
    // 客户端有数据
    recv(client_fd, buf, sizeof(buf), 0);
}
```

**关键点**：
- 单线程监听多个 fd
- nfds = 最大 fd + 1
- select 会修改 fd_set，每轮循环需重新设置
- 上限 1024 个 fd（可用 epoll 突破）

---

## 5. 常见陷阱与最佳实践

### 5.1 僵尸进程

**问题**：fork 后不 wait，子进程退出变僵尸（占用 PID）

**解决**：
- 父进程 wait/waitpid 回收
- 或 signal(SIGCHLD, SIG_IGN) 自动回收

### 5.2 数据竞争

**问题**：多线程同时访问共享数据，未加锁

**解决**：
- 互斥锁保护共享数据
- 编译时加 `-fsanitize=thread` 检测


### 5.3 死锁

**问题**：多个线程互相等待对方释放锁

**解决**：
- 固定加锁顺序
- 使用 trylock() 避免阻塞
- 减少锁持有时间

### 5.4 忘记关闭 fd

**问题**：socket/pipe 忘记 close，耗尽文件描述符

**解决**：
- 用完立即 close
- RAII 封装（析构函数自动关闭）

### 5.5 字节序问题

**问题**：端口/IP 直接赋值，跨平台出错

**解决**：
- 端口用 htons/ntohs
- IP 用 htonl/ntohl 或 inet_pton

---

## 6. 编译与调试

### 6.1 编译选项

```bash
# 基础编译
g++ -o prog prog.cpp

# 链接线程库
g++ -o prog prog.cpp -pthread

# 调试模式
g++ -g -o prog prog.cpp

# 检测数据竞争
g++ -g -fsanitize=thread -o prog prog.cpp -pthread

# 检测内存泄漏
g++ -g -fsanitize=address -o prog prog.cpp
```

### 6.2 常用调试工具

```bash
# gdb 调试
gdb ./prog

# valgrind 检测内存泄漏
valgrind --leak-check=full ./prog

# strace 追踪系统调用
strace ./prog
```

---

## 7. 知识体系总结

```
Linux 系统编程
│
├─ 进程管理
│  ├─ fork()     - 创建子进程
│  ├─ exec()     - 替换进程映像
│  └─ wait()     - 回收子进程
│
├─ IPC 进程间通信
│  ├─ 管道        - 单向通信
│  ├─ 共享内存    - 最快
│  └─ 消息队列    - 结构化
│
├─ 多线程编程
│  ├─ pthread     - 创建线程
│  ├─ mutex       - 互斥锁
│  └─ condition   - 条件变量
│
└─ 网络编程
   ├─ TCP        - 可靠有序
   ├─ UDP        - 高效无连接
   └─ select     - I/O 多路复用
```

---

> **速查表完成！** 📚 包含进程/IPC/线程/网络四大模块，共 7 个章节。
