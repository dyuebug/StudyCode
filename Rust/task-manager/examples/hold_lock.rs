// ============================================
// 实验：人为占住 SQLite 写锁（第 39 天）
// 运行：cargo run --example hold_lock -- <库路径> <持锁秒数>
// 例：  cargo run --example hold_lock -- target/loadrun/busy0.db 3
// ============================================

/*
【这个实验要证明什么】

`busy_timeout` 是"撞到写锁时最多等多久"。线上日志里看不到
`SQLITE_BUSY`，**可能是**因为压根没撞上，也可能是被超时吸收了。
两种情况的结论完全不同：

    没撞上      → 现在的并发还能撑住
    被吸收了    → 并发再高一点就会开始 500，只是**现在还没到**

自然压测（`scripts/load_test.sh`，P=50，事务写）一次都没撞出来——
因为每条写事务只有几十微秒，撞上的概率太低。

所以这里换一种确定性的做法：**人为开一个写事务并睡几秒**。
写锁在此期间一直被占着，任何人来写都必须等。

【为什么用 `BEGIN IMMEDIATE` 而不是 `BEGIN`】
默认的 `BEGIN`（DEFERRED）只拿读锁，第一次写才升级——
那样在"升级"失败的瞬间拿到的 `SQLITE_BUSY` 是 **不能靠重试解决** 的
（snapshot 已经旧了）。`BEGIN IMMEDIATE` 一上来就抢写锁，
是 SQLite 官方推荐的多写者模式。

【为什么最后一定 ROLLBACK】
实验只验证"锁被占住"，不需要改数据。ROLLBACK 比 COMMIT 更安全：
写不进去的时候也不会留下半条记录。
*/

use std::time::Duration;

use sqlx::sqlite::{SqliteConnectOptions, SqlitePoolOptions};
use std::str::FromStr;

#[actix_web::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let mut args = std::env::args().skip(1);
    let db = args.next().unwrap_or_else(|| {
        eprintln!("用法: cargo run --example hold_lock -- <库路径> <持锁秒数>");
        std::process::exit(2);
    });
    let secs: u64 = args.next().and_then(|s| s.parse().ok()).unwrap_or(3);

    // 只开一个连接的池：本实验只想要"一条持锁的连接"，
    // 开多了反而可能自己跟自己抢。
    let opts = SqliteConnectOptions::from_str(&format!("sqlite:{db}"))?;
    let pool = SqlitePoolOptions::new()
        .max_connections(1)
        .connect_with(opts)
        .await?;

    println!("🔒 获取写锁（BEGIN IMMEDIATE），保持 {secs} 秒……");
    sqlx::query("BEGIN IMMEDIATE").execute(&pool).await?;

    actix_web::rt::time::sleep(Duration::from_secs(secs)).await;

    sqlx::query("ROLLBACK").execute(&pool).await?;
    println!("🔓 已释放写锁（ROLLBACK）");

    pool.close().await;
    Ok(())
}
