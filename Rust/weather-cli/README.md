# 🌤️ Weather CLI - 天气查询命令行工具

一个使用 Rust 开发的实用天气查询 CLI 工具，整合了 clap、reqwest 和 serde 三个核心库。

---

## ✨ 功能特性

- ✅ 命令行查询城市天气
- ✅ 显示温度、天气状况、湿度、风速等信息
- ✅ 详细模式（verbose）显示更多信息
- ✅ 自动保存查询历史
- ✅ JSON 格式历史记录
- ✅ 清除历史功能
- ✅ 友好的错误提示
- ✅ 自动生成帮助信息

---

## 🚀 快速开始

### 安装依赖

```bash
cargo build --release
```

### 基本使用

```bash
# 查询天气
cargo run -- get Beijing

# 详细模式
cargo run -- get Shanghai --verbose

# 查看历史
cargo run -- history

# 清除历史
cargo run -- clear

# 查看帮助
cargo run -- --help
```

---

## 📖 使用示例

### 1. 查询天气

```bash
$ cargo run -- get Beijing
🔍 正在查询 Beijing 的天气...

📍 位置: Beijing, China
🌡️  温度: 27°C (81°F)
🌤️  天气: Sunny

💾 已保存到历史记录
```

### 2. 详细模式

```bash
$ cargo run -- get Shanghai --verbose
🔍 正在查询 Shanghai 的天气...

📍 位置: Shanghai, China
🌡️  温度: 24°C (76°F)
🌤️  天气: Partly cloudy
💧 湿度: 83%
💨 风速: 24 km/h
🤔 体感温度: 27°C

💾 已保存到历史记录
```

### 3. 查看历史

```bash
$ cargo run -- history
📜 最近 10 条查询记录:

1. Shanghai - 24°C - Partly cloudy (2026-06-14 20:00:42)
2. Beijing - 27°C - Sunny (2026-06-14 20:00:30)
```

### 4. 限制历史条数

```bash
$ cargo run -- history --limit 5
📜 最近 5 条查询记录:
...
```

### 5. 清除历史

```bash
$ cargo run -- clear
🗑️  已清除所有历史记录
```

---

## 🎨 命令说明

### 子命令

| 命令 | 说明 | 示例 |
|------|------|------|
| `get <CITY>` | 查询城市天气 | `get Beijing` |
| `history` | 查看查询历史 | `history --limit 5` |
| `clear` | 清除历史记录 | `clear` |

### 选项

**get 命令**：
- `-v, --verbose`：显示详细信息（湿度、风速、体感温度）

**history 命令**：
- `-l, --limit <N>`：显示最近 N 条记录（默认 10）

**全局选项**：
- `-h, --help`：显示帮助信息
- `-V, --version`：显示版本信息

---

## 🏗️ 技术架构

### 技术栈

| 库 | 版本 | 用途 |
|------|------|------|
| **clap** | 4.5 | 命令行参数解析 |
| **reqwest** | 0.12 | HTTP 客户端 |
| **serde** | 1.0 | JSON 序列化/反序列化 |
| **serde_json** | 1.0 | JSON 处理 |
| **chrono** | 0.4 | 时间处理 |

### 代码结构

```rust
// 1. clap - 定义命令行结构
#[derive(Parser)]
struct Cli {
    #[command(subcommand)]
    command: Commands,
}

// 2. serde - 定义数据结构
#[derive(Deserialize)]
struct WeatherResponse {
    current_condition: Vec<CurrentCondition>,
    // ...
}

// 3. reqwest - 发送 HTTP 请求
fn fetch_weather(city: &str) -> Result<WeatherResponse> {
    let response = client.get(&url).send()?;
    let weather: WeatherResponse = response.json()?;
    Ok(weather)
}
```

### 数据流

```
命令行输入 (clap)
    ↓
解析参数
    ↓
HTTP 请求 (reqwest)
    ↓
API 响应 (JSON)
    ↓
反序列化 (serde)
    ↓
数据处理和展示
    ↓
保存历史 (JSON 文件)
```

---

## 🌐 API 说明

本项目使用 [wttr.in](https://wttr.in) 提供的免费天气 API。

**API 端点**：
```
https://wttr.in/{城市名}?format=j1
```

**特点**：
- 免费使用
- 无需注册
- 无需 API Key
- 支持全球城市
- JSON 格式响应

---

## 📁 项目结构

```
weather-cli/
├── Cargo.toml              # 项目配置和依赖
├── README.md               # 本文件
├── weather_history.json    # 历史记录（自动生成）
└── src/
    └── main.rs             # 主程序（约 250 行）
```

---

## 🎓 学习要点

### clap 应用

```rust
#[derive(Parser)]
#[command(name = "weather")]
#[command(about = "🌤️  天气查询 CLI 工具")]
struct Cli {
    #[command(subcommand)]
    command: Commands,
}

#[derive(Subcommand)]
enum Commands {
    Get {
        city: String,
        #[arg(short, long)]
        verbose: bool,
    },
    History { /* ... */ },
}
```

**学到的**：
- 子命令定义（enum）
- 参数注解
- 自动帮助生成

### reqwest 应用

```rust
let response = client
    .get(&url)
    .header("User-Agent", "weather-cli/1.0")
    .send()?;

let weather: WeatherResponse = response.json()?;
```

**学到的**：
- HTTP 请求
- 自动 JSON 反序列化
- 错误处理

### serde 应用

```rust
#[derive(Deserialize)]
struct WeatherResponse {
    current_condition: Vec<CurrentCondition>,
}

#[derive(Serialize, Deserialize)]
struct AppConfig {
    history: Vec<HistoryRecord>,
}
```

**学到的**：
- 结构体序列化
- JSON 文件读写
- 嵌套结构处理

---

## 💡 扩展建议

如果想继续改进，可以添加：

1. **更多城市支持**：
   - 支持中文城市名
   - 城市名自动补全

2. **更多天气信息**：
   - 未来几天预报
   - 小时级天气
   - 日出日落时间

3. **配置文件**：
   - 默认城市
   - 温度单位（摄氏/华氏）
   - API 配置

4. **美化输出**：
   - 使用 colored crate 彩色输出
   - ASCII 天气图标
   - 表格化显示

5. **更多功能**：
   - 多城市对比
   - 天气提醒
   - 导出数据

---

## 🐛 故障排除

### 网络错误

```
❌ 查询失败: error sending request
```

**解决**：检查网络连接，确保可以访问 wttr.in

### 城市名错误

```
💡 提示：请检查城市名是否正确（使用英文或拼音）
```

**解决**：使用英文城市名或拼音，如：
- Beijing（北京）
- Shanghai（上海）
- Guangzhou（广州）

---

## 📊 性能统计

- **代码行数**：约 250 行
- **编译时间**：约 18 秒
- **二进制大小**：约 5 MB（release）
- **依赖数量**：5 个核心库
- **查询速度**：1-2 秒（取决于网络）

---

## 🎯 项目目标达成

✅ **整合三大库**：clap + reqwest + serde
✅ **实用功能**：天气查询 + 历史记录
✅ **良好体验**：友好提示 + 自动帮助
✅ **代码质量**：清晰结构 + 错误处理
✅ **完整文档**：使用说明 + 技术细节

---

## 🏆 学习成果

通过这个项目，你学会了：

1. **clap 子命令**：复杂的 CLI 结构
2. **reqwest 实战**：真实 API 调用
3. **serde 应用**：配置和数据管理
4. **错误处理**：Result 和 ? 运算符
5. **项目组织**：模块化设计

**这是一个完整的、可用的实战项目！** 🎉

---

## 📝 许可

本项目为学习项目，欢迎自由使用和修改。

---

**享受你的天气查询工具！** ☀️🌧️⛈️❄️
