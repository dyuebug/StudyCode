// ============================================
// task-manager 库入口
// 日期：2026-11-01（第37天）
// ============================================

/*
【这一课的目标不是"再写一遍 todo-api"】

todo-api 是一个文件、一张表、五个 handler。task-manager 要回答的是
**"当东西变多以后，边界画在哪里"**：

    models      数据形状 + SQL 映射（不认识 HTTP）
    handlers    把 HTTP 请求翻译成对 models 的调用（不认识 SQL 细节）
    mw          横切关注点（认证、日志），在 handler 外面
    db          连接池与迁移
    lib.rs      对外接口 + 装配（main 与 tests 共用）
    main.rs     进程入口，只负责"起服务"

判断边界有没有画对，用一句话检查：
**"改这个东西时，我需要动几个文件？"**
换数据库 → 只动 db.rs；加字段 → models + 迁移；改路由 → 只动 lib.rs。

【为什么第 37 天就拆 lib.rs】

第 36 天在 todo-api 上验证过一次：`tests/` 是独立 crate，
没有 lib target 就什么都 import 不到。
所以这次**一开始就按能测的形状搭**，而不是等要写测试时再返工。

这正是"综合项目"的意义：把前面零散学到的约束
（lib target、装配收敛、中间件顺序、测试隔离）
**在动手前就装进结构里**。
*/

pub mod db;
pub mod error;
pub mod handlers;
pub mod models;
pub mod mw;
pub mod state;

use actix_web::body::BoxBody;
use actix_web::dev::{ServiceFactory, ServiceRequest, ServiceResponse};
use actix_web::{App, Error, web};

use state::AppState;

/// 构建完整的应用（含中间件）
///
/// # 为什么这个函数存在
/// 只把**路由**收敛到 `configure` 是不够的：App 级的 `.wrap(...)`
/// 必须挂在 `App` 上，而 `ServiceConfig` 拿不到 `App`。
/// 如果 `main.rs` 和 `tests/` 各写一遍 `.wrap()`，
/// 那"线上有日志、测试里没有"这种事迟早发生——
/// 而测试恰恰要验的就是"没有 token 会不会被拒"。
///
/// 所以把**整个 App 的构建**收敛到这里，两边都调它。
///
/// # 返回类型为什么这么长
/// `App` 的内层工厂类型在链式调用中每 `wrap` 一次就换一次类型，
/// 没法写死。用 `impl ServiceFactory<...>` 描述"它满足什么契约"
/// 而不是"它是什么类型"——这是 `impl Trait` 在返回值位置的典型用法。
///
/// ⚠️ 两个踩过的坑（actix-web 4.18 实测）：
/// 1. `App` **只有一个泛型参数** `App<T>`；写成 `App<T, BoxBody>`
///    会得到 `error[E0107]: struct takes 1 generic argument but 2 were supplied`
///    （`App<T, B>` 是更早版本的形状）。
/// 2. 约束里的请求类型必须是 `ServiceRequest`，**不是** `Request`。
///    照抄 `HttpServer` 的签名写成 `ServiceFactory<Request>` 会得到
///    `error[E0277]: the trait bound ...: ServiceFactory<actix_http::Request> is not satisfied`。
///    原因：App 内部的路由层处理的是已经"进了 App"的请求
///    （`ServiceRequest`，带有 extensions 与 payload），
///    而 `Request` 是 TCP 层刚解出来的原始请求。
///    源码依据：`impl<T, B> IntoServiceFactory<AppInit<T, B>, Request> for App<T>
///    where T: ServiceFactory<ServiceRequest, ...>`。
///
/// # 中间件顺序（第 35 天实测结论）
/// **后注册的 `wrap` 在外层。** 这里只有一个，所以只要保证它在
/// `.configure(...)` **之前**调用即可覆盖全部路由——
/// 第 35 天验证过：`wrap` 先写，之后注册的路由与 `default_service`
/// 都仍然被包住（404 也进了日志）。
pub fn app(
    state: AppState,
) -> App<
    impl ServiceFactory<
        ServiceRequest,
        Config = (),
        Response = ServiceResponse<BoxBody>,
        Error = Error,
        InitError = (),
    > + 'static,
> {
    App::new()
        .app_data(web::Data::new(state))
        // 日志在 App 级：这样 404（default_service）也会被记录。
        // 想监控一切，它就必须在最外层。
        .wrap(mw::logging())
        .configure(configure)
}

/// 注册全部路由
///
/// `main.rs` 与 `tests/` 都通过 [`app`] 走到这里——不会漂移。
///
/// 【路由分组：为什么要用 scope】
/// `/projects` 下的所有接口都需要认证，`/health` 不需要。
/// 把认证中间件挂在 `web::scope("/projects")` 上，
/// 就得到一条**结构性**保证：以后新增的 `/projects/xxx` 自动受保护。
/// 反之若在每个路由上单独 `.wrap(auth)`，漏写一个就是越权——
/// 而漏写不会报错，只会静默通过。
pub fn configure(cfg: &mut web::ServiceConfig) {
    cfg.route("/health", web::get().to(handlers::health))
        .service(
            web::scope("/projects")
                .wrap(mw::auth())
                .service(
                    web::resource("")
                        .route(web::get().to(handlers::list_projects))
                        .route(web::post().to(handlers::create_project)),
                )
                .service(
                    web::resource("/{project_id}")
                        .route(web::delete().to(handlers::delete_project)),
                )
                .service(
                    web::resource("/{project_id}/tasks")
                        .route(web::get().to(handlers::list_tasks))
                        .route(web::post().to(handlers::create_task)),
                ),
        )
        // 第 38 天：任务按 id 直接访问。
        //
        // 【为什么单开一个 /tasks scope，而不是全挂在 /projects 下】
        // 任务 id 本身全局唯一，资源位置不必再嵌套一层项目。
        // 路径也更省：`/tasks/7` 比 `/projects/1/tasks/7` 少一次"这个数字是谁"的歧义
        // （`/projects/{project_id}` 与 `/projects/{id}/tasks/{id}` 并存时尤其明显）。
        //
        // 鉴权不受影响：`owned_task` 会 JOIN 到 projects 判断归属，
        // 所以这个 scope 同样只对"自己的任务"生效。
        .service(
            web::scope("/tasks")
                .wrap(mw::auth())
                .service(
                    web::resource("/{task_id}")
                        .route(web::get().to(handlers::get_task))
                        .route(web::patch().to(handlers::patch_task))
                        .route(web::delete().to(handlers::delete_task)),
                )
                .service(
                    web::resource("/{task_id}/status")
                        .route(web::patch().to(handlers::patch_task_status)),
                )
                .service(web::resource("/{task_id}/tags").route(web::post().to(handlers::add_tag)))
                .service(
                    web::resource("/{task_id}/tags/{name}")
                        .route(web::delete().to(handlers::remove_tag)),
                ),
        );
}

/*
【路由表（第 38 天）】

    GET    /health                    免认证，查库连通性
    GET    /projects                  列出当前用户的全部项目
    POST   /projects                  创建项目
    DELETE /projects/{id}             删项目（级联删任务）
    GET    /projects/{id}/tasks       列任务（?status=&priority=&limit=&offset=）
    POST   /projects/{id}/tasks       建任务
    GET    /tasks/{id}                任务详情（含标签）
    PATCH  /tasks/{id}                改 title / priority / assignee_id / due_at
    DELETE /tasks/{id}                删任务
    PATCH  /tasks/{id}/status         状态流转（todo → doing → done）
    POST   /tasks/{id}/tags           贴标签（不存在则新建）
    DELETE /tasks/{id}/tags/{name}    撕标签（只删关联，不删标签本身）

第 39 天已补：统一错误层（`error::ApiError` + `ResponseError`）、
request-id 与耗时桶、4 worker 并发压测、`busy_timeout` 对照实验、README 收尾。
错误处理不再散落在 handler 里：`?` 直接把 `sqlx::Error` 变成 500，
而 500 的响应体是固定文案——真实原因只进日志。

【为什么第 37 天只做 4 个接口】

目标是**先打通一条完整链路**：
迁移 → 模型 → handler → 中间件 → 集成测试 → 真实服务。
8 个半成品接口的调试成本，比 4 个走通的接口高得多——
半成品会让人误以为"结构已经对了"，实际上只是问题还没被触发。
*/
