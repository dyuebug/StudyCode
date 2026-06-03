// ============================================
// 64. 函数对象与 STL 适配器 - 超详细注释版
// 日期：2026-05-06（第20天）
// 难度：⭐⭐ 中级
// ============================================

/*
【核心概念】

什么是函数对象（Function Object / Functor）？
- 重载了 operator() 的类/结构体
- 可以像函数一样被调用
- 本质是对象，但行为像函数

什么是 STL 适配器？
- 将一种可调用对象转换为另一种的工具
- 例如：bind 绑定参数，not1 取反，mem_fn 包装成员函数

为什么需要函数对象？
- C++11 之前没有 lambda，函数对象是唯一的选择
- 函数对象可以有状态（成员变量）
- 可以作为模板参数（lambda 在 C++20 前不行）

本文件重点：理解函数对象的概念，了解 STL 适配器的用法
（注：现代 C++ 中，lambda 基本取代了函数对象和适配器）
*/

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <functional>
using namespace std;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

int main() {
    cout << string(60, '=') << endl;
    cout << "第一部分：核心概念详解" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 1.1 函数对象是什么？—— 用函数来理解
    // --------------------------------------------

    cout << "1.1 函数对象是什么？" << endl;
    cout << string(60, '-') << endl;

    // 定义一个函数对象（仿函数）
    struct Add {
        int operator()(int a, int b) const {
            return a + b;
        }
    };

    // 使用函数对象
    Add add;
    cout << "函数对象调用：add(3, 4) = " << add(3, 4) << endl;

    // 也可以直接创建临时对象
    cout << "临时对象调用：Add()(3, 4) = " << Add()(3, 4) << endl;

    cout << R"(
【对比：普通函数 vs 函数对象 vs Lambda】

普通函数：
  int add(int a, int b) { return a + b; }
  // 有函数名，无状态

函数对象：
  struct Add {
      int operator()(int a, int b) const { return a + b; }
  };
  Add add;
  // 有状态（成员变量），可作为模板参数

Lambda：
  auto add = [](int a, int b) { return a + b; };
  // 匿名，简洁，可捕获外部变量

本质：
  - 函数对象 = 重载 operator() 的类
  - Lambda = 编译器生成的匿名函数对象
  - 两者本质相同，只是语法不同
)"" << endl;

    // --------------------------------------------
    // 1.2 有状态的函数对象 ⭐⭐⭐⭐⭐
    // --------------------------------------------

    cout << "1.2 有状态的函数对象" << endl;
    cout << string(60, '-') << endl;

    // 带状态的函数对象：计数器
    struct Counter {
        int count = 0;  // 状态（成员变量）

        void operator()(int n) {
            count++;  // 修改状态
            cout << "处理第 " << count << " 个元素：" << n << endl;
        }
    };

    vector<int> nums = {10, 20, 30, 40, 50};

    Counter counter;
    for_each(nums.begin(), nums.end(), counter);
    // 注意：这里 counter 是按值传递的，所以内部的 count 不会传出来

    // 用引用传递才能获取修改后的状态
    Counter counter2;
    for_each(nums.begin(), nums.end(), ref(counter2));  // std::ref 包装
    cout << "总共处理了 " << counter2.count << " 个元素" << endl;

    // 用 lambda 实现同样的功能（更简洁）
    int lambdaCount = 0;
    for_each(nums.begin(), nums.end(), [&lambdaCount](int n) {
        lambdaCount++;
        cout << "Lambda 处理第 " << lambdaCount << " 个元素：" << n << endl;
    });
    cout << "Lambda 总共处理了 " << lambdaCount << " 个元素" << endl;

    cout << endl;

    // --------------------------------------------
    // 1.3 标准库中的函数对象 ⭐⭐⭐⭐
    // --------------------------------------------

    cout << "1.3 标准库中的函数对象" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【标准库预定义的函数对象】（#include <functional>）

算术运算：
  plus<T>         a + b
  minus<T>        a - b
  multiplies<T>   a * b
  divides<T>      a / b
  modulus<T>      a % b
  negate<T>       -a

比较运算：
  equal_to<T>     a == b
  not_equal_to<T> a != b
  greater<T>      a > b
  less<T>         a < b（默认排序用的）
  greater_equal<T> a >= b
  less_equal<T>   a <= b

逻辑运算：
  logical_and<T>  a && b
  logical_or<T>   a || b
  logical_not<T>  !a

示例：
  greater<int>()(3, 5)  // 返回 false，因为 3 不大于 5
  less<int>()(3, 5)     // 返回 true，因为 3 小于 5
)"" << endl;

    // 使用标准库函数对象
    vector<int> data = {5, 3, 8, 1, 9, 2, 7};

    // 用 greater<int>() 降序排序
    sort(data.begin(), data.end(), greater<int>());
    cout << "greater 降序：";
    for (int n : data) cout << n << " ";  // 9 8 7 5 3 2 1
    cout << endl;

    // 用 less<int>() 升序排序（默认）
    sort(data.begin(), data.end(), less<int>());
    cout << "less 升序：";
    for (int n : data) cout << n << " ";  // 1 2 3 5 7 8 9
    cout << endl;

    cout << endl;

    return 0;
}

// ============================================
// 第二部分：基础操作实践
// ============================================

int main_practice() {
    cout << string(60, '=') << endl;
    cout << "第二部分：基础操作实践" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 2.1 自定义函数对象示例
    // --------------------------------------------

    cout << "2.1 自定义函数对象示例" << endl;
    cout << string(60, '-') << endl;

    // 示例1：带阈值的比较器
    struct ThresholdCompare {
        int threshold;

        ThresholdCompare(int t) : threshold(t) {}

        bool operator()(int value) const {
            return value > threshold;
        }
    };

    vector<int> scores = {85, 90, 78, 92, 88, 65, 95, 72, 55};

    // 使用函数对象
    ThresholdCompare isExcellent(90);
    int excellent = count_if(scores.begin(), scores.end(), isExcellent);
    cout << "优秀(>90)：" << excellent << " 人" << endl;

    // 也可以用 lambda（更简洁）
    int threshold = 90;
    int excellent2 = count_if(scores.begin(), scores.end(),
        [threshold](int s) { return s > threshold; }
    );
    cout << "Lambda 统计优秀：" << excellent2 << " 人" << endl;

    // 示例2：累加器
    struct Accumulator {
        int sum = 0;

        void operator()(int n) {
            sum += n;
        }
    };

    vector<int> nums = {1, 2, 3, 4, 5};
    Accumulator acc;
    for_each(nums.begin(), nums.end(), ref(acc));
    cout << "累加结果：" << acc.sum << endl;  // 15

    cout << endl;

    // --------------------------------------------
    // 2.2 bind 适配器（绑定参数）
    // --------------------------------------------

    cout << "2.2 bind 适配器（绑定参数）" << endl;
    cout << string(60, '-') << endl;

    // 普通函数
    int add(int a, int b) { return a + b; }
    int multiply(int a, int b) { return a * b; }

    // 用 bind 绑定部分参数
    using namespace std::placeholders;

    // 绑定第一个参数为 10
    auto add10 = bind(add, 10, _1);
    cout << "add10(5) = " << add10(5) << endl;  // 15

    // 绑定第二个参数为 100
    auto addTo100 = bind(add, _1, 100);
    cout << "addTo100(5) = " << addTo100(5) << endl;  // 105

    // 绑定两个参数（创建无参函数）
    auto add34 = bind(add, 3, 4);
    cout << "add34() = " << add34() << endl;  // 7

    // 交换参数顺序
    auto reverseAdd = bind(add, _2, _1);
    cout << "reverseAdd(3, 4) = " << reverseAdd(3, 4) << endl;  // 7

    // 组合使用
    auto multiplyBy2 = bind(multiply, _1, 2);
    cout << "multiplyBy2(5) = " << multiplyBy2(5) << endl;  // 10

    auto multiplyBy2AndAdd10 = bind(add, bind(multiply, _1, 2), 10);
    cout << "multiplyBy2AndAdd10(5) = " << multiplyBy2AndAdd10(5) << endl;  // 20

    cout << R"(
【bind 语法】

  bind(可调用对象, 参数...)

  占位符：
    _1  — 第一个参数
    _2  — 第二个参数
    _3  — 第三个参数
    ... 以此类推

  示例：
    bind(f, _1, _2)      — 等价于 f(a, b)
    bind(f, _2, _1)      — 交换参数：f(b, a)
    bind(f, 10, _1)      — 绑定第一个参数：f(10, a)
    bind(f, _1, 100)     — 绑定第二个参数：f(a, 100)
    bind(f, 10, 100)     — 绑定所有参数：f(10, 100)

  头文件：<functional>
)"" << endl;

    cout << endl;

    // --------------------------------------------
    // 2.3 not1 / not2 适配器（取反）
    // --------------------------------------------

    cout << "2.3 not1 / not2 适配器（取反）" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【not1 / not2】

  not1(pred)  — 对一元谓词取反
  not2(pred)  — 对二元谓词取反

  C++11 起已弃用，用 lambda 替代

  旧写法：
    count_if(v.begin(), v.end(), not1(bind2nd(less<int>(), 5)));

  新写法（lambda）：
    count_if(v.begin(), v.end(), [](int x) { return !(x < 5); });
    // 或
    count_if(v.begin(), v.end(), [](int x) { return x >= 5; });

  现代 C++ 中几乎不用 not1/not2，lambda 更清晰
)"" << endl;

    vector<int> nums = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // 统计不小于 5 的个数
    int count = count_if(nums.begin(), nums.end(),
        [](int x) { return x >= 5; }
    );
    cout << ">=5 的个数：" << count << endl;  // 6

    cout << endl;

    // --------------------------------------------
    // 2.4 mem_fn 适配器（包装成员函数）
    // --------------------------------------------

    cout << "2.4 mem_fn 适配器（包装成员函数）" << endl;
    cout << string(60, '-') << endl;

    struct Student {
        string name;
        int age;

        void print() const {
            cout << name << " (" << age << "岁)" << endl;
        }

        bool isAdult() const {
            return age >= 18;
        }
    };

    vector<Student> students = {
        {"Alice", 20},
        {"Bob", 17},
        {"Charlie", 22},
        {"Diana", 16}
    };

    // 用 mem_fn 包装成员函数
    cout << "所有学生：" << endl;
    for_each(students.begin(), students.end(), mem_fn(&Student::print));

    // 用 mem_fn 包装成员函数用于算法
    int adults = count_if(students.begin(), students.end(),
        mem_fn(&Student::isAdult)
    );
    cout << "成年学生：" << adults << " 人" << endl;  // 2

    // 用 lambda 替代（更简洁）
    cout << "Lambda 版本：" << endl;
    for_each(students.begin(), students.end(),
        [](const Student& s) { s.print(); }
    );

    int adults2 = count_if(students.begin(), students.end(),
        [](const Student& s) { return s.isAdult(); }
    );
    cout << "Lambda 统计成年：" << adults2 << " 人" << endl;

    cout << R"(
【mem_fn vs Lambda】

mem_fn：
  for_each(v.begin(), v.end(), mem_fn(&Student::print));
  // 需要 #include <functional>
  // 语法较复杂

Lambda：
  for_each(v.begin(), v.end(),
      [](const Student& s) { s.print(); }
  );
  // 更简洁，更清晰

现代 C++ 中，几乎总是用 lambda 替代 mem_fn
)"" << endl;

    cout << endl;

    // --------------------------------------------
    // 2.5 ref / cref 适配器（引用包装）
    // --------------------------------------------

    cout << "2.5 ref / cref 适配器（引用包装）" << endl;
    cout << string(60, '-') << endl;

    // 问题：算法按值传递函数对象，无法获取修改后的状态
    struct Counter {
        int count = 0;
        void operator()(int n) { count++; }
    };

    Counter c;
    vector<int> data = {1, 2, 3, 4, 5};

    // 错误：c 是按值传递的，内部 count 不会传出来
    for_each(data.begin(), data.end(), c);
    cout << "按值传递后 count：" << c.count << endl;  // 0！

    // 正确：用 ref 包装，按引用传递
    Counter c2;
    for_each(data.begin(), data.end(), ref(c2));
    cout << "ref 包装后 count：" << c2.count << endl;  // 5

    // cref 是 const 引用版本（不能修改）
    const Counter c3;
    // for_each(data.begin(), data.end(), ref(c3));  // 编译错误！不能修改 const
    // for_each(data.begin(), data.end(), cref(c3));  // OK，但 operator() 必须是 const

    cout << R"(
【ref / cref】

  ref(obj)   — 创建引用包装器，可以修改 obj
  cref(obj)  — 创建 const 引用包装器，不能修改 obj

  用途：
  - 算法按值传递时，用 ref 保持引用语义
  - 避免不必要的拷贝
  - 保持函数对象的状态

  头文件：<functional>
)"" << endl;

    cout << endl;

    return 0;
}

// ============================================
// 第三部分：深入理解（选学）🔍
// ============================================

/*
如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【🔍 深入理解1：函数对象 vs Lambda 的性能】

两者性能几乎相同：
- 函数对象：编译器知道类型，可以内联 operator()
- Lambda：编译器生成匿名类，也可以内联

函数对象的优势（历史原因）：
1. C++11 前没有 lambda，函数对象是唯一选择
2. 函数对象可以作为模板参数（C++20 前 lambda 不行）
3. 函数对象可以有复杂的构造逻辑

Lambda 的优势（现代 C++）：
1. 语法简洁，逻辑就近表达
2. 可以捕获外部变量
3. 编译器优化更容易

建议：
- 优先用 Lambda
- 需要复杂状态或作为模板参数时，用函数对象

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

【🔍 深入理解2：bind 的实现原理】

bind 返回一个未指定类型的函数对象：
  auto f = bind(add, 10, _1);
  // f 的类型是编译器生成的匿名类

这个类大致是：
  class __bind_t {
      int (*func)(int, int);
      int arg1;
      placeholder<1> ph1;
  public:
      int operator()(int x) {
          return func(arg1, x);
      }
  };

所以 bind 和 lambda 本质相同，只是语法不同。

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

【🔍 深入理解3：为什么 lambda 在 C++20 前不能作为模板参数】

模板参数需要在编译时确定类型：
  template<typename Compare>
  void sort(Compare comp);

Lambda 的类型是匿名的，无法写出类型名：
  sort([](int a, int b) { return a < b; });  // OK，模板推导
  // 但不能作为模板参数：
  map<int, string, decltype(lambda)> m;  // C++20 前不行

C++20 解决了这个问题：
  // C++20：lambda 可以作为模板参数
  auto comp = [](int a, int b) { return a > b; };
  map<int, string, decltype(comp)> m(comp);  // OK
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

int main_errors() {
    cout << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 4.1 错误1：函数对象按值传递导致状态丢失
    // --------------------------------------------
    cout << "4.1 错误1：函数对象按值传递导致状态丢失" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
struct Counter {
    int count = 0;
    void operator()(int n) { count++; }
};

Counter c;
for_each(v.begin(), v.end(), c);  // c 按值传递
cout << c.count;  // 输出 0！状态丢失

原因：for_each 按值接收函数对象
     算法内部修改的是副本，不是原始对象

✅ 正确代码：
// 方法1：用 ref 包装
Counter c;
for_each(v.begin(), v.end(), ref(c));
cout << c.count;  // 正确输出

// 方法2：用 lambda（按引用捕获）
int count = 0;
for_each(v.begin(), v.end(), [&count](int n) { count++; });
cout << count;  // 正确输出

记忆技巧：
- 算法按值传递函数对象
- 需要保持状态 → 用 ref 包装或 lambda
)"" << endl;

    // --------------------------------------------
    // 4.2 错误2：bind 占位符命名空间错误
    // --------------------------------------------
    cout << "4.2 错误2：bind 占位符命名空间错误" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
auto f = bind(add, 10, _1);  // 编译错误！_1 未定义

原因：占位符在 std::placeholders 命名空间中

✅ 正确代码：
using namespace std::placeholders;
auto f = bind(add, 10, _1);  // OK

// 或者完整命名空间
auto f = bind(add, 10, std::placeholders::_1);  // OK

记忆技巧：
- 使用 bind 前添加 using namespace std::placeholders;
- 或者用完整命名空间 std::placeholders::_1
)"" << endl;

    // --------------------------------------------
    // 4.3 错误3：bind 绑定的参数生命周期问题
    // --------------------------------------------
    cout << "4.3 错误3：bind 绑定的参数生命周期问题" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
function<int(int)> createAdder() {
    int base = 100;
    return bind(add, base, _1);  // base 按值捕获，OK
    // 但如果 base 是引用...
}
auto f = createAdder();
cout << f(5);  // 安全，base 是值拷贝

原因：bind 默认按值绑定参数
     如果需要引用，需要用 ref 包装

✅ 正确代码：
function<int(int)> createAdder() {
    int base = 100;
    return bind(add, base, _1);  // base 按值，安全
}

// 如果需要引用
int base = 100;
auto f = bind(add, ref(base), _1);
base = 200;
cout << f(5);  // 205（使用修改后的 base）

记忆技巧：
- bind 默认按值绑定
- 需要引用时用 ref 包装
- 注意绑定参数的生命周期
)"" << endl;

    // --------------------------------------------
    // 4.4 错误4：mem_fn 与虚函数
    // --------------------------------------------
    cout << "4.4 错误4：mem_fn 与虚函数" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
class Base {
public:
    virtual void print() { cout << "Base" << endl; }
};

class Derived : public Base {
public:
    void print() override { cout << "Derived" << endl; }
};

Derived d;
Base* ptr = &d;
auto f = mem_fn(&Base::print);
f(ptr);  // 输出 "Base"！不是 "Derived"

原因：mem_fn 绑定的是 Base::print 的地址
     不会触发动态绑定（虚函数机制）

✅ 正确代码：
// 直接调用虚函数
ptr->print();  // 输出 "Derived"（正确触发动态绑定）

// 或用 lambda
auto f = [](Base* ptr) { ptr->print(); };
f(ptr);  // 输出 "Derived"

记忆技巧：
- mem_fn 绑定的是静态类型，不支持动态绑定
- 需要虚函数机制时，直接调用或用 lambda
)"" << endl;

    // --------------------------------------------
    // 4.5 错误5：混用 bind 和 lambda
    // --------------------------------------------
    cout << "4.5 错误5：混用 bind 和 lambda" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码（难以阅读）：
auto f = bind(add, bind(multiply, _1, 2), 10);
// 嵌套 bind 很难理解

✅ 正确代码（用 lambda 更清晰）：
auto f = [](int x) { return multiply(x, 2) + 10; };
// 或者
auto f = [](int x) { return x * 2 + 10; };

记忆技巧：
- 简单绑定用 bind
- 复杂逻辑用 lambda
- Lambda 通常比嵌套 bind 更清晰
)"" << endl;

    // --------------------------------------------
    // 4.6 错误6：忘记 #include <functional>
    // --------------------------------------------
    cout << "4.6 错误6：忘记 #include <functional>" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

int main() {
    vector<int> v = {3, 1, 2};
    sort(v.begin(), v.end(), greater<int>());  // 编译错误！
}

原因：greater 等函数对象在 <functional> 中

✅ 正确代码：
#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>  // 添加这个头文件
using namespace std;

常用头文件对应：
  <functional>  → greater, less, bind, ref, function, mem_fn
  <algorithm>   → sort, find, count, for_each, transform

记忆技巧：
- 用标准库函数对象/适配器 → #include <functional>
)"" << endl;

    return 0;
}

// ============================================
// 第五部分：函数卡片速查
// ============================================

int main_reference() {
    cout << R"(
============================================
函数对象与适配器函数卡片速查
============================================

【函数对象定义】

struct MyFunctor {
    返回类型 operator()(参数列表) const {
        // 函数体
    }
};

MyFunctor f;
f(参数);  // 调用

【标准库函数对象】（#include <functional>）

算术：
  plus<T>         a + b
  minus<T>        a - b
  multiplies<T>   a * b
  divides<T>      a / b
  modulus<T>      a % b
  negate<T>       -a

比较：
  equal_to<T>     a == b
  not_equal_to<T> a != b
  greater<T>      a > b
  less<T>         a < b
  greater_equal<T> a >= b
  less_equal<T>   a <= b

逻辑：
  logical_and<T>  a && b
  logical_or<T>   a || b
  logical_not<T>  !a

【bind 适配器】

using namespace std::placeholders;

bind(f, _1, _2)      — 等价于 f(a, b)
bind(f, _2, _1)      — 交换参数
bind(f, 10, _1)      — 绑定第一个参数
bind(f, _1, 100)     — 绑定第二个参数
bind(f, 10, 100)     — 绑定所有参数

【mem_fn 适配器】

mem_fn(&Class::method)  — 包装成员函数
mem_fn(&Class::member)  — 包装成员变量访问

【ref / cref】

ref(obj)   — 引用包装器（可修改）
cref(obj)  — const 引用包装器（只读）

【function 包装器】

function<返回类型(参数类型...)> f = 可调用对象;
// 可以存储函数、函数对象、lambda
)"" << endl;

    return 0;
}

// ============================================
// 第六部分：练习题
// ============================================

int main_exercises() {
    cout << string(60, '=') << endl;
    cout << "练习题" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
1. 函数对象练习
   编写一个函数对象 IsDivisible，接受一个除数 n，
   判断传入的数是否能被 n 整除。
   示例：IsDivisible(3)(9) 返回 true
         IsDivisible(3)(10) 返回 false
   提示：构造函数保存 n，operator() 判断整除

2. bind 练习
   给定函数 int power(int base, int exp) { return pow(base, exp); }
   a) 用 bind 创建 square = power(_, 2)
   b) 用 bind 创建 cube = power(_, 3)
   c) 用 bind 创建 powerOf2 = power(2, _)
   提示：using namespace std::placeholders;

3. ref 练习
   编写一个函数对象 Accumulator，重载 operator()(int)，
   累加传入的值到成员变量 sum。
   在 for_each 中使用 ref(acc) 保持状态。
   提示：用 std::ref 包装

4. 综合练习
   给定 vector<Student>，Student 有 name 和 score 成员。
   a) 用 mem_fn 打印所有学生
   b) 用 bind + count_if 统计分数大于 80 的学生
   c) 用 lambda 实现同样的功能
   提示：对比 mem_fn/bind 和 lambda 的写法

5. 思考题
   为什么现代 C++ 推荐用 lambda 而不是函数对象和 bind？
   什么时候函数对象仍然比 lambda 更合适？
   提示：考虑可读性、灵活性、模板参数
)"" << endl;

    return 0;
}

// 编译命令：g++ -std=c++11 -o 64_functors_adapters 64_functors_adapters.cpp
// 运行方式：./64_functors_adapters 或 64_functors_adapters.exe
// C++版本：C++11（bind、ref、function 需要）
