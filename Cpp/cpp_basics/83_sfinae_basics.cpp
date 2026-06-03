// ============================================
// 83_sfinae_basics.cpp - SFINAE 基础
// 功能：学习 SFINAE、enable_if、类型萃取
// ============================================

#include <iostream>
#include <type_traits>
#include <vector>
#include <string>
using namespace std;

// ============================================
// 示例1：enable_if 基础用法
// ============================================

// 只接受整数类型
template<typename T>
typename enable_if<is_integral<T>::value, T>::type
double_value(T value) {
    return value * 2;
}

// 只接受浮点类型
template<typename T>
typename enable_if<is_floating_point<T>::value, T>::type
half_value(T value) {
    return value / 2;
}

// ============================================
// 示例2：函数重载选择
// ============================================

// 处理整数
template<typename T>
enable_if_t<is_integral<T>::value>
print_type(T value) {
    cout << "整数：" << value << endl;
}

// 处理浮点数
template<typename T>
enable_if_t<is_floating_point<T>::value>
print_type(T value) {
    cout << "浮点数：" << value << endl;
}

// 处理指针
template<typename T>
enable_if_t<is_pointer<T>::value>
print_type(T value) {
    cout << "指针：" << value << endl;
}

// 处理字符串
void print_type(const string& value) {
    cout << "字符串：" << value << endl;
}

// ============================================
// 示例3：检测成员函数是否存在
// ============================================

// 检测是否有 size() 方法
template<typename T>
class has_size {
    template<typename U>
    static auto test(int) -> decltype(declval<U>().size(), true_type{});

    template<typename>
    static false_type test(...);

public:
    static constexpr bool value = decltype(test<T>(0))::value;
};

// 检测是否有 push_back() 方法
template<typename T>
class has_push_back {
    template<typename U>
    static auto test(int) -> decltype(declval<U>().push_back(declval<typename U::value_type>()), true_type{});

    template<typename>
    static false_type test(...);

public:
    static constexpr bool value = decltype(test<T>(0))::value;
};

// ============================================
// 示例4：条件编译
// ============================================

// 如果有 size() 方法，打印大小
template<typename T>
void print_size(T& container) {
    if constexpr (has_size<T>::value) {
        cout << "容器大小：" << container.size() << endl;
    } else {
        cout << "不支持 size()" << endl;
    }
}

// ============================================
// 示例5：通用的打印函数
// ============================================

// 打印数值类型
template<typename T>
enable_if_t<is_arithmetic<T>::value>
print_value(T value) {
    cout << "数值：" << value << endl;
}

// 打印字符串
template<typename T>
enable_if_t<is_same<T, string>::value>
print_value(const T& value) {
    cout << "字符串：" << value << endl;
}

// 打印容器
template<typename T>
enable_if_t<has_size<T>::value && !is_same<T, string>::value>
print_value(const T& container) {
    cout << "容器（大小 " << container.size() << "）：[";
    bool first = true;
    for (const auto& item : container) {
        if (!first) cout << ", ";
        cout << item;
        first = false;
    }
    cout << "]" << endl;
}

// ============================================
// 示例6：enable_if 的三种使用方式
// ============================================

// 方式1：返回类型
template<typename T>
typename enable_if<is_integral<T>::value, void>::type
method1(T value) {
    cout << "方式1（返回类型）：" << value << endl;
}

// 方式2：模板参数
template<typename T, typename = enable_if_t<is_integral<T>::value>>
void method2(T value) {
    cout << "方式2（模板参数）：" << value << endl;
}

// 方式3：函数参数
template<typename T>
void method3(T value, enable_if_t<is_integral<T>::value>* = nullptr) {
    cout << "方式3（函数参数）：" << value << endl;
}

// ============================================
// 示例7：类型萃取
// ============================================

template<typename T>
void analyze_type() {
    cout << "类型分析：" << endl;
    cout << "  是整数？" << is_integral<T>::value << endl;
    cout << "  是浮点数？" << is_floating_point<T>::value << endl;
    cout << "  是指针？" << is_pointer<T>::value << endl;
    cout << "  是数组？" << is_array<T>::value << endl;
    cout << "  是类？" << is_class<T>::value << endl;
    cout << "  是 const？" << is_const<T>::value << endl;
}

// ============================================
// 示例8：条件类型选择
// ============================================

// 根据条件选择类型
template<bool Condition, typename TrueType, typename FalseType>
struct conditional_type {
    using type = TrueType;
};

template<typename TrueType, typename FalseType>
struct conditional_type<false, TrueType, FalseType> {
    using type = FalseType;
};

// 使用示例
template<typename T>
using storage_type = typename conditional_type<
    sizeof(T) <= 4,
    int,      // 小于等于4字节用 int
    long long // 大于4字节用 long long
>::type;

// ============================================
// 示例9：SFINAE 友好的函数
// ============================================

// 只对有 begin() 和 end() 的类型有效
template<typename T>
auto print_container(const T& container)
    -> decltype(container.begin(), container.end(), void()) {
    cout << "容器元素：";
    for (const auto& item : container) {
        cout << item << " ";
    }
    cout << endl;
}

// ============================================
// 示例10：检测运算符是否存在
// ============================================

// 检测是否支持 + 运算符
template<typename T>
class has_plus_operator {
    template<typename U>
    static auto test(int) -> decltype(declval<U>() + declval<U>(), true_type{});

    template<typename>
    static false_type test(...);

public:
    static constexpr bool value = decltype(test<T>(0))::value;
};

// ============================================
// 常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonErrors() {
    cout << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
❌ 错误1：enable_if 条件写反（不满足的类型通过，应排除的类型进来）

   template<typename T>
   enable_if_t<!is_integral<T>::value, T>   // ❌ ! 多了或少了，语义反转
   process(T val) { return val * 2; }

   // 本意是"只处理整数"，结果变成"只处理非整数"
   ✅ 先在注释中明确条件，再写 enable_if

------------------------------------------------------------

❌ 错误2：忘记返回类型 void 时 enable_if 的写法差异

   template<typename T>
   enable_if_t<is_integral<T>::value>        // ✅ 没有第二个参数 → 返回 void
   printInt(T val) { cout << val; }

   template<typename T>
   enable_if_t<is_integral<T>::value, T>     // ✅ 第二个参数 T → 返回 T
   doubleInt(T val) { return val * 2; }

   template<typename T>
   enable_if_t<is_integral<T>::value, void>  // ✅ 显式指定 void，与无第二参数等价
   printInt2(T val) { cout << val; }

------------------------------------------------------------

❌ 错误3：用 is_same 比较时忘记 remove_reference / remove_cv

   template<typename T>
   enable_if_t<is_same<T, int>::value>
   func(T val) { ... }

   int x = 5;
   func(x);          // ✅ T=int，通过
   const int y = 5;
   func(y);          // ❌ T=const int，is_same<const int, int> = false，不通过！

   ✅ 用 decay_t 去除 const/volatile/引用：
   enable_if_t<is_same<decay_t<T>, int>::value>

------------------------------------------------------------

❌ 错误4：enable_if 模板函数重载产生歧义

   template<typename T>
   enable_if_t<is_integral<T>::value>   process(T val) { ... }

   template<typename T>
   enable_if_t<is_arithmetic<T>::value> process(T val) { ... }

   process(5);   // ❌ 歧义：int 既满足 is_integral，也满足 is_arithmetic

   ✅ 用互斥条件：
   enable_if_t<is_integral<T>::value>
   enable_if_t<is_floating_point<T>::value>   // 整数和浮点不重叠

------------------------------------------------------------

❌ 错误5：has_xxx 类型检测中 declval 用法错误

   template<typename T>
   struct has_size {
       template<typename U>
       static auto test(int) -> decltype(U().size(), true_type{});  // ❌ U() 需要默认构造
   };

   // 如果 T 没有默认构造函数，编译错误

   ✅ 用 declval<U>() 代替 U()（不要求可构造）：
   static auto test(int) -> decltype(declval<U>().size(), true_type{});

------------------------------------------------------------

❌ 错误6：C++17 if constexpr 和 SFINAE 混用造成混乱

   template<typename T>
   void func(T val) {
       if constexpr (is_integral_v<T>) { ... }
       else if constexpr (is_floating_point_v<T>) { ... }
   }
   // ✅ C++17 中这个更简单，不需要 enable_if
   // 但不能同时用 enable_if 过滤函数签名 + if constexpr 分支

   ✅ 规则：
   - C++17 之前：用 enable_if 做分支选择（SFINAE）
   - C++17 起：优先用 if constexpr，更直观
   - enable_if 仍有用：需要精确控制重载集合时（如禁止某类型实例化）
)" << endl;
}

// ============================================
// 函数卡片速查
// ============================================

void showFunctionCards() {
    cout << string(60, '=') << endl;
    cout << "函数卡片速查" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【enable_if 三种写法】

// 返回类型中（传统写法）
template<typename T>
typename enable_if<条件, 返回类型>::type func(T val);

// C++14 简化（enable_if_t）
template<typename T>
enable_if_t<条件, 返回类型> func(T val);

// 模板参数默认值（避免修改返回类型）
template<typename T, enable_if_t<条件>* = nullptr>
返回类型 func(T val);

【常用类型萃取速查】
is_integral<T>         — int/long/char/bool 等整数
is_floating_point<T>   — float/double/long double
is_arithmetic<T>       — 整数或浮点
is_pointer<T>          — 指针类型
is_reference<T>        — 引用类型
is_same<T, U>          — 两类型完全相同
is_base_of<Base, T>    — T 是 Base 的派生类
is_constructible<T, Args...> — T 可以用 Args 构造

【类型变换速查】
remove_reference_t<T>  — 去掉引用：int& → int
remove_cv_t<T>         — 去掉 const/volatile
decay_t<T>             — 数组→指针、函数→指针、去cv去引用

【自定义类型检测模板】
template<typename T, typename = void>
struct has_method : false_type {};

template<typename T>
struct has_method<T, void_t<decltype(declval<T>().method())>> : true_type {};
)" << endl;
}

// ============================================
// 练习题
// ============================================

void showExercises() {
    cout << string(60, '=') << endl;
    cout << "练习题（独立思考）" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
1. 实现 safe_divide：
   - 整数版本：检查除数不为0，抛 runtime_error
   - 浮点版本：返回 inf（浮点除以0是合法的）
   - 用 enable_if 分别选择不同实现

2. 实现 has_toString 类型检测：
   检测 T 是否有 toString() 成员函数（返回 string）
   然后实现：
   template<typename T>
   string toStr(T val)  // 若 T 有 toString() 调用它，否则用 to_string(val)

3. 用 if constexpr（C++17）重写本文件中某个用 enable_if 实现的函数，
   对比两种写法的可读性差异。
)" << endl;
}

int main() {
    cout << "=== SFINAE 示例 ===" << endl;

    // 示例1：enable_if 基础用法
    {
        cout << "\n【示例1】enable_if 基础用法" << endl;
        cout << string(60, '-') << endl;

        cout << "double_value(10) = " << double_value(10) << endl;
        cout << "double_value(5) = " << double_value(5) << endl;

        cout << "half_value(10.0) = " << half_value(10.0) << endl;
        cout << "half_value(7.5) = " << half_value(7.5) << endl;

        // cout << double_value(3.14) << endl;  // 编译错误：不是整数类型
        // cout << half_value(10) << endl;      // 编译错误：不是浮点类型
    }

    // 示例2：函数重载选择
    {
        cout << "\n【示例2】函数重载选择" << endl;
        cout << string(60, '-') << endl;

        print_type(42);
        print_type(3.14);
        int x = 10;
        print_type(&x);
        print_type(string("Hello"));
    }

    // 示例3：检测成员函数
    {
        cout << "\n【示例3】检测成员函数" << endl;
        cout << string(60, '-') << endl;

        cout << "vector<int> 有 size()？" << has_size<vector<int>>::value << endl;
        cout << "string 有 size()？" << has_size<string>::value << endl;
        cout << "int 有 size()？" << has_size<int>::value << endl;

        cout << "\nvector<int> 有 push_back()？" << has_push_back<vector<int>>::value << endl;
        cout << "string 有 push_back()？" << has_push_back<string>::value << endl;
        cout << "int 有 push_back()？" << has_push_back<int>::value << endl;
    }

    // 示例4：条件编译
    {
        cout << "\n【示例4】条件编译" << endl;
        cout << string(60, '-') << endl;

        vector<int> vec = {1, 2, 3};
        print_size(vec);

        string str = "hello";
        print_size(str);

        int x = 10;
        print_size(x);
    }

    // 示例5：通用的打印函数
    {
        cout << "\n【示例5】通用的打印函数" << endl;
        cout << string(60, '-') << endl;

        print_value(42);
        print_value(3.14);
        print_value(string("Hello"));
        print_value(vector<int>{1, 2, 3, 4, 5});
    }

    // 示例6：enable_if 的三种使用方式
    {
        cout << "\n【示例6】enable_if 的三种使用方式" << endl;
        cout << string(60, '-') << endl;

        method1(10);
        method2(20);
        method3(30);

        // method1(3.14);  // 编译错误
        // method2(3.14);  // 编译错误
        // method3(3.14);  // 编译错误
    }

    // 示例7：类型萃取
    {
        cout << "\n【示例7】类型萃取" << endl;
        cout << string(60, '-') << endl;

        cout << "int 类型：" << endl;
        analyze_type<int>();

        cout << "\ndouble 类型：" << endl;
        analyze_type<double>();

        cout << "\nint* 类型：" << endl;
        analyze_type<int*>();

        cout << "\nstring 类型：" << endl;
        analyze_type<string>();

        cout << "\nconst int 类型：" << endl;
        analyze_type<const int>();
    }

    // 示例8：条件类型选择
    {
        cout << "\n【示例8】条件类型选择" << endl;
        cout << string(60, '-') << endl;

        cout << "char 的存储类型大小：" << sizeof(storage_type<char>) << endl;
        cout << "int 的存储类型大小：" << sizeof(storage_type<int>) << endl;
        cout << "double 的存储类型大小：" << sizeof(storage_type<double>) << endl;

        cout << "\nchar 使用 " << (sizeof(storage_type<char>) == 4 ? "int" : "long long") << endl;
        cout << "int 使用 " << (sizeof(storage_type<int>) == 4 ? "int" : "long long") << endl;
        cout << "double 使用 " << (sizeof(storage_type<double>) == 4 ? "int" : "long long") << endl;
    }

    // 示例9：SFINAE 友好的函数
    {
        cout << "\n【示例9】SFINAE 友好的函数" << endl;
        cout << string(60, '-') << endl;

        vector<int> vec = {1, 2, 3, 4, 5};
        print_container(vec);

        string str = "hello";
        print_container(str);

        // int x = 10;
        // print_container(x);  // 编译错误：int 没有 begin() 和 end()
    }

    // 示例10：检测运算符是否存在
    {
        cout << "\n【示例10】检测运算符是否存在" << endl;
        cout << string(60, '-') << endl;

        cout << "int 支持 + 运算符？" << has_plus_operator<int>::value << endl;
        cout << "string 支持 + 运算符？" << has_plus_operator<string>::value << endl;
        cout << "vector<int> 支持 + 运算符？" << has_plus_operator<vector<int>>::value << endl;
    }

    // 示例11：C++17 类型萃取简化版本
    {
        cout << "\n【示例11】C++17 类型萃取简化版本" << endl;
        cout << string(60, '-') << endl;

        cout << "使用 _v 后缀：" << endl;
        cout << "is_integral_v<int> = " << is_integral_v<int> << endl;
        cout << "is_floating_point_v<double> = " << is_floating_point_v<double> << endl;
        cout << "is_pointer_v<int*> = " << is_pointer_v<int*> << endl;

        cout << "\n等价于：" << endl;
        cout << "is_integral<int>::value = " << is_integral<int>::value << endl;
        cout << "is_floating_point<double>::value = " << is_floating_point<double>::value << endl;
        cout << "is_pointer<int*>::value = " << is_pointer<int*>::value << endl;
    }

    // 示例12：实际应用 - 通用的 swap 函数
    {
        cout << "\n【示例12】实际应用 - 通用的 swap 函数" << endl;
        cout << string(60, '-') << endl;

        // 对于基本类型，使用简单交换
        auto swap_simple = [](auto& a, auto& b) {
            if constexpr (is_arithmetic_v<decay_t<decltype(a)>>) {
                cout << "使用简单交换" << endl;
                auto temp = a;
                a = b;
                b = temp;
            } else {
                cout << "使用 std::swap" << endl;
                std::swap(a, b);
            }
        };

        int x = 10, y = 20;
        cout << "交换前：x = " << x << ", y = " << y << endl;
        swap_simple(x, y);
        cout << "交换后：x = " << x << ", y = " << y << endl;

        string s1 = "hello", s2 = "world";
        cout << "\n交换前：s1 = " << s1 << ", s2 = " << s2 << endl;
        swap_simple(s1, s2);
        cout << "交换后：s1 = " << s1 << ", s2 = " << s2 << endl;
    }

    cout << "\n程序结束" << endl;
    return 0;
}
