# proc-macros

第 45 天补做项：**派生宏（derive macro）与 workspace**。
对应原路线图第 7 周"宏"里未落地的部分——之前只写了 `macro_rules!`，没写过程宏。

目标不是"能生成代码"，而是**生成的报错能被使用者看懂**：
过程宏最容易被忽视的质量维度是 span（错误指向哪里），
一个 `unwrap()` 就能把错误指到宏内部，让使用者完全无从下手。

---

## 1. 当前状态（2026-11-09）

| 项 | 状态 | 依据 |
|---|---|---|
| 编译 | ✅ 零警告 | `cargo build --offline` |
| 测试 | ✅ 8 条通过 | `cargo test --offline` → `8 passed; 0 failed` |
| 格式 | ✅ | `cargo fmt --all --check` 退出码 0 |
| 静态检查 | ✅ | `cargo clippy --all-targets --offline -- -D warnings` 退出码 0 |
| 报错质量 | ✅ 手工验证 | 未知属性键的 caret 精确指向 `min_lenn` |
| 依赖 | syn / quote / proc-macro2（本地缓存可用，crates.io 不可达） | 见 §4 |

---

## 2. 目录结构

```
proc-macros/
├── Cargo.toml                  workspace 根（只有 [workspace]，没有 [package]）
├── validate-derive/            proc-macro = true，只能导出宏
│   ├── Cargo.toml
│   └── src/lib.rs              194 行：解析 #[validate(...)] 并生成 impl
└── validate-demo/              消费者：普通 lib + bin + 集成测试
    ├── src/lib.rs              User / Article / Ping 三个用例
    ├── src/main.rs             demo 可执行文件
    └── tests/validate.rs       8 条集成测试
```

**为什么必须是两个 crate**：
`proc-macro = true` 的 crate **只能**导出过程宏，不能同时当普通库用。
这不是约定，是硬错误——试着在 proc-macro crate 里写一个 trait：

```
error: `proc-macro` crate types currently cannot export any items other than
       functions tagged with `#[proc_macro]`, `#[proc_macro_derive]`,
       or `#[proc_macro_attribute]`
 --> src\lib.rs:1:1
  |
1 | pub trait Validate { fn validate(&self) -> bool; }
  | ^^^^^^^^^^^^^^^^^^
```

而派生宏生成 `impl Validate for X` 需要使用者能 `use` 到 `Validate` 这个 trait ——
所以至少两个 crate。这正是 workspace 在真实项目里的第一个理由。

---

## 3. 用法

```rust
use validate_derive::Validate;

#[derive(Validate)]
struct User {
    #[validate(min_len = 3, max_len = 20)]
    name: String,
    #[validate(min = 1, max = 150)]
    age: u8,
    email: String,
}

let errs: Vec<String> = user.validate().unwrap_err();  // 一次收集全部错误
```

生成的 `validate(&self) -> Result<(), Vec<String>>`：

- **累积**所有字段错误后统一返回（不是遇到第一个就返回）
- 只支持具名结构体；其他形状走 `syn::Error::to_compile_error()` 变成编译错误
- 未知键、重复键、非整数字面量都会定位到**用户写的那一行**

```bash
cargo run --offline -p validate-demo
```

实测输出：

```
=== #[derive(Validate)] 演示 ===

用例 1 · name="张三" age=30
  1 项不合法：
    - name: 长度 2 < 最小 3

用例 2 · name="张三丰" age=30
  校验通过

用例 3 · name="" age=0
  2 项不合法：
    - name: 长度 0 < 最小 3
    - age: 0 < 最小 1

用例 4 · name="xxxxxxxxxxxxxxxxxxxxxxxxx" age=200
  2 项不合法：
    - name: 长度 25 > 最大 20
    - age: 200 > 最大 150
```

注意用例 1：`"张三"` 是 2 个字符、6 个字节。若宏用 `len()`，这里会误判为通过。

---

## 4. 三条实测结论

1. **字符数不是字节数**。`"张三"` 是 2 个 `char`、6 个字节。
   宏里若用 `s.len()`，`min_len = 3` 会错误地放行。
   `tests/validate.rs::中文按字符计数而不是字节数` 就是专门防这个的：
   它断言 `"张三"` 必须报错、且错误文案里是「长度 2」。

2. **报错质量靠 span 决定**。把属性写成 `#[validate(min_lenn = 3)]`，
   用 `new_spanned(meta.path, ...)` 构造错误，caret 会精确指向 `min_lenn`；
   若改成 `format!` + `TokenStream::from_str`，错误会指向整个 derive 调用点，
   使用者看不出是哪一行的哪个键错了。

3. **本机 crates.io 不可达**（`curl` 到 crates.io 返回 `000`），
   全部构建都靠 `cargo --offline` 与本地注册表缓存。
   缓存里有 syn / quote / proc-macro2 / thiserror / tracing 系列，
   **没有 clap、没有 criterion** —— 这直接决定了另外两个项目改用零依赖方案。

---

## 5. 未验证 / 已知局限

- 只支持具名结构体、`min_len/max_len/min/max` 四种规则；不支持枚举、元组结构体、泛型约束细化
- 没有实现 `#[validate(crate = path)]`：真实库（如 `validator`）需要它来解耦
  「宏 crate」与「trait crate」，否则使用者必须同时依赖两者
- 未在生产规模的 crate 上试过编译耗时；派生宏增加编译时间这一点只有感知，无数据

---

## 6. 下一步（若继续）

1. 把 `Validate` trait 抽到第三个 crate（`validate-core`），宏只生成 `impl`
2. 支持枚举：`#[validate(min_len)]` 作用于 `String` 变体
3. 给错误加结构化字段（字段名 + 规则 + 实际值），而不是纯 String
