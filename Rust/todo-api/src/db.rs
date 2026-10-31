// ============================================
// Todo API - 数据库层
// 日期：2026-10-29（第33天）
// ============================================

/*
【这一层的职责】

  1. 建立连接池（WAL、外键、超时）
  2. 执行迁移（表结构的唯一合法入口）
  3. 一次性导入旧的 todos.json（如果存在且表为空）

业务逻辑一律不放在这里。handler 只调 `pool`，不关心连接怎么来的。
*/

use crate::models::Todo;

use sqlx::sqlite::{SqliteConnectOptions, SqliteJournalMode, SqlitePool, SqlitePoolOptions};
use sqlx::Row;

use std::str::FromStr;
use std::time::Duration;

/// 初始化数据库：建池 → 跑迁移
///
/// # 参数
/// - `url`: SQLite 连接串，如 `"sqlite:todos.db"`
///
/// # 返回
/// 可用的连接池；迁移失败会返回错误（**启动即失败**，而不是等到第一个请求）
pub async fn init_db(url: &str) -> Result<SqlitePool, Box<dyn std::error::Error>> {
    // 用 ConnectOptions 而不是字符串 DSN，是因为下面三个开关
    // 在 DSN 里写起来不直观，而且容易漏（尤其 foreign_keys）
    let opts = SqliteConnectOptions::from_str(url)?
        // 库文件不存在就创建。等价于 DSN 里的 ?mode=rwc
        .create_if_missing(true)
        // WAL：读不被写阻塞。单机后端这是必开项
        .journal_mode(SqliteJournalMode::Wal)
        // ⚠️ SQLite 的外键约束默认是**关闭**的，必须显式打开。
        // 这是 2001 年为了向后兼容留下的默认值，不是 bug。
        .foreign_keys(true)
        // 写锁冲突时最多等 5 秒再报 SQLITE_BUSY，避免瞬时并发直接失败
        .busy_timeout(Duration::from_secs(5));

    let pool = SqlitePoolOptions::new()
        // SQLite 写操作全局串行，连接给太多没有收益
        .max_connections(5)
        // 池空时不无限挂起，3 秒拿不到连接就报 PoolTimedOut
        .acquire_timeout(Duration::from_secs(3))
        .connect_with(opts)
        .await?;

    // 迁移文件由 `migrate!` 宏在**编译期**读进二进制，运行期幂等执行
    sqlx::migrate!("./migrations").run(&pool).await?;

    Ok(pool)
}

/// 若 `todos.json` 存在且 tasks 表为空，把旧数据导入数据库
///
/// # 为什么需要这一步
/// 第 27 天的持久化把数据写在 `todos.json` 里。换成数据库后，
/// 如果不导入，用户会看到"数据凭空消失了"。
/// **迁移数据是迁移方案的一部分，不是可选项。**
///
/// # 返回
/// 导入的行数（0 表示无需导入）
pub async fn import_json_if_empty(
    pool: &SqlitePool,
    json_path: &str,
) -> Result<usize, Box<dyn std::error::Error>> {
    // 表已有数据 → 说明要么已导入过，要么是正常运行的库，两种情况都不该覆盖
    let row = sqlx::query("SELECT COUNT(*) AS cnt FROM todos")
        .fetch_one(pool)
        .await?;
    let existing: i64 = row.get("cnt");
    if existing > 0 {
        return Ok(0);
    }

    let path = std::path::Path::new(json_path);
    if !path.exists() {
        return Ok(0); // 首次运行，没有旧数据，正常情况
    }

    let raw = std::fs::read_to_string(path)?;
    let old: Vec<Todo> = serde_json::from_str(&raw)?;
    if old.is_empty() {
        return Ok(0);
    }

    // 整个导入放在一个事务里：中途失败不会留下半份数据
    let mut tx = pool.begin().await?;
    for t in &old {
        sqlx::query(
            // 时间戳交给 SQL 生成：`datetime('now')` 返回 UTC 的
            // "YYYY-MM-DD HH:MM:SS"，不引入额外的时间库依赖。
            // 旧数据没有时间戳，用导入时刻补齐（日志里会标注这是推定值）。
            "INSERT INTO todos (id, title, completed, created_at, updated_at)
             VALUES (?, ?, ?, datetime('now'), datetime('now'))",
        )
        .bind(t.id)
        .bind(&t.title)
        .bind(t.completed)
        .execute(&mut *tx)
        .await?;
    }
    tx.commit().await?;

    // 导入完成后把旧文件改名存档，避免下次启动重复判断
    // 用 rename 而不是 remove：万一导入的判断有误，数据还能找回
    let archive = format!("{}.imported", json_path);
    if let Err(e) = std::fs::rename(path, &archive) {
        eprintln!("⚠️  旧数据文件改名失败（数据已导入，可手动处理）: {}", e);
    } else {
        println!("📦 旧数据已导入并归档为 {}", archive);
    }

    Ok(old.len())
}
