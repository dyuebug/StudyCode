# Rust 学习仓库 · 作品集总览

> 16 周路线的学习成果索引。想快速判断"会什么"，看第 2 节；
> 想知道"怎么学的"，看第 4 节；想直接跑起来，看第 3 节。
>
> 叙事版总结见 [`学习总结.md`](学习总结.md)（覆盖到第 26 天），
> 逐日记录见 [`daily_logs/`](daily_logs)，统计口径见
> [`knowledge_base/进度统计.md`](knowledge_base/进度统计.md)。

---

## 1. 现状

| 项 | 值 |
|---|---|
| 学习天数 | 45 天（有效 37 天；第 27–32 天、第 7–8 天未实际学习） |
| 时间跨度 | 2026-05-29 ～ 2026-11-09 |
| 学习时长 | 约 99–101 小时（**推定**，无逐项计时） |
| 学习文件 | `rust_basics/` 下 **26 个 .rs** |
| 完整项目 | **13 个** |
| 知识库 | 6 篇专题速查表 + `静默失败检查清单.md` + 进度统计 + 剩余课程规划 |
| 门禁状态 | 第 45 天新增 3 个项目 fmt / clippy / test 三连退出码 **0**（2026-11-09 实测）；`realtime-chat` 于 2026-11-08 复跑为 **0**；其余项目为各自完成日的结果，未在 2026-11-09 全量复跑 |

⚠️ **证据分级**：本仓库所有数字都标注了来源。标【实测】的是有运行输出的，
标【推定】的是估算值。没有标注的，一律当成不可信。

---

## 2. 能力清单（按"能交付什么"组织）

### 语言核心
所有权 / 借用 / 生命周期、结构体与枚举、错误处理（`Result` + `?`）、
泛型与 trait、智能指针（`Box` / `Rc` / `RefCell` / `Weak`）、闭包与迭代器、
并发（线程 / `mpsc` / `Mutex` / `Arc` / `RwLock` / 线程池）、
异步（`async` / `await` / tokio）、宏（`macro_rules!` + 三种过程宏）、
`unsafe` 与 FFI。
> 材料：`rust_basics/01`–`22`、`28`–`32`

### 工程能力
- **CLI 工具**：clap 子命令、配置文件、错误信息设计
- **HTTP 服务**：actix-web / axum 路由、中间件（洋葱模型、短路、提取器）
- **数据库**：SQLite + sqlx（迁移、连接池、事务、执行计划分析）
- **实时通信**：WebSocket 协议升级、广播、两道僵尸连接闸门
- **测试**：单元 + 集成（真实 TCP）、时间可注入、`Drop` 守卫清理
- **部署**：多阶段 Dockerfile、非 root、健康检查、优雅停机
- **质量门禁**：fmt / clippy / test / 数量断言 / 缓存键设计

### 判断力（这部分最容易被低估）
- "没输出"、"很快完成"、"退出码 0" 都不等于成功 —— 有一份专门的
  [`静默失败检查清单`](knowledge_base/静默失败检查清单.md)
- 相信实测而不是常识：`strip` 在 MSVC 上省了 **0 字节**，
  `stop(true)` 让测试从 1.01 s 变成 31.05 s

---

## 3. 怎么跑

环境：Windows 11 + Git Bash + rustc 1.92.0 + Docker Desktop。

```bash
# 单个学习文件（多数是独立可运行的演示）
cd rust_basics
rustc --edition 2021 30_ci_cd.rs -o 30_ci_cd.exe && ./30_ci_cd.exe

# 有 Cargo 配置的项目
cd realtime-chat
cargo run                       # 起服务，默认 127.0.0.1:8090
cargo test                      # 10 passed，约 1 秒
cargo fmt --all -- --check
cargo clippy --all-targets -- -D warnings
```

完整门禁（含数量断言）见 [`realtime-chat/CI.md`](realtime-chat/CI.md)。

---

## 4. 项目一览

| # | 项目 | 技术 | 亮点 | 证据 |
|---|---|---|---|---|
| 1 | `http_server/` | 纯 std（TcpListener + 线程池） | 零依赖实现 HTTP 协议与路由 | 约 170 行，多线程并发实测 |
| 2 | `minigrep/` | 迭代器 + 文件 I/O + serde | 完整 CLI，JSON 配置与搜索历史 | 单测覆盖，约 310 行 |
| 3 | `serde_demo/` | serde / serde_json | 序列化五层示例 | 约 300 行 |
| 4 | `weather-cli/` | clap + reqwest + serde | 真实 API（wttr.in），查询历史 | 约 250 行 |
| 5 | `translate-cli/` | 同上 | 模式复用，1 小时完成 | 比从零快 4–6 倍 |
| 6 | `translate-cli-async/` | tokio + 异步 reqwest | 顺序 vs 并发对照 | 【实测】单请求 17.8×、批量 4.98× |
| 7 | `async_demo/` | tokio | 8 个 async 示例 | 【实测】并发提升 3–6 倍 |
| 8 | `todo-api/` | actix-web + sqlx(SQLite) | 完整 CRUD、持久化从 JSON 迁到 SQLite、`lib.rs` + `configure()` 让集成测试与 `main` 共用装配 | 【实测】`tests/api.rs` 8 条全绿 |
| 9 | `task-manager/` | axum + sqlx | 5 表迁移、统一错误层、请求 ID + 耗时分桶、执行计划实验、并发压测 | 【实测】20 条集成测试；`busy_timeout` 0 vs 5000 对照（1.3 ms 500 / 3.045 s 201） |
| 10 | `realtime-chat/` | actix-web + actix-ws + Docker | 同端口 HTTP + WebSocket、广播、两道僵尸闸门、多阶段镜像 | 【实测】10 passed / 1.01 s；镜像 123 MB；容器 `/health` 200 |
| 11 | `proc-macros/` | syn / quote + workspace | `#[derive(Validate)]` 属性解析，报错 span 精确指向用户写的键 | 【实测】8 passed；未知键 caret 指向 `min_lenn`；proc-macro crate 导出 trait 会编译失败 |
| 12 | `errors-and-logs/` | thiserror / anyhow / tracing | 错误分层 + `chain()` 逐层展开、配置可注入、`instrument` 带字段 | 【实测】9 passed；`{}` vs `{:#}` 证明前者丢根因 |
| 13 | `file-manager/` | 纯 std（零依赖） | `symlink_metadata` 不跟随、错误累积、64 KiB 流式复制、手写子命令解析 | 【实测】16 passed；用法错误退出码 1（`\| head` 后变 0，`pipefail` 恢复 1）；junction 上验证链接不被展开（改回 `metadata` 后测试立刻变红） |

部署与门禁材料：`realtime-chat/{Dockerfile,DEPLOY.md,CI.md}` +
仓库根 `.github/workflows/rust-realtime-chat.yml`（**未在云端实测**）。

---

## 5. 学习路线覆盖情况

| 阶段 | 内容 | 状态 |
|---|---|---|
| 1 | Rust 基础（第 1–4 周） | ✅ |
| 2 | 并发 + 异步（第 5–6 周） | ✅ |
| 3 | 宏 / unsafe / FFI（第 7 周） | ✅ 第 28–30 天 |
| 4 | CLI / 网络 / 数据库（第 8–10 周） | ✅ |
| 5 | Web 开发（第 11–13 周） | ✅ 含 WebSocket 与中间件 |
| 6 | 综合项目（第 14–16 周） | ✅ `task-manager` / `realtime-chat` / 部署 / 质量门禁 |

未覆盖或仅有认知的项（诚实清单）：
- 矩阵构建、跨平台发布、musl 静态链接 —— **未实测**
- diesel（只用过 sqlx）
- 覆盖率工具、Miri、`cargo expand` —— **未实测**

---

## 6. 目录导航

| 想找什么 | 去哪 |
|---|---|
| 逐日学习材料（26 个 .rs，六部分结构） | `rust_basics/` |
| 每日任务与日志 | `daily_tasks/`、`daily_logs/` |
| 周报 | `weekly_reports/` |
| 专题速查表 | `knowledge_base/*速查表.md` |
| 静默失败这一类坑的统一清单 | `knowledge_base/静默失败检查清单.md` |
| 进度与统计口径 | `knowledge_base/进度统计.md` |
| 后续课程施工清单 | `knowledge_base/剩余课程规划.md` |
| 自动化行为约定 | `CLAUDE.md` |

---

## 7. 已知局限

| # | 局限 | 后果 |
|---|---|---|
| 1 | CI 从未在云端运行 | "CI 通过"目前只是配置写好，不是事实 |
| 2 | 学习时长为推定值 | 不能用来做效率对比 |
| 3 | 第 27–32 天为预生成模板 | 该区间的日志不是真实学习记录 |
| 4 | 第 38 天动手项 A–D 未执行 | 那部分结论来自文档而非亲手验证 |
| 5 | 所有项目都是单机、无鉴权、无多实例 | 是学习项目，不是生产系统 |
| 6 | `file-manager` 的链接保护只在 Windows junction 上验证过 | Unix 真 `symlink` 与 `a -> b -> a` 环未实测，两者对 `is_symlink` 的行为可能不同 |
| 7 | `errors-and-logs` 未接真实数据库 | `DbInner` 是构造出来的错误，不来自 sqlx |
| 8 | 内存序结论只在本机 x86_64 成立 | 材料 32 的"跑通了"不代表内存序正确，x86 测不出 Relaxed 的错 |
