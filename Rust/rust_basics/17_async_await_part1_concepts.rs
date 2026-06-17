// ============================================
// 异步编程 (async/await) - 超详细注释版
// 日期：2026-06-18（第22天）
// 难度：⭐⭐⭐⭐ 高级
// ============================================

/*
【核心概念】简要说明

是什么？
- 异步编程是一种"等待时不阻塞"的编程方式
- 让程序在等待（如网络请求）时可以做其他事情

为什么需要？
- 传统同步代码：等待时 CPU 空闲，浪费资源
- 异步代码：等待时可以处理其他任务，高效利用 CPU
- 特别适合 I/O 密集型任务（网络、文件读写）

核心特点？
- async：将函数标记为异步，返回 Future
- await：等待 Future 完成
- 需要运行时（runtime）来调度执行

Rust 特色？
- 零成本抽象：异步代码编译后性能接近手写状态机
- 类型安全：Future 是类型系统的一部分
- 需要显式 .await（不会自动等待）
*/

// ============================================
// 第一部分：理解异步的必要性 ⭐⭐⭐⭐⭐
// ============================================

/*
在写异步代码之前，我们先理解"为什么需要异步"。
*/

use std::thread;
use std::time::Duration;

fn synchronous_example() {
    println!("============================================================");
    println!("对比：同步 vs 异步");
    println!("============================================================\n");

    println!("【场景】：煮3个鸡蛋，每个需要10秒");
    println!();

    // 同步方式：一个一个煮
    println!("方式A：同步（一个一个来）");
    let start = std::time::Instant::now();

    println!("  🥚 煮第1个鸡蛋...");
    thread::sleep(Duration::from_secs(3)); // 模拟煮蛋（实际10秒，这里缩短到3秒）
    println!("  ✅ 第1个完成");

    println!("  🥚 煮第2个鸡蛋...");
    thread::sleep(Duration::from_secs(3));
    println!("  ✅ 第2个完成");

    println!("  🥚 煮第3个鸡蛋...");
    thread::sleep(Duration::from_secs(3));
    println!("  ✅ 第3个完成");

    let elapsed = start.elapsed();
    println!("\n  总耗时：{:.1}秒", elapsed.as_secs_f64());
    println!("  😫 太慢了！每次都要等上一个完成\n");

    /*
    问题在哪？
    - 煮蛋时，我们只是"等待"，CPU 其实空闲
    - 但程序被"阻塞"了，不能做别的事
    - 这就是"同步阻塞"
    */
}

// ============================================
// 第二部分：Future 的概念 ⭐⭐⭐⭐⭐
// ============================================

/*
在 Rust 中，异步的核心是 Future trait。

【Future 是什么？】

Future = "未来的值"
- 表示一个"尚未完成"的计算
- 就像一张"欠条"：现在没有值，但将来会有

【类比】：
- 同步：你去餐厅点餐，站在那里等厨师做好（阻塞）
- 异步：你拿到一个"号码牌"（Future），可以坐着玩手机，叫号时再取餐

【Future trait（简化版）】：

trait Future {
    type Output;  // 未来会返回什么类型的值

    fn poll(&mut self, cx: &mut Context) -> Poll<Self::Output>;
    // poll = "轮询"，问一句："完成了吗？"
    // 返回 Poll::Ready(value) 或 Poll::Pending
}

【重要】：
- Future 本身不会自动执行
- 需要运行时（runtime）反复 poll 它
- 这就是为什么需要 tokio
*/

fn future_concept() {
    println!("============================================================");
    println!("理解 Future 的概念");
    println!("============================================================\n");

    println!("【Future 类比】");
    println!();
    println!("  餐厅点餐：");
    println!("  1. 你：我要一份炒饭");
    println!("  2. 服务员：给你一个号码牌（Future）");
    println!("  3. 你：拿着号码牌，可以坐下玩手机");
    println!("  4. 广播：3号请取餐！");
    println!("  5. 你：拿着号码牌（Future）去取餐（.await）");
    println!("  6. 你：拿到炒饭（Future 完成，得到值）");
    println!();

    println!("【Rust 中的对应】：");
    println!("  号码牌     → Future");
    println!("  等待叫号   → poll (由 runtime 负责)");
    println!("  去取餐     → .await");
    println!("  拿到炒饭   → 得到 Future 的返回值");
    println!();
}

// ============================================
// 第三部分：async 和 await 语法 ⭐⭐⭐⭐⭐
// ============================================

/*
【async 关键字】

async fn 做了什么？
- 将函数标记为异步
- 函数返回一个 Future，而不是直接返回值
- 函数体内可以使用 .await

例如：
async fn hello() -> String {
    "Hello".to_string()
}

实际上相当于：
fn hello() -> impl Future<Output = String> {
    // 返回一个 Future
}

【await 关键字】

.await 做了什么？
- 等待一个 Future 完成
- 如果 Future 还没完成，会"让出控制权"给其他任务
- 这是关键：不是阻塞等待，而是"挂起"当前任务

【重要区别】：
- 同步等待：thread::sleep() → 阻塞线程，什么都不能做
- 异步等待：.await → 挂起任务，运行时可以执行其他任务
*/

// 注意：这些函数现在还不能运行，因为需要 tokio
// 我们先理解语法，稍后会实际运行

// 一个简单的 async 函数
async fn say_hello() -> String {
    "Hello, Async World!".to_string()
}

// 一个带延迟的 async 函数
async fn cook_egg(number: u32) {
    println!("  🥚 开始煮第{}个鸡蛋...", number);

    // tokio::time::sleep 是异步版本的 sleep
    // 注意：这里先注释掉，后面会取消注释
    // tokio::time::sleep(Duration::from_secs(3)).await;

    println!("  ✅ 第{}个鸡蛋完成！", number);
}

// async 函数可以调用其他 async 函数
async fn cook_all_eggs() {
    // 注意 .await 的使用
    cook_egg(1).await;  // 等待第1个完成
    cook_egg(2).await;  // 等待第2个完成
    cook_egg(3).await;  // 等待第3个完成

    // 这样还是顺序执行的！
    // 要并发执行，需要用 tokio::join! 或 tokio::spawn
}

fn syntax_explanation() {
    println!("============================================================");
    println!("async/await 语法讲解");
    println!("============================================================\n");

    println!("【async 函数签名】：");
    println!();
    println!("  async fn hello() -> String {{ ... }}");
    println!("  ↓ 编译器实际看到的");
    println!("  fn hello() -> impl Future<Output = String> {{ ... }}");
    println!();

    println!("【.await 的作用】：");
    println!();
    println!("  let result = some_future.await;");
    println!("  ↓");
    println!("  1. 检查 Future 是否完成");
    println!("  2. 如果完成 → 返回值");
    println!("  3. 如果未完成 → 挂起当前任务，让其他任务运行");
    println!();

    println!("【关键洞察】：");
    println!("  .await 不是\"阻塞等待\"，而是\"礼貌让座\"");
    println!("  当前任务：我等的事情还没好，你们先来");
    println!("  运行时：好的，那我去执行其他任务");
    println!();
}

// ============================================
// 第四部分：tokio 运行时 ⭐⭐⭐⭐⭐
// ============================================

/*
【为什么需要 tokio？】

问题：async 函数返回 Future，但 Future 不会自动执行
- 就像号码牌不会自动变成炒饭
- 需要有人（运行时）去"轮询"它

tokio 就是这个"运行时"：
- 负责调度和执行 Future
- 管理任务队列
- 处理 I/O 事件

【使用 tokio】：

1. 在 Cargo.toml 添加依赖：
   tokio = { version = "1", features = ["full"] }

2. 在 main 函数上加 #[tokio::main]：
   #[tokio::main]
   async fn main() {
       // 现在可以用 .await 了
   }

3. 或者手动创建运行时：
   let rt = tokio::runtime::Runtime::new().unwrap();
   rt.block_on(async {
       // 异步代码
   });
*/

fn runtime_explanation() {
    println!("============================================================");
    println!("tokio 运行时讲解");
    println!("============================================================\n");

    println!("【为什么需要运行时？】");
    println!();
    println!("  async 函数 → 返回 Future（一张\"欠条\"）");
    println!("  Future → 需要有人去\"兑现\"它");
    println!("  tokio → 就是那个\"兑现者\"");
    println!();

    println!("【tokio 做什么？】");
    println!("  1. 维护一个任务队列");
    println!("  2. 反复 poll 每个 Future");
    println!("  3. Future 完成时返回结果");
    println!("  4. Future 未完成时切换到其他任务");
    println!();

    println!("【使用方式】：");
    println!();
    println!("  // 方式1：宏（最简单）");
    println!("  #[tokio::main]");
    println!("  async fn main() {{");
    println!("      let result = some_async_fn().await;");
    println!("  }}");
    println!();
    println!("  // 方式2：手动（更灵活）");
    println!("  fn main() {{");
    println!("      let rt = Runtime::new().unwrap();");
    println!("      rt.block_on(async {{");
    println!("          let result = some_async_fn().await;");
    println!("      }});");
    println!("  }}");
    println!();
}

// ============================================
// 主函数：概念讲解部分
// ============================================

fn main() {
    println!("\n🌐 Rust 异步编程 (async/await) 学习\n");

    // 第一部分：对比同步和异步
    synchronous_example();

    println!("\n按回车继续...");
    let mut input = String::new();
    std::io::stdin().read_line(&mut input).ok();

    // 第二部分：Future 概念
    future_concept();

    println!("\n按回车继续...");
    std::io::stdin().read_line(&mut input).ok();

    // 第三部分：语法讲解
    syntax_explanation();

    println!("\n按回车继续...");
    std::io::stdin().read_line(&mut input).ok();

    // 第四部分：运行时讲解
    runtime_explanation();

    println!("============================================================");
    println!("✅ 概念讲解完成！");
    println!("============================================================\n");

    println!("💡 下一步：");
    println!("  现在你理解了异步的\"为什么\"和\"是什么\"");
    println!("  接下来我们会创建实际可运行的异步代码");
    println!("  需要先安装 tokio：在项目的 Cargo.toml 添加依赖");
    println!();
}

/*
==============================================
🔍 深入理解：async/await 的底层（选学）
==============================================

如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【async fn 的展开】

async fn hello() -> String {
    "Hello".to_string()
}

编译器会展开成类似这样：

fn hello() -> impl Future<Output = String> {
    // 生成一个状态机
    struct HelloFuture;

    impl Future for HelloFuture {
        type Output = String;

        fn poll(&mut self, cx: &mut Context) -> Poll<String> {
            Poll::Ready("Hello".to_string())
        }
    }

    HelloFuture
}

【.await 的展开】

let result = some_future.await;

展开成：

loop {
    match some_future.poll(cx) {
        Poll::Ready(value) => {
            result = value;
            break;
        }
        Poll::Pending => {
            // 挂起当前任务，让运行时执行其他任务
            // 运行时会在适当时机再次 poll
            yield_to_runtime();
        }
    }
}

【为什么 Rust 的异步是零成本的？】

1. 编译期展开：
   - async/await 是语法糖
   - 编译器展开成状态机
   - 没有运行时开销

2. 不需要堆分配：
   - Future 可以在栈上
   - 不像其他语言需要装箱

3. 内联优化：
   - 编译器可以内联 Future 的 poll
   - 最终代码接近手写状态机

【状态机的例子】

async fn example() {
    println!("Step 1");
    some_async_fn().await;
    println!("Step 2");
    another_async_fn().await;
    println!("Step 3");
}

编译器生成的状态机：

enum ExampleState {
    Start,
    WaitingFirst,
    WaitingSecond,
    Done,
}

struct ExampleFuture {
    state: ExampleState,
    // 保存中间结果
}

impl Future for ExampleFuture {
    fn poll(&mut self, cx: &mut Context) -> Poll<()> {
        loop {
            match self.state {
                Start => {
                    println!("Step 1");
                    self.state = WaitingFirst;
                    // poll some_async_fn
                }
                WaitingFirst => {
                    // 继续 poll some_async_fn
                    // 如果 Ready，进入下一状态
                    // 如果 Pending，返回 Pending
                }
                // ... 其他状态
            }
        }
    }
}

这就是为什么 Rust 的异步如此高效：
- 没有额外开销
- 状态机在栈上
- 编译器优化

==============================================
*/
