# epoll 最佳实践与常见陷阱

> **文档日期**：2026-06-17（第72天）  
> **目的**：总结 epoll 的常见错误和最佳实践

---

## 🚨 常见陷阱列表

### 陷阱1：ET 模式忘记循环读取

**错误代码**：
```cpp
// ❌ 错误：ET 模式只读一次
char buf[1024];
int n = recv(fd, buf, sizeof(buf), 0);
if (n > 0) {
    send(fd, buf, n, 0);
}
```

**问题**：
- 如果数据 > 1024 字节，剩余部分丢失
- ET 模式只通知一次，不会再次触发

**正确代码**：
```cpp
// ✅ 正确：循环读取直到 EAGAIN
while (true) {
    char buf[1024];
    int n = recv(fd, buf, sizeof(buf), 0);
    if (n > 0) {
        send(fd, buf, n, 0);
    } else if (n == 0) {
        break;  // 连接关闭
    } else {
        if (errno == EAGAIN) {
            break;  // 数据读完
        } else {
            perror("recv");
            break;
        }
    }
}
```

---

### 陷阱2：ET 模式使用阻塞 I/O

**错误代码**：
```cpp
// ❌ 错误：ET + 阻塞 I/O
// 忘记设置非阻塞
while (true) {
    int n = recv(fd, buf, sizeof(buf), 0);  // 阻塞模式
    // 最后一次 recv 会永久阻塞
}
```

**问题**：
- 最后一次 recv 会阻塞等待新数据
- 整个事件循环卡住
- 其他连接无法处理

**正确代码**：
```cpp
// ✅ 正确：ET + 非阻塞 I/O
setNonBlocking(fd);
while (true) {
    int n = recv(fd, buf, sizeof(buf), 0);  // 非阻塞模式
    if (n < 0 && errno == EAGAIN) {
        break;  // 数据读完，正常退出
    }
}
```

---

### 陷阱3：把 EAGAIN 当作错误

**错误代码**：
```cpp
// ❌ 错误：把 EAGAIN 当错误
int n = recv(fd, buf, sizeof(buf), 0);
if (n < 0) {
    perror("recv");  // 错误地打印 EAGAIN
    close(fd);       // 错误地关闭连接
}
```

**问题**：
- EAGAIN 不是错误，是"暂时没有数据"
- 错误地关闭了正常连接

**正确代码**：
```cpp
// ✅ 正确：区分 EAGAIN 和真正的错误
int n = recv(fd, buf, sizeof(buf), 0);
if (n < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;  // 正常：数据读完
    } else {
        perror("recv");  // 真正的错误
        close(fd);
    }
}
```

---

### 陷阱4：忘记设置 listen_fd 为非阻塞

**错误代码**：
```cpp
// ❌ 错误：只设置 client_fd 为非阻塞
int client_fd = accept(listen_fd, ...);
setNonBlocking(client_fd);  // 只设置 client_fd
```

**问题**：
- ET 模式下，accept 也要循环
- 如果 listen_fd 是阻塞的，最后一次 accept 会阻塞

**正确代码**：
```cpp
// ✅ 正确：listen_fd 和 client_fd 都要非阻塞
setNonBlocking(listen_fd);  // 关键

while (true) {
    int client_fd = accept(listen_fd, ...);
    if (client_fd < 0 && errno == EAGAIN) {
        break;  // 所有新连接都处理完了
    }
    setNonBlocking(client_fd);
}
```

---

### 陷阱5：EPOLLONESHOT 后忘记重新注册

**错误代码**：
```cpp
// ❌ 错误：处理完成后忘记重新注册
void handleClient(int fd, int epfd) {
    // 处理数据...
    // 忘记重新注册
}
```

**问题**：
- EPOLLONESHOT 触发后自动移除 fd
- 如果不重新注册，fd 永远不会再触发
- 连接"死掉"

**正确代码**：
```cpp
// ✅ 正确：处理完成后重新注册
void handleClient(int fd, int epfd) {
    // 处理数据...
    
    // 重新注册（关键）
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    ev.data.fd = fd;
    epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
}
```

---

### 陷阱6：多线程环境不使用 EPOLLONESHOT

**错误代码**：
```cpp
// ❌ 错误：多线程 + epoll 不使用 EPOLLONESHOT
ev.events = EPOLLIN | EPOLLET;  // 没有 EPOLLONESHOT
pool.enqueue([fd]() { handleClient(fd); });
// 同一 fd 可能被多个线程同时处理
```

**问题**：
- 同一 fd 可能被多次触发
- 多个工作线程同时处理同一 fd
- 竞态条件、数据乱序

**正确代码**：
```cpp
// ✅ 正确：多线程必须使用 EPOLLONESHOT
ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
pool.enqueue([fd]() { handleClient(fd); });
```

---

### 陷阱7：忘记处理 EPOLLHUP 和 EPOLLERR

**错误代码**：
```cpp
// ❌ 错误：只检查 EPOLLIN
if (events[i].events & EPOLLIN) {
    // 处理可读事件
}
// 忘记处理 EPOLLHUP 和 EPOLLERR
```

**问题**：
- 客户端异常断开时，会触发 EPOLLHUP
- 错误时会触发 EPOLLERR
- 如果不处理，fd 会一直触发

**正确代码**：
```cpp
// ✅ 正确：处理所有事件类型
if (events[i].events & (EPOLLHUP | EPOLLERR)) {
    // 连接异常，关闭 fd
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);
} else if (events[i].events & EPOLLIN) {
    // 处理可读事件
}
```

---

### 陷阱8：epoll_ctl 的 event 参数为 NULL（DEL 时可以）

**错误代码**：
```cpp
// ❌ 错误：ADD/MOD 时 event 为 NULL
epoll_ctl(epfd, EPOLL_CTL_ADD, fd, NULL);  // 错误
epoll_ctl(epfd, EPOLL_CTL_MOD, fd, NULL);  // 错误
```

**正确代码**：
```cpp
// ✅ 正确：ADD/MOD 时必须提供 event
struct epoll_event ev;
ev.events = EPOLLIN | EPOLLET;
ev.data.fd = fd;
epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);

// DEL 时可以为 NULL
epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
```

---

### 陷阱9：忘记检查 epoll_ctl 的返回值

**错误代码**：
```cpp
// ❌ 错误：忘记检查返回值
epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
// 如果失败（例如 fd 已关闭），不知道
```

**正确代码**：
```cpp
// ✅ 正确：检查返回值
if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) < 0) {
    perror("epoll_ctl MOD");
    // 处理错误
}
```

---

### 陷阱10：在主线程处理耗时业务

**错误代码**：
```cpp
// ❌ 错误：在主线程处理业务逻辑
while (true) {
    int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
    for (int i = 0; i < n; i++) {
        // 处理业务逻辑（很慢）
        process_business_logic();  // 阻塞主线程
    }
}
```

**问题**：
- 主线程阻塞，其他就绪的 fd 无法处理
- 响应变慢

**正确代码**：
```cpp
// ✅ 正确：交给线程池处理
while (true) {
    int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
    for (int i = 0; i < n; i++) {
        pool.enqueue([fd]() {
            process_business_logic();  // 工作线程处理
        });
    }
}
```

---

## ✅ 最佳实践

### 1. 文件描述符管理

```cpp
// ✅ 所有 fd 都设置为非阻塞（ET 模式）
setNonBlocking(listen_fd);
setNonBlocking(client_fd);

// ✅ 关闭 fd 前先从 epoll 删除
epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
close(fd);
```

---

### 2. 事件类型设置

```cpp
// ✅ 单线程：ET 模式
ev.events = EPOLLIN | EPOLLET;

// ✅ 多线程：ET + EPOLLONESHOT
ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;

// ✅ listen_fd 不需要 EPOLLONESHOT
ev.events = EPOLLIN | EPOLLET;  // listen_fd
```

---

### 3. 错误处理

```cpp
// ✅ 区分 EAGAIN 和真正的错误
if (n < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;  // 正常
    } else {
        perror("recv");  // 真错误
        // 清理资源
    }
}

// ✅ 检查所有系统调用的返回值
if (epoll_ctl(...) < 0) {
    perror("epoll_ctl");
}
```

---

### 4. 循环读取模式

```cpp
// ✅ ET 模式必须循环读取
while (true) {
    int n = recv(fd, buf, sizeof(buf), 0);
    if (n > 0) {
        // 处理数据
    } else if (n == 0) {
        // 连接关闭
        break;
    } else {
        if (errno == EAGAIN) {
            break;  // 数据读完
        }
    }
}
```

---

### 5. EPOLLONESHOT 使用模式

```cpp
// ✅ 注册时设置
ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);

// ✅ 处理完成后重新注册
ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);

// ✅ 连接关闭时直接删除（不重新注册）
epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
close(fd);
```

---

## 🐛 调试技巧

### 1. 打印调试信息

```cpp
// 打印 fd 的状态变化
cout << "[epoll_ctl] ADD fd=" << fd << endl;
cout << "[epoll_wait] fd=" << fd << " 就绪" << endl;
cout << "[recv] fd=" << fd << " 读取 " << n << " 字节" << endl;
cout << "[epoll_ctl] MOD fd=" << fd << " 重新注册" << endl;
cout << "[close] fd=" << fd << endl;
```

---

### 2. 检查 fd 泄漏

```cpp
// 定期打印当前打开的 fd 数量
ls -l /proc/$(pidof your_server)/fd | wc -l
```

如果数量持续增长，说明有 fd 泄漏。

---

### 3. 使用 strace 追踪系统调用

```bash
strace -e epoll_wait,epoll_ctl,recv,send ./your_server
```

观察 epoll 的调用流程。

---

### 4. 模拟大量连接

```bash
# 使用 ab（Apache Bench）压测
ab -n 10000 -c 1000 http://localhost:8082/

# 使用 wrk
wrk -t4 -c1000 -d30s http://localhost:8082/
```

---

## 📊 性能优化建议

### 1. 合理设置线程池大小

```cpp
// CPU 密集型：线程数 = CPU 核数
int num_threads = thread::hardware_concurrency();

// I/O 密集型：线程数 = CPU 核数 × 2
int num_threads = thread::hardware_concurrency() * 2;
```

---

### 2. 避免频繁的小数据传输

```cpp
// ❌ 不好：每次发送 1 字节
for (int i = 0; i < 1000; i++) {
    send(fd, &data[i], 1, 0);
}

// ✅ 好：一次发送 1000 字节
send(fd, data, 1000, 0);
```

---

### 3. 使用 sendfile 零拷贝

```cpp
// 发送文件时使用 sendfile
off_t offset = 0;
sendfile(socket_fd, file_fd, &offset, file_size);
```

---

## 🎓 总结

**epoll 的三个核心**：
1. **ET 模式**：减少系统调用，提升性能
2. **非阻塞 I/O**：避免阻塞事件循环
3. **EPOLLONESHOT**：多线程环境防止竞态

**记住**：
- ET 必须循环读取
- EAGAIN 不是错误
- EPOLLONESHOT 后必须重新注册
- 多线程必须用 EPOLLONESHOT

---

> **掌握这些最佳实践，你就能写出工业级的 epoll 服务器！** 🚀
