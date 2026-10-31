// CLI 层只做三件事：解析参数 → 调 lib → 打印。
// 业务逻辑一律留在 lib.rs，这样 tests/ 能直接测逻辑而不必跑进程。

use file_manager::cli::{self, Cmd};
use file_manager::{copy_streaming, find_by_ext, human, walk, WalkStats};
use std::process::ExitCode;

fn main() -> ExitCode {
    // 跳过 argv[0]（程序名）
    let args: Vec<String> = std::env::args().skip(1).collect();

    let cmd = match cli::parse(args) {
        Ok(c) => c,
        Err(msg) => {
            // 用法错误走 stderr + 非零退出码：脚本里能立刻发现
            eprintln!("用法错误：{msg}\n");
            eprint!("{}", cli::help_text());
            return ExitCode::FAILURE;
        }
    };

    match cmd {
        Cmd::Help => {
            print!("{}", cli::help_text());
        }

        Cmd::Tree { path, depth } => {
            let (entries, stats) = walk(&path, depth);
            for e in &entries {
                let indent = "  ".repeat(e.depth.saturating_sub(1));
                let mark = if e.is_symlink {
                    " -> (symlink，未展开)"
                } else if e.is_dir {
                    "/"
                } else {
                    ""
                };
                let size = if e.is_dir || e.is_symlink {
                    String::new()
                } else {
                    format!("  {}", human(e.len))
                };
                println!(
                    "{indent}{}{mark}{size}",
                    e.path.file_name().unwrap_or_default().to_string_lossy()
                );
            }
            report(&stats);
        }

        Cmd::Find { path, ext } => {
            let (hits, stats) = find_by_ext(&path, &ext);
            for h in &hits {
                println!("{}", h.display());
            }
            println!("\n命中 {} 个文件", hits.len());
            report(&stats);
        }

        Cmd::Du { path } => {
            let (_, stats) = walk(&path, usize::MAX);
            println!("目录：{}", path.display());
            println!("文件 {} 个 / 子目录 {} 个", stats.files, stats.dirs);
            println!("合计 {}", human(stats.bytes));
            report(&stats);
        }

        Cmd::Copy { src, dst, dry_run } => match copy_streaming(&src, &dst, dry_run) {
            Ok(n) if dry_run => {
                println!("[dry-run] 将复制 {}（{}）", src.display(), human(n));
            }
            Ok(n) => println!("已复制 {} 字节 → {}", n, dst.display()),
            Err(e) => {
                eprintln!("复制失败：{e}");
                return ExitCode::FAILURE;
            }
        },
    }

    ExitCode::SUCCESS
}

/// 统一报告"扫描时遇到的问题"。
///
/// 关键设计：出错**不影响退出码**——扫描到一半的目录没权限是常态，
/// 把它变成整条命令失败会让人不再敢用。但也不能不说，否则就是静默失败。
fn report(stats: &WalkStats) {
    if stats.symlinks_skipped > 0 {
        println!(
            "\n跳过 {} 个符号链接（不展开，避免成环）",
            stats.symlinks_skipped
        );
    }
    if !stats.errors.is_empty() {
        eprintln!("\n有 {} 处无法访问：", stats.errors.len());
        for (path, reason) in &stats.errors {
            eprintln!("  {} — {reason}", path.display());
        }
    }
}
