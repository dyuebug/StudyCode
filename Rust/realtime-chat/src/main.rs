// ============================================
// realtime-chat 入口
// 日期：2026-11-04（第40天）
// ============================================

/*
【main 只做三件事】读配置 → 建共享状态 → 起服务

⭐ 关键点：`Rooms` 必须在 `HttpServer::new` **外面**创建，
然后 clone 进每个 worker 的闭包。

为什么？`HttpServer::new` 的闭包每个 worker 调用一次。
如果写成：

    HttpServer::new(move || {
        let rooms = Rooms::new();          // ❌ 每个 worker 一份
        realtime_chat::app(rooms)
    })

4 个 worker 就有 4 份独立的注册表：A 连 worker1、B 连 worker2，
两人永远看不到对方，`/rooms/lobby/members` 也永远只显示 1。
更糟的是它**不报错**——就像两张不同的白板。
*/

use std::time::Duration;

use realtime_chat::chat::ChatConfig;
use realtime_chat::room::Rooms;

#[actix_web::main]
async fn main() -> std::io::Result<()> {
    let bind = std::env::var("BIND_ADDR").unwrap_or_else(|_| "127.0.0.1:8090".to_string());
    let workers: usize = std::env::var("WORKERS")
        .ok()
        .and_then(|s| s.parse().ok())
        .unwrap_or(4);

    // 只有一份注册表，所有 worker 共享
    let rooms = Rooms::new();
    let cfg = ChatConfig::from_env();

    println!("🚀 聊天服务 http://{bind}（{workers} 个 worker）");
    println!(
        "   心跳 {} ms，写超时 {} ms",
        cfg.heartbeat.as_millis(),
        cfg.send_timeout.as_millis()
    );
    println!("   打开 http://{bind}/ 即可，或在任意 WebSocket 客户端连 ws://{bind}/ws/lobby");
    println!("   HTTP 观测：GET /rooms   GET /rooms/{{room}}/members");

    actix_web::HttpServer::new(move || realtime_chat::app(rooms.clone(), cfg))
        .workers(workers)
        // 空闲连接 10 分钟还不发任何东西就断开。
        // 这是"兜底"：心跳 30 秒一次，正常连接不会触发；
        // 但客户端如果连上就装死（既不读也不回 pong），
        // 服务端的心跳写也会失败——两层机制互为保险。
        .client_request_timeout(Duration::from_secs(600))
        .bind(&bind)?
        .run()
        .await
}
