// ============================================
// 86_requires_expressions.cpp - requires 表达式详解
// 功能：学习 requires 表达式的四种形式和约束组合
// 编译：g++ -std=c++20 -o 86_requires_expressions.exe 86_requires_expressions.cpp
// ============================================

#include <iostream>
#include <concepts>
#include <vector>
#include <string>
#include <iterator>
using namespace std;

// ============================================
// 示例1：简单要求（Simple Requirements）
// ============================================

template<typename T>
concept Addable = requires(T a, T b) {
    a + b;  // 表达式必须有效
};

template<Addable T>
T add(T a, T b) {
    return a + b;
}

template<typename T>
concept Subtractable = requires(T a, T b) {
    a - b;
};

template<typename T>
concept Multipliable = requires(T a, T b) {
    a * b;
};

// ============================================
// 示例2：类型要求（Type Requirements）
// ============================================

template<typename T>
concept HasValueType = requires {
    typename T::value_type;  // 必须有 value_type 类型
};

template<typename T>
concept HasIterator = requires {
    typename T::iterator;
    typename T::const_iterator;
};

template<typename T>
concept HasAllTypes = requires {
    typename T::value_type;
    typename T::iterator;
    typename T::size_type;
};

// ============================================
// 示例3：复合要求（Compound Requirements）
// ============================================

template<typename T>
concept HasSize = requires(T t) {
    { t.size() } -> std::convertible_to<size_t>;  // 返回类型约束
};

template<typename T>
concept HasEmpty = requires(T t) {
    { t.empty() } noexcept -> std::same_as<bool>;  // noexcept 约束
};

template<typename T>
concept Container = requires(T t) {
    { t.begin() } -> std::input_iterator;
    { t.end() } -> std::input_iterator;
    { t.size() } -> std::convertible_to<size_t>;
};

// ============================================
// 示例4：嵌套要求（Nested Requirements）
// ============================================

template<typename T>
concept SortableContainer = requires(T t) {
    requires Container<T>;  // 嵌套概念
    requires std::totally_ordered<typename T::value_type>;
};

template<typename T>
concept RandomAccessContainer = requires(T t) {
    requires Container<T>;
    requires std::random_access_iterator<typename T::iterator>;
};

// ============================================
// 示例5：约束的组合
// ============================================

// 逻辑与（&&）
template<typename T>
concept SignedIntegral = std::integral<T> && std::signed_integral<T>;

// 逻辑或（||）
template<typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

// 逻辑非（!）
template<typename T>
concept NotPointer = !std::is_pointer_v<T>;

// 复杂组合
template<typename T>
concept StringLike =
    std::convertible_to<T, std::string> ||
    std::same_as<T, const char*> ||
    std::same_as<T, char*>;

// ============================================
// 示例6：概念的特化
// ============================================

// 基础概念
template<typename T>
concept Arithmetic = std::integral<T> || std::floating_point<T>;

// 通用版本
template<Arithmetic T>
T multiply(T a, T b) {
    cout << "通用乘法：";
    return a * b;
}

// 特化版本（整数优化）
template<std::integral T>
T multiply(T a, T b) {
    cout << "整数乘法优化：";
    return a * b;
}

// ============================================
// 示例7：复杂的 requires 表达式
// ============================================

template<typename T>
concept ComplexContainer = requires(T t, typename T::value_type v) {
    // 类型要求
    typename T::value_type;
    typename T::iterator;
    typename T::const_iterator;

    // 简单要求
    t.begin();
    t.end();

    // 复合要求
    { t.size() } -> std::convertible_to<size_t>;
    { t.empty() } -> std::same_as<bool>;
    { t.push_back(v) } -> std::same_as<void>;

    // 嵌套要求
    requires std::copyable<typename T::value_type>;
};

// ============================================
// 示例8：可调用概念
// ============================================

template<typename F, typename T>
concept UnaryPredicate = requires(F f, T t) {
    { f(t) } -> std::convertible_to<bool>;
};

template<typename T, UnaryPredicate<T> F>
void filter_print(const vector<T>& vec, F pred) {
    cout << "[";
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
// 示例9：比较概念
// ============================================

template<typename T>
concept EqualityComparable = requires(T a, T b) {
    { a == b } -> std::convertible_to<bool>;
    { a != b } -> std::convertible_to<bool>;
};

template<typename T>
concept LessThanComparable = requires(T a, T b) {
    { a < b } -> std::convertible_to<bool>;
};

template<typename T>
concept FullyOrdered = requires(T a, T b) {
    { a < b } -> std::convertible_to<bool>;
    { a > b } -> std::convertible_to<bool>;
    { a <= b } -> std::convertible_to<bool>;
    { a >= b } -> std::convertible_to<bool>;
    { a == b } -> std::convertible_to<bool>;
    { a != b } -> std::convertible_to<bool>;
};

// ============================================
// 示例10：迭代器概念
// ============================================

template<typename T>
concept InputIteratorLike = requires(T it) {
    { *it };        // 可解引用
    { ++it } -> std::same_as<T&>;  // 可前置递增
    { it++ };       // 可后置递增
    { it == it } -> std::convertible_to<bool>;  // 可比较
    { it != it } -> std::convertible_to<bool>;
};

// ============================================
// 常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonErrors() {
    cout << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
❌ 错误1：简单要求误写为类型要求

   requires {
       typename T::value_type;   // ✅ 类型要求：检查类型是否存在
       T::value_type;            // ❌ 这不是类型要求！是表达式要求（值是否合法）
   }

   记忆：类型要求必须加 typename 前缀

------------------------------------------------------------

❌ 错误2：复合要求的返回类型约束写法错误

   requires(T t) {
       { t.size() } -> size_t;              // ❌ 不能直接写类型名
       { t.size() } -> std::same_as<size_t>; // ✅ 必须是概念（可调用形式）
       { t.size() } -> std::convertible_to<size_t>; // ✅ 也可以用 convertible_to
   }

------------------------------------------------------------

❌ 错误3：嵌套要求中的表达式不是布尔常量表达式

   requires(T t) {
       requires t.size() > 0;  // ❌ t.size() 是运行期值，不是编译期常量
       requires sizeof(T) > 4; // ✅ sizeof 是编译期常量
       requires std::integral<T>; // ✅ 概念是编译期布尔
   }

------------------------------------------------------------

❌ 错误4：requires 子句约束多个参数时遗漏 &&

   template<typename T, typename U>
   requires std::integral<T> std::floating_point<U>  // ❌ 缺少 &&
   void func(T a, U b) {}

   ✅ 多个约束用 && 或 ||：
   requires std::integral<T> && std::floating_point<U>

------------------------------------------------------------

❌ 错误5：requires 表达式仅检测接口，无法检测语义正确性

   template<typename T>
   concept Sortable = requires(T t) {
       t.sort();   // 只检查 sort() 方法存在，不检查是否真的排序
   };

   struct Fake {
       void sort() { /* 什么都不做 */ }
   };
   // Fake 满足 Sortable，但 sort() 是假的
   // ✅ 概念验证接口，语义正确性靠文档和单元测试保证
)" << endl;
}

int main() {
    cout << "=== requires 表达式详解 ===" << endl;

    // 示例1：简单要求
    {
        cout << "\n【示例1】简单要求" << endl;
        cout << string(60, '-') << endl;

        cout << "add(10, 20) = " << add(10, 20) << endl;
        cout << "add(3.14, 2.86) = " << add(3.14, 2.86) << endl;
        cout << "add(\"hello\", \"world\") = " << add(string("hello"), string("world")) << endl;

        cout << "\nAddable<int>: " << Addable<int> << endl;
        cout << "Addable<string>: " << Addable<string> << endl;
        cout << "Subtractable<int>: " << Subtractable<int> << endl;
        cout << "Subtractable<string>: " << Subtractable<string> << endl;
    }

    // 示例2：类型要求
    {
        cout << "\n【示例2】类型要求" << endl;
        cout << string(60, '-') << endl;

        cout << "HasValueType<vector<int>>: " << HasValueType<vector<int>> << endl;
        cout << "HasValueType<int>: " << HasValueType<int> << endl;

        cout << "HasIterator<vector<int>>: " << HasIterator<vector<int>> << endl;
        cout << "HasIterator<string>: " << HasIterator<string> << endl;
        cout << "HasIterator<int>: " << HasIterator<int> << endl;
    }

    // 示例3：复合要求
    {
        cout << "\n【示例3】复合要求" << endl;
        cout << string(60, '-') << endl;

        cout << "HasSize<vector<int>>: " << HasSize<vector<int>> << endl;
        cout << "HasSize<string>: " << HasSize<string> << endl;
        cout << "HasSize<int>: " << HasSize<int> << endl;

        cout << "\nContainer<vector<int>>: " << Container<vector<int>> << endl;
        cout << "Container<string>: " << Container<string> << endl;
        cout << "Container<int>: " << Container<int> << endl;
    }

    // 示例4：嵌套要求
    {
        cout << "\n【示例4】嵌套要求" << endl;
        cout << string(60, '-') << endl;

        cout << "SortableContainer<vector<int>>: " << SortableContainer<vector<int>> << endl;
        cout << "SortableContainer<vector<string>>: " << SortableContainer<vector<string>> << endl;

        cout << "\nRandomAccessContainer<vector<int>>: " << RandomAccessContainer<vector<int>> << endl;
    }

    // 示例5：约束的组合
    {
        cout << "\n【示例5】约束的组合" << endl;
        cout << string(60, '-') << endl;

        cout << "SignedIntegral<int>: " << SignedIntegral<int> << endl;
        cout << "SignedIntegral<unsigned int>: " << SignedIntegral<unsigned int> << endl;

        cout << "\nNumeric<int>: " << Numeric<int> << endl;
        cout << "Numeric<double>: " << Numeric<double> << endl;
        cout << "Numeric<string>: " << Numeric<string> << endl;

        cout << "\nStringLike<string>: " << StringLike<string> << endl;
        cout << "StringLike<const char*>: " << StringLike<const char*> << endl;
        cout << "StringLike<int>: " << StringLike<int> << endl;
    }

    // 示例6：概念的特化
    {
        cout << "\n【示例6】概念的特化" << endl;
        cout << string(60, '-') << endl;

        cout << multiply(10, 20) << endl;
        cout << multiply(3.14, 2.0) << endl;
    }

    // 示例7：ComplexContainer 概念
    {
        cout << "\n【示例7】ComplexContainer 概念" << endl;
        cout << string(60, '-') << endl;

        cout << "ComplexContainer<vector<int>>: " << ComplexContainer<vector<int>> << endl;
        cout << "ComplexContainer<vector<string>>: " << ComplexContainer<vector<string>> << endl;
    }

    // 示例8：UnaryPredicate 概念
    {
        cout << "\n【示例8】UnaryPredicate 概念" << endl;
        cout << string(60, '-') << endl;

        vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

        cout << "偶数：";
        filter_print(numbers, [](int x) { return x % 2 == 0; });

        cout << "大于5：";
        filter_print(numbers, [](int x) { return x > 5; });

        cout << "能被3整除：";
        filter_print(numbers, [](int x) { return x % 3 == 0; });
    }

    // 示例9：比较概念
    {
        cout << "\n【示例9】比较概念" << endl;
        cout << string(60, '-') << endl;

        cout << "EqualityComparable<int>: " << EqualityComparable<int> << endl;
        cout << "EqualityComparable<string>: " << EqualityComparable<string> << endl;

        cout << "\nLessThanComparable<int>: " << LessThanComparable<int> << endl;
        cout << "LessThanComparable<string>: " << LessThanComparable<string> << endl;

        cout << "\nFullyOrdered<int>: " << FullyOrdered<int> << endl;
        cout << "FullyOrdered<string>: " << FullyOrdered<string> << endl;
    }

    // 示例10：迭代器概念
    {
        cout << "\n【示例10】迭代器概念" << endl;
        cout << string(60, '-') << endl;

        cout << "InputIteratorLike<vector<int>::iterator>: "
             << InputIteratorLike<vector<int>::iterator> << endl;
        cout << "InputIteratorLike<int*>: " << InputIteratorLike<int*> << endl;
        cout << "InputIteratorLike<int>: " << InputIteratorLike<int> << endl;
    }

    // 示例11：requires 表达式的四种形式总结
    {
        cout << "\n【示例11】requires 表达式的四种形式总结" << endl;
        cout << string(60, '-') << endl;

        cout << R"(
1. 简单要求（Simple Requirements）：
   requires(T a, T b) {
       a + b;  // 表达式必须有效
   }

2. 类型要求（Type Requirements）：
   requires {
       typename T::value_type;  // 必须有该类型
   }

3. 复合要求（Compound Requirements）：
   requires(T t) {
       { t.size() } -> std::convertible_to<size_t>;  // 返回类型约束
       { t.empty() } noexcept -> std::same_as<bool>;  // noexcept 约束
   }

4. 嵌套要求（Nested Requirements）：
   requires(T t) {
       requires Container<T>;  // 嵌套其他概念
       requires std::totally_ordered<typename T::value_type>;
   }
)" << endl;
    }

    // 示例12：概念组合运算符
    {
        cout << "\n【示例12】概念组合运算符" << endl;
        cout << string(60, '-') << endl;

        cout << R"(
逻辑与（&&）：
template<typename T>
concept SignedIntegral = std::integral<T> && std::signed_integral<T>;

逻辑或（||）：
template<typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

逻辑非（!）：
template<typename T>
concept NotPointer = !std::is_pointer_v<T>;

复杂组合：
template<typename T>
concept StringLike =
    std::convertible_to<T, std::string> ||
    std::same_as<T, const char*> ||
    std::same_as<T, char*>;
)" << endl;
    }

    cout << "\n程序结束" << endl;
    return 0;
}
