// ============================================
// examples/stay_online.rs —— 连上就挂着
// 日期：2026-11-04（第40天）
// ============================================
//
// 【为什么需要这个"什么都不做"的客户端】
// WebSocket 的状态在服务端内存里，HTTP 是唯一的观测窗口。
// 想验证 `GET /rooms/{room}/members` 报的是**真实在线数**，
// 就得在"人还连着"的那一刻去查——而 `two_clients` 跑完就退出了。
//
// 用法：
//     cargo run --example stay_online -- ws://127.0.0.1:8091/ws/demo 8
//     第 1 个参数：地址        第 2 个参数：挂多少秒（默认 10）
//
// 它会把收到的每一帧原样打印——包括心跳 Ping、别人的消息、leave。
// 第 41 天做"只连不读"实验时，也用它当对照组。

use std::time::Duration;

use futures_util::StreamExt;

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let mut args = std::env::args().skip(1);
    let url = args
        .next()
        .unwrap_or_else(|| "ws://127.0.0.1:8091/ws/demo".to_string());
    let secs: u64 = args.next().and_then(|s| s.parse().ok()).unwrap_or(10);

    let (mut ws, _) = tokio_tungstenite::connect_async(&url).await?;
    println!("[{url}] 已连接，保持 {secs} 秒");

    let deadline = tokio::time::Instant::now() + Duration::from_secs(secs);
    loop {
        tokio::select! {
            // 到点自己关，模拟"用户关掉页面"
            _ = tokio::time::sleep_until(deadline) => {
                println!("[{url}] 到点，主动断开");
                let _ = ws.close(None).await;
                break;
            }
            frame = ws.next() => {
                match frame {
                    Some(Ok(msg)) => println!("[{url}] 收到 {msg:?}"),
                    Some(Err(e)) => { println!("[{url}] 错误 {e}"); break; }
                    None => { println!("[{url}] 连接已关闭"); break; }
                }
            }
        }
    }
    Ok(())
}
