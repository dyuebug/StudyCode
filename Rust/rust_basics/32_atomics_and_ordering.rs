// ============================================================================
//  原子类型与内存序：AtomicUsize / Ordering / 自旋锁 / compare_exchange
// ============================================================================
//  日期：2026-11-09（第45天）
//  难度：⭐⭐⭐⭐⭐
//
//  这一篇的核心不是"怎么用原子类型"，而是**你为什么无法用跑程序来验证它**。
//  内存序错误是唯一一类"测试全绿、上线偶发"的 bug，
//  所以本篇的每一处【实测】都只证明"这段代码在本机跑对了"，
//  不证明"它在任何平台任何负载下都对"。
//
//  运行：rustc --edition 2021 32_atomics_and_ordering.rs && ./32_atomics_and_ordering
//  实测环境：x86_64-pc-windows-msvc / rustc 1.92.0
//
//  ⚠ 本篇结论若用于真实项目，请以官方文档的 happens-before 定义为准；
//  本文件是学习材料，不是内存模型的权威来源。
// ============================================================================

use std::cell::UnsafeCell;
use std::hint::spin_loop;
use std::ops::{Deref, DerefMut};
use std::sync::atomic::{AtomicBool, AtomicUsize, Ordering};
use std::sync::Arc;
use std::thread;

// ============================================================================
//  第一部分：核心概念详解
// ============================================================================
//
//  1.1 原子类型解决什么、不解决什么
//      AtomicUsize::fetch_add 保证"读-改-写"三步不被别的线程插进来。
//      它**不**保证你的业务逻辑正确：
//        - 两个原子变量之间不构成事务（不能原子地一起改）
//        - 原子类型不能当普通整数用（`let v: usize = a;` 直接报 E0308）
//
//  1.2 Ordering 五档
//      Relaxed  —— 只保证该操作本身不可分割，不建立任何跨线程顺序
//      Acquire  —— 用在**读**：此后的读写不会被挪到它前面
//      Release  —— 用在**写**：此前的读写不会被挪到它后面
//      AcqRel   —— 读改写操作（compare_exchange / fetch_add）同时要两边
//      SeqCst   —— 在所有线程间存在单一全局顺序，最贵、最省心
//      规则：**Acquire 与 Release 配对**才有同步语义。
//      两边都是 Relaxed → 没有任何同步，只是数字没错而已。
//
//  1.3 compare_exchange：CAS
//      `compare_exchange(期望值, 新值, 成功序, 失败序)`：
//      当前值等于期望值才写入，返回 `Result<旧值, 实际值>`。
//      自旋锁、无锁栈、引用计数全建立在它上面。
//
//  1.4 关键认知：内存序错误**无法**通过跑程序证明存在
//      在 x86（TSO 强内存模型）上，Relaxed 和 SeqCst 生成的 load/store 几乎一样，
//      真正让程序出错的是**编译器重排**和 ARM 的弱内存模型。
//      所以在 x86 上"跑了 100 万次都对"不能证明代码正确——
//      这一点本篇 3.4 会用一个刻意的错误示范演示。
//
//  1.5 Arc 与 Mutex 的底层就是这两件东西
//      Arc = AtomicUsize 引用计数 + UnsafeCell
//      Mutex = AtomicBool/AtomicUsize 状态 + 等待队列
//      自己写一遍 SpinLock，等于把 Mutex 拆开看了一次。
// ============================================================================

// ============================================================================
//  第二部分：基础实践
// ============================================================================

/// 演示 1：多线程累加。fetch_add 保证计数不丢。
fn demo_counter() {
    println!("--- 1.1 原子计数 ---");
    const THREADS: usize = 8;
    const PER_THREAD: usize = 10_000;

    let counter = Arc::new(AtomicUsize::new(0));
    let mut handles = Vec::new();

    for _ in 0..THREADS {
        let c = Arc::clone(&counter);
        handles.push(thread::spawn(move || {
            for _ in 0..PER_THREAD {
                // SeqCst 是默认选择；这里只需要数字正确，用 Relaxed 也够，
                // 但"够不够"是要论证的，见 3.1
                c.fetch_add(1, Ordering::SeqCst);
            }
        }));
    }
    for h in handles {
        h.join().unwrap();
    }

    let total = counter.load(Ordering::SeqCst);
    println!(
        "{THREADS} 线程 × {PER_THREAD} = 期望 {} / 实际 {total} / 匹配 {}",
        THREADS * PER_THREAD,
        total == THREADS * PER_THREAD
    );
}

/// 演示 2：用 compare_exchange 自己实现自旋锁。
fn demo_spinlock() {
    println!("--- 1.3 自旋锁 ---");
    let lock = Arc::new(SpinLock::new(0usize));
    let mut handles = Vec::new();

    for _ in 0..4 {
        let l = Arc::clone(&lock);
        handles.push(thread::spawn(move || {
            for _ in 0..2_000 {
                // 临界区：guard 在语句结束时 Drop，自动解锁
                let mut guard = l.lock();
                *guard += 1;
            }
        }));
    }
    for h in handles {
        h.join().unwrap();
    }

    println!("4 线程 × 2000 = 期望 8000 / 实际 {}", *lock.lock());
}

/// 演示 3：纯统计场景——这里 Relaxed 是**正确**的用法。
///
/// 因为没有任何其他数据依赖这个计数来建立可见性：
/// 它只在最后 join 之后被读一次，join 本身提供了同步。
fn demo_relaxed_ok() {
    println!("--- 1.2 Relaxed 的正确用法 ---");
    let hits = Arc::new(AtomicUsize::new(0));
    let mut handles = Vec::new();

    for _ in 0..4 {
        let h = Arc::clone(&hits);
        handles.push(thread::spawn(move || {
            for _ in 0..1_000 {
                h.fetch_add(1, Ordering::Relaxed);
            }
        }));
    }
    // join 建立 happens-before：此后读到的值一定包含所有线程的写入
    for h in handles {
        h.join().unwrap();
    }

    println!(
        "Relaxed 累加结果 {}（正确，因为没有用它传递任何其他数据）",
        hits.load(Ordering::Relaxed)
    );
}

/// 演示 4：只跑一次"错误示范"，观察它**没有**出错。
///
/// 这把锁用 Relaxed 做状态位，语义上是错的（临界区里的写可能对别的线程不可见）。
/// 但在 x86 上几乎必然跑对 —— 这正是内存序 bug 最危险的地方。
fn demo_relaxed_lock_is_silently_wrong() {
    println!("--- 3.4 错误示范：Relaxed 自旋锁 ---");
    let lock = Arc::new(BrokenLock::new(0usize));
    let mut handles = Vec::new();

    for _ in 0..4 {
        let l = Arc::clone(&lock);
        handles.push(thread::spawn(move || {
            for _ in 0..1_000 {
                let mut g = l.lock();
                *g += 1;
            }
        }));
    }
    for h in handles {
        h.join().unwrap();
    }

    let result = *lock.lock();
    println!(
        "4 线程 × 1000 = 期望 4000 / 实际 {result} / 这次{}",
        if result == 4000 {
            "「通过」了 —— 但这不是正确性证据"
        } else {
            "暴露了问题（弱内存模型或编译器重排下会这样）"
        }
    );
}

// ---------------------------- SpinLock（正确版） ----------------------------

/// 最小自旋锁：一个 AtomicBool + 一个 UnsafeCell。
///
/// `UnsafeCell` 是 Rust 唯一允许"通过共享引用改内容"的类型；
/// `unsafe impl Sync` 是向编译器承诺"我自己保证并发安全"。
pub struct SpinLock<T> {
    locked: AtomicBool,
    value: UnsafeCell<T>,
}

impl<T> SpinLock<T> {
    pub const fn new(value: T) -> Self {
        Self {
            locked: AtomicBool::new(false),
            value: UnsafeCell::new(value),
        }
    }

    pub fn lock(&self) -> SpinGuard<'_, T> {
        // Acquire：拿到锁之后的读写，不能被挪到这次 CAS 之前
        while self
            .locked
            .compare_exchange_weak(false, true, Ordering::Acquire, Ordering::Relaxed)
            .is_err()
        {
            // 告诉 CPU 我们在自旋，省点电、让出流水线
            spin_loop();
        }
        SpinGuard { lock: self }
    }
}

// SAFETY: 互斥由 `locked` 保证；T 可以跨线程转移即可。
unsafe impl<T: Send> Sync for SpinLock<T> {}

pub struct SpinGuard<'a, T> {
    lock: &'a SpinLock<T>,
}

impl<T> Deref for SpinGuard<'_, T> {
    type Target = T;
    fn deref(&self) -> &T {
        // SAFETY: 持有 guard 期间独占访问
        unsafe { &*self.lock.value.get() }
    }
}

impl<T> DerefMut for SpinGuard<'_, T> {
    fn deref_mut(&mut self) -> &mut T {
        // SAFETY: 同上，且 &mut self 保证不与其他 guard 并存
        unsafe { &mut *self.lock.value.get() }
    }
}

impl<T> Drop for SpinGuard<'_, T> {
    fn drop(&mut self) {
        // Release：解锁前的写必须对下一个 Acquire 者可见
        self.lock.locked.store(false, Ordering::Release);
    }
}

// ---------------------------- 反例锁（故意错误） ----------------------------

/// 反面教材：把两处 Ordering 都换成 Relaxed。
///
/// 它在 x86 上通常能跑对，但语义上无法保证临界区内容对其他线程可见。
/// 保留它是为了让"跑通了"和"正确"这两件事的区别可被看到。
pub struct BrokenLock<T> {
    locked: AtomicBool,
    value: UnsafeCell<T>,
}

impl<T> BrokenLock<T> {
    pub const fn new(value: T) -> Self {
        Self {
            locked: AtomicBool::new(false),
            value: UnsafeCell::new(value),
        }
    }

    pub fn lock(&self) -> BrokenGuard<'_, T> {
        while self
            .locked
            .compare_exchange_weak(false, true, Ordering::Relaxed, Ordering::Relaxed)
            .is_err()
        {
            spin_loop();
        }
        BrokenGuard { lock: self }
    }
}

unsafe impl<T: Send> Sync for BrokenLock<T> {}

pub struct BrokenGuard<'a, T> {
    lock: &'a BrokenLock<T>,
}

impl<T> Deref for BrokenGuard<'_, T> {
    type Target = T;
    fn deref(&self) -> &T {
        unsafe { &*self.lock.value.get() }
    }
}

impl<T> DerefMut for BrokenGuard<'_, T> {
    fn deref_mut(&mut self) -> &mut T {
        unsafe { &mut *self.lock.value.get() }
    }
}

impl<T> Drop for BrokenGuard<'_, T> {
    fn drop(&mut self) {
        self.lock.locked.store(false, Ordering::Relaxed);
    }
}

// ============================================================================
//  第三部分：深入理解（可选）
// ============================================================================
//
//  3.1 什么时候 Relaxed 是安全的
//      判别标准只有一个：**这个原子值是否被用来传递其他数据的可见性？**
//        - 只是计数、打点、统计 → Relaxed 足够（配合 join 或外层同步）
//        - 用它来决定"别的数据能不能读"（如锁状态、就绪标志）→ 必须 Acquire/Release
//      本篇 demo_relaxed_ok 是前者，demo_spinlock 是后者。
//
//  3.2 Release 与 Acquire 建立 happens-before 的机制
//      线程 A：写 data → store(flag, Release)
//      线程 B：load(flag, Acquire) 看到 true → 读 data
//      Release 保证 A 中"写 data"不会被挪到 store 之后；
//      Acquire 保证 B 中"读 data"不会被挪到 load 之前。
//      两者配对，构成 A 的写到 B 的读的可见性边。
//      缺任何一边，这条边就不存在——编译器与 CPU 都可以重排。
//
//  3.3 为什么"x86 上 everything works"
//      x86 是 TSO（Total Store Order）：硬件几乎不做重排，
//      所以 SeqCst 和 Relaxed 在机器码层面常常都是普通 mov。
//      但 **编译器重排**在任何平台上都发生。
//      结论：在 x86 上测不出 Relaxed 的错，不代表换到 ARM 也不错。
//      【推定】这是"本地全绿、CI 全绿、上线偶发"的一类典型来源。
//
//  3.4 compare_exchange_weak vs strong
//      weak 允许"虚假失败"（值没变也返回 Err），代价是循环里用更高效；
//      在自旋循环或 LL/SC 架构（ARM）上，weak 是正确选择。
//      strong 适合"不想自己写循环"的单次尝试场景。
//
//  3.5 自旋锁不是什么场景都能用
//      自旋假设"锁很快会释放"。如果临界区里有 IO、有 sleep、或持锁线程被抢占，
//      自旋会纯粹浪费 CPU 周期（多核上是别人在干正事，你在空转）。
//      真实项目中优先 std::sync::Mutex；自旋锁只在临界区极短、
//      且确定不会被长时间占用时才有意义。
//
//  3.6 死锁：同一线程二次 lock
//      SpinLock 不可重入。下面的写法会把线程永久卡在 spin_loop 里：
//        let _a = lock.lock();
//        let _b = lock.lock();   // 永远等不到
//      这也是为什么本文件**不**写这条 demo：它会让整个进程挂住。
// ============================================================================

// ============================================================================
//  第四部分：常见错误和陷阱
// ============================================================================

//  4.1 原子类型不能当普通整数用 —— E0308
//
//     let a = AtomicUsize::new(0);
//     let v: usize = a;
//
//     error[E0308]: mismatched types
//      --> p3.rs:4:20
//       |
//     4 |     let v: usize = a;
//       |            -----   ^ expected `usize`, found `AtomicUsize`
//       |            |
//       |            expected due to this
//
//     原子类型没有 Deref，也没有隐式转换。必须 `a.load(Ordering::Relaxed)`。

//  4.2 Ordering 变体大小写写错 —— E0599
//
//     let _ = a.load(Ordering::Seqcst);
//
//     error[E0599]: no variant or associated item named `Seqcst` found for enum
//     `std::sync::atomic::Ordering` in the current scope
//      --> p3.rs:6:30
//       |
//     6 |     let _ = a.load(Ordering::Seqcst);
//       |                              ^^^^^^ variant or associated item not found
//       |
//     help: there is a variant with a similar name (notice the capitalization)
//       |
//     6 -     let _ = a.load(Ordering::Seqcst);
//     6 +     let _ = a.load(Ordering::SeqCst);
//
//     注意 help 里的 6 - / 6 + 看起来一样——因为差异只有一个字母的大小写，
//     肉眼看输出很容易忽略"notice the capitalization"这句提示。
//     正确拼写是 `SeqCst`。

//  4.3 闭包借用而不是转移 —— E0373
//
//     let n = AtomicUsize::new(0);
//     let h = std::thread::spawn(|| { n.fetch_add(1, Ordering::Relaxed); });
//
//     error[E0373]: closure may outlive the current function, but it borrows `n`,
//                   which is owned by the current function
//      --> p1.rs:4:32
//       |
//     4 |     let h = std::thread::spawn(|| { n.fetch_add(1, ...); });
//       |                                ^^   - `n` is borrowed here
//       |                                |
//       |                                may outlive borrowed value `n`
//       |
//     help: to force the closure to take ownership ... use the `move` keyword
//       |
//     4 |     let h = std::thread::spawn(move || { ... });
//       |                                ++++
//
//     即便加了 move，单线程也只能有一个所有者——多线程共享仍需 `Arc::clone`。

//  4.4 用 RefCell 做"线程安全" —— E0277
//
//     let c = Arc::new(RefCell::new(0));
//     std::thread::spawn(move || { *c2.borrow_mut() += 1; });
//
//     error[E0277]: `RefCell<i32>` cannot be shared between threads safely
//       --> p2.rs:6:24
//        |
//      6 |     std::thread::spawn(move || { *c2.borrow_mut() += 1; })
//        |     ------------------ ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//        |     |
//        |     required by a bound introduced by this call
//        |
//        = help: the trait `Sync` is not implemented for `RefCell<i32>`
//        = note: if you want to do aliasing and mutation between multiple threads,
//                use `std::sync::RwLock` instead
//        = note: required for `Arc<RefCell<i32>>` to implement `Send`
//
//     教训：`Arc<T>` 只有 `T: Sync` 时才 `Send`。Arc 负责"共享"，
//     RefCell 只负责"单线程内可变借用"，两者拼起来并不等于线程安全。

//  4.5 用 Relaxed 做互斥 —— 无编译错误，无法稳定复现
//
//     见本文件 BrokenLock。它的每次运行结果都可能是 4000。
//     这类"错误"不会被任何静态检查、单元测试或 x86 上的压测抓到。
//     唯一可靠的防线是**写代码时明确每个 Ordering 的理由**，
//     而不是"先 Relaxed，跑不通再改 SeqCst"。
//     【实测】本机运行 5 次全部得 4000——这恰恰说明了问题。

//  4.6 fetch_add 的返回值是"旧值"不是"新值"
//
//     let old = counter.fetch_add(1, Ordering::Relaxed);
//     // 返回 0 表示之前是 0，加完之后是 1
//     想拿"加完之后的值"必须再 load，或写
//     `counter.fetch_add(1, SeqCst) + 1`（且在并发下这两者语义不同：
//     前者拿到的可能是别人也加过之后的值）。这是最容易写错的一行。

//  4.7 在自旋锁里访问可变共享状态 —— 无法安全地绕过 UnsafeCell
//
//     想在 `&self` 里改字段，必须用 `UnsafeCell`。
//     用 `Cell<T>` 只能拿到 Copy 的量级，用 `RefCell<T>` 会失去 Sync。
//     换句话说：**手写任何同步原语都必然要 unsafe**，
//     这不是设计缺陷，而是"你在实现安全抽象本身"的必然代价。

// ============================================================================
//  第五部分：函数/宏卡片速查
// ============================================================================
//
//  | 你要做的事                        | 写法                                                |
//  |-----------------------------------|-----------------------------------------------------|
//  | 读                               | `a.load(Ordering::Acquire)`                         |
//  | 写                               | `a.store(v, Ordering::Release)`                     |
//  | 加并拿旧值                        | `a.fetch_add(1, Ordering::Relaxed)`                 |
//  | 加并拿新值                        | `a.fetch_add(1, Ordering::SeqCst) + 1`              |
//  | 条件更新（CAS）                   | `compare_exchange(old, new, Acquire, Relaxed)`      |
//  | 循环里 CAS                        | `compare_exchange_weak(...)`                        |
//  | 自旋提示                          | `std::hint::spin_loop()`                            |
//  | 允许 &self 内部可变               | `UnsafeCell<T>`                                     |
//  | 声明本类型跨线程安全              | `unsafe impl<T: Send> Sync for X<T> {}`             |
//  | 知道"临界区里改了值"              | `Acquire`（锁）+ `Release`（解锁）配对               |
//  | 只是统计打点                      | `Relaxed` + 用 join / 外层同步收尾                   |
//
//  Ordering 选择口诀：
//    这个值是否用来"让它后面的数据可见"？
//      否 → Relaxed
//      是 → 读用 Acquire、写用 Release、读改写用 AcqRel
//    拿不准 → SeqCst。它慢，但它不会让你在 ARM 上偶发丢更新。

// ============================================================================
//  第六部分：练习题（不提供答案）
// ============================================================================
//
//  1. 把提示信息改成"用 Relaxed"的版本重构 `demo_counter`，
//     并论证：为什么在这里 Relaxed 与 SeqCst 结果一致？
//     论证中必须指出"是哪一次同步操作兜住了可见性"。
//
//  2. 给 `SpinLock` 加一个 `try_lock(&self) -> Option<SpinGuard<'_, T>>`，
//     然后写一个测试：持锁时 try_lock 必须返回 None。
//     （注意：不要写"持锁时再 lock"，那会挂住。）
//
//  3. 用 `compare_exchange` 实现一个 `OnceFlag`：只有第一个调用返回 true。
//     再写测试用 8 个线程竞争，断言恰好 1 个拿到 true。
//
//  4. 给 `SpinLock` 加 `is_locked()`（用 `Acquire` 读），
//     并解释：如果改用 Relaxed，会漏掉哪种重排？
//
//  5. 写一个可重入自旋锁（记录持有线程 id + 重入次数）。
//     写完之后回答：这个实现相比不可重入版本新增了哪两个失败模式？
//
//  6. 用 `std::sync::atomic::AtomicPtr` + CAS 实现一个极简无锁栈 push/pop，
//     并说明它存在 ABA 问题。给出一种规避方案并说明其代价。
//
//  7. 复现 4.3 的 E0373，然后把 `n` 改成 `Arc<AtomicUsize>`，
//     观察报错如何从 E0373 变成"没有错误"。思考：move 之后还是有代价吗？
//
//  8. 在 4.5 的错误示范上做实验：把 BrokenLock 的临界区改成
//     "写入一个非原子数组的多个元素"，在多核机器上压测 10 分钟，
//     记录是否出现数据不一致。无论结果如何，写下你**不能**由此得出的结论。

fn main() {
    println!("=== 原子类型与内存序 ===\n");
    demo_counter();
    println!();
    demo_relaxed_ok();
    println!();
    demo_spinlock();
    println!();
    demo_relaxed_lock_is_silently_wrong();
    println!("\n⚠ 本篇所有【实测】只代表本机 x86_64 跑通，不代表内存序正确。");
}
