# 📦 serde 学习 - 序列化和反序列化

学习 Rust 中最重要的序列化库 serde 和 serde_json。

---

## 🎯 学习目标

- ✅ 理解序列化和反序列化
- ✅ 使用 serde 处理 JSON 数据
- ✅ 掌握常用属性和选项
- ✅ 实际应用场景

---

## 📚 核心概念

### 序列化（Serialization）
将数据结构转换为字符串或字节流：
```
Rust 结构体 → JSON 字符串
```

### 反序列化（Deserialization）
将字符串或字节流转换回数据结构：
```
JSON 字符串 → Rust 结构体
```

---

## 🚀 快速开始

### 1. 添加依赖

```toml
[dependencies]
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
```

### 2. 基础使用

```rust
use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize, Debug)]
struct Person {
    name: String,
    age: u32,
    email: String,
}

fn main() {
    let person = Person {
        name: String::from("张三"),
        age: 30,
        email: String::from("zhangsan@example.com"),
    };

    // 序列化：结构体 → JSON
    let json = serde_json::to_string(&person).unwrap();
    println!("{}", json);
    // 输出：{"name":"张三","age":30,"email":"zhangsan@example.com"}

    // 反序列化：JSON → 结构体
    let person2: Person = serde_json::from_str(&json).unwrap();
    println!("{:?}", person2);
}
```

---

## 📖 示例说明

### 示例1：基础结构体
- 简单结构体的序列化和反序列化
- `to_string()` vs `to_string_pretty()`

### 示例2：复杂数据结构
- 嵌套结构体
- 数组（Vec）
- 布尔值

### 示例3：Option 和 Vec
- Option 对应 JSON 的 null
- 空数组的处理
- 可选字段

### 示例4：字段重命名
- `#[serde(rename = "...")]`
- API 字段名适配

### 示例5：配置文件
- 完整的应用配置示例
- 多层嵌套结构

---

## 🎨 常用属性

### 1. 字段重命名
```rust
#[derive(Serialize, Deserialize)]
struct User {
    #[serde(rename = "user_id")]
    id: u32,
}
```

### 2. 跳过字段
```rust
#[derive(Serialize, Deserialize)]
struct User {
    name: String,
    
    #[serde(skip)]
    password: String,  // 不会被序列化
}
```

### 3. 默认值
```rust
#[derive(Serialize, Deserialize)]
struct Config {
    #[serde(default)]
    debug: bool,  // 如果 JSON 中没有，使用 false
}
```

### 4. 可选字段
```rust
#[derive(Serialize, Deserialize)]
struct User {
    name: String,
    email: Option<String>,  // 可以是 null
}
```

---

## 💡 实际应用场景

### 1. 配置文件处理

```rust
use std::fs;

#[derive(Serialize, Deserialize)]
struct Config {
    host: String,
    port: u16,
    debug: bool,
}

// 读取配置
let config_str = fs::read_to_string("config.json")?;
let config: Config = serde_json::from_str(&config_str)?;

// 保存配置
let config_str = serde_json::to_string_pretty(&config)?;
fs::write("config.json", config_str)?;
```

### 2. HTTP API 数据

```rust
#[derive(Serialize, Deserialize)]
struct ApiResponse {
    code: u32,
    message: String,
    data: Vec<User>,
}

// 解析 API 响应
let response: ApiResponse = serde_json::from_str(&json_str)?;
```

### 3. 数据存储

```rust
// 保存到文件
let data = MyData { /* ... */ };
let json = serde_json::to_string_pretty(&data)?;
std::fs::write("data.json", json)?;

// 从文件加载
let json = std::fs::read_to_string("data.json")?;
let data: MyData = serde_json::from_str(&json)?;
```

---

## 🔥 高级特性

### 1. 自定义序列化

```rust
use serde::Serializer;

#[derive(Serialize)]
struct User {
    name: String,
    
    #[serde(serialize_with = "serialize_uppercase")]
    email: String,
}

fn serialize_uppercase<S>(s: &str, serializer: S) -> Result<S::Ok, S::Error>
where
    S: Serializer,
{
    serializer.serialize_str(&s.to_uppercase())
}
```

### 2. 枚举处理

```rust
#[derive(Serialize, Deserialize)]
#[serde(tag = "type")]
enum Message {
    Text { content: String },
    Image { url: String },
    Video { url: String, duration: u32 },
}
```

---

## 📊 性能提示

1. **预分配容量**：序列化大量数据时预分配 String 容量
2. **流式处理**：使用 `serde_json::Serializer` 处理大文件
3. **避免克隆**：使用引用而非克隆数据

---

## 🎓 学习要点

### 核心 API

| 函数 | 用途 |
|------|------|
| `to_string()` | 序列化为紧凑 JSON |
| `to_string_pretty()` | 序列化为美化 JSON |
| `from_str()` | 从字符串反序列化 |
| `to_writer()` | 序列化到 Writer |
| `from_reader()` | 从 Reader 反序列化 |

### 常用属性

| 属性 | 用途 |
|------|------|
| `#[serde(rename = "...")]` | 重命名字段 |
| `#[serde(skip)]` | 跳过字段 |
| `#[serde(default)]` | 使用默认值 |
| `#[serde(flatten)]` | 扁平化嵌套 |

---

## 🚀 下一步

学完 serde 后，你可以：

1. **实战应用**：
   - 改进 minigrep：添加 JSON 配置文件
   - HTTP 服务器：返回 JSON 响应

2. **学习更多格式**：
   - `serde_yaml`：YAML 格式
   - `toml`：TOML 格式（Cargo.toml）
   - `bincode`：二进制格式（高性能）

3. **学习 clap**：
   - 命令行参数解析
   - 与 serde 配合使用

---

## 🎯 实战练习

尝试这些练习巩固学习：

1. **配置管理器**：
   - 读取 JSON 配置文件
   - 修改配置
   - 保存回文件

2. **数据转换工具**：
   - JSON → 美化 JSON
   - JSON → Rust 结构体定义

3. **API 模拟器**：
   - 定义 API 数据结构
   - 生成示例 JSON

---

**运行示例**：
```bash
cargo run
```

查看完整的 5 个示例输出！
