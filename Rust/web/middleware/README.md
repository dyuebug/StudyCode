# middleware —— Actix-web 中间件（第 35 天）

> 学习目标：把日志、鉴权这类"横切关注点"从 handler 里抽出来，
> 做成可组合、不可绕过的一层。
>
> 与 `realtime/ws-basics` 一样，这一课需要**真实运行的服务**才能验证
> （401 会不会进日志、401 时 handler 有没有执行、中间件有没有吃掉 body），
> 所以做成 Cargo 项目，而不是 `rust_basics/` 下可直接 `rustc` 编译的单文件。

## 为什么不是一个单文件

中间件的三个关键结论都只能靠**运行时行为**验证：

1. `wrap` 的书写顺序 vs 实际执行顺序（写反了程序照样编译通过）
2. 短路后内层到底执行了没有
3. 中间件有没有消费掉请求体

`rustc 27_middleware.rs` 编译不出这些证据。所以拆成三层：

```
src/mw/logging.rs   日志中间件（Transform + Service 两层，逐行注释）
src/mw/auth.rs      鉴权中间件 + UserId 提取器（中间件写、提取器读）
src/main.rs         六部分教学材料 + 路由装配 + 启动入口
```

## 运行

```bash
cargo run
# → http://127.0.0.1:8082
```

另开终端：

```bash
curl -i http://127.0.0.1:8082/health
curl -i http://127.0.0.1:8082/api/todos                        # 401
curl -i -H "Authorization: Bearer secret-token" \
        http://127.0.0.1:8082/api/todos                        # 200
curl -i -H "Authorization: Bearer other-token" \
        http://127.0.0.1:8082/api/whoami                       # bob
curl -i -H "Authorization: Bearer secret-token" \
        http://127.0.0.1:8082/api/slow/300                     # 等 300ms
curl -i http://127.0.0.1:8082/debug/forgot-auth                # 漏挂鉴权的现场
```

## 已实测（2026-10-30）

### 1. 中间件链条与执行顺序

装配代码（书写顺序是**从内到外**）：

```rust
App::new()
    .wrap(Logging::with_prefix("[INNER]"))   // 先写 → 里层
    .wrap(Logging::with_prefix("[OUTER]"))   // 后写 → 外层
    .service(...)
    .service(
        web::scope("/api")
            .wrap(Logging::with_prefix("[API]"))  // 先写 → auth 里面
            .wrap(auth.clone())                    // 后写 → auth 外面
```

实测输出：

```text
[API]   GET /api/whoami → 200 OK (0 ms)      ← 最里层先打印
[INNER] GET /api/whoami → 200 OK (0 ms)
[OUTER] GET /api/whoami → 200 OK (0 ms)

[AUTH] 拒绝 GET /api/todos
[INNER] GET /api/todos → 401 Unauthorized (0 ms)   ← 没有 [API]
[OUTER] GET /api/todos → 401 Unauthorized (0 ms)

[INNER] GET /nope → 404 Not Found (0 ms)
[OUTER] GET /nope → 404 Not Found (0 ms)
```

三条结论，全部来自这次运行：

- **后注册的 wrap 在外层**。`[INNER]` 先打印，说明它在里层。
- **401 时 `[API]` 一行都没有** → 证明 `Auth` 短路在它外面，内层完全没执行。
- **404 也被日志记到了** → App 级中间件包住了 `default_service`。

### 2. 鉴权与身份传递

```text
$ curl -i http://127.0.0.1:8082/api/todos
HTTP/1.1 401 Unauthorized
www-authenticate: Bearer
content-type: application/json

{"error":"缺少或无效的 Authorization: Bearer <token>","success":false}

$ curl -H "Authorization: Bearer other-token" .../api/whoami
{"source":"由 Auth 中间件写入 extensions，再由 UserId 提取器读出","user":"bob"}
```

### 3. 中间件没有吃掉请求体

```text
$ curl -X POST -H "Authorization: Bearer secret-token" \
       -H "Content-Type: application/json" --data-binary @t.json .../api/todos

{"created_by":"alice","data":{"completed":false,"id":4,"title":"测试中间件不吃 body"},"success":true}
```

如果鉴权中间件错误地读了 payload 而没有放回，这里会变成
`Json deserialize error: EOF while parsing a value`。

### 4. 漏挂鉴权中间件会在第一次调用时报出来

```text
$ curl -i http://127.0.0.1:8082/debug/forgot-auth
HTTP/1.1 401 Unauthorized

服务端：
[AUTH] ⚠️ GET /debug/forgot-auth 没有 UserId —— 检查是否漏挂鉴权中间件
```

handler 签名里写了 `user: UserId`，但路由没经过 `Auth`。
提取器找不到 → 401 + 一条服务端警告。**不会静默放过。**

### 5. 两个踩过的坑

| 现象 | 原因 |
|---|---|
| `POST /api/todos → 405` | 写了两个 `web::resource("/todos")`，第一个匹配到就返回了 |
| `Authorization: bearer ...` → 401 | `strip_prefix("Bearer ")` 大小写敏感，而 RFC 7235 规定 scheme 不分大小写 |

## 文件导读

| 文件 | 内容 |
|---|---|
| `src/mw/logging.rs` | `Transform`/`Service` 两层、`forward_ready!`、`Pin<Box<dyn Future>>` 的原因 |
| `src/mw/auth.rs` | 鉴权短路、`extensions` 传身份、`EitherBody`、`FromRequest` 提取器 |
| `src/main.rs` | 六部分教学材料、路由装配、7 个常见错误（含真实编译器输出）、7 道练习 |
| `README.md` | 本文件：实测记录与结论 |

## 关键结论（值得背下来）

1. **后注册的 `wrap` 在外层**。写代码从内到外，请求从外到内。
2. **要监控一切，就把那个中间件写在最后**（否则看不到 401/404）。
3. **`Transform::new_transform` 每 worker 一次，`Service::call` 每请求一次**。
   把每个请求要做的事写进 `new_transform`，症状是"日志只打印一次"。
4. **短路就是不调用内层**，用 `req.into_response(...)` 直接造响应。
5. **响应体类型变了就要 `EitherBody`**，否则报 `E0271`。
6. **身份用 `extensions` 传，不要用 `AppState`**：前者随请求，后者是全局的，会被并发覆盖。
7. **`extensions_mut()` 的 `RefMut` 不能跨 `await`**——内部是 `RefCell`，跨了就是运行时 panic。
8. **中间件不该读 body**；真读了必须 `set_payload` 放回去。
9. **泛型参数默认都要 `'static`**，`HttpServer` 还要求闭包 `Send`（所以 `Rc` → `Arc`）。
   这两条的报错位置都在 `main.rs`，根因在中间件。

## 与后续课程的关系

- 第 36 天（自动化测试）：用 `actix_web::test` 给这些中间件补集成测试，
  届时会用到 `init_service` + `call_service`
- 第 37–39 天（`task-manager/` 综合项目）：把 `Auth` 换成真正的 JWT 校验
- 第 40–41 天（`realtime-chat/`）：给 WebSocket 握手加鉴权中间件
