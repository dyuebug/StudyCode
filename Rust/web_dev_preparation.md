# Web 开发环境准备清单

**日期**：2026-06-20（第24天）
**目标**：为明天的 Web 开发做好准备

---

## ✅ 准备清单

### 1. 依赖库确认

**需要的依赖**：
```toml
[package]
name = "todo-api"
version = "0.1.0"
edition = "2024"

[dependencies]
actix-web = "4"
tokio = { version = "1", features = ["full"] }
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
```

**说明**：
- `actix-web = "4"`：最新的 Actix-web 4.x 版本
- `tokio`：异步运行时（Actix-web 内部使用）
- `serde`：序列化/反序列化框架
- `serde_json`：JSON 处理

**检查**：
- ✅ 版本号明确
- ✅ 所有必要的 features 已包含
- ✅ 与你之前学的库一致（serde、tokio）

---

### 2. API 测试工具

**选项1：curl 命令（推荐）**

**优点**：
- ✅ 系统自带，无需安装
- ✅ 命令行操作，快速方便
- ✅ 易于脚本化

**基本用法**：
```bash
# GET 请求
curl http://localhost:8080/todos

# POST 请求（JSON）
curl -X POST http://localhost:8080/todos \
  -H "Content-Type: application/json" \
  -d '{"title":"学习 Rust"}'

# PUT 请求
curl -X PUT http://localhost:8080/todos/1 \
  -H "Content-Type: application/json" \
  -d '{"completed":true}'

# DELETE 请求
curl -X DELETE http://localhost:8080/todos/1

# 格式化输出（可选）
curl http://localhost:8080/todos | jq
```

**检查**：
- ✅ curl 命令可用
- ✅ 理解基本用法
- ✅ 知道如何发送 JSON 数据

---

**选项2：创建测试脚本**

创建 `test_api.sh`：
```bash
#!/bin/bash

BASE_URL="http://localhost:8080"

echo "========================================="
echo "Testing Todo API"
echo "========================================="

# 测试1：创建任务
echo -e "\n1. Creating todo..."
curl -X POST $BASE_URL/todos \
  -H "Content-Type: application/json" \
  -d '{"title":"学习 Rust"}'

# 测试2：获取所有任务
echo -e "\n\n2. Getting all todos..."
curl $BASE_URL/todos

# 测试3：获取单个任务
echo -e "\n\n3. Getting todo by id..."
curl $BASE_URL/todos/1

# 测试4：更新任务
echo -e "\n\n4. Updating todo..."
curl -X PUT $BASE_URL/todos/1 \
  -H "Content-Type: application/json" \
  -d '{"completed":true}'

# 测试5：删除任务
echo -e "\n\n5. Deleting todo..."
curl -X DELETE $BASE_URL/todos/1

echo -e "\n\n========================================="
echo "Tests completed!"
echo "========================================="
```

**检查**：
- ✅ 脚本已创建（明天可用）
- ✅ 包含所有 API 测试
- ✅ 易于运行和修改

---

### 3. 项目结构规划

**目录结构**：
```
todo-api/
├── Cargo.toml              # 项目配置
├── README.md               # 项目文档
├── test_api.sh             # API 测试脚本
└── src/
    ├── main.rs            # 主程序（约100行）
    ├── models.rs          # 数据模型（约50行）
    ├── handlers.rs        # 请求处理（约150行）
    └── state.rs           # 状态管理（约30行）
```

**文件职责**：

**main.rs**：
- 导入模块
- 创建共享状态
- 配置路由
- 启动服务器

**models.rs**：
- Todo 结构体
- CreateTodo 结构体
- UpdateTodo 结构体
- 响应结构体

**handlers.rs**：
- get_all：获取所有任务
- get_one：获取单个任务
- create：创建任务
- update：更新任务
- delete：删除任务

**state.rs**：
- AppState 类型定义
- 状态管理辅助函数

**检查**：
- ✅ 理解项目结构
- ✅ 知道每个文件的职责
- ✅ 清楚代码组织方式

---

### 4. 数据模型设计

**核心数据结构**：

```rust
// models.rs

use serde::{Deserialize, Serialize};

/// Todo 任务
#[derive(Serialize, Deserialize, Clone, Debug)]
pub struct Todo {
    pub id: u32,
    pub title: String,
    pub completed: bool,
}

/// 创建 Todo 的请求数据
#[derive(Deserialize, Debug)]
pub struct CreateTodo {
    pub title: String,
}

/// 更新 Todo 的请求数据
#[derive(Deserialize, Debug)]
pub struct UpdateTodo {
    pub title: Option<String>,
    pub completed: Option<bool>,
}

/// 标准响应
#[derive(Serialize)]
pub struct ApiResponse {
    pub message: String,
}

/// 错误响应
#[derive(Serialize)]
pub struct ErrorResponse {
    pub error: String,
}
```

**检查**：
- ✅ 理解每个结构体的用途
- ✅ 知道为什么需要不同的结构体
- ✅ 理解 serde 的作用

---

### 5. 状态管理设计

**共享状态**：

```rust
// state.rs

use std::sync::{Arc, Mutex};
use crate::models::Todo;

/// 应用状态：存储所有 todos
pub type AppState = Arc<Mutex<Vec<Todo>>>;

/// 创建初始状态
pub fn create_state() -> AppState {
    Arc::new(Mutex::new(Vec::new()))
}
```

**关键理解**：
- `Vec<Todo>`：存储所有任务
- `Mutex`：保证线程安全（同一时间只有一个线程可以修改）
- `Arc`：允许多个线程共享（原子引用计数）

**为什么需要这样？**
- Web 服务器是多线程的
- 多个请求可能同时到达
- 需要保证数据安全

**检查**：
- ✅ 理解 Arc<Mutex<Vec<T>>> 的作用
- ✅ 知道为什么需要线程安全
- ✅ 回想第16天学的并发知识

---

### 6. 路由配置规划

**路由表**：

```rust
// main.rs（路由配置部分）

App::new()
    .app_data(web::Data::new(app_state.clone()))
    // 路由配置
    .route("/todos", web::get().to(handlers::get_all))
    .route("/todos", web::post().to(handlers::create))
    .route("/todos/{id}", web::get().to(handlers::get_one))
    .route("/todos/{id}", web::put().to(handlers::update))
    .route("/todos/{id}", web::delete().to(handlers::delete))
```

**路由映射表**：

| HTTP方法 | 路径 | 处理函数 | 功能 |
|----------|------|----------|------|
| GET | /todos | get_all | 获取所有任务 |
| POST | /todos | create | 创建任务 |
| GET | /todos/:id | get_one | 获取单个任务 |
| PUT | /todos/:id | update | 更新任务 |
| DELETE | /todos/:id | delete | 删除任务 |

**检查**：
- ✅ 理解路由配置方式
- ✅ 知道如何映射 HTTP 方法
- ✅ 理解路径参数（:id）

---

### 7. 开发流程规划

**明天的开发步骤**：

1. **创建项目**（5分钟）
   ```bash
   cargo new todo-api
   cd todo-api
   ```

2. **配置依赖**（5分钟）
   - 编辑 Cargo.toml
   - 添加所有依赖

3. **创建文件结构**（5分钟）
   ```bash
   touch src/models.rs
   touch src/handlers.rs
   touch src/state.rs
   ```

4. **实现数据模型**（20分钟）
   - 定义 Todo 结构
   - 定义请求/响应结构

5. **实现状态管理**（10分钟）
   - 定义 AppState
   - 创建初始化函数

6. **实现基础 handler**（40分钟）
   - get_all（获取所有）
   - create（创建）

7. **配置路由和启动**（20分钟）
   - 配置路由
   - 启动服务器

8. **测试**（20分钟）
   - 测试 GET /todos
   - 测试 POST /todos

**总预计时间**：2-2.5小时

**检查**：
- ✅ 理解开发流程
- ✅ 知道每步要做什么
- ✅ 时间估算合理

---

## 🔍 知识回顾

### 与之前学习的联系

**第16天（并发编程）**：
```rust
// 学过的
Arc<Mutex<T>>  // 多线程共享可变数据
```

**今天（Web开发）**：
```rust
// 应用
type AppState = Arc<Mutex<Vec<Todo>>>;  // Web多线程共享状态
```

---

**第22-23天（异步编程）**：
```rust
// 学过的
#[tokio::main]
async fn main() { }

async fn handler() -> Result<T> {
    something.await
}
```

**今天（Web开发）**：
```rust
// 应用
#[actix_web::main]
async fn main() { }

async fn handler() -> impl Responder {
    // 可以用 .await
}
```

---

**第19-21天（CLI工具）**：
```rust
// 学过的
use serde::{Serialize, Deserialize};

#[derive(Serialize, Deserialize)]
struct Data { }
```

**今天（Web开发）**：
```rust
// 应用
#[derive(Serialize, Deserialize)]
struct Todo { }  // JSON 请求/响应
```

**关键发现**：
- ✅ 你学的所有知识都能用上！
- ✅ Web 开发整合了所有技能
- ✅ 并发 + 异步 + JSON = Web API

---

## 📝 准备检查清单

完成准备后，确认：

**环境准备**：
- [ ] 知道需要哪些依赖库
- [ ] 理解如何测试 API（curl）
- [ ] 清楚项目文件结构

**概念理解**：
- [ ] 理解 RESTful API 设计
- [ ] 理解数据模型设计
- [ ] 理解状态管理（Arc<Mutex<Vec<T>>>）

**开发计划**：
- [ ] 知道明天的开发步骤
- [ ] 理解每步的时间分配
- [ ] 准备好开始编码

**知识联系**：
- [ ] 回顾了并发知识（Arc + Mutex）
- [ ] 回顾了异步知识（async/await）
- [ ] 回顾了 JSON 知识（serde）

**全部完成 → 准备就绪！明天开始编码 ✅**

---

## 🎉 今日总结

**完成的任务**：
1. ✅ 复习异步编程核心概念
2. ✅ 了解 Actix-web 框架
3. ✅ 规划 Todo List API 项目
4. ✅ 准备开发环境

**学习时长**：约2-2.5小时

**收获**：
- 巩固了异步编程知识
- 理解了 Web 框架基础
- 设计了完整的 API
- 准备好 Web 开发

**明天目标**：
- 开始编码
- 实现基础 API
- 测试功能

---

**准备工作全部完成！明天就开始真正的 Web 开发了！** 🚀🎉

**休息一下，明天见！** 😊
