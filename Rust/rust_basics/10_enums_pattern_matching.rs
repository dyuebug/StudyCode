// ============================================
// 枚举与模式匹配 - 超详细注释版
// 日期：2026-06-07（第9天）
// 难度：⭐⭐ 中级
// ============================================

/*
【核心概念】枚举与模式匹配

是什么？
- 枚举（enum）：定义一个类型，它可以是几种可能值之一
- 模式匹配（match）：根据值的形状执行不同代码

为什么需要？
- 枚举：表达"非此即彼"的数据，如状态机、错误类型
- match：穷举处理所有可能的情况，编译器保证不遗漏

核心特点：
- Rust 的枚举变体可以携带数据
- match 必须穷举所有情况，用 _ 匹配剩余
- Option<T> 和 Result<T,E> 是最重要的两个枚举

Rust特色：
- 没有 null！用 Option<T> 表示"可能没有值"
- 用 Result<T,E> 表示"可能失败的操作"
- match 是表达式，可以返回值
*/

// ============================================
// 枚举定义
// ============================================

// 简单枚举：没有数据
#[derive(Debug)]
enum Direction {
    North,
    South,
    East,
    West,
}

// 枚举变体可以携带不同类型的数据
#[derive(Debug)]
enum Message {
    Quit,                        // 无数据
    Move { x: i32, y: i32 },    // 命名字段（类似结构体）
    Write(String),               // 单个值
    ChangeColor(u8, u8, u8),     // 多个值（类似元组）
}

impl Message {
    fn process(&self) {
        match self {
            Message::Quit => println!("退出"),
            Message::Move { x, y } => println!("移动到 ({}, {})", x, y),
            Message::Write(text) => println!("写入: {}", text),
            Message::ChangeColor(r, g, b) => println!("颜色: ({},{},{})", r, g, b),
        }
    }
}

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

fn main() {
    println!("============================================================");
    println!("第一部分：枚举基础");
    println!("============================================================");

    // --------------------------------------------
    // 1.1 基本枚举使用
    // --------------------------------------------

    let dir = Direction::North;
    println!("方向: {:?}", dir);

    // match 匹配枚举
    let description = match dir {
        Direction::North => "向北",
        Direction::South => "向南",
        Direction::East  => "向东",
        Direction::West  => "向西",
    };
    println!("{}", description);

    // --------------------------------------------
    // 1.2 携带数据的枚举
    // --------------------------------------------

    let msgs = vec![
        Message::Move { x: 10, y: 20 },
        Message::Write(String::from("hello")),
        Message::ChangeColor(255, 128, 0),
        Message::Quit,
    ];

    for msg in &msgs {
        msg.process();
    }

    println!("\n============================================================");
    println!("第二部分：Option<T> —— 替代 null");
    println!("============================================================");

    // Option<T> 是标准库内置枚举：
    //   enum Option<T> {
    //       Some(T),  // 有值
    //       None,     // 没有值
    //   }

    // --------------------------------------------
    // 2.1 Option 基本用法
    // --------------------------------------------

    let some_number: Option<i32> = Some(42);
    let no_number: Option<i32> = None;

    // 用 match 处理 Option
    match some_number {
        Some(n) => println!("有值: {}", n),
        None    => println!("没有值"),
    }

    // --------------------------------------------
    // 2.2 if let —— 只关心一种情况时的简写
    // --------------------------------------------

    // if let 比 match 更简洁，适合只处理一个分支的情况
    if let Some(n) = some_number {
        println!("if let 获取到值: {}", n);
    }

    // --------------------------------------------
    // 2.3 Option 常用方法
    // --------------------------------------------

    let value = some_number.unwrap_or(0);        // 有值返回值，None 返回默认值
    let doubled = some_number.map(|n| n * 2);    // 对 Some 内部值做变换
    let is_some = some_number.is_some();          // 是否有值

    println!("unwrap_or: {}", value);
    println!("map doubled: {:?}", doubled);
    println!("is_some: {}", is_some);
    println!("no_number.unwrap_or(0): {}", no_number.unwrap_or(0));

    println!("\n============================================================");
    println!("第三部分：match 高级用法");
    println!("============================================================");

    // --------------------------------------------
    // 3.1 match 守卫（额外条件）
    // --------------------------------------------

    let num = 7;
    let description = match num {
        n if n < 0  => "负数",
        0           => "零",
        n if n < 10 => "个位数",
        _           => "大数",   // _ 匹配剩余所有情况
    };
    println!("{} 是{}", num, description);

    // --------------------------------------------
    // 3.2 match 绑定值 @
    // --------------------------------------------

    let n = 15;
    match n {
        x @ 1..=10  => println!("1到10之间: {}", x),
        x @ 11..=20 => println!("11到20之间: {}", x),
        x           => println!("其他: {}", x),
    }

    // --------------------------------------------
    // 3.3 解构元组
    // --------------------------------------------

    let point = (0, -2);
    match point {
        (0, 0) => println!("原点"),
        (x, 0) => println!("在x轴上: {}", x),
        (0, y) => println!("在y轴上: {}", y),
        (x, y) => println!("在({}, {})", x, y),
    }

    println!("\n============================================================");
    println!("第四部分：while let");
    println!("============================================================");

    // while let：只要模式匹配就持续循环
    let mut stack = vec![1, 2, 3];
    while let Some(top) = stack.pop() {
        println!("弹出: {}", top); // 3, 2, 1
    }
}

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

/*
❌ 错误1：match 没有穷举所有情况
```rust
let dir = Direction::North;
match dir {
    Direction::North => println!("北"),
    Direction::South => println!("南"),
    // 错误！缺少 East 和 West
}
```
编译器错误：non-exhaustive patterns: `East` and `West` not covered
✅ 正确：加上剩余分支，或用 _ 通配
```rust
match dir {
    Direction::North => println!("北"),
    _ => println!("其他方向"),
}
```

────────────────────────────────────

❌ 错误2：直接 unwrap() None 导致 panic
```rust
let x: Option<i32> = None;
let v = x.unwrap(); // 运行时 panic！
```
✅ 正确：用 unwrap_or、if let、match 安全处理
```rust
let v = x.unwrap_or(0);
```

────────────────────────────────────

❌ 错误3：match 分支返回类型不一致
```rust
let v = match some_number {
    Some(n) => n,
    None    => "没有值", // 错误！一个是 i32，一个是 &str
};
```
编译器错误：match arms have incompatible types
✅ 正确：所有分支返回相同类型

────────────────────────────────────

❌ 错误4：枚举变体当函数名使用
```rust
enum Color { Red, Green, Blue }
let c = Red; // 错误！必须加路径
```
编译器错误：cannot find value `Red` in this scope
✅ 正确：let c = Color::Red;

────────────────────────────────────

❌ 错误5：解构枚举时字段名写错
```rust
match msg {
    Message::Move { a, b } => ... // 错误！字段名是 x, y
}
```
编译器错误：struct `Move` does not have a field named `a`
✅ 正确：使用正确的字段名 Message::Move { x, y }
*/

// ============================================
// 第五部分：卡片速查
// ============================================

/*
【枚举定义】

enum 名称 {
    变体1,                         // 无数据
    变体2(类型),                   // 元组式
    变体3 { 字段: 类型 },          // 结构体式
}

【match 语法】

match 值 {
    模式1 => 表达式,
    模式2 if 条件 => 表达式,       // 带守卫
    变量 @ 范围 => 表达式,         // 绑定并匹配范围
    _ => 表达式,                   // 通配
}

【Option<T> 常用方法】

.is_some() / .is_none()           — 判断
.unwrap()                         — 取值（None 时 panic）
.unwrap_or(默认值)                — 安全取值
.map(|x| ...)                     — 变换内部值
.and_then(|x| ...)                — 链式 Option 操作
.as_ref()                         — Option<T> -> Option<&T>

【if let 语法】

if let 模式 = 表达式 {
    // 匹配时执行
} else {
    // 不匹配时执行
}
*/

// ============================================
// 第六部分：练习题
// ============================================

/*
1. 实现 Traffic Light（交通灯）枚举
   要求：Red/Yellow/Green 三个变体
         实现 duration() 方法返回每种灯的持续秒数
         用 match 实现

2. 安全除法
   要求：实现 safe_divide(a: f64, b: f64) -> Option<f64>
         b 为 0 时返回 None，否则返回 Some(a/b)

3. 解析命令
   要求：定义枚举 Command { Quit, Move(i32, i32), Print(String) }
         实现 parse(input: &str) -> Option<Command>
         "quit" -> Some(Quit), "move 3 4" -> Some(Move(3,4))
   提示：用 .split_whitespace() 分割字符串

4. 链式 Option 操作
   要求：给定 Option<String>，
         用 .map() 转为大写，再用 .filter() 只保留长度>3的，
         最后用 .unwrap_or() 提供默认值
   提示：链式调用

5. 挑战：实现简单计算器
   要求：定义 Expr 枚举 { Num(f64), Add(Box<Expr>, Box<Expr>), Mul(Box<Expr>, Box<Expr>) }
         实现 eval(expr: &Expr) -> f64 用递归 match 计算结果
   提示：Box<T> 用于在枚举中存储递归类型
*/
