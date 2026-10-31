// ============================================
// 日志中间件 —— 手写一遍，彻底看清 Transform / Service 两层
// 日期：2026-10-30（第35天）
// ============================================

/*
【这个文件回答三个问题】

1. `Transform` 和 `Service` 分别在哪一步被调用？
2. 请求进来、响应出去，中间件怎么"两头都插一手"？
3. 为什么 `type Future` 要写成 `Pin<Box<dyn Future<...>>>`，而不能直接 `async fn`？

【执行时序】对一次 `GET /api/todos`

```text
Logging::new_transform(inner)        ← 应用启动时，每个 worker 一次
  └─ 返回 LoggingMiddleware { service: inner }

LoggingMiddleware::poll_ready()      ← 每次请求前
  └─ forward_ready!(service) 转发给内层

LoggingMiddleware::call(req)         ← 每个请求一次
  ├─ 记录 start / method / path     ← 请求"进入"时
  ├─ fut = inner.call(req)          ← 把请求交给内层（鉴权 → handler）
  ├─ 返回一个包装后的 Future
  └─ 该 Future 被 await 时：
       res = fut.await              ← 等内层产出响应
       打印 status + 耗时            ← 响应"出去"时
       把 res 原样返回               ← 绝不吞掉、绝不改写
```
*/

use std::future::{ready, Future, Ready};
use std::pin::Pin;
use std::time::Instant;

use actix_web::dev::{forward_ready, Service, ServiceRequest, ServiceResponse, Transform};
use actix_web::Error;

// --------------------------------------------
// 第 1 层：Transform —— 工厂
// --------------------------------------------

/// 日志中间件的**配置**。
///
/// 注意这个类型里只有"配置"，没有"状态"。
/// 每个 worker 会用自己的 `Logging` 建一个新 `LoggingMiddleware`，
/// 所以它必须能廉价复制（这里是 `String`，够便宜）。
#[derive(Clone)]
pub struct Logging {
    /// 每条日志的前缀，方便在混合输出里 grep。
    prefix: String,
}

impl Logging {
    /// 允许自定义前缀，用来对比"哪一层先打印"。
    ///
    /// 本项目用三个前缀验证洋葱模型：`[OUTER]` / `[INNER]` / `[API]`。
    pub fn with_prefix(prefix: impl Into<String>) -> Self {
        Self {
            prefix: prefix.into(),
        }
    }
}

/*
【签名逐项解释】

impl<S, B> Transform<S, ServiceRequest> for Logging
where
    S: Service<ServiceRequest, Response = ServiceResponse<B>, Error = Error> + 'static,
    B: 'static,

- `S`：内层服务。对最外层中间件来说，内层就是"整个 App 的路由树"。
- `B`：响应体类型。`ServiceResponse<B>` 里 `B` 是 body，
  这里不关心具体是什么（可能是 `BoxBody`，也可能是 `EitherBody`），
  所以整个中间件对 `B` 泛型，**完全不碰 body**。
- `S: 'static` / `B: 'static`：actix-web 要求服务本身活得和 worker 一样久。
  少写这个约束，编译器的报错会出现在**很远的地方**（`HttpServer::new` 那里），
  这是新手最容易踩的一个坑，第 4 部分有完整报错。

为什么 `Self::Response = ServiceResponse<B>` 而不是换个类型？
→ 中间件"透明"：进来是什么响应类型，出去还是什么。这样它才能插在任意位置。
   一旦你想换 body 类型（比如压缩中间件），就得用 `EitherBody`，
   那是另一个话题。
*/
impl<S, B> Transform<S, ServiceRequest> for Logging
where
    S: Service<ServiceRequest, Response = ServiceResponse<B>, Error = Error> + 'static,
    B: 'static,
{
    type Response = ServiceResponse<B>;
    type Error = Error;

    /// 工厂造出来的那个"真正干活的"服务。
    type Transform = LoggingMiddleware<S>;

    /// 建服务可能失败（比如读配置文件失败）。这里不会失败，用 `()`。
    type InitError = ();

    /// `new_transform` 是异步的（要返回 Future），但我们这里是同步逻辑，
    /// 所以用 `future::ready` 立即完成。
    ///
    /// ⚠️ 注意：`Ready` 是**同步就绪**的 future，里面**不能** `.await`。
    /// 想把 `new_transform` 写成 `async fn`，必须先引入 `LocalBoxFuture`
    /// 或 `Box::pin`，否则编译器会报 "cannot await in a function that is not async"。
    type Future = Ready<Result<Self::Transform, Self::InitError>>;

    fn new_transform(&self, service: S) -> Self::Future {
        // 这里只做"组装"，**不要**在这里访问请求——此刻还没有任何请求。
        ready(Ok(LoggingMiddleware {
            service,
            prefix: self.prefix.clone(),
        }))
    }
}

// --------------------------------------------
// 第 2 层：Service —— 真正干活
// --------------------------------------------

/// 包着内层服务的中间件本体。
pub struct LoggingMiddleware<S> {
    /// 内层服务。中间件自己不处理请求，只是"包一层"。
    service: S,
    prefix: String,
}

impl<S, B> Service<ServiceRequest> for LoggingMiddleware<S>
where
    S: Service<ServiceRequest, Response = ServiceResponse<B>, Error = Error>,
    // ↓ 把内层 future 捕获进 `Box::pin(...)` 时，`dyn Future` 默认要求 `'static`。
    //   少写这一条会报 E0310：
    //   error[E0310]: the associated type `<S as Service<ServiceRequest>>::Future`
    //                  may not live long enough
    S::Future: 'static,
{
    type Response = ServiceResponse<B>;
    type Error = Error;

    /*
    【为什么是 `Pin<Box<dyn Future>>` 而不是 `async fn`？】

    trait 方法不能是 `async fn`（Rust 1.75 之前完全不行；之后在 trait 里可以，
    但返回的 future 类型是 `impl Future`，无法在 trait 关联类型里直接写）。
    所以只能显式写出 future 类型。

    两条路：
    a) `Pin<Box<dyn Future<...>>>`  ← 本文件用的，简单，每个请求一次堆分配
    b) `pin_project!` 手写一个 future 结构体 ← actix 官方 Logger 用的，
       零分配，但要自己实现 `poll`

    对一个学习项目，(a) 足够；生产环境高频路径才值得 (b)。
    这个差异在第 3 部分「深入理解」里展开。

    ⚠️ 这个 future **不是 `Send`**，因为它捕获了 `S::Future`，
    而 `S::Future` 本身就不保证 `Send`。
    actix-web 每个 worker 跑在自己的 `LocalSet` 上，所以不需要 `Send`——
    如果哪天你想把它 `tokio::spawn` 出去，会立刻在这里撞墙。
    */
    type Future = Pin<Box<dyn Future<Output = Result<Self::Response, Self::Error>>>>;

    // 把 `poll_ready` 转发给内层。
    //
    // 展开后就是：
    // ```ignore
    // fn poll_ready(&self, cx) -> Poll<Result<(), Error>> {
    //     self.service.poll_ready(cx).map_err(Into::into)
    // }
    // ```
    // 这是"背压"的传递路径：内层说"我忙"，外层也得说"我忙"。
    // 中间件如果自己吞掉 `Pending` 直接返回 `Ready`，
    // 就等于**绕过背压**——上游会继续把请求压进来。
    //
    // 注：这里必须用 `//` 而不是 `///`，因为宏调用不生成文档项，
    // 写 `///` 会报 `warning: unused doc comment`。
    forward_ready!(service);

    fn call(&self, req: ServiceRequest) -> Self::Future {
        // ---------- 请求「进入」侧 ----------
        // 这三个值必须在 `req` 被 move 进内层之前取出来。
        // 取 `method` 时用 `to_string()` 而不是 `&Method`，
        // 因为 `req` 后面会被移走，引用活不到响应阶段。
        let start = Instant::now();
        let method = req.method().to_string();
        let path = req.path().to_string();
        let prefix = self.prefix.clone();

        // ---------- 把请求交给内层 ----------
        // 注意：这里只是**拿到** future，还没 await。
        // 真正的执行发生在下面那个 Box::pin 的 future 被 poll 的时候。
        let fut = self.service.call(req);

        // ---------- 响应「出去」侧 ----------
        Box::pin(async move {
            // 等待内层产出响应。内层可能是另一个中间件，也可能是 handler。
            let res = fut.await;
            let ms = start.elapsed().as_millis();

            // 关键：**读**响应，但不改写。
            match &res {
                Ok(r) => println!("{prefix} {method} {path} → {} ({ms} ms)", r.status()),
                // 内层返回 Err 时，actix 会把它转成 500 响应。
                // 这里记下来，方便区分"业务 500"和"框架 500"。
                Err(e) => println!("{prefix} {method} {path} → 错误: {e} ({ms} ms)"),
            }

            // 原样返回。中间件篡改响应是排查问题时最恶心的 bug 来源之一。
            res
        })
    }
}
