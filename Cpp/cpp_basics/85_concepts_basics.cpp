// ============================================
// 85_concepts_basics.cpp - C++20 概念基础
// 功能：学习概念的定义、使用和标准库概念
// 编译：g++ -std=c++20 -o 85_concepts_basics.exe 85_concepts_basics.cpp
// ============================================

#include <iostream>
#include <concepts>
#include <vector>
#include <string>
#include <type_traits>
using namespace std;

// ============================================
// 示例1：定义简单概念
// ============================================

// 数值类型概念
template<typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

// 使用概念约束函数
template<Numeric T>
T add(T a, T b) {
    return a + b;
}

// ============================================
// 示例2：可打印概念
// ============================================

template<typename T>
concept Printable = requires(T t) {
    { cout << t } -> std::same_as<ostream&>;
};

template<Printable T>
void print(const T& value) {
    cout << value << endl;
}

// ============================================
// 示例3：容器概念
// ============================================

template<typename T>
concept Container = requires(T t) {
    typename T::value_type;
    { t.size() } -> std::convertible_to<size_t>;
    { t.begin() };
    { t.end() };
};

template<Container C>
void print_container(const C& container) {
    cout << "[";
    bool first = true;
    for (const auto& item : container) {
        if (!first) cout << ", ";
        cout << item;
        first = false;
    }
    cout << "]" << endl;
}

// ============================================
// 示例4：多个概念组合
// ============================================

template<typename T>
concept Addable = requires(T a, T b) {
    { a + b } -> std::same_as<T>;
};

template<typename T>
concept Comparable = std::totally_ordered<T>;

template<typename T>
requires Addable<T> && Comparable<T>
T max_sum(T a, T b, T c) {
    T sum1 = a + b;
    T sum2 = b + c;
    return sum1 > sum2 ? sum1 : sum2;
}

// ============================================
// 示例5：概念的四种使用方式
// ============================================

// 方式1：requires 子句
template<typename T>
requires std::integral<T>
void method1(T value) {
    cout << "方式1（requires 子句）：" << value << endl;
}

// 方式2：简写形式（推荐）
template<std::integral T>
void method2(T value) {
    cout << "方式2（简写形式）：" << value << endl;
}

// 方式3：尾置 requires
template<typename T>
void method3(T value) requires std::integral<T> {
    cout << "方式3（尾置 requires）：" << value << endl;
}

// 方式4：auto 参数（C++20）
void method4(std::integral auto value) {
    cout << "方式4（auto 参数）：" << value << endl;
}

// ============================================
// 示例6：标准库概念
// ============================================

// 使用标准库的核心概念
template<std::signed_integral T>
T negate_value(T value) {
    return -value;
}

// 使用比较概念
template<std::equality_comparable T>
bool are_equal(const T& a, const T& b) {
    return a == b;
}

// 使用对象概念
template<std::copyable T>
T duplicate(const T& value) {
    return value;
}

// ============================================
// 示例7：自定义复杂概念
// ============================================

// 可迭代概念
template<typename T>
concept Iterable = requires(T t) {
    { t.begin() } -> std::input_iterator;
    { t.end() } -> std::input_iterator;
};

// 有 size() 方法的概念
template<typename T>
concept HasSize = requires(T t) {
    { t.size() } -> std::convertible_to<size_t>;
};

// 可排序容器概念
template<typename T>
concept SortableContainer =
    Iterable<T> &&
    HasSize<T> &&
    requires(T t) {
        requires std::totally_ordered<typename T::value_type>;
    };

template<SortableContainer C>
void print_sorted_info(const C& container) {
    cout << "可排序容器，大小：" << container.size() << endl;
}

// ============================================
// 示例8：概念与 if constexpr 结合
// ============================================

template<typename T>
concept StringLike =
    std::same_as<T, string> ||
    std::same_as<T, const char*>;

template<typename T>
void process(const T& value) {
    if constexpr (std::integral<T>) {
        cout << "处理整数：" << value << endl;
    } else if constexpr (std::floating_point<T>) {
        cout << "处理浮点数：" << value << endl;
    } else if constexpr (StringLike<T>) {
        cout << "处理字符串：" << value << endl;
    } else {
        cout << "处理其他类型" << endl;
    }
}

// ============================================
// 示例9：概念的特化
// ============================================

// 通用版本
template<Numeric T>
T square(T value) {
    cout << "通用平方：";
    return value * value;
}

// 整数特化版本
template<std::integral T>
T square(T value) {
    cout << "整数平方优化：";
    return value * value;
}

// ============================================
// 示例10：可调用概念
// ============================================

// 谓词概念（返回 bool 的可调用对象）
template<typename F, typename T>
concept Predicate = std::predicate<F, T>;

template<typename T, Predicate<T> F>
void filter_print(const vector<T>& vec, F pred) {
    cout << "过滤结果：[";
    bool first = true;
    for (const auto& item : vec) {
        if (pred(item)) {
            if (!first) cout << ", ";
            cout << item;
            first = false;
        }
    }
    cout << "]" << endl;
}

// ============================================
// 常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonErrors() {
    cout << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
❌ 错误1：在非模板函数中使用概念（概念只用于模板）

   Integral auto x = 3.14;   // ❌ 3.14 不满足 Integral，编译错误
   // 但错误信息比 SFINAE 清晰：直接说 "3.14 不满足概念 Integral"

   ✅ 概念是约束模板参数的工具，不是运行期类型检查
   template<Integral T>
   void func(T val) { ... }   // ✅ 约束模板参数

------------------------------------------------------------

❌ 错误2：概念定义中使用了运行期表达式（概念是编译期的）

   template<typename T>
   concept HasPositiveSize = T().size() > 0;  // ❌ 运行期值，概念无法使用

   ✅ 概念只检查表达式是否合法（类型层面），不检查运行期值：
   template<typename T>
   concept HasSize = requires(T t) { t.size(); };  // ✅ 只检查 size() 能否调用

------------------------------------------------------------

❌ 错误3：requires 子句和 requires 表达式混淆

   // requires 子句：函数模板的约束（后接布尔表达式）
   template<typename T>
   requires is_integral_v<T>   // ← requires 子句
   void func(T val) {}

   // requires 表达式：检查表达式是否合法，返回 bool
   template<typename T>
   concept HasSize = requires(T t) { t.size(); };  // ← requires 表达式（用于定义概念）

   // 混淆写法：
   template<typename T>
   requires requires(T t) { t.size(); }   // ← 两个 requires：外层子句，内层表达式
   void func(T val) {}   // 合法但难看，推荐定义命名概念

------------------------------------------------------------

❌ 错误4：概念约束不够精确（仅检查接口存在，不检查语义）

   template<typename T>
   concept Container = requires(T t) {
       t.begin(); t.end(); t.size();
   };
   // ❌ int* 也有 begin/end（通过 std::begin），可能误匹配

   ✅ 用标准库概念更精确：
   template<std::ranges::range T>   // ✅ 标准 range 概念，语义明确
   void process(T& c) { ... }

------------------------------------------------------------

❌ 错误5：C++20 概念编译器支持不完整时的报错

   // g++ < 10、clang++ < 10 不支持概念
   // 编译时报："concepts" not found / unknown syntax

   ✅ 确认编译命令：g++ -std=c++20 file.cpp
   ✅ 确认编译器版本：g++ >= 10，clang++ >= 10，MSVC >= 19.28

------------------------------------------------------------

❌ 错误6：用概念替代所有类型检查（过度约束）

   template<std::integral T, std::floating_point U,
            std::same_as<std::string> S>
   void tooConstrained(T a, U b, S c) { ... }

   // 过度约束导致灵活性下降，合理的调用都被拒绝
   tooConstrained(1, 2.0, string("x"));  // 勉强能通过
   tooConstrained(1, 2.0, "x");          // ❌ "x" 是 const char*，不是 string

   ✅ 只约束真正需要的操作，不过度限制：
   template<std::integral T, std::floating_point U, typename S>
   requires std::convertible_to<S, string>
   void func(T a, U b, S c) { ... }
)" << endl;
}

int main() {
    cout << "=== C++20 概念基础示例 ===" << endl;

    // 示例1：Numeric 概念
    {
        cout << "\n【示例1】Numeric 概念" << endl;
        cout << string(60, '-') << endl;

        cout << "add(10, 20) = " << add(10, 20) << endl;
        cout << "add(3.14, 2.86) = " << add(3.14, 2.86) << endl;

        // add("hello", "world");  // 编译错误：string 不满足 Numeric
        cout << "注意：add(\"hello\", \"world\") 会编译错误" << endl;
    }

    // 示例2：Printable 概念
    {
        cout << "\n【示例2】Printable 概念" << endl;
        cout << string(60, '-') << endl;

        print(42);
        print(3.14);
        print(string("Hello"));
        print('A');
    }

    // 示例3：Container 概念
    {
        cout << "\n【示例3】Container 概念" << endl;
        cout << string(60, '-') << endl;

        vector<int> vec = {1, 2, 3, 4, 5};
        print_container(vec);

        vector<string> words = {"hello", "world", "!"};
        print_container(words);
    }

    // 示例4：多个概念组合
    {
        cout << "\n【示例4】多个概念组合" << endl;
        cout << string(60, '-') << endl;

        cout << "max_sum(1, 2, 3) = " << max_sum(1, 2, 3) << endl;
        cout << "max_sum(1.5, 2.5, 3.5) = " << max_sum(1.5, 2.5, 3.5) << endl;
    }

    // 示例5：概念的四种使用方式
    {
        cout << "\n【示例5】概念的四种使用方式" << endl;
        cout << string(60, '-') << endl;

        method1(10);
        method2(20);
        method3(30);
        method4(40);
    }

    // 示例6：标准库概念
    {
        cout << "\n【示例6】标准库概念" << endl;
        cout << string(60, '-') << endl;

        cout << "negate_value(10) = " << negate_value(10) << endl;
        cout << "negate_value(-5) = " << negate_value(-5) << endl;

        cout << "are_equal(10, 10) = " << are_equal(10, 10) << endl;
        cout << "are_equal(10, 20) = " << are_equal(10, 20) << endl;

        string s = "Hello";
        string s2 = duplicate(s);
        cout << "duplicate(\"Hello\") = \"" << s2 << "\"" << endl;
    }

    // 示例7：SortableContainer 概念
    {
        cout << "\n【示例7】SortableContainer 概念" << endl;
        cout << string(60, '-') << endl;

        vector<int> vec = {5, 3, 1, 4, 2};
        print_sorted_info(vec);

        vector<string> words = {"hello", "world", "!"};
        print_sorted_info(words);
    }

    // 示例8：概念与 if constexpr 结合
    {
        cout << "\n【示例8】概念与 if constexpr 结合" << endl;
        cout << string(60, '-') << endl;

        process(42);
        process(3.14);
        process(string("Hello"));
        process("World");
    }

    // 示例9：概念的特化
    {
        cout << "\n【示例9】概念的特化" << endl;
        cout << string(60, '-') << endl;

        cout << square(10) << endl;
        cout << square(3.14) << endl;
    }

    // 示例10：Predicate 概念
    {
        cout << "\n【示例10】Predicate 概念" << endl;
        cout << string(60, '-') << endl;

        vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

        // 过滤偶数
        filter_print(numbers, [](int x) { return x % 2 == 0; });

        // 过滤大于5的数
        filter_print(numbers, [](int x) { return x > 5; });
    }

    // 示例11：概念 vs SFINAE 对比
    {
        cout << "\n【示例11】概念 vs SFINAE 对比" << endl;
        cout << string(60, '-') << endl;

        cout << R"(
SFINAE 方式（复杂）：
template<typename T>
enable_if_t<is_integral<T>::value>
func(T value) { ... }

概念方式（清晰）：
template<std::integral T>
void func(T value) { ... }

或者更简洁：
void func(std::integral auto value) { ... }

优势：
1. 代码更清晰易读
2. 编译错误信息更友好
3. 是现代 C++ 的最佳实践
)" << endl;
    }

    // 示例12：标准库概念速查
    {
        cout << "\n【示例12】标准库概念速查" << endl;
        cout << string(60, '-') << endl;

        cout << R"(
核心语言概念（<concepts>）：
- std::same_as<T, U>           // T 和 U 是相同类型
- std::derived_from<T, U>      // T 派生自 U
- std::convertible_to<T, U>    // T 可以转换为 U
- std::integral<T>             // T 是整数类型
- std::floating_point<T>       // T 是浮点类型
- std::signed_integral<T>      // T 是有符号整数
- std::unsigned_integral<T>    // T 是无符号整数

比较概念：
- std::equality_comparable<T>  // T 支持 == 和 !=
- std::totally_ordered<T>      // T 支持 <, >, <=, >=

对象概念：
- std::movable<T>              // T 可以移动
- std::copyable<T>             // T 可以拷贝
- std::semiregular<T>          // T 可以默认构造和拷贝
- std::regular<T>              // T 是规则类型

可调用概念：
- std::invocable<F, Args...>   // F 可以用 Args... 调用
- std::predicate<F, Args...>   // F 是谓词（返回 bool）
)" << endl;
    }

    cout << "\n程序结束" << endl;
    return 0;
}
