// ============================================
// 文件持久化与 JSON 存储 - 超详细注释版
// 日期：2026-06-22（第27天）
// 难度：⭐⭐⭐ 中级
// ============================================

/*
【重要说明】
本文件是纯教程文件，需要 serde 和 serde_json 依赖才能编译运行。

学习方式：
1. 仔细阅读所有注释和说明
2. 理解核心概念和函数用法
3. 在 todo-api 项目中实际实践

运行方式：
- 本文件不需要编译运行
- 直接阅读学习即可
- 实战代码在 todo-api 项目中实现
*/

/*
【核心概念】数据持久化

是什么？
- 将内存中的数据保存到磁盘，使数据在程序重启后仍然存在

为什么需要？
- 内存数据是易失的（程序关闭、崩溃、重启都会丢失）
- 持久化数据可以长期保存
- Web 应用必须具备数据持久化能力

核心特点：
- 数据保存到文件或数据库
- 程序重启后可以恢复数据
- 需要处理读写错误
- 需要保证数据一致性

Rust 特色：
- 所有权系统保证内存安全
- Result 类型强制错误处理
- serde 提供强大的序列化能力
*/

use serde::{Deserialize, Serialize};
use std::fs;
use std::io;
use std::path::Path;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

fn main() {
    println!("============================================================");
    println!("第一部分：核心概念详解");
    println!("============================================================");

    // --------------------------------------------
    // 1.1 数据持久化的必要性
    // --------------------------------------------

    println!("\n1.1 为什么需要数据持久化？");
    println!("------------------------------------------------------------");

    /*
    【内存数据 vs 持久化数据】

    内存数据（Vec<Todo>）：
    - 存储在 RAM 中
    - 程序关闭后消失
    - 访问速度极快（纳秒级）
    - 容量有限（几GB）

    持久化数据（文件/数据库）：
    - 存储在磁盘中
    - 永久保存（除非删除）
    - 访问速度较慢（毫秒级）
    - 容量很大（几TB）

    Web 应用需求：
    - 用户数据必须持久化
    - 服务器重启不能丢数据
    - 多个服务器实例共享数据
    */

    // --------------------------------------------
    // 1.2 JSON 作为存储格式
    // --------------------------------------------

    println!("\n1.2 为什么选择 JSON？");
    println!("------------------------------------------------------------");

    /*
    【JSON 的优势】

    人类可读：
    - 文本格式，易于查看和调试
    - 可以用任何文本编辑器打开

    跨语言：
    - 几乎所有语言都支持 JSON
    - 易于与其他系统集成

    结构化：
    - 支持对象、数组、嵌套
    - 类型安全（字符串、数字、布尔）

    生态成熟：
    - Rust 有 serde_json 库
    - 序列化和反序列化自动化
    */

    // --------------------------------------------
    // 1.3 文件操作的核心函数
    // --------------------------------------------

    println!("\n1.3 Rust 文件操作核心函数");
    println!("------------------------------------------------------------");

    /*
    【std::fs 模块】

    读取文件：
    - fs::read_to_string(path) -> Result<String, Error>
      读取整个文件为字符串（适合文本文件）

    - fs::read(path) -> Result<Vec<u8>, Error>
      读取整个文件为字节数组（适合二进制）

    写入文件：
    - fs::write(path, contents) -> Result<(), Error>
      写入内容到文件（覆盖现有内容）

    - fs::OpenOptions::new() -> OpenOptions
      更灵活的文件打开方式（追加、读写等）

    文件操作：
    - fs::rename(from, to) -> Result<(), Error>
      重命名或移动文件（原子操作）

    - fs::remove_file(path) -> Result<(), Error>
      删除文件

    检查文件：
    - Path::new(path).exists() -> bool
      检查文件或目录是否存在

    - fs::metadata(path) -> Result<Metadata, Error>
      获取文件元数据（大小、修改时间等）
    */

    /*
    【serde_json 序列化函数】

    序列化（Rust -> JSON）：
    - serde_json::to_string(&value) -> Result<String, Error>
      紧凑格式（无空格换行）

    - serde_json::to_string_pretty(&value) -> Result<String, Error>
      美化格式（有缩进和换行，适合人类阅读）

    - serde_json::to_writer(writer, &value) -> Result<(), Error>
      直接写入到 Writer（如文件）

    反序列化（JSON -> Rust）：
    - serde_json::from_str::<T>(s) -> Result<T, Error>
      从字符串解析

    - serde_json::from_reader::<R, T>(reader) -> Result<T, Error>
      从 Reader 读取并解析
    */

    // --------------------------------------------
    // 1.4 原子写入模式
    // --------------------------------------------

    println!("\n1.4 原子写入 - 保证数据安全");
    println!("------------------------------------------------------------");

    /*
    【为什么需要原子写入？】

    普通写入的问题：
    ```rust
    fs::write("data.json", json_string)?;
    ```

    风险场景：
    1. 写入到一半，程序崩溃 → 文件损坏
    2. 写入到一半，系统断电 → 文件损坏
    3. 写入到一半，磁盘满了 → 文件损坏

    后果：
    - 原有数据丢失
    - 新数据也没保存
    - 文件变成无效的 JSON
    - 下次启动程序会失败

    【原子写入解决方案】

    步骤：
    1. 写入到临时文件（如 data.json.tmp）
    2. 写入成功后，重命名临时文件为目标文件
    3. 重命名操作是原子的（要么成功，要么失败）

    为什么安全？
    - 如果写入临时文件失败，原文件不受影响
    - 如果重命名失败，原文件不受影响
    - 重命名是原子操作（操作系统保证）

    代码模式：
    ```rust
    // 1. 写入临时文件
    fs::write("data.json.tmp", json_string)?;

    // 2. 原子重命名（覆盖原文件）
    fs::rename("data.json.tmp", "data.json")?;
    ```

    结果：
    - 要么完全成功（新数据保存）
    - 要么完全失败（保留旧数据）
    - 永远不会出现半成品
    */
}

// ============================================
// 第二部分：基础操作实践
// ============================================

#[derive(Debug, Serialize, Deserialize, Clone)]
struct Todo {
    id: u32,
    title: String,
    completed: bool,
}

// 示例1：基本的文件读写
fn example_1_basic_file_operations() {
    println!("\n============================================================");
    println!("示例1：基本的文件读写");
    println!("============================================================");

    // 准备测试数据
    let todos = vec![
        Todo { id: 1, title: "学习 Rust".to_string(), completed: true },
        Todo { id: 2, title: "学习 Actix-web".to_string(), completed: false },
    ];

    let file_path = "test_todos_basic.json";

    // --------------------------------------------
    // 步骤1：序列化为 JSON 字符串
    // --------------------------------------------

    // to_string_pretty() 生成美化的 JSON（有缩进和换行）
    // 参数：&todos - 引用，不获取所有权
    // 返回：Result<String, Error> - 成功返回 JSON 字符串
    match serde_json::to_string_pretty(&todos) {
        Ok(json_string) => {
            println!("✅ 序列化成功：");
            println!("{}", json_string);

            // --------------------------------------------
            // 步骤2：写入文件
            // --------------------------------------------

            // fs::write() 将字符串写入文件（覆盖模式）
            // 参数1：文件路径
            // 参数2：内容（字符串或字节数组）
            // 返回：Result<(), Error>
            match fs::write(file_path, &json_string) {
                Ok(_) => println!("✅ 文件写入成功：{}", file_path),
                Err(e) => println!("❌ 文件写入失败：{}", e),
            }
        }
        Err(e) => println!("❌ 序列化失败：{}", e),
    }

    // --------------------------------------------
    // 步骤3：从文件读取
    // --------------------------------------------

    // fs::read_to_string() 读取整个文件为字符串
    // 参数：文件路径
    // 返回：Result<String, Error>
    match fs::read_to_string(file_path) {
        Ok(content) => {
            println!("✅ 文件读取成功：");
            println!("{}", content);

            // --------------------------------------------
            // 步骤4：反序列化为 Rust 对象
            // --------------------------------------------

            // from_str() 从 JSON 字符串解析为 Rust 对象
            // 需要显式指定类型 ::<Vec<Todo>>
            // 返回：Result<Vec<Todo>, Error>
            match serde_json::from_str::<Vec<Todo>>(&content) {
                Ok(loaded_todos) => {
                    println!("✅ 反序列化成功，加载了 {} 条数据：", loaded_todos.len());
                    for todo in loaded_todos {
                        println!("  - [{}] {} (完成: {})", todo.id, todo.title, todo.completed);
                    }
                }
                Err(e) => println!("❌ 反序列化失败：{}", e),
            }
        }
        Err(e) => println!("❌ 文件读取失败：{}", e),
    }

    // 清理测试文件
    let _ = fs::remove_file(file_path);
}

// 示例2：原子写入实现
fn example_2_atomic_write() {
    println!("\n============================================================");
    println!("示例2：原子写入实现");
    println!("============================================================");

    let todos = vec![
        Todo { id: 1, title: "重要数据".to_string(), completed: false },
    ];

    let file_path = "test_todos_atomic.json";
    let temp_path = "test_todos_atomic.json.tmp";

    // --------------------------------------------
    // 原子写入步骤
    // --------------------------------------------

    println!("开始原子写入...");

    // 步骤1：序列化
    let json_string = match serde_json::to_string_pretty(&todos) {
        Ok(s) => s,
        Err(e) => {
            println!("❌ 序列化失败：{}", e);
            return;
        }
    };

    // 步骤2：写入临时文件
    if let Err(e) = fs::write(temp_path, &json_string) {
        println!("❌ 写入临时文件失败：{}", e);
        return;
    }
    println!("✅ 临时文件写入成功：{}", temp_path);

    // 步骤3：原子重命名（这一步是关键！）
    // rename() 是原子操作，要么成功要么失败
    // 如果目标文件存在，会被覆盖
    if let Err(e) = fs::rename(temp_path, file_path) {
        println!("❌ 重命名失败：{}", e);
        // 清理临时文件
        let _ = fs::remove_file(temp_path);
        return;
    }
    println!("✅ 原子重命名成功：{} -> {}", temp_path, file_path);

    println!("✅ 原子写入完成！数据安全保存！");

    // 清理测试文件
    let _ = fs::remove_file(file_path);
}

// 示例3：检查文件是否存在
fn example_3_check_file_exists() {
    println!("\n============================================================");
    println!("示例3：检查文件是否存在");
    println!("============================================================");

    let file_path = "test_todos_check.json";

    // --------------------------------------------
    // 方法1：使用 Path::exists()
    // --------------------------------------------

    // Path::new() 创建路径对象
    // exists() 检查路径是否存在（文件或目录）
    if Path::new(file_path).exists() {
        println!("✅ 文件存在：{}", file_path);
    } else {
        println!("❌ 文件不存在：{}", file_path);
    }

    // --------------------------------------------
    // 创建文件
    // --------------------------------------------

    println!("\n创建文件...");
    let todos = vec![Todo { id: 1, title: "测试".to_string(), completed: false }];
    let json = serde_json::to_string_pretty(&todos).unwrap();
    fs::write(file_path, json).unwrap();

    // --------------------------------------------
    // 再次检查
    // --------------------------------------------

    if Path::new(file_path).exists() {
        println!("✅ 文件存在：{}", file_path);

        // 获取文件元数据
        match fs::metadata(file_path) {
            Ok(metadata) => {
                println!("  - 文件大小：{} 字节", metadata.len());
                println!("  - 是否为文件：{}", metadata.is_file());
                println!("  - 是否为目录：{}", metadata.is_dir());
            }
            Err(e) => println!("❌ 获取元数据失败：{}", e),
        }
    }

    // 清理测试文件
    let _ = fs::remove_file(file_path);
}

// 示例4：错误处理 - 文件不存在
fn example_4_handle_missing_file() {
    println!("\n============================================================");
    println!("示例4：错误处理 - 文件不存在");
    println!("============================================================");

    let file_path = "non_existent.json";

    // --------------------------------------------
    // 尝试读取不存在的文件
    // --------------------------------------------

    match fs::read_to_string(file_path) {
        Ok(content) => {
            println!("✅ 文件读取成功：{}", content);
        }
        Err(e) => {
            // 打印错误信息
            println!("❌ 文件读取失败：{}", e);

            // 检查错误类型
            // io::ErrorKind 包含各种错误类型
            match e.kind() {
                io::ErrorKind::NotFound => {
                    println!("  原因：文件不存在");
                    println!("  处理方式：返回空数组或创建默认文件");
                }
                io::ErrorKind::PermissionDenied => {
                    println!("  原因：没有读取权限");
                }
                _ => {
                    println!("  原因：其他错误 {:?}", e.kind());
                }
            }
        }
    }
}

// 示例5：错误处理 - JSON 格式错误
fn example_5_handle_invalid_json() {
    println!("\n============================================================");
    println!("示例5：错误处理 - JSON 格式错误");
    println!("============================================================");

    let file_path = "test_invalid.json";

    // --------------------------------------------
    // 创建格式错误的 JSON 文件
    // --------------------------------------------

    let invalid_json = r#"
    {
        "id": 1,
        "title": "测试",
        // 这是注释（JSON 不支持注释）
        "completed": true,
    }
    "#;

    fs::write(file_path, invalid_json).unwrap();
    println!("已创建格式错误的 JSON 文件");

    // --------------------------------------------
    // 尝试解析
    // --------------------------------------------

    match fs::read_to_string(file_path) {
        Ok(content) => {
            println!("文件读取成功，尝试解析...");

            match serde_json::from_str::<Vec<Todo>>(&content) {
                Ok(todos) => {
                    println!("✅ 解析成功：{:?}", todos);
                }
                Err(e) => {
                    println!("❌ JSON 解析失败：{}", e);
                    println!("  处理方式：");
                    println!("  1. 返回空数组");
                    println!("  2. 备份损坏的文件");
                    println!("  3. 创建新的空文件");
                }
            }
        }
        Err(e) => {
            println!("❌ 文件读取失败：{}", e);
        }
    }

    // 清理测试文件
    let _ = fs::remove_file(file_path);
}

// 示例6：完整的持久化管理器
struct PersistenceManager {
    file_path: String,
}

impl PersistenceManager {
    // 创建新的管理器实例
    fn new(file_path: &str) -> Self {
        Self {
            file_path: file_path.to_string(),
        }
    }

    // 保存数据到文件（使用原子写入）
    fn save(&self, todos: &[Todo]) -> Result<(), String> {
        // 步骤1：序列化
        let json_string = serde_json::to_string_pretty(todos)
            .map_err(|e| format!("序列化失败: {}", e))?;

        // 步骤2：写入临时文件
        let temp_path = format!("{}.tmp", self.file_path);
        fs::write(&temp_path, &json_string)
            .map_err(|e| format!("写入临时文件失败: {}", e))?;

        // 步骤3：原子重命名
        fs::rename(&temp_path, &self.file_path)
            .map_err(|e| format!("重命名失败: {}", e))?;

        Ok(())
    }

    // 从文件加载数据
    fn load(&self) -> Result<Vec<Todo>, String> {
        // 检查文件是否存在
        if !Path::new(&self.file_path).exists() {
            // 文件不存在，返回空数组
            return Ok(vec![]);
        }

        // 读取文件
        let content = fs::read_to_string(&self.file_path)
            .map_err(|e| format!("读取文件失败: {}", e))?;

        // 解析 JSON
        let todos = serde_json::from_str::<Vec<Todo>>(&content)
            .map_err(|e| format!("JSON 解析失败: {}", e))?;

        Ok(todos)
    }
}

fn example_6_persistence_manager() {
    println!("\n============================================================");
    println!("示例6：完整的持久化管理器");
    println!("============================================================");

    let manager = PersistenceManager::new("test_manager.json");

    // --------------------------------------------
    // 场景1：首次启动（文件不存在）
    // --------------------------------------------

    println!("\n场景1：首次启动");
    match manager.load() {
        Ok(todos) => {
            println!("✅ 加载成功，数据量：{}", todos.len());
            if todos.is_empty() {
                println!("  → 文件不存在或为空，返回空数组");
            }
        }
        Err(e) => println!("❌ 加载失败：{}", e),
    }

    // --------------------------------------------
    // 场景2：保存数据
    // --------------------------------------------

    println!("\n场景2：保存数据");
    let todos = vec![
        Todo { id: 1, title: "学习 Rust".to_string(), completed: true },
        Todo { id: 2, title: "开发 Web API".to_string(), completed: false },
    ];

    match manager.save(&todos) {
        Ok(_) => println!("✅ 保存成功"),
        Err(e) => println!("❌ 保存失败：{}", e),
    }

    // --------------------------------------------
    // 场景3：重新加载（模拟程序重启）
    // --------------------------------------------

    println!("\n场景3：程序重启，重新加载");
    match manager.load() {
        Ok(loaded_todos) => {
            println!("✅ 加载成功，数据量：{}", loaded_todos.len());
            for todo in loaded_todos {
                println!("  - [{}] {}", todo.id, todo.title);
            }
        }
        Err(e) => println!("❌ 加载失败：{}", e),
    }

    // 清理测试文件
    let _ = fs::remove_file("test_manager.json");
}

// ============================================
// 主函数：运行所有示例
// ============================================

#[allow(dead_code)]
fn run_all_examples() {
    example_1_basic_file_operations();
    example_2_atomic_write();
    example_3_check_file_exists();
    example_4_handle_missing_file();
    example_5_handle_invalid_json();
    example_6_persistence_manager();
}

// ============================================
// 第三部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

#[allow(dead_code)]
fn common_mistakes() {
    println!("\n============================================================");
    println!("常见错误和陷阱 ⭐⭐⭐⭐⭐");
    println!("============================================================");

    /*
    ❌ 错误1：直接写入，不使用原子写入

    错误代码：
    ```rust
    let json = serde_json::to_string(&todos)?;
    fs::write("data.json", json)?;
    // 如果写入到一半程序崩溃，文件损坏
    ```

    后果：
    - 写入过程中程序崩溃 → 文件损坏
    - 原有数据丢失，新数据也没保存完整

    ✅ 正确代码：
    ```rust
    let json = serde_json::to_string(&todos)?;
    fs::write("data.json.tmp", json)?;
    fs::rename("data.json.tmp", "data.json")?;
    // 原子操作，要么成功要么失败
    ```

    记忆技巧：
    - 重要数据必须使用原子写入
    - 临时文件 + 重命名 = 安全

    ════════════════════════════════════════════════════════════

    ❌ 错误2：不检查文件是否存在

    错误代码：
    ```rust
    let content = fs::read_to_string("data.json").unwrap();
    // 如果文件不存在，程序直接 panic
    ```

    后果：
    - 首次启动时文件不存在
    - 程序 panic，无法启动

    ✅ 正确代码：
    ```rust
    let todos = if Path::new("data.json").exists() {
        let content = fs::read_to_string("data.json")?;
        serde_json::from_str(&content)?
    } else {
        vec![]  // 文件不存在，返回空数组
    };
    ```

    记忆技巧：
    - 首次启动时文件不存在是正常的
    - 总是检查文件是否存在
    - 不存在时返回默认值

    ════════════════════════════════════════════════════════════

    ❌ 错误3：不处理 JSON 格式错误

    错误代码：
    ```rust
    let content = fs::read_to_string("data.json")?;
    let todos: Vec<Todo> = serde_json::from_str(&content).unwrap();
    // 如果 JSON 格式错误，程序 panic
    ```

    后果：
    - 文件被意外修改或损坏
    - 程序无法启动

    ✅ 正确代码：
    ```rust
    let content = fs::read_to_string("data.json")?;
    let todos = match serde_json::from_str::<Vec<Todo>>(&content) {
        Ok(t) => t,
        Err(e) => {
            eprintln!("JSON 解析失败: {}", e);
            // 备份损坏的文件
            let _ = fs::rename("data.json", "data.json.backup");
            vec![]  // 返回空数组
        }
    };
    ```

    记忆技巧：
    - JSON 格式错误是可能发生的
    - 备份损坏的文件
    - 返回空数组，不要 panic

    ════════════════════════════════════════════════════════════

    ❌ 错误4：忘记清理临时文件

    错误代码：
    ```rust
    fs::write("data.json.tmp", json)?;
    if some_condition {
        return Err("操作失败".into());
        // 临时文件没有被清理
    }
    fs::rename("data.json.tmp", "data.json")?;
    ```

    后果：
    - 临时文件积累，占用磁盘空间
    - 可能影响后续操作

    ✅ 正确代码：
    ```rust
    let temp_path = "data.json.tmp";
    fs::write(temp_path, json)?;

    if let Err(e) = fs::rename(temp_path, "data.json") {
        // 清理临时文件
        let _ = fs::remove_file(temp_path);
        return Err(e.into());
    }
    ```

    记忆技巧：
    - 临时文件用完必须清理
    - 使用 defer 模式或 RAII

    ════════════════════════════════════════════════════════════

    ❌ 错误5：使用 unwrap() 而不是 ? 运算符

    错误代码：
    ```rust
    let json = serde_json::to_string(&todos).unwrap();
    fs::write("data.json", json).unwrap();
    // 任何错误都会导致 panic
    ```

    后果：
    - 错误无法优雅处理
    - 用户体验差
    - 程序直接崩溃

    ✅ 正确代码：
    ```rust
    fn save(todos: &[Todo]) -> Result<(), String> {
        let json = serde_json::to_string(todos)
            .map_err(|e| format!("序列化失败: {}", e))?;
        fs::write("data.json", json)
            .map_err(|e| format!("写入失败: {}", e))?;
        Ok(())
    }
    ```

    记忆技巧：
    - 库代码永远不要使用 unwrap()
    - 使用 ? 运算符传播错误
    - 返回 Result 类型

    ════════════════════════════════════════════════════════════

    ❌ 错误6：在异步上下文中使用同步 I/O

    错误代码：
    ```rust
    pub async fn save(&self) -> Result<(), String> {
        fs::write("data.json", json)?;  // 阻塞异步运行时
    }
    ```

    后果：
    - 阻塞异步运行时
    - 影响其他并发任务
    - 性能下降

    ✅ 正确代码：
    ```rust
    use tokio::fs;

    pub async fn save(&self) -> Result<(), String> {
        fs::write("data.json", json).await?;  // 异步 I/O
    }
    ```

    记忆技巧：
    - 异步函数使用 tokio::fs
    - 记得 .await
    - 不要阻塞运行时

    ════════════════════════════════════════════════════════════

    ❌ 错误7：并发写入没有加锁

    错误代码：
    ```rust
    // 多个线程同时写入
    thread::spawn(|| save_data(&todos1));
    thread::spawn(|| save_data(&todos2));
    // 数据可能损坏
    ```

    后果：
    - 两个写入操作交织
    - 文件内容损坏
    - 数据丢失

    ✅ 正确代码：
    ```rust
    let manager = Arc::new(Mutex::new(PersistenceManager::new("data.json")));

    let m1 = manager.clone();
    thread::spawn(move || {
        let mgr = m1.lock().unwrap();
        mgr.save(&todos1).unwrap();
    });
    ```

    记忆技巧：
    - 文件写入需要互斥
    - 使用 Arc<Mutex<PersistenceManager>>
    - 确保一次只有一个写入者
    */
}

/*
==============================================
🔍 深入理解：文件系统和原子操作（选学）
==============================================

如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【为什么 fs::rename() 是原子的？】

操作系统层面：
- rename() 是一个系统调用
- 操作系统保证原子性
- 只修改文件系统的元数据（inode）
- 不涉及数据复制

具体过程：
1. 文件系统维护一个目录表
2. 每个文件名对应一个 inode
3. rename() 只是修改目录表中的映射
4. 这个修改是原子的（单个操作）

为什么安全？
- 如果修改成功，新文件名指向 inode
- 如果修改失败，保持原状
- 不存在"修改了一半"的状态

【为什么不能保证跨文件系统的原子性？】

跨文件系统 rename：
- 如果源文件和目标文件在不同文件系统
- rename() 会变成 copy + delete
- 不再是原子操作

解决方案：
- 确保临时文件和目标文件在同一文件系统
- 通常在同一目录下即可

【性能考虑】

写入性能：
- 小文件（< 1MB）：可以每次修改都保存
- 中文件（1-10MB）：考虑定时保存
- 大文件（> 10MB）：考虑增量保存或数据库

原子写入的开销：
- 多一次文件写入（临时文件）
- 多一次系统调用（rename）
- 但保证了数据安全
- 对于小文件，开销可以忽略

【备份策略】

简单备份：
- 保存前，将当前文件重命名为 .backup
- 新数据写入成功后，删除 .backup
- 失败时可以恢复

版本控制：
- 每次保存生成带时间戳的文件
- data.2024-03-06-10-30-00.json
- 保留最近 N 个版本

==============================================
*/

// ============================================
// 第四部分：函数/宏卡片速查
// ============================================

/*
【函数1：fs::read_to_string()】

语法：fn read_to_string<P: AsRef<Path>>(path: P) -> Result<String, Error>
参数：
  - path: 文件路径（&str 或 Path）
返回：Result<String, Error> - 文件内容或错误

所有权：
  - path 使用引用，不获取所有权
  - 返回新分配的 String（获取所有权）

示例：
```rust
let content = fs::read_to_string("data.json")?;
```

────────────────────────────────────────────

【函数2：fs::write()】

语法：fn write<P: AsRef<Path>, C: AsRef<[u8]>>(path: P, contents: C) -> Result<(), Error>
参数：
  - path: 文件路径（&str 或 Path）
  - contents: 要写入的内容（&str 或 &[u8]）
返回：Result<(), Error> - 成功或错误

所有权：
  - 参数都使用引用，不获取所有权
  - 会覆盖现有文件内容

示例：
```rust
fs::write("data.json", json_string)?;
```

────────────────────────────────────────────

【函数3：fs::rename()】

语法：fn rename<P: AsRef<Path>, Q: AsRef<Path>>(from: P, to: Q) -> Result<(), Error>
参数：
  - from: 源文件路径
  - to: 目标文件路径
返回：Result<(), Error> - 成功或错误

所有权：
  - 参数使用引用，不获取所有权
  - 如果目标文件存在，会被覆盖

特性：
  - 原子操作（同一文件系统内）
  - 用于实现安全的文件更新

示例：
```rust
fs::rename("data.json.tmp", "data.json")?;
```

────────────────────────────────────────────

【函数4：serde_json::to_string_pretty()】

语法：fn to_string_pretty<T: Serialize>(value: &T) -> Result<String, Error>
参数：
  - value: 要序列化的值（必须实现 Serialize trait）
返回：Result<String, Error> - JSON 字符串或错误

所有权：
  - 使用引用，不获取所有权
  - 返回新分配的 String

特性：
  - 美化格式（带缩进和换行）
  - 人类可读

示例：
```rust
let json = serde_json::to_string_pretty(&todos)?;
```

────────────────────────────────────────────

【函数5：serde_json::from_str()】

语法：fn from_str<'a, T: Deserialize<'a>>(s: &'a str) -> Result<T, Error>
参数：
  - s: JSON 字符串
返回：Result<T, Error> - 反序列化的对象或错误

所有权：
  - 参数使用引用
  - 返回新分配的对象（获取所有权）

特性：
  - 需要显式指定类型 ::<Vec<Todo>>
  - 自动验证 JSON 格式

示例：
```rust
let todos = serde_json::from_str::<Vec<Todo>>(&content)?;
```

────────────────────────────────────────────

【函数6：Path::new()】

语法：fn new<S: AsRef<OsStr> + ?Sized>(s: &S) -> &Path
参数：
  - s: 路径字符串
返回：&Path - 路径引用

所有权：
  - 零成本抽象，不分配内存
  - 返回引用

示例：
```rust
if Path::new("data.json").exists() { ... }
```

────────────────────────────────────────────

【方法7：Path::exists()】

语法：fn exists(&self) -> bool
参数：无
返回：bool - 文件或目录是否存在

示例：
```rust
if Path::new("data.json").exists() {
    // 文件存在
}
```
*/

// ============================================
// 第五部分：练习题
// ============================================

/*
练习1：实现一个带备份的保存函数
要求：
- 保存前，将当前文件重命名为 .backup
- 使用原子写入保存新数据
- 如果保存失败，恢复 .backup
提示：先检查文件是否存在，再进行备份

练习2：实现一个带版本控制的保存函数
要求：
- 每次保存生成带时间戳的文件名
- 例如：data.2024-03-06-10-30-00.json
- 保留最近 5 个版本，删除旧版本
提示：使用 chrono 库获取时间戳

练习3：实现一个增量保存策略
要求：
- 只有数据真正变化时才保存
- 使用哈希值检测变化
- 避免不必要的磁盘写入
提示：计算数据的哈希值并缓存

练习4：实现自动恢复功能
要求：
- 如果主文件损坏，自动尝试从备份恢复
- 如果备份也损坏，返回空数组
- 记录恢复日志
提示：依次尝试主文件、备份文件

练习5：实现定时保存
要求：
- 不是每次修改都保存，而是定时保存（如每 10 秒）
- 使用标记记录是否有修改
- 只有有修改时才真正保存
提示：使用 AtomicBool 作为修改标记

练习6：处理多种错误场景
要求：
- 文件不存在
- 文件损坏（JSON 格式错误）
- 权限不足
- 磁盘空间不足
- 为每种错误提供合适的处理方式
提示：使用 io::ErrorKind 区分错误类型

练习7：实现一个完整的持久化管理器
要求：
- 支持保存和加载
- 自动备份
- 错误恢复
- 线程安全（Arc<Mutex<>>）
- 详细的错误信息
提示：结合前面所有知识点
*/

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_persistence_manager() {
        let manager = PersistenceManager::new("test_todos.json");

        // 测试保存
        let todos = vec![
            Todo { id: 1, title: "测试".to_string(), completed: false },
        ];
        assert!(manager.save(&todos).is_ok());

        // 测试加载
        let loaded = manager.load().unwrap();
        assert_eq!(loaded.len(), 1);
        assert_eq!(loaded[0].id, 1);

        // 清理
        let _ = fs::remove_file("test_todos.json");
    }

    #[test]
    fn test_load_nonexistent_file() {
        let manager = PersistenceManager::new("nonexistent.json");
        let result = manager.load().unwrap();
        assert_eq!(result.len(), 0);
    }
}





