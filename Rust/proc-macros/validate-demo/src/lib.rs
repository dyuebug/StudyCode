// ============================================================================
//  过程宏消费者 · 被生成的代码长什么样
// ============================================================================
//  这个 crate 只做两件事：
//    1. 用 #[derive(Validate)] 标注结构体（让宏在编译期展开）
//    2. 断言展开后的 `validate()` 行为正确
//
//  注意：`validate()` 方法在源码里**根本不存在**——它是宏生成的。
//  `cargo expand -p validate-demo` 能看到展开结果（本机未装，见 README）。
// ============================================================================

use validate_derive::Validate;

/// 最典型的表单对象：字符串长度 + 数值范围。
#[derive(Debug, Validate)]
pub struct User {
    /// `chars().count()` 计数，不是 `len()`：中文一个字算一个
    #[validate(min_len = 3, max_len = 20)]
    pub name: String,

    /// 数值范围，闭区间（等于边界算通过）
    #[validate(min = 1, max = 150)]
    pub age: u8,

    /// 没有 `#[validate]` 的字段：不应产生任何校验，
    /// 但必须能正常编译（证明宏不会对无规则字段乱生成代码）
    pub email: String,
}

/// 只有一个字段有规则，用来确认「部分字段带规则」也成立。
#[derive(Debug, Validate)]
pub struct Article {
    #[validate(min_len = 1, max_len = 80)]
    pub title: String,

    /// 只有下限，没有上限
    #[validate(min_len = 10)]
    pub body: String,
}

/// 完全没有规则的结构体：`validate()` 永远返回 `Ok(())`。
/// 这是宏最容易写崩的情况（生成的函数体为空 → 未使用变量警告）。
#[derive(Debug, Validate)]
pub struct Ping {
    pub id: u32,
}

/// 演示：把 `Result` 转成面向用户的文本。
pub fn format_errors(result: &Result<(), Vec<String>>) -> String {
    match result {
        Ok(()) => "校验通过".to_string(),
        Err(errs) => format!("{} 项不合法：\n  - {}", errs.len(), errs.join("\n  - ")),
    }
}
