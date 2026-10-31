// ============================================
// task-manager 数据库层
// 日期：2026-11-01（第37天）
// ============================================

/*
【职责】
  1. 建连接池（并配好三个必须显式设置的开关）
  2. 跑迁移
业务逻辑一律不在这里。handler 只拿 pool，不关心连接怎么来的。
*/

use sqlx::sqlite::{SqliteConnectOptions, SqliteJournalMode, SqlitePool, SqlitePoolOptions};
use std::str::FromStr;
use std::time::Duration;

/// 初始化数据库：建池 → 跑迁移
///
/// # 参数
/// - `url`: 如 `"sqlite:task-manager.db"`
///
/// # 返回
/// 可用连接池；迁移失败即返回错误（启动失败，而不是等到第一个请求）
///
/// # 三个必须显式设置的开关
/// `SqliteConnectOptions::from_str` 出来的是 SQLite 的**默认值**，
/// 而 SQLite 的默认值有三处不适合服务端：
///   1. `journal_mode` 默认 DELETE —— 读写互斥；WAL 才能读不阻塞写
///   2. `foreign_keys` 默认 **OFF** —— 外键约束形同虚设（2001 年的兼容遗留）
///   3. `busy_timeout` 默认 0 —— 撞上写锁立刻 SQLITE_BUSY，而不是等一下
/// 三个都容易漏，尤其第 2 个：**外键写了却没用，是一个静默错误**。
pub async fn init_db(url: &str) -> Result<SqlitePool, Box<dyn std::error::Error>> {
    // 默认 5 秒。单独开一个函数是为了让压测能把它设成 0，
    // 从而**看到** `SQLITE_BUSY` 到底长什么样——见 README §5.8。
    init_db_with_busy_timeout(url, Duration::from_secs(5)).await
}

/// 显式指定 `busy_timeout` 的版本
///
/// # 为什么值得留一个可调参数
/// `busy_timeout` 是"撞到写锁时最多等多久"。默认 5 秒意味着：
/// 短写事务的争用会被**吸收**，你在日志里什么都看不见。
/// 看不见不等于不存在——把它设成 0，同样的并发立刻会冒出
/// `database is locked`。这条对照实验比任何一句"busy_timeout 很重要"都有说服力。
pub async fn init_db_with_busy_timeout(
    url: &str,
    busy_timeout: Duration,
) -> Result<SqlitePool, Box<dyn std::error::Error>> {
    let opts = SqliteConnectOptions::from_str(url)?
        .create_if_missing(true)
        .journal_mode(SqliteJournalMode::Wal)
        .foreign_keys(true)
        .busy_timeout(busy_timeout);

    let pool = SqlitePoolOptions::new()
        // SQLite 的写操作全局串行，连接给太多没有收益
        .max_connections(5)
        // 池空时不无限挂起
        .acquire_timeout(Duration::from_secs(3))
        .connect_with(opts)
        .await?;

    // `migrate!` 在**编译期**把 migrations 目录读进二进制，运行期幂等执行。
    // 好处：部署时不用单独跑迁移脚本，也不会出现"代码新了、库还旧"。
    sqlx::migrate!("./migrations").run(&pool).await?;

    Ok(pool)
}
