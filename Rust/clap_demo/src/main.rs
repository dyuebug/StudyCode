// ============================================
// clap 学习 - 命令行参数解析
// ============================================

use clap::Parser;

// ============================================
// 示例1：基础命令行参数
// ============================================

/// 简单的问候程序
#[derive(Parser, Debug)]
#[command(name = "greet")]
#[command(about = "一个友好的问候程序", long_about = None)]
struct Args {
    /// 要问候的人的名字
    #[arg(short, long)]
    name: String,

    /// 问候次数
    #[arg(short, long, default_value_t = 1)]
    count: u8,
}

fn example1_basic() {
    println!("============================================================");
    println!("示例1：基础命令行参数");
    println!("============================================================\n");

    println!("尝试运行：");
    println!("  cargo run -- --name Alice");
    println!("  cargo run -- --name Bob --count 3");
    println!("  cargo run -- -n Charlie -c 2");
    println!("  cargo run -- --help\n");
}

// ============================================
// 示例2：位置参数和可选参数
// ============================================

/// 文件处理工具
#[derive(Parser, Debug)]
#[command(name = "file-tool")]
#[command(about = "文件处理工具")]
struct FileArgs {
    /// 输入文件路径（必需）
    input: String,

    /// 输出文件路径（可选）
    output: Option<String>,

    /// 是否详细输出
    #[arg(short, long)]
    verbose: bool,

    /// 是否强制覆盖
    #[arg(short, long)]
    force: bool,
}

fn example2_positional() {
    println!("============================================================");
    println!("示例2：位置参数和可选参数");
    println!("============================================================\n");

    println!("位置参数：不需要 --flag，直接写值");
    println!("可选参数：Option<T> 类型\n");

    println!("尝试运行：");
    println!("  cargo run -- input.txt");
    println!("  cargo run -- input.txt output.txt");
    println!("  cargo run -- input.txt --verbose");
    println!("  cargo run -- input.txt output.txt -v -f\n");
}

// ============================================
// 示例3：子命令
// ============================================

/// Git 风格的命令行工具
#[derive(Parser, Debug)]
#[command(name = "mytool")]
#[command(about = "一个支持子命令的工具")]
struct Cli {
    #[command(subcommand)]
    command: Commands,
}

#[derive(Parser, Debug)]
enum Commands {
    /// 添加文件
    Add {
        /// 文件路径
        file: String,
    },
    /// 删除文件
    Remove {
        /// 文件路径
        file: String,
        /// 是否强制删除
        #[arg(short, long)]
        force: bool,
    },
    /// 列出文件
    List {
        /// 是否显示详细信息
        #[arg(short, long)]
        verbose: bool,
    },
}

fn example3_subcommands() {
    println!("============================================================");
    println!("示例3：子命令（类似 git add/remove/list）");
    println!("============================================================\n");

    println!("子命令让你的 CLI 像 git 一样强大！\n");

    println!("尝试运行：");
    println!("  cargo run -- add file.txt");
    println!("  cargo run -- remove file.txt");
    println!("  cargo run -- remove file.txt --force");
    println!("  cargo run -- list");
    println!("  cargo run -- list --verbose");
    println!("  cargo run -- help\n");
}

// ============================================
// 示例4：值验证
// ============================================

use clap::ValueEnum;

/// 日志级别
#[derive(ValueEnum, Clone, Debug)]
enum LogLevel {
    Debug,
    Info,
    Warn,
    Error,
}

/// 配置工具
#[derive(Parser, Debug)]
#[command(name = "config")]
struct ConfigArgs {
    /// 日志级别
    #[arg(short, long, value_enum, default_value_t = LogLevel::Info)]
    log_level: LogLevel,

    /// 端口号（1024-65535）
    #[arg(short, long, value_parser = clap::value_parser!(u16).range(1024..))]
    port: Option<u16>,

    /// 超时时间（秒，必须 > 0）
    #[arg(short, long, value_parser = clap::value_parser!(u32).range(1..))]
    timeout: Option<u32>,
}

fn example4_validation() {
    println!("============================================================");
    println!("示例4：值验证");
    println!("============================================================\n");

    println!("clap 可以自动验证参数值！\n");

    println!("尝试运行：");
    println!("  cargo run -- --log-level debug");
    println!("  cargo run -- --log-level info --port 8080");
    println!("  cargo run -- --port 80          # 错误：端口 < 1024");
    println!("  cargo run -- --timeout 0        # 错误：必须 > 0");
    println!("  cargo run -- --log-level xyz    # 错误：无效的日志级别\n");
}

// ============================================
// 示例5：实用示例 - 搜索工具
// ============================================

/// 文本搜索工具（类似 grep）
#[derive(Parser, Debug)]
#[command(name = "search")]
#[command(about = "在文件中搜索文本", long_about = None)]
#[command(version)]
struct SearchArgs {
    /// 搜索模式
    pattern: String,

    /// 文件路径
    files: Vec<String>,

    /// 忽略大小写
    #[arg(short, long)]
    ignore_case: bool,

    /// 显示行号
    #[arg(short = 'n', long)]
    line_number: bool,

    /// 只显示匹配的文件名
    #[arg(short = 'l', long)]
    files_with_matches: bool,

    /// 上下文行数
    #[arg(short = 'C', long, default_value_t = 0)]
    context: usize,
}

fn example5_practical() {
    println!("============================================================");
    println!("示例5：实用示例 - 搜索工具");
    println!("============================================================\n");

    println!("一个完整的 grep 风格搜索工具！\n");

    println!("尝试运行：");
    println!("  cargo run -- \"hello\" file.txt");
    println!("  cargo run -- \"hello\" file1.txt file2.txt");
    println!("  cargo run -- \"hello\" file.txt -i");
    println!("  cargo run -- \"hello\" file.txt -n");
    println!("  cargo run -- \"hello\" file.txt -i -n");
    println!("  cargo run -- \"hello\" file.txt -C 2");
    println!("  cargo run -- --version");
    println!("  cargo run -- --help\n");
}

fn main() {
    println!("\n🎯 clap 学习 - 命令行参数解析\n");

    example1_basic();
    example2_positional();
    example3_subcommands();
    example4_validation();
    example5_practical();

    println!("============================================================");
    println!("✅ 学习完成！");
    println!("============================================================\n");

    println!("💡 关键要点:");
    println!("1. #[derive(Parser)] 自动生成参数解析");
    println!("2. #[arg(short, long)] 定义短选项和长选项");
    println!("3. 位置参数不需要 --flag");
    println!("4. Option<T> 表示可选参数");
    println!("5. bool 类型自动成为开关（flag）");
    println!("6. 子命令用 enum 定义");
    println!("7. 自动生成 --help 和 --version");
    println!("8. value_parser 可以验证参数值");
    println!("\n🎯 实际应用：CLI 工具、配置管理、自动化脚本");
    println!("\n💡 提示：运行 cargo run -- --help 查看帮助信息！");
}
