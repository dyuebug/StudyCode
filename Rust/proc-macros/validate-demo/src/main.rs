use validate_demo::{format_errors, Article, Ping, User};

fn main() {
    println!("=== #[derive(Validate)] 演示 ===\n");

    let cases = [
        User {
            name: "张三".to_string(),
            age: 30,
            email: "a@b.c".to_string(),
        },
        User {
            name: "张三丰".to_string(),
            age: 30,
            email: "a@b.c".to_string(),
        },
        User {
            name: "".to_string(),
            age: 0,
            email: "".to_string(),
        },
        User {
            name: "x".repeat(25),
            age: 200,
            email: "".to_string(),
        },
    ];

    for (i, user) in cases.iter().enumerate() {
        let result = user.validate();
        println!("用例 {} · name={:?} age={}", i + 1, user.name, user.age);
        println!("  {}\n", format_errors(&result).replace('\n', "\n  "));
    }

    let article = Article {
        title: "过程宏".to_string(),
        body: "短".to_string(),
    };
    println!("Article 校验：{}", format_errors(&article.validate()));

    let ping = Ping { id: 1 };
    println!("无规则结构体：{}", format_errors(&ping.validate()));
}
