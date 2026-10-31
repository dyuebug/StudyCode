// ============================================
// Todo API - 数据模型
// 日期：2026-06-21（第25天）创建
// 更新：2026-10-29（第33天）迁移到 SQLite，id 改为 i64
// ============================================

use serde::{Deserialize, Serialize};

// --------------------------------------------
// Todo - 任务数据结构
// --------------------------------------------

/// Todo 任务结构
///
/// 字段说明：
/// - `id`: 数据库主键。SQLite 的 INTEGER 是**64 位有符号整数**，
///   所以用 `i64` 而不是原来的 `u32`（u32 装不下大 id，且与 SQL 类型不匹配）
/// - `title`: 任务标题
/// - `completed`: 是否完成（数据库里存 0/1，sqlx 自动转换）
///
/// `#[derive(FromRow)]` 让 sqlx 按**字段名**把一行映射成这个结构体。
/// 因此 SQL 里要显式写列名（`SELECT id, title, completed`），
/// `SELECT *` 也能工作但列一改就悄悄失效。
///
/// 注意：表里还有 `created_at` / `updated_at` 两列，但**没有**出现在本结构体中。
/// `FromRow` 只要求 SQL 返回的列能填满结构体字段，多余的列会被忽略——
/// 这让"内部字段"与"API 字段"可以分离。
#[derive(Serialize, Deserialize, Clone, Debug, sqlx::FromRow)]
pub struct Todo {
    pub id: i64,
    pub title: String,
    pub completed: bool,
}

impl Todo {
    /// 创建新的 Todo 任务
    ///
    /// # 参数
    /// - `id`: 任务 ID（由数据库生成后回填）
    /// - `title`: 任务标题
    ///
    /// # 返回
    /// 新创建的 Todo 实例，`completed` 默认为 `false`
    pub fn new(id: i64, title: String) -> Self {
        Todo {
            id,
            title,
            completed: false, // 新任务默认未完成
        }
    }
}

// --------------------------------------------
// CreateTodo - 创建任务的请求数据
// --------------------------------------------

/// 创建任务的请求结构
///
/// 只需要提供 title，id 与时间戳由服务器/数据库生成
#[derive(Deserialize, Debug)]
pub struct CreateTodo {
    pub title: String,
}

// --------------------------------------------
// UpdateTodo - 更新任务的请求数据
// --------------------------------------------

/// 更新任务的请求结构
///
/// 所有字段都是 Option，允许部分更新：
/// - `Some(value)`: 更新该字段
/// - `None`: 保持原值不变
///
/// 这个 `Option` 在 handler 里会被直接 bind 进 SQL，
/// 配合 `COALESCE(?, title)` 实现"没传就不改"——一条 SQL 搞定部分更新。
#[derive(Deserialize, Debug)]
pub struct UpdateTodo {
    pub title: Option<String>,
    pub completed: Option<bool>,
}

// --------------------------------------------
// 响应结构
// --------------------------------------------

/// API 响应结构（成功）
#[derive(Serialize)]
pub struct ApiResponse<T> {
    pub success: bool,
    pub data: T,
}

/// API 响应结构（错误）
#[derive(Serialize)]
pub struct ErrorResponse {
    pub success: bool,
    pub error: String,
}

impl<T> ApiResponse<T> {
    /// 创建成功响应
    pub fn success(data: T) -> Self {
        ApiResponse {
            success: true,
            data,
        }
    }
}

impl ErrorResponse {
    /// 创建错误响应
    pub fn error(message: &str) -> Self {
        ErrorResponse {
            success: false,
            error: message.to_string(),
        }
    }
}

/*
【第 33 天迁移要点】

1. id 从 u32 改成 i64
   SQLite 的 INTEGER 是 8 字节有符号整数。用 u32 会在两处出问题：
   - sqlx 解码时类型不匹配
   - 数据库中已有的大 id 无法表达
   跨语言/跨层时，"能装下"比"够用"重要。

2. #[derive(sqlx::FromRow)]
   按字段名映射，不依赖 SQL 的列顺序。代价是列名必须对得上，
   而 SQL 里写错列名只有 `query!` 宏能在编译期发现。

3. 表里有、结构体里没有的列（created_at / updated_at）
   FromRow 容忍多余的列。这给了我们一个干净的边界：
   **时间戳属于存储层，不属于 API 契约**。
   如果将来要暴露，再加字段即可，不会破坏现有字段。

4. Clone 依然需要
   查询结果从 `sqlx::Row` 解码成 `Todo` 后，
   handler 需要在构造响应时持有它——克隆一份最省事。
   （数据库连接本身是可复用的，不再有"锁守卫不能跨 await"的限制了）
*/
