// ============================================
// 异步编程实战 - tokio 示例
// 展示异步的真正威力
// ============================================

use tokio::time::{sleep, Duration};
use std::time::Instant;

// ============================================
// 示例1：基础 async 函数
// ============================================

async fn say_hello() -> String {
    "Hello, Async World!".to_string()
}

async fn basic_example() {
    println!("============================================================");
    println!("示例1：基础 async 函数");
    println!("============================================================\n");

    // 调用 async 函数，必须用 .await
    let message = say_hello().await;
    println!("消息：{}\n", message);

    /*
    关键点：
    - say_hello() 返回 Future，不是 String
    - .await 等待 Future 完成，得到 String
    */
}

// ============================================
// 示例2：对比同步 vs 异步的区别
// ============================================

async fn cook_egg_async(number: u32) {
    println!("  🥚 开始煮第{}个鸡蛋", number);
    sleep(Duration::from_secs(2)).await;  // 异步 sleep
    println!("  ✅ 第{}个鸡蛋完成", number);
}

async fn sequential_cooking() {
    println!("============================================================");
    println!("示例2：顺序执行（还是慢）");
    println!("============================================================\n");

    let start = Instant::now();

    // 顺序执行：一个接一个
    cook_egg_async(1).await;
    cook_egg_async(2).await;
    cook_egg_async(3).await;

    let elapsed = start.elapsed();
    println!("\n总耗时：{:.1}秒", elapsed.as_secs_f64());
    println!("😐 还是要等每个完成...\n");

    /*
    问题：虽然用了 async，但顺序 .await 还是会一个一个等
    解决：需要并发执行！
    */
}

// ============================================
// 示例3：并发执行 - tokio::join!
// ============================================

async fn concurrent_cooking_join() {
    println!("============================================================");
    println!("示例3：并发执行 - tokio::join!");
    println!("============================================================\n");

    let start = Instant::now();

    // tokio::join! 并发执行多个 Future
    tokio::join!(
        cook_egg_async(1),
        cook_egg_async(2),
        cook_egg_async(3)
    );

    let elapsed = start.elapsed();
    println!("\n总耗时：{:.1}秒", elapsed.as_secs_f64());
    println!("😎 3个一起煮，只要2秒！\n");

    /*
    tokio::join! 的作用：
    - 同时启动多个 Future
    - 等待所有 Future 完成
    - 返回所有结果的元组
    */
}

// ============================================
// 示例4：并发执行 - tokio::spawn（真正的任务）
// ============================================

async fn concurrent_cooking_spawn() {
    println!("============================================================");
    println!("示例4：并发执行 - tokio::spawn");
    println!("============================================================\n");

    let start = Instant::now();

    // tokio::spawn 创建独立任务
    let task1 = tokio::spawn(cook_egg_async(1));
    let task2 = tokio::spawn(cook_egg_async(2));
    let task3 = tokio::spawn(cook_egg_async(3));

    // 等待所有任务完成
    let _ = tokio::join!(task1, task2, task3);

    let elapsed = start.elapsed();
    println!("\n总耗时：{:.1}秒", elapsed.as_secs_f64());
    println!("🚀 用 spawn 创建独立任务\n");

    /*
    tokio::spawn vs tokio::join!：
    - spawn：创建独立任务，可以在后台运行
    - join!：在当前任务中并发执行
    - 都能实现并发，spawn 更灵活
    */
}

// ============================================
// 示例5：有返回值的异步函数
// ============================================

async fn fetch_data(id: u32) -> String {
    println!("  📡 开始获取数据 {}...", id);
    sleep(Duration::from_secs(1)).await;
    println!("  ✅数据 {} 获取成功", id);
    format!("Data-{}", id)
}

async fn concurrent_with_results() {
    println!("============================================================");
    println!("示例5：并发执行并获取结果");
    println!("============================================================\n");

    let start = Instant::now();

    // 并发执行并获取所有结果
    let (data1, data2, data3) = tokio::join!(
        fetch_data(1),
        fetch_data(2),
        fetch_data(3)
    );

    let elapsed = start.elapsed();

    println!("\n结果：");
    println!("  {}", data1);
    println!("  {}", data2);
    println!("  {}", data3);
    println!("\n总耗时：{:.1}秒", elapsed.as_secs_f64());
    println!("💡 3个请求并发，只用了1秒\n");
}

// ============================================
// 示例6：错误处理
// ============================================

async fn might_fail(id: u32) -> Result<String, String> {
    sleep(Duration::from_millis(500)).await;

    if id == 2 {
        Err(format!("任务 {} 失败了", id))
    } else {
        Ok(format!("任务 {} 成功", id))
    }
}

async fn error_handling_example() {
    println!("============================================================");
    println!("示例6：异步错误处理");
    println!("============================================================\n");

    // 方式1：逐个处理
    match might_fail(1).await {
        Ok(result) => println!("✅ {}", result),
        Err(e) => println!("❌ {}", e),
    }

    match might_fail(2).await {
        Ok(result) => println!("✅ {}", result),
        Err(e) => println!("❌ {}", e),
    }

    // 方式2：? 运算符（在返回 Result 的 async 函数中）
    println!();
}

// ============================================
// 示例7：超时控制
// ============================================

async fn slow_task() -> String {
    println!("  ⏳ 慢任务开始...");
    sleep(Duration::from_secs(5)).await;
    "慢任务完成".to_string()
}

async fn timeout_example() {
    println!("============================================================");
    println!("示例7：超时控制");
    println!("============================================================\n");

    // 给异步操作设置超时
    let result = tokio::time::timeout(
        Duration::from_secs(2),
        slow_task()
    ).await;

    match result {
        Ok(value) => println!("✅ 任务完成：{}", value),
        Err(_) => println!("⏰ 任务超时！（超过2秒）"),
    }

    println!();
}

// ============================================
// 示例8：select! - 谁先完成用谁
// ============================================

async fn task_a() -> &'static str {
    sleep(Duration::from_secs(2)).await;
    "Task A"
}

async fn task_b() -> &'static str {
    sleep(Duration::from_secs(1)).await;
    "Task B"
}

async fn select_example() {
    println!("============================================================");
    println!("示例8：select! - 竞速");
    println!("============================================================\n");

    let start = Instant::now();

    // tokio::select! 等待第一个完成的
    let result = tokio::select! {
        a = task_a() => a,
        b = task_b() => b,
    };

    let elapsed = start.elapsed();

    println!("获胜者：{}", result);
    println!("耗时：{:.1}秒", elapsed.as_secs_f64());
    println!("💡 Task B 更快，所以返回它\n");

    /*
    select! 的用途：
    - 多个操作，只要一个完成
    - 超时控制
    - 取消操作
    */
}

// ============================================
// 主函数
// ============================================

#[tokio::main]  // 这个宏创建 tokio 运行时
async fn main() {
    println!("\n🚀 Tokio 异步编程实战\n");

    // 示例1：基础
    basic_example().await;

    println!("按回车继续...");
    let mut input = String::new();
    std::io::stdin().read_line(&mut input).ok();

    // 示例2：顺序执行
    sequential_cooking().await;

    println!("按回车继续...");
    std::io::stdin().read_line(&mut input).ok();

    // 示例3：并发执行（join!）
    concurrent_cooking_join().await;

    println!("按回车继续...");
    std::io::stdin().read_line(&mut input).ok();

    // 示例4：并发执行（spawn）
    concurrent_cooking_spawn().await;

    println!("按回车继续...");
    std::io::stdin().read_line(&mut input).ok();

    // 示例5：有返回值
    concurrent_with_results().await;

    println!("按回车继续...");
    std::io::stdin().read_line(&mut input).ok();

    // 示例6：错误处理
    error_handling_example().await;

    println!("按回车继续...");
    std::io::stdin().read_line(&mut input).ok();

    // 示例7：超时控制
    timeout_example().await;

    println!("按回车继续...");
    std::io::stdin().read_line(&mut input).ok();

    // 示例8：select!
    select_example().await;

    println!("============================================================");
    println!("✅ 所有示例完成！");
    println!("============================================================\n");

    println!("💡 关键要点：");
    println!("1. async 函数返回 Future");
    println!("2. .await 等待 Future 完成");
    println!("3. tokio::join! 并发执行");
    println!("4. tokio::spawn 创建独立任务");
    println!("5. 异步特别适合 I/O 密集型任务");
    println!();
}
