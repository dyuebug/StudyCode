// ============================================
// 87_concepts_print.cpp - 使用概念的通用打印函数
// 功能：用 C++20 概念重写通用打印函数，对比 SFINAE 版本
// 编译：g++ -std=c++20 -o 87_concepts_print.exe 87_concepts_print.cpp
// ============================================

#include <iostream>
#include <concepts>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <string>
using namespace std;

// ============================================
// 概念定义
// ============================================

// 算术类型概念
template<typename T>
concept Arithmetic = std::integral<T> || std::floating_point<T>;

// 字符串类型概念
template<typename T>
concept StringLike =
    std::same_as<T, string> ||
    std::same_as<T, const char*>;

// 容器概念（排除 string）
template<typename T>
concept Container = requires(T t) {
    typename T::value_type;
    { t.begin() };
    { t.end() };
} && !std::same_as<T, string>;

// 指针概念（排除 const char*）
template<typename T>
concept PointerLike = std::is_pointer_v<T> && !std::same_as<T, const char*>;

// ============================================
// 前向声明（解决循环依赖）
// ============================================

template<Container C>
void print(const C& container);

// ============================================
// 打印函数（使用概念）
// ============================================

// 1. 打印算术类型
template<Arithmetic T>
void print(const T& value) {
    cout << value;
}

// 2. 打印布尔值
void print(bool value) {
    cout << (value ? "true" : "false");
}

// 3. 打印字符串（必须在指针之前）
template<StringLike T>
void print(const T& value) {
    cout << "\"" << value << "\"";
}

// 3b. 打印 C 字符串（显式重载）
void print(const char* value) {
    cout << "\"" << value << "\"";
}

// 4. 打印字符
void print(char value) {
    cout << "'" << value << "'";
}

// 5. 打印指针
template<PointerLike T>
void print(const T& value) {
    if (value == nullptr) {
        cout << "nullptr";
    } else {
        cout << "ptr(" << value << ")";
    }
}

// 6. 打印 pair
template<typename T1, typename T2>
void print(const pair<T1, T2>& p) {
    cout << "(";
    print(p.first);
    cout << ", ";
    print(p.second);
    cout << ")";
}

// 7. 打印容器
template<Container C>
void print(const C& container) {
    cout << "[";
    bool first = true;
    for (const auto& item : container) {
        if (!first) cout << ", ";
        print(item);
        first = false;
    }
    cout << "]";
}

// 8. 可变参数打印
template<typename... Args>
void println(Args&&... args) {
    (print(args), ...);
    cout << endl;
}

// 9. 打印带分隔符
template<typename T>
void print_with_sep(const T& value) {
    print(value);
}

template<typename T, typename... Args>
void print_with_sep(const T& first, Args&&... rest) {
    print(first);
    if (sizeof...(rest) > 0) {
        cout << ", ";
    }
    print_with_sep(std::forward<Args>(rest)...);
}

// 10. 打印带标签
template<typename T>
void print_labeled(const char* label, const T& value) {
    cout << label << " = ";
    print(value);
    cout << endl;
}

// 11. 调试打印（带类型信息）
template<typename T>
void debug_print(const T& value) {
    cout << "[";

    // 打印类型信息
    if constexpr (std::integral<T>) {
        cout << "int";
    } else if constexpr (std::floating_point<T>) {
        cout << "float";
    } else if constexpr (std::is_pointer_v<T>) {
        cout << "ptr";
    } else if constexpr (std::same_as<T, string>) {
        cout << "string";
    } else if constexpr (Container<T>) {
        cout << "container";
    } else {
        cout << "unknown";
    }

    cout << "] ";
    print(value);
    cout << endl;
}

// ============================================
// 常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonErrors() {
    cout << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
❌ 错误1：概念版 print 对 string 当作 range 遍历（输出逐字符）

   template<std::ranges::range T>
   void print(const T& v) {
       for (auto& x : v) { print(x); }
   }
   print(string("hello"));  // ❌ 输出 h e l l o，不是 "hello"

   ✅ string 要单独重载（比模板更精确优先）：
   void print(const string& s) { cout << s; }   // 优先于模板

------------------------------------------------------------

❌ 错误2：概念约束互相重叠导致歧义

   template<Printable T>
   void print(T val) { ... }   // 重载1

   template<std::ranges::range T>
   void print(const T& v) { ... }  // 重载2

   // vector<int> 同时满足两个概念，调用歧义
   print(vector<int>{1,2,3});  // ❌ 模糊

   ✅ 用互斥概念，或用 requires 子句精确排除：
   template<std::ranges::range T>
   requires (!Printable<T>)   // 排除已被重载1覆盖的类型
   void print(const T& v) { ... }

------------------------------------------------------------

❌ 错误3：concepts 版本和 SFINAE 版本混用导致选择混乱

   // 同一文件中同时有：
   template<typename T>
   enable_if_t<is_arithmetic_v<T>> print(T v) { ... }  // SFINAE 版

   template<std::integral T>
   void print(T v) { ... }  // Concepts 版

   // int 同时匹配两个 → 歧义

   ✅ 选一种风格，不混用：C++20 项目用 concepts，旧代码用 SFINAE

------------------------------------------------------------

❌ 错误4：忘记为 pair 和 map 特化 print

   map<string, int> m = {{"a", 1}};
   print(m);   // 若没有 pair 重载，尝试直接打印 pair → 编译错误

   ✅ 提供 pair 重载：
   template<typename K, typename V>
   void print(const pair<K,V>& p) {
       print(p.first); cout << ":"; print(p.second);
   }

------------------------------------------------------------

❌ 错误5：concepts 在 GCC/Clang 旧版本不支持（忘记指定 -std=c++20）

   g++ file.cpp               // ❌ 默认 C++17，concepts 语法报错
   g++ -std=c++20 file.cpp    // ✅

   常见报错提示："concepts" is not a member of "std"
               "'concept' is not valid in C++17"
)" << endl;
}

int main() {
    cout << "=== 使用概念的通用打印函数 ===" << endl;

    // 示例1：打印基本类型
    {
        cout << "\n【示例1】打印基本类型" << endl;
        cout << string(60, '-') << endl;

        cout << "整数：";
        println(42);

        cout << "浮点数：";
        println(3.14);

        cout << "布尔值：";
        println(true, ", ", false);

        cout << "字符：";
        println('A', ", ", 'B');
    }

    // 示例2：打印字符串
    {
        cout << "\n【示例2】打印字符串" << endl;
        cout << string(60, '-') << endl;

        cout << "string：";
        println(string("Hello"));

        cout << "C 字符串：";
        println("World");

        cout << "混合：";
        print_with_sep(string("C++"), "is", "awesome");
        cout << endl;
    }

    // 示例3：打印指针
    {
        cout << "\n【示例3】打印指针" << endl;
        cout << string(60, '-') << endl;

        int x = 10;
        int* ptr = &x;
        int* null_ptr = nullptr;

        cout << "普通指针：";
        println(ptr);

        cout << "空指针：";
        println(null_ptr);
    }

    // 示例4：打印 pair
    {
        cout << "\n【示例4】打印 pair" << endl;
        cout << string(60, '-') << endl;

        cout << "int pair：";
        println(make_pair(1, 2));

        cout << "string pair：";
        println(make_pair(string("key"), string("value")));

        cout << "混合 pair：";
        println(make_pair(1, string("one")));
    }

    // 示例5：打印容器
    {
        cout << "\n【示例5】打印容器" << endl;
        cout << string(60, '-') << endl;

        cout << "vector<int>：";
        vector<int> vec = {1, 2, 3, 4, 5};
        println(vec);

        cout << "list<string>：";
        list<string> lst = {"hello", "world", "!"};
        println(lst);

        cout << "set<int>：";
        set<int> s = {5, 3, 1, 4, 2};
        println(s);
    }

    // 示例6：打印嵌套容器
    {
        cout << "\n【示例6】打印嵌套容器" << endl;
        cout << string(60, '-') << endl;

        cout << "vector<vector<int>>：";
        vector<vector<int>> nested = {{1, 2}, {3, 4}, {5, 6}};
        println(nested);

        cout << "vector<pair<int, string>>：";
        vector<pair<int, string>> pairs = {
            {1, "one"},
            {2, "two"},
            {3, "three"}
        };
        println(pairs);
    }

    // 示例7：可变参数打印
    {
        cout << "\n【示例7】可变参数打印" << endl;
        cout << string(60, '-') << endl;

        cout << "多个参数：";
        println(1, ", ", 2.5, ", ", string("hello"), ", ", 'c');

        cout << "带分隔符：";
        print_with_sep(1, 2, 3, 4, 5);
        cout << endl;
    }

    // 示例8：打印带标签
    {
        cout << "\n【示例8】打印带标签" << endl;
        cout << string(60, '-') << endl;

        print_labeled("整数", 42);
        print_labeled("浮点数", 3.14);
        print_labeled("字符串", string("Hello"));

        vector<int> vec = {1, 2, 3};
        print_labeled("vector", vec);
    }

    // 示例9：调试打印
    {
        cout << "\n【示例9】调试打印（带类型信息）" << endl;
        cout << string(60, '-') << endl;

        debug_print(42);
        debug_print(3.14);
        debug_print(string("Hello"));

        vector<int> vec = {1, 2, 3};
        debug_print(vec);

        int x = 10;
        debug_print(&x);
    }

    // 示例10：复杂数据结构
    {
        cout << "\n【示例10】复杂数据结构" << endl;
        cout << string(60, '-') << endl;

        // map 需要特殊处理（因为元素是 pair）
        map<int, string> m = {
            {1, "one"},
            {2, "two"},
            {3, "three"}
        };

        cout << "map<int, string>：";
        println(m);

        // 嵌套的复杂结构
        vector<pair<int, vector<int>>> complex = {
            {1, {10, 20}},
            {2, {30, 40}},
            {3, {50, 60}}
        };

        cout << "复杂嵌套：";
        println(complex);
    }

    // 示例11：概念 vs SFINAE 对比
    {
        cout << "\n【示例11】概念 vs SFINAE 对比" << endl;
        cout << string(60, '-') << endl;

        cout << R"(
SFINAE 版本（昨天的代码）：
------------------------------------------------------------
// 打印算术类型
template<typename T>
enable_if_t<is_arithmetic<T>::value>
print(const T& value) {
    cout << value;
}

// 打印容器
template<typename T>
enable_if_t<has_iterator<T>::value && !is_same<T, string>::value>
print(const T& container) {
    cout << "[";
    for (const auto& item : container) {
        print(item);
    }
    cout << "]";
}

概念版本（今天的代码）：
------------------------------------------------------------
// 打印算术类型
template<Arithmetic T>
void print(const T& value) {
    cout << value;
}

// 打印容器
template<Container C>
void print(const C& container) {
    cout << "[";
    for (const auto& item : container) {
        print(item);
    }
    cout << "]";
}

对比总结：
------------------------------------------------------------
1. 代码更清晰：Arithmetic 比 enable_if_t<is_arithmetic<T>::value> 更易读
2. 意图更明确：Container 直接表达"这是一个容器"的约束
3. 错误信息更友好：编译错误会直接说"不满足 Arithmetic 概念"
4. 定义更简洁：概念可以命名和复用，SFINAE 需要重复写约束表达式
5. 组合更灵活：概念可以用 &&、|| 组合，SFINAE 需要复杂的嵌套

结论：概念是现代 C++ 模板编程的最佳实践！
)" << endl;
    }

    // 示例12：实际应用 - 日志系统
    {
        cout << "\n【示例12】实际应用 - 日志系统" << endl;
        cout << string(60, '-') << endl;

        auto log = [](const char* level, auto&&... args) {
            cout << "[" << level << "] ";
            print_with_sep(std::forward<decltype(args)>(args)...);
            cout << endl;
        };

        log("INFO", "服务器启动", "端口", 8080);
        log("DEBUG", "连接数", 5, "活跃用户", 3);
        log("ERROR", "连接失败", "错误码", 404);

        vector<string> users = {"Alice", "Bob", "Charlie"};
        log("INFO", "在线用户", users);
    }

    cout << "\n程序结束" << endl;
    return 0;
}
