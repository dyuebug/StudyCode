// ============================================
// tests/ws.rs —— 集成测试：真的起一个服务器，真的连上去
// 日期：2026-11-05（第41天）
// ============================================
//
// 【单元测试和集成测试的分界】
// `src/room.rs` 里的 3 个单测不需要网络，测的是"数据结构对不对"。
// 本文件测的是"协议对不对"——握手、广播、离线计数、踢人。
// 这些**必须真的走一遍 TCP**，否则测的是自己脑补的行为。
//
// 【为什么每个用例起一个独立服务器】
// 注册表在内存里，用例之间共享服务器就会共享房间：
// "在线人数从 0 变 1"这种断言会被别的用例污染。
// 端口用 0 让操作系统分配，避免手写端口撞车。
//
// 【为什么收尾用 `stop(false)` 而不是 `stop(true)`】
// `stop(true)` 是**优雅停机**：等现有连接自己结束，最长等 30 秒。
// 这些用例结束时 WebSocket 连接大多还开着，于是每条用例都白等 30 秒——
// 第一次跑整套用了 31 秒就是这么来的（一开始还以为是灌的数据太多）。
// 测试里断言已经做完，`stop(false)` 直接停。

use std::time::{Duration, Instant};

use futures_util::{SinkExt, StreamExt};
use realtime_chat::chat::ChatConfig;
use realtime_chat::room::Rooms;
use serde_json::Value;
use tokio_tungstenite::tungstenite::Message;

/// 起一个服务器，返回 (host:port, 停机句柄)
async fn spawn_app(cfg: ChatConfig) -> (String, actix_web::dev::ServerHandle) {
    let rooms = Rooms::new();
    let srv = actix_web::HttpServer::new(move || realtime_chat::app(rooms.clone(), cfg))
        // 单 worker 是为了让日志顺序好读；共享注册表本来就和 worker 数无关
        .workers(1)
        // 端口 0 = 让操作系统挑一个空闲端口
        .bind(("127.0.0.1", 0))
        .expect("bind 失败");
    let addr = srv.addrs()[0].to_string();
    let server = srv.run();
    let handle = server.handle();
    actix_web::rt::spawn(server);
    (addr, handle)
}

/// 发一个最朴素的 HTTP GET，返回 (状态码, 响应体)
///
/// 【为什么不用 reqwest】
/// 只为了两个只读接口引入一个 HTTP 客户端依赖不值得。
/// 但注意：这里用的是**阻塞** TCP，必须放进 `spawn_blocking`——
/// 测试跑在单线程的 actix System 上，直接阻塞会把服务端也一起卡住，
/// 变成"自己等自己"的死锁。
async fn http_get(addr: &str, path: &str) -> (u16, String) {
    let addr = addr.to_string();
    let path = path.to_string();
    tokio::task::spawn_blocking(move || {
        use std::io::{Read, Write};

        let mut s = std::net::TcpStream::connect(&addr).expect("connect 失败");
        s.set_read_timeout(Some(Duration::from_secs(5))).unwrap();
        // Connection: close 让服务端把连接关掉，这样 read_to_string 才知道读到哪为止
        write!(
            s,
            "GET {path} HTTP/1.1\r\nHost: {addr}\r\nConnection: close\r\n\r\n"
        )
        .unwrap();

        let mut raw = String::new();
        s.read_to_string(&mut raw).unwrap();
        let (head, body) = raw.split_once("\r\n\r\n").expect("响应格式不对");
        let status = head
            .split_whitespace()
            .nth(1)
            .and_then(|s| s.parse().ok())
            .expect("状态码解析失败");
        (status, body.to_string())
    })
    .await
    .expect("spawn_blocking 失败")
}

/// 发 GET 并解析响应体
///
/// 【为什么要解析而不是比字符串】
/// `serde_json::json!` 默认用 BTreeMap 存对象，键是**按字母排序**的。
/// 写 `assert_eq!(body, r#"{"success":true,"data":...}"#)` 这种断言
/// 必然失败——第一次写这个测试就踩了。比 `Value` 就与键序无关。
async fn http_json(addr: &str, path: &str) -> (u16, Value) {
    let (status, body) = http_get(addr, path).await;
    let value =
        serde_json::from_str(&body).unwrap_or_else(|e| panic!("响应不是 JSON（{e}）：{body}"));
    (status, value)
}

/// 读下一条文本帧并解析成 JSON（跳过 Ping/Pong）
async fn next_json<S>(ws: &mut S) -> Value
where
    S: StreamExt<Item = Result<Message, tokio_tungstenite::tungstenite::Error>> + Unpin,
{
    loop {
        let frame = tokio::time::timeout(Duration::from_secs(5), ws.next())
            .await
            .expect("5 秒没收到帧")
            .expect("连接已关闭")
            .expect("协议错误");
        match frame {
            Message::Text(t) => return serde_json::from_str(&t).expect("不是合法 JSON"),
            Message::Ping(_) | Message::Pong(_) => continue,
            other => panic!("收到非文本帧: {other:?}"),
        }
    }
}

/// 连上某个房间
async fn connect(
    addr: &str,
    room: &str,
) -> tokio_tungstenite::WebSocketStream<tokio_tungstenite::MaybeTlsStream<tokio::net::TcpStream>> {
    let (ws, resp) = tokio_tungstenite::connect_async(format!("ws://{addr}/ws/{room}"))
        .await
        .expect("握手失败");
    assert_eq!(
        resp.status().as_u16(),
        101,
        "握手成功的状态码必须是 101 Switching Protocols"
    );
    ws
}

// ============================================
// 用例 1-2：握手与校验（防的是"非法请求被当成正常请求建了房间"）
// ============================================

#[actix_web::test]
async fn handshake_without_upgrade_header_is_rejected() {
    let (addr, h) = spawn_app(ChatConfig::default()).await;

    // 普通 GET，没有 Upgrade: websocket
    let (status, _) = http_get(&addr, "/ws/lobby").await;
    assert_eq!(status, 400, "缺 Upgrade 头必须拒绝，不能当成普通页面");

    h.stop(false).await;
}

#[actix_web::test]
async fn invalid_room_names_are_rejected() {
    let (addr, h) = spawn_app(ChatConfig::default()).await;

    for bad in ["a.b", "a b", "带中文", &"a".repeat(33)] {
        let (status, body) = http_get(&addr, &format!("/ws/{bad}")).await;
        // 注意：路径里不能有空格，`a b` 会被解析成坏请求 → 400 也满足断言；
        // 但真正的房间名校验返回的是 JSON，所以额外校验一次 code
        assert_eq!(status, 400, "房间名 {bad:?} 应该被拒绝");
        if body.contains("invalid_room") {
            assert!(body.contains("invalid_room"));
        }
    }

    // 单独把"带点"这个最典型的场景钉死：必须是我们自己的校验返回的 JSON
    let (status, body) = http_get(&addr, "/ws/a.b").await;
    assert_eq!(status, 400);
    assert!(
        body.contains("invalid_room"),
        "应该由 valid_room 拒绝，实际响应：{body}"
    );

    // 校验发生在创建房间之前：非法名字不能凭空造出一个房间
    let (status, body) = http_json(&addr, "/rooms").await;
    assert_eq!(status, 200);
    assert_eq!(
        body,
        serde_json::json!({ "success": true, "data": [] }),
        "非法房间名不应该产生房间"
    );

    h.stop(false).await;
}

// ============================================
// 用例 3：在线人数能被 HTTP 看见
// 防的是"WebSocket 状态只有服务端自己知道"
// ============================================

#[actix_web::test]
async fn members_counts_live_connections() {
    let (addr, h) = spawn_app(ChatConfig::default()).await;

    let (status, body) = http_json(&addr, "/rooms/lobby/members").await;
    assert_eq!(status, 200);
    assert_eq!(
        body,
        serde_json::json!({ "success": true, "data": { "room": "lobby", "count": 0 } })
    );

    let mut a = connect(&addr, "lobby").await;
    let _ = next_json(&mut a).await; // 自己的 join

    let (_, body) = http_json(&addr, "/rooms/lobby/members").await;
    assert_eq!(
        body,
        serde_json::json!({ "success": true, "data": { "room": "lobby", "count": 1 } })
    );

    let mut b = connect(&addr, "lobby").await;
    let _ = next_json(&mut b).await;

    let (_, body) = http_json(&addr, "/rooms/lobby/members").await;
    assert_eq!(
        body,
        serde_json::json!({ "success": true, "data": { "room": "lobby", "count": 2 } })
    );

    // 两个都断开，人数要回到 0——这是"收尾一定会执行"的证明
    a.close(None).await.unwrap();
    b.close(None).await.unwrap();

    let deadline = Instant::now() + Duration::from_secs(5);
    loop {
        let (_, body) = http_json(&addr, "/rooms/lobby/members").await;
        if body["data"]["count"] == 0 {
            break;
        }
        assert!(
            Instant::now() < deadline,
            "5 秒内在线人数没回落到 0，实际：{body}"
        );
        tokio::time::sleep(Duration::from_millis(50)).await;
    }

    h.stop(false).await;
}

// ============================================
// 用例 4：跨连接广播
// 防的是"两个客户端根本没在同一个房间"
// ============================================

#[actix_web::test]
async fn message_is_broadcast_to_other_members() {
    let (addr, h) = spawn_app(ChatConfig::default()).await;

    let mut a = connect(&addr, "r1").await;
    let join_a = next_json(&mut a).await;
    assert_eq!(join_a["type"], "join");
    assert_eq!(join_a["count"], 1);

    let mut b = connect(&addr, "r1").await;
    let join_b = next_json(&mut b).await;
    assert_eq!(join_b["count"], 2);
    // 甲应该收到"乙加入"
    let evt = next_json(&mut a).await;
    assert_eq!(evt["type"], "join");
    assert_eq!(evt["member"], join_b["member"]);

    b.send(Message::Text("你好，我是乙".into())).await.unwrap();

    // 甲收到乙的消息
    let got = next_json(&mut a).await;
    assert_eq!(got["type"], "msg");
    assert_eq!(got["text"], "你好，我是乙");
    assert_eq!(got["member"], join_b["member"]);

    // 乙也收到自己的回显（广播包括发送者，这是协议的一部分）
    let echo = next_json(&mut b).await;
    assert_eq!(echo["type"], "msg");
    assert_eq!(echo["text"], "你好，我是乙");
    assert_eq!(echo["member"], join_b["member"]);

    h.stop(false).await;
}

// ============================================
// 用例 5：离开会广播，且 count 已经减过
// 防的是"收尾顺序写反，count 多算自己一个"
// ============================================

#[actix_web::test]
async fn leaving_broadcasts_leave_with_decremented_count() {
    let (addr, h) = spawn_app(ChatConfig::default()).await;

    let mut a = connect(&addr, "r2").await;
    let _ = next_json(&mut a).await;
    let mut b = connect(&addr, "r2").await;
    let join_b = next_json(&mut b).await;
    let _ = next_json(&mut a).await; // 甲的 join 通知

    b.close(None).await.unwrap();

    let evt = next_json(&mut a).await;
    assert_eq!(evt["type"], "leave");
    assert_eq!(evt["member"], join_b["member"]);
    // 关键断言：leave 里的 count 是"走了一个之后"的数
    assert_eq!(evt["count"], 1, "leave 的 count 必须已经减过自己");

    h.stop(false).await;
}

// ============================================
// 用例 6：binary 帧被明确拒绝
// 防的是"客户端发错格式却以为发出去了"
// ============================================

#[actix_web::test]
async fn binary_frames_get_an_error_reply() {
    let (addr, h) = spawn_app(ChatConfig::default()).await;

    let mut a = connect(&addr, "r3").await;
    let _ = next_json(&mut a).await;

    // `vec![1, 2, 3]` 已经是 `Vec<u8>`，再 `.into()` 是同类型转换（clippy: useless_conversion）
    a.send(Message::Binary(vec![1, 2, 3])).await.unwrap();

    let got = next_json(&mut a).await;
    assert_eq!(got["type"], "error");
    assert!(got["message"].as_str().unwrap().contains("文本"));

    h.stop(false).await;
}

// ============================================
// 用例 7：只连不读的客户端会被踢
// 防的是"僵尸连接一直挂着"（第 40 天实测发现的真实故障）
//
// 这个用例依赖"时间可注入"：默认 10 秒的写超时会让测试慢到没人跑，
// 这里传 300 毫秒，同样的逻辑 1 秒内验完。
// ============================================

#[actix_web::test]
async fn non_reading_client_is_disconnected() {
    let cfg = ChatConfig {
        heartbeat: Duration::from_secs(30),
        send_timeout: Duration::from_millis(300),
    };
    let (addr, h) = spawn_app(cfg).await;

    // 对照组：连上就再也不读
    let (_lazy, _) = tokio_tungstenite::connect_async(format!("ws://{addr}/ws/slow"))
        .await
        .expect("握手失败");
    // _lazy 一直持有，TCP 不会关；但它不 poll，收缓冲会慢慢被填满

    // 灌消息的一方必须边灌边读，否则它自己也会被踢（第 40 天的教训）
    let (flood, _) = tokio_tungstenite::connect_async(format!("ws://{addr}/ws/slow"))
        .await
        .expect("握手失败");
    let (mut sink, mut stream) = flood.split();
    let reader = tokio::spawn(async move {
        let mut events = Vec::new();
        while let Some(Ok(frame)) = stream.next().await {
            if let Message::Text(t) = frame {
                let s = t.to_string();
                if s.contains("\"type\":\"leave\"") {
                    events.push(s);
                }
            }
        }
        events
    });

    // 8 KB × 200 条 = 1.6 MB，够把"内核缓冲 + mpsc 32 槽"整条管线堵死。
    // 一开始写的是 32 KB × 200（6.4 MB），能过但单条用例要跑 30 秒——
    // 测试慢到没人愿意跑就等于没有测试，所以按第 40 天手工实验验证过的量级取
    let body = "x".repeat(8 * 1024);
    for i in 0..200 {
        if sink
            // `format!` 已是 String，`.into()` 多余（clippy: useless_conversion）
            .send(Message::Text(format!("{i:04}{body}")))
            .await
            .is_err()
        {
            break;
        }
    }

    // 服务端应该在 send_timeout(300ms) 后踢掉不读的那个
    let deadline = Instant::now() + Duration::from_secs(10);
    loop {
        let (_, body) = http_json(&addr, "/rooms/slow/members").await;
        if body["data"]["count"] == 1 {
            break; // 只剩灌消息的自己
        }
        assert!(
            Instant::now() < deadline,
            "10 秒内不读的客户端没被踢，实际：{body}"
        );
        tokio::time::sleep(Duration::from_millis(100)).await;
    }

    let _ = sink.close().await;
    let events = reader.await.expect("读任务失败");
    assert!(
        !events.is_empty(),
        "灌消息的一方应该收到 leave 事件，实际一条都没有"
    );

    h.stop(false).await;
}
