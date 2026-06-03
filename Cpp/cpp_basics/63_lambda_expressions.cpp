// ============================================
// 63. Lambda 表达式详解 - 超详细注释版
// 日期：2026-05-06（第20天）
// 难度：⭐⭐ 中级
// ============================================

/*
【核心概念】

什么是 Lambda 表达式？
- C++11 引入的"匿名函数"，无需单独定义函数名
- 语法：[捕获列表](参数列表) -> 返回类型 { 函数体 }
- 本质：编译器自动生成的匿名函数对象（仿函数）

为什么需要 Lambda？
- 算法需要自定义操作时，不想单独定义一个函数
- 代码更紧凑，逻辑就近表达
- 捕获外部变量比全局变量更安全

核心特点：
- 可以捕获外部变量（值捕获、引用捕获）
- 可以指定返回类型（通常可省略，编译器自动推导）
- 可以作为算法的参数（sort比较、find_if条件、for_each操作）

本文件重点：掌握 lambda 语法、捕获列表、与算法配合使用
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
    // 1.1 Lambda 是什么？—— 用函数来理解
    // --------------------------------------------

    cout << "1.1 Lambda 是什么？" << endl;
    cout << string(60, '-') << endl;

    // 传统方式：定义一个普通函数
    auto addFunc = [](int a, int b) -> int {
        return a + b;
    };
    cout << "函数方式：addFunc(3, 4) = " << addFunc(3, 4) << endl;

    // Lambda 方式：直接内联定义
    auto addLambda = [](int a, int b) { return a + b; };
    cout << "Lambda方式：addLambda(3, 4) = " << addLambda(3, 4) << endl;

    cout << R"(
【对比：普通函数 vs Lambda】

普通函数：
  int add(int a, int b) { return a + b; }
  // 需要单独定义，有函数名

Lambda：
  auto add = [](int a, int b) { return a + b; };
  // 直接内联定义，无需函数名

本质相同：
  - 都是可调用对象（callable）
  - 都有参数列表和函数体
  - 都可以赋值给变量

Lambda 的优势：
  - 代码更紧凑，逻辑就近表达
  - 可以捕获外部变量
  - 适合作为算法参数
)"" << endl;

    // --------------------------------------------
    // 1.2 Lambda 语法结构 ⭐⭐⭐⭐⭐
    // --------------------------------------------

    cout << "1.2 Lambda 语法结构" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【Lambda 完整语法】

  [捕获列表](参数列表) -> 返回类型 { 函数体 }

  ┌─────────────────────────────────────────────────┐
  │  [捕获列表](参数列表) -> 返回类型 { 函数体 }    │
  │   ↑           ↑           ↑          ↑         │
  │   │           │           │          │         │
  │  捕获外部   函数参数    返回值     执行逻辑    │
  │  变量的方式  (可选)     (可选)                  │
  └─────────────────────────────────────────────────┘

各部分详解：

1. [捕获列表] — 必需
   - 定义 lambda 如何访问外部变量
   - []       — 不捕获任何外部变量
   - [x]      — 按值捕获 x
   - [&x]     — 按引用捕获 x
   - [=]      — 按值捕获所有外部变量
   - [&]      — 按引用捕获所有外部变量
   - [=, &x]  — 默认按值捕获，x 按引用捕获

2. (参数列表) — 可选
   - 和普通函数参数一样
   - 可以省略（无参数时）
   - 示例：(int a, int b)、(int x)、()

3. -> 返回类型 — 可选
   - 通常省略，编译器自动推导
   - 复杂情况需要显式指定
   - 示例：-> int、-> double、-> string

4. { 函数体 } — 必需
   - 函数执行的代码
   - 可以有 return 语句
   - 单行表达式可以省略 {}
)"" << endl;

    // --------------------------------------------
    // 1.3 Lambda 基础用法示例
    // --------------------------------------------

    cout << "1.3 Lambda 基础用法示例" << endl;
    cout << string(60, '-') << endl;

    // 最简单的 lambda：无参数，无捕获
    auto sayHello = []() { cout << "Hello Lambda!" << endl; };
    sayHello();  // 调用 lambda

    // 带参数的 lambda
    auto square = [](int x) { return x * x; };
    cout << "5 的平方 = " << square(5) << endl;

    // 多参数的 lambda
    auto maxOf = [](int a, int b) { return a > b ? a : b; };
    cout << "max(3, 7) = " << maxOf(3, 7) << endl;

    // 立即调用的 lambda（IIFE）
    int result = [](int x) { return x * 2 + 1; }(5);
    cout << "立即调用：5 * 2 + 1 = " << result << endl;

    cout << endl;

    // --------------------------------------------
    // 1.4 捕获列表详解 ⭐⭐⭐⭐⭐
    // --------------------------------------------

    cout << "1.4 捕获列表详解" << endl;
    cout << string(60, '-') << endl;

    int x = 10;
    int y = 20;
    string name = "Alice";

    // 1. 不捕获 [] — 不能访问外部变量
    auto noCapture = []() {
        // cout << x;  // 编译错误！不能访问外部变量
        return 42;
    };
    cout << "不捕获：" << noCapture() << endl;

    // 2. 按值捕获 [x] — 复制一份，lambda 内修改不影响外部
    auto byValue = [x]() {
        // x = 100;  // 编译错误！按值捕获的变量默认是 const
        return x * 2;
    };
    cout << "按值捕获 x：" << byValue() << endl;
    cout << "外部 x 仍然是：" << x << endl;  // x 仍然是 10

    // 3. 按引用捕获 [&x] — 直接引用，lambda 内修改影响外部
    auto byRef = [&x]() {
        x = 100;  // OK！按引用捕获可以修改
        return x;
    };
    cout << "按引用捕获并修改：" << byRef() << endl;
    cout << "外部 x 变成了：" << x << endl;  // x 变成 100

    // 4. 按值捕获所有 [=] — 复制所有外部变量
    x = 10;  // 重置
    auto captureAll = [=]() {
        return x + y;  // 可以访问 x 和 y
    };
    cout << "捕获所有：" << captureAll() << endl;

    // 5. 按引用捕获所有 [&] — 引用所有外部变量
    auto captureAllRef = [&]() {
        x = 50;
        y = 60;
        return x + y;
    };
    cout << "引用所有：" << captureAllRef() << endl;
    cout << "外部 x=" << x << ", y=" << y << endl;  // x=50, y=60

    // 6. 混合捕获 [=, &x] — 默认按值，x 按引用
    x = 10;
    y = 20;
    auto mixedCapture = [=, &x]() {
        x = 999;  // x 按引用，可以修改
        // y = 888;  // 编译错误！y 按值，不能修改
        return x + y;
    };
    cout << "混合捕获：" << mixedCapture() << endl;
    cout << "外部 x=" << x << ", y=" << y << endl;  // x=999, y=20

    cout << R"(
【捕获列表速查表】

  语法          含义                          能否修改外部变量
  ───────────────────────────────────────────────────────────
  []            不捕获                        不能访问外部变量
  [x]           按值捕获 x                    不能（默认 const）
  [&x]          按引用捕获 x                  能
  [=]           按值捕获所有                  不能（默认 const）
  [&]           按引用捕获所有                能
  [=, &x]       默认按值，x 按引用            x 能，其他不能
  [&, x]        默认按引用，x 按值            x 不能，其他能
  [this]        按引用捕获 this 指针          能修改成员变量

  mutable 关键字：
  [x]() mutable { x++; }  // 允许修改按值捕获的变量（只改副本）
)"" << endl;

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
    // 2.1 Lambda + sort 自定义排序
    // --------------------------------------------

    cout << "2.1 Lambda + sort 自定义排序" << endl;
    cout << string(60, '-') << endl;

    vector<int> nums = {5, 3, 8, 1, 9, 2, 7};

    // 用 lambda 实现降序排序
    sort(nums.begin(), nums.end(), [](int a, int b) {
        return a > b;  // 降序
    });
    cout << "降序排序：";
    for (int n : nums) cout << n << " ";  // 9 8 7 5 3 2 1
    cout << endl;

    // 按绝对值排序
    vector<int> values = {-5, 3, -8, 1, 9, -2};
    sort(values.begin(), values.end(), [](int a, int b) {
        return abs(a) < abs(b);  // 按绝对值升序
    });
    cout << "按绝对值排序：";
    for (int n : values) cout << n << " ";  // 1 -2 3 -5 -8 9
    cout << endl;

    // 字符串按长度排序
    vector<string> words = {"apple", "hi", "banana", "ok", "cherry"};
    sort(words.begin(), words.end(), [](const string& a, const string& b) {
        return a.size() < b.size();  // 按长度升序
    });
    cout << "按长度排序：";
    for (const string& w : words) cout << w << " ";  // hi ok apple banana cherry
    cout << endl;

    cout << endl;

    // --------------------------------------------
    // 2.2 Lambda + find_if 条件查找
    // --------------------------------------------

    cout << "2.2 Lambda + find_if 条件查找" << endl;
    cout << string(60, '-') << endl;

    vector<int> data = {3, -1, 7, -5, 2, -8, 4};

    // 找第一个负数
    auto neg = find_if(data.begin(), data.end(), [](int n) {
        return n < 0;
    });
    if (neg != data.end()) {
        cout << "第一个负数：" << *neg << endl;  // -1
    }

    // 找第一个大于 5 的数
    auto big = find_if(data.begin(), data.end(), [](int n) {
        return n > 5;
    });
    if (big != data.end()) {
        cout << "第一个 >5 的数：" << *big << endl;  // 7
    }

    // 捕获外部变量作为条件阈值
    int threshold = 3;
    auto aboveThreshold = find_if(data.begin(), data.end(),
        [threshold](int n) { return n > threshold; }
    );
    if (aboveThreshold != data.end()) {
        cout << "第一个 >" << threshold << " 的数：" << *aboveThreshold << endl;  // 7
    }

    cout << endl;

    // --------------------------------------------
    // 2.3 Lambda + count_if 条件计数
    // --------------------------------------------

    cout << "2.3 Lambda + count_if 条件计数" << endl;
    cout << string(60, '-') << endl;

    vector<int> scores = {85, 90, 78, 92, 88, 65, 95, 72, 55};

    // 统计优秀（>=90）
    int excellent = count_if(scores.begin(), scores.end(),
        [](int s) { return s >= 90; }
    );
    cout << "优秀(>=90)：" << excellent << " 人" << endl;  // 3

    // 统计及格（>=60）
    int pass = count_if(scores.begin(), scores.end(),
        [](int s) { return s >= 60; }
    );
    cout << "及格(>=60)：" << pass << " 人" << endl;  // 8

    // 捕获外部变量作为条件
    int passingScore = 60;
    int fail = count_if(scores.begin(), scores.end(),
        [passingScore](int s) { return s < passingScore; }
    );
    cout << "不及格(<" << passingScore << ")：" << fail << " 人" << endl;  // 1

    cout << endl;

    // --------------------------------------------
    // 2.4 Lambda + for_each 遍历操作
    // --------------------------------------------

    cout << "2.4 Lambda + for_each 遍历操作" << endl;
    cout << string(60, '-') << endl;

    vector<int> nums2 = {1, 2, 3, 4, 5};

    // 对每个元素执行操作
    cout << "翻倍：";
    for_each(nums2.begin(), nums2.end(), [](int n) {
        cout << n * 2 << " ";  // 2 4 6 8 10
    });
    cout << endl;

    // 修改外部变量（按引用捕获）
    int sum = 0;
    for_each(nums2.begin(), nums2.end(), [&sum](int n) {
        sum += n;  // 累加到外部变量
    });
    cout << "总和：" << sum << endl;  // 15

    // 打印带索引的元素
    int index = 0;
    for_each(nums2.begin(), nums2.end(), [&index](int n) {
        cout << "[" << index << "]=" << n << " ";
        index++;
    });
    cout << endl;

    cout << endl;

    // --------------------------------------------
    // 2.5 Lambda + transform 转换
    // --------------------------------------------

    cout << "2.5 Lambda + transform 转换" << endl;
    cout << string(60, '-') << endl;

    vector<string> names = {"alice", "bob", "charlie"};

    // 转换为大写
    vector<string> upperNames(names.size());
    transform(names.begin(), names.end(), upperNames.begin(),
        [](string s) {
            for (char& c : s) c = toupper(c);
            return s;
        }
    );
    cout << "大写：";
    for (const string& s : upperNames) cout << s << " ";  // ALICE BOB CHARLIE
    cout << endl;

    // 添加前缀
    vector<string> prefixed(names.size());
    transform(names.begin(), names.end(), prefixed.begin(),
        [](const string& s) { return "Mr./Ms. " + s; }
    );
    cout << "添加前缀：";
    for (const string& s : prefixed) cout << s << " ";
    cout << endl;

    cout << endl;

    // --------------------------------------------
    // 2.6 存储 Lambda 变量
    // --------------------------------------------

    cout << "2.6 存储 Lambda 变量" << endl;
    cout << string(60, '-') << endl;

    // 用 auto 存储（推荐）
    auto add = [](int a, int b) { return a + b; };
    cout << "auto 存储：" << add(3, 4) << endl;

    // 用 function 存储（需要 #include <functional>）
    function<int(int, int)> multiply = [](int a, int b) { return a * b; };
    cout << "function 存储：" << multiply(3, 4) << endl;

    // function 可以存储任何可调用对象
    function<void(int)> print = [](int n) { cout << n << " "; };
    cout << "function 遍历：";
    for (int n : {1, 2, 3, 4, 5}) print(n);
    cout << endl;

    cout << endl;

    // --------------------------------------------
    // 2.7 捕获 this 指针（类中使用 lambda）
    // --------------------------------------------

    cout << "2.7 捕获 this 指针" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【在类成员函数中使用 Lambda】

class Calculator {
    int base = 100;
public:
    void test() {
        // 捕获 this，可以访问成员变量
        auto addBase = [this](int x) { return base + x; };
        cout << addBase(5);  // 105

        // C++17: [*this] 按值捕获 this（复制整个对象）
        // auto addBaseCopy = [*this](int x) { return base + x; };
    }
};

注意：
- [this] 按引用捕获（指向当前对象）
- [*this] 按值捕获（C++17，复制整个对象）
)"" << endl;

    return 0;
}

// ============================================
// 第三部分：深入理解（选学）🔍
// ============================================

/*
如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【🔍 深入理解1：Lambda 的本质 — 编译器生成的仿函数】

当你写：
  auto add = [](int a, int b) { return a + b; };

编译器会生成类似这样的代码：
  class __lambda_1 {
  public:
      int operator()(int a, int b) const {
          return a + b;
      }
  };
  auto add = __lambda_1{};

所以：
- Lambda 就是编译器帮你生成的匿名类
- 捕获的变量变成类的成员变量
- operator() 是 const 的（默认不能修改按值捕获的变量）
- 加 mutable 后 operator() 不是 const

这就是为什么 lambda 可以赋值给 auto 和 function：
- auto 推导出具体的 lambda 类型
- function 是类型擦除的包装器

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

【🔍 深入理解2：按值捕获的生命周期】

按值捕获的变量在 lambda 定义时复制：
  int x = 10;
  auto f = [x]() { return x; };  // x 被复制到 lambda 内部
  x = 20;
  cout << f();  // 输出 10（不是 20！）

按引用捕获的变量直接引用原始变量：
  int x = 10;
  auto f = [&x]() { return x; };  // f 内部引用原始 x
  x = 20;
  cout << f();  // 输出 20

危险情况 — 悬空引用：
  function<int()> createLambda() {
      int local = 42;
      return [&local]() { return local; };  // 危险！local 即将销毁
  }
  // 调用 createLambda() 返回的 lambda 是未定义行为！

安全做法：
  function<int()> createLambda() {
      int local = 42;
      return [local]() { return local; };  // OK，按值捕获
  }

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

【🔍 深入理解3：Lambda 与性能】

Lambda 通常和手写函数一样快（甚至更快）：
1. 编译器可以内联 lambda 代码
2. 没有函数调用开销
3. 捕获的变量直接访问，无需参数传递

性能对比：
  // 手写函数
  bool isEven(int n) { return n % 2 == 0; }
  count_if(v.begin(), v.end(), isEven);

  // Lambda
  count_if(v.begin(), v.end(), [](int n) { return n % 2 == 0; });

  // 性能几乎相同，lambda 可能更优（内联更容易）

建议：
- 简单操作用 lambda（更紧凑）
- 复杂逻辑用普通函数（更清晰）
- 不要担心 lambda 的性能
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

int main_errors() {
    cout << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 4.1 错误1：按值捕获后尝试修改
    // --------------------------------------------
    cout << "4.1 错误1：按值捕获后尝试修改" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
int x = 10;
auto f = [x]() {
    x = 100;  // 编译错误！按值捕获的变量默认是 const
    return x;
};

原因：按值捕获的变量在 lambda 内部是 const 的
     编译器生成的 operator() 是 const 成员函数

✅ 正确代码：
// 方法1：按引用捕获
int x = 10;
auto f = [&x]() {
    x = 100;  // OK，按引用捕获可以修改
    return x;
};

// 方法2：使用 mutable（只修改副本，不影响外部）
int x = 10;
auto f = [x]() mutable {
    x = 100;  // OK，mutable 允许修改副本
    return x;
};
// 外部 x 仍然是 10

记忆技巧：
- 按值捕获 = 只读副本（除非 mutable）
- 按引用捕获 = 可读写引用
- 想修改外部变量 → 用 [&x] 或 [&]
)"" << endl;

    // --------------------------------------------
    // 4.2 错误2：捕获局部变量的引用后返回 lambda
    // --------------------------------------------
    cout << "4.2 错误2：悬空引用（捕获局部变量引用）" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
function<int()> createLambda() {
    int local = 42;
    return [&local]() { return local; };  // 危险！
}
auto f = createLambda();
cout << f();  // 未定义行为！local 已经销毁

原因：
- local 是栈上的局部变量
- createLambda() 返回后，local 被销毁
- lambda 内部的 &local 变成悬空引用

✅ 正确代码：
function<int()> createLambda() {
    int local = 42;
    return [local]() { return local; };  // OK，按值捕获
}
auto f = createLambda();
cout << f();  // 安全输出 42

记忆技巧：
- 返回 lambda 时，不要捕获局部变量的引用
- 按值捕获是安全的
- 如果必须引用，确保被引用的对象生命周期足够长
)"" << endl;

    // --------------------------------------------
    // 4.3 错误3：忘记捕获外部变量
    // --------------------------------------------
    cout << "4.3 错误3：忘记捕获外部变量" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
int factor = 2;
auto multiply = [](int x) {
    return x * factor;  // 编译错误！factor 未被捕获
};

原因：lambda 默认不能访问外部变量
     必须在捕获列表中明确指定

✅ 正确代码：
int factor = 2;
auto multiply = [factor](int x) {  // 捕获 factor
    return x * factor;  // OK
};

// 或按引用捕获
auto multiply = [&factor](int x) {
    return x * factor;  // OK
};

记忆技巧：
- 用到外部变量 → 必须捕获
- 编译器会告诉你哪些变量需要捕获
)"" << endl;

    // --------------------------------------------
    // 4.4 错误4：Lambda 作为函数返回值类型推导失败
    // --------------------------------------------
    cout << "4.4 错误4：Lambda 返回值类型推导失败" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
auto getOperation(bool flag) {
    if (flag) {
        return [](int x) { return x + 1; };
    } else {
        return [](int x) { return x - 1; };
    }
    // 编译错误！两个 lambda 类型不同，auto 无法推导
}

原因：每个 lambda 有唯一的类型
     两个不同的 lambda 不能用 auto 统一返回

✅ 正确代码：
function<int(int)> getOperation(bool flag) {
    if (flag) {
        return [](int x) { return x + 1; };
    } else {
        return [](int x) { return x - 1; };
    }
}

记忆技巧：
- 多个 lambda 返回时，用 function 包装
- function 是类型擦除的通用可调用对象包装器
)"" << endl;

    // --------------------------------------------
    // 4.5 错误5：在循环中按值捕获循环变量
    // --------------------------------------------
    cout << "4.5 错误5：在循环中按值捕获循环变量" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码（C++11）：
vector<function<int()>> funcs;
for (int i = 0; i < 5; i++) {
    funcs.push_back([i]() { return i; });
}
// 所有 lambda 都捕获了相同的 i 值（循环结束时的值）

原因：C++11 中，lambda 在循环结束时才真正创建
     所有 lambda 捕获的是同一个 i

✅ 正确代码（C++14）：
vector<function<int()>> funcs;
for (int i = 0; i < 5; i++) {
    funcs.push_back([i = i]() { return i; });  // C++14 初始化捕获
}
// 每个 lambda 捕获不同的值

// 或者用函数包装
auto createFunc = [](int i) {
    return [i]() { return i; };
};
for (int i = 0; i < 5; i++) {
    funcs.push_back(createFunc(i));
}

记忆技巧：
- C++11 循环中按值捕获循环变量 → 所有 lambda 捕获相同值
- C++14 用初始化捕获 [i = i] 解决
- 或用函数包装，每次创建新的 lambda
)"" << endl;

    // --------------------------------------------
    // 4.6 错误6：Lambda 内使用 this 但对象已销毁
    // --------------------------------------------
    cout << "4.6 错误6：Lambda 捕获 this 后对象被销毁" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
class Button {
    string text = "Click me";
public:
    function<void()> getHandler() {
        return [this]() { cout << text; };  // 捕获 this
    }
};
function<void()> handler;
{
    Button btn;
    handler = btn.getHandler();
}  // btn 被销毁
handler();  // 未定义行为！this 指向已销毁的对象

原因：lambda 捕获的是 this 指针（指向 Button 对象）
     对象销毁后，this 变成悬空指针

✅ 正确代码：
// 方法1：确保对象生命周期足够长
Button btn;
auto handler = btn.getHandler();
handler();  // OK，btn 仍然存活

// 方法2：使用 shared_ptr 管理生命周期
// 方法3：C++17 用 [*this] 按值捕获

记忆技巧：
- 捕获 this 时，确保对象生命周期足够长
- 返回 lambda 时特别小心
)"" << endl;

    // --------------------------------------------
    // 4.7 错误7：Lambda 与 STL 容器的交互
    // --------------------------------------------
    cout << "4.7 错误7：Lambda 作为 map 的比较函数" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
auto comp = [](int a, int b) { return a > b; };
map<int, string, decltype(comp)> m(comp);  // 需要传递给构造函数

原因：map 的比较函数类型在模板参数中指定
     但 lambda 不能作为模板参数（C++20 前）

✅ 正确代码：
// 方法1：用 function
map<int, string, function<bool(int,int)>> m(
    [](int a, int b) { return a > b; }
);

// 方法2：用函数对象（仿函数）
struct Greater {
    bool operator()(int a, int b) const { return a > b; }
};
map<int, string, Greater> m;

// 方法3：C++20 支持 lambda 作为模板参数

记忆技巧：
- C++20 前，lambda 不能直接作为模板参数
- 用 function 或仿函数替代
)"" << endl;

    return 0;
}

// ============================================
// 第五部分：函数卡片速查
// ============================================

int main_reference() {
    cout << R"(
============================================
Lambda 表达式函数卡片速查
============================================

【基本语法】

  [捕获列表](参数列表) -> 返回类型 { 函数体 }

  auto f = []() { };                        // 最简单
  auto f = [](int x) { return x * 2; };     // 带参数
  auto f = [x]() { return x; };             // 按值捕获
  auto f = [&x]() { x++; };                 // 按引用捕获
  auto f = [=]() { return x + y; };         // 按值捕获所有
  auto f = [&]() { x++; y++; };             // 按引用捕获所有
  auto f = [=, &x]() { x++; return y; };    // 混合捕获
  auto f = [](int x) -> double { return x; }; // 显式返回类型

【捕获列表速查】

  []            不捕获
  [x]           按值捕获 x（只读）
  [&x]          按引用捕获 x（可读写）
  [=]           按值捕获所有（只读）
  [&]           按引用捕获所有（可读写）
  [=, &x]       默认按值，x 按引用
  [&, x]        默认按引用，x 按值
  [this]        捕获 this 指针
  [*this]       按值捕获 *this（C++17）
  [x = expr]    初始化捕获（C++14）

【与算法配合】

  // sort 自定义排序
  sort(v.begin(), v.end(), [](int a, int b) { return a > b; });

  // find_if 条件查找
  auto it = find_if(v.begin(), v.end(), [](int x) { return x > 5; });

  // count_if 条件计数
  int n = count_if(v.begin(), v.end(), [](int x) { return x % 2 == 0; });

  // for_each 遍历操作
  for_each(v.begin(), v.end(), [](int x) { cout << x; });

  // transform 转换
  transform(v.begin(), v.end(), v.begin(), [](int x) { return x * 2; });

【存储 Lambda】

  auto f = [](int x) { return x; };              // 推荐，类型精确
  function<int(int)> f = [](int x) { return x; }; // 通用包装器

【mutable 关键字】

  [x]() mutable { x++; }  // 允许修改按值捕获的变量（只改副本）
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
1. 基础 Lambda
   编写 lambda 表达式：
   a) 计算两个数的乘积
   b) 判断一个数是否为偶数
   c) 将字符串转为大写
   提示：参数列表 + 函数体

2. 捕获列表练习
   给定 int base = 100;
   a) 编写 lambda 返回 base + 参数
   b) 编写 lambda 将 base 修改为 200
   c) 解释按值捕获和按引用捕获的区别
   提示：[base] vs [&base]

3. Lambda + sort
   给定 vector<string> = {"banana", "apple", "cherry", "date"}
   a) 按字符串长度排序
   b) 按字符串逆字母顺序排序
   提示：lambda 作为 sort 的第三个参数

4. Lambda + count_if
   给定 vector<int> = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
   a) 统计偶数个数
   b) 统计大于 5 的个数
   c) 统计能被 3 整除的个数
   提示：lambda 作为 count_if 的第三个参数

5. 综合练习：成绩分析
   给定 vector<double> = {85.5, 90.0, 78.5, 92.5, 88.0, 65.0, 95.0}
   a) 用 lambda + count_if 统计各等级人数（优秀>=90，良好>=80，及格>=60）
   b) 用 lambda + find_if 找到第一个不及格的成绩
   c) 用 lambda + for_each 计算平均分
   提示：组合使用多个 lambda

6. 思考题
   为什么 lambda 可以赋值给 auto，但不能直接赋值给 function？
   什么时候必须用 function 而不能用 auto？
   提示：function 是类型擦除的包装器
)"" << endl;

    return 0;
}

// 编译命令：g++ -std=c++11 -o 63_lambda_expressions 63_lambda_expressions.cpp
// 运行方式：./63_lambda_expressions 或 63_lambda_expressions.exe
// C++版本：C++11（lambda、auto、function 需要）
