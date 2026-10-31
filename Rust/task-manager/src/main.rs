// ============================================
// task-manager 入口
// 日期：2026-11-01（第37天）
// ============================================

/*
【main.rs 只做三件事】
    读配置 → 初始化依赖 → 起服务器

任何一行业务逻辑出现在这里，都是分层开始腐烂的第一个信号。
路由也不在这里：路由在 `lib.rs::configure`，
因为集成测试用的必须是同一份路由表（第36天 todo-api 的教训）。
*/

use actix_web::{HttpServer, web};
use task_manager::{db, state::AppState};

#[actix_web::main]
async fn main() -> std::io::Result<()> {
    // 配置来自环境变量，代码里只留"合理的默认值"。
    // 换库、换路径、部署到只读文件系统的容器，都不需要改代码。
    let db_url =
        std::env::var("DATABASE_URL").unwrap_or_else(|_| "sqlite:task-manager.db".to_string());
    let bind = std::env::var("BIND_ADDR").unwrap_or_else(|_| "127.0.0.1:8090".to_string());

    // worker 数默认 4（actix 自己的默认值是 CPU 核数，这里显式写死是为了
    // 让"4 worker 并发"这个实验在任何机器上都能复现）。
    //
    // 【多 worker 意味着什么】
    // 每个 worker 是**独立线程**，各有一份 App 实例；
    // 但它们**共享同一个连接池**（`AppState` 里的 `pool` 是 clone 出来的句柄，
    // 内部是 `Arc`）。所以 "4 worker" 不等于 "4 个数据库连接"，也不等于
    // "4 个写者"——SQLite 的写锁是**进程级/文件级**的，无论多少 worker
    // 同一时刻只有一个写事务在跑。这一点在第 39 天的压测里能直接看到。
    let workers: usize = std::env::var("WORKERS")
        .ok()
        .and_then(|s| s.parse().ok())
        .unwrap_or(4);

    // 数据库起不来就直接退出：
    // "启动成功但每个请求都 500" 比 "启动失败" 难排查得多。
    //
    // `DB_BUSY_TIMEOUT_MS` 只为压测实验存在：设成 0 就能复现 SQLITE_BUSY。
    // 生产里别调小——它越小，越容易把正常的写锁争用变成 500。
    let busy_ms: u64 = std::env::var("DB_BUSY_TIMEOUT_MS")
        .ok()
        .and_then(|s| s.parse().ok())
        .unwrap_or(5_000);
    let pool =
        match db::init_db_with_busy_timeout(&db_url, std::time::Duration::from_millis(busy_ms))
            .await
        {
            Ok(p) => p,
            Err(e) => {
                eprintln!("❌ 数据库初始化失败（{db_url}）: {e}");
                std::process::exit(1);
            }
        };
    println!("✅ 数据库就绪：{db_url}（busy_timeout = {busy_ms} ms）");

    let state = AppState::new(pool.clone());
    println!("🚀 监听 http://{bind}（{workers} 个 worker）");
    println!("   演示用 token：Authorization: Bearer token-alice / token-bob");

    HttpServer::new(move || {
        // `app()` 来自 lib，测试里用的是同一个函数——
        // 路由、中间件、状态注入三处配置天然不会漂移。
        task_manager::app(state.clone()).app_data(web::JsonConfig::default().limit(64 * 1024))
    })
    .workers(workers)
    .bind(&bind)?
    .run()
    .await
}
