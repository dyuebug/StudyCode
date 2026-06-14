# 🔍 minigrep - 文本搜索工具

一个用 Rust 实现的命令行文本搜索工具，类似于 Unix 的 `grep` 命令。

---

## ✨ 功能特性

- ✅ 在文件中搜索指定文本
- ✅ 大小写敏感/不敏感搜索
- ✅ 显示匹配的行号
- ✅ 清晰的错误提示
- ✅ 环境变量配置
- ✅ 完整的单元测试
- ✅ **JSON 配置文件** 🆕
- ✅ **搜索历史记录** 🆕

---

## 🆕 新功能：配置文件和搜索历史

### 查看配置和历史

```bash
cargo run -- --config
```

输出：
```
📋 当前配置:
  默认忽略大小写: false
  最大历史记录: 10

  最近搜索:
    1. to
    2. nobody

  最近文件:
    1. poem.txt
```

### 配置文件格式

配置自动保存在 `minigrep_config.json`：

```json
{
  "default_ignore_case": false,
  "recent_searches": [
    "to",
    "nobody"
  ],
  "recent_files": [
    "poem.txt"
  ],
  "max_history": 10
}
```

### 手动编辑配置

你可以直接编辑 `minigrep_config.json` 来修改配置：

```json
{
  "default_ignore_case": true,
  "recent_searches": [],
  "recent_files": [],
  "max_history": 20
}
```

---

## 🚀 使用方法

### 基本搜索（大小写敏感）

```bash
cargo run -- <搜索词> <文件名>

# 示例：搜索 "nobody"
cargo run -- nobody poem.txt
```

输出：
```
🔍 搜索词: "nobody"
📄 文件名: poem.txt
🔤 大小写: 敏感

✅ 找到 2 个匹配:

  行 1: I'm nobody! Who are you?
  行 2: Are you nobody, too?
```

### 大小写不敏感搜索

设置环境变量 `IGNORE_CASE` 即可忽略大小写：

**Linux/Mac:**
```bash
IGNORE_CASE=1 cargo run -- to poem.txt
```

**Windows (PowerShell):**
```powershell
$env:IGNORE_CASE=1; cargo run -- to poem.txt
```

**Windows (CMD):**
```cmd
set IGNORE_CASE=1 && cargo run -- to poem.txt
```

输出：
```
🔍 搜索词: "to"
📄 文件名: poem.txt
🔤 大小写: 忽略

✅ 找到 4 个匹配:

  行 2: Are you nobody, too?
  行 6: How dreary to be somebody!
  行 8: To tell your name the livelong day
  行 9: To an admiring bog!
```

---

## 📦 编译和安装

### 编译

```bash
cargo build --release
```

编译后的可执行文件在 `target/release/minigrep`

### 运行

```bash
# 直接运行
./target/release/minigrep nobody poem.txt

# 或安装到系统
cargo install --path .
minigrep nobody poem.txt
```

---

## 🧪 运行测试

```bash
cargo test
```

测试包括：
- 大小写敏感搜索测试
- 大小写不敏感搜索测试

---

## 📚 代码结构

```
minigrep/
├── Cargo.toml          # 项目配置（新增 serde 依赖）
├── README.md           # 本文件
├── poem.txt            # 测试文件
├── minigrep_config.json # 配置文件（自动生成）🆕
└── src/
    ├── main.rs         # 主程序：命令行参数处理 + 配置管理 🆕
    ├── lib.rs          # 核心逻辑：搜索功能和测试
    └── config.rs       # 配置模块：JSON 配置处理 🆕
```

### 核心模块

**`src/lib.rs`**：
- `Config` 结构体：存储配置（搜索词、文件名、大小写设置）
- `search()` 函数：大小写敏感搜索
- `search_case_insensitive()` 函数：大小写不敏感搜索
- `run()` 函数：主逻辑
- 单元测试

**`src/main.rs`**：
- 解析命令行参数
- 调用库函数
- 错误处理

---

## 🎯 技术亮点

### 1. 迭代器和闭包
```rust
contents
    .lines()
    .enumerate()
    .filter(|(_, line)| line.contains(query))
    .map(|(index, line)| (index + 1, line))
    .collect()
```

### 2. 生命周期注解
```rust
pub fn search<'a>(query: &str, contents: &'a str) -> Vec<(usize, &'a str)>
```

### 3. 错误处理
```rust
pub fn run(config: Config) -> Result<(), Box<dyn Error>>
```

### 4. 环境变量
```rust
let ignore_case = env::var("IGNORE_CASE").is_ok();
```

### 5. 测试驱动开发
```rust
#[cfg(test)]
mod tests {
    #[test]
    fn case_sensitive() { ... }
}
```

---

## 📖 学习要点

通过这个项目，你将掌握：
- ✅ 命令行参数处理（`std::env::args`）
- ✅ 文件 I/O（`std::fs::read_to_string`）
- ✅ 迭代器方法（`lines()`, `filter()`, `map()`, `collect()`）
- ✅ 闭包使用
- ✅ 生命周期注解
- ✅ 错误处理（`Result`, `?`, `Box<dyn Error>`）
- ✅ 环境变量（`std::env::var`）
- ✅ 模块化设计（lib.rs 和 main.rs 分离）
- ✅ 单元测试（`#[cfg(test)]`, `#[test]`）

---

## 🎨 扩展建议

如果想继续改进这个项目，可以添加：

1. **正则表达式支持**
   ```bash
   minigrep --regex "no.*y" poem.txt
   ```

2. **递归搜索目录**
   ```bash
   minigrep nobody ./docs/
   ```

3. **多文件搜索**
   ```bash
   minigrep nobody file1.txt file2.txt file3.txt
   ```

4. **彩色输出**
   - 高亮匹配的文本
   - 使用 `colored` crate

5. **性能优化**
   - 使用 `mmap` 读取大文件
   - 并行搜索多个文件

6. **更多选项**
   - `-n` 只显示行号
   - `-c` 只显示匹配数量
   - `-v` 反向匹配（显示不匹配的行）

---

## 🏆 项目成果

- ✅ 完整的 CLI 工具
- ✅ 零外部依赖（只用标准库）
- ✅ 良好的代码结构
- ✅ 完整的测试覆盖
- ✅ 清晰的错误处理
- ✅ 实用的功能

---

## 📝 许可

本项目为学习项目，欢迎自由使用和修改。
