// ============================================
// 天气查询 CLI 工具
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
#[command(name = "weather")]
#[command(about = "🌤️  天气查询 CLI 工具", long_about = None)]
#[command(version)]
struct Cli {
    #[command(subcommand)]
    command: Commands,
}

#[derive(Subcommand)]
enum Commands {
    /// 查询城市天气
    Get {
        /// 城市名（英文或拼音）
        city: String,

        /// 显示详细信息
        #[arg(short, long)]
        verbose: bool,
    },

    /// 查看查询历史
    History {
        /// 显示最近N条记录
        #[arg(short, long, default_value_t = 10)]
        limit: usize,
    },

    /// 清除历史记录
    Clear,
}

// ============================================
// 数据结构定义（serde）
// ============================================

#[derive(Deserialize, Debug)]
struct WeatherResponse {
    current_condition: Vec<CurrentCondition>,
    nearest_area: Vec<NearestArea>,
}

#[derive(Deserialize, Debug)]
struct CurrentCondition {
    temp_C: String,
    temp_F: String,
    weatherDesc: Vec<WeatherDesc>,
    humidity: String,
    windspeedKmph: String,
    #[serde(rename = "FeelsLikeC")]
    feels_like_c: String,
}

#[derive(Deserialize, Debug)]
struct WeatherDesc {
    value: String,
}

#[derive(Deserialize, Debug)]
struct NearestArea {
    areaName: Vec<AreaName>,
    country: Vec<Country>,
}

#[derive(Deserialize, Debug)]
struct AreaName {
    value: String,
}

#[derive(Deserialize, Debug)]
struct Country {
    value: String,
}

// ============================================
// 配置和历史记录
// ============================================

#[derive(Serialize, Deserialize, Debug)]
struct HistoryRecord {
    city: String,
    temperature: String,
    description: String,
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
// 天气查询功能（reqwest）
// ============================================

fn fetch_weather(city: &str) -> Result<WeatherResponse, Box<dyn std::error::Error>> {
    let client = Client::new();

    // 使用 wttr.in API（免费，无需注册）
    let url = format!("https://wttr.in/{}?format=j1", city);

    println!("🔍 正在查询 {} 的天气...\n", city);

    let response = client
        .get(&url)
        .header("User-Agent", "weather-cli/1.0")
        .send()?;

    if !response.status().is_success() {
        return Err(format!("API 请求失败: {}", response.status()).into());
    }

    let weather: WeatherResponse = response.json()?;
    Ok(weather)
}

fn display_weather(weather: &WeatherResponse, verbose: bool) {
    if let Some(condition) = weather.current_condition.first() {
        let desc = condition.weatherDesc.first()
            .map(|d| d.value.as_str())
            .unwrap_or("未知");

        let location = weather.nearest_area.first()
            .map(|area| {
                format!(
                    "{}, {}",
                    area.areaName.first().map(|a| a.value.as_str()).unwrap_or(""),
                    area.country.first().map(|c| c.value.as_str()).unwrap_or("")
                )
            })
            .unwrap_or_else(|| "未知位置".to_string());

        println!("📍 位置: {}", location);
        println!("🌡️  温度: {}°C ({}°F)", condition.temp_C, condition.temp_F);
        println!("🌤️  天气: {}", desc);

        if verbose {
            println!("💧 湿度: {}%", condition.humidity);
            println!("💨 风速: {} km/h", condition.windspeedKmph);
            println!("🤔 体感温度: {}°C", condition.feels_like_c);
        }
    }
}

// ============================================
// 主程序
// ============================================

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let cli = Cli::parse();

    let config_file = "weather_history.json";
    let mut config = AppConfig::load(config_file).unwrap_or_default();

    match cli.command {
        Commands::Get { city, verbose } => {
            // 查询天气
            match fetch_weather(&city) {
                Ok(weather) => {
                    display_weather(&weather, verbose);

                    // 保存到历史
                    if let Some(condition) = weather.current_condition.first() {
                        let desc = condition.weatherDesc.first()
                            .map(|d| d.value.clone())
                            .unwrap_or_else(|| "未知".to_string());

                        let record = HistoryRecord {
                            city: city.clone(),
                            temperature: format!("{}°C", condition.temp_C),
                            description: desc,
                            timestamp: chrono::Local::now().format("%Y-%m-%d %H:%M:%S").to_string(),
                        };

                        config.add_history(record);
                        config.save(config_file)?;

                        println!("\n💾 已保存到历史记录");
                    }
                }
                Err(e) => {
                    eprintln!("❌ 查询失败: {}", e);
                    eprintln!("💡 提示：请检查城市名是否正确（使用英文或拼音）");
                    std::process::exit(1);
                }
            }
        }

        Commands::History { limit } => {
            // 显示历史记录
            if config.history.is_empty() {
                println!("📝 暂无查询历史");
            } else {
                println!("📜 最近 {} 条查询记录:\n", limit.min(config.history.len()));
                for (i, record) in config.history.iter().take(limit).enumerate() {
                    println!("{}. {} - {} - {} ({})",
                        i + 1,
                        record.city,
                        record.temperature,
                        record.description,
                        record.timestamp
                    );
                }
            }
        }

        Commands::Clear => {
            // 清除历史
            config.clear_history();
            config.save(config_file)?;
            println!("🗑️  已清除所有历史记录");
        }
    }

    Ok(())
}
