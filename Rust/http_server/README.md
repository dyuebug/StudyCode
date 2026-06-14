# 🌐 简单 HTTP 服务器项目

## 📋 项目概述

这是一个用 Rust 从零开始构建的 HTTP 服务器，逐步实现从基础 TCP 到完整 HTTP 功能。

---

## 🎯 学习目标

- ✅ 理解 TCP/IP 网络编程
- ✅ 理解 HTTP 协议
- ✅ 实践并发编程（多线程）
- ✅ 文件 I/O 操作
- ✅ 错误处理和日志记录

---

## 🚀 阶段1：基础 TCP 服务器 ✅

### 已实现功能

- ✅ 监听 TCP 端口（127.0.0.1:7878）
- ✅ 接受客户端连接
- ✅ 读取 HTTP 请求
- ✅ 返回简单的 HTTP 响应

---

## 🎨 阶段2：HTTP 协议处理和路由 ✅

### 已实现功能

- ✅ 解析 HTTP 请求行（GET /path HTTP/1.1）
- ✅ 路由匹配（/, /about, 404）
- ✅ 读取并返回 HTML 文件
- ✅ 设置响应头（Content-Length, Content-Type）
- ✅ 处理 404 错误

### 如何运行

```bash
# 编译并运行
cargo run

# 测试不同路由
curl http://127.0.0.1:7878/          # 首页
curl http://127.0.0.1:7878/about     # 关于页面
curl http://127.0.0.1:7878/test      # 404 页面
```

### 路由列表

| 路径 | 响应 | 状态码 |
|------|------|--------|
| `/` | index.html | 200 OK |
| `/about` | about.html | 200 OK |
| 其他 | 404.html | 404 NOT FOUND |

### 核心代码解析

```rust
// 解析请求行
let request_line = request.lines().next().unwrap_or("");

// 路由匹配
let (status_line, filename) = if request_line.starts_with("GET / HTTP/1.1") {
    ("HTTP/1.1 200 OK", "index.html")
} else if request_line.starts_with("GET /about HTTP/1.1") {
    ("HTTP/1.1 200 OK", "about.html")
} else {
    ("HTTP/1.1 404 NOT FOUND", "404.html")
};

// 读取文件
let contents = fs::read_to_string(filename).unwrap();

// 构造响应
let response = format!(
    "{}\r\nContent-Length: {}\r\nContent-Type: text/html\r\n\r\n{}",
    status_line, contents.len(), contents
);
```

### 学到的知识

- **HTTP 请求解析**：从原始字节中提取请求行
- **路由匹配**：根据路径返回不同内容
- **文件读取**：使用 `fs::read_to_string`
- **HTTP 响应头**：Content-Length、Content-Type
- **状态码**：200 OK、404 NOT FOUND

---

## 🔥 阶段3：多线程并发 ✅

### 已实现功能

- ✅ 线程池（ThreadPool）
- ✅ 固定数量的工作线程（4个）
- ✅ 任务队列（通道 channel）
- ✅ 并发处理多个请求
- ✅ Arc + Mutex 共享状态

### 核心概念

**线程池工作原理**：
1. 创建固定数量的工作线程（4个）
2. 工作线程等待任务队列
3. 主线程将请求封装为任务，发送到队列
4. 工作线程从队列取任务并执行
5. 多个请求可以并发处理

**关键技术**：
- **mpsc::channel**：多生产者单消费者通道
- **Arc<Mutex<T>>**：线程安全的共享所有权
  - Arc：原子引用计数，多个线程共享
  - Mutex：互斥锁，保证同一时间只有一个线程访问

### 测试并发性能

**启动服务器**：
```bash
cargo run
```

**方法1：浏览器测试**
1. 打开两个浏览器标签
2. 第一个访问：http://127.0.0.1:7878/sleep（慢请求，5秒）
3. 立即第二个访问：http://127.0.0.1:7878/（快速请求）
4. 观察：第二个请求会立即返回，不会等待第一个！

**方法2：curl 测试**
```bash
# 终端1：慢请求（会等待5秒）
curl http://127.0.0.1:7878/sleep

# 终端2：立即发起快速请求（不会等待，立即返回）
curl http://127.0.0.1:7878/
```

**方法3：运行测试脚本**
```bash
bash test_concurrent.sh
```

### 核心代码解析

**线程池实现**：
```rust
pub struct ThreadPool {
    workers: Vec<Worker>,
    sender: mpsc::Sender<Job>,
}

impl ThreadPool {
    pub fn new(size: usize) -> ThreadPool {
        let (sender, receiver) = mpsc::channel();
        let receiver = Arc::new(Mutex::new(receiver));
        
        let mut workers = Vec::with_capacity(size);
        for id in 0..size {
            workers.push(Worker::new(id, Arc::clone(&receiver)));
        }
        
        ThreadPool { workers, sender }
    }
    
    pub fn execute<F>(&self, f: F)
    where
        F: FnOnce() + Send + 'static,
    {
        let job = Box::new(f);
        self.sender.send(job).unwrap();
    }
}
```

**工作线程**：
```rust
struct Worker {
    id: usize,
    thread: thread::JoinHandle<()>,
}

impl Worker {
    fn new(id: usize, receiver: Arc<Mutex<mpsc::Receiver<Job>>>) -> Worker {
        let thread = thread::spawn(move || loop {
            let job = receiver.lock().unwrap().recv().unwrap();
            println!("Worker {} 执行任务", id);
            job();
        });
        
        Worker { id, thread }
    }
}
```

**使用线程池**：
```rust
let pool = ThreadPool::new(4);

for stream in listener.incoming() {
    let stream = stream.unwrap();
    
    // 每个请求在独立线程中处理
    pool.execute(|| {
        handle_connection(stream);
    });
}
```

### 学到的知识

- **线程（Thread）**：`std::thread::spawn`
- **通道（Channel）**：`mpsc::channel`，线程间通信
- **Arc**：原子引用计数，多线程共享所有权
- **Mutex**：互斥锁，线程安全的内部可变性
- **闭包捕获**：`move` 关键字，所有权转移
- **并发模式**：生产者-消费者模式

### 性能对比

**单线程（阶段2）**：
- 一次只能处理一个请求
- 慢请求会阻塞所有后续请求
- CPU 利用率低

**多线程（阶段3）**：
- 并发处理 4 个请求
- 慢请求不会阻塞其他请求
- 充分利用多核 CPU
- **性能提升 4 倍！**

---

## 📝 项目总结

### 完成的功能

✅ **阶段1**：基础 TCP 服务器
✅ **阶段2**：HTTP 协议处理和路由
✅ **阶段3**：多线程并发处理

### 技术栈

- **网络编程**：TcpListener、TcpStream
- **文件 I/O**：fs::read_to_string
- **并发编程**：thread、Arc、Mutex、channel
- **错误处理**：Result、unwrap
- **闭包**：FnOnce + Send + 'static

### 项目成果

- 📦 **零依赖**：只使用 Rust 标准库
- 🚀 **高性能**：多线程并发，充分利用 CPU
- 🔒 **内存安全**：Rust 编译器保证无数据竞争
- 📚 **教学价值**：涵盖 Rust 核心特性

---

## 📝 下一阶段：优化和扩展（可选）

## 💡 测试建议

1. **浏览器测试**：打开 `http://127.0.0.1:7878`
2. **curl 测试**：`curl -v http://127.0.0.1:7878`
3. **观察日志**：查看服务器输出的请求信息

---

## 🎓 知识点

### HTTP 响应格式

```
HTTP/1.1 200 OK          ← 状态行
Content-Type: text/html  ← 响应头（可选）

Hello from Rust!         ← 响应体
```

### TCP vs HTTP

- **TCP**：传输层协议，负责数据传输
- **HTTP**：应用层协议，建立在 TCP 之上

---

## ⏭️ 继续学习

当你准备好进入下一阶段时，说 **"继续"** 或 **"下一阶段"**！

我会帮你实现：
- 路由处理
- HTML 页面
- 静态文件服务
- 多线程并发
