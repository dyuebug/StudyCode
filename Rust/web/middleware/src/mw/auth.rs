// ============================================
// 鉴权中间件 —— 中间件「写入」、提取器「读出」
// 日期：2026-10-30（第35天）
// ============================================

/*
【这个文件的核心问题】

handler 怎么拿到"中间件已经认证过的用户"？

三条路，只有第三条是对的：

1. ❌ 塞进 `web::Data<AppState>`（全局共享）
   → 那是**所有请求共用的一份**，并发请求会互相覆盖。
     浏览器 A 的 user 被写到浏览器 B 的 handler 里，是最典型的越权 bug。

2. ❌ 让 handler 再解析一次 header
   → 认证逻辑写两遍；中间件拒掉的请求在 handler 里不会执行，
     但 handler 里那份代码一旦和中间件不一致，就会出现"日志说拒绝了、实际放行了"。

3. ✅ `req.extensions_mut().insert(UserId(..))`，handler 用自定义提取器读出来
   → extensions 是**随请求走**的（`Rc<RefCell<Extensions>>`），
     生命周期 = 这一次请求，天然隔离。

【为什么 handler 不直接写 `req.extensions().get::<UserId>()`】

也能，但那会让每个 handler 都写一遍 `unwrap` + 错误处理。
自定义提取器把这段重复逻辑收成一个类型 `UserId`：
handler 签名里写 `user: UserId`，拿不到就是 401，**类型系统替你保证"已经鉴权"**。
这是「用类型表达意图」的一个实例——签名里没有 `Option<UserId>`，
就意味着"进到这里的一定是登录用户"。
*/

use std::collections::HashMap;
use std::future::{ready, Future, Ready};
use std::pin::Pin;
use std::sync::Arc;

use actix_web::body::{EitherBody, MessageBody};
use actix_web::dev::{forward_ready, Payload, Service, ServiceRequest, ServiceResponse, Transform};
use actix_web::http::header;
use actix_web::{Error, FromRequest, HttpMessage, HttpRequest, HttpResponse};

// --------------------------------------------
// 身份类型：从中间件流向 handler 的唯一载体
// --------------------------------------------

/// 已认证的用户身份。
///
/// 这个类型在 extensions 里**只可能由鉴权中间件写入**，
/// 所以 handler 只要拿到它，就不用再怀疑"是否已登录"。
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct UserId(pub String);

/*
【提取器实现】

`FromRequest` 就是"从请求里造出一个值"的 trait。
`web::Json<T>`、`web::Path<T>`、`web::Query<T>` 全都实现了它，
我们这里实现第 4 个：`UserId`。

签名里的 `payload: &mut Payload` 我们不用——
它只在需要读请求体时才有用（`web::Json` 用它读 body）。
*/
impl FromRequest for UserId {
    /// 提取失败时返回的错误。`Into<Error>` 即可，这里直接用 `Error`。
    type Error = Error;

    /// 同步提取，所以用 `Ready`。
    /// 如果要查数据库（比如"token 对应哪个用户"），这里就得换成
    /// `LocalBoxFuture<'static, Result<Self, Self::Error>>` 真正的异步版本。
    type Future = Ready<Result<Self, Self::Error>>;

    fn from_request(req: &HttpRequest, _payload: &mut Payload) -> Self::Future {
        // `extensions()` 来自 `HttpMessage` trait，所以文件头 `use` 了它。
        match req.extensions().get::<UserId>() {
            Some(user) => ready(Ok(user.clone())),
            None => {
                // 走到这里说明两种可能：
                // a) 这条路由没有挂鉴权中间件（配置错误）
                // b) 中间件挂了但没写入（实现错误）
                // 两者都是**服务端 bug**，不是客户端 401 的场景。
                // 但对外仍返回 401，避免把内部结构泄露给调用方。
                println!("[AUTH] ⚠️ {} {} 没有 UserId —— 检查是否漏挂鉴权中间件", req.method(), req.path());
                ready(Err(actix_web::error::ErrorUnauthorized(
                    "缺少身份：该路由未经过鉴权中间件",
                )))
            }
        }
    }
}

// --------------------------------------------
// 第 1 层：Transform
// --------------------------------------------

/// 鉴权中间件配置：一张 token → 用户名 的表。
///
/// 这里用 `Arc<HashMap>` 而不是 `Rc`，原因很具体：
/// `HttpServer::new` 的闭包要求 `Fn() -> I + Send + Clone`，
/// 闭包里捕获的 `Auth` 必须能跨线程送出（每个 worker 线程各跑一份闭包）。
/// `Rc` 是单线程计数，不满足 `Send`——换成 `Rc` 会得到：
///
/// ```text
/// error[E0277]: `Rc<HashMap<String, String>>` cannot be sent between threads safely
///    --> src/main.rs:776:5
///     |     HttpServer::new(move || {
///     |     ^^^^^^^^^^
///     = help: the trait `Send` is not implemented for `Rc<HashMap<String, String>>`
///     = note: required by a bound in `HttpServer`
///         F: Fn() -> I + Send + Clone + 'static
/// ```
///
/// 注意报错位置在 `main.rs`，根因在中间件类型——和 4.1 的 `'static` 是同一类"报错很远"。
#[derive(Clone)]
pub struct Auth {
    tokens: Arc<HashMap<String, String>>,
}

impl Auth {
    /// 从 `(token, username)` 列表构造。
    ///
    /// 真实项目里这张表来自数据库或 JWT 校验，但中间件的**结构**完全一样：
    /// 拿 header → 判断 → 成功写入 extensions / 失败直接返回响应。
    pub fn new(tokens: &[(&str, &str)]) -> Self {
        Self {
            tokens: Arc::new(
                tokens
                    .iter()
                    .map(|(t, u)| ((*t).to_string(), (*u).to_string()))
                    .collect(),
            ),
        }
    }

    /// 从请求头里取 `Authorization: Bearer <token>`。
    fn bearer(req: &HttpRequest) -> Option<String> {
        let value = req.headers().get(header::AUTHORIZATION)?.to_str().ok()?;
        // `strip_prefix` 区分大小写。HTTP 的 auth scheme 按 RFC 7235 是
        // case-insensitive，严格实现应该用 `eq_ignore_ascii_case`。
        // 这里刻意保留简单写法，因为它是个真实的坑：
        // 实测 `curl -H "Authorization: bearer secret-token"` → **401**，
        // 而 token 完全正确。客户端换个库、改个大小写就"突然登不上了"。
        value.strip_prefix("Bearer ").map(str::to_owned)
    }
}

impl<S, B> Transform<S, ServiceRequest> for Auth
where
    S: Service<ServiceRequest, Response = ServiceResponse<B>, Error = Error> + 'static,
    B: MessageBody + 'static,
{
    /*
    【为什么响应类型变成了 `EitherBody<B>`？】

    鉴权有两条出口：
    - 通过 → 原样返回内层的 `ServiceResponse<B>`
    - 拒绝 → 构造一个 `ServiceResponse<BoxBody>`（我们自己的 JSON）

    两种 body 类型不同，但 `Service::Response` 只能有一个关联类型。
    `EitherBody<L, R>` 就是 actix 给的答案：一个枚举，装左右两种之一。
    `EitherBody<B>` 是 `EitherBody<B, BoxBody>` 的简写。

    这就是 4.x「中间件想换 body 类型怎么办」的标准解法。
    如果写成 `type Response = ServiceResponse<B>`，403 分支会报：

    ```text
    error[E0271]: expected `{async block@...}` to be a future that resolves to
                  `Result<ServiceResponse<B>, Error>`, but it resolves to `Result<ServiceResponse, _>`
        = note: expected enum `Result<ServiceResponse<B>, actix_web::Error>`
                   found enum `Result<ServiceResponse<BoxBody>, _>`
    ```
    */
    type Response = ServiceResponse<EitherBody<B>>;
    type Error = Error;
    type Transform = AuthMiddleware<S>;
    type InitError = ();
    type Future = Ready<Result<Self::Transform, Self::InitError>>;

    fn new_transform(&self, service: S) -> Self::Future {
        ready(Ok(AuthMiddleware {
            service,
            tokens: Arc::clone(&self.tokens),
        }))
    }
}

// --------------------------------------------
// 第 2 层：Service
// --------------------------------------------

pub struct AuthMiddleware<S> {
    service: S,
    tokens: Arc<HashMap<String, String>>,
}

impl<S, B> Service<ServiceRequest> for AuthMiddleware<S>
where
    S: Service<ServiceRequest, Response = ServiceResponse<B>, Error = Error>,
    // `S::Future: 'static`：内层 future 会被捕获进我们 `Box::pin` 的 future，
    // 而 `Pin<Box<dyn Future>>` 里的 `dyn` 默认带 `'static`。
    // 少写这一条会报 E0310（见 4.1 的升级版）。
    S::Future: 'static,
    B: MessageBody + 'static,
{
    type Response = ServiceResponse<EitherBody<B>>;
    type Error = Error;
    type Future = Pin<Box<dyn Future<Output = Result<Self::Response, Self::Error>>>>;

    forward_ready!(service);

    fn call(&self, req: ServiceRequest) -> Self::Future {
        /*
        【借用顺序是这个函数最容易写错的地方】

        1. `req.request()` 借 `req`（不可变）
        2. 我们用这个借用查表，得到**拥有所有权**的 `Option<String>`
        3. 借用结束
        4. 才能 `self.service.call(req)` 把 `req` **move** 走

        如果写成 `let user = Self::bearer(req.request());`（返回 `&str`），
        借用会一直活到后面，第 4 步就会报
        `error[E0505]: cannot move out of `req` because it is borrowed`。
        所以这里必须 `.and_then` 出 `String`，把借用了结掉。
        */
        let user = Auth::bearer(req.request()).and_then(|t| self.tokens.get(&t).cloned());

        match user {
            Some(name) => {
                // ---------- 认证通过 ----------
                // `extensions_mut()` 来自 `HttpMessage`（同 `extensions()`）。
                // 内部是 `Rc<RefCell<Extensions>>`，所以这里写入的内容
                // 后续 handler 拿到的同一个 `HttpRequest` 都看得到。
                //
                // ⚠️ `RefCell::borrow_mut()` 在已借用时会 **panic**。
                // 不要拿着 `let mut ext = req.request().extensions_mut();`
                // 的 `RefMut` 跨 `.await`，否则并发时会 panic 而不是报错。
                req.request().extensions_mut().insert(UserId(name.clone()));

                let fut = self.service.call(req);
                Box::pin(async move {
                    // 认证通过后不在响应侧做任何事：透明的"守卫"，
                    // 不是"装饰器"。这是鉴权与日志两种中间件的本质差别。
                    //
                    // `map_into_left_body`：把内层原来的 `B` 装进 `EitherBody` 的左半边。
                    // 这一步**不复制 body、不装箱**，只是换了个包装类型。
                    fut.await.map(ServiceResponse::map_into_left_body)
                })
            }
            None => {
                // ---------- 认证失败 ----------
                println!("[AUTH] 拒绝 {} {}", req.method(), req.path());

                // `into_response` **消费** `req`，所以它必须放在最后，
                // 之后再也不能使用 `req`。这是编译器强制的（E0382）。
                let res = req.into_response(
                    HttpResponse::Unauthorized()
                        // 告诉客户端"该用哪种认证方式"，这是 401 的标准做法。
                        .insert_header((header::WWW_AUTHENTICATE, "Bearer"))
                        .json(serde_json::json!({
                            "success": false,
                            "error": "缺少或无效的 Authorization: Bearer <token>"
                        })),
                );

                // 直接返回 401，**不再调用内层**。
                // 这就是"短路"：handler 根本不会执行。
                //
                // `map_into_right_body`：把我们自己的 `BoxBody` 装进右半边。
                Box::pin(async move { Ok(res.map_into_right_body()) })
            }
        }
    }
}

/*
【注意：中间件这里没有碰请求体】

`signed` 等需要读 body 的中间件，必须
`let (_, mut payload) = req.parts_mut();`
然后从 payload 里读出全部字节、校验、**再放回去**（`req.set_payload(...)`）。
漏掉"放回去"这一步，handler 里的 `web::Json<T>` 会收到空 body，
报 `Json deserialize error: EOF while parsing a value`。

这也是为什么鉴权**不该**读 body：
- 一旦读了，要么整包缓冲进内存（大文件上传直接炸），要么就得处理分块
- 而且 token 在 header 里，本来就不需要看 body
*/
