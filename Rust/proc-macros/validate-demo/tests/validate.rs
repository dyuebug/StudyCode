// 集成测试：只通过 pub API 验证宏生成的行为。
// 这些断言全部针对「宏做的事」，所以宏写错了这里必红。

use validate_demo::{Article, Ping, User};

fn user(name: &str, age: u8) -> User {
    User {
        name: name.to_string(),
        age,
        email: "a@b.c".to_string(),
    }
}

#[test]
fn 合法值返回_ok() {
    assert!(user("张三丰", 1).validate().is_ok());
}

#[test]
fn 边界值算通过_闭区间() {
    // 长度下限 3 / 上限 20，数值 1 / 150 —— 两端都该通过
    assert!(user("abc", 1).validate().is_ok());
    assert!(user(&"a".repeat(20), 150).validate().is_ok());
}

#[test]
fn 中文按字符计数而不是字节() {
    // "张三" = 2 chars / 6 bytes。若宏误用 len()，6 >= 3 会通过 —— 这条断言就是防这个
    let errs = user("张三", 30).validate().unwrap_err();
    assert_eq!(errs.len(), 1, "只有 name 长度一项不合法，实际：{errs:?}");
    assert!(
        errs[0].contains("长度 2"),
        "错误消息应报字符数 2，实际：{}",
        errs[0]
    );

    // "张三丰" = 3 chars，等于下限，必须通过
    assert!(user("张三丰", 30).validate().is_ok());
}

#[test]
fn 一次返回全部错误而不是第一个() {
    // 空名字（< 3）+ age=0（< 1）→ 两项都要报
    let errs = user("", 0).validate().unwrap_err();
    assert_eq!(errs.len(), 2, "应同时报 name 与 age，实际：{errs:?}");
    assert!(errs.iter().any(|e| e.starts_with("name:")));
    assert!(errs.iter().any(|e| e.starts_with("age:")));
}

#[test]
fn 只写下限时上限不生效() {
    let article = Article {
        title: "t".to_string(),
        body: "a".repeat(10), // 正好等于下限
    };
    assert!(article.validate().is_ok());

    let too_short = Article {
        title: "t".to_string(),
        body: "短文".to_string(), // 2 chars < 10
    };
    let errs = too_short.validate().unwrap_err();
    assert_eq!(errs.len(), 1);
    assert!(errs[0].starts_with("body:"), "实际：{}", errs[0]);
}

#[test]
fn 超上限也会报错() {
    let errs = user(&"a".repeat(21), 30).validate().unwrap_err();
    assert_eq!(errs.len(), 1);
    assert!(errs[0].contains("长度 21"), "实际：{}", errs[0]);

    let errs = user("abc", 151).validate().unwrap_err();
    assert_eq!(errs.len(), 1);
    assert!(errs[0].contains("151"), "实际：{}", errs[0]);
}

#[test]
fn 没有任何规则的结构体永远通过() {
    // 这是宏最容易写崩的路径：生成的函数体为空，errs 变量未使用
    assert!(Ping { id: 0 }.validate().is_ok());
}

#[test]
fn 无规则字段被完整忽略() {
    // email 没有 #[validate]，填什么都该通过
    let u = User {
        name: "abc".to_string(),
        age: 20,
        email: String::new(),
    };
    assert!(u.validate().is_ok());
}
