# unsafe Rust 速查表

> 对应学习材料：`rust_basics/21_unsafe.rs`（第 29 天）、`rust_basics/22_ffi.rs`（第 30 天）
> 核心立场：`unsafe` 是**给编译器看的承诺**，不是"高级模式"的开关。

---

## 1. 一句话

`unsafe` 只做一件事：**把编译器原本要替你证明的责任，交回给你**。
它不解锁"更多功能"，只解锁"更多责任"。

---

## 2. 解锁的五项能力（只此五项，背下来）

| # | 能力 | 写法 |
|---|---|---|
| 1 | 解引用裸指针 | `*ptr`，`ptr.as_ref().unwrap_unchecked()` |
| 2 | 调用 `unsafe fn` | FFI 函数、`get_unchecked` |
| 3 | 访问 / 修改 `static mut` | `static mut COUNTER: u32 = 0;` |
| 4 | 实现 `unsafe trait` | `unsafe impl Send for MyType {}` |
| 5 | 访问 `union` 字段 | `union U { a: u32, b: f32 }` |

**不在这五项里的，`unsafe` 一概不解锁**：
借用检查照旧、类型检查照旧、生命周期照旧、可变性规则照旧。

---

## 3. 语法速查

### 3.1 `unsafe {}` 块 vs `unsafe fn`

```rust
// unsafe fn：调用者必须再包一层 unsafe —— 责任向下传递
unsafe fn raw_read(p: *const u8) -> u8 { *p }

// 现代写法：函数本身安全，危险部分局部圈起来
fn safe_read(p: *const u8) -> Option<u8> {
    if p.is_null() { return None; }
    // SAFETY: 上面已排除空指针；调用方保证指针在其生命周期内有效
    Some(unsafe { *p })
}
```

规则：**能用 `unsafe {}` 块就不要用 `unsafe fn`**。
块的作用域越小，要证明的越少，review 时越容易确认。

### 3.2 裸指针与引用互转

```rust
let x = 5;
let r: &i32 = &x;
let p: *const i32 = r as *const i32;     // 引用 → 裸指针：安全
let v: &i32 = unsafe { &*p };            // 裸指针 → 引用：必须 unsafe
```

裸指针不会自动判空、不检查别名、不检查生命周期——这三件事全归你。

### 3.3 `static mut` 与现代替代

```rust
static mut COUNTER: u32 = 0;

fn bump() {
    // ⚠️ 从 edition 2024 起，对 static mut 取引用直接报错
    unsafe { COUNTER += 1; }
}
```

**替代方案（优先用）**：`AtomicU32` / `OnceLock` / `Mutex`。
第 36 天实测：`28_testing.rs` 在 `--edition 2024` 下编译报
`deny(static_mut_refs)` —— 这不是笔误，是语言在推你换掉这种写法。

### 3.4 FFI

```rust
extern "C" {
    fn abs(input: i32) -> i32;
}

fn main() {
    let n = unsafe { abs(-7) };
    println!("{n}");
}
```

要点：
- `extern "C"` 声明的是**外部符号**，链接期解析（`#[link(name="...")]`）
- 所有 FFI 调用都是 `unsafe`：C 侧没有类型安全，没有生命周期
- edition 2024 起 `unsafe extern "C" { ... }` 成为推荐写法（把 unsafe 声明在块上）

---

## 4. 安全抽象：唯一正确的用法

```rust
pub struct Buf {
    ptr: *mut u8,
    len: usize,
}

impl Buf {
    /// 安全接口：外部看不到裸指针，也构造不出非法状态
    pub fn new(len: usize) -> Self { /* ... */ }

    pub fn get(&self, i: usize) -> Option<u8> {
        if i >= self.len { return None; }          // 边界在安全层检查
        // SAFETY: i < len，且 ptr 指向 new() 分配的 len 字节
        Some(unsafe { *self.ptr.add(i) })
    }
}

impl Drop for Buf {
    fn drop(&mut self) {
        // SAFETY: ptr 来自 new() 里的 Layout::array::<u8>(len) 分配，
        // 且 Drop 只执行一次，所以这里用的是与分配时**完全相同**的 layout
        unsafe { std::alloc::dealloc(self.ptr, std::alloc::Layout::array::<u8>(self.len).unwrap()) }
    }
}
```

三条铁律：
1. **unsafe 实现，safe 暴露**——不安全代码绝不出现在公开签名里
2. 每个 `unsafe` 块上写 `// SAFETY:` 注释，说明"为什么这里安全"
3. 安全接口一旦承诺（如"`get` 越界返回 `None`"），`unsafe` 内部必须守住

---

## 5. 排查与约束工具

| 手段 | 作用 |
|---|---|
| `#![forbid(unsafe_code)]` | 整个 crate 禁止 unsafe，编译期强制 |
| `#![deny(unsafe_op_in_unsafe_fn)]` | `unsafe fn` 内部也必须显式写 `unsafe {}`（推荐打开） |
| `// SAFETY:` 注释 | 不是给编译器看的，是给 review 的人看的 |
| Miri（`cargo +nightly miri test`） | 检测 UB：越界、悬垂、数据竞争。本项目**未实测** |

---

## 6. 踩过的坑

### 6.1 以为 `unsafe` 能绕过借用检查

```rust
let mut v = vec![1, 2, 3];
let a = &v[0];
unsafe { v.push(4); }        // ❌ 依然报错：借用检查不受 unsafe 影响
```

`unsafe` 管的是"内存安全由谁证明"，不是"可变性规则可以无视"。

### 6.2 `static mut` 在多线程下的假安全感

```rust
static mut COUNTER: u32 = 0;
// 两个线程同时 unsafe { COUNTER += 1 } —— 编译通过，运行数据竞争
```

`+=` 不是原子操作。能编译 ≠ 正确。用 `AtomicU32` 才是解法，
`unsafe` 只是把编译器的抗议关掉了。

### 6.3 SAFETY 注释写成"应该是安全的"

不合格的注释：`// SAFETY: 这里应该没问题`
合格的注释必须回答**具体问题**：指针非空吗？在生命周期内吗？别名规则满足吗？
谁保证的（调用方前置条件 / 上面的 `if` / 类型不变量）？

---

## 7. 判断清单：这段代码需要 unsafe 吗

- [ ] 只为了"绕过借用检查"？→ **不需要**，那是设计问题（用 `Rc<RefCell>` / `Arc<Mutex>` / 重构所有权）
- [ ] 只为了"少一次边界检查"？→ 先测：那通常是万分之一的时间
- [ ] 与 C 库交互？→ 需要，且必须包一层 safe 接口
- [ ] 实现底层容器（自写 `Vec`）？→ 需要，且必须有 `SAFETY` 注释 + Miri
- [ ] 以上都不是？→ 不需要

**默认答案是不需要。** `unsafe` 的成本不在写的时候，在改的时候——
三个月后没人记得当初为什么那样写是安全的。
