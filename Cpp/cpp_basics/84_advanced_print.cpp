// ============================================
// 84_advanced_print.cpp - 通用打印函数
// 功能：综合应用可变参数模板和 SFINAE
// ============================================

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <type_traits>
#include <utility>
using namespace std;

// ============================================
// 辅助：检测是否有 begin/end
// ============================================

template<typename T>
class has_iterator {
    template<typename U>
    static auto test(int) -> decltype(
        declval<U>().begin(),
        declval<U>().end(),
        true_type{}
    );

    template<typename>
    static false_type test(...);

public:
    static constexpr bool value = decltype(test<T>(0))::value;
};

// ============================================
// 前向声明（解决循环依赖）
// ============================================

// 声明容器打印函数
template<typename T>
enable_if_t<has_iterator<T>::value && !is_same<T, string>::value>
print(const T& container);

// ============================================
// 1. 打印基本类型（整数、浮点数）
// ============================================

template<typename T>
enable_if_t<is_arithmetic<T>::value>
print(const T& value) {
    cout << value;
}

// ============================================
// 2. 打印布尔值
// ============================================

void print(bool value) {
    cout << (value ? "true" : "false");
}

// ============================================
// 3. 打印字符串
// ============================================

void print(const string& value) {
    cout << "\"" << value << "\"";
}

void print(const char* value) {
    cout << "\"" << value << "\"";
}

// ============================================
// 4. 打印字符
// ============================================

void print(char value) {
    cout << "'" << value << "'";
}

// ============================================
// 5. 打印指针
// ============================================

template<typename T>
enable_if_t<is_pointer<T>::value && !is_same<T, const char*>::value>
print(const T& value) {
    if (value == nullptr) {
        cout << "nullptr";
    } else {
        cout << "ptr(" << value << ")";
    }
}

// ============================================
// 6. 打印 pair
// ============================================

template<typename T1, typename T2>
void print(const pair<T1, T2>& p) {
    cout << "(";
    print(p.first);
    cout << ", ";
    print(p.second);
    cout << ")";
}

// ============================================
// 7. 打印容器（vector, list, set 等）
// ============================================

template<typename T>
enable_if_t<has_iterator<T>::value && !is_same<T, string>::value>
print(const T& container) {
    cout << "[";
    bool first = true;
    for (const auto& item : container) {
        if (!first) cout << ", ";
        print(item);
        first = false;
    }
    cout << "]";
}

// ============================================
// 8. 可变参数打印（带分隔符）
// ============================================

template<typename... Args>
void println(Args&&... args) {
    (print(args), ...);
    cout << endl;
}

template<typename T>
void print_with_sep(const T& value) {
    print(value);
}

template<typename T, typename... Args>
void print_with_sep(const T& first, Args... rest) {
    print(first);
    if (sizeof...(rest) > 0) {
        cout << ", ";
    }
    print_with_sep(rest...);
}

// ============================================
// 9. 打印带标签
// ============================================

template<typename T>
void print_labeled(const char* label, const T& value) {
    cout << label << " = ";
    print(value);
    cout << endl;
}

// ============================================
// 10. 调试打印（带类型信息）
// ============================================

template<typename T>
void debug_print(const T& value) {
    cout << "[";

    // 打印类型信息
    if constexpr (is_integral<T>::value) {
        cout << "int";
    } else if constexpr (is_floating_point<T>::value) {
        cout << "float";
    } else if constexpr (is_pointer<T>::value) {
        cout << "ptr";
    } else if constexpr (is_same<T, string>::value) {
        cout << "string";
    } else if constexpr (has_iterator<T>::value) {
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
❌ 错误1：SFINAE 检测 has_begin 时没有用 declval（要求默认构造）

   template<typename T, typename = void>
   struct has_begin : false_type {};

   template<typename T>
   struct has_begin<T, void_t<decltype(T().begin())>> : true_type {};
   // ❌ T() 要求 T 有默认构造函数；若无，SFINAE 失败变成编译错误

   ✅ 用 declval<T>() 绕过构造要求：
   struct has_begin<T, void_t<decltype(declval<T>().begin())>> : true_type {};

------------------------------------------------------------

❌ 错误2：print 打印容器时，对 string 也当容器展开（输出逐字符）

   // string 满足 has_begin<string>，被当作容器遍历，输出 h e l l o
   template<typename T>
   enable_if_t<has_begin<T>::value> print(const T& v) {
       for (auto& x : v) { print(x); cout << " "; }
   }

   ✅ 先特化或优先匹配 string：
   // 为 string 提供更精确的重载（普通函数比模板更优先）
   void print(const string& s) { cout << s; }

------------------------------------------------------------

❌ 错误3：递归 print 模板的优先级与特化顺序错误

   // 若先声明 print(T)，编译器可能选错版本
   template<typename T> void print(T val) { cout << val; }  // 通用版

   template<typename T>
   enable_if_t<has_begin<T>::value> print(const T& v) { ... } // 容器版

   // 当 T = vector<int> 时：两个模板都匹配，需要偏序选择
   // ❌ 如果特化不完整，可能导致无限递归或歧义

   ✅ 用不同的 enable_if 条件互斥覆盖所有情况，确保无重叠

------------------------------------------------------------

❌ 错误4：可变参数 print 展开顺序（逗号表达式 vs 初始化列表）

   template<typename... Args>
   void print(Args... args) {
       // ❌ 求值顺序未定义：
       int d[] = { (print(args), 0)... };  // 各元素求值顺序不保证

       // ✅ C++17 折叠表达式，从左到右：
       (print(args), ...);
   }

------------------------------------------------------------

❌ 错误5：打印 map 时混淆 pair 的 first/second 访问

   // map 的元素是 pair<const K, V>
   for (auto& elem : m) {
       cout << elem;      // ❌ pair 没有 operator<<
       cout << elem.first << ":" << elem.second;  // ✅
   }

   // 通用 print 需要专门处理 pair：
   template<typename K, typename V>
   void print(const pair<K,V>& p) {
       print(p.first); cout << ":"; print(p.second);
   }
)" << endl;
}

int main() {
    cout << "=== 通用打印函数示例 ===" << endl;

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

    // 示例11：实际应用 - 日志系统
    {
        cout << "\n【示例11】实际应用 - 日志系统" << endl;
        cout << string(60, '-') << endl;

        auto log = [](const char* level, auto... args) {
            cout << "[" << level << "] ";
            print_with_sep(args...);
            cout << endl;
        };

        log("INFO", "服务器启动", "端口", 8080);
        log("DEBUG", "连接数", 5, "活跃用户", 3);
        log("ERROR", "连接失败", "错误码", 404);

        vector<string> users = {"Alice", "Bob", "Charlie"};
        log("INFO", "在线用户", users);
    }

    // 示例12：格式化输出
    {
        cout << "\n【示例12】格式化输出" << endl;
        cout << string(60, '-') << endl;

        auto print_table_row = [](auto... values) {
            cout << "| ";
            ((print(values), cout << " | "), ...);
            cout << endl;
        };

        cout << "表格输出：" << endl;
        print_table_row("ID", "Name", "Age");
        cout << string(40, '-') << endl;
        print_table_row(1, "Alice", 25);
        print_table_row(2, "Bob", 30);
        print_table_row(3, "Charlie", 35);
    }

    cout << "\n程序结束" << endl;
    return 0;
}
