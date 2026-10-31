// ============================================================================
//  错误处理工程化 + 结构化日志
// ============================================================================
//  日期：2026-11-09（第45天）
//  难度：⭐⭐⭐
//
//  三个 crate 各管一段，边界是这条项目里最容易搞混的地方：
//
//  | 工具      | 用在哪            | 产出的类型               |
//  |-----------|-------------------|--------------------------|
//  | thiserror | 领域层（可穷举）  | `AppError`（enum，可 match）|
//  | anyhow    | 边界层（不可穷举）| `anyhow::Error`（加 context）|
//  | tracing   | 全部              | span + 结构化字段（不是字符串拼接）|
//
//  判断标准只有一句：**调用方需不需要按变体分支？**
//  需要 → thiserror；不需要（打到日志/返回给用户就完事）→ anyhow。
//
//  与 `task-manager` 手写 `ApiError` 的差别不是「更简单」，而是：
//    Display / Error / source / From 四件事由宏生成，且**不会漏**——
//    漏掉的 `source` 不会报错，只会让日志里少一层原因。
// ============================================================================

pub mod config;
pub mod error;

use error::{AppError, DbInner};
use tracing::{info, info_span};

/// 带 span 的领域函数。
///
/// `#[tracing::instrument]` 做三件事：
///   1. 进入函数自动开 span、退出自动关闭（包括 `?` 提前返回）
///   2. 把参数按 Debug 记进 span 字段
///   3. 返回值/错误也记进同一个 span（需要 `err`/`ret`，默认记错误）
///
/// `skip_all` 是必要的：不加它，大结构体会被整个 Debug 打印进日志。
#[tracing::instrument(name = "service.find_task", skip_all, fields(task.id = id))]
pub fn find_task(id: i64) -> Result<String, AppError> {
    if id <= 0 {
        return Err(AppError::Invalid {
            reason: format!("id 必须为正整数，实际 {id}"),
        });
    }
    if id > 100 {
        // 故意制造一条两层错误链：模拟「查询失败」
        return Err(AppError::Db {
            op: "select task",
            source: DbInner {
                code: 1,
                message: "no such row".to_string(),
            },
        });
    }
    Ok(format!("task-{id}"))
}

/// 对照实现：同一个语义，但用任何调用方都不需要 match 的边界函数。
///
/// 返回 `anyhow::Result` 的场合：启动期加载、脚本、CLI 的 main ——
/// 失败就是失败，没人会去 match 变体。
pub fn load_or_default(path: std::path::PathBuf) -> anyhow::Result<config::Config> {
    use anyhow::Context;
    config::Config::from_file(&path)
        .with_context(|| format!("启动时加载配置失败：{}", path.display()))
}

/// 手工 span（不用 instrument 的场合：需要跨多个函数的同一段逻辑）。
///
/// 关键点：span **不是**「更花哨的 println」。它的价值是**字段**——
/// `port=8080` 之后可以被日志系统按字段筛选、聚合，字符串拼进去就不能了。
pub fn startup_banner(cfg: &config::Config) {
    let span = info_span!("startup", host = %cfg.host, port = cfg.port, level = %cfg.log_level);
    let _guard = span.enter();

    info!("服务配置已加载");
    if cfg.port < 1024 {
        // 注意：这里是 warn 而不是 error —— 严重程度决定是否会触发告警
        tracing::warn!(port = cfg.port, "端口低于 1024，非 root 用户可能无法绑定");
    }
    // 这一行演示「读到的值必须打出来」：配置不生效时第一个要看的就是它
    info!(config = %cfg.describe(), "生效配置");
}
