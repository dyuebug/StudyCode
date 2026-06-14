// ============================================
// minigrep - 文本搜索工具
// 库模块：核心搜索逻辑
// ============================================

pub mod config;

use std::env;
use std::error::Error;
use std::fs;

/// 配置结构体
pub struct Config {
    pub query: String,
    pub file_path: String,
    pub ignore_case: bool,
}

impl Config {
    /// 从命令行参数构建配置
    pub fn build(args: &[String]) -> Result<Config, &'static str> {
        if args.len() < 3 {
            return Err("参数不足！用法: minigrep <搜索词> <文件名>");
        }

        let query = args[1].clone();
        let file_path = args[2].clone();

        // 检查环境变量 IGNORE_CASE
        // 如果设置了（任何值），则忽略大小写
        let ignore_case = env::var("IGNORE_CASE").is_ok();

        Ok(Config {
            query,
            file_path,
            ignore_case,
        })
    }
}

/// 运行程序主逻辑
pub fn run(config: Config) -> Result<(), Box<dyn Error>> {
    // 读取文件
    let contents = fs::read_to_string(config.file_path)?;

    // 根据配置选择搜索方式
    let results = if config.ignore_case {
        search_case_insensitive(&config.query, &contents)
    } else {
        search(&config.query, &contents)
    };

    // 显示结果
    if results.is_empty() {
        println!("😔 没有找到匹配的行");
    } else {
        println!("✅ 找到 {} 个匹配:\n", results.len());
        for (line_num, line) in results {
            println!("  行 {}: {}", line_num, line);
        }
    }

    Ok(())
}

/// 大小写敏感搜索
pub fn search<'a>(query: &str, contents: &'a str) -> Vec<(usize, &'a str)> {
    contents
        .lines()
        .enumerate()
        .filter(|(_, line)| line.contains(query))
        .map(|(index, line)| (index + 1, line))
        .collect()
}

/// 大小写不敏感搜索
pub fn search_case_insensitive<'a>(query: &str, contents: &'a str) -> Vec<(usize, &'a str)> {
    let query = query.to_lowercase();

    contents
        .lines()
        .enumerate()
        .filter(|(_, line)| line.to_lowercase().contains(&query))
        .map(|(index, line)| (index + 1, line))
        .collect()
}

// ============================================
// 测试模块
// ============================================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn case_sensitive() {
        let query = "duct";
        let contents = "\
Rust:
safe, fast, productive.
Pick three.
Duct tape.";

        let results = search(query, contents);
        assert_eq!(results, vec![(2, "safe, fast, productive.")]);
    }

    #[test]
    fn case_insensitive() {
        let query = "rUsT";
        let contents = "\
Rust:
safe, fast, productive.
Pick three.
Trust me.";

        let results = search_case_insensitive(query, contents);
        assert_eq!(results.len(), 2);
        assert_eq!(results[0], (1, "Rust:"));
        assert_eq!(results[1], (4, "Trust me."));
    }
}
