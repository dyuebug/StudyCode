// ============================================
// minigrep 配置模块
// 使用 serde 处理 JSON 配置文件
// ============================================

use serde::{Deserialize, Serialize};
use std::fs;
use std::path::Path;

/// 配置文件结构
#[derive(Serialize, Deserialize, Debug)]
pub struct AppConfig {
    /// 默认是否忽略大小写
    pub default_ignore_case: bool,

    /// 最近搜索的关键词
    pub recent_searches: Vec<String>,

    /// 最近搜索的文件
    pub recent_files: Vec<String>,

    /// 最大历史记录数
    pub max_history: usize,
}

impl Default for AppConfig {
    fn default() -> Self {
        AppConfig {
            default_ignore_case: false,
            recent_searches: Vec::new(),
            recent_files: Vec::new(),
            max_history: 10,
        }
    }
}

impl AppConfig {
    /// 从文件加载配置
    pub fn load<P: AsRef<Path>>(path: P) -> Result<Self, Box<dyn std::error::Error>> {
        let contents = fs::read_to_string(path)?;
        let config: AppConfig = serde_json::from_str(&contents)?;
        Ok(config)
    }

    /// 保存配置到文件
    pub fn save<P: AsRef<Path>>(&self, path: P) -> Result<(), Box<dyn std::error::Error>> {
        let json = serde_json::to_string_pretty(self)?;
        fs::write(path, json)?;
        Ok(())
    }

    /// 添加搜索历史
    pub fn add_search_history(&mut self, query: &str, file: &str) {
        // 添加搜索关键词（避免重复）
        if !self.recent_searches.contains(&query.to_string()) {
            self.recent_searches.insert(0, query.to_string());
            if self.recent_searches.len() > self.max_history {
                self.recent_searches.truncate(self.max_history);
            }
        }

        // 添加文件路径（避免重复）
        if !self.recent_files.contains(&file.to_string()) {
            self.recent_files.insert(0, file.to_string());
            if self.recent_files.len() > self.max_history {
                self.recent_files.truncate(self.max_history);
            }
        }
    }

    /// 显示配置信息
    pub fn display(&self) {
        println!("\n📋 当前配置:");
        println!("  默认忽略大小写: {}", self.default_ignore_case);
        println!("  最大历史记录: {}", self.max_history);

        if !self.recent_searches.is_empty() {
            println!("\n  最近搜索:");
            for (i, query) in self.recent_searches.iter().enumerate().take(5) {
                println!("    {}. {}", i + 1, query);
            }
        }

        if !self.recent_files.is_empty() {
            println!("\n  最近文件:");
            for (i, file) in self.recent_files.iter().enumerate().take(5) {
                println!("    {}. {}", i + 1, file);
            }
        }
        println!();
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_default_config() {
        let config = AppConfig::default();
        assert_eq!(config.default_ignore_case, false);
        assert_eq!(config.max_history, 10);
    }

    #[test]
    fn test_add_history() {
        let mut config = AppConfig::default();
        config.add_search_history("test", "file.txt");

        assert_eq!(config.recent_searches.len(), 1);
        assert_eq!(config.recent_searches[0], "test");
        assert_eq!(config.recent_files[0], "file.txt");
    }

    #[test]
    fn test_max_history() {
        let mut config = AppConfig::default();
        config.max_history = 3;

        for i in 0..5 {
            config.add_search_history(&format!("query{}", i), &format!("file{}.txt", i));
        }

        // 应该只保留最近的 3 个
        assert_eq!(config.recent_searches.len(), 3);
    }
}
