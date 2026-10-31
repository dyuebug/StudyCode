// ============================================
// task-manager 集成测试
// 日期：2026-11-01（第37天）
// ============================================

/*
【集成测试测的是"整个 App"，不是某个函数】

所以这里不用 `#[test]` 直接调 handler，而是：
    建临时库 → 跑迁移 → 用 `task_manager::app()` 组装真实 App
    → 用 `TestRequest` 发真实 HTTP 请求 → 断言状态码和 JSON

【为什么必须是 `app()` 而不是自己拼 App】
第 36 天的教训：测试里重新写一遍路由表和中间件，测的就是"另一份代码"。
生产环境漏了 `.wrap(auth())`，测试照样全绿——这种绿是最危险的绿。
`app()` 在 lib 里，main 和测试共用，配置天然只有一份。

【为什么不用 `sqlite::memory:`】
第 36 天实测过：它确实全连接共享一份内存库（`shared_cache`），能跑。
但它不支持 WAL，而生产用的是 WAL。测试跑在另一条代码路径上，
等于放弃了"WAL 下行为是否正常"这一整类问题的覆盖。
所以这里用**临时目录里的真文件**，还能顺带验证迁移脚本能在干净库上跑通。
*/

use std::path::{Path, PathBuf};
use std::sync::atomic::{AtomicU64, Ordering};
use std::time::Duration;

use actix_web::test;
use serde_json::{Value, json};
use sqlx::sqlite::SqlitePool;
use task_manager::state::AppState;

static SEQ: AtomicU64 = AtomicU64::new(0);

const ALICE: &str = "Bearer token-alice";
const BOB: &str = "Bearer token-bob";

// ============================================
// 测试环境
// ============================================

/// 临时目录守卫：`Drop` 时删掉
///
/// 【为什么清理要写进 `Drop`，而不是测试函数最后一行】
/// 测试 panic 时，最后一行根本不会执行——失败的那次恰好会留下垃圾目录。
/// `Drop` 在 panic 展开（unwind）过程中仍会运行，成功失败都能清干净。
///
/// 【为什么要重试】
/// Windows 上刚 `close()` 掉的 SQLite 库文件仍可能被 WAL 的 `-shm`
/// 内存映射占用着，`remove_dir_all` 会拿到 `os error 32`（文件被占用）。
/// 这是**时序问题**，不是逻辑错误，所以用有限次重试而不是 `sleep`
/// 猜一个"足够长"的固定值——猜短了失败，猜长了每次测试都白等。
struct TempDirGuard(PathBuf);

impl TempDirGuard {
    fn new(tag: &str) -> std::io::Result<Self> {
        let n = SEQ.fetch_add(1, Ordering::Relaxed);
        let dir = std::env::temp_dir().join(format!("{tag}-{}-{n}", std::process::id()));
        std::fs::create_dir_all(&dir)?;
        Ok(Self(dir))
    }

    fn path(&self) -> &Path {
        &self.0
    }
}

impl Drop for TempDirGuard {
    fn drop(&mut self) {
        for _ in 0..10 {
            if std::fs::remove_dir_all(&self.0).is_ok() {
                return;
            }
            std::thread::sleep(Duration::from_millis(50));
        }
        // 清不掉不是测试失败的理由，但必须留痕，否则问题会一直隐身
        eprintln!(
            "⚠️ 临时目录未能删除（不影响测试结论）: {}",
            self.0.display()
        );
    }
}

/// 一套隔离环境：独立临时目录 + 独立数据库
struct TestEnv {
    _dir: TempDirGuard,
    pool: SqlitePool,
}

impl TestEnv {
    async fn new() -> Self {
        let dir = TempDirGuard::new("task-manager-test").expect("创建临时目录失败");
        let db_path = dir.path().join("test.db");
        // Windows 的反斜杠在 URL 里要转成正斜杠，否则 sqlx 解析路径出错
        let url = format!(
            "sqlite:{}",
            db_path.display().to_string().replace('\\', "/")
        );
        let pool = task_manager::db::init_db(&url)
            .await
            .expect("初始化测试库失败（迁移没跑通？）");
        Self { _dir: dir, pool }
    }

    fn state(&self) -> AppState {
        AppState::new(self.pool.clone())
    }

    /// 显式关池：不关就 `Drop` 时文件可能还开着
    async fn cleanup(self) {
        self.pool.close().await;
    }
}

/// 组装与生产完全一致的 App
macro_rules! app {
    ($state:expr) => {
        test::init_service(task_manager::app($state)).await
    };
}

/// 读 body 为 JSON，便于按字段断言
///
/// 泛型 `B` 不能省：响应体类型会随中间件变化
/// （`auth` 用 `EitherBody` 区分放行/拒绝两个分支），
/// 写死 `ServiceResponse` 的默认 `BoxBody` 会匹配不上调用点的类型。
async fn body_json<B>(resp: actix_web::dev::ServiceResponse<B>) -> Value
where
    B: actix_web::body::MessageBody,
{
    test::read_body_json(resp).await
}

// ============================================
// 用例
// ============================================

/// 健康检查不需要 token
///
/// 【为什么专门测这一条】
/// 认证中间件挂在 `/projects` scope 上，而非 App 级——如果哪天有人
/// 图省事把它挪到 App 级，这条用例会立刻变红。
/// 负载均衡器探活不带 token，健康检查被拦就是"服务全部下线"。
#[actix_web::test]
async fn health_needs_no_token_and_reports_db() {
    let env = TestEnv::new().await;
    let app = app!(env.state());

    let resp = test::call_service(&app, test::TestRequest::get().uri("/health").to_request()).await;
    assert_eq!(resp.status(), 200);

    let body = body_json(resp).await;
    assert_eq!(body["success"], true);
    assert_eq!(body["data"]["db"], "ok", "健康检查应带上数据库状态");

    env.cleanup().await;
}

/// 没 token → 401，且响应带 `WWW-Authenticate`
#[actix_web::test]
async fn projects_without_token_is_401() {
    let env = TestEnv::new().await;
    let app = app!(env.state());

    let resp =
        test::call_service(&app, test::TestRequest::get().uri("/projects").to_request()).await;
    assert_eq!(resp.status(), 401);
    // RFC 9110：401 必须带 WWW-Authenticate 告诉客户端该用哪种认证
    assert!(
        resp.headers().contains_key("www-authenticate"),
        "401 响应缺少 WWW-Authenticate 头"
    );

    let body = body_json(resp).await;
    assert_eq!(body["code"], "unauthorized");

    // 无效 token 走同一条拒绝路径
    let resp = test::call_service(
        &app,
        test::TestRequest::get()
            .uri("/projects")
            .insert_header(("Authorization", "Bearer wrong"))
            .to_request(),
    )
    .await;
    assert_eq!(resp.status(), 401);

    env.cleanup().await;
}

/// 项目列表按 owner 隔离：alice 看到种子项目，bob 看到空
///
/// 这条用例的价值不在"能列出来"，而在**两个用户看到的数据不同**。
/// 只测一个用户的话，`WHERE owner_id = ?` 写错成 `WHERE 1` 也照样绿。
#[actix_web::test]
async fn projects_are_scoped_to_owner() {
    let env = TestEnv::new().await;
    let app = app!(env.state());

    let resp = test::call_service(
        &app,
        test::TestRequest::get()
            .uri("/projects")
            .insert_header(("Authorization", ALICE))
            .to_request(),
    )
    .await;
    assert_eq!(resp.status(), 200);
    let body = body_json(resp).await;
    let list = body["data"].as_array().expect("data 应是数组");
    assert_eq!(list.len(), 1, "alice 应看到 1 个种子项目");
    assert_eq!(list[0]["owner_id"], 1);
    assert_eq!(list[0]["name"], "学习 Rust");

    let resp = test::call_service(
        &app,
        test::TestRequest::get()
            .uri("/projects")
            .insert_header(("Authorization", BOB))
            .to_request(),
    )
    .await;
    let body = body_json(resp).await;
    assert_eq!(
        body["data"].as_array().expect("data 应是数组").len(),
        0,
        "bob 不该看到 alice 的项目"
    );

    env.cleanup().await;
}

/// 建项目 → 201 → 再查能看到
#[actix_web::test]
async fn create_project_returns_201_then_visible_in_list() {
    let env = TestEnv::new().await;
    let app = app!(env.state());

    let resp = test::call_service(
        &app,
        test::TestRequest::post()
            .uri("/projects")
            .insert_header(("Authorization", ALICE))
            .set_json(json!({ "name": "面试准备", "description": "八股 + 项目复盘" }))
            .to_request(),
    )
    .await;
    assert_eq!(resp.status(), 201);
    let created = body_json(resp).await;
    assert_eq!(created["data"]["name"], "面试准备");
    assert_eq!(
        created["data"]["owner_id"], 1,
        "owner_id 必须来自 token，不能来自 body"
    );
    let new_id = created["data"]["id"].as_i64().expect("应返回自增 id");

    let resp = test::call_service(
        &app,
        test::TestRequest::get()
            .uri("/projects")
            .insert_header(("Authorization", ALICE))
            .to_request(),
    )
    .await;
    let body = body_json(resp).await;
    let list = body["data"].as_array().unwrap();
    assert_eq!(list.len(), 2);
    assert!(list.iter().any(|p| p["id"].as_i64() == Some(new_id)));

    // description 省略时应落到数据库默认值空串，而不是报错
    let resp = test::call_service(
        &app,
        test::TestRequest::post()
            .uri("/projects")
            .insert_header(("Authorization", ALICE))
            .set_json(json!({ "name": "只有名字" }))
            .to_request(),
    )
    .await;
    assert_eq!(resp.status(), 201);
    assert_eq!(body_json(resp).await["data"]["description"], "");

    env.cleanup().await;
}

/// 同用户重名 → 409；不同用户重名 → 允许
///
/// 这条验证的是 `UNIQUE(owner_id, name)` 的**作用域**。
/// 写成 `UNIQUE(name)` 时第一个断言仍然通过，只有第二段会红——
/// 所以两段都要有。
#[actix_web::test]
async fn duplicate_project_name_is_409_per_owner() {
    let env = TestEnv::new().await;
    let app = app!(env.state());

    let make = |token: &'static str, name: &'static str| {
        test::TestRequest::post()
            .uri("/projects")
            .insert_header(("Authorization", token))
            .set_json(json!({ "name": name }))
            .to_request()
    };

    let resp = test::call_service(&app, make(ALICE, "重名测试")).await;
    assert_eq!(resp.status(), 201);

    let resp = test::call_service(&app, make(ALICE, "重名测试")).await;
    assert_eq!(resp.status(), 409, "同一用户下重名应 409");
    assert_eq!(body_json(resp).await["code"], "duplicate_name");

    let resp = test::call_service(&app, make(BOB, "重名测试")).await;
    assert_eq!(resp.status(), 201, "不同用户可以有同名项目");

    env.cleanup().await;
}

/// 建任务 → 列表里能看到，且默认值正确
#[actix_web::test]
async fn create_task_then_list_roundtrip() {
    let env = TestEnv::new().await;
    let app = app!(env.state());

    let resp = test::call_service(
        &app,
        test::TestRequest::post()
            .uri("/projects/1/tasks")
            .insert_header(("Authorization", ALICE))
            .set_json(json!({ "title": "写第 37 天日志" }))
            .to_request(),
    )
    .await;
    assert_eq!(resp.status(), 201);
    let created = body_json(resp).await;
    assert_eq!(created["data"]["title"], "写第 37 天日志");
    assert_eq!(created["data"]["status"], "todo", "status 应取数据库默认值");
    assert_eq!(created["data"]["priority"], 2, "priority 省略时应为 2");
    assert_eq!(created["data"]["project_id"], 1, "project_id 必须来自 URL");

    let resp = test::call_service(
        &app,
        test::TestRequest::get()
            .uri("/projects/1/tasks")
            .insert_header(("Authorization", ALICE))
            .to_request(),
    )
    .await;
    assert_eq!(resp.status(), 200);
    let body = body_json(resp).await;
    let list = body["data"].as_array().unwrap();
    assert_eq!(list.len(), 4, "3 条种子 + 1 条新建");
    // 排序规则：priority 升序（1 最高优先级排前面）
    assert_eq!(list[0]["priority"], 1);
    assert_eq!(list.last().unwrap()["priority"], 2);

    env.cleanup().await;
}

/// 别人的项目 → 404（不是 403）
///
/// 【为什么断言 404 而不是 403】
/// 403 等于承认"这个项目存在"。项目 id 是自增的，
/// 承认存在就能被枚举出总量。这条断言把"不泄露"这个决定钉进测试里，
/// 以后谁改成 403 都会红。
#[actix_web::test]
async fn foreign_project_is_404_not_403() {
    let env = TestEnv::new().await;
    let app = app!(env.state());

    let resp = test::call_service(
        &app,
        test::TestRequest::get()
            .uri("/projects/1/tasks")
            .insert_header(("Authorization", BOB))
            .to_request(),
    )
    .await;
    assert_eq!(resp.status(), 404);
    assert_eq!(body_json(resp).await["code"], "not_found");

    let resp = test::call_service(
        &app,
        test::TestRequest::post()
            .uri("/projects/1/tasks")
            .insert_header(("Authorization", BOB))
            .set_json(json!({ "title": "偷偷塞一条" }))
            .to_request(),
    )
    .await;
    assert_eq!(resp.status(), 404, "写接口同样不能越权");

    // 不存在的项目也是 404 —— 与越权无法区分，正是设计意图
    let resp = test::call_service(
        &app,
        test::TestRequest::get()
            .uri("/projects/9999/tasks")
            .insert_header(("Authorization", ALICE))
            .to_request(),
    )
    .await;
    assert_eq!(resp.status(), 404);

    env.cleanup().await;
}

/// 参数校验：priority 越界 → 400，空标题 → 400
#[actix_web::test]
async fn invalid_input_is_400() {
    let env = TestEnv::new().await;
    let app = app!(env.state());

    let resp = test::call_service(
        &app,
        test::TestRequest::post()
            .uri("/projects/1/tasks")
            .insert_header(("Authorization", ALICE))
            .set_json(json!({ "title": "越界", "priority": 9 }))
            .to_request(),
    )
    .await;
    assert_eq!(resp.status(), 400, "priority=9 超出 1..=4");
    assert_eq!(body_json(resp).await["code"], "invalid_priority");

    let resp = test::call_service(
        &app,
        test::TestRequest::post()
            .uri("/projects/1/tasks")
            .insert_header(("Authorization", ALICE))
            .set_json(json!({ "title": "   " }))
            .to_request(),
    )
    .await;
    assert_eq!(resp.status(), 400, "纯空白标题应被拒");

    env.cleanup().await;
}

/// 不存在的方法 / 路径 → 日志中间件能看到 404
///
/// 这条是给第 35 天结论做的回归：日志中间件必须挂在 App 级，
/// 否则 404 是在路由匹配阶段产生的，中间件根看不到这条请求。
/// 断言的是"请求正常返回 404"——日志本身在 stdout，
/// 需要人看，测试无法断言；但用例名和注释把意图留在这里。
#[actix_web::test]
async fn unknown_route_is_404() {
    let env = TestEnv::new().await;
    let app = app!(env.state());

    let resp = test::call_service(
        &app,
        test::TestRequest::get()
            .uri("/no-such-path")
            .insert_header(("Authorization", ALICE))
            .to_request(),
    )
    .await;
    assert_eq!(resp.status(), 404);

    // 路径存在、方法不存在 → 405（因为用的是 web::resource 合并路由，
    // 而不是两次 cfg.route；第 36 天实测：后者会变成 404）
    let resp = test::call_service(
        &app,
        test::TestRequest::put()
            .uri("/projects")
            .insert_header(("Authorization", ALICE))
            .to_request(),
    )
    .await;
    assert_eq!(resp.status(), 405);

    env.cleanup().await;
}

// ============================================
// 第 38 天：任务单条读写 / 状态机 / 标签 / 级联
// ============================================

/// PATCH 只改给的字段，其余不动
#[actix_web::test]
async fn patch_title_keeps_other_fields() {
    let env = TestEnv::new().await;
    let app = app!(env.state());

    let resp = test::call_service(
        &app,
        test::TestRequest::patch()
            .uri("/tasks/1")
            .insert_header(("Authorization", ALICE))
            .set_json(json!({ "title": "改个名字" }))
            .to_request(),
    )
    .await;
    assert_eq!(resp.status(), 200);
    let body = body_json(resp).await;
    assert_eq!(body["data"]["title"], "改个名字");
    assert_eq!(body["data"]["status"], "done", "status 不该被 PATCH 改动");
    assert_eq!(body["data"]["priority"], 1, "没给的字段必须保持原值");
    assert_eq!(body["data"]["project_id"], 1);

    env.cleanup().await;
}

/// `Option<Option<T>>` 三态：没给 → 不变；给 null → 清空；给值 → 改
///
/// 【这条用例为什么必须存在】
/// serde 对 `Option<Option<T>>` 的默认行为会把 `null` 和"字段缺失"
/// 都解析成 `None`——于是"清空截止时间"永远无效，**而且不报错**。
/// 没有这条断言，这个 bug 只能等用户来报。
#[actix_web::test]
async fn patch_due_at_is_three_state() {
    let env = TestEnv::new().await;
    let app = app!(env.state());

    // 1) 给值 → 写入
    let resp = test::call_service(
        &app,
        test::TestRequest::patch()
            .uri("/tasks/3")
            .insert_header(("Authorization", ALICE))
            .set_json(json!({ "due_at": "2026-11-30" }))
            .to_request(),
    )
    .await;
    assert_eq!(body_json(resp).await["data"]["due_at"], "2026-11-30");

    // 2) 不给这个字段 → 保持原值
    let resp = test::call_service(
        &app,
        test::TestRequest::patch()
            .uri("/tasks/3")
            .insert_header(("Authorization", ALICE))
            .set_json(json!({ "title": "只改标题" }))
            .to_request(),
    )
    .await;
    let body = body_json(resp).await;
    assert_eq!(body["data"]["title"], "只改标题");
    assert_eq!(body["data"]["due_at"], "2026-11-30", "字段缺失不该清空");

    // 3) 显式 null → 清空
    let resp = test::call_service(
        &app,
        test::TestRequest::patch()
            .uri("/tasks/3")
            .insert_header(("Authorization", ALICE))
            .set_json(json!({ "due_at": null }))
            .to_request(),
    )
    .await;
    assert_eq!(
        body_json(resp).await["data"]["due_at"],
        Value::Null,
        "显式 null 应清空"
    );

    env.cleanup().await;
}

/// 合法流转：todo → doing → done
#[actix_web::test]
async fn status_transition_legal_path() {
    let env = TestEnv::new().await;
    let app = app!(env.state());

    for next in ["doing", "done"] {
        let resp = test::call_service(
            &app,
            test::TestRequest::patch()
                .uri("/tasks/3/status")
                .insert_header(("Authorization", ALICE))
                .set_json(json!({ "status": next }))
                .to_request(),
        )
        .await;
        assert_eq!(resp.status(), 200, "todo→doing→done 应放行（目标 {next}）");
        assert_eq!(body_json(resp).await["data"]["status"], next);
    }

    // 同值幂等：重复 PATCH 成 done 不应报错
    let resp = test::call_service(
        &app,
        test::TestRequest::patch()
            .uri("/tasks/3/status")
            .insert_header(("Authorization", ALICE))
            .set_json(json!({ "status": "done" }))
            .to_request(),
    )
    .await;
    assert_eq!(resp.status(), 200, "同值应为幂等");

    env.cleanup().await;
}

/// 非法流转：todo → done 必须被拒（409）
#[actix_web::test]
async fn status_transition_illegal_is_409() {
    let env = TestEnv::new().await;
    let app = app!(env.state());

    let resp = test::call_service(
        &app,
        test::TestRequest::patch()
            .uri("/tasks/3/status")
            .insert_header(("Authorization", ALICE))
            .set_json(json!({ "status": "done" }))
            .to_request(),
    )
    .await;
    assert_eq!(resp.status(), 409, "跳过 doing 直接 done 应被拒");
    let body = body_json(resp).await;
    assert_eq!(body["code"], "illegal_transition");

    // 拒绝之后状态必须没变
    let resp = test::call_service(
        &app,
        test::TestRequest::get()
            .uri("/tasks/3")
            .insert_header(("Authorization", ALICE))
            .to_request(),
    )
    .await;
    assert_eq!(body_json(resp).await["data"]["status"], "todo");

    // 未知状态值 → 400（是客户端写错，不是服务端出错）
    let resp = test::call_service(
        &app,
        test::TestRequest::patch()
            .uri("/tasks/3/status")
            .insert_header(("Authorization", ALICE))
            .set_json(json!({ "status": "blocked" }))
            .to_request(),
    )
    .await;
    assert_eq!(resp.status(), 400);
    assert_eq!(body_json(resp).await["code"], "invalid_status");

    env.cleanup().await;
}

/// 标签：贴两次只留一行，撕掉后回到空
///
/// 【用任务 3 而不是任务 1】
/// 种子数据里任务 1 已经挂了 `rust` 标签（`task_tags (1,1)`），
/// 拿它断言"只有 1 个标签"会直接失败——第一次跑这条用例就是被这个坑绊住的。
/// 测试数据里**已有的关联**必须算进去，否则断言写的是"我以为的初始状态"。
#[actix_web::test]
async fn tags_are_idempotent_and_removable() {
    let env = TestEnv::new().await;
    let app = app!(env.state());

    for _ in 0..2 {
        let resp = test::call_service(
            &app,
            test::TestRequest::post()
                .uri("/tasks/3/tags")
                .insert_header(("Authorization", ALICE))
                .set_json(json!({ "name": "urgent" }))
                .to_request(),
        )
        .await;
        assert_eq!(resp.status(), 200);
    }

    let resp = test::call_service(
        &app,
        test::TestRequest::get()
            .uri("/tasks/3")
            .insert_header(("Authorization", ALICE))
            .to_request(),
    )
    .await;
    let tags = body_json(resp).await["data"]["tags"].clone();
    let tags = tags.as_array().expect("tags 应是数组");
    assert_eq!(tags.len(), 1, "重复贴同一标签不应产生第二行");
    assert_eq!(tags[0], "urgent");

    let resp = test::call_service(
        &app,
        test::TestRequest::delete()
            .uri("/tasks/3/tags/urgent")
            .insert_header(("Authorization", ALICE))
            .to_request(),
    )
    .await;
    assert_eq!(resp.status(), 200);
    // ⚠️ 注意形状差异：`GET /tasks/{id}` 返回 `{task..., tags: [...]}`，
    // 而贴/撕标签返回的是 `data: [...]` 本身（它只关心标签列表）。
    // 按 GET 的形状去读 DELETE 的响应，`data.tags` 就是 `null`——
    // 断言不会报"形状不对"，只会报 `unwrap()` on None。
    assert_eq!(
        body_json(resp).await["data"].as_array().unwrap().len(),
        0,
        "撕掉后应为空"
    );

    env.cleanup().await;
}

/// 列表筛选：status / priority / limit
#[actix_web::test]
async fn list_tasks_filters() {
    let env = TestEnv::new().await;
    let app = app!(env.state());

    // 用局部宏而不是闭包：闭包返回的 future 会借走 `&app`，
    // 捕获引用 + 返回 async block 在生命周期上很难写对，
    // 而这里只是想把"发请求、数数组长度"这四行重复 6 次省掉。
    macro_rules! count {
        ($uri:expr) => {{
            let resp = test::call_service(
                &app,
                test::TestRequest::get()
                    .uri($uri)
                    .insert_header(("Authorization", ALICE))
                    .to_request(),
            )
            .await;
            assert_eq!(resp.status(), 200, "{} 应返回 200", $uri);
            body_json(resp).await["data"].as_array().unwrap().len()
        }};
    }

    assert_eq!(count!("/projects/1/tasks"), 3, "无条件 = 全部");
    assert_eq!(count!("/projects/1/tasks?status=doing"), 1);
    assert_eq!(count!("/projects/1/tasks?priority=1"), 2);
    assert_eq!(count!("/projects/1/tasks?status=todo&priority=1"), 0);
    assert_eq!(count!("/projects/1/tasks?limit=1"), 1, "limit 应生效");
    assert_eq!(
        count!("/projects/1/tasks?limit=999"),
        3,
        "limit 被 clamp，但只有 3 条"
    );

    env.cleanup().await;
}

/// 删任务后 404，且不残留标签关联
#[actix_web::test]
async fn delete_task_then_404() {
    let env = TestEnv::new().await;
    let app = app!(env.state());

    // 种子任务 2 挂了 rust / web 两个标签
    let resp = test::call_service(
        &app,
        test::TestRequest::delete()
            .uri("/tasks/2")
            .insert_header(("Authorization", ALICE))
            .to_request(),
    )
    .await;
    assert_eq!(resp.status(), 204);

    let resp = test::call_service(
        &app,
        test::TestRequest::get()
            .uri("/tasks/2")
            .insert_header(("Authorization", ALICE))
            .to_request(),
    )
    .await;
    assert_eq!(resp.status(), 404);

    // 外键 `ON DELETE CASCADE` 应把 task_tags 里的关联一并删掉。
    // 这里直接查库，因为 API 已经没有入口能看到"孤儿关联"了。
    let left: i64 = sqlx::query_scalar("SELECT COUNT(*) FROM task_tags WHERE task_id = 2")
        .fetch_one(&env.pool)
        .await
        .unwrap();
    assert_eq!(left, 0, "任务删除后 task_tags 不应残留");

    env.cleanup().await;
}

/// 删项目 → 任务级联删除（验证 `foreign_keys(true)` 真的生效）
///
/// 【这条是整份测试里最有价值的一条】
/// SQLite 的外键默认**关闭**。如果 `db.rs` 里少写 `.foreign_keys(true)`，
/// 删项目**不会删任务**，而且**不报任何错**——
/// 数据悄悄变成孤儿。只有这条断言能发现它。
#[actix_web::test]
async fn delete_project_cascades_tasks() {
    let env = TestEnv::new().await;
    let app = app!(env.state());

    let before: i64 = sqlx::query_scalar("SELECT COUNT(*) FROM tasks WHERE project_id = 1")
        .fetch_one(&env.pool)
        .await
        .unwrap();
    assert_eq!(before, 3);

    let resp = test::call_service(
        &app,
        test::TestRequest::delete()
            .uri("/projects/1")
            .insert_header(("Authorization", ALICE))
            .to_request(),
    )
    .await;
    assert_eq!(resp.status(), 204);

    let after: i64 = sqlx::query_scalar("SELECT COUNT(*) FROM tasks WHERE project_id = 1")
        .fetch_one(&env.pool)
        .await
        .unwrap();
    assert_eq!(after, 0, "外键没生效：删项目后任务成了孤儿行");

    // 别人的项目删不掉，且返回 404（不泄露存在性）
    let resp = test::call_service(
        &app,
        test::TestRequest::delete()
            .uri("/projects/9999")
            .insert_header(("Authorization", BOB))
            .to_request(),
    )
    .await;
    assert_eq!(resp.status(), 404);

    env.cleanup().await;
}

// ============================================
// 第 39 天：统一错误层
// ============================================

/// 500 必须是 JSON，且**不能**泄露 SQL 原文
///
/// 【怎么可靠地造出一个 500】
/// 正常请求走不到数据库错误那条路（业务校验全在前），
/// 所以最直接的办法是**把表删掉**：`DROP TABLE tasks` 之后，
/// 任何读任务的请求都会拿到 `no such table: tasks`。
/// 这不是"模拟"，是真的让下层炸掉——错误是 sqlx 自己产生的。
#[actix_web::test]
async fn internal_error_is_json_and_hides_sql() {
    let env = TestEnv::new().await;

    sqlx::query("DROP TABLE tasks")
        .execute(&env.pool)
        .await
        .expect("删表失败——后面的断言就没有意义了");

    let app = app!(env.state());
    let resp = test::call_service(
        &app,
        test::TestRequest::get()
            .uri("/projects/1/tasks")
            .insert_header(("Authorization", ALICE))
            .to_request(),
    )
    .await;

    assert_eq!(resp.status(), 500);

    let ct = resp
        .headers()
        .get(actix_web::http::header::CONTENT_TYPE)
        .and_then(|v| v.to_str().ok())
        .unwrap_or("")
        .to_string();
    assert!(
        ct.starts_with("application/json"),
        "500 的 Content-Type 应以 application/json 开头，实际是「{ct}」"
    );

    let body: Value = body_json(resp).await;
    assert_eq!(body["success"], false);
    assert_eq!(body["code"], "internal", "500 应该是固定的 internal 码");
    assert!(body["message"].is_string());

    // 泄漏检查：这些词组任一出现在响应体里都说明内部信息外传了
    let text = body.to_string();
    for leak in ["no such table", "tasks", "SELECT", "sqlx", "sqlite"] {
        assert!(
            !text.contains(leak),
            "500 响应体泄露了内部信息「{leak}」：{text}"
        );
    }

    env.cleanup().await;
}

/// 业务错误的形状必须完全一致：`{success, code, message}`
///
/// 【为什么值得单独测】
/// 形状不一致时客户端不会报错，只会 `unwrap()` 到 `None`（第 38 天踩过）。
/// 三种状态码各取一条，比较 key 集合——这比逐字段断言更能锁住"形状"。
#[actix_web::test]
async fn error_bodies_share_one_shape() {
    let env = TestEnv::new().await;
    let app = app!(env.state());

    async fn keys_of<B>(resp: actix_web::dev::ServiceResponse<B>) -> Vec<String>
    where
        B: actix_web::body::MessageBody,
    {
        let v: Value = body_json(resp).await;
        let mut ks: Vec<String> = v
            .as_object()
            .expect("错误响应体应当是 JSON 对象")
            .keys()
            .cloned()
            .collect();
        ks.sort();
        ks
    }

    // 404：不存在的任务
    let r404 = test::call_service(
        &app,
        test::TestRequest::get()
            .uri("/tasks/999999")
            .insert_header(("Authorization", ALICE))
            .to_request(),
    )
    .await;
    assert_eq!(r404.status(), 404);

    // 400：标题为空
    let r400 = test::call_service(
        &app,
        test::TestRequest::post()
            .uri("/projects/1/tasks")
            .insert_header(("Authorization", ALICE))
            .set_json(json!({ "title": "   " }))
            .to_request(),
    )
    .await;
    assert_eq!(r400.status(), 400);

    // 409：同一个人建同名项目（`学习 Rust` 是迁移里的种子项目）
    let r409 = test::call_service(
        &app,
        test::TestRequest::post()
            .uri("/projects")
            .insert_header(("Authorization", ALICE))
            .set_json(json!({ "name": "学习 Rust" }))
            .to_request(),
    )
    .await;
    assert_eq!(r409.status(), 409);

    let k404 = keys_of(r404).await;
    let k400 = keys_of(r400).await;
    let k409 = keys_of(r409).await;

    assert_eq!(
        k404, k400,
        "404 与 400 的错误体形状不同：{k404:?} vs {k400:?}"
    );
    assert_eq!(
        k400, k409,
        "400 与 409 的错误体形状不同：{k400:?} vs {k409:?}"
    );
    assert_eq!(k404, vec!["code", "message", "success"]);

    env.cleanup().await;
}

/// 每个响应都带 `x-request-id`，且两次请求的 ID 不同
#[actix_web::test]
async fn responses_carry_unique_request_id() {
    let env = TestEnv::new().await;
    let app = app!(env.state());

    let mut ids = Vec::new();
    for _ in 0..2 {
        let resp =
            test::call_service(&app, test::TestRequest::get().uri("/health").to_request()).await;
        assert_eq!(resp.status(), 200);
        let id = resp
            .headers()
            .get("x-request-id")
            .expect("响应缺少 x-request-id 头")
            .to_str()
            .unwrap()
            .to_string();
        assert!(
            id.parse::<u64>().is_ok_and(|n| n > 0),
            "x-request-id 应当是正整数，实际是「{id}」"
        );
        ids.push(id);
    }

    assert_ne!(ids[0], ids[1], "两次请求拿到了同一个 request id：{ids:?}");

    env.cleanup().await;
}

// ============================================
// 【已知局限】第 39 天补
// ============================================
//
// 1. 标签写入的**事务回滚**没有被真正触发过：
//    要触发回滚，得让 `INSERT task_tags` 在归属校验通过之后失败，
//    而正常请求构造不出这种场景（归属校验在前，FK 冲突在后）。
//    所以目前只验证了"幂等"（重复贴不产生第二行），
//    没有验证"第二步失败时第一步不留下标签行"——这一条仍是**材料**而非实测。
// 2. `EXPLAIN QUERY PLAN` 的结论在 `examples/explain_plan.rs` 里可复现，
//    但它输出的是"给人读的证据"，不适合写成断言（计划会随行数变化）。
// 3. `actix_web::test` 绕过 TCP：测不到端口占用、超时、多 worker 并发。
//    并发能力由 `scripts/load_test.sh` 覆盖，但它是脚本，**不在 cargo test 里**——
//    "测试全绿"并不包含并发结论，这一点必须说清楚。
// 4. `internal_error_is_json_and_hides_sql` 靠 `DROP TABLE tasks` 造 500。
//    它验证的是"错误层的出口"，不是"这条 SQL 会不会失败"。
//    如果哪天 handler 改成了不碰 tasks 表，这条用例会变成"200"而不是"500"，
//    届时应当换一张表来触发，而不是删掉用例。
// 5. `busy_timeout` 的对照实验在 `examples/hold_lock.rs` + README §5.9 里，
//    同样是**可复现实验**而非断言：它依赖"锁被占住"这个时序，写成测试会不稳定。
