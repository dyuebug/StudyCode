// ============================================
// 现代 C++ 语法糖 - 超详细注释版
// 日期：2026-05-26（第23天）
// 难度：⭐⭐ 中级
// ============================================

/*
【核心概念】现代 C++ 语法糖

是什么？
- C++11/14/17 引入的简化语法特性
- 让代码更简洁、更安全、更易读

为什么需要？
- 减少重复代码（auto 自动推导类型）
- 避免错误（nullptr 替代 NULL）
- 提高可读性（范围 for 循环）

核心特点：
- auto：自动类型推导
- 范围 for：简化容器遍历
- 初始化列表：统一初始化语法
- nullptr：类型安全的空指针
*/

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
using namespace std;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

void part1_concepts() {
    cout << string(60, '=') << endl;
    cout << "第一部分：核心概念详解" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 1.1 auto 关键字 ⭐⭐⭐⭐⭐
    // --------------------------------------------

    cout << "\n1.1 auto 关键字" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【函数卡片：auto】

作用：让编译器自动推导变量类型
引入版本：C++11

完整语法：
auto 变量名 = 初始值;

推导规则：
1. 根据初始值的类型推导
2. 忽略顶层 const
3. 引用会退化为值类型

使用场景：
- 复杂类型名（迭代器、lambda）
- 类型显而易见的情况
- 模板编程中类型不确定

示例：
auto x = 10;              // int
auto y = 3.14;            // double
auto s = string("Hi");    // string
auto v = vector<int>{};   // vector<int>
)" << endl;

    // --------------------------------------------
    // 1.2 decltype 关键字
    // --------------------------------------------

    cout << "\n1.2 decltype 关键字" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【函数卡片：decltype】

作用：推导表达式的类型，但不求值
引入版本：C++11

完整语法：
decltype(表达式) 变量名;

与 auto 的区别：
- auto：根据初始值推导
- decltype：根据表达式推导，保留 const 和引用

使用场景：
- 需要保留 const 和引用
- 模板编程中推导返回类型
- 声明与另一个变量相同类型的变量

示例：
int x = 10;
decltype(x) y = 20;        // int
const int& ref = x;
decltype(ref) z = x;       // const int&
)" << endl;

    // --------------------------------------------
    // 1.3 范围 for 循环 ⭐⭐⭐⭐⭐
    // --------------------------------------------

    cout << "\n1.3 范围 for 循环（Range-based for loop）" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【函数卡片：范围 for 循环】

作用：简化容器遍历，自动迭代每个元素
引入版本：C++11

完整语法：
for (元素类型 元素变量 : 容器) {
    // 使用元素变量
}

三种形式：
1. 按值遍历：for (auto x : vec)
2. 按引用遍历：for (auto& x : vec)
3. 按 const 引用：for (const auto& x : vec)

使用场景：
- 遍历容器所有元素
- 不需要索引的情况
- 代码更简洁、更安全

示例：
vector<int> vec = {1, 2, 3};
for (const auto& x : vec) {  // 推荐：const 引用
    cout << x << " ";
}
)" << endl;

    // --------------------------------------------
    // 1.4 统一初始化列表
    // --------------------------------------------

    cout << "\n1.4 统一初始化列表（Uniform Initialization）" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【统一初始化列表】

作用：使用 {} 统一所有类型的初始化语法
引入版本：C++11

完整语法：
类型 变量名 {初始值};
类型 变量名 = {初始值};

优点：
1. 防止窄化转换（如 int 到 char）
2. 统一语法，适用于所有类型
3. 可以初始化容器

示例：
int x{10};                    // 基本类型
vector<int> vec{1, 2, 3};     // 容器
string s{"Hello"};            // 对象
int arr[]{1, 2, 3};          // 数组
)" << endl;

    // --------------------------------------------
    // 1.5 nullptr
    // --------------------------------------------

    cout << "\n1.5 nullptr" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【nullptr】

作用：类型安全的空指针常量
引入版本：C++11

为什么需要？
- NULL 是整数 0，可能导致重载歧义
- nullptr 是指针类型，类型安全

示例：
int* p1 = nullptr;  // 推荐
int* p2 = NULL;     // 旧写法，不推荐
int* p3 = 0;        // 旧写法，不推荐
)" << endl;
}

// ============================================
// 第二部分：基础操作实践
// ============================================

void part2_practice() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分：基础操作实践" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 2.1 auto 的使用
    // --------------------------------------------

    cout << "\n2.1 auto 的使用" << endl;
    cout << string(60, '-') << endl;

    // 基本类型推导
    auto x = 10;           // int
    auto y = 3.14;         // double
    auto c = 'A';          // char
    auto b = true;         // bool
    auto s = string("Hello");  // string

    cout << "x 的值: " << x << " (int)" << endl;
    cout << "y 的值: " << y << " (double)" << endl;

    // 容器类型推导
    auto vec = vector<int>{1, 2, 3, 4, 5};
    auto map_data = map<string, int>{{"Alice", 90}, {"Bob", 85}};

    cout << "vec 的大小: " << vec.size() << endl;
    cout << "map 的大小: " << map_data.size() << endl;

    // 迭代器类型推导（避免冗长的类型名）
    // 传统写法：vector<int>::iterator it = vec.begin();
    auto it = vec.begin();  // 简洁！
    cout << "第一个元素: " << *it << endl;

    // --------------------------------------------
    // 2.2 范围 for 循环的使用
    // --------------------------------------------

    cout << "\n2.2 范围 for 循环的使用" << endl;
    cout << string(60, '-') << endl;

    vector<int> numbers{10, 20, 30, 40, 50};

    // 按值遍历（拷贝元素）
    cout << "按值遍历: ";
    for (auto num : numbers) {  // 拷贝每个元素
        cout << num << " ";
    }
    cout << endl;

    // 按引用遍历（可修改元素）
    cout << "按引用遍历并修改: ";
    for (auto& num : numbers) {  // 引用，可修改
        num *= 2;  // 修改原容器的元素
    }
    for (auto num : numbers) {
        cout << num << " ";
    }
    cout << endl;

    // 按 const 引用遍历（推荐，不拷贝且不修改）
    cout << "按 const 引用遍历: ";
    for (const auto& num : numbers) {  // const 引用，不能修改
        cout << num << " ";
        // num *= 2;  // 错误！const 不能修改
    }
    cout << endl;

    // 遍历 map
    map<string, int> scores{{"Alice", 90}, {"Bob", 85}, {"Charlie", 95}};
    cout << "遍历 map: " << endl;
    for (const auto& pair : scores) {  // pair 是 pair<const string, int>
        cout << "  " << pair.first << ": " << pair.second << endl;
    }

    // 结构化绑定（C++17）
    cout << "结构化绑定（C++17）: " << endl;
    for (const auto& [name, score] : scores) {  // 直接解包
        cout << "  " << name << ": " << score << endl;
    }

    // --------------------------------------------
    // 2.3 统一初始化列表的使用
    // --------------------------------------------

    cout << "\n2.3 统一初始化列表的使用" << endl;
    cout << string(60, '-') << endl;

    // 基本类型初始化
    int a{10};
    double b{3.14};
    char c{'A'};

    // 容器初始化
    vector<int> v1{1, 2, 3, 4, 5};
    map<string, int> m1{{"Alice", 90}, {"Bob", 85}};

    cout << "vector 初始化: ";
    for (auto x : v1) cout << x << " ";
    cout << endl;

    // 数组初始化
    int arr[]{10, 20, 30};
    cout << "数组初始化: ";
    for (auto x : arr) cout << x << " ";
    cout << endl;

    // 防止窄化转换
    // int narrow{3.14};  // 错误！double 到 int 是窄化转换
    int narrow = 3.14;    // 旧语法允许，但会丢失精度
    cout << "窄化转换（旧语法）: " << narrow << endl;

    // --------------------------------------------
    // 2.4 nullptr 的使用
    // --------------------------------------------

    cout << "\n2.4 nullptr 的使用" << endl;
    cout << string(60, '-') << endl;

    int* p1 = nullptr;  // 推荐
    int* p2 = NULL;     // 旧写法
    int* p3 = 0;        // 旧写法

    if (p1 == nullptr) {
        cout << "p1 是空指针" << endl;
    }

    // nullptr 的类型安全优势
    auto checkNull = [](int* ptr) {
        cout << "接收到指针参数" << endl;
    };

    checkNull(nullptr);  // 正确
    // checkNull(NULL);  // 可能有歧义（NULL 是整数）
}

/*
==============================================
🔍 深入理解：auto 的类型推导规则（选学）
==============================================

如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【1. auto 的推导规则】

规则1：忽略顶层 const
const int x = 10;
auto y = x;  // y 是 int，不是 const int

规则2：引用会退化为值类型
int x = 10;
int& ref = x;
auto y = ref;  // y 是 int，不是 int&

规则3：保留底层 const
const int* p = nullptr;
auto q = p;  // q 是 const int*，保留底层 const

【2. 如何保留 const 和引用？】

使用 auto& 或 const auto&：
const int x = 10;
auto& y = x;        // y 是 const int&
const auto& z = x;  // z 是 const int&

【3. auto 与 decltype 的区别】

auto：根据初始值推导，忽略顶层 const 和引用
decltype：根据表达式推导，保留所有类型信息

示例：
int x = 10;
const int& ref = x;

auto a = ref;        // a 是 int
decltype(ref) b = x; // b 是 const int&

【4. 范围 for 的底层实现】

范围 for 循环：
for (auto x : vec) { ... }

等价于：
{
    auto&& __range = vec;
    auto __begin = __range.begin();
    auto __end = __range.end();
    for (; __begin != __end; ++__begin) {
        auto x = *__begin;
        ...
    }
}

【5. 初始化列表的优先级】

vector<int> v1(10, 1);   // 10 个元素，值为 1
vector<int> v2{10, 1};   // 2 个元素：10 和 1

{} 优先匹配 initializer_list 构造函数！

==============================================
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void part4_common_mistakes() {
    cout << "\n" << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 4.1 错误1：auto 丢失 const 和引用
    // --------------------------------------------

    cout << "\n4.1 错误1：auto 丢失 const 和引用" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
const int x = 10;
auto y = x;  // y 是 int，不是 const int
y = 20;      // 可以修改！

结果：丢失了 const 属性

原因：auto 忽略顶层 const

✅ 正确代码：
const int x = 10;
const auto y = x;  // y 是 const int
// y = 20;  // 错误！不能修改

或者：
auto& y = x;  // y 是 const int&（保留 const）

记忆技巧：
- 需要 const 时显式写 const auto
- 需要引用时显式写 auto&
)" << endl;

    // --------------------------------------------
    // 4.2 错误2：范围 for 中按值遍历大对象
    // --------------------------------------------

    cout << "\n4.2 错误2：范围 for 中按值遍历大对象" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
vector<string> names{"Alice", "Bob", "Charlie"};
for (auto name : names) {  // 拷贝每个 string！
    cout << name << endl;
}

结果：每次循环都拷贝 string，性能差

原因：auto 按值拷贝

✅ 正确代码：
for (const auto& name : names) {  // const 引用，不拷贝
    cout << name << endl;
}

记忆技巧：
- 遍历容器时默认使用 const auto&
- 需要修改元素时使用 auto&
- 只有基本类型（int、char）才用 auto
)" << endl;

    // --------------------------------------------
    // 4.3 错误3：范围 for 中修改容器
    // --------------------------------------------

    cout << "\n4.3 错误3：范围 for 中修改容器" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
vector<int> vec{1, 2, 3};
for (auto x : vec) {
    vec.push_back(x * 2);  // 修改容器！
}

结果：未定义行为，可能崩溃

原因：
- 范围 for 使用迭代器
- 修改容器可能导致迭代器失效

✅ 正确代码：
vector<int> vec{1, 2, 3};
vector<int> result;
for (auto x : vec) {
    result.push_back(x * 2);  // 修改另一个容器
}

记忆技巧：
- 范围 for 中不要修改正在遍历的容器
- 需要修改时使用传统 for 循环或创建新容器
)" << endl;

    // --------------------------------------------
    // 4.4 错误4：初始化列表的歧义
    // --------------------------------------------

    cout << "\n4.4 错误4：初始化列表的歧义" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
vector<int> v1{10, 1};  // 期望：10 个元素，值为 1
                        // 实际：2 个元素：10 和 1

结果：与预期不符

原因：{} 优先匹配 initializer_list 构造函数

✅ 正确代码：
vector<int> v1(10, 1);   // 10 个元素，值为 1
vector<int> v2{10, 1};   // 2 个元素：10 和 1

记忆技巧：
- () 调用普通构造函数
- {} 优先调用 initializer_list 构造函数
- 明确意图时选择合适的语法
)" << endl;

    // --------------------------------------------
    // 4.5 错误5：auto 推导出意外的类型
    // --------------------------------------------

    cout << "\n4.5 错误5：auto 推导出意外的类型" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
vector<bool> flags{true, false, true};
auto flag = flags[0];  // flag 不是 bool！
flag = false;          // 可能不会修改 flags[0]

结果：flag 是 vector<bool>::reference，不是 bool

原因：vector<bool> 是特化版本，返回代理对象

✅ 正确代码：
bool flag = flags[0];  // 显式指定类型

记忆技巧：
- 不确定类型时显式指定
- vector<bool> 是特殊情况，避免使用 auto
)" << endl;
}

// ============================================
// 第五部分：函数卡片速查
// ============================================

void part5_function_cards() {
    cout << "\n" << string(60, '=') << endl;
    cout << "函数卡片速查" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【关键字1：auto】

语法：auto 变量名 = 初始值;
作用：自动类型推导
引入版本：C++11

推导规则：
- 忽略顶层 const
- 引用退化为值类型
- 保留底层 const

常用形式：
auto x = 10;           // 值类型
auto& x = y;           // 引用
const auto& x = y;     // const 引用

────────────────────────────────────

【关键字2：decltype】

语法：decltype(表达式) 变量名;
作用：推导表达式类型，保留 const 和引用
引入版本：C++11

示例：
int x = 10;
decltype(x) y = 20;        // int
const int& ref = x;
decltype(ref) z = x;       // const int&

────────────────────────────────────

【语法3：范围 for 循环】

语法：for (类型 变量 : 容器) { ... }
作用：简化容器遍历
引入版本：C++11

三种形式：
for (auto x : vec)          // 按值（拷贝）
for (auto& x : vec)         // 按引用（可修改）
for (const auto& x : vec)   // 按 const 引用（推荐）

────────────────────────────────────

【语法4：统一初始化列表】

语法：类型 变量名{初始值};
作用：统一初始化语法，防止窄化转换
引入版本：C++11

示例：
int x{10};
vector<int> v{1, 2, 3};
int arr[]{1, 2, 3};

────────────────────────────────────

【关键字5：nullptr】

语法：类型* 指针名 = nullptr;
作用：类型安全的空指针常量
引入版本：C++11

示例：
int* p = nullptr;  // 推荐
if (p == nullptr) { ... }
)" << endl;
}

// ============================================
// 第六部分：练习题
// ============================================

void part6_exercises() {
    cout << "\n" << string(60, '=') << endl;
    cout << "练习题" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
1. 类型推导练习
   要求：预测以下代码中变量的类型
   const int x = 10;
   auto y = x;
   auto& z = x;
   const auto& w = x;
   提示：auto 的推导规则

2. 范围 for 循环改写
   要求：将以下传统 for 循环改写为范围 for
   vector<string> names = {"Alice", "Bob", "Charlie"};
   for (size_t i = 0; i < names.size(); ++i) {
       cout << names[i] << endl;
   }
   提示：使用 const auto&

3. 性能对比
   要求：对比以下两种遍历方式的性能差异
   for (auto s : vec)          // 方式1
   for (const auto& s : vec)   // 方式2
   提示：使用大对象（如 string）测试

4. 初始化列表陷阱
   要求：解释以下代码的输出
   vector<int> v1(10, 1);
   vector<int> v2{10, 1};
   cout << v1.size() << " " << v2.size() << endl;
   提示：() 和 {} 的区别

5. 实现一个函数模板
   要求：使用 auto 和范围 for 实现一个打印容器的函数
   template<typename Container>
   void print(const Container& c) {
       // 实现代码
   }
   提示：const auto& 遍历

6. nullptr 的使用
   要求：修复以下代码的问题
   void func(int x) { cout << "int" << endl; }
   void func(int* p) { cout << "pointer" << endl; }
   func(NULL);  // 调用哪个函数？
   提示：使用 nullptr

7. decltype 的应用
   要求：使用 decltype 实现一个返回类型与参数相同的函数
   template<typename T>
   ??? add(T a, T b) {
       return a + b;
   }
   提示：decltype(a + b)
)" << endl;
}

// ============================================
// 主函数
// ============================================

int main() {
    cout << "============================================" << endl;
    cout << "现代 C++ 语法糖 - 学习笔记" << endl;
    cout << "============================================" << endl;

    part1_concepts();
    part2_practice();
    part4_common_mistakes();
    part5_function_cards();
    part6_exercises();

    cout << "\n" << string(60, '=') << endl;
    cout << "学习完成！" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【总结】

核心概念：
✅ auto：自动类型推导，简化代码
✅ decltype：推导表达式类型，保留 const 和引用
✅ 范围 for：简化容器遍历
✅ 初始化列表：统一初始化语法
✅ nullptr：类型安全的空指针

使用建议：
✅ 遍历容器时使用 const auto&
✅ 迭代器类型使用 auto
✅ 需要 const 时显式写 const auto
✅ 使用 {} 初始化防止窄化转换
✅ 使用 nullptr 替代 NULL

注意事项：
⚠️ auto 会丢失顶层 const 和引用
⚠️ 范围 for 中不要修改正在遍历的容器
⚠️ {} 优先匹配 initializer_list 构造函数
⚠️ vector<bool> 不要使用 auto

下一步：
→ 学习 Lambda 表达式
→ 学习完美转发
→ 学习 constexpr 和 consteval
)" << endl;

    return 0;
}













