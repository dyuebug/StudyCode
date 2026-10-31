// ============================================
// examples/lazy_reader.rs —— "只连不读"的客户端
// 日期：2026-11-04（第40天）
// ============================================
//
// 【它故意什么都不做】
// 完成握手之后就再也不 poll 这个流——不读、不回 pong、不发消息。
// 这正是"客户端进程卡死""标签页被系统冻结""网络单向中断"时的样子：
// 服务端以为它还在，它其实已经不可能收到任何东西了。
//
// 【为什么必须留一个这样的例子】
// 只看代码，`broadcast` 的 `Lagged` 看起来能兜住所有慢客户端。
// 但 `Lagged` 只有在**我们能读到 `rx.recv()`** 时才会出现；
// 而对端不读会让 `session.text().await` 先卡死，
// 于是根本走不到 `rx.recv()`。这个结论必须实测，不能靠读代码断言。
//
// 用法：
//     cargo run --example lazy_reader -- ws://127.0.0.1:8091/ws/lobby 30
//
// 退出方式：到点自动退出，或者 Ctrl-C。

use std::time::Duration;

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let mut args = std::env::args().skip(1);
    let url = args
        .next()
        .unwrap_or_else(|| "ws://127.0.0.1:8091/ws/lobby".to_string());
    let secs: u64 = args.next().and_then(|s| s.parse().ok()).unwrap_or(30);

    // `_ws` 必须绑定住：它一被 drop，TCP 连接就关了，
    // 那就变成"正常断开"，测不出"连着但不读"这种情况
    let (_ws, _) = tokio_tungstenite::connect_async(&url).await?;
    println!("[{url}] 握手完成，接下来 {secs} 秒内**不读任何帧**");

    // ⚠️ 关键：这里没有 `ws.next()`。
    // `ws` 只是被持有住，所以 TCP 连接不会关——但它的接收缓冲会慢慢被填满。
    // 填满之后，服务端的写就会堵住。
    tokio::time::sleep(Duration::from_secs(secs)).await;

    println!("[{url}] 到点，退出");
    // ws 在这里被 drop，TCP 连接随之关闭
    Ok(())
}
