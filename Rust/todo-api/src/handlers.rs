// ============================================
// Todo API - 请求处理函数
// 日期：2026-06-21（第25天）创建
// 更新：2026-06-22（第27天：自动保存）
// 更新：2026-10-29（第33天：改为 SQLite）
// ============================================

use crate::models::{ApiResponse, CreateTodo, ErrorResponse, Todo, UpdateTodo};
use crate::state::AppState;

use actix_web::{web, HttpResponse, Responder};

// --------------------------------------------
// 通用错误响应
// --------------------------------------------

/// 把数据库错误统一翻译成 500
///
/// 【为什么细节不返回给客户端】
/// 数据库错误信息里可能包含表名、列名、SQL 片段——
/// 那是给攻击者的地图。详细内容进日志（服务端），
/// 客户端只看到 "内部数据库错误"。
fn internal_error(e: sqlx::Error) -> HttpResponse {
    eprintln!("❌ 数据库错误: {}", e);
    HttpResponse::InternalServerError().json(ErrorResponse::error("内部数据库错误"))
}

fn not_found() -> HttpResponse {
    HttpResponse::NotFound().json(ErrorResponse::error("Todo not found"))
}

// --------------------------------------------
// GET /todos - 获取所有任务
// --------------------------------------------

/// 获取所有任务
///
/// # 说明
/// 第 27 天的实现是"加锁 → 克隆 Vec → 解锁 → 序列化"。
/// 现在只有一步：查数据库。**没有锁，也没有内存副本。**
///
/// 代价是每次请求都要走一次 SQL——但这正是数据库该做的事：
/// 它保证你看到的永远是当前真相，而不是某个线程缓存的快照。
pub async fn get_all(state: web::Data<AppState>) -> impl Responder {
    match sqlx::query_as::<_, Todo>("SELECT id, title, completed FROM todos ORDER BY id")
        .fetch_all(&state.pool)
        .await
    {
        Ok(todos) => HttpResponse::Ok().json(ApiResponse::success(todos)),
        Err(e) => internal_error(e),
    }
}

// --------------------------------------------
// POST /todos - 创建新任务
// --------------------------------------------

/// 创建新任务
///
/// # 请求
/// ```json
/// { "title": "学习 Actix-web" }
/// ```
///
/// # 响应（201 Created）
/// ```json
/// { "success": true, "data": { "id": 1, "title": "学习 Actix-web", "completed": false } }
/// ```
///
/// # 与第 27 天实现的差异
/// - 旧：`todos.iter().max() + 1` 手工生成 id，然后整体写回文件
/// - 新：由数据库生成 id（`INTEGER PRIMARY KEY`），只写入这一行
///
/// **手工生成 id 在并发下必然重复**：两个请求同时读到 max=5，
/// 都算出 6，后写的覆盖先写的。数据库的自增是原子的，这类 bug 直接消失。
pub async fn create(
    state: web::Data<AppState>,
    new_todo: web::Json<CreateTodo>,
) -> impl Responder {
    let result = sqlx::query(
        "INSERT INTO todos (title, completed, created_at, updated_at)
         VALUES (?, 0, datetime('now'), datetime('now'))",
    )
    .bind(&new_todo.title)
    .execute(&state.pool)
    .await;

    match result {
        Ok(r) => {
            // `last_insert_rowid()` 是本次 INSERT 生成的主键
            let id = r.last_insert_rowid();
            let todo = Todo::new(id, new_todo.title.clone());
            HttpResponse::Created().json(ApiResponse::success(todo))
        }
        Err(e) => internal_error(e),
    }
}

// --------------------------------------------
// GET /todos/{id} - 获取单个任务
// --------------------------------------------

/// 获取单个任务
///
/// # 说明
/// 用 `fetch_optional` 而不是 `fetch_one`：
/// "查不到"是**业务上的正常分支**（→ 404），不是程序错误。
/// 用 `fetch_one` 会把 404 变成 500，这是新手最常犯的错误之一。
pub async fn get_by_id(state: web::Data<AppState>, id: web::Path<i64>) -> impl Responder {
    let todo_id = id.into_inner();

    let found = sqlx::query_as::<_, Todo>("SELECT id, title, completed FROM todos WHERE id = ?")
        .bind(todo_id)
        .fetch_optional(&state.pool)
        .await;

    match found {
        Ok(Some(todo)) => HttpResponse::Ok().json(ApiResponse::success(todo)),
        Ok(None) => not_found(),
        Err(e) => internal_error(e),
    }
}

// --------------------------------------------
// PUT /todos/{id} - 更新任务
// --------------------------------------------

/// 更新任务（支持部分更新）
///
/// # 请求
/// ```json
/// { "title": "新标题", "completed": true }
/// ```
/// 两个字段都可省略：
/// ```json
/// { "completed": true }
/// ```
///
/// # 实现要点：COALESCE 做部分更新
/// ```sql
/// UPDATE todos SET title = COALESCE(?, title), completed = COALESCE(?, completed)
/// ```
/// - 传了值 → 用新值
/// - 传 NULL → 保持原值
///
/// 这比"先 SELECT 出来、在 Rust 里合并、再 UPDATE 整行"少一次往返，
/// 也**避免了读-改-写之间的竞态窗口**。
///
/// # RETURNING
/// SQLite 3.35+ 支持 `RETURNING`，一条语句同时完成"改"和"取回改后的值"。
/// 这又是一次"少一次往返"，且天然原子。
pub async fn update(
    state: web::Data<AppState>,
    id: web::Path<i64>,
    update_data: web::Json<UpdateTodo>,
) -> impl Responder {
    let todo_id = id.into_inner();

    let updated = sqlx::query_as::<_, Todo>(
        "UPDATE todos
         SET title     = COALESCE(?, title),
             completed = COALESCE(?, completed),
             updated_at = datetime('now')
         WHERE id = ?
         RETURNING id, title, completed",
    )
    .bind(&update_data.title) // Option<&String> → NULL 或文本
    .bind(update_data.completed) // Option<bool> → NULL 或 0/1
    .bind(todo_id)
    .fetch_optional(&state.pool)
    .await;

    match updated {
        // 命中并返回了改后的行
        Ok(Some(todo)) => HttpResponse::Ok().json(ApiResponse::success(todo)),
        // 没有行被更新 —— id 不存在
        Ok(None) => not_found(),
        Err(e) => internal_error(e),
    }
}

// --------------------------------------------
// DELETE /todos/{id} - 删除任务
// --------------------------------------------

/// 删除任务
///
/// # 说明
/// 同样用 `RETURNING` 把"删"和"取回被删的行"合成一条语句。
///
/// 第 27 天的实现是 `position()` + `remove(index)`，需要先找到索引；
/// 数据库里 `DELETE ... WHERE id = ?` 直接表达意图，不需要知道位置。
pub async fn delete(state: web::Data<AppState>, id: web::Path<i64>) -> impl Responder {
    let todo_id = id.into_inner();

    let removed = sqlx::query_as::<_, Todo>(
        "DELETE FROM todos WHERE id = ? RETURNING id, title, completed",
    )
    .bind(todo_id)
    .fetch_optional(&state.pool)
    .await;

    match removed {
        Ok(Some(todo)) => HttpResponse::Ok().json(ApiResponse::success(todo)),
        // 删一张不存在的行不是错误，只是"没命中" → 404
        Ok(None) => not_found(),
        Err(e) => internal_error(e),
    }
}

// --------------------------------------------
// GET /health - 健康检查
// --------------------------------------------

/// 健康检查：确认进程活着 **且** 数据库可用
///
/// 【为什么不能只返回 200】
/// 进程活着但数据库挂了，负载均衡器仍会把流量打进来，
/// 每个请求都 500。这种"假健康"比直接 DOWN 更难排查。
/// 所以这里真的查一次库。
pub async fn health(state: web::Data<AppState>) -> impl Responder {
    match sqlx::query("SELECT 1").execute(&state.pool).await {
        Ok(_) => HttpResponse::Ok().json(serde_json::json!({
            "success": true,
            "data": { "status": "ok", "db": "ok" }
        })),
        Err(e) => {
            eprintln!("❌ 健康检查失败: {}", e);
            HttpResponse::ServiceUnavailable().json(serde_json::json!({
                "success": false,
                "error": "数据库不可用"
            }))
        }
    }
}

/*
【第 33 天迁移要点：旧实现里的问题在这里如何消失】

1. 「手工生成 ID」→ 数据库自增
   旧：`todos.iter().map(|t| t.id).max().unwrap_or(0) + 1`
   并发下两个请求算出同一个 id，后写覆盖先写。
   新：`INTEGER PRIMARY KEY` 由数据库原子分配。

2. 「整体覆写文件」→ 行级写入
   旧：`state.save_to_file()` 把整个 Vec 序列化后重写。
   改一条 = 写全部，且不是原子的（靠 tmp+rename 才勉强救回来）。
   新：`UPDATE ... WHERE id = ?` 只碰一行，事务性由数据库保证。

3. 「锁守卫不能跨 await」→ 问题不存在了
   旧：必须先 clone 数据、drop 锁，再 await 保存。
   新：`&state.pool` 可以直接跨 await（Pool 是 Send + Sync）。

4. 「保存失败只记日志」→ 语义正确了
   旧：内存已改、文件写失败 → 内存与磁盘不一致。
   新：INSERT/UPDATE 返回 Ok 就意味着**已落库**；
   返回 Err 就是真的没做成，直接 500 给客户端。

5. 仍然存在的并发问题
   PUT 现在是单条原子的 UPDATE，不会丢失更新。
   但如果将来实现"先 SELECT 校验业务规则再 UPDATE"，
   丢失更新会重新出现——那时才需要乐观锁（见第 32 天）。
   届时加 `version` 列即可，API 形状不用变。
*/
