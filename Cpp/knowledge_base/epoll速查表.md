# epoll 速查表

> **快速参考**：epoll API、常用模式、陷阱检查清单  
> **使用场景**：开发时快速查阅，避免常见错误

---

## 📋 核心 API 快速参考

### API 1：epoll_create1()

```cpp
int epoll_create1(int flags);
```

**参数**：
- `flags`：0 或 EPOLL_CLOEXEC

**返回**：
- 成功：epoll 文件描述符（epfd）
- 失败：-1

**示例**：
```cpp
int epfd = epoll_create1(0);
if (epfd < 0) {
    perror("epoll_create1");
    exit(1);
}
```

---

### API 2：epoll_ctl()

```cpp
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
```

**参数**：
- `epfd`：epoll 实例
- `op`：操作类型
  - `EPOLL_CTL_ADD`：添加
  - `EPOLL_CTL_MOD`：修改
  - `EPOLL_CTL_DEL`：删除
- `fd`：要操作的文件描述符
- `event`：事件结构体（DEL 时可为 NULL）

**event 结构**：
```cpp
struct epoll_event {
    uint32_t events;   // 事件类型
    epoll_data_t data; // 用户数据
};
```

**events 常用标志**：
- `EPOLLIN`：可读
- `EPOLLOUT`：可写
- `EPOLLET`：边缘触发
- `EPOLLONESHOT`：一次性触发
- `EPOLLERR`：错误
- `EPOLLHUP`：挂断

**示例**：
```cpp
struct epoll_event ev;
ev.events = EPOLLIN | EPOLLET;
ev.data.fd = client_fd;
epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);
```

---

### API 3：epoll_wait()

```cpp
int epoll_wait(int epfd, struct epoll_event *events, 
               int maxevents, int timeout);
```

**参数**：
- `epfd`：epoll 实例
- `events`：输出参数，存储就绪事件
- `maxevents`：events 数组大小
- `timeout`：超时时间（毫秒）
  - `-1`：永久阻塞
  - `0`：立即返回
  - `>0`：超时毫秒数

**返回**：
- `>0`：就绪的 fd 数量
- `0`：超时
- `-1`：错误

**示例**：
```cpp
struct epoll_event events[1024];
int n = epoll_wait(epfd, events, 1024, -1);
for (int i = 0; i < n; i++) {
    int fd = events[i].data.fd;
    // 处理 fd
}
```

---

## 🔧 设置非阻塞 I/O

```cpp
void setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
```

---

## 📦 常用模式代码片段

### 模式1：LT 模式服务器模板

```cpp
// 1. 创建 epoll
int epfd = epoll_create1(0);

// 2. 添加 listen_fd
struct epoll_event ev;
ev.events = EPOLLIN;  // LT 模式（默认）
ev.data.fd = listen_fd;
epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev);

// 3. 事件循环
while (true) {
    struct epoll_event events[MAX_EVENTS];
    int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
    
    for (int i = 0; i < n; i++) {
        int fd = events[i].data.fd;
        
        if (fd == listen_fd) {
            // 新连接
            int client_fd = accept(listen_fd, ...);
            ev.events = EPOLLIN;
            ev.data.fd = client_fd;
            epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);
        } else {
            // 数据到达
            char buf[1024];
            int len = recv(fd, buf, sizeof(buf), 0);
            if (len > 0) {
                send(fd, buf, len, 0);  // 回显
            } else if (len == 0) {
                epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                close(fd);
            }
        }
    }
}
```

---

### 模式2：ET 模式服务器模板

```cpp
// 1. 创建 epoll
int epfd = epoll_create1(0);

// 2. 设置非阻塞
setNonBlocking(listen_fd);

// 3. 添加 listen_fd（ET 模式）
struct epoll_event ev;
ev.events = EPOLLIN | EPOLLET;  // ET 模式
ev.data.fd = listen_fd;
epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev);

// 4. 事件循环
while (true) {
    struct epoll_event events[MAX_EVENTS];
    int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
    
    for (int i = 0; i < n; i++) {
        int fd = events[i].data.fd;
        
        if (fd == listen_fd) {
            // ⭐ ET 模式：循环 accept
            while (true) {
                int client_fd = accept(listen_fd, ...);
                if (client_fd < 0) {
                    if (errno == EAGAIN) break;  // 所有连接处理完
                    perror("accept");
                    break;
                }
                setNonBlocking(client_fd);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = client_fd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);
            }
        } else {
            // ⭐ ET 模式：循环读取直到 EAGAIN
            while (true) {
                char buf[1024];
                int len = recv(fd, buf, sizeof(buf), 0);
                if (len > 0) {
                    send(fd, buf, len, 0);
                } else if (len == 0) {
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                    break;
                } else {
                    if (errno == EAGAIN) break;  // 数据读完
                    perror("recv");
                    break;
                }
            }
        }
    }
}
```

---

### 模式3：epoll + 线程池模板

```cpp
// 1. 创建线程池
ThreadPool pool(4);

// 2. 创建 epoll
int epfd = epoll_create1(0);
setNonBlocking(listen_fd);

// 3. 添加 listen_fd（不需要 EPOLLONESHOT）
struct epoll_event ev;
ev.events = EPOLLIN | EPOLLET;
ev.data.fd = listen_fd;
epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev);

// 4. 主线程事件循环
while (true) {
    struct epoll_event events[MAX_EVENTS];
    int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
    
    for (int i = 0; i < n; i++) {
        int fd = events[i].data.fd;
        
        if (fd == listen_fd) {
            // 主线程处理新连接（快）
            while (true) {
                int client_fd = accept(listen_fd, ...);
                if (client_fd < 0) {
                    if (errno == EAGAIN) break;
                    break;
                }
                setNonBlocking(client_fd);
                // ⭐ 使用 EPOLLONESHOT
                ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
                ev.data.fd = client_fd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);
            }
        } else {
            // ⭐ 交给线程池处理（不阻塞主线程）
            int epfd_copy = epfd;
            pool.enqueue([fd, epfd_copy]() {
                handleClient(fd, epfd_copy);
            });
        }
    }
}

// 5. 工作线程处理函数
void handleClient(int fd, int epfd) {
    // 循环读取
    while (true) {
        char buf[1024];
        int len = recv(fd, buf, sizeof(buf), 0);
        if (len > 0) {
            send(fd, buf, len, 0);
        } else if (len == 0) {
            epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
            close(fd);
            return;
        } else {
            if (errno == EAGAIN) break;
            break;
        }
    }
    
    // ⭐⭐⭐ 关键：重新注册（EPOLLONESHOT）
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    ev.data.fd = fd;
    epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
}
```

---

## ✅ 陷阱检查清单

### ET 模式检查清单（7 项）

- [ ] **所有 fd 都设置为非阻塞**（listen_fd + client_fd）
- [ ] **accept 循环直到 EAGAIN**
- [ ] **recv 循环直到 EAGAIN**
- [ ] **EAGAIN 时 break，不是 close**
- [ ] **event 使用 EPOLLET 标志**
- [ ] **send 也要循环**（防止发送缓冲区满）
- [ ] **处理 EPOLLHUP 和 EPOLLERR**

---

### EPOLLONESHOT 检查清单（3 项）

- [ ] **client_fd 使用 EPOLLONESHOT**（防止竞态）
- [ ] **listen_fd 不使用 EPOLLONESHOT**
- [ ] **处理完成后 EPOLL_CTL_MOD 重新注册**

---

### 通用检查清单（5 项）

- [ ] **检查所有系统调用的返回值**
- [ ] **关闭 fd 前先 EPOLL_CTL_DEL**
- [ ] **主线程不处理耗时业务**（交给线程池）
- [ ] **正确区分 EAGAIN 和真正的错误**
- [ ] **正确处理连接关闭**（recv 返回 0）

---

## 🐛 快速调试指南

### 问题1：程序卡死

**可能原因**：
- ET 模式使用阻塞 I/O
- 最后一次 recv 永久阻塞

**解决方案**：
```cpp
setNonBlocking(fd);  // 所有 fd 都要非阻塞
```

---

### 问题2：数据丢失

**可能原因**：
- ET 模式忘记循环读取
- 只读一次就退出

**解决方案**：
```cpp
while (true) {
    int n = recv(fd, buf, sizeof(buf), 0);
    if (n > 0) {
        // 处理数据
    } else if (errno == EAGAIN) {
        break;  // 数据读完
    }
}
```

---

### 问题3：连接异常断开

**可能原因**：
- 把 EAGAIN 当作错误
- 错误地关闭连接

**解决方案**：
```cpp
if (n < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;  // 正常，不关闭连接
    } else {
        close(fd);  // 真正的错误
    }
}
```

---

### 问题4：连接"死掉"

**可能原因**：
- EPOLLONESHOT 后忘记重新注册

**解决方案**：
```cpp
// 处理完成后必须重新注册
ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
ev.data.fd = fd;
epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
```

---

### 问题5：数据乱序/竞态

**可能原因**：
- 多线程环境不使用 EPOLLONESHOT

**解决方案**：
```cpp
// 必须使用 EPOLLONESHOT
ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
```

---

## 📊 性能调优建议

### 1. 合理设置线程池大小

```cpp
// CPU 密集型：线程数 = CPU 核数
int num_threads = std::thread::hardware_concurrency();

// I/O 密集型：线程数 = CPU 核数 × 2
int num_threads = std::thread::hardware_concurrency() * 2;
```

---

### 2. 合理设置缓冲区大小

```cpp
// 太小：recv 调用次数多
// 太大：内存浪费
// 推荐：4KB - 8KB
char buf[4096];  // 推荐
```

---

### 3. 使用批量操作

```cpp
// 不好：每次只处理 1 个事件
int n = epoll_wait(epfd, events, 1, -1);

// 好：一次处理多个事件
int n = epoll_wait(epfd, events, 1024, -1);
```

---

## 🔍 调试工具

### strace 追踪系统调用

```bash
strace -e epoll_wait,epoll_ctl,recv,send ./your_server
```

---

### lsof 查看打开的 fd

```bash
lsof -p $(pidof your_server)
```

---

### top 查看 CPU 使用率

```bash
top -p $(pidof your_server)
```

---

## 📚 快速参考表

| 场景 | 推荐方案 |
|------|---------|
| 开发阶段 | LT 模式（简单） |
| <100 连接 | LT 模式 |
| 1000+ 连接 | ET 模式 |
| I/O 密集 | 单线程 epoll |
| CPU + I/O 密集 | epoll + 线程池 |
| 跨平台 | select |

---

> **这就是你的 epoll 速查手册！** 📖  
> 开发时快速查阅，避免常见错误！
