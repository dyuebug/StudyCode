// ============================================
// 自动化测试 - 超详细注释版
// 日期：2026-10-31（第36天）
// 难度：⭐⭐ 中级
// ============================================

/*
【核心概念】简要说明

是什么？
- 测试 = 一段**会自己判断对错**的代码。它调用你的函数，比较结果，
  不符合预期就让整个构建失败。

为什么需要？
- 手工验证的结论只在"你记得测过哪些"的范围内有效。
  改了 300 行之后，只有测试能告诉你"原来那 12 个行为还在不在"。
- 测试的价值不是"证明代码对"，而是**在你改坏它的时候立刻告诉你**。

核心特点？
- **零成本编译**：`#[cfg(test)]` 标记的模块在正常构建时被整体剔除，
  不进二进制，不影响发布体积。
- **默认并行执行**：每个 `#[test]` 跑在自己的线程里，所以测试之间
  天然共享全局状态（文件、环境变量、`static`）——这既带来速度，也带来坑。
- **失败即中断**：`assert!` 系列失败会 panic，panic 被测试框架捕获并记为 FAILED。

Rust 特色？
- 测试是**语言内建**的：`#[test]` 是编译器认识的属性，不需要第三方框架。
  C++ 要 gtest、Java 要 JUnit，Rust 用标准库 + `cargo test` 就够了。
- 因为测试模块和被测代码在**同一个 crate** 里，它可以访问 `private` 函数——
  这是 Rust 单元测试比多数语言更方便的地方。

【本文件怎么跑】

```bash
# 方式 1（推荐）：测试模式，main 不执行，只跑测试
rustc --test --edition 2021 28_testing.rs -o 28_testing.exe
./28_testing

# 常用参数
./28_testing --nocapture          # 显示 println! 的输出（默认被吞掉）
./28_testing --test-threads=1     # 串行执行，排查"测试打架"
./28_testing --ignored            # 只跑标记了 #[ignore] 的
./28_testing parse                # 只跑名字里含 "parse" 的

# 方式 2：普通模式，跑 main，打印本文件导览
rustc --edition 2021 28_testing.rs -o 28_testing.exe && ./28_testing.exe
```
*/

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐（必学）
// ============================================

// --------------------------------------------
// 1.1 单元测试 / 集成测试 / 文档测试
// --------------------------------------------
/*
| 类型 | 位置 | 能看到什么 | 适合测什么 |
|---|---|---|---|
| 单元测试 | 源码文件内的 `#[cfg(test)] mod tests` | **包括 private** | 函数级逻辑、边界条件 |
| 集成测试 | 项目根的 `tests/` 目录 | 只有 `pub` API | 多个模块协作、HTTP 端点 |
| 文档测试 | `///` 注释里的 ```rust 代码块 | 公开 API | 示例代码是否真的能跑 |

三者不是"三选一"，而是**不同高度**：

```text
        文档测试      ← 用户看到的示例
           ↑
      集成测试        ← 系统装起来长什么样
           ↑
      单元测试        ← 每个零件转不转
```

对 Web 项目最实际的组合：
- 纯函数（解析、计算、校验）→ 单元测试，快、无依赖
- handler（含路由、中间件、JSON）→ 集成测试，用 `actix_web::test`

**关键区别**：单元测试写在 `src` 里，能测私有函数；
`tests/` 下的集成测试是**独立 crate**，只能看到 `pub`。
所以"要不要为了可测性把函数改成 pub"这个问题，在 Rust 里往往是
"把测试写进 src 里"就能绕过。
*/

// --------------------------------------------
// 1.2 断言宏家族
// --------------------------------------------
/*
| 宏 | 用途 | 失败时的输出 |
|---|---|---|
| `assert!(expr)` | 布尔条件 | `assertion failed: expr` |
| `assert!(expr, "…{}…", x)` | 带说明 | 说明 + 值 |
| `assert_eq!(a, b)` | 相等 | `left: 41 right: 42` |
| `assert_ne!(a, b)` | 不等 | 证明"确实不一样" |
| `panic!("…")` | 直接失败 | 消息 |

⚠️ **`assert_eq!` 的参数顺序是"实际值, 期望值"**。
写反了不会报错，只是失败信息里的 `left`/`right` 反了，
排查时容易把"期望"当成"实际"。第 4.2 节有实测输出。

三类"不好比"的值，各自的写法：
- **浮点**：不能 `assert_eq!`，要用误差比较（`(a - b).abs() < 1e-9`）
- **集合**：`Vec` 可以直接 `assert_eq!`（实现了 `PartialEq`），
  但顺序敏感的断言会在实现改成 `HashSet` 时误报
- **自定义结构体**：需要 `#[derive(Debug, PartialEq)]`，
  否则编译不过（`assert_eq!` 要求 `Debug`）
*/

// --------------------------------------------
// 1.3 测试函数的硬性规则
// --------------------------------------------
/*
- **不能有参数**：测试框架不知道从哪给你传参。
  需要参数就写成循环（表驱动）或辅助函数。
- **不能是 `async fn`**：`#[test]` 不认识 Future。
  异步要换成 `#[tokio::test]` / `#[actix_web::test]`。
- **必须能返回 `()` 或 `Result<(), E>`**：返回 `Result` 时可以用 `?`，
  出错自动判 FAILED——这比一堆 `unwrap()` 干净得多。
- **名字要能说明"测什么"**：失败列表里只有函数名，
  叫 `test1` 等于强迫自己回去看代码。
*/

// --------------------------------------------
// 1.4 并行执行与隔离
// --------------------------------------------
/*
`cargo test` / libtest **默认并行**跑测试。好处是快，代价是：

- 同一时刻可能有 8 个测试在读写同一个文件
- `std::env::set_var` 是全局的，一个测试改了会影响别人
- `static mut` / `lazy_static` 初始化只看"谁先到"

这不是"偶发 bug"，而是**确定性冲突**，只是出现顺序随机。

三种处理方式：
1. **不共享**：每个测试用自己的临时文件/临时目录（最推荐）
2. **串行**：`--test-threads=1`（排查期临时用，不当长期方案）
3. **真的需要共享**：用 `OnceLock` / `Once` 只初始化一次，
   且共享的东西必须是**只读**或**自带同步**
*/

// --------------------------------------------
// 1.5 测试到底该断言什么
// --------------------------------------------
/*
最容易写出的"假测试"：

```rust
#[test]
fn test_add() {
    let result = add(2, 3);
    if result != 5 {
        println!("好像不对");        // ← 没有断言，永远 PASS
    }
}
```

这条测试**永远不会失败**，因此**永远不会告诉你任何事**。

判断一条测试有没有价值，只有一个问题：
**"它在什么情况下会失败？"** 答不出来，就是在做无用功。

第二条标准：**断言行为，不断言实现**。
- ✅ `assert_eq!(status, 401)` —— 无论内部怎么实现，只要是 401 就通过
- ❌ "断言中间件里调用了一次 X" —— 重构一下就得改测试，而行为没变

测试应该锁定"对外承诺"，而不是"当前写法"。
*/

// ============================================
// 第二部分：基础操作实践 ⭐⭐⭐⭐⭐
// ============================================
//
// 下面是被测代码。注意它们都在 `pub` 之外——真实项目里这些通常是 private，
// 而测试能访问它们，因为测试模块和它们同属一个 crate。

/// 华氏度转摄氏度。教学用，故意保留"四舍五入到一位小数"的行为。
pub fn fahrenheit_to_celsius(f: f64) -> f64 {
    ((f - 32.0) * 5.0 / 9.0 * 10.0).round() / 10.0
}

/// 解析端口号。故意要求"1..=65535 且不含前导零"，
/// 用来演示"边界条件"该怎么测。
pub fn parse_port(s: &str) -> Result<u16, String> {
    if s.is_empty() {
        return Err("端口不能为空".to_string());
    }
    if s.len() > 1 && s.starts_with('0') {
        return Err(format!("端口不接受前导零: {s}"));
    }
    let n: u32 = s.parse().map_err(|_| format!("不是数字: {s}"))?;
    if n == 0 {
        return Err("端口 0 不可用".to_string());
    }
    if n > 65535 {
        return Err(format!("端口超出范围: {n}"));
    }
    Ok(n as u16)
}

/// 按 `-` 切分标识符。注意返回的是**切片**，借用了入参——
/// 这本身就是一个值得测的点：返回值不能比入参活得久。
pub fn split_kebab(s: &str) -> Vec<&str> {
    s.split('-').filter(|p| !p.is_empty()).collect()
}

/// 安全除法：除零返回 `None`，而不是 panic。
pub fn safe_div(a: i32, b: i32) -> Option<i32> {
    if b == 0 { None } else { Some(a / b) }
}

/// 故意会 panic 的函数，用来演示 `#[should_panic]`。
pub fn assert_positive(n: i32) {
    if n <= 0 {
        panic!("必须是正数，收到 {n}");
    }
}

// --------------------------------------------
// 2.1 测试模块
// --------------------------------------------
//
// `#[cfg(test)]` 让整个模块只在测试构建时编译。
// `use super::*` 把外部作用域的东西引进来——没有它就得写 `super::parse_port`。
#[cfg(test)]
mod tests {
    use super::*;

    /// 浮点比较的辅助函数——任何"用 == 比浮点"的地方都该换成它。
    ///
    /// 放在 `mod tests` 里面而不是外面，是有意的：它**只为测试存在**。
    /// 放在外面（不带 `#[cfg(test)]`）的话，普通构建时编译器会报
    /// `warning: function 'approx_eq' is never used`——这正是
    /// "测试代码不该进入正式构建"的一个具体表现。
    fn approx_eq(a: f64, b: f64) -> bool {
        (a - b).abs() < 1e-9
    }

    // ---------- 基本断言 ----------
    #[test]
    fn celsius_conversion_known_points() {
        // 三个"众所周知的点"：冰点、沸点、-40°（两个标度相遇的地方）
        assert_eq!(fahrenheit_to_celsius(32.0), 0.0);
        assert_eq!(fahrenheit_to_celsius(212.0), 100.0);
        assert_eq!(fahrenheit_to_celsius(-40.0), -40.0);
    }

    #[test]
    fn celsius_conversion_is_approximate() {
        // 37°C 是人类体温，换算后是 2.77…，被四舍五入到 2.8
        let c = fahrenheit_to_celsius(98.6);
        assert!(
            approx_eq(c, 37.0),
            "98.6°F 应约等于 37.0°C，实际得到 {c}"
        );
    }

    // ---------- 表驱动：一个测试覆盖多个用例 ----------
    #[test]
    fn parse_port_accepts_valid() {
        // 把"输入 → 期望输出"写成表，加用例只需要加一行。
        // 这比复制 6 个几乎一样的 #[test] 函数好维护得多。
        let cases = [
            ("1", 1u16),
            ("80", 80),
            ("8080", 8080),
            ("65535", 65535),
        ];
        for (input, expected) in cases {
            let got = parse_port(input).expect("应当解析成功");
            assert_eq!(got, expected, "输入 {input}");
        }
    }

    #[test]
    fn parse_port_rejects_invalid() {
        // 边界条件永远是 bug 的藏身处：空、零、越界、前导零、非数字。
        let bad = ["", "0", "65536", "080", "abc", "-1"];
        for input in bad {
            let got = parse_port(input);
            assert!(
                got.is_err(),
                "输入 {input:?} 应当被拒绝，实际返回 {got:?}"
            );
        }
    }

    // ---------- 用 Result 作为测试返回值：可以直接用 `?` ----------
    #[test]
    fn question_mark_returns_ok() -> Result<(), String> {
        // 返回 Result 时，`?` 一旦拿到 Err，这条测试就 FAILED，
        // 并且会把 Err 的内容打印出来。比层层 unwrap() 干净得多。
        let port = parse_port("8080").map_err(|e| format!("解析失败: {e}"))?;
        assert_eq!(port, 8080);
        Ok(())
    }

    #[test]
    fn parse_port_error_message_explains_why() -> Result<(), String> {
        // 测"错误消息有没有说清原因"，而不是只测 is_err()。
        // 注意：错误消息的具体文案会变，所以只断言关键词，不断言整句。
        let err = match parse_port("abc") {
            Err(e) => e,
            Ok(v) => return Err(format!("应当失败却成功了: {v}")),
        };
        if !err.contains("不是数字") {
            return Err(format!("错误消息应当指出原因，实际是: {err}"));
        }
        Ok(())
    }

    // ---------- Option 的测试 ----------
    #[test]
    fn safe_div_by_zero_is_none() {
        assert_eq!(safe_div(10, 0), None);
        assert_eq!(safe_div(10, 2), Some(5));
        // 注意 -7 / 2 在 Rust 里是向零取整 → -3，不是 -4
        assert_eq!(safe_div(-7, 2), Some(-3));
    }

    // ---------- 借用：返回值不能比入参活得久 ----------
    #[test]
    fn split_kebab_drops_empty_parts() {
        assert_eq!(split_kebab("a-b-c"), vec!["a", "b", "c"]);
        assert_eq!(split_kebab("a--b"), vec!["a", "b"]);
        assert_eq!(split_kebab("-a-"), vec!["a"]);
        assert_eq!(split_kebab(""), Vec::<&str>::new());
    }

    // ---------- should_panic：断言"应该炸"，并尽量断言炸的原因 ----------
    #[test]
    #[should_panic(expected = "必须是正数")]
    fn assert_positive_panics_on_negative() {
        // `expected` 会匹配 panic 消息的**子串**。
        // 不写 expected 就只能测到"炸了"，测不到"为什么炸"——
        // 那样即使把整个函数体删掉换成 panic!("随便什么")，测试照样通过。
        assert_positive(-1);
    }

    // ---------- ignore：默认不跑，需要时手动触发 ----------
    #[test]
    #[ignore = "需要真实网络，本地默认跳过"]
    fn network_dependent_placeholder() {
        // 这里放"慢 / 需要外部资源"的测试。
        // 用 `cargo test -- --ignored` 或 `--include-ignored` 才会执行。
        unreachable!("本测试默认被跳过，不应被执行");
    }

    // ---------- 辅助函数：把重复的构造逻辑抽出来 ----------
    fn sample_ports() -> Vec<&'static str> {
        vec!["1", "80", "8080"]
    }

    #[test]
    fn helper_is_reusable() {
        for p in sample_ports() {
            assert!(parse_port(p).is_ok(), "{p} 应当合法");
        }
    }
}

// ============================================
// 第三部分：深入理解（选学）
// ============================================
/*
🔍 深入理解 1：为什么默认并行，以及它带来的真实故障

libtest 为每个测试起一个线程，默认线程数 = CPU 核数。
好处：100 个测试在 16 核上大概慢 6 倍而不是 100 倍。

真实故障长这样（Web 项目最常见）：

```text
test create_todo ... ok
test delete_todo ... FAILED      ← 单独跑 100% 通过，一起跑间歇失败
```

原因：两条测试都连同一个 SQLite 文件，`delete_todo` 删掉了
`create_todo` 刚插入的行。

三种解法（按推荐度）：
1. 每个测试用一个独立的内存库 `sqlite::memory:` 或临时文件
2. 测试串行 `--test-threads=1`
3. 用 `#[serial]`（serial_test crate）只把冲突的那几条串起来

**判断依据**：症状是"单独过、一起挂"，第一反应就该查共享资源，
而不是怀疑被测代码。

────────────────────────────────────

🔍 深入理解 2：`#[actix_web::test]` 到底做了什么

（下面这段在 `web/middleware` 与 `todo-api/tests/` 里会真正编译运行，
本文件是 std-only，所以这里只作说明。）

```rust,ignore
use actix_web::{test, web, App};

#[actix_web::test]
async fn health_returns_ok() {
    // ① 构建一个"真的 App"（含中间件、路由、状态）
    let app = test::init_service(
        App::new()
            .wrap(Logging::with_prefix("[TEST]"))
            .service(web::resource("/health").to(health)),
    )
    .await;

    // ② 造请求
    let req = test::TestRequest::get().uri("/health").to_request();

    // ③ 跑一遍，拿响应
    let resp = test::call_service(&app, req).await;

    assert!(resp.status().is_success());
}
```

三个要点：
- `init_service` 是 `async` 的（要跑 `new_transform`），必须 `.await`
- 它构建的是**完整的**服务链，所以中间件也在内——这一点很关键，
  否则你测的是"没有鉴权的接口"
- `to_request()` 之后请求就变成 `Request` 了，要用 `call_service` 发出去

读 body 的两种方式：
```rust,ignore
// 拿 JSON
let body: serde_json::Value = test::read_body_json(resp).await;
// 拿原始字节
let bytes = test::read_body(resp).await;
```

────────────────────────────────────

🔍 深入理解 3：测试覆盖率是个"下限指标"

覆盖率 90% 不代表质量好——它只说明"90% 的行被执行过"。
被执行的代码完全可以在没有断言的情况下"跑过去"。

更有用的替代指标：
- **变异测试（mutation testing）**：故意把 `>` 改成 `>=`，看有没有测试失败。
  没有测试失败 → 那行代码其实没被约束。
- **"这条测试能抓住哪种回归？"**：写不出来就是冗余测试。

────────────────────────────────────

🔍 深入理解 4：`cargo test` 的三个构建目标

`cargo test` 会依次构建并运行：
1. `--lib` 的单元测试（`src` 内的 `#[cfg(test)]`）
2. `--bins` 里二进制 crate 的测试（也一样写在 src 里）
3. `tests/` 下每个文件 = 一个独立的测试 crate（会被**并行编译**）
4. 文档测试（`///` 里的代码块）

第 3 条的实际影响：`tests/common/mod.rs` 这种共享辅助模块不会自己变成
测试 crate（子目录里的 `mod.rs` 被当成模块，不是测试入口）。
而 `tests/common.rs` **会**被当成一个测试 crate 编译——里面没有 `#[test]`
就会警告"0 tests"。所以共享代码统一放子目录。
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

// --------------------------------------------
// 4.1 测试函数带参数，或写成 `async fn`
// --------------------------------------------
/*
❌ 错误代码：

```rust
#[test]
fn with_args(x: u32) {}

#[test]
async fn async_test() {}
```

```text
error: functions used as tests can not have any arguments
 --> t.rs:2:1
  |
2 | fn with_args(x: u32) {}
  | ^^^^^^^^^^^^^^^^^^^^^^^

error: async functions cannot be used for tests
 --> t.rs:5:1
  |
5 | async fn async_test() {}
  | -----^^^^^^^^^^^^^^^^^^^
  | |
  | `async` because of this
```

原因：测试框架通过反射式地"无参调用"来运行测试函数，
它没有地方去取参数，也没有执行器去驱动 Future。

✅ 正确写法：
```rust
// 要参数 → 表驱动
#[test]
fn with_args_style() {
    for x in [1u32, 2, 3] { assert!(x > 0); }
}

// 要异步 → 换属性（需 cargo 依赖）
#[tokio::test]
async fn async_ok() {
    let n = 1 + 1;
    assert_eq!(n, 2);
}
```

**记忆技巧**：`#[test]` 只认"无参、非 async、返回 `()` 或 `Result`"的函数。
*/

// --------------------------------------------
// 4.2 `assert_eq!` 参数写反 —— 失败信息会误导你
// --------------------------------------------
/*
❌ 错误代码（把期望值写在左边）：

```rust
#[test]
fn order_matters() {
    let expected = 42;
    let actual = 41;
    assert_eq!(actual, expected);   // 其实这个顺序是对的
}
```

实测输出：

```text
---- order_matters stdout ----
thread 'order_matters' panicked at t2.rs:8:5:
assertion `left == right` failed
  left: 41
 right: 42
```

看 `left: 41 / right: 42` 你得自己回忆"哪边是期望"。

✅ 约定：**`assert_eq!(实际值, 期望值)`**，于是 `left` 永远是实际值。
这样失败信息的读法是固定的："实际是 41，期望 42"。

**记忆技巧**：`left` = 你代码算出来的（**l**eft ≈ **l**ocal），
`right` = 你要求的。顺序统一了，读日志才不用猜。
*/

// --------------------------------------------
// 4.3 浮点直接 `assert_eq!`
// --------------------------------------------
/*
❌ 错误代码：

```rust
#[test]
fn float_eq() { assert_eq!(0.1 + 0.2, 0.3); }
```

实测输出：

```text
thread 'float_eq' panicked at t2.rs:2:17:
assertion `left == right` failed
  left: 0.30000000000000004
 right: 0.3
```

原因：`0.1 + 0.2` 在 IEEE 754 里不等于 `0.3`。这不是 Rust 的问题，
所有用二进制浮点的语言都一样。

✅ 正确写法：
```rust
#[test]
fn float_close_enough() {
    assert!((0.1 + 0.2 - 0.3).abs() < 1e-9);
}
```

或者用 `approx` crate 的 `assert_relative_eq!` 处理量级差异大的情况
（绝对误差 1e-9 对 1e18 这样的量级没有意义）。

**记忆技巧**：**浮点没有"等于"，只有"足够接近"**。
业务上更该做的是——金额别用浮点（用整数分），从根上避免这个问题。
*/

// --------------------------------------------
// 4.4 `#[should_panic]` 不写 `expected`
// --------------------------------------------
/*
❌ 错误代码：

```rust
#[test]
#[should_panic]
fn assert_positive_panics() {
    assert_positive(-1);
}
```

问题：这条测试只检查"炸了"。把整个函数体改成 `panic!("随便")`，
测试**照样通过**。它约束的东西比你以为的少得多。

✅ 正确写法：
```rust
#[test]
#[should_panic(expected = "必须是正数")]
fn assert_positive_panics() {
    assert_positive(-1);
}
```

注意 `expected` 是**子串匹配**，不是全等。
所以写 `"必须是正数"` 比写整句更耐改（改字号、改标点都不会误报）。

**记忆技巧**：`should_panic` 少了 `expected`，等于只测了"这里有 panic"，
而 panic 可能是 `unwrap()` 爆的、越界爆的、甚至是你写错了变量名爆的。
*/

// --------------------------------------------
// 4.5 用 `if` 代替断言 —— 测试永远 PASS
// --------------------------------------------
/*
❌ 错误代码（把 `add` 写成故意错的 `a + b + 1000` 也一样）：

```rust
fn add(a: i32, b: i32) -> i32 { a + b + 1000 }   // 明显的 bug

#[test]
fn add_works_fake() {
    let result = add(2, 3);
    if result != 5 {
        println!("结果异常: {result}");     // 只是打印，不失败
    }
}
```

实测输出（`rustc --test --edition 2021 t3.rs` 后运行）：

```text
running 3 tests
test add_works_fake ... ok
...
test result: FAILED. 1 passed; 2 failed; ...
```

**`add(2, 3)` 明明返回 1005，这条测试却是 `ok`**，而且连那句
`结果异常: 1005` 都没显示——测试框架默认吞掉了通过的测试的 stdout。

原因：测试失败的唯一途径是 panic。`println!` 不是 panic。
（想看到输出要加 `--nocapture`，但那只是让消息可见，测试结论仍然是 PASS。）

✅ 正确写法：
```rust
#[test]
fn add_works() {
    assert_eq!(add(2, 3), 5);
}
```

**记忆技巧**：测试函数里出现 `if ... { println! }` 就要警觉——
这几乎总是"想断言但不敢断言"的残留。第 1.5 节的问题同样适用：
**它在什么情况下会失败？** 答不出来就是没用。

Rust 的编译器不会帮你抓这个（`if` 是合法代码），
所以这一条只能靠自觉 + code review。**这是最常见、也最危险的假测试**：
它让你以为有测试覆盖，实际上什么都没保护。
*/

// --------------------------------------------
// 4.6 测试之间共享可变全局状态
// --------------------------------------------
/*
❌ 错误代码：

```rust
static mut COUNTER: i32 = 0;

#[test]
fn shared_1() {
    unsafe { COUNTER += 1; }
    std::thread::sleep(std::time::Duration::from_millis(20));
    unsafe { assert_eq!(COUNTER, 1); }
}

#[test]
fn shared_2() {
    unsafe { COUNTER += 1; }
    std::thread::sleep(std::time::Duration::from_millis(20));
    unsafe { assert_eq!(COUNTER, 1); }
}
```

实测输出（edition 2021）：

```text
test shared_1 ... FAILED
test shared_2 ... FAILED

---- shared_1 stdout ----
thread 'shared_1' panicked at t3.rs:17:14:
assertion `left == right` failed
  left: 2
 right: 1
...
test result: FAILED. 1 passed; 2 failed; ...
```

两条都断言"我是第一个加 1 的"，谁先谁后不确定，
但**总有一条会看到 2**。这里因为两条都先 sleep 再断言，
恰好两条都失败——换成别的时序就是"只挂一条"，而且每次挂的可能是不同那条。
**这才是最折磨人的地方：单独跑都过，一起跑随机挂。**

另外注意编译期的警告（edition 2021）：

```text
warning: creating a shared reference to mutable static
  = note: `#[warn(static_mut_refs)]` (part of `#[warn(rust_2024_compatibility)]`) on by default
```

同一个文件用 `--edition 2024` 编译，这条就从 warning 变成 **error**：

```text
error: creating a shared reference to mutable static
  = note: `#[deny(static_mut_refs)]` (part of `#[deny(rust_2024_compatibility)]`) on by default
```

——所以 `static mut` 在新版边界的项目里连编译都过不了。

✅ 三种正解：
```rust
use std::sync::atomic::{AtomicI32, Ordering};
static COUNTER: AtomicI32 = AtomicI32::new(0);   // 原子，但断言依然会被顺序影响

use std::sync::OnceLock;
static CONFIG: OnceLock<String> = OnceLock::new();  // 只读共享

// 最推荐：每个测试用自己的局部变量
#[test]
fn t1() { let mut counter = 0; counter += 1; assert_eq!(counter, 1); }
```

**记忆技巧**：测试里出现 `static mut` / 全局文件路径 / 环境变量，
就等于"测试之间有了隐藏的通信通道"。并行执行会把这条通道变成随机失败。
*/

// --------------------------------------------
// 4.7 测试写死了文件路径，污染开发数据
// --------------------------------------------
/*
❌ 错误代码：

```rust
#[test]
fn save_and_load() {
    std::fs::write("todos.db", b"test").unwrap();   // ← 覆盖开发库！
    assert_eq!(std::fs::read("todos.db").unwrap(), b"test");
}
```

症状：跑完测试，开发环境的数据没了。而且并行跑时两个测试互相覆盖。

✅ 正确写法：每个测试一个临时目录，**且目录名要能区分并行线程**
```rust
use std::sync::atomic::{AtomicUsize, Ordering};
static SEQ: AtomicUsize = AtomicUsize::new(0);

#[test]
fn save_and_load() {
    // ⚠️ 只用 process::id() 不够：同一个测试进程里的多个测试线程
    // 拿到的是同一个 pid，并行跑时照样撞名。必须再加自增计数。
    let n = SEQ.fetch_add(1, Ordering::Relaxed);
    let dir = std::env::temp_dir().join(format!("t-{}-{n}", std::process::id()));
    std::fs::create_dir_all(&dir).unwrap();
    let path = dir.join("todos.db");

    std::fs::write(&path, b"test").unwrap();
    assert_eq!(std::fs::read(&path).unwrap(), b"test");

    let _ = std::fs::remove_dir_all(&dir);   // 清理失败不影响测试结论
}
```

**记忆技巧**：测试里的路径必须是"每个测试独一份"的。
出现裸文件名（`"todos.db"`、`"config.json"`）就是红灯。

【实测补充：手工清理有两个坑】（2026-10-31 在 `todo-api/tests/api.rs` 里踩到）

1. **测试 panic 时，最后那句 `remove_dir_all` 永远执行不到。**
   于是每挂一次就在磁盘上留一个库，越积越多。
   解决办法是把它写成 `Drop` 实现（RAII）：panic 展开时 `Drop` 一定会跑。
   `tempfile::TempDir` 就是干这个的；不想加依赖就自己写 20 行。
2. **Windows 上单次删除会和 WAL 的 `-shm` 内存映射赛跑**，
   实测 8 个测试里有 2 个报
   `另一个程序正在使用此文件，进程无法访问。 (os error 32)`，
   即使 `pool.close().await` 已经返回。
   解决办法是重试（10 次 × 50ms），而不是加一个 `sleep` 猜时间。
*/

// ============================================
// 第五部分：函数/宏卡片速查
// ============================================
/*
【`#[test]`】

语法：#[test] 写在 `fn` 上方（无参、非 async）
作用：把函数注册为测试用例
返回：`()` 或 `Result<(), E>`（后者可用 `?`）
注意：`#[cfg(test)]` 只控制"编不编译"，`#[test]` 控制"跑不跑"。
      两个都要——只写 `#[test]` 会让测试代码进入正式构建。

────────────────────────────────────

【`assert!` / `assert_eq!` / `assert_ne!`】

语法：
  assert!(cond)
  assert!(cond, "格式 {}", arg)
  assert_eq!(实际值, 期望值)
  assert_ne!(a, b)
失败行为：panic，被测试框架捕获并记为 FAILED
要求：`assert_eq!` 的类型必须实现 `Debug + PartialEq`

────────────────────────────────────

【`#[should_panic(expected = "…")]`】

作用：断言"这次调用必须 panic"，`expected` 匹配 panic 消息子串
不写 `expected` 的后果：只测到"炸了"，测不到"为什么炸"

────────────────────────────────────

【`#[ignore = "原因"]`】

作用：默认跳过；`--ignored` 只跑它们，`--include-ignored` 一起跑
用途：慢测试、需要网络/外部服务的测试

────────────────────────────────────

【`std::sync::OnceLock`】

```rust
static CONFIG: OnceLock<String> = OnceLock::new();
let v = CONFIG.get_or_init(|| load_config());
```
用途：测试间共享**只读**初始化结果，且只初始化一次（并行安全）

────────────────────────────────────

【`actix_web::test` 三件套】（在 todo-api / web/middleware 中验证）

| 函数 | 用途 |
|---|---|
| `test::init_service(app).await` | 构建完整 App（含中间件） |
| `test::TestRequest::get().uri(..).to_request()` | 造请求 |
| `test::call_service(&app, req).await` | 发请求拿响应 |
| `test::read_body_json(resp).await` | 读 JSON body |
| `test::read_body(resp).await` | 读原始字节 |

────────────────────────────────────

【`cargo test` 常用参数】

| 参数 | 作用 |
|---|---|
| `-- --nocapture` | 显示测试里的 println! 输出 |
| `-- --test-threads=1` | 串行执行 |
| `-- --ignored` | 只跑 `#[ignore]` 的 |
| `-- --include-ignored` | 全跑，包括 ignore |
| `-- <关键字>` | 只跑名字含关键字的测试 |
| `--lib` / `--test <name>` | 只跑单元测试 / 只跑某个集成测试 |
*/

// ============================================
// 第六部分：练习题
// ============================================
/*
1. **给 `split_kebab` 补一条"不分配"的测试**（必做）
   要求：验证返回值切片**指向原字符串**（用 `as_ptr()` 比较地址），
   证明它是零拷贝而非新建 `String`。
   提示：`s.as_ptr() as usize` 与返回切片第一个元素的地址比较。
   注意：空输入时没有元素，要先处理这种情况。

2. **把练习 2.4 的表驱动改成"失败时指出是哪个用例"**（必做）
   现在 `assert!(got.is_err())` 失败时只说"断言失败"。
   要求：让失败信息包含**具体的非法输入**。
   提示：`assert!(cond, "…")` 的格式串里带上 `input`。

3. **写一条"会真的失败"的测试，然后修好它**（必做）
   要求：挑一个函数，写一条断言它当前**不满足**的行为，
   跑出 FAILED，读输出，再改代码让它通过。
   目的：确认你的测试真的能失败——这是"假测试"的唯一检测手段。

4. **给 `parse_port` 加一个真实 bug，看哪条测试能抓住**（必做）
   例：把 `n > 65535` 改成 `n >= 65535`。
   要求：记录哪些测试失败、哪些没失败，并解释为什么。
   思考：有没有"没人测"的边界？

5. **把浮点比较改成相对误差**（进阶）
   现在的 `approx_eq` 用绝对误差 1e-9，对 `1e18` 这样的量级无意义。
   要求：实现 `approx_rel(a, b, rel)`，并补一条测试证明
   `1e18 + 1` 与 `1e18` 在 `rel = 1e-6` 下"相等"。
   提示：`(a - b).abs() / a.abs().max(b.abs())`，注意除零。

6. **找出本文件里"测了但没测到"的测试**（思考题）
   提示：2.1 节里 `parse_port_rejects_invalid` 用的是 `assert!(got.is_err())`，
   它**不关心为什么错**。所以把 `parse_port` 改成"任何输入都返回同一个 Err"
   （哪怕是 `-1`），这条测试**照样通过**。
   要求：改成能区分六种非法原因（空 / 零 / 越界 / 前导零 / 非数字 / 负数）的版本。

7. **设计 todo-api 的测试隔离方案**（思考题）
   要求：写出 3 种方案（共享内存库 / 每测试一个临时文件 / 串行），
   分别说明优缺点，并选出你要用的那个及理由。
   提示：先别信"内存库每个连接是独立的库"这个说法——去
   `sqlx-sqlite-0.8.6/src/options/parse.rs` 里看 `:memory:` 被改写成了什么，
   再决定。另一个更关键的问题是：**内存库跑不跑得到 WAL 分支**，
   而线上用的正是 `journal_mode(Wal)`。
*/

// ============================================
// 普通模式下的导览（测试模式下不会执行）
// ============================================

fn main() {
    println!("============================================================");
    println!("自动化测试 —— 第 36 天");
    println!("============================================================");
    println!("本文件是测试教学材料。正确跑法：");
    println!();
    println!("  rustc --test --edition 2021 28_testing.rs -o 28_testing.exe");
    println!("  ./28_testing");
    println!();
    println!("常用参数：");
    println!("  --nocapture            显示 println! 输出");
    println!("  --test-threads=1       串行执行");
    println!("  --ignored              只跑 #[ignore] 的");
    println!("  parse                  只跑名字含 parse 的");
    println!();
    println!("被测函数（可以在普通模式下直接观察行为）：");
    println!("  fahrenheit_to_celsius(98.6) = {}", fahrenheit_to_celsius(98.6));
    println!("  parse_port(\"8080\")          = {:?}", parse_port("8080"));
    println!("  parse_port(\"080\")           = {:?}", parse_port("080"));
    println!("  split_kebab(\"-a--b-\")       = {:?}", split_kebab("-a--b-"));
    println!("  safe_div(10, 0)             = {:?}", safe_div(10, 0));
    println!("============================================================");
}
