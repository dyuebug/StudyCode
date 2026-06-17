# Actix-web 框架入门指南

**日期**：2026-06-20（第24天）
**目标**：了解 Actix-web 的基本概念和使用方式

---

## 📖 第一部分：Actix-web 是什么？

### 基本介绍

**Actix-web** 是一个功能强大、实用且极速的 Rust Web 框架。

**核心特点**：
- 🚀 **高性能**：全球最快的 Web 框架之一
- ⚡ **异步驱动**：基于 tokio 异步运行时
- 🔒 **类型安全**：编译时保证类型正确
- 🎯 **灵活路由**：强大的路由系统
- 🧩 **模块化**：易于扩展和维护

### 为什么选择 Actix-web？

1. **性能卓越**：
   - 在 TechEmpower 基准测试中排名前列
   - 能够处理大量并发请求
   - 低延迟、高吞吐量

2. **基于异步**：
   - 利用你刚学的异步知识
   - 高效处理 I/O 密集任务
   - 不浪费等待时间

3. **生态成熟**：
   - 文档完善
   - 社区活跃
   - 示例丰富

4. **Rust 特性**：
   - 内存安全
   - 无运行时开销
   - 编译时错误检查

---

## 🏗️ 第二部分：核心概念

### 1. HttpServer - HTTP 服务器

**作用**：创建和管理 HTTP 服务器

```rust
use actix_web::HttpServer;

HttpServer::new(|| {
    // 应用配置
})
.bind("127.0.0.1:8080")?  // 绑定地址和端口
.run()                     // 运行服务器
.await                     // 异步等待
```

**关键点**：
- `new()` 接受一个闭包，返回 App
- `bind()` 绑定 IP 地址和端口
- `run()` 启动服务器
- `.await` 异步等待（服务器会一直运行）

---

### 2. App - 应用实例

**作用**：配置应用的路由、中间件、状态等

```rust
use actix_web::App;

App::new()
    .route("/", web::get().to(handler))       // 添加路由
    .route("/api", web::post().to(handler2))  // 另一个路由
    .service(web::scope("/users"))            // 路由作用域
```

**关键点**：
- 每个工作线程都有自己的 App 实例
- 使用链式调用配置路由
- 可以添加中间件和共享状态

---

### 3. Route - 路由定义

**作用**：将 HTTP 请求映射到处理函数

```rust
use actix_web::web;

// GET 请求
web::get().to(handler)

// POST 请求
web::post().to(handler)

// 多种方法
web::route()
    .method(Method::GET)
    .method(Method::POST)
    .to(handler)
```

**路由路径**：
```rust
// 静态路径
.route("/", web::get().to(index))

// 路径参数
.route("/users/{id}", web::get().to(get_user))

// 查询参数
// /search?q=rust
```

---

### 4. Handler - 请求处理函数

**作用**：处理具体的 HTTP 请求

```rust
// 最简单的 handler
async fn hello() -> impl Responder {
    "Hello, World!"
}

// 带参数的 handler
async fn greet(name: web::Path<String>) -> impl Responder {
    format!("Hello, {}!", name)
}

// 返回 JSON
async fn get_user() -> impl Responder {
    web::Json(User { id: 1, name: "Alice".to_string() })
}
```

**关键点**：
- Handler 是 `async fn`（异步函数）
- 返回类型必须实现 `Responder` trait
- 可以接受多种提取器作为参数

---

### 5. Extractor - 请求数据提取

**作用**：从请求中提取数据

**常用提取器**：

```rust
use actix_web::web;

// 路径参数
async fn get_user(path: web::Path<u32>) -> impl Responder {
    format!("User ID: {}", path)
}

// 查询参数
async fn search(query: web::Query<SearchParams>) -> impl Responder {
    format!("Search: {}", query.keyword)
}

// JSON 数据
async fn create_user(user: web::Json<User>) -> impl Responder {
    format!("Created user: {}", user.name)
}

// 多个提取器
async fn complex(
    path: web::Path<u32>,
    query: web::Query<Params>,
    data: web::Json<Body>,
) -> impl Responder {
    // 处理逻辑
}
```

---

### 6. Responder - 响应生成

**作用**：生成 HTTP 响应

**常见响应类型**：

```rust
// 字符串响应
async fn hello() -> impl Responder {
    "Hello, World!"
}

// JSON 响应
async fn json() -> impl Responder {
    web::Json(MyData { field: "value" })
}

// 自定义响应
async fn custom() -> impl Responder {
    HttpResponse::Ok()
        .content_type("text/html")
        .body("<h1>Hello</h1>")
}

// 状态码
async fn not_found() -> impl Responder {
    HttpResponse::NotFound().body("404 Not Found")
}
```

---

## 💻 第三部分：完整示例

### Hello World 示例

```rust
use actix_web::{web, App, HttpServer, Responder};

// Handler 函数
async fn hello() -> impl Responder {
    "Hello, World!"
}

async fn greet(name: web::Path<String>) -> impl Responder {
    format!("Hello, {}!", name)
}

#[actix_web::main]  // 创建异步运行时
async fn main() -> std::io::Result<()> {
    println!("🚀 Starting server at http://127.0.0.1:8080");

    HttpServer::new(|| {
        App::new()
            .route("/", web::get().to(hello))           // GET /
            .route("/hello/{name}", web::get().to(greet))  // GET /hello/Alice
    })
    .bind("127.0.0.1:8080")?
    .run()
    .await
}
```

**测试方法**：
```bash
# 启动服务器
cargo run

# 测试
curl http://127.0.0.1:8080/
# 输出：Hello, World!

curl http://127.0.0.1:8080/hello/Rust
# 输出：Hello, Rust!
```

---

### JSON API 示例

```rust
use actix_web::{web, App, HttpServer, Responder};
use serde::{Deserialize, Serialize};

// 数据模型
#[derive(Serialize, Deserialize)]
struct User {
    id: u32,
    name: String,
}

// 获取用户
async fn get_user(id: web::Path<u32>) -> impl Responder {
    let user = User {
        id: *id,
        name: format!("User {}", id),
    };
    web::Json(user)
}

// 创建用户
async fn create_user(user: web::Json<User>) -> impl Responder {
    println!("Creating user: {} with ID {}", user.name, user.id);
    web::Json(user.into_inner())
}

#[actix_web::main]
async fn main() -> std::io::Result<()> {
    HttpServer::new(|| {
        App::new()
            .route("/users/{id}", web::get().to(get_user))
            .route("/users", web::post().to(create_user))
    })
    .bind("127.0.0.1:8080")?
    .run()
    .await
}
```

**测试方法**：
```bash
# 获取用户
curl http://127.0.0.1:8080/users/1
# 输出：{"id":1,"name":"User 1"}

# 创建用户
curl -X POST http://127.0.0.1:8080/users \
  -H "Content-Type: application/json" \
  -d '{"id":2,"name":"Alice"}'
# 输出：{"id":2,"name":"Alice"}
```

---

## 🔗 第四部分：与异步编程的联系

### Actix-web 使用异步的地方

```rust
#[actix_web::main]  // ← 创建 tokio 运行时（和 #[tokio::main] 类似）
async fn main() -> std::io::Result<()> {
    //↑ async main

    HttpServer::new(|| {
        App::new()
            .route("/", web::get().to(handler))
            //                         ↑ handler 是 async fn
    })
    .bind("127.0.0.1:8080")?
    .run()
    .await  // ← 异步等待
}

async fn handler() -> impl Responder {
//↑ async fn
    // 可以在这里调用其他异步函数
    let data = fetch_data().await;  // ← 可以用 .await
    web::Json(data)
}
```

### 为什么 Web 服务器需要异步？

**场景**：服务器同时处理1000个请求

**同步方式**（阻塞）：
- 需要1000个线程
- 每个线程占用内存（约2MB）
- 总内存：约2GB
- 线程切换开销大

**异步方式**（非阻塞）：
- 少量工作线程（通常是 CPU 核心数）
- 处理1000个异步任务
- 等待时不占用线程
- 高效利用资源

**结论**：Web 服务器是异步的最佳应用场景！

---

## 📊 第五部分：Actix-web vs 你学过的内容

### 与 CLI 工具的对比

| 特性 | CLI 工具 | Web API |
|------|----------|---------|
| 输入 | 命令行参数 | HTTP 请求 |
| 输出 | 终端输出 | HTTP 响应 |
| 交互 | 一次性执行 | 持续运行 |
| 并发 | 单个任务 | 多个并发请求 |
| 例子 | translate-cli | Actix-web API |

**共同点**：
- 都处理输入输出
- 都可以用异步
- 都可以用 serde 处理 JSON

**差异点**：
- CLI：命令行交互
- Web：HTTP 请求响应

---

### 与异步编程的联系

**你已经学过**（第22-23天）：
```rust
#[tokio::main]
async fn main() {
    let result = async_function().await;
}
```

**Actix-web**（今天学习）：
```rust
#[actix_web::main]  // ← 和 tokio::main 类似
async fn main() {
    HttpServer::new(|| App::new())
        .run()
        .await  // ← 和之前学的 .await 一样
}
```

**关键发现**：
- `#[actix_web::main]` 内部使用 tokio
- 所有 handler 都是 `async fn`
- 可以在 handler 中使用 `.await`
- 你的异步知识直接能用！

---

## 🎯 第六部分：学习检查清单

完成学习后，确认你理解：

**基本概念**：
- [ ] HttpServer 的作用是什么？（创建 HTTP 服务器）
- [ ] App 用来做什么？（配置路由和应用）
- [ ] Route 是什么？（将请求映射到处理函数）
- [ ] Handler 是什么？（处理请求的异步函数）

**代码结构**：
- [ ] 如何创建一个基本的 Web 服务器？
- [ ] 如何定义路由？
- [ ] 如何返回 JSON 响应？
- [ ] 如何提取路径参数？

**与异步的联系**：
- [ ] Actix-web 是否基于异步？（是）
- [ ] Handler 是否是异步函数？（是）
- [ ] 能否在 handler 中使用 .await？（能）

**全部理解 → 学习完成！继续下一步 ✅**

---

## 📚 扩展阅读（可选）

**官方资源**：
- 官方文档：https://actix.rs/
- GitHub：https://github.com/actix/actix-web
- 示例代码：https://github.com/actix/examples

**推荐看的示例**（按顺序）：
1. Hello World
2. JSON 响应
3. 路由参数
4. POST 请求处理
5. 错误处理

**不用全看完**，了解基本模式就够了！

---

**Actix-web 学习完成！你现在理解了 Web 框架的基本概念！** 🎉

下一步：规划你的第一个 Web API 项目！🚀
