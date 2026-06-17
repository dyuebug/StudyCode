# Todo List API 项目规划

**日期**：2026-06-20（第24天）
**项目名称**：todo-api
**目标**：创建一个完整的 RESTful API，掌握 Web 开发基础

---

## 🎯 项目目标

### 功能目标

实现一个 Todo List（待办事项）管理 API，包含完整的 CRUD 操作：

- ✅ **C**reate - 创建任务
- ✅ **R**ead - 读取任务（单个和列表）
- ✅ **U**pdate - 更新任务
- ✅ **D**elete - 删除任务

### 学习目标

- 掌握 Actix-web 框架的实际应用
- 理解 RESTful API 设计原则
- 实践异步编程在 Web 中的应用
- 学习错误处理和状态管理
- 了解 HTTP 方法和状态码

---

## 📋 API 接口设计

### 1. 获取所有任务

```
GET /todos
```

**响应示例**：
```json
[
  {
    "id": 1,
    "title": "学习 Rust",
    "completed": false
  },
  {
    "id": 2,
    "title": "写代码",
    "completed": true
  }
]
```

**状态码**：
- 200 OK - 成功

---

### 2. 获取单个任务

```
GET /todos/:id
```

**请求示例**：
```
GET /todos/1
```

**响应示例**：
```json
{
  "id": 1,
  "title": "学习 Rust",
  "completed": false
}
```

**状态码**：
- 200 OK - 成功
- 404 Not Found - 任务不存在

---

### 3. 创建任务

```
POST /todos
```

**请求体**：
```json
{
  "title": "学习 Actix-web"
}
```

**响应示例**：
```json
{
  "id": 3,
  "title": "学习 Actix-web",
  "completed": false
}
```

**状态码**：
- 201 Created - 创建成功
- 400 Bad Request - 请求数据无效

---

### 4. 更新任务

```
PUT /todos/:id
```

**请求体**：
```json
{
  "title": "学习 Actix-web",
  "completed": true
}
```

**响应示例**：
```json
{
  "id": 3,
  "title": "学习 Actix-web",
  "completed": true
}
```

**状态码**：
- 200 OK - 更新成功
- 404 Not Found - 任务不存在
- 400 Bad Request - 请求数据无效

---

### 5. 删除任务

```
DELETE /todos/:id
```

**响应示例**：
```json
{
  "message": "Todo deleted successfully"
}
```

**状态码**：
- 200 OK - 删除成功
- 404 Not Found - 任务不存在

---

## 🏗️ 数据模型设计

### Todo 结构

```rust
use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize, Clone)]
struct Todo {
    id: u32,
    title: String,
    completed: bool,
}
```

**字段说明**：
- `id`：任务唯一标识符（自动生成）
- `title`：任务标题（必填）
- `completed`：是否完成（默认 false）

---

### 请求数据结构

**创建任务**：
```rust
#[derive(Deserialize)]
struct CreateTodo {
    title: String,
}
```

**更新任务**：
```rust
#[derive(Deserialize)]
struct UpdateTodo {
    title: Option<String>,
    completed: Option<bool>,
}
```

---

### 响应数据结构

**标准响应**：
```rust
#[derive(Serialize)]
struct ApiResponse {
    message: String,
}
```

**错误响应**：
```rust
#[derive(Serialize)]
struct ErrorResponse {
    error: String,
}
```

---

## 📁 项目结构

```
todo-api/
├── Cargo.toml              # 依赖配置
├── README.md               # 项目文档
└── src/
    ├── main.rs            # 主程序（路由配置和服务器启动）
    ├── models.rs          # 数据模型定义
    ├── handlers.rs        # 请求处理函数
    └── state.rs           # 应用状态管理（存储 todos）
```

**文件职责**：
- `main.rs`：配置路由，启动服务器
- `models.rs`：定义数据结构
- `handlers.rs`：实现业务逻辑
- `state.rs`：管理共享状态（内存存储）

---

## 🔧 技术栈

### 依赖库

```toml
[dependencies]
actix-web = "4"
tokio = { version = "1", features = ["full"] }
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
```

**说明**：
- `actix-web`：Web 框架
- `tokio`：异步运行时
- `serde`：序列化/反序列化
- `serde_json`：JSON 处理

---

### 数据存储

**第一版：内存存储**
- 使用 `Vec<Todo>` 存储任务
- 使用 `Mutex` 保证线程安全
- 使用 `Arc` 实现共享

**结构**：
```rust
use std::sync::{Arc, Mutex};

type AppState = Arc<Mutex<Vec<Todo>>>;
```

**优点**：
- 简单易实现
- 无需数据库配置
- 专注学习 Web 框架

**缺点**：
- 重启后数据丢失
- 不适合生产环境

**未来扩展**：
- 可以添加 SQLite 数据库
- 可以添加 Redis 缓存
- 可以添加持久化

---

## 🎨 核心代码结构

### main.rs（主程序）

```rust
use actix_web::{web, App, HttpServer};
use std::sync::{Arc, Mutex};

mod models;
mod handlers;
mod state;

#[actix_web::main]
async fn main() -> std::io::Result<()> {
    // 创建共享状态
    let app_state = Arc::new(Mutex::new(Vec::new()));

    println!("🚀 Starting server at http://127.0.0.1:8080");

    HttpServer::new(move || {
        App::new()
            .app_data(web::Data::new(app_state.clone()))
            // 路由配置
            .route("/todos", web::get().to(handlers::get_all))
            .route("/todos", web::post().to(handlers::create))
            .route("/todos/{id}", web::get().to(handlers::get_one))
            .route("/todos/{id}", web::put().to(handlers::update))
            .route("/todos/{id}", web::delete().to(handlers::delete))
    })
    .bind("127.0.0.1:8080")?
    .run()
    .await
}
```

---

### handlers.rs（处理函数）

```rust
use actix_web::{web, HttpResponse, Responder};

// 获取所有任务
pub async fn get_all(
    data: web::Data<AppState>
) -> impl Responder {
    let todos = data.lock().unwrap();
    HttpResponse::Ok().json(&*todos)
}

// 创建任务
pub async fn create(
    data: web::Data<AppState>,
    todo: web::Json<CreateTodo>,
) -> impl Responder {
    let mut todos = data.lock().unwrap();
    let new_id = todos.len() as u32 + 1;
    let new_todo = Todo {
        id: new_id,
        title: todo.title.clone(),
        completed: false,
    };
    todos.push(new_todo.clone());
    HttpResponse::Created().json(new_todo)
}

// 其他 handler...
```

---

## 🧪 测试计划

### 测试工具

**选项1：curl 命令**
```bash
# 获取所有任务
curl http://localhost:8080/todos

# 创建任务
curl -X POST http://localhost:8080/todos \
  -H "Content-Type: application/json" \
  -d '{"title":"学习 Rust"}'

# 获取单个任务
curl http://localhost:8080/todos/1

# 更新任务
curl -X PUT http://localhost:8080/todos/1 \
  -H "Content-Type: application/json" \
  -d '{"completed":true}'

# 删除任务
curl -X DELETE http://localhost:8080/todos/1
```

**选项2：创建测试脚本**
- 写一个 bash 脚本
- 自动测试所有 API
- 验证响应结果

---

### 测试场景

**基本功能测试**：
1. ✅ 创建任务 → 返回 201
2. ✅ 获取所有任务 → 返回列表
3. ✅ 获取单个任务 → 返回任务详情
4. ✅ 更新任务 → 返回更新后的任务
5. ✅ 删除任务 → 返回成功消息

**错误处理测试**：
1. ✅ 获取不存在的任务 → 返回 404
2. ✅ 更新不存在的任务 → 返回 404
3. ✅ 删除不存在的任务 → 返回 404
4. ✅ 创建空标题的任务 → 返回 400

**边界测试**：
1. ✅ 创建100个任务 → 正常工作
2. ✅ 删除所有任务 → 列表为空
3. ✅ 更新部分字段 → 其他字段不变

---

## 📈 开发计划

### 第1阶段：基础框架（明天，第25天）

**目标**：搭建项目骨架，实现最基本的功能

**任务**：
1. 创建项目结构
2. 配置依赖
3. 定义数据模型
4. 实现 GET /todos（获取所有任务）
5. 实现 POST /todos（创建任务）
6. 测试基本功能

**预计时长**：2-3小时

---

### 第2阶段：完整CRUD（第26天）

**目标**：实现所有 API 接口

**任务**：
1. 实现 GET /todos/:id
2. 实现 PUT /todos/:id
3. 实现 DELETE /todos/:id
4. 添加错误处理
5. 完整测试

**预计时长**：2-3小时

---

### 第3阶段：优化和扩展（第27天，可选）

**目标**：改进项目质量

**可选任务**：
1. 添加输入验证
2. 改进错误消息
3. 添加日志记录
4. 添加测试用例
5. 编写文档

**预计时长**：2小时

---

## 🎯 学习检查清单

完成规划后，确认你理解：

**API 设计**：
- [ ] 知道要实现哪些接口？（5个：GET/POST/GET/:id/PUT/:id/DELETE/:id）
- [ ] 理解 RESTful 设计原则？（资源、HTTP 方法、状态码）
- [ ] 知道如何设计请求和响应？（JSON 格式）

**数据模型**：
- [ ] 知道 Todo 有哪些字段？（id, title, completed）
- [ ] 理解如何存储数据？（Vec + Mutex + Arc）

**项目结构**：
- [ ] 知道需要哪些文件？（main.rs, models.rs, handlers.rs）
- [ ] 理解每个文件的职责？（路由、模型、处理）

**技术栈**：
- [ ] 知道需要哪些依赖？（actix-web, tokio, serde）
- [ ] 理解为什么需要它们？（Web框架、异步、JSON）

**全部理解 → 规划完成！准备开发 ✅**

---

## 💡 关键设计决策

### 为什么用内存存储？

**优点**：
- ✅ 简单易实现
- ✅ 无需配置数据库
- ✅ 专注学习 Web 框架
- ✅ 快速迭代

**缺点**：
- ❌ 重启后数据丢失
- ❌ 不支持并发扩展
- ❌ 不适合生产环境

**未来扩展**：学会框架后，可以轻松添加数据库

---

### 为什么选择 Todo List？

**理由**：
1. **简单易懂**：每个人都知道待办事项是什么
2. **涵盖全面**：包含所有 CRUD 操作
3. **容易测试**：功能清晰，容易验证
4. **易于扩展**：可以添加用户、分类、优先级等

---

### RESTful API 设计原则

**核心原则**：
1. **资源导向**：URL 表示资源（/todos）
2. **HTTP 方法**：使用标准方法（GET/POST/PUT/DELETE）
3. **状态码**：使用标准状态码（200/201/404/400）
4. **无状态**：每个请求独立，不依赖服务器状态
5. **统一接口**：一致的 API 设计

**示例**：
```
GET    /todos      - 获取所有资源
POST   /todos      - 创建新资源
GET    /todos/:id  - 获取单个资源
PUT    /todos/:id  - 更新资源
DELETE /todos/:id  - 删除资源
```

---

## 🚀 准备开始

你现在已经：
- ✅ 理解了 Actix-web 框架
- ✅ 设计了完整的 API 接口
- ✅ 规划了项目结构
- ✅ 确定了技术栈
- ✅ 制定了开发计划

**明天就可以开始编码了！** 🎉

---

**项目规划完成！准备好迎接第一个 Web API 项目了吗？** 🚀
