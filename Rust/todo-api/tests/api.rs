// ============================================
// Todo API - 集成测试（第36天，2026-10-31）
// ============================================
//
// 【这一层测什么】
//
// 单元测试测"函数算得对不对"，集成测试测"接口对外承诺了什么"。
// 这里断言的全部是 HTTP 层面可观察的行为：
// 状态码、JSON 结构、字段值、404 与 405 的区别。
//
// **刻意不测的东西**：handler 内部怎么实现、SQL 长什么样、
// 用了 `fetch_optional` 还是 `fetch_one`。
// 一旦断言实现细节，重构时就得跟着改测试，而对外行为一个字都没变。
//
// 【为什么用 tests/ 而不是写在 src 里】
//
//   src 里的 `#[cfg(test)]` 单元测试     → 能测 private，属"白盒"
//   tests/ 下的集成测试（本文件）        → 只能测 pub，属"黑盒"
//
// 黑盒是刻意的限制：它强迫你从"调用者"的角度思考，
// 而不是从"我写了什么"的角度思考。这恰好是 API 的视角。
//
// 【为什么每个测试一个独立数据库】
//
// `cargo test` 默认**并行**跑测试。如果把 8 个测试都指向 `todos.db`：
//   - 测试 A 插入的行会被测试 B 的"列表应为空"断言看到 → 假失败
//   - 测试 C 的 `DELETE` 可能删掉测试 D 刚建的数据 → 随机失败
//   - 更糟：把开发环境的真实数据删了
//
// 本文件的方案：每个测试建一个**独立临时目录 + 独立 SQLite 文件**，
// 测试结束整目录删除。代价是每次要跑一遍迁移（毫秒级），
// 换来的是"任何测试都能单独、重复、并行地跑"。
//
// 为什么不用 `sqlite::memory:`？见文件末尾的【为什么不用内存库】。

use std::path::{Path, PathBuf};
use std::sync::atomic::{AtomicUsize, Ordering};
use std::time::Duration;

use actix_web::{test, web, App};
use serde_json::Value;
use sqlx::sqlite::SqlitePool;
use todo_api::{db, state::AppState};

// --------------------------------------------
// 测试环境：一个测试 = 一个临时数据库
// --------------------------------------------

/// 每个测试的独有编号，保证并行时不会两个测试建到同一个文件
///
/// 只用 `process::id()` 不够：同一个测试进程里的多个测试线程
/// 拿到的是同一个 pid。加上自增计数才是唯一的。
static SEQ: AtomicUsize = AtomicUsize::new(0);

/// 临时目录守卫：**drop 时删目录**
///
/// 这是本文件里最容易被忽略、但对"测试能否重复跑"最关键的一段代码。
///
/// 为什么挂在 `Drop` 上而不是在测试末尾写一句 `remove_dir_all`？
/// 因为测试**失败时会 panic**，panic 之后那句删除代码永远执行不到。
/// 于是每挂一次就在磁盘上留一个几 KB 的库，越积越多，
/// 最后"跑测试"变成一个会慢慢污染机器的事。
/// `Drop` 是 panic 展开（unwind）过程中一定会执行的，所以清理不会漏。
///
/// 为什么还要重试？（这一段是实测出来的）
/// 最初写的是"关池 → `remove_dir_all` 一次"，实测 8 个测试里有 2 个报：
///
/// ```text
/// 另一个程序正在使用此文件，进程无法访问。 (os error 32)
/// ```
///
/// `pool.close().await` 已经返回，但 SQLite 在 WAL 模式下的 `-shm`
/// 文件是内存映射，Windows 上解映射不是同步完成的。
/// 单次删除因此会和它赛跑，输赢取决于调度。
/// **解决办法不是加 sleep 猜时间，而是重试到成功或明确放弃。**
///
/// 生产项目通常会直接用 `tempfile::TempDir`（逻辑更完善）。
/// 这里手写是因为本项目不想为 25 行功能再拉一个依赖树。
struct TempDirGuard(PathBuf);

impl TempDirGuard {
    fn new(prefix: &str) -> std::io::Result<Self> {
        let dir = std::env::temp_dir().join(prefix);
        std::fs::create_dir_all(&dir)?;
        Ok(Self(dir))
    }

    fn path(&self) -> &Path {
        &self.0
    }
}

impl Drop for TempDirGuard {
    fn drop(&mut self) {
        // 10 次 × 50ms = 最多等半秒，够 Windows 释放映射了
        for _ in 0..10 {
            if std::fs::remove_dir_all(&self.0).is_ok() {
                return;
            }
            std::thread::sleep(Duration::from_millis(50));
        }
        // 删不掉只是留垃圾，**绝不能让测试因此失败**
        eprintln!("⚠️ 临时目录未能删除（不影响测试结论）: {}", self.0.display());
    }
}

struct TestEnv {
    /// 字段名带下划线是刻意的：它从不被读，唯一作用是"活着"。
    /// 它一被 drop，目录就没了——这是 RAII 式的清理。
    _dir: TempDirGuard,
    pool: SqlitePool,
}

impl TestEnv {
    /// 建一个空库（含迁移）的测试环境
    ///
    /// 这里调用的是**生产代码里的** `db::init_db`，不是另写一份建库逻辑。
    /// 否则测试跑的是"另一个库"，WAL 没开、外键没开这类问题永远测不出来。
    async fn new() -> Self {
        let n = SEQ.fetch_add(1, Ordering::Relaxed);
        let dir = TempDirGuard::new(&format!("todo-api-test-{}-{n}", std::process::id()))
            .expect("建临时目录失败");

        // Windows 的路径是反斜杠，SQLite 连接串里要用正斜杠，
        // 否则 `sqlite:C:\...` 会被当成不合法的 URL。
        let db_path = dir.path().join("test.db").display().to_string().replace('\\', "/");
        let url = format!("sqlite:{db_path}");

        let pool = db::init_db(&url).await.expect("初始化测试库失败");
        Self { _dir: dir, pool }
    }

    /// 把连接池包装成 handler 需要的 `web::Data<AppState>`
    fn state(&self) -> AppState {
        AppState::new(self.pool.clone())
    }

    /// 测试结束：只要关池，目录由 `TempDirGuard` 负责删
    ///
    /// 关池不能省：它让 SQLite 把 WAL 落盘、释放文件句柄。
    /// 少了这一步，删除会一直撞上 os error 32。
    async fn cleanup(self) {
        self.pool.close().await;
        // self 在这里被 drop → _dir 被 drop → 目录被删
    }
}

/// 用 `AppState` 构建一个可发请求的应用
///
/// 注意 `configure`：路由来自 `todo_api::configure`，
/// 和 `main.rs` 里跑的是**同一个函数**。测试与线上不会漂移。
macro_rules! app {
    ($state:expr) => {
        test::init_service(
            App::new()
                .app_data(web::Data::new($state))
                .configure(todo_api::configure),
        )
        .await
    };
}

// --------------------------------------------
// 请求构造的小工具
// --------------------------------------------

fn get_req(uri: &str) -> actix_http::Request {
    test::TestRequest::get().uri(uri).to_request()
}

fn post_json(uri: &str, body: &str) -> actix_http::Request {
    test::TestRequest::post()
        .uri(uri)
        .insert_header(("content-type", "application/json"))
        .set_payload(body.to_string())
        .to_request()
}

fn put_json(uri: &str, body: &str) -> actix_http::Request {
    test::TestRequest::put()
        .uri(uri)
        .insert_header(("content-type", "application/json"))
        .set_payload(body.to_string())
        .to_request()
}

fn delete_req(uri: &str) -> actix_http::Request {
    test::TestRequest::delete().uri(uri).to_request()
}

// ============================================
// 用例 1：健康检查
// ============================================

/// 健康检查必须真的查一次库
///
/// 断言的是契约：`200` 且 `data.db == "ok"`。
/// 如果哪天有人把实现改成"直接返回 200 不查库"，
/// 这条测试**依然会通过**——这是它的已知局限（见文末）。
///
/// 想让局限可测，就得在 handler 里注入一个可替换的探针，
/// 但对 demo 项目来说那是过度设计（YAGNI）。
#[actix_web::test]
async fn health_returns_200_and_reports_db_ok() {
    let env = TestEnv::new().await;
    let app = app!(env.state());

    let resp = test::call_service(&app, get_req("/health")).await;
    assert_eq!(resp.status(), 200, "健康检查应当返回 200");

    let body: Value = test::read_body_json(resp).await;
    assert_eq!(body["success"], true);
    assert_eq!(body["data"]["db"], "ok");

    env.cleanup().await;
}

// ============================================
// 用例 2：创建
// ============================================

/// POST 返回 201，并回显服务器生成的 id
#[actix_web::test]
async fn create_returns_201_with_generated_id() {
    let env = TestEnv::new().await;
    let app = app!(env.state());

    let resp = test::call_service(&app, post_json("/todos", r#"{"title":"学集成测试"}"#)).await;
    assert_eq!(resp.status(), 201, "创建成功应当是 201 而不是 200");

    let body: Value = test::read_body_json(resp).await;
    assert_eq!(body["data"]["title"], "学集成测试");
    assert_eq!(body["data"]["completed"], false, "新任务默认未完成");
    assert_eq!(body["data"]["id"], 1, "空库里第一条应当是 id=1");

    env.cleanup().await;
}

/// 缺少必填字段 → 400
///
/// 这是 actix 的 `web::Json` 提取器在**进入 handler 之前**就拒绝的，
/// 所以 handler 里那套 `match ... Err(e) => internal_error(e)` 根本没执行。
/// 区分"框架拒绝（400）"和"业务失败（500）"是读日志的前提。
#[actix_web::test]
async fn create_rejects_body_without_title() {
    let env = TestEnv::new().await;
    let app = app!(env.state());

    let resp = test::call_service(&app, post_json("/todos", r#"{"completed":true}"#)).await;
    assert_eq!(resp.status(), 400, "缺少 title 应当被提取器拒绝");

    // 坏请求不该留下任何行
    let resp = test::call_service(&app, get_req("/todos")).await;
    let body: Value = test::read_body_json(resp).await;
    assert_eq!(body["data"].as_array().unwrap().len(), 0, "400 之后库里应当仍为空");

    env.cleanup().await;
}

// ============================================
// 用例 3：完整 CRUD 闭环
// ============================================

/// 一条测试走完 C→R→U→D，验证每一步的真实响应
///
/// 为什么不在一条测试里"只测一件事"？
/// 因为 CRUD 的每一步都依赖前一步的结果，拆开就得写 4 份 setup。
/// 关注的粒度是"一个业务场景"，而不是"一个函数调用"。
///
/// 但要注意：一条测试失败时，后面的断言不再执行，
/// 定位需要看是在哪一行挂的。所以**每个断言都要带说明**——
/// 失败信息里能直接看到"是 PUT 那步挂了"还是"DELETE 那步挂了"。
#[actix_web::test]
async fn full_crud_cycle() {
    let env = TestEnv::new().await;
    let app = app!(env.state());

    // --- C：创建 ---
    let resp = test::call_service(&app, post_json("/todos", r#"{"title":"原始标题"}"#)).await;
    assert_eq!(resp.status(), 201, "[C] 创建失败");
    let created: Value = test::read_body_json(resp).await;
    let id = created["data"]["id"].as_i64().expect("[C] 响应里没有 id");

    // --- R：按 id 读 ---
    let resp = test::call_service(&app, get_req(&format!("/todos/{id}"))).await;
    assert_eq!(resp.status(), 200, "[R] 刚创建的任务应当可读");
    let fetched: Value = test::read_body_json(resp).await;
    assert_eq!(fetched["data"]["title"], "原始标题", "[R] 标题不一致");

    // --- U：部分更新（只传 completed）---
    // 这一步是重点：**没传的 title 必须保持不变**。
    // 实现依赖 SQL 的 `COALESCE(?, title)`，一旦有人把它改成
    // "先 SELECT 再整体 UPDATE"却漏了字段，这条断言就会抓住。
    let resp = test::call_service(&app, put_json(&format!("/todos/{id}"), r#"{"completed":true}"#)).await;
    assert_eq!(resp.status(), 200, "[U] 更新应当返回 200");
    let updated: Value = test::read_body_json(resp).await;
    assert_eq!(updated["data"]["completed"], true, "[U] completed 没被更新");
    assert_eq!(
        updated["data"]["title"], "原始标题",
        "[U] 没传 title 却被改掉了 —— 部分更新语义被破坏"
    );

    // --- 再读一次：确认更新真的落库，而不是只改了响应 ---
    let resp = test::call_service(&app, get_req(&format!("/todos/{id}"))).await;
    let refetched: Value = test::read_body_json(resp).await;
    assert_eq!(refetched["data"]["completed"], true, "[R] 更新没有持久化");

    // --- D：删除 ---
    let resp = test::call_service(&app, delete_req(&format!("/todos/{id}"))).await;
    assert_eq!(resp.status(), 200, "[D] 删除应当返回 200 并带回被删的行");
    let deleted: Value = test::read_body_json(resp).await;
    assert_eq!(deleted["data"]["id"], id, "[D] 返回的不是被删的那一行");

    // --- 删完之后必须 404 ---
    let resp = test::call_service(&app, get_req(&format!("/todos/{id}"))).await;
    assert_eq!(resp.status(), 404, "[D] 删掉的任务还能读到");

    // --- 列表为空 ---
    let resp = test::call_service(&app, get_req("/todos")).await;
    let list: Value = test::read_body_json(resp).await;
    assert_eq!(list["data"].as_array().unwrap().len(), 0, "删除后列表应当为空");

    env.cleanup().await;
}

// ============================================
// 用例 4：404 的三种来源
// ============================================

/// 不存在的 id：GET / PUT / DELETE 都该是 404，而不是 500
///
/// 这类测试的价值在于**区分 404 和 500**。
/// 用 `fetch_one` 代替 `fetch_optional` 时，"没查到"会变成
/// `RowNotFound` 错误 → 500 → 客户端以为是服务端挂了。
/// 这是 SQL 层最典型的一类误用，而它**只有测试能发现**——
/// 手工点页面时，你未必会去点一个不存在的 id。
#[actix_web::test]
async fn missing_id_yields_404_on_all_methods() {
    let env = TestEnv::new().await;
    let app = app!(env.state());

    let resp = test::call_service(&app, get_req("/todos/999")).await;
    assert_eq!(resp.status(), 404, "GET 不存在的 id 应当 404");
    let body: Value = test::read_body_json(resp).await;
    assert_eq!(body["success"], false, "404 的响应体也应当是统一结构");
    assert!(body["error"].is_string(), "404 应当带错误说明");

    let resp = test::call_service(&app, put_json("/todos/999", r#"{"completed":true}"#)).await;
    assert_eq!(resp.status(), 404, "PUT 不存在的 id 应当 404（不是 200，也不是 500）");

    let resp = test::call_service(&app, delete_req("/todos/999")).await;
    assert_eq!(resp.status(), 404, "DELETE 不存在的 id 应当 404");

    env.cleanup().await;
}

// ============================================
// 用例 5：方法不匹配
// ============================================

/// `/todos` 只接受 GET 和 POST
///
/// 期望 **405** 而不是 404：路径匹配到了、方法没匹配。
/// 这个区别在排查"我的 PUT 为什么没反应"时很关键——
/// 404 说明路径写错了，405 说明方法写错了。
///
/// 这条测试同时锁住了一个**实测出来的坑**（2026-10-31，actix-web 4.18.1）：
///
/// ```text
/// cfg.route("/todos", get) + cfg.route("/todos", post)  → 404
/// web::resource("/todos").route(get).route(post)        → 405
/// ```
///
/// `cfg.route()` 每次都注册一个独立的 resource，各管一个方法；
/// 方法不匹配 → 整条 resource 不匹配 → 落到 default_service → 404。
/// 所以 `lib.rs` 的 `configure` 用的是 `web::resource(...)`。
/// 如果哪天有人为了"简洁"改回 `cfg.route`，这条断言会立刻挂——
/// 挂的信息会直接指出"集合上的 PUT 变成了 404"。
#[actix_web::test]
async fn collection_rejects_put() {
    let env = TestEnv::new().await;
    let app = app!(env.state());

    let resp = test::call_service(&app, put_json("/todos", r#"{"completed":true}"#)).await;
    assert_eq!(
        resp.status(),
        405,
        "集合上的 PUT 应当是 405（路径存在、方法不支持），不是 404"
    );

    env.cleanup().await;
}

// ============================================
// 用例 6：并发创建
// ============================================

/// 并发创建 N 条，id 必须互不相同
///
/// 这条测试保护的是一个**已经修好的历史 bug**：
/// 第 25 天的实现在内存里用 `todos.iter().max() + 1` 生成 id。
/// 两个请求几乎同时到达 → 都读到 max=5 → 都算出 6 → 后写覆盖先写。
/// 那时候手工测不出来，因为手速永远达不到并发。
///
/// 现在 id 由 SQLite 的 `INTEGER PRIMARY KEY` 分配，天然原子。
/// 这条测试就是那个保证的回归防护：如果哪天有人改回"手工算 id"，
/// 它会立刻挂在"id 重复"上。
///
/// 注意 `tokio::join!` 三个请求：它们在**同一个测试任务里**并发推进，
/// 不是真的多线程——但已经足够让"先读 max 再写"的时序暴露。
/// 真要压多线程得用 `actix_web::rt::spawn`，对本项目来说是过度设计。
#[actix_web::test]
async fn concurrent_creates_get_distinct_ids() {
    let env = TestEnv::new().await;
    let app = app!(env.state());

    let (ra, rb, rc) = tokio::join!(
        test::call_service(&app, post_json("/todos", r#"{"title":"并发A"}"#)),
        test::call_service(&app, post_json("/todos", r#"{"title":"并发B"}"#)),
        test::call_service(&app, post_json("/todos", r#"{"title":"并发C"}"#)),
    );

    assert_eq!(ra.status(), 201);
    assert_eq!(rb.status(), 201);
    assert_eq!(rc.status(), 201);

    let ai = test::read_body_json::<Value, _>(ra).await["data"]["id"].as_i64().unwrap();
    let bi = test::read_body_json::<Value, _>(rb).await["data"]["id"].as_i64().unwrap();
    let ci = test::read_body_json::<Value, _>(rc).await["data"]["id"].as_i64().unwrap();

    let mut ids = vec![ai, bi, ci];
    ids.sort_unstable();
    ids.dedup();
    assert_eq!(ids.len(), 3, "并发创建产生了重复 id: {ai}, {bi}, {ci}");

    // 库里应当正好三行
    let resp = test::call_service(&app, get_req("/todos")).await;
    let list: Value = test::read_body_json(resp).await;
    assert_eq!(list["data"].as_array().unwrap().len(), 3, "并发创建丢了行");

    env.cleanup().await;
}

// ============================================
// 用例 7：测试隔离本身
// ============================================

/// 两个 `TestEnv` 之间数据必须互不可见
///
/// 这条测试测的不是业务代码，而是**测试基础设施**。
/// 如果 `TestEnv::new()` 哪天被改成共用同一个库，
/// 所有"列表应当为空"的断言都会开始随机失败，
/// 而那时没人会想到问题出在测试脚手架里。
///
/// 顺手把这条测试当作隔离方案的说明书：读它就知道
/// "一个测试 = 一个库"是靠什么实现的。
#[actix_web::test]
async fn two_envs_do_not_share_data() {
    let env_a = TestEnv::new().await;
    let app_a = app!(env_a.state());
    let resp = test::call_service(&app_a, post_json("/todos", r#"{"title":"只属于 A"}"#)).await;
    assert_eq!(resp.status(), 201);

    let env_b = TestEnv::new().await;
    let app_b = app!(env_b.state());
    let resp = test::call_service(&app_b, get_req("/todos")).await;
    let list: Value = test::read_body_json(resp).await;
    assert_eq!(
        list["data"].as_array().unwrap().len(),
        0,
        "B 看到了 A 的数据 —— 测试之间没有隔离"
    );

    env_a.cleanup().await;
    env_b.cleanup().await;
}

/*
【为什么不用 `sqlite::memory:`】

先纠正一个流传很广的说法："连接池里每个连接都是独立的内存库"。
**在 sqlx 0.8 里这句话是错的**，而且我一开始也这么写，实测才发现不对。

sqlx 源码（`sqlx-sqlite-0.8.6/src/options/parse.rs`）在解析

    sqlite::memory:

时做了两件事：

    options.in_memory = true;
    options.shared_cache = true;
    let seqno = IN_MEMORY_DB_SEQ.fetch_add(1, Ordering::Relaxed);
    options.filename = format!("file:sqlx-in-memory-{seqno}");

也就是说 `:memory:` 被改写成**具名的共享内存库**，
同一个池的所有连接看到的是同一个库。
实测：同时持有两条连接，连接 1 建表、连接 2 立刻能查到——
所以"表不存在"这个担心不成立。

> 真正会踩坑的场景是：你自己用 `sqlx::sqlite::SqliteConnectOptions`
> 手动设 `filename(":memory:")`，或者用 `sqlx::query` 各自 `connect()`。
> 那时每条连接确实是独立的库。

那为什么本项目仍然用临时文件？**首要原因不是正确性，而是覆盖面**：

1. **内存库没有 WAL。** SQLite 官方明确指出 WAL 依赖共享内存机制，
   内存库用不上。而线上的 `init_db` 是 `journal_mode(Wal)`。
   用内存库测试，等于**永远不测 WAL 那条路径**——
   而"WAL 文件被占用""并发读写"这些问题恰好只在 WAL 下出现。
2. **每次 `init_db("sqlite::memory:")` 都会拿到新的 seqno**，
   两个 `TestEnv` 天然隔离——这点和临时文件等价。
   但内存库随池关闭而消失，**失败后无法"尸检"**：
   临时文件可以直接用 sqlite3 打开看数据到底长什么样。
3. 临时文件与线上只差一个路径，没有第二处差异。

一句话：**测试环境应当和生产环境在"有影响的差异"上尽量一致。
WAL 是有影响的，路径不是。**

【这套测试的已知局限】（诚实标注，避免高估覆盖）

1. **健康检查测不出"有没有真的查库"**
   handler 里把 `sqlx::query("SELECT 1")` 删掉、直接返回 200，
   测试照样通过。要覆盖它需要可注入的探针，属于过度设计。

2. **测不到数据库真的挂掉时返回 503**
   那需要关掉池再发请求，会让测试依赖实现顺序。

3. **测不到 WAL 并发行为**
   `actix_web::test` 只在一两个线程里推进，不是真实的压测。
   真正的并发问题要用 `wrk` / `ab` 对着 `cargo run` 打。

4. **测不到中间件**
   `configure` 只注册路由，没有 `.wrap(...)`。
   等第 35 天的 `Auth` 中间件接进来，还要补"401 不经过 handler"的用例
   ——那时断言"handler 没执行"需要别的证据（比如日志或计数器）。
*/
