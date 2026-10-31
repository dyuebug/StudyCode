// ============================================
// chat.rs —— 单个连接的读写循环
// 日期：2026-11-04（第40天）
// ============================================

/*
【一个 WebSocket 连接的生命周期就三段】

    握手（lib.rs 里一次）
      ↓
    循环：收 / 发 / 心跳（本文件）
      ↓
    收尾：离开房间 + 广播 leave + close

【为什么循环里必须有三个分支】
只写"收"和"发"两个分支，会漏掉**心跳**：
TCP 连接可能半开着（对端掉电、断网），操作系统不会告诉你。
只有定期 ping 才能发现"对面已经不在了"。
反过来，只写心跳不处理消息，客户端发的东西永远没被读——
TCP 接收缓冲填满后，对端的写也会卡住。

【为什么"落后就断开"而不是"跳过丢失的消息"】
broadcast 的 `Lagged(n)` 已经**丢了** n 条。对一个聊天室来说，
继续连着但历史有洞，比断开更糟：用户以为自己看到的是全部。
所以这里主动告诉客户端 `{"type":"lagged","missed":n}` 然后关闭，
让它自己重连、重新拉成员列表。
*/

use std::sync::Arc;
use std::time::Duration;

use actix_ws::{Message, MessageStream, Session};
use futures_util::StreamExt;
use serde_json::json;
use tokio::sync::broadcast::error::RecvError;

use crate::room::Room;

/// 心跳间隔
///
/// 30 秒是常见取值：比大多数 NAT/代理的空闲超时（通常 60 秒）短一半，
/// 又不会频繁到浪费带宽。规律是"要比最容易断你的那一层更频繁"。
pub const HEARTBEAT: Duration = Duration::from_secs(30);

/// 单次写的超时
///
/// 【为什么必须有这个超时】
/// `actix_ws::Session` 内部是一个**容量 32 的 `tokio::sync::mpsc`**，
/// `session.text(..).await` 在队列满时会挂起等待——这是真实的背压。
/// 那么"只连不读"的客户端会怎样？
///
/// ```text
/// 它不读 → 它的 TCP 接收缓冲被填满 → 服务端的 TCP 发送缓冲被填满
///       → mpsc 32 个槽位也填满 → `session.text().await` **永远挂住**
/// ```
///
/// 挂住的是我们这个连接任务本身：它既不再读客户端发来的帧，
/// 也走不到 `rx.recv()` 去看 `Lagged`。也就是说——
/// **广播层的"落后就断开"根本发现不了这种客户端**，它只会像僵尸一样挂着，
/// 直到操作系统级的 TCP 超时（可能十几分钟）才回收。
///
/// 所以"落后就断开"和"写超时就断开"是两道互补的闸门：
///
/// ```text
/// 前者管"读得慢"（客户端在收，但跟不上）
/// 后者管"根本不读"（客户端收了不取，整条管线堵死）
/// ```
pub const SEND_TIMEOUT: Duration = Duration::from_secs(10);

/// 可注入的时间参数
///
/// 【为什么不是直接用常量】
/// 集成测试要验证"只连不读会被踢"。如果超时写死 10 秒，
/// 这一条用例就要跑 10 秒以上——测试一旦慢到没人愿意跑，就等于没有测试。
/// 把它做成配置后，测试里传 200 毫秒，同样的逻辑 0.2 秒就能验完。
///
/// 这也是"可测性"的一般规律：**凡是"等一段时间"的行为，时间都必须是参数**。
/// 生产默认值由 [`Default`] 提供，不受影响。
#[derive(Clone, Copy, Debug)]
pub struct ChatConfig {
    /// 心跳间隔
    pub heartbeat: Duration,
    /// 单次写的超时
    pub send_timeout: Duration,
}

impl Default for ChatConfig {
    fn default() -> Self {
        Self {
            heartbeat: HEARTBEAT,
            send_timeout: SEND_TIMEOUT,
        }
    }
}

impl ChatConfig {
    /// 从环境变量读取，缺省则用默认值
    ///
    /// `HEARTBEAT_MS` / `SEND_TIMEOUT_MS`，单位毫秒。
    /// 只认能解析成正整数的值，解析失败就当没设置——
    /// 因为"配置写错了"不应该让服务起不来，而应该退回安全默认值。
    pub fn from_env() -> Self {
        let mut cfg = Self::default();
        if let Some(ms) = env_ms("HEARTBEAT_MS") {
            cfg.heartbeat = Duration::from_millis(ms);
        }
        if let Some(ms) = env_ms("SEND_TIMEOUT_MS") {
            cfg.send_timeout = Duration::from_millis(ms);
        }
        cfg
    }
}

fn env_ms(key: &str) -> Option<u64> {
    std::env::var(key)
        .ok()
        .and_then(|s| s.parse::<u64>().ok())
        .filter(|v| *v > 0)
}

/// 一次写入的结果
///
/// 区分"超时"和"关闭"是为了日志：前者说明对端不读（我们主动踢），
/// 后者说明对端已经走了（正常收尾）。两者都退出循环，但原因不同。
enum SendOutcome {
    Ok,
    Closed,
    TimedOut,
}

/// 带超时地写一条文本
///
/// 参数收 `String` 而不是 `impl Into<ByteString>`：`ByteString` 来自 `actix_http`，
/// 我们的 Cargo.toml 并没有直接依赖它，为了一个签名去加依赖不值得。
/// `String` 可以直接 `Into<ByteString>`，够用。
async fn send_text(session: &mut Session, text: String, timeout: Duration) -> SendOutcome {
    match tokio::time::timeout(timeout, session.text(text)).await {
        Ok(Ok(())) => SendOutcome::Ok,
        Ok(Err(_)) => SendOutcome::Closed,
        Err(_) => SendOutcome::TimedOut,
    }
}

/// 带超时地写一个 ping
async fn send_ping(session: &mut Session, timeout: Duration) -> SendOutcome {
    match tokio::time::timeout(timeout, session.ping(b"hb")).await {
        Ok(Ok(())) => SendOutcome::Ok,
        Ok(Err(_)) => SendOutcome::Closed,
        Err(_) => SendOutcome::TimedOut,
    }
}

/// 连接主循环
///
/// # 参数
/// - `session`: 写端（发消息、ping、close）
/// - `stream`:  读端（客户端发来的帧）
/// - `room`:    所在房间（广播、在线数）
/// - `member`:  成员编号（房间里唯一）
/// - `cfg`:     心跳间隔与写超时（测试里会调小）
///
/// # 为什么读端和写端是两个对象
/// `actix_ws::handle` 返回的 `Session` 和 `MessageStream` 是分离的：
/// 这个设计逼你**明确区分"我要发"和"我要收"**，
/// 也让"一个任务同时处理收发"变成 `select!`，而不是共享可变状态。
pub async fn run(
    mut session: Session,
    mut stream: MessageStream,
    room: Arc<Room>,
    member: u64,
    cfg: ChatConfig,
) {
    let send_timeout = cfg.send_timeout;

    // 先订阅，再广播 join：顺序反了的话，join 消息自己收不到，
    // 客户端会少一行"你加入了"的提示（不影响别人，但自己会困惑）
    let mut rx = room.subscribe();

    let count = room.online();
    room.publish(json!({ "type": "join", "member": member, "count": count }).to_string());

    let mut hb = tokio::time::interval(cfg.heartbeat);
    // interval 的第一次 tick 是"立刻"返回的，跳过它，
    // 否则连接刚建立就白 ping 一次
    hb.tick().await;

    // "写不进去"退出时置位。收尾时要靠它决定还需不需要发 close：
    // 对一个已经不读的对端再发 close，只会让收尾再等满一个超时。
    let mut writer_blocked = false;

    loop {
        tokio::select! {
            // ---------- 分支 1：客户端发来的帧 ----------
            incoming = stream.next() => {
                match incoming {
                    Some(Ok(Message::Text(text))) => {
                        // 原样转发给房间里的所有人（含自己）——
                        // "自己也得看到自己发的消息"是聊天室的常识，
                        // 客户端不要自己插一条，否则多端登录时顺序会乱
                        //
                        // ⚠️ `text` 是 `actix_http::ByteString`，它 Deref 到 `str`。
                        // 直接写 `text.as_str()` 会解析成 `str::as_str`（不稳定 API），
                        // 正确写法是 `&*text` —— 解引用后取引用，得到 `&str`
                        room.publish(
                            json!({
                                "type": "msg",
                                "member": member,
                                "text": &*text,
                            })
                            .to_string(),
                        );
                    }
                    Some(Ok(Message::Binary(data))) => {
                        // 本项目只支持文本协议。binary 明确拒绝，而不是静默丢弃，
                        // 否则客户端会以为"发出去了，只是没人理"
                        let out = send_text(
                            &mut session,
                            json!({ "type": "error", "message": "只支持文本消息" }).to_string(),
                            send_timeout,
                        )
                        .await;
                        println!("[WS] 成员 {member} 发了 {} 字节二进制，已拒绝", data.len());
                        if !matches!(out, SendOutcome::Ok) {
                            break;
                        }
                    }
                    Some(Ok(Message::Ping(payload))) => {
                        // 客户端主动 ping 要回 pong；不回的话对端会判我们掉线。
                        // pong 也会进同一条有界队列，所以同样要设超时
                        let r = tokio::time::timeout(send_timeout, session.pong(&payload)).await;
                        if !matches!(r, Ok(Ok(()))) {
                            break;
                        }
                    }
                    Some(Ok(Message::Pong(_))) => {
                        // 心跳的回应，不需要处理
                    }
                    Some(Ok(Message::Close(_))) => break,
                    Some(Ok(_)) => {
                        // Continuation / Nop：actix-ws 已经帮我们把分片拼好了，
                        // 这里理论上不会出现
                    }
                    Some(Err(e)) => {
                        println!("[WS] 成员 {member} 协议错误: {e}");
                        break;
                    }
                    None => break,
                }
            }

            // ---------- 分支 2：房间广播 ----------
            received = rx.recv() => {
                match received {
                    Ok(text) => {
                        match send_text(&mut session, text, send_timeout).await {
                            SendOutcome::Ok => {}
                            SendOutcome::Closed => {
                                // 对端已关：继续循环没意义
                                break;
                            }
                            SendOutcome::TimedOut => {
                                // 对端"连着但不读"：整条写管线堵死了。
                                // 这就是任务清单里说的"只连不读要踢掉"，
                                // 只是触发点不在广播层，而在写入这一层
                                println!(
                                    "[WS] 成员 {member} 写超时 {send_timeout:?}（对端不读），主动断开"
                                );
                                writer_blocked = true;
                                break;
                            }
                        }
                    }
                    Err(RecvError::Lagged(missed)) => {
                        // 慢客户端：缓冲撑爆了，已经丢了 missed 条。
                        // 注意这条通知本身也要写出去——它同样可能卡住，
                        // 所以用 send_text 而不是裸 session.text().await
                        println!("[WS] 成员 {member} 落后 {missed} 条，主动断开");
                        let _ = send_text(
                            &mut session,
                            json!({ "type": "lagged", "missed": missed }).to_string(),
                            send_timeout,
                        )
                        .await;
                        break;
                    }
                    Err(RecvError::Closed) => break,
                }
            }

            // ---------- 分支 3：心跳 ----------
            _ = hb.tick() => {
                match send_ping(&mut session, send_timeout).await {
                    SendOutcome::Ok => {}
                    SendOutcome::Closed => break,
                    SendOutcome::TimedOut => {
                        println!(
                            "[WS] 成员 {member} 心跳写超时 {send_timeout:?}（对端不读），主动断开"
                        );
                        writer_blocked = true;
                        break;
                    }
                }
            }
        }
    }

    // 收尾顺序：先减在线数，再广播 leave，最后 close。
    // 顺序反了的话，leave 消息里的 count 会多算自己一个。
    room.leave();
    room.publish(json!({ "type": "leave", "member": member, "count": room.online() }).to_string());
    // close 也是一条要写出去的帧：对端不读的话，它同样会堵。
    // 所以只在写通道还通的时候发 close；已经堵死就直接放弃——
    // 反正对端也收不到，再等一个超时只是白白多挂 10 秒
    // （实测过：不加这个判断，被踢的连接会在收尾处再卡满 SEND_TIMEOUT）
    if !writer_blocked {
        let _ = tokio::time::timeout(send_timeout, session.close(None)).await;
    }
    println!(
        "[WS] 成员 {member} 离开 {}，剩余 {}",
        room.name,
        room.online()
    );
}
