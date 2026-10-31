// ============================================
// EXPLAIN QUERY PLAN 实验（第 38 天）
// ============================================
//
// 用途：回答"筛选查询到底有没有用上索引"。
//
// 跑法：
//     cargo run --example explain_plan
//
// 为什么单独开一个 example 而不是塞进测试：
//   1. 它输出的是**解释计划**，是给人读的证据，不是断言；
//   2. 它需要"足够多的行"才能让优化器的选择有代表性（这里造 2000 行）；
//   3. 结论要贴进学习日志，所以必须能一条命令复现。
//
// 【为什么要造 2000 行】
// SQLite 在只有 3 行的表上几乎总是选择全表扫描——
// 表小到不值得用索引时，那才是正确决策。
// 用 3 行数据验证"索引被用上了"会得出相反的错误结论。

use sqlx::Row;
use task_manager::db;

/// 打印一条查询的执行计划
async fn plan(pool: &sqlx::SqlitePool, title: &str, sql: &str, binds: &[i64]) {
    println!("\n── {title} ──");
    println!(
        "SQL: {}",
        sql.split_whitespace().collect::<Vec<_>>().join(" ")
    );
    // ⚠️ 必须先 `let` 出来：`sqlx::query(&format!(...))` 会让
    // `Query<'_>` 借用那个临时 String，而临时值在这条语句结束就被释放
    // → `error[E0716]: temporary value dropped while borrowed`。
    let full_sql = format!("EXPLAIN QUERY PLAN {sql}");
    let mut q = sqlx::query(&full_sql);
    for b in binds {
        q = q.bind(b);
    }
    match q.fetch_all(pool).await {
        Ok(rows) => {
            for r in rows {
                // SQLite 的 EXPLAIN QUERY PLAN 返回 4 列：
                // id / parent / notused / detail；只有 detail 有信息量
                let detail: String = r.try_get("detail").unwrap_or_default();
                println!("   {detail}");
            }
        }
        Err(e) => println!("   执行计划失败: {e}"),
    }
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    // 内存库即可：执行计划与"数据在哪个文件"无关
    let pool = db::init_db("sqlite::memory:").await?;

    // 造数据：2000 条任务挂在项目 1 下，状态轮转
    let mut tx = pool.begin().await?;
    for i in 0..2000 {
        let status = ["todo", "doing", "done"][i % 3];
        sqlx::query("INSERT INTO tasks (project_id, title, status, priority) VALUES (1, ?, ?, ?)")
            .bind(format!("任务 {i}"))
            .bind(status)
            .bind((i % 4 + 1) as i64)
            .execute(&mut *tx)
            .await?;
    }
    tx.commit().await?;

    let total: i64 = sqlx::query_scalar("SELECT COUNT(*) FROM tasks WHERE project_id = 1")
        .fetch_one(&pool)
        .await?;
    println!("准备完毕：项目 1 下共 {total} 条任务");

    // ── 1. 只有 project_id 条件 ──
    plan(
        &pool,
        "1) WHERE project_id = ? ORDER BY priority, id",
        "SELECT id FROM tasks WHERE project_id = ? ORDER BY priority, id LIMIT 20",
        &[1],
    )
    .await;

    // ── 2. project_id + status（复合索引的第二列）──
    plan(
        &pool,
        "2) WHERE project_id = ? AND status = ?",
        "SELECT id FROM tasks WHERE project_id = ? AND status = ? ORDER BY priority, id LIMIT 20",
        &[1, 0],
    )
    .await;

    // ── 3. 用 OR 表达"条件可选"（错误示范）──
    plan(
        &pool,
        "3) WHERE project_id = ? AND (? IS NULL OR status = ?)  ← 错误示范",
        "SELECT id FROM tasks WHERE project_id = ? AND (? IS NULL OR status = ?) \
         ORDER BY priority, id LIMIT 20",
        &[1, 1, 0],
    )
    .await;

    // ── 4. 越权校验用的 JOIN ──
    plan(
        &pool,
        "4) 归属校验：tasks JOIN projects",
        "SELECT t.id FROM tasks t JOIN projects p ON p.id = t.project_id \
         WHERE t.id = ? AND p.owner_id = ?",
        &[1, 1],
    )
    .await;

    println!(
        "\n解读要点：\n\
         - 出现 `USING INDEX` / `USING COVERING INDEX` 才算用上了索引；\n\
         - `SCAN tasks` 是全表扫描；\n\
         - `USE TEMP B-TREE FOR ORDER BY` 表示排序另开了一次临时结构；\n\
           复合索引 (project_id, status) 的第二列只帮筛选，不帮 `ORDER BY priority`。"
    );

    pool.close().await;
    Ok(())
}
