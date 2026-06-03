// ============================================
// 99_cpp20_practice.cpp - C++20 综合实践项目
// 功能：综合应用 Concepts、Ranges、Coroutines、Modules 概念
// 日期：2026-06-06（第37天）
// 难度：⭐⭐⭐⭐⭐ 高级
// 编译：g++ -std=c++20 -o 99_cpp20_practice.exe 99_cpp20_practice.cpp
// 注意：本文件展示概念，部分代码为伪代码（协程部分需要库支持）
// ============================================

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <ranges>
#include <concepts>
#include <functional>
using namespace std;

// ============================================
// 第一部分：使用 Concepts 定义类型约束 ⭐⭐⭐⭐⭐
// ============================================

/*
【项目背景】
创建一个数据处理系统，支持多种数据类型的处理和分析。
使用 C++20 四大特性构建类型安全、高效、易用的系统。
*/

// --------------------------------------------
// 1.1 定义核心概念
// --------------------------------------------

// 概念1：可处理的数据类型
template<typename T>
concept Processable = requires(T t) {
    { t > T{} } -> std::convertible_to<bool>;  // 可比较
    { t + t } -> std::convertible_to<T>;       // 可相加
    { t * 2 } -> std::convertible_to<T>;       // 可乘法
};

// 概念2：可序列化的类型
template<typename T>
concept Serializable = requires(T t) {
    { t.to_string() } -> std::convertible_to<std::string>;
};

// 概念3：可统计的范围
template<typename R>
concept StatisticalRange = std::ranges::range<R> &&
    Processable<std::ranges::range_value_t<R>>;

// --------------------------------------------
// 1.2 使用概念约束函数
// --------------------------------------------

// 函数1：计算平均值（使用 Processable 约束）
template<Processable T>
double average(const vector<T>& data) {
    if (data.empty()) return 0.0;

    T sum = T{};
    for (const auto& val : data) {
        sum = sum + val;
    }

    return static_cast<double>(sum) / data.size();
}

// 函数2：查找最大值（使用 Processable 约束）
template<Processable T>
T find_max(const vector<T>& data) {
    if (data.empty()) return T{};

    T max_val = data[0];
    for (const auto& val : data) {
        if (val > max_val) {
            max_val = val;
        }
    }

    return max_val;
}

// 函数3：统计分析（使用 StatisticalRange 约束）
template<StatisticalRange R>
void analyze(R&& data) {
    using T = std::ranges::range_value_t<R>;

    if (std::ranges::empty(data)) {
        cout << "数据为空" << endl;
        return;
    }

    // 计算统计信息
    auto min_val = *std::ranges::min_element(data);
    auto max_val = *std::ranges::max_element(data);
    auto count = std::ranges::distance(data);

    T sum = T{};
    for (const auto& val : data) {
        sum = sum + val;
    }
    double avg = static_cast<double>(sum) / count;

    cout << "统计分析结果：" << endl;
    cout << "  数据量：" << count << endl;
    cout << "  最小值：" << min_val << endl;
    cout << "  最大值：" << max_val << endl;
    cout << "  平均值：" << avg << endl;
}

// ============================================
// 第二部分：使用 Ranges 处理数据 ⭐⭐⭐⭐⭐
// ============================================

// --------------------------------------------
// 2.1 数据过滤和转换
// --------------------------------------------

// 函数1：过滤正数并加倍
template<Processable T>
auto filter_and_double(const vector<T>& data) {
    return data
        | views::filter([](const T& x) { return x > T{}; })
        | views::transform([](const T& x) { return x * 2; });
}

// 函数2：分组处理（手动实现，因为 chunk 可能不可用）
template<Processable T>
vector<T> group_by_range(const vector<T>& data, int group_size) {
    vector<T> result;
    for (size_t i = 0; i < data.size(); i += group_size) {
        T sum = T{};
        for (size_t j = i; j < i + group_size && j < data.size(); ++j) {
            sum = sum + data[j];
        }
        result.push_back(sum);
    }
    return result;
}

// 函数3：数据清洗管道
template<Processable T>
auto data_cleaning_pipeline(const vector<T>& data, T threshold) {
    return data
        | views::filter([threshold](const T& x) { return x > T{} && x < threshold; })
        | views::transform([](const T& x) { return x * 2; })
        | views::take(10);
}

// ============================================
// 第三部分：协程概念展示（伪代码）⭐⭐⭐⭐⭐
// ============================================

/*
注意：以下协程代码为概念展示，实际运行需要协程库支持（如 cppcoro）

// 生成器：生成斐波那契数列
Generator<int> fibonacci_generator(int count) {
    int a = 0, b = 1;
    for (int i = 0; i < count; ++i) {
        co_yield a;
        int next = a + b;
        a = b;
        b = next;
    }
}

// 异步任务：模拟异步数据获取
Task<vector<int>> async_fetch_data(int count) {
    // 模拟异步操作
    co_await std::suspend_always{};

    vector<int> data;
    for (int i = 0; i < count; ++i) {
        data.push_back(i * 10);
    }

    co_return data;
}

// 组合使用：协程 + Ranges
Task<void> process_async_data() {
    // 异步获取数据
    auto data = co_await async_fetch_data(20);

    // 使用 Ranges 处理数据
    auto result = data
        | views::filter([](int x) { return x > 50; })
        | views::transform([](int x) { return x * 2; })
        | views::take(5);

    // 输出结果
    for (int x : result) {
        cout << x << " ";
    }
}
*/

// ============================================
// 第四部分：综合实践示例 ⭐⭐⭐⭐⭐
// ============================================

int main() {
    cout << string(60, '=') << endl;
    cout << "C++20 综合实践项目" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 示例1：使用 Concepts 约束类型
    // --------------------------------------------

    cout << "\n【示例1】使用 Concepts 约束类型" << endl;
    cout << string(60, '-') << endl;

    // 测试数据
    vector<int> int_data = {10, -5, 20, -3, 15, 8, -2, 25};
    vector<double> double_data = {3.5, -1.2, 5.8, -0.5, 4.2};

    // 使用 Concepts 约束的函数
    cout << "整数数据平均值：" << average(int_data) << endl;
    cout << "浮点数据平均值：" << average(double_data) << endl;

    cout << "整数数据最大值：" << find_max(int_data) << endl;
    cout << "浮点数据最大值：" << find_max(double_data) << endl;

    // 错误示例（编译时会报错）：
    // vector<string> str_data = {"hello", "world"};
    // average(str_data);  // 编译错误：string 不满足 Processable 概念

    cout << "\n✅ Concepts 优势：" << endl;
    cout << "  - 编译期类型检查" << endl;
    cout << "  - 清晰的错误信息" << endl;
    cout << "  - 类型约束明确" << endl;

    // --------------------------------------------
    // 示例2：使用 Ranges 处理数据
    // --------------------------------------------

    cout << "\n【示例2】使用 Ranges 处理数据" << endl;
    cout << string(60, '-') << endl;

    // 原始数据
    vector<int> raw_data = {5, -3, 8, -1, 12, 3, -7, 15, 2, -4};

    cout << "原始数据：";
    for (int x : raw_data) {
        cout << x << " ";
    }
    cout << endl;

    // 使用 Ranges 过滤和转换
    cout << "\n过滤正数并加倍：";
    auto filtered = filter_and_double(raw_data);
    for (int x : filtered) {
        cout << x << " ";
    }
    cout << endl;

    // 使用 Ranges 分组处理
    cout << "\n分组求和（每组3个）：";
    auto grouped = group_by_range(raw_data, 3);
    for (int x : grouped) {
        cout << x << " ";
    }
    cout << endl;

    // 使用 Ranges 数据清洗管道
    cout << "\n数据清洗管道（过滤、转换、取前10个）：";
    auto cleaned = data_cleaning_pipeline(raw_data, 20);
    for (int x : cleaned) {
        cout << x << " ";
    }
    cout << endl;

    cout << "\n✅ Ranges 优势：" << endl;
    cout << "  - 惰性求值，节省内存" << endl;
    cout << "  - 零拷贝，提升性能" << endl;
    cout << "  - 管道操作，代码简洁" << endl;

    // --------------------------------------------
    // 示例3：Concepts + Ranges 组合
    // --------------------------------------------

    cout << "\n【示例3】Concepts + Ranges 组合" << endl;
    cout << string(60, '-') << endl;

    // 使用 Concepts 约束的统计分析函数
    cout << "整数数据统计分析：" << endl;
    analyze(int_data);

    cout << "\n浮点数据统计分析：" << endl;
    analyze(double_data);

    // 对 Ranges 视图进行统计分析
    cout << "\n过滤后的数据统计分析：" << endl;
    auto positive_data = raw_data | views::filter([](int x) { return x > 0; });
    analyze(positive_data);

    cout << "\n✅ 组合优势：" << endl;
    cout << "  - Concepts 提供类型安全" << endl;
    cout << "  - Ranges 提供高效处理" << endl;
    cout << "  - 两者完美配合" << endl;

    // --------------------------------------------
    // 示例4：实际应用场景
    // --------------------------------------------

    cout << "\n【示例4】实际应用场景：学生成绩分析" << endl;
    cout << string(60, '-') << endl;

    // 学生成绩数据
    vector<int> scores = {85, 92, 78, 95, 88, 76, 90, 82, 94, 87,
                          91, 79, 86, 93, 81, 89, 77, 96, 84, 80};

    cout << "原始成绩：";
    for (int score : scores) {
        cout << score << " ";
    }
    cout << endl;

    // 场景1：筛选优秀成绩（>=90）
    cout << "\n优秀成绩（>=90）：";
    auto excellent = scores | views::filter([](int s) { return s >= 90; });
    for (int s : excellent) {
        cout << s << " ";
    }
    cout << " | 人数：" << ranges::distance(excellent) << endl;

    // 场景2：筛选及格成绩（>=60）并计算平均分
    auto passed = scores | views::filter([](int s) { return s >= 60; });
    int sum = 0;
    int count = 0;
    for (int s : passed) {
        sum += s;
        count++;
    }
    cout << "及格平均分：" << (count > 0 ? static_cast<double>(sum) / count : 0.0) << endl;

    // 场景3：成绩排名（前5名）
    vector<int> sorted_scores = scores;
    ranges::sort(sorted_scores, greater<int>());
    cout << "\n成绩排名（前5名）：";
    for (int s : sorted_scores | views::take(5)) {
        cout << s << " ";
    }
    cout << endl;

    // 场景4：成绩分段统计
    auto range_90_100 = scores | views::filter([](int s) { return s >= 90 && s <= 100; });
    auto range_80_89 = scores | views::filter([](int s) { return s >= 80 && s < 90; });
    auto range_70_79 = scores | views::filter([](int s) { return s >= 70 && s < 80; });
    auto range_60_69 = scores | views::filter([](int s) { return s >= 60 && s < 70; });

    cout << "\n成绩分段统计：" << endl;
    cout << "  90-100分：" << ranges::distance(range_90_100) << " 人" << endl;
    cout << "  80-89分：" << ranges::distance(range_80_89) << " 人" << endl;
    cout << "  70-79分：" << ranges::distance(range_70_79) << " 人" << endl;
    cout << "  60-69分：" << ranges::distance(range_60_69) << " 人" << endl;

    // 场景5：使用统计分析函数
    cout << "\n成绩统计分析：" << endl;
    analyze(scores);

    cout << "\n✅ 实际应用优势：" << endl;
    cout << "  - 代码简洁易读" << endl;
    cout << "  - 性能高效" << endl;
    cout << "  - 易于维护和扩展" << endl;

    // --------------------------------------------
    // 示例5：模块化设计思想展示
    // --------------------------------------------

    cout << "\n【示例5】模块化设计思想" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【模块化设计】

如果使用 Modules，可以这样组织代码：

// data_processor.cppm（模块接口单元）
export module data_processor;

import std;

// 导出概念
export template<typename T>
concept Processable = requires(T t) {
    { t > T{} } -> std::convertible_to<bool>;
    { t + t } -> std::convertible_to<T>;
};

// 导出函数
export template<Processable T>
double average(const std::vector<T>& data);

export template<Processable T>
auto filter_and_double(const std::vector<T>& data);

// 私有辅助函数（不导出）
template<typename T>
T validate(T x) { return x; }

// main.cpp
import data_processor;

int main() {
    std::vector<int> data = {1, 2, 3, 4, 5};
    double avg = average(data);  // ✅ 可以使用
    // validate(10);             // ❌ 编译错误：validate 未导出
}

【优势】
1. 编译速度快：模块只编译一次
2. 封装性强：私有函数不导出
3. 依赖清晰：import 显式声明依赖
4. 代码组织：模块化的代码结构
)" << endl;

    // --------------------------------------------
    // 示例6：协程概念展示
    // --------------------------------------------

    cout << "\n【示例6】协程概念展示（伪代码）" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【协程应用场景】

场景1：生成器
------------------------------------------------------------
// 使用协程生成斐波那契数列
Generator<int> fibonacci(int count) {
    int a = 0, b = 1;
    for (int i = 0; i < count; ++i) {
        co_yield a;
        int next = a + b;
        a = b;
        b = next;
    }
}

// 使用 Ranges 处理协程生成的数据
auto result = fibonacci(20)
    | views::filter([](int x) { return x % 2 == 0; })
    | views::take(5);

for (int x : result) {
    cout << x << " ";  // 0 2 8 34 144
}

场景2：异步数据处理
------------------------------------------------------------
// 异步获取数据
Task<vector<int>> fetch_data_async(string url) {
    auto response = co_await http_get(url);
    co_return parse_data(response);
}

// 异步处理数据
Task<void> process_data_async() {
    auto data = co_await fetch_data_async("https://api.example.com");

    // 使用 Ranges 处理数据
    auto result = data
        | views::filter([](int x) { return x > 0; })
        | views::transform([](int x) { return x * 2; });

    for (int x : result) {
        cout << x << " ";
    }
}

【优势】
1. 避免回调地狱：同步风格写异步代码
2. 惰性求值：生成器按需生成数据
3. 与 Ranges 配合：处理异步数据流
)" << endl;

    // --------------------------------------------
    // 总结
    // --------------------------------------------

    cout << "\n" << string(60, '=') << endl;
    cout << "综合实践总结" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【C++20 四大特性综合应用】

1. Concepts（类型安全）
   ✅ 编译期类型检查
   ✅ 清晰的错误信息
   ✅ 类型约束明确

2. Ranges（数据处理）
   ✅ 惰性求值，节省内存
   ✅ 零拷贝，提升性能
   ✅ 管道操作，代码简洁

3. Coroutines（异步编程）
   ✅ 避免回调地狱
   ✅ 生成器模式
   ✅ 异步数据流处理

4. Modules（代码组织）
   ✅ 编译速度快
   ✅ 封装性强
   ✅ 依赖清晰

【学习成果】

通过本次综合实践，你已经：
✅ 理解了四大特性的核心价值
✅ 掌握了特性之间的组合使用
✅ 学会了在实际场景中应用特性
✅ 建立了现代 C++ 编程思维

【下一步】

1. 在实际项目中应用这些特性
2. 深入学习标准库实现
3. 关注 C++23 和未来标准
4. 持续实践和积累经验

恭喜你完成 C++20 综合实践！🎉
)" << endl;

    cout << "\n程序结束" << endl;
    return 0;
}

