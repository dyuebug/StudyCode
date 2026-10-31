// ============================================
// 宏（Macros）- 超详细注释版
// 日期：2026-10-29（第28天）
// 难度：⭐⭐⭐ 高级
// ============================================

/*
【核心概念】简要说明

是什么？
- 宏是**在编译期展开的代码模板**。你写的不是函数调用，而是一段"用来生成代码的规则"，
  编译器在类型检查之前把宏展开成真正的代码。

为什么需要？
- 消除重复：一堆结构相同、类型不同的实现（为 i32/f64/Point 各写一遍加法）
- 语法扩展：println!/vec!/format! 这类"看起来不像函数"的语法只能靠宏
- 编译期计算：能在编译期报错（断言写的常量是否为 2 的幂），函数做不到
- 可变参数：Rust 函数不支持变参，宏可以

核心特点？
- 声明宏 `macro_rules!`：模式匹配式，写起来像 match
- 三种过程宏：derive（派生）、attribute（属性）、function-like（函数式）
- 卫生性（hygiene）：宏内部引入的变量不会污染调用处
- 零运行时开销：展开后就是普通代码

Rust 特色？
- 与 C 的 #define 根本不同：宏操作的是**词法树（token tree）**，不是文本替换，
  所以不存在 `#define SQUARE(x) x*x` 然后 `SQUARE(1+2)` 得到 `1+2*1+2` 这种经典灾难。
- 与 C++ 模板也不同：模板是泛型机制的载体，Rust 的泛型独立存在，宏负责"语法层"的抽象。
*/

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================
//
// 本文件用注释讲解概念，用 main() 做可运行的实证。
// 每个宏在使用前都有对应的卡片讲解（见第五部分）。


// ============================================
// 宏定义区
// ============================================
//
// 说明：macro_rules! 必须在**使用之前**定义（文本顺序上），
// 否则报 "cannot find macro `xxx` in this scope"。
// 这是与函数相反的地方——函数可以定义在调用之后。

// --------------------------------------------
// log_info! —— 带时间戳的风格化日志
// --------------------------------------------
macro_rules! log_info {
    // 模式 1：只有一条消息
    ($msg:expr) => {
        println!("[INFO] {}", $msg)
    };

    // 模式 2：带格式化参数
    // 注意 $(...)* 里的 $arg:expr 表示"0 个或多个表达式"
    ($fmt:expr, $($arg:expr),*) => {
        println!("[INFO] {}", format!($fmt, $($arg),*))
    };
}

// --------------------------------------------
// square! —— 证明宏是 AST 匹配而非文本替换
// --------------------------------------------
macro_rules! square {
    ($x:expr) => {
        $x * $x
    };
}

// --------------------------------------------
// my_vec! —— 多模式：空 / 列表 / 重复
// --------------------------------------------
macro_rules! my_vec {
    // 空
    () => {
        Vec::new()
    };

    // 重复形式：my_vec![elem; n]
    // 注意这个分支必须放在列表分支之前或之后都行（模式互不重叠），
    // 但顺序会影响匹配优先级，习惯上把更具体的放前面
    ($elem:expr; $n:expr) => {
        vec![$elem; $n] // 复用标准库的 vec!
    };

    // 列表形式：my_vec![a, b, c]
    ($($x:expr),+ $(,)?) => {
        {
            let mut v = Vec::new();
            $( v.push($x); )+
            v
        }
    };
}

// --------------------------------------------
// count_args! —— 计数：演示重复模式与递归
// --------------------------------------------
macro_rules! count_args {
    // 递归终止：零参数
    () => { 0usize };

    // 递归：吃掉一个参数，剩余继续
    // $head 被"消费"掉，$($tail),* 递归匹配
    ($head:expr $(, $tail:expr)*) => {
        1usize + count_args!($($tail),*)
    };
}

// --------------------------------------------
// max_of! —— 可变参数求最大值（递归 + 比较）
// --------------------------------------------
macro_rules! max_of {
    // 单参数直接返回
    ($x:expr) => {
        $x
    };

    // 多参数：先比前两个，剩下的递归
    ($a:expr, $($rest:expr),+) => {{
        let a = $a;
        let b = max_of!($($rest),+);
        if a > b { a } else { b }
    }};
}

// --------------------------------------------
// sum_all! —— 求和，演示 + 0 的起点
// --------------------------------------------
macro_rules! sum_all {
    // 用 fold 风格的递归求和
    ($($x:expr),*) => {{
        let mut total = 0;
        $( total += $x; )*
        total
    }};
}

macro_rules! const_assert {
    ($cond:expr) => {
        // 技巧：定义一个 const，值由 panic! 决定
        // 编译期能算出条件为 false 时，panic! 在 const 求值阶段就会触发
        // 若为 true，则生成一个空 const，运行期完全消失
        const _: () = assert!($cond);
    };
}

// --------------------------------------------
// show_expr! —— 打印表达式源码 + 求值结果
// --------------------------------------------
macro_rules! show_expr {
    // stringify! 把 token 序列还原成字符串，这是宏独有的能力
    ($e:expr) => {
        println!("{} = {:?}", stringify!($e), $e)
    };
}


// 宏内部用 let x 遮蔽，验证卫生性
// 注意：宏内部的 e=1、x=999，结算结果是 1 + 999 = 1000
macro_rules! with_temp {
    () => {{
        let x = 999; // 这个 x 属于宏内部上下文
        let e = 1; // $e 不是捕获的，是宏内部变量
        e + x
    }};
}

fn main() {
    println!("============================================================");
    println!("第一部分：核心概念详解");
    println!("============================================================");

    section_1_1_what_is_macro();
    section_1_2_token_tree();
    section_1_3_hygiene();
    section_2_basic_practice();
    section_3_advanced();
    println!();
    println!("============================================================");
    println!("第四部分：常见错误见文件末尾注释（编译错误需手动取消注释验证）");
    println!("============================================================");
}

// --------------------------------------------
// 1.1 宏 vs 函数：差在哪
// --------------------------------------------

fn section_1_1_what_is_macro() {
    println!();
    println!("1.1 宏和函数的本质区别");
    println!("------------------------------------------------------------");

    /*
    【问题】为什么下面这件事函数做不到？

    想让下面三种类型都支持"取最大值"，函数写法：
        fn max_i32(a: i32, b: i32) -> i32 { ... }
        fn max_f64(a: f64, b: f64) -> f64 { ... }
        fn max_point(a: Point, b: Point) -> Point { ... }
    → 要写 N 遍。用泛型呢？
        fn max<T: PartialOrd>(a: T, b: T) -> T { if a > b { a } else { b } }
    → 这个可以，所以"类型不同"这类重复**优先用泛型，不该用宏**。

    【真正必须用宏的场景】
    1. 参数个数可变：
         println!("{} {}", 1, 2);   // 函数无法表达
         my_vec![1, 2, 3, 4, 5];    // 函数无法接收任意个参数
    2. 语法本身要变：
         let v = my_vec![0; 5];     // 中缀分号语法，函数调用写不出
    3. 在编译期做检查或生成代码：
         const_assert_is_power_of_two!(1024);
    4. 需要拿到"参数的名字"而不只是值：
         assert_eq!(a, b) 能打印出 "a == b 失败" 靠的是宏能看见标识符文本

    【一句话判据】
    先问"泛型能不能解决？"能就用泛型。泛型解决不了（变参、语法、编译期检查、
    需要操作调用处的符号名），才用宏。
    */

    // 泛型版本——这才是"不同类型重复"的正确解法
    println!("泛型 max(3, 7) = {}", max_generic(3, 7));
    println!("泛型 max(2.5, 1.5) = {}", max_generic(2.5, 1.5));

    // 宏版本——处理"任意个参数"
    println!("宏 max_of!(3, 7, 2, 9, 4) = {}", max_of!(3, 7, 2, 9, 4));
    println!("宏 max_of!(2.5, 1.5) = {}", max_of!(2.5, 1.5));
}

/// 泛型函数：处理"类型不同"的重复
///
/// 参数：
/// - a, b: 任意实现了 PartialOrd 的类型
/// 返回：两者中较大者
///
/// 所有权：T 按值传入，返回 T 的所有权
fn max_generic<T: PartialOrd>(a: T, b: T) -> T {
    if a > b {
        a
    } else {
        b
    }
}

// --------------------------------------------
// 1.2 宏操作的是"词法树"，不是文本
// --------------------------------------------

fn section_1_2_token_tree() {
    println!();
    println!("1.2 macro_rules! 匹配的是词法树（token tree）");
    println!("------------------------------------------------------------");

    /*
    【核心概念】

    C 的宏是**文本替换**，所以：
        #define SQUARE(x) x * x
        SQUARE(1 + 2)   →  展开成  1 + 2 * 1 + 2  →  结果是 5，不是 9

    Rust 的 macro_rules! 匹配的是**词法单元（token）**，`1 + 2` 是一个完整的
    token 序列，被 `$x:expr` 整体捕获，所以不存在这个问题：

        macro_rules! square { ($x:expr) => { $x * $x }; }
        square!(1 + 2)  →  展开成  (1 + 2) * (1 + 2)  →  结果是 9

    【为什么 Rust 自动加了括号？】
    因为 `$x` 捕获的是一个 **AST 片段**（这里是 expr），插入时作为完整表达式插入，
    不会与周围运算符"粘连"。这是 Rust 宏比 C 宏安全的结构性原因。

    【匹配器（fragment specifier）一览】
        $x:expr    → 表达式
        $x:ty      → 类型
        $x:ident   → 标识符（变量名、函数名、类型名）
        $x:path    → 路径（如 std::vec::Vec）
        $x:pat     → 模式（match 分支里那种）
        $x:stmt    → 语句
        $x:block   → 代码块 { ... }
        $x:literal → 字面量（这个最严格，只接受 1、"a"、true 这类）
        $x:tt      → 单个词法树，最宽松，过程宏式的写法常用它
        $x:item    → 项（fn、struct、impl 等）
    */

    // 验证：C 宏 vs Rust 宏的经典差异
    println!("square!(1 + 2) = {}", square!(1 + 2));
    println!("square!(3)     = {}", square!(3));
    // 若把上面的宏改成 `$x:tt` 匹配，square!(1 + 2) 会编译失败——
    // 因为 $x:tt 只能吃一个 token，`1` 后面多出的 `+ 2` 无处安放。
    // 这就是"匹配器选得太窄"的典型症状。
}

// --------------------------------------------
// 1.3 卫生性（hygiene）
// --------------------------------------------

fn section_1_3_hygiene() {
    println!();
    println!("1.3 卫生性：宏内部变量不会污染调用处");
    println!("------------------------------------------------------------");

    /*
    【概念】卫生性（hygiene）指：宏展开产生的标识符属于"宏内部的语法上下文"，
    调用处的同名标识符是"调用处的语法上下文"，两者即使同名也**不会互相遮蔽或覆盖**。

    【表达式位置的卫生性】
        let x = 1;
        let y = with_temp!();      // 宏内部：let x = 999; $e + x
        println!("{}", x);         // 仍然是 1，不是 999

    【但有一个重要例外】
    macro_rules! 的卫生性只对**局部变量**成立。下面这些都**不卫生**：
      - 宏里定义的 fn / struct / 类型：会泄露到调用处的作用域
      - 宏里引用的**外部路径**：按调用处解析，不是定义处
      - $x 捕获进来的标识符：按调用处的上下文解析

    所以"macro_rules! 是部分卫生的"，不是完全卫生。这一点与 Scheme 的完全卫生宏不同。

    【实践含义】
    宏内部不要依赖调用处"应该存在"某个同名变量；
    宏内部定义的辅助函数要么加前缀（__my_macro_helper），要么放在块里。
    */

    let x = 1;
    let y = with_temp!();
    println!("调用处的 x = {}（没有被宏内部的 x 污染）", x);
    println!("宏返回的 y = {}", y);
}


// ============================================
// 第二部分：基础操作实践
// ============================================

fn section_2_basic_practice() {
    println!();
    println!("============================================================");
    println!("第二部分：基础操作实践");
    println!("============================================================");

    // --------------------------------------------
    // 2.1 最简单的宏：单模式
    // --------------------------------------------
    println!();
    println!("2.1 单模式宏 log_info!");
    println!("------------------------------------------------------------");

    log_info!("服务器启动");
    log_info!("监听端口 {}", 8080);
    log_info!("{} 条数据，耗时 {} ms", 42, 15);

    // --------------------------------------------
    // 2.2 多模式（像 match 一样分派）
    // --------------------------------------------
    println!();
    println!("2.2 多模式宏 my_vec!");
    println!("------------------------------------------------------------");

    let empty: Vec<i32> = my_vec![];
    let list = my_vec![1, 2, 3];
    let repeated = my_vec![0u8; 4];

    println!("my_vec![]        = {:?}", empty);
    println!("my_vec![1,2,3]   = {:?}", list);
    println!("my_vec![0u8; 4]  = {:?}", repeated);

    // --------------------------------------------
    // 2.3 重复模式 $( ... ),*
    // --------------------------------------------
    println!();
    println!("2.3 重复模式：处理可变参数");
    println!("------------------------------------------------------------");

    println!("count_args!()                 = {}", count_args!());
    println!("count_args!(a)                = {}", count_args!(a));
    println!("count_args!(a, b, c, d)       = {}", count_args!(a, b, c, d));

    println!("max_of!(3)                    = {}", max_of!(3));
    println!("max_of!(3, 9, 2)              = {}", max_of!(3, 9, 2));

    println!("sum_all!(1, 2, 3, 4, 5)       = {}", sum_all!(1, 2, 3, 4, 5));

    // --------------------------------------------
    // 2.4 生成 impl 块：批量消除重复
    // --------------------------------------------
    println!();

    /*
    【为什么这里不用宏？】

    批量生成 impl 是 macro_rules! 的经典用途：
        impl_add!(Meters);  impl_add!(Point);
    一次生成 N 个 `impl Add`。

    但这个例子**刻意手写**，理由是：

    1. 字段级运算无法用 `$t:ty` 直接生成
       `Meters(self.0 + other.0)` 里的 `.0` 是字段访问，
       宏拿到的 `$t:ty` 是类型 AST，**访问不到字段名和字段个数**。
       要生成字段级代码必须用过程宏（derive），不是 macro_rules!。

    2. 可见性成本
       一个 impl_add! 宏会让 rust-analyzer 的跳转失效，
       后来的维护者必须"展开宏"才知道 Add 到底怎么实现的。
       4 行手写 vs 1 行宏调用 + 一个需要读的宏定义——后者更贵。

    3. 编译时间
       每个宏调用点都是一次独立展开，宏越多编译越慢。

    【判据】宏适合"重复的是**语法形状**"；
           如果重复的只是"字段乘以字段"这种**结构**，那是派生宏（derive）的活，
           不是 macro_rules! 的活。
    */

    println!("2.4 批量 impl：宏能做什么、不该做什么");
    println!("------------------------------------------------------------");

    let a = Meters(3.0);
    let b = Meters(4.5);
    println!("Meters(3.0) + Meters(4.5) = {:?}", a + b);
    println!("Meters(3.0) * 2.0         = {:?}", a * 2.0);

    let p1 = Point { x: 1, y: 2 };
    let p2 = Point { x: 10, y: 20 };
    println!("Point + Point = {:?}", p1 + p2);

    // --------------------------------------------
    // 2.5 编译期检查：const 断言
    // --------------------------------------------
    println!();
    println!("2.5 编译期断言 const_assert!");
    println!("------------------------------------------------------------");

    // 这两行在编译期完成校验，运行期零开销
    const_assert!(true);
    const_assert!(1 + 1 == 2);
    println!("const_assert! 通过（编译期校验，运行期无开销）");

    // 下面这行会**编译失败**，这是它的价值所在：
    // const_assert!(1 == 2);
    // 编译器输出：
    //   error[E0080]: evaluation of `main::_::{constant#0}` failed
    //    --> src/main.rs:xxx
    //    = note: assertion failed: 1 == 2

    // --------------------------------------------
    // 2.6 作用域宏（local macro）
    // --------------------------------------------
    println!();
    println!("2.6 局部宏：不影响其他模块");
    println!("------------------------------------------------------------");

    // 下面这个宏只在本函数内可见，因为它定义在函数体内部
    macro_rules! local_double {
        ($x:expr) => {
            $x * 2
        };
    }
    println!("local_double!(21) = {}", local_double!(21));
    // 出了这个函数，local_double! 就不存在了——这是控制宏污染最有效的手段

    // --------------------------------------------
    // 2.7 调试辅助：dbg 风格打印
    // --------------------------------------------
    println!();
    println!("2.7 能打印「表达式原文」的宏");
    println!("------------------------------------------------------------");

    let price = 19.9;
    let qty = 3.0;
    // 注意输出里带上了 "price * qty" 这个**源码文本**，函数做不到这件事
    show_expr!(price * qty);
}

// ============================================
// 第三部分：深入理解（可选）
// ============================================

fn section_3_advanced() {
    println!();
    println!("============================================================");
    println!("第三部分：深入理解（可选）");
    println!("============================================================");

    /*
    如果你想知道"为什么"，读这一节。觉得难可以跳过，不影响后续学习。

    【1】展开顺序：宏在类型检查之前
        parse → expand（宏展开）→ name resolution → type check → borrow check → codegen

        所以宏**看不到类型信息**。你无法写"如果 T 实现了 Display 就走 A 分支"这种宏
        （除非用过程宏 + trait 探测技巧，那是高阶玩法）。
        这也意味着：宏展开出错时，报错位置常常指向宏内部而非调用处，
        用 `cargo expand` 可以看到真实展开结果。

    【2】macro_rules! 是递归下降
        每次匹配一个模式 → 展开 → 对展开结果再做一次匹配。所以：
          - 可以写递归宏（my_vec! 就是递归的）
          - 递归深度默认上限 128 层
          - 递归宏写错会得到 "recursion limit reached while expanding"，
            这种错误最难读，因为看不到递归到哪一步

    【3】三种过程宏
        derive 宏（#[derive(MyTrait)]）：
            输入是 struct/enum 的 token 流，输出是 impl 块的 token 流。
            serde 的 #[derive(Serialize)] 就是它。
            必须放在独立的 proc-macro crate 里（crate-type = ["proc-macro"]）。
        attribute 宏（#[my_route(GET, "/x")]）：
            附着在 item 上，可以读、修改、替换它。actix-web 的 #[get("/")] 属于这类。
        function-like 宏（my_macro!(...)）：
            长得像 macro_rules!，但能拿到完整 token 流、做任意解析。
            sqlx 的 query! 会在编译期连数据库、校验 SQL 与类型的匹配，就是它。

    【4】为什么宏不能替代泛型
        宏展开发生在类型检查前，所以宏对每个调用点生成一份代码，
        N 个类型 = N 份展开 = 代码膨胀（二进制变大）。
        泛型走单态化，也是每个类型一份，但编译器能优化、能做类型检查。
        结论：能用泛型就不用宏，宏留给"语法层"的问题。

    【5】宏的成本模型
        - 编译期：宏多了会拖慢编译（尤其递归宏 + 大量调用点）
        - 运行期：零。展开后就是普通代码，没有虚函数、没有装箱
        - 可读性：这是宏的真实成本。一个 200 行的 macro_rules! 会让后来者难以维护，
          这时应该换成更简单的方案，或者拆成多个小宏

    【6】`$crate` 与 #[macro_export]
        用 #[macro_export] 导出宏后，宏内部的路径按**调用处**解析，
        所以调用处没引入 serde 时 `serde_json::to_string` 会找不到。
        解法：用 `$crate::` 指回定义它的 crate，这是跨 crate 宏必须遵守的规则。
    */

    println!("第三部分为概念讲解，无需运行代码。");
    println!("深入点：展开顺序 / 递归 / 三种过程宏 / 泛型优先 / $crate");
}

// ============================================
// 被宏生成/辅助的类型
// ============================================

/// 用宏批量实现运算符的类型 1
#[derive(Debug, Default, Clone, Copy)]
struct Meters(f64);

// 手写 impl：字段级运算手写更清楚，这里刻意不用宏（见 2.4 的取舍说明）
impl std::ops::Add for Meters {
    type Output = Meters;
    fn add(self, other: Meters) -> Meters {
        Meters(self.0 + other.0)
    }
}

impl std::ops::Mul<f64> for Meters {
    type Output = Meters;
    fn mul(self, other: f64) -> Meters {
        Meters(self.0 * other)
    }
}

/// 用宏批量实现运算符的类型 2
#[derive(Debug, Default, Clone, Copy, PartialEq)]
struct Point {
    x: i32,
    y: i32,
}

impl std::ops::Add for Point {
    type Output = Point;
    fn add(self, other: Point) -> Point {
        Point {
            x: self.x + other.x,
            y: self.y + other.y,
        }
    }
}

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

/*
以下每个错误都给出：错误代码、真实编译器错误、正确写法、原因。
本文件为了让所有示例共存，错误示例以注释形式保留，验证时取消注释即可复现。


// --------------------------------------------
// 4.1 错误：宏定义在使用之后
// --------------------------------------------
❌ 错误代码：
```rust
fn main() {
    hello!();          // 使用在前
}

macro_rules! hello {   // 定义在后
    () => { println!("hi") };
}
```

编译器错误信息：
```
error: cannot find macro `hello` in this scope
 --> src/main.rs:2:5
  |
2 |     hello!();
  |     ^^^^^ not found in this scope
  |
  = note: `hello` is defined later
```

原因：macro_rules! 按**文本顺序**生效，不像 fn 那样可以前向引用。

✅ 正确代码：
```rust
macro_rules! hello { () => { println!("hi") }; }

fn main() { hello!(); }
```

记忆技巧：**宏像"变量声明"，必须先声明后使用；函数像"函数声明"，可以任意顺序。**


// --------------------------------------------
// 4.2 错误：匹配器选得太窄
// --------------------------------------------
❌ 错误代码：
```rust
macro_rules! bad_square {
    ($x:tt) => { $x * $x };   // tt 只能吃一个 token tree
}

fn main() { bad_square!(1 + 2); }
```

编译器错误信息：
```
error: no rules expected `+` while trying to match `+`
 --> src/main.rs:6:19
  |
6 |     bad_square!(1 + 2);
  |                  ^ no rules expected this token in macro call
```

原因：`1 + 2` 是三个 token（`1`、`+`、`2`）。`$x:tt` 只捕获第一个 `1`，
剩下 `+ 2` 没有规则能匹配。

✅ 正确代码：
```rust
macro_rules! good_square {
    ($x:expr) => { $x * $x };  // expr 捕获整个表达式
}
```

记忆技巧：**想要"一整段表达式"就用 expr；只有写过程宏式解析时才用 tt。**


// --------------------------------------------
// 4.3 错误：重复模式里分隔符不一致
// --------------------------------------------
❌ 错误代码：
```rust
macro_rules! bad_list {
    ($($x:expr),*) => { vec![$($x),*] };   // 定义用逗号
}
// 调用时用分号
// bad_list!(1; 2; 3);
```

编译器错误信息：
```
error: no rules expected `;` while trying to match `;`
 --> src/main.rs:8:14
  |
8 |     bad_list!(1; 2; 3);
  |               ^ no rules expected this token in macro call
```

原因：重复模式 `$(...),*` 里的分隔符是**契约的一部分**。定义用逗号，
调用就必须用逗号。

✅ 正确代码：
```rust
bad_list!(1, 2, 3);
// 想支持分号？再加一个分支：
// ($($x:expr);*) => { vec![$($x),*] };
```

记忆技巧：**分隔符写在 `*` 前面，定义与调用必须一致。**


// --------------------------------------------
// 4.4 错误：变量只定义在某个分支里
// --------------------------------------------
❌ 错误代码：
```rust
macro_rules! bad_multi {
    () => { let v = Vec::new(); };
    ($($x:expr),*) => { let mut v = Vec::new(); $(v.push($x);)* };
}

fn main() {
    bad_multi!(1, 2);
    // 调用处的 v 不存在
}
```

编译器错误信息：
```
error[E0425]: cannot find value `v` in this scope
 --> src/main.rs:10:5
  |
10 |     v.len();
  |     ^ not found in this scope
```

原因：宏展开的 `let` 处在宏自己的**卫生上下文**里，调用处的 `v` 是另一个 `v`。
这是卫生性在起作用——**不是 bug，是保护**。

✅ 正确代码：用块表达式把值**返回**出去
```rust
macro_rules! good_multi {
    () => {{ Vec::<i32>::new() }};
    ($($x:expr),*) => {{
        let mut v = Vec::new();
        $( v.push($x); )*
        v
    }};
}
let v = good_multi!(1, 2);
```

记忆技巧：**宏要"产出值"而不是"声明变量"，用双花括号 `{{ ... }}` 包成块表达式。**


// --------------------------------------------
// 4.5 错误：重复模式里写成 `$($x),*` 却用了 `$x`
// --------------------------------------------
❌ 错误代码：
```rust
macro_rules! bad_print {
    ($($x:expr),*) => {
        $( println!("{}", $x); )*
        println!("{}", $x);      // ← 想打印最后一个？不行
    };
}
```

编译器错误信息：
```
error: attempted to repeat an expression containing no syntax variables matched as repeating at this depth
 --> src/main.rs:4:22
  |
4 |         println!("{}", $x);
  |                      ^^
```

原因：`$x` 只存在于 `$(...)*` 重复块**内部**。重复块外没有"当前 $x"这种概念。

✅ 正确代码：需要在重复内完成所有事，或用递归把"最后一个"单独剥出
```rust
macro_rules! last_of {
    ($x:expr) => { $x };
    ($_head:expr, $($rest:expr),+) => { last_of!($($rest),+) };
}
```

记忆技巧：**`$var` 的生命周期只到它所在的重复块结束。**


// --------------------------------------------
// 4.6 错误：递归宏没有终止分支
// --------------------------------------------
❌ 错误代码：
```rust
macro_rules! bad_rec {
    ($head:expr, $($tail:expr),*) => {
        1 + bad_rec!($($tail),*)     // 没有 () => 终止分支
    };
}
fn main() { bad_rec!(1, 2); }
```

编译器错误信息：
```
error: recursion limit reached while expanding `bad_rec!`
 --> src/main.rs:6:5
  |
  = help: consider increasing the recursion limit by adding a
          `#![recursion_limit = "256"]` attribute to your crate
```

原因：`bad_rec!(2)` 里 `$($tail),*` 是空的，但 `$head:expr, $($tail),*` 要求
至少有一个逗号分隔的 head，于是退化成 `bad_rec!(2,)` → 再展开 → 无限循环。

✅ 正确代码：**永远先写终止分支**
```rust
macro_rules! good_rec {
    () => { 0usize };                               // ← 终止
    ($head:expr $(, $tail:expr)*) => {
        1usize + good_rec!($($tail),*)              // ← 注意没有 $head 后的逗号
    };
}
```

记忆技巧：**递归宏的第一个分支必须是"空输入"。** 把终止分支写在下边的递归宏，
迟早会炸。注意正确写法里是 `$($tail),*` 而不是 `, $($tail),*`——后者会在
只剩一个元素时匹配失败。


// --------------------------------------------
// 4.7 错误：跨 crate 用宏，路径找不到
// --------------------------------------------
❌ 错误代码（在另一个 crate 里）：
```rust
// mylib 里：
#[macro_export]
macro_rules! save_json {
    ($v:expr) => {
        serde_json::to_string(&$v).unwrap()   // ← 按调用处解析
    };
}

// 调用方 crate（没依赖 serde_json）：
// use mylib::save_json;
// save_json!(vec![1,2,3]);
```

编译器错误信息：
```
error[E0433]: failed to resolve: use of undeclared crate or module `serde_json`
 --> src/main.rs:3:9
  |
3 |         serde_json::to_string(&$v).unwrap()
  |         ^^^^^^^^^^ use of undeclared crate or module `serde_json`
```

原因：宏展开后的**路径按调用处解析**（macro_rules! 的不卫生部分）。
调用方没有 serde_json 依赖，自然找不到。

✅ 正确代码：用 `$crate::` 指回定义处
```rust
#[macro_export]
macro_rules! save_json {
    ($v:expr) => {
        $crate::serde_json::to_string(&$v).unwrap()
        // 前提：mylib 里 re-export 了 serde_json
        // pub use serde_json;
    };
}
```

记忆技巧：**导出宏里的一切路径都加 `$crate::` 前缀。** 这是跨 crate 宏的铁律。


// --------------------------------------------
// 4.8 错误：在宏里 `use` 了外部类型，但只匹配了部分情况
// --------------------------------------------
❌ 错误代码：
```rust
macro_rules! make_adder {
    ($a:expr) => {
        |x| x + $a
    };
}
// 传入非 Copy 类型会移动
// let s = String::from("x");
// let f = make_adder!(s);      // s 被移进闭包
// println!("{}", s);           // ← 报错
```

编译器错误信息：
```
error[E0382]: borrow of moved value: `s`
 --> src/main.rs:7:20
  |
5 | let f = make_adder!(s);
  |                      - value moved here
7 | println!("{}", s);
  |                ^ value borrowed here after move
```

原因：宏展开后 `|x| x + $a` 会按值捕获 `$a`。宏不改变所有权规则——
**展开结果是什么语义，宏就是什么语义**。

✅ 正确代码：让调用方显式 clone，或用引用
```rust
let f = make_adder!(s.clone());
// 或
macro_rules! make_adder_ref { ($a:expr) => { |x| x + &$a }; }
```

记忆技巧：**宏没有魔法，展开后的借用检查就是你写的那段代码的借用检查。**
*/

// ============================================
// 第五部分：函数/宏卡片速查
// ============================================

/*
【宏1：log_info!】

语法：log_info!(消息)
      log_info!(格式串, 参数...)
参数：
  - $msg: expr —— 单条消息
  - $fmt: expr —— 格式化串（必须含 {} 占位符）
  - $arg: expr —— 0 个或多个填充参数
返回：()（直接打印，无返回值）
所有权：参数按值传入立即消耗，不保留引用；参数表达式在 println! 内部求值
示例：
```rust
log_info!("启动");
log_info!("端口 {}", 8080);
```
────────────────────────────────────

【宏2：square!】

语法：square!(表达式)
参数：
  - $x: expr —— 任意表达式，作为整体参与乘法
返回：同类型值
所有权：$x 若为非 Copy 类型会被**求值两次**（这是 expr 重复展开的固有代价）
示例：
```rust
square!(1 + 2)  // → (1+2)*(1+2) = 9
```
⚠️ 注意：`square!(s.len())` 会调用 len() 两次；
    `square!(String::from("a"))` 会因两次求值而失败或产生两份临时值。
    需要"只求值一次"就写 `{ let t = $x; t * t }`。
────────────────────────────────────

【宏3：my_vec!】

语法：my_vec![]
      my_vec![a, b, c]
      my_vec![elem; n]
参数：
  - 无 / 0+ 个 $x: expr / ($elem: expr, $n: expr)
返回：Vec<T>
所有权：元素被 move 进 Vec；$elem 在重复形式下要求 Clone
示例：
```rust
let empty: Vec<i32> = my_vec![];
let v = my_vec![1, 2, 3];
let r = my_vec![0u8; 4];
```
────────────────────────────────────

【宏4：count_args!】

语法：count_args!(任意个表达式)
参数：0+ 个 $head: expr
返回：usize（参数个数）
所有权：不解引用、不求值内部的"值语义"，仅计数；但每个表达式需能被解析
示例：
```rust
assert_eq!(count_args!(), 0);
assert_eq!(count_args!(a, b, c), 3);
```
────────────────────────────────────

【宏5：max_of!】

语法：max_of!(x, y, z, ...)
参数：1+ 个 $expr（必须实现 PartialOrd 且为 Copy 或可比较类型）
返回：最大元素（按值）
所有权：所有参数被 move；两侧临时值在同一块作用域内比较
示例：
```rust
max_of!(3, 9, 2)  // → 9
```
────────────────────────────────────

【宏6：const_assert!】

语法：const_assert!(编译期布尔表达式)
参数：$cond: expr —— 必须能在编译期求值
返回：无（成功时展开为 `const _: () = ();`）
所有权：无运行期影响
示例：
```rust
const_assert!(1 + 1 == 2);   // 通过
const_assert!(1 == 2);       // 编译失败 error[E0080]
```
────────────────────────────────────

【宏7：show_expr!】

语法：show_expr!(任意表达式)
参数：$e: expr
返回：()（打印 "源码文本 = 值"）
所有权：只求值一次
示例：
```rust
let price = 19.9; let qty = 3;
show_expr!(price * qty);   // 输出：price * qty = 59.7
```
────────────────────────────────────

【函数1：stringify!】

语法：stringify!(token 序列) -> &'static str
参数：任意 token（不求值，只取文本）
返回：&'static str，内容为参数的源码文本
所有权：不涉及，纯编译期操作
示例：
```rust
stringify!(a + b)  // → "a + b"
```
────────────────────────────────────

【函数2：concat!】

语法：concat!(字面量1, 字面量2, ...) -> &'static str
参数：必须是字面量（字符串、数字、bool、char），不能是变量
返回：拼接后的 &'static str
所有权：编译期完成，零运行期成本
示例：
```rust
concat!("[INFO] ", "{}")  // → "[INFO] {}"
```
────────────────────────────────────

【函数3：assert! / assert_eq!】

语法：assert!(cond)、assert!(cond, "失败信息 {}", x)
      assert_eq!(left, right)、assert_eq!(left, right, "信息")
参数：布尔条件 / 两个可比较值（需实现 Debug + PartialEq）
返回：()
所有权：assert_eq! 借用两侧（不需要 Copy），但要求实现 Debug
示例：
```rust
assert_eq!(count_args!(a, b), 2, "应该有两个参数");
```
────────────────────────────────────

【属性：#[macro_export]】

语法：#[macro_export] macro_rules! name { ... }
作用：把宏导出到 crate 根，其他 crate 可 `use crate_name::name;` 或直接调用
注意：导出后宏内所有路径需加 `$crate::` 前缀
────────────────────────────────────

【属性：#[macro_use]】

语法：#[macro_use] mod macros;  或  #[macro_use] extern crate foo;
作用：把子模块中的 macro_rules! 提升到父作用域（2015 edition 风格）
注意：2018+ 推荐用 #[macro_export] + use，#[macro_use] 已属遗留写法
*/

// ============================================
// 第六部分：练习题（不提供答案）
// ============================================

/*
【练习 1：基础 —— 写一个 `assert_with_msg!`】
要求：
  - 接受 (条件, 消息)，条件失败时打印消息并 panic
  - 消息要能带格式化参数，如 assert_with_msg!(x > 0, "x 应为正数，实际 {}", x)
提示：
  - 需要两个分支：不带格式参数、带格式参数
  - 检查你自己写出的宏，`x > 0` 用的是 `expr` 还是 `tt`？为什么？
注意：
  - 宏内对 cond 的求值只能发生一次，否则 `x > 0` 里的 x 可能被求值两次

【练习 2：基础 —— 写一个 `derive_display!` 风格的宏】
要求：
  - 给定一个只有字段的 struct 名，生成 `impl std::fmt::Display`
  - 输出格式：`Name { field1: v1 }`（一个字段的情况即可）
提示：
  - `struct $name:ident { $field:ident: $ty:ty }` 这种模式可以解构结构体定义
  - 需要用 `stringify!($field)` 拿到字段名文本
注意：
  - 这个宏只能处理"恰好一个字段"的情况。想处理多字段必须引入重复模式
  - 思考：为什么标准库要让 derive 走过程宏而不是 macro_rules!？

【练习 3：进阶 —— 把 `count_args!` 改成 `count_with!` 支持自定义初值】
要求：
  - `count_with!(10; a, b, c)` 返回 13
  - `count_with!(0;)` 返回 0
提示：
  - 注意分号作为"初值"与"列表"的边界
  - 递归终止分支要能匹配"初值 + 空列表"
注意：
  - 顺序很重要：`($init:expr;)` 必须放在 `($init:expr; $($x:expr),+)` 之前，
    否则空列表情形会匹配失败

【练习 4：进阶 —— 实现 `hashmap_like!`】
要求：
  - `let m = my_map!{ "a" => 1, "b" => 2 };` 生成 HashMap<&str, i32>
  - 空调用 `my_map!{}` 生成空 HashMap
提示：
  - 重复模式的分隔符用 `=>` 之外还需要逗号：`$($k:expr => $v:expr),*`
  - 空调用需要单独分支，否则类型推断不出 K/V

【练习 5：高阶 —— 实现编译期断言 `const_assert_eq!`】
要求：
  - `const_assert_eq!(4, 2 + 2)` 编译通过
  - `const_assert_eq!(4, 5)` 编译失败并给出可读信息
提示：
  - 参考 `const _: () = assert!(...);` 的写法
  - 想给出自定义错误信息，需要 `panic!` 在 const 上下文中被调用，
    而 `panic!` 的参数在 const 求值里受限——思考如何绕开
注意：
  - 编译期断言失败时，错误位置指向**宏定义的 const 定义处**，
    用户看到的行号不是调用处，这是已知的体验缺陷

【练习 6：综合 —— 用宏重写 todo-api 的 handler 样板】
要求：
  - todo-api 的 5 个 handler 都有"取锁 → 操作 → drop → save"的重复结构
  - 设计一个 `with_state!` 宏把它压缩成 3 行
提示：
  - 必须保证锁在 await 之前释放（async 环境里持有 MutexGuard 跨 await 会编译失败）
  - 先想清楚：这个抽象真的让代码更清楚，还是只是更短？
注意：
  - 这是"该不该用宏"的判断题。如果答案是"不该"，请写出你的理由——
    在面试中，"我知道怎么做但我选择不做"比"我会写宏"更有价值
*/

// ============================================
// 文件结束
// ============================================
