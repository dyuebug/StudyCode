// ============================================
// task-manager 中间件
// 日期：2026-11-01（第37天，复用第35天 web/middleware 的写法）
// ============================================

/*
【两个中间件，两种用途】

    logging()   App 级：监控所有请求，包括 404 与 401
    auth()      /projects scope 级：认证 + 把身份写进 extensions

【为什么日志写在 App 级、认证写在 scope 级】
日志要"覆盖一切"，所以挂最外层；
认证要"只保护业务接口"，所以挂具体的 scope。
如果反过来（认证在 App 级），`/health` 也会被要求 token——
健康检查通常是负载均衡器在调，它没有 token。

【这套模板可以背下来】
每个中间件都是"一个工厂类型 + 一个服务类型 + 三个关联类型"：
    struct Foo                      实现 Transform，new_transform 每 worker 一次
    struct FooMiddleware<S>         实现 Service，call 每请求一次
    type Future = Pin<Box<dyn Future<Output = Result<...>>>>;
两个 where 约束必写：泛型 `'static`、`S::Future: 'static`。
不写会得到 E0310，而且报错位置往往在 main.rs。
*/

use std::collections::HashMap;
use std::future::{Future, Ready, ready};
use std::pin::Pin;
use std::sync::Arc;
use std::sync::atomic::{AtomicU64, Ordering};
use std::time::Instant;

use actix_web::body::{EitherBody, MessageBody};
use actix_web::dev::{Payload, Service, ServiceRequest, ServiceResponse, Transform, forward_ready};
use actix_web::http::header::{self, HeaderName, HeaderValue};
use actix_web::{Error, FromRequest, HttpMessage, HttpRequest, HttpResponse};
use serde_json::json;

// ============================================
// 一、日志
// ============================================

/// 请求 ID（第 39 天）
///
/// 【为什么需要它】
/// 一次请求可能打印多行日志（认证、业务、错误），并发时这些行会**交错**。
/// 没有 ID 就只能靠"时间戳差不多"去猜哪几行属于同一次请求。
/// 有了它，`grep '#42'` 就能还原一条完整链路。
///
/// 【为什么用进程内计数器而不是 UUID】
/// 单进程内唯一就够用了（本项目就是单进程多线程）。UUID 需要额外依赖，
/// 而计数器是零依赖、可读、还能看出"第几个请求"。
/// 跨进程/跨服务时再换 UUID 或 trace id。
///
/// 【为什么要写进响应头】
/// 客户端报障时说"第 42 号请求"比"刚刚有一次失败"有用得多。
#[derive(Clone, Copy, Debug)]
pub struct RequestId(pub u64);

/// `0` 留给"没经过日志中间件的请求"，从 1 开始计数
static NEXT_REQUEST_ID: AtomicU64 = AtomicU64::new(1);

/// 耗时分桶（第 39 天）
///
/// 【为什么分桶而不是只记毫秒数】
/// 单个毫秒数在日志里几乎没法聚合：想把"慢请求"捞出来得人眼扫一遍。
/// 分桶之后 `grep '≥1s'` 就能直接筛出异常。
/// 这是 Prometheus histogram 的手工版——真上监控时把这行换成
/// `metrics::histogram!` 即可，分桶边界不用重想。
fn bucket(ms: u128) -> &'static str {
    if ms < 10 {
        "<10ms"
    } else if ms < 100 {
        "<100ms"
    } else if ms < 1_000 {
        "<1s"
    } else {
        ">=1s"
    }
}

/// 日志中间件工厂
///
/// 无状态，所以可以直接 `Clone`。
#[derive(Clone, Copy)]
pub struct Logging;

/// 构造日志中间件
pub fn logging() -> Logging {
    Logging
}

/// 包装后的服务
pub struct LoggingMiddleware<S> {
    service: S,
}

impl<S, B> Transform<S, ServiceRequest> for Logging
where
    S: Service<ServiceRequest, Response = ServiceResponse<B>, Error = Error> + 'static,
    B: 'static,
{
    type Response = ServiceResponse<B>;
    type Error = Error;
    type Transform = LoggingMiddleware<S>;
    type InitError = ();
    type Future = Ready<Result<Self::Transform, Self::InitError>>;

    /// 每个 worker 启动时调用**一次**。
    /// 把"每请求都要做"的事情写在这里，症状是"只打印一条日志"。
    fn new_transform(&self, service: S) -> Self::Future {
        ready(Ok(LoggingMiddleware { service }))
    }
}

impl<S, B> Service<ServiceRequest> for LoggingMiddleware<S>
where
    S: Service<ServiceRequest, Response = ServiceResponse<B>, Error = Error>,
    // `Box::pin(...)` 把内层 future 装进 `dyn Future`，
    // 而 `dyn Future` 的默认约束是 `'static`。不写这行会报 E0310。
    S::Future: 'static,
{
    type Response = ServiceResponse<B>;
    type Error = Error;
    type Future = Pin<Box<dyn Future<Output = Result<Self::Response, Self::Error>>>>;

    // 转发 `poll_ready`，即背压：内层没准备好，外层也别急着接活。
    // 自己实现时若直接 `ready(Ok(()))`（吞掉 Pending），背压就断了。
    forward_ready!(service);

    fn call(&self, req: ServiceRequest) -> Self::Future {
        let start = Instant::now();
        let method = req.method().to_string();
        let path = req.path().to_string();

        // 请求一进来就分配 ID，**再**交给内层：
        // 内层（handler 或别的中间件）如果也要打日志，此时 extensions 里已经有 ID 了。
        let id = NEXT_REQUEST_ID.fetch_add(1, Ordering::Relaxed);
        req.request().extensions_mut().insert(RequestId(id));

        let fut = self.service.call(req);

        Box::pin(async move {
            let mut res = fut.await;
            let elapsed = start.elapsed();
            let ms = elapsed.as_millis();
            let b = bucket(ms);

            match &mut res {
                Ok(r) => {
                    // 把 ID 回给客户端。失败（非法 header 值）不可能发生——
                    // 内容是纯数字，但仍然不做 unwrap：中间件不该因为
                    // "日志辅助信息"把整个请求搞挂。
                    let _ = r.headers_mut().insert(
                        HeaderName::from_static("x-request-id"),
                        HeaderValue::from_str(&id.to_string())
                            .unwrap_or(HeaderValue::from_static("0")),
                    );
                    println!(
                        "[LOG] #{id} {method} {path} → {} ({ms} ms, {b})",
                        r.status()
                    );
                }
                Err(e) => println!("[LOG] #{id} {method} {path} → 错误 {e} ({ms} ms, {b})"),
            }
            res
        })
    }
}

// ============================================
// 二、认证
// ============================================

/// 已认证用户（请求级数据）
///
/// 由 `auth()` 写入 `request.extensions()`，由本提取器读出。
/// 用 newtype 而不是裸 `i64`：裸 `i64` 会和"任何别的 i64"混在一起，
/// 而 `CurrentUser(7)` 只能由认证中间件产生。
#[derive(Clone, Copy, Debug)]
pub struct CurrentUser(pub i64);

/// `CurrentUser` 的"读"侧
///
/// 中间件写、提取器读——这就是"身份怎么到达 handler"的完整闭环。
/// handler 只要在签名里写 `user: CurrentUser`，就**不可能**忘记认证：
/// 取不到就直接 401，而不是拿到一个空身份继续跑。
impl FromRequest for CurrentUser {
    type Error = Error;
    type Future = Ready<Result<Self, Self::Error>>;

    fn from_request(req: &HttpRequest, _payload: &mut Payload) -> Self::Future {
        match req.extensions().get::<CurrentUser>() {
            // `extensions()` 内部是 `Rc<RefCell<...>>`，返回的是克隆出来的值，
            // 不持有借用，所以可以安全地跨 await（这里是 ready，不涉及）。
            Some(u) => ready(Ok(*u)),
            None => {
                println!(
                    "[AUTH] ⚠️ {} {} 拿不到 CurrentUser —— 该路由没经过 auth 中间件",
                    req.method(),
                    req.path()
                );
                ready(Err(actix_web::error::ErrorUnauthorized("缺少身份")))
            }
        }
    }
}

/// 认证中间件
///
/// `tokens` 用 `Arc` 共享：每个 worker 会各建一份中间件，
/// `Arc` 让它们共享同一张表而不是各拷一份。
/// 用 `Rc` 编译得过，但会在 `HttpServer::new` 处报
/// `Rc<...> cannot be sent between threads safely`（E0277）——
/// 报错位置在上层，根因在这里。
#[derive(Clone)]
pub struct Auth {
    tokens: Arc<HashMap<String, i64>>,
}

/// 构造认证中间件（演示用的静态 token 表）
///
/// 生产项目这里会是 JWT 校验或查库，
/// 但**形状完全一样**：从 header 取凭据 → 换出用户身份 → 写进 extensions。
pub fn auth() -> Auth {
    let mut tokens = HashMap::new();
    tokens.insert("token-alice".to_string(), 1);
    tokens.insert("token-bob".to_string(), 2);
    Auth {
        tokens: Arc::new(tokens),
    }
}

impl Auth {
    /// 从 `Authorization: Bearer <token>` 取出 token
    ///
    /// ⚠️ 这里用 `strip_prefix`，**大小写敏感**。
    /// RFC 7235 规定 scheme 不区分大小写，所以严格实现应当
    /// 用 `eq_ignore_ascii_case`。保留简单写法是因为
    /// "客户端换个库就突然登不上"是个真实的互操作坑，值得记住。
    fn bearer(req: &HttpRequest) -> Option<String> {
        req.headers()
            .get(header::AUTHORIZATION)?
            .to_str()
            .ok()?
            .strip_prefix("Bearer ")
            .map(|s| s.to_string())
    }
}

pub struct AuthMiddleware<S> {
    service: S,
    tokens: Arc<HashMap<String, i64>>,
}

impl<S, B> Transform<S, ServiceRequest> for Auth
where
    S: Service<ServiceRequest, Response = ServiceResponse<B>, Error = Error> + 'static,
    // 拒绝分支会自己造一个 `HttpResponse`，它的 body 是 `BoxBody`；
    // 通过分支的 body 是 `B`。两种 body 要装进同一个 `Response` 类型，
    // 所以要求 `B: MessageBody`（`EitherBody` 的约束）。
    B: MessageBody + 'static,
{
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

impl<S, B> Service<ServiceRequest> for AuthMiddleware<S>
where
    S: Service<ServiceRequest, Response = ServiceResponse<B>, Error = Error>,
    S::Future: 'static,
    B: MessageBody + 'static,
{
    type Response = ServiceResponse<EitherBody<B>>;
    type Error = Error;
    type Future = Pin<Box<dyn Future<Output = Result<Self::Response, Self::Error>>>>;

    forward_ready!(service);

    fn call(&self, req: ServiceRequest) -> Self::Future {
        let user = Auth::bearer(req.request())
            .and_then(|t| self.tokens.get(&t).copied())
            .map(CurrentUser);

        match user {
            Some(user) => {
                // 写进 extensions：随请求走，不跨请求共享
                req.request().extensions_mut().insert(user);
                let fut = self.service.call(req);
                Box::pin(async move {
                    // 通过分支：body 类型是 B → 装进 EitherBody 的左边
                    fut.await.map(ServiceResponse::map_into_left_body)
                })
            }
            None => {
                println!("[AUTH] 拒绝 {} {}", req.method(), req.path());
                // 短路：**不调用** self.service.call(req)，内层完全不会执行
                let res = req.into_response(
                    HttpResponse::Unauthorized()
                        .insert_header((header::WWW_AUTHENTICATE, "Bearer"))
                        .json(json!({
                            "success": false,
                            "code": "unauthorized",
                            "message": "缺少或无效的 Authorization: Bearer <token>"
                        })),
                );
                Box::pin(async move {
                    // 拒绝分支：body 是 BoxBody → 装进右边
                    Ok(res.map_into_right_body())
                })
            }
        }
    }
}
