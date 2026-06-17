# 🌐 Translate CLI - 翻译命令行工具

基于 weather-cli 改造的翻译工具，复用 clap + reqwest + serde 三库组合模式。

---

## ✨ 功能特性

- ✅ 命令行翻译文本
- ✅ 支持多种语言（中、英、日、韩、法、德等）
- ✅ 自动保存翻译历史
- ✅ JSON 格式历史记录
- ✅ 清除历史功能
- ✅ 语言代码查询
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
# 翻译文本（英文 → 中文）
cargo run -- text "Hello World" -f en -t zh

# 翻译文本（中文 → 英文）
cargo run -- text "你好，世界" -f zh -t en

# 查看历史
cargo run -- history

# 清除历史
cargo run -- clear

# 查看支持的语言
cargo run -- languages

# 查看帮助
cargo run -- --help
```

---

## 📖 使用示例

### 1. 翻译文本

```bash
$ cargo run -- text "Hello World" -f en -t zh
🔍 正在翻译...

📝 原文: Hello World
🌐 译文: 你好世界

💾 已保存到历史记录
```

### 2. 自动检测语言

```bash
$ cargo run -- text "Bonjour" -t zh
# 默认 from=auto 自动检测源语言
```

### 3. 查看历史

```bash
$ cargo run -- history
📜 最近 10 条翻译记录:

1. zh → en
   原文: 你好，世界
   译文: Hello world
   时间: 2026-06-14 21:06:04

2. en → zh
   原文: Hello World
   译文: 你好世界
   时间: 2026-06-14 21:05:50
```

### 4. 限制历史条数

```bash
$ cargo run -- history --limit 5
```

### 5. 查看支持的语言

```bash
$ cargo run -- languages
📚 支持的常用语言代码:

  auto - 自动检测
  zh - 中文
  en - 英语
  ja - 日语
  ...
```

---

## 🎨 命令说明

### 子命令

| 命令 | 说明 | 示例 |
|------|------|------|
| `text <TEXT>` | 翻译文本 | `text "Hello" -f en -t zh` |
| `history` | 查看翻译历史 | `history --limit 5` |
| `clear` | 清除历史记录 | `clear` |
| `languages` | 显示支持的语言 | `languages` |

### 选项

**text 命令**：
- `-f, --from <LANG>`：源语言（默认 auto 自动检测）
- `-t, --to <LANG>`：目标语言（默认 zh 中文）

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

### 与 weather-cli 的对比

| 特性 | weather-cli | translate-cli |
|------|------------|---------------|
| **API** | wttr.in（天气）| MyMemory（翻译）|
| **参数** | city | text + from/to |
| **历史** | 城市 + 温度 | 原文 + 译文 |
| **代码** | 约250行 | 约240行 |

**复用的模式**：
- ✅ clap 子命令结构
- ✅ reqwest API 调用
- ✅ serde 数据处理
- ✅ 历史记录管理

**只需修改**：
- API 端点和参数
- 响应数据结构
- 业务逻辑

---

## 🌐 API 说明

本项目使用 [MyMemory](https://mymemory.translated.net/) 提供的免费翻译 API。

**API 端点**：
```
https://api.mymemory.translated.net/get
```

**参数**：
- `q`：要翻译的文本
- `langpair`：语言对（如 en|zh）

**特点**：
- 免费使用
- 无需注册
- 无需 API Key
- 支持多种语言
- JSON 格式响应

---

## 📁 项目结构

```
translate-cli/
├── Cargo.toml              # 项目配置和依赖
├── README.md               # 本文件
├── translate_history.json  # 历史记录（自动生成）
└── src/
    └── main.rs             # 主程序（约 240 行）
```

---

## 🎓 学习要点

### 从 weather-cli 到 translate-cli

**改造的内容**：

1. **命令行参数**：
   ```rust
   // weather-cli
   Get { city: String, verbose: bool }
   
   // translate-cli
   Text { text: String, from: String, to: String }
   ```

2. **API 调用**：
   ```rust
   // weather-cli
   let url = format!("https://wttr.in/{}?format=j1", city);
   
   // translate-cli
   let url = "https://api.mymemory.translated.net/get";
   client.get(url).query(&[("q", text), ("langpair", &format!("{}|{}", from, to))])
   ```

3. **响应结构**：
   ```rust
   // weather-cli
   struct WeatherResponse { current_condition: Vec<...> }
   
   // translate-cli
   struct TranslateResponse { response_data: ResponseData }
   ```

**不变的模式**：
- clap 子命令结构
- reqwest 调用流程
- serde 数据处理
- 历史记录管理
- 错误处理方式

---

## 💡 扩展建议

如果想继续改进，可以添加：

1. **更多功能**：
   - 批量翻译
   - 文件翻译
   - 翻译质量评分

2. **更好的体验**：
   - 彩色输出
   - 进度条
   - 翻译缓存

3. **更多 API**：
   - 支持多个翻译服务
   - API 切换
   - 质量对比

4. **高级功能**：
   - 配置文件
   - 常用短语
   - 翻译建议

---

## 🔄 改造其他工具

基于这个模式，你可以轻松改造为：

**汇率查询工具**：
```rust
Commands::Get { amount: f64, from: String, to: String }
// API: exchangerate-api.com
```

**GitHub 工具**：
```rust
Commands::Search { query: String, language: Option<String> }
// API: api.github.com
```

**股票查询工具**：
```rust
Commands::Quote { symbol: String }
// API: finnhub.io
```

**只需要**：
- 改 API 端点
- 改数据结构
- 改业务逻辑

**核心模式不变！**

---

## 📊 性能统计

- **代码行数**：约 240 行
- **编译时间**：约 18 秒
- **二进制大小**：约 5 MB（release）
- **依赖数量**：5 个核心库
- **翻译速度**：1-2 秒（取决于网络）

---

## 🎯 项目目标达成

✅ **复用三库组合**：完美复用模式
✅ **快速改造**：1-2小时完成
✅ **实用功能**：翻译 + 历史
✅ **良好体验**：友好提示
✅ **完整文档**：使用说明

---

## 🏆 学习成果

通过这个改造项目，你学会了：

1. **模式复用**：如何复用成功的模式
2. **快速开发**：基于模板快速开发
3. **API 适配**：不同 API 的适配方法
4. **灵活变通**：修改必要的部分

**这证明了模式的价值！** 🎉

---

## 🐛 故障排除

### 网络错误

```
❌ 翻译失败: error sending request
```

**解决**：检查网络连接

### API 限制

免费 API 有请求限制，如果遇到限制：
- 减少请求频率
- 或使用其他翻译 API

---

**享受你的翻译工具！** 🌐✨
