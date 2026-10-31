// ============================================================================
//  error.rs · thiserror 的职责边界
// ============================================================================
//  一条经验规则（后文 config.rs 会对照）：
//    - **库 / 领域层**：用 thiserror 定义**有限的、可穷举的**错误变体，
//      让调用方可以 match；每条变体自带 Display 文案与 HTTP 映射
//    - **应用 / 边界层**：用 anyhow 把「不知道会以什么姿态失败」的错误
//      串成因果链，加 context，不做穷举
//
//  为什么不用 `Box<dyn Error>`：它能装一切，但装进去之后就没人能 match 了。
//  错误类型的精度，决定了调用方能不能优雅降级（404 vs 500 就是两条路径）。
// ============================================================================

use std::error::Error;
use thiserror::Error;

/// 数据库驱动层抛出的原始错误。
///
/// 单独一个类型的原因是：它需要 `#[source]` 挂在 `AppError::Db` 上，
/// 这样 `err.source()` 才能拿到原文——**给日志看**。给客户端看的永远只有 code。
#[derive(Debug, Error)]
#[error("SQLite 错误（code={code}）：{message}")]
pub struct DbInner {
    pub code: i32,
    pub message: String,
}

/// 应用错误：穷举，可 match，每一条都带 HTTP 映射。
#[derive(Debug, Error)]
pub enum AppError {
    #[error("资源不存在：{resource} id={id}")]
    NotFound { resource: &'static str, id: i64 },

    #[error("参数不合法：{reason}")]
    Invalid { reason: String },

    #[error("数据库操作失败：{op}")]
    Db {
        op: &'static str,
        /// `#[source]` 让 `Error::source()` 返回它，
        /// `#[from]` 让 `?` 可以从 `DbInner` 自动转换
        #[source]
        source: DbInner,
    },

    #[error("配置缺失：{key}")]
    ConfigMissing { key: String },

    /// `#[error(transparent)]`：Display 直接转发给内部错误，
    /// `#[from]`：`?` 遇到 `std::io::Error` 自动包装
    #[error(transparent)]
    Io(#[from] std::io::Error),
}

impl AppError {
    /// 映射到 HTTP 状态码。
    ///
    /// 关键：`Db` 与 `Io` 都映射 500，但**日志里必须能分出是哪一种**——
    /// 所以 `code()` 给了对外可区分的字符串，`chain()` 给了对内完整的因果链。
    pub fn status(&self) -> u16 {
        match self {
            AppError::NotFound { .. } => 404,
            AppError::Invalid { .. } => 400,
            AppError::ConfigMissing { .. } => 500,
            AppError::Db { .. } | AppError::Io(_) => 500,
        }
    }

    /// 对外错误码。**不能包含内部细节**（SQL、路径、主机名）。
    pub fn code(&self) -> &'static str {
        match self {
            AppError::NotFound { .. } => "not_found",
            AppError::Invalid { .. } => "invalid_argument",
            AppError::ConfigMissing { .. } => "internal",
            AppError::Db { .. } | AppError::Io(_) => "internal",
        }
    }

    /// 逐层收集因果链：`[自身, 源错误, 源的源, ...]`。
    ///
    /// 这是 `#[source]` 的价值所在：`println!("{err}")` 只打印最外层，
    /// 而日志需要"为什么失败"的全貌。
    pub fn chain(&self) -> Vec<String> {
        let mut out = vec![self.to_string()];
        let mut cur = self.source();
        while let Some(e) = cur {
            out.push(e.to_string());
            cur = e.source();
        }
        out
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn db_错误的链上有两层() {
        let err = AppError::Db {
            op: "list_tasks",
            source: DbInner {
                code: 1,
                message: "no such table: tasks".to_string(),
            },
        };
        let chain = err.chain();
        assert_eq!(chain.len(), 2);
        assert!(chain[0].contains("list_tasks"));
        // 第二层是给日志看的原文，不会进响应体
        assert!(chain[1].contains("no such table"));
    }

    #[test]
    fn io_错误可被问号自动转换() {
        fn read_missing() -> Result<String, AppError> {
            let content = std::fs::read_to_string("绝对不存在的文件.txt")?;
            Ok(content)
        }
        let err = read_missing().unwrap_err();
        assert_eq!(err.status(), 500);
        assert_eq!(err.code(), "internal");
    }
}
