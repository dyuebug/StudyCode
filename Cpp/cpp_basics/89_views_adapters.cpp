// ============================================
// 89_views_adapters.cpp - 视图和适配器详解
// 功能：深入学习视图的特性和常用适配器
// 编译：g++ -std=c++20 -o 89_views_adapters.exe 89_views_adapters.cpp
// ============================================

#include <iostream>
#include <ranges>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
using namespace std;
namespace views = std::ranges::views;

void part1_lazy_evaluation();
void part2_zero_copy();
void part3_view_composition();
void part4_advanced_adapters();
void part5_practical_examples();

int main() {
    cout << "=== 视图和适配器详解 ===" << endl;

    part1_lazy_evaluation();
    part2_zero_copy();
    part3_view_composition();
    part4_advanced_adapters();
    part5_practical_examples();

    cout << "\n程序结束" << endl;
    return 0;
}

// ============================================
// 第一部分：惰性求值演示
// ============================================

void part1_lazy_evaluation() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第一部分：惰性求值演示" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 1.1 示例1：惰性求值的工作原理
    // --------------------------------------------

    cout << "\n【示例1】惰性求值的工作原理" << endl;
    cout << string(60, '-') << endl;

    {
        vector<int> numbers = {1, 2, 3, 4, 5};

        cout << "创建视图（不执行计算）" << endl;
        // 创建视图：filter + transform
        // 注意：此时不会执行任何计算！
        auto view = numbers
            | views::filter([](int x) {
                cout << "  filter(" << x << ")" << endl;
                return x % 2 == 0;
            })
            | views::transform([](int x) {
                cout << "  transform(" << x << ")" << endl;
                return x * x;
            });

        cout << "\n开始迭代（此时才计算）：" << endl;
        // 只有在迭代时才开始计算
        // 每个元素依次通过 filter 和 transform
        for (int x : view) {
            cout << "结果：" << x << endl;
        }
    }

    // --------------------------------------------
    // 1.2 示例2：惰性求值 vs 立即求值
    // --------------------------------------------

    cout << "\n【示例2】惰性求值 vs 立即求值" << endl;
    cout << string(60, '-') << endl;

    {
        vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

        // 传统 STL：立即求值
        cout << "传统 STL（立即求值）：" << endl;
        vector<int> temp;
        cout << "  执行 copy_if..." << endl;
        copy_if(numbers.begin(), numbers.end(), back_inserter(temp),
                [](int x) { return x % 2 == 0; });
        cout << "  创建了中间容器，大小：" << temp.size() << endl;

        vector<int> result;
        cout << "  执行 transform..." << endl;
        transform(temp.begin(), temp.end(), back_inserter(result),
                  [](int x) { return x * x; });
        cout << "  创建了结果容器，大小：" << result.size() << endl;

        // Ranges：惰性求值
        cout << "\nRanges（惰性求值）：" << endl;
        cout << "  创建视图..." << endl;
        auto view = numbers
            | views::filter([](int x) { return x % 2 == 0; })
            | views::transform([](int x) { return x * x; });
        cout << "  没有创建任何中间容器！" << endl;

        cout << "  开始迭代..." << endl;
        for (int x : view) {
            cout << "    " << x;
        }
        cout << endl;
    }

    // --------------------------------------------
    // 1.3 示例3：提前终止的优势
    // --------------------------------------------

    cout << "\n【示例3】提前终止的优势" << endl;
    cout << string(60, '-') << endl;

    {
        vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

        // 传统 STL：处理所有元素
        cout << "传统 STL：" << endl;
        vector<int> temp, result;
        copy_if(numbers.begin(), numbers.end(), back_inserter(temp),
                [](int x) { return x % 2 == 0; });
        transform(temp.begin(), temp.end(), back_inserter(result),
                  [](int x) { return x * x; });
        // 即使只需要前3个，也处理了所有元素
        cout << "  处理了 " << result.size() << " 个元素" << endl;
        cout << "  结果（前3个）：";
        for (int i = 0; i < 3; ++i) cout << result[i] << " ";
        cout << endl;

        // Ranges：只处理需要的元素
        cout << "\nRanges：" << endl;
        int count = 0;
        auto view = numbers
            | views::filter([&count](int x) {
                ++count;
                return x % 2 == 0;
            })
            | views::transform([](int x) { return x * x; })
            | views::take(3);

        cout << "  结果（前3个）：";
        for (int x : view) cout << x << " ";
        cout << endl;
        cout << "  只处理了 " << count << " 个元素（提前终止）" << endl;
    }
}

// ============================================
// 第二部分：零拷贝演示
// ============================================

void part2_zero_copy() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分：零拷贝演示" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 2.1 示例1：零拷贝的工作原理
    // --------------------------------------------

    cout << "\n【示例1】零拷贝的工作原理" << endl;
    cout << string(60, '-') << endl;

    {
        vector<int> numbers = {1, 2, 3, 4, 5};
        auto view = numbers | views::take(3);

        cout << "原始视图：";
        for (int x : view) cout << x << " ";
        cout << endl;

        // 修改原数据
        cout << "\n修改原数据：numbers[0] = 100" << endl;
        numbers[0] = 100;

        cout << "修改后视图：";
        for (int x : view) cout << x << " ";
        cout << endl;

        cout << "\n说明：视图不拥有数据，只是原数据的\"窗口\"" << endl;
    }

    // --------------------------------------------
    // 2.2 示例2：视图 vs 容器的内存占用
    // --------------------------------------------

    cout << "\n【示例2】视图 vs 容器的内存占用" << endl;
    cout << string(60, '-') << endl;

    {
        vector<int> numbers(1000000);  // 100万个元素
        for (int i = 0; i < 1000000; ++i) {
            numbers[i] = i;
        }

        cout << "原始容器大小：" << numbers.size() << " 个元素" << endl;
        cout << "内存占用：约 " << numbers.size() * sizeof(int) / 1024 / 1024 << " MB" << endl;

        // 创建视图
        auto view = numbers
            | views::filter([](int x) { return x % 2 == 0; })
            | views::transform([](int x) { return x * x; });

        cout << "\n创建视图后：" << endl;
        cout << "视图不拷贝数据，内存占用几乎为0" << endl;
        cout << "（视图只存储迭代器和lambda表达式）" << endl;

        // 如果转换为容器
        cout << "\n如果转换为容器：" << endl;
        vector<int> result(view.begin(), view.end());
        cout << "结果容器大小：" << result.size() << " 个元素" << endl;
        cout << "内存占用：约 " << result.size() * sizeof(int) / 1024 / 1024 << " MB" << endl;
    }

    // --------------------------------------------
    // 2.3 示例3：视图的生命周期陷阱
    // --------------------------------------------

    cout << "\n【示例3】视图的生命周期陷阱" << endl;
    cout << string(60, '-') << endl;

    {
        cout << "正确用法：" << endl;
        vector<int> numbers = {1, 2, 3, 4, 5};
        auto view = numbers | views::filter([](int x) { return x % 2 == 0; });

        cout << "  视图：";
        for (int x : view) cout << x << " ";
        cout << endl;
        cout << "  （numbers 仍然有效）" << endl;

        cout << "\n错误用法（已在常见错误部分说明）：" << endl;
        cout << "  不要返回局部变量的视图" << endl;
        cout << "  不要在原数据销毁后使用视图" << endl;
    }
}

// ============================================
// 第三部分：视图组合
// ============================================

void part3_view_composition() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第三部分：视图组合" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 3.1 示例1：复杂组合
    // --------------------------------------------

    cout << "\n【示例1】复杂组合" << endl;
    cout << string(60, '-') << endl;

    {
        vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

        // 复杂组合：过滤 -> 转换 -> 取部分 -> 反转
        auto result = numbers
            | views::filter([](int x) { return x > 2; })
            | views::transform([](int x) { return x * 2; })
            | views::take(5)
            | views::reverse;

        cout << "原始数据：";
        for (int x : numbers) cout << x << " ";
        cout << endl;

        cout << "\n执行流程：" << endl;
        cout << "1. filter(x > 2)：3 4 5 6 7 8 9 10" << endl;
        cout << "2. transform(x * 2)：6 8 10 12 14 16 18 20" << endl;
        cout << "3. take(5)：6 8 10 12 14" << endl;
        cout << "4. reverse：14 12 10 8 6" << endl;

        cout << "\n最终结果：";
        for (int x : result) cout << x << " ";
        cout << endl;
    }

    // --------------------------------------------
    // 3.2 示例2：多层嵌套
    // --------------------------------------------

    cout << "\n【示例2】多层嵌套" << endl;
    cout << string(60, '-') << endl;

    {
        vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

        // 多层过滤和转换
        auto result = numbers
            | views::filter([](int x) { return x % 2 == 0; })  // 偶数
            | views::transform([](int x) { return x * x; })     // 平方
            | views::filter([](int x) { return x > 20; })       // 大于20
            | views::transform([](int x) { return x / 2; });    // 除以2

        cout << "结果：";
        for (int x : result) cout << x << " ";
        cout << endl;
    }

    // --------------------------------------------
    // 3.3 示例3：组合的顺序很重要
    // --------------------------------------------

    cout << "\n【示例3】组合的顺序很重要" << endl;
    cout << string(60, '-') << endl;

    {
        vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

        // 顺序1：先 take 再 filter
        cout << "顺序1：先 take(5) 再 filter(偶数)" << endl;
        auto result1 = numbers
            | views::take(5)
            | views::filter([](int x) { return x % 2 == 0; });
        cout << "  结果：";
        for (int x : result1) cout << x << " ";
        cout << endl;

        // 顺序2：先 filter 再 take
        cout << "\n顺序2：先 filter(偶数) 再 take(5)" << endl;
        auto result2 = numbers
            | views::filter([](int x) { return x % 2 == 0; })
            | views::take(5);
        cout << "  结果：";
        for (int x : result2) cout << x << " ";
        cout << endl;

        cout << "\n说明：顺序不同，结果不同！" << endl;
    }
}

// ============================================
// 第四部分：高级适配器
// ============================================

void part4_advanced_adapters() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第四部分：高级适配器" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 4.1 示例1：split 分割字符串
    // --------------------------------------------

    cout << "\n【示例1】split 分割字符串" << endl;
    cout << string(60, '-') << endl;

    {
        string text = "hello,world,C++,Ranges";

        cout << "原始字符串：" << text << endl;
        cout << "\n分割结果：" << endl;

        // split 返回的是子范围的视图
        for (auto part : text | views::split(',')) {
            cout << "  \"";
            for (char c : part) {
                cout << c;
            }
            cout << "\"" << endl;
        }
    }

    // --------------------------------------------
    // 4.2 示例2：join 连接范围
    // --------------------------------------------

    cout << "\n【示例2】join 连接范围" << endl;
    cout << string(60, '-') << endl;

    {
        vector<vector<int>> nested = {{1, 2}, {3, 4}, {5, 6}};

        cout << "原始嵌套容器：" << endl;
        for (const auto& vec : nested) {
            cout << "  [";
            for (int x : vec) cout << x << " ";
            cout << "]" << endl;
        }

        cout << "\njoin 后：";
        for (int x : nested | views::join) {
            cout << x << " ";
        }
        cout << endl;
    }

    // --------------------------------------------
    // 4.3 示例3：keys/values 提取键值
    // --------------------------------------------

    cout << "\n【示例3】keys/values 提取键值" << endl;
    cout << string(60, '-') << endl;

    {
        map<int, string> m = {
            {1, "one"},
            {2, "two"},
            {3, "three"},
            {4, "four"},
            {5, "five"}
        };

        cout << "原始 map：" << endl;
        for (const auto& [k, v] : m) {
            cout << "  " << k << " -> " << v << endl;
        }

        // 提取键
        cout << "\nkeys：";
        for (int k : m | views::keys) {
            cout << k << " ";
        }
        cout << endl;

        // 提取值
        cout << "values：";
        for (const auto& v : m | views::values) {
            cout << v << " ";
        }
        cout << endl;

        // 组合：提取键 -> 过滤 -> 转换
        cout << "\n组合（键>2，转换为平方）：";
        for (int x : m | views::keys
                       | views::filter([](int k) { return k > 2; })
                       | views::transform([](int k) { return k * k; })) {
            cout << x << " ";
        }
        cout << endl;
    }

    // --------------------------------------------
    // 4.4 示例4：drop_while / take_while
    // --------------------------------------------

    cout << "\n【示例4】drop_while / take_while" << endl;
    cout << string(60, '-') << endl;

    {
        vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

        cout << "原始数据：";
        for (int x : numbers) cout << x << " ";
        cout << endl;

        // drop_while：跳过满足条件的元素，直到遇到不满足的
        cout << "\ndrop_while(x < 5)：";
        for (int x : numbers | views::drop_while([](int x) { return x < 5; })) {
            cout << x << " ";
        }
        cout << endl;

        // take_while：取满足条件的元素，直到遇到不满足的
        cout << "take_while(x < 5)：";
        for (int x : numbers | views::take_while([](int x) { return x < 5; })) {
            cout << x << " ";
        }
        cout << endl;
    }
}

// ============================================
// 第五部分：实际应用示例
// ============================================

void part5_practical_examples() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第五部分：实际应用示例" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 5.1 示例1：数据处理管道
    // --------------------------------------------

    cout << "\n【示例1】数据处理管道" << endl;
    cout << string(60, '-') << endl;

    {
        vector<int> scores = {85, 92, 78, 95, 88, 76, 90, 82, 94, 89};

        cout << "原始成绩：";
        for (int x : scores) cout << x << " ";
        cout << endl;

        // 处理管道：过滤及格 -> 转换为等级 -> 统计
        auto passing = scores
            | views::filter([](int x) { return x >= 80; })
            | views::transform([](int x) {
                if (x >= 90) return 'A';
                else if (x >= 85) return 'B';
                else return 'C';
            });

        cout << "\n及格成绩的等级：";
        for (char grade : passing) {
            cout << grade << " ";
        }
        cout << endl;

        // 统计各等级数量
        vector<char> grades(passing.begin(), passing.end());
        cout << "\n统计：" << endl;
        cout << "  A: " << ranges::count(grades, 'A') << endl;
        cout << "  B: " << ranges::count(grades, 'B') << endl;
        cout << "  C: " << ranges::count(grades, 'C') << endl;
    }

    // --------------------------------------------
    // 5.2 示例2：字符串处理
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

        // 提取单词（简化版）
        cout << "\n单词列表：" << endl;
        for (auto word : text | views::split(' ')) {
            cout << "  \"";
            for (char c : word) {
                if (isalpha(c)) cout << c;
            }
            cout << "\"" << endl;
        }
    }

    // --------------------------------------------
    // 5.3 示例3：生成数学序列
    // --------------------------------------------

    cout << "\n【示例3】生成数学序列" << endl;
    cout << string(60, '-') << endl;

    {
        // 生成平方数序列
        cout << "前10个平方数：";
        for (int x : views::iota(1, 11) | views::transform([](int x) { return x * x; })) {
            cout << x << " ";
        }
        cout << endl;

        // 生成斐波那契数列（前10个）
        cout << "\n前10个斐波那契数：";
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
        for (int x : views::iota(0, 10) | views::transform(fib)) {
            cout << x << " ";
        }
        cout << endl;

        // 生成质数序列（前10个）
        cout << "\n前10个质数：";
        auto is_prime = [](int n) {
            if (n < 2) return false;
            for (int i = 2; i * i <= n; ++i) {
                if (n % i == 0) return false;
            }
            return true;
        };
        for (int x : views::iota(2) | views::filter(is_prime) | views::take(10)) {
            cout << x << " ";
        }
        cout << endl;
    }

    // --------------------------------------------
    // 5.4 示例4：Ranges vs 传统 STL 对比
    // --------------------------------------------

    cout << "\n【示例4】Ranges vs 传统 STL 对比" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
任务：从1-100中，找出所有偶数，转换为平方，取前5个

传统 STL：
------------------------------------------------------------
vector<int> numbers(100);
iota(numbers.begin(), numbers.end(), 1);

vector<int> evens;
copy_if(numbers.begin(), numbers.end(), back_inserter(evens),
        [](int x) { return x % 2 == 0; });

vector<int> squares;
transform(evens.begin(), evens.end(), back_inserter(squares),
          [](int x) { return x * x; });

vector<int> result;
copy_n(squares.begin(), 5, back_inserter(result));

Ranges：
------------------------------------------------------------
auto result = views::iota(1, 101)
    | views::filter([](int x) { return x % 2 == 0; })
    | views::transform([](int x) { return x * x; })
    | views::take(5);

优势总结：
------------------------------------------------------------
1. 代码行数：传统 STL 12行，Ranges 4行
2. 中间容器：传统 STL 3个，Ranges 0个
3. 内存占用：传统 STL 高，Ranges 低
4. 可读性：Ranges 更清晰，从左到右的数据流
5. 性能：Ranges 惰性求值，只计算需要的元素
)" << endl;
}
