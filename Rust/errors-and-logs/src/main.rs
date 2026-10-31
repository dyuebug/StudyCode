// ============================================================================
//  main.rs · 让日志真的被配置起来
// ============================================================================
//  最常见的「静默失败」：代码里写了 `info!`，但从没调用 `tracing_subscriber::fmt().init()`，
//  于是日志一条都不出现，而程序**不报错**。
//  这就是 `静默失败检查清单` §2.3「配置写了但不生效」的日志版。
// ============================================================================

use errors_and_logs::{config::Config, error::AppError, find_task, startup_banner};
use tracing::{error, info};

fn main() {
    // 顺序很重要：subscriber 必须在**任何 span/event 之前**安装，
    // 否则前面那些日志会被丢掉（tracing 没有缓冲重放机制）
    tracing_subscriber::fmt()
        .with_max_level(tracing::Level::DEBUG)
        // 关掉 ANSI 便于把输出贴进文档/CI 日志
        .with_ansi(false)
        .with_target(false)
        // 关键：默认只有「span 内部打了日志」时 span 才会出现在输出里。
        // 加上这一行才会在 span 结束时输出一条 close 事件——
        // 这就是「`#[instrument]` 加了却什么都看不到」的常见原因。
        .with_span_events(tracing_subscriber::fmt::format::FmtSpan::CLOSE)
        .init();

    // ---- 1. 配置：本机没有环境变量时走默认值 ----
    let cfg = match Config::from_env() {
        Ok(c) => c,
        Err(err) => {
            // 启动期错误用 anyhow 的 `{:#}`：一次打印完整因果链
            error!("配置加载失败：{err:#}");
            std::process::exit(1);
        }
    };
    startup_banner(&cfg);

    // ---- 2. 三种结果：成功、参数错误（4xx）、内部错误（5xx + 错误链）----
    for id in [7, 0, 999] {
        match find_task(id) {
            Ok(name) => info!(task = %name, "命中"),
            Err(err) => {
                // 对外：只有 status + code，没有内部细节
                info!(status = err.status(), code = err.code(), "请求被拒绝");
                // 对内：完整链路逐层打印——这就是 #[source] 的作用
                log_chain(&err);
            }
        }
    }

    // ---- 3. 对照：anyhow 的 `{}` 与 `{:#}` ----
    println!("\n=== anyhow 的两种打印 ===");
    match Config::from_file(std::path::Path::new("不存在的配置文件.conf")) {
        Ok(_) => info!("意外成功"),
        Err(err) => {
            println!("{{}}  （只有最外层）：{err}");
            println!("{{:#}}（完整因果链）：{err:#}");
        }
    }
}

/// 把错误链逐层写进日志，每层一个字段。
///
/// 为什么不用 `error!("{err:#}")` 一把梭：
/// 逐层入日志后，**每一层都能被单独检索**（比如按 `layer=1` 聚合数据库原文），
/// 拼成一整行字符串就失去了结构化查询能力。
fn log_chain(err: &AppError) {
    for (layer, msg) in err.chain().iter().enumerate() {
        error!(layer, msg = %msg, "错误链");
    }
}
