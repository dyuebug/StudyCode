// ============================================
// serde 学习 - 序列化和反序列化
// ============================================

use serde::{Deserialize, Serialize};

// ============================================
// 第一部分：基础概念
// ============================================

/*
【序列化 Serialization】
- 将数据结构转换为字符串或字节流
- Rust 结构体 → JSON 字符串

【反序列化 Deserialization】
- 将字符串或字节流转换回数据结构
- JSON 字符串 → Rust 结构体

【serde】
- Rust 序列化框架
- 支持 JSON、YAML、TOML、MessagePack 等格式
- 使用 derive 宏自动实现序列化

【serde_json】
- serde 的 JSON 实现
- 最常用的格式
*/

// ============================================
// 示例1：基础结构体序列化
// ============================================

#[derive(Serialize, Deserialize, Debug)]
struct Person {
    name: String,
    age: u32,
    email: String,
}

fn example1_basic() {
    println!("============================================================");
    println!("示例1：基础结构体序列化");
    println!("============================================================\n");

    // 创建结构体实例
    let person = Person {
        name: String::from("张三"),
        age: 30,
        email: String::from("zhangsan@example.com"),
    };

    // 序列化：结构体 → JSON 字符串
    let json = serde_json::to_string(&person).unwrap();
    println!("📤 序列化结果:\n{}\n", json);

    // 美化输出（带缩进）
    let json_pretty = serde_json::to_string_pretty(&person).unwrap();
    println!("📤 美化输出:\n{}\n", json_pretty);

    // 反序列化：JSON 字符串 → 结构体
    let json_str = r#"{"name":"李四","age":25,"email":"lisi@example.com"}"#;
    let person2: Person = serde_json::from_str(json_str).unwrap();
    println!("📥 反序列化结果:\n{:?}\n", person2);
}

// ============================================
// 示例2：复杂数据结构
// ============================================

#[derive(Serialize, Deserialize, Debug)]
struct Address {
    city: String,
    street: String,
}

#[derive(Serialize, Deserialize, Debug)]
struct Employee {
    id: u32,
    name: String,
    age: u32,
    address: Address,              // 嵌套结构体
    skills: Vec<String>,           // 数组
    active: bool,                  // 布尔值
}

fn example2_complex() {
    println!("============================================================");
    println!("示例2：复杂数据结构");
    println!("============================================================\n");

    let employee = Employee {
        id: 1001,
        name: String::from("王五"),
        age: 28,
        address: Address {
            city: String::from("北京"),
            street: String::from("朝阳路123号"),
        },
        skills: vec![
            String::from("Rust"),
            String::from("Python"),
            String::from("JavaScript"),
        ],
        active: true,
    };

    // 序列化
    let json = serde_json::to_string_pretty(&employee).unwrap();
    println!("📤 复杂结构体序列化:\n{}\n", json);

    // 反序列化
    let employee2: Employee = serde_json::from_str(&json).unwrap();
    println!("📥 反序列化验证:");
    println!("  姓名: {}", employee2.name);
    println!("  城市: {}", employee2.address.city);
    println!("  技能: {:?}\n", employee2.skills);
}

// ============================================
// 示例3：Option 和 Vec
// ============================================

#[derive(Serialize, Deserialize, Debug)]
struct User {
    id: u32,
    name: String,
    email: Option<String>,         // 可选字段
    phone: Option<String>,
    tags: Vec<String>,             // 数组
}

fn example3_option_vec() {
    println!("============================================================");
    println!("示例3：Option 和 Vec");
    println!("============================================================\n");

    // 部分字段为 None
    let user1 = User {
        id: 1,
        name: String::from("Alice"),
        email: Some(String::from("alice@example.com")),
        phone: None,  // 可选字段为空
        tags: vec![String::from("admin"), String::from("vip")],
    };

    let json1 = serde_json::to_string_pretty(&user1).unwrap();
    println!("📤 用户1（有 email，无 phone）:\n{}\n", json1);

    // 所有可选字段都有值
    let user2 = User {
        id: 2,
        name: String::from("Bob"),
        email: Some(String::from("bob@example.com")),
        phone: Some(String::from("13800138000")),
        tags: vec![],  // 空数组
    };

    let json2 = serde_json::to_string_pretty(&user2).unwrap();
    println!("📤 用户2（都有值，空数组）:\n{}\n", json2);

    // 反序列化包含 null 的 JSON
    let json_str = r#"{
        "id": 3,
        "name": "Charlie",
        "email": null,
        "phone": "13900139000",
        "tags": ["user"]
    }"#;

    let user3: User = serde_json::from_str(json_str).unwrap();
    println!("📥 反序列化（email 为 null）:");
    println!("  {:?}\n", user3);
}

// ============================================
// 示例4：属性重命名
// ============================================

#[derive(Serialize, Deserialize, Debug)]
struct ApiResponse {
    #[serde(rename = "user_id")]
    id: u32,

    #[serde(rename = "user_name")]
    name: String,

    #[serde(rename = "is_active")]
    active: bool,
}

fn example4_rename() {
    println!("============================================================");
    println!("示例4：字段重命名");
    println!("============================================================\n");

    let response = ApiResponse {
        id: 123,
        name: String::from("测试用户"),
        active: true,
    };

    // 序列化时使用重命名后的字段名
    let json = serde_json::to_string_pretty(&response).unwrap();
    println!("📤 重命名后的 JSON:\n{}\n", json);

    // 反序列化也使用重命名后的字段名
    let json_str = r#"{
        "user_id": 456,
        "user_name": "另一个用户",
        "is_active": false
    }"#;

    let response2: ApiResponse = serde_json::from_str(json_str).unwrap();
    println!("📥 反序列化结果:");
    println!("  id: {}", response2.id);
    println!("  name: {}", response2.name);
    println!("  active: {}\n", response2.active);
}

// ============================================
// 示例5：实际应用 - 配置文件
// ============================================

#[derive(Serialize, Deserialize, Debug)]
struct DatabaseConfig {
    host: String,
    port: u16,
    username: String,
    password: String,
    database: String,
}

#[derive(Serialize, Deserialize, Debug)]
struct ServerConfig {
    host: String,
    port: u16,
}

#[derive(Serialize, Deserialize, Debug)]
struct AppConfig {
    app_name: String,
    version: String,
    debug: bool,
    server: ServerConfig,
    database: DatabaseConfig,
}

fn example5_config() {
    println!("============================================================");
    println!("示例5：配置文件处理");
    println!("============================================================\n");

    // 创建配置
    let config = AppConfig {
        app_name: String::from("MyApp"),
        version: String::from("1.0.0"),
        debug: true,
        server: ServerConfig {
            host: String::from("127.0.0.1"),
            port: 8080,
        },
        database: DatabaseConfig {
            host: String::from("localhost"),
            port: 5432,
            username: String::from("admin"),
            password: String::from("secret123"),
            database: String::from("myapp_db"),
        },
    };

    // 序列化为 JSON（可以保存到文件）
    let json = serde_json::to_string_pretty(&config).unwrap();
    println!("📤 配置文件内容:\n{}\n", json);

    // 从 JSON 加载配置
    let config2: AppConfig = serde_json::from_str(&json).unwrap();
    println!("📥 加载的配置:");
    println!("  应用名: {}", config2.app_name);
    println!("  服务器: {}:{}", config2.server.host, config2.server.port);
    println!("  数据库: {}@{}", config2.database.username, config2.database.host);
}

fn main() {
    println!("\n🎯 serde 学习 - 序列化和反序列化\n");

    example1_basic();
    example2_complex();
    example3_option_vec();
    example4_rename();
    example5_config();

    println!("============================================================");
    println!("✅ 学习完成！");
    println!("============================================================\n");

    println!("💡 关键要点:");
    println!("1. #[derive(Serialize, Deserialize)] 自动实现序列化");
    println!("2. to_string() 序列化，from_str() 反序列化");
    println!("3. Option 类型对应 JSON 的 null");
    println!("4. #[serde(rename = \"...\")] 重命名字段");
    println!("5. 支持嵌套结构体和数组");
    println!("\n🎯 实际应用：配置文件、API 数据、数据存储");
}
