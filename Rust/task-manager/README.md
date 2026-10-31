# task-manager

第 37–39 天的综合项目：一个多用户的任务管理系统 API。
用 actix-web 4 + sqlx 0.8（SQLite）+ tokio，目标不是"能跑"，而是**边界清楚、可测、可解释**。

---

## 1. 当前状态（2026-11-03，第 39 天）

| 项 | 状态 | 依据 |
|---|---|---|
| 编译 | ✅ 零警告 | `cargo build --all-targets` 输出只有 `Finished` |
| 测试 | ✅ 20 条集成用例通过 | `cargo test` → `20 passed; 0 failed` |
| 真实服务 | ✅ 多 worker + 并发压测 | `scripts/load_test.sh`，见 §5.8 |
| 接口数 | 12 个（+1 个健康检查） | 见 §4 |
| 迁移 | ✅ 5 张表 + 种子数据 | `migrations/0001_init.sql` |
| 错误层 | ✅ 单一 JSON 形状，500 不泄露 SQL | `src/error.rs`，§5.7 |
| 可观测性 | ✅ request-id + 耗时分桶 | `src/mw.rs`，§5.10 |
| 实验 | ✅ 三个可复现实验 | `explain_plan` / `slow_bucket` / `hold_lock` |

**三条最值得记住的实测结论**（细节见 §5）：

1. 500 的响应体必须是 JSON 且**不含 SQL 原文**——把 `sqlx::Error` 的 `Display`
   直接给客户端，等于公开数据模型（§5.7）。
2. 并发压测**没有**出现 `SQLITE_BUSY`，但这不代表"并发没问题"：
   短事务撞锁概率太低。把 `busy_timeout` 设成 0、再人为占住写锁，
   立刻看到 500 `database is locked`（§5.9）。
3. `busy_timeout` 决定的不是"会不会失败"，而是"失败前等多久"：
   持锁 3 秒时，`0` → 1.3ms 内 500，`5000` → 3.045s 后 201（§5.9）。

---

## 2. 目录与边界

```
task-manager/
├── migrations/
│   └── 0001_init.sql     表结构即契约：先写 SQL，再写 Rust
├── src/
│   ├── lib.rs            对外契约 + 装配（main 与 tests 共用 app()）
│   ├── main.rs           进程入口：读配置 → 初始化 → 起服务
│   ├── db.rs             连接池 + 三个必须显式设置的 SQLite 开关
│   ├── state.rs          进程级共享状态（只有连接池）
│   ├── models.rs         数据形状（不认识 HTTP）
│   ├── handlers.rs       HTTP 翻译层（校验 / 鉴权 / SQL 绑定）
│   ├── error.rs          统一错误层：ApiError + ResponseError
│   └── mw.rs             中间件：logging（App 级）、auth（scope 级）
├── scripts/
│   └── load_test.sh      并发压测：状态码分布 + 耗时统计
├── examples/
│   ├── explain_plan.rs   执行计划实验：造 2000 行，打印 EXPLAIN QUERY PLAN
│   ├── slow_bucket.rs    日志分桶实验：<10ms / <1s / ≥1s 三档
│   └── hold_lock.rs      人为占住 SQLite 写锁，复现 SQLITE_BUSY
└── tests/
    └── api.rs            集成测试：真文件库 + 真实 App
```

边界自检方法：**改一个东西要动几个文件？**

- 换数据库 → 只动 `db.rs`
- 加字段 → `migrations` + `models.rs`
- 改路由 → 只动 `lib.rs::configure`
- 换认证方式 → 只动 `mw.rs::auth`

---

## 3. 快速开始

```bash
# 默认：sqlite:task-manager.db，监听 127.0.0.1:8090，4 个 worker
cargo run

# 换库 / 换端口 / 换 worker 数
DATABASE_URL="sqlite:/tmp/demo.db" BIND_ADDR="127.0.0.1:8091" WORKERS=4 cargo run
```

环境变量：

| 变量 | 默认 | 说明 |
|---|---|---|
| `DATABASE_URL` | `sqlite:task-manager.db` | 库文件路径 |
| `BIND_ADDR` | `127.0.0.1:8090` | 监听地址 |
| `WORKERS` | `4` | worker 数（actix 默认是 CPU 核数，这里写死便于复现压测） |
| `DB_BUSY_TIMEOUT_MS` | `5000` | **只为实验存在**：设 0 可复现 `SQLITE_BUSY`，生产别调小 |

迁移在启动时自动执行（`sqlx::migrate!` 把脚本编进二进制），首次运行会建表并写入种子数据。

演示用 token（写死在 `mw.rs`，仅用于教学）：

| token | 用户 | 种子数据 |
|---|---|---|
| `token-alice` | alice (id=1) | 1 个项目「学习 Rust」+ 3 条任务 |
| `token-bob` | bob (id=2) | 无 |

---

## 4. 接口

| 方法 | 路径 | 认证 | 说明 |
|---|---|---|---|
| GET | `/health` | ❌ | 真的查一次库（`SELECT 1`），进程活着 ≠ 服务可用 |
| GET | `/projects` | ✅ | 只返回**自己**的项目 |
| POST | `/projects` | ✅ | `owner_id` 取自 token，绝不取自 body |
| DELETE | `/projects/{id}` | ✅ | 删项目 → **级联**删任务与标签关联 |
| GET | `/projects/{id}/tasks` | ✅ | 支持 `?status=&priority=&limit=&offset=` |
| POST | `/projects/{id}/tasks` | ✅ | `project_id` 取自 URL，`status`/`priority` 走库默认值 |
| GET | `/tasks/{id}` | ✅ | 任务详情 + 标签列表 |
| PATCH | `/tasks/{id}` | ✅ | 改 `title` / `priority` / `assignee_id` / `due_at` |
| DELETE | `/tasks/{id}` | ✅ | 删任务，级联删 `task_tags` 关联 |
| PATCH | `/tasks/{id}/status` | ✅ | 状态流转，**唯一入口** |
| POST | `/tasks/{id}/tags` | ✅ | 贴标签（不存在则新建），幂等 |
| DELETE | `/tasks/{id}/tags/{name}` | ✅ | 撕标签：只删关联，不删标签本身 |

统一响应形状：

```json
{ "success": true,  "data": { } }
{ "success": false, "code": "duplicate_name", "message": "..." }
```

`code` 给程序判断（稳定），`message` 给人看（可变）。
每个响应都带 `x-request-id` 头（§5.10），报障时直接报这个数字。

错误码表（**所有**错误都是上面那个 JSON 形状）：

| HTTP | `code` | 触发条件 | 谁的问题 |
|---|---|---|---|
| 400 | `invalid_name` / `invalid_title` | 空或超长 | 客户端 |
| 400 | `invalid_priority` | 不在 1~4 | 客户端 |
| 400 | `invalid_status` | 不是 todo/doing/done | 客户端 |
| 400 | `invalid_tag` | 空或超长 | 客户端 |
| 401 | `unauthorized` | 缺 token 或 token 无效（中间件短路） | 客户端 |
| 404 | `not_found` | 不存在，**或不属于你**（不区分，防枚举） | 客户端 |
| 409 | `duplicate_name` | 同一 owner 下重名项目 | 客户端 |
| 409 | `illegal_transition` | `todo → done` 之类的非法跃迁 | 客户端 |
| 500 | `corrupted_data` | 库里出现了枚举外的状态值（有人绕过 API 写库） | 服务端 |
| 500 | `internal` | 数据库真的炸了（§5.7），**细节只在服务端日志里** | 服务端 |

---

## 4.1 `PATCH` 的三态语义

| 请求体 | 含义 | 结果 |
|---|---|---|
| `{}` | 什么都没给 | 一个字段都不改（但仍会刷新 `updated_at`） |
| `{"due_at": null}` | 显式清空 | `due_at` 变成 NULL |
| `{"due_at": "2026-11-30"}` | 给值 | 写入 |

serde 默认**做不到**前两行的区分：`Option<Option<T>>` 遇到 `null`
会解析成外层 `None`，与"字段缺失"完全一样。必须加
`#[serde(default, deserialize_with = "deserialize_some")]`。
不加的后果是"清空"功能永久失效，**而且不报错**。

---

## 4.2 状态机

```
todo ──▶ doing ──▶ done
  │        │         │
  └────────┴─────────┘  同值 → 200 幂等
```

`todo → done` 返回 **409 `illegal_transition`**。
状态只能通过 `PATCH /tasks/{id}/status` 改，
`PATCH /tasks/{id}` 不接受 `status` 字段——这样"必须先开始才能完成"
这条业务规则才有唯一入口可以保证。

---

## 5. 实测记录

以下输出来自 2026-11-01 ～ 2026-11-03 的真实运行（`cargo test` + 手工 `curl` + 压测脚本），
不是"应该如此"。§5.1–5.6 是第 37–38 天的记录，§5.7 起是第 39 天的。

### 5.1 测试

```
running 9 tests
test health_needs_no_token_and_reports_db ... ok
test projects_without_token_is_401 ... ok
test projects_are_scoped_to_owner ... ok
test create_project_returns_201_then_visible_in_list ... ok
test duplicate_project_name_is_409_per_owner ... ok
test create_task_then_list_roundtrip ... ok
test foreign_project_is_404_not_403 ... ok
test invalid_input_is_400 ... ok
test unknown_route_is_404 ... ok
test result: ok. 9 passed; 0 failed
```

跑完检查：`$TEMP/task-manager-test-*` 无残留；项目目录没有多出 `.db` 文件。

### 5.2 真实服务

```
GET /health                        → 200 {"success":true,"data":{"db":"ok","status":"ok"}}
GET /projects            (无token) → 401 {"code":"unauthorized",...}
GET /no-such-path                  → 404
GET /projects   (Bearer alice)     → 200 {"success":true,"data":[{"id":1,...,"name":"学习 Rust"}]}
```

服务端日志（**注意行序**）：

```
[LOG] GET /health → 200 OK (0 ms)
[AUTH] 拒绝 GET /projects
[LOG] GET /projects → 401 Unauthorized (0 ms)
[LOG] GET /no-such-path → 404 Not Found (0 ms)
[LOG] GET /projects → 200 OK (0 ms)
```

两件事同时被证实：

1. **404 进了日志** → 日志中间件确实在 App 级，覆盖 `default_service`。
2. **`[AUTH]` 在 `[LOG] ... 401` 之前** → 日志在外层、认证在内层。
   顺序反了的话，`[AUTH]` 那行会在 `[LOG]` 之后甚至完全不出现。

### 5.3 `created_at` 是 UTC，不是本地时间

种子数据的 `created_at` 落在 `2026-10-29 17:14`，而本机时间是
`2026-10-30 01:14`（GMT+8）。因为 SQLite 的 `datetime('now')` **返回 UTC**。
展示给用户前必须转换时区——这是最容易在"看起来正常"的情况下错 8 小时的地方。

### 5.4 第 38 天：17 条用例

```
running 17 tests ... test result: ok. 17 passed; 0 failed  (0.13s)
```

其中三条是新写的"证明型"用例，不是"能跑就行"型：

| 用例 | 它证明的东西 |
|---|---|
| `patch_due_at_is_three_state` | `null` 与"字段缺失"确实被区分开了 |
| `status_transition_illegal_is_409` | `todo → done` 被拒，**且拒绝后状态没变** |
| `delete_project_cascades_tasks` | `foreign_keys(true)` 真的生效（否则任务成孤儿行，还不报错） |

### 5.5 执行计划（`cargo run --example explain_plan`，2003 行）

```
WHERE project_id = ? ORDER BY priority, id
  → SEARCH tasks USING INDEX idx_tasks_project (project_id=?)
  → USE TEMP B-TREE FOR ORDER BY

WHERE project_id = ? AND status = ?
  → SEARCH tasks USING INDEX idx_tasks_project_status (project_id=? AND status=?)

WHERE project_id = ? AND (? IS NULL OR status = ?)
  → SEARCH tasks USING INDEX idx_tasks_project (project_id=?)   ← 只用上前缀

tasks t JOIN projects p ON p.id = t.project_id WHERE t.id = ? AND p.owner_id = ?
  → SEARCH t USING INTEGER PRIMARY KEY (rowid=?)
  → SEARCH p USING COVERING INDEX idx_projects_owner (owner_id=? AND rowid=?)
```

三条实测结论：

1. **复合索引 `(project_id, status)` 的第二列确实被用上了**——前提是
   `status` 是确定条件。
2. **`OR` 写法并没有"完全放弃索引"**（这是过度概括），
   它退化成只用 `project_id` 前缀，复合索引后半段白建了。
3. **`ORDER BY priority` 永远要临时 B-Tree**：没有任何索引以 `priority` 起始。
   想省掉这次排序就得建 `(project_id, priority)`，但那会多一份写放大——
   2000 行量级不值得，先记着。

### 5.6 一次"接口 500、别的接口都正常"的排查

第 38 天刚接完 `/tasks/*`，六个用例同时 500：

```
[LOG] PATCH /tasks/1 → 500 Internal Server Error (1 ms)
```

原因：归属校验改成了 `JOIN`，而列清单常量是 `id, project_id, ...`，
`tasks` 和 `projects` **都有 `id`** → SQLite 报 `ambiguous column name: id`。
**这是运行时错误**，编译期完全看不出来。

修法：给 JOIN 用的列清单单独一份带 `t.` 前缀的常量（`TASK_COLUMNS_ALIASED`）。
这类问题的表征是"一组接口同时 500、另一组正常"，看到这个形状先查 SQL 里的裸列名。

### 5.7 统一错误层：20 条用例，500 里没有 SQL

```
running 20 tests
test internal_error_is_json_and_hides_sql ... ok
test error_bodies_share_one_shape ... ok
test responses_carry_unique_request_id ... ok
test result: ok. 20 passed; 0 failed
```

三条新用例各钉住一件事：

| 用例 | 它证明的东西 |
|---|---|
| `internal_error_is_json_and_hides_sql` | 500 是 JSON，且 body 里没有 `no such table` / SQL 片段 |
| `error_bodies_share_one_shape` | 404 / 400 / 409 的 key 集合**完全一致** |
| `responses_carry_unique_request_id` | 每个响应都有 `x-request-id`，且两次请求不同 |

怎么可靠地造出一个真 500：把表删掉（`DROP TABLE tasks`），
再请求 `GET /projects/1/tasks`。错误由 sqlx 自己产生，不是模拟的。

实测响应体：

```json
{"success":false,"code":"internal","message":"服务器内部错误，请稍后重试或联系管理员"}
```

同一时刻服务端日志里留着真实原因（**客户端看不到**）：

```
[ERR] 内部错误: error returned from database: (code: 1) no such table: tasks
```

这就是统一错误层的全部要点：**日志给人看，响应体给客户端看，两者不是同一份文案。**

### 5.8 并发压测：4 worker，数字说话

服务端：`WORKERS=4 ./target/release/task-manager.exe`，客户端 `scripts/load_test.sh`。

| 场景 | 请求数 / 并发度 | 结果 | 最大耗时 |
|---|---|---|---|
| 单条 INSERT（建任务） | 200 / 20 | 200 × `201` | 0.004 s |
| 单条 INSERT（建项目） | 100 / 20 | 100 × `201` | 0.005 s |
| 事务写（贴标签，3 条语句 + COMMIT） | 300 / 50 | 300 × `200` | 0.012 s |

三种场景**都没有出现** `SQLITE_BUSY`，服务端日志里的 201/200 计数与客户端完全一致
（`grep -c 'POST /projects/1/tasks'` = 200）。

⚠️ **但"没撞上"和"撞不上"是两件事**——见 §5.9。

#### 压测里踩到的两个坑

1. **尺子坏了，不是服务坏了。** 第一版脚本让 20 个并发进程 `>>` 追加同一个文件，
   200 个请求只记到 105 行。服务端日志显示 200 个请求全部 201——
   丢的是**测量数据**。改成"每个请求写自己的文件，最后合并"。
2. **100 个请求全 400。** `MODE=projects` 的 `description` 里写了中文，
   Git Bash 的 curl 按本地代码页（GBK）发出去，服务端收到非法 UTF-8：
   `Json deserialize error: invalid unicode code point`。
   服务端没错——先怀疑请求本身是否合法。

### 5.9 `busy_timeout` 对照实验：它决定"等多久"，不是"会不会失败"

自然压测撞不出 `SQLITE_BUSY`（写事务只有几十微秒，撞锁概率太低），
所以用 `examples/hold_lock.rs` **人为占住写锁**：`BEGIN IMMEDIATE` + 睡 N 秒。

| 场景 | `busy_timeout` | 客户端看到 | 耗时 |
|---|---|---|---|
| 持锁 3 s 时写入 | `0` | `500` `{"code":"internal"}` | 0.0013 s |
| 持锁 3 s 时写入 | `5000` | `201 Created` | 3.045 s |

两个值得记住的细节：

1. `busy_timeout=0` 时，失败几乎是**立即**的（1.3 ms）——它不是"稍后重试"，是"根本不等"。
2. `busy_timeout=5000` 时请求**成功了**，但代价是 3 秒延迟。
   日志里这一行是 `(3045 ms, >=1s)`——这正是耗时分桶的价值：
   一个"成功但很慢"的请求，只有分桶才捞得出来。

```
[LOG] #2 POST /projects/1/tasks → 201 Created (3045 ms, >=1s)
```

结论：`busy_timeout` 是**缓解**，不是**解决**。并发再高一点，
5 秒也会被吃完——那时的正确做法是减少写事务、或换掉"单写者"这个约束本身。

### 5.10 request-id 与耗时分桶

`cargo run --example slow_bucket`：

```
[LOG] #1 GET /fast       → 200 OK (0 ms, <10ms)
← /fast 响应头 x-request-id = 1
[LOG] #2 GET /slow-200   → 200 OK (202 ms, <1s)
← /slow-200 响应头 x-request-id = 2
[LOG] #3 GET /slow-1100  → 200 OK (1102 ms, >=1s)
← /slow-1100 响应头 x-request-id = 3
```

三档分桶都是**跑出来**的，不是看代码"觉得对"。
（第 38 天的教训：不报错的地方最容易永远错着。）

`x-request-id` 由进程内 `AtomicU64` 分配。并发测试时日志里能看到
ID 跳跃（22 → 35 → 37），说明用例真在并行跑、计数器确实是全局的。

---

## 6. 设计决策（附踩坑记录）

### 6.1 为什么先写迁移 SQL，再写 Rust 结构体

表结构是最晚能改的东西：代码重构是分钟级，表结构改了是"数据要迁移"级别。
用 SQL 定，而不是先用结构体描述一遍再翻译——翻译过程中的每个"应该差不多"
都会在第一次跑迁移时变成错误。

### 6.2 三个必须显式设置的 SQLite 开关

`SqliteConnectOptions::from_str` 出来的是 SQLite **默认值**，其中三处不适合服务端：

| 开关 | 默认 | 本项目 | 不设置的后果 |
|---|---|---|---|
| `journal_mode` | DELETE | WAL | 读写互斥 |
| `foreign_keys` | **OFF** | ON | 外键写了等于没写（**静默失效**） |
| `busy_timeout` | 0 | 5s | 撞上写锁立刻 `SQLITE_BUSY` |

第二个最危险：不会报错，只会"约束不生效"。

### 6.3 认证用 `CurrentUser` 提取器，而不是"记得写检查"

handler 签名里写 `user: CurrentUser`，取不到身份就直接 401。
**漏写检查**这种 bug 因此变成了**漏写参数**——编译期就报错。
`CurrentUser` 是 newtype：裸 `i64` 会和任何别的 `i64` 混淆，
`CurrentUser(7)` 只可能由认证中间件产生。

### 6.4 越权返回 404 而不是 403

403 等于确认"项目存在，只是不属于你"；项目 id 是自增的，据此可以枚举。
404 什么也不泄露。测试 `foreign_project_is_404_not_403` 把这条决定钉住了。

### 6.5 `UNIQUE(owner_id, name)` 而不是 `UNIQUE(name)`

同一个人不能有两个同名项目，不同人之间互不影响。
判重交给数据库：应用层"先查再插"在并发下必然有窗口。
识别冲突用 `DatabaseError::is_unique_violation()`，
**不要** `e.to_string().contains("UNIQUE")`——那依赖 SQLite 的文案。

### 6.6 `App` 的返回类型：两个编译器教我的坑

```rust
pub fn app(state: AppState) -> App<
    impl ServiceFactory<
        ServiceRequest,                     // ← 不是 Request
        Config = (), Response = ServiceResponse<BoxBody>,
        Error = Error, InitError = (),
    > + 'static,
>
```

- 写 `App<T, BoxBody>` → `error[E0107]: struct takes 1 generic argument but 2 were supplied`
  （`App<T, B>` 是旧版本的形状；4.x 只有 `App<T>`）
- 约束写 `ServiceFactory<Request>` → `error[E0277]: ...: ServiceFactory<actix_http::Request> is not satisfied`
  源码依据：`impl<T, B> IntoServiceFactory<AppInit<T, B>, Request> for App<T>
  where T: ServiceFactory<ServiceRequest, ...>` —— App 内层工厂处理的是
  已经进 App 的 `ServiceRequest`，不是 TCP 层刚解出来的 `Request`。

修好之后 `actix-http` 不再需要出现在 `Cargo.toml` 里（直接依赖只加
"自己代码真的写了名字"的 crate）。

### 6.7 测试用临时目录里的真文件库，而不是 `sqlite::memory:`

内存库确实全连接共享（第 36 天实测 `shared_cache`），能跑；
但它**不支持 WAL**，而生产用 WAL——测试等于放弃了整条生产路径的覆盖。
用真文件库还能顺带验证迁移能在干净库上跑通，以及"清理是否干净"。

清理写在 `Drop` 里而不是测试最后一行：panic 时最后一行不执行，
而 `Drop` 在 unwind 中仍会运行。Windows 上 `remove_dir_all` 可能拿到
`os error 32`（WAL 的 `-shm` 映射），所以用 10 × 50ms 重试而不是猜一个固定 sleep。

失败用例仍可能留下目录：`cleanup()`（真正 `pool.close()`）在断言失败后不执行，
而 `Drop` 只能删文件、关不了异步池。目录名带 pid 与序号，便于定位是哪一次跑的。

### 6.8 三态更新用一条 UPDATE，不用"读-改-写"

```sql
SET title       = COALESCE(?, title),                              -- 没给 → 保持
    assignee_id = CASE WHEN ? = 1 THEN ? ELSE assignee_id END,     -- 出现过？写值(Null 也行)
    updated_at  = datetime('now')
```

"读出来 → 在 Rust 里改 → 整行写回"有**丢失更新**：
两个并发 PATCH 各改不同字段，后写的用自己的旧快照覆盖前一个的结果。
一条 UPDATE 只写被指定的列，并发改不同字段互不影响。

### 6.9 状态流转单独开一个入口

`PATCH /tasks/{id}` 不接受 `status`；只能走 `PATCH /tasks/{id}/status`。
有唯一入口，"必须先开始才能完成"才是可保证的规则，而不是靠调用方自觉。
未知状态值 → 400（客户端写错），非法跃迁 → 409（形状对、语义错）。

### 6.10 级联删除的前提是 `foreign_keys(true)`

`ON DELETE CASCADE` 写在迁移里，但 SQLite **默认关闭外键**。
所以 `delete_project_cascades_tasks` 这条用例真正验证的是 `db.rs` 里那个开关——
少了它，删项目不删任务，**一行错误都不会报**。

### 6.11 多对多：删的是边，不是点

`DELETE /tasks/{id}/tags/{name}` 只删 `task_tags` 里的关联行，
`tags` 里的标签保留——同一个标签可能挂在别的任务上。
贴标签用 `ON CONFLICT DO NOTHING`（标签名唯一 + 复合主键唯一），
不用"先查再插"：并发下必撞唯一约束。

### 6.12 错误也是类型的一部分

```rust
pub enum ApiError {
    NotFound(&'static str),
    BadRequest { code: &'static str, message: String },
    Conflict   { code: &'static str, message: String },
    Internal(sqlx::Error),          // 只有它装了下层错误
}
impl ResponseError for ApiError { /* status_code / error_response */ }
impl From<sqlx::Error> for ApiError { /* 让 `?` 能直接用 */ }
```

handler 统一返回 `Result<HttpResponse, ApiError>`，于是：

```rust
let rows = sqlx::query_as::<_, Task>(sql).fetch_all(pool).await?;   // ← 一行搞定
```

**为什么不是每个 handler 各写各的 `map_err`**：18 个出口，
漏一个就少一层保护，而且漏的那个不会报错——它只是"某天开始返回纯文本"。
把转换写进 `From`，等于把"不许泄漏"变成编译期约束。

`Internal` 是唯一装了下层错误的变体，也是唯一**不允许原样外传**的：
`Display` 带着 SQL 片段和表名，进日志；响应体给固定文案。

### 6.13 request-id 用进程内计数器，而不是 UUID

单进程内唯一就够用（本项目就是单进程多线程），计数器零依赖、可读、
还能看出"这是第几个请求"。UUID 需要额外依赖，且日志里难扫。
跨进程/跨服务时再换 UUID 或 trace id —— 换成 UUID 只需要改 `call` 里那一行。

中间件在把请求交给内层**之前**就写好 ID，所以内层（handler、别的中间件）
打日志时 extensions 里已经有它了。

---

## 7. 已知局限

1. 标签写入的**事务回滚**没有被真正触发过（构造不出"归属校验通过后 FK 失败"的请求），
   目前只验证了幂等
2. token 表写死在代码里，无过期、无刷新；生产需 JWT 或会话
3. `Authorization` 的 scheme 大小写敏感（RFC 7235 要求不敏感）
4. `actix_web::test` 绕过 TCP：测不到端口占用、超时、多 worker——
   这部分靠 `scripts/load_test.sh` 补，但它只是**脚本**，不在 `cargo test` 里
5. 并发压测没有真正的**性能**结论（无 TPS/延迟分位统计），只有"成功率 + 耗时极值"
6. 失败用例仍可能留下临时目录（`cleanup()` 不执行，`Drop` 关不了异步池）
7. 没有限流、没有请求体大小之外的防滥用措施
8. 单机 SQLite 的"单写者"上限没有绕开：`busy_timeout` 只是把冲突推迟

---

## 8. 运行

```bash
cargo test                          # 20 条集成用例，约 0.12s
cargo test -- --nocapture           # 想看 [LOG] / [AUTH] 的输出
cargo run --example explain_plan    # 造 2000 行，打印筛选查询的执行计划
cargo run --example slow_bucket     # 验证日志的 <10ms / <1s / >=1s 三档
cargo run --example hold_lock -- <库路径> 3   # 占住写锁 3 秒
cargo build --release && ./target/release/task-manager.exe   # 4 worker 起服务
BASE=http://127.0.0.1:8090 N=200 P=20 MODE=tasks bash scripts/load_test.sh
```

测试之间互不共享数据：每条用例一个独立临时目录 + 独立数据库。
压测的实验库放在 `target/loadrun/`（`target/` 不进版本库）。
