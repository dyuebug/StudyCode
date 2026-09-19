// ============================================
// Todo API - 状态管理（带持久化）
// 日期：2026-06-22（第27天更新）
// ============================================

use crate::models::Todo;
use crate::persistence::PersistenceManager;
use std::sync::{Arc, Mutex};

// --------------------------------------------
// AppState - 应用状态结构
// --------------------------------------------

/// 应用状态结构
///
/// 包含两部分：
/// 1. todos: 任务数据（内存）
/// 2. persistence: 持久化管理器（负责文件读写）
///
/// 为什么使用结构体？
/// - 之前只有 Vec<Todo>，现在需要添加持久化功能
/// - 结构体可以组合多个字段
/// - 更好的扩展性
#[derive(Clone)]
pub struct AppState {
    /// 任务数据（内存）
    /// Arc<Mutex<>> 保证多线程安全访问
    pub todos: Arc<Mutex<Vec<Todo>>>,

    /// 持久化管理器
    /// Arc<Mutex<>> 保证多线程安全写入
    pub persistence: Arc<Mutex<PersistenceManager>>,
}

impl AppState {
    /// 创建新的应用状态
    ///
    /// # 参数
    /// - file_path: 数据文件路径（如 "todos.json"）
    ///
    /// # 流程
    /// 1. 创建持久化管理器
    /// 2. 尝试加载历史数据
    /// 3. 如果加载失败，使用空数组
    ///
    /// # 返回
    /// 初始化的 AppState
    pub fn new(file_path: &str) -> Self {
        // 创建持久化管理器
        let persistence = PersistenceManager::new(file_path);

        // 尝试加载历史数据
        let todos = match persistence.load() {
            Ok(loaded_todos) => {
                if loaded_todos.is_empty() {
                    println!("📂 数据文件不存在或为空，从零开始");
                } else {
                    println!("📂 成功加载 {} 条历史数据", loaded_todos.len());
                }
                loaded_todos
            }
            Err(e) => {
                eprintln!("⚠️  加载历史数据失败: {}", e);
                eprintln!("⚠️  将从空数据开始");
                vec![]
            }
        };

        Self {
            todos: Arc::new(Mutex::new(todos)),
            persistence: Arc::new(Mutex::new(persistence)),
        }
    }

    /// 保存当前数据到文件
    ///
    /// 这是一个异步方法，可以在 handler 中调用
    ///
    /// # 返回
    /// - Ok(()): 保存成功
    /// - Err(String): 保存失败，包含错误信息
    ///
    /// # 示例
    /// ```rust
    /// if let Err(e) = state.save_to_file().await {
    ///     eprintln!("保存失败: {}", e);
    /// }
    /// ```
    pub async fn save_to_file(&self) -> Result<(), String> {
        // 1. 获取 todos 的锁，读取数据
        let todos = self.todos.lock()
            .map_err(|e| format!("获取 todos 锁失败: {}", e))?
            .clone(); // 克隆数据，避免长时间持有锁

        // 2. 获取 persistence 的锁，保存数据
        let persistence = self.persistence.lock()
            .map_err(|e| format!("获取 persistence 锁失败: {}", e))?;

        // 3. 调用 save 方法（原子写入）
        persistence.save(&todos)?;

        Ok(())
    }
}

// --------------------------------------------
// 初始化函数（保持兼容性）
// --------------------------------------------

/// 初始化应用状态
///
/// 默认使用 "todos.json" 作为数据文件
///
/// # 返回
/// 初始化的 AppState
pub fn init_state() -> AppState {
    AppState::new("todos.json")
}

/// 初始化应用状态（指定文件路径）
///
/// # 参数
/// - file_path: 数据文件路径
///
/// # 返回
/// 初始化的 AppState
pub fn init_state_with_file(file_path: &str) -> AppState {
    AppState::new(file_path)
}

/*
【关键知识点】

1. 从 type 别名到结构体：
   旧版本：
   ```rust
   pub type AppState = Arc<Mutex<Vec<Todo>>>;
   ```

   新版本：
   ```rust
   pub struct AppState {
       pub todos: Arc<Mutex<Vec<Todo>>>,
       pub persistence: Arc<Mutex<PersistenceManager>>,
   }
   ```

   为什么改变？
   - 需要添加新字段（persistence）
   - type 别名只能表示单一类型
   - 结构体更灵活，可以添加方法

2. #[derive(Clone)]：
   - 为 AppState 自动实现 Clone trait
   - 因为 Arc 实现了 Clone（只是增加引用计数）
   - handler 中需要克隆 state

3. 启动时加载历史数据：
   - 在 new() 方法中自动加载
   - 如果文件不存在，返回空数组（正常情况）
   - 如果加载失败，打印错误并使用空数组

4. save_to_file() 方法：
   - 异步方法（async fn）
   - 可以在 handler 中调用
   - 使用原子写入保证安全

5. 锁的使用策略：
   - 先获取 todos 的锁，克隆数据
   - 立即释放锁（避免阻塞其他请求）
   - 再获取 persistence 的锁，保存数据
   - 最小化锁的持有时间

6. 错误处理：
   - 使用 Result<(), String> 返回类型
   - map_err() 转换错误信息
   - ? 运算符传播错误

7. 保持向后兼容：
   - init_state() 函数保持不变
   - 旧代码无需修改
   - 平滑迁移

8. 为什么 persistence 也需要 Mutex？
   - 文件写入不是线程安全的
   - 多个线程可能同时保存数据
   - Mutex 保证同一时间只有一个写入操作
*/

