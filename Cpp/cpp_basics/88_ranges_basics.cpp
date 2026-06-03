// ============================================
// 88_ranges_basics.cpp - C++20 Ranges 基础
// 功能：学习 Ranges 库的核心概念和基本用法
// 编译：g++ -std=c++20 -o 88_ranges_basics.exe 88_ranges_basics.cpp
// ============================================

#include <iostream>
#include <ranges>
#include <vector>
#include <string>
#include <algorithm>
#include <map>
using namespace std;
namespace views = std::ranges::views;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

/*
【C++20 Ranges 核心概念】

是什么？
- Ranges 是 C++20 引入的现代化容器操作库
- 对传统 STL 算法和迭代器的重大改进
- 提供更简洁、更安全、更高效的数据处理方式

为什么需要？
- 传统 STL 需要手动管理 begin/end 迭代器，容易出错
- 代码冗长，可读性差
- 无法方便地组合多个操作

核心特点：
1. Range（范围）：可以迭代的对象（容器、数组、字符串等）
2. View（视图）：轻量级的 range，不拥有数据，支持惰性求值
3. 管道操作符（|）：链式组合多个操作，类似 Unix 管道
4. Ranges 算法：直接操作 range，不需要 begin/end
*/

void part1_concepts();
void part2_basic_operations();
void part3_deep_understanding();
void part4_common_errors();

int main() {
    cout << "=== C++20 Ranges 基础示例 ===" << endl;

    part1_concepts();
    part2_basic_operations();
    part3_deep_understanding();
    part4_common_errors();

    cout << "\n程序结束" << endl;
    return 0;
}

void part1_concepts() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第一部分：核心概念详解" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 1.1 概念1：Range（范围）
    // --------------------------------------------

    cout << "\n1.1 Range（范围）是什么？" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【Range】

是什么？
- Range 是一个可以迭代的对象
- 任何有 begin() 和 end() 的对象都是 range

为什么需要？
- 统一容器、数组、字符串的操作方式
- 简化算法接口，不需要传递 begin/end

核心特点：
- vector、array、list、string 都是 range
- C 风格数组也是 range
- 自定义类型只要有 begin/end 就是 range
)" << endl;

    // 示例：各种 range
    {
        cout << "示例：各种 range" << endl;

        // vector 是 range
        vector<int> vec = {1, 2, 3, 4, 5};
        cout << "vector: ";
        for (int x : vec) cout << x << " ";
        cout << endl;

        // string 是 range
        string str = "hello";
        cout << "string: ";
        for (char c : str) cout << c << " ";
        cout << endl;

        // C 数组是 range
        int arr[] = {10, 20, 30};
        cout << "array: ";
        for (int x : arr) cout << x << " ";
        cout << endl;
    }

    // --------------------------------------------
    // 1.2 概念2：View（视图）⭐⭐⭐⭐⭐
    // --------------------------------------------

    cout << "\n1.2 View（视图）是什么？" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【View】

是什么？
- View 是一个轻量级的 range
- 不拥有数据，只是原数据的"窗口"
- 支持惰性求值（lazy evaluation）

为什么需要？
- 避免不必要的数据拷贝
- 延迟计算，只在需要时才执行
- 可以无限组合，构建复杂的数据处理管道

核心特点：
- 零拷贝：不复制数据
- 惰性求值：只在迭代时才计算
- 可组合：可以用管道操作符 | 组合
)" << endl;

    // 示例：创建视图
    {
        cout << "示例：创建视图" << endl;

        vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

        // 创建过滤视图（只看偶数）
        // 参数：原 range，过滤条件（lambda 表达式）
        // 返回：一个视图对象，不拷贝数据
        auto evens = numbers | views::filter([](int x) { return x % 2 == 0; });

        cout << "偶数视图: ";
        for (int x : evens) cout << x << " ";
        cout << endl;

        // 创建转换视图（转换为平方）
        // 参数：原 range，转换函数（lambda 表达式）
        // 返回：一个视图对象，不拷贝数据
        auto squares = numbers | views::transform([](int x) { return x * x; });

        cout << "平方视图: ";
        for (int x : squares) cout << x << " ";
        cout << endl;
    }

    // --------------------------------------------
    // 1.3 概念3：管道操作符（|）⭐⭐⭐⭐⭐
    // --------------------------------------------

    cout << "\n1.3 管道操作符（|）是什么？" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【管道操作符】

是什么？
- 管道操作符 | 用于组合多个视图
- 类似 Unix 的管道：data | filter | transform | ...

为什么需要？
- 让代码更易读，从左到右的数据流
- 避免嵌套函数调用
- 可以无限组合

核心特点：
- 从左到右执行
- 每个操作返回一个新视图
- 惰性求值，只在最后迭代时才计算
)" << endl;

    // 示例：管道组合
    {
        cout << "示例：管道组合" << endl;

        vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

        // 组合多个操作：过滤偶数 -> 转换为平方 -> 取前3个
        // 执行顺序：
        // 1. numbers | views::filter(...) 创建过滤视图
        // 2. 过滤视图 | views::transform(...) 创建转换视图
        // 3. 转换视图 | views::take(3) 创建取前3个的视图
        // 注意：此时还没有执行任何计算！
        auto result = numbers
            | views::filter([](int x) { return x % 2 == 0; })
            | views::transform([](int x) { return x * x; })
            | views::take(3);

        cout << "偶数的平方（前3个）: ";
        // 只有在迭代时才开始计算
        for (int x : result) cout << x << " ";
        cout << endl;
    }

    // --------------------------------------------
    // 1.4 重要函数：常用视图适配器 ⭐⭐⭐⭐⭐
    // --------------------------------------------

    cout << "\n1.4 常用视图适配器" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【视图适配器速查】

1. views::filter(predicate)
   作用：过滤满足条件的元素
   参数：predicate - 返回 bool 的函数
   示例：numbers | views::filter([](int x) { return x > 5; })

2. views::transform(func)
   作用：转换每个元素
   参数：func - 转换函数
   示例：numbers | views::transform([](int x) { return x * 2; })

3. views::take(n)
   作用：取前 n 个元素
   参数：n - 元素个数
   示例：numbers | views::take(5)

4. views::drop(n)
   作用：跳过前 n 个元素
   参数：n - 元素个数
   示例：numbers | views::drop(5)

5. views::reverse
   作用：反转顺序
   参数：无
   示例：numbers | views::reverse

6. views::keys / views::values
   作用：提取 map 的键/值
   参数：无
   示例：map | views::keys

7. views::iota(start, end)
   作用：生成序列 [start, end)
   参数：start - 起始值，end - 结束值（不包含）
   示例：views::iota(1, 11) 生成 1-10
)" << endl;
}

// ============================================
// 第二部分：基础操作实践
// ============================================

void part2_basic_operations() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分：基础操作实践" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 2.1 示例1：基本视图操作
    // --------------------------------------------

    cout << "\n【示例1】基本视图操作" << endl;
    cout << string(60, '-') << endl;

    {
        // 创建原始数据
        vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

        cout << "原始数据: ";
        for (int x : numbers) cout << x << " ";
        cout << endl;

        // 1. filter 视图：过滤偶数
        // 参数：lambda 表达式，返回 bool
        // 工作原理：遍历时只返回满足条件的元素
        cout << "\n1. filter - 过滤偶数:" << endl;
        auto evens = numbers | views::filter([](int x) { return x % 2 == 0; });
        cout << "   结果: ";
        for (int x : evens) cout << x << " ";
        cout << endl;

        // 2. transform 视图：转换为平方
        // 参数：lambda 表达式，返回转换后的值
        // 工作原理：遍历时对每个元素应用转换函数
        cout << "\n2. transform - 转换为平方:" << endl;
        auto squares = numbers | views::transform([](int x) { return x * x; });
        cout << "   结果: ";
        for (int x : squares) cout << x << " ";
        cout << endl;

        // 3. take 视图：取前5个
        // 参数：整数 n
        // 工作原理：只返回前 n 个元素
        cout << "\n3. take - 取前5个:" << endl;
        auto first5 = numbers | views::take(5);
        cout << "   结果: ";
        for (int x : first5) cout << x << " ";
        cout << endl;

        // 4. drop 视图：跳过前5个
        // 参数：整数 n
        // 工作原理：跳过前 n 个元素，返回剩余的
        cout << "\n4. drop - 跳过前5个:" << endl;
        auto skip5 = numbers | views::drop(5);
        cout << "   结果: ";
        for (int x : skip5) cout << x << " ";
        cout << endl;

        // 5. reverse 视图：反转
        // 参数：无
        // 工作原理：从后向前遍历
        cout << "\n5. reverse - 反转:" << endl;
        auto reversed = numbers | views::reverse;
        cout << "   结果: ";
        for (int x : reversed) cout << x << " ";
        cout << endl;
    }

    // --------------------------------------------
    // 2.2 示例2：管道组合
    // --------------------------------------------

    cout << "\n【示例2】管道组合" << endl;
    cout << string(60, '-') << endl;

    {
        vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

        cout << "原始数据: ";
        for (int x : numbers) cout << x << " ";
        cout << endl;

        // 组合1：过滤偶数 -> 转换为平方
        // 执行流程：
        // 1. filter 创建过滤视图
        // 2. transform 在过滤视图上创建转换视图
        // 3. 迭代时：先过滤，再转换
        cout << "\n组合1：过滤偶数 -> 转换为平方" << endl;
        auto result1 = numbers
            | views::filter([](int x) { return x % 2 == 0; })
            | views::transform([](int x) { return x * x; });
        cout << "结果: ";
        for (int x : result1) cout << x << " ";
        cout << endl;

        // 组合2：过滤偶数 -> 转换为平方 -> 取前3个
        // 执行流程：
        // 1. filter 创建过滤视图
        // 2. transform 在过滤视图上创建转换视图
        // 3. take 在转换视图上创建取前3个的视图
        // 4. 迭代时：先过滤，再转换，最后只取3个
        cout << "\n组合2：过滤偶数 -> 转换为平方 -> 取前3个" << endl;
        auto result2 = numbers
            | views::filter([](int x) { return x % 2 == 0; })
            | views::transform([](int x) { return x * x; })
            | views::take(3);
        cout << "结果: ";
        for (int x : result2) cout << x << " ";
        cout << endl;

        // 组合3：跳过前3个 -> 取接下来的5个 -> 反转
        // 执行流程：
        // 1. drop 跳过前3个
        // 2. take 取接下来的5个
        // 3. reverse 反转顺序
        cout << "\n组合3：跳过前3个 -> 取接下来的5个 -> 反转" << endl;
        auto result3 = numbers
            | views::drop(3)
            | views::take(5)
            | views::reverse;
        cout << "结果: ";
        for (int x : result3) cout << x << " ";
        cout << endl;
    }

    // --------------------------------------------
    // 2.3 示例3：iota 生成序列
    // --------------------------------------------

    cout << "\n【示例3】iota 生成序列" << endl;
    cout << string(60, '-') << endl;

    {
        // iota(start, end) 生成 [start, end) 的序列
        // 参数：start - 起始值（包含），end - 结束值（不包含）
        // 返回：一个生成序列的视图
        cout << "1. 生成 1-10:" << endl;
        auto seq1 = views::iota(1, 11);
        cout << "   ";
        for (int x : seq1) cout << x << " ";
        cout << endl;

        // iota(start) 生成无限序列（需要配合 take）
        // 参数：start - 起始值
        // 返回：一个无限序列的视图
        // 注意：必须配合 take 使用，否则会无限循环
        cout << "\n2. 无限序列（取前10个）:" << endl;
        auto seq2 = views::iota(1) | views::take(10);
        cout << "   ";
        for (int x : seq2) cout << x << " ";
        cout << endl;

        // 组合：生成序列 -> 过滤偶数 -> 转换为平方
        cout << "\n3. 生成1-20 -> 过滤偶数 -> 转换为平方:" << endl;
        auto seq3 = views::iota(1, 21)
            | views::filter([](int x) { return x % 2 == 0; })
            | views::transform([](int x) { return x * x; });
        cout << "   ";
        for (int x : seq3) cout << x << " ";
        cout << endl;
    }

    // --------------------------------------------
    // 2.4 示例4：Ranges 算法
    // --------------------------------------------

    cout << "\n【示例4】Ranges 算法" << endl;
    cout << string(60, '-') << endl;

    {
        vector<int> vec = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3};

        cout << "原始数据: ";
        for (int x : vec) cout << x << " ";
        cout << endl;

        // 1. ranges::sort - 排序
        // 参数：range（不需要 begin/end）
        // 作用：原地排序
        cout << "\n1. ranges::sort - 排序" << endl;
        vector<int> vec1 = vec;
        ranges::sort(vec1);
        cout << "   排序后: ";
        for (int x : vec1) cout << x << " ";
        cout << endl;

        // 2. ranges::find - 查找
        // 参数：range，要查找的值
        // 返回：迭代器（找到）或 end（未找到）
        cout << "\n2. ranges::find - 查找5" << endl;
        auto it = ranges::find(vec, 5);
        if (it != vec.end()) {
            cout << "   找到5，位置: " << distance(vec.begin(), it) << endl;
        }

        // 3. ranges::count - 计数
        // 参数：range，要计数的值
        // 返回：出现次数
        cout << "\n3. ranges::count - 计数1的个数" << endl;
        auto count = ranges::count(vec, 1);
        cout << "   1的个数: " << count << endl;

        // 4. ranges::reverse - 反转
        // 参数：range
        // 作用：原地反转
        cout << "\n4. ranges::reverse - 反转" << endl;
        vector<int> vec2 = vec;
        ranges::reverse(vec2);
        cout << "   反转后: ";
        for (int x : vec2) cout << x << " ";
        cout << endl;

        // 5. ranges::max / ranges::min - 最大/最小值
        // 参数：range
        // 返回：最大/最小值
        cout << "\n5. ranges::max / ranges::min" << endl;
        cout << "   最大值: " << ranges::max(vec) << endl;
        cout << "   最小值: " << ranges::min(vec) << endl;

        // 6. ranges::count_if - 条件计数
        // 参数：range，条件函数
        // 返回：满足条件的元素个数
        cout << "\n6. ranges::count_if - 偶数个数" << endl;
        auto even_count = ranges::count_if(vec, [](int x) { return x % 2 == 0; });
        cout << "   偶数个数: " << even_count << endl;
    }

    // --------------------------------------------
    // 2.5 示例5：keys/values 视图
    // --------------------------------------------

    cout << "\n【示例5】keys/values 视图" << endl;
    cout << string(60, '-') << endl;

    {
        // 创建 map
        map<int, string> m = {
            {1, "one"},
            {2, "two"},
            {3, "three"},
            {4, "four"},
            {5, "five"}
        };

        cout << "原始 map:" << endl;
        for (const auto& [k, v] : m) {
            cout << "  " << k << " -> " << v << endl;
        }

        // views::keys - 提取所有键
        // 参数：map 或 pair 的 range
        // 返回：只包含键的视图
        cout << "\nviews::keys - 提取所有键:" << endl;
        cout << "  ";
        for (int k : m | views::keys) {
            cout << k << " ";
        }
        cout << endl;

        // views::values - 提取所有值
        // 参数：map 或 pair 的 range
        // 返回：只包含值的视图
        cout << "\nviews::values - 提取所有值:" << endl;
        cout << "  ";
        for (const auto& v : m | views::values) {
            cout << v << " ";
        }
        cout << endl;

        // 组合：提取键 -> 过滤大于2的 -> 转换为平方
        cout << "\n组合：提取键 -> 过滤>2 -> 转换为平方:" << endl;
        cout << "  ";
        for (int x : m | views::keys
                       | views::filter([](int k) { return k > 2; })
                       | views::transform([](int k) { return k * k; })) {
            cout << x << " ";
        }
        cout << endl;
    }
}

// ============================================
// 第三部分：深入理解（可选）
// ============================================

void part3_deep_understanding() {
    cout << "\n" << string(60, '=') << endl;
    cout << "🔍 深入理解：Ranges（选学）" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【深入1：惰性求值的工作原理】

什么是惰性求值？
- 视图不会立即计算结果
- 只在迭代时才逐个计算元素
- 避免创建中间容器，节省内存

示例：
auto view = numbers
    | views::filter([](int x) { return x % 2 == 0; })
    | views::transform([](int x) { return x * x; });

// 此时还没有执行任何计算！
// 只有在迭代时才开始计算：
for (int x : view) {  // 现在才开始计算
    cout << x;
}

工作流程：
1. 迭代器请求第一个元素
2. filter 检查 numbers[0]，不满足条件，跳过
3. filter 检查 numbers[1]，满足条件，传给 transform
4. transform 计算平方，返回结果
5. 重复步骤1-4，直到遍历完成

优势：
- 不需要创建中间 vector
- 内存占用小
- 可以处理无限序列（配合 take）

────────────────────────────────────

【深入2：零拷贝的实现原理】

什么是零拷贝？
- 视图不拥有数据，只是原数据的"引用"
- 不会复制元素，只是记录如何访问原数据

示例：
vector<int> numbers = {1, 2, 3, 4, 5};
auto view = numbers | views::take(3);

// view 内部只存储：
// - numbers 的迭代器
// - 要取的元素个数（3）

// 不会创建新的 vector<int>

注意事项：
- 原数据必须在视图使用期间保持有效
- 修改原数据会影响视图
- 视图的生命周期不能超过原数据

────────────────────────────────────

【深入3：管道操作符的实现原理】

管道操作符如何工作？
- | 是一个重载的运算符
- 左边是 range，右边是视图适配器
- 返回一个新的视图对象

示例：
numbers | views::filter(pred)

等价于：
views::filter(numbers, pred)

链式调用：
numbers | views::filter(pred) | views::transform(func)

等价于：
views::transform(views::filter(numbers, pred), func)

优势：
- 从左到右的数据流更符合直觉
- 避免嵌套函数调用
- 代码更易读

────────────────────────────────────

【深入4：Ranges vs 传统 STL 的性能对比】

内存占用：
- 传统 STL：每个操作创建新容器，内存占用大
- Ranges：视图不拷贝数据，内存占用小

计算时机：
- 传统 STL：每个操作立即计算，可能浪费计算
- Ranges：惰性求值，只计算需要的元素

示例：
// 传统 STL：创建3个 vector
vector<int> temp1, temp2, temp3;
copy_if(numbers.begin(), numbers.end(), back_inserter(temp1), pred);
transform(temp1.begin(), temp1.end(), back_inserter(temp2), func);
copy_n(temp2.begin(), 5, back_inserter(temp3));

// Ranges：不创建中间容器
auto result = numbers
    | views::filter(pred)
    | views::transform(func)
    | views::take(5);

性能优势：
- 减少内存分配
- 减少数据拷贝
- 提前终止（take 只计算需要的元素）

────────────────────────────────────

【深入5：视图的组合规则】

视图可以无限组合：
auto view = numbers
    | views::filter(pred1)
    | views::transform(func1)
    | views::filter(pred2)
    | views::transform(func2)
    | views::take(10)
    | views::reverse;

执行顺序：
- 从左到右构建视图链
- 从右到左迭代（reverse 先执行）
- 每个元素依次通过所有视图

注意事项：
- 某些视图不能组合（如 reverse + reverse）
- 某些组合会影响性能（如 reverse 后 take）
- 建议：先 filter/transform，后 take/drop

)" << endl;
}

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void part4_common_errors() {
    cout << "\n" << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 4.1 错误1：视图的生命周期问题
    // --------------------------------------------

    cout << "\n4.1 错误1：视图的生命周期问题" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
auto get_view() {
    vector<int> numbers = {1, 2, 3, 4, 5};
    return numbers | views::filter([](int x) { return x % 2 == 0; });
}

auto view = get_view();
for (int x : view) {  // 悬空引用！numbers 已销毁
    cout << x;
}

结果：未定义行为，可能崩溃或输出垃圾值

原因：
- 视图不拥有数据，只是原数据的"引用"
- numbers 在函数返回后销毁
- 视图引用的数据已不存在

✅ 正确代码：
// 方法1：返回容器而不是视图
auto get_evens() {
    vector<int> numbers = {1, 2, 3, 4, 5};
    auto view = numbers | views::filter([](int x) { return x % 2 == 0; });
    return vector<int>(view.begin(), view.end());  // 转换为容器
}

// 方法2：确保原数据的生命周期
vector<int> numbers = {1, 2, 3, 4, 5};
auto view = numbers | views::filter([](int x) { return x % 2 == 0; });
for (int x : view) {  // 安全：numbers 仍然有效
    cout << x;
}

记忆技巧/预防措施：
- 视图的生命周期不能超过原数据
- 如果需要返回结果，转换为容器
- 使用 ranges::to<vector>() 转换（C++23）
)" << endl;

    // --------------------------------------------
    // 4.2 错误2：忘记 iota 的范围是 [start, end)
    // --------------------------------------------

    cout << "\n4.2 错误2：忘记 iota 的范围是 [start, end)" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
// 想生成 1-10，但写成：
auto seq = views::iota(1, 10);  // 实际生成 1-9

for (int x : seq) {
    cout << x << " ";  // 输出：1 2 3 4 5 6 7 8 9（缺少10）
}

结果：缺少最后一个元素

原因：iota(start, end) 生成 [start, end)，不包含 end

✅ 正确代码：
// 生成 1-10，应该写：
auto seq = views::iota(1, 11);  // [1, 11) = 1-10

for (int x : seq) {
    cout << x << " ";  // 输出：1 2 3 4 5 6 7 8 9 10
}

记忆技巧/预防措施：
- 记住：[start, end) 左闭右开
- 想生成 1-N，写 iota(1, N+1)
- 类似 for (int i = 1; i < 11; ++i)
)" << endl;

    // --------------------------------------------
    // 4.3 错误3：修改原数据影响视图
    // --------------------------------------------

    cout << "\n4.3 错误3：修改原数据影响视图" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
vector<int> numbers = {1, 2, 3, 4, 5};
auto view = numbers | views::filter([](int x) { return x % 2 == 0; });

// 修改原数据
numbers[1] = 100;  // 2 -> 100

// 视图也会受影响
for (int x : view) {
    cout << x << " ";  // 输出：100 4（不是预期的 2 4）
}

结果：视图输出的是修改后的数据

原因：视图不拷贝数据，只是原数据的"窗口"

✅ 正确代码：
// 方法1：如果需要独立的数据，转换为容器
vector<int> numbers = {1, 2, 3, 4, 5};
auto view = numbers | views::filter([](int x) { return x % 2 == 0; });
vector<int> evens(view.begin(), view.end());  // 拷贝到新容器

numbers[1] = 100;  // 修改原数据

for (int x : evens) {
    cout << x << " ";  // 输出：2 4（不受影响）
}

// 方法2：如果需要视图反映最新数据，这是正确行为
vector<int> numbers = {1, 2, 3, 4, 5};
auto view = numbers | views::filter([](int x) { return x % 2 == 0; });

numbers[1] = 100;  // 修改原数据

for (int x : view) {
    cout << x << " ";  // 输出：100 4（反映最新数据）
}

记忆技巧/预防措施：
- 视图是"窗口"，不是"快照"
- 如果需要独立数据，转换为容器
- 如果需要反映最新数据，使用视图
)" << endl;

    // --------------------------------------------
    // 4.4 错误4：无限序列忘记 take
    // --------------------------------------------

    cout << "\n4.4 错误4：无限序列忘记 take" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
// 创建无限序列
auto infinite = views::iota(1);

// 忘记 take，导致无限循环
for (int x : infinite) {  // 永远不会结束！
    cout << x << " ";
}

结果：程序卡死，无限循环

原因：iota(start) 生成无限序列，必须配合 take 使用

✅ 正确代码：
// 创建无限序列，配合 take
auto infinite = views::iota(1) | views::take(10);

for (int x : infinite) {
    cout << x << " ";  // 输出：1 2 3 4 5 6 7 8 9 10
}

记忆技巧/预防措施：
- iota(start) 是无限序列，必须配合 take
- iota(start, end) 是有限序列，不需要 take
- 使用无限序列时，总是加上 take
)" << endl;

    // --------------------------------------------
    // 4.5 错误5：视图不能直接赋值给容器
    // --------------------------------------------

    cout << "\n4.5 错误5：视图不能直接赋值给容器" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
vector<int> numbers = {1, 2, 3, 4, 5};
auto view = numbers | views::filter([](int x) { return x % 2 == 0; });

// 尝试直接赋值
vector<int> evens = view;  // 编译错误！

结果：编译错误，类型不匹配

原因：视图和容器是不同的类型，不能直接赋值

✅ 正确代码：
// 方法1：使用迭代器构造
vector<int> numbers = {1, 2, 3, 4, 5};
auto view = numbers | views::filter([](int x) { return x % 2 == 0; });
vector<int> evens(view.begin(), view.end());

// 方法2：使用 ranges::copy
vector<int> evens;
ranges::copy(view, back_inserter(evens));

// 方法3：使用 ranges::to（C++23）
// auto evens = view | ranges::to<vector>();

记忆技巧/预防措施：
- 视图 ≠ 容器，不能直接赋值
- 使用迭代器构造：vector<int>(view.begin(), view.end())
- 使用 ranges::copy 复制元素
)" << endl;

    // --------------------------------------------
    // 4.6 错误6：在视图上调用容器方法
    // --------------------------------------------

    cout << "\n4.6 错误6：在视图上调用容器方法" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
vector<int> numbers = {1, 2, 3, 4, 5};
auto view = numbers | views::filter([](int x) { return x % 2 == 0; });

// 尝试调用容器方法
view.push_back(6);  // 编译错误！
view.size();        // 编译错误！（某些视图支持，但不是全部）

结果：编译错误，视图没有这些方法

原因：视图不是容器，不支持容器的所有方法

✅ 正确代码：
// 如果需要容器方法，转换为容器
vector<int> numbers = {1, 2, 3, 4, 5};
auto view = numbers | views::filter([](int x) { return x % 2 == 0; });
vector<int> evens(view.begin(), view.end());

evens.push_back(6);  // 正确
cout << evens.size();  // 正确

记忆技巧/预防措施：
- 视图是轻量级的，不支持所有容器方法
- 如果需要容器方法，转换为容器
- 视图主要用于遍历和组合操作
)" << endl;

    // --------------------------------------------
    // 4.7 错误7：过度使用视图导致性能下降
    // --------------------------------------------

    cout << "\n4.7 错误7：过度使用视图导致性能下降" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

// 过度组合视图
auto view = numbers
    | views::filter([](int x) { return x > 0; })
    | views::transform([](int x) { return x; })
    | views::filter([](int x) { return x < 100; })
    | views::transform([](int x) { return x; });

// 多次遍历
for (int i = 0; i < 1000; ++i) {
    for (int x : view) {
        // 每次迭代都重新计算
    }
}

结果：性能下降，因为每次迭代都重新计算

原因：
- 视图是惰性求值，每次迭代都重新计算
- 过度组合导致计算开销增加
- 多次遍历时，应该转换为容器

✅ 正确代码：
vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

// 如果需要多次遍历，转换为容器
auto view = numbers
    | views::filter([](int x) { return x % 2 == 0; })
    | views::transform([](int x) { return x * x; });

vector<int> result(view.begin(), view.end());  // 计算一次

// 多次遍历容器
for (int i = 0; i < 1000; ++i) {
    for (int x : result) {
        // 直接访问，不重新计算
    }
}

记忆技巧/预防措施：
- 视图适合一次性遍历
- 多次遍历时，转换为容器
- 避免过度组合视图
- 性能测试：视图 vs 容器
)" << endl;
}
