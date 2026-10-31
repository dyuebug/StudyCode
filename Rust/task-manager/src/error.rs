// ============================================
// task-manager 统一错误层（第 39 天）
// 日期：2026-11-03
// ============================================

/*
【要解决的两个问题】

1. 形状不一致
   第 38 天之前，业务错误是 JSON（`{success, code, message}`），
   而数据库错误走 `ErrorInternalServerError` → 500 + 纯文本 body。
   客户端于是要写两套解析逻辑，这属于**把内部实现泄漏成接口契约**。

2. 内部信息泄漏
   `sqlx::Error` 的 `Display` 里带着 SQL 片段、表名、列名。
   把 `e` 直接塞进响应体，等于把数据模型公开给所有调用方。

【做法】
让 handler 统一返回 `Result<HttpResponse, ApiError>`，
`ApiError` 实现 `ResponseError`，于是：

    let rows = sqlx::query(...).fetch_all(pool).await?;   // ← `?` 直接转换

`?` 能工作的原因：`impl From<sqlx::Error> for ApiError`。
这就是"错误也该是类型系统的一部分"——而不是每个 handler 各自
`map_err(...)`，漏一处就少一层保护。
*/

use actix_web::http::StatusCode;
use actix_web::{HttpResponse, ResponseError};

use crate::models::ErrorResponse;

/// 接口错误
///
/// 只有 `Internal` 装了下层错误，其余变体都是**已经决定好给客户端看**的信息。
/// 这个区分是有意的：内部错误不能原样外传。
#[derive(Debug)]
pub enum ApiError {
    NotFound(&'static str),
    BadRequest {
        code: &'static str,
        message: String,
    },
    Conflict {
        code: &'static str,
        message: String,
    },
    /// 只有数据库错误会走到这里
    Internal(sqlx::Error),
}

impl ApiError {
    pub fn bad_request(code: &'static str, message: impl Into<String>) -> Self {
        Self::BadRequest {
            code,
            message: message.into(),
        }
    }

    pub fn conflict(code: &'static str, message: impl Into<String>) -> Self {
        Self::Conflict {
            code,
            message: message.into(),
        }
    }
}

impl std::fmt::Display for ApiError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::NotFound(what) => write!(f, "{what} 不存在"),
            Self::BadRequest { code, message } => write!(f, "400 {code}: {message}"),
            Self::Conflict { code, message } => write!(f, "409 {code}: {message}"),
            // 注意：`Display` 会出现在服务端日志里，所以这里要带上下层错误——
            // 日志是给运维看的，响应体才是给客户端看的，两者**不是同一份文案**。
            Self::Internal(e) => write!(f, "500 internal: {e}"),
        }
    }
}

impl ResponseError for ApiError {
    fn status_code(&self) -> StatusCode {
        match self {
            Self::NotFound(_) => StatusCode::NOT_FOUND,
            Self::BadRequest { .. } => StatusCode::BAD_REQUEST,
            Self::Conflict { .. } => StatusCode::CONFLICT,
            Self::Internal(_) => StatusCode::INTERNAL_SERVER_ERROR,
        }
    }

    fn error_response(&self) -> HttpResponse {
        match self {
            Self::NotFound(what) => HttpResponse::NotFound()
                .json(ErrorResponse::new("not_found", format!("{what}不存在"))),
            Self::BadRequest { code, message } => {
                HttpResponse::BadRequest().json(ErrorResponse::new(code, message.clone()))
            }
            Self::Conflict { code, message } => {
                HttpResponse::Conflict().json(ErrorResponse::new(code, message.clone()))
            }
            Self::Internal(e) => {
                // 真实原因只进日志。响应体固定文案：
                // 客户端拿 SQL 片段没有任何用处，攻击者拿它有用。
                println!("[ERR] 内部错误: {e}");
                HttpResponse::InternalServerError().json(ErrorResponse::new(
                    "internal",
                    "服务器内部错误，请稍后重试或联系管理员",
                ))
            }
        }
    }
}

/// 让 `?` 直接把数据库错误升级成 500
impl From<sqlx::Error> for ApiError {
    fn from(e: sqlx::Error) -> Self {
        Self::Internal(e)
    }
}
