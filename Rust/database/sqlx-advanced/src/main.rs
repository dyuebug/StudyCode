// ============================================
// sqlx 进阶 - 超详细注释版
// 日期：2026-10-29（第32天）
// 难度：⭐⭐⭐ 高级
//
// ⚠️ 本文件是真实 Cargo 项目的一部分：
//    项目路径：database/sqlx-advanced/
//    运行方式：cd database/sqlx-advanced && cargo run
//    首次运行会创建 advanced.db（已在 .gitignore 中忽略）
// ============================================

/*
【核心概念】简要说明

是什么？
- 这一课讲的不是"多几个 API"，而是让数据库代码**能上生产**的四件事：
  迁移（schema 版本化）、动态 SQL、并发控制、性能可见性。

为什么需要？
- 第 31 天的代码在 demo 里没问题，但：
  · 表结构靠手写 CREATE TABLE，第二台机器怎么升级？
  · 查询条件是可选的（前端传了就过滤），怎么拼 SQL 还不注入？
  · 两个请求同时改同一条数据，谁赢？
  · 查询慢了你凭什么判断是缺索引？

核心特点？
- `sqlx::migrate!` 把迁移文件**编译进二进制**，启动即校验
- `QueryBuilder` 拼动态 SQL，值走 bind，表名/列名走白名单
- 乐观锁用 `version` 列 + `rows_affected()`，不需要数据库锁
- `EXPLAIN QUERY PLAN` 可以在代码里跑，索引有没有生效一目了然

Rust 特色？
- 与 Django / Rails 的迁移对比：sqlx 的迁移是纯 SQL 文件，
  没有 DSL、没有自动生成的 down 迁移——**你写什么就是什么**
- 与 Hibernate 的乐观锁对比：`@Version` 注解换成一列 + 一条 WHERE，
  机制相同，只是没有隐式魔法
*/

use chrono::Utc;
use futures::TryStreamExt;
use sqlx::sqlite::{SqliteConnectOptions, SqliteJournalMode, SqlitePool, SqlitePoolOptions};
use sqlx::{FromRow, QueryBuilder, Row};

use std::str::FromStr;
use std::time::Instant;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    println!("============================================================");
    println!("第一部分：核心概念详解");
    println!("============================================================");
    explain_migrations();
    explain_dynamic_sql();
    explain_concurrency();

    println!();
    println!("============================================================");
    println!("第二部分：基础操作实践");
    println!("============================================================");

    // --------------------------------------------
    // 准备：文件库 + WAL + 迁移
    // --------------------------------------------
    //
    // 【为什么要用文件库而不是 :memory:】
    // 迁移是"持久化 schema 版本"的机制，内存库每次启动都是空的，
    // 看不出迁移的价值。用文件库才能演示"第二次启动不重复执行"。
    //
    // 先删掉旧库，保证每次运行从零开始（仅本地 demo 这样做）
    let _ = std::fs::remove_file("advanced.db");
    let _ = std::fs::remove_file("advanced.db-shm");
    let _ = std::fs::remove_file("advanced.db-wal");

    // SqliteConnectOptions 是比字符串 DSN 更明确的配置方式
    let opts = SqliteConnectOptions::from_str("sqlite:advanced.db")?
        .create_if_missing(true)
        // WAL：读不被写阻塞。写操作仍然全局串行，但并发能力提升一个量级
        .journal_mode(SqliteJournalMode::Wal)
        // 外键约束默认是关闭的！这是 SQLite 最反直觉的默认值
        .foreign_keys(true);

    let pool = SqlitePoolOptions::new()
        // 文件库可以给多个连接；SQLite 写串行，给太多没意义
        .max_connections(5)
        // 池空时最多等 3 秒，超时报 PoolTimedOut 而不是无限挂起
        .acquire_timeout(std::time::Duration::from_secs(3))
        .connect_with(opts)
        .await?;

    // --------------------------------------------
    // 2.1 迁移：编译期内嵌，启动时自动执行
    // --------------------------------------------
    println!();
    println!("2.1 迁移（migrations）");
    println!("------------------------------------------------------------");

    /*
    `migrate!` 宏在**编译期**读取 ./migrations 目录，把 SQL 内容写进二进制。
    运行时它做的三件事：
      1. 若库中不存在 `_sqlx_migrations` 表，先建表
      2. 按版本号顺序执行未执行过的脚本，每执行一个就插入一条记录
      3. 已执行过的跳过 —— 所以这段代码可以每次启动都跑，是**幂等**的

    文件名格式必须是 `{版本号}_{描述}.sql`，版本号决定执行顺序。
    */
    sqlx::migrate!("./migrations").run(&pool).await?;
    println!("迁移执行完成（第二次运行会全部跳过）");

    // 看看迁移记录长什么样
    let applied: Vec<(i64, String, bool)> =
        sqlx::query_as("SELECT version, description, success FROM _sqlx_migrations ORDER BY version")
            .fetch_all(&pool)
            .await?;
    for (v, d, ok) in &applied {
        println!("  迁移 v{}  {}  成功={}", v, d, ok);
    }

    // --------------------------------------------
    // 2.2 动态 SQL：QueryBuilder + 白名单
    // --------------------------------------------
    println!();
    println!("2.2 动态 SQL：QueryBuilder");
    println!("------------------------------------------------------------");

    seed(&pool).await?;
    println!("已插入 6 条测试数据");

    // 前端可能传 0~2 个过滤条件，还有排序方向。SQL 的形状在运行时才确定。
    // 这种场景 `query!` 宏用不了（它要求 SQL 是字面量），只能 QueryBuilder。
    let found = search(&pool, Some("写"), None, Some("priority"), true).await?;
    println!("搜索「写」按 priority 降序 → {:?}",
             found.iter().map(|t| (&t.title, t.priority)).collect::<Vec<_>>());

    let found2 = search(&pool, None, Some(true), Some("id"), false).await?;
    println!("仅筛已完成按 id 升序 → {:?}",
             found2.iter().map(|t| &t.title).collect::<Vec<_>>());

    // --------------------------------------------
    // 2.3 批量插入
    // --------------------------------------------
    println!();
    println!("2.3 批量插入：单条循环 vs 多值 INSERT");
    println!("------------------------------------------------------------");

    /*
    单条 INSERT 循环 N 次 = N 次网络/协议往返。即使都在本机，
    每次也有语句解析 + 计划生成的成本。

    两种优化：
      (a) 一条 SQL 多个 VALUES：解析一次、计划一次，但参数上限有限
          （SQLite 默认 32766 个变量，Postgres 65535）
      (b) 事务包住 N 条单行 INSERT：减少提交次数（每次 COMMIT 都要刷盘）

    实测（本机 SQLite，50 条）：
      无事务循环        ~ 最慢，每条一次隐式提交
      事务包住循环      ~ 快一个数量级
      单条多值 INSERT   ~ 与事务包循环相当，但受变量上限约束
    */
    for (name, tx_wrapped) in [("无事务循环", false), ("事务包住循环", true)] {
        let t0 = Instant::now();
        if tx_wrapped {
            let mut tx = pool.begin().await?;
            for i in 0..50 {
                sqlx::query("INSERT INTO tasks (title, completed, created_at) VALUES (?, ?, ?)")
                    .bind(format!("批量-{}-{}", name, i))
                    .bind(false)
                    .bind(Utc::now())
                    .execute(&mut *tx)
                    .await?;
            }
            tx.commit().await?;
        } else {
            for i in 0..50 {
                sqlx::query("INSERT INTO tasks (title, completed, created_at) VALUES (?, ?, ?)")
                    .bind(format!("批量-{}-{}", name, i))
                    .bind(false)
                    .bind(Utc::now())
                    .execute(&pool)
                    .await?;
            }
        }
        println!("  {:<14} 50 条耗时 {:?}", name, t0.elapsed());
    }

    // --------------------------------------------
    // 2.4 分页：OFFSET vs 键集（cursor）
    // --------------------------------------------
    println!();
    println!("2.4 分页：OFFSET 与键集分页");
    println!("------------------------------------------------------------");

    /*
    【OFFSET 分页】`LIMIT 20 OFFSET 10000`
    数据库必须**扫描并丢弃**前 10000 行才能返回结果。
    页码越大越慢，是 O(offset)。而且期间有新插入会漏行/重复行。

    【键集分页】`WHERE id > <上一页最后的 id> ORDER BY id LIMIT 20`
    直接从索引位置开始，恒定 O(LIMIT)。缺点是只能"下一页"，
    不能跳到第 87 页——但列表页的用户几乎从不跳页。
    */
    let page1 = page_offset(&pool, 0, 3).await?;
    let page2 = page_offset(&pool, 3, 3).await?;
    println!("OFFSET 第 1 页：{:?}", page1.iter().map(|t| t.id).collect::<Vec<_>>());
    println!("OFFSET 第 2 页：{:?}", page2.iter().map(|t| t.id).collect::<Vec<_>>());

    let last_id = *page1.last().map(|t| &t.id).unwrap_or(&0);
    let next = page_keyset(&pool, last_id, 3).await?;
    println!("键集下一页（id > {}）：{:?}", last_id,
             next.iter().map(|t| t.id).collect::<Vec<_>>());

    // --------------------------------------------
    // 2.5 乐观锁
    // --------------------------------------------
    println!();
    println!("2.5 乐观锁：version 列 + rows_affected");
    println!("------------------------------------------------------------");

    let id = page1[0].id;
    // 第一次更新：带当前版本号
    let r1 = update_optimistic(&pool, id, "改后的标题", 0).await?;
    println!("首次更新（expected_version=0）→ {:?}", r1);

    // 第二次：还用过期版本号，模拟"别人已经改过了"
    let r2 = update_optimistic(&pool, id, "另一个标题", 0).await?;
    println!("重复更新（expected_version=0，已过期）→ {:?}", r2);

    // --------------------------------------------
    // 2.6 错误分类
    // --------------------------------------------
    println!();
    println!("2.6 错误分类：把数据库错误翻译成 HTTP 语义");
    println!("------------------------------------------------------------");

    // title 上有唯一索引，重复插入必然失败。
    // 用「写周报」而不是「买牛奶」：后者在 §2.5 已被改名，此时并不冲突。
    let dup = sqlx::query("INSERT INTO tasks (title, completed, created_at) VALUES (?, ?, ?)")
        .bind("写周报")
        .bind(false)
        .bind(Utc::now())
        .execute(&pool)
        .await;

    match dup {
        Ok(_) => println!("不该走到这里"),
        Err(e) => {
            // `is_unique_violation()` 是跨数据库的统一判断，
            // 比自己比对错误码字符串可靠（SQLite/PG/MySQL 码都不一样）
            let kind = if let sqlx::Error::Database(ref db_err) = e {
                if db_err.is_unique_violation() {
                    "唯一约束冲突 → HTTP 409"
                } else if db_err.is_foreign_key_violation() {
                    "外键冲突 → HTTP 400"
                } else {
                    "其他约束错误 → HTTP 400"
                }
            } else {
                "非数据库层错误 → HTTP 500"
            };
            println!("  {}（原始：{}）", kind, e);
        }
    }

    // --------------------------------------------
    // 2.7 流式查询
    // --------------------------------------------
    println!();
    println!("2.7 流式查询：fetch 而不是 fetch_all");
    println!("------------------------------------------------------------");

    /*
    `fetch_all` 把所有行一次性收进 Vec：百万行 = 全在内存里。
    `fetch` 返回 Stream，逐行处理，内存占用恒定。

    代价：必须保证流在连接上完成（连接被流借用），
    处理慢会长时间占住连接——这类场景要配合 `max_connections` 或分批。
    */
    let mut stream = sqlx::query_as::<_, TaskRow>(
        "SELECT id, title, completed, priority, version, created_at, done_at
         FROM tasks WHERE title LIKE ? ORDER BY id",
    )
    .bind("批量-%")
    .fetch(&pool);

    let mut n = 0;
    while let Some(row) = stream.try_next().await? {
        let _t: TaskRow = row;
        n += 1;
    }
    println!("流式读取 {} 行（逐行处理，未一次性加载）", n);

    // --------------------------------------------
    // 2.8 性能可见性：EXPLAIN QUERY PLAN
    // --------------------------------------------
    println!();
    println!("2.8 EXPLAIN QUERY PLAN：索引有没有生效");
    println!("------------------------------------------------------------");

    /*
    不要靠猜。SQLite 的 `EXPLAIN QUERY PLAN` 会明确告诉你：
      SCAN tasks           → 全表扫描，没有索引
      SEARCH tasks USING INDEX idx_tasks_completed (completed=?)  → 走了索引
    */
    show_plan(&pool, "SELECT * FROM tasks WHERE completed = 1").await?;
    show_plan(&pool, "SELECT * FROM tasks WHERE priority > 2").await?;
    show_plan(&pool, "SELECT * FROM tasks WHERE title = '买牛奶'").await?;

    println!();
    println!("============================================================");
    println!("第三部分：深入理解（概念，无需运行）");
    println!("============================================================");
    println!("见文件末尾注释：索引原理 / 连接池耗尽 / N+1 / 迁移的不可逆性");

    Ok(())
}

// ============================================
// 概念说明函数（纯打印，便于把「为什么」贴在对应代码旁）
// ============================================

fn explain_migrations() {
    println!();
    println!("1.1 为什么表结构必须版本化");
    println!("------------------------------------------------------------");
    println!("手写 CREATE TABLE 的三个致命问题：");
    println!("  1. 老库已经建过表，再执行会报 table already exists");
    println!("  2. 第二台机器/生产环境不知道要跑哪些脚本、按什么顺序");
    println!("  3. 出问题无法回答「线上现在是第几版结构」");
    println!("迁移解决了这三点：版本号 + 执行记录 + 幂等。");
}

fn explain_dynamic_sql() {
    println!();
    println!("1.2 动态 SQL 的边界：能 bind 什么，不能 bind 什么");
    println!("------------------------------------------------------------");
    println!("能 bind（走协议层，永远当数据）：所有值 —— 字符串、数字、时间");
    println!("不能 bind（SQL 引擎要求编译期确定）：");
    println!("  · 表名 / 列名");
    println!("  · ORDER BY 的字段与方向");
    println!("  · LIMIT / OFFSET 的部分数据库实现（SQLite 可以 bind）");
    println!("→ 这些只能用「白名单校验后直接拼接」，绝不接受用户原始输入。");
}

fn explain_concurrency() {
    println!();
    println!("1.3 并发控制的两条路：悲观锁与乐观锁");
    println!("------------------------------------------------------------");
    println!("悲观锁：先锁住再改（SELECT ... FOR UPDATE / BEGIN IMMEDIATE）");
    println!("  + 冲突时不需要重试");
    println!("  - 持锁期间其他事务全部阻塞，高并发下吞吐骤降；有死锁风险");
    println!("乐观锁：改的时候检查版本号，冲突就失败让上层重试");
    println!("  + 不持锁，读多写少场景吞吐极高");
    println!("  - 冲突时需要重试逻辑；写热点上反复失败");
    println!("Web 后端默认选乐观锁：冲突是少数，重试一次的成本低于全程持锁。");
}

// ============================================
// 第二部分用到的查询函数
// ============================================

/// 动态条件 + 动态排序的搜索
///
/// # 参数
/// - `keyword`: 标题模糊匹配（None 表示不过滤）
/// - `completed`: 完成状态过滤
/// - `order_by`: 排序字段，**必须来自白名单**
/// - `desc`: 是否降序
///
/// # 为什么 order_by 不能 bind
/// SQL 引擎在**生成执行计划时**就必须知道按哪一列排序，
/// 占位符的值在那时还不存在。所以只能拼字符串，
/// 而一旦拼字符串就必须用白名单，否则就是注入。
async fn search(
    pool: &SqlitePool,
    keyword: Option<&str>,
    completed: Option<bool>,
    order_by: Option<&str>,
    desc: bool,
) -> Result<Vec<TaskRow>, sqlx::Error> {
    let mut qb: QueryBuilder<sqlx::Sqlite> = QueryBuilder::new(
        "SELECT id, title, completed, priority, version, created_at, done_at FROM tasks WHERE 1=1",
    );

    // 值：走 push_bind，安全
    if let Some(kw) = keyword {
        qb.push(" AND title LIKE ").push_bind(format!("%{}%", kw));
    }
    if let Some(c) = completed {
        qb.push(" AND completed = ").push_bind(c);
    }

    // 标识符：走白名单
    //
    // 注意 `push` 是把片段**原样拼进 SQL**，所以这里绝对不能直接
    // `qb.push(order_by)` —— 那就是注入。先校验，再拼。
    let column = match order_by {
        Some("id") => "id",
        Some("priority") => "priority",
        Some("title") => "title",
        Some("created_at") => "created_at",
        _ => "id", // 默认值，也顺手挡掉了所有非法输入
    };
    qb.push(" ORDER BY ").push(column);
    qb.push(if desc { " DESC" } else { " ASC" });

    // build_query_as 会把拼好的 SQL + 绑定的参数组装成可执行的 QueryAs
    qb.build_query_as::<TaskRow>().fetch_all(pool).await
}

/// OFFSET 分页
async fn page_offset(
    pool: &SqlitePool,
    offset: i64,
    limit: i64,
) -> Result<Vec<TaskRow>, sqlx::Error> {
    sqlx::query_as(
        "SELECT id, title, completed, priority, version, created_at, done_at
         FROM tasks ORDER BY id LIMIT ? OFFSET ?",
    )
    .bind(limit)
    .bind(offset)
    .fetch_all(pool)
    .await
}

/// 键集分页：从 `after_id` 之后继续
async fn page_keyset(
    pool: &SqlitePool,
    after_id: i64,
    limit: i64,
) -> Result<Vec<TaskRow>, sqlx::Error> {
    sqlx::query_as(
        "SELECT id, title, completed, priority, version, created_at, done_at
         FROM tasks WHERE id > ? ORDER BY id LIMIT ?",
    )
    .bind(after_id)
    .bind(limit)
    .fetch_all(pool)
    .await
}

/// 乐观锁更新
///
/// # 返回
/// - `Ok(true)`  更新成功
/// - `Ok(false)` 版本不匹配（别人改过了）或行不存在
async fn update_optimistic(
    pool: &SqlitePool,
    id: i64,
    new_title: &str,
    expected_version: i64,
) -> Result<bool, sqlx::Error> {
    let r = sqlx::query(
        "UPDATE tasks SET title = ?, version = version + 1
         WHERE id = ? AND version = ?",
    )
    .bind(new_title)
    .bind(id)
    .bind(expected_version)
    .execute(pool)
    .await?;

    // 关键：`rows_affected() == 0` 同时覆盖了两种情况
    // （不存在 / 版本过期）。要区分 404 和 409，需要再查一次存在性。
    Ok(r.rows_affected() == 1)
}

/// 打印 SQLite 的执行计划
async fn show_plan(pool: &SqlitePool, sql: &str) -> Result<(), sqlx::Error> {
    // EXPLAIN QUERY PLAN 的返回列名是 detail / id / parent / notused
    let rows = sqlx::query(&format!("EXPLAIN QUERY PLAN {}", sql))
        .fetch_all(pool)
        .await?;

    println!("  SQL: {}", sql);
    for row in rows {
        let detail: String = row.get("detail");
        println!("    → {}", detail);
    }
    Ok(())
}

/// 种子数据
async fn seed(pool: &SqlitePool) -> Result<(), sqlx::Error> {
    let items = [
        ("买牛奶", false, 1),
        ("写周报", false, 3),
        ("跑测试", true, 2),
        ("写文档", false, 5),
        ("优化索引", true, 4),
        ("复习 SQLx", false, 2),
    ];

    let mut tx = pool.begin().await?;
    for (title, completed, priority) in items {
        sqlx::query(
            "INSERT INTO tasks (title, completed, priority, created_at) VALUES (?, ?, ?, ?)",
        )
        .bind(title)
        .bind(completed)
        .bind(priority)
        .bind(Utc::now())
        .execute(&mut *tx)
        .await?;
    }
    tx.commit().await?;
    Ok(())
}

/// 一行任务
///
/// 字段名与列名一一对应。`done_at` 来自第二个迁移文件新增的列。
///
/// 【为什么加 `#[allow(dead_code)]`】
/// `FromRow` 需要这些字段存在才能解码，但本文件的 demo 没读完所有字段，
/// 编译器于是报 `fields ... are never read`。
/// 这是 sqlx 项目里非常常见的摩擦点——**不是**代码有问题。
/// 另一种解法是把它们真的用起来（比如 `Debug` 打印整行），
/// 但在只关心部分列的查询里，`allow` 更直接。
#[derive(Debug, FromRow)]
#[allow(dead_code)]
struct TaskRow {
    id: i64,
    title: String,
    completed: bool,
    priority: i64,
    version: i64,
    created_at: chrono::DateTime<Utc>,
    /// 第二版迁移新增的可空列 —— 老数据没有值，所以必须 Option
    done_at: Option<chrono::DateTime<Utc>>,
}

// ============================================
// 第三部分：深入理解（可选）
// ============================================

/*
如果你想知道"为什么"，读这一节。觉得难可以跳过，不影响后续学习。

【1】索引为什么能让查询变快（B 树到底存了什么）

    没有索引时，`WHERE completed = 1` 只能全表扫描：
    逐行读取、逐行比较，复杂度 O(n)。

    有了索引，SQLite 建一棵 B 树，键是 (completed, rowid)：
      - 定位到 completed = 1 的区间：O(log n)
      - 顺序读出该区间所有 rowid：O(k)，k 是命中行数
    查询复杂度从 O(n) 变成 O(log n + k)。

    代价（必须权衡）：
      · 每次 INSERT / UPDATE / DELETE 都要维护索引 → 写变慢
      · 索引占磁盘，通常是被索引列的 20%–50%
      · 索引列的选择性低（比如只有 0/1 的 bool）时，B 树退化成"两个大桶"，
        此时优化器可能**主动不用**它（还是全表扫描更快）

    判断该不该建索引的依据只有一个：**这条 SQL 在慢查询日志里出现过几次**。
    "这个字段看起来重要"不是依据。

【2】复合索引的「最左前缀」原则

    建了 `INDEX (a, b, c)`，则以下查询都能用上：
        WHERE a = ?              ✅
        WHERE a = ? AND b = ?    ✅
        WHERE a = ? AND b = ? AND c = ?  ✅
        WHERE b = ?              ❌ 用不上（跳过最左列）
        WHERE a = ? AND c = ?    ⚠️ 只能用 a 的部分，c 要回表过滤

    所以复合索引的列顺序 = **等值条件在前，范围条件在后**。
    这条规则在数据量小的时候完全看不出来，在百万行时决定快 100 倍还是慢 100 倍。

【3】连接池耗尽：生产最常见的事故

    症状：
        Error: pool timed out while waiting for an open connection

    根因几乎总是同一个：**某个地方借了连接没还**。

    常见形态：
      · 把 `PoolConnection` 存进 struct 长期持有（clone 池才对，别 clone 连接）
      · 在持有连接时 `await` 了一个永远不会完成的 future
      · 事务里写了死循环，`tx` 一直不 drop
      · 用 `fetch` 流但消费到一半就 return（流没 drop → 连接没还）

    排查方法：
      1. 看 `pool.size()` 与 `pool.num_idle()`
      2. 临时把 `acquire_timeout` 调到 100ms，让泄漏立刻暴露成报错
      3. `RUST_LOG=sqlx=debug` 打开 sqlx 的连接日志，看 acquire/release 配对

【4】N+1 查询：ORM 时代的经典病，sqlx 也会犯

    场景：列出所有任务及其标签
        let tasks = query_as("SELECT * FROM tasks").fetch_all(&pool).await?;   // 1 次
        for t in &tasks {
            let tags = query_as("SELECT * FROM tags WHERE task_id = ?")
                .bind(t.id).fetch_all(&pool).await?;                          // N 次
        }
    100 条任务 = 101 次数据库往返。

    修法：
      (a) 一次 JOIN 把标签取回来，在 Rust 里按 task_id 分组
          SELECT t.*, g.name FROM tasks t LEFT JOIN tags g ON g.task_id = t.id
      (b) 或先收集所有 id，一次 `WHERE task_id IN (...)` 查回来再分组
          （sqlx 的 `QueryBuilder::push_tuples` / separated 可以构造 IN 列表）

    判断标准：**凡是"循环里 await 数据库"，先怀疑 N+1。**

【5】迁移是不可逆的单行道

    sqlx 的迁移**没有 down 脚本**（对比 Django 的 `migrate app 0001`）。
    已执行的迁移记录在 `_sqlx_migrations` 里，删掉文件也不会重跑。

    这意味着：
      · `ALTER TABLE ... DROP COLUMN` 一旦上线，数据就没了
      · 改列类型要"建新列 → 回填 → 删旧列"，分三个迁移版本做
      · 生产环境的破坏性变更必须写成"加新列 + 双写 + 灰度切换 + 删旧列"

    这是数据库工程里最贵的教训之一：**schema 变更永远按"可回滚"设计。**

【6】为什么本文件用文件库而不是 :memory:

    三个具体原因：
      1. 迁移的幂等性只有在"第二次启动"时才看得出来
      2. WAL 模式对内存库没有意义（内存库没有 WAL 文件）
      3. 事务隔离在文件库上才是真实磁盘语义

    但文件库也有代价：每次 `cargo run` 前要 `remove_file`，
    否则种子数据会因唯一索引冲突而失败。这就是 .gitignore 里
    那三个文件的由来（db / db-shm / db-wal —— WAL 是**三个文件**，不是一���）。

【7】什么时候该从 SQLite 换到 PostgreSQL

    不是"项目变大"这种模糊说法，而是具体的信号：
      · 写并发成为瓶颈：WAL 下单写者仍然串行，写入 QPS 上不去
      · 需要多机部署：SQLite 是单机文件，没有网络协议
      · 需要真正的并发控制：`SELECT FOR UPDATE`、可串行化隔离级别
      · 需要 `jsonb`、全文检索、地理位置、窗口函数的高级形态
      · 需要在线 DDL 不锁表、逻辑复制、PITR 备份

    反过来：单机、读多写少、数据 < 100GB，SQLite 是**正确选择**而非妥协。
    迁移成本主要在 SQL 方言差异（`?` vs `$1`、`AUTOINCREMENT` vs `SERIAL`）
    和数据类型（SQLite 没有真正的 BOOLEAN / DATETIME / JSONB 类型）。
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

/*
// --------------------------------------------
// 4.1 错误：把用户输入拼进 ORDER BY
// --------------------------------------------
❌ 错误代码：
```rust
let sort = req.query.sort;                       // 用户控制
let sql = format!("SELECT * FROM tasks ORDER BY {}", sort);
sqlx::query_as(&sql).fetch_all(&pool).await?;
```
攻击者传 `(CASE WHEN (SELECT substr(password,1,1) FROM users)='a' THEN id ELSE title END)`
就变成了**盲注**：能通过排序结果反推任意数据。

原因：ORDER BY 后面必须是标识符，无法 bind。

✅ 正确代码：白名单
```rust
let column = match sort.as_str() {
    "id" => "id",
    "priority" => "priority",
    "created_at" => "created_at",
    _ => "id",
};
qb.push(" ORDER BY ").push(column);
```

记忆技巧：**值和标识符是两类东西。值 bind，标识符白名单。没有第三种。**


// --------------------------------------------
// 4.2 错误：启动时执行迁移但没处理并发
// --------------------------------------------
❌ 错误代码（多实例部署）：
```rust
sqlx::migrate!("./migrations").run(&pool).await?;
// 同时有 3 个实例启动
```
后果：三个实例同时往 `_sqlx_migrations` 插入记录，可能报
`UNIQUE constraint failed: _sqlx_migrations.version`，
或两个实例同时执行同一条 `ALTER TABLE` 导致其中一个失败。

原因：迁移不是为"多写者并发"设计的。

✅ 正确做法：
  · 把迁移从应用启动流程里**移出去**，作为部署流水线的单独一步
        sqlx migrate run --database-url ...
  · 或者用外部锁（PostgreSQL 的 advisory lock）保证单实例执行
  · SQLite 单机部署不存在这个问题，但代码一旦复用到 PG 就踩上了

记忆技巧：**迁移是"一次且仅一次"的操作，不该挂在"每次启动"上。**


// --------------------------------------------
// 4.3 错误：忘记 SQLite 默认关闭外键
// --------------------------------------------
❌ 错误代码：
```rust
let pool = SqlitePool::connect("sqlite:app.db").await?;
// 建表时写了 FOREIGN KEY，以为会生效
```
后果：删父行时子行不会报错，也不会级联 —— 数据库里留下孤儿记录。
**外键约束在 SQLite 里默认是关闭的**（为了向后兼容 2001 年的行为）。

✅ 正确代码：
```rust
SqliteConnectOptions::new()
    .filename("app.db")
    .foreign_keys(true)         // 每个连接都要打开
```

记忆技巧：**SQLite 的 `PRAGMA foreign_keys` 是**连接级**开关，
池子每次新建连接都要重新设置 —— 用 `SqliteConnectOptions` 一次配好。**


// --------------------------------------------
// 4.4 错误：乐观锁只返回 bool，丢失 404 / 409 的区分
// --------------------------------------------
❌ 错误代码：
```rust
let ok = update_optimistic(&pool, id, title, ver).await?;   // true / false
if !ok { return Err(InternalError); }
```
客户端拿到 500，无法知道是"这条不存在"还是"被别人改过"。

✅ 正确代码：
```rust
if rows_affected == 0 {
    let exists: Option<(i64,)> = query_as("SELECT id FROM tasks WHERE id = ?")
        .bind(id).fetch_optional(&pool).await?;
    return match exists {
        None => Err(NotFound),                       // 404
        Some(_) => Err(Conflict("版本已过期")),       // 409
    };
}
```

记忆技巧：**`rows_affected() == 0` 是"未命中"，不是"错误类型"。
要区分原因就必须再查一次。**


// --------------------------------------------
// 4.5 错误：用 fetch 流但提前 return
// --------------------------------------------
❌ 错误代码：
```rust
let mut stream = query_as::<_, TaskRow>("SELECT ...").fetch(&pool);
while let Some(row) = stream.try_next().await? {
    if row.id > 100 { return Ok(()); }    // 提前结束，流的 drop 时机不确定
}
```
后果：连接可能被长时间占住（取决于 drop 时机与实现），
高并发下表现为池耗尽。而且 SQLite 上未消费完的游标会阻塞写事务。

✅ 正确代码：要么消费完，要么显式 drop
```rust
{
    let mut stream = ...fetch(&pool);
    // 消费
}   // 流的生命周期锁在这个块内
```

记忆技巧：**流借用连接。流的存活时间 = 连接被占用的时间。**


// --------------------------------------------
// 4.6 错误：把 bool 当独立类型查
// --------------------------------------------
❌ 错误代码：
```rust
query_as("SELECT * FROM tasks WHERE completed IS TRUE")
```
SQLite 里没有 `TRUE` 字面量（3.23 之前），`completed` 是 INTEGER。
写成 `IS TRUE` 在老版本上会静默匹配不到任何行（`TRUE` 被当成标识符）。

✅ 正确代码：
```rust
query_as("SELECT * FROM tasks WHERE completed = ?").bind(true)
```

记忆技巧：**SQLite 只有 5 种存储类别：NULL / INTEGER / REAL / TEXT / BLOB。
没有 BOOLEAN、DATETIME、JSON。全靠约定。**


// --------------------------------------------
// 4.7 错误：以为迁移文件改了会重跑
// --------------------------------------------
❌ 错误代码（改了已执行的迁移）：
```sql
-- migrations/0001_init.sql（已被执行过，现在改了内容）
CREATE TABLE tasks ( id INTEGER PRIMARY KEY, title TEXT NOT NULL, extra TEXT );
```
后果：**不会重跑**。已执行的迁移按版本号记录，sqlx 只检查新版本。
本地库和同事的库结构悄悄分叉，线上更是完全不同的样子。

✅ 正确做法：新增一个迁移文件
```sql
-- migrations/0003_add_extra.sql
ALTER TABLE tasks ADD COLUMN extra TEXT;
```

记忆技巧：**已提交的迁移文件视为不可变。改结构 = 加新文件。**


// --------------------------------------------
// 4.8 错误：把 PoolConnection 存进状态结构体
// --------------------------------------------
❌ 错误代码：
```rust
struct AppState {
    conn: sqlx::pool::PoolConnection<sqlx::Sqlite>,   // 借出来的连接
}
```
后果：整个应用只有 1 条连接可用，所有并发请求排队；
且 `PoolConnection` 的 drop 才会归还 —— 它活到进程结束，等于池里永远少一条。

✅ 正确代码：
```rust
struct AppState {
    pool: SqlitePool,     // 存池，用时 acquire，用完立即 drop
}
```

记忆技巧：**状态里存「池」，不存「连接」。**
连接是有借有还的短期资源，池才是长期持有的东西。
*/

// ============================================
// 第五部分：函数/宏卡片速查
// ============================================

/*
【宏：sqlx::migrate!】

语法：sqlx::migrate!("./migrations").run(&pool).await?;
作用：编译期内嵌 migrations 目录，运行时按版本号执行未执行的迁移
前提：Cargo feature `migrate`；目录内文件名须为 `{版本}_{描述}.sql`
幂等：已执行的跳过（记录在 `_sqlx_migrations` 表）
注意：**无 down 迁移**；已提交的迁移文件不可修改
────────────────────────────────────

【类型：QueryBuilder】

语法：QueryBuilder::<Sqlite>::new("SELECT ... WHERE 1=1")
方法：
  .push(sql_fragment: &str)      原样拼接（**只能放自己写的字面量**）
  .push_bind(value)              绑定参数（安全）
  .build_query_as::<T>()         转 QueryAs<T>，可 fetch_all
  .build_query()                 转 Query，取 Row
  .separated(", ")               构造 IN 列表（配 push_bind_unseparated）
作用：动态 SQL 的唯一正确方式
示例：
```rust
qb.push(" AND title LIKE ").push_bind("%rust%");
let rows = qb.build_query_as::<TaskRow>().fetch_all(&pool).await?;
```
────────────────────────────────────

【方法：push_tuples / separated】

语法：qb.push(" IN (").push_tuples(ids, |mut b, id| { b.push_bind(id); }).push(")")
作用：批量 bind（避免手写 N 个占位符）
注意：IN 列表长度上限受数据库变量数限制（SQLite 32766）
────────────────────────────────────

【方法：SqliteConnectOptions】

语法：SqliteConnectOptions::from_str("sqlite:app.db")? 或 ::new().filename(..)
方法：
  .create_if_missing(true)     文件不存在则创建（等价 ?mode=rwc）
  .journal_mode(SqliteJournalMode::Wal)   开 WAL，读不被写阻塞
  .foreign_keys(true)          **必须显式打开**（默认关闭）
  .synchronous(SqliteSynchronous::Normal)  WAL 下的推荐值
  .busy_timeout(Duration)      写锁冲突时等待多久
────────────────────────────────────

【方法：Pool::begin / Transaction::commit】

语法：let mut tx = pool.begin().await?; ... tx.commit().await?;
语义：RAII，drop 时未 commit 则自动 rollback
关键：事务内语句必须 `.execute(&mut *tx)`
隔离：SQLite 默认 `BEGIN DEFERRED`（首次读才加锁）；
      `.begin_with("BEGIN IMMEDIATE")` 可立即取写锁，避免升级锁失败
────────────────────────────────────

【trait：DatabaseError】

语法：if let sqlx::Error::Database(e) = err { e.is_unique_violation() }
方法：
  is_unique_violation()        唯一约束冲突 → 409
  is_foreign_key_violation()   外键冲突 → 400
  is_check_violation()         CHECK 约束 → 400
  code()                       数据库原始错误码（跨库不通用，慎用）
作用：把数据库错误翻译成 HTTP 语义，**不要靠解析错误文本**
────────────────────────────────────

【方法：Pool::fetch / Stream】

语法：let mut s = query_as::<_, T>(..).fetch(&pool);
      while let Some(row) = s.try_next().await? { .. }
返回：BoxStream<'_, Result<T, Error>>，需 `futures::TryStreamExt`
作用：逐行处理，内存恒定
注意：流借用连接，必须消费完或显式 drop，否则连接被长期占用
────────────────────────────────────

【SQL：EXPLAIN QUERY PLAN】

语法：EXPLAIN QUERY PLAN SELECT ...;
输出关键行：`detail` 列
  SCAN table            → 全表扫描（无可用索引）
  SEARCH ... USING INDEX idx_x (col=?)  → 走索引
  USE TEMP B-TREE FOR ORDER BY          → 排序未利用索引
────────────────────────────────────

【SQL：WAL 模式】

开启：PRAGMA journal_mode = WAL;
效果：
  + 读不被写阻塞（读者看快照）
  + 写更快（顺序追加日志文件）
  - 产生 3 个文件：db / db-wal / db-shm
  - 需要定期的 checkpoint（自动，默认 1000 页）
  - 不能用于网络文件系统（NFS）——共享内存映射会失效
────────────────────────────────────

【类型映射补充（本文件涉及）】

i64 / bool / String / Option<T> / DateTime<Utc>
多值 INSERT：`INSERT INTO t (a,b) VALUES (?,?),(?,?),(?,?)`
变量上限：SQLite 默认 32766
────────────────────────────────────
*/

// ============================================
// 第六部分：练习题（不提供答案）
// ============================================

/*
【练习 1：基础 —— 写第三个迁移】
要求：
  - 新增 `migrations/0003_*.sql`，给 tasks 加 `deleted_at TEXT`（软删除）
  - 不加代码逻辑，只写 SQL
  - 运行两次 `cargo run`，确认第二次不重复执行
提示：
  - 文件名版本号必须大于 0002
  - 迁移执行记录在 `_sqlx_migrations` 表，可以自己 SELECT 出来看
注意：
  - 不要修改 0001 或 0002。改结构只能加新文件

【练习 2：基础 —— 给 completed 查询加索引并验证】
要求：
  - 先删掉 `idx_tasks_completed`（写一个新迁移）
  - 用 `EXPLAIN QUERY PLAN` 确认变回 `SCAN tasks`
  - 再加回来，确认变回 `SEARCH ... USING INDEX`
提示：
  - 本文件的 §2.8 已经打好了三份执行计划，先对照看
注意：
  - 数据量很小时（10 条），SQLite 可能**仍然选全表扫描**——
    优化器会算成本，"有索引"不等于"一定用索引"

【练习 3：进阶 —— IN 列表的安全构造】
要求：
  - `fn get_tasks_by_ids(pool, ids: &[i64]) -> Result<Vec<TaskRow>, Error>`
  - 用 `QueryBuilder::push_tuples`（或 `separated`）构造 `WHERE id IN (...)`
  - 空列表要返回空 Vec，不能生成 `IN ()`（那是语法错误）
提示：
  - `IN ()` 在 SQLite 里是语法错误，必须提前 return
  - 手写 `?,?,?` 的版本也要试一次，对比可读性
注意：
  - 不要把 ids 拼成字符串 —— 那既是注入，又会丢失类型

【练习 4：进阶 —— 完整的乐观锁重试】
要求：
  - 写 `update_with_retry(pool, id, new_title, max_retries)`
  - 版本冲突时重新读最新 version 并重试，最多 N 次
  - 区分三种结果：成功 / 冲突耗尽 / 不存在
提示：
  - 每次重试之间可以考虑 `tokio::time::sleep` 一小段（退避）
  - 不要无限重试——那是活锁
注意：
  - 思考：读-改-写在事务里做，和"读在事务外、改在事务内"有区别吗？
    后者仍会冲突，前者需要 `BEGIN IMMEDIATE`

【练习 5：高阶 —— 修一个真实的 N+1】
要求：
  - 新建 `tags (id, task_id, name)` 表（写迁移）
  - 先写 N+1 版本（循环里查标签），用 `Instant` 计时
  - 再写 JOIN 版本，计时对比
  - 数据量至少 200 条任务时比较
提示：
  - 数据量小时差距不明显，这本身就是"为什么要测量"的教材
  - JOIN 版本要用 `GROUP BY` 或在 Rust 里手工分组（后者更好读）
注意：
  - 结论要写进日志：差多少倍，在那个数据量下

【练习 6：综合 —— 生产级启动流程】
要求：
  - 写一个 `init_db() -> Result<SqlitePool>`，包含：
    · 连接池配置（WAL、外键、超时）
    · 迁移执行
    · 启动自检（能否 SELECT 1、`_sqlx_migrations` 是否完整）
  - 迁移失败要给出可操作的错误信息，不是裸 `?`
提示：
  - 自检的目的是"启动就失败"，而不是"运行到第一个请求才失败"
  - 错误信息里要能看出是"连不上"还是"迁移失败"还是"版本不匹配"
注意：
  - 这是把本课所有内容合成一个函数，写完它你就掌握了"能上生产"的最小形态
*/

// ============================================
// 文件结束
// ============================================
