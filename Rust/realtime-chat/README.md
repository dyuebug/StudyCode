# realtime-chat —— 实时聊天应用（第 40 天）

actix-web + actix-ws 的多人聊天室。同一端口上同时提供 HTTP 接口和 WebSocket，
用 HTTP 观测 WebSocket 的内部状态。

> 本文所有数字都是**实测**结果，采集方式在各节写明；
> 未经实测的内容会标注"未验证"。

---

## 1. 现状

| 项 | 状态 |
|---|---|
| 编译 | `cargo build --all-targets` 零警告 |
| 测试 | `cargo test` 10 passed（3 个单测 + 7 个集成测试，整套约 1 秒） |
| 握手 | 无法升级的请求 → 400；非法房间名 → 400 `invalid_room` |
| 广播 | 两个客户端实测"甲发乙收"，双方都收到自己的回显 |
| 在线人数 | `GET /rooms/{room}/members` 与 WebSocket 实际连接数一致 |
| 慢客户端 | "只连不读"的客户端在写入超时后被踢，其他人收到 leave |
| 部署 | 多阶段镜像 **123 MB**，容器内 `/health` 200、`HEALTHCHECK` healthy（见 `DEPLOY.md`） |
| 质量门禁 | 本地 fmt / clippy / test 三连全绿；CI workflow 已写但**未在云端实测**（见 `CI.md`） |

---

## 2. 目录结构

```
realtime-chat/
├── src/
│   ├── lib.rs      # app() / configure() / 路由 / 握手 handler
│   ├── main.rs     # 读配置、建共享状态、起服务
│   ├── room.rs     # Room（广播+在线数）与 Rooms（注册表）
│   └── chat.rs     # 单连接的读写循环：收 / 发 / 心跳 / 收尾
├── static/
│   └── chat.html   # 聊天页，用 include_str! 编进二进制
├── tests/
│   └── ws.rs       # 7 个集成测试：真的起服务器、真的连线
├── examples/
│   ├── two_clients.rs   # 固定剧本：甲发乙收，可当冒烟测试
│   ├── stay_online.rs   # 连上挂着，用来在"人还在"时查在线人数
│   ├── lazy_reader.rs   # 只连不读的客户端（慢客户端对照组）
│   └── flood.rs         # 往房间里灌消息
├── Dockerfile           # 多阶段构建：rust:1-slim 编译 → debian:bookworm-slim 运行
├── .dockerignore        # 排除 target/、tests/、examples/（构建上下文会整包发送）
├── DEPLOY.md            # 部署说明：环境变量、健康检查、优雅停机、构建排障
├── CI.md                # 质量门禁：本地实测表、0 用例陷阱、缓存键、明确没做的事
└── target/evidence/     # 本次实验的原始输出
```

> CI workflow 不在本目录：GitHub 只读**仓库根**的 `.github/workflows/`，
> 放在子项目里的同名文件会被静默忽略。本项目放在
> `StudyCode/.github/workflows/rust-realtime-chat.yml`，用 `paths` 只对
> `Rust/realtime-chat/**` 触发。

---

## 3. 环境变量

| 变量 | 默认 | 说明 |
|---|---|---|
| `BIND_ADDR` | `127.0.0.1:8090` | 监听地址 |
| `WORKERS` | `4` | worker 线程数。**注意**：注册表在 `HttpServer::new` 外面创建，worker 数量不影响房间共享 |
| `HEARTBEAT_MS` | `30000` | 心跳间隔（毫秒） |
| `SEND_TIMEOUT_MS` | `10000` | 单次写的超时（毫秒），超时即判定"对端不读"并踢掉 |

后两个是为可测性留的入口：集成测试里把写超时调到 300 毫秒，
同样的踢人逻辑 1 秒内就能验完，而不是傻等 10 秒。
生产环境一般不需要动。

---

## 4. 消息协议

### 4.1 入口

```
GET /ws/{room}     # 房间名 1~32 字符，只允许字母、数字、- 和 _
```

握手失败（缺 `Upgrade` 头）→ `400`；房间名非法 → `400` + `invalid_room`。

### 4.2 服务端 → 客户端（JSON 文本帧）

| type | 字段 | 何时发 |
|---|---|---|
| `join` | `member`, `count` | 有人进房。**自己进房也会收到自己那条** |
| `leave` | `member`, `count` | 有人离开（主动关、被踢、超时都算） |
| `msg` | `member`, `text` | 有人发了消息。**发送者自己也会收到** |
| `error` | `message` | 收到 binary 帧等不支持的输入 |
| `lagged` | `missed` | 广播缓冲撑爆，已丢 `missed` 条，随后断开 |

`count` 是"发这条消息那一刻"的在线数，不是"现在"。要最新的值请查 HTTP。

### 4.3 客户端 → 服务端

纯文本帧，内容原样作为 `msg.text` 广播出去（不做富文本、不做转义）。
binary 帧会被拒绝并回一条 `error`。

### 4.4 HTTP 观测接口

| 接口 | 返回 |
|---|---|
| `GET /health` | `{"status":"ok"}` |
| `GET /rooms` | `{"success":true,"data":[{"room":"lobby","count":2}, ...]}` |
| `GET /rooms/{room}/members` | `{"success":true,"data":{"room":"lobby","count":2}}`，房间不存在时 `count` 为 0 |

---

## 5. 实测记录

### 5.1 握手的失败路径

```
$ curl -o /dev/null -w '%{http_code}' http://127.0.0.1:8091/ws/lobby
400                                  # 没有 Upgrade: websocket，actix_ws::handle 直接拒绝

$ curl http://127.0.0.1:8091/ws/a.b
{"code":"invalid_room","message":"房间名需 1~32 个字符，只允许字母、数字、- 和 _","success":false}

$ curl http://127.0.0.1:8091/ws/aaaaaaaa...（33 个 a）
{"code":"invalid_room", ...}         # 长度上限确实生效
```

### 5.2 甲发乙收（`examples/two_clients.rs`）

```
[甲] 已连接
[甲] 收到 → {"count":1,"member":1,"type":"join"}
[乙] 已连接
[乙] 收到 → {"count":2,"member":2,"type":"join"}
[甲] 收到 → {"count":2,"member":2,"type":"join"}
[乙] 发送 → 你好，我是乙
[甲] 收到 → {"member":2,"text":"你好，我是乙","type":"msg"}
[乙] 收到（自己的回显）→ {"member":2,"text":"你好，我是乙","type":"msg"}
[甲] 发送 → 收到，我是甲
[乙] 收到 → {"member":1,"text":"收到，我是甲","type":"msg"}
[甲] 收到（自己的回显）→ {"member":1,"text":"收到，我是甲","type":"msg"}
[乙] 已断开
[甲] 收到 → {"count":1,"member":2,"type":"leave"}
```

三件事同时被验证：跨连接广播、发送者回显、断开后别人收到 `leave`。

### 5.3 在线人数（HTTP 观测）

两个 `stay_online` 客户端各挂 7 秒，第 3 秒去查：

```
{"data":{"count":2,"room":"lobby"},"success":true}
{"data":[{"count":0,"room":"demo"},{"count":2,"room":"lobby"}],"success":true}
—— 两个客户端都退出后 ——
{"data":{"count":0,"room":"lobby"},"success":true}
```

`count` 从 0 → 2 → 0，与"实际连着几条连接"同步。

### 5.4 只连不读的客户端会被踢

对照组 `lazy_reader`（握手成功后再也不读任何帧）+ 灌消息的 `flood`
（连灌 200 条 × 8 KB，并且自己边灌边读，否则它自己也会被踢）：

```
== 灌之前 == {"data":{"count":1,"room":"slow"},"success":true}
灌 200 条 × 约 8192 字节
收到状态事件 2 条：
  {"count":2,"member":2,"type":"join"}
  {"count":1,"member":1,"type":"leave"}      ← 不读的那个被踢了
== 灌之后 == {"data":{"count":0,"room":"slow"},"success":true}

服务端日志：
[WS] 成员 1 加入 slow，当前 1
[WS] 成员 2 加入 slow，当前 2
[WS] 成员 1 写超时 10s（对端不读），主动断开
[WS] 成员 1 离开 slow，剩余 1
[WS] 成员 2 离开 slow，剩余 0
```

**关键结论**：踢人的不是广播层的 `Lagged`，而是写入层的超时。
原因见 §6.4。

### 5.5 收尾的 close 也会被堵（一次修掉的 bug）

第一次跑 §5.4 时，日志顺序是错的：

```
[WS] 成员 1 写超时 10s（对端不读），主动断开
[WS] 成员 2 离开 slow，剩余 0     ← 2 先退出
[WS] 成员 1 离开 slow，剩余 0     ← 1 晚了 10 秒才打印
```

原因：收尾时 `session.close(None).await` 也是**一次写**，对端不读，它在同一个
有界队列上又卡满了一个 `SEND_TIMEOUT`。修法是记住"写通道已经堵死"这个事实
（`writer_blocked`），这种情况下直接放弃发 close。修完日志顺序恢复正常（§5.4）。

### 5.6 本机测到的一次异常

同一台机器连跑多轮实验后，新建连接报过：

```
Error: Io(Os { code: 10055, kind: Uncategorized,
       message: "由于系统缓冲区空间不足或队列已满，不能执行套接字上的操作。" })
```

这是 Windows 的 `WSAENOBUFS`，等几秒后重试即恢复。判断依据：同一时刻只跑了
客户端，服务端负载正常，且重试成功。**结论**：这是本机套接字资源紧张，不是
服务端逻辑问题；但如果高频出现，就该怀疑自己的连接有没有被及时关掉。

---

## 6. 设计要点

### 6.1 WebSocket 是"协议升级"，不是"另一个服务"

握手就是一个普通 HTTP 请求（`Upgrade: websocket` + `Connection: Upgrade` +
`Sec-WebSocket-Key`），服务端回 `101 Switching Protocols`，**之后**这条 TCP 上
跑的就是 WebSocket 帧。所以 `/ws/{room}` 和 `/rooms` 能共用同一个端口、同一个
worker 池。

### 6.2 `Rooms` 必须在 `HttpServer::new` 外面创建

`HttpServer::new` 的闭包**每个 worker 调用一次**。写成：

```rust
HttpServer::new(move || {
    let rooms = Rooms::new();   // ❌ 每个 worker 一份
    realtime_chat::app(rooms)
})
```

4 个 worker 就有 4 份注册表：A 连 worker1、B 连 worker2 永远看不到对方，
`/rooms/lobby/members` 永远只显示 1。而且**它不报错**——像两张不同的白板。
正确写法是闭包外面建一份，`clone`（`Arc`）进闭包。

### 6.3 广播用 `broadcast`，不是 `Arc<Mutex<Vec<Session>>>`

官方示例的 `Mutex<Vec<Session>>` 可行，但发包时持锁，所有连接排在同一个锁后面；
而且慢客户端没有背压信号。`tokio::sync::broadcast` 给每个订阅者独立 `Receiver`，
慢的那个自己落后，恢复时拿到 `RecvError::Lagged(n)` —— 明确告诉它丢了几条。

### 6.4 两道闸门：`Lagged` 管"读得慢"，写超时管"根本不读"

这是本次最重要的一个发现：

- **读得慢**（客户端在收，但跟不上）→ 缓冲区撑爆 → `rx.recv()` 返回
  `Lagged(n)` → 本项目策略是通知后断开。
- **根本不读**（客户端收了不取）→ 完全不同的路径：`Session` 内部是
  **容量 32 的 mpsc**，`session.text().await` 在队列满时挂起；而挂住的正是
  我们自己的连接任务，它**再也走不到 `rx.recv()`**，所以 `Lagged` 永远不会出现。
  不加超时的话，这个连接会像僵尸一样挂着，直到操作系统级 TCP 超时。

所以 `actix_ws::Session` 的 `text/ping/pong/close` 全部包了 `SEND_TIMEOUT`。

### 6.5 成员编号只增不减

每个 `Room` 有自己的 `AtomicU64`，只 `fetch_add` + 1，从不回收。同一个房间
第二次实验里成员编号从 2、3 开始，这是预期行为（见 §7.2）。

### 6.6 在线数只用 CAS 递减

`leave()` 用 `fetch_update(..checked_sub(1))`，多减一次就停在 0，不会回绕成
`18446744073709551615`。"重复 leave" 本身是 bug，但宁可让它卡在 0。

### 6.7 房间名校验只放在会**创建**房间的入口

`/ws/{room}` 校验并可能在 `Rooms` 里插入新 key；`/rooms/{room}/members`、`/rooms`
是纯读，不校验（查一个非法名字得到 `count: 0` 是合理的答案）。白名单校验放在
会产生副作用的地方，读接口保持简单。

---

## 7. 已知局限

1. **房间对象永不销毁**：`Rooms` 的 HashMap 只增不减，空房间 `count` 为 0 但仍占内存。
   长时间运行的公开服务需要"空房间超时回收"。
2. **成员编号不回收**：见 §6.5，编号会一直增长。
3. **无鉴权、无昵称**：任何人可进任何房间，身份只有一个数字。
4. **无历史消息**：进房看不到之前的对话，新成员只能看到自己进房之后的。
5. **单进程**：注册表在内存里，多实例部署时两个实例互不可见。
   要水平扩展得用 Redis pub/sub 或类似的外部广播。
6. **`SEND_TIMEOUT` 固定 10 秒**：网络很差的移动端可能被误杀，正确做法是
   根据 ping/pong 往返时间动态调整。
7. **房间名大小写敏感**：`Lobby` 和 `lobby` 是两个房间。
8. **无业务层消息大小限制**：单条消息多大由底层协议决定，本项目没有设阈值。

---

## 8. 测试

```bash
cargo test              # 全部：3 个单测 + 7 个集成测试
cargo test --test ws    # 只跑集成测试
```

| 用例 | 防的是什么回归 |
|---|---|
| `handshake_without_upgrade_header_is_rejected` | 普通 GET 被当成 WebSocket 放行 |
| `invalid_room_names_are_rejected` | 非法房间名混进注册表（注入日志、占内存） |
| `members_counts_live_connections` | 在线人数与实际连接数脱节；收尾漏减 |
| `message_is_broadcast_to_other_members` | 两个人连上了但不在同一个房间 |
| `leaving_broadcasts_leave_with_decremented_count` | 收尾顺序写反，`leave` 里 count 多算一个人 |
| `binary_frames_get_an_error_reply` | 不支持的帧被静默丢弃，客户端以为发出去了 |
| `non_reading_client_is_disconnected` | 僵尸连接一直挂着（见 §6.4） |

两点写法上的踩坑记录：

1. **每个用例起一个独立服务器、端口用 0**。共享服务器就共享房间，
   "在线人数从 0 变 1"这类断言会被别的用例污染。
2. **收尾必须 `stop(false)`**。`stop(true)` 是优雅停机，要等现有连接结束，
   最长 30 秒；这些用例结束时 WebSocket 大多还开着，
   于是每条都白等 30 秒——第一版整套跑 31 秒，一开始还误以为是灌的数据太多。

断言 HTTP 响应时**比较解析后的 JSON 而不是原始字符串**：
`serde_json::json!` 默认用 BTreeMap，键是按字母排序的，
写 `assert_eq!(body, r#"{"success":true,...}"#)` 必然失败。

---

## 9. 运行方式

```bash
# 服务端
cargo run
# 打开 http://127.0.0.1:8090/

# 命令行验证
cargo run --example two_clients -- ws://127.0.0.1:8090/ws/demo
cargo run --example stay_online -- ws://127.0.0.1:8090/ws/lobby 10
cargo run --example lazy_reader -- ws://127.0.0.1:8090/ws/slow 40   # 对照组
cargo run --example flood       -- ws://127.0.0.1:8090/ws/slow 200 8192 15
```
