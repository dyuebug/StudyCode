// ============================================
// examples/flood.rs —— 往房间里灌消息的客户端
// 日期：2026-11-04（第40天）
// ============================================
//
// 【它自己必须边灌边读】
// 服务端会把每条消息广播给房间里的**所有人，包括发送者**。
// 如果这个客户端只写不读，它自己的接收缓冲也会被填满，
// 然后它自己也会被服务端判定为"不读的客户端"——
// 实验就变成了"两个客户端互相把对方踢了"，什么也证明不了。
//
// 所以这里用 `split()` 把连接拆成读半和写半：
//   读半 → 丢进一个独立任务，只统计帧数（相当于一个健康的客户端）
//   写半 → 主任务，按节奏灌消息
//
// 用法：
//     cargo run --example flood -- ws://127.0.0.1:8091/ws/lobby 200 8192
//     参数：地址 / 条数 / 每条字节数
//
// 观察点：灌的过程中如果收到 `leave`，说明房间里另一个客户端被服务端踢掉了。

use std::time::Duration;

use futures_util::{SinkExt, StreamExt};
use tokio_tungstenite::tungstenite::Message;

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let mut args = std::env::args().skip(1);
    let url = args
        .next()
        .unwrap_or_else(|| "ws://127.0.0.1:8091/ws/lobby".to_string());
    let count: usize = args.next().and_then(|s| s.parse().ok()).unwrap_or(200);
    let size: usize = args.next().and_then(|s| s.parse().ok()).unwrap_or(8192);
    // 灌完之后还守着多少秒。要观察"别人被踢"，这个时间必须大于服务端的写超时
    let wait: u64 = args.next().and_then(|s| s.parse().ok()).unwrap_or(2);

    let (ws, _) = tokio_tungstenite::connect_async(&url).await?;
    let (mut sink, mut stream) = ws.split();

    // 读半交给独立任务：统计收到多少条，并记录所有 join/leave 事件
    let reader = tokio::spawn(async move {
        let mut msgs = 0usize;
        let mut events: Vec<String> = Vec::new();
        while let Some(frame) = stream.next().await {
            match frame {
                Ok(Message::Text(t)) => {
                    let s = t.to_string();
                    // join/leave 是状态事件，单独记下来；msg 只计数
                    if s.contains("\"type\":\"join\"") || s.contains("\"type\":\"leave\"") {
                        events.push(s);
                    } else {
                        msgs += 1;
                    }
                }
                Ok(_) => {}
                Err(e) => {
                    events.push(format!("<协议错误 {e}>"));
                    break;
                }
            }
        }
        (msgs, events)
    });

    // 每条消息都带上序号和填充，保证大小稳定
    let body = "x".repeat(size.saturating_sub(32));
    println!("灌 {count} 条 × 约 {size} 字节到 {url}");

    for i in 0..count {
        let payload = format!("{i:08}|{body}");
        // 注意：这里不写 `.into()`。`format!` 出来已经是 String，再 `.into()`
        // 是同类型转换，clippy 会报 `useless_conversion`（第 43 天实测）。
        if let Err(e) = sink.send(Message::Text(payload)).await {
            println!("第 {i} 条发送失败：{e}");
            break;
        }
    }
    println!("灌完，再守 {wait} 秒，看服务端是否踢掉不读的那个客户端");
    tokio::time::sleep(Duration::from_secs(wait)).await;

    // 关掉写半，读任务会因为连接关闭而结束
    let _ = sink.close().await;
    let (msgs, events) = reader.await?;

    println!("收到普通消息 {msgs} 条");
    println!("收到状态事件 {} 条：", events.len());
    for e in &events {
        println!("  {e}");
    }
    Ok(())
}
