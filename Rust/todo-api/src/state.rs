// ============================================
// Todo API - 状态管理
// 日期：2026-06-22（第27天：加上持久化管理器）
// 更新：2026-10-29（第33天：改为持有数据库连接池）
// ============================================

use sqlx::sqlite::SqlitePool;

/// 应用状态结构
///
/// 【第 33 天的关键变化】
/// 之前是：
/// ```text
/// AppState { todos: Arc<Mutex<Vec<Todo>>>, persistence: Arc<Mutex<PersistenceManager>> }
/// ```
/// 现在是：
/// ```text
/// AppState { pool: SqlitePool }
/// ```
///
/// 为什么能砍掉两个 `Arc<Mutex<>>`？
/// - `SqlitePool` 内部已经是 `Arc`，clone 只是增加引用计数
/// - 并发安全由**数据库**负责（事务 + 锁），不再需要用 Mutex 保护一个 Vec
/// - 于是"锁的持有时间""两个 Mutex 的加锁顺序""锁守卫不能跨 await"
///   这一整类问题**从代码里消失了**
///
/// 这正是引入数据库最容易被忽视的收益：**它替你做掉了并发正确性**。
#[derive(Clone)]
pub struct AppState {
    /// 数据库连接池
    ///
    /// ⚠️ 存「池」，不存「连接」。
    /// `PoolConnection` 是借出来的短期资源，存进结构体等于永久占用一条连接，
    /// 整个应用只剩 max_connections - 1 条可用。
    pub pool: SqlitePool,
}

impl AppState {
    /// 创建应用状态
    ///
    /// # 参数
    /// - `pool`: 已初始化并跑完迁移的连接池
    pub fn new(pool: SqlitePool) -> Self {
        Self { pool }
    }
}

/*
【关键知识点】

1. 为什么 Pool 可以直接 Clone 而不需要 Arc
   `SqlitePool = Pool<Sqlite>`，内部是 `Arc<PoolInner>`。
   clone 一次 ≈ 一次原子加引用计数。

2. actix-web 的共享方式没有变
   仍然是 `web::Data::new(state.clone())` + handler 里 `web::Data<AppState>`。
   变的只是 AppState 内部的字段。

3. 不再需要 Mutex，不代表不再有并发问题
   "先查后改"的丢失更新依然存在（见 §handler 的 PUT 说明）。
   数据库把问题从"内存数据竞争"降级成"逻辑层竞态"——
   **更好解决，但不是不存在。**

4. 为什么不在 AppState 里存 config
   YAGNI。当前没有可配置项，加一个空结构体只是噪音。
   真需要时再加，clone 成本几乎为零。
*/
