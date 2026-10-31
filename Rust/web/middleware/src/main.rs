// ============================================
// Actix-web 中间件 —— 超详细注释版
// 日期：2026-10-30（第35天）
// 难度：⭐⭐⭐ 高级
// ============================================

/*
【核心概念】简要说明

是什么？
- 中间件 = 包在路由树外面的**一层服务**。请求穿过它进来，响应穿过它出去，
  两头都能插一手，也可以直接短路（不叫内层）。

为什么需要？
- 有一类逻辑不属于任何单个 handler，却需要出现在很多 handler 周围：
  日志、鉴权、限流、超时、压缩、CORS、请求 ID、panic 兜底。
- 把它们写进每个 handler → 重复 N 遍，且总有几个忘了写。
- 中间件的价值不只是"少写代码"，而是**不能被绕过**：
  handler 就算写错了，也逃不出中间件那一层。

核心特点？
- **洋葱模型**：先 wrap 的在最外层，请求先进后出，响应后进先出。
- **透明**：好的中间件进来什么响应、出去还是什么，不篡改 body。
- **可短路**：返回响应而不调用内层（鉴权 401 就是这条路）。
- **按作用域挂**：`App::wrap` 全应用，`Scope::wrap` 只包那棵子树。

Rust 特色？
- actix-web 的中间件不是"函数回调"，而是三个 trait 的组合：
  `Service`（能处理请求的东西）、`Transform`（把 Service 包成新 Service 的工厂）、
  `ServiceFactory`（App 内部用，用户很少直接写）。
- 好处：**编译期组合，零虚函数调用**，一层中间件就是一个泛型类型嵌套。
  代价：类型签名写起来吓人，报错位置通常离真正的问题很远。

【本文件的结构】与 `realtime/ws-basics` 一致：
- 中间件实现在 `src/mw/logging.rs`（日志）与 `src/mw/auth.rs`（鉴权 + 提取器），带逐行注释
- 本文件只放"怎么用"：路由装配、六部分教学材料、练习题
- 想要完整逐行讲解，去读 `src/mw/` 下的 logging.rs 与 auth.rs
*/

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐（必学）
// ============================================

// --------------------------------------------
// 1.1 中间件解决的是什么问题
// --------------------------------------------
/*
把"日志"写进 handler 会发生什么：

```rust
async fn get_todos() -> impl Responder {
    let start = Instant::now();
    let res = HttpResponse::Ok().json(vec![...]);
    println!("GET /todos -> 200 ({} ms)", start.elapsed().as_millis());
    res
}
```

问题不在写法，而在**覆盖**：
- 新增一个接口 → 忘了加日志
- 有一个接口 404 了 → 它没进 handler，根本没日志
- 鉴权中间件拒绝了请求 → handler 没执行，日志里看不到"谁被拒了"

中间件放在 handler **外面**，所以：
- 404 也能记到（路由匹配失败发生在内层）
- 401 也能记到（鉴权中间件短路时，外层日志仍会被 await）
- 新增接口自动被覆盖，**不需要记得**

这就是"不能被绕过"的实际含义。
*/

// --------------------------------------------
// 1.2 三个 trait 的分工
// --------------------------------------------
/*
| trait | 一句话 | 什么时候调用 |
|---|---|---|
| `Service<Req>` | "能处理一个请求"的东西。`call(req) -> Future<Response>` | 每个请求 |
| `Transform<S, Req>` | "把 S 包成新 Service" 的工厂。`new_transform(S) -> Future<Service>` | 应用启动时，每个 worker 一次 |
| `ServiceFactory<Req>` | 管理"配置 → 建 Service"的订阅生命周期 | 由 App 内部使用，几乎不用自己写 |

最小实现 = 两个类型 + 三个关联类型：

```rust
struct MyMw;                                  // 配置
struct MyMwService<S> { inner: S }            // 干活

impl<S, B> Transform<S, ServiceRequest> for MyMw { ... }
impl<S, B> Service<ServiceRequest> for MyMwService<S> { ... }
```

⚠️ **`new_transform` 里不要放每个请求都要做的事**。
它每个 worker 只跑一次，你在这里读 `req` 根本读不到（没有 req 参数）。
想"每个请求都做"的逻辑，必须写在 `Service::call` 里。
这个错位是新手最常见的理解错误，症状是"日志只打印了一次"。
*/

// --------------------------------------------
// 1.3 洋葱模型：`wrap` 的顺序（⚠️ 与直觉相反）
// --------------------------------------------
/*
【实测结论，2026-10-30，本项目】

```rust
App::new()
    .wrap(Logging::with_prefix("[INNER]"))   // 先写
    .wrap(Logging::with_prefix("[OUTER]"))   // 后写
```

请求 `GET /health` 的输出：

```text
[INNER] GET /health → 200 OK (0 ms)
[OUTER] GET /health → 200 OK (0 ms)
```

两条日志都在**响应阶段**打印（中间件先 await 内层，再打日志）。
响应是"后进先出"，先打印说明 `[INNER]` 更靠近 handler。

→ **后注册的 `wrap` 在外层；先写的离 handler 更近。**

这不是文档里的说法，是实测得出的。原因在源码里：`App::wrap` 每次
都立刻把 `self.endpoint` 包一层（`App { endpoint: apply(mw, self.endpoint) }`），
所以第二次 `wrap` 包住的是"已经被包过的服务"——它在**更外面**。

一次请求的完整路径：

```text
请求 ──→ [OUTER] 进入 ──→ [INNER] 进入 ──→ Auth ──→ handler ──→ 响应
                                                    │
                              Auth 拒绝直接返回 ────┘
                              （handler 不执行）

响应 ←── [OUTER] 退出 ←── [INNER] 退出 ←──────────────────────┘
```

三条推论：

1. **"要看到一切"的中间件必须最后写**。想让日志记到 401/404，
   `.wrap(Logging)` 就必须放在**所有 `wrap` 之后**。
   写在最前面 → 它在最里层 → 被鉴权拒掉的请求根本进不来，
   你会看到"客户端一直 401，服务端日志一条没有"。

2. **耗时统计包含谁，取决于顺序**。日志在 auth 外面 → 耗时 = 鉴权 + handler；
   在 auth 里面 → 耗时只有 handler。排查性能问题前，先确认自己在看哪个数字。

3. **短路只影响内层**。Auth 拒绝时，比它更外层的东西照常收到 401 响应，
   比它更内层的（包括中间件和 handler）一个都不执行。
   本项目 `/api` scope 下的 `[API]` 日志就是这样——它注册在 auth **之前**，
   所以在 auth 里面，401 不经过它（见下面的实测输出）。

一句话记忆：**写代码的顺序是从内到外，请求的方向是从外到内。**
*/

// --------------------------------------------
// 1.4 中间件 / 提取器 / guard 的分工
// --------------------------------------------
/*
三种"拦在 handler 前面"的机制，边界经常被搞混：

| 机制 | 能做什么 | 不能做什么 |
|---|---|---|
| 中间件 `wrap` | 看请求头、提前返回响应、给响应加头、统计耗时 | 拿已解析的 body；按路由参数区分 |
| 提取器 `FromRequest` | 把请求变成**类型**，失败转成响应 | 全局生效；在 handler 之前编排逻辑 |
| guard `.guard()` | 按条件决定"这条路由适不适用" | 返回自定义响应（只有匹配/不匹配） |
| `route.wrap` | 只包某一条路由 | 顺序可控性比 App 层更差 |

经验法则：
- **需要"不能被绕过"→ 中间件**（鉴权、日志）
- **需要"给我一个类型"→ 提取器**（`UserId`、`Json<T>`）
- **需要"这条路由收不收"→ guard**

本项目的鉴权 = 中间件（写入）+ 提取器（读出），
这个组合是 actix 里"认证 → 业务"最干净的写法。
*/

// --------------------------------------------
// 1.5 什么时候不该用中间件
// --------------------------------------------
/*
- **只对一个接口生效** → 直接在 handler 里做，或挂 `route.wrap`，
  别为了两行代码写两个类型。
- **要读 request body** → 慎重。body 是流，读一次就没了，
  中间件读完必须 `set_payload` 放回去；大文件上传还会整包进内存。
- **要访问数据库** → 中间件可以，但要清楚"每个请求都查一次库"的代价；
  多数场景应该放进提取器（只在需要用户信息时查）。
- **要做重活** → 中间件在请求路径上，慢 1 ms 就是所有请求都慢 1 ms。
*/

// ============================================
// 第二部分：基础操作实践 ⭐⭐⭐⭐⭐
// ============================================
//
// 本文件的代码就是"Part 2"。运行方式：
//
//   cargo run
//   # 另一个终端：
//   curl -i http://127.0.0.1:8082/health
//   curl -i http://127.0.0.1:8082/api/todos
//   curl -i -H "Authorization: Bearer secret-token" http://127.0.0.1:8082/api/todos
//   curl -i -H "Authorization: Bearer secret-token" http://127.0.0.1:8082/api/whoami
//   curl -i -H "Authorization: Bearer secret-token" http://127.0.0.1:8082/api/slow/300
//
// 逐条对照 README.md 里的实测输出。

use std::time::Duration;

use actix_web::http::header;
use actix_web::web;
use actix_web::{App, HttpResponse, HttpServer, Responder};
use serde::Deserialize;

mod mw;

use mw::auth::{Auth, UserId};
use mw::logging::Logging;

// --------------------------------------------
// 2.1 handler：注意它们有多"干净"
// --------------------------------------------
/*
对比第 1.1 节的反例：handler 里**没有**日志、没有鉴权、没有耗时统计。
它们只关心业务，横切关注点全部在 `wrap` 里。

这就是中间件带来的最直接收益：handler 的签名和函数体，
一眼就能看出它做什么。
*/

/// 公开端点：不挂鉴权。给负载均衡探活用。
async fn health() -> impl Responder {
    HttpResponse::Ok().json(serde_json::json!({
        "status": "ok",
        "service": "middleware-demo",
    }))
}

/// 首页：列出所有端点，方便手工测试。
async fn index() -> impl Responder {
    HttpResponse::Ok().json(serde_json::json!({
        "endpoints": [
            {"method": "GET",  "path": "/health",     "auth": false, "note": "公开"},
            {"method": "GET",  "path": "/api/todos",  "auth": true,  "note": "要 Bearer token"},
            {"method": "POST", "path": "/api/todos",  "auth": true,  "note": "要 Bearer token"},
            {"method": "GET",  "path": "/api/whoami", "auth": true,  "note": "回显中间件写入的身份"},
            {"method": "GET",  "path": "/api/slow/{ms}", "auth": true, "note": "故意睡 ms 毫秒，观察日志耗时"},
            {"method": "GET",  "path": "/debug/forgot-auth", "auth": false, "note": "漏挂鉴权中间件的现场"}
        ],
        "tokens": {"secret-token": "alice", "other-token": "bob"}
    }))
}

/*
【2.2 提取器读出中间件写入的身份】

`user: UserId` —— 就这一个参数。
- 没鉴权 → 提取器返回 401（本项目 `/debug/forgot-auth` 就是这个现场）
- 鉴权通过 → `user.0` 是中间件写入的用户名

handler 里**没有** `if !authorized { return 401 }`，
因为"能拿到 UserId"这件事本身，已经等价于"已鉴权"。
*/
async fn whoami(user: UserId) -> impl Responder {
    HttpResponse::Ok().json(serde_json::json!({
        "user": user.0,
        "source": "由 Auth 中间件写入 extensions，再由 UserId 提取器读出",
    }))
}

/// 故意睡一会儿：用来对比"日志耗时"与"实际业务耗时"。
async fn slow(path: web::Path<u64>) -> impl Responder {
    // 上限 3000 ms：手工测试时别把服务挂住。
    let ms = (*path).min(3_000);
    actix_web::rt::time::sleep(Duration::from_millis(ms)).await;
    HttpResponse::Ok().json(serde_json::json!({
        "slept_ms": ms,
        "note": "看服务端日志里的耗时字段，应当 ≈ slept_ms + 少量开销",
    }))
}

#[derive(Deserialize)]
struct CreateTodo {
    title: String,
}

/// 列表：无状态，每次返回同一份固定数据。
///
/// 刻意的设计取舍：本课的主题是**中间件**，
/// 引入数据库会把注意力从 `Transform`/`Service` 上移开（YAGNI）。
async fn list_todos() -> impl Responder {
    HttpResponse::Ok().json(serde_json::json!({
        "success": true,
        "data": [
            {"id": 1, "title": "读 sqlx-basics", "completed": true},
            {"id": 2, "title": "写 todo-api", "completed": true},
            {"id": 3, "title": "学中间件", "completed": false}
        ]
    }))
}

/// 新建：演示"中间件不碰 body，所以 `web::Json` 在 handler 里能正常工作"。
async fn create_todo(body: web::Json<CreateTodo>, user: UserId) -> impl Responder {
    // 没有数据库，id 用固定值；重点是 `body` 能被正常解析。
    // 如果鉴权中间件错误地消费了 payload，这里会报
    // `Json deserialize error: EOF while parsing a value`。
    println!("[API] {} 创建 todo: {}", user.0, body.title);
    HttpResponse::Created().json(serde_json::json!({
        "success": true,
        "data": {"id": 4, "title": body.title, "completed": false},
        "created_by": user.0
    }))
}

/// 漏挂鉴权中间件的现场。
///
/// 这条路由**没有**经过 `Auth`，但 handler 要求 `UserId`。
/// 提取器找不到 → 401 + 一条服务端警告。
/// 这类 bug 不会静默放过，它会在**第一次调用时**就报出来，
/// 这是"用类型表达意图"的另一个收益。
async fn forgot_auth(user: UserId) -> impl Responder {
    HttpResponse::Ok().json(serde_json::json!({ "user": user.0 }))
}

// ============================================
// 第三部分：深入理解（选学）
// ============================================
/*
🔍 深入理解 1：为什么是 `Pin<Box<dyn Future>>`，官方为什么用 `pin_project!`

`Service::call` 返回的 future 捕获了 `self`（中间件状态）和内层的 future。
想在 trait 关联类型里写这个类型，只有三条路：

a) `Pin<Box<dyn Future<Output = ...>>>` —— 每个请求一次堆分配（约几十 ns）。
   actix 一个请求本身就要分配很多次，这点开销可以忽略。**本文件用这条。**

b) 手写 future 结构体 + `pin_project!` —— 零分配，但要自己实现 `poll`，
   还要处理"内层 future 放在结构体里怎么被 pin 住"。
   官方 `Logger` 用的就是这条（`LoggerResponse` 结构体）。

c) 用 `async fn` in trait（Rust 1.75+）—— 返回 `impl Future`，
   无法写进 `Service` 的关联类型，所以**这条在 actix 走不通**。

结论：学习阶段用 (a)，性能敏感的高频中间件才考虑 (b)。

────────────────────────────────────

🔍 深入理解 2：`poll_ready` 与背压

`Service` 在 `call` 之前会被调一次 `poll_ready`。它的语义是：
"我现在能接活吗？" 返回 `Poll::Pending` 就是在说"等等"。

`forward_ready!(service)` 把这个问题原样转给内层。如果中间件不转发
（比如自己直接 `Poll::Ready(Ok(()))`），会出现：
- 内层已经过载，外层还继续收请求
- actix 的 worker 并发上限被绕过，队列在内存里无限增长

这是"中间件写错不会立刻坏，只会在压力大时炸"的典型例子。

────────────────────────────────────

🔍 深入理解 3：extensions 为什么能跨中间件传值

`HttpRequest` 内部是 `Rc<RefCell<Extensions>>`。
`Rc` → 克隆 `HttpRequest` 时共享同一份；`RefCell` → 拿到 `&self` 也能写。

两个直接后果：
1. 中间件写入，handler 读得到（同一个 `Rc`）
2. **`RefCell` 的借用规则是运行时的**：你拿着 `RefMut` 的同时再 `borrow_mut()`
   → **panic**，不是编译错误。

所以：`let mut ext = req.request().extensions_mut();` 之后**不要** `.await`，
`await` 期间其他任务可能去 borrow，直接 panic 掉整个 worker。
正确写法是"用完立即释放"（本项目就是这样：`insert` 完那一行就结束）。

────────────────────────────────────

🔍 深入理解 4：中间件的三种错误处理姿势

1. **短路**：直接返回响应（鉴权 401）。内层不执行。
2. **转换**：`res.map_body(...)` 改 body，或 `res.err()` 转成统一错误格式。
   注意改 body 会改变 `B` 的类型，需要 `EitherBody`。
3. **兜底**：`ServiceResponse` 是 `Err` 时，把它变成 500 + 统一 JSON。
   注意 `Err` 分支拿不到原响应体（已经在 `Error` 里了），只能记日志。

本项目只用了第 1 种。第 2、3 种是练习题的素材。
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================
/*
这 7 条全部来自"真的写错过"的场景。每条的错误代码都能编译复现，
把代码贴进 `src/mw/logging.rs` 改掉对应部分即可看到同样的报错。
*/

// --------------------------------------------
// 4.1 漏写 `'static` 约束 —— 报错出现在很远的地方
// --------------------------------------------
/*
❌ 错误代码（`impl` 的 where 子句少了 `+ 'static`）：

```rust
impl<S, B> Transform<S, ServiceRequest> for Logging
where
    S: Service<ServiceRequest, Response = ServiceResponse<B>, Error = Error>,
    B: MessageBody,
{ ... }
```

编译到 `HttpServer::new(...)` 那一行才报：

```text
error[E0310]: the parameter type `B` may not live long enough
   --> src/mw/logging.rs:...
    |
    |     B: MessageBody,
    |        ^^^^^^^^^^ ...so that the type `B` will meet its required lifetime bounds
    |
help: consider adding an explicit lifetime bound
    |
    = note: ...  `B: 'static`
```

原因：`HttpServer::new` 的闭包要返回 `'static` 的服务；
服务里保存着 `B` 的类型参数，所以 `B` 必须 `'static`。

✅ 正确写法：`B: MessageBody + 'static`（本项目写成 `B: 'static`，
因为没用到 `MessageBody` 的方法）。

**记忆技巧**：所有出现在中间件泛型参数里的类型，默认都要 `'static`。
**排查技巧**：报错位置在 `main.rs` 的 `HttpServer::new`，
真正的问题通常在你写的最后一个 `impl` 上——从报错往上找第一个"自己写的类型"。
*/

// --------------------------------------------
// 4.2 `type Future = Ready<...>` 里想 `.await`
// --------------------------------------------
/*
❌ 错误代码：

```rust
fn new_transform(&self, service: S) -> Self::Future {
    async {                      // ← 想用 async 块
        let cfg = load_config().await;
        Ok(LoggingMiddleware::new(service, cfg))
    }
}
```

```text
error[E0308]: mismatched types
   --> src/mw/logging.rs:...
    |
    |     type Future = Ready<Result<Self::Transform, Self::InitError>>;
    |                   ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ expected `Ready<...>`, found `async` block
```

原因：`Ready` 是"已经就绪"的 future，不可能挂起，所以里面装不下 `.await`。

✅ 两种改法：
```rust
// (a) 不 await，直接 ready
type Future = Ready<Result<Self::Transform, Self::InitError>>;
fn new_transform(&self, service: S) -> Self::Future { ready(Ok(...)) }

// (b) 真的需要异步初始化 → 换类型
type Future = Pin<Box<dyn Future<Output = Result<Self::Transform, Self::InitError>>>>;
fn new_transform(&self, service: S) -> Self::Future {
    Box::pin(async move { let cfg = load_config().await; Ok(...) })
}
```

**记忆技巧**：`Ready` = 同步，`Box::pin(async move {...})` = 异步。
选哪个取决于"初始化要不要 await"。
*/

// --------------------------------------------
// 4.3 在 `call` 里把 `&req` 的借用带过了 move
// --------------------------------------------
/*
❌ 错误代码（`bearer` 返回 `&str`）：

```rust
fn call(&self, req: ServiceRequest) -> Self::Future {
    let token = Self::bearer(req.request());   // Option<&str>，借用 req
    let fut = self.service.call(req);          // ← 这里要 move req
    ...
}
```

```text
error[E0505]: cannot move out of `req` because it is borrowed
   --> src/mw/auth.rs:...
    |
    |     let token = Self::bearer(req.request());
    |         ----- borrow of `req` occurs here
    |     let fut = self.service.call(req);
    |                                 ^^^ move out of `req` occurs here
```

原因：`borrow` 活到了第二次使用，而 `call` 需要**所有权**。

✅ 正确写法：在把 `req` 交出去之前，把所有信息"取出来变成拥有所有权的值"：

```rust
let token = Self::bearer(req.request()).map(str::to_owned); // Option<String>
let fut = self.service.call(req);                            // 借用已结束
```

**记忆技巧**：中间件的 `call` 里有一条硬分界线——
"交出 req 之前"只能做**读**，"交出之后"只能用**拷出来的值**。
*/

// --------------------------------------------
// 4.4 中间件里想读 `web::Json<T>`
// --------------------------------------------
/*
❌ 错误代码（在 `call` 里直接提取）：

```rust
async fn call(&self, req: ServiceRequest) {
    let body: web::Json<CreateTodo> = web::Json::from_request(req.request(), ...).await?;
    ...
}
```

问题不在编译，而在**运行时**：`Json` 会把 payload 从请求里读走，
handler 里再 `web::Json<CreateTodo>` 就只拿到空 body：

```text
Json deserialize error: EOF while parsing a value at line 1 column 0
```

✅ 如果确实要在中间件里读 body，必须放回去：

```rust
let (http_req, mut payload) = req.parts_mut();
let body = payload.read_to_bytes().await?;       // 读完就没
// ... 校验 ...
req.set_payload(actix_web::web::Bytes::from(body).into());   // ← 必须放回
```

**记忆技巧**：payload 是**流**，不是 `&[u8]`。
读一次就没了，用完要还。而鉴权通常**根本不需要**读 body（token 在 header 里）。
*/

// --------------------------------------------
// 4.5 用 `AppState` 传"当前用户"
// --------------------------------------------
/*
❌ 错误代码：

```rust
#[derive(Default)]
struct AppState {
    current_user: Mutex<Option<String>>,   // ← 全局唯一
}
```

```rust
let state = web::Data::new(AppState::default());
// 中间件里： state.current_user.lock().unwrap().replace(name);
```

症状：本地单用户测试全过；两个人同时用时，B 的请求被当成 A。
这不是"并发 bug 偶尔出现"，而是**每次并发都错**——只是单线程手工测试看不出来。

✅ 正确做法：`req.extensions_mut().insert(UserId(name))`。
extensions 的生命周期 = 一次请求，天然隔离；而且**无法**被别的请求读到。

**记忦技巧**：问自己"这个值属于**这次请求**，还是属于**整个进程**？"
前者 → extensions；后者（配置、连接池）→ `web::Data`。
*/

// --------------------------------------------
// 4.6 `wrap` 顺序写反，导致 401 不进日志
// --------------------------------------------
/*
❌ 错误代码（日志写在最前面，结果落在最里层）：

```rust
App::new()
    .wrap(Logging::new())      // 先写 → 里层
    .wrap(Auth::new(...))      // 后写 → 外层（先拒绝）
```

症状：客户端持续收到 401，服务端日志**一条都没有**。
排查时会怀疑"日志中间件坏了"，实际是它被关在鉴权里面——
auth 短路后就返回了，根本没往内层走。

✅ 正确顺序：把日志放到**最后**写。

```rust
App::new()
    .wrap(Auth::new(...))
    .wrap(Logging::new())      // 最后写 → 最外层，401/404 都能记到
```

**记忆技巧**：**后注册的在最外层**（本项目实测，见 1.3）。
"我要监控的行为"应该最后写，否则监控不到拒绝路径。
如果你按"从上往下 = 从外到内"去读，就会写反——这是本条错误的全部来源。
*/

// --------------------------------------------
// 4.7 `extensions_mut()` 的 `RefMut` 跨了 `.await`
// --------------------------------------------
/*
❌ 错误代码：

```rust
let mut ext = req.request().extensions_mut();   // RefMut
do_something().await;                            // ← 期间其他人 borrow_mut
ext.insert(UserId(name));
```

症状：不是编译错误，是运行时 panic：

```text
thread 'actix-rt|system:0|arbiter:0' panicked at library/core/src/cell.rs:
already borrowed: BorrowMutError
```

原因：extensions 内部是 `RefCell`，借用检查在**运行时**，且 `!Send` 不报错。

✅ 正确写法：`insert` 完立刻结束借用，不跨 `await`。

```rust
req.request().extensions_mut().insert(UserId(name));
```

**记忆技巧**：看到 `RefMut` / `Ref` 就跟看到 `MutexGuard` 一样——
**绝不跨 await**。这是同一个坑的两种包装。
*/

// ============================================
// 第五部分：函数/宏卡片速查
// ============================================
/*
【`Transform::new_transform`】

签名：fn new_transform(&self, service: S) -> Self::Future
参数：
  - service: 内层服务（对最外层来说就是整个路由树）
返回：Future<Result<Self::Transform, Self::InitError>>
调用时机：应用启动，每个 worker 一次
所有权：`service` 被移入新服务；`&self` 只是配置

示例：
    fn new_transform(&self, service: S) -> Self::Future {
        ready(Ok(LoggingMiddleware { service }))
    }

────────────────────────────────────

【`Service::call`】

签名：fn call(&self, req: ServiceRequest) -> Self::Future
参数：
  - req: 本次请求，**拥有所有权**，通常原样交给内层
返回：Future<Result<ServiceResponse<B>, Error>>
调用时机：每个请求一次
所有权：`req` 被移走；`&self` 只能读，不能改（中间件状态要改就用 `Cell`/`Mutex`）

示例：
    self.service.call(req)

────────────────────────────────────

【`forward_ready!`】

签名：forward_ready!(service);
参数：内层服务的字段名
作用：把 `poll_ready` 转发给内层，保持背压
等价于：
    fn poll_ready(&self, cx) -> Poll<Result<(), Self::Error>> {
        self.service.poll_ready(cx).map_err(Into::into)
    }

────────────────────────────────────

【`ServiceRequest::request` / `parts_mut` / `into_response`】

| 方法 | 用途 | 所有权 |
|---|---|---|
| `request() -> &HttpRequest` | 读 method/path/headers | 借用 |
| `parts_mut() -> (&mut HttpRequest, &mut Payload)` | 要操作 payload 时用 | 可变借用 |
| `into_response(res) -> ServiceResponse<B>` | 短路：直接造响应 | **消费 req** |

────────────────────────────────────

【`HttpMessage::extensions` / `extensions_mut`】

签名：fn extensions(&self) -> Ref<'_, Extensions>
      fn extensions_mut(&self) -> RefMut<'_, Extensions>
注意：来自 `HttpMessage` trait，**必须 `use actix_web::HttpMessage;`**
      `&self` 就能写（内部 `RefCell`），但 **不能跨 await**
返回：`Ref`/`RefMut`，`get::<T>()` / `insert(T)` 都是 `T: 'static`

────────────────────────────────────

【`App::wrap` / `Scope::wrap` / `Route::wrap`】

| 写法 | 作用范围 |
|---|---|
| `App::wrap(m)` | 整个应用，包括 404 |
| `Scope::wrap(m)` | 该 scope 下的所有路由 |
| `Route::wrap(m)` | 单条路由 |
| `.wrap_fn(\|req, srv\| ...)` | 一次性的闭包中间件，不需要写类型 |

顺序：**后注册的在最外层**（实测结论，见 1.3）。
写代码是从内到外，请求的方向是从外到内。
*/

// ============================================
// 第六部分：练习题
// ============================================
/*
1. **超时中间件**（必做）
   写一个 `Timeout` 中间件：handler 超过 200 ms 没有返回，就返回 503。
   要求：
   - 用 `actix_web::rt::time::timeout` 包住内层 future
   - 超时后**不能**让内层继续跑（想清楚：`timeout` 只是不再等，内层 future 会被 drop）
   提示：`timeout(dur, fut).await` 返回 `Result<T, Elapsed>`。
   注意：`fut` 被 drop 不等于"服务端停止工作"，
   写之前先想清楚这一点。

2. **请求 ID 中间件**（必做）
   给每个请求生成一个短 ID（例如自增计数器），
   写进 extensions，同时在响应头里加 `X-Request-Id`。
   要求：
   - 同时用 `ServiceResponse::headers_mut()` 改响应头
   - handler 里能通过一个 `RequestId` 提取器读到它
   注意：中间件状态要改 → `&self` 不够，需要 `Cell<u64>` 或 `AtomicU64`。
   想清楚为什么不能用 `&mut self`。

3. **统一错误格式**（必做）
   现在 handler 返回 `Err` 时 actix 会直接给纯文本 500。
   写一个中间件，把所有 `Err` 转成 `{"success":false,"error":"..."}`。
   提示：`res` 是 `Result<ServiceResponse<B>, Error>`，
   在 `Err` 分支用 `req.error_response(e)` —— 但 `req` 已经被移走了，
   所以要在 `call` 里提前 clone 一份 `HttpRequest` 才能这样用。
   思考：这是不是"不该用中间件做"的场景？为什么？

4. **按路由区分是否鉴权**（进阶）
   现在 `/api` 下的所有路由都要鉴权。改成：
   `GET /api/todos` 公开，`POST /api/todos` 需要 token。
   要求：用 `Scope::wrap` + `Route::wrap` 组合实现，**不要**写两套中间件配置。

5. **给日志中间件加"响应字节数"**（进阶）
   在日志里打印 body 大小。
   提示：`ServiceResponse<B>` 上拿 body 大小需要 `B: MessageBody`，
   而 `size()` 可能返回 `None`（流式 body 不知道总长）。
   思考：为什么这点很重要？

6. **写出"为什么 `Logging` 必须在最外层"的三个反例**（思考题）
   分别对应：401 不进日志、耗时包含了不该包含的部分、
   panic 兜底中间件收不到。

7. **把 `Auth` 写成 `wrap_fn`**（思考题）
   `.wrap_fn(|req, srv| { ... })` 可以省掉两个类型。
   试着改一遍，然后回答：什么时候该用 `wrap_fn`，什么时候必须写类型？
   提示：多个路由要复用、需要配置参数、需要单测——三条都是写类型的理由。
*/

// ============================================
// 服务装配
// ============================================

#[actix_web::main]
async fn main() -> std::io::Result<()> {
    env_logger::init_from_env(env_logger::Env::default().default_filter_or("info"));

    println!("============================================================");
    println!("Actix-web 中间件演示（第 35 天）");
    println!("============================================================");
    println!("  公开：  GET  /health");
    println!("  公开：  GET  /debug/forgot-auth   ← 漏挂鉴权的现场，必看");
    println!("  鉴权：  GET  /api/todos           token: secret-token (alice)");
    println!("  鉴权：  POST /api/todos           token: secret-token");
    println!("  鉴权：  GET  /api/whoami");
    println!("  鉴权：  GET  /api/slow/{{ms}}");
    println!();
    println!("  中间件链条（从外到内）：[OUTER] → [INNER] → [API] → Auth → handler");
    println!("  注意书写顺序是**从内到外**：[API] 先写，[OUTER] 最后写。");
    println!();
    println!("  观察点 1：200 请求打三行日志，顺序 [API] → [INNER] → [OUTER]");
    println!("            —— 响应后进先出，最里层最先打印。");
    println!("  观察点 2：401 请求只打 [INNER]/[OUTER] 两行，没有 [API]");
    println!("            —— Auth 短路在 [API] 外面，它根本没机会执行。");
    println!("============================================================");
    println!("📍 http://127.0.0.1:8082");
    println!();

    // token → 用户名。真实项目里应来自数据库或 JWT 校验。
    let auth = Auth::new(&[("secret-token", "alice"), ("other-token", "bob")]);

    HttpServer::new(move || {
        App::new()
            // ---------- ① 顺序实验：两个日志中间件 ----------
            // 先写 → 里面那个；后写 → 外面那个。
            // 观察输出：[INNER] 先打印，[OUTER] 后打印（响应后进先出）。
            .wrap(Logging::with_prefix("[INNER]"))
            .wrap(Logging::with_prefix("[OUTER]"))
            // ---------- ② 公开路由 ----------
            .service(web::resource("/").to(index))
            .service(web::resource("/health").to(health))
            .service(web::resource("/debug/forgot-auth").to(forgot_auth))
            // ---------- ③ 加鉴权的子路由树 ----------
            .service(
                web::scope("/api")
                    // 同一个中间件、两个层层叠加，用不同前缀就能**看见洋葱模型**：
                    // [API] 先写 → 在里层 → 它的日志比 [INNER]/[OUTER] 都先打印。
                    //
                    // ⚠️ 也正因为它在 auth **里面**：被 auth 拒掉的请求
                    //    不会经过 [API]，所以 401 只有 [INNER]/[OUTER] 两行。
                    //    这就是 4.6 那个坑的最小现场。
                    .wrap(Logging::with_prefix("[API]"))
                    // scope 级 wrap：只包这棵子树。
                    // `/health` 和 `/debug/*` 不吃这套 —— 这就是"按作用域挂"。
                    .wrap(auth.clone())
                    // ⚠️ 同一路径的多个方法必须写在**一个** `web::resource` 上。
                    // 写两个 `resource("/todos")` 的话，第一个匹配到就返回了，
                    // 第二个永远不生效——实测表现为 `POST /api/todos → 405`。
                    .service(
                        web::resource("/todos")
                            .route(web::get().to(list_todos))
                            .route(web::post().to(create_todo)),
                    )
                    .service(web::resource("/whoami").to(whoami))
                    .service(web::resource("/slow/{ms}").to(slow)),
            )
            // ---------- ④ 说明：官方 Logger 也能这样挂 ----------
            // .wrap(actix_web::middleware::Logger::default())
            //
            // 与手写版的两个差别：
            // 1. 它走 `log` crate，没有 env_logger 就什么都不打印
            // 2. 它用 `pin_project!` 手写 future，零堆分配
            //
            // 想对比就看：`RUST_LOG=info cargo run`，把上面两行的注释互换。
            .default_service(web::to(|| async {
                HttpResponse::NotFound()
                    .insert_header((header::CONTENT_TYPE, "text/plain; charset=utf-8"))
                    .body("404 —— 试着访问 / 看看有哪些端点")
            }))
    })
    .bind(("127.0.0.1", 8082))?
    .run()
    .await
}
