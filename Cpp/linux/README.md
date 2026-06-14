# Linux 系统编程学习材料

> **学习日期**：第64-68天  
> **学习阶段**：阶段12 - Linux 系统编程  
> **环境要求**：Linux/Unix/WSL  
> **编译器**：g++ (涉及线程的文件需要 `-lpthread` 链接线程库)

---

## 📚 目录说明

本目录包含 Linux 系统编程的完整学习材料，涵盖进程、IPC、多线程、网络编程四大模块。

### 进程与文件（第64天）

| 文件 | 主题 | 内容 |
|------|------|------|
| `01_process_basics.cpp` | 进程基础 | fork/exec/wait、COW 写时复制、僵尸进程预防 |
| `02_pipe_ipc.cpp` | 管道通信 | 匿名管道、命名管道 FIFO、短读陷阱、管道死锁 |
| `03_file_operations.cpp` | POSIX 文件操作 | open/read/write/lseek/fcntl、文件锁、POSIX vs FILE* |

### 信号与 IPC（第65天）

| 文件 | 主题 | 内容 |
|------|------|------|
| `04_signals.cpp` | 信号机制 | signal/sigaction、信号处理函数、常见信号 |
| `05_shared_memory.cpp` | 共享内存 | shmget/shmat/shmdt、最快的 IPC |
| `06_message_queue.cpp` | 消息队列 | msgget/msgsnd/msgrcv、结构化通信 |

### 多线程编程（第66天）

| 文件 | 主题 | 内容 |
|------|------|------|
| `07_pthread_basics.cpp` | pthread 基础 | pthread_create/join/detach、线程参数传递、互斥锁 |
| `08_condition_variable.cpp` | 条件变量 | pthread_cond_t、生产者-消费者、虚假唤醒 |
| `09_thread_pool.cpp` | 线程池 | 任务队列、工作线程组、手写 ThreadPool 类、优雅关闭 |

### 网络编程（第67天）

| 文件 | 主题 | 内容 |
|------|------|------|
| `10_tcp_socket.cpp` | TCP Socket | 服务端五步、客户端三步、字节序转换 |
| `11_udp_socket.cpp` | UDP Socket | 无连接通信、sendto/recvfrom、UDP vs TCP |
| `12_io_multiplexing.cpp` | I/O 多路复用 | select、fd_set、单线程多客户端 |

### 综合实战（第68天）

| 文件 | 主题 | 内容 |
|------|------|------|
| `13_mini_server.cpp` | 多线程服务器 | TCP + pthread + 互斥锁，完整回显服务器 |

---

## 🛠️ 编译与运行

### 基础编译

```bash
# 不涉及线程
g++ -std=c++17 -o prog 01_process_basics.cpp
./prog

# 涉及线程（需要 -lpthread）
g++ -std=c++17 -o prog 07_pthread_basics.cpp -lpthread
./prog
```

### 调试编译

```bash
# 添加调试信息
g++ -g -std=c++17 -o prog 13_mini_server.cpp -lpthread

# 检测数据竞争
g++ -g -fsanitize=thread -o prog 08_condition_variable.cpp -lpthread

# 检测内存泄漏
g++ -g -fsanitize=address -o prog 05_shared_memory.cpp
```

---

## 🎓 学习路线

### 推荐顺序

```
01 → 02 → 03  (进程、管道、文件)
    ↓
04 → 05 → 06  (信号、共享内存、消息队列)
    ↓
07 → 08 → 09  (线程、条件变量、线程池)
    ↓
10 → 11 → 12  (TCP、UDP、多路复用)
    ↓
13             (综合实战：多线程服务器)
```

### 知识依赖

- `08_condition_variable` 依赖 `07_pthread_basics`
- `09_thread_pool` 依赖 `07/08`
- `13_mini_server` 依赖 `07`（线程）和 `10`（TCP）

---

## 📖 知识点快速索引

### 进程相关
- **创建进程**：`01_process_basics.cpp` - fork()
- **替换程序**：`01_process_basics.cpp` - exec 系列
- **回收子进程**：`01_process_basics.cpp` - wait/waitpid
- **僵尸进程**：`01_process_basics.cpp` - 不 wait 的后果

### IPC 进程间通信
- **管道**：`02_pipe_ipc.cpp` - pipe/read/write、FIFO
- **信号**：`04_signals.cpp` - signal/sigaction
- **共享内存**：`05_shared_memory.cpp` - shmget/shmat
- **消息队列**：`06_message_queue.cpp` - msgget/msgsnd

### 文件操作
- **POSIX I/O**：`03_file_operations.cpp` - open/read/write/lseek
- **文件锁**：`03_file_operations.cpp` - fcntl

### 多线程
- **创建线程**：`07_pthread_basics.cpp` - pthread_create
- **互斥锁**：`07_pthread_basics.cpp` - pthread_mutex_t
- **条件变量**：`08_condition_variable.cpp` - pthread_cond_t
- **线程池**：`09_thread_pool.cpp` - 生产者消费者

### 网络编程
- **TCP 服务端**：`10_tcp_socket.cpp` - 五步流程
- **TCP 客户端**：`10_tcp_socket.cpp` - 三步流程
- **UDP 通信**：`11_udp_socket.cpp` - sendto/recvfrom
- **多路复用**：`12_io_multiplexing.cpp` - select
- **字节序**：`10_tcp_socket.cpp` - htons/ntohs

### 综合应用
- **多线程服务器**：`13_mini_server.cpp` - socket + pthread + mutex

---

## ⚠️ 常见问题

### Q1: 编译报错 "undefined reference to pthread_create"

**原因**：未链接线程库

**解决**：添加 `-lpthread` 选项
```bash
g++ -std=c++17 -o prog prog.cpp -lpthread
```

### Q2: Windows 编译失败

**原因**：这些代码是 Linux POSIX API，Windows 不支持

**解决**：
- 使用 WSL (Windows Subsystem for Linux)
- 或使用 Linux 虚拟机
- Windows 网络编程需用 Winsock API

### Q3: bind 失败 "Address already in use"

**原因**：端口被占用或 TIME_WAIT 状态

**解决**：
```cpp
int opt = 1;
setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
```

### Q4: 多线程程序出现数据错乱

**原因**：未加锁保护共享数据

**解决**：
- 所有共享数据访问前加锁
- 使用 `-fsanitize=thread` 检测

---

## 📚 参考资料

### 速查表
- `knowledge_base/Linux系统编程_速查表.md` - 完整知识体系

### 学习日志
- `daily_logs/2026-06-07_第64天_日志.md` - 进程、管道、文件
- `daily_logs/2026-06-07_第65天_日志.md` - 信号、共享内存、消息队列
- `daily_logs/2026-06-09_第66天_日志.md` - 多线程
- `daily_logs/2026-06-10_第67天_日志.md` - 网络编程
- `daily_logs/2026-06-13_第68天_日志.md` - 综合实战

### 扩展阅读
- APUE (Advanced Programming in the UNIX Environment)
- UNP (UNIX Network Programming)
- man 手册：`man 2 fork`, `man 3 pthread_create`

---

> **学习完成！** 🎉 你已掌握 Linux 系统编程四大核心模块！
