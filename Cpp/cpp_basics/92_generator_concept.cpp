// ============================================
// 92_generator_concept.cpp - 生成器概念示例
// 功能：通过模拟代码理解生成器的工作原理
// 注意：本文件不使用真实的协程，而是用类模拟生成器行为
// 编译：g++ -std=c++20 -o 92_generator_concept.exe 92_generator_concept.cpp
// ============================================

#include <iostream>
#include <vector>
#include <optional>
using namespace std;

// ============================================
// 第一部分：模拟生成器实现
// ============================================

/*
【生成器的核心思想】

生成器是一种特殊的迭代器，具有以下特点：
1. 惰性求值：只在需要时才计算下一个值
2. 节省内存：不需要一次性生成所有值
3. 可以生成无限序列

【模拟实现】

由于真实的协程需要复杂的样板代码，我们用类来模拟生成器的行为：
- 使用成员变量保存状态
- 使用 next() 方法获取下一个值
- 使用 has_next() 方法检查是否还有值
*/

// 模拟生成器：计数器
class CounterGenerator {
private:
    int current;  // 当前值（协程的局部变量）
    int end;      // 结束值
    int state;    // 执行状态（模拟协程的执行位置）

public:
    // 构造函数：初始化生成器
    CounterGenerator(int start, int end) : current(start), end(end), state(0) {
        cout << "[生成器创建] start=" << start << ", end=" << end << endl;
    }

    // 检查是否还有值
    bool has_next() const {
        return current < end;
    }

    // 获取下一个值（模拟 co_yield）
    optional<int> next() {
        if (!has_next()) {
            return nullopt;  // 没有更多值
        }

        // 模拟协程的执行流程
        switch (state) {
            case 0:
                cout << "[生成器] 返回值: " << current << "，然后暂停" << endl;
                state = 1;  // 更新状态
                return current++;  // 返回当前值，然后递增
        }

        return nullopt;
    }
};

// 模拟生成器：斐波那契数列
class FibonacciGenerator {
private:
    int a, b;     // 斐波那契数列的两个变量
    int count;    // 已生成的个数
    int max_count; // 最大生成个数

public:
    FibonacciGenerator(int max_count) : a(0), b(1), count(0), max_count(max_count) {
        cout << "[斐波那契生成器创建] max_count=" << max_count << endl;
    }

    bool has_next() const {
        return count < max_count;
    }

    optional<int> next() {
        if (!has_next()) {
            return nullopt;
        }

        // 返回当前值
        int result = a;
        cout << "[斐波那契] 返回值: " << result << "，然后暂停" << endl;

        // 计算下一个值（协程恢复后执行）
        int temp = a + b;
        a = b;
        b = temp;
        ++count;

        return result;
    }
};

// 模拟生成器：质数生成器
class PrimeGenerator {
private:
    int current;   // 当前检查的数字
    int count;     // 已生成的质数个数
    int max_count; // 最大生成个数

    // 检查是否为质数
    bool is_prime(int n) const {
        if (n < 2) return false;
        for (int i = 2; i * i <= n; ++i) {
            if (n % i == 0) return false;
        }
        return true;
    }

public:
    PrimeGenerator(int max_count) : current(2), count(0), max_count(max_count) {
        cout << "[质数生成器创建] max_count=" << max_count << endl;
    }

    bool has_next() const {
        return count < max_count;
    }

    optional<int> next() {
        if (!has_next()) {
            return nullopt;
        }

        // 查找下一个质数
        while (!is_prime(current)) {
            ++current;
        }

        int result = current;
        cout << "[质数] 返回值: " << result << "，然后暂停" << endl;

        ++current;
        ++count;

        return result;
    }
};

// ============================================
// 第二部分：生成器 vs 传统函数对比
// ============================================

void part1_generator_vs_function() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第一部分：生成器 vs 传统函数对比" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 1.1 传统函数：一次性生成所有值
    // --------------------------------------------

    cout << "\n【传统函数】一次性生成所有值" << endl;
    cout << string(60, '-') << endl;

    {
        // 传统函数：生成 1-10
        auto generate_numbers = [](int n) {
            vector<int> result;
            for (int i = 1; i <= n; ++i) {
                result.push_back(i);
            }
            return result;
        };

        cout << "调用 generate_numbers(10)..." << endl;
        auto numbers = generate_numbers(10);
        cout << "立即生成了所有 " << numbers.size() << " 个数字" << endl;
        cout << "内存占用：" << numbers.size() * sizeof(int) << " 字节" << endl;

        cout << "\n使用前5个数字：";
        for (int i = 0; i < 5; ++i) {
            cout << numbers[i] << " ";
        }
        cout << endl;
        cout << "（但是已经生成了全部10个数字，浪费了内存）" << endl;
    }

    // --------------------------------------------
    // 1.2 生成器：按需生成值
    // --------------------------------------------

    cout << "\n【生成器】按需生成值" << endl;
    cout << string(60, '-') << endl;

    {
        cout << "创建 CounterGenerator(1, 11)..." << endl;
        CounterGenerator gen(1, 11);
        cout << "生成器创建完成，还没有生成任何数字" << endl;
        cout << "内存占用：只保存状态（current, end, state）" << endl;

        cout << "\n使用前5个数字：" << endl;
        for (int i = 0; i < 5; ++i) {
            auto value = gen.next();
            if (value) {
                cout << "  第" << (i+1) << "次调用 next()，得到：" << *value << endl;
            }
        }
        cout << "（只生成了5个数字，节省了内存）" << endl;
    }

    // --------------------------------------------
    // 1.3 对比总结
    // --------------------------------------------

    cout << "\n【对比总结】" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
┌──────────────┬────────────────┬────────────────┐
│   特性       │   传统函数     │   生成器       │
├──────────────┼────────────────┼────────────────┤
│ 生成时机     │ 一次性生成     │ 按需生成       │
│ 内存占用     │ 高（存储所有） │ 低（只存状态） │
│ 是否可暂停   │ 否             │ 是             │
│ 无限序列     │ 不支持         │ 支持           │
│ 适用场景     │ 数据量小       │ 数据量大       │
└──────────────┴────────────────┴────────────────┘

示例：生成 100 万个数字
- 传统函数：立即占用 4MB 内存（100万 × 4字节）
- 生成器：只占用 12 字节（3个int变量）
)" << endl;
}

// ============================================
// 第三部分：生成器实践示例
// ============================================

void part2_generator_examples() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分：生成器实践示例" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 2.1 示例1：斐波那契数列生成器
    // --------------------------------------------

    cout << "\n【示例1】斐波那契数列生成器" << endl;
    cout << string(60, '-') << endl;

    {
        FibonacciGenerator fib(10);

        cout << "\n生成前10个斐波那契数：" << endl;
        while (fib.has_next()) {
            auto value = fib.next();
            if (value) {
                cout << *value << " ";
            }
        }
        cout << endl;

        cout << "\n说明：" << endl;
        cout << "- 生成器只保存 a、b 两个变量" << endl;
        cout << "- 每次调用 next() 才计算下一个值" << endl;
        cout << "- 可以生成无限序列（只需去掉 max_count 限制）" << endl;
    }

    // --------------------------------------------
    // 2.2 示例2：质数生成器
    // --------------------------------------------

    cout << "\n【示例2】质数生成器" << endl;
    cout << string(60, '-') << endl;

    {
        PrimeGenerator prime(10);

        cout << "\n生成前10个质数：" << endl;
        while (prime.has_next()) {
            auto value = prime.next();
            if (value) {
                cout << *value << " ";
            }
        }
        cout << endl;

        cout << "\n说明：" << endl;
        cout << "- 生成器只保存 current、count 两个变量" << endl;
        cout << "- 每次调用 next() 才查找下一个质数" << endl;
        cout << "- 避免一次性计算所有质数（耗时长）" << endl;
    }

    // --------------------------------------------
    // 2.3 示例3：生成器的组合
    // --------------------------------------------

    cout << "\n【示例3】生成器的组合" << endl;
    cout << string(60, '-') << endl;

    {
        cout << "生成斐波那契数列中的偶数（前5个）：" << endl;

        FibonacciGenerator fib(20);
        int count = 0;

        while (fib.has_next() && count < 5) {
            auto value = fib.next();
            if (value && *value % 2 == 0) {
                cout << *value << " ";
                ++count;
            }
        }
        cout << endl;

        cout << "\n说明：" << endl;
        cout << "- 可以组合多个生成器" << endl;
        cout << "- 可以对生成器的输出进行过滤" << endl;
        cout << "- 类似 Ranges 的管道操作" << endl;
    }
}

// ============================================
// 第四部分：生成器的优势
// ============================================

void part3_generator_advantages() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第三部分：生成器的优势" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【优势1：节省内存】

传统方式：
------------------------------------------------------------
vector<int> numbers;
for (int i = 0; i < 1000000; ++i) {
    numbers.push_back(i);
}
// 内存占用：4MB

生成器方式：
------------------------------------------------------------
CounterGenerator gen(0, 1000000);
// 内存占用：12字节

节省：4MB - 12字节 ≈ 4MB

────────────────────────────────────

【优势2：惰性求值】

传统方式：
------------------------------------------------------------
auto numbers = generate_all();  // 立即生成所有数字
for (int i = 0; i < 10; ++i) {
    cout << numbers[i];  // 只用了前10个
}
// 浪费：生成了所有数字，但只用了前10个

生成器方式：
------------------------------------------------------------
auto gen = counter_generator();
for (int i = 0; i < 10; ++i) {
    cout << gen.next();  // 只生成需要的10个
}
// 高效：只生成需要的数字

────────────────────────────────────

【优势3：支持无限序列】

传统方式：
------------------------------------------------------------
vector<int> infinite_sequence() {
    vector<int> result;
    int i = 0;
    while (true) {  // 无限循环！
        result.push_back(i++);
    }
    return result;  // 永远不会返回
}
// 问题：无法实现无限序列

生成器方式：
------------------------------------------------------------
InfiniteGenerator gen() {
    int i = 0;
    while (true) {
        co_yield i++;  // 每次返回一个值，然后暂停
    }
}

auto gen = infinite_generator();
for (int i = 0; i < 10; ++i) {
    cout << gen.next();  // 按需生成
}
// 优势：可以实现无限序列

────────────────────────────────────

【优势4：代码简洁】

传统方式（斐波那契）：
------------------------------------------------------------
class FibonacciIterator {
    int a, b, count, max_count;
public:
    FibonacciIterator(int max) : a(0), b(1), count(0), max_count(max) {}
    bool has_next() const { return count < max_count; }
    int next() {
        int result = a;
        int temp = a + b;
        a = b;
        b = temp;
        ++count;
        return result;
    }
};
// 代码：15行

生成器方式（伪代码）：
------------------------------------------------------------
Generator<int> fibonacci(int max) {
    int a = 0, b = 1;
    for (int i = 0; i < max; ++i) {
        co_yield a;
        int temp = a + b;
        a = b;
        b = temp;
    }
}
// 代码：8行，更简洁

────────────────────────────────────

【优势总结】

1. 内存效率：只保存状态，不保存所有值
2. 计算效率：按需计算，不浪费
3. 表达能力：支持无限序列
4. 代码简洁：比迭代器更简洁

适用场景：
- 大数据处理（不能一次性加载到内存）
- 无限序列（质数、斐波那契等）
- 惰性求值（只在需要时才计算）
- 流式处理（逐个处理数据）
)" << endl;
}

// ============================================
// 第五部分：真实协程 vs 模拟生成器
// ============================================

void part4_real_vs_simulated() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第四部分：真实协程 vs 模拟生成器" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【真实协程（C++20）】

优势：
- 编译器自动管理状态
- 语法简洁，使用 co_yield
- 性能优化（编译器优化）
- 标准化，跨平台

劣势：
- 需要复杂的样板代码（promise_type、awaiter）
- 编译器支持要求高（GCC 10+、Clang 10+）
- 学习曲线陡峭
- 调试困难

示例（伪代码）：
------------------------------------------------------------
Generator<int> counter(int n) {
    for (int i = 0; i < n; ++i) {
        co_yield i;  // 简洁！
    }
}

────────────────────────────────────

【模拟生成器（本文件）】

优势：
- 不需要编译器支持
- 容易理解，适合学习
- 可以自定义行为
- 调试简单

劣势：
- 需要手动管理状态
- 代码冗长（需要写类）
- 性能稍差（没有编译器优化）
- 不是标准化的

示例：
------------------------------------------------------------
class CounterGenerator {
    int current, end;
public:
    CounterGenerator(int n) : current(0), end(n) {}
    optional<int> next() {
        if (current < end) return current++;
        return nullopt;
    }
};

────────────────────────────────────

【对比总结】

┌──────────────┬────────────────┬────────────────┐
│   特性       │   真实协程     │   模拟生成器   │
├──────────────┼────────────────┼────────────────┤
│ 语法简洁度   │ 高（co_yield） │ 低（需要写类） │
│ 编译器要求   │ 高（C++20）    │ 低（C++11）    │
│ 学习难度     │ 高             │ 低             │
│ 性能         │ 高             │ 中             │
│ 适用场景     │ 生产环境       │ 学习、原型     │
└──────────────┴────────────────┴────────────────┘

【学习建议】

1. 先理解生成器的概念（本文件）
2. 再学习真实协程的语法
3. 最后实践真实协程的应用

【下一步】

- 如果你的编译器支持 C++20 协程，可以尝试真实协程
- 如果不支持，可以使用第三方库（如 cppcoro）
- 或者继续使用模拟生成器（足够应对大多数场景）
)" << endl;
}

// ============================================
// 常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonErrors() {
    cout << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
❌ 错误1：生成器不检查 hasNext() 直接调用 next()（越界访问）

   Generator gen = fibonacci();
   for (int i = 0; i < 100; i++) {
       cout << gen.next();   // ❌ 生成器可能已耗尽，next() 返回未定义值
   }

   ✅ 每次调用 next() 前检查 hasNext()：
   while (gen.hasNext()) {
       cout << gen.next();
   }
   // 或用范围 for（要求生成器实现迭代器接口）

------------------------------------------------------------

❌ 错误2：无限生成器直接用范围 for 遍历（死循环）

   for (int x : fibonacci()) {   // ❌ fibonacci 无限，永远不结束
       cout << x;
   }

   ✅ 结合 take/limit 限制数量：
   for (int x : fibonacci() | views::take(10)) {   // ✅ 只取10个
       cout << x;
   }
   // 或手动限制：
   int count = 0;
   while (gen.hasNext() && count++ < 10) cout << gen.next();

------------------------------------------------------------

❌ 错误3：生成器内部状态共享导致并发问题

   static Generator sharedGen = fibonacci();   // ❌ 全局生成器，多线程竞争内部状态

   ✅ 每个线程/调用创建独立生成器实例：
   auto gen = fibonacci();   // 局部创建，独立状态

------------------------------------------------------------

❌ 错误4：模拟生成器用 vector 缓存所有值（内存浪费）

   Generator gen;
   gen.values = {1, 1, 2, 3, 5, ...};   // ❌ 预先计算所有值，失去惰性求值优势

   ✅ 真正的生成器应按需计算，每次 next() 计算一个：
   // 只存当前状态（如 a, b），每次调用 next() 才计算下一个

------------------------------------------------------------

❌ 错误5：co_yield 协程版生成器在旧编译器/标准下不可用

   Generator<int> fibonacci() {
       int a = 0, b = 1;
       while (true) {
           co_yield a;   // ❌ C++17 及以下不支持
           tie(a, b) = {b, a + b};
       }
   }

   ✅ 本文件使用类模拟生成器，C++11 起可用；
   ✅ 真实协程需 C++20 + GCC 10+/Clang 12+，并实现 promise_type
)" << endl;
}

int main() {
    cout << "=== 生成器概念示例 ===" << endl;

    part1_generator_vs_function();
    part2_generator_examples();
    part3_generator_advantages();
    part4_real_vs_simulated();

    cout << "\n程序结束" << endl;
    return 0;
}
