// ============================================
// task-manager 请求处理器
// 日期：2026-11-01（第37天）
// ============================================

/*
【这一层只做三件事】

    1. 校验（形状对不对、取值范围对不对）
    2. 鉴权（这个资源是不是你的）
    3. 调 SQL（参数绑定，绝不拼字符串）

【`CurrentUser` 出现在签名里 = 这个接口必须认证】
这是本层最重要的设计：认证不是靠"记得写检查"，而是靠**类型**。
handler 少了 `user: CurrentUser` 就是少了个参数，编译不过；
有了它拿不到身份就直接 401，不会"以空身份继续跑"。
*/

use actix_web::{HttpResponse, web};
use serde_json::json;
use sqlx::error::DatabaseError;
use sqlx::sqlite::SqlitePool;

use crate::error::ApiError;
use crate::models::{
    ApiResponse, ChangeStatus, CreateProject, CreateTask, ErrorResponse, PatchTask, Project,
    TagBody, Task, TaskStatus, TaskWithTags,
};
use crate::mw::CurrentUser;
use crate::state::AppState;

/// 任务查询的列清单
///
/// 抽成常量是因为它在"列表"和"插入(RETURNING)"两处必须完全一致——
/// 漏一列，`FromRow` 会在运行时报 `ColumnNotFound`，而且只在某一条路径上炸。
const TASK_COLUMNS: &str = "id, project_id, title, status, priority, assignee_id, \
                            due_at, created_at, updated_at";

/// 带表别名的版本，**用于 JOIN**
///
/// 【为什么必须有第二份】
/// `tasks` 和 `projects` 都有 `id` 列，`SELECT id ... JOIN projects`
/// 会让 SQLite 直接报 `ambiguous column name: id`（运行时错误，不是编译错误）。
/// 这类问题的表现是"这个接口 500、别的接口正常"，且日志里只有一句 SQL 错误，
/// 所以别名的成本远低于排查成本。
const TASK_COLUMNS_ALIASED: &str = "t.id, t.project_id, t.title, t.status, t.priority, \
                                    t.assignee_id, t.due_at, t.created_at, t.updated_at";

// ============================================
// 健康检查（不认证）
// ============================================

/// GET /health
///
/// 【为什么要真的查一次库】
/// 进程活着 ≠ 服务可用。数据库文件被删、权限变了、磁盘满了，
/// 进程照样"活着"，但每个业务请求都会 500。
/// 健康检查的价值就在于区分这两件事，否则它只是个摆设。
pub async fn health(state: web::Data<AppState>) -> HttpResponse {
    match sqlx::query_scalar::<_, i64>("SELECT 1")
        .fetch_one(&state.pool)
        .await
    {
        Ok(_) => HttpResponse::Ok().json(ApiResponse::ok(json!({
            "status": "ok",
            "db": "ok"
        }))),
        Err(e) => {
            println!("[HEALTH] 数据库不可用: {e}");
            HttpResponse::ServiceUnavailable().json(json!({
                "success": false,
                "data": { "status": "degraded", "db": "down" }
            }))
        }
    }
}

// ============================================
// 项目
// ============================================

/// GET /projects
///
/// 只返回**自己**的项目。`WHERE owner_id = ?` 而不是"查全部再过滤"——
/// 后者一旦漏写过滤，就是越权读取全部数据。
pub async fn list_projects(
    state: web::Data<AppState>,
    user: CurrentUser,
) -> Result<HttpResponse, ApiError> {
    let projects = sqlx::query_as::<_, Project>(
        "SELECT id, owner_id, name, description, created_at
           FROM projects
          WHERE owner_id = ?
          ORDER BY id",
    )
    .bind(user.0)
    .fetch_all(&state.pool)
    .await?;

    Ok(HttpResponse::Ok().json(ApiResponse::ok(projects)))
}

/// POST /projects
pub async fn create_project(
    state: web::Data<AppState>,
    user: CurrentUser,
    body: web::Json<CreateProject>,
) -> Result<HttpResponse, ApiError> {
    let name = body.name.trim();
    if name.is_empty() {
        return Err(ApiError::bad_request("invalid_name", "项目名不能为空"));
    }
    if name.chars().count() > 64 {
        return Err(ApiError::bad_request(
            "invalid_name",
            "项目名不能超过 64 个字符",
        ));
    }

    let res = sqlx::query_as::<_, Project>(
        "INSERT INTO projects (owner_id, name, description)
              VALUES (?, ?, ?)
           RETURNING id, owner_id, name, description, created_at",
    )
    .bind(user.0)
    .bind(name)
    .bind(&body.description)
    .fetch_one(&state.pool)
    .await;

    match res {
        Ok(p) => Ok(HttpResponse::Created().json(ApiResponse::ok(p))),
        // 唯一约束是 `UNIQUE(owner_id, name)`：同一个人不能有两个同名项目，
        // 不同用户之间互不影响。
        //
        // 【绝不解析错误字符串】
        // `e.to_string().contains("UNIQUE")` 依赖 SQLite 的文案，
        // 换个数据库、换个版本就失效。`is_unique_violation()` 是 sqlx
        // 提供的**语义化**判断，它在底层还是看错误码，但把差异封装掉了。
        //
        // 【第 39 天：这里改成走 ApiError】
        // 原本 409 是手搓的 `HttpResponse::Conflict().json(...)`，500 是
        // `ErrorInternalServerError`（纯文本 body）。同一个接口的两种失败
        // 给了客户端两种形状，这就是"错误层不统一"的具体代价。
        Err(e)
            if e.as_database_error()
                .is_some_and(|d: &dyn DatabaseError| d.is_unique_violation()) =>
        {
            Err(ApiError::conflict(
                "duplicate_name",
                format!("你已经有一个叫「{name}」的项目了"),
            ))
        }
        Err(e) => Err(ApiError::from(e)),
    }
}

// ============================================
// 任务
// ============================================

/// 列表查询参数（全部可选）
///
/// 【为什么 WHERE 条件要"按需拼"，而不是 `WHERE (? IS NULL OR status = ?)`】
/// 后者看着优雅，但 `OR` 会让**复合索引的第二个列失效**：
/// 条件变成 `? IS NULL OR status = ?` 之后，`status` 不再是一个确定值，
/// SQLite 只能按 `project_id` 定位再逐行判 `OR`。
///
/// 【实测，2026-11-02，`cargo run --example explain_plan`，2003 行】
/// ```text
/// WHERE project_id = ? AND status = ?
///   → SEARCH tasks USING INDEX idx_tasks_project_status (project_id=? AND status=?)
///
/// WHERE project_id = ? AND (? IS NULL OR status = ?)
///   → SEARCH tasks USING INDEX idx_tasks_project (project_id=?)   ← 只用上前缀
/// ```
/// 注意：并没有"完全放弃索引"，退化为只用 `project_id` 那一列——
/// 见过的"OR 一定导致全表扫描"是**过度概括**，真正的代价是
/// 复合索引后半段白建了（外加每次多判一次 `OR`）。
///
/// ⚠️ 拼的只有**条件片段**，值一律用 `?` 绑定；绝不把值拼进 SQL。
#[derive(serde::Deserialize, Debug)]
pub struct TaskQuery {
    pub status: Option<String>,
    pub priority: Option<i64>,
    #[serde(default = "TaskQuery::default_limit")]
    pub limit: i64,
    #[serde(default)]
    pub offset: i64,
}

impl TaskQuery {
    fn default_limit() -> i64 {
        20
    }
}

/// GET /projects/{project_id}/tasks
pub async fn list_tasks(
    state: web::Data<AppState>,
    user: CurrentUser,
    path: web::Path<i64>,
    query: web::Query<TaskQuery>,
) -> Result<HttpResponse, ApiError> {
    let project_id = path.into_inner();

    // 先确认"这个项目是你的"。
    //
    // 为什么不直接 `SELECT tasks WHERE project_id = ?`？
    // 那样 A 能拿到 B 的任务——项目 id 是自增的，猜起来毫无难度。
    //
    // 为什么越权返回 404 而不是 403？
    // 403 等于确认"这个项目存在，只是不属于你"，
    // 攻击者据此就能枚举出别人的项目 id。404 什么也不泄露。
    if owned_project(&state.pool, project_id, user.0)
        .await?
        .is_none()
    {
        return Err(ApiError::NotFound("项目"));
    }

    // 分页上限硬顶在 100：没有上限的 limit 等于给了一条
    // "一次拖垮数据库"的公开接口
    let limit = query.limit.clamp(1, 100);
    let offset = query.offset.max(0);

    let mut sql = format!("SELECT {TASK_COLUMNS} FROM tasks WHERE project_id = ?");
    if query.status.is_some() {
        sql.push_str(" AND status = ?");
    }
    if query.priority.is_some() {
        sql.push_str(" AND priority = ?");
    }
    sql.push_str(" ORDER BY priority, id LIMIT ? OFFSET ?");

    let mut q = sqlx::query_as::<_, Task>(&sql).bind(project_id);
    if let Some(s) = &query.status {
        q = q.bind(s);
    }
    if let Some(p) = query.priority {
        q = q.bind(p);
    }
    let tasks = q.bind(limit).bind(offset).fetch_all(&state.pool).await?;

    Ok(HttpResponse::Ok().json(ApiResponse::ok(tasks)))
}

/// POST /projects/{project_id}/tasks
pub async fn create_task(
    state: web::Data<AppState>,
    user: CurrentUser,
    path: web::Path<i64>,
    body: web::Json<CreateTask>,
) -> Result<HttpResponse, ApiError> {
    let project_id = path.into_inner();

    let title = body.title.trim();
    if title.is_empty() {
        return Err(ApiError::bad_request("invalid_title", "任务标题不能为空"));
    }

    // 应用层校验 + 数据库 CHECK 双层
    //
    // 应用层这一层不是多余的：数据库 CHECK 拒绝时只能给出一句
    // "CHECK constraint failed: priority BETWEEN 1 AND 4"，
    // 既不适合直接给用户看，也没法告诉他是哪个字段填错了。
    // 但数据库那一层**仍然要留着**——它是最后一道防线，
    // 挡住绕过 API 直接写库的脚本。
    if let Some(p) = body.priority {
        if !(1..=4).contains(&p) {
            return Err(ApiError::bad_request(
                "invalid_priority",
                "priority 只能是 1(最高) ~ 4(最低)",
            ));
        }
    }

    if owned_project(&state.pool, project_id, user.0)
        .await?
        .is_none()
    {
        return Err(ApiError::NotFound("项目"));
    }

    // `COALESCE(?, 2)`：参数为 NULL 时回落到数据库默认值 2。
    // 写成"在 Rust 里 `unwrap_or(2)`"也能工作，但那样默认值就有了两份，
    // 改一处漏一处；让数据库当唯一事实来源更稳。
    let sql = format!(
        "INSERT INTO tasks (project_id, title, priority, due_at)
              VALUES (?, ?, COALESCE(?, 2), ?)
           RETURNING {TASK_COLUMNS}"
    );
    let task = sqlx::query_as::<_, Task>(&sql)
        .bind(project_id)
        .bind(title)
        .bind(body.priority)
        .bind(&body.due_at)
        .fetch_one(&state.pool)
        .await?;

    Ok(HttpResponse::Created().json(ApiResponse::ok(task)))
}

// ============================================
// 内部工具
// ============================================

/// 查"属于该用户的项目"
///
/// 返回 `Option` 而不是 `Result<bool>`：调用方需要的不只是"有没有权限"，
/// 后面往往还要用项目本身（比如取 owner_id 做二次判断）。
/// 这里把"查"和"判权"合成一个动作，少一次查询往返。
async fn owned_project(
    pool: &SqlitePool,
    project_id: i64,
    user_id: i64,
) -> Result<Option<Project>, sqlx::Error> {
    sqlx::query_as::<_, Project>(
        "SELECT id, owner_id, name, description, created_at
           FROM projects
          WHERE id = ? AND owner_id = ?",
    )
    .bind(project_id)
    .bind(user_id)
    .fetch_optional(pool)
    .await
}

/// 查"属于该用户的任务"
///
/// 任务的归属要**穿过项目**才能判断：`tasks → projects.owner_id`。
/// 所以是 `JOIN` 而不是一个 `WHERE user_id = ?`——任务表里根本没有 owner 列。
///
/// 【为什么不在 tasks 表里冗余一个 owner_id】
/// 冗余字段会带来"两处不一致"的可能（项目转手后任务还指着旧主人）。
/// `JOIN` 是每次多一次索引查找，而一致性错误是永久的。
async fn owned_task(
    pool: &SqlitePool,
    task_id: i64,
    user_id: i64,
) -> Result<Option<Task>, sqlx::Error> {
    let sql = format!(
        "SELECT {TASK_COLUMNS_ALIASED}
           FROM tasks t
           JOIN projects p ON p.id = t.project_id
          WHERE t.id = ? AND p.owner_id = ?"
    );
    sqlx::query_as::<_, Task>(&sql)
        .bind(task_id)
        .bind(user_id)
        .fetch_optional(pool)
        .await
}

/// 读某任务的标签名（已排序）
async fn task_tags(pool: &SqlitePool, task_id: i64) -> Result<Vec<String>, sqlx::Error> {
    sqlx::query_scalar::<_, String>(
        "SELECT g.name
           FROM tags g
           JOIN task_tags tt ON tt.tag_id = g.id
          WHERE tt.task_id = ?
          ORDER BY g.name",
    )
    .bind(task_id)
    .fetch_all(pool)
    .await
}

// ============================================
// 第 38 天：任务单条读写
// ============================================

/// GET /tasks/{task_id}
pub async fn get_task(
    state: web::Data<AppState>,
    user: CurrentUser,
    path: web::Path<i64>,
) -> Result<HttpResponse, ApiError> {
    let task_id = path.into_inner();
    let Some(task) = owned_task(&state.pool, task_id, user.0).await? else {
        return Err(ApiError::NotFound("任务"));
    };
    let tags = task_tags(&state.pool, task_id).await?;

    Ok(HttpResponse::Ok().json(ApiResponse::ok(TaskWithTags { task, tags })))
}

/// PATCH /tasks/{task_id}
///
/// 【为什么状态不在这里改】
/// 见 `patch_task_status` 的说明：状态有迁移规则，必须走单独入口。
/// 这里能改的只有"自由字段"。
pub async fn patch_task(
    state: web::Data<AppState>,
    user: CurrentUser,
    path: web::Path<i64>,
    body: web::Json<PatchTask>,
) -> Result<HttpResponse, ApiError> {
    let task_id = path.into_inner();

    if let Some(t) = &body.title {
        if t.trim().is_empty() {
            return Err(ApiError::bad_request("invalid_title", "标题不能为空"));
        }
        if t.chars().count() > 200 {
            return Err(ApiError::bad_request(
                "invalid_title",
                "标题不能超过 200 个字符",
            ));
        }
    }
    if let Some(p) = body.priority {
        if !(1..=4).contains(&p) {
            return Err(ApiError::bad_request(
                "invalid_priority",
                "priority 只能是 1(最高) ~ 4(最低)",
            ));
        }
    }

    if owned_task(&state.pool, task_id, user.0).await?.is_none() {
        return Err(ApiError::NotFound("任务"));
    }

    // 【一波 UPDATE 搞定三态】
    //
    // `COALESCE(?, title)`：`?` 为 NULL 时保留原值 → 对应"字段没出现"。
    // `CASE WHEN ? = 1 THEN ? ELSE assignee_id END`：
    //   第一个 `?` 是"这个字段出现过吗"（0/1），
    //   第二个 `?` 是要写入的值（可以是 NULL）→ 对应"显式设为空"。
    //
    // 为什么不用"读出来 → 在 Rust 里改 → 整行写回"？
    // 那个写法有**丢失更新**：两个并发 PATCH 各改不同字段时，
    // 后写的会用自己的旧快照覆盖前一个的结果。
    // 一条 UPDATE 只写被指定的列，并发改不同字段互不影响。
    let sql = format!(
        "UPDATE tasks
            SET title       = COALESCE(?, title),
                priority    = COALESCE(?, priority),
                assignee_id = CASE WHEN ? = 1 THEN ? ELSE assignee_id END,
                due_at      = CASE WHEN ? = 1 THEN ? ELSE due_at END,
                updated_at  = datetime('now')
          WHERE id = ?
      RETURNING {TASK_COLUMNS}"
    );
    let has_assignee = i64::from(body.assignee_id.is_some());
    let has_due = i64::from(body.due_at.is_some());

    let task = sqlx::query_as::<_, Task>(&sql)
        .bind(body.title.as_deref().map(str::trim))
        .bind(body.priority)
        .bind(has_assignee)
        .bind(body.assignee_id.flatten())
        .bind(has_due)
        .bind(body.due_at.clone().flatten())
        .bind(task_id)
        .fetch_one(&state.pool)
        .await?;

    let tags = task_tags(&state.pool, task_id).await?;

    Ok(HttpResponse::Ok().json(ApiResponse::ok(TaskWithTags { task, tags })))
}

/// DELETE /tasks/{task_id}
pub async fn delete_task(
    state: web::Data<AppState>,
    user: CurrentUser,
    path: web::Path<i64>,
) -> Result<HttpResponse, ApiError> {
    let task_id = path.into_inner();
    if owned_task(&state.pool, task_id, user.0).await?.is_none() {
        return Err(ApiError::NotFound("任务"));
    }

    // 条件里再带上归属，避免"校验和删除之间被别人删掉/转移"的窗口。
    // 影响行数为 0 → 说明这期间状态变了，按 404 处理（等价于不存在）。
    let affected = sqlx::query(
        "DELETE FROM tasks
          WHERE id = ?
            AND project_id IN (SELECT id FROM projects WHERE owner_id = ?)",
    )
    .bind(task_id)
    .bind(user.0)
    .execute(&state.pool)
    .await?
    .rows_affected();

    if affected == 0 {
        return Err(ApiError::NotFound("任务"));
    }
    Ok(HttpResponse::NoContent().finish())
}

/// PATCH /tasks/{task_id}/status
///
/// 状态机唯一入口。合法性由 `TaskStatus::can_transition_to` 决定。
pub async fn patch_task_status(
    state: web::Data<AppState>,
    user: CurrentUser,
    path: web::Path<i64>,
    body: web::Json<ChangeStatus>,
) -> Result<HttpResponse, ApiError> {
    let task_id = path.into_inner();

    // 未知状态值是"请求不合法"（400），不是"服务器出错"（500）：
    // 客户端写错了枚举值，不该让服务端日志里堆满 5xx
    let Some(next) = TaskStatus::parse(body.status.trim()) else {
        return Err(ApiError::bad_request(
            "invalid_status",
            "status 只能是 todo / doing / done",
        ));
    };

    let Some(task) = owned_task(&state.pool, task_id, user.0).await? else {
        return Err(ApiError::NotFound("任务"));
    };

    // 数据库里出现了枚举外的值 → 说明有人绕过 API 写库，
    // 或 CHECK 约束没生效（`foreign_keys` 那类"静默失效"的坑）
    //
    // 【这里为什么不用 ApiError】
    // `ApiError::Internal` 装的是 `sqlx::Error`，语义是"下层炸了，细节不外传"。
    // 而这里是**我们自己判断出数据坏了**，文案已经决定好给客户端看。
    // 硬塞进 `Internal(_)` 反而要造一个假错误。它响应形状仍是同一套 JSON，
    // 只是不走 `From<sqlx::Error>` 那条路。
    let Some(current) = TaskStatus::parse(&task.status) else {
        println!("[DATA] 任务 {task_id} 的 status 非法: {}", task.status);
        return Ok(HttpResponse::InternalServerError()
            .json(ErrorResponse::new("corrupted_data", "任务状态异常")));
    };

    if !current.can_transition_to(next) {
        return Err(ApiError::conflict(
            "illegal_transition",
            format!(
                "不允许从 {} 直接变成 {}（必须 todo → doing → done）",
                current.as_str(),
                next.as_str()
            ),
        ));
    }

    let sql = format!(
        "UPDATE tasks SET status = ?, updated_at = datetime('now')
          WHERE id = ? RETURNING {TASK_COLUMNS}"
    );
    let task = sqlx::query_as::<_, Task>(&sql)
        .bind(next.as_str())
        .bind(task_id)
        .fetch_one(&state.pool)
        .await?;

    Ok(HttpResponse::Ok().json(ApiResponse::ok(task)))
}

// ============================================
// 第 38 天：标签（多对多）
// ============================================

/// POST /tasks/{task_id}/tags
///
/// 【三步必须在同一个事务里】
/// `INSERT tags`（可能新建）→ `SELECT tag_id` → `INSERT task_tags`。
/// 若在第一步和第三步之间崩溃，会留下一个"没人用"的标签行；
/// 更糟的是第三步失败（比如任务刚被删）时，标签行已经写进去了。
/// 事务保证"要么都成，要么都不成"。
pub async fn add_tag(
    state: web::Data<AppState>,
    user: CurrentUser,
    path: web::Path<i64>,
    body: web::Json<TagBody>,
) -> Result<HttpResponse, ApiError> {
    let task_id = path.into_inner();
    let name = body.name.trim();
    if name.is_empty() || name.chars().count() > 32 {
        return Err(ApiError::bad_request("invalid_tag", "标签名需 1~32 个字符"));
    }

    if owned_task(&state.pool, task_id, user.0).await?.is_none() {
        return Err(ApiError::NotFound("任务"));
    }

    let mut tx = state.pool.begin().await?;

    // `ON CONFLICT(name) DO NOTHING`：标签是全局共享的，
    // 多个人贴同一个标签名只应有一行。
    // 不用"先 SELECT 再 INSERT"——那在并发下会撞唯一约束。
    sqlx::query("INSERT INTO tags (name) VALUES (?) ON CONFLICT(name) DO NOTHING")
        .bind(name)
        .execute(&mut *tx)
        .await?;

    let tag_id: i64 = sqlx::query_scalar("SELECT id FROM tags WHERE name = ?")
        .bind(name)
        .fetch_one(&mut *tx)
        .await?;

    // 中间表复合主键天然去重：重复贴同一个标签不报错，也不产生第二行
    sqlx::query("INSERT INTO task_tags (task_id, tag_id) VALUES (?, ?) ON CONFLICT DO NOTHING")
        .bind(task_id)
        .bind(tag_id)
        .execute(&mut *tx)
        .await?;

    tx.commit().await?;

    let tags = task_tags(&state.pool, task_id).await?;

    Ok(HttpResponse::Ok().json(ApiResponse::ok(tags)))
}

/// DELETE /tasks/{task_id}/tags/{name}
///
/// 只解除关联，**不删标签本身**——同一个标签可能挂在别的任务上。
/// 这是多对多的常规语义：删的是"边"，不是"点"。
pub async fn remove_tag(
    state: web::Data<AppState>,
    user: CurrentUser,
    path: web::Path<(i64, String)>,
) -> Result<HttpResponse, ApiError> {
    let (task_id, name) = path.into_inner();

    if owned_task(&state.pool, task_id, user.0).await?.is_none() {
        return Err(ApiError::NotFound("任务"));
    }

    sqlx::query(
        "DELETE FROM task_tags
          WHERE task_id = ?
            AND tag_id = (SELECT id FROM tags WHERE name = ?)",
    )
    .bind(task_id)
    .bind(&name)
    .execute(&state.pool)
    .await?;

    let tags = task_tags(&state.pool, task_id).await?;

    Ok(HttpResponse::Ok().json(ApiResponse::ok(tags)))
}

// ============================================
// 第 38 天：删除项目（验证 CASCADE）
// ============================================

/// DELETE /projects/{project_id}
///
/// 项目下的任务、任务标签关联都由数据库的 `ON DELETE CASCADE` 处理——
/// **前提是连接上开了 `foreign_keys(true)`**（见 `db.rs`）。
/// 这个前提没满足时，MySQL 的老手会觉得"外键是自动生效的"，
/// 而在 SQLite 上实际什么都不会发生：任务变成孤儿行，还不报错。
pub async fn delete_project(
    state: web::Data<AppState>,
    user: CurrentUser,
    path: web::Path<i64>,
) -> Result<HttpResponse, ApiError> {
    let project_id = path.into_inner();

    let affected = sqlx::query("DELETE FROM projects WHERE id = ? AND owner_id = ?")
        .bind(project_id)
        .bind(user.0)
        .execute(&state.pool)
        .await?
        .rows_affected();

    if affected == 0 {
        return Err(ApiError::NotFound("项目"));
    }
    Ok(HttpResponse::NoContent().finish())
}
