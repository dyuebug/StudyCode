// ============================================
// 结构体（Struct）- 超详细注释版
// 日期：2026-06-07（第9天）
// 难度：⭐⭐ 中级
// ============================================

/*
【核心概念】结构体

是什么？
- 结构体是自定义的数据类型，将多个相关字段组合在一起
- 类似其他语言的 class，但没有继承

为什么需要？
- 将相关数据组织在一起
- 为数据添加方法（行为）
- 创建领域概念的抽象

核心特点：
- 三种结构体：命名字段、元组结构体、单元结构体
- 方法通过 impl 块定义
- 关联函数（如 new）不接受 self 参数

Rust特色：
- 结构体默认不可打印，需派生 Debug trait
- 方法中 self 的所有权规则与普通变量相同
- 没有 null，用 Option<T> 表示可选字段
*/

// 派生 Debug trait，允许用 {:?} 打印
#[derive(Debug)]
struct User {
    username: String,
    email: String,
    age: u32,
    active: bool,
}

// 元组结构体：字段没有名字，用索引访问
#[derive(Debug)]
struct Point(f64, f64);

#[derive(Debug)]
struct Color(u8, u8, u8);

// ============================================
// impl 块：为结构体添加方法
// ============================================

impl User {
    // 关联函数（构造函数）：没有 self 参数
    // 调用方式：User::new(...)
    fn new(username: &str, email: &str, age: u32) -> User {
        User {
            username: String::from(username),
            email: String::from(email),
            age,          // 字段名与变量名相同时可以简写
            active: true,
        }
    }

    // 方法：第一个参数是 &self（不可变借用）
    // 只读，不修改结构体
    fn describe(&self) -> String {
        format!("用户: {}, 邮箱: {}, 年龄: {}", self.username, self.email, self.age)
    }

    // 方法：第一个参数是 &mut self（可变借用）
    // 可以修改结构体的字段
    fn deactivate(&mut self) {
        self.active = false;
    }

    // 方法：第一个参数是 self（获取所有权）
    // 调用后原变量失效（很少用）
    fn into_username(self) -> String {
        self.username // 消耗 self，返回 username
    }
}

impl Point {
    fn new(x: f64, y: f64) -> Point {
        Point(x, y)
    }

    // 计算到原点的距离
    fn distance_from_origin(&self) -> f64 {
        // self.0 和 self.1 是元组结构体的字段
        (self.0 * self.0 + self.1 * self.1).sqrt()
    }
}

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

fn main() {
    println!("============================================================");
    println!("第一部分：创建和使用结构体");
    println!("============================================================");

    // --------------------------------------------
    // 1.1 创建结构体实例
    // --------------------------------------------

    let mut user1 = User {
        username: String::from("Alice"),
        email: String::from("alice@example.com"),
        age: 25,
        active: true,
    };

    // 访问字段
    println!("用户名: {}", user1.username);
    println!("活跃: {}", user1.active);

    // 修改字段（结构体实例必须是 mut）
    user1.email = String::from("new@example.com");
    println!("新邮箱: {}", user1.email);

    // --------------------------------------------
    // 1.2 使用构造函数
    // --------------------------------------------

    let user2 = User::new("Bob", "bob@example.com", 30);
    println!("{}", user2.describe());

    // --------------------------------------------
    // 1.3 结构体更新语法
    // --------------------------------------------

    // 基于 user1 创建 user3，只修改部分字段
    // ..user1 表示其余字段从 user1 复制
    // 注意：如果复制的字段包含 String，user1 会被移动（部分移动）
    let user3 = User {
        email: String::from("user3@example.com"),
        username: String::from("Charlie"),
        ..user2  // 其余字段（age, active）从 user2 复制
    };
    println!("{:?}", user3);

    println!("\n============================================================");
    println!("第二部分：元组结构体");
    println!("============================================================");

    let p = Point::new(3.0, 4.0);
    println!("点: ({}, {})", p.0, p.1);
    println!("到原点距离: {:.2}", p.distance_from_origin()); // 5.00

    let black = Color(0, 0, 0);
    let red = Color(255, 0, 0);
    println!("黑色: {:?}", black);
    println!("红色: {:?}", red);

    println!("\n============================================================");
    println!("第三部分：方法调用");
    println!("============================================================");

    let mut user4 = User::new("Dave", "dave@example.com", 28);
    println!("活跃: {}", user4.active);

    user4.deactivate(); // &mut self 方法
    println!("停用后: {}", user4.active);

    let name = user4.into_username(); // self 方法，user4 被移动
    println!("用户名: {}", name);
    // user4 不能再使用了

    println!("\n============================================================");
    println!("第四部分：打印结构体");
    println!("============================================================");

    let user5 = User::new("Eve", "eve@example.com", 22);

    // {:?} 需要 #[derive(Debug)]
    println!("{:?}", user5);

    // {:#?} 是格式化的多行打印，更易读
    println!("{:#?}", user5);
}

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

/*
❌ 错误1：忘记 #[derive(Debug)] 就使用 {:?}
```rust
struct Foo { x: i32 }
let f = Foo { x: 1 };
println!("{:?}", f); // 错误！
```
编译器错误：`Foo` doesn't implement `Debug`
✅ 正确：在结构体前加 #[derive(Debug)]

────────────────────────────────────

❌ 错误2：修改不可变结构体的字段
```rust
let user = User::new("Alice", "a@b.com", 20);
user.active = false; // 错误！user 不是 mut
```
编译器错误：cannot assign to `user.active`, as `user` is not declared as mutable
✅ 正确：let mut user = User::new(...)

────────────────────────────────────

❌ 错误3：结构体更新语法导致部分移动
```rust
let u1 = User::new("Alice", "a@b.com", 20);
let u2 = User { email: String::from("b@c.com"), ..u1 };
println!("{}", u1.username); // 错误！username 已被移动到 u2
```
编译器错误：value borrowed here after partial move
✅ 正确：移动后不再使用被移动的字段，或者使用 .clone()

────────────────────────────────────

❌ 错误4：方法中用 self 而不是 &self 导致所有权转移
```rust
impl User {
    fn get_name(self) -> String { // self 而非 &self
        self.username
    }
}
let user = User::new("Alice", "a@b.com", 20);
println!("{}", user.get_name());
println!("{}", user.active); // 错误！user 已被移动
```
✅ 正确：只读方法用 &self，修改方法用 &mut self

────────────────────────────────────

❌ 错误5：忘记在 impl 块外定义结构体
```rust
impl MyStruct { // 错误！MyStruct 未定义
    fn new() -> MyStruct { ... }
}
```
✅ 正确：先定义结构体，再写 impl 块
*/

// ============================================
// 第五部分：卡片速查
// ============================================

/*
【结构体定义】

struct 名称 {
    字段名: 类型,
    ...
}

【元组结构体】

struct 名称(类型1, 类型2);
访问：实例.0, 实例.1

【impl 块】

impl 结构体名 {
    fn new(...) -> 结构体名 { ... }     // 关联函数（构造器）
    fn method(&self) { ... }            // 不可变方法
    fn method_mut(&mut self) { ... }    // 可变方法
    fn consume(self) { ... }            // 消耗所有权
}

【常用派生 trait】

#[derive(Debug)]    — 允许 {:?} 打印
#[derive(Clone)]    — 允许 .clone()
#[derive(Copy)]     — 允许隐式复制（仅适用于栈类型）
#[derive(PartialEq)] — 允许 == 比较
*/

// ============================================
// 第六部分：练习题
// ============================================

/*
1. 定义 Rectangle 结构体
   要求：字段 width: f64, height: f64
         实现方法：area()、perimeter()、is_square()
   提示：is_square 返回 bool

2. 实现 Stack<i32> 结构体
   要求：内部用 Vec 存储，实现 push、pop、peek、is_empty
   提示：peek 返回 Option<&i32>

3. 为 Point 实现 distance_to(&self, other: &Point) -> f64
   要求：计算两点之间的距离
   提示：(x2-x1)² + (y2-y1)² 开平方

4. 结构体更新语法练习
   要求：定义 Config 结构体（有5个字段），
         用默认值创建实例，再用更新语法修改其中2个字段
   提示：可以实现 Config::default() 关联函数

5. 挑战：链式方法（Builder 模式）
   要求：为 User 实现 Builder，每个方法返回 Self
         例如：User::builder().name("Alice").age(20).build()
   提示：builder 方法返回 &mut Self 或 Self
*/
