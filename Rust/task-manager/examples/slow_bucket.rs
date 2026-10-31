// ============================================
// 实验：日志中间件的耗时桶到底能不能看出来（第 39 天）
// 运行：cargo run --example slow_bucket
// ============================================

/*
【为什么要专门做个实验，而不是"看代码觉得对"】

`bucket()` 是个纯函数，写起来毫无技术含量，但它服务的对象是**日志**——
而日志的价值只有在"真的打出来"之后才成立。
本实验要拿到两条不同桶的日志行：

    (<10ms)     普通处理器
    (<1s)       睡 200ms
    (>=1s)      睡 1100ms

🚩 关键点：这里的 200ms / 1100ms 是**故意**跨在边界两侧的。
如果只睡 200ms，"≥1s 那档能不能打出来"就从来没有被验证过——
这正是第 38 天总结的毛病：不报错的地方，最容易永远错着。

【为什么要用 actix_web::test 而不是真起服务】
本实验只关心"中间件打什么日志"，不关心端口和 TCP。
`test::init_service` 直接调服务栈，省掉端口占用和异步收尾的麻烦。
完整的 4 worker 并发压测是另一件事，用真服务做（见 README §5.8）。
*/

use std::time::Duration;

use actix_web::{App, HttpResponse, test, web};
use task_manager::mw;

async fn fast() -> HttpResponse {
    HttpResponse::Ok().finish()
}

async fn sleep_200ms() -> HttpResponse {
    actix_web::rt::time::sleep(Duration::from_millis(200)).await;
    HttpResponse::Ok().finish()
}

async fn sleep_1100ms() -> HttpResponse {
    actix_web::rt::time::sleep(Duration::from_millis(1100)).await;
    HttpResponse::Ok().finish()
}

#[actix_web::main]
async fn main() {
    // 只挂日志中间件：本实验不涉及认证
    let app = test::init_service(
        App::new()
            .wrap(mw::logging())
            .route("/fast", web::get().to(fast))
            .route("/slow-200", web::get().to(sleep_200ms))
            .route("/slow-1100", web::get().to(sleep_1100ms)),
    )
    .await;

    println!("────────────────────────────────────────────────────────");
    println!("分别请求三个处理器，观察同一行日志里的耗时与分桶");
    println!("────────────────────────────────────────────────────────");

    for uri in ["/fast", "/slow-200", "/slow-1100"] {
        let resp = test::call_service(&app, test::TestRequest::get().uri(uri).to_request()).await;
        // 打印响应里的 request id，验证中间件确实把它回给了客户端
        let id = resp
            .headers()
            .get("x-request-id")
            .and_then(|v| v.to_str().ok())
            .unwrap_or("<缺失>")
            .to_string();
        println!("← {uri} 响应头 x-request-id = {id}");
    }

    println!("────────────────────────────────────────────────────────");
    println!("应当依次看到 <10ms / <1s / >=1s 三档");
}
