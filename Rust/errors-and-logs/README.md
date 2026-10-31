# errors-and-logs

第 45 天补做项：**错误处理工程化 + 可观测性 + 配置**。
对应原路线图第 3 周（错误处理）与第 5–6 周（并发/异步）之间，
一直没落地的那段「生产代码怎么写错误和日志」。

三个主题放在一个项目里，是因为它们在真实服务里**必然耦合**：
错误要能分层打印、日志要能带上下文、配置缺失要能在启动时就报清楚。

---

## 1. 当前状态（2026-11-09）

| 项 | 状态 | 依据 |
|---|---|---|
| 编译 | ✅ 零警告 | `cargo build --offline --all-targets` |
| 测试 | ✅ 9 条通过 | `cargo test --offline` → `9 passed; 0 failed` |
| 格式 / 静态检查 | ✅ | `cargo fmt --check` 与 `clippy -D warnings` 退出码 0 |
| 错误链 | ✅ 可逐层枚举 | `AppError::chain()`，`#[source]` 逐级展开 |
| 配置 | ✅ 可注入测试 | `Config::from_pairs(\|k\| ...)` |
| 日志 | ✅ span 带字段 | `#[instrument(skip_all, fields(task.id = id))]` |
| 依赖 | thiserror / anyhow / tracing / tracing-subscriber（均为本地缓存可用） | 离线构建 |

---

## 2. 目录结构

```
errors-and-logs/
└── src/
    ├── lib.rs      82 行  对外契约：find_task / load_or_default / startup_banner
    ├── error.rs   124 行  领域错误 AppError（thiserror）+ 状态码/错误码/错误链
    ├── config.rs  183 行  配置：from_env / from_pairs / from_file + anyhow 上下文
    └── main.rs     70 行  接线：tracing 初始化 + 三个场景演示
```

---

## 3. 两个库的分工（本篇最重要的一条判断）

| 问题 | 用 thiserror | 用 anyhow |
|---|---|---|
| 调用方是否需要**按变体分支**？ | 是 | 否 |
| 类型 | `enum AppError` | `anyhow::Error` |
| 特征 | `#[from]` / `#[source]` / `transparent` | `.context()` / `.with_context()` |
| 场景 | 服务边界、库的公开 API、要映射 HTTP 状态码 | 应用内部、启动流程、脚本 |

本项目同时用了两者，并且**在同一个函数里区分**：
`find_task` 返回 `AppError`（调用方要按 `NotFound`/`Invalid` 分支），
`load_or_default` 返回 `anyhow::Result`（调用方只关心"启动失败"）。

---

## 4. 实测输出（`cargo run --offline -q`）

```
INFO startup{host=127.0.0.1 port=8080 level=info}: 服务配置已加载
INFO startup{host=127.0.0.1 port=8080 level=info}: 生效配置 config=host=127.0.0.1 port=8080 log_level=info
INFO startup{host=127.0.0.1 port=8080 level=info}: close time.busy=52.5µs time.idle=6.80µs
INFO service.find_task{task.id=7}: close time.busy=700ns time.idle=1.20µs
INFO 命中 task=task-7
INFO service.find_task{task.id=0}: close time.busy=600ns time.idle=1.10µs
INFO 请求被拒绝 status=400 code="invalid_argument"
ERROR 错误链 layer=0 msg=参数不合法：id 必须为正整数，实际 0
INFO service.find_task{task.id=999}: close time.busy=700ns time.idle=1.00µs
INFO 请求被拒绝 status=500 code="internal"
ERROR 错误链 layer=0 msg=数据库操作失败：select task
ERROR 错误链 layer=1 msg=SQLite 错误（code=1）：no such row

=== anyhow 的两种打印 ===
{}  （只有最外层）：读取配置文件失败：不存在的配置文件.conf
{:#}（完整因果链）：读取配置文件失败：不存在的配置文件.conf: 系统找不到指定的文件。 (os error 2)
```

---

## 5. 三条实测结论

1. **加了 `#[instrument]` 却看不到任何输出，不是宏没生效**，
   而是没有告诉 subscriber 要打印 span 的生命周期。必须：
   ```rust
   .with_span_events(FmtSpan::CLOSE)
   ```
   否则 `close time.busy=...` 这行永远不会出现。
   这是本篇花时间最多的一次误判。

2. **`{}` 与 `{:#}` 在 anyhow 上的差别是"丢不丢因果"**：
   前者只有最外层（`读取配置文件失败：...conf`），
   后者把 `os error 2` 也带出来。
   日志里必须用 `{:#}` 或 `{:?}`，否则排查时永远缺最后一层。

3. **配置要能注入**。`Config::from_env()` 无法在测试里模拟"缺 key"，
   所以内部实现成 `from_pairs(get: impl Fn(&str) -> Option<String>)`，
   `from_env` 只是 `from_pairs(|k| std::env::var(k).ok())` 的一行包装。
   这样「缺 JWT_SECRET 时启动必须失败」才是一条可断言的测试，而不是注释。

---

## 6. 未验证 / 已知局限

- 没有接真实数据库：`DbInner` 是构造出来的错误，不来自 sqlx
- 日志只有 fmt 输出，未接 JSON 层（`tracing-subscriber` 的 `json` feature 未启用/未缓存）
- 未做日志采样与限流：`ERROR 错误链` 在真实高 QPS 下会刷屏
- 没有断言过 span 的**结构化查询**（如按 `task.id` 过滤），只断言了可见性
- `AppError::status()` 到 HTTP 的映射没有 handler 消费（本项目不带 web 框架）

---

## 7. 下一步（若继续）

1. 接 `actix-web` 的 `ResponseError`，把 `AppError` 映射成统一 JSON 错误体
2. 加 `#[instrument(err)]`：出错时自动把 `Err` 记进 span
3. 用 `tracing_subscriber::filter::EnvFilter` 让 `RUST_LOG=warn` 真能压掉 INFO
