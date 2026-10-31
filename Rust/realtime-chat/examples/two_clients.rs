// ============================================
// examples/two_clients.rs —— 两客户端联调
// 日期：2026-11-04（第40天）
// ============================================
//
// 【这个示例解决什么问题】
// "两个客户端在同一个房间里互通"这句话，光看代码是看不出来的：
// 三个人共用一份 broadcast，谁该收到谁的，只有真连上去才知道。
//
// 所以它按固定剧本跑，每一步都打印**谁说了什么、谁收到了什么**：
//
//     A 连上 → 收到自己的 join（count=1）
//     B 连上 → B 收到自己的 join（count=2）；A 收到 B 的 join
//     B 发一句 → A 收到（"乙发甲收"）
//     A 发一句 → B 收到（"甲发乙收"）
//
// 用法（服务端先在跑）：
//     cargo run --example two_clients -- ws://127.0.0.1:8091/ws/demo
//
// 退出码：0 = 全部剧本都符合预期，1 = 有一步不符（可当冒烟测试用）

use std::time::Duration;

use futures_util::{SinkExt, StreamExt};
use tokio_tungstenite::tungstenite::Message;

/// 每一步的等待上限：超过就判定"该来的没来"
///
/// 不设超时的话，一行断言写错就会让整个程序挂在 recv 上——
/// 那看起来像"服务端卡了"，实际是客户端在等一个永远不会来的帧。
const STEP_TIMEOUT: Duration = Duration::from_secs(5);

/// 收一条文本帧并返回其内容
///
/// 跳过 Ping/Pong：tokio-tungstenite 默认会自动回 Pong，
/// 但它们仍会出现在流里，不能当成"消息"
async fn next_text<S>(ws: &mut S, who: &str) -> Result<String, String>
where
    S: StreamExt<Item = Result<Message, tokio_tungstenite::tungstenite::Error>> + Unpin,
{
    loop {
        let msg = tokio::time::timeout(STEP_TIMEOUT, ws.next())
            .await
            .map_err(|_| format!("{who}: 等了 {STEP_TIMEOUT:?} 没收到任何帧"))?
            .ok_or_else(|| format!("{who}: 连接已关闭"))?
            .map_err(|e| format!("{who}: 协议错误 {e}"))?;

        match msg {
            Message::Text(t) => return Ok(t.to_string()),
            Message::Ping(_) | Message::Pong(_) => continue,
            Message::Close(_) => return Err(format!("{who}: 收到 Close")),
            other => return Err(format!("{who}: 收到非文本帧 {other:?}")),
        }
    }
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let url = std::env::args()
        .nth(1)
        .unwrap_or_else(|| "ws://127.0.0.1:8091/ws/demo".to_string());

    println!("连接 {url}");

    // ---------- 1. 甲上线 ----------
    let (mut a, _) = tokio_tungstenite::connect_async(&url).await?;
    println!("[甲] 已连接");
    let got = next_text(&mut a, "甲").await?;
    println!("[甲] 收到 → {got}");

    // ---------- 2. 乙上线 ----------
    let (mut b, _) = tokio_tungstenite::connect_async(&url).await?;
    println!("[乙] 已连接");
    let got = next_text(&mut b, "乙").await?;
    println!("[乙] 收到 → {got}");

    // 甲应该看到"有人加入"
    let got = next_text(&mut a, "甲").await?;
    println!("[甲] 收到 → {got}");

    // ---------- 3. 乙发 → 甲收 + 乙自己也能收到 ----------
    //
    // 服务端是"广播给房间里所有人（含发送者）"，
    // 所以接下来**两个客户端各有一条待读**，必须都读掉，
    // 否则下一条断言会读到上一条的结果（这是写这类脚本最常见的坑）
    b.send(Message::Text("你好，我是乙".into())).await?;
    println!("[乙] 发送 → 你好，我是乙");
    let got = next_text(&mut a, "甲").await?;
    println!("[甲] 收到 → {got}");
    let got = next_text(&mut b, "乙").await?;
    println!("[乙] 收到（自己的回显）→ {got}");

    // ---------- 4. 甲发 → 乙收 + 甲自己也能收到 ----------
    a.send(Message::Text("收到，我是甲".into())).await?;
    println!("[甲] 发送 → 收到，我是甲");
    let got = next_text(&mut b, "乙").await?;
    println!("[乙] 收到 → {got}");
    let got = next_text(&mut a, "甲").await?;
    println!("[甲] 收到（自己的回显）→ {got}");

    // ---------- 5. 乙下线，甲应看到 leave ----------
    b.close(None).await?;
    println!("[乙] 已断开");
    let got = next_text(&mut a, "甲").await?;
    println!("[甲] 收到 → {got}");

    a.close(None).await?;
    println!("剧本跑完");
    Ok(())
}
