// ============================================
// sqlx 基础 - 超详细注释版
// 日期：2026-10-29（第31天）
// 难度：⭐⭐ 中级
//
// ⚠️ 本文件是**真实 Cargo 项目**的一部分，不能用 `rustc` 单独编译：
//    项目路径：database/sqlx-basics/
//    运行方式：cd database/sqlx-basics && cargo run
//    原因：sqlx 依赖 tokio 运行时与数据库驱动，必须有依赖树
// ============================================

/*
【核心概念】简要说明

是什么？
- `sqlx` 是 Rust 的**异步**数据库工具箱，支持 PostgreSQL / MySQL / SQLite。
- 与 ORM（Diesel、SeaORM）不同，sqlx 不生成 SQL ——
  **你写 SQL，它负责执行、类型映射和编译期校验**。

为什么需要？
- 第 27 天的 JSON 文件持久化在真实后端里撑不住（下面详列边界）
- 数据库提供的并发控制、事务、索引是手写文件永远追不上的

核心特点？
- 纯 Rust 协议实现，不依赖 libpq / libmysqlclient（SQLite 则内嵌 sqlite3）
- 异步：与 tokio 原生集成，连接池天然适合高并发
- `query!` 宏在**编译期**连库校验 SQL 与类型
- 无 ORM 魔法：SQL 就是字符串，看得见、可 copy 到 DBeaver 里跑

Rust 特色？
- 与 Django ORM / Hibernate 对比：sqlx 不做实体映射的自动化，
  迁移、索引、执行计划仍在你的掌控里
- 与 Go 的 database/sql 对比：sqlx 的编译期校验是 Go 生态没有的
- 与 Node 的 Prisma 对比：Sqlx 没有 schema DSL，SQL 是唯一真相源
*/

/*
【为什么要换掉 JSON 文件】—— 先想清楚，再学 API

第 27 天的 `PersistenceManager` 解决了"数据跨进程存活"，但它的边界很明确：

  1. **全量读写**：改一条 todo 要重写整个数组。10 万条时每次 POST 都是 10MB 写盘
  2. **无并发控制**：两个请求同时 create，后写覆盖先写（丢失更新）
  3. **无查询能力**：想查"未完成的、标题含 rust 的 todo"只能全加载后内存过滤
  4. **无事务**：跨多个实体的操作无法原子完成
  5. **无索引**：任何条件查询都是 O(n) 扫描
  6. **无约束**：邮箱唯一、外键引用完整性全靠代码保证

数据库把这六件事一次性解决。这就是后端必须用数据库的全部理由——
**不是因为"更专业"，而是因为文件方案的代价随数据量线性增长，而数据库不会。**
*/

use chrono::{DateTime, Utc};

use sqlx::sqlite::{SqlitePool, SqlitePoolOptions};
use sqlx::{FromRow, Row};

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    println!("============================================================");
    println!("第一部分：核心概念详解");
    println!("============================================================");
    println!();
    println!("1.1 连接池与异步：为什么不是「一个 Connection 走天下」");
    println!("------------------------------------------------------------");
    println!("见源码注释：Pool 是 N 个连接的复用队列，acquire 时借出，drop 时归还");
    println!("异步的价值：等待 I/O 时不占线程，1 个线程可服务上千并发查询");

    // --------------------------------------------
    // 建立连接池
    // --------------------------------------------
    //
    // `SqlitePoolOptions` 用于配置连接池参数。
    // `max_connections(1)` 在这里是**必须的**，原因是 `:memory:` 数据库
    // 是"每个连接一份"的私有内存库 —— 若池里有 2 个连接，
    // 建表在 A 连接、查询走 B 连接，就会报 "no such table"。
    //
    // 这是 SQLite + 内存库最有名的坑。真实项目用文件库或
    // `sqlite::memory:?cache=shared`，就不必锁成 1。
    let pool = SqlitePoolOptions::new()
        .max_connections(1)
        .connect("sqlite::memory:")
        .await?;

    println!();
    println!("1.2 三种执行入口：query / query_as / query!");
    println!("------------------------------------------------------------");
    println!("query(sql)          → 返回 Row，手动按列名取值，无类型保证");
    println!("query_as::<_, T>    → 返回 T（元组或 #[derive(FromRow)] 结构体）");
    println!("query!(sql)         → 编译期连库校验 SQL 与类型（需 DATABASE_URL）");
    println!("本文件用前两种；query! 的原理见第三部分【2】");

    println!();
    println!("============================================================");
    println!("第二部分：基础操作实践");
    println!("============================================================");

    // --------------------------------------------
    // 2.0 建表
    // --------------------------------------------
    println!();
    println!("2.0 CREATE TABLE");
    println!("------------------------------------------------------------");

    // `sqlx::query` 执行任意 SQL，不返回行。
    // 参数用 `?` 占位（SQLite 风格），由 `bind()` 按顺序填充 —— **不要拼字符串**，
    // 那正是 SQL 注入的来源。
    sqlx::query(
        r#"
        CREATE TABLE todos (
            id         INTEGER PRIMARY KEY AUTOINCREMENT,
            title      TEXT    NOT NULL,
            completed  INTEGER NOT NULL DEFAULT 0,
            note       TEXT,
            created_at TEXT    NOT NULL
        )
        "#,
    )
    .execute(&pool)
    .await?;

    println!("表 todos 创建成功");

    // --------------------------------------------
    // 2.1 插入
    // --------------------------------------------
    println!();
    println!("2.1 INSERT + 拿自增主键");
    println!("------------------------------------------------------------");

    let result = sqlx::query(
        "INSERT INTO todos (title, completed, note, created_at) VALUES (?, ?, ?, ?)",
    )
    .bind("学习 sqlx")
    .bind(false) // bool 会被编码成 0/1
    .bind(Option::<String>::None) // NULL 必须用 Option 表达
    .bind(Utc::now()) // chrono 类型，sqlite 里存成 TEXT
    .execute(&pool)
    .await?;

    // `last_insert_rowid()` 是本条 INSERT 生成的主键
    let first_id = result.last_insert_rowid();
    println!("插入 1 条，id = {}，影响行数 = {}", first_id, result.rows_affected());

    // 批量插入：真实项目里应该用事务包起来（见 2.5）
    for (i, title) in ["写文档", "跑测试", "优化索引"].iter().enumerate() {
        sqlx::query("INSERT INTO todos (title, completed, note, created_at) VALUES (?, ?, ?, ?)")
            .bind(*title)
            .bind(i == 1) // 第二条标为已完成
            .bind(Option::<String>::None)
            .bind(Utc::now())
            .execute(&pool)
            .await?;
    }
    println!("再插入 3 条");

    // --------------------------------------------
    // 2.2 查询
    // --------------------------------------------
    println!();
    println!("2.2 三种查询取法：fetch_one / fetch_optional / fetch_all");
    println!("------------------------------------------------------------");

    // ---- 方式一：query_as + 元组 ----
    // 元组类型必须与 SELECT 的列**顺序和类型**严格一致
    let row: (i64, String, bool) =
        sqlx::query_as("SELECT id, title, completed FROM todos WHERE id = ?")
            .bind(first_id)
            .fetch_one(&pool) // 没有行 → Err(RowNotFound)
            .await?;
    println!("fetch_one  → id={} title={:?} completed={}", row.0, row.1, row.2);

    // ---- 方式二：fetch_optional ----
    // 查不到不是错误 → 返回 Option，这是"可能没有"的正确建模
    let missing: Option<(i64,)> = sqlx::query_as("SELECT id FROM todos WHERE id = ?")
        .bind(999_999_i64)
        .fetch_optional(&pool)
        .await?;
    println!("fetch_optional（不存在的 id）→ {:?}", missing);

    // ---- 方式三：query_as + 结构体 ----
    // `#[derive(FromRow)]` 按**字段名**匹配列名，不依赖顺序
    let all: Vec<TodoRow> = sqlx::query_as(
        "SELECT id, title, completed, note, created_at FROM todos ORDER BY id",
    )
    .fetch_all(&pool)
    .await?;

    println!("fetch_all  → 共 {} 条：", all.len());
    for t in &all {
        println!(
            "  #{} {:<10} completed={} note={:?} at={}",
            t.id,
            t.title,
            t.completed,
            t.note,
            t.created_at.format("%Y-%m-%d %H:%M:%S")
        );
    }

    // ---- 方式四：底层 Row 取值 ----
    // query_as 覆盖不了动态 SQL 时用这个：按列名/列号取，类型自己声明
    let row = sqlx::query("SELECT COUNT(*) AS cnt FROM todos")
        .fetch_one(&pool)
        .await?;
    let cnt: i64 = row.get("cnt");
    println!("通过 Row::get 取聚合值 → count = {}", cnt);

    // --------------------------------------------
    // 2.3 更新与删除
    // --------------------------------------------
    println!();
    println!("2.3 UPDATE / DELETE + rows_affected");
    println!("------------------------------------------------------------");

    let upd = sqlx::query("UPDATE todos SET completed = ? WHERE id = ?")
        .bind(true)
        .bind(first_id)
        .execute(&pool)
        .await?;
    println!("UPDATE 影响 {} 行", upd.rows_affected());

    // ⚠️ 关键设计点：UPDATE 不存在的行**不是错误**，只是 rows_affected == 0。
    // REST API 里 PUT /todos/999999 应该返回 404，靠的就是这个 0。
    let upd_missing = sqlx::query("UPDATE todos SET completed = ? WHERE id = ?")
        .bind(true)
        .bind(999_999_i64)
        .execute(&pool)
        .await?;
    println!("UPDATE 不存在的行 → 影响 {} 行（不报错）", upd_missing.rows_affected());

    let del = sqlx::query("DELETE FROM todos WHERE id = ?")
        .bind(first_id)
        .execute(&pool)
        .await?;
    println!("DELETE 影响 {} 行", del.rows_affected());

    // --------------------------------------------
    // 2.4 条件查询与分页
    // --------------------------------------------
    println!();
    println!("2.4 条件查询与分页");
    println!("------------------------------------------------------------");

    let completed: Vec<TodoRow> = sqlx::query_as(
        "SELECT id, title, completed, note, created_at
         FROM todos WHERE completed = ? ORDER BY id LIMIT ? OFFSET ?",
    )
    .bind(true)
    .bind(10_i64)
    .bind(0_i64)
    .fetch_all(&pool)
    .await?;

    println!("已完成的前 10 条：");
    for t in &completed {
        println!("  #{} {}", t.id, t.title);
    }

    // 模糊查询：LIKE 的参数也必须 bind
    let like: Vec<TodoRow> = sqlx::query_as(
        "SELECT id, title, completed, note, created_at
         FROM todos WHERE title LIKE ? ORDER BY id",
    )
    .bind("%测试%")
    .fetch_all(&pool)
    .await?;
    println!("标题含「测试」的：{:?}", like.iter().map(|t| &t.title).collect::<Vec<_>>());

    // --------------------------------------------
    // 2.5 事务
    // --------------------------------------------
    println!();
    println!("2.5 事务：要么全成，要么全不成");
    println!("------------------------------------------------------------");

    /*
    【为什么必须用事务】
    批量插入时如果中途失败（约束冲突、进程被杀），没有事务会留下"写了一半"的数据。
    事务保证原子性：commit 之前所有改动对外不可见。

    【sqlx 的事务是 RAII 的】
      - `pool.begin()` 借出一个连接并开启事务
      - 忘记 `commit()` 时，`Transaction` 在 drop 时会自动 **rollback**
      - 这意味着"忘了提交"的后果是丢数据，而不是留脏数据 —— 更安全的默认值
    */
    let before = count(&pool).await?;

    let mut tx = pool.begin().await?;
    sqlx::query("INSERT INTO todos (title, completed, created_at) VALUES (?, ?, ?)")
        .bind("事务内插入 A")
        .bind(false)
        .bind(Utc::now())
        .execute(&mut *tx) // ⚠️ 传 &mut *tx 而不是 &pool
        .await?;
    sqlx::query("INSERT INTO todos (title, completed, created_at) VALUES (?, ?, ?)")
        .bind("事务内插入 B")
        .bind(false)
        .bind(Utc::now())
        .execute(&mut *tx)
        .await?;
    tx.commit().await?;

    println!("事务提交：{} 条 → {} 条", before, count(&pool).await?);

    // 演示回滚：不调用 commit，让 tx 在作用域结束时 drop
    {
        let mut tx2 = pool.begin().await?;
        sqlx::query("INSERT INTO todos (title, completed, created_at) VALUES (?, ?, ?)")
            .bind("这条会被回滚")
            .bind(false)
            .bind(Utc::now())
            .execute(&mut *tx2)
            .await?;

        // 在事务**内部**计数：能看到未提交的那条（说明改动只对本连接可见）
        let inside = count(&mut *tx2).await?;
        println!("事务内看到的行数 = {}（含未提交的那条）", inside);

        // 没有 tx2.commit() → drop 时自动 rollback
        println!("事务未提交，即将离开作用域（自动回滚）");
    }
    println!("回滚后总数 = {}（比事务内少 1，说明那条插入被撤销了）", count(&pool).await?);

    // --------------------------------------------
    // 2.6 类型映射
    // --------------------------------------------
    println!();
    println!("2.6 类型映射速查（SQLite 只有 5 种存储类别）");
    println!("------------------------------------------------------------");
    println!("Rust i64      <-> INTEGER");
    println!("Rust f64      <-> REAL");
    println!("Rust String   <-> TEXT      （&str 可 bind，不能直接作为查询返回类型）");
    println!("Rust Vec<u8>  <-> BLOB");
    println!("Rust bool     <-> INTEGER   0/1（SQLite 没有 BOOLEAN）");
    println!("Rust Option<T><-> NULL      （列可空就必须用 Option）");
    println!("chrono 类型   <-> TEXT      （ISO 8601 格式，需开启 chrono feature）");

    // MAX(id) 可能为 NULL（空表），所以必须 Option<i64>
    let max_id: (Option<i64>,) = sqlx::query_as("SELECT MAX(id) FROM todos")
        .fetch_one(&pool)
        .await?;
    println!("MAX(id) 的解码类型必须是 Option<i64> → {:?}", max_id.0);

    println!();
    println!("============================================================");
    println!("第三部分：深入理解（概念，无需运行）");
    println!("============================================================");
    println!("见文件末尾注释：query! 的编译期校验 / 连接池原理 / SQLite 的并发真相");

    Ok(())
}

/// 从 `todos` 表查出的一行
///
/// `#[derive(FromRow)]` 生成的实现按**字段名**匹配列名。
/// 所以 SQL 里必须 `SELECT id, title, ...` 把列名显式写出来，
/// `SELECT *` 也能工作，但列名一改就悄悄失效。
#[derive(Debug, FromRow)]
struct TodoRow {
    id: i64,
    title: String,
    /// SQLite 里是 0/1，sqlx 自动转成 bool
    completed: bool,
    /// 列可空 → 必须 Option
    note: Option<String>,
    /// 存的是 TEXT，sqlx 借 chrono feature 解码成 DateTime<Utc>
    created_at: DateTime<Utc>,
}

/// 统计表内行数的小工具
///
/// 参数写成泛型 `impl Executor`，而不是 `&SqlitePool`，
/// 这样它既能接受连接池，也能接受事务（`&mut *tx`）。
/// 这是 sqlx 的组织习惯：**凡是查询函数都应对 Executor 泛型**，
/// 否则就无法在事务里复用。
async fn count<'e, E>(executor: E) -> Result<i64, sqlx::Error>
where
    E: sqlx::Executor<'e, Database = sqlx::Sqlite>,
{
    let row = sqlx::query("SELECT COUNT(*) AS cnt FROM todos")
        .fetch_one(executor)
        .await?;
    Ok(row.get("cnt"))
}

// ============================================
// 第三部分：深入理解（可选）
// ============================================

/*
如果你想知道"为什么"，读这一节。觉得难可以跳过，不影响后续学习。

【1】连接池到底解决什么问题

    建立一个数据库连接的成本远高于一次查询：
      - TCP 三次握手
      - 认证（SCRAM / TLS 握手）
      - 初始化会话状态
    这通常是**毫秒级**，而一次简单查询是**微秒级**。
    每个请求新建连接，开销比业务逻辑大两个数量级。

    连接池的做法：启动时建好 N 个连接放在队列里。
      `pool.acquire()` 借出一个，drop 时归还。
      并发超过 N 时，后来的请求**排队等待**，而不是压垮数据库。

    参数经验值（PostgreSQL）：
      max_connections ≈ CPU 核数 × 2 ~ 4
    不是越大越好：数据库侧每个连接都有内存开销与锁竞争，
    1000 个连接会让数据库比 20 个连接更慢。

    SQLite 特殊：写操作全局串行（见【4】），连接池主要是为了
    让读操作并行，max_connections 给 5~10 就够。

【2】query! 宏：编译期连库校验的原理

    写法：
        let rows = sqlx::query!("SELECT id, title FROM todos WHERE id = ?", id)
            .fetch_all(&pool).await?;

    编译时 sqlx 会：
      1. 读环境变量 `DATABASE_URL` 连上数据库
      2. 把这条 SQL 发给数据库做**预处理**（prepare）
      3. 拿到结果集的列名与类型
      4. 生成一个**匿名的行结构体**，字段类型与列类型严格对应
      5. 把预处理语句缓存在 `.sqlx/` 目录（供 CI 离线校验）

    收益：
      - SQL 写错（表名、列名）→ **编译失败**，而不是运行时 panic
      - 类型不匹配（把 TEXT 当 i64 解码）→ 编译失败
      - 字段名由宏生成，不用手写元组

    代价（必须知道）：
      - 编译依赖数据库可达（CI 里要用 `cargo sqlx prepare` 生成的 `.sqlx/` 缓存）
      - SQL 字符串必须是**字面量**，不能是 `format!` 拼出来的 —— 宏看不到运行时值
      - `cargo check` 在干净的机器上会失败，除非提交了 `.sqlx/`

    判断标准：
      固定 SQL（绝大多数 CRUD）→ 用 `query!`，白拿类型检查
      动态 SQL（条件拼接、排序字段可变）→ 用 `query_as`，只能运行时校验

【3】SQLite 的"数据库文件"到底能撑多大

    别低估它：SQLite 官方测试过数十 GB 的库，单表上亿行。
    - 读：并发无锁（MVCC 的 WAL 模式）
    - 写：**全局串行**（同一时刻只有一个写者）
    - 适合：读多写少、单机、嵌入式、中小型服务
    - 不适合：高写入并发的多机服务 → 用 PostgreSQL

    `todo-api` 这类单机项目用 SQLite 是**正确选择**，不是"简化版"。
    上真机后要做的第一件事是开 WAL：
        PRAGMA journal_mode = WAL;
    它让读不被写阻塞，把并发能力提升一个量级。

【4】"写操作全局串行"到底意味着什么

    两个 POST 请求同时到达 → 它们的 INSERT 必须排队。
    如果你的接口里有"读-改-写"逻辑（先 SELECT 再 UPDATE），
    没有事务保护就会丢失更新：

        请求 A: SELECT completed=0
        请求 B: SELECT completed=0
        请求 A: UPDATE completed=1
        请求 B: UPDATE completed=1        ← A 的修改被覆盖，无感知

    正确做法三种（按代价从低到高）：
      1. 单条原子 UPDATE：
             UPDATE todos SET completed = NOT completed WHERE id = ?
      2. 加版本号（乐观锁）：
             UPDATE todos SET ... , version = version + 1
             WHERE id = ? AND version = ?
         用 `rows_affected() == 0` 判断"被别人改过"，返回 409
      3. 事务 + SELECT ... FOR UPDATE（PostgreSQL/MySQL 支持，
         SQLite 上用 `BEGIN IMMEDIATE` 等价）

    这是数据库入门最重要的一个概念：**并发不是"加了数据库就解决"，是"要显式设计"。**

【5】迁移（migrations）：表结构不是一次成型的

    手写 `CREATE TABLE` 在 demo 里可以，真实项目必须版本化：

        sqlx migrate add create_todos      # 生成 migrations/20261029_create_todos.sql
        sqlx migrate run                    # 按顺序执行未执行过的

    sqlx 在库里维护一张 `_sqlx_migrations` 表记录已执行的版本。
    或者用代码内嵌：
        sqlx::migrate!("./migrations").run(&pool).await?;

    **永远不要**手工改线上表结构（`ALTER TABLE` 敲错一次就可能丢数据）。
    这类操作的唯一合法入口是迁移文件，它进 git、能 review、能回滚。

【6】为什么 sqlx 不做 ORM

    对比两条路线（权衡而非优劣）：

    完整 ORM（Diesel / SeaORM / Django ORM）
      + 关系导航方便（`user.todos()` 自动 JOIN）
      + 编译期保证 schema 与代码一致
      - 生成的 SQL 不可见，性能问题难定位
      - N+1 查询是常客
      - 复杂查询要"跳出 ORM 写裸 SQL"，两套体系并存

    查询构建器（sqlx）
      + SQL 完全可见，可以原样贴到数据库客户端里 EXPLAIN
      + 学习成本低：会 SQL 就会 sqlx
      - 手写 SQL，字段名拼错只能靠 `query!` 或测试发现
      - 没有关系导航，JOIN 要自己写

    Rust 生态当前的主流判断：**业务简单 → sqlx；领域模型复杂 → SeaORM**。
    两者都不用 Diesel，因为它的类型体操学习曲线过陡。
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

/*
// --------------------------------------------
// 4.1 错误：用 format! 拼 SQL（SQL 注入）
// --------------------------------------------
❌ 错误代码：
```rust
let title = req.title;
let sql = format!("SELECT * FROM todos WHERE title = '{}'", title);
sqlx::query(&sql).fetch_all(&pool).await?;
```
当 `title` 是 `' OR '1'='1` 时，SQL 变成：
    SELECT * FROM todos WHERE title = '' OR '1'='1'
返回全表。更糟的是 `'; DROP TABLE todos; --`。

原因：参数被当成 SQL **代码**而非**数据**解析。

✅ 正确代码：
```rust
sqlx::query("SELECT * FROM todos WHERE title = ?")
    .bind(title)          // bind 走协议层，永远只当数据
    .fetch_all(&pool).await?;
```

记忆技巧：**SQL 里只允许字面量，变量一律 bind。**
这条规则没有例外，包括 ORDER BY 的字段名（那种情况要用白名单校验，不能 bind）。


// --------------------------------------------
// 4.2 错误：内存库 + 连接池，报 no such table
// --------------------------------------------
❌ 错误代码：
```rust
let pool = SqlitePool::connect("sqlite::memory:").await?;
sqlx::query("CREATE TABLE t (id INTEGER)").execute(&pool).await?;
// 后续查询有时报错有时不报
sqlx::query("SELECT * FROM t").fetch_all(&pool).await?;
```

运行时错误（不一定每次都出现）：
```
error: no such table: t
```

原因：`sqlite::memory:` 是**每个连接一份**的私有内存数据库。
建表用了连接 A，查询可能被分到连接 B —— B 的库里没有这张表。
因为是随机分配，症状表现为"时好时坏"，极难排查。

✅ 两种正确做法：
```rust
// 方案一：锁成单连接（本文件采用）
SqlitePoolOptions::new().max_connections(1).connect("sqlite::memory:").await?;

// 方案二：共享缓存
SqlitePool::connect("sqlite::memory:?cache=shared").await?;
```

记忆技巧：**`:memory:` ≠ 一个共享的内存库，是"每个连接一个库"。**
测试代码里这个坑几乎人人踩一次。


// --------------------------------------------
// 4.3 错误：可空列用非 Option 类型解码
// --------------------------------------------
❌ 错误代码：
```rust
let row: (i64, String) = sqlx::query_as("SELECT id, note FROM todos")
    .fetch_one(&pool).await?;
```
当 `note` 是 NULL 时，运行时错误：
```
error: `UnexpectedNullError`: unexpected null value
```

原因：`String` 不能表达 NULL。类型系统在这里恰好是诚实的——
它逼你显式处理"没有值"的情况。

✅ 正确代码：
```rust
let row: (i64, Option<String>) = sqlx::query_as("SELECT id, note FROM todos")
    .fetch_one(&pool).await?;
match row.1 {
    Some(note) => println!("note = {}", note),
    None => println!("note 为空"),
}
```

记忆技巧：**建表时写了 `NULL`，解码就必须写 `Option`。**
规则一一对应，没有例外。


// --------------------------------------------
// 4.4 错误：事务里传 &pool 而不是 &mut *tx
// --------------------------------------------
❌ 错误代码：
```rust
let mut tx = pool.begin().await?;
sqlx::query("INSERT INTO t VALUES (?)").bind(1)
    .execute(&pool)          // ← 走的是池里的另一个连接
    .await?;
tx.commit().await?;
```

为什么是问题：**能编译、能运行、事务看起来"成功"了**，
但那条 INSERT 根本不在事务里（它用的是池中另一条连接）。
回滚时它不会被撤销，隔离级别也不适用。
这是最阴险的一类 bug：没有报错，只有错误的结果。

✅ 正确代码：
```rust
.execute(&mut *tx).await?;   // 显式解引用 Transaction，拿到它持有的连接
```

记忆技巧：**事务内的每一条语句都必须挂在 `tx` 上，不能挂 `pool`。**
看到 `pool.begin()` 之后还有 `&pool`，就一定有 bug。


// --------------------------------------------
// 4.5 错误：以为 UPDATE 不存在的行为会报错
// --------------------------------------------
❌ 错误代码：
```rust
sqlx::query("UPDATE todos SET title = ? WHERE id = ?")
    .bind("新标题").bind(id)
    .execute(&pool).await?;
return Ok(HttpResponse::Ok().json(/* 假装成功 */));
```

为什么是问题：id 不存在时**不会报错**，`rows_affected()` 是 0，
但代码把 200 返回给了客户端。前端以为改成功了。

✅ 正确代码：
```rust
let r = sqlx::query("UPDATE todos SET title = ? WHERE id = ?")
    .bind("新标题").bind(id)
    .execute(&pool).await?;
if r.rows_affected() == 0 {
    return Err(NotFound);        // → 404
}
```

记忆技巧：**`rows_affected()` 是 UPDATE / DELETE 的返回值，
不是可选信息，是"操作是否真的命中"的唯一证据。**


// --------------------------------------------
// 4.6 错误：在异步代码里持有连接做长时间计算
// --------------------------------------------
❌ 错误代码：
```rust
let mut conn = pool.acquire().await?;
let rows = sqlx::query("SELECT * FROM big").fetch_all(&mut *conn).await?;
let report = heavy_cpu_work(&rows);      // 持有连接做 3 秒 CPU 计算
drop(conn);
```
后果：连接池里少一个可用连接整整 3 秒。
并发一高，其他请求全部排在 `acquire()` 上，超时。

✅ 正确代码：先取数据、立即还连接
```rust
let rows = {
    let mut conn = pool.acquire().await?;
    sqlx::query("SELECT * FROM big").fetch_all(&mut *conn).await?
};                                        // conn 在此 drop，连接归还
let report = heavy_cpu_work(&rows);
```
更好的做法：CPU 重活丢给 `tokio::task::spawn_blocking`。

记忆技巧：**连接的持有时间应覆盖"数据库交互"，不覆盖"业务计算"。**


// --------------------------------------------
// 4.7 错误：query! 宏在 CI 上编译失败
// --------------------------------------------
❌ 错误代码（在干净 CI 机器上）：
```rust
let x = sqlx::query!("SELECT id FROM todos").fetch_all(&pool).await?;
```
CI 错误：
```
error: error communicating with database: Connection refused
 --> src/main.rs:5:13
```

原因：`query!` 在**编译期**要连数据库校验。CI 上没有数据库，也没有 `.sqlx/` 缓存。

✅ 正确代码：
```bash
# 本地（数据库可达时）生成离线缓存
cargo install sqlx-cli --no-default-features --features sqlite
cargo sqlx prepare
git add .sqlx/          # 提交缓存，CI 就不需要数据库了
```
并在 CI 里设置：
```bash
export SQLX_OFFLINE=true
```

记忆技巧：**用了 `query!` 就必须提交 `.sqlx/` 目录。**
这是 sqlx 唯一的"额外记性负担"，换来的是编译期 SQL 校验。


// --------------------------------------------
// 4.8 错误：连接串忘了建库模式
// --------------------------------------------
❌ 错误代码：
```rust
let pool = SqlitePool::connect("sqlite:data/app.db").await?;
```
首次运行时错误：
```
error: unable to open database file
```

原因：SQLite 打不开不存在的库文件，除非显式要求"不存在就创建"。

✅ 正确代码：
```rust
SqlitePool::connect("sqlite:data/app.db?mode=rwc").await?;
//                                            ^^^^^^^^ r=read w=write c=create
```
或者用显式选项：
```rust
use sqlx::sqlite::SqliteConnectOptions;
let opts = SqliteConnectOptions::new()
    .filename("data/app.db")
    .create_if_missing(true);
let pool = SqlitePoolOptions::new().connect_with(opts).await?;
```

记忆技巧：**SQLite 的连接串不是"路径"，是一条 DSN。**
`mode=rwc` 是本地开发最常漏的三个字符。
*/

// ============================================
// 第五部分：函数/宏卡片速查
// ============================================

/*
【类型：SqlitePool】

语法：SqlitePool::connect(dsn: &str) -> Result<Pool, Error>
      SqlitePoolOptions::new().max_connections(n).connect(dsn)
作用：连接池，可 Clone（内部是 Arc），在 handler 间共享
DSN 示例：
  "sqlite::memory:"                       内存库（每个连接独立！）
  "sqlite::memory:?cache=shared"          共享缓存的内存库
  "sqlite:data/app.db?mode=rwc"           文件库，不存在则创建
关键：Pool 的 clone 是廉价的（Arc），必须作为 axum/actix 的共享状态
────────────────────────────────────

【函数：sqlx::query】

语法：sqlx::query(sql: &str) -> Query
方法：.bind(v) / .execute(&pool) / .fetch_one / .fetch_optional / .fetch_all
返回：execute → QueryResult；fetch_* → Row（需手动 get）
适用：动态 SQL、DDL、聚合查询
示例：
```rust
let r = sqlx::query("DELETE FROM t WHERE id = ?").bind(1).execute(&pool).await?;
println!("{}", r.rows_affected());
```
────────────────────────────────────

【函数：sqlx::query_as】

语法：sqlx::query_as::<_, T>(sql) -> QueryAs<T>
      T: FromRow（元组或 #[derive(FromRow)] 结构体）
返回：T / Option<T> / Vec<T>
校验：**运行时**（列与字段不匹配时报编码/解码错误）
示例：
```rust
let r: (i64, String) = sqlx::query_as("SELECT id, name FROM t")
    .fetch_one(&pool).await?;
```
────────────────────────────────────

【宏：sqlx::query!】

语法：sqlx::query!("SELECT id, title FROM t WHERE id = ?", id)
返回：匿名结构体（字段名 == 列名）
校验：**编译期**（需 DATABASE_URL 或 .sqlx/ 缓存）
限制：SQL 必须是字面量；CI 需 SQLX_OFFLINE=true + 提交的 .sqlx/
示例：
```rust
let rows = sqlx::query!("SELECT id FROM t").fetch_all(&pool).await?;
for r in rows { println!("{}", r.id); }
```
────────────────────────────────────

【trait：FromRow】

语法：#[derive(sqlx::FromRow)] struct S { .. }
作用：按字段名把一行映射成结构体
注意：列名与字段名必须一致（用 `#[sqlx(rename = "...")]` 可改）
示例：
```rust
#[derive(FromRow)]
struct Todo { id: i64, title: String }
```
────────────────────────────────────

【方法：Pool::begin】

语法：pool.begin() -> Result<Transaction<'_, DB>, Error>
返回：事务句柄，RAII，drop 未 commit 则自动 rollback
关键：事务内所有语句用 `&mut *tx`，不能用 `&pool`
示例：
```rust
let mut tx = pool.begin().await?;
sqlx::query("...").execute(&mut *tx).await?;
tx.commit().await?;
```
────────────────────────────────────

【方法：QueryResult::rows_affected / last_insert_rowid】

rows_affected()       -> u64  影响行数（UPDATE/DELETE 命中判断的唯一依据）
last_insert_rowid()   -> i64  本次 INSERT 生成的自增主键（SqliteQueryResult）
示例：
```rust
let r = sqlx::query("INSERT ...").execute(&pool).await?;
println!("id={} rows={}", r.last_insert_rowid(), r.rows_affected());
```
────────────────────────────────────

【方法：Row::get】

语法：row.get::<T, _>(index_or_name) -> T
作用：query（非 query_as）取列的底层方式
注意：类型写错会在**运行时** panic（不是 Result），用 `try_get` 可得 Result
示例：
```rust
let n: i64 = row.get("cnt");
let n: i64 = row.try_get("cnt")?;
```
────────────────────────────────────

【工具：sqlx-cli】

安装：cargo install sqlx-cli --no-default-features --features sqlite
命令：
  sqlx database create          创建数据库
  sqlx migrate add <name>       生成迁移文件
  sqlx migrate run              执行未运行的迁移
  sqlx prepare                  生成 .sqlx/ 离线缓存（CI 用）
  sqlx database reset           清库重建（本地开发）
────────────────────────────────────

【类型映射（SQLite）】

i64 ↔ INTEGER      f64 ↔ REAL      String ↔ TEXT
Vec<u8> ↔ BLOB     bool ↔ INTEGER(0/1)
Option<T> ↔ NULL   DateTime<Utc> ↔ TEXT(需 chrono feature)
────────────────────────────────────

【Cargo 依赖】

```toml
sqlx = { version = "0.8", features = ["runtime-tokio", "sqlite", "chrono"] }
tokio = { version = "1", features = ["full"] }
chrono = "0.4"
```
feature 说明：
  runtime-tokio   异步运行时（必须）
  sqlite / postgres / mysql   数据库驱动
  chrono / time   时间类型支持
  uuid            UUID 类型
  json            serde_json 字段
  migrate         内嵌迁移
*/

// ============================================
// 第六部分：练习题（不提供答案）
// ============================================

/*
【练习 1：基础 —— 补全 CRUD】
要求：
  - 建表 `notes (id INTEGER PRIMARY KEY AUTOINCREMENT, body TEXT NOT NULL, created_at TEXT NOT NULL)`
  - 写四个函数：`create_note` / `get_note` / `list_notes` / `delete_note`
  - 每个函数返回 `Result<_, sqlx::Error>`
提示：
  - `get_note` 返回 `Option<...>`（用 `fetch_optional`），不要用 `fetch_one`
  - `delete_note` 要检查 `rows_affected()`
注意：
  - SQL 里的变量一律 `bind`，不要 `format!`

【练习 2：基础 —— 修 4.2 的那个坑】
要求：
  - 复现"内存库 + 多连接 → no such table"
  - 用两种方案分别修好：`max_connections(1)` 和 `?cache=shared`
  - 记录两种方案的行为差异
提示：
  - 复现时需要**多次**执行查询，因为这个 bug 是概率性的
  - `?cache=shared` 下连接仍然各自独立，但共享底层页缓存
注意：
  - 不要因此得出"内存库不可靠"的结论。它只是"每个连接一个库"的设计

【练习 3：进阶 —— 用事务实现"安全的转账"】
要求：
  - 建 `accounts (id, balance)`，插入两个账户各 100
  - 写 `transfer(from, to, amount)`，用事务保证两笔 UPDATE 原子
  - 构造余额不足的情况，验证回滚后两边余额都没变
提示：
  - `UPDATE ... SET balance = balance - ? WHERE id = ? AND balance >= ?`
    然后用 `rows_affected() == 0` 判断余额不足，返回错误并让 tx drop
  - 注意：出错时要 `return Err(...)`，让 `tx` 在离开作用域时自动回滚
注意：
  - 这题是练习 4 的种子题（并发下的丢失更新），做完先想想两个 transfer
    同时执行会怎样

【练习 4：进阶 —— 乐观锁】
要求：
  - 给 `todos` 表加 `version INTEGER NOT NULL DEFAULT 0`
  - 实现 `update_with_version(id, new_title, expected_version)`
  - 返回三态：成功 / 不存在 / 版本冲突
提示：
  - `UPDATE ... SET title=?, version=version+1 WHERE id=? AND version=?`
  - `rows_affected() == 0` 时再查一次存在性，区分 404 和 409
注意：
  - 这是真实生产代码里的标准模式，不是练习题造出来的场景

【练习 5：高阶 —— 接入 `query!` 宏】
要求：
  - 用 `sqlx-cli` 创建数据库和迁移
  - 把练习 1 的四个函数改成 `query!` 版本
  - 执行 `cargo sqlx prepare`，提交 `.sqlx/`
  - 删掉数据库文件，验证 `SQLX_OFFLINE=true cargo build` 仍能通过
提示：
  - `DATABASE_URL=sqlite:notes.db` 需要在 `.env` 里（sqlx 会自动读）
  - `query!` 返回的字段名就是列名，空值对应 `Option<T>`
注意：
  - 这一步做完你就理解了"编译期 SQL 校验"的收益与代价

【练习 6：综合 —— 决定 todo-api 怎么迁移】
要求：
  - 列出把 todo-api 从 JSON 换成 SQLite 需要改的所有文件
  - 给出迁移步骤（按依赖顺序，不能乱）
  - 判断：事务要不要加？索引要加在哪一列？
提示：
  - 从 `persistence.rs` 的 `save`/`load` 两个方法出发反推
  - 索引判断依据是**查询条件**，不是"哪个字段看着重要"
注意：
  - 这题的产出是第 32–33 天的施工图，写进日志
*/

// ============================================
// 文件结束
// ============================================
