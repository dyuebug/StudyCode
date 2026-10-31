// ============================================
// WebSocket 实时通信 - 超详细注释版
// 日期：2026-10-29（第34天）
// 难度：⭐⭐⭐ 高级
// 运行：cargo run              → 打印教学演示，然后启动服务
//      cargo run -- --demo-only → 只打印演示，不起服务
//      cargo test               → 跑协议层单元测试
// ============================================

/*
【核心概念】简要说明

是什么？
- WebSocket 是**一条长连接上的双向字节流**，建立在一次 HTTP 握手之后。
  握手完成（HTTP 101）之后，这条 TCP 连接就不再属于 HTTP 了。

为什么需要？
- HTTP 是"客户端问、服务端答"。服务端**没有办法主动说话**。
  聊天、协作编辑、行情推送、进度条——这些都要求服务端能主动推。

核心特点？
- 全双工：同一时刻双向都能发
- 帧（frame）为单位：不是字节流随便写，而是有边界的消息
- 低开销：握手后每帧头部只有 2–14 字节，没有 HTTP 头

Rust/actix 特色？
- `actix-ws::handle()` 一次调用完成握手，返回
  `(HttpResponse, Session, MessageStream)` 三件套：
  响应交给框架、`Session` 负责写、`MessageStream` 负责读。
- **读写是两个独立的对象**——这是理解 async Rust 里"如何同时读写一条连接"的关键：
  不需要锁，把读写拆成两个任务即可。
*/

use std::sync::atomic::{AtomicUsize, Ordering};

use actix_web::{web, App, Error, HttpRequest, HttpResponse, HttpServer, Responder};
use actix_ws::{AggregatedMessage, CloseCode, CloseReason};
use serde::{Deserialize, Serialize};

mod protocol;
mod room;

use protocol::{accept_key, backoff_ms, decode_frame, encode_frame, FrameError, Opcode};
use room::{Alive, OnlineGuard, Room, IDLE_TIMEOUT, PING_INTERVAL};

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

fn section_1_1_why_websocket() {
    println!("1.1 为什么需要 WebSocket");
    println!("------------------------------------------------------------");
    /*
    【四种"服务端推送"方案】

    ┌─────────────┬──────────────┬────────────┬──────────────┐
    │ 方案        │ 实时性       │ 服务端开销 │ 主要问题     │
    ├─────────────┼──────────────┼────────────┼──────────────┤
    │ 短轮询      │ 一个间隔     │ 高         │ 空请求极多   │
    │ 长轮询      │ 近似实时     │ 中         │ 每消息一次重连│
    │ SSE         │ 实时         │ 低         │ 只能单向下行 │
    │ WebSocket   │ 实时         │ 低         │ 需要状态管理 │
    └─────────────┴──────────────┴────────────┴──────────────┘

    短轮询：每秒问一次"有新消息吗"。99% 的回答是"没有"。
            1000 个客户端 = 每秒 1000 个空 HTTP 请求，每个都带完整头部。

    长轮询：没消息就挂住不返回，有消息立刻返。省掉了空请求，
            但**每条消息都要重新发一次 HTTP 请求**，头部开销仍然存在。

    SSE（Server-Sent Events）：一条 HTTP 响应流，服务端持续往下写。
            完美适合"只下行"的场景（股票行情、日志流）。
            但它**不能上行**——客户端说话还得另开请求。

    WebSocket：一次握手，之后双向。协议开销降到每帧 2 字节起。

    【什么时候别用 WebSocket】
    - 只需要服务端推、客户端不说话 → 用 SSE，更简单，自动重连
    - 消息频率很低（几分钟一次）→ 用轮询，省掉长连接的状态管理
    - 只是要"等一个结果" → 用普通 HTTP + 异步任务查询

    **WebSocket 的成本不在协议，在状态管理**：
    谁在线、消息发给了谁、断线了怎么恢复、横向扩展时如何跨实例广播。
    */
    println!("   四种方案：短轮询 / 长轮询 / SSE / WebSocket");
    println!("   SSE 只能下行；WebSocket 才是全双工");
    println!("   WebSocket 的成本在「连接状态管理」，不在协议本身");
}

fn section_1_2_handshake() {
    println!();
    println!("1.2 握手：一次 HTTP 请求如何变成全双工连接");
    println!("------------------------------------------------------------");
    /*
    【客户端发出的请求】

    GET /ws HTTP/1.1
    Host: 127.0.0.1:8081
    Upgrade: websocket                 ← 我想换协议
    Connection: Upgrade                ← 这个头必须一起出现（历史上防代理误处理）
    Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==   ← 16 字节随机数的 Base64
    Sec-WebSocket-Version: 13          ← 版本，13 是唯一的现行版本
    Sec-WebSocket-Protocol: chat       ← （可选）子协议协商

    【服务端必须回的响应】

    HTTP/1.1 101 Switching Protocols   ← 101 是唯一正确的状态码
    Upgrade: websocket
    Connection: Upgrade
    Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=   ← 由 Key 算出来

    【Accept 怎么算】
        accept = Base64( SHA1( key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11" ) )

    为什么搞这么一套？
    答案很朴素：**防止"这不是 WebSocket"的连接被误升级**。
    - Key 是客户端随机生成的，服务端无法预知
    - 拼上固定 GUID 再 SHA-1，保证结果不可预测也无法伪造
    - 某个中间缓存如果错误地重放了这个响应，Accept 对不上，连接就不会建立

    ⚠️ SHA-1 在这里**不承担任何安全职责**。它只是"让结果看起来像随机"。
    不要因为这里出现 SHA-1 就以为 WebSocket 的加密用了 SHA-1。
    WebSocket 本身**不加密**：明文 ws:// 与加密 wss:// 的区别，
    完全来自底下是 TCP 还是 TLS。

    【握手之后】
    101 响应发出的那一刻，这条 TCP 连接就被**移交给 WebSocket 协议**了。
    之后上面跑的不再是 HTTP 报文，而是 WebSocket 帧。
    这也解释了为什么不能在同一个端口上"既当 HTTP 又当 WS"——
    其实可以，靠路径区分，本项目的 `/` 和 `/ws` 就是这样。
    */
    let sample_key = "dGhlIHNhbXBsZSBub25jZQ==";
    println!("   示例 Key    : {}", sample_key);
    println!("   应回的 Accept: {}", accept_key(sample_key));
    println!("   握手之后这条 TCP 连接不再传输 HTTP 报文");
}

fn section_1_3_frames() {
    println!();
    println!("1.3 帧结构：2 字节头部 + 可选扩展 + 载荷");
    println!("------------------------------------------------------------");
    /*
    【一个帧的字节布局】

     0                   1                   2                   3
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-------+-+-------------+-------------------------------+
    |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
    |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
    |N|V|V|V|       |S|             |   (if payload len==126/127)   |
    | |1|2|3|       |K|             |                               |
    +-+-+-+-+-------+-+-------------+-------------------------------+
    |     Extended payload length continued, if payload len == 127  |
    +-------------------------------+-------------------------------+
    |  Masking-key, if MASK set to 1 (4 bytes)                      |
    +---------------------------------------------------------------+
    |  Payload Data                                                 |
    +---------------------------------------------------------------+

    【FIN】1 = 这是消息的最后一帧。大消息可以分片，前几帧 FIN = 0。
    【RSV1-3】没用扩展时必须全 0。用了 permessage-deflate 压缩时 RSV1 = 1。
    【opcode】
        0x0 延续帧（分片后续）
        0x1 文本（载荷必须是合法 UTF-8）
        0x2 二进制（任意字节）
        0x8 关闭
        0x9 ping
        0xA pong
        0x3-0x7 / 0xB-0xF 保留 → 收到必须关闭连接
    【MASK】客户端→服务端**必须** 1；服务端→客户端**必须** 0。

    【三种长度编码】
    - 负载 < 126        ：7 bit 直接写
    - 126 ≤ 负载 ≤ 65535：标记位写 126，后跟 2 字节大端长度
    - 负载 > 65535      ：标记位写 127，后跟 8 字节大端长度

    注意 126 和 127 不是"长度值"，是**逃逸标记**。
    这解释了为什么 126 字节的载荷要占 4 字节头而不是 2 字节——
    多花 2 字节，换取"短消息永远只花 2 字节头"。

    【掩码的作用】
    客户端必须给自己的载荷做 XOR 掩码（键是 4 字节随机数）。
    服务端收到的帧要先 XOR 还原才是真数据。
    目的不是保密（键就在帧里，谁都能还原），
    而是**防止中间代理被投毒**：见第三部分的深入理解。
    */
    let a = encode_frame(true, Opcode::Text, b"hi", Some([0x01, 0x02, 0x03, 0x04]));
    println!("   文本帧 \"hi\" + 掩码键 [01,02,03,04] 的字节：{}", hex(&a));
    println!("   解读：81 = FIN+Text，82 = MASK+len(2)，后 4 字节掩码键，末 2 字节异或后的载荷");
    println!("   （82 里 0x80 是 MASK 位，0x02 是长度 2）");
}

fn section_1_4_actix_model() {
    println!();
    println!("1.4 actix-ws 的三个对象");
    println!("------------------------------------------------------------");
    /*
    let (response, session, stream) = actix_ws::handle(&req, body)?;

    response: HttpResponse
        一个 101 响应。**必须由你返回**（actix-ws 只是帮你填好头部）。
        如果处理函数里 `await` 太久才返回它，握手就超时了。

    session: Session
        写端。有 text() / binary() / ping() / pong() / close()。
        `Session: Clone`，可以克隆多份给不同任务用（内部有锁保护写操作）。
        ⚠️ close() 的签名是 `close(mut self, ...)`——**消费 self**。
        调用后就不能再用这个 session 写了，编译期就会拦住你。

    stream: MessageStream
        读端。`.recv().await -> Option<Result<Message, ProtocolError>>`
        `.max_frame_size(n)`：单帧上限，防 DoS
        `.aggregate_continuations()`：自动把分片拼成完整消息，
                                     之后拿到的是 AggregatedMessage

    【最重要的一点】
    session 和 stream 是**分开的两个值**。
    所以"同时读写一条连接"根本不需要锁——把读交给一个任务、写交给另一个任务，
    各自持有一半即可。这是 async Rust 处理长连接的标准姿势。

    【为什么必须 spawn】
    处理函数必须**立刻返回 response**，握手才算完成。
    如果处理函数自己去 `while let Some(msg) = stream.recv().await`，
    那个循环永远不会结束，response 永远不返回，客户端只能看到超时。
    → 长连接逻辑必须 `actix_web::rt::spawn` 出去。
    */
    println!("   handle() → (HttpResponse, Session, MessageStream)");
    println!("   写 = Session（可 Clone）  读 = MessageStream");
    println!("   长连接逻辑必须 spawn，handler 必须立刻返回 HttpResponse");
}

// ============================================
// 第二部分：基础操作实践
// ============================================

fn section_2_1_accept_key() {
    println!();
    println!("2.1 计算 Sec-WebSocket-Accept");
    println!("------------------------------------------------------------");
    // RFC 6455 §1.3 的官方测试向量
    let key = "dGhlIHNhbXBsZSBub25jZQ==";
    let got = accept_key(key);
    println!("   Key            = {}", key);
    println!("   Accept         = {}", got);
    println!("   期望值         = s3pPLMBiTxaQ9kYGzzhZRbK+xOo=");
    println!("   一致           = {}", got == "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=");
    println!("   （这个向量来自 RFC 6455 原文，能对上说明 SHA-1 + Base64 实现正确）");
}

fn section_2_2_frame_roundtrip() {
    println!();
    println!("2.2 帧编码与解码闭环");
    println!("------------------------------------------------------------");

    for (label, payload) in [
        ("短载荷 5 字节", b"hello".to_vec()),
        ("中载荷 300 字节", vec![0xAB; 300]),
        ("长载荷 70000 字节", vec![0x5A; 70_000]),
    ] {
        let raw = encode_frame(true, Opcode::Binary, &payload, Some([1, 2, 3, 4]));
        let header = match raw[1] & 0x7F {
            126 => "标记 126 + 2 字节长度",
            127 => "标记 127 + 8 字节长度",
            _ => "7 bit 直接长度",
        };
        println!("   {:<18} 帧总长 {:>6} 字节  头部 {}", label, raw.len(), header);

        let (frame, consumed) = decode_frame(&raw, true).expect("自己编码的帧必须能解码");
        assert_eq!(consumed, raw.len());
        assert_eq!(frame.payload, payload, "解码后载荷必须与原文一致");
    }
    println!("   三种长度编码都能闭环，解码后的载荷与原文逐字节一致");

    // 错误路径
    println!();
    let unmasked = encode_frame(true, Opcode::Text, b"hi", None);
    println!("   服务端发出的帧不带掩码，按客户端帧解析 → {:?}",
             decode_frame(&unmasked, true));
    println!("   同样字节按服务端帧解析 → {}", if decode_frame(&unmasked, false).is_ok() { "Ok" } else { "Err" });

    let partial = encode_frame(true, Opcode::Text, b"hello", None);
    println!("   只喂 3 个字节给解码器 → {:?}",
             decode_frame(&partial[..3], false).map(|_| ()));

    let mut bomb = vec![0x82u8, 0xFF];
    bomb.extend_from_slice(&(2u64 * 1024 * 1024 * 1024).to_be_bytes());
    println!("   声称 2 GiB 载荷的头部 → {:?}", decode_frame(&bomb, false));
    println!("   注意：没有任何载荷字节，却立刻返回 PayloadTooLarge —— 先判大小再等数据");
}

fn section_2_3_backoff_table() {
    println!();
    println!("2.3 断线重连的指数退避 + 抖动");
    println!("------------------------------------------------------------");
    println!("   {:<8} {:<12} {:<12}", "尝试", "无抖动(ms)", "有抖动(ms)");
    for attempt in 0..8 {
        let no_jitter = (500u64 << attempt).min(30_000);
        let with = backoff_ms(attempt, 500, 30_000, 1234 + attempt as u64 * 7);
        println!("   {:<8} {:<12} {:<12}", attempt, no_jitter, with);
    }
    println!("   无抖动时所有客户端会同一毫秒重连（惊群），抖动把它打散");
}

fn section_2_4_err_handling() {
    println!();
    println!("2.4 FrameError 的四个变体分别意味着什么");
    println!("------------------------------------------------------------");
    let cases: Vec<(&str, FrameError, &str)> = vec![
        ("Incomplete", FrameError::Incomplete, "不是错误，继续读"),
        ("ReservedOpcode", FrameError::ReservedOpcode(3), "协议错误，关闭连接"),
        ("PayloadTooLarge", FrameError::PayloadTooLarge(2 << 30), "DoS 防护，关闭连接"),
        ("MissingMask", FrameError::MissingMask, "协议错误，关闭连接"),
    ];
    for (name, _, action) in cases {
        println!("   {:<18} → {}", name, action);
    }
    println!();
    println!("   把 Incomplete 单独建一个变体，是为了让调用方**无法**把它");
    println!("   和真正的协议错误混在一起处理——这是「用类型表达意图」的一个实例。");
}

// ============================================
// 第三部分：深入理解（可选）
// ============================================

/*
==============================================
🔍 深入理解 1：客户端为什么必须掩码
==============================================

如果你觉得"键就写在帧里，掩码毫无保密作用"，那是对的。
掩码的目的不是保密，而是**防止中间缓存被投毒**。

历史事件（2000 年代中期，配合浏览器插件）：
攻击者控制一个恶意网站，让浏览器通过代理向某个"看起来无害"的 URL
发大量精心构造的请求。如果攻击者能**完全预测**请求体（包括 TCP 层字节），
某些共享代理就会认为两次请求相同，把第一次的响应缓存下来，
再把它当作第二次请求的响应返回给**另一个用户**。

关键在于攻击者需要一个"能让受害者浏览器发出的、内容可控但字节完全确定"的请求。
WebSocket 帧如果允许不带掩码，恰好满足这个条件。

服务端检查"必须带掩码"，就等于强制客户端在载荷里引入 4 字节随机数——
攻击者无法预测帧的字节，也就无法制造可缓存的重复请求。

**所以掩码是给中间设备看的，不是给服务端看的。**
这也是为什么反方向（服务端→客户端）不掩码：没有这个威胁模型。

==============================================
🔍 深入理解 2：broadcast 的 ring buffer 与 Lagged
==============================================

`tokio::sync::broadcast::channel(capacity)` 内部是一个**定长环形缓冲区**。

写入：覆盖最旧的位置。
读取：每个 Receiver 记住自己读到哪了。

当某个 Receiver 落后超过 capacity 时，它要读的位置已经被覆盖了。
此时不是返回旧消息（已经不在了），也不是阻塞发送方，
而是返回 `Err(RecvError::Lagged(n))` —— **明确告诉你丢了 n 条**。

这个设计的关键在于：**唯一的诚实反馈**。
- 如果静默跳过 → 客户端以为自己没丢，做出错误判断
- 如果阻塞发送方 → 一个慢客户端拖垮全场
- 如果无限缓冲 → 内存无上限增长

`Lagged` 把"你要不要继续"的决定权交给你：
聊天室可以忽略（丢几条历史消息没关系），
行情推送必须处理（丢了价格就必须重新拉一次快照）。

==============================================
🔍 深入理解 3：为什么不用 HTTP/2 流代替 WebSocket
==============================================

HTTP/2 已经有双向流了，为什么还要 WebSocket？

- RFC 8441（"Bootstrapping WebSockets with HTTP/2"）就是干这个的，
  它让 WS 握手能跑在 HTTP/2 的流上。
- 但它**复用的仍然是 WebSocket 的帧协议**——只是握手搬到了 H2 上。
- 真正的"纯 H2 双向流"在浏览器里没有 API：fetch 不能拿到可读写流，
  JS 侧无法访问双向 body 流。

所以浏览器里要全双工，WebSocket 仍是唯一选择。
服务端之间的通信则完全可以用 HTTP/2 或 gRPC 双向流——
**那里没有浏览器 API 的限制**。

==============================================
🔍 深入理解 4：横向扩展时的广播
==============================================

单机 broadcast 只在**一个进程内**有效。
部署两台实例时，A 实例的用户发的消息，B 实例的用户收不到。

解法是在两个进程之间再架一层：
- Redis Pub/Sub（最简单，但 fire-and-forget，不保证送达）
- Redis Streams / Kafka（有回溯能力，可补发离线消息）
- NATS / 自建消息总线

架构变成：连接 → 本机 broadcast → 同时发布到消息总线 → 各实例订阅 → 各自本机 broadcast。

**注意这一点**：WebSocket 让"水平扩展"从"加机器就行"变成
"加机器 + 消息总线 + 会话粘滞（或共享会话状态）"。
这是选型时必须算进去的成本。
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

/*
4.1 在处理函数里直接读 stream，导致握手超时

❌ 错误代码：
```rust
async fn ws(req: HttpRequest, body: web::Payload) -> Result<HttpResponse, Error> {
    let (response, session, mut stream) = actix_ws::handle(&req, body)?;
    while let Some(Ok(msg)) = stream.recv().await {   // 永不结束
        println!("{:?}", msg);
    }
    Ok(response)                                       // 永远走不到
}
```

客户端现象：
    WebSocket connection to 'ws://127.0.0.1:8081/ws' failed:
    Error during WebSocket handshake: Unexpected response code: 200/500

原因：101 响应根本没发出去。客户端在等 101，服务端在等消息。

✅ 正确代码：
```rust
let (response, session, mut stream) = actix_ws::handle(&req, body)?;
actix_web::rt::spawn(async move {
    while let Some(Ok(msg)) = stream.recv().await { /* ... */ }
});
Ok(response)     // 立刻返回，握手完成
```

记忆技巧：**handler 的职责是把 connection 交出去，不是持有它。**

────────────────────────────────────────────────────────────

4.2 用 `?` 把 `handle()` 的错误直接抛出去

❌ 错误代码：
```rust
let (response, session, stream) = actix_ws::handle(&req, body)?;
```
`handle()` 返回的是 `Result<(HttpResponse, Session, MessageStream), Error>`，
用 `?` 本身没错。但如果你的函数返回类型是 `HttpResponse` 而不是
`Result<HttpResponse, Error>`，编译就会报错：

    error[E0277]: the `?` operator can only be used in a function
                  that returns `Result` or `Option`

✅ 正确代码：
```rust
async fn ws(req: HttpRequest, body: web::Payload) -> Result<HttpResponse, Error> {
    let (response, session, stream) = actix_ws::handle(&req, body)?;
    // ...
    Ok(response)
}
```

Rust 提示：`?` 的可用性由**函数返回类型**决定，不是由表达式决定。
看到 E0277 说 "the `?` operator can only be used..."，
第一反应应该是去看函数签名。

────────────────────────────────────────────────────────────

4.3 服务端给帧加了掩码

❌ 错误代码：
```rust
// 服务端代码里，自己拼帧并加了掩码
let frame = encode_frame(true, Opcode::Text, b"hi", Some([1,2,3,4]));
session.binary(frame.into()).await?;
```

客户端现象：
    WebSocket is closed before the connection is established
    （浏览器控制台，或者服务端日志里 1002 protocol error）

原因：RFC 6455 §5.1 规定服务端→客户端的帧 **MASK 位必须为 0**。
浏览器看到掩码位置 1，直接判定协议错误并关闭连接。

✅ 正确代码：
```rust
// 服务端只负责发载荷，帧由 actix-ws 组装
session.text("hi").await?;
// 若确实要手拼帧，掩码参数传 None
let frame = encode_frame(true, Opcode::Text, b"hi", None);
```

记忆技巧：**掩码是客户端的义务，服务端的禁令。**

────────────────────────────────────────────────────────────

4.4 忘记处理 Close 帧，连接泄漏

❌ 错误代码：
```rust
while let Some(Ok(msg)) = stream.recv().await {
    if let AggregatedMessage::Text(t) = msg {
        room.publish(t).await;
    }
    // Close 帧被忽略了
}
```

现象：客户端主动关闭后，服务端 `online` 计数不下降；
`Vec<Session>` 里积累着一堆已经断开的 session，广播越发越慢。

原因：`AggregatedMessage::Close` 是**客户端说"我要走了"**。
收到后必须回一个 close 帧并**结束循环**，否则你自己这边认为连接还活着。

✅ 正确代码：
```rust
while let Some(Ok(msg)) = stream.recv().await {
    match msg {
        AggregatedMessage::Close(reason) => {
            let _ = session.close(reason).await;
            break;                       // ← 关键：退出循环
        }
        AggregatedMessage::Text(t) => { room.publish(t).await; }
        _ => {}
    }
}
// 循环退出后兜底关闭
let _ = session.close(None).await;
```

Rust 提示：`while let` 里的 `break` 是退出长连接循环的正常手段。
不要试图用 `return` —— 那只会退出当前任务，清理代码照样要写在循环后面。

────────────────────────────────────────────────────────────

4.5 用 `Mutex<Vec<Sender>>` 遍历广播，持锁 await

❌ 错误代码：
```rust
let mut guard = sessions.lock().await;      // 拿锁
for s in guard.iter_mut() {
    s.send(msg.clone()).await?;             // 持锁 await，而且是串行
}
```

现象：一个卡住的客户端会让所有人的消息都延迟；
更糟的情况（同一把锁被重入）→ 死锁。

原因：`await` 期间锁不释放。`for` 循环让所有发送**串行化**。

✅ 正确代码（方案一，与官方示例同思路）：
```rust
let mut guard = sessions.lock().await;
let mut futs = FuturesUnordered::new();
for mut s in guard.drain(..) {              // 先拿出所有权，减少持锁逻辑
    futs.push(async move { (s.send(msg.clone()).await, s) });
}
// ...
```
方案二（本项目采用）：`broadcast::channel`，每个连接持有自己的 Receiver，
发送方只写 ring buffer 一次，不持有任何连接的锁：
```rust
let _ = room.publish(msg);                  // 不等任何人
```

记忆技巧：**跨 await 持锁 = 灾难的种子。** 参见第 27 天"锁守卫不能跨 await"。

────────────────────────────────────────────────────────────

4.6 只依赖 TCP keepalive，不做应用层心跳

❌ 错误代码：
```rust
// 什么都不做，指望 TCP 自己发现连接断了
```

现象：
- 客户端拔网线（不是正常关闭），服务端连接永远挂着 → 内存与在线数泄漏
- 中间代理（Nginx 默认 60s）静默关掉空闲连接，客户端和服务端都不知道

原因：TCP keepalive 的默认间隔是**小时级**，而且它可以被中间设备拦截。
应用层无法依赖它。

✅ 正确代码：
```rust
let mut ticker = actix_web::rt::time::interval(PING_INTERVAL);
loop {
    ticker.tick().await;
    if session.ping(b"hb").await.is_err() { break; }
    if alive.elapsed() > IDLE_TIMEOUT { break; }   // 超时判定已死
}
```

⚠️ IDLE_TIMEOUT 必须 **大于 PING_INTERVAL 的两倍**，否则丢一个 ping 就误杀。

────────────────────────────────────────────────────────────

4.7 忽略 `Lagged`，或者对它 panic

❌ 错误代码：
```rust
while let Ok(msg) = rx.recv().await {     // Lagged 是 Err，直接退出循环
    session.text(msg).await?;
}
```
现象：一个慢客户端被判定 Lagged 之后，它的连接**直接断开**——
而它其实只是网络抖了一下。

```rust
let msg = rx.recv().await.unwrap();       // 更糟：Lagged 直接 panic
```
现象：任务 panic，连接消失，日志里一个看不懂的 panic 信息。

✅ 正确代码：
```rust
loop {
    match rx.recv().await {
        Ok(msg) => { if session.text(msg).await.is_err() { break; } }
        Err(broadcast::error::RecvError::Lagged(n)) => {
            // 明确知道丢了 n 条：可以记日志、可以给客户端发"请重新同步"
            let _ = session.text(format!("(已跳过 {} 条消息)", n)).await;
        }
        Err(broadcast::error::RecvError::Closed) => break,
    }
}
```

记忆技巧：**`Lagged` 是信息，不是故障。**

────────────────────────────────────────────────────────────

4.8 在 `main` 里用 `#[actix_web::main]` 的同时又建 tokio runtime

❌ 错误代码：
```rust
#[actix_web::main]
async fn main() {
    let rt = tokio::runtime::Runtime::new().unwrap();   // 运行时里再建运行时
    rt.block_on(async { ... });
}
```

运行时错误：
    thread 'main' panicked at 'Cannot start a runtime from within a runtime'

原因：`#[actix_web::main]` 已经建好了 single-thread runtime。
`block_on` 不能在 async 上下文里调用。

✅ 正确代码：
```rust
#[actix_web::main]
async fn main() -> std::io::Result<()> {
    // 直接用 .await，不要再建 runtime
    HttpServer::new(|| App::new()).bind(("127.0.0.1", 8081))?.run().await
}
```

Rust 提示：这个 panic 是**运行时**才发现的，编译器无法帮你。
async 相关的坑大多属于这一类——类型对了，语义错了。
*/

// ============================================
// 第五部分：函数/宏卡片速查
// ============================================

/*
【函数 1：actix_ws::handle()】

语法：
```rust
pub fn handle(
    req: &HttpRequest,
    body: web::Payload,
) -> Result<(HttpResponse, Session, MessageStream), Error>
```

参数：
  - req: &HttpRequest —— 用来读取握手头（Upgrade / Sec-WebSocket-Key / 协议协商）
  - body: web::Payload —— 请求体流，握手后变成帧的来源

返回（三件套）：
  - HttpResponse  : 已经填好 101 与所有必需头的响应，**必须由你返回**
  - Session       : 写端，Clone，text/binary/ping/pong/close
  - MessageStream : 读端，recv()/max_frame_size()/aggregate_continuations()

错误：
  - 请求不是合法的 WebSocket 升级请求时返回 Err

────────────────────────────────────

【方法 1：Session::text()】

语法：
```rust
pub async fn text(&mut self, msg: impl Into<ByteString>) -> Result<(), Closed>
```

参数：
  - msg: 任意能转成 ByteString 的类型（&str、String 都可以）
返回：
  - Ok(()) 已写入发送缓冲
  - Err(Closed) 连接已关闭
所有权：
  - 借用 &mut self；内部把 msg 转成 ByteString（可能分配）

────────────────────────────────────

【方法 2：Session::ping() / pong()】

语法：
```rust
pub async fn ping(&mut self, msg: &[u8]) -> Result<(), Closed>
pub async fn pong(&mut self, msg: &[u8]) -> Result<(), Closed>
```

参数：
  - msg: 随心跳携带的字节（≤ 125 字节）。对端会在 pong 里原样带回
返回：
  - Ok(()) / Err(Closed)
要点：
  - 协议层的心跳，浏览器**自动**回 pong
  - 因此客户端不需要手写 pong 逻辑

────────────────────────────────────

【方法 3：Session::close()】

语法：
```rust
pub async fn close(mut self, reason: Option<CloseReason>) -> Result<(), Closed>
```

参数：
  - reason: 关闭码 + 说明文本。None 表示"正常关闭，无说明"
返回：
  - Ok(()) 关闭帧已发送
所有权：
  - **消费 self**。调用之后这个 Session 就没了，编译期防止你继续写
常用关闭码：
  - CloseCode::Normal (1000)        正常结束
  - CloseCode::Away (1001)          服务端主动下线 / 心跳超时
  - CloseCode::Policy (1008)        违反策略（如消息太大、未鉴权）

────────────────────────────────────

【方法 4：MessageStream::recv()】

语法：
```rust
pub async fn recv(&mut self) -> Option<Result<Message, ProtocolError>>
```

返回：
  - Some(Ok(msg))  一个帧
  - Some(Err(e))   协议错误 / IO 错误
  - None           流结束（对端关闭）
要点：
  - 用 `?` 不能直接解，因为它套了三层
  - 常见写法：`while let Some(msg) = stream.recv().await { match msg { Ok(m) => ..., Err(e) => break } }`

────────────────────────────────────

【方法 5：MessageStream::max_frame_size() / aggregate_continuations()】

语法：
```rust
pub fn max_frame_size(self, max_size: usize) -> Self
pub fn aggregate_continuations(self) -> AggregatedMessageStream
```

参数：
  - max_size: 单帧最大字节数。超过直接报错并断开
返回：
  - 新的流（**消费 self**，链式调用）
要点：
  - 默认上限很小（64 KiB），大消息场景必须显式放大
  - aggregate_continuations 之后拿到的是 AggregatedMessage，
    分片已被拼好，不需要自己处理 Continuation

────────────────────────────────────

【函数 2：tokio::sync::broadcast::channel()】

语法：
```rust
pub fn channel<T: Clone>(capacity: usize) -> (Sender<T>, Receiver<T>)
```

参数：
  - capacity: ring buffer 容量，即"单个订阅者最多落后多少条"
返回：
  - (Sender, Receiver)。Sender 可 Clone 且指向同一频道
  - Sender::subscribe() 可随时新增订阅者（收不到历史消息）
  - Receiver::recv() 返回 `Result<T, RecvError>`
    - RecvError::Lagged(n) 落后了 n 条，缓冲区已覆盖
    - RecvError::Closed   所有 Sender 已 Drop

────────────────────────────────────

【函数 3：accept_key()】本项目 protocol.rs

语法：
```rust
pub fn accept_key(client_key: &str) -> String
```

参数：
  - client_key: 请求头 `Sec-WebSocket-Key` 的值
返回：
  - 应回填到 `Sec-WebSocket-Accept` 的 Base64 字符串
算法：
  - Base64(SHA1(key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"))

────────────────────────────────────

【函数 4：encode_frame() / decode_frame()】本项目 protocol.rs

语法：
```rust
pub fn encode_frame(fin: bool, opcode: Opcode, payload: &[u8],
                    mask_key: Option<[u8;4]>) -> Vec<u8>
pub fn decode_frame(buf: &[u8], is_client_frame: bool)
    -> Result<(Frame, usize), FrameError>
```

要点：
  - encode：客户端传 mask_key = Some(...)，服务端传 None
  - decode：返回 (帧, 消耗字节数)；字节不够返回 FrameError::Incomplete
  - decode 在分配内存前先检查 MAX_PAYLOAD，防内存炸弹
*/

// ============================================
// 第六部分：练习题（不提供答案）
// ============================================

/*
练习 1（入门）：给握手补上完整校验
    要求：
    - 在 `ws_handler` 里手工检查三个头：
      `Upgrade: websocket`、`Connection` 含 `upgrade`、`Sec-WebSocket-Version: 13`
    - 缺任何一个返回 400 并给出明确原因
    提示：
    - `req.headers().get("Upgrade")` 返回 `Option<&HeaderValue>`，
      转字符串用 `to_str().ok()`
    - `Connection` 头的值可能是 "keep-alive, Upgrade"，要按逗号切分后**忽略大小写**比较
    注意：
    - actix-ws 的 `handle()` 已经做了这些校验。这题是为了让你看清它做了什么。

练习 2（入门）：把 echo 和广播分开
    要求：
    - `/ws/echo`：只把收到的消息回给发送者
    - `/ws/room`：广播给所有人（含发送者）
    提示：
    - echo 不需要 Room，直接用 session.text() 即可
    - 两个路由共用同一个 handler 会导致行为歧义，拆成两个函数更清晰

练习 3（进阶）：处理 Lagged
    要求：
    - 把 writer 任务的 `rx.recv()` 改成 match，Lagged 时给客户端发一条
      `{"type":"resync","skipped":n}` 而不是断开连接
    提示：
    - 首先要定义消息结构体，用 serde_json 序列化
    - 测试方法：把 capacity 调成 2，然后快速发 10 条，观察客户端收到什么
    注意：
    - 别让"发 resync 消息"本身再次触发错误——要检查 `text()` 的返回值

练习 4（进阶）：心跳超时踢人
    要求：
    - 在 writer 任务里加一个 interval，定期检查 `alive.elapsed()`
    - 超过 IDLE_TIMEOUT 就 `close(Some(CloseReason::from(CloseCode::Away)))`
    提示：
    - 现有代码只发了 ping，**没有检查 pong 回来没有**——这是本练习的核心
    - 找一个能模拟客户端不回 pong 的办法（比如发完 ping 立刻停掉客户端）
    注意：
    - `tokio::select!` 里两个分支都必须是**取消安全**的。
      `interval.tick()` 与 `rx.recv()` 都满足，可以放心用

练习 5（进阶）：二进制帧传文件分片
    要求：
    - 客户端发一个 200 KiB 的二进制帧（超过默认 max_frame_size）
    - 服务端调大 max_frame_size 并正确接收，返回长度
    提示：
    - `stream.max_frame_size(1024 * 1024)`
    - 分片（FIN=0 的后续帧）需要 `aggregate_continuations()`
    注意：
    - 调大 max_frame_size 就是允许更大的内存分配，
      必须同时配合超时和速率限制，否则又是一个 DoS 入口

练习 6（综合，为第 40 天的 realtime-chat 铺路）：
    "带用户名的聊天室"
    要求：
    - 客户端连上后先发 `{"type":"join","name":"alice"}`
    - 之后发的 `{"type":"text","text":"..."}` 都带上用户名广播
    - 有人加入/离开时，向所有人广播系统消息
    - 广播内容里带当前在线人数
    提示：
    - 连接级状态（用户名）要放在**连接自己的任务里**，
      不能放进 Room（Room 是全局共享的）
    - "加入/离开"消息要发在正确的位置：
      加入在订阅之后，离开在 OnlineGuard drop 之前或之后都可以，
      但要保证顺序看起来合理
    注意：
    - 用户名需要校验（长度、字符），否则有人在名字里放换行符/控制字符
    - 在 WebSocket 里做鉴权，通常用 URL 上的 token 或首条消息，
      不要相信客户端第一条消息里的 "name" 是唯一的

练习 7（思考题，不用写代码）：
    问：为什么 `Session::close()` 要消费 `self`，而其他方法只借用？
    提示：想想"close 之后再用它写"会发生什么。
    答完之后再想：同样的设计在标准库里还有哪些？
*/

// ============================================
// 服务端实现
// ============================================

/// 全局连接计数器，用来给每个连接分配一个显示名
static NEXT_CONN: AtomicUsize = AtomicUsize::new(1);

/// 客户端发来的消息
#[derive(Debug, Serialize, Deserialize)]
struct ClientMsg {
    /// 消息正文
    text: String,
}

/// 服务端广播的消息
#[derive(Debug, Serialize)]
struct ServerMsg<'a> {
    /// 消息类型：message / system
    #[serde(rename = "type")]
    kind: &'a str,
    /// 发送者名字
    from: &'a str,
    /// 正文
    text: &'a str,
    /// 当前在线人数
    online: usize,
}

/// 打开聊天室页面
async fn index() -> impl Responder {
    HttpResponse::Ok()
        .content_type("text/html; charset=utf-8")
        .body(include_str!("../static/chat.html"))
}

/// WebSocket 握手入口
///
/// 【执行顺序，不能乱】
///   1. handle()          完成握手，拿到三件套
///   2. 放大帧上限 + 聚合分片
///   3. 进入房间（OnlineGuard，Drop 时自动 -1）
///   4. 订阅广播
///   5. spawn 出读写任务
///   6. **立刻返回 response** ← 第 6 步晚了握手就超时
async fn ws_handler(
    req: HttpRequest,
    body: web::Payload,
    room: web::Data<Room>,
) -> Result<HttpResponse, Error> {
    // 1. 握手
    let (response, session, stream) = actix_ws::handle(&req, body)?;

    // 2. 单帧上限 128 KiB，并让 actix-ws 自动拼接分片
    let mut stream = stream
        .max_frame_size(128 * 1024)
        .aggregate_continuations();

    // 3. 进入房间。守卫在这里创建，随任务一起被 move 走，
    //    任务结束时自动 Drop → 在线数减一
    let room = room.get_ref().clone();
    let guard = OnlineGuard::enter(room.clone());

    // 4. 订阅。注意：订阅之前的消息收不到——新连接不该看到历史
    let mut rx = room.subscribe();

    // 心跳登记簿：读任务 touch，写任务检查。
    // 两个任务共享同一份，所以它内部必须是 Arc + Mutex（见 room.rs）
    let alive = Alive::new();

    let conn_no = NEXT_CONN.fetch_add(1, Ordering::Relaxed);
    let me = format!("用户{}", conn_no);

    // 广播"有人加入"
    let join_msg = serde_json::to_string(&ServerMsg {
        kind: "system",
        from: "系统",
        text: &format!("{} 加入了聊天室", me),
        online: room.online(),
    })
    .unwrap_or_default();
    room.publish(join_msg);

    // ---- 5a. 读任务：收客户端消息 → 发进房间 ----
    //
    // 它持有 stream（读端）和一个 session 克隆（只为回 pong / 关闭）
    let mut read_session = session.clone();
    let read_room = room.clone();
    let read_me = me.clone();
    let read_alive = alive.clone();
    actix_web::rt::spawn(async move {
        // 守卫随读任务存活；读任务结束 → 连接被认为已关闭
        let guard = guard;

        // 客户端主动关闭时带来的原因，留到循环之后统一 close 时使用
        // （close() 消费 self，所以只能在循环**外面**调用一次）
        let mut close_reason: Option<CloseReason> = None;

        while let Some(item) = stream.recv().await {
            // 收到任何数据（包括 pong）都算"还活着"
            read_alive.touch();

            match item {
                Ok(AggregatedMessage::Text(text)) => {
                    // 客户端发纯文本；解析失败就当成纯文本处理（宽容）
                    let payload = match serde_json::from_str::<ClientMsg>(&text) {
                        Ok(m) => m.text,
                        Err(_) => text.to_string(),
                    };

                    // 超过 2000 字的直接截断，防止有人拿长消息刷屏
                    let payload = if payload.chars().count() > 2000 {
                        payload.chars().take(2000).collect()
                    } else {
                        payload
                    };

                    let out = serde_json::to_string(&ServerMsg {
                        kind: "message",
                        from: &read_me,
                        text: &payload,
                        online: read_room.online(),
                    })
                    .unwrap_or_default();

                    // 返回值是"有几个订阅者收到了"，0 表示没人在听
                    read_room.publish(out);
                }
                Ok(AggregatedMessage::Ping(bytes)) => {
                    // 协议级心跳，必须原样回 pong
                    if read_session.pong(&bytes).await.is_err() {
                        break;
                    }
                }
                Ok(AggregatedMessage::Close(reason)) => {
                    // 客户端说"我要走了"：记下原因，退出循环后统一关闭
                    println!("[{}] 收到 Close 帧 {:?}", read_me, reason);
                    close_reason = reason;
                    break;
                }
                Ok(_) => {}                  // Pong / Binary：这里不处理
                Err(e) => {
                    eprintln!("[{}] 读错误: {}", read_me, e);
                    break;
                }
            }
        }

        // 循环结束后统一关闭一次（close 消费 self，所以只能在这里调）
        let _ = read_session.close(close_reason).await;

        // 先退出房间再广播，这样"离开"消息里的在线数是减过之后的
        drop(guard);

        let leave_msg = serde_json::to_string(&ServerMsg {
            kind: "system",
            from: "系统",
            text: &format!("{} 离开了聊天室", read_me),
            online: read_room.online(),
        })
        .unwrap_or_default();
        read_room.publish(leave_msg);
    });

    // ---- 5b. 写任务：从 rx 取广播 → 写给本客户端；同时定时发 ping ----
    let mut write_session = session;
    // `me` 在下面还要用（最后那行 println），所以给写任务一份克隆
    let write_me = me.clone();
    actix_web::rt::spawn(async move {
        let mut ticker = actix_web::rt::time::interval(PING_INTERVAL);

        loop {
            tokio::select! {
                // 分支 1：有广播消息就发给本客户端
                msg = rx.recv() => {
                    match msg {
                        Ok(text) => {
                            if write_session.text(text).await.is_err() {
                                break;   // 连接已关闭
                            }
                        }
                        Err(tokio::sync::broadcast::error::RecvError::Lagged(n)) => {
                            // 慢客户端：明确告诉它丢了多少条，而不是静默或断开
                            let notice = serde_json::json!({
                                "type": "resync",
                                "skipped": n,
                            });
                            if write_session.text(notice.to_string()).await.is_err() {
                                break;
                            }
                        }
                        Err(tokio::sync::broadcast::error::RecvError::Closed) => break,
                    }
                }
                // 分支 2：定时心跳 + 超时判定
                _ = ticker.tick() => {
                    if write_session.ping(b"hb").await.is_err() {
                        break;
                    }
                    if alive.elapsed() > IDLE_TIMEOUT {
                        eprintln!("[{}] 心跳超时，关闭连接", write_me);
                        let _ = write_session
                            .close(Some(CloseReason::from(CloseCode::Away)))
                            .await;
                        break;
                    }
                }
            }
        }
    });

    // 6. 立刻返回 101，握手完成
    println!("[{}] 握手完成", me);
    Ok(response)
}

// ============================================
// 工具函数
// ============================================

/// 把字节切片转成空格分隔的十六进制串（调试用）
fn hex(bytes: &[u8]) -> String {
    bytes
        .iter()
        .map(|b| format!("{:02X}", b))
        .collect::<Vec<_>>()
        .join(" ")
}

// ============================================
// 无浏览器测试客户端（cargo run -- --client）
// ============================================
//
// 【为什么要自己写一个客户端】
// 浏览器能连，但你**看不到字节**。
// 这段代码把浏览器做的事全部摊开：
//   1. 建 TCP
//   2. 手写 HTTP 升级请求
//   3. 校验服务端回的 Sec-WebSocket-Accept
//   4. 发**带掩码**的文本帧（客户端义务）
//   5. 读**不带掩码**的帧、自动回 pong
//   6. 发关闭帧
//
// 全程只用 tokio::net::TcpStream + 本项目的 protocol.rs，没有任何 WS 库。
// 跑通它，说明你真正理解了协议；不跑通，说明还有地方是"背下来的"。

async fn client_mode() -> std::io::Result<()> {
    use std::io::ErrorKind;
    use tokio::io::{AsyncReadExt, AsyncWriteExt};

    let addr = "127.0.0.1:8081";
    let host = addr;

    println!("[client] 连接 {} ...", addr);
    let mut tcp = tokio::net::TcpStream::connect(addr).await?;

    // ---- 1. 手写升级请求 ----
    // 用 RFC 官方的示例 key，方便对照文档
    let key = "dGhlIHNhbXBsZSBub25jZQ==";
    let request = format!(
        "GET /ws HTTP/1.1\r\n\
         Host: {}\r\n\
         Upgrade: websocket\r\n\
         Connection: Upgrade\r\n\
         Sec-WebSocket-Key: {}\r\n\
         Sec-WebSocket-Version: 13\r\n\
         \r\n",
        host, key
    );
    tcp.write_all(request.as_bytes()).await?;
    println!("[client] 已发送升级请求");

    // ---- 2. 读响应头（读到 \r\n\r\n 为止）----
    let mut buf: Vec<u8> = Vec::new();
    let mut chunk = [0u8; 2048];
    let header_end = loop {
        let n = tcp.read(&mut chunk).await?;
        if n == 0 {
            return Err(std::io::Error::new(
                ErrorKind::UnexpectedEof,
                "握手尚未完成连接就被关闭",
            ));
        }
        buf.extend_from_slice(&chunk[..n]);
        if let Some(pos) = buf.windows(4).position(|w| w == b"\r\n\r\n") {
            break pos + 4;
        }
    };

    let headers = String::from_utf8_lossy(&buf[..header_end]).to_string();
    println!("[client] 收到响应头：");
    for line in headers.lines() {
        println!("    {}", line);
    }

    // ---- 3. 校验 Accept ----
    let expected = accept_key(key);
    let got = headers
        .lines()
        .find(|l| l.to_ascii_lowercase().starts_with("sec-websocket-accept:"))
        .map(|l| l["sec-websocket-accept:".len()..].trim().to_string());

    match got {
        Some(v) if v == expected => println!("[client] ✅ Accept 校验通过：{}", v),
        Some(v) => {
            println!("[client] ❌ Accept 不匹配：期望 {} 实际 {}", expected, v);
            return Ok(());
        }
        None => {
            println!("[client] ❌ 响应里没有 Sec-WebSocket-Accept，说明服务端没完成升级");
            return Ok(());
        }
    }

    // 握手响应头之后可能已经跟着一些帧字节（服务端的"加入"广播）
    let mut pending: Vec<u8> = buf[header_end..].to_vec();

    // ---- 4. 发一个带掩码的文本帧 ----
    let payload = serde_json::to_string(&ClientMsg {
        text: "你好，我是 raw client（手写握手的那位）".to_string(),
    })
    .unwrap();
    // 客户端**必须**加掩码。这里用固定键，真实实现必须用随机数
    let frame = encode_frame(true, Opcode::Text, payload.as_bytes(), Some([0x11, 0x22, 0x33, 0x44]));
    tcp.write_all(&frame).await?;
    println!("[client] 已发送文本帧（{} 字节，已掩码）", frame.len());

    // ---- 5. 读帧循环 ----
    // 不按"收够几条就退出"，而是给整个会话一个**总时限**——
    // 注意不能只靠"多久没数据"，因为服务端每 5 秒一个 ping，
    // 数据永远不会断，空闲超时永远不会触发。
    // 这是长连接客户端的一个真实陷阱：**空闲 ≠ 没收到数据**。
    let deadline = tokio::time::Instant::now() + std::time::Duration::from_secs(12);
    let mut texts = 0usize;
    let mut pings = 0usize;

    loop {
        // 先尝试从已缓冲的字节里解帧
        match decode_frame(&pending, false) {
            Ok((f, consumed)) => {
                pending.drain(..consumed);
                match f.opcode {
                    Opcode::Text => {
                        println!("[client] ← 文本帧：{}", String::from_utf8_lossy(&f.payload));
                        texts += 1;
                    }
                    Opcode::Ping => {
                        pings += 1;
                        println!("[client] ← ping，回 pong");
                        let pong = encode_frame(true, Opcode::Pong, &f.payload, Some([0x11, 0x22, 0x33, 0x44]));
                        tcp.write_all(&pong).await?;
                    }
                    Opcode::Pong => println!("[client] ← pong"),
                    Opcode::Close => {
                        println!("[client] ← 服务端要求关闭");
                        break;
                    }
                    _ => println!("[client] ← 其它帧 {:?}", f.opcode),
                }
                continue;
            }
            Err(FrameError::Incomplete) => { /* 继续读 */ }
            Err(e) => {
                println!("[client] ❌ 解帧失败：{:?}", e);
                break;
            }
        }

        // 需要更多字节：用"剩余总时间"做上限，避免挂死
        let remaining = deadline.saturating_duration_since(tokio::time::Instant::now());
        if remaining.is_zero() {
            println!("[client] 到达 12 秒会话时限，主动结束");
            break;
        }
        let n = match tokio::time::timeout(remaining, tcp.read(&mut chunk)).await {
            Ok(Ok(n)) => n,
            Ok(Err(e)) => {
                println!("[client] 读失败：{}", e);
                break;
            }
            Err(_) => {
                println!("[client] 到达 12 秒会话时限，主动结束");
                break;
            }
        };
        if n == 0 {
            println!("[client] 服务端关闭了连接");
            break;
        }
        pending.extend_from_slice(&chunk[..n]);
    }

    // ---- 6. 关闭握手 ----
    //
    // RFC 6455 §7.1.1：TCP 不能"发完 close 就立刻断"。
    // 发 Close → **等对端回 Close** → 再关 TCP。
    // 直接断 socket 会让对端读到 EOF 而不是关闭帧，日志里就会出现
    // "payload reached EOF before completing" 这类假错误。
    let mut close_payload = vec![0x03, 0xE8]; // 1000 大端
    close_payload.extend_from_slice("再见".as_bytes());
    let close = encode_frame(true, Opcode::Close, &close_payload, Some([0x11, 0x22, 0x33, 0x44]));
    let _ = tcp.write_all(&close).await;
    println!("[client] 已发送关闭帧");

    // 等服务端回 Close（最多 2 秒，超时就算了）
    match tokio::time::timeout(std::time::Duration::from_secs(2), tcp.read(&mut chunk)).await {
        Ok(Ok(n)) if n > 0 => println!("[client] 收到对端关闭响应（{} 字节），关闭握手完成", n),
        Ok(Ok(_)) => println!("[client] 对端直接关闭了 TCP"),
        Ok(Err(e)) => println!("[client] 关闭时读错误：{}", e),
        Err(_) => println!("[client] 2 秒内没等到关闭响应"),
    }
    let _ = tcp.shutdown().await;

    println!("[client] 本次共收到 {} 条文本、{} 次 ping", texts, pings);

    Ok(())
}

// ============================================
// 入口
// ============================================

#[actix_web::main]
async fn main() -> std::io::Result<()> {
    // 客户端模式：连本机 8081 的服务端，手写握手 + 手拼帧
    // 用法：先在另一个终端 `cargo run`，再 `cargo run -- --client`
    if std::env::args().any(|a| a == "--client") {
        return client_mode().await;
    }

    let demo_only = std::env::args().any(|a| a == "--demo-only");

    println!("============================================================");
    println!("第34天：WebSocket 实时通信");
    println!("============================================================");
    println!();
    println!("第一部分：核心概念");
    println!("============================================================");
    section_1_1_why_websocket();
    section_1_2_handshake();
    section_1_3_frames();
    section_1_4_actix_model();

    println!();
    println!("============================================================");
    println!("第二部分：基础操作实践");
    println!("============================================================");
    section_2_1_accept_key();
    section_2_2_frame_roundtrip();
    section_2_3_backoff_table();
    section_2_4_err_handling();

    println!();
    println!("============================================================");
    println!("演示结束（第三至第六部分见本文件源码注释）");
    println!("============================================================");

    if demo_only {
        println!("--demo-only 模式，不启动服务器。");
        return Ok(());
    }

    let room = Room::new("lobby", 64);
    let host = "127.0.0.1";
    let port = 8081;

    println!();
    println!("启动服务：http://{}:{}/", host, port);
    println!("  /       聊天室页面（浏览器打开）");
    println!("  /ws     WebSocket 端点");
    println!("按 Ctrl+C 停止");
    println!("============================================================");

    HttpServer::new(move || {
        App::new()
            .app_data(web::Data::new(room.clone()))
            .route("/", web::get().to(index))
            .route("/ws", web::get().to(ws_handler))
    })
    .bind((host, port))?
    .run()
    .await
}
