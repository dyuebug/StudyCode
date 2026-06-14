// ============================================
// 泛型与 Trait - 超详细注释版
// 日期：2026-06-13（第14天）
// 难度：⭐⭐⭐ 重要
// ============================================

/*
【核心概念】泛型与 Trait

是什么？
- 泛型（Generics）：编写可以处理多种类型的代码，用 <T> 表示"任意类型"
- Trait：定义共享行为的接口，类似其他语言的接口，但更强大

为什么需要？
- 泛型：避免为每种类型重复写相同逻辑的代码（如 largest_i32、largest_f64...）
- Trait：定义类型必须实现的行为，实现多态和代码复用

核心特点？
- 泛型在编译期单态化（monomorphization）：泛型代码被编译成具体类型的代码，零运行时开销
- Trait 可以有默认方法实现
- Trait 约束（trait bound）：限制泛型必须实现某些 trait

Rust特色？
- 零成本抽象：泛型代码和手写具体类型的代码性能完全一样
- Trait 比传统接口更灵活：可以为外部类型实现 trait（孤儿规则有限制）
- 编译期保证：所有 trait 约束在编译期检查，没有运行时反射开销
*/

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

fn part1_concepts() {
    println!("============================================================");
    println!("第一部分：核心概念详解");
    println!("============================================================");

    // --------------------------------------------
    // 1.1 什么是泛型？
    // --------------------------------------------

    println!("\n1.1 什么是泛型？");
    println!("------------------------------------------------------------");

    /*
    【泛型 Generics】

    是什么？
    - 用占位符（如 <T>）代表"任意类型"
    - 让函数/结构体/枚举可以处理多种类型
    - T 是 Type 的缩写，也可以用其他名字（E、K、V 等）

    为什么需要？
    - 避免代码重复：不需要为每种类型写一遍相同的逻辑
    - 类型安全：编译器会检查类型是否匹配

    示例场景：
    - 不用泛型：largest_i32(list: &[i32])、largest_f64(list: &[f64])...
    - 用泛型：largest<T>(list: &[T]) 一次搞定所有类型

    Rust特色：
    - 编译期单态化：泛型代码编译时被展开成具体类型的代码
    - 零运行时开销：泛型版本和手写版本性能完全一样
    */

    println!("泛型让我们用 <T> 写一次代码，处理多种类型");

    // --------------------------------------------
    // 1.2 什么是 Trait？
    // --------------------------------------------

    println!("\n1.2 什么是 Trait？");
    println!("------------------------------------------------------------");

    /*
    【Trait】

    是什么？
    - 定义共享行为的接口
    - 类似 Java/C# 的接口，但更强大
    - 定义"类型能做什么"而不是"类型是什么"

    为什么需要？
    - 多态：不同类型可以实现相同的 trait，以统一的方式使用
    - 代码复用：trait 可以有默认方法实现
    - 约束泛型：用 trait 约束泛型必须实现某些行为

    核心特点：
    - 可以有默认方法实现（接口通常不行）
    - 可以为外部类型实现 trait（有孤儿规则限制）
    - 作为 trait 约束使用：<T: Display> 表示 T 必须实现 Display

    Rust特色：
    - Trait 是 Rust 实现多态的主要方式（没有传统的类继承）
    - 编译期静态分发：没有虚函数表的运行时开销
    - Trait 对象（dyn Trait）支持动态分发（有开销，但更灵活）
    */

    println!("Trait 定义类型的'能力'，是 Rust 实现多态的核心机制");

    // --------------------------------------------
    // 1.3 泛型与 Trait 的关系
    // --------------------------------------------

    println!("\n1.3 泛型与 Trait 的关系");
    println!("------------------------------------------------------------");

    /*
    【泛型 + Trait = 灵活且安全的代码】

    单独使用泛型的问题：
    - fn largest<T>(list: &[T]) -> T { ... }
    - 问题：T 是任意类型，可能没有比较大小的能力（如何比较？）
    - 编译器会报错：T 没有实现 PartialOrd trait

    解决方案：Trait 约束（Trait Bound）
    - fn largest<T: PartialOrd>(list: &[T]) -> T { ... }
    - 限制：T 必须实现 PartialOrd trait（可比较大小）
    - 效果：编译器知道 T 可以用 > < 比较，代码通过编译

    核心思想：
    - 泛型提供"类型抽象"能力
    - Trait 约束提供"能力保证"
    - 两者结合 = 既灵活又安全

    常见 Trait 约束：
    - T: Display        → T 可以被打印
    - T: Clone          → T 可以被克隆
    - T: PartialOrd     → T 可以比较大小
    - T: Debug          → T 可以用 {:?} 调试打印
    - T: Display + Clone → T 必须同时实现两个 trait（用 + 连接）
    */

    println!("Trait 约束让泛型既灵活又安全：<T: PartialOrd> 保证 T 可以比较");
}

// ============================================
// 第二部分：基础操作实践
// ============================================

fn part2_basic_usage() {
    println!("\n============================================================");
    println!("第二部分：基础操作实践");
    println!("============================================================");

    // --------------------------------------------
    // 2.1 泛型函数
    // --------------------------------------------

    println!("\n2.1 泛型函数");
    println!("------------------------------------------------------------");

    // 示例1：不用泛型的重复代码
    fn largest_i32(list: &[i32]) -> i32 {
        let mut largest = list[0]; // 假设第一个是最大
        for &item in list {
            if item > largest {
                largest = item;
            }
        }
        largest
    }

    fn largest_char(list: &[char]) -> char {
        let mut largest = list[0];
        for &item in list {
            if item > largest {
                largest = item;
            }
        }
        largest
    }

    let numbers = vec![34, 50, 25, 100, 65];
    println!("最大的 i32: {}", largest_i32(&numbers));

    let chars = vec!['y', 'm', 'a', 'q'];
    println!("最大的 char: {}", largest_char(&chars));

    println!("→ 问题：两个函数逻辑完全相同，只是类型不同！");

    // 示例2：用泛型消除重复
    // 注意：需要 trait 约束 PartialOrd（可比较）+ Copy（可复制）
    fn largest<T: PartialOrd + Copy>(list: &[T]) -> T {
        // T: PartialOrd → T 必须可以用 > < 比较
        // T: Copy → T 必须可以 Copy（否则 list[0] 会移动所有权）
        let mut largest = list[0]; // Copy：复制第一个元素
        for &item in list { // &item：借用每个元素，item 是 T 类型
            if item > largest { // PartialOrd：可以用 > 比较
                largest = item; // Copy：可以赋值
            }
        }
        largest
    }

    println!("\n→ 用泛型一次搞定所有类型：");
    println!("最大的 i32: {}", largest(&numbers)); // T = i32
    println!("最大的 char: {}", largest(&chars));  // T = char

    // --------------------------------------------
    // 2.2 泛型结构体
    // --------------------------------------------

    println!("\n2.2 泛型结构体");
    println!("------------------------------------------------------------");

    // 示例1：单个泛型参数
    #[derive(Debug)] // 让 Point 可以用 {:?} 打印
    struct Point<T> {
        // T 是泛型参数，x 和 y 必须是相同类型
        x: T,
        y: T,
    }

    let integer_point = Point { x: 5, y: 10 }; // T = i32
    let float_point = Point { x: 1.0, y: 4.0 }; // T = f64
    println!("整数点：{:?}", integer_point);
    println!("浮点点：{:?}", float_point);

    // 错误示例：x 和 y 类型不同
    // let mixed_point = Point { x: 5, y: 4.0 }; // ❌ 编译错误：x 是 i32，y 是 f64

    // 示例2：多个泛型参数
    #[derive(Debug)]
    struct Point2<T, U> {
        // T 和 U 可以是不同类型
        x: T,
        y: U,
    }

    let mixed_point = Point2 { x: 5, y: 4.0 }; // T = i32, U = f64
    println!("混合类型点：{:?}", mixed_point);

    println!("→ 单个 T：x 和 y 必须同类型；多个 T、U：可以不同类型");

    // --------------------------------------------
    // 2.3 泛型方法
    // --------------------------------------------

    println!("\n2.3 泛型方法");
    println!("------------------------------------------------------------");

    #[derive(Debug)]
    struct Point3<T> {
        x: T,
        y: T,
    }

    // impl 后面的 <T> 表示：这是针对泛型 Point3<T> 的实现
    impl<T> Point3<T> {
        // 方法1：返回 x 的引用
        fn x(&self) -> &T {
            // &self：借用 self
            // -> &T：返回 T 的引用（不移动所有权）
            &self.x
        }

        // 方法2：这个方法有类型问题（下面会用正确版本演示）
        // fn mixup<U>(self, other: Point3<U>) -> Point3<T> {
        //     Point3 {
        //         x: self.x,   // T 类型
        //         y: other.y,  // U 类型... 类型不匹配！
        //     }
        // }
    }

    // 正确版本：mixup 返回 Point3<T> 时，y 也必须是 T 类型
    // 所以应该返回一个新的泛型结构体 Point2<T, U>

    #[derive(Debug)]
    struct Point4<T, U> {
        x: T,
        y: U,
    }

    impl<T, U> Point4<T, U> {
        fn mixup<V, W>(self, other: Point4<V, W>) -> Point4<T, W> {
            // self.x: T 类型
            // other.y: W 类型
            // 返回 Point4<T, W>：x 来自第一个点，y 来自第二个点
            Point4 {
                x: self.x,
                y: other.y,
            }
        }
    }

    let p1 = Point4 { x: 5, y: 10.4 };      // T=i32, U=f64
    let p2 = Point4 { x: "Hello", y: 'c' }; // V=&str, W=char
    let p3 = p1.mixup(p2);                   // 返回 Point4<i32, char>
    println!("混合后的点：x = {}, y = {}", p3.x, p3.y);
    // 输出：x = 5, y = c

    println!("→ impl<T> 声明泛型参数，方法内可以继续使用新的泛型参数");

    // --------------------------------------------
    // 2.4 定义 Trait
    // --------------------------------------------

    println!("\n2.4 定义 Trait");
    println!("------------------------------------------------------------");

    // 定义一个 trait：Summary（摘要能力）
    trait Summary {
        // 方法签名：必须实现的方法
        fn summarize(&self) -> String;
        // &self：借用自己
        // -> String：返回摘要字符串
    }

    // 为 NewsArticle 实现 Summary trait
    struct NewsArticle {
        headline: String,
        location: String,
        author: String,
        content: String,
    }

    impl Summary for NewsArticle {
        // impl Trait for Type：为类型实现 trait
        fn summarize(&self) -> String {
            format!("{}, by {} ({})", self.headline, self.author, self.location)
        }
    }

    // 为 Tweet 实现 Summary trait
    struct Tweet {
        username: String,
        content: String,
        reply: bool,
        retweet: bool,
    }

    impl Summary for Tweet {
        fn summarize(&self) -> String {
            format!("{}: {}", self.username, self.content)
        }
    }

    let article = NewsArticle {
        headline: String::from("Penguins win the Stanley Cup Championship!"),
        location: String::from("Pittsburgh, PA, USA"),
        author: String::from("Iceburgh"),
        content: String::from("The Pittsburgh Penguins once again are the best..."),
    };

    let tweet = Tweet {
        username: String::from("horse_ebooks"),
        content: String::from("of course, as you probably already know, people"),
        reply: false,
        retweet: false,
    };

    println!("新闻摘要: {}", article.summarize());
    println!("推特摘要: {}", tweet.summarize());

    println!("→ 不同类型实现相同 trait，可以用统一方式调用 summarize()");

    // --------------------------------------------
    // 2.5 Trait 默认方法
    // --------------------------------------------

    println!("\n2.5 Trait 默认方法");
    println!("------------------------------------------------------------");

    // 定义带默认方法的 trait
    trait Summary2 {
        // 必须实现的方法
        fn summarize_author(&self) -> String;

        // 默认方法实现（可选覆盖）
        fn summarize(&self) -> String {
            format!("(Read more from {}...)", self.summarize_author())
        }
    }

    struct Tweet2 {
        username: String,
        content: String,
    }

    impl Summary2 for Tweet2 {
        // 只实现必须的方法
        fn summarize_author(&self) -> String {
            format!("@{}", self.username)
        }
        // summarize() 使用默认实现
    }

    struct Article2 {
        author: String,
        content: String,
    }

    impl Summary2 for Article2 {
        fn summarize_author(&self) -> String {
            self.author.clone()
        }

        // 覆盖默认实现
        fn summarize(&self) -> String {
            let len = self.content.len().min(50);
            format!("Article by {}: {}", self.author, &self.content[..len])
        }
    }

    let tweet = Tweet2 {
        username: String::from("horse_ebooks"),
        content: String::from("of course, as you probably already know, people"),
    };

    let article = Article2 {
        author: String::from("Alice"),
        content: String::from("Rust is a systems programming language..."),
    };

    println!("推特（默认）: {}", tweet.summarize());
    println!("文章（覆盖）: {}", article.summarize());

    println!("→ 默认方法可以调用同 trait 的其他方法，实现代码复用");

    // --------------------------------------------
    // 2.6 Trait 约束（Trait Bound）
    // --------------------------------------------

    println!("\n2.6 Trait 约束（Trait Bound）");
    println!("------------------------------------------------------------");

    // 语法1：在泛型参数上直接约束
    fn notify<T: Summary>(item: &T) {
        // T: Summary → T 必须实现 Summary trait
        println!("Breaking news! {}", item.summarize());
    }

    // 语法2：多个 trait 约束（用 + 连接）
    use std::fmt::Display;
    fn notify2<T: Summary + Display>(item: &T) {
        // T 必须同时实现 Summary 和 Display
        println!("Breaking news! {}", item.summarize());
    }

    // 语法3：where 子句（复杂约束时更清晰）
    fn some_function<T, U>(_t: &T, _u: &U)
    where
        T: Display + Clone,
        U: Clone + Summary,
    {
        // 等价于：fn some_function<T: Display + Clone, U: Clone + Summary>(...)
        // where 子句让约束更易读
        println!("在 where 子句中定义约束");
    }

    let tweet = Tweet {
        username: String::from("news_bot"),
        content: String::from("Rust 1.60 released!"),
        reply: false,
        retweet: false,
    };

    notify(&tweet); // tweet 实现了 Summary，可以传入

    println!("→ Trait 约束让泛型既灵活又安全，编译期保证类型能力");

    // --------------------------------------------
    // 2.7 返回实现了 Trait 的类型
    // --------------------------------------------

    println!("\n2.7 返回实现了 Trait 的类型");
    println!("------------------------------------------------------------");

    // 语法：impl Trait 作为返回类型
    fn returns_summarizable() -> impl Summary {
        // 返回"实现了 Summary 的某种类型"
        Tweet {
            username: String::from("bot"),
            content: String::from("Hello from function!"),
            reply: false,
            retweet: false,
        }
    }

    let item = returns_summarizable();
    println!("返回的对象: {}", item.summarize());

    // 限制：只能返回单一具体类型
    // 下面的代码无法编译：
    /*
    fn returns_summarizable_wrong(switch: bool) -> impl Summary {
        if switch {
            NewsArticle { ... } // ❌ 返回 NewsArticle
        } else {
            Tweet { ... }       // ❌ 返回 Tweet
        }
        // 编译错误：impl Trait 只能返回一种具体类型
        // 解决方案：使用 trait 对象 Box<dyn Summary>
    }
    */

    println!("→ impl Trait 适合返回单一具体类型，需要多种类型用 trait 对象");
}

// ============================================
// 第三部分：深入理解（可选）🔍
// ============================================

/*
==============================================
🔍 深入理解：零成本抽象与单态化（选学）
==============================================

如果你想知道"为什么泛型没有性能开销"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【零成本抽象（Zero-Cost Abstraction）】

核心原则：
"你不为你不使用的东西付费，使用的东西无法手写得更好"

泛型的编译过程：单态化（Monomorphization）
1. 编译器分析代码，找到泛型函数的所有调用
2. 为每个具体类型生成一份专门的代码
3. 泛型代码消失，只留下具体类型的代码

示例：
```rust
fn largest<T: PartialOrd + Copy>(list: &[T]) -> T { ... }

let numbers = vec![1, 2, 3];
let result = largest(&numbers); // T = i32

let chars = vec!['a', 'b', 'c'];
let result2 = largest(&chars); // T = char
```

编译后等价于：
```rust
fn largest_i32(list: &[i32]) -> i32 { ... }
fn largest_char(list: &[char]) -> char { ... }

let numbers = vec![1, 2, 3];
let result = largest_i32(&numbers);

let chars = vec!['a', 'b', 'c'];
let result2 = largest_char(&chars);
```

优点：
- 运行时性能和手写具体类型的代码完全一样
- 没有虚函数表查找开销
- 没有运行时类型检查开销

缺点：
- 编译时间更长（需要生成多份代码）
- 生成的二进制文件更大（代码膨胀）

==============================================
*/

/*
==============================================
🔍 深入理解：Trait 对象与动态分发（选学）
==============================================

静态分发 vs 动态分发

【静态分发（泛型 + trait 约束）】
```rust
fn notify<T: Summary>(item: &T) {
    println!("{}", item.summarize());
}
```
- 编译期确定具体类型
- 单态化：为每种类型生成一份代码
- 性能好（没有运行时开销）
- 但只能返回单一具体类型

【动态分发（trait 对象）】
```rust
fn notify(item: &dyn Summary) {
    println!("{}", item.summarize());
}
```
- dyn Summary：trait 对象，运行时确定类型
- 使用虚函数表（vtable）查找方法
- 灵活（可以存储不同类型）
- 有性能开销（虚函数调用）

使用场景：
- 静态分发：性能关键、类型数量少
- 动态分发：需要存储不同类型的集合（如 Vec<Box<dyn Summary>>）

==============================================
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

fn part4_common_errors() {
    println!("\n============================================================");
    println!("第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐");
    println!("============================================================");

    // --------------------------------------------
    // 4.1 错误1：泛型没有 trait 约束
    // --------------------------------------------

    println!("\n4.1 错误1：泛型没有 trait 约束");
    println!("------------------------------------------------------------");

    /*
    ❌ 错误代码：
    ```rust
    fn largest<T>(list: &[T]) -> T {
        let mut largest = list[0];
        for &item in list {
            if item > largest {  // ❌ 编译错误
                largest = item;
            }
        }
        largest
    }
    ```

    编译器错误信息：
    error[E0369]: binary operation `>` cannot be applied to type `T`
    help: consider restricting type parameter `T`: `T: std::cmp::PartialOrd`

    原因：
    - T 是任意类型，可能没有 > 比较能力
    - 编译器不知道 T 是否实现了 PartialOrd trait

    ✅ 正确代码：
    ```rust
    fn largest<T: PartialOrd + Copy>(list: &[T]) -> T {
        let mut largest = list[0];
        for &item in list {
            if item > largest {  // ✅ T 实现了 PartialOrd，可以比较
                largest = item;
            }
        }
        largest
    }
    ```

    记忆技巧：
    - 使用泛型操作时，想想"这个操作需要什么能力"
    - 比较大小 → PartialOrd
    - 复制值 → Copy
    - 打印 → Display 或 Debug
    */

    println!("→ 泛型使用操作前，必须用 trait 约束保证类型有这个能力");

    // --------------------------------------------
    // 4.2 错误2：结构体泛型参数数量不匹配
    // --------------------------------------------

    println!("\n4.2 错误2：结构体泛型参数数量不匹配");
    println!("------------------------------------------------------------");

    /*
    ❌ 错误代码：
    ```rust
    struct Point<T> {
        x: T,
        y: T,
    }

    let p = Point { x: 5, y: 4.0 }; // ❌ 编译错误
    ```

    编译器错误信息：
    error[E0308]: mismatched types
    expected integer, found floating-point number

    原因：
    - Point<T> 只有一个泛型参数 T
    - x 和 y 必须是相同类型
    - x 是 i32，y 是 f64，类型不匹配

    ✅ 正确代码：
    ```rust
    // 方案1：两个字段用相同类型
    let p1 = Point { x: 5, y: 10 };      // T = i32
    let p2 = Point { x: 1.0, y: 4.0 };   // T = f64

    // 方案2：使用多个泛型参数
    struct Point2<T, U> {
        x: T,
        y: U,
    }
    let p3 = Point2 { x: 5, y: 4.0 };    // T = i32, U = f64
    ```

    记忆技巧：
    - 单个 T：所有用到 T 的地方必须是相同类型
    - 需要不同类型？增加泛型参数 <T, U>
    */

    println!("→ 单个泛型参数要求相同类型，不同类型需要多个泛型参数");

    // --------------------------------------------
    // 4.3 错误3：忘记在 impl 块声明泛型参数
    // --------------------------------------------

    println!("\n4.3 错误3：忘记在 impl 块声明泛型参数");
    println!("------------------------------------------------------------");

    /*
    ❌ 错误代码：
    ```rust
    struct Point<T> {
        x: T,
        y: T,
    }

    impl Point<T> {  // ❌ 编译错误
        fn x(&self) -> &T {
            &self.x
        }
    }
    ```

    编译器错误信息：
    error[E0412]: cannot find type `T` in this scope
    help: you might be missing a type parameter: `impl<T>`

    原因：
    - impl 块不知道 T 是什么
    - 必须在 impl 后声明泛型参数

    ✅ 正确代码：
    ```rust
    impl<T> Point<T> {  // ✅ 声明 T 是泛型参数
        fn x(&self) -> &T {
            &self.x
        }
    }
    ```

    记忆技巧：
    - impl<T> → 声明泛型参数
    - Point<T> → 使用泛型参数
    - 两个 <T> 是配对的：声明 + 使用
    */

    println!("→ impl<T> 声明泛型参数，Point<T> 使用泛型参数");

    // --------------------------------------------
    // 4.4 错误4：为外部类型实现外部 trait（孤儿规则）
    // --------------------------------------------

    println!("\n4.4 错误4：为外部类型实现外部 trait（孤儿规则）");
    println!("------------------------------------------------------------");

    /*
    ❌ 错误代码：
    ```rust
    use std::fmt::Display;

    impl Display for Vec<i32> {  // ❌ 编译错误
        fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
            write!(f, "{:?}", self)
        }
    }
    ```

    编译器错误信息：
    error[E0117]: only traits defined in the current crate can be implemented for arbitrary types
    note: `Display` is not defined in the current crate

    原因：孤儿规则（Orphan Rule）
    - 为类型实现 trait 时，trait 或类型至少有一个在当前 crate 定义
    - Display 和 Vec 都是标准库的，不能在你的代码中实现
    - 防止不同 crate 对同一类型实现同一 trait 产生冲突

    ✅ 正确做法：
    ```rust
    // 方案1：定义自己的 trait
    trait MyDisplay {
        fn my_fmt(&self) -> String;
    }

    impl MyDisplay for Vec<i32> {  // ✅ 自己的 trait
        fn my_fmt(&self) -> String {
            format!("{:?}", self)
        }
    }

    // 方案2：定义自己的类型
    struct MyVec(Vec<i32>);

    impl Display for MyVec {  // ✅ 自己的类型
        fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
            write!(f, "{:?}", self.0)
        }
    }
    ```

    记忆技巧：
    - "trait 或类型至少一个是你的"
    - 想为标准库类型实现标准库 trait？不行！
    - 解决方案：newtype 模式（包装类型）
    */

    println!("→ 孤儿规则：trait 或类型至少一个在当前 crate，防止冲突");

    // --------------------------------------------
    // 4.5 错误5：impl Trait 返回不同类型
    // --------------------------------------------

    println!("\n4.5 错误5：impl Trait 返回不同类型");
    println!("------------------------------------------------------------");

    /*
    ❌ 错误代码：
    ```rust
    fn returns_summarizable(switch: bool) -> impl Summary {
        if switch {
            NewsArticle { ... }  // 返回 NewsArticle
        } else {
            Tweet { ... }        // 返回 Tweet
        }
    }
    ```

    编译器错误信息：
    error[E0308]: if and else have incompatible types
    expected struct `NewsArticle`, found struct `Tweet`

    原因：
    - impl Trait 在编译期被单态化成一个具体类型
    - if 和 else 分支必须返回相同类型
    - NewsArticle 和 Tweet 是不同类型

    ✅ 正确代码：
    ```rust
    // 方案1：只返回一种类型
    fn returns_summarizable() -> impl Summary {
        Tweet { ... }  // 总是返回 Tweet
    }

    // 方案2：使用 trait 对象（动态分发）
    fn returns_summarizable(switch: bool) -> Box<dyn Summary> {
        if switch {
            Box::new(NewsArticle { ... })
        } else {
            Box::new(Tweet { ... })
        }
    }
    ```

    记忆技巧：
    - impl Trait：编译期确定，只能返回一种具体类型
    - Box<dyn Trait>：运行时确定，可以返回不同类型
    - 静态分发快，动态分发灵活
    */

    println!("→ impl Trait 只能返回单一类型，多种类型用 Box<dyn Trait>");

    // --------------------------------------------
    // 4.6 错误6：trait 方法与固有方法同名
    // --------------------------------------------

    println!("\n4.6 错误6：trait 方法与固有方法同名");
    println!("------------------------------------------------------------");

    /*
    ❌ 混淆代码：
    ```rust
    trait Animal {
        fn name(&self) -> String;
    }

    struct Dog {
        name: String,
    }

    impl Dog {
        fn name(&self) -> String {  // 固有方法
            format!("Dog: {}", self.name)
        }
    }

    impl Animal for Dog {
        fn name(&self) -> String {  // trait 方法
            format!("Animal: {}", self.name)
        }
    }

    let dog = Dog { name: String::from("Buddy") };
    println!("{}", dog.name());  // ⚠️ 调用的是哪个？
    ```

    结果：
    - dog.name() 调用固有方法（优先级更高）
    - 输出：Dog: Buddy

    ✅ 明确调用 trait 方法：
    ```rust
    println!("{}", dog.name());           // 固有方法
    println!("{}", Animal::name(&dog));   // trait 方法（完全限定语法）
    ```

    记忆技巧：
    - 方法调用优先级：固有方法 > trait 方法
    - 完全限定语法：Trait::method(&value)
    - 避免歧义：不要让固有方法和 trait 方法同名
    */

    println!("→ 固有方法优先级高于 trait 方法，用完全限定语法消除歧义");

    // --------------------------------------------
    // 4.7 错误7：Copy trait 约束遗漏
    // --------------------------------------------

    println!("\n4.7 错误7：Copy trait 约束遗漏");
    println!("------------------------------------------------------------");

    /*
    ❌ 错误代码：
    ```rust
    fn largest<T: PartialOrd>(list: &[T]) -> T {
        let mut largest = list[0];  // ❌ 编译错误
        for &item in list {
            if item > largest {
                largest = item;
            }
        }
        largest
    }
    ```

    编译器错误信息：
    error[E0508]: cannot move out of type `[T]`, a non-copy slice
    help: consider restricting type parameter `T`: `T: Copy`

    原因：
    - list[0] 会尝试移动所有权
    - T 没有实现 Copy，不能直接复制
    - 对于 String 等类型，list[0] 会移动所有权，但 list 是借用的

    ✅ 正确代码：
    ```rust
    // 方案1：添加 Copy 约束
    fn largest<T: PartialOrd + Copy>(list: &[T]) -> T {
        let mut largest = list[0];  // ✅ Copy：可以复制
        for &item in list {
            if item > largest {
                largest = item;
            }
        }
        largest
    }

    // 方案2：返回引用（不需要 Copy）
    fn largest<T: PartialOrd>(list: &[T]) -> &T {
        let mut largest = &list[0];  // 借用
        for item in list {
            if item > largest {
                largest = item;
            }
        }
        largest
    }
    ```

    记忆技巧：
    - 需要复制值？添加 Copy 约束
    - 不想复制？返回引用
    - Copy 类型：基本类型（i32、f64、char）和不包含 String/Vec 的简单结构体
    */

    println!("→ 移动值需要 Copy trait，或者改为返回引用避免移动");
}

// ============================================
// 第五部分：函数/宏卡片速查
// ============================================

/*
【泛型语法】

1. 泛型函数
语法：fn function_name<T>(param: T) -> T
说明：<T> 声明泛型参数，T 可以用在参数和返回值中
示例：fn largest<T: PartialOrd>(list: &[T]) -> &T { ... }

────────────────────────────────────

2. 泛型结构体
语法：struct StructName<T> { field: T }
说明：<T> 声明泛型参数，字段可以使用 T
示例：struct Point<T> { x: T, y: T }

────────────────────────────────────

3. 泛型枚举
语法：enum EnumName<T> { Variant(T) }
说明：标准库的 Option<T> 和 Result<T, E> 就是泛型枚举
示例：
  enum Option<T> {
      Some(T),
      None,
  }

────────────────────────────────────

4. 泛型方法
语法：impl<T> StructName<T> { fn method(&self) { ... } }
说明：impl<T> 声明泛型参数，方法可以使用 T
示例：
  impl<T> Point<T> {
      fn x(&self) -> &T {
          &self.x
      }
  }

────────────────────────────────────

【Trait 语法】

5. 定义 Trait
语法：
  trait TraitName {
      fn method(&self) -> Type;
  }
说明：定义共享行为的接口
示例：
  trait Summary {
      fn summarize(&self) -> String;
  }

────────────────────────────────────

6. 实现 Trait
语法：impl TraitName for TypeName { ... }
说明：为类型实现 trait
示例：
  impl Summary for Article {
      fn summarize(&self) -> String {
          format!("{}", self.title)
      }
  }

────────────────────────────────────

7. Trait 默认方法
语法：
  trait TraitName {
      fn method(&self) -> Type {
          // 默认实现
      }
  }
说明：trait 可以提供默认实现，类型可以选择覆盖
示例：
  trait Summary {
      fn summarize(&self) -> String {
          String::from("(Read more...)")
      }
  }

────────────────────────────────────

8. Trait 约束
语法：fn function<T: Trait>(param: T)
说明：限制泛型必须实现某个 trait
示例：fn notify<T: Summary>(item: T) { ... }

────────────────────────────────────

9. 多个 Trait 约束
语法：fn function<T: Trait1 + Trait2>(param: T)
说明：T 必须同时实现多个 trait
示例：fn notify<T: Summary + Display>(item: T) { ... }

────────────────────────────────────

10. where 子句
语法：
  fn function<T, U>(t: T, u: U)
  where
      T: Trait1 + Trait2,
      U: Trait3,
  { ... }
说明：复杂 trait 约束时更清晰
示例：
  fn some_function<T, U>(t: T, u: U)
  where
      T: Display + Clone,
      U: Clone + Debug,
  { ... }

────────────────────────────────────

11. impl Trait 返回类型
语法：fn function() -> impl Trait
说明：返回"实现了 Trait 的某种类型"
限制：只能返回单一具体类型
示例：fn returns_tweet() -> impl Summary { Tweet { ... } }

────────────────────────────────────

12. Trait 对象
语法：&dyn Trait 或 Box<dyn Trait>
说明：动态分发，可以存储不同类型
性能：有虚函数表查找开销
示例：fn notify(item: &dyn Summary) { ... }

*/

// ============================================
// 第六部分：练习题
// ============================================

/*
练习题（不提供参考答案，鼓励独立思考！）

【泛型函数练习】

1. 编写一个泛型函数 `swap<T>`
   要求：交换两个变量的值
   函数签名：fn swap<T>(a: &mut T, b: &mut T)
   提示：使用 std::mem::swap 或自己实现
   测试：swap(&mut x, &mut y)，交换 i32、String 等类型

2. 编写一个泛型函数 `first<T>`
   要求：返回切片的第一个元素的引用，如果为空返回 None
   函数签名：fn first<T>(list: &[T]) -> Option<&T>
   提示：检查 list.len()，用 Some 和 None
   测试：first(&[1, 2, 3]) 返回 Some(&1)

3. 编写一个泛型函数 `contains<T>`
   要求：判断切片是否包含某个值
   函数签名：fn contains<T: PartialEq>(list: &[T], value: &T) -> bool
   提示：需要 PartialEq trait 才能用 == 比较
   注意：为什么需要 PartialEq trait？

────────────────────────────────────

【泛型结构体练习】

4. 定义一个泛型结构体 `Pair<T>`
   要求：包含两个相同类型的字段 first 和 second
   添加方法 new(first: T, second: T) -> Self
   添加方法 swap(&mut self) 交换两个字段
   测试：创建 Pair::new(1, 2)，调用 swap()

5. 定义一个泛型结构体 `Container<T>`
   要求：包含一个 Vec<T> 字段
   添加方法 new() 创建空容器
   添加方法 add(&mut self, item: T) 添加元素
   添加方法 get(&self, index: usize) -> Option<&T>
   提示：Vec 的 get 方法返回 Option<&T>

────────────────────────────────────

【Trait 定义与实现练习】

6. 定义一个 trait `Describable`
   要求：有一个方法 describe(&self) -> String
   为 String、i32、Vec<i32> 实现这个 trait
   测试：调用 describe() 打印不同类型的描述

7. 定义一个 trait `Drawable`
   要求：
   - 必须实现的方法：draw(&self)
   - 默认方法：draw_multiple(&self, count: usize) 调用 draw() count 次
   为至少两个类型实现这个 trait
   一个类型使用默认的 draw_multiple，另一个覆盖它
   注意：默认方法可以调用同 trait 的其他方法

────────────────────────────────────

【Trait 约束练习】

8. 编写一个泛型函数 `print_if_positive<T>`
   要求：如果值 > 0，打印它；否则打印 "Not positive"
   函数签名：fn print_if_positive<T: PartialOrd + Display>(value: T)
   提示：需要 PartialOrd（比较）和 Display（打印）
   注意：如何与 0 比较？需要用 T::default() 或传入一个"零值"

9. 编写一个泛型函数 `clone_and_print<T>`
   要求：克隆一个值并打印原值和克隆值
   函数签名：fn clone_and_print<T: Clone + Debug>(value: &T)
   提示：需要 Clone（克隆）和 Debug（{:?} 打印）
   测试：传入 Vec、String 等类型

────────────────────────────────────

【综合练习】

10. 实现一个通用的 `max` 函数
    要求：返回两个值中较大的一个（返回引用）
    函数签名：fn max<T: PartialOrd>(a: &T, b: &T) -> &T
    提示：用 if 比较 a 和 b
    注意：为什么返回引用而不是值？

11. 定义一个 trait `Summable` 和一个泛型函数 `sum`
    要求：
    - trait 有一个方法 add(&self, other: &Self) -> Self
    - 为 i32、f64 实现这个 trait
    - 编写函数 fn sum<T: Summable>(list: &[T]) -> T
    挑战：如何处理空切片？需要一个"零值"
    提示：可以添加一个 zero() 关联函数到 trait

12. 实现一个带有 Trait 约束的泛型缓存结构
    要求：
    - 定义 struct Cache<T> { value: Option<T> }
    - 实现方法：get_or_compute(&mut self, compute: fn() -> T) -> &T
      如果 value 是 None，调用 compute() 计算并缓存
      如果 value 是 Some，直接返回引用
    - T 需要什么 trait 约束？思考为什么
    挑战：这个练习涉及所有权、Option、闭包和泛型

*/

// ============================================
// 主函数
// ============================================

fn main() {
    println!("============================================");
    println!("泛型与 Trait - 学习笔记");
    println!("============================================");

    part1_concepts();
    part2_basic_usage();
    part4_common_errors();

    println!("\n============================================");
    println!("学习完成！");
    println!("============================================");
    println!("\n💡 关键要点：");
    println!("1. 泛型提供类型抽象，Trait 提供能力保证");
    println!("2. Trait 约束让泛型既灵活又安全：<T: Trait>");
    println!("3. 零成本抽象：泛型代码编译后和手写一样快");
    println!("4. impl Trait 返回单一类型，Box<dyn Trait> 返回多种类型");
    println!("5. 孤儿规则：trait 或类型至少一个在当前 crate");
    println!("\n🎯 下一步：完成练习题，巩固泛型与 trait 的使用！");
}

