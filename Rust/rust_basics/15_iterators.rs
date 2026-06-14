// ============================================
// 迭代器 - 超详细注释版
// 日期：2026-06-16（第17天）
// 难度：⭐⭐⭐ 重要
// ============================================

/*
【核心概念】迭代器 (Iterators)

是什么？
- 迭代器是一种模式，允许你遍历集合中的元素
- 核心是 Iterator trait：fn next(&mut self) -> Option<Self::Item>
- Rust 中几乎所有集合都实现了迭代器

为什么需要？
- 代码简洁：比手写循环更清晰
- 函数式编程：链式调用、组合操作
- 零成本抽象：性能和手写循环一样

核心特点？
- 惰性求值：适配器不会立即执行
- 链式调用：可以组合多个操作
- 类型安全：编译期保证正确性

Rust特色？
- 零成本抽象：编译器内联优化后和手写循环性能完全一样
- 所有权集成：iter()、into_iter()、iter_mut() 对应三种所有权模式
*/

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

fn part1_concepts() {
    println!("============================================================");
    println!("第一部分：核心概念详解");
    println!("============================================================");

    // --------------------------------------------
    // 1.1 什么是迭代器？
    // --------------------------------------------

    println!("\n1.1 什么是迭代器？");
    println!("------------------------------------------------------------");

    /*
    【迭代器 Iterator】

    是什么？
    - 实现了 Iterator trait 的类型
    - 可以逐个访问集合中的元素
    - 核心方法：next(&mut self) -> Option<Self::Item>

    Iterator trait 定义：
    ```rust
    trait Iterator {
        type Item;
        fn next(&mut self) -> Option<Self::Item>;
        // 还有很多基于 next 实现的默认方法
    }
    ```

    工作原理：
    - 每次调用 next() 返回 Some(item) 或 None
    - None 表示迭代结束
    - 所有其他方法都基于 next 实现

    Rust 特色：
    - 几乎所有集合都实现了迭代器
    - for 循环实际上就是迭代器的语法糖
    - 零成本抽象：性能和手写索引循环一样
    */

    // 示例1：手动调用 next
    let v = vec![1, 2, 3];
    let mut iter = v.iter();
    // iter() 返回迭代器

    println!("手动调用 next:");
    println!("{:?}", iter.next()); // Some(1)
    println!("{:?}", iter.next()); // Some(2)
    println!("{:?}", iter.next()); // Some(3)
    println!("{:?}", iter.next()); // None

    // 示例2：for 循环是迭代器的语法糖
    let v = vec![1, 2, 3];
    println!("\nfor 循环遍历:");
    for item in v.iter() {
        println!("{}", item);
    }

    println!("→ 迭代器的核心是 next() 方法，返回 Option<Item>");

    // --------------------------------------------
    // 1.2 三种迭代方法
    // --------------------------------------------

    println!("\n1.2 三种迭代方法");
    println!("------------------------------------------------------------");

    /*
    【iter() - 不可变引用】
    - 返回：Iterator<Item = &T>
    - 元素类型：&T（不可变引用）
    - 所有权：不消耗集合，集合仍可用
    - 使用场景：只需读取元素

    【into_iter() - 获取所有权】
    - 返回：Iterator<Item = T>
    - 元素类型：T（拥有所有权）
    - 所有权：消耗集合，集合不再可用
    - 使用场景：需要移动元素、集合不再使用

    【iter_mut() - 可变引用】
    - 返回：Iterator<Item = &mut T>
    - 元素类型：&mut T（可变引用）
    - 所有权：可变借用集合，可以修改元素
    - 使用场景：需要修改元素

    选择指南：
    - 只读？→ iter()
    - 修改？→ iter_mut()
    - 消耗/移动？→ into_iter()
    */

    // 示例1：iter() - 不可变引用
    let v1 = vec![1, 2, 3];
    println!("\niter() - 不可变引用:");
    for item in v1.iter() {
        // item 的类型是 &i32
        println!("  {}", item);
    }
    println!("v1 仍可用: {:?}", v1); // ✅ v1 仍然可用

    // 示例2：into_iter() - 获取所有权
    let v2 = vec![String::from("a"), String::from("b")];
    println!("\ninto_iter() - 获取所有权:");
    for item in v2.into_iter() {
        // item 的类型是 String
        println!("  {}", item);
    }
    // println!("{:?}", v2); // ❌ 编译错误：v2 已被移动

    // 示例3：iter_mut() - 可变引用
    let mut v3 = vec![1, 2, 3];
    println!("\niter_mut() - 可变引用:");
    for item in v3.iter_mut() {
        // item 的类型是 &mut i32
        *item += 10; // 修改元素
    }
    println!("修改后: {:?}", v3); // [11, 12, 13]

    println!("→ 三种迭代方法对应三种所有权模式：&T、T、&mut T");
}

// ============================================
// 第二部分：基础操作实践
// ============================================

fn part2_basic_usage() {
    println!("\n============================================================");
    println!("第二部分：基础操作实践");
    println!("============================================================");

    // --------------------------------------------
    // 2.1 消费器（Consumers）
    // --------------------------------------------

    println!("\n2.1 消费器（Consumers）");
    println!("------------------------------------------------------------");

    /*
    【消费器 Consumers】

    是什么？
    - 消耗迭代器，产生最终结果
    - 调用 next() 直到返回 None
    - 例：collect、sum、fold、count

    特点：
    - 触发实际的迭代执行
    - 迭代器被消耗，不能再使用
    - 必须在适配器链的最后调用

    常用消费器：
    - collect()：收集到集合
    - sum()、product()：求和、求积
    - fold()：折叠操作
    - count()：计数
    - any()、all()：判断
    */

    let numbers = vec![1, 2, 3, 4, 5];

    // collect() - 收集到集合
    let doubled: Vec<i32> = numbers.iter()
        .map(|x| x * 2)
        .collect();
    println!("collect: {:?}", doubled); // [2, 4, 6, 8, 10]

    // sum() - 求和
    let sum: i32 = numbers.iter().sum();
    println!("sum: {}", sum); // 15

    // fold() - 折叠操作
    let product = numbers.iter().fold(1, |acc, x| acc * x);
    //                                  ↑初始值  ↑累积函数
    println!("fold (product): {}", product); // 120

    // count() - 计数
    let count = numbers.iter().count();
    println!("count: {}", count); // 5

    // any() - 是否有元素满足条件
    let has_even = numbers.iter().any(|x| x % 2 == 0);
    println!("any (has even): {}", has_even); // true

    // all() - 是否所有元素满足条件
    let all_positive = numbers.iter().all(|x| *x > 0);
    println!("all (positive): {}", all_positive); // true

    println!("→ 消费器触发迭代执行，产生最终结果");

    // --------------------------------------------
    // 2.2 适配器（Adapters）
    // --------------------------------------------

    println!("\n2.2 适配器（Adapters）");
    println!("------------------------------------------------------------");

    /*
    【适配器 Adapters】

    是什么？
    - 产生新的迭代器
    - 不会立即执行（惰性求值）
    - 必须调用消费器才会实际迭代

    特点：
    - 惰性：只是构建操作链
    - 可组合：可以链式调用多个适配器
    - 零成本：编译器优化后和手写循环一样快

    常用适配器：
    - map()：转换元素
    - filter()：过滤元素
    - take()：取前N个
    - skip()：跳过前N个
    - enumerate()：添加索引
    - zip()：合并两个迭代器
    */

    let numbers = vec![1, 2, 3, 4, 5];

    // map() - 转换元素
    let doubled: Vec<i32> = numbers.iter()
        .map(|x| x * 2)
        .collect();
    println!("map (x * 2): {:?}", doubled);

    // filter() - 过滤元素
    let evens: Vec<&i32> = numbers.iter()
        .filter(|x| *x % 2 == 0)
        .collect();
    println!("filter (even): {:?}", evens); // [2, 4]

    // take() - 取前N个
    let first_three: Vec<&i32> = numbers.iter()
        .take(3)
        .collect();
    println!("take(3): {:?}", first_three); // [1, 2, 3]

    // skip() - 跳过前N个
    let skip_two: Vec<&i32> = numbers.iter()
        .skip(2)
        .collect();
    println!("skip(2): {:?}", skip_two); // [3, 4, 5]

    // enumerate() - 添加索引
    println!("enumerate:");
    for (index, value) in numbers.iter().enumerate() {
        println!("  [{}] = {}", index, value);
    }

    println!("→ 适配器是惰性的，必须调用消费器才执行");

    // --------------------------------------------
    // 2.3 链式调用
    // --------------------------------------------

    println!("\n2.3 链式调用");
    println!("------------------------------------------------------------");

    /*
    【链式调用】

    原理：
    - 每个适配器返回新的迭代器
    - 可以连续调用多个适配器
    - 最后用消费器触发执行

    优势：
    - 代码简洁、可读性高
    - 表达力强：描述"做什么"而不是"怎么做"
    - 性能好：编译器优化后和手写循环一样

    模式：
    collection.iter()
        .adapter1()
        .adapter2()
        .adapter3()
        .consumer()
    */

    let numbers = vec![1, 2, 3, 4, 5, 6, 7, 8, 9, 10];

    // 链式调用：map -> filter -> collect
    let result: Vec<i32> = numbers.iter()
        .cloned()                 // &i32 -> i32
        .map(|x| x * 2)           // 每个元素 * 2
        .filter(|x| *x > 10)      // 只保留 > 10 的
        .collect();               // 收集到 Vec
    println!("链式调用结果: {:?}", result); // [12, 14, 16, 18, 20]

    // 复杂链式调用
    let sum: i32 = numbers.iter()
        .filter(|x| *x % 2 == 0)  // 偶数
        .map(|x| x * x)           // 平方
        .take(3)                  // 前3个
        .sum();                   // 求和
    println!("复杂链式: {}", sum); // 4 + 16 + 36 = 56

    println!("→ 链式调用让代码更简洁、更表达力强");

    // --------------------------------------------
    // 2.4 惰性求值
    // --------------------------------------------

    println!("\n2.4 惰性求值");
    println!("------------------------------------------------------------");

    /*
    【惰性求值 Lazy Evaluation】

    是什么？
    - 适配器不会立即执行
    - 只是构建操作链
    - 调用消费器时才实际迭代

    为什么？
    - 性能优化：避免不必要的计算
    - 内存优化：不需要中间结果的额外存储
    - 组合优化：多个操作可以合并执行

    注意：
    - 适配器本身不会做任何事
    - 忘记调用消费器 = 什么都不会发生
    */

    let v = vec![1, 2, 3];

    println!("创建迭代器（不执行）:");
    let iter = v.iter().map(|x| {
        println!("  处理 {}", x);
        x * 2
    });
    // 此时还没有任何输出！

    println!("调用消费器（开始执行）:");
    let result: Vec<i32> = iter.collect();
    // 现在才输出 "处理 1"、"处理 2"、"处理 3"

    println!("结果: {:?}", result);

    println!("→ 惰性求值：适配器只是构建操作链，消费器触发执行");
}

fn main() {
    println!("============================================");
    println!("迭代器 - 学习笔记");
    println!("============================================");

    part1_concepts();
    part2_basic_usage();

    println!("\n============================================");
    println!("学习完成！");
    println!("============================================");

    println!("\n💡 关键要点：");
    println!("1. 三种迭代方法：iter()、into_iter()、iter_mut()");
    println!("2. 消费器：触发执行，产生结果（collect、sum、fold）");
    println!("3. 适配器：惰性求值，产生新迭代器（map、filter、take）");
    println!("4. 链式调用：简洁、表达力强");
    println!("5. 零成本抽象：性能和手写循环一样");
    println!("\n🎯 下一步：完成练习题，巩固迭代器的使用！");
}
