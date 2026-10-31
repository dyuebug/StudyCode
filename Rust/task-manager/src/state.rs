// ============================================
// task-manager 共享状态
// 日期：2026-11-01（第37天）
// ============================================

use sqlx::sqlite::SqlitePool;

/// 应用状态
///
/// 只有连接池一个字段——这是刻意的。
///
/// 【为什么不把"当前用户"放进状态】
/// 状态是**进程级**的、被所有请求共享的。
/// 把请求相关的东西（当前用户、请求 ID、追踪号）放进来，
/// 并发请求会互相覆盖，症状是"偶尔看到别人的数据"——
/// 一个既难复现、又极其严重的越权 bug。
///
/// 请求级数据走 `HttpRequest::extensions()`（见 `mw.rs`）：
/// 它随请求走，天然隔离。
///
/// 一句话：**进程级的东西放状态，请求级的东西放 extensions。**
#[derive(Clone)]
pub struct AppState {
    pub pool: SqlitePool,
}

impl AppState {
    pub fn new(pool: SqlitePool) -> Self {
        Self { pool }
    }
}
