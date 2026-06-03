// ============================================
// 阶段4复习总结 - STL和高级特性
// 日期：2026-05-27（第24天）
// 目标：系统复习，建立知识体系
// ============================================

/*
【复习目标】

本文件帮助你快速回顾阶段4的核心知识点：
1. STL 容器和算法
2. Lambda 表达式和函数对象
3. 模板编程
4. 智能指针和 RAII
5. 移动语义和现代 C++ 语法

通过系统复习，建立完整的知识体系。
*/

#include <iostream>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <algorithm>
#include <memory>
#include <string>
using namespace std;

// ============================================
// 第一部分：STL 容器和算法 ⭐⭐⭐⭐⭐
// ============================================

void part1_stl_review() {
    cout << string(60, '=') << endl;
    cout << "第一部分：STL 容器和算法复习" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 1.1 容器选型决策树
    // --------------------------------------------

    cout << R"(
【容器选型决策树】

需要随机访问？
├─ 是 → 需要频繁插入/删除？
│      ├─ 否 → vector（默认选择）
│      └─ 是 → deque（双端队列）
└─ 否 → 需要排序？
       ├─ 是 → 需要键值对？
       │      ├─ 是 → map（有序键值对）
       │      └─ 否 → set（有序集合）
       └─ 否 → 需要频繁插入/删除？
              ├─ 是 → list（双向链表）
              └─ 否 → vector

【容器特点总结】

vector：
- 底层：动态数组
- 优点：随机访问 O(1)，尾部插入 O(1)
- 缺点：中间插入/删除 O(n)
- 使用场景：默认选择，需要随机访问

list：
- 底层：双向链表
- 优点：任意位置插入/删除 O(1)
- 缺点：随机访问 O(n)，内存开销大
- 使用场景：频繁插入/删除

map：
- 底层：红黑树
- 优点：有序，查找 O(log n)
- 缺点：内存开销大
- 使用场景：需要键值对，需要排序

set：
- 底层：红黑树
- 优点：自动去重，有序，查找 O(log n)
- 缺点：不能修改元素
- 使用场景：需要去重，需要排序
)" << endl;

    // --------------------------------------------
    // 1.2 迭代器复习
    // --------------------------------------------

    cout << R"(
【迭代器类型】

1. 输入迭代器：只读，单向
2. 输出迭代器：只写，单向
3. 前向迭代器：读写，单向
4. 双向迭代器：读写，双向（list、map、set）
5. 随机访问迭代器：读写，随机访问（vector、deque）

【迭代器失效场景】

vector：
- 插入/删除：插入点之后的迭代器失效
- 扩容：所有迭代器失效

list：
- 插入：不失效
- 删除：只有被删除元素的迭代器失效

map/set：
- 插入：不失效
- 删除：只有被删除元素的迭代器失效

【最佳实践】
- 使用 const_iterator 遍历只读容器
- 使用 auto 简化迭代器类型
- 优先使用范围 for 循环
)" << endl;

    // 示例：迭代器使用
    vector<int> vec{1, 2, 3, 4, 5};

    // 传统迭代器
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        cout << *it << " ";
    }
    cout << endl;

    // 范围 for（推荐）
    for (const auto& x : vec) {
        cout << x << " ";
    }
    cout << endl;

    // --------------------------------------------
    // 1.3 常用算法复习
    // --------------------------------------------

    cout << R"(
【STL 常用算法】

排序和查找：
- sort(begin, end)：排序
- find(begin, end, value)：查找元素
- find_if(begin, end, pred)：条件查找
- binary_search(begin, end, value)：二分查找（需要有序）

统计：
- count(begin, end, value)：统计元素个数
- count_if(begin, end, pred)：条件统计

修改：
- reverse(begin, end)：反转
- transform(begin, end, out, func)：变换
- for_each(begin, end, func)：遍历执行

【算法使用要点】
- 大多数算法不修改容器大小
- 算法通过迭代器操作，与容器类型无关
- 配合 Lambda 表达式使用更灵活
)" << endl;

    // 示例：算法使用
    vector<int> nums{5, 2, 8, 1, 9};

    // 排序
    sort(nums.begin(), nums.end());

    // 查找
    auto it = find(nums.begin(), nums.end(), 8);
    if (it != nums.end()) {
        cout << "找到: " << *it << endl;
    }

    // 条件查找（Lambda）
    auto it2 = find_if(nums.begin(), nums.end(), [](int x) { return x > 5; });
    if (it2 != nums.end()) {
        cout << "第一个大于5的数: " << *it2 << endl;
    }

    // 统计
    int cnt = count_if(nums.begin(), nums.end(), [](int x) { return x % 2 == 0; });
    cout << "偶数个数: " << cnt << endl;
}

// ============================================
// 第二部分：Lambda 和模板 ⭐⭐⭐⭐
// ============================================

void part2_lambda_template_review() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分：Lambda 和模板复习" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 2.1 Lambda 表达式复习
    // --------------------------------------------

    cout << R"(
【Lambda 表达式语法】

[捕获列表](参数列表) -> 返回类型 { 函数体 }

【捕获列表】
- []：不捕获任何变量
- [=]：值捕获所有外部变量
- [&]：引用捕获所有外部变量
- [x]：值捕获变量 x
- [&x]：引用捕获变量 x
- [=, &x]：值捕获所有，引用捕获 x
- [&, x]：引用捕获所有，值捕获 x

【值捕获 vs 引用捕获】
- 值捕获：拷贝变量，Lambda 内修改不影响外部
- 引用捕获：引用变量，Lambda 内修改影响外部

【使用场景】
- 配合 STL 算法（sort、find_if、count_if）
- 简化回调函数
- 替代函数对象
)" << endl;

    // 示例：Lambda 使用
    int x = 10;
    int y = 20;

    // 值捕获
    auto f1 = [x, y]() { return x + y; };
    cout << "值捕获: " << f1() << endl;

    // 引用捕获
    auto f2 = [&x, &y]() { x++; y++; };
    f2();
    cout << "引用捕获后: x=" << x << ", y=" << y << endl;

    // 配合算法
    vector<int> nums{1, 2, 3, 4, 5};
    int threshold = 3;
    auto cnt = count_if(nums.begin(), nums.end(),
                        [threshold](int n) { return n > threshold; });
    cout << "大于 " << threshold << " 的个数: " << cnt << endl;

    // --------------------------------------------
    // 2.2 模板编程复习
    // --------------------------------------------

    cout << R"(
【函数模板】

语法：
template<typename T>
T max(T a, T b) {
    return a > b ? a : b;
}

特点：
- 编译期类型推导
- 自动实例化
- 类型安全

【类模板】

语法：
template<typename T>
class Stack {
    vector<T> data;
public:
    void push(const T& x) { data.push_back(x); }
    T pop() { T x = data.back(); data.pop_back(); return x; }
};

使用：
Stack<int> s;  // 显式指定类型

【模板要点】
- 模板定义通常放在头文件
- 模板实例化发生在编译期
- 模板参数可以有默认值
- 模板可以特化
)" << endl;

    // 示例：函数模板
    auto maxInt = [](auto a, auto b) { return a > b ? a : b; };
    cout << "max(10, 20) = " << maxInt(10, 20) << endl;
    cout << "max(3.14, 2.71) = " << maxInt(3.14, 2.71) << endl;
}

// ============================================
// 第三部分：智能指针和移动语义 ⭐⭐⭐⭐⭐
// ============================================

void part3_smart_ptr_move_review() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第三部分：智能指针和移动语义复习" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 3.1 智能指针复习
    // --------------------------------------------

    cout << R"(
【三种智能指针】

unique_ptr：
- 独占所有权
- 不能拷贝，只能移动
- 零开销，性能等同裸指针
- 使用场景：默认选择，类成员变量

shared_ptr：
- 共享所有权
- 引用计数管理
- 最后一个释放时才释放资源
- 使用场景：需要共享资源

weak_ptr：
- 弱引用，不拥有资源
- 不增加引用计数
- 打破 shared_ptr 循环引用
- 使用场景：观察者模式，缓存

【RAII 原则】
- 资源获取即初始化
- 构造函数获取资源
- 析构函数释放资源
- 利用对象生命周期自动管理资源

【最佳实践】
- 使用 make_unique/make_shared 创建
- 优先使用 unique_ptr
- 避免裸指针和 new/delete
)" << endl;

    // 示例：智能指针使用
    auto p1 = make_unique<int>(42);
    cout << "unique_ptr: " << *p1 << endl;

    auto p2 = make_shared<string>("Hello");
    auto p3 = p2;  // 共享所有权
    cout << "shared_ptr 引用计数: " << p2.use_count() << endl;

    // --------------------------------------------
    // 3.2 移动语义复习
    // --------------------------------------------

    cout << R"(
【移动语义核心概念】

左值 vs 右值：
- 左值：有名字，可取地址
- 右值：临时对象，字面量

右值引用（&&）：
- 绑定到右值
- 实现移动语义的基础

移动构造函数：
ClassName(ClassName&& other) noexcept {
    // 1. 偷走源对象的资源
    data = other.data;
    // 2. 将源对象置为安全状态
    other.data = nullptr;
}

移动赋值运算符：
ClassName& operator=(ClassName&& other) noexcept {
    if (this != &other) {
        delete[] data;           // 释放旧资源
        data = other.data;       // 偷走资源
        other.data = nullptr;    // 置空源对象
    }
    return *this;
}

【std::move】
- 将左值转换为右值引用
- 本身不移动任何东西
- 真正的移动发生在移动构造/赋值中

【五法则（Rule of Five）】
管理资源的类需要定义：
1. 析构函数
2. 拷贝构造函数
3. 拷贝赋值运算符
4. 移动构造函数
5. 移动赋值运算符

【关键要点】
- 移动函数必须标记 noexcept
- 移动后源对象处于"有效但未定义"状态
- 移动比拷贝快100倍以上
)" << endl;

    // --------------------------------------------
    // 3.3 现代 C++ 语法复习
    // --------------------------------------------

    cout << R"(
【现代 C++ 语法糖】

auto：
- 自动类型推导
- 忽略顶层 const 和引用
- 需要时显式写 const auto 或 auto&

范围 for 循环：
for (const auto& x : container) { ... }

推荐形式：
- const auto&：只读遍历（推荐）
- auto&：可修改遍历
- auto：按值遍历（仅基本类型）

统一初始化列表（{}）：
- 防止窄化转换
- 统一所有类型的初始化语法
- 优先匹配 initializer_list 构造函数

nullptr：
- 类型安全的空指针常量
- 替代 NULL 和 0

【最佳实践总结】
✅ 遍历容器使用 const auto&
✅ 迭代器类型使用 auto
✅ 返回大对象利用移动语义
✅ 使用 make_unique/make_shared
✅ 使用 nullptr 替代 NULL
✅ 移动函数标记 noexcept
✅ Lambda 配合算法使用
)" << endl;

    // 示例：现代 C++ 语法
    vector<string> names{"Alice", "Bob", "Charlie"};

    // 范围 for（推荐）
    for (const auto& name : names) {
        cout << name << " ";
    }
    cout << endl;

    // Lambda + 算法
    sort(names.begin(), names.end(),
         [](const string& a, const string& b) {
             return a.length() < b.length();
         });

    // 智能指针
    auto ptr = make_unique<vector<int>>(vector<int>{1, 2, 3});
}

// ============================================
// 第四部分：知识体系总结
// ============================================

void part4_knowledge_system() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第四部分：知识体系总结" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【阶段4知识体系图】

现代 C++ 编程
│
├─ 容器（数据结构）
│  ├─ 顺序容器：vector、list、deque
│  ├─ 关联容器：map、set
│  └─ 容器适配器：stack、queue
│
├─ 算法（操作）
│  ├─ 排序：sort、stable_sort
│  ├─ 查找：find、find_if、binary_search
│  ├─ 统计：count、count_if
│  └─ 修改：transform、for_each、reverse
│
├─ 迭代器（桥梁）
│  ├─ 类型：输入、输出、前向、双向、随机访问
│  ├─ 操作：begin、end、++、--、*
│  └─ 失效：插入、删除、扩容
│
├─ 函数式编程
│  ├─ Lambda 表达式：[捕获](参数){函数体}
│  ├─ 捕获列表：[]、[=]、[&]、[x]、[&x]
│  └─ 函数对象：仿函数、适配器
│
├─ 泛型编程
│  ├─ 函数模板：template<typename T>
│  ├─ 类模板：template<typename T> class
│  └─ 模板特化：全特化、偏特化
│
├─ 资源管理
│  ├─ RAII 原则：构造获取，析构释放
│  ├─ 智能指针：unique_ptr、shared_ptr、weak_ptr
│  └─ 异常安全：自动清理资源
│
└─ 性能优化
   ├─ 移动语义：转移所有权，避免拷贝
   ├─ 右值引用：&&、std::move
   ├─ 五法则：析构、拷贝构造/赋值、移动构造/赋值
   └─ 现代语法：auto、范围for、nullptr

【核心思想】

1. 容器 + 算法 + 迭代器 = STL 三大组件
2. Lambda 让算法使用更灵活
3. 模板实现泛型编程，类型安全
4. 智能指针自动管理内存，避免泄漏
5. 移动语义大幅提升性能
6. 现代语法让代码更简洁安全
)" << endl;
}

// ============================================
// 第五部分：常见陷阱和最佳实践
// ============================================

void part5_best_practices() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第五部分：常见陷阱和最佳实践" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【常见陷阱】

1. 迭代器失效
   ❌ 错误：插入/删除后继续使用旧迭代器
   ✅ 正确：使用返回的新迭代器

2. 范围 for 中修改容器
   ❌ 错误：for (auto x : vec) { vec.push_back(x); }
   ✅ 正确：使用传统 for 或创建新容器

3. auto 丢失 const 和引用
   ❌ 错误：auto x = const_value;  // 丢失 const
   ✅ 正确：const auto& x = const_value;

4. 移动后继续使用对象
   ❌ 错误：auto s2 = std::move(s1); cout << s1;
   ✅ 正确：移动后不再使用 s1

5. 忘记标记 noexcept
   ❌ 错误：ClassName(ClassName&& other) { ... }
   ✅ 正确：ClassName(ClassName&& other) noexcept { ... }

6. 返回局部变量的引用
   ❌ 错误：string& func() { string s; return s; }
   ✅ 正确：string func() { string s; return s; }

7. 循环引用导致内存泄漏
   ❌ 错误：两个 shared_ptr 互相持有
   ✅ 正确：使用 weak_ptr 打破循环

【最佳实践】

容器选择：
✅ 默认使用 vector
✅ 需要频繁插入/删除用 list
✅ 需要键值对用 map
✅ 需要去重用 set

迭代器使用：
✅ 优先使用范围 for 循环
✅ 只读遍历用 const_iterator
✅ 使用 auto 简化类型

算法使用：
✅ 配合 Lambda 表达式
✅ 优先使用标准算法而不是手写循环
✅ 注意算法的前提条件（如 binary_search 需要有序）

Lambda 使用：
✅ 简单逻辑用 Lambda
✅ 复杂逻辑用函数对象
✅ 注意捕获列表的生命周期

智能指针：
✅ 默认使用 unique_ptr
✅ 需要共享用 shared_ptr
✅ 使用 make_unique/make_shared
✅ 避免裸指针和 new/delete

移动语义：
✅ 返回大对象时利用移动
✅ 容器操作使用 std::move
✅ 移动函数标记 noexcept
✅ 移动后不再使用源对象

现代语法：
✅ 遍历容器用 const auto&
✅ 使用 nullptr 替代 NULL
✅ 使用 {} 初始化
✅ 使用 auto 简化类型声明
)" << endl;
}

// ============================================
// 主函数
// ============================================

int main() {
    cout << "============================================" << endl;
    cout << "阶段4复习总结 - STL和高级特性" << endl;
    cout << "============================================" << endl;

    part1_stl_review();
    part2_lambda_template_review();
    part3_smart_ptr_move_review();
    part4_knowledge_system();
    part5_best_practices();

    cout << "\n" << string(60, '=') << endl;
    cout << "复习完成！" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【阶段4学习成果】

已掌握的核心技能：
✅ STL 容器：vector、list、map、set
✅ STL 算法：sort、find、count、transform
✅ 迭代器：类型、使用、失效场景
✅ Lambda 表达式：语法、捕获列表
✅ 函数对象：仿函数、适配器
✅ 模板编程：函数模板、类模板
✅ 智能指针：unique_ptr、shared_ptr、weak_ptr
✅ RAII 原则：资源自动管理
✅ 移动语义：右值引用、移动构造/赋值
✅ 现代语法：auto、范围for、nullptr

学习统计：
📚 完成文件：75个
⏰ 学习时长：17小时
🎯 掌握技能：65个
📈 学习天数：24天

【现代 C++ 编程规范】

代码风格：
✅ 使用 const auto& 遍历容器
✅ 使用 auto 简化类型声明
✅ 使用 {} 统一初始化
✅ 使用 nullptr 替代 NULL

性能优化：
✅ 利用移动语义避免拷贝
✅ 使用 reserve 预分配容器空间
✅ 使用引用传递大对象
✅ 移动函数标记 noexcept

资源管理：
✅ 使用智能指针管理内存
✅ 遵循 RAII 原则
✅ 避免裸指针和 new/delete
✅ 使用 make_unique/make_shared

代码质量：
✅ 优先使用标准算法
✅ 配合 Lambda 简化代码
✅ 使用模板实现泛型
✅ 遵循五法则管理资源

【下一步计划】

阶段5：项目实战（第15-17周）
├─ 第15周：学生管理系统
├─ 第16周：简易图书馆管理系统
└─ 第17周：命令行游戏项目

准备工作：
✅ 复习完成，知识体系建立
✅ 掌握现代 C++ 核心特性
✅ 理解最佳实践和常见陷阱
✅ 准备好进入项目实战

【总结】

恭喜你完成阶段4的学习！

你已经掌握了现代 C++ 的核心特性：
- STL 让你高效操作数据
- Lambda 让你的代码更简洁
- 模板让你实现泛型编程
- 智能指针让你告别内存泄漏
- 移动语义让你的程序飞起来

现在，你已经具备了开发实际项目的能力！

接下来，让我们通过实战项目巩固所学知识，
将理论转化为实践，成为真正的 C++ 开发者！

加油！💪
)" << endl;

    return 0;
}










