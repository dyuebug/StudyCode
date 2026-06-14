// ============================================
// Rust 并发编程 - 完整教程
// 日期：2026-06-16
// 难度：⭐⭐⭐⭐ 高级且重要
// ============================================

/*
【核心概念】并发编程 (Concurrency)

是什么？
- 并发：程序的不同部分可以独立执行
- 线程：操作系统调度的最小执行单元
- Rust 并发：编译期保证内存安全和线程安全

为什么需要？
- 充分利用多核 CPU
- 提高程序性能
- 处理 I/O 密集型任务（网络、文件）

核心特点？
- 无数据竞争：编译器保证
- Send trait：可以在线程间转移所有权
- Sync trait：可以在线程间共享引用

Rust特色？
- 零成本抽象：编译期检查，无运行时开销
- 内存安全：所有权系统防止数据竞争
- "Fearless Concurrency"：无畏并发
*/

use std::sync::{mpsc, Arc, Mutex, RwLock};
use std::thread;
use std::time::Duration;

// ============================================
// 模块1：线程基础
// ============================================

fn module1_threads() {
    println!("============================================================");
    println!("模块1：线程基础");
    println!("============================================================");

    // --------------------------------------------
    // 1.1 创建线程
    // --------------------------------------------

    println!("\n1.1 创建线程");
    println!("------------------------------------------------------------");

    /*
    【thread::spawn】

    语法：thread::spawn(闭包)
    返回：JoinHandle<T>
    作用：创建新线程并立即执行

    特点：
    - 新线程与主线程并发执行
    - 闭包在新线程中运行
    - 需要 move 捕获环境（通常）
    */

    // 示例1：简单线程
    let handle = thread::spawn(|| {
        for i in 1..5 {
            println!("  子线程: {}", i);
            thread::sleep(Duration::from_millis(100));
        }
    });

    // 主线程继续执行
    for i in 1..3 {
        println!("主线程: {}", i);
        thread::sleep(Duration::from_millis(100));
    }

    // 等待子线程完成
    handle.join().unwrap();
    // join() 阻塞当前线程，直到子线程完成
    // 返回 Result<T, E>

    println!("→ 线程并发执行，join() 等待完成");

    // --------------------------------------------
    // 1.2 move 闭包
    // --------------------------------------------

    println!("\n1.2 move 闭包");
    println!("------------------------------------------------------------");

    /*
    【move 关键字】

    为什么需要？
    - 闭包捕获环境变量
    - 新线程可能活得比原线程长
    - 必须转移所有权到新线程

    使用场景：
    - 几乎所有 thread::spawn 都需要 move
    - 确保数据在线程中有效
    */

    let v = vec![1, 2, 3];

    let handle = thread::spawn(move || {
        // move 强制获取 v 的所有权
        println!("  子线程中的向量: {:?}", v);
    });

    // println!("{:?}", v); // ❌ 编译错误：v 已被移动

    handle.join().unwrap();

    println!("→ move 确保数据所有权转移到新线程");

    // --------------------------------------------
    // 1.3 返回值
    // --------------------------------------------

    println!("\n1.3 线程返回值");
    println!("------------------------------------------------------------");

    /*
    【JoinHandle<T>】

    join() 返回 Result<T, E>：
    - T：闭包的返回值类型
    - E：线程 panic 的错误

    使用场景：
    - 从子线程获取计算结果
    */

    let handle = thread::spawn(|| {
        // 计算并返回结果
        let mut sum = 0;
        for i in 1..=100 {
            sum += i;
        }
        sum // 返回值
    });

    let result = handle.join().unwrap();
    println!("  子线程计算结果: {}", result); // 5050

    println!("→ join() 获取子线程返回值");
}

// ============================================
// 模块2：消息传递（Channels）
// ============================================

fn module2_channels() {
    println!("\n============================================================");
    println!("模块2：消息传递（Channels）");
    println!("============================================================");

    // --------------------------------------------
    // 2.1 基础通道
    // --------------------------------------------

    println!("\n2.1 基础通道");
    println!("------------------------------------------------------------");

    /*
    【mpsc::channel】

    mpsc = Multiple Producer, Single Consumer
    多生产者，单消费者

    语法：
    let (tx, rx) = mpsc::channel();
    - tx：发送端（Sender）
    - rx：接收端（Receiver）

    特点：
    - 线程安全的消息传递
    - tx 可以克隆（多个发送者）
    - rx 不能克隆（单个接收者）
    */

    let (tx, rx) = mpsc::channel();

    thread::spawn(move || {
        let msg = String::from("Hello from thread!");
        tx.send(msg).unwrap();
        // send() 转移所有权
        // println!("{}", msg); // ❌ 编译错误：msg 已被移动
    });

    let received = rx.recv().unwrap();
    // recv() 阻塞等待消息
    // 返回 Result<T, RecvError>
    println!("  收到消息: {}", received);

    println!("→ channel 实现线程间安全通信");

    // --------------------------------------------
    // 2.2 发送多个消息
    // --------------------------------------------

    println!("\n2.2 发送多个消息");
    println!("------------------------------------------------------------");

    let (tx, rx) = mpsc::channel();

    thread::spawn(move || {
        let vals = vec![
            String::from("消息1"),
            String::from("消息2"),
            String::from("消息3"),
        ];

        for val in vals {
            tx.send(val).unwrap();
            thread::sleep(Duration::from_millis(100));
        }
    });

    // 接收所有消息
    // rx 实现了 Iterator trait
    for received in rx {
        println!("  收到: {}", received);
    }
    // 当发送端关闭（drop），迭代结束

    println!("→ 可以发送多个消息，rx 是迭代器");

    // --------------------------------------------
    // 2.3 多个生产者
    // --------------------------------------------

    println!("\n2.3 多个生产者");
    println!("------------------------------------------------------------");

    let (tx, rx) = mpsc::channel();

    // 克隆发送端
    let tx1 = tx.clone();
    let tx2 = tx.clone();

    // 生产者1
    thread::spawn(move || {
        tx1.send("生产者1: 消息A".to_string()).unwrap();
        thread::sleep(Duration::from_millis(50));
        tx1.send("生产者1: 消息B".to_string()).unwrap();
    });

    // 生产者2
    thread::spawn(move || {
        tx2.send("生产者2: 消息X".to_string()).unwrap();
        thread::sleep(Duration::from_millis(50));
        tx2.send("生产者2: 消息Y".to_string()).unwrap();
    });

    // 释放原始 tx（否则 rx 不会结束）
    drop(tx);

    // 接收所有消息
    for received in rx {
        println!("  {}", received);
    }

    println!("→ 多个发送者可以克隆 tx");
}

// ============================================
// 模块3：共享状态（Mutex 和 Arc）
// ============================================

fn module3_shared_state() {
    println!("\n============================================================");
    println!("模块3：共享状态（Mutex 和 Arc）");
    println!("============================================================");

    // --------------------------------------------
    // 3.1 Mutex 互斥锁
    // --------------------------------------------

    println!("\n3.1 Mutex 互斥锁");
    println!("------------------------------------------------------------");

    /*
    【Mutex<T>】

    Mutex = Mutual Exclusion（互斥）

    作用：
    - 保证同一时间只有一个线程访问数据
    - 提供内部可变性

    API：
    - lock()：获取锁，返回 MutexGuard<T>
    - MutexGuard：智能指针，离开作用域自动释放锁

    特点：
    - 阻塞：如果锁被占用，lock() 会等待
    - 自动释放：MutexGuard drop 时释放锁
    */

    let m = Mutex::new(5);

    {
        let mut num = m.lock().unwrap();
        // lock() 获取锁，返回 MutexGuard
        *num = 6; // 修改数据
    } // MutexGuard 离开作用域，自动释放锁

    println!("  m = {:?}", m);

    println!("→ Mutex 保证同一时间只有一个线程访问");

    // --------------------------------------------
    // 3.2 Arc + Mutex 组合
    // --------------------------------------------

    println!("\n3.2 Arc + Mutex 组合");
    println!("------------------------------------------------------------");

    /*
    【Arc<T>】

    Arc = Atomic Reference Counted
    原子引用计数

    作用：
    - 多线程共享所有权
    - 线程安全的引用计数（Rc 的多线程版本）

    【Arc<Mutex<T>> 组合】

    为什么需要两者？
    - Arc：允许多个线程共享所有权
    - Mutex：允许修改共享数据

    模式：
    Arc<Mutex<T>> = 多线程共享可变数据
    */

    let counter = Arc::new(Mutex::new(0));
    let mut handles = vec![];

    for i in 0..10 {
        let counter = Arc::clone(&counter);
        // 克隆 Arc，增加引用计数

        let handle = thread::spawn(move || {
            let mut num = counter.lock().unwrap();
            *num += 1;
            println!("  线程 {} 增加计数器", i);
        });

        handles.push(handle);
    }

    // 等待所有线程完成
    for handle in handles {
        handle.join().unwrap();
    }

    println!("  最终计数: {}", *counter.lock().unwrap());

    println!("→ Arc<Mutex<T>> 实现多线程共享可变数据");

    // --------------------------------------------
    // 3.3 RwLock 读写锁
    // --------------------------------------------

    println!("\n3.3 RwLock 读写锁");
    println!("------------------------------------------------------------");

    /*
    【RwLock<T>】

    RwLock = Read-Write Lock

    特点：
    - 多个读者 OR 一个写者
    - 读多写少的场景更高效

    API：
    - read()：获取读锁，返回 RwLockReadGuard
    - write()：获取写锁，返回 RwLockWriteGuard

    与 Mutex 对比：
    - Mutex：任何访问都需要独占锁
    - RwLock：多个线程可以同时读
    */

    let data = Arc::new(RwLock::new(vec![1, 2, 3]));
    let mut handles = vec![];

    // 多个读线程
    for i in 0..5 {
        let data = Arc::clone(&data);
        let handle = thread::spawn(move || {
            let vec = data.read().unwrap();
            println!("  读线程 {}: {:?}", i, *vec);
        });
        handles.push(handle);
    }

    // 一个写线程
    let data_clone = Arc::clone(&data);
    let handle = thread::spawn(move || {
        thread::sleep(Duration::from_millis(10));
        let mut vec = data_clone.write().unwrap();
        vec.push(4);
        println!("  写线程: 添加元素");
    });
    handles.push(handle);

    for handle in handles {
        handle.join().unwrap();
    }

    println!("  最终数据: {:?}", *data.read().unwrap());

    println!("→ RwLock 允许多个读者或一个写者");
}

fn main() {
    println!("============================================");
    println!("Rust 并发编程 - 完整教程");
    println!("============================================");

    module1_threads();
    module2_channels();
    module3_shared_state();

    println!("\n============================================");
    println!("学习完成！");
    println!("============================================");

    println!("\n💡 关键要点：");
    println!("1. 线程：thread::spawn + move 闭包");
    println!("2. 消息传递：mpsc::channel，多生产者单消费者");
    println!("3. 共享状态：Arc<Mutex<T>>，多线程共享可变数据");
    println!("4. RwLock：多读一写，读多写少场景更高效");
    println!("5. Send + Sync：Rust 的线程安全保证");
    println!("\n🎯 下一步：实践这些概念，构建并发应用！");
}
