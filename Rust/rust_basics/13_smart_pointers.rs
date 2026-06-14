// ============================================
// 智能指针 - 超详细注释版
// 日期：2026-06-14（第15天）
// 难度：⭐⭐⭐ 高级
// ============================================

/*
【核心概念】智能指针

是什么？
- 智能指针是一种数据结构，行为类似指针，但有额外的元数据和功能
- 最常见的是：Box<T>、Rc<T>、RefCell<T>
- 普通引用只是借用数据，智能指针通常拥有数据

为什么需要？
- Box<T>：在堆上分配数据，处理编译期大小未知的类型
- Rc<T>：允许多个所有者共享数据（引用计数）
- RefCell<T>：在不可变引用下修改数据（内部可变性）

核心特点？
- 实现了 Deref trait：可以像引用一样使用
- 实现了 Drop trait：离开作用域时自动清理
- Box 和 Rc 编译期检查，RefCell 运行时检查

Rust特色？
- 零成本抽象：Box 的性能和手动堆分配完全一样
- 类型安全：编译器保证内存安全（除了 RefCell 的运行时检查）
- 无 GC：智能指针通过所有权系统自动管理内存，无需垃圾回收
*/

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

fn part1_concepts() {
    println!("============================================================");
    println!("第一部分：核心概念详解");
    println!("============================================================");

    // --------------------------------------------
    // 1.1 什么是智能指针？
    // --------------------------------------------

    println!("\n1.1 什么是智能指针？");
    println!("------------------------------------------------------------");

    /*
    【智能指针 Smart Pointer】

    是什么？
    - 包含指针的数据结构，但有额外的元数据和功能
    - 通常实现了 Deref 和 Drop trait
    - 常见的智能指针：Box<T>、Rc<T>、RefCell<T>、Arc<T>

    与普通引用的区别：
    - 引用（&T）：只借用数据，不拥有
    - 智能指针：通常拥有数据，负责清理

    实现细节：
    - 智能指针通常是结构体
    - 通过 Deref trait 实现解引用（*）
    - 通过 Drop trait 实现自动清理

    Rust 标准库中的智能指针：
    - Box<T>：在堆上分配数据
    - Rc<T>：引用计数智能指针（单线程）
    - Arc<T>：原子引用计数（多线程）
    - RefCell<T>：内部可变性
    - Mutex<T>、RwLock<T>：线程安全的内部可变性
    */

    println!("智能指针 = 拥有数据 + Deref + Drop");

    // --------------------------------------------
    // 1.2 堆 vs 栈
    // --------------------------------------------

    println!("\n1.2 堆 vs 栈");
    println!("------------------------------------------------------------");

    /*
    【栈（Stack）】

    特点：
    - 后进先出（LIFO）
    - 分配速度快：只需移动栈指针
    - 大小固定：编译期必须知道大小
    - 自动管理：离开作用域自动释放

    适用场景：
    - 基本类型：i32、f64、bool
    - 固定大小的数组：[i32; 5]
    - 已知大小的结构体

    【堆（Heap）】

    特点：
    - 无序存储
    - 分配速度慢：需要找到足够大的空间
    - 大小可变：运行时确定
    - 手动管理：需要显式释放（Rust 通过所有权自动管理）

    适用场景：
    - 编译期大小未知：Vec<T>、String
    - 递归类型：链表、树
    - 大数据：避免栈溢出
    - 需要跨作用域共享数据

    为什么需要 Box？
    - 编译期不知道大小的类型（如递归类型）
    - 大数据不想复制（拥有所有权但存在堆上）
    - 只关心类型实现了某个 trait，不关心具体类型
    */

    println!("栈：快速、固定大小、自动管理");
    println!("堆：灵活、可变大小、需要智能指针管理");

    // --------------------------------------------
    // 1.3 三种主要智能指针的区别
    // --------------------------------------------

    println!("\n1.3 三种主要智能指针的区别");
    println!("------------------------------------------------------------");

    /*
    【Box<T>】
    - 所有权：单一所有者
    - 借用检查：编译期
    - 使用场景：堆分配、递归类型、大数据
    - 性能：零成本抽象

    【Rc<T>】
    - 所有权：多个所有者（引用计数）
    - 借用检查：编译期
    - 可变性：只允许不可变借用
    - 使用场景：数据需要多个所有者共享
    - 线程安全：否（单线程）

    【RefCell<T>】
    - 所有权：单一所有者
    - 借用检查：运行时
    - 可变性：内部可变性（不可变引用下可修改）
    - 使用场景：需要运行时借用检查、内部可变性
    - 性能：有运行时开销

    【组合使用】
    - Rc<RefCell<T>>：多个所有者 + 可变数据
    - Box<dyn Trait>：trait 对象（动态分发）

    【选择指南】
    - 需要堆分配？→ Box<T>
    - 需要多个所有者？→ Rc<T>
    - 需要内部可变性？→ RefCell<T>
    - 多个所有者 + 可变？→ Rc<RefCell<T>>
    - 多线程共享？→ Arc<T> + Mutex<T>
    */

    println!("Box：单一所有者，编译期检查");
    println!("Rc：多个所有者，引用计数");
    println!("RefCell：内部可变性，运行时检查");
}

// ============================================
// 第二部分：基础操作实践
// ============================================

fn part2_basic_usage() {
    println!("\n============================================================");
    println!("第二部分：基础操作实践");
    println!("============================================================");

    // --------------------------------------------
    // 2.1 Box<T> 基础使用
    // --------------------------------------------

    println!("\n2.1 Box<T> 基础使用");
    println!("------------------------------------------------------------");

    // 示例1：在堆上分配简单值
    let b = Box::new(5);
    // Box::new(5)：在堆上分配 i32 值 5
    // b：拥有堆上数据的所有权
    println!("堆上的值: {}", b); // 自动解引用

    // 示例2：显式解引用
    let x = *b; // *b：解引用，获取堆上的值
    println!("解引用后的值: {}", x);

    // 示例3：Box 离开作用域自动清理
    {
        let temp = Box::new(10);
        println!("temp 在作用域内: {}", temp);
    } // temp 离开作用域，堆上的数据自动释放

    println!("→ Box 在堆上分配数据，拥有所有权，离开作用域自动清理");

    // --------------------------------------------
    // 2.2 Box<T> 递归类型
    // --------------------------------------------

    println!("\n2.2 Box<T> 递归类型");
    println!("------------------------------------------------------------");

    // 定义链表（递归类型）
    #[derive(Debug)]
    enum List {
        Cons(i32, Box<List>), // 递归：包含 Box<List>
        Nil,
    }

    // 创建链表：1 -> 2 -> 3 -> Nil
    // 注意：函数内定义的枚举，变体用 List:: 前缀引用
    let list = List::Cons(
        1,
        Box::new(List::Cons(2, Box::new(List::Cons(3, Box::new(List::Nil))))),
    );
    println!("链表: {:?}", list);

    /*
    为什么需要 Box？
    - 没有 Box：enum List { Cons(i32, List), Nil }
    - 问题：List 的大小无限递归，编译器无法确定大小
    - 解决：Box<List> 是指针，大小固定（usize），打破递归
    */

    println!("→ Box 让我们定义递归类型（链表、树等）");

    // --------------------------------------------
    // 2.3 Rc<T> 引用计数
    // --------------------------------------------

    println!("\n2.3 Rc<T> 引用计数");
    println!("------------------------------------------------------------");

    use std::rc::Rc;

    // 示例1：多个所有者共享数据
    let a = Rc::new(5);
    // Rc::new(5)：在堆上分配 5，引用计数 = 1
    println!("引用计数 a: {}", Rc::strong_count(&a)); // 1

    let b = Rc::clone(&a);
    // Rc::clone(&a)：增加引用计数，不复制数据
    // a 和 b 指向同一份堆数据
    println!("引用计数 a: {}", Rc::strong_count(&a)); // 2
    println!("引用计数 b: {}", Rc::strong_count(&b)); // 2

    {
        let c = Rc::clone(&a);
        println!("引用计数 a: {}", Rc::strong_count(&a)); // 3
        println!("a = {}, b = {}, c = {}", a, b, c);
    } // c 离开作用域，引用计数 -1

    println!("引用计数 a: {}", Rc::strong_count(&a)); // 2

    // 示例2：共享链表节点
    #[derive(Debug)]
    enum ListRc {
        Cons(i32, Rc<ListRc>),
        Nil,
    }

    // 函数内定义的枚举，变体用 ListRc:: 前缀引用
    let shared = Rc::new(ListRc::Cons(3, Rc::new(ListRc::Nil)));
    // list1: 1 -> 2 -> shared
    let list1 = ListRc::Cons(1, Rc::new(ListRc::Cons(2, Rc::clone(&shared))));
    // list2: 4 -> 5 -> shared
    let list2 = ListRc::Cons(4, Rc::new(ListRc::Cons(5, Rc::clone(&shared))));

    println!("list1: {:?}", list1);
    println!("list2: {:?}", list2);
    println!("shared 引用计数: {}", Rc::strong_count(&shared)); // 3

    println!("→ Rc 允许多个所有者共享数据，通过引用计数自动管理");

    // --------------------------------------------
    // 2.4 RefCell<T> 内部可变性
    // --------------------------------------------

    println!("\n2.4 RefCell<T> 内部可变性");
    println!("------------------------------------------------------------");

    use std::cell::RefCell;

    // 示例1：不可变引用下修改数据
    let x = RefCell::new(5);
    // x 是不可变的，但可以修改内部值
    *x.borrow_mut() = 10;
    // borrow_mut()：获取可变借用（运行时检查）
    // *：解引用并赋值
    println!("修改后的值: {}", x.borrow()); // borrow()：获取不可变借用

    // 示例2：运行时借用检查
    let value = RefCell::new(42);
    let r1 = value.borrow(); // 不可变借用
    let r2 = value.borrow(); // 可以有多个不可变借用
    println!("r1 = {}, r2 = {}", r1, r2);
    drop(r1); // 释放借用
    drop(r2);

    let mut r3 = value.borrow_mut(); // 可变借用
    *r3 += 1;
    println!("修改后: {}", *r3);
    drop(r3); // 必须释放可变借用

    // ⚠️ 违反借用规则会 panic（运行时错误）
    // let r4 = value.borrow();
    // let r5 = value.borrow_mut(); // panic: 已经有不可变借用

    println!("→ RefCell 在不可变引用下修改数据，运行时检查借用规则");

    // --------------------------------------------
    // 2.5 Rc<RefCell<T>> 组合使用
    // --------------------------------------------

    println!("\n2.5 Rc<RefCell<T>> 组合使用");
    println!("------------------------------------------------------------");

    // 场景：多个所有者 + 可变数据
    let shared_value = Rc::new(RefCell::new(5));
    // Rc：允许多个所有者
    // RefCell：允许内部可变性

    let a = Rc::clone(&shared_value);
    let b = Rc::clone(&shared_value);

    // 通过 a 修改数据
    *a.borrow_mut() += 10;
    println!("通过 a 修改后: {}", shared_value.borrow()); // 15

    // 通过 b 修改数据
    *b.borrow_mut() += 5;
    println!("通过 b 修改后: {}", shared_value.borrow()); // 20

    println!("→ Rc<RefCell<T>> 实现多个所有者共享可变数据");

    // --------------------------------------------
    // 2.6 Deref trait
    // --------------------------------------------

    println!("\n2.6 Deref trait");
    println!("------------------------------------------------------------");

    use std::ops::Deref;

    // 自定义智能指针
    struct MyBox<T>(T);

    impl<T> MyBox<T> {
        fn new(x: T) -> MyBox<T> {
            MyBox(x)
        }
    }

    // 实现 Deref trait
    impl<T> Deref for MyBox<T> {
        type Target = T; // 关联类型

        fn deref(&self) -> &Self::Target {
            &self.0 // 返回内部值的引用
        }
    }

    let x = 5;
    let y = MyBox::new(x);

    assert_eq!(5, x);
    assert_eq!(5, *y); // *y 等价于 *(y.deref())

    // 解引用强制转换（Deref Coercion）
    fn hello(name: &str) {
        println!("Hello, {}!", name);
    }

    let m = MyBox::new(String::from("Rust"));
    hello(&m); // &MyBox<String> -> &String -> &str
    // 编译器自动调用 deref 转换

    println!("→ Deref trait 让智能指针像引用一样使用，支持自动解引用");

    // --------------------------------------------
    // 2.7 Drop trait
    // --------------------------------------------

    println!("\n2.7 Drop trait");
    println!("------------------------------------------------------------");

    // 自定义清理逻辑
    struct CustomSmartPointer {
        data: String,
    }

    impl Drop for CustomSmartPointer {
        fn drop(&mut self) {
            println!("清理 CustomSmartPointer，数据: `{}`", self.data);
        }
    }

    {
        let c = CustomSmartPointer {
            data: String::from("my stuff"),
        };
        let d = CustomSmartPointer {
            data: String::from("other stuff"),
        };
        println!("CustomSmartPointer 创建完成");
    } // c 和 d 离开作用域，自动调用 drop（后进先出：d 先 drop）

    // 提前释放：使用 std::mem::drop
    let e = CustomSmartPointer {
        data: String::from("some data"),
    };
    println!("提前释放前");
    drop(e); // 手动调用 drop（不能调用 e.drop()）
    println!("提前释放后");

    println!("→ Drop trait 实现自动清理，离开作用域时自动调用");
}

// ============================================
// 第三部分：深入理解（可选）🔍
// ============================================

/*
==============================================
🔍 深入理解：循环引用与内存泄漏（选学）
==============================================

如果你想知道"Rc 的陷阱"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【循环引用问题】

Rc<T> 通过引用计数自动释放内存，但如果存在循环引用，引用计数永远不会降到 0，导致内存泄漏。

示例：
```rust
use std::rc::Rc;
use std::cell::RefCell;

struct Node {
    value: i32,
    next: Option<Rc<RefCell<Node>>>,
}

let a = Rc::new(RefCell::new(Node { value: 1, next: None }));
let b = Rc::new(RefCell::new(Node { value: 2, next: Some(Rc::clone(&a)) }));

// 创建循环引用
a.borrow_mut().next = Some(Rc::clone(&b));
// a -> b -> a，循环引用

// 离开作用域后：
// a 的引用计数 = 1（b 持有）
// b 的引用计数 = 1（a 持有）
// 两者都不会被释放 → 内存泄漏
```

【解决方案：Weak<T>】

Weak<T> 是弱引用，不增加引用计数，用于打破循环引用。

- Rc::downgrade(&rc)：创建 Weak<T>
- Weak::upgrade(&weak)：尝试获取 Rc<T>（如果数据已释放返回 None）

使用场景：
- 父节点 → 子节点：Rc<T>（强引用）
- 子节点 → 父节点：Weak<T>（弱引用）

修复后的代码：
```rust
struct Node {
    value: i32,
    parent: RefCell<Weak<Node>>, // 弱引用父节点
    children: RefCell<Vec<Rc<Node>>>, // 强引用子节点
}
```

==============================================
*/

/*
==============================================
🔍 深入理解：Rc 与 Arc 的区别（选学）
==============================================

【Rc<T>】：引用计数（Reference Counted）
- 单线程：不能跨线程共享
- 性能：无原子操作，更快
- 使用场景：单线程数据共享

【Arc<T>】：原子引用计数（Atomic Reference Counted）
- 多线程：可以跨线程共享
- 性能：有原子操作，稍慢
- 使用场景：多线程数据共享

选择指南：
- 单线程？用 Rc<T>
- 多线程？用 Arc<T>

注意：Arc<T> 只保证引用计数的线程安全，不保证数据的可变性。
多线程可变数据需要：Arc<Mutex<T>> 或 Arc<RwLock<T>>

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
    // 4.1 错误1：递归类型没有用 Box
    // --------------------------------------------

    println!("\n4.1 错误1：递归类型没有用 Box");
    println!("------------------------------------------------------------");

    /*
    ❌ 错误代码：
    ```rust
    enum List {
        Cons(i32, List), // ❌ 编译错误
        Nil,
    }
    ```

    编译器错误信息：
    error[E0072]: recursive type `List` has infinite size
    help: insert some indirection (e.g., a `Box`, `Rc`, or `&`) to break the cycle

    原因：
    - List 包含 List，大小无限递归
    - 编译器无法确定 List 的大小

    ✅ 正确代码：
    ```rust
    enum List {
        Cons(i32, Box<List>), // ✅ Box 是指针，大小固定
        Nil,
    }
    ```

    记忆技巧：
    - 递归类型必须用指针（Box、Rc）打破无限递归
    - Box 的大小是固定的（usize，指针大小）
    */

    println!("→ 递归类型必须用 Box 打破无限递归");

    // --------------------------------------------
    // 4.2 错误2：用 clone 而不是 Rc::clone
    // --------------------------------------------

    println!("\n4.2 错误2：混淆 clone 和 Rc::clone");
    println!("------------------------------------------------------------");

    /*
    ⚠️ 容易混淆：
    ```rust
    use std::rc::Rc;

    let a = Rc::new(5);
    let b = a.clone();        // 可以，但不推荐
    let c = Rc::clone(&a);    // 推荐写法
    ```

    区别：
    - a.clone()：也是增加引用计数（Rc 的 clone 实现）
    - Rc::clone(&a)：明确表示这是引用计数克隆

    为什么推荐 Rc::clone？
    - 代码可读性：明确表示这是 Rc 的引用计数克隆
    - 区分深拷贝：避免与普通 clone（深拷贝）混淆
    - 性能提示：Rc::clone 只增加计数，不复制数据

    ✅ 最佳实践：
    - 使用 Rc::clone(&a) 而不是 a.clone()
    - 让代码意图更清晰
    */

    println!("→ 推荐用 Rc::clone(&a) 而不是 a.clone()，意图更清晰");

    // --------------------------------------------
    // 4.3 错误3：RefCell 运行时借用 panic
    // --------------------------------------------

    println!("\n4.3 错误3：RefCell 运行时借用 panic");
    println!("------------------------------------------------------------");

    /*
    ❌ 错误代码：
    ```rust
    use std::cell::RefCell;

    let value = RefCell::new(5);
    let r1 = value.borrow_mut();  // 可变借用
    let r2 = value.borrow_mut();  // ❌ panic！
    ```

    运行时错误信息：
    thread 'main' panicked at 'already borrowed: BorrowMutError'

    原因：
    - RefCell 在运行时检查借用规则
    - 不能同时有两个可变借用
    - 违反规则不是编译错误，而是运行时 panic

    ✅ 正确代码：
    ```rust
    let value = RefCell::new(5);
    {
        let r1 = value.borrow_mut();
        // 使用 r1
    } // r1 离开作用域，释放借用

    let r2 = value.borrow_mut(); // ✅ 现在可以了
    ```

    记忆技巧：
    - RefCell 把借用检查从编译期推迟到运行时
    - 灵活性的代价是可能 panic
    - 注意借用的作用域，及时释放
    - borrow() 多个可以，borrow_mut() 只能一个
    */

    println!("→ RefCell 运行时检查借用，违反规则会 panic（不是编译错误）");

    // --------------------------------------------
    // 4.4 错误4：循环引用导致内存泄漏
    // --------------------------------------------

    println!("\n4.4 错误4：循环引用导致内存泄漏");
    println!("------------------------------------------------------------");

    /*
    ❌ 问题代码：
    ```rust
    use std::rc::Rc;
    use std::cell::RefCell;

    struct Node {
        next: Option<Rc<RefCell<Node>>>,
    }

    let a = Rc::new(RefCell::new(Node { next: None }));
    let b = Rc::new(RefCell::new(Node { next: Some(Rc::clone(&a)) }));
    a.borrow_mut().next = Some(Rc::clone(&b)); // 循环引用！
    // a -> b -> a，引用计数永远不为 0
    ```

    问题：
    - Rc 通过引用计数释放内存
    - 循环引用：a 和 b 相互持有，计数永远 > 0
    - 内存泄漏：永远不会被释放

    ✅ 解决方案：使用 Weak<T>
    ```rust
    use std::rc::{Rc, Weak};

    struct Node {
        parent: RefCell<Weak<Node>>,    // 弱引用（不增加计数）
        children: RefCell<Vec<Rc<Node>>>, // 强引用
    }

    // 创建弱引用
    let weak = Rc::downgrade(&rc);
    // 尝试升级为强引用
    if let Some(strong) = weak.upgrade() {
        // 数据还存在
    }
    ```

    记忆技巧：
    - 父→子：Rc（强引用）
    - 子→父：Weak（弱引用）
    - Weak 不增加引用计数，打破循环
    */

    println!("→ 循环引用导致内存泄漏，用 Weak<T> 打破循环");

    // --------------------------------------------
    // 4.5 错误5：手动调用 drop 方法
    // --------------------------------------------

    println!("\n4.5 错误5：手动调用 drop 方法");
    println!("------------------------------------------------------------");

    /*
    ❌ 错误代码：
    ```rust
    let c = CustomSmartPointer { data: String::from("data") };
    c.drop(); // ❌ 编译错误
    ```

    编译器错误信息：
    error[E0040]: explicit use of destructor method
    help: consider using `drop` function: `drop(c)`

    原因：
    - 不能直接调用 Drop::drop 方法
    - 否则会导致双重释放（drop 调用后，离开作用域又调用一次）

    ✅ 正确代码：
    ```rust
    let c = CustomSmartPointer { data: String::from("data") };
    drop(c); // ✅ 使用 std::mem::drop 函数
    // drop(c) 会获取 c 的所有权并提前释放
    ```

    记忆技巧：
    - 不能调用 c.drop()
    - 用 drop(c) 函数提前释放
    - Rust 自动管理，通常不需要手动 drop
    */

    println!("→ 不能调用 c.drop()，要用 drop(c) 函数提前释放");

    // --------------------------------------------
    // 4.6 错误6：Box 移动后使用
    // --------------------------------------------

    println!("\n4.6 错误6：Box 移动后使用");
    println!("------------------------------------------------------------");

    /*
    ❌ 错误代码：
    ```rust
    let b = Box::new(5);
    let c = b;        // b 的所有权移动到 c
    println!("{}", b); // ❌ 编译错误：b 已被移动
    ```

    编译器错误信息：
    error[E0382]: borrow of moved value: `b`

    原因：
    - Box 拥有所有权
    - let c = b 移动了所有权
    - b 不再有效

    ✅ 正确代码：
    ```rust
    // 方案1：使用引用
    let b = Box::new(5);
    let c = &b;        // 借用
    println!("{}", b); // ✅ b 仍然有效

    // 方案2：克隆（如果需要独立副本）
    let b = Box::new(5);
    let c = b.clone(); // 克隆数据
    println!("{}", b); // ✅ b 仍然有效
    ```

    记忆技巧：
    - Box 遵循所有权规则
    - 移动后原变量失效
    - 需要保留？用引用或 clone
    */

    println!("→ Box 遵循所有权规则，移动后原变量失效");
}

// ============================================
// 第五部分：函数/类型卡片速查
// ============================================

/*
【Box<T>】

创建：Box::new(value)
说明：在堆上分配数据，返回拥有所有权的智能指针
解引用：*box_value
使用场景：堆分配、递归类型、大数据、trait 对象

示例：
  let b = Box::new(5);
  let value = *b; // 解引用

────────────────────────────────────

【Rc<T>】（需要 use std::rc::Rc;）

创建：Rc::new(value)
说明：引用计数智能指针，允许多个所有者
克隆：Rc::clone(&rc) — 增加引用计数（不复制数据）
计数：Rc::strong_count(&rc) — 获取引用计数
弱引用：Rc::downgrade(&rc) — 创建 Weak<T>

示例：
  let a = Rc::new(5);
  let b = Rc::clone(&a);
  println!("{}", Rc::strong_count(&a)); // 2

────────────────────────────────────

【RefCell<T>】（需要 use std::cell::RefCell;）

创建：RefCell::new(value)
说明：内部可变性，运行时借用检查
不可变借用：refcell.borrow() — 返回 Ref<T>
可变借用：refcell.borrow_mut() — 返回 RefMut<T>
注意：违反借用规则会 panic

示例：
  let x = RefCell::new(5);
  *x.borrow_mut() = 10;
  println!("{}", x.borrow());

────────────────────────────────────

【Weak<T>】（需要 use std::rc::Weak;）

创建：Rc::downgrade(&rc)
说明：弱引用，不增加引用计数，用于打破循环引用
升级：weak.upgrade() — 返回 Option<Rc<T>>
计数：Rc::weak_count(&rc) — 获取弱引用计数

示例：
  let weak = Rc::downgrade(&rc);
  if let Some(strong) = weak.upgrade() {
      // 数据还存在
  }

────────────────────────────────────

【Deref trait】（需要 use std::ops::Deref;）

定义：
  impl<T> Deref for MyBox<T> {
      type Target = T;
      fn deref(&self) -> &Self::Target {
          &self.0
      }
  }
说明：实现解引用操作符 *，支持解引用强制转换

────────────────────────────────────

【Drop trait】

定义：
  impl Drop for MyType {
      fn drop(&mut self) {
          // 清理逻辑
      }
  }
说明：离开作用域时自动调用，实现自定义清理
提前释放：drop(value) — 使用 std::mem::drop 函数

────────────────────────────────────

【组合类型】

Rc<RefCell<T>>：多个所有者 + 可变数据
Box<dyn Trait>：trait 对象（动态分发）
Arc<Mutex<T>>：多线程共享可变数据

*/

// ============================================
// 第六部分：练习题
// ============================================

/*
练习题（不提供参考答案，鼓励独立思考！）

【Box<T> 练习】

1. 使用 Box 定义一个二叉树
   要求：
   - 定义 enum Tree { Node(i32, Box<Tree>, Box<Tree>), Leaf }
   - 创建一棵简单的二叉树
   - 编写一个函数计算树的所有节点值之和
   提示：递归遍历，注意 Box 的解引用
   注意：为什么二叉树需要 Box？

2. 实现一个简单的链表
   要求：
   - 定义 enum List { Cons(i32, Box<List>), Nil }
   - 编写函数 sum(list: &List) -> i32 计算链表元素之和
   - 编写函数 length(list: &List) -> usize 计算链表长度
   提示：模式匹配 + 递归

3. Box<dyn Trait> 练习
   要求：
   - 定义 trait Shape { fn area(&self) -> f64; }
   - 为 Circle 和 Rectangle 实现 Shape
   - 创建 Vec<Box<dyn Shape>> 存储不同形状
   - 遍历计算总面积
   提示：Box<dyn Shape> 是 trait 对象，可存储不同类型

────────────────────────────────────

【Rc<T> 练习】

4. 共享数据的引用计数
   要求：
   - 创建一个 Rc<String>
   - 克隆 3 次（在不同作用域）
   - 在每个步骤打印引用计数
   - 观察引用计数的变化
   提示：使用 Rc::strong_count 和作用域 {}

5. 共享配置数据
   要求：
   - 定义一个 Config 结构体
   - 用 Rc 包装，让多个"组件"共享同一个配置
   - 模拟 3 个组件持有同一个 Rc<Config>
   提示：每个组件用 Rc::clone 获取配置

────────────────────────────────────

【RefCell<T> 练习】

6. 内部可变性计数器
   要求：
   - 定义 struct Counter { count: RefCell<i32> }
   - 实现方法 increment(&self)（注意：&self 不是 &mut self）
   - 实现方法 get(&self) -> i32
   提示：使用 borrow_mut() 修改，borrow() 读取
   注意：为什么用 &self 也能修改？

7. 模拟消息记录器
   要求：
   - 定义 struct Logger { messages: RefCell<Vec<String>> }
   - 实现方法 log(&self, msg: &str) 添加消息
   - 实现方法 print_all(&self) 打印所有消息
   提示：RefCell<Vec<String>> 允许在 &self 下修改 Vec

────────────────────────────────────

【Rc<RefCell<T>> 练习】

8. 共享可变状态
   要求：
   - 创建 Rc<RefCell<Vec<i32>>>
   - 克隆给两个"所有者" a 和 b
   - 通过 a 添加元素 1, 2, 3
   - 通过 b 添加元素 4, 5
   - 打印最终的 Vec（应该包含所有元素）
   提示：Rc 共享所有权，RefCell 允许修改

9. 实现一个简单的图节点
   要求：
   - 定义 struct GraphNode { value: i32, neighbors: RefCell<Vec<Rc<GraphNode>>> }
   - 创建几个节点
   - 添加邻居关系
   - 遍历打印某个节点的所有邻居
   提示：Rc<GraphNode> 允许节点被多个节点引用

────────────────────────────────────

【Deref 和 Drop 练习】

10. 实现自定义智能指针
    要求：
    - 定义 struct MyBox<T>(T)
    - 实现 new 方法
    - 实现 Deref trait
    - 测试解引用：*mybox
    - 测试解引用强制转换（传给接受 &str 的函数）
    提示：参考课程中的 MyBox 示例

11. 实现带日志的资源管理
    要求：
    - 定义 struct Resource { name: String }
    - 实现 Drop trait，在 drop 时打印 "释放资源: {name}"
    - 创建多个 Resource，观察释放顺序
    - 用 drop() 函数提前释放一个
    提示：Drop 的顺序是后进先出（LIFO）

────────────────────────────────────

【综合挑战】

12. 实现一个引用计数的树（带父节点）
    要求：
    - 定义 struct TreeNode {
          value: i32,
          parent: RefCell<Weak<TreeNode>>,  // 弱引用父节点
          children: RefCell<Vec<Rc<TreeNode>>>, // 强引用子节点
      }
    - 创建一个根节点和几个子节点
    - 设置父子关系
    - 从子节点访问父节点（通过 upgrade）
    - 打印引用计数（strong_count 和 weak_count）
    挑战：理解为什么父节点用 Weak，子节点用 Rc
    思考：如果父节点也用 Rc 会发生什么？

*/

// ============================================
// 主函数
// ============================================

fn main() {
    println!("============================================");
    println!("智能指针 - 学习笔记");
    println!("============================================");

    part1_concepts();
    part2_basic_usage();
    part4_common_errors();

    println!("\n============================================");
    println!("学习完成！");
    println!("============================================");
    println!("\n💡 关键要点：");
    println!("1. Box<T>：堆分配，单一所有者，递归类型");
    println!("2. Rc<T>：引用计数，多个所有者（单线程）");
    println!("3. RefCell<T>：内部可变性，运行时借用检查");
    println!("4. Rc<RefCell<T>>：多个所有者 + 可变数据");
    println!("5. Deref：让智能指针像引用一样使用");
    println!("6. Drop：离开作用域自动清理");
    println!("7. Weak<T>：弱引用，打破循环引用");
    println!("\n🎯 下一步：完成练习题，巩固智能指针的使用！");
}

