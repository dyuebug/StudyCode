// ============================================
// Todo API - 数据模型
// 日期：2026-06-21（第25天）
// ============================================

use serde::{Deserialize, Serialize};

// --------------------------------------------
// Todo - 任务数据结构
// --------------------------------------------

/// Todo 任务结构
///
/// 字段说明：
/// - id: 任务唯一标识符
/// - title: 任务标题/内容
/// - completed: 是否完成
#[derive(Serialize, Deserialize, Clone, Debug)]
pub struct Todo {
    pub id: u32,
    pub title: String,
    pub completed: bool,
}

impl Todo {
    /// 创建新的 Todo 任务
    ///
    /// # 参数
    /// - id: 任务ID
    /// - title: 任务标题
    ///
    /// # 返回
    /// 新创建的 Todo 实例，completed 默认为 false
    pub fn new(id: u32, title: String) -> Self {
        Todo {
            id,
            title,
            completed: false,  // 新任务默认未完成
        }
    }
}

// --------------------------------------------
// CreateTodo - 创建任务的请求数据
// --------------------------------------------

/// 创建任务的请求结构
///
/// 只需要提供 title，id 由服务器自动生成
#[derive(Deserialize, Debug)]
pub struct CreateTodo {
    pub title: String,
}

// --------------------------------------------
// UpdateTodo - 更新任务的请求数据
// --------------------------------------------

/// 更新任务的请求结构
///
/// 所有字段都是 Option，允许部分更新
/// - Some(value): 更新该字段
/// - None: 保持原值不变
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
【关键知识点】

1. Serialize vs Deserialize：
   - Serialize：结构 → JSON（用于响应）
   - Deserialize：JSON → 结构（用于请求）
   - Todo 两者都需要（请求和响应都用）

2. Clone trait：
   - Todo 需要 Clone 是因为 Vec<Todo> 在多线程中需要克隆
   - Mutex 锁住数据时，读取需要克隆出副本

3. Debug trait：
   - 用于调试打印
   - 使用 {:?} 或 {:#?} 格式化输出

4. pub 关键字：
   - 让其他模块可以访问这些类型
   - 字段也需要 pub 才能被外部访问

5. Option<T> 的使用：
   - UpdateTodo 中使用 Option 实现部分更新
   - 客户端可以只更新需要改变的字段
*/
