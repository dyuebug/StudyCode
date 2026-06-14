// ============================================
// minigrep - 文本搜索工具
// 主程序：支持 JSON 配置文件
// ============================================

use std::env;
use std::process;

use minigrep::config::AppConfig;
use minigrep::Config;

const CONFIG_FILE: &str = "minigrep_config.json";

fn main() {
    // 1. 加载配置文件（如果存在）
    let mut app_config = match AppConfig::load(CONFIG_FILE) {
        Ok(config) => {
            println!("✅ 已加载配置文件\n");
            config
        }
        Err(_) => {
            println!("📝 使用默认配置（首次运行）\n");
            AppConfig::default()
        }
    };

    // 2. 读取命令行参数
    let args: Vec<String> = env::args().collect();

    // 特殊命令：显示配置
    if args.len() >= 2 && args[1] == "--config" {
        app_config.display();
        return;
    }

    // 3. 解析搜索参数
    let config = Config::build(&args).unwrap_or_else(|err| {
        eprintln!("❌ 参数解析错误: {}", err);
        eprintln!("用法: minigrep <搜索词> <文件名>");
        eprintln!("      minigrep --config  (显示配置)");
        eprintln!("\n💡 提示：设置环境变量 IGNORE_CASE=1 可忽略大小写");
        process::exit(1);
    });

    // 4. 显示搜索信息
    println!("🔍 搜索词: \"{}\"", config.query);
    println!("📄 文件名: {}", config.file_path);
    println!("🔤 大小写: {}\n", if config.ignore_case { "忽略" } else { "敏感" });

    // 5. 添加到搜索历史
    app_config.add_search_history(&config.query, &config.file_path);

    // 6. 运行搜索
    if let Err(e) = minigrep::run(config) {
        eprintln!("❌ 应用错误: {}", e);
        process::exit(1);
    }

    // 7. 保存配置（包含搜索历史）
    if let Err(e) = app_config.save(CONFIG_FILE) {
        eprintln!("⚠️  保存配置失败: {}", e);
    } else {
        println!("\n💾 搜索历史已保存到 {}", CONFIG_FILE);
        println!("💡 使用 'minigrep --config' 查看配置和历史记录");
    }
}
