// ============================================
// 翻译 CLI 工具
// 整合 clap + reqwest + serde
// ============================================

use clap::{Parser, Subcommand};
use reqwest::blocking::Client;
use serde::{Deserialize, Serialize};
use std::fs;
use std::path::Path;

// ============================================
// 命令行参数定义（clap）
// ============================================

#[derive(Parser)]
#[command(name = "translate")]
#[command(about = "🌐 翻译 CLI 工具", long_about = None)]
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
// 翻译功能（reqwest）
// ============================================

fn translate_text(text: &str, from: &str, to: &str) -> Result<String, Box<dyn std::error::Error>> {
    let client = Client::new();

    // 使用 MyMemory API（免费，无需注册）
    let url = "https://api.mymemory.translated.net/get";

    println!("🔍 正在翻译...\n");

    let response = client
        .get(url)
        .query(&[
            ("q", text),
            ("langpair", &format!("{}|{}", from, to)),
        ])
        .header("User-Agent", "translate-cli/1.0")
        .send()?;

    if !response.status().is_success() {
        return Err(format!("API 请求失败: {}", response.status()).into());
    }

    let translate_response: TranslateResponse = response.json()?;
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
    println!("  translate text \"Hello\" -f en -t zh");
    println!("  translate text \"你好\" -f zh -t en");
}

// ============================================
// 主程序
// ============================================

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let cli = Cli::parse();

    let config_file = "translate_history.json";
    let mut config = AppConfig::load(config_file).unwrap_or_default();

    match cli.command {
        Commands::Text { text, from, to } => {
            // 翻译文本
            match translate_text(&text, &from, &to) {
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
            // 显示历史记录
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
            // 清除历史
            config.clear_history();
            config.save(config_file)?;
            println!("🗑️  已清除所有历史记录");
        }

        Commands::Languages => {
            // 显示支持的语言
            display_languages();
        }
    }

    Ok(())
}
