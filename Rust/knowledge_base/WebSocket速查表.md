# WebSocket 速查表

> 对应材料：`realtime/ws-basics/`（第 34 天）、`realtime-chat/`（第 40–41 天）
> 本表结论全部来自这两个项目的实测，采集方式见各条注明。

---

## 1. 一句话

WebSocket = **用一次 HTTP 请求换取一条长连接**。
握手那一下还是 HTTP，之后就是双向的帧（frame），直到任意一方关闭。

---

## 2. 握手：一次伪装成 HTTP 的协议升级

```
客户端 → 服务端
GET /ws/lobby HTTP/1.1
Host: 127.0.0.1:8090
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==
Sec-WebSocket-Version: 13

服务端 → 客户端
HTTP/1.1 101 Switching Protocols
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=
```

三个必须记住的点：

| 点 | 说明 |
|---|---|
| 状态码 **101** | `Switching Protocols`。不是 200——200 意味着"协商失败，还当普通页面" |
| 缺 `Upgrade` 头 | 服务端应当**拒绝**，不能当成普通 GET 放行（本项目实测返回 400） |
| `Sec-WebSocket-Accept` | `base64(sha1(key + 固定 GUID))`，服务端回错就不算握手成功 |

> `actix-ws` 会替你算 `Accept`，但**不会**替你判断"这个请求到底是不是想升级"。
> 本项目实测：不带 `Upgrade` 头的 `GET /ws/lobby` 返回 400 —— 这条断言存在，
> 就是为了防止"非法请求被当成正常请求建了房间"。

---

## 3. 帧类型

| 帧 | 用途 | 必须回应吗 |
|---|---|---|
| `Text` | UTF-8 文本，本项目唯一的业务帧 | — |
| `Binary` | 二进制 | 本项目明确拒绝并回 `error`（实测用例 6） |
| `Ping` | 探活 | **必须回 Pong**，否则对端按超时断开 |
| `Pong` | 应答 | — |
| `Close` | 关闭握手 | 应回 Close 再断 |

写测试时的第一条纪律：**读到 `Ping`/`Pong` 要跳过**，否则断言会被无关帧打乱
（`tests/ws.rs` 的 `next_json()` 就是干这个的）。

---

## 4. actix-web + actix-ws 的最小骨架

```rust
use actix_web::{web, HttpRequest, HttpResponse};
use actix_ws::Message;
use futures_util::StreamExt;

pub async fn ws_handler(
    req: HttpRequest,
    body: web::Payload,
    path: web::Path<String>,
) -> actix_web::Result<HttpResponse> {
    let room = path.into_inner();
    if !valid_room(&room) {
        // 校验必须发生在建房间**之前**
        return Ok(HttpResponse::BadRequest().json(serde_json::json!({
            "success": false, "code": "invalid_room"
        })));
    }
    // 没有 Upgrade 头时会失败 → 400，而不是 101
    let (response, session, mut msg_stream) = actix_ws::handle(&req, body)?;

    actix_web::rt::spawn(async move {
        let mut session = session;               // 写半
        while let Some(Ok(msg)) = msg_stream.next().await {
            match msg {
                Message::Text(t) => { /* 广播 */ }
                Message::Ping(p) => { let _ = session.pong(&p).await; }
                _ => {}
            }
        }
    });

    Ok(response)                               // 101 Switching Protocols
}
```

---

## 5. 广播：`tokio::sync::broadcast`

```rust
let (tx, _rx) = tokio::sync::broadcast::channel::<Event>(32);

// 每个连接一个接收端
let mut rx = tx.subscribe();

loop {
    match rx.recv().await {
        Ok(evt) => { session.text(evt.to_json()).await?; }
        Err(RecvError::Lagged(n)) => {
            // 自己落后了 n 条：消费不过来，但连接本身没问题
            continue;
        }
        Err(RecvError::Closed) => break,
    }
}
```

关键参数：`32` 是**每个订阅者的队列槽数**。它决定了"慢到多慢才算慢"。

---

## 6. ⭐ 本项目最重要的实测结论：两道闸门缺一不可

**`Lagged` 抓不住"根本不读"的客户端。**

原因：`Lagged` 出现在 `rx.recv()` —— 也就是**应用层**。
但消息要先从 `Session` 内部一个 32 槽的 mpsc 走到 socket。
一个"连上就再也不读"的客户端不会在应用层落后，它卡在**更上游**：
`session.text()` 写不进去，队列满了，`rx.recv()` 根本轮不到。

实测（第 40 天）：只加 `Lagged` 处理时，僵尸连接一直挂着；
加了写超时之后，服务端日志出现
`写超时 10s（对端不读），主动断开`，其他成员收到 `leave`。

| 闸门 | 拦谁 | 在哪一层 |
|---|---|---|
| `Lagged(n)` | **读得慢**的客户端（应用层消费不过来） | `rx.recv()` |
| 写超时 | **根本不读**的客户端（内核缓冲 + mpsc 被填满） | `session.text()` |

写超时必须可注入，否则测试要等 10 秒：

```rust
#[derive(Clone, Copy)]
pub struct ChatConfig {
    pub heartbeat: Duration,      // 默认 30s
    pub send_timeout: Duration,   // 默认 10s，测试里设 300ms
}
```

---

## 7. 关闭与优雅停机

| 写法 | 行为 | 实测耗时 |
|---|---|---|
| `server_handle.stop(true)` | 优雅：等现有连接自己结束，最长 30 s | 整套测试 **31.05 s** |
| `server_handle.stop(false)` | 直接停 | 整套测试 **1.01 s** |

教训：测试里断言已经做完了，`stop(false)` 就够。
`stop(true)` 会让每条用例白等 30 秒——**测试慢到没人跑就等于没有测试**。

WebSocket 长连接还带来一个部署侧的坑：容器停机时连接**不会**发 `bye`，
客户端看到的是"异常断开"。要么实现关闭帧广播，要么客户端做自动重连。
本项目目前**没有** `bye`（已知局限）。

---

## 8. 踩过的坑

### 8.1 用 127.0.0.1 起服务然后从另一台机器连

`127.0.0.1` 只监听回环。容器里更是只有容器自己的 loopback。
服务端启动时应当对"绑定到 127.0.0.1"给出警告（`29_deployment.rs` 里就是这么做的）。

### 8.2 灌消息的客户端自己也只写不读

广播会把消息发给**房间里所有人，包括发送者**。
所以灌消息的客户端如果不读，它自己也会被判定为"不读的客户端"，
实验就变成"两个客户端互相踢对方"，什么也证明不了。
解法：`split()` 拆成读半和写半，读半丢进独立任务（`examples/flood.rs`）。

### 8.3 把 JSON 字符串直接比对

`serde_json` 的对象是**按字母排序**的（`BTreeMap`），
写 `assert_eq!(body, r#"{"success":true,"data":...}"#)` 必然失败。
一律解析成 `Value` 再比。

### 8.4 测试里用阻塞 TCP 访问自己的服务

测试跑在单线程的 actix `System` 上，直接阻塞的 `TcpStream::read` 会把服务端
也一起卡住，变成"自己等自己"的死锁。必须放进 `spawn_blocking`。

---

## 9. 检查清单（新写一个 WS 服务前）

- [ ] 握手失败（无 `Upgrade`）返回 **400**，不是 200
- [ ] 路径参数在**建房间之前**校验
- [ ] `Ping` 回 `Pong`
- [ ] 广播包含发送者（协议要写清楚，测试要断言）
- [ ] `Lagged` **和**写超时两道闸门都有
- [ ] 超时/心跳值可注入，测试能压缩到毫秒级
- [ ] 测试收尾用 `stop(false)`
- [ ] 知道停机时客户端会看到"异常断开"
