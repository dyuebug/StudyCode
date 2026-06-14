# 🌐 reqwest + clap 学习总结

学习了两个最常用的 Rust 库：clap（命令行参数解析）和 reqwest（HTTP 客户端）。

---

## 📦 clap - 命令行参数解析

### 核心概念

clap 让命令行参数解析变得简单：
- 使用 derive 宏自动生成解析代码
- 自动生成帮助信息
- 支持验证和类型转换

### 基础用法

```rust
use clap::Parser;

#[derive(Parser)]
#[command(name = "myapp")]
#[command(about = "我的应用")]
struct Args {
    /// 名字（必需参数）
    #[arg(short, long)]
    name: String,
    
    /// 次数（可选，默认1）
    #[arg(short, long, default_value_t = 1)]
    count: u8,
    
    /// 是否详细输出（flag）
    #[arg(short, long)]
    verbose: bool,
}

fn main() {
    let args = Args::parse();
    println!("Hello, {}!", args.name);
}
```

### 关键特性

1. **短选项和长选项**：
   - `-n` 和 `--name`
   - 自动支持

2. **位置参数**：
   ```rust
   input: String,  // 不需要 --flag
   ```

3. **可选参数**：
   ```rust
   output: Option<String>,
   ```

4. **默认值**：
   ```rust
   #[arg(default_value_t = 1)]
   count: u8,
   ```

5. **子命令**：
   ```rust
   #[derive(Parser)]
   enum Commands {
       Add { file: String },
       Remove { file: String },
   }
   ```

6. **值验证**：
   ```rust
   #[arg(value_parser = clap::value_parser!(u16).range(1024..))]
   port: u16,
   ```

---

## 🌐 reqwest - HTTP 客户端

### 核心概念

reqwest 让 HTTP 请求变得简单：
- 同步和异步支持
- 自动 JSON 序列化/反序列化
- 完整的 HTTP 功能

### 基础用法

```rust
use reqwest::blocking::Client;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    // GET 请求
    let response = reqwest::blocking::get("https://api.example.com/data")?;
    let body = response.text()?;
    println!("{}", body);
    
    Ok(())
}
```

### 关键特性

1. **GET 请求**：
   ```rust
   let response = client.get(url).send()?;
   ```

2. **带参数的 GET**：
   ```rust
   client.get(url)
       .query(&[("key", "value")])
       .send()?;
   ```

3. **POST JSON**：
   ```rust
   client.post(url)
       .json(&data)
       .send()?;
   ```

4. **自定义请求头**：
   ```rust
   client.get(url)
       .header("User-Agent", "MyApp")
       .send()?;
   ```

5. **解析 JSON 响应**：
   ```rust
   let data: MyStruct = client.get(url)
       .send()?
       .json()?;
   ```

6. **错误处理**：
   ```rust
   match response.status() {
       s if s.is_success() => { /* 2xx */ }
       s if s.is_client_error() => { /* 4xx */ }
       s if s.is_server_error() => { /* 5xx */ }
       _ => {}
   }
   ```

7. **超时**：
   ```rust
   client.get(url)
       .timeout(Duration::from_secs(10))
       .send()?;
   ```

---

## 🎯 三库组合（clap + reqwest + serde）

### 完美组合

这三个库一起使用可以构建强大的 CLI 工具：

```rust
use clap::Parser;
use reqwest::blocking::Client;
use serde::Deserialize;

#[derive(Parser)]
struct Args {
    #[arg(short, long)]
    city: String,
}

#[derive(Deserialize)]
struct Weather {
    temperature: f32,
    description: String,
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    // 1. clap 解析参数
    let args = Args::parse();
    
    // 2. reqwest 发送请求
    let client = Client::new();
    let url = format!("https://api.weather.com/{}", args.city);
    
    // 3. serde 解析 JSON
    let weather: Weather = client.get(&url)
        .send()?
        .json()?;
    
    println!("{}°C - {}", weather.temperature, weather.description);
    
    Ok(())
}
```

---

## 📊 对比表

| 功能 | 手动实现 | 使用库 |
|------|---------|--------|
| **参数解析** | env::args() + 手动处理 | clap derive 宏 |
| **帮助信息** | 手动编写 | 自动生成 |
| **参数验证** | 手动检查 | value_parser |
| **HTTP 请求** | TcpStream + 手动构造 | reqwest 一行代码 |
| **JSON 处理** | serde_json | reqwest + serde 自动 |

---

## 🎓 学习要点

### clap 核心 API

| 注解 | 用途 |
|------|------|
| `#[derive(Parser)]` | 启用参数解析 |
| `#[arg(short, long)]` | 短选项 + 长选项 |
| `#[arg(default_value_t)]` | 默认值 |
| `#[command(about)]` | 程序描述 |
| `Option<T>` | 可选参数 |
| `Vec<T>` | 多值参数 |

### reqwest 核心 API

| 方法 | 用途 |
|------|------|
| `Client::new()` | 创建客户端 |
| `.get(url)` | GET 请求 |
| `.post(url)` | POST 请求 |
| `.json(&data)` | 发送 JSON |
| `.send()?.json::<T>()` | 接收 JSON |
| `.query(&params)` | 查询参数 |
| `.header(k, v)` | 请求头 |
| `.timeout(d)` | 超时设置 |

---

## 💡 实际应用场景

### clap 适用于

1. **CLI 工具**：任何命令行程序
2. **自动化脚本**：参数化脚本
3. **配置管理**：命令行配置工具

### reqwest 适用于

1. **API 客户端**：调用 REST API
2. **数据抓取**：爬虫、数据采集
3. **微服务通信**：服务间 HTTP 调用
4. **测试工具**：API 测试、性能测试

### 组合使用

1. **天气查询工具**：clap + reqwest + serde
2. **API 测试工具**：命令行 + HTTP 请求
3. **数据下载器**：参数化 + 批量下载
4. **监控工具**：定时检查 + 报告

---

## 🚀 下一步

你现在可以：

1. **实战项目**：
   - 天气查询 CLI
   - GitHub API 客户端
   - 汇率查询工具

2. **改进现有项目**：
   - minigrep 使用 clap
   - HTTP 服务器返回 JSON

3. **学习异步**：
   - reqwest 异步版本
   - tokio 运行时

---

## 📝 完整示例

查看项目目录：
- `clap_demo/` - 5个 clap 示例
- `reqwest_demo/` - 6个 reqwest 示例

运行方式：
```bash
cd clap_demo && cargo run -- --help
cd reqwest_demo && cargo run  # 需要网络
```

---

**学习完成！** 🎉

你现在掌握了 Rust 三大实用库：
- ✅ serde（序列化）
- ✅ clap（命令行）
- ✅ reqwest（HTTP）

这些是构建实用 CLI 工具的黄金组合！
