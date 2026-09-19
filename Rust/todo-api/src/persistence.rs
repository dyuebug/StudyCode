// ============================================
// 持久化模块 - 数据保存与加载
// ============================================

use crate::models::Todo;
use serde_json;
use std::fs;
use std::path::Path;

/// 持久化管理器
///
/// 负责将 Todo 数据保存到 JSON 文件，以及从文件加载数据
///
/// # 核心功能
/// - save(): 保存数据到文件（使用原子写入）
/// - load(): 从文件加载数据
///
/// # 线程安全
/// 本身不包含内部可变性，需要配合 Arc<Mutex<>> 使用
#[derive(Debug, Clone)]
pub struct PersistenceManager {
    /// 数据文件路径
    file_path: String,
}

impl PersistenceManager {
    /// 创建新的持久化管理器
    ///
    /// # 参数
    /// - file_path: JSON 文件路径（如 "todos.json"）
    ///
    /// # 示例
    /// ```
    /// let manager = PersistenceManager::new("todos.json");
    /// ```
    pub fn new(file_path: &str) -> Self {
        Self {
            file_path: file_path.to_string(),
        }
    }

    /// 保存数据到文件（使用原子写入）
    ///
    /// # 原子写入流程
    /// 1. 序列化数据为 JSON 字符串
    /// 2. 写入临时文件（.tmp）
    /// 3. 原子重命名临时文件为目标文件
    ///
    /// # 为什么安全？
    /// - 如果写入临时文件失败，原文件不受影响
    /// - 如果重命名失败，原文件不受影响
    /// - 重命名是原子操作（操作系统保证）
    ///
    /// # 参数
    /// - todos: Todo 数组的引用
    ///
    /// # 返回
    /// - Ok(()): 保存成功
    /// - Err(String): 保存失败，包含错误信息
    ///
    /// # 示例
    /// ```
    /// let todos = vec![Todo { id: 1, title: "学习 Rust".to_string(), completed: false }];
    /// manager.save(&todos)?;
    /// ```
    pub fn save(&self, todos: &[Todo]) -> Result<(), String> {
        // 步骤1：序列化为 JSON 字符串（美化格式）
        // to_string_pretty() 生成带缩进和换行的 JSON，便于人类阅读
        let json_string = serde_json::to_string_pretty(todos)
            .map_err(|e| format!("序列化失败: {}", e))?;

        // 步骤2：写入临时文件
        // 临时文件名 = 目标文件名 + .tmp 后缀
        let temp_path = format!("{}.tmp", self.file_path);

        // 将 JSON 字符串写入临时文件
        fs::write(&temp_path, &json_string)
            .map_err(|e| format!("写入临时文件失败 ({}): {}", temp_path, e))?;

        // 步骤3：原子重命名
        // rename() 是原子操作，要么完全成功，要么完全失败
        // 如果目标文件已存在，会被覆盖
        fs::rename(&temp_path, &self.file_path)
            .map_err(|e| {
                // 如果重命名失败，清理临时文件
                let _ = fs::remove_file(&temp_path);
                format!("重命名失败 ({} -> {}): {}", temp_path, self.file_path, e)
            })?;

        // 保存成功
        Ok(())
    }

    /// 从文件加载数据
    ///
    /// # 错误处理策略
    /// - 文件不存在 → 返回空数组（首次启动正常情况）
    /// - 文件读取失败 → 返回错误
    /// - JSON 解析失败 → 返回错误
    ///
    /// # 返回
    /// - Ok(Vec<Todo>): 加载的数据（可能为空数组）
    /// - Err(String): 加载失败，包含错误信息
    ///
    /// # 示例
    /// ```
    /// match manager.load() {
    ///     Ok(todos) => println!("加载了 {} 条数据", todos.len()),
    ///     Err(e) => eprintln!("加载失败: {}", e),
    /// }
    /// ```
    pub fn load(&self) -> Result<Vec<Todo>, String> {
        // 检查文件是否存在
        if !Path::new(&self.file_path).exists() {
            // 文件不存在是正常情况（首次启动）
            // 返回空数组，不是错误
            return Ok(vec![]);
        }

        // 读取文件内容为字符串
        let content = fs::read_to_string(&self.file_path)
            .map_err(|e| format!("读取文件失败 ({}): {}", self.file_path, e))?;

        // 解析 JSON 字符串为 Vec<Todo>
        // 需要显式指定类型 ::<Vec<Todo>>
        let todos = serde_json::from_str::<Vec<Todo>>(&content)
            .map_err(|e| format!("JSON 解析失败 ({}): {}", self.file_path, e))?;

        // 加载成功
        Ok(todos)
    }

    /// 获取文件路径
    ///
    /// 用于日志输出或调试
    pub fn file_path(&self) -> &str {
        &self.file_path
    }
}

// ============================================
// 单元测试
// ============================================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_save_and_load() {
        let test_file = "test_todos_save_load.json";
        let manager = PersistenceManager::new(test_file);

        // 准备测试数据
        let todos = vec![
            Todo {
                id: 1,
                title: "测试任务1".to_string(),
                completed: false,
            },
            Todo {
                id: 2,
                title: "测试任务2".to_string(),
                completed: true,
            },
        ];

        // 测试保存
        assert!(manager.save(&todos).is_ok());

        // 测试加载
        let loaded = manager.load().unwrap();
        assert_eq!(loaded.len(), 2);
        assert_eq!(loaded[0].id, 1);
        assert_eq!(loaded[0].title, "测试任务1");
        assert_eq!(loaded[1].id, 2);
        assert_eq!(loaded[1].completed, true);

        // 清理测试文件
        let _ = fs::remove_file(test_file);
    }

    #[test]
    fn test_load_nonexistent_file() {
        let manager = PersistenceManager::new("nonexistent_test_file.json");

        // 文件不存在应该返回空数组，不是错误
        let result = manager.load();
        assert!(result.is_ok());
        assert_eq!(result.unwrap().len(), 0);
    }

    #[test]
    fn test_atomic_write() {
        let test_file = "test_todos_atomic.json";
        let manager = PersistenceManager::new(test_file);

        let todos = vec![
            Todo {
                id: 1,
                title: "原子写入测试".to_string(),
                completed: false,
            },
        ];

        // 保存成功后，临时文件应该被删除
        assert!(manager.save(&todos).is_ok());
        assert!(!Path::new(&format!("{}.tmp", test_file)).exists());

        // 目标文件应该存在
        assert!(Path::new(test_file).exists());

        // 清理测试文件
        let _ = fs::remove_file(test_file);
    }
}
