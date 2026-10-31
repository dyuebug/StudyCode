// ============================================
// Todo API - 主程序入口
// 日期：2026-06-21（第25天）创建
// 更新：2026-10-29（第33天：持久层换成 SQLite）
// 更新：2026-10-31（第36天：模块搬到 lib.rs，这里只负责"起服务"）
// ============================================

// 模块定义移到了 lib.rs（见那边的说明）。这里通过 crate 名引用它们，
// 好处是 tests/ 下的集成测试能用**同一份**代码与**同一份**路由装配。
use todo_api::{db, state::AppState};

use actix_web::{web, App, HttpServer};

/// 应用入口
///
/// 【启动流程】
///   1. 建连接池（WAL、外键、超时）
///   2. 跑迁移（表结构的唯一合法入口）
///   3. 导入旧的 todos.json（如果存在且表为空）
///   4. 起 HTTP 服务
///
/// 顺序不能变：没表就查不了，没数据就少一条导入路径。
/// 每一步失败都让**进程启动失败**，而不是留到第一个请求才崩。
#[actix_web::main]
async fn main() -> std::io::Result<()> {
    println!("============================================");
    println!("🚀 Todo API 服务器启动中（SQLite 版）");
    println!("============================================");

    const DB_URL: &str = "sqlite:todos.db";

    // 1 + 2. 建池并跑迁移
    let pool = db::init_db(DB_URL).await.map_err(|e| {
        eprintln!("❌ 数据库初始化失败: {}", e);
        std::io::Error::other(e.to_string())
    })?;
    println!("✅ 数据库已就绪（{}）", DB_URL);

    // 3. 旧数据导入（第 27 天遗留的 todos.json）
    match db::import_json_if_empty(&pool, "todos.json").await {
        Ok(0) => println!("📂 无需导入旧数据"),
        Ok(n) => println!("📦 已从 todos.json 导入 {} 条历史数据", n),
        Err(e) => {
            // 导入失败不应该阻止服务启动：数据仍在文件里，可以人工处理
            eprintln!("⚠️  导入旧数据失败（服务继续启动）: {}", e);
        }
    }

    // 4. 应用状态
    let app_state = AppState::new(pool);
    println!("✅ 应用状态初始化完成");

    let host = "127.0.0.1";
    let port = 8888;
    println!("📍 服务器地址: http://{}:{}", host, port);
    println!("🔧 路由:");
    println!("   - GET    /health        健康检查（含数据库连通性）");
    println!("   - GET    /todos         获取所有任务");
    println!("   - POST   /todos         创建新任务");
    println!("   - GET    /todos/{{id}}    获取单个任务");
    println!("   - PUT    /todos/{{id}}    更新任务");
    println!("   - DELETE /todos/{{id}}    删除任务");
    println!("============================================");

    HttpServer::new(move || {
        App::new()
            // SqlitePool 内部是 Arc，clone 只是加引用计数
            .app_data(web::Data::new(app_state.clone()))
            // 路由只在 lib.rs 里定义一次：集成测试用的是同一个函数
            .configure(todo_api::configure)
    })
    .bind((host, port))?
    .run()
    .await
}

/*
【第 33 天迁移要点】

1. 为什么 `init_db` 返回 `Box<dyn std::error::Error>`
   它同时要报"连接失败"（sqlx::Error）和"迁移失败"（MigrateError），
   两个错误类型不同。用 trait object 统一，避免为 demo 定义自定义错误枚举。
   生产项目里通常用 `thiserror` 定义 `AppError`，并为每种错误映射 HTTP 状态码。

2. `std::io::Error::other(...)`
   把任意错误转成 main 的返回类型。这是 Rust 1.74 起稳定的便捷构造器
   （旧写法是 `std::io::Error::new(ErrorKind::Other, ...)`）。
   也可以改用 `anyhow::Result`，但 demo 不值得多一个依赖。

3. 导入失败为什么不让启动失败
   数据仍在 `todos.json` 里（改名归档只在成功后发生）。
   让它启动，运维还能用 API 查看空库、人工补数据；
   直接崩，连排查入口都没有。**降级要比失败更可用。**

4. 为什么删掉了 `mod persistence;`
   `persistence.rs` 作为第 27 天的实现保留在目录里供对照，
   但不再参与编译——它 `use crate::models::Todo`，
   而 `Todo.id` 已经从 u32 变成 i64，留着编译不过。
   想复习旧实现时看 `git log` 或文件本身即可。

5. 数据库文件的三个伴生文件
   `todos.db`、`todos.db-wal`、`todos.db-shm` 都会出现在工作目录。
   它们必须一起进 `.gitignore`——只忽略 `todos.db` 会把 WAL 日志提交上去。
*/
