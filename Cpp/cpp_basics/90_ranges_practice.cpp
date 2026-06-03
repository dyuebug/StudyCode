// ============================================
// 90_ranges_practice.cpp - Ranges 综合实践
// 功能：使用 Ranges 重写传统 STL 算法，体验 Ranges 的简洁性
// 编译：g++ -std=c++20 -o 90_ranges_practice.exe 90_ranges_practice.cpp
// ============================================

#include <iostream>
#include <ranges>
#include <vector>
#include <string>
#include <algorithm>
#include <numeric>
using namespace std;
namespace views = std::ranges::views;

// ============================================
// 常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonErrors() {
    cout << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
❌ 错误1：将视图结果当作容器（视图是惰性的，不存储数据）

   auto v = views::filter(vec, [](int x){ return x > 0; });
   v[0];   // ❌ 视图不支持随机访问（filter 产生的是前向迭代器）
   v.size(); // ❌ 视图没有 size()（惰性求值，不知道结果数量）

   ✅ 如需随机访问或 size，收集到容器：
   vector<int> result(v.begin(), v.end());
   // C++23：ranges::to<vector>(v)

------------------------------------------------------------

❌ 错误2：视图的生命周期短于底层容器（悬空引用）

   auto getView() {
       vector<int> v = {1, 2, 3, 4, 5};
       return v | views::filter([](int x){ return x > 2; });
   }   // v 析构！视图悬空

   auto view = getView();
   for (int x : view) cout << x;   // ❌ 未定义行为

   ✅ 视图不拥有数据，确保底层容器生命周期足够长：
   vector<int> v = {1, 2, 3, 4, 5};
   auto view = v | views::filter([](int x){ return x > 2; });
   // v 和 view 在同一作用域

------------------------------------------------------------

❌ 错误3：对同一视图多次迭代（某些视图不可重复迭代）

   auto v = istream_iterator<int>(cin) | views::take(5);
   for (auto x : v) cout << x;   // 第一次：OK
   for (auto x : v) cout << x;   // ❌ 输入流已消耗，第二次迭代无效

   ✅ 只有 forward_range 及以上的视图才能多次迭代
   ✅ 对 input_range（如 istream）只能迭代一次

------------------------------------------------------------

❌ 错误4：管道链中 lambda 捕获了局部变量，视图逃逸作用域

   auto makeFilter(int threshold) {
       return views::filter([threshold](int x){ return x > threshold; });
   }
   // threshold 是局部变量，lambda 值捕获 → 安全
   // 若是引用捕获 [&threshold]，threshold 离开作用域后视图使用 → 悬空

   ✅ 视图中的 lambda 用值捕获（[=] 或 [threshold]），不用引用捕获

------------------------------------------------------------

❌ 错误5：ranges::sort 对不支持随机访问的视图排序

   auto v = vec | views::filter([](int x){ return x > 0; });
   ranges::sort(v);   // ❌ filter 视图不是 random_access_range，sort 要求随机访问

   ✅ 先收集再排序：
   vector<int> tmp(v.begin(), v.end());
   ranges::sort(tmp);
)" << endl;
}

int main() {
    cout << "=== Ranges 综合实践 ===" << endl;

    // --------------------------------------------
    // 示例1：数据处理管道
    // --------------------------------------------

    cout << "\n【示例1】数据处理管道" << endl;
    cout << string(60, '-') << endl;

    {
        vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

        cout << "原始数据：";
        for (int x : numbers) cout << x << " ";
        cout << endl;

        // 传统 STL 方式
        cout << "\n传统 STL：" << endl;
        vector<int> temp1, temp2;
        copy_if(numbers.begin(), numbers.end(), back_inserter(temp1),
                [](int x) { return x % 2 == 0; });
        transform(temp1.begin(), temp1.end(), back_inserter(temp2),
                  [](int x) { return x * x; });
        sort(temp2.begin(), temp2.end());

        cout << "结果：";
        for (int x : temp2) cout << x << " ";
        cout << endl;

        // Ranges 方式
        cout << "\nRanges：" << endl;
        auto result = numbers
            | views::filter([](int x) { return x % 2 == 0; })
            | views::transform([](int x) { return x * x; });

        vector<int> vec(result.begin(), result.end());
        ranges::sort(vec);

        cout << "结果：";
        for (int x : vec) cout << x << " ";
        cout << endl;

        cout << "\n对比：" << endl;
        cout << "  传统 STL：需要2个中间容器，代码冗长" << endl;
        cout << "  Ranges：不需要中间容器，代码简洁" << endl;
    }

    // --------------------------------------------
    // 示例2：字符串处理
    // --------------------------------------------

    cout << "\n【示例2】字符串处理" << endl;
    cout << string(60, '-') << endl;

    {
        string text = "Hello World! This is C++20 Ranges.";

        cout << "原始文本：" << text << endl;

        // 统计大写字母
        auto uppercase_count = ranges::count_if(text, [](char c) {
            return isupper(c);
        });
        cout << "\n大写字母数量：" << uppercase_count << endl;

        // 提取所有大写字母
        cout << "大写字母：";
        for (char c : text | views::filter([](char c) { return isupper(c); })) {
            cout << c << " ";
        }
        cout << endl;

        // 转换为小写
        cout << "转小写：";
        for (char c : text | views::transform([](char c) { return (char)tolower(c); })) {
            cout << c;
        }
        cout << endl;

        // 提取字母并转大写
        cout << "只保留字母（大写）：";
        for (char c : text
                     | views::filter([](char c) { return isalpha(c); })
                     | views::transform([](char c) { return (char)toupper(c); })) {
            cout << c;
        }
        cout << endl;
    }

    // --------------------------------------------
    // 示例3：生成斐波那契数列
    // --------------------------------------------

    cout << "\n【示例3】生成斐波那契数列" << endl;
    cout << string(60, '-') << endl;

    {
        // 使用 iota 生成索引，然后计算斐波那契数
        auto fib = [](int n) {
            if (n <= 1) return n;
            int a = 0, b = 1;
            for (int i = 2; i <= n; ++i) {
                int temp = a + b;
                a = b;
                b = temp;
            }
            return b;
        };

        cout << "前10个斐波那契数：";
        for (int x : views::iota(0, 10) | views::transform(fib)) {
            cout << x << " ";
        }
        cout << endl;

        // 只取偶数的斐波那契数
        cout << "前10个偶数斐波那契数：";
        for (int x : views::iota(0)
                    | views::transform(fib)
                    | views::filter([](int x) { return x % 2 == 0; })
                    | views::take(10)) {
            cout << x << " ";
        }
        cout << endl;
    }

    // --------------------------------------------
    // 示例4：质数生成器
    // --------------------------------------------

    cout << "\n【示例4】质数生成器" << endl;
    cout << string(60, '-') << endl;

    {
        auto is_prime = [](int n) {
            if (n < 2) return false;
            for (int i = 2; i * i <= n; ++i) {
                if (n % i == 0) return false;
            }
            return true;
        };

        cout << "前15个质数：";
        for (int x : views::iota(2) | views::filter(is_prime) | views::take(15)) {
            cout << x << " ";
        }
        cout << endl;

        // 100以内的质数
        cout << "\n100以内的质数：";
        for (int x : views::iota(2, 101) | views::filter(is_prime)) {
            cout << x << " ";
        }
        cout << endl;
    }

    // --------------------------------------------
    // 示例5：数据统计分析
    // --------------------------------------------

    cout << "\n【示例5】数据统计分析" << endl;
    cout << string(60, '-') << endl;

    {
        vector<int> scores = {85, 92, 78, 95, 88, 76, 90, 82, 94, 89, 72, 96};

        cout << "原始成绩：";
        for (int x : scores) cout << x << " ";
        cout << endl;

        // 及格人数
        auto passing = ranges::count_if(scores, [](int x) { return x >= 60; });
        cout << "\n及格人数：" << passing << endl;

        // 优秀人数（>=90）
        auto excellent = ranges::count_if(scores, [](int x) { return x >= 90; });
        cout << "优秀人数：" << excellent << endl;

        // 平均分
        auto sum = accumulate(scores.begin(), scores.end(), 0);
        cout << "平均分：" << sum / scores.size() << endl;

        // 最高分和最低分
        cout << "最高分：" << ranges::max(scores) << endl;
        cout << "最低分：" << ranges::min(scores) << endl;

        // 及格成绩列表
        cout << "\n及格成绩：";
        for (int x : scores | views::filter([](int x) { return x >= 60; })) {
            cout << x << " ";
        }
        cout << endl;

        // 成绩等级分布
        auto get_grade = [](int score) {
            if (score >= 90) return 'A';
            else if (score >= 80) return 'B';
            else if (score >= 70) return 'C';
            else if (score >= 60) return 'D';
            else return 'F';
        };

        cout << "\n成绩等级：";
        for (char grade : scores | views::transform(get_grade)) {
            cout << grade << " ";
        }
        cout << endl;
    }

    // --------------------------------------------
    // 示例6：嵌套容器处理
    // --------------------------------------------

    cout << "\n【示例6】嵌套容器处理" << endl;
    cout << string(60, '-') << endl;

    {
        vector<vector<int>> matrix = {
            {1, 2, 3},
            {4, 5, 6},
            {7, 8, 9}
        };

        cout << "原始矩阵：" << endl;
        for (const auto& row : matrix) {
            for (int x : row) {
                cout << x << " ";
            }
            cout << endl;
        }

        // 展平矩阵
        cout << "\n展平后：";
        for (int x : matrix | views::join) {
            cout << x << " ";
        }
        cout << endl;

        // 展平 + 过滤偶数 + 转换为平方
        cout << "展平 + 过滤偶数 + 平方：";
        for (int x : matrix
                    | views::join
                    | views::filter([](int x) { return x % 2 == 0; })
                    | views::transform([](int x) { return x * x; })) {
            cout << x << " ";
        }
        cout << endl;
    }

    // --------------------------------------------
    // 示例7：笛卡尔积
    // --------------------------------------------

    cout << "\n【示例7】笛卡尔积" << endl;
    cout << string(60, '-') << endl;

    {
        vector<int> a = {1, 2, 3};
        vector<int> b = {10, 20};

        cout << "集合A：";
        for (int x : a) cout << x << " ";
        cout << endl;

        cout << "集合B：";
        for (int x : b) cout << x << " ";
        cout << endl;

        cout << "\n笛卡尔积 A × B：" << endl;
        for (int x : a) {
            for (int y : b) {
                cout << "(" << x << ", " << y << ") ";
            }
        }
        cout << endl;

        // 使用 Ranges 实现
        cout << "\n使用 Ranges（展平）：";
        auto cartesian = a
            | views::transform([&b](int x) {
                return b | views::transform([x](int y) {
                    return x * 100 + y;  // 编码为一个数字
                });
            })
            | views::join;

        for (int encoded : cartesian) {
            int x = encoded / 100;
            int y = encoded % 100;
            cout << "(" << x << ", " << y << ") ";
        }
        cout << endl;
    }

    // --------------------------------------------
    // 示例8：滑动窗口
    // --------------------------------------------

    cout << "\n【示例8】滑动窗口" << endl;
    cout << string(60, '-') << endl;

    {
        vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

        cout << "原始数据：";
        for (int x : numbers) cout << x << " ";
        cout << endl;

        // 滑动窗口大小为3
        int window_size = 3;
        cout << "\n滑动窗口（大小=" << window_size << "）：" << endl;

        for (int i = 0; i <= (int)numbers.size() - window_size; ++i) {
            cout << "窗口" << i << "：";
            for (int x : numbers | views::drop(i) | views::take(window_size)) {
                cout << x << " ";
            }
            cout << endl;
        }

        // 计算每个窗口的和
        cout << "\n每个窗口的和：" << endl;
        for (int i = 0; i <= (int)numbers.size() - window_size; ++i) {
            auto window = numbers | views::drop(i) | views::take(window_size);
            int sum = accumulate(window.begin(), window.end(), 0);
            cout << "窗口" << i << "：" << sum << endl;
        }
    }

    // --------------------------------------------
    // 示例9：对比 Ranges vs 传统 STL
    // --------------------------------------------

    cout << "\n【示例9】对比 Ranges vs 传统 STL" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
任务：处理1-100的数字
1. 过滤出偶数
2. 转换为平方
3. 过滤出大于100的
4. 取前5个
5. 排序

传统 STL：
------------------------------------------------------------
vector<int> numbers(100);
iota(numbers.begin(), numbers.end(), 1);

// 步骤1：过滤偶数
vector<int> evens;
copy_if(numbers.begin(), numbers.end(), back_inserter(evens),
        [](int x) { return x % 2 == 0; });

// 步骤2：转换为平方
vector<int> squares;
transform(evens.begin(), evens.end(), back_inserter(squares),
          [](int x) { return x * x; });

// 步骤3：过滤大于100的
vector<int> filtered;
copy_if(squares.begin(), squares.end(), back_inserter(filtered),
        [](int x) { return x > 100; });

// 步骤4：取前5个
vector<int> result;
copy_n(filtered.begin(), 5, back_inserter(result));

// 步骤5：排序
sort(result.begin(), result.end());

统计：
- 代码行数：20+
- 中间容器：4个
- 内存分配：5次
- 可读性：差（需要追踪多个变量）

Ranges：
------------------------------------------------------------
auto result = views::iota(1, 101)
    | views::filter([](int x) { return x % 2 == 0; })
    | views::transform([](int x) { return x * x; })
    | views::filter([](int x) { return x > 100; })
    | views::take(5);

vector<int> vec(result.begin(), result.end());
ranges::sort(vec);

统计：
- 代码行数：8
- 中间容器：0个（视图）
- 内存分配：1次（最终转换）
- 可读性：优（清晰的数据流）

优势总结：
------------------------------------------------------------
1. 代码更简洁：8行 vs 20+行
2. 内存更高效：0个中间容器 vs 4个
3. 性能更好：惰性求值，提前终止
4. 可读性更强：从左到右的数据流
5. 更易维护：修改管道中的任何步骤都很容易
)" << endl;

    // 实际运行对比
    {
        cout << "实际运行结果：" << endl;

        // Ranges 方式
        auto result = views::iota(1, 101)
            | views::filter([](int x) { return x % 2 == 0; })
            | views::transform([](int x) { return x * x; })
            | views::filter([](int x) { return x > 100; })
            | views::take(5);

        vector<int> vec;
        ranges::copy(result, back_inserter(vec));
        ranges::sort(vec);

        cout << "结果：";
        for (int x : vec) cout << x << " ";
        cout << endl;
    }

    // --------------------------------------------
    // 示例10：实际应用场景
    // --------------------------------------------

    cout << "\n【示例10】实际应用场景" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
Ranges 的实际应用场景：

1. 数据处理管道
   - 日志分析：过滤 -> 解析 -> 统计
   - 数据清洗：验证 -> 转换 -> 去重
   - ETL 流程：提取 -> 转换 -> 加载

2. 算法竞赛
   - 快速原型：简洁的代码，快速实现
   - 性能优化：惰性求值，减少内存占用

3. 文本处理
   - 分词 -> 过滤 -> 统计
   - 正则匹配 -> 提取 -> 转换

4. 数学计算
   - 生成序列 -> 过滤 -> 转换
   - 矩阵运算 -> 展平 -> 聚合

5. 游戏开发
   - 实体过滤 -> 排序 -> 渲染
   - 碰撞检测 -> 响应 -> 更新

最佳实践：
------------------------------------------------------------
1. 一次性遍历：使用视图
2. 多次遍历：转换为容器
3. 复杂逻辑：拆分为多个管道
4. 性能关键：测试对比 Ranges vs 传统 STL
5. 可读性优先：清晰的数据流比性能更重要
)" << endl;

    cout << "\n程序结束" << endl;
    return 0;
}
