# ws-basics —— WebSocket 实时通信（第 34 天）

> 学习目标：从 HTTP 的"请求-响应"模型，转到 WebSocket 的**全双工长连接**。
>
> 与 `database/sqlx-*` 一样，这一课需要真实运行的服务，所以做成 Cargo 项目，
> 而不是 `rust_basics/` 下可以直接 `rustc` 编译的单文件。

## 为什么不是一个单文件

WebSocket 必须有**两端**：手写握手 + 帧编解码 + 异步读写任务 + 浏览器页面。
`rustc 26_websocket.rs && ./26_websocket` 无法承载这些。
所以这一课拆成三层，每层都可独立验证：

```
src/protocol.rs   纯函数：Accept 计算、帧编解码、退避算法   → cargo test 可测
src/room.rs       广播房间：tokio broadcast + 在线计数守卫   → cargo test 可测
src/main.rs       教学材料（六部分）+ actix-ws 服务端 + 手写客户端
static/chat.html  浏览器测试页（不依赖任何前端框架）
```

## 运行

```bash
# 1. 只看教学演示（不启动服务）
cargo run -- --demo-only

# 2. 跑协议层单元测试（13 个）
cargo test

# 3. 启动服务（前台）
cargo run
#    → http://127.0.0.1:8081/          聊天室页面
#    → ws://127.0.0.1:8081/ws         WebSocket 端点

# 4. 无浏览器的协议级客户端（另开一个终端）
cargo run -- --client
#    手写 HTTP 升级请求 → 校验 Accept → 发掩码文本帧 → 回 pong → 关闭握手
```

## 已实测（2026-10-29）

**两个客户端并发时**（`--client` × 2）：

```
客户端 A：
  ← 用户3 加入了聊天室（online 1）
  ← 用户3：你好，我是 raw client
  ← 用户4 加入了聊天室（online 2）      ← 别人加入
  ← 用户4：你好，我是 raw client        ← 别人的消息
  本次共收到 4 条文本、3 次 ping

客户端 B：
  ← 用户4 加入了聊天室（online 2）
  ← ping / ping / ping
  ← 用户3 离开了聊天室（online 1）      ← 别人离开
  本次共收到 3 条文本、3 次 ping

服务端：
  [lobby] 连接进入，在线 1
  [用户3] 握手完成
  [lobby] 连接进入，在线 2
  [用户4] 握手完成
  [用户3] 收到 Close 帧 code: Normal, description: 再见
  [lobby] 连接离开，在线 1
  [用户4] 收到 Close 帧 code: Normal, description: 再见
  [lobby] 连接离开，在线 0
```

握手响应实测：

```
HTTP/1.1 101 Switching Protocols
connection: upgrade
upgrade: websocket
sec-websocket-accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=      ← 与 RFC 6455 向量一致
```

## 文件导读

| 文件 | 内容 |
|---|---|
| `src/main.rs` | 六部分教学材料（核心概念 / 基础实践 / 深入理解 / 7 个常见错误 / 函数卡片 / 7 道练习）+ 服务端 + 手写客户端 |
| `src/protocol.rs` | `accept_key`（SHA-1+Base64）、`encode_frame`/`decode_frame`（三种长度编码 + 掩码）、`backoff_ms` |
| `src/room.rs` | `Room`（broadcast）、`OnlineGuard`（RAII 计数）、`Alive`（心跳打点）、`Lagged` 语义 |
| `static/chat.html` | 浏览器测试页，含指数退避重连与 `resync` 提示 |

## 关键结论（值得背下来）

1. **握手是 HTTP，之后就不是了。** `101 Switching Protocols` 一发出，这条 TCP 就归 WebSocket 管。
2. **`Accept = Base64(SHA1(key + 固定GUID))`**。SHA-1 在这里不承担安全职责，只防"误升级"。
3. **客户端必须掩码，服务端必须不掩码。** 掩码是给中间代理看的，不是保密。
4. **handler 必须立刻返回 101**，长连接逻辑一律 `spawn` 出去，否则握手超时。
5. **读写是两个对象**（`Session` / `MessageStream`），所以同时读写不需要锁。
6. **心跳检查的是"有没有收到数据"，不是"有没有收到 pong"**——任何帧都能证明连接活着。
7. **`Lagged` 是信息不是故障**：它明确告诉你丢了 n 条，由你决定继续还是重同步。
8. **关闭要握手**：发 Close → 等对端 Close → 再关 TCP。直接断 socket 会让对端读到 EOF。
9. **空闲超时不能只看"没数据"**：服务端每 5 秒一个 ping，数据流永远不会真的断。

## 与后续课程的关系

- 第 35 天（Actix 中间件）：给 `/ws` 加鉴权中间件、日志中间件
- 第 40–41 天（`realtime-chat/` 综合项目）：在本项目基础上加用户名、房间、持久化消息
- 横向扩展（多实例广播）见 `src/main.rs` 第三部分「深入理解 4」
