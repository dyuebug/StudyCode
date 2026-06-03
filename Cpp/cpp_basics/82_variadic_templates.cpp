// ============================================
// 82_variadic_templates.cpp - 可变参数模板
// 功能：学习可变参数模板、参数包展开、折叠表达式
// ============================================

#include <iostream>
#include <string>
#include <tuple>
#include <vector>
#include <memory>
using namespace std;

// ============================================
// 示例1：递归展开打印
// ============================================

// 递归终止条件
void print() {
    cout << endl;
}

// 递归展开
template<typename T, typename... Args>
void print(T first, Args... rest) {
    cout << first;
    if (sizeof...(rest) > 0) {
        cout << ", ";
    }
    print(rest...);  // 递归调用
}

// ============================================
// 示例2：折叠表达式（C++17）
// ============================================

// 左折叠求和
template<typename... Args>
auto sum(Args... args) {
    return (... + args);  // ((arg1 + arg2) + arg3) + ...
}

// 右折叠求和
template<typename... Args>
auto sum_right(Args... args) {
    return (args + ...);  // arg1 + (arg2 + (arg3 + ...))
}

// 折叠表达式：逻辑与
template<typename... Args>
bool all(Args... args) {
    return (... && args);
}

// 折叠表达式：逻辑或
template<typename... Args>
bool any(Args... args) {
    return (... || args);
}

// ============================================
// 示例3：可变参数构造函数
// ============================================

template<typename... Args>
class Printer {
public:
    Printer(Args... args) {
        cout << "Printer 构造，参数个数：" << sizeof...(Args) << endl;
        cout << "参数：";
        print(args...);
    }
};

// ============================================
// 示例4：完美转发 + 可变参数
// ============================================

template<typename T, typename... Args>
unique_ptr<T> make_unique_custom(Args&&... args) {
    return unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// ============================================
// 示例5：参数包展开到容器
// ============================================

template<typename... Args>
vector<int> make_vector(Args... args) {
    return {args...};
}

// ============================================
// 示例6：递归计算参数包
// ============================================

// 递归终止条件
template<typename T>
T max_value(T value) {
    return value;
}

// 递归计算最大值
template<typename T, typename... Args>
T max_value(T first, Args... rest) {
    T rest_max = max_value(rest...);
    return first > rest_max ? first : rest_max;
}

// 递归终止条件
template<typename T>
T min_value(T value) {
    return value;
}

// 递归计算最小值
template<typename T, typename... Args>
T min_value(T first, Args... rest) {
    T rest_min = min_value(rest...);
    return first < rest_min ? first : rest_min;
}

// ============================================
// 示例7：参数包展开的多种方式
// ============================================

// 方式1：初始化列表展开
template<typename... Args>
void print_with_init_list(Args... args) {
    cout << "使用初始化列表展开：";
    int dummy[] = {(cout << args << " ", 0)...};
    (void)dummy;  // 避免未使用警告
    cout << endl;
}

// 方式2：逗号运算符展开
template<typename... Args>
void print_with_comma(Args... args) {
    cout << "使用逗号运算符展开：";
    ((cout << args << " "), ...);
    cout << endl;
}

// ============================================
// 示例8：可变参数模板类
// ============================================

// 简化版 tuple 实现
template<typename... Types>
class SimpleTuple;

// 空 tuple
template<>
class SimpleTuple<> {
public:
    SimpleTuple() {
        cout << "空 SimpleTuple 构造" << endl;
    }
};

// 递归定义
template<typename Head, typename... Tail>
class SimpleTuple<Head, Tail...> : private SimpleTuple<Tail...> {
private:
    Head head;
public:
    SimpleTuple(Head h, Tail... t) : SimpleTuple<Tail...>(t...), head(h) {
        cout << "SimpleTuple 构造，当前元素：" << head << endl;
    }

    Head getHead() const { return head; }
};

// ============================================
// 示例9：参数包的索引访问
// ============================================

// 获取第 N 个参数
template<size_t N, typename T, typename... Args>
auto get_nth(T first, Args... rest) {
    if constexpr (N == 0) {
        return first;
    } else {
        return get_nth<N - 1>(rest...);
    }
}

// ============================================
// 示例10：可变参数 Lambda
// ============================================

auto variadic_lambda = [](auto... args) {
    cout << "Lambda 参数个数：" << sizeof...(args) << endl;
    cout << "参数：";
    ((cout << args << " "), ...);
    cout << endl;
};

// ============================================
// 常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonErrors() {
    cout << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
❌ 错误1：递归展开忘写终止条件（无限递归，编译报错）

   template<typename T, typename... Args>
   void print(T first, Args... rest) {
       cout << first;
       print(rest...);   // ❌ 当 rest 为空时，print() 找不到匹配，编译错误
   }

   ✅ 必须提供零参数版本作为终止：
   void print() {}   // 终止条件：参数为空时调用这个
   template<typename T, typename... Args>
   void print(T first, Args... rest) {
       cout << first;
       print(rest...);   // ✅ 最终调用 print()
   }

   C++17 替代：用 if constexpr 在函数内部处理终止，更简洁：
   template<typename T, typename... Args>
   void print(T first, Args... rest) {
       cout << first;
       if constexpr (sizeof...(rest) > 0) print(rest...);
   }

------------------------------------------------------------

❌ 错误2：折叠表达式对空参数包报错

   template<typename... Args>
   auto sum(Args... args) { return (... + args); }

   sum();   // ❌ 编译错误：空参数包，+ 折叠表达式无初始值

   ✅ 提供初始值（带初始值的折叠）：
   auto sum(Args... args) { return (0 + ... + args); }
   // 空参数时返回 0

   注意：&& 折叠空包返回 true，|| 折叠空包返回 false（有特殊规定）

------------------------------------------------------------

❌ 错误3：参数包展开位置错误

   template<typename... Args>
   void bad(Args... args) {
       vector<int> v;
       v.push_back(args);   // ❌ 语法错误：不能直接展开到单个函数调用
   }

   ✅ 正确展开方式：
   // 方式1：初始化列表
   vector<int> v = {args...};

   // 方式2：折叠表达式（C++17）
   (v.push_back(args), ...);

   // 方式3：initializer_list 展开 trick
   int dummy[] = {(v.push_back(args), 0)...};

------------------------------------------------------------

❌ 错误4：sizeof... 运算符与 sizeof 混淆

   template<typename... Args>
   void f(Args... args) {
       cout << sizeof(args);    // ❌ 编译错误或非预期：args 是参数包，不是单个值
       cout << sizeof...(args); // ✅ 参数包中的参数数量
   }

   f(1, 2.0, "x");
   // sizeof...(args) = 3

------------------------------------------------------------

❌ 错误5：在可变参数模板中误用 std::move（移走后续使用的参数）

   template<typename... Args>
   void badForward(Args&&... args) {
       process(std::move(args)...);   // ❌ 全部 move，即使是左值也被转走
   }

   string s = "hello";
   badForward(s);   // s 被 move 走，之后 s 是空字符串

   ✅ 用 forward 保持值类别：
   process(std::forward<Args>(args)...);

------------------------------------------------------------

❌ 错误6：递归模板类的基类访问需要显式指定

   template<typename Head, typename... Tail>
   class Tuple : public Tuple<Tail...> {
       Head head;
   public:
       // 访问父类成员需要 Tuple<Tail...>::method()，不能直接 method()
       // 原因：依赖基类模板，编译器不自动查找
   };
)" << endl;
}

int main() {
    cout << "=== 可变参数模板示例 ===" << endl;

    // 示例1：递归展开打印
    {
        cout << "\n【示例1】递归展开打印" << endl;
        cout << string(60, '-') << endl;

        cout << "打印不同类型：";
        print(1, 2.5, "hello", 'c', true);

        cout << "打印整数：";
        print(1, 2, 3, 4, 5);

        cout << "打印字符串：";
        print(string("C++"), string("is"), string("awesome"));
    }

    // 示例2：折叠表达式
    {
        cout << "\n【示例2】折叠表达式" << endl;
        cout << string(60, '-') << endl;

        cout << "sum(1, 2, 3, 4, 5) = " << sum(1, 2, 3, 4, 5) << endl;
        cout << "sum(1.5, 2.5, 3.5) = " << sum(1.5, 2.5, 3.5) << endl;
        cout << "sum_right(1, 2, 3, 4, 5) = " << sum_right(1, 2, 3, 4, 5) << endl;

        cout << "\n逻辑运算：" << endl;
        cout << "all(true, true, true) = " << all(true, true, true) << endl;
        cout << "all(true, false, true) = " << all(true, false, true) << endl;
        cout << "any(false, false, true) = " << any(false, false, true) << endl;
        cout << "any(false, false, false) = " << any(false, false, false) << endl;
    }

    // 示例3：可变参数构造函数
    {
        cout << "\n【示例3】可变参数构造函数" << endl;
        cout << string(60, '-') << endl;

        Printer p1(1, 2, 3);
        Printer p2("hello", "world", "!");
        Printer p3(1, 2.5, "mixed", 'x');
    }

    // 示例4：完美转发 + 可变参数
    {
        cout << "\n【示例4】完美转发 + 可变参数" << endl;
        cout << string(60, '-') << endl;

        auto ptr1 = make_unique_custom<string>("Hello", 5, 'x');
        cout << "创建的字符串：" << *ptr1 << endl;

        auto ptr2 = make_unique_custom<vector<int>>(5, 100);
        cout << "创建的 vector 大小：" << ptr2->size() << endl;
    }

    // 示例5：参数包展开到容器
    {
        cout << "\n【示例5】参数包展开到容器" << endl;
        cout << string(60, '-') << endl;

        auto vec = make_vector(1, 2, 3, 4, 5);
        cout << "vector 元素：";
        for (int v : vec) {
            cout << v << " ";
        }
        cout << endl;

        cout << "vector 大小：" << vec.size() << endl;
    }

    // 示例6：递归计算
    {
        cout << "\n【示例6】递归计算最大值和最小值" << endl;
        cout << string(60, '-') << endl;

        cout << "max_value(3, 7, 2, 9, 1) = " << max_value(3, 7, 2, 9, 1) << endl;
        cout << "max_value(1.5, 2.8, 0.9) = " << max_value(1.5, 2.8, 0.9) << endl;

        cout << "min_value(3, 7, 2, 9, 1) = " << min_value(3, 7, 2, 9, 1) << endl;
        cout << "min_value(1.5, 2.8, 0.9) = " << min_value(1.5, 2.8, 0.9) << endl;
    }

    // 示例7：参数包展开的多种方式
    {
        cout << "\n【示例7】参数包展开的多种方式" << endl;
        cout << string(60, '-') << endl;

        print_with_init_list(1, 2, 3, 4, 5);
        print_with_comma("hello", "world", "!");
    }

    // 示例8：可变参数模板类
    {
        cout << "\n【示例8】可变参数模板类" << endl;
        cout << string(60, '-') << endl;

        SimpleTuple<int, double, string> t(42, 3.14, "hello");
        cout << "第一个元素：" << t.getHead() << endl;
    }

    // 示例9：参数包的索引访问
    {
        cout << "\n【示例9】参数包的索引访问" << endl;
        cout << string(60, '-') << endl;

        cout << "get_nth<0>(1, 2, 3, 4, 5) = " << get_nth<0>(1, 2, 3, 4, 5) << endl;
        cout << "get_nth<2>(1, 2, 3, 4, 5) = " << get_nth<2>(1, 2, 3, 4, 5) << endl;
        cout << "get_nth<4>(1, 2, 3, 4, 5) = " << get_nth<4>(1, 2, 3, 4, 5) << endl;

        cout << "get_nth<1>(\"a\", \"b\", \"c\") = " << get_nth<1>("a", "b", "c") << endl;
    }

    // 示例10：可变参数 Lambda
    {
        cout << "\n【示例10】可变参数 Lambda" << endl;
        cout << string(60, '-') << endl;

        variadic_lambda(1, 2, 3);
        variadic_lambda("hello", "world");
        variadic_lambda(1, 2.5, "mixed", 'x');
    }

    // 示例11：sizeof... 运算符
    {
        cout << "\n【示例11】sizeof... 运算符" << endl;
        cout << string(60, '-') << endl;

        auto count_args = [](auto... args) {
            return sizeof...(args);
        };

        cout << "count_args(1, 2, 3) = " << count_args(1, 2, 3) << endl;
        cout << "count_args(\"a\", \"b\", \"c\", \"d\") = "
             << count_args("a", "b", "c", "d") << endl;
    }

    // 示例12：实际应用 - 通用工厂函数
    {
        cout << "\n【示例12】实际应用 - 通用工厂函数" << endl;
        cout << string(60, '-') << endl;

        // 创建不同类型的对象
        auto s = make_unique_custom<string>("Factory");
        auto v = make_unique_custom<vector<int>>(3, 100);

        cout << "创建的 string：" << *s << endl;
        cout << "创建的 vector：";
        for (int val : *v) {
            cout << val << " ";
        }
        cout << endl;
    }

    cout << "\n程序结束" << endl;
    return 0;
}
