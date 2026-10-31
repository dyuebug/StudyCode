// ============================================================================
//  类型系统进阶：常量泛型 / PhantomData / GAT / Cow / async fn in trait
// ============================================================================
//  日期：2026-11-09（第45天）
//  难度：⭐⭐⭐⭐
//
//  这一篇补的是「会用泛型」和「会设计泛型接口」之间的那段。
//  五个主题的共同点：它们都不是新语法，而是**类型系统能表达什么**的边界。
//
//  运行：rustc --edition 2021 31_type_system_advanced.rs && ./31_type_system_advanced
//
//  注意：本文件全部代码在本机 rustc 1.92.0 上实测编译通过、零警告。
//  第四部分里的错误信息全部是**真实复现**的输出，不是手工编的。
// ============================================================================

use std::borrow::Cow;
use std::future::Future;
use std::marker::PhantomData;

// ============================================================================
//  第一部分：核心概念详解
// ============================================================================
//
//  1.1 常量泛型（const generics）
//      `struct Buffer<const N: usize>`：把**值**而不是**类型**作为泛型参数。
//      `Buffer<4>` 和 `Buffer<8>` 是两个不同的类型，`[u8; N]` 因此能作为字段类型。
//
//  1.2 PhantomData<T>
//      「我确实需要这个类型参数，但它不出现在任何字段里」。
//      不加它 → 编译器报 E0392（见第四部分 4.1）。
//
//  1.3 GAT（Generic Associated Types）
//      `type Item<'a> where Self: 'a`：关联类型自己带生命周期参数。
//      没有 GAT 就写不出「借出自身内部的迭代器」（LendingIterator）。
//
//  1.4 Cow<'a, B> = Clone-on-Write
//      「能借用就借用，需要改才克隆」。函数返回 `Cow<str>` 时，
//      调用方无法假设它一定拥有数据，也无法假设它一定不拥有——所以用 enum 表达。
//
//  1.5 async fn in trait（RPITIT，1.75 稳定）
//      省掉 `#[async_trait]` crate，但**代价是失去 dyn 兼容性**（见第四部分 4.2）。
// ============================================================================

// ---------------------------- 1.1 常量泛型 ----------------------------

#[derive(Debug)]
struct Buffer<const N: usize> {
    data: [u8; N],
}

impl<const N: usize> Buffer<N> {
    fn filled(value: u8) -> Self {
        Self { data: [value; N] }
    }

    /// N 在函数体里是**编译期常量**，可以直接当数组长度、循环上界
    fn capacity(&self) -> usize {
        N
    }

    fn as_slice(&self) -> &[u8] {
        &self.data
    }
}

// ---------------------------- 1.2 PhantomData ----------------------------

/// 类型安全的 ID：`Id<User>` 与 `Id<Order>` 不可互相赋值。
///
/// `raw: u32` 里其实不关心 T，但**语义上**必须区分。
/// 如果不写 `_marker`，编译器会报 E0392：T 从未被使用。
struct Id<T> {
    raw: u32,
    _marker: PhantomData<T>,
}

impl<T> Id<T> {
    fn new(raw: u32) -> Self {
        Self {
            raw,
            _marker: PhantomData,
        }
    }

    fn raw(&self) -> u32 {
        self.raw
    }
}

struct User;
struct Order;

// ---------------------------- 1.3 GAT ----------------------------

/// 「借出自身内部数据的迭代器」。
///
/// 标准 `Iterator` 做不到：`fn next(&mut self) -> Option<&mut Self::Item>`
/// 无法表达「返回的引用活多久」——它必须绑定到 `&mut self` 的借用上。
trait Lender {
    type Item<'a>
    where
        Self: 'a;

    fn next_item(&mut self) -> Option<Self::Item<'_>>;
}

struct ByteWindows {
    data: Vec<u8>,
    pos: usize,
    width: usize,
}

impl Lender for ByteWindows {
    // 关键：impl 里必须**重复** trait 那句 `where Self: 'a`
    type Item<'a>
        = &'a [u8]
    where
        Self: 'a;

    fn next_item(&mut self) -> Option<Self::Item<'_>> {
        // 借用检查在这里是关键：窗口从 self.data 借出，
        // 且必须在下一次 &mut self 之前用完 —— GAT 让这个约束可表达
        let end = self.pos + self.width;
        if end > self.data.len() {
            return None;
        }
        let window = &self.data[self.pos..end];
        self.pos += 1;
        Some(window)
    }
}

// ---------------------------- 1.4 Cow ----------------------------

/// 只在需要时分配：全是小写就借用，否则克隆一份改。
fn normalize_name(input: &str) -> Cow<'_, str> {
    if input.chars().all(|c| !c.is_uppercase()) {
        Cow::Borrowed(input)
    } else {
        Cow::Owned(input.to_lowercase())
    }
}

// ---------------------------- 1.5 async fn in trait ----------------------------

trait Repository {
    /// RPITIT：返回类型由编译器推导，不需要 `#[async_trait]`
    async fn find(&self, id: u32) -> Option<String>;
}

struct MemRepo;

impl Repository for MemRepo {
    async fn find(&self, id: u32) -> Option<String> {
        (id == 1).then(|| "task-1".to_string())
    }
}

async fn describe<R: Repository>(repo: &R, id: u32) -> String {
    match repo.find(id).await {
        Some(name) => format!("找到 {name}"),
        None => format!("id={id} 不存在"),
    }
}

// ============================================================================
//  第二部分：基础实践
// ============================================================================

fn demo_const_generics() {
    println!("--- 1.1 常量泛型 ---");
    let b4 = Buffer::<4>::filled(7);
    let b8 = Buffer::<8>::filled(9);

    println!(
        "Buffer<4>: 容量 {} / 字节数 {} / 内容 {:?}",
        b4.capacity(),
        std::mem::size_of::<Buffer<4>>(),
        b4.as_slice()
    );
    println!(
        "Buffer<8>: 容量 {} / 字节数 {} / 内容 {:?}",
        b8.capacity(),
        std::mem::size_of::<Buffer<8>>(),
        b8.as_slice()
    );
    // Buffer<4> 与 Buffer<8> 是不同类型，下面这行根本编译不过：
    // let _ = b4 == b8;
}

fn demo_phantom() {
    println!("--- 1.2 PhantomData ---");
    let user_id: Id<User> = Id::new(42);
    let order_id: Id<Order> = Id::new(42);
    println!("user_id = {} / order_id = {}", user_id.raw(), order_id.raw());
    // 下面这行是编译错误（E0308），这正是 PhantomData 的价值：
    // let _: Id<User> = order_id;
    println!("两者都是 u32，但类型不同，所以不会互相传错");
}

fn demo_gat() {
    println!("--- 1.3 GAT ---");
    let mut windows = ByteWindows {
        data: b"abcdef".to_vec(),
        pos: 0,
        width: 3,
    };
    let mut collected = Vec::new();
    while let Some(w) = windows.next_item() {
        // 每轮借用都是独立的：上一条窗口在下一次 next_item 前必须用完
        collected.push(String::from_utf8_lossy(w).to_string());
    }
    println!("滑动窗口：{collected:?}");
}

fn demo_cow() {
    println!("--- 1.4 Cow ---");
    for input in ["already-lower", "Has-UPPER"] {
        let out = normalize_name(input);
        // Cow 能告诉你"到底有没有克隆"——这是它的可观测价值
        let kind = match &out {
            Cow::Borrowed(_) => "借用（零分配）",
            Cow::Owned(_) => "拥有（发生了克隆）",
        };
        println!("{input:?} -> {out:?}  {kind}");
    }
}

fn demo_async_trait() {
    println!("--- 1.5 async fn in trait ---");
    let repo = MemRepo;
    // 本文件不依赖任何异步运行时，因此手写一个最小 block_on（见第三部分）
    println!("{}", block_on(describe(&repo, 1)));
    println!("{}", block_on(describe(&repo, 2)));
}

// ============================================================================
//  第三部分：深入理解（可选）
// ============================================================================
//
//  3.1 为什么 `Buffer<4>` 和 `Buffer<8>` 是两个类型？
//      常量泛型走的是**单态化**：编译器为每个 N 生成一份独立的代码。
//      好处是零成本（N 是编译期常量，循环上界可被优化）；
//      代价是**代码膨胀**：N 的每个取值都是一份机器码。
//      所以不要用它做「运行期可变的长度」——那仍然是 Vec 的活。
//
//  3.2 PhantomData 不只是"占位"，它还影响三件事
//      - **方差**：`PhantomData<T>` 让 `Id<T>` 对 T 协变，`PhantomData<fn(T)>` 则逆变
//      - **drop check**：告诉 dropck「我可能在 Drop 里访问 T」
//      - **auto trait**：`PhantomData<Rc<()>>` 会让类型变得 !Send
//      本次只需要第一层（占位），但要知道后面两层存在，否则会在库设计里踩坑。
//
//  3.3 GAT 为什么强制 `where Self: 'a`
//      编译器需要知道「`Item<'a>` 只在 self 活着的区间内可用」。
//      不写这个约束，实现方可能返回一个比 self 活得更久的引用。
//      报错文案很直白：`missing required bound on Item`，并直接给出应该加的子句。
//
//  3.4 Cow 的 ToOwned
//      `Cow<'a, B>` 要求 `B: ToOwned`。`&str` 的 ToOwned 是 String，
//      `&[u8]` 的是 Vec<u8>。所以参数化函数签名时可以用 `Cow<'a, [u8]>` 同时接受两者。
//
//  3.5 RPITIT 与 dyn 的取舍
//      1.75 之后 `async fn` 可以直接写在 trait 里，但它返回的是一个**匿名 Future**，
//      编译器无法为它构造 vtable → 该 trait 立刻失去 dyn 兼容性。
//      需要 `Vec<Box<dyn Trait>>` 时仍然得用 `#[async_trait]`（它把方法签名
//      改写成返回 `Pin<Box<dyn Future>>`）。这是「零成本」与「动态分发」的经典二选一。
// ============================================================================

/// 最小 `block_on`：把 Future 跑完，不需要 tokio。
///
/// 这 25 行把三件平时被运行时藏起来的事摊开了：
///   1. `Pin` —— Future 可能在 poll 之间被移动，`pin!` 保证它不动
///   2. `Waker` —— 唤醒机制，决定 Pending 之后怎么被重新 poll
///   3. `Context` —— poll 需要的另一半参数
fn block_on<F: Future>(fut: F) -> F::Output {
    use std::pin::pin;
    use std::sync::{Arc, Condvar, Mutex};
    use std::task::{Context, Poll, Wake, Waker};

    struct Signal {
        done: Mutex<bool>,
        cv: Condvar,
    }

    impl Wake for Signal {
        fn wake(self: Arc<Self>) {
            self.wake_by_ref();
        }
        fn wake_by_ref(self: &Arc<Self>) {
            *self.done.lock().unwrap() = true;
            self.cv.notify_all();
        }
    }

    let signal = Arc::new(Signal {
        done: Mutex::new(false),
        cv: Condvar::new(),
    });
    let waker = Waker::from(Arc::clone(&signal));
    let mut cx = Context::from_waker(&waker);

    // pin! 把 Future 钉在栈上：之后不能再移动它
    let mut fut = pin!(fut);

    loop {
        match fut.as_mut().poll(&mut cx) {
            Poll::Ready(value) => return value,
            Poll::Pending => {
                // 等待唤醒；期间不占 CPU（对比 4.6 的自旋）
                let mut done = signal.done.lock().unwrap();
                while !*done {
                    done = signal.cv.wait(done).unwrap();
                }
                *done = false;
            }
        }
    }
}

// ============================================================================
//  第四部分：常见错误和陷阱
// ============================================================================

//  4.1 类型参数从未被使用 —— E0392
//
//     struct Marker<T> { count: usize }
//
//     error[E0392]: type parameter `T` is never used
//      --> a.rs:1:15
//       |
//     1 | struct Marker<T> { count: usize }
//       |               ^ unused type parameter
//       |
//       = help: consider removing `T`, referring to it in a field, or using a marker such as `PhantomData`
//
//     编译器直接给出了三种改法。选第三种才是下面这种「类型安全的 ID」的场景。

//  4.2 async fn in trait 不能 dyn —— E0038
//
//     trait Store { async fn save(&self); }
//     fn main() { let _boxed: Box<dyn Store> = Box::new(S); }
//
//     error[E0038]: the trait `Store` is not dyn compatible
//      --> b.rs:4:33
//       |
//     4 | fn main() { let _boxed: Box<dyn Store> = Box::new(S); }
//       |                                 ^^^^^ `Store` is not dyn compatible
//      |
//     note: for a trait to be dyn compatible it needs to allow building a vtable
//      --> b.rs:1:24
//       |
//     1 | trait Store { async fn save(&self); }
//       |       -----            ^^^^ ...because method `save` is `async`
//
//     注意报错是 **note 里指到 async 关键字**，不是指到 Box<dyn>。
//     这就是"编译器比你更清楚为什么"的典型：真因在签名，不在使用处。

//  4.3 GAT 少了 `where Self: 'a` —— 不是 E 编号错误
//
//     trait Lender { type Item<'a>; fn next(&mut self) -> Option<Self::Item<'_>>; }
//
//     error: missing required bound on `Item`
//      --> e.rs:1:16
//       |
//     1 | trait Lender { type Item<'a>; fn next(&mut self) -> Option<Self::Item<'_>>; }
//       |                ^^^^^^^^^^^^^-
//       |                             |
//       |                             help: add the required where clause: `where Self: 'a`
//
//     这是"编译器直接给出修复代码"的例子：照着 help 抄就行。

//  4.4 Cow 不能借用临时值 —— E0515
//
//     fn label() -> Cow<'static, str> { Cow::Borrowed(&String::from("x")) }
//
//     error[E0515]: cannot return value referencing temporary value
//      --> d.rs:2:35
//       |
//     2 | fn label() -> Cow<'static, str> { Cow::Borrowed(&String::from("x")) }
//       |                                   ^^^^^^^^^^^^^^^-----------------^
//       |                                   |              |
//       |                                   |              temporary value created here
//       |                                   returns a value referencing data owned by the current function
//
//     这条错误的迷惑点：`Cow::Borrowed` 看起来"不拥有数据"，所以"应该没问题"。
//     但 `'static` 要求借用活到程序结束，而临时 String 在函数返回时就没了。
//     修法：`Cow::Owned(String::from("x"))`。

//  4.5 常量参数在泛型参数位置要加花括号 —— E0747
//
//     let size = std::mem::size_of::<Buffer<NOPE>>();
//
//     error[E0412]: cannot find type `NOPE` in this scope
//     error[E0747]: unresolved item provided when a constant was expected
//      help: if this generic argument was intended as a const parameter, surround it with braces
//       |
//     7 |     let size = std::mem::size_of::<Buffer<{ NOPE }>>();
//       |                                           +      +
//
//     `Buffer<4>` 里 4 是字面量没问题；但一旦要填**表达式或常量名**，
//     必须写 `Buffer<{ N }>` —— 否则编译器按"类型参数"解析，找不到这个类型。

//  4.6 Windows 上 `rustc -o /dev/null` 会莫名失败 —— LNK1107
//
//     error: linking with `link.exe` failed: exit code: 1107
//      = note: nul : fatal error LNK1107: 文件无效或损坏: 无法在 0x10000 处读取
//
//     这是环境坑而不是代码坑：Windows 没有 `/dev/null`，
//     MSVC 链接器把 `nul` 当成目标文件去读。想"只检查不生成产物"用
//     `rustc --emit=metadata` 或 `cargo check`。
//     （本次写材料时就先撞了这条，误以为是代码错误。）

//  4.7 原子类型不能当普通整数读写
//
//     见下一篇 `32_atomics_and_ordering.rs` 4.1：`let v: usize = atomic;`
//     报 E0308，必须用 `load(Ordering::...)`。

// ============================================================================
//  第五部分：函数/宏卡片速查
// ============================================================================
//
//  | 你要做的事                          | 写法                                   |
//  |-------------------------------------|----------------------------------------|
//  | 用值做泛型参数                      | `struct Buf<const N: usize> { d: [u8; N] }` |
//  | 在函数体里当常量用                  | `fn cap(&self) -> usize { N }`         |
//  | 占住一个不落字段的类型参数          | `_m: PhantomData<T>`                   |
//  | 让类型对 T 逆变                      | `_m: PhantomData<fn(T)>`               |
//  | 让类型不是 Send                      | `_m: PhantomData<Rc<()>>`              |
//  | 关联类型带生命周期                   | `type Item<'a> where Self: 'a;`        |
//  | 在 impl 里重复该约束                | `type Item<'a> = &'a [u8] where Self: 'a;` |
//  | 借用优先、必要时克隆                | `fn f(s: &str) -> Cow<'_, str>`        |
//  | 判断 Cow 是否克隆了                 | `matches!(cow, Cow::Borrowed(_))`      |
//  | trait 里直接写 async fn             | `async fn find(&self) -> T;`（1.75+）  |
//  | 把 Future 钉住以便 poll             | `let mut f = std::pin::pin!(fut);`     |
//  | 泛型参数位置放常量表达式            | `Buffer<{ N * 2 }>`                    |
//  | 只做类型检查不生成产物              | `rustc --emit=metadata` / `cargo check` |
//
//  一个判断口诀：
//    「这个参数我只在类型层面用」  → PhantomData
//    「这个参数是个数值」          → const 泛型
//    「关联类型要借出自身」        → GAT
//    「可能借用也可能拥有」        → Cow
//    「要放进 dyn Trait」          → 不能用 RPITIT，回到 #[async_trait]

// ============================================================================
//  第六部分：练习题（不提供答案）
// ============================================================================
//
//  1. 写 `struct Matrix<const R: usize, const C: usize> { cells: [[f64; C]; R] }`，
//     实现 `transpose` 返回 `Matrix<C, R>`。注意泛型参数顺序不能反。
//
//  2. 用 PhantomData 实现 `Meters(f64)` 与 `Feet(f64)` 两个不互通的单位类型，
//     并实现 `Add` 只允许同类相加。再用 `PhantomData<fn(T)>` 版本说明方差差异
//     （提示：试着把它放进 `Box<dyn Fn(&'static str)>` 之类的场景）。
//
//  3. 用 GAT 实现一个 `Splitter`：每次 `next_item()` 返回**下一行**的 `&str`，
//     且不分配内存。先写标准 `Iterator` 版本，观察它为什么编译不过。
//
//  4. 写 `fn sanitize(s: &str) -> Cow<'_, str>`：去掉首尾空白，若无变化则借用。
//     再用 `#[test]` 断言"无空白时零分配"——提示：用 `matches!` 判断变体。
//
//  5. 把 `1.5` 的 `Repository` 改成能放进 `Vec<Box<dyn Repository>>` 的版本。
//     改写后记录：为了 dyn 兼容性你付出了什么（Box 分配？`Pin<Box<dyn Future>>`？）。
//
//  6. 手写 `block_on` 目前只支持"一次唤醒"。给它加一个 sleep 场景：
//     用 `thread::spawn` + `Wake` 在 100 ms 后唤醒，验证它在等待期间不占 CPU。
//
//  7. 复现 4.5 的 E0747：写一个 `const N: usize = 4;`，分别尝试
//     `Buffer<N>` 与 `Buffer<{ N }>`，记录两次报错的区别。
//
//  8. 复现 4.2：把 `async fn` 换成 `fn -> impl Future`（同样 RPITIT），
//     看是否仍然不能 dyn。然后换成 `#[async_trait]`，确认它能 dyn。
//     （说明：`#[async_trait]` 需要外部 crate，本项目未引入，只做推理记录。）

fn main() {
    println!("=== 类型系统进阶 ===\n");
    demo_const_generics();
    println!();
    demo_phantom();
    println!();
    demo_gat();
    println!();
    demo_cow();
    println!();
    demo_async_trait();
}
