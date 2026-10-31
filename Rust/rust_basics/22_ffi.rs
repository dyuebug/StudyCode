// ============================================
// FFI（外部函数接口）- 超详细注释版
// 日期：2026-10-29（第30天）
// 难度：⭐⭐⭐ 高级
// ============================================

/*
【核心概念】简要说明

是什么？
- FFI = Foreign Function Interface，让 Rust 调用其他语言（主要是 C）的函数，
  以及让别人调用 Rust 的函数。
- 它是 Rust 唯一的"跨语言 ABI 通道"，也是 Rust 能嵌入操作系统、
  被 Python/Go/Node 调用的根本原因。

为什么需要？
- 复用庞大的 C 生态（OpenSSL、SQLite、zlib、libcurl、GPU 驱动……）
- 高性能库先写核心，再用 Rust 包一层安全 API（rustls、ripgrep 的 regex 引擎都这么干）
- 把 Rust 当成 C 的替代品嵌入现有系统（逐步替换，而不是重写）

核心特点？
- **每一次 FFI 调用都是 unsafe**：编译器无法验证另一侧语言的类型、生命周期、线程安全
- 数据必须用 **C ABI** 表述：`#[repr(C)]`、`extern "C"`、NUL 结尾字符串
- 两侧的内存管理规则完全不同：谁分配、谁释放必须写清楚
- panic **绝不能**穿过 FFI 边界（Rust 1.81+ 会直接 abort，而不是 UB）

Rust 特色？
- 与传统 C 项目对比：C 里调用任何东西都是 FFI，安全靠约定；
  Rust 把 FFI 隔离成显式的 `extern` 块 + `unsafe` 调用点，
  其余代码仍受编译器保护
- 与 Go 对比：Go 的 cgo 有固定开销（每次调用切换栈），
  Rust 的 FFI 是零开销的直接调用
- 与 Python/Java 的 JNI 对比：那两者需要一层 VM 桥接，Rust 是原生 ABI
*/

// ============================================
// 导入
// ============================================

// `CString` / `CStr`：跨语言的字符串容器
use std::ffi::{CStr, CString};

// `c_char` / `c_int`：C 的基础类型别名
//
// ⚠️ 为什么不用 i8 / i32 硬写？
//    `c_char` 在 x86 上是 i8，在 ARM Linux 上是 u8。
//    写死 i8 在 ARM 上会得到类型不匹配，或者更糟——默默通过然后行为不对。
//    这类别名的作用就是让代码与平台无关。
//
// 注：`std::os::raw::*` 是旧路径，新代码推荐 `core::ffi::*`（1.64+），
//     两者当前是同一批类型别名。这里用 os::raw 因为它在老代码里更常见。
use std::os::raw::{c_char, c_int};

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

fn main() {
    println!("============================================================");
    println!("第一部分：核心概念详解");
    println!("============================================================");
    section_1_1_why_unsafe();
    section_1_2_abi();

    println!();
    println!("============================================================");
    println!("第二部分：基础操作实践");
    println!("============================================================");
    section_2_1_call_c();
    section_2_2_strings_across_boundary();
    section_2_3_export_to_c();
    section_2_4_repr_c_layout();
    section_2_5_callback();

    println!();
    println!("============================================================");
    println!("第三部分：深入理解（概念，无需运行）");
    println!("============================================================");
    println!("见文件末尾注释：符号名修饰 / 谁分配谁释放 / panic 为什么不能跨界");
}

// --------------------------------------------
// 1.1 为什么 FFI 必然 unsafe
// --------------------------------------------

fn section_1_1_why_unsafe() {
    println!();
    println!("1.1 为什么每一次 FFI 调用都是 unsafe");
    println!("------------------------------------------------------------");

    /*
    Rust 编译器能证明"安全"，靠的是它**看得见**代码。
    跨过 FFI 边界的函数，它一个字都看不见：

    - 那个 C 函数会不会返回空指针？
    - 它会不会保留传入的指针，等你释放之后再使用？
    - 它会不会在多线程里被同时调用，而内部有静态缓冲区？
    - 它要求的长度参数是字节数还是元素个数？

    这些问题没有任何签名能表达，所以 Rust 的答案很干脆：
    **声明阶段就标成 unsafe，把证明责任交给写 unsafe 块的人。**

    对照一下：
      Rust 内部调用   → 编译器检查类型、生命周期、借用 → 可 safe
      FFI 调用        → 编译器只知道"有个函数符合 C 调用约定" → 必须 unsafe

    这不是保守，是诚实：**能证明的才敢叫 safe。**
    */

    println!("编译器能验证 Rust 侧的类型，但无法验证 C 侧的行为");
    println!("→ 所以 extern 声明出来的函数一律 unsafe 调用");
}

// --------------------------------------------
// 1.2 ABI：extern "C" 到底指什么
// --------------------------------------------

fn section_1_2_abi() {
    println!();
    println!("1.2 ABI：extern \"C\" 约束的是什么");
    println!("------------------------------------------------------------");

    /*
    ABI（Application Binary Interface）是一组**二进制层面的约定**：

      1. 参数怎么传：用哪些寄存器、按什么顺序、栈上还是寄存器
      2. 返回值怎么给：寄存器 / 隐藏的第一个指针参数
      3. 谁负责清理栈
      4. 名字怎么修饰（name mangling）
      5. 结构体字段的内存布局

    Rust 默认的 ABI 是 `extern "Rust"`，它**没有稳定保证**，编译器可以随时改。
    `extern "C"` 用的是平台上的 C ABI，几十年没变，是跨语言的通用语。

    所以：
      - 调 C 函数 → `extern "C" { fn foo(); }`
      - 给 C 调的函数 → `#[no_mangle] pub extern "C" fn foo()`
      - 跨语言传结构体 → `#[repr(C)]`

    三者缺一不可。少一个，你就在赌编译器的实现细节。

    【版本提示】
    Rust 2024 edition 起，`extern` 块本身需要写 `unsafe`：
        unsafe extern "C" { fn abs(i: c_int) -> c_int; }
    因为"声明一个外部函数的签名"这件事本身就是个承诺。
    本文件用 edition 2021 编译，仍可写 `extern "C" {}`。
    */

    println!("extern \"C\" = 用 C 的二进制调用约定");
    println!("#[repr(C)]  = 用 C 的内存布局约定");
    println!("#[no_mangle] = 不要给符号名做修饰");
}

// ============================================
// 第二部分：基础操作实践
// ============================================

// 声明 C 标准库函数
//
// 【声明 ≠ 实现】这里没有任何函数体，编译器只会生成一个外部符号引用，
// 具体地址在链接阶段由 C 运行库填上。
//
// 选 abs / strlen 是因为它们在 C 标准库里必定存在，无需额外链接参数。
// 换成第三方库（如 SQLite）就需要 `#[link(name = "sqlite3")]` 或 build.rs。
extern "C" {
    /// C 标准库的 abs，参数与返回值都是 int（对应 Rust 的 c_int = i32）
    fn abs(i: c_int) -> c_int;

    /// C 标准库的 strlen，参数是 NUL 结尾字符串的指针，返回**字节数**（不含 NUL）
    fn strlen(s: *const c_char) -> usize;
}

// --------------------------------------------
// 2.1 调用 C 函数
// --------------------------------------------

fn section_2_1_call_c() {
    println!();
    println!("2.1 调用 C 标准库函数");
    println!("------------------------------------------------------------");

    // SAFETY: abs 是纯函数，接受任意 i32，不访问指针、不持有状态、不分配内存。
    //         唯一的边界情况是 i32::MIN（绝对值溢出），这里传入 -42，不触发。
    let v = unsafe { abs(-42) };
    println!("abs(-42) = {}", v);

    // 手动构造一个 NUL 结尾的字节数组
    // 注意结尾的 \0 —— strlen 靠它确定长度，缺了它会一直往后读，直到撞上随机内存里的 0
    let c_str: &[u8] = b"hello ffi\0";

    // SAFETY: c_str 是以 NUL 结尾的字节序列，指针有效，生命周期覆盖本次调用
    let len = unsafe { strlen(c_str.as_ptr() as *const c_char) };
    println!("strlen(\"hello ffi\") = {}", len);
    println!("Rust 侧 len()       = {}", "hello ffi".len());
}

// --------------------------------------------
// 2.2 字符串跨边界：CString / CStr
// --------------------------------------------

fn section_2_2_strings_across_boundary() {
    println!();
    println!("2.2 字符串跨边界：CString 与 CStr");
    println!("------------------------------------------------------------");

    /*
    【核心差异】
      Rust `String` / `&str`：
        - 长度单独存储（胖指针 ptr + len）
        - **允许**内含 NUL 字节
        - 不保证 NUL 结尾
      C `char*`：
        - 只有指针，长度靠"读到 NUL 为止"
        - **不允许**内含 NUL（会截断）
        - 必须 NUL 结尾

    所以两个方向各有一个专用类型：
      `CString`  → Rust 字符串转成 C 风格（拥有所有权，drop 时释放）
      `CStr`     → C 风格字符串借用成 Rust 视角（不拥有，不释放）
    */

    // Rust → C
    let original = "你好 FFI";

    // CString::new 会检查内容，含 NUL 就返回 Err（因为 C 侧会被截断，语义已变）
    let c_string = CString::new(original).expect("字符串中不能含 NUL 字节");

    println!("CString 字节数（含结尾 NUL）= {}", c_string.as_bytes_with_nul().len());

    // SAFETY: ptr 来自 c_string，c_string 在本次调用期间存活且未被修改
    let byte_len = unsafe { strlen(c_string.as_ptr()) };
    println!("C 侧 strlen（字节数，不含 NUL）= {}", byte_len);
    println!("Rust 侧 .len()（字节数）        = {}", original.len());
    println!("Rust 侧 .chars().count()（字符数）= {}", original.chars().count());

    // C → Rust
    // SAFETY: c_string.as_ptr() 指向以 NUL 结尾的合法字符串；
    //         CStr::from_ptr 不获取所有权，borrow 的生命周期由我们保证覆盖使用期
    let borrowed: &CStr = unsafe { CStr::from_ptr(c_string.as_ptr()) };

    // to_str() 会校验 UTF-8 —— C 那边给的字节不一定合法
    match borrowed.to_str() {
        Ok(s) => println!("CStr 转回 &str = {}", s),
        Err(e) => println!("不是合法 UTF-8: {}", e),
    }

    // 演示 CString::new 的检查：含 NUL 会失败
    match CString::new("a\0b") {
        Ok(_) => println!("不该走到这里"),
        Err(e) => println!("含 NUL 的字符串被拒绝：{}", e),
    }
}

// --------------------------------------------
// 2.3 导出给 C 调用：no_mangle + extern "C"
// --------------------------------------------

fn section_2_3_export_to_c() {
    println!();
    println!("2.3 把 Rust 函数导出给 C");
    println!("------------------------------------------------------------");

    /*
    【Rust 默认会给符号名"加料"】这叫 name mangling。
    比如 `rust_add` 编译后可能变成 `_ZN7mycrate8rust_add17h3f9a...E`
    这样不同 crate 里的同名函数不会冲突，泛型单态化也有地方编码。
    但 C 找不到这个名字。

    `#[no_mangle]` 的作用就是：**保持原名，不加修饰**。
    `extern "C"` 的作用是：**用 C 的调用约定**（寄存器分配、栈清理）。
    两个都要，缺 C 就找不到或调不对。

    【C 侧代码】（本文件无法编译 C，仅供对照）
        // rustlib.h
        int rust_add(int a, int b);
        void rust_free_string(char *p);

        // main.c
        #include <stdio.h>
        #include "rustlib.h"
        int main(void) {
            printf("%d\n", rust_add(2, 3));      // 5
            return 0;
        }

    编译链路：
        cargo build --release                      # 产出 librustlib.so / .a / .dylib / .dll
        cc main.c -L target/release -l rustlib -o demo
    需要在 Cargo.toml 里指定 crate-type：
        [lib]
        crate-type = ["cdylib"]     # 给 C 动态链接用（.so/.dll）
        # 或 "staticlib"  → .a，静态链接进 C 程序
        # 或 "rlib"       → 只给 Rust 用（默认）
    */

    // 在 Rust 内部同样可以直接调用它（它既是 C ABI，也是普通 Rust 函数）
    println!("rust_add(2, 3) = {}", rust_add(2, 3));

    // 也可以当成函数指针传递 —— 这正好说明 `extern "C" fn` 是一种**类型**
    let f: extern "C" fn(c_int, c_int) -> c_int = rust_add;
    println!("通过 extern \"C\" fn 指针调用 = {}", f(40, 2));
}

/// 导出给 C 的加法函数
///
/// - `#[no_mangle]`：保持符号名 `rust_add` 不加修饰
/// - `pub extern "C"`：使用 C 调用约定
///
/// 注意返回类型只能是 C ABI 能表达的类型。
/// 返回 `String` / `Vec` / `Result` 都是**错的**——C 那边看不懂 Rust 的类型布局。
#[no_mangle]
pub extern "C" fn rust_add(a: c_int, b: c_int) -> c_int {
    a + b
}

// --------------------------------------------
// 2.4 #[repr(C)]：结构体的内存布局契约
// --------------------------------------------

fn section_2_4_repr_c_layout() {
    println!();
    println!("2.4 #[repr(C)]：跨语言传结构体");
    println!("------------------------------------------------------------");

    /*
    【默认布局 repr(Rust)】字段顺序由编译器自由决定，唯一保证是"能装下"。
    目的是减少填充（padding），节省内存。但编译器随时可以改，不能用于 FFI。

    【#[repr(C)]】按 C 的规则排布：
      - 字段顺序 == 声明顺序
      - 每个字段按其自然对齐补齐
      - 总大小按最大对齐向上取整

    C 侧对应：
        typedef struct {
            uint8_t  id;
            uint32_t len;
            uint16_t flag;
        } Packet;
    */

    // SAFETY: 无 unsafe 操作，仅为对齐概念演示
    let size = std::mem::size_of::<Packet>();
    let align = std::mem::align_of::<Packet>();

    println!("Packet 大小 = {} 字节，对齐 = {}", size, align);
    println!("  offset_of!(Packet, id)   = {}", std::mem::offset_of!(Packet, id));
    println!("  offset_of!(Packet, len)  = {}", std::mem::offset_of!(Packet, len));
    println!("  offset_of!(Packet, flag) = {}", std::mem::offset_of!(Packet, flag));

    /*
    为什么 size 是 12 而不是 1+4+2=7？

    按 C 规则排布：
      offset 0   : id    (u8,  1 字节)
      offset 1-3 : 填充  (3 字节，因为 len 要求 4 字节对齐)
      offset 4-7 : len   (u32, 4 字节)
      offset 8-9 : flag  (u16, 2 字节)
      offset 10-11: 尾部填充（总大小必须是 max_align=4 的倍数）

    字段顺序会显著影响大小。同样的三个字段换个顺序：
      struct { u32 len; u16 flag; u8 id; }  →  8 字节
    这就是"结构体字段排序"优化的由来——**顺序改一改，内存省 33%**。
    */
    println!("  （1+4+2 却占 12 字节，多出的 5 字节全是填充）");
}

/// 与 C 侧结构体逐字节兼容的包结构
#[repr(C)]
struct Packet {
    id: u8,
    len: u32,
    flag: u16,
}

// --------------------------------------------
// 2.5 C 风格回调
// --------------------------------------------

fn section_2_5_callback() {
    println!();
    println!("2.5 C 风格回调：extern \"C\" fn 作为参数");
    println!("------------------------------------------------------------");

    /*
    C 里传函数指针的写法：
        void qsort(void *base, size_t n, size_t size,
                   int (*cmp)(const void *, const void *));

    Rust 对应：参数类型写 `extern "C" fn(...) -> ...`。
    **注意不是** `Box<dyn Fn(...)>`——闭包有捕获环境，布局与 C 函数指针不同，
    没法直接传给 C。要传给 C 只能用 `extern "C"` 的普通函数（或子类型的函数指针）。

    想传带状态的逻辑给 C，标准做法是把状态指针作为额外参数
    （C 里叫 `void *userdata`），这是 C 生态的通用模式。
    */

    let doubled = apply_twice(square_c, 5);
    println!("apply_twice(square_c, 5) = {}", doubled);

    let negated = apply_twice(negate_c, 5);
    println!("apply_twice(negate_c, 5) = {}", negated);
}

/// C ABI 的平方函数
extern "C" fn square_c(x: c_int) -> c_int {
    x * x
}

/// C ABI 的取负函数
extern "C" fn negate_c(x: c_int) -> c_int {
    -x
}

/// 模拟 C 库的高阶函数：把 `f` 应用两次
///
/// 签名里的 `extern "C" fn(c_int) -> c_int` 就是 C 眼中的 `int (*)(int)`。
fn apply_twice(f: extern "C" fn(c_int) -> c_int, x: c_int) -> c_int {
    // SAFETY: f 是安全的函数指针类型（不是 unsafe fn），
    //         调用约定由类型系统保证，无需额外 unsafe 块
    f(f(x))
}

// ============================================
// 第三部分：深入理解（可选）
// ============================================

/*
如果你想知道"为什么"，读这一节。觉得难可以跳过，不影响后续学习。

【1】为什么 panic 不能穿过 FFI 边界

    C 没有异常机制，但有栈展开（stack unwinding）。
    Rust 的 panic 默认是 unwind（逐层清理栈然后退出）。
    如果一个 Rust 函数在 extern "C" 里 panic 了，展开要经过 C 的栈帧——
    而 C 的栈帧里没有 Rust 需要的清理信息（landing pad）。
    结果是 UB：析构不执行、栈指针错位、崩溃点完全随机。

    Rust 的处理（1.81 起稳定）：在 `extern "C"` 边界上把 panic 捕获住，
    直接 **abort 整个进程**（`panic in a function that cannot unwind`），
    而不是让它展开出去。

    这也是为什么导出的 C 函数里应该用 `catch_unwind` 自己兜住：
        #[no_mangle]
        pub extern "C" fn safe_entry() -> c_int {
            std::panic::catch_unwind(|| { /* 可能会 panic 的逻辑 */ 1 }).unwrap_or(-1)
        }
    注意 `catch_unwind` 要求参数是 `UnwindSafe`，且只能接住 unwind 型 panic——
    如果编译时设了 `panic = "abort"`，它什么都接不住。

【2】谁分配、谁释放：FFI 的头号 bug 来源

    跨语言传字符串/数组时，最容易写出的 bug 是"在 A 侧分配、在 B 侧用错方式释放"。
        Rust 分配（malloc via GlobalAlloc）→ C 用 free() 释放   ← 可能崩（分配器不同）
        C 分配（malloc）→ Rust 用 Box::from_raw 释放           ← 可能崩

    正确做法二选一：
      (a) **同侧配对**：提供一段导出函数专门负责释放
            #[no_mangle]
            pub extern "C" fn rust_free_string(p: *mut c_char) {
                if p.is_null() { return; }
                // SAFETY: p 必定来自 CString::into_raw（本库唯一的产出来源）
                unsafe { drop(CString::from_raw(p)); }
            }
      (b) **调用方提供缓冲区**：C 那边分配好，把 buf 和 len 传进来让 Rust 填。
          这是 C 生态更常见的做法（避免跨语言释放）。

    【推论】凡是导出给 C 的函数返回 `CString::into_raw()` 得到的指针，
    就**必须**同时导出一个释放函数。这是 API 契约的一部分，要写进头文件注释。

【3】bindgen / cbindgen：别手写绑定

    手写 `extern "C" {}` 声明是体力活，而且容易写错类型（`size_t` 是 u64 还是 u32？
    `enum` 是 int 还是 uint？`bool` 在 C 里占几字节？——答案是 1 字节，
    但很多手写绑定写成 4 字节，在结构体里就错位了）。

    社区方案：
      - `bindgen`：给一个 C 头文件，生成 Rust 的 `extern` 声明（C → Rust）
      - `cbindgen`：给 Rust 源码，生成 C 头文件（Rust → C）
      - `cc` crate：在 build.rs 里编译 C 源码，随 Rust 一起构建

    `bool` 的坑值得单独记：Rust 的 `bool` 与 C 的 `_Bool` 都是 1 字节，
    但 C99 之前的 `bool` 存在过 4 字节的实现。跨编译器时用 `u8` 更稳。

【4】为什么要 `#[repr(C)]` 的第二个理由：字段重排会静默破坏数据

    这不是"可能出错"，而是"错得毫无征兆"。
    Rust 默认布局下，`{ id: u8, len: u32 }` 可能被重排成 `len` 在前。
    C 侧按 `{ u8; u32 }` 读写，两边的 offset 差 3 字节——
    结果不是崩溃，是**读到正确结构里的错误字段**，数据静默损坏。
    这类 bug 通常要跑几周才被发现，且复现率极低。

【5】FFI 的安全边界应该画在哪

    经验法则：**unsafe 只出现在"翻译层"，业务逻辑仍然是 safe Rust。**

        ┌──────────────────────────────┐
        │ safe Rust：业务逻辑、handler  │  ← 99% 的代码
        ├──────────────────────────────┤
        │ unsafe 翻译层：类型转换、校验  │  ← 一个模块，几百行
        ├──────────────────────────────┤
        │ extern "C"：符号声明           │  ← 生成的，不手写
        ├──────────────────────────────┤
        │ C 库                          │
        └──────────────────────────────┘

    这样做的价值：安全审计只需要看中间那一层。
    如果你的 unsafe 撒在业务代码各处，这个项目就已经没有"安全"可言了。

【6】性能真相

    FFI 调用本身几乎没有开销（一次直接跳转，与普通函数调用同阶）。
    真实的开销来源是**数据转换**：
      - `CString::new` 要分配 + 拷贝 + 扫描 NUL
      - `CStr::to_str` 要做 UTF-8 校验
      - 大结构体按值跨边界传递要拷贝

    所以在热路径上，减少跨边界次数远比"优化单次调用"重要。
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

/*
// --------------------------------------------
// 4.1 错误：忘了 #[no_mangle]，C 找不到符号
// --------------------------------------------
❌ 错误代码：
```rust
pub extern "C" fn rust_add(a: i32, b: i32) -> i32 { a + b }
```

C 侧链接错误：
```
undefined reference to `rust_add'
```
用 `nm -D librustlib.so` 看，符号名实际是
`_ZN7mycrate8rust_add17h...E`（被 mangled 了）。

原因：`extern "C"` 只定 ABI，不定符号名；名字修饰仍然生效。

✅ 正确代码：
```rust
#[no_mangle]
pub extern "C" fn rust_add(a: i32, b: i32) -> i32 { a + b }
```

记忆技巧：**`extern "C"` 管"怎么调"，`#[no_mangle]` 管"叫什么"，两个都要。**

Rust 2024 提示：edition 2024 起要写 `#[unsafe(no_mangle)]`，
因为它是编译器无法校验的承诺（承诺符号不冲突）。


// --------------------------------------------
// 4.2 错误：返回 Rust 原生类型给 C
// --------------------------------------------
❌ 错误代码：
```rust
#[no_mangle]
pub extern "C" fn greet(name: &str) -> String {
    format!("Hello, {}", name)
}
```

为什么是问题：**能编译**（Rust 侧看不出问题），但 C 侧完全无法使用：
1. `&str` 是胖指针（ptr + len），C 只按一个指针读，把 len 当成别的
2. `String` 是 `Vec<u8>` 的封装（ptr + cap + len 三个机器字），
   C 按"返回值是 char*"去接，直接读到垃圾

✅ 正确代码：
```rust
#[no_mangle]
pub extern "C" fn greet(name: *const c_char) -> *mut c_char {
    // SAFETY: 调用者保证 name 是 NUL 结尾的合法字符串
    let name = unsafe { CStr::from_ptr(name) }.to_string_lossy();
    // into_raw 把所有权交给调用方 —— 必须同时导出释放函数
    CString::new(format!("Hello, {}", name)).unwrap().into_raw()
}
```
并且必须配套：
```rust
#[no_mangle]
pub extern "C" fn greet_free(p: *mut c_char) {
    if !p.is_null() { unsafe { drop(CString::from_raw(p)); } }
}
```

记忆技巧：**跨 FFI 边界只用 POD 类型**（整数、浮点、裸指针、`#[repr(C)]` 结构体）。
`&str` / `String` / `Vec` / `Option` / `Result` 一律不能直接过界。


// --------------------------------------------
// 4.3 错误：NUL 截断 / 缺 NUL 结尾
// --------------------------------------------
❌ 错误代码：
```rust
let s = "abc\0def";
// 骗过检查
let c = unsafe { CString::from_vec_with_nul_unchecked(s.as_bytes().to_vec()) };
```
C 侧读到的只有 "abc"，`def` 被静默丢弃。

另一个方向：
```rust
let bytes = b"abc";              // 没有 \0
let len = unsafe { strlen(bytes.as_ptr() as *const c_char) };
```
`strlen` 会越过这 3 个字节继续读，直到随机撞上 0——
读到 3 是运气，读到 30000 然后段错误是常态。

✅ 正确代码：
```rust
let c = CString::new("abc").unwrap();          // 自动补 NUL，且检查内含 NUL
let c2 = CString::new("abc").unwrap();
let len = unsafe { strlen(c2.as_ptr()) };      // 3
```

记忆技巧：**C 字符串 = 字节序列 + 一个 NUL。少了是 UB，多了是截断。**
`CString::new` 帮你同时防住这两种，别用 `_unchecked` 变体走捷径。


// --------------------------------------------
// 4.4 错误：into_raw 之后不提供释放函数
// --------------------------------------------
❌ 错误代码：
```rust
#[no_mangle]
pub extern "C" fn get_name() -> *mut c_char {
    CString::new("rust").unwrap().into_raw()
}
// 没有 get_name_free()
```
C 侧拿到指针后，**没有任何合法方式释放它**：
  - 用 `free()` → 分配器可能不同（Rust 用系统 malloc，通常是同一个，但不保证）
  - 不释放 → 每次调用泄漏一份

✅ 正确代码：见 §3【2】的 `rust_free_string`。

记忆技巧：**导出 `*mut T` 的函数，必须成对导出 `*_free(p: *mut T)`。
这是 C API 的礼仪，也是内存安全的唯一收口点。**


// --------------------------------------------
// 4.5 错误：把闭包传给 C
// --------------------------------------------
❌ 错误代码：
```rust
let threshold = 10;
let f = |x: c_int| -> c_int { if x > threshold { x } else { 0 } };
// 想传给 C 风格 API
register_callback(f);   // error: expected fn pointer, found closure
```

编译器错误信息：
```
error[E0308]: mismatched types
  = note: expected fn pointer `extern "C" fn(i32) -> i32`
             found closure `{closure@src/main.rs:3:13}`
  = note: closures can only be coerced to `Fn` traits, not to fn pointers
```

原因：闭包捕获了 `threshold`，它不是一个裸函数地址，
而是一个"函数地址 + 捕获的环境"组合体，布局与 C 函数指针不兼容。

✅ 正确代码：把环境显式传过去（C 的 `void *userdata` 模式）
```rust
extern "C" fn thunk(x: c_int, userdata: *mut c_void) -> c_int {
    let threshold = userdata as *const c_int;
    // SAFETY: 调用方保证 userdata 指向有效的 c_int
    if x > unsafe { *threshold } { x } else { 0 }
}
let threshold = 10;
let userdata = &threshold as *const c_int as *mut c_void;
```
注意：`userdata` 的生命周期必须覆盖回调的整个存活期，
如果它指向栈上的变量，而回调在函数返回后仍被调用 → 悬垂指针。

记忆技巧：**C 没有闭包，只有"函数 + 一个你不许看的指针"。**


// --------------------------------------------
// 4.6 错误：忘了 #[repr(C)]
// --------------------------------------------
❌ 错误代码：
```rust
struct Packet { id: u8, len: u32 }   // 默认 repr(Rust)
#[no_mangle]
pub extern "C" fn handle(p: *const Packet) { /* ... */ }
```

为什么是问题：**编译通过，数据静默错位。**
Rust 默认布局会重排字段以减少 padding，
`id` 和 `len` 的实际 offset 可能与 C 侧假设的完全不同。
症状不是崩溃，而是"读到了正确结构里的错误字段"。

✅ 正确代码：
```rust
#[repr(C)]
struct Packet { id: u8, len: u32 }
```

记忆技巧：**任何跨 FFI 边界的结构体，第一行必须写 `#[repr(C)]`。
不加它，你就在赌编译器的字段排序实现。**


// --------------------------------------------
// 4.7 错误：让 panic 穿过 FFI
// --------------------------------------------
❌ 错误代码：
```rust
#[no_mangle]
pub extern "C" fn maybe_panic(x: i32) -> i32 {
    if x < 0 { panic!("负数不支持"); }   // panic 展开到 C 栈
    x * 2
}
```

运行时输出：
```
thread 'main' panicked at src/main.rs:3:15:
负数不支持
note: panic in a function that cannot unwind
thread caused non-unwinding panic. aborting.
进程被 abort（不是正常退出）
```

为什么是问题：
C 的栈帧没有 Rust 的清理信息（landing pad），展开穿过 C 就是 UB。
Rust 1.81+ 的应对是**直接 abort**——比 UB 好，但你的 C 宿主程序整个死掉，
C 侧写多少 `if (err)` 都救不回来。

✅ 正确代码：自己兜住
```rust
#[no_mangle]
pub extern "C" fn maybe_panic(x: i32) -> i32 {
    match std::panic::catch_unwind(|| {
        if x < 0 { panic!("负数不支持"); }
        x * 2
    }) {
        Ok(v) => v,
        Err(_) => -1,          // 用错误码表达失败，这是 C 的方式
    }
}
```

记忆技巧：**FFI 边界上 panic 必须变成错误码。
Rust 的 unwind 是给 Rust 栈用的，不是给 C 栈用的。**


// --------------------------------------------
// 4.8 错误：以为 extern 声明的函数一定存在
// --------------------------------------------
❌ 错误代码：
```rust
extern "C" {
    fn sqlite3_open(filename: *const c_char, db: *mut *mut c_void) -> c_int;
}
```
只写声明，不告诉链接器去哪找实现。

链接错误：
```
error: linking with `link.exe` failed
  = note: undefined symbol: sqlite3_open
```

✅ 正确代码：用 build.rs 或 Cargo.toml 指定
```rust
// 方式一：属性
#[link(name = "sqlite3")]
extern "C" { fn sqlite3_open(...); }
```
```toml
# 方式二：Cargo.toml（推荐，可指定搜索路径）
```
或者直接用 crate：`rusqlite`（内含 build.rs 自动处理）。

记忆技巧：**`extern` 声明的是"存在这样一个符号"，
链接器负责验证它真的存在。声明成功 ≠ 链接成功。**
*/

// ============================================
// 第五部分：函数/宏卡片速查
// ============================================

/*
【关键字：extern "C" 块】

语法：extern "C" { fn name(args) -> ret; }
作用：声明外部（C ABI）函数，无函数体，链接期解析
调用：必须 `unsafe { name(...) }`
版本：edition 2024 起写作 `unsafe extern "C" { .. }`
示例：
```rust
extern "C" {
    fn abs(i: c_int) -> c_int;
}
let v = unsafe { abs(-1) };
```
────────────────────────────────────

【属性：#[no_mangle]】

语法：#[no_mangle] pub extern "C" fn name(..) {..}
作用：禁止符号名修饰，让 C 能用原名链接
版本：edition 2024 起写作 #[unsafe(no_mangle)]
注意：同一 crate 内符号名必须唯一，否则链接期重复定义
示例：
```rust
#[no_mangle]
pub extern "C" fn rust_add(a: i32, b: i32) -> i32 { a + b }
```
────────────────────────────────────

【属性：#[repr(C)]】

语法：#[repr(C)] struct S {..}  /  #[repr(C)] enum E {..}
作用：按 C 规则排布字段（声明顺序 + 自然对齐 + 尾部补齐）
用途：跨 FFI 结构体、union、需要稳定布局的场景
示例：
```rust
#[repr(C)]
struct Packet { id: u8, len: u32 }   // size = 8, 不是 5
```
────────────────────────────────────

【类型：std::ffi::CString】

语法：CString::new(impl Into<Vec<u8>>) -> Result<CString, NulError>
参数：字节序列，**不能含 NUL**
返回：拥有所有权的 C 风格字符串（末尾自动补 NUL）
所有权：Drop 时自动释放
示例：
```rust
let c = CString::new("hi").unwrap();
let p = c.as_ptr();          // *const c_char
```
────────────────────────────────────

【类型：std::ffi::CStr】

语法：unsafe fn CStr::from_ptr(p: *const c_char) -> &CStr
参数：指向 NUL 结尾字符串的指针，必须有效且以 NUL 结尾
返回：借用的 C 字符串视图（生命周期由调用者担保）
方法：`to_str() -> Result<&str, Utf8Error>`（做 UTF-8 校验）
示例：
```rust
let s = unsafe { CStr::from_ptr(p) }.to_str()?;
```
────────────────────────────────────

【方法：CString::into_raw / from_raw】

语法：CString::into_raw(self) -> *mut c_char
      unsafe fn CString::from_raw(p: *mut c_char) -> CString
作用：放弃 / 收回所有权，跨语言传递字符串的标准手法
注意：**必须成对**，且必须导出配对的 `*_free` 函数给 C 侧
示例：
```rust
let p = CString::new("x").unwrap().into_raw();
unsafe { drop(CString::from_raw(p)); }
```
────────────────────────────────────

【类型：extern "C" fn(..) -> ..】

语法：let f: extern "C" fn(c_int) -> c_int = some_fn;
作用：C 函数指针类型，可直接作为参数传给 C 风格 API
限制：**不能**接闭包（闭包有捕获环境，布局不兼容）
示例：
```rust
fn apply(f: extern "C" fn(i32) -> i32, x: i32) -> i32 { f(x) }
```
────────────────────────────────────

【宏：std::mem::offset_of!】

语法：offset_of!(Type, field) -> usize
作用：编译期得到字段相对结构体起始的字节偏移
用途：验证 `#[repr(C)]` 布局与 C 侧头文件一致
版本：Rust 1.77 稳定
示例：
```rust
assert_eq!(std::mem::offset_of!(Packet, len), 4);
```
────────────────────────────────────

【函数：std::panic::catch_unwind】

语法：catch_unwind<F: FnOnce() -> R + UnwindSafe, R>(f: F) -> Result<R, Box<dyn Any>>
作用：在 FFI 边界内兜住 panic，转成错误码
限制：`panic = "abort"` 编译时无效；参数必须 UnwindSafe
示例：
```rust
let r = catch_unwind(|| { risky() }).unwrap_or(-1);
```
────────────────────────────────────

【类型：std::os::raw::*】

`c_char`  = i8（MSVC/大多数平台）或 u8 —— 用这个，别硬写 i8
`c_int`   = i32
`c_uint`  = u32
`c_long`  = i64（Linux x86_64）/ i32（Windows）
`c_void`  = 空类型，用于 `*mut c_void`
用途：替代 `std::ffi::*` 的旧路径；新代码推荐 `core::ffi::*`
────────────────────────────────────

【工具：bindgen / cbindgen / cc】

bindgen   : C 头文件 → Rust extern 声明
cbindgen  : Rust 源码 → C 头文件
cc crate  : build.rs 里编译 C 源码，随 Rust 一起构建
────────────────────────────────────

【crate-type（Cargo.toml）】

rlib      : 只给 Rust 用（默认）
cdylib    : 动态库，给 C 动态链接（.so / .dll / .dylib）
staticlib : 静态库（.a / .lib），链接进 C 程序
proc-macro: 过程宏

```toml
[lib]
crate-type = ["cdylib", "staticlib"]
```
*/

// ============================================
// 第六部分：练习题（不提供答案）
// ============================================

/*
【练习 1：基础 —— 补全释放函数】
要求：
  下面这个导出函数泄漏内存。写出配套的释放函数，并把注释写清楚。
    #[no_mangle]
    pub extern "C" fn make_greeting(name: *const c_char) -> *mut c_char {
        let name = unsafe { CStr::from_ptr(name) }.to_string_lossy();
        CString::new(format!("Hello, {}", name)).unwrap().into_raw()
    }
提示：
  - 释放函数签名应为 `pub extern "C" fn make_greeting_free(p: *mut c_char)`
  - 必须判空：C 侧可能传 NULL 进来
  - SAFETY 注释要说明"p 必定来自 make_greeting"
注意：
  - 不要在释放函数里用 `free()`——分配器和释放器必须配对

【练习 2：基础 —— 复现字段重排】
要求：
  - 定义 `Packet { id: u8, len: u32, flag: u16 }`
  - 分别用 `#[repr(C)]` 和默认布局打印 `size_of` 与三个 `offset_of!`
  - 记录两者差异，并解释差异从哪来
提示：
  - 默认布局的 offset 在不同 rustc 版本下**可能不同**——这本身就是"不能用"的理由
  - 只打印 `size_of` 不够，offset 才是关键
注意：
  - 不要得出结论"默认布局更省内存所以更好"。省内存和可互操作是两件事

【练习 3：进阶 —— 用 catch_unwind 包住 panic】
要求：
  - 写一个 `#[no_mangle] pub extern "C" fn divide(a: c_int, b: c_int) -> c_int`
  - `b == 0` 时 **不 panic**，返回 `-1`
  - 再写一个**故意 panic** 的版本，用 `catch_unwind` 包住，观察进程不被 abort
提示：
  - 先跑不包 catch_unwind 的版本，看看真实的 abort 输出
  - `catch_unwind` 的返回值是 `Result<R, Box<dyn Any>>`
注意：
  - 如果 Cargo.toml 里写了 `panic = "abort"`，这题做不成——先确认配置

【练习 4：进阶 —— 结构体字段排序优化】
要求：
  - 一个含 `u8, u32, u16, u64, u8` 的 `#[repr(C)]` 结构体
  - 计算当前大小
  - 重排字段（保持 `#[repr(C)]`），使大小最小
  - 报告省了多少字节、省在哪
提示：
  - 规则：大对齐的字段放前面
  - 用 `size_of` + `offset_of!` 验证，不要靠心算
注意：
  - 重排字段会改变**二进制布局**。如果这个结构体已经用于 FFI，
    改顺序等于改协议——C 侧必须同步改

【练习 5：高阶 —— 真的跑一次 C ↔ Rust】
要求：
  - `cargo new --lib rustlib`，`Cargo.toml` 设 `crate-type = ["cdylib"]`
  - 导出一个 `add` 和一个 `greet`（含 free 配对）
  - 写 `main.c`，用 `cc main.c -L target/debug -l rustlib` 编译
  - 运行，验证两个方向都通
提示：
  - Windows 上链接动态库需要 `rustlib.dll` 在 PATH 或同目录
  - `nm -D`（Linux）/ `dumpbin /exports`（Windows）可以验证符号真的导出了
注意：
  - 这题会暴露 4.1（缺 no_mangle）和 4.4（缺 free）两个错误的真实后果

【练习 6：综合 —— 判断该不该用 FFI】
要求：
  - 列出你当前项目里可能需要 FFI 的三个场景（如果没有就假设三个）
  - 对每个场景回答：有没有成熟 crate 可用？自己写绑定的成本是多少？
  - 给出"用 / 不用"的结论和理由
提示：
  - 判断标准：有成熟 crate（如 rusqlite、ring、openssl-sys）就绝不要手写绑定
  - 手写绑定的成本主要在**错误处理映射**和**生命周期断言**，不在写 extern 声明
注意：
  - 正确答案往往全是"用 crate"。FFI 是最后手段，不是能力展示
*/

// ============================================
// 文件结束
// ============================================
