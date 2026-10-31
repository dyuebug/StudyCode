// ============================================
// task-manager 数据模型
// 日期：2026-11-01（第37天）
// ============================================

/*
【这一层只认数据，不认 HTTP】

models 里不该出现 `HttpResponse`、`StatusCode`、`web::Json` 这类东西。
理由很实际：一旦 models 依赖了 actix，
"把同一份逻辑搬去做 CLI 或 gRPC" 就必须改 models。
而 models 是**最不该动**的一层。

所以这里只有：
  - 数据库行的映射（`#[derive(sqlx::FromRow)]`）
  - 请求体的形状（`Deserialize`）
  - 响应体的形状（`Serialize`）
*/

use serde::{Deserialize, Serialize};

// --------------------------------------------
// 数据库实体
// --------------------------------------------

/// 项目
#[derive(Serialize, Debug, sqlx::FromRow)]
pub struct Project {
    pub id: i64,
    pub owner_id: i64,
    pub name: String,
    pub description: String,
    pub created_at: String,
}

/// 任务
///
/// 【`status` 为什么是 `String` 而不是枚举】
/// 数据库里用 `TEXT + CHECK` 约束。若在这里也用枚举，
/// 需要在 Rust 与 SQL 之间维护两套取值集合：
/// 加一个状态要改 CHECK、改枚举、还要补一个"字符串 → 枚举"的转换。
/// 第 37 天先按字符串传递，把校验交给数据库的 CHECK——
/// **单一事实来源**在数据库里。
///
/// 代价：应用层拿不到穷尽性检查（`match` 不会提醒你漏了某个状态）。
/// 等第 38 天实现"状态流转"时再引入枚举，那时才真正需要穷尽匹配。
#[derive(Serialize, Debug, sqlx::FromRow)]
pub struct Task {
    pub id: i64,
    pub project_id: i64,
    pub title: String,
    pub status: String,
    pub priority: i64,
    pub assignee_id: Option<i64>,
    pub due_at: Option<String>,
    pub created_at: String,
    pub updated_at: String,
}

// --------------------------------------------
// 请求体
// --------------------------------------------

/// 创建项目
#[derive(Deserialize, Debug)]
pub struct CreateProject {
    pub name: String,
    /// 省略时数据库中默认为空串
    #[serde(default)]
    pub description: String,
}

/// 创建任务
///
/// `project_id` 不在请求体里——它来自 URL 路径。
/// 这是 REST 的常规做法：**资源的位置由 URL 表达，不由 body 重复表达**。
/// 若两处都能指定，就必然出现"URL 说项目 1、body 说项目 2"的矛盾，
/// 而处理这个矛盾的代码永远比省下的那几个字符贵。
#[derive(Deserialize, Debug)]
pub struct CreateTask {
    pub title: String,
    /// 1=最高 4=最低，省略时数据库默认 2
    pub priority: Option<i64>,
    pub due_at: Option<String>,
}

// --------------------------------------------
// 响应体
// --------------------------------------------

/// 成功响应
#[derive(Serialize)]
pub struct ApiResponse<T> {
    pub success: bool,
    pub data: T,
}

impl<T> ApiResponse<T> {
    pub fn ok(data: T) -> Self {
        Self {
            success: true,
            data,
        }
    }
}

/// 错误响应
///
/// `code` 是给**程序**看的稳定标识，`message` 是给**人**看的。
/// 只给人看的消息会变（改文案、多语言），客户端不该去匹配它。
#[derive(Serialize)]
pub struct ErrorResponse {
    pub success: bool,
    pub code: &'static str,
    pub message: String,
}

impl ErrorResponse {
    pub fn new(code: &'static str, message: impl Into<String>) -> Self {
        Self {
            success: false,
            code,
            message: message.into(),
        }
    }
}

// --------------------------------------------
// 第 38 天：状态枚举
// --------------------------------------------

/// 任务状态
///
/// 【为什么第 38 天要从 `String` 换成枚举】
/// 第 37 天只"存储"状态，字符串够用。
/// 第 38 天要"判断迁移是否合法"，就必须 `match (当前, 目标)`
/// ——这时枚举的**穷尽性检查**才有价值：以后数据库 CHECK 里加一个
/// `blocked`，`can_transition_to` 的 `match` 不会提醒你，
/// 但 `TaskStatus::parse` 会返回 `None`，非法值立刻暴露成 400 而不是被当作合法状态。
///
/// 数据库仍是唯一事实来源：枚举只是它的 Rust 镜像，
/// `CHECK (status IN (...))` 决定了能出现哪些值。
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TaskStatus {
    Todo,
    Doing,
    Done,
}

impl TaskStatus {
    pub const fn as_str(self) -> &'static str {
        match self {
            Self::Todo => "todo",
            Self::Doing => "doing",
            Self::Done => "done",
        }
    }

    /// 字符串 → 枚举。未知值返回 `None`（由调用方决定给 400 还是 500）
    pub fn parse(s: &str) -> Option<Self> {
        match s {
            "todo" => Some(Self::Todo),
            "doing" => Some(Self::Doing),
            "done" => Some(Self::Done),
            _ => None,
        }
    }

    /// 迁移是否合法
    ///
    /// 允许的路径只有两条：`todo → doing → done`。
    /// 同值视为幂等（重复 `PATCH` 不该报错）。
    ///
    /// 【为什么不用 `PUT` 直接改 status】
    /// `PUT /tasks/{id}` 允许客户端写任意字段任意值，
    /// 于是 `todo → done` 这种"跳过进行中"的跃迁就能悄悄发生。
    /// 状态机必须有一个**唯一的入口**，否则"任务必须先开始才能完成"
    /// 这条业务规则就无法保证——而且它不会报错，只会产生脏数据。
    pub const fn can_transition_to(self, next: Self) -> bool {
        matches!(
            (self, next),
            (Self::Todo, Self::Todo)
                | (Self::Todo, Self::Doing)
                | (Self::Doing, Self::Doing)
                | (Self::Doing, Self::Done)
                | (Self::Done, Self::Done)
        )
    }
}

// --------------------------------------------
// 第 38 天：请求体
// --------------------------------------------

/// `PATCH` 请求体
///
/// 【`Option<Option<T>>` 的三态】
///   - 字段不出现          → `None`          → **不改**
///   - 字段出现且为 `null` → `Some(None)`    → **改成空**
///   - 字段出现且有值      → `Some(Some(v))` → **改成 v**
///
/// ⚠️ serde 默认**做不到**这件事：对 `Option<Option<i64>>` 写 `null` 时，
/// 它会走"外层 Option 的 none"，得到 `None`，与"字段没出现"完全一样。
/// 所以必须加 `deserialize_with = "deserialize_some"` 强制包装一层。
/// 这是 `PATCH` 语义里最经典的一个坑：不加这行，
/// "把截止时间清空"这个功能**永远无效**，而且不报错。
#[derive(Deserialize, Debug)]
pub struct PatchTask {
    pub title: Option<String>,
    pub priority: Option<i64>,
    #[serde(default, deserialize_with = "deserialize_some")]
    pub assignee_id: Option<Option<i64>>,
    #[serde(default, deserialize_with = "deserialize_some")]
    pub due_at: Option<Option<String>>,
}

/// 把"字段出现了"这件事保住：先按内层类型反序列化，再套一层 `Some`
fn deserialize_some<'de, T, D>(de: D) -> Result<Option<T>, D::Error>
where
    T: Deserialize<'de>,
    D: serde::Deserializer<'de>,
{
    T::deserialize(de).map(Some)
}

/// 状态流转请求体
#[derive(Deserialize, Debug)]
pub struct ChangeStatus {
    pub status: String,
}

/// 贴标签请求体
#[derive(Deserialize, Debug)]
pub struct TagBody {
    pub name: String,
}

// --------------------------------------------
// 第 38 天：响应体
// --------------------------------------------

/// 任务详情（带标签）
#[derive(Serialize)]
pub struct TaskWithTags {
    #[serde(flatten)]
    pub task: Task,
    pub tags: Vec<String>,
}
