// ============================================
// 翻译 CLI 工具 - 异步版本 🚀
// 整合 clap + reqwest(异步) + serde + tokio
// ============================================

use clap::{Parser, Subcommand};
use reqwest::Client;  // 注意：去掉了 blocking::Client
use serde::{Deserialize, Serialize};
use std::fs;
use std::path::Path;

// ============================================
// 命令行参数定义（clap）
// ============================================

#[derive(Parser)]
#[command(name = "translate-async")]
#[command(about = "🌐 翻译 CLI 工具 - 异步版本 ⚡", long_about = None)]
#[command(version)]
struct Cli {
    #[command(subcommand)]
    command: Commands,
}

#[derive(Subcommand)]
enum Commands {
    /// 翻译文本
    Text {
        /// 要翻译的文本
        text: String,

        /// 源语言（默认：auto 自动检测）
        #[arg(short = 'f', long, default_value = "auto")]
        from: String,

        /// 目标语言（默认：zh 中文）
        #[arg(short = 't', long, default_value = "zh")]
        to: String,
    },

    /// 查看翻译历史
    History {
        /// 显示最近N条记录
        #[arg(short, long, default_value_t = 10)]
        limit: usize,
    },

    /// 清除历史记录
    Clear,

    /// 显示支持的语言
    Languages,

    /// 批量翻译（并发执行）⚡
    Batch {
        /// 要翻译的多个文本
        #[arg(required = true)]
        texts: Vec<String>,

        /// 源语言（默认：auto 自动检测）
        #[arg(short = 'f', long, default_value = "auto")]
        from: String,

        /// 目标语言（默认：zh 中文）
        #[arg(short = 't', long, default_value = "zh")]
        to: String,
    },
}

// ============================================
// 数据结构定义（serde）
// ============================================

#[derive(Deserialize, Debug)]
struct TranslateResponse {
    #[serde(rename = "responseData")]
    response_data: ResponseData,
}

#[derive(Deserialize, Debug)]
struct ResponseData {
    #[serde(rename = "translatedText")]
    translated_text: String,
}

// ============================================
// 配置和历史记录
// ============================================

#[derive(Serialize, Deserialize, Debug)]
struct HistoryRecord {
    source_text: String,
    translated_text: String,
    from_lang: String,
    to_lang: String,
    timestamp: String,
}

#[derive(Serialize, Deserialize, Debug)]
struct AppConfig {
    history: Vec<HistoryRecord>,
}

impl Default for AppConfig {
    fn default() -> Self {
        AppConfig {
            history: Vec::new(),
        }
    }
}

impl AppConfig {
    fn load<P: AsRef<Path>>(path: P) -> Result<Self, Box<dyn std::error::Error>> {
        let contents = fs::read_to_string(path)?;
        let config: AppConfig = serde_json::from_str(&contents)?;
        Ok(config)
    }

    fn save<P: AsRef<Path>>(&self, path: P) -> Result<(), Box<dyn std::error::Error>> {
        let json = serde_json::to_string_pretty(self)?;
        fs::write(path, json)?;
        Ok(())
    }

    fn add_history(&mut self, record: HistoryRecord) {
        self.history.insert(0, record);
        if self.history.len() > 50 {
            self.history.truncate(50);
        }
    }

    fn clear_history(&mut self) {
        self.history.clear();
    }
}


// ============================================
// 翻译功能（异步版本）⚡
// ============================================

/*
【核心改动1】：async fn

同步版本：
fn translate_text(...) -> Result<...>

异步版本：
async fn translate_text(...) -> Result<...>
  ↑ 添加 async 关键字
*/

async fn translate_text(text: &str, from: &str, to: &str) -> Result<String, Box<dyn std::error::Error>> {
    // 创建异步 HTTP 客户端（不是 blocking::Client）
    let client = Client::new();

    // 使用 MyMemory API（免费，无需注册）
    let url = "https://api.mymemory.translated.net/get";

    println!("🔍 正在翻译...\n");

    /*
    【核心改动2】：.await

    同步版本：
    let response = client.get(url).send()?;

    异步版本：
    let response = client.get(url).send().await?;
                                          ↑ 添加 .await

    关键理解：
    - .send() 返回一个 Future
    - .await 等待 Future 完成（不是阻塞，而是挂起）
    - 挂起时，运行时可以执行其他任务
    */

    let response = client
        .get(url)
        .query(&[
            ("q", text),
            ("langpair", &format!("{}|{}", from, to)),
        ])
        .header("User-Agent", "translate-cli-async/1.0")
        .send()
        .await?;  // ⚡ 异步等待

    if !response.status().is_success() {
        return Err(format!("API 请求失败: {}", response.status()).into());
    }

    // .json() 也是异步的，需要 .await
    let translate_response: TranslateResponse = response.json().await?;  // ⚡ 异步等待
    Ok(translate_response.response_data.translated_text)
}

fn display_languages() {
    println!("📚 支持的常用语言代码:\n");

    let languages = vec![
        ("auto", "自动检测"),
        ("zh", "中文"),
        ("en", "英语"),
        ("ja", "日语"),
        ("ko", "韩语"),
        ("fr", "法语"),
        ("de", "德语"),
        ("es", "西班牙语"),
        ("it", "意大利语"),
        ("ru", "俄语"),
        ("pt", "葡萄牙语"),
        ("ar", "阿拉伯语"),
        ("th", "泰语"),
        ("vi", "越南语"),
    ];

    for (code, name) in languages {
        println!("  {} - {}", code, name);
    }

    println!("\n💡 使用示例:");
    println!("  translate-async text \"Hello\" -f en -t zh");
    println!("  translate-async text \"你好\" -f zh -t en");
}


// ============================================
// 主程序（异步版本）⚡
// ============================================

/*
【核心改动3】：#[tokio::main] async fn main

同步版本：
fn main() -> Result<...> { }

异步版本：
#[tokio::main]
async fn main() -> Result<...> { }
  ↑ 添加宏          ↑ 添加 async

关键理解：
- #[tokio::main] 宏会创建 tokio 运行时
- 等价于手动写：
  fn main() {
      let rt = tokio::runtime::Runtime::new().unwrap();
      rt.block_on(async {
          // 异步代码
      });
  }
*/

#[tokio::main]  // ⚡ tokio 运行时宏
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let cli = Cli::parse();

    let config_file = "translate_history_async.json";  // 使用不同的历史文件
    let mut config = AppConfig::load(config_file).unwrap_or_default();

    match cli.command {
        Commands::Text { text, from, to } => {
            /*
            【核心改动4】：调用异步函数需要 .await

            同步版本：
            match translate_text(&text, &from, &to) { }

            异步版本：
            match translate_text(&text, &from, &to).await { }
                                                     ↑ 添加 .await
            */

            // 翻译文本（异步调用）
            match translate_text(&text, &from, &to).await {  // ⚡ 异步调用
                Ok(translated) => {
                    println!("📝 原文: {}", text);
                    println!("🌐 译文: {}\n", translated);

                    // 保存到历史
                    let record = HistoryRecord {
                        source_text: text.clone(),
                        translated_text: translated,
                        from_lang: from.clone(),
                        to_lang: to.clone(),
                        timestamp: chrono::Local::now()
                            .format("%Y-%m-%d %H:%M:%S")
                            .to_string(),
                    };

                    config.add_history(record);
                    config.save(config_file)?;

                    println!("💾 已保存到历史记录");
                }
                Err(e) => {
                    eprintln!("❌ 翻译失败: {}", e);
                    eprintln!("💡 提示：请检查网络连接或语言代码");
                    std::process::exit(1);
                }
            }
        }

        Commands::History { limit } => {
            // 显示历史记录（同步操作，无需改动）
            if config.history.is_empty() {
                println!("📝 暂无翻译历史");
            } else {
                println!("📜 最近 {} 条翻译记录:\n", limit.min(config.history.len()));
                for (i, record) in config.history.iter().take(limit).enumerate() {
                    println!("{}. {} → {}", i + 1, record.from_lang, record.to_lang);
                    println!("   原文: {}", record.source_text);
                    println!("   译文: {}", record.translated_text);
                    println!("   时间: {}\n", record.timestamp);
                }
            }
        }

        Commands::Clear => {
            // 清除历史（同步操作，无需改动）
            config.clear_history();
            config.save(config_file)?;
            println!("🗑️  已清除所有历史记录");
        }

        Commands::Languages => {
            // 显示支持的语言（同步操作，无需改动）
            display_languages();
        }

        Commands::Batch { texts, from, to } => {
            // ============================================
            // 批量翻译（并发执行）⚡
            // ============================================

            println!("============================================================");
            println!("🚀 批量翻译模式 - 并发执行");
            println!("============================================================\n");
            println!("📝 待翻译数量: {} 个", texts.len());
            println!("🌐 {} → {}\n", from, to);

            // 先测试顺序执行（对比用）
            println!("============================================================");
            println!("🐢 方式1：顺序执行（慢）");
            println!("============================================================\n");

            let start_seq = std::time::Instant::now();

            for (i, text) in texts.iter().enumerate() {
                println!("正在翻译 {}/{}: {}", i + 1, texts.len(), text);
                match translate_text(text, &from, &to).await {
                    Ok(translated) => {
                        println!("  ✅ {}\n", translated);
                    }
                    Err(e) => {
                        eprintln!("  ❌ 翻译失败: {}\n", e);
                    }
                }
            }

            let duration_seq = start_seq.elapsed();
            println!("⏱️  顺序执行总耗时: {:.2?}\n", duration_seq);

            // 再测试并发执行
            println!("============================================================");
            println!("⚡ 方式2：并发执行（快）");
            println!("============================================================\n");

            let start_concurrent = std::time::Instant::now();

            /*
            【核心技术】：tokio::join! 并发执行

            创建多个 Future，同时启动：
            - 每个 Future 独立执行
            - 不会相互阻塞
            - 等待所有完成
            - 返回所有结果

            注意：tokio::join! 最多支持64个参数
            对于动态数量的任务，使用 futures::future::join_all
            */

            // 创建所有翻译任务（Future）
            let tasks: Vec<_> = texts
                .iter()
                .map(|text| translate_text(text, &from, &to))
                .collect();

            // 并发执行所有任务
            let results = futures::future::join_all(tasks).await;

            let duration_concurrent = start_concurrent.elapsed();

            // 显示结果
            for (i, (text, result)) in texts.iter().zip(results.iter()).enumerate() {
                match result {
                    Ok(translated) => {
                        println!("{}/{}. {} → {}", i + 1, texts.len(), text, translated);
                    }
                    Err(e) => {
                        eprintln!("{}/{}. {} → ❌ 失败: {}", i + 1, texts.len(), text, e);
                    }
                }
            }

            println!("\n⏱️  并发执行总耗时: {:.2?}", duration_concurrent);

            // 性能对比
            println!("\n============================================================");
            println!("📊 性能对比");
            println!("============================================================\n");
            println!("顺序执行: {:.2?}", duration_seq);
            println!("并发执行: {:.2?}", duration_concurrent);

            let speedup = duration_seq.as_secs_f64() / duration_concurrent.as_secs_f64();
            println!("\n⚡ 性能提升: {:.2}x", speedup);
            println!("\n💡 结论: 并发执行比顺序执行快 {:.0}%", (speedup - 1.0) * 100.0);
        }
    }

    Ok(())
}

/*
==============================================
🎯 异步改造总结
==============================================

核心改动点（只有4处）：

1. Cargo.toml：
   - 去掉 reqwest 的 "blocking" feature
   - 添加 tokio = { version = "1", features = ["full"] }

2. 函数定义：
   fn translate_text(...) -> Result<...>
   ↓
   async fn translate_text(...) -> Result<...>

3. 异步调用：
   .send()?
   ↓
   .send().await?

4. main 函数：
   fn main() -> Result<...>
   ↓
   #[tokio::main]
   async fn main() -> Result<...>

==============================================
💡 关键理解
==============================================

1. async fn 返回 Future，不是直接返回值
2. .await 是"等待 Future 完成"，不是"阻塞"
3. 等待时运行时可以执行其他任务（高效）
4. 网络请求是 I/O 密集，最适合异步

==============================================
🔍 对比：同步 vs 异步
==============================================

同步版本（blocking）：
- 发送请求 → 阻塞等待 → 浪费 CPU
- 适合：简单场景，单个请求

异步版本（async）：
- 发送请求 → 挂起任务 → CPU 做其他事
- 适合：并发场景，多个请求

单个请求：差异不大
多个请求：异步显著更快 ⚡

==============================================
*/
