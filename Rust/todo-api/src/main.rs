// ============================================
// Todo API - 主程序入口
// 日期：2026-06-21（第25天）
// ============================================

// 声明模块
mod models;
mod handlers;
mod state;

// 导入依赖
use actix_web::{web, App, HttpServer};
use state::init_state;

// --------------------------------------------
// 主函数
// --------------------------------------------

/// 应用入口
///
/// #[actix_web::main] 宏：
/// - 类似 #[tokio::main]
/// - 设置异步运行时
/// - 允许 main 函数使用 async/await
#[actix_web::main]
async fn main() -> std::io::Result<()> {
    println!("============================================");
    println!("🚀 Todo API 服务器启动中...");
    println!("============================================");

    // 1. 初始化共享状态
    //    创建空的任务列表，包装在 Arc<Mutex<>> 中
    let app_state = init_state();
    println!("✅ 应用状态初始化完成");

    // 2. 配置服务器地址
    let host = "127.0.0.1";
    let port = 8888;
    println!("📍 服务器地址: http://{}:{}", host, port);

    // 3. 创建并启动 HTTP 服务器
    println!("🔧 配置路由...");
    println!("   - GET    /todos         获取所有任务");
    println!("   - POST   /todos         创建新任务");
    println!("   - GET    /todos/:id     获取单个任务");
    println!("   - PUT    /todos/:id     更新任务");
    println!("   - DELETE /todos/:id     删除任务");
    println!("============================================");

    HttpServer::new(move || {
        // 4. 创建 App 实例（每个工作线程一个）
        App::new()
            // 5. 注入共享状态
            //    .clone() 是必要的，因为每个线程需要自己的 Arc 引用
            //    Arc 的 clone 只是增加引用计数，不会复制数据
            .app_data(web::Data::new(app_state.clone()))

            // 6. 配置路由
            //    .route(path, handler) - 定义路由规则
            //    web::get() / web::post() / web::put() / web::delete() - HTTP 方法
            //    .to(function) - 指定处理函数
            //
            // 路由顺序很重要：
            // - 精确路由（/todos）在前
            // - 参数路由（/todos/{id}）在后
            // - 避免路由冲突
            .route("/todos", web::get().to(handlers::get_all))
            .route("/todos", web::post().to(handlers::create))
            .route("/todos/{id}", web::get().to(handlers::get_by_id))
            .route("/todos/{id}", web::put().to(handlers::update))
            .route("/todos/{id}", web::delete().to(handlers::delete))
    })
    // 7. 绑定地址和端口
    .bind((host, port))?
    // 8. 运行服务器（阻塞，直到收到关闭信号）
    .run()
    .await
}

/*
【关键知识点】

1. #[actix_web::main] 宏：
   - 设置 Actix-web 的异步运行时
   - 类似 #[tokio::main]（实际上 Actix-web 底层用的就是 tokio）
   - 允许 main 函数是 async fn

2. HttpServer::new()：
   - 接受一个闭包（工厂函数）
   - 每个工作线程都会调用这个闭包创建 App 实例
   - move 关键字：闭包获取外部变量的所有权

3. App::new()：
   - 创建应用实例
   - 配置路由、中间件、状态
   - 每个工作线程有独立的 App 实例

4. .app_data()：
   - 注入共享状态
   - web::Data::new() 包装状态
   - 在 handler 中通过 web::Data<T> 提取

5. .route()：
   - 第一参数：路径（可以包含参数，如 "/todos/{id}"）
   - 第二参数：HTTP 方法 + 处理函数
   - web::get() / web::post() / web::put() / web::delete() - 指定 HTTP 方法
   - .to(handler) - 指定处理函数

6. 路径参数语法：
   - /todos/{id} - 路径参数用 {} 包裹
   - 参数名（id）需要与 handler 中的提取器匹配
   - 支持多个参数：/users/{user_id}/posts/{post_id}
   - 参数类型由 web::Path<T> 的泛型决定

7. 路由顺序的重要性：
   - 精确路由（/todos）应该在参数路由（/todos/{id}）之前
   - Actix-web 按顺序匹配路由
   - 避免路由冲突和意外匹配

8. .bind()：
   - 绑定 IP 地址和端口
   - 返回 Result<Server>
   - ? 操作符传播错误

7. .run().await：
   - 启动服务器
   - 等待服务器运行（阻塞）
   - 按 Ctrl+C 停止

8. move 闭包：
   - move || { ... }
   - 闭包获取 app_state 的所有权
   - 因为 app_state 是 Arc，clone 只是增加引用计数

9. 多线程模型：
   - Actix-web 默认使用多个工作线程
   - 每个线程有独立的 App 实例
   - 所有线程共享同一个 AppState（通过 Arc）

10. 为什么需要 .clone()？
    - HttpServer::new() 的闭包会被调用多次（每个线程一次）
    - 每次调用都需要 app_state
    - Arc::clone() 只是增加引用计数，不复制数据
    - 所有线程的 Arc 都指向同一份数据

11. 异步运行时：
    - #[actix_web::main] 自动配置
    - 不需要手动创建 tokio runtime
    - main 函数可以使用 .await
*/
