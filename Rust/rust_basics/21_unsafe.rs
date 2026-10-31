// ============================================
// unsafe Rust - 超详细注释版
// 日期：2026-10-29（第29天）
// 难度：⭐⭐⭐ 高级
// ============================================

/*
【核心概念】简要说明

是什么？
- `unsafe` 是 Rust 的"逃生舱"：它允许你**亲自承担编译器的证明责任**。
  Rust 的安全保证建立在"编译器能证明你的代码不违反规则"之上；
  有些合法程序编译器证明不了，`unsafe` 就是对这些程序开的例外通道。

为什么需要？
- 与 C 交互（FFI）必然绕过 Rust 的类型规则
- 底层数据结构（Vec、HashMap 的内部）需要手工管理内存
- 性能临界路径需要省掉边界检查
- 实现编译器无法验证但人类知道安全的不变量

核心特点？
- `unsafe` **只解除五项检查**，不多不少（见下）
- `unsafe` **不会**关掉借用检查、类型检查、生命周期检查
- `unsafe` 的代价是**证明责任**：你必须写清楚"为什么这里安全"
- 正确用法永远是：**用 unsafe 实现，用 safe 接口暴露**（安全抽象）

Rust 特色？
- 与 C 的"全语言都是 unsafe"相比：Rust 把危险隔离成小块，其余 99% 代码受编译器保护
- 与 Go/Java 的 GC 相比：Rust 不付运行时成本，代价是编译期证明
- `unsafe` 是**给编译器看的承诺**，不是给程序员的开关——写错它不会让代码"更灵活"，
  只会让代码**未定义行为（UB）**
*/

/*
【unsafe 解锁的五项能力】—— 只此五项，背下来

  1. 解引用裸指针（`*const T` / `*mut T`）
  2. 调用 `unsafe fn`（包括 FFI 函数）
  3. 访问或修改 `static mut` 变量
  4. 实现 `unsafe trait`（如 `Send` / `Sync`）
  5. 访问 `union` 的字段

  不在这五项里的，`unsafe` 一概不解锁：
  借用检查照旧、类型检查照旧、生命周期照旧、可变性规则照旧。
*/

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

fn main() {
    println!("============================================================");
    println!("第一部分：核心概念详解");
    println!("============================================================");

    section_1_1_borrow_check_still_works();
    section_1_2_two_kinds_of_unsafe();

    println!();
    println!("============================================================");
    println!("第二部分：基础操作实践");
    println!("============================================================");
    section_2_1_raw_pointers();
    section_2_2_split_at_mut();
    section_2_3_unsafe_fn();
    section_2_4_union();
    section_2_5_safe_abstraction();

    println!();
    println!("============================================================");
    println!("第三部分：深入理解（概念，无需运行）");
    println!("============================================================");
    println!("见文件末尾注释：UB 的定义 / Miri / 为什么安全抽象是唯一正确用法");
}

// --------------------------------------------
// 1.1 unsafe 块里，借用检查器照样工作
// --------------------------------------------

// 本函数内刻意放了一个不含 unsafe 操作的 unsafe 块作教学演示，
// 因此整体关闭 `unused_unsafe` 警告
#[allow(unused_unsafe)]
fn section_1_1_borrow_check_still_works() {
    println!();
    println!("1.1 unsafe 不解锁借用检查");
    println!("------------------------------------------------------------");

    /*
    【最常见的误解】"用了 unsafe 就可以随便改数据、绕过所有权"。

    ❌ 事实：即使包在 unsafe 块里，下面这些**依然编译失败**：

        let mut v = vec![1, 2, 3];
        let r = &v[0];
        unsafe {
            v.push(4);   // error[E0502]: cannot borrow `v` as mutable
            println!("{}", r);
        }

    报错原因与不加 unsafe 时**一模一样**。因为借用检查发生在类型系统层面，
    与"操作是否危险"是两件事。

    【unsafe 真正绕开的，是编译器对"运行时前提"的证明】
    比如 `*p` 解引用：编译器无法证明 p 指向有效内存，所以要求你签名担保。
    而"两个 &mut 不能共存"这种**静态可判定**的规则，编译器自己就能查，不需要你担保。

    【为什么这个区分重要】
    如果 unsafe 关掉了借用检查，那 Rust 的内存安全就完全不成立了。
    unsafe 的设计哲学是：**危险的是"编译器查不了的"，不是"所有规则"**。
    */

    // 正确示例：unsafe 里仍然遵守所有权
    //
    // 注意下面这个 unsafe 块里**没有任何 unsafe 操作**——编译器会警告
    // `unnecessary unsafe block`，这个警告本身就是知识点：
    // unsafe 是一种"声明"，它不改变块内代码受检的程度。
    // 这里刻意保留，用函数上的 `#[allow(unused_unsafe)]` 压掉警告。
    let mut v = vec![1, 2, 3];
    unsafe {
        // 这里只是演示 unsafe 块能嵌入普通安全代码
        if let Some(first) = v.first() {
            println!("unsafe 块内读取 first = {}", first);
        }
    }
    v.push(4);
    println!("unsafe 块外 push 正常，v = {:?}", v);
}

// --------------------------------------------
// 1.2 unsafe fn 与 unsafe 块：两种声明
// --------------------------------------------

fn section_1_2_two_kinds_of_unsafe() {
    println!();
    println!("1.2 `unsafe fn` 与 `unsafe {{}}` 的区别");
    println!("------------------------------------------------------------");

    /*
    【`unsafe fn f()`】声明"调用者必须满足某些前提"。
        调用它的时候，必须写 `unsafe { f() }`。
        但在**函数体内**，Rust 2015/2018 允许直接做 unsafe 操作（不推荐）。
        Rust 2024 edition 起，`unsafe fn` 内部也需要显式 `unsafe {}` 块——
        因为经验证明"整个函数体都是 unsafe"会导致审查疲劳，写不出准确的 SAFETY 注释。

    【`unsafe { ... }`】标记"这一段我担保安全"。
        块外照旧受保护。更细粒度，更推荐。

    【最佳实践（业界共识）】
      - 对外 API 尽量是 `fn`（safe），内部用 `unsafe {}` 实现
      - 只有当"安全性完全取决于调用者"时，才暴露 `unsafe fn`
        典型例子：`Vec::set_len`、`slice::get_unchecked`
      - 每个 unsafe 块前面必须写 `// SAFETY: <为什么安全>`
    */

    let v = [10u32, 20, 30];

    // 调用 unsafe fn：必须显式 unsafe 块
    // SAFETY: 索引 1 < v.len()，在界内，满足 get_unchecked 的前提
    let val = unsafe { v.get_unchecked(1) };
    println!("get_unchecked(1) = {}", val);

    // 对照：safe 版本会自动做边界检查
    println!("普通索引 [1]      = {}", v[1]);
}

// ============================================
// 第二部分：基础操作实践
// ============================================

// --------------------------------------------
// 2.1 裸指针：*const T / *mut T
// --------------------------------------------

fn section_2_1_raw_pointers() {
    println!();
    println!("2.1 裸指针的创建、读写、判空");
    println!("------------------------------------------------------------");

    let mut x = 42i32;

    // 创建裸指针**不需要 unsafe**——因为创建本身不碰内存
    let p_const: *const i32 = &x;
    let p_mut: *mut i32 = &mut x;

    println!("p_const 是空指针？{}", p_const.is_null());
    println!("p_mut 是空指针？  {}", p_mut.is_null());

    // 解引用**需要 unsafe**
    // SAFETY: p_const 来自 &x，x 存活且未被移动，指针有效且对齐
    let read = unsafe { *p_const };
    println!("通过 *const 读取 = {}", read);

    // SAFETY: p_mut 来自 &mut x，独占借用期尚未结束，写入合法
    unsafe {
        *p_mut = 100;
    }
    println!("通过 *mut 写入后 x = {}", x);

    // 裸指针可以跨作用域长期持有——这正是它们危险的原因：
    // 编译器不再追踪它们的生命周期，悬垂与否全靠你
    let dangling_risk = make_pointer(&x);
    // SAFETY: x 仍在当前作用域内存活，dangling_risk 仍指向它
    println!("跨函数返回的指针再解引用 = {}", unsafe { *dangling_risk });
}

/// 返回指向内部值的裸指针
///
/// 这是**故意写错示范的反面**：函数签名里看不出任何生命周期约束，
/// 返回后编译器不会再检查 x 是否还活着。
///
/// # Safety
/// 调用者必须保证 `v` 在指针被解引用时仍然存活。
fn make_pointer(v: &i32) -> *const i32 {
    v as *const i32
}

// --------------------------------------------
// 2.2 经典案例：手写 split_at_mut
// --------------------------------------------

fn section_2_2_split_at_mut() {
    println!();
    println!("2.2 经典案例：为什么要 unsafe");
    println!("------------------------------------------------------------");

    /*
    【问题】想同时拿到 Vec 的两个不重叠的可变切片。

    直接写：
        fn split_at_mut(v: &mut [i32], mid: usize) -> (&mut [i32], &mut [i32]) {
            (&mut v[..mid], &mut v[mid..])
        }
    编译失败：
        error[E0499]: cannot borrow `*v` as mutable more than once at a time

    【为什么编译器不让过】
    它不知道 `..mid` 和 `mid..` 是不相交的。对编译器来说，
    `v[..a]` 和 `v[b..]` 完全是两次独立的可变借用——**除非它做区间分析**。
    Rust 的借用检查器不做这种分析（这是静态分析的复杂度取舍）。

    【标准库怎么解决】
    用裸指针 + `slice::from_raw_parts_mut` 手工构造两个切片，
    由人类担保"这两段确实不重叠"。这就是 `std::slice::split_at_mut` 的实现思路。
    */

    let mut data = [1, 2, 3, 4, 5, 6];

    // ⚠️ 这里用花括号限定 left/right 的存活范围。
    // 因为 my_split_at_mut 的可变借用覆盖整个 data，
    // 括号不结束就不能再 `&data` 打印——即使底层两个切片互不重叠，
    // 编译器也只知道"data 被可变借用了"。
    // 这个细节恰好说明：unsafe 让函数**实现**绕过了借用检查，
    // 但**签名**层面的 &mut data 借用规则一点没松动。
    {
        let (left, right) = my_split_at_mut(&mut data, 3);
        left[0] = 100;
        right[0] = 600;
        println!("left = {:?}  right = {:?}", left, right);
    }

    println!("修改后 data = {:?}", data);
}

/// 把可变切片劈成两半（`split_at_mut` 的手写版）
///
/// # 参数
/// - `v`: 可变切片
/// - `mid`: 切分点，必须满足 `mid <= v.len()`
///
/// # 返回
/// 两个不重叠的可变切片
///
/// # Panics
/// `mid > v.len()` 时 panic
fn my_split_at_mut(v: &mut [i32], mid: usize) -> (&mut [i32], &mut [i32]) {
    let len = v.len();
    assert!(mid <= len, "mid ({}) 超出切片长度 ({})", mid, len);

    let ptr = v.as_mut_ptr();

    // SAFETY:
    // 1. `ptr` 来自 `v`，有效且对齐；
    // 2. `[ptr, ptr+mid)` 与 `[ptr+mid, ptr+len)` 不相交（mid 相同，区间互补）；
    // 3. 两段的长度之和 == 原长度，不越界；
    // 4. 原 `v` 的可变借用被 `from_raw_parts_mut` 消耗，不会同时存在重叠的可变引用。
    unsafe {
        (
            std::slice::from_raw_parts_mut(ptr, mid),
            std::slice::from_raw_parts_mut(ptr.add(mid), len - mid),
        )
    }
}

// --------------------------------------------
// 2.3 不检查 UTF-8 的字符串构造
// --------------------------------------------

fn section_2_3_unsafe_fn() {
    println!();
    println!("2.3 from_utf8_unchecked：用承诺换性能");
    println!("------------------------------------------------------------");

    /*
    `std::str::from_utf8` 会逐字节校验 UTF-8，失败返回 Result。
    `from_utf8_unchecked` 直接构造 `&str`，不校验——
    它假设你已经验证过内容。这是 unsafe fn，因为传入非法 UTF-8
    会立刻产生 UB（`&str` 的类型不变量被破坏，后续任何字符串操作都可能越界）。
    */

    let bytes: Vec<u8> = vec![72, 101, 108, 108, 111]; // "Hello"

    // SAFETY: 这些字节是从 ASCII 字面量来的，必然是合法 UTF-8
    let s = unsafe { std::str::from_utf8_unchecked(&bytes) };
    println!("from_utf8_unchecked = {}", s);

    // 对照：safe 版本
    match std::str::from_utf8(&bytes) {
        Ok(v) => println!("from_utf8 (safe)    = {}", v),
        Err(e) => println!("非法 UTF-8: {}", e),
    }
}

// --------------------------------------------
// 2.4 union：unsafe 的第 5 项能力
// --------------------------------------------

fn section_2_4_union() {
    println!();
    println!("2.4 union：读字段必须 unsafe");
    println!("------------------------------------------------------------");

    /*
    `union` 的所有字段共享同一块内存，大小 = 最大字段的大小。
    写入任一字段是安全的，**读取**则是 unsafe——
    因为编译器无法知道当前活跃的是哪个字段，读错就是类型混淆（type punning）。

    典型用途：与 C 的 union 对接、位级操作。
    日常业务代码几乎用不到，能不用就不用。
    */

    let u = IntOrFloat { i: 0x3F800000 }; // IEEE 754: 1.0f32 的位模式

    // SAFETY: 刚写入的是 `i` 字段（u32），这里读的也是 `i`，字段匹配
    let as_int = unsafe { u.i };
    println!("以 u32 读 = {} (0x{:X})", as_int, as_int);

    // SAFETY: 按 IEEE 754 规范，0x3F800000 正是 1.0f32 的位模式；
    //         读 f32 字段是有意的类型双关，位模式有效
    let as_float = unsafe { u.f };
    // 用 {:?} 而不是 {}：Display 会把 1.0f32 打成 "1"，Debug 才显示 "1.0"
    println!("以 f32 读 = {:?}", as_float);
}

/// 同一块内存，两种解释
#[repr(C)]
union IntOrFloat {
    i: u32,
    f: f32,
}

// --------------------------------------------
// 2.5 安全抽象：unsafe 的正确用法
// --------------------------------------------

fn section_2_5_safe_abstraction() {
    println!();
    println!("2.5 安全抽象：unsafe 藏在内，safe 露在外");
    println!("------------------------------------------------------------");

    /*
    【原则】unsafe 的边界应该在**模块内部**，而不是 API 表面。
    对外的签名是 `fn`，调用者完全不需要知道里面有 unsafe。

    标准库全是这么做的：
        Vec::push       → safe 签名，内部 unsafe 管理容量与写入
        String::from    → safe 签名，内部 unsafe 处理字节
        Rc::clone       → safe 签名，内部 unsafe 操作引用计数

    【为什么不能反过来】
    如果对外暴露 `unsafe fn`，每个调用点都要写 SAFETY 注释——
    证明责任被摊到 N 个地方，而正确做法是集中在 1 个地方一次证明清楚。
    */

    let mut counter = Counter::new();
    counter.add();
    counter.add();
    counter.add();
    println!("Counter 计数 = {}", counter.get());
    // 调用者完全看不到 unsafe，也不可能用错
}

/// 用裸指针实现一个极简计数器
///
/// 这是"安全抽象"的最小示例：
/// - 内部用 `*mut u32` 手工管理
/// - 对外全部是 safe 方法
/// - unsafe 只出现在 `add` 与 `Drop` 内部，且都带 SAFETY 说明
struct Counter {
    ptr: *mut u32,
}

impl Counter {
    /// 创建计数器
    ///
    /// `Box::into_raw` 把堆上的 u32 转成裸指针。所有权现在由我们手工管理，
    /// 因此**必须**在 `Drop` 里 `Box::from_raw` 收回来，否则内存泄漏。
    fn new() -> Self {
        let boxed = Box::new(0u32);
        Counter {
            ptr: Box::into_raw(boxed),
        }
    }

    /// 计数 +1（safe 接口）
    ///
    /// 调用者无法制造 UB：指针的有效性由结构体自身保证。
    fn add(&mut self) {
        // SAFETY: ptr 由 new() 通过 Box::into_raw 创建，始终非空且对齐；
        //         只要 Counter 未被 drop，该指针就一直有效；
        //         这里通过 &mut self 获得独占访问，不会与其他引用冲突。
        unsafe {
            *self.ptr += 1;
        }
    }

    /// 读取当前计数（safe 接口）
    fn get(&self) -> u32 {
        // SAFETY: 同 add()，ptr 有效且对齐
        unsafe { *self.ptr }
    }
}

impl Drop for Counter {
    fn drop(&mut self) {
        // SAFETY: ptr 来自 Box::into_raw，且只在这里被消费一次；
        //         drop 只会执行一次，不会 double free
        unsafe {
            drop(Box::from_raw(self.ptr));
        }
    }
}

// ============================================
// 第三部分：深入理解（可选）
// ============================================

/*
如果你想知道"为什么"，读这一节。觉得难可以跳过，不影响后续学习。

【1】"未定义行为（UB）"到底是什么意思
    不是"程序崩溃"，而是"编译器可以做任何假设"。
    UB 一旦发生，后果包括但不限于：结果错误、内存泄漏、静默数据损坏、
    程序在调试版能跑而在 release 版崩溃（优化器假设 UB 不会发生，
    会直接删掉它认为"不可能"的分支）。

    unsafe 代码的责任是**确保 UB 永不发生**，而不是"确保不崩溃"。

    五种经典 UB：
      - 解引用悬垂指针
      - 越界读写
      - 同时存在两个 &mut 指向同一数据
      - 读取未初始化内存
      - 破坏类型不变量（如构造非法 &str）

【2】为什么 `get_unchecked` 不是"更快"的默认选择
    它省掉一次 `idx < len` 比较——在热循环里可能有意义，
    但在 99% 的业务代码里，一次比较的开销远小于一次 UB 的代价。
    而且现代 CPU 的分支预测让边界检查几乎免费。
    经验法则：**先 profile，证明这里是瓶颈，再考虑 unsafe。**
    Rust 社区的说法："unsafe for performance" 通常是没有测量的臆想。

【3】Miri：把 UB 检测出来
    `rustup component add miri`
    `cargo +nightly miri test`
    Miri 是一个解释器，能在运行测试时检测越界、悬垂、未初始化读取、数据竞争。
    **任何 unsafe 代码合入前都应该跑一遍 Miri。** 它比 code review 可靠得多。

【4】为什么 unsafe fn 内部还要再写 unsafe 块（2024 edition）
    Rust 2024 起，`unsafe fn` 体内的 unsafe 操作也需要显式块。
    原因：整个函数体都 unsafe 时，reviewer 无法区分"哪几行真的危险"和
    "哪几行只是碰巧在 unsafe fn 里"，SAFETY 注释会变得又长又空泛。
    显式分块迫使人定位危险点。

【5】`#[repr(C)]` 与 FFI
    结构体默认的内存布局是"编译器自由重排"（`repr(Rust)`），
    与 C 对接必须标 `#[repr(C)]` 才能保证字段顺序与对齐一致。
    `union` 也一样 —— 本文件里的 `IntOrFloat` 加了 `#[repr(C)]`。

【6】Send / Sync 是 unsafe trait
    它们没有方法，是"标记 trait"。手动 `unsafe impl Send for X` 等于向编译器保证
    "把 X 移到别的线程是安全的"。这个保证一旦写错，
    编译器就再也无法帮你发现数据竞争——这是最难查的 bug 类型。
    99% 的情况应该让编译器自动推导，不要手写。
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

/*
// --------------------------------------------
// 4.1 错误：以为 unsafe 能让可变借用共存
// --------------------------------------------
❌ 错误代码：
```rust
let mut v = vec![1, 2, 3];
let a = &mut v[0];
unsafe {
    let b = &mut v[0];   // 想绕过借用检查
    *a += 1;
    *b += 1;
}
```

编译器错误信息：
```
error[E0499]: cannot borrow `v` as mutable more than once at a time
 --> src/main.rs:4:17
  |
3 | let a = &mut v[0];
  |         - first mutable borrow occurs here
4 |     let b = &mut v[0];
  |             ^ second mutable borrow occurs here
```

原因：借用检查在 type check 阶段完成，与 unsafe 无关。

✅ 正确代码：真的需要就别用引用，退到裸指针（并自己担保不重叠）
```rust
let p: *mut i32 = v.as_mut_ptr();
// SAFETY: 两次写入指向同一位置，但写的是同一个有效对象，无别名冲突
unsafe { *p += 1; *p += 1; }
```
⚠️ 注意：同时持有两个 `&mut` 指向同一数据**本身就是 UB**，
即使编译通过。改成裸指针只是让编译器闭嘴，不是让它变安全。

记忆技巧：**unsafe 解除的是"编译器查不了的检查"，不是"所有检查"。**


// --------------------------------------------
// 4.2 错误：unsafe 块写得太宽
// --------------------------------------------
❌ 错误代码：
```rust
fn process(v: &mut [i32]) -> i32 {
    unsafe {
        let len = v.len();          // 这里根本不需要 unsafe
        let a = v[0];               // 安全操作
        let b = *v.as_mut_ptr().add(1);  // 只有这一行需要 unsafe
        a + b + len as i32
    }
}
```

为什么是问题：不报编译错误，但**审查失效**。
reviewer 看到一个大 unsafe 块，无法判断危险点在哪，SAFETY 注释也只能写得笼统。

✅ 正确代码：把 unsafe 缩到最小
```rust
fn process(v: &mut [i32]) -> i32 {
    let len = v.len();
    let a = v[0];
    // SAFETY: v.len() >= 2（由调用约定保证），add(1) 在界内
    let b = unsafe { *v.as_mut_ptr().add(1) };
    a + b + len as i32
}
```

记忆技巧：**unsafe 块应该小到"一眼能看完 + 一句 SAFETY 能说清"。**


// --------------------------------------------
// 4.3 错误：返回裸指针却不管生命周期
// --------------------------------------------
❌ 错误代码：
```rust
fn get_ptr() -> *const i32 {
    let x = 42;
    &x as *const i32      // x 在函数结束时被释放
}

fn main() {
    let p = get_ptr();
    unsafe { println!("{}", *p); }   // 悬垂指针
}
```

编译器错误信息：
```
warning: function cannot return without recursing   ← 不会报！编译器放行了
```
真正的问题是：**编译器不会报错**，运行时可能打印垃圾值或崩溃。
这正是裸指针最危险的地方——它绕过了生命周期检查。

✅ 正确代码：让生命周期显式绑定
```rust
fn get_ptr<'a>(x: &'a i32) -> *const i32 { x as *const i32 }
```

记忆技巧：**看到 `-> *const T` / `-> *mut T` 的返回类型，第一反应是"生命周期谁保证？"**


// --------------------------------------------
// 4.4 错误：Box::into_raw 后忘记回收
// --------------------------------------------
❌ 错误代码：
```rust
struct Holder { ptr: *mut u32 }

impl Holder {
    fn new() -> Self { Holder { ptr: Box::into_raw(Box::new(0)) } }
    // 没有 impl Drop
}
```

为什么是问题：不报编译错误，但**内存泄漏**。
`Box::into_raw` 是"放弃所有权"，必须有人用 `Box::from_raw` 接回来。
凡是 `into_raw` 都必须有配对的 `from_raw`。

✅ 正确代码：见本文件 §2.5 的 `Counter`，`Drop` 里调用 `Box::from_raw`。

记忆技巧：**`into_raw` / `from_raw` 是一对，写一个就想另一个。**


// --------------------------------------------
// 4.5 错误：把非法 UTF-8 传给 from_utf8_unchecked
// --------------------------------------------
❌ 错误代码：
```rust
let bytes = vec![0xFF, 0xFE];   // 非法 UTF-8
let s = unsafe { std::str::from_utf8_unchecked(&bytes) };
println!("{}", s.len());        // 已经 UB，后续行为不可预测
```

为什么是问题：**没有任何编译期或运行期报错**。
`&str` 的类型不变量（"内容必须是合法 UTF-8"）被破坏后，
任何字符串操作（`chars()`、切片、`len()` 的语义）都建立在错误前提上。

✅ 正确代码：不确定就校验
```rust
match std::str::from_utf8(&bytes) {
    Ok(s) => println!("{}", s),
    Err(e) => eprintln!("非法 UTF-8: {}", e),
}
```

记忆技巧：**名字里带 `_unchecked` 的函数，都在向你索要一个"你已经验证过"的承诺。**


// --------------------------------------------
// 4.6 错误：手写 unsafe impl Send
// --------------------------------------------
❌ 错误代码：
```rust
struct MyBox(*mut u8);

unsafe impl Send for MyBox {}   // "我保证它能跨线程"

fn main() {
    let b = MyBox(Box::into_raw(Box::new(0)));
    std::thread::spawn(move || {
        unsafe { *b.0 += 1; }    // 与主线程的访问竞争
    });
}
```

为什么是问题：**编译通过，但是数据竞争**。
`unsafe impl Send` 是把编译器唯一的检测手段关掉了，而数据竞争的后果是
不可复现的随机错误——最难查的一类 bug。

✅ 正确代码：改用 `Arc<Mutex<u32>>`，让编译器自动推导 Send/Sync
```rust
let v = std::sync::Arc::new(std::sync::Mutex::new(0u32));
```

记忆技巧：**`unsafe impl Send/Sync` 是红线。没有三年并发经验不要碰。**


// --------------------------------------------
// 4.7 错误：以为 unsafe 能提升性能
// --------------------------------------------
❌ 错误代码：
```rust
// "优化"：把安全索引换成 unchecked
for i in 0..v.len() {
    total += unsafe { *v.get_unchecked(i) };   // 省掉边界检查
}
```

为什么是问题：
1. 现代 CPU 对边界检查有分支预测，`v[i]` 的额外开销常在 1% 以内
2. 一旦 `i` 因逻辑错误越界，就是 UB，而不是 panic
3. 没有 profile 就做这种"优化"，是在用正确性换一个不存在的瓶颈

✅ 正确代码：先 profile，再决定
```bash
cargo build --release
cargo install flamegraph && cargo flamegraph
# 只有 get_unchecked 出现在火焰图顶部 5% 时，才值得考虑
```

记忆技巧：**没有火焰图支撑的 unsafe，都是负优化。**
*/

// ============================================
// 第五部分：函数/宏卡片速查
// ============================================

/*
【关键字：unsafe】

语法：
  unsafe { ... }                    —— unsafe 块
  unsafe fn name(..) { .. }         —— unsafe 函数
  unsafe impl Trait for Type {}     —— unsafe trait 实现
  unsafe trait Name { .. }          —— 定义 unsafe trait
解锁能力：仅五项（裸指针解引用 / 调用 unsafe fn / static mut / unsafe trait / union 字段）
不解除：借用检查、类型检查、生命周期检查、可变性规则
────────────────────────────────────

【类型：*const T / *mut T】

语法：let p: *const T = &x;   let q: *mut T = &mut x;
创建：**安全**（不碰内存）
解引用：**unsafe**
所有权：无（编译器不追踪，不自动释放）
示例：
```rust
let mut x = 1;
let p = &mut x as *mut i32;
unsafe { *p = 2; }
```
────────────────────────────────────

【函数：std::slice::from_raw_parts_mut】

语法：unsafe fn from_raw_parts_mut<'a, T>(ptr: *mut T, len: usize) -> &'a mut [T]
参数：
  - ptr: 指向首元素的裸指针，必须非空、对齐、指向 len 个有效元素
  - len: 元素个数
返回：&'a mut [T]，生命周期由调用者推定
前提（违反即 UB）：ptr 有效、内存未被别名可变借用、总大小不超 isize::MAX
示例：
```rust
let s = unsafe { std::slice::from_raw_parts_mut(p, n) };
```
────────────────────────────────────

【方法：as_mut_ptr / as_ptr / add / offset】

语法：slice.as_mut_ptr() -> *mut T ； ptr.add(n) -> *mut T
参数：n 为**元素个数**（不是字节数）
返回：新指针
注意：add(n) 在 n 处不计算中间指针，比 offset 更不易触发 UB
示例：
```rust
let p = v.as_mut_ptr();
let q = unsafe { p.add(3) };   // 第 4 个元素
```
────────────────────────────────────

【函数：Box::into_raw / Box::from_raw】

语法：Box::into_raw(b: Box<T>) -> *mut T
      unsafe fn Box::from_raw(p: *mut T) -> Box<T>
作用：放弃 / 收回堆所有权，手工管理内存的入口
注意：必须严格配对，否则内存泄漏或 double free
示例：
```rust
let p = Box::into_raw(Box::new(0u32));
unsafe { drop(Box::from_raw(p)); }
```
────────────────────────────────────

【函数：str::from_utf8_unchecked】

语法：unsafe fn from_utf8_unchecked(v: &[u8]) -> &str
参数：v 必须是合法 UTF-8
返回：&str（无校验）
前提（违反即 UB）：字节序列是合法 UTF-8
示例：
```rust
let s = unsafe { std::str::from_utf8_unchecked(b"hi") };
```
────────────────────────────────────

【方法：get_unchecked / get_unchecked_mut】

语法：unsafe fn get_unchecked(&self, i: usize) -> &T
参数：i 必须 < len
返回：&T（无边界检查）
前提（违反即 UB）：i 在界内
示例：
```rust
let x = unsafe { v.get_unchecked(0) };
```
────────────────────────────────────

【属性：#[repr(C)]】

语法：#[repr(C)] struct S { .. } ； #[repr(C)] union U { .. }
作用：强制 C 风格内存布局（字段顺序 + 对齐规则）
用途：FFI、union、需要稳定布局的场景
注意：默认 repr(Rust) 允许编译器自由重排字段，不可用于 FFI
────────────────────────────────────

【工具：Miri】

用法：rustup component add miri && cargo +nightly miri test
作用：解释执行并检测 UB（越界、悬垂、未初始化读取、数据竞争）
注意：只覆盖测试跑到的路径；不替代 review，但比 review 更可靠
*/

// ============================================
// 第六部分：练习题（不提供答案）
// ============================================

/*
【练习 1：基础 —— 找出反例】
要求：
  下面这段代码能编译通过，但包含 UB。指出哪一行、为什么、后果是什么。
    let mut v = vec![1, 2, 3];
    let p = v.as_mut_ptr();
    v.push(4);                       // ← 关键
    unsafe { *p = 99; }
提示：
  - push 可能导致重新分配，旧指针失效
  - 这是"悬垂指针"最典型的伪装形态：它的来源看起来完全合法
  - 思考：如果把 push 换成 v[0] = 10，还是 UB 吗？为什么？

【练习 2：基础 —— 写一个安全的 peek 函数】
要求：
  - `fn peek(s: &[i32], i: usize) -> Option<&i32>`，用 `get_unchecked` 实现
  - 必须**不 panic**，越界返回 None
提示：
  - 先做 `if i >= s.len() { return None; }`，再做 unsafe 解引用
  - 写出你的 SAFETY 注释。注意：这里的保证来自**你自己刚做的检查**
注意：
  - 这个实现比 `s.get(i)` 更慢（多一次比较），作为练习可以，实战中没必要

【练习 3：进阶 —— 手写 `split_first_mut`】
要求：
  - `fn split_first_mut<T>(s: &mut [T]) -> Option<(&mut T, &mut [T])>`
  - 空切片返回 None
  - 用 `from_raw_parts_mut` 实现，不能直接返回 `(&mut s[0], &mut s[1..])`
    （先试试直接写会不会编译失败，理解为什么）
提示：
  - 空切片的 `as_mut_ptr()` 是非空的悬垂指针，构造 0 长度切片是合法的
  - 你的 SAFETY 注释要覆盖两点：指针有效、两段不重叠

【练习 4：进阶 —— 给 Counter 加上 `Send` 判定】
要求：
  - 不写 `unsafe impl Send`，尝试让 `Counter` 能跨线程使用
  - 做到后再解释：为什么原版 `Counter` 自动就不是 `Send`？
提示：
  - 裸指针 `*mut T` 既不实现 Send 也不实现 Sync（故意的）
  - 换成 `Arc<Mutex<u32>>` 后能跨线程，代价是什么？
注意：
  - 这题的正确答案是"不用 unsafe impl"。如果你写出来了，说明思路需要修正

【练习 5：高阶 —— Miri 验证】
要求：
  - 安装 Miri，对一个含 unsafe 的小 Cargo 项目运行 `cargo +nightly miri test`
  - 故意写一个越界 `get_unchecked`，看 Miri 报什么
  - 记录报错原文到日志里
提示：
  - Miri 只在 nightly 可用
  - 它报出的错误会精确到"读了多少字节、地址在哪、为什么越界"
注意：
  - 这是本文件最有价值的一题。Miri 的输出会彻底改变你对 UB 的直觉

【练习 6：综合 —— 审计现有代码】
要求：
  - 用 `rg "unsafe" todo-api/src` 等命令检查自己所有项目里有没有 unsafe
  - 若有，逐个检查：有没有 SAFETY 注释？块是否最小？能否改成 safe？
  - 把结论写进日志
提示：
  - 大概率一个都没有——那么这个练习的正确答案是"确认零 unsafe，这是好事"
  - 面试中"我的项目里零 unsafe，因为没有一个场景需要"是加分回答，
    比"我用了 unsafe 做优化"更可信
*/

// ============================================
// 文件结束
// ============================================
