// ============================================
// Todo API - 状态管理
// 日期：2026-06-21（第25天）
// ============================================

use crate::models::Todo;
use std::sync::{Arc, Mutex};

// --------------------------------------------
// AppState - 应用状态类型
// --------------------------------------------

/// 应用状态类型定义
///
/// 类型组成：Arc<Mutex<Vec<Todo>>>
/// - Vec<Todo>: 存储所有任务的向量
/// - Mutex<T>: 互斥锁，保证同一时间只有一个线程可以访问数据
/// - Arc<T>: 原子引用计数，允许多个线程共享同一份数据
///
/// 为什么需要这样的结构？
/// - Web 服务器是多线程的，每个请求可能在不同线程处理
/// - 多个请求可能同时访问或修改数据
/// - Mutex 保证数据安全（防止竞态条件）
/// - Arc 允许多个 handler 共享同一份数据
pub type AppState = Arc<Mutex<Vec<Todo>>>;

// --------------------------------------------
// 初始化函数
// --------------------------------------------

/// 初始化应用状态
///
/// 创建一个空的任务列表，包装在 Arc<Mutex<>> 中
///
/// # 返回
/// 初始化的 AppState
///
/// # 示例
/// ```rust
/// let state = init_state();
/// ```
pub fn init_state() -> AppState {
    Arc::new(Mutex::new(Vec::new()))
}

/// 初始化应用状态（带初始数据）
///
/// 用于测试或演示，创建包含示例数据的状态
///
/// # 返回
/// 包含示例任务的 AppState
pub fn init_state_with_data() -> AppState {
    let todos = vec![
        Todo {
            id: 1,
            title: "学习 Rust Web 开发".to_string(),
            completed: false,
        },
        Todo {
            id: 2,
            title: "完成 Todo API 项目".to_string(),
            completed: false,
        },
    ];

    Arc::new(Mutex::new(todos))
}

/*
【关键知识点】

1. Arc<Mutex<T>> 组合模式（第16天学的！）：
   - Arc: Atomic Reference Counting（原子引用计数）
   - Mutex: Mutual Exclusion（互斥锁）
   - 这是 Rust 中多线程共享可变数据的标准模式

2. 为什么不能只用 Vec<Todo>？
   - Vec<Todo> 没有实现 Sync trait
   - 不能在多线程间安全共享

3. 为什么不能只用 Mutex<Vec<Todo>>？
   - Mutex<T> 本身不能被多个线程共享
   - 需要 Arc 来包装，允许多个所有者

4. type 别名：
   - 简化复杂类型的书写
   - AppState 比 Arc<Mutex<Vec<Todo>>> 更易读
   - 提高代码可维护性

5. 线程安全保证：
   - Mutex::lock() 获取锁（阻塞其他线程）
   - 锁守卫离开作用域时自动释放
   - 防止数据竞争（Data Race）

6. Web 服务器的多线程模型：
   - Actix-web 默认使用多个工作线程
   - 每个请求可能在不同线程处理
   - 状态必须是线程安全的（Send + Sync）
*/
