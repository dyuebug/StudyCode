# 异步编程核心概念快速复习

**日期**：2026-06-20（第24天）

---

## 🎯 核心概念回顾

### 1. async/await 语法

**async fn**：
```rust
// async fn 返回 Future，不是直接返回值
async fn fetch_data() -> String {
    // 异步操作
    "data".to_string()
}

// 等价于
fn fetch_data() -> impl Future<Output = String> {
    // ...
}
```

**await**：
```rust
// .await 等待 Future 完成
let result = fetch_data().await;

// 关键理解：
// - .await 不是"阻塞"，而是"挂起"
// - 挂起时运行时可以执行其他任务
// - 不浪费 CPU 时间
```

---

### 2. tokio 运行时

**作用**：
- 维护任务队列
- 反复 poll 每个 Future
- Future 完成时返回结果
- Future 未完成时切换任务

**使用方式**：
```rust
#[tokio::main]  // 宏创建运行时
async fn main() {
    // 异步代码
}

// 等价于
fn main() {
    let rt = tokio::runtime::Runtime::new().unwrap();
    rt.block_on(async {
        // 异步代码
    });
}
```

---

### 3. 异步改造三步法

**步骤1：函数定义加 async**
```rust
// 同步
fn translate_text(...) -> Result<...> { }

// 异步
async fn translate_text(...) -> Result<...> { }
//↑ 加 async
```

**步骤2：异步操作加 .await**
```rust
// 同步
let response = client.get(url).send()?;

// 异步
let response = client.get(url).send().await?;
//                                    ↑ 加 .await
```

**步骤3：main 函数加宏和 async**
```rust
// 同步
fn main() { }

// 异步
#[tokio::main]
async fn main() { }
//↑ 加宏      ↑ 加 async
```

---

### 4. 并发执行

**顺序执行（慢）**：
```rust
for item in items {
    process(item).await;  // 一个一个来
}
// 总时间 = 每个时间之和
```

**并发执行（快）**：
```rust
use futures::future::join_all;

// 创建所有 Future
let tasks: Vec<_> = items.iter()
    .map(|item| process(item))
    .collect();

// 并发执行
let results = join_all(tasks).await;
// 总时间 ≈ 最慢的那个
```

---

### 5. 性能对比（实测数据）

**第23天的测试结果**：
- 单个请求：17.8倍提升 🚀
- 批量请求：4.98倍提升 ⚡
- 并发效率：99.6%

**核心原因**：
- 不是"执行得更快"
- 而是"不浪费等待时间"
- 等待时可以做其他事

---

### 6. 何时使用异步

**适合异步**：✅
- 网络请求（HTTP、数据库）
- 文件 I/O
- 多个 API 调用
- 等待时间长的操作

**不适合异步**：❌
- CPU 密集计算（用多线程）
- 简单脚本
- 单个请求
- 没有等待操作

---

### 7. 关键理解

**Future 概念**：
- Future = "未来的值"
- 就像餐厅的号码牌
- 不会自动执行
- 需要运行时去 poll

**async fn 的本质**：
```rust
async fn hello() -> String {
    "Hello".to_string()
}

// 调用它
let future = hello();  // 返回 Future，还没执行
let result = future.await;  // 现在才执行
```

**.await 的本质**：
- 不是阻塞（blocking）
- 而是挂起（suspend）
- "礼貌让座"
- 让运行时执行其他任务

---

### 8. 异步 vs 多线程

| 特性 | 多线程 | 异步 |
|------|--------|------|
| 适用场景 | CPU 密集 | I/O 密集 |
| 执行方式 | 真正并行 | 并发（切换） |
| 开销 | 大（创建线程） | 小（任务切换） |
| 例子 | 图像处理 | 网络请求 |

**第16天学习**：多线程并发
**第22-23天学习**：异步并发

**两者可以结合使用**！

---

## 🔗 与 Web 开发的联系

**重要发现**：
- ✅ Actix-web 是基于异步的！
- ✅ 使用 `#[actix_web::main]` 宏
- ✅ 所有处理函数都是 `async fn`
- ✅ 你昨天学的知识直接能用！

**示例**：
```rust
use actix_web::{web, App, HttpServer, Responder};

// ← 异步函数！
async fn hello() -> impl Responder {
    "Hello, World!"
}

#[actix_web::main]  // ← 创建异步运行时
async fn main() -> std::io::Result<()> {
    HttpServer::new(|| {
        App::new()
            .route("/", web::get().to(hello))
    })
    .bind("127.0.0.1:8080")?
    .run()
    .await  // ← 异步等待！
}
```

**关键洞察**：
- Web 服务器需要处理大量并发请求
- 这正是异步的最佳应用场景
- 你已经掌握了异步，Web 开发会很顺利！

---

## ✅ 复习检查清单

完成复习后，确认你能回答：
- [ ] async fn 返回什么？（Future）
- [ ] .await 的作用是什么？（等待 Future 完成，但不阻塞）
- [ ] 异步改造需要几步？（3步）
- [ ] 并发执行用什么工具？（join_all）
- [ ] 异步适合什么场景？（I/O 密集）
- [ ] Actix-web 是否基于异步？（是）

**全部能回答 → 复习完成！继续下一步 ✅**

---

**复习完成！现在你已经做好准备，可以开始学习 Web 开发了！** 🚀
