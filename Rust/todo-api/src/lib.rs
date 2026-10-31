// ============================================
// Todo API - 库入口（lib target）
// 日期：2026-10-31（第36天：为集成测试而拆出）
// ============================================

/*
【为什么一个"只会跑服务"的项目需要 lib.rs】

`tests/` 下的集成测试是**独立的 crate**——它通过 `use todo_api::...`
引用当前包，而不是 include 源码。这意味着：

- 如果包只有 `src/main.rs`（binary target），`tests/` 里**什么也 import 不到**。
  报错是 `unresolved import 'todo_api'`，因为"可被外部引用的接口"不存在。
- 加上 `src/lib.rs`（lib target）后，包同时产出 lib + bin 两个目标，
  `tests/` 就能 `use todo_api::...`。

于是有一条必须遵守的纪律：
**能被测试断言的东西，都得在 lib 里；main.rs 只做"读配置 + 起服务"。**

这也是把路由装配从 `main()` 里搬到 `configure()` 的原因：
如果测试自己重新写一遍 `.route(...)`，那么"测试里的路由"和
"线上的路由"就是两份代码——改了一处忘了另一处，
测试会持续通过而线上已经 404。这就是**测试漂移（test drift）**。

一句话：**测试和应用必须共享同一份装配代码，否则测的不是同一个东西。**
*/

pub mod db;
pub mod handlers;
pub mod models;
pub mod state;

use actix_web::web;

/// 注册全部路由
///
/// # 为什么是 `ServiceConfig` 而不是 `App`
/// `App` 的生命周期绑在 worker 上，函数签名会带一堆泛型参数；
/// `ServiceConfig` 是光秃秃的 `&mut`，既能给 `App::configure` 用，
/// 也能给 `web::scope(...).configure(...)` 用——换挂载位置不用改函数。
///
/// # 用法
/// ```ignore
/// // main.rs
/// App::new()
///     .app_data(web::Data::new(state))
///     .configure(todo_api::configure)
///
/// // tests/api.rs
/// test::init_service(
///     App::new()
///         .app_data(web::Data::new(state))
///         .configure(todo_api::configure),
/// )
/// ```
pub fn configure(cfg: &mut web::ServiceConfig) {
    cfg.route("/health", web::get().to(handlers::health))
        // 同一路径的多个方法必须挂在**同一个 resource** 上。
        //
        // 实测对比（2026-10-31，actix-web 4.18.1，对 / 发 PUT）：
        //   cfg.route("/todos", get) + cfg.route("/todos", post)  → 404
        //   web::resource("/todos").route(get).route(post)        → 405
        //
        // 原因：`cfg.route()` 每次都注册一个**独立**的 resource，
        // 每个只认识自己那个方法；方法不匹配就整条 resource 不匹配，
        // 落到 default_service → 404。
        // 而合并成一个 resource 后，方法不匹配是"资源存在、方法不支持"，
        // 按 RFC 9110 应当回 405。
        //
        // 这个区别对排查很关键：404 = 路径写错了，405 = 方法写错了。
        .service(
            web::resource("/todos")
                .route(web::get().to(handlers::get_all))
                .route(web::post().to(handlers::create)),
        )
        .service(
            web::resource("/todos/{id}")
                .route(web::get().to(handlers::get_by_id))
                .route(web::put().to(handlers::update))
                .route(web::delete().to(handlers::delete)),
        );
}

/*
【拆分后各文件的边界】

    src/lib.rs        对外接口：模块导出 + 路由装配（测试与线上共用）
    src/main.rs       进程入口：只负责"建池 → 迁移 → 起 HTTP"，不含任何业务
    src/db.rs         连接池与迁移
    src/handlers.rs   请求处理
    src/models.rs     数据结构与序列化契约
    src/state.rs      共享状态

main.rs 里剩下的代码，理论上可以换成任意一个启动脚本而测试不受影响——
这正是"关注点分离"在可测性上的直接体现。
*/
