// ============================================
// 39_constructor_basics.cpp - 超详细注释版
// 日期：2026-03-18（第12天）
// 难度：⭐⭐ 中级
// 主题：构造函数基础 — 默认构造、带参、重载、初始化列表
// ============================================

/*
【核心概念】

是什么？
- 构造函数：对象创建时自动调用的特殊成员函数，负责初始化成员变量
- 函数名与类名完全相同，无返回类型（连 void 也不写）

为什么需要？
- 保证对象一创建就处于有效状态（避免未初始化的垃圾值）
- 可以接受参数灵活初始化，支持重载

核心特点：
- 可以重载：多个构造函数，参数列表不同
- 初始化列表：比在函数体内赋值效率更高
- const/引用成员：必须用初始化列表初始化
*/

#include <iostream>
#include <string>
using namespace std;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

/*
【函数卡片：构造函数语法】

声明：类名(参数列表);
定义：类名::类名(参数列表) : 初始化列表 { 函数体 }

初始化列表语法：
    ClassName(int a, string b) : x(a), name(b) { }
    //                           ↑ 成员初始化，比函数体内赋值效率更高

------------------------------------------------------------

【初始化列表 vs 函数体赋值】

函数体赋值（效率低）：
    Student(string n) { name = n; }   // 先默认构造 name，再赋值（两步）

初始化列表（效率高）：
    Student(string n) : name(n) { }   // 直接构造 name（一步）

必须用初始化列表的场景：
    1. const 成员变量（只能初始化，不能赋值）
    2. 引用成员变量（必须绑定到具体对象）
    3. 没有默认构造函数的成员对象
*/

// ============================================
// 第二部分：基础操作实践
// ============================================

// ---- 2.1 默认构造函数 + 带参数构造函数 ----

class Point {
    int x, y;
public:
    // 默认构造函数：无参数，初始化为原点
    Point() : x(0), y(0) {
        cout << "  [构造] Point() → (" << x << "," << y << ")" << endl;
    }

    // 带参数构造函数：指定坐标
    Point(int x, int y) : x(x), y(y) {
        cout << "  [构造] Point(" << x << "," << y << ")" << endl;
    }

    void print() const { cout << "  (" << x << ", " << y << ")" << endl; }
    ~Point() { cout << "  [析构] Point(" << x << "," << y << ")" << endl; }
};

void demoBasicConstructor() {
    cout << string(60, '=') << endl;
    cout << "第二部分 2.1：默认 + 带参数构造函数" << endl;
    cout << string(60, '=') << endl;

    cout << "\n--- 默认构造：Point p1; ---" << endl;
    Point p1;          // 调用 Point()
    p1.print();

    cout << "\n--- 带参数：Point p2(3, 4); ---" << endl;
    Point p2(3, 4);    // 调用 Point(int, int)
    p2.print();

    cout << "\n--- 多个对象，析构顺序与构造相反 ---" << endl;
    Point p3(1, 1), p4(2, 2);
    // 离开作用域：p4 先析构，p3 后析构（栈：后进先出）
}

// ---- 2.2 构造函数重载 ----

class Student {
    string name;
    int    age;
    double score;
public:
    // 重载1：默认
    Student() : name("未命名"), age(0), score(0.0) {
        cout << "  [构造1] Student()" << endl;
    }
    // 重载2：只传名字
    Student(const string& n) : name(n), age(0), score(0.0) {
        cout << "  [构造2] Student(name=" << n << ")" << endl;
    }
    // 重载3：名字+年龄
    Student(const string& n, int a) : name(n), age(a), score(0.0) {
        cout << "  [构造3] Student(name=" << n << ", age=" << a << ")" << endl;
    }
    // 重载4：全参数
    Student(const string& n, int a, double s) : name(n), age(a), score(s) {
        cout << "  [构造4] Student(name=" << n << ", age=" << a << ", score=" << s << ")" << endl;
    }

    void print() const {
        cout << "  " << name << " | 年龄:" << age << " | 成绩:" << score << endl;
    }
};

void demoOverload() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.2：构造函数重载" << endl;
    cout << string(60, '=') << endl;

    Student s1;                          // 调用重载1
    Student s2("Alice");                 // 调用重载2
    Student s3("Bob", 20);              // 调用重载3
    Student s4("Charlie", 21, 95.5);    // 调用重载4

    cout << "\n所有学生信息：" << endl;
    s1.print(); s2.print(); s3.print(); s4.print();
}

// ---- 2.3 初始化列表的必要性（const 成员）----

class Circle {
    double       radius;
    const double PI;    // const 成员：必须用初始化列表
public:
    // const 成员只能在初始化列表中初始化，不能在函数体内赋值
    Circle(double r) : radius(r), PI(3.14159) {
        // PI = 3.14159;  // ❌ 编译错误：const 成员不能赋值
        cout << "  [构造] Circle(r=" << r << ")" << endl;
    }

    double area() const { return PI * radius * radius; }
    void print() const {
        cout << "  圆：r=" << radius << "，面积=" << area() << endl;
    }
};

// ---- 2.4 默认参数构造函数 ----

class Rectangle {
    double w, h;
public:
    // 默认参数：未传参时 w=1.0，h=1.0
    Rectangle(double width = 1.0, double height = 1.0) : w(width), h(height) {
        cout << "  [构造] Rectangle(" << w << ", " << h << ")" << endl;
    }

    double area() const { return w * h; }
    void print() const {
        cout << "  矩形：" << w << "×" << h << "=" << area() << endl;
    }
};

void demoDefaultParams() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.4：默认参数构造函数" << endl;
    cout << string(60, '=') << endl;

    Rectangle r1;         // 使用默认值 (1.0, 1.0)
    Rectangle r2(3.0);    // w=3.0，h=1.0（默认）
    Rectangle r3(4.0, 5.0);  // 全部指定

    r1.print(); r2.print(); r3.print();
}

// ============================================
// 第三部分：深入理解（可选）
// ============================================

/*
🔍 深入理解：初始化列表的执行顺序

初始化列表中成员的初始化顺序，与列表中的书写顺序无关！
而是按照成员在类中的声明顺序执行。

class Foo {
    int a;    // 先声明
    int b;    // 后声明
public:
    Foo(int x) : b(x), a(b) {}  // ❌ 危险！a 先于 b 初始化
    // a 用 b 初始化，但此时 b 还是垃圾值（b 的声明在 a 之后）
};

✅ 规则：初始化列表书写顺序与成员声明顺序保持一致，避免依赖未初始化的成员

🔍 深入理解：编译器生成的默认构造函数

只有在没有定义任何构造函数时，编译器才会生成默认构造函数。
一旦定义了带参数的构造函数，编译器不再自动生成默认构造函数：

class Foo {
public:
    Foo(int x) {}   // 定义了带参构造函数
};
Foo f;   // ❌ 编译错误：没有默认构造函数了！

✅ 解决：显式定义默认构造函数，或用 = default
    Foo() = default;   // C++11：让编译器生成默认版本
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonErrors() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第四部分：常见错误和陷阱" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
❌ 错误1：构造函数写了返回类型

   void Point() { ... }    // ❌ 构造函数不能有返回类型
   int  Point() { ... }    // ❌ 同上

   ✅ 正确：无返回类型（连 void 也不写）
   Point() { ... }

------------------------------------------------------------

❌ 错误2：const 成员在函数体内赋值（编译错误）

   class Circle {
       const double PI;
   public:
       Circle(double r) {
           PI = 3.14159;  // ❌ 编译错误：const 成员不能赋值
       }
   };

   ✅ const 成员必须用初始化列表：
   Circle(double r) : PI(3.14159) { }

------------------------------------------------------------

❌ 错误3：初始化列表顺序与声明顺序不一致（依赖未初始化成员）

   class Foo {
       int a, b;   // 声明顺序：a 先，b 后
   public:
       Foo(int x) : b(x), a(b) {}  // ❌ a 先初始化，但依赖 b（此时 b 未初始化）
   };

   ✅ 初始化列表中的顺序与声明顺序一致：
   Foo(int x) : a(x), b(a) {}   // a 先声明，先初始化，b 可以依赖 a

------------------------------------------------------------

❌ 错误4：定义了带参构造函数后，无参构造函数消失

   class Foo {
   public:
       Foo(int x) {}   // 定义了带参构造函数
   };
   Foo f;   // ❌ 编译错误：没有默认构造函数！

   ✅ 显式提供默认构造函数：
   Foo() {}         // 手动定义
   Foo() = default; // C++11：让编译器生成

------------------------------------------------------------

❌ 错误5：最令人烦恼的解析（Most Vexing Parse）

   class Foo { public: Foo() {} };
   Foo f();   // ❌ 这不是创建对象！这是函数声明：返回 Foo 的函数 f

   ✅ 创建对象的正确写法：
   Foo f;         // 调用默认构造函数（不加括号）
   Foo f{};       // C++11：统一初始化语法，更清晰

------------------------------------------------------------

❌ 错误6：在构造函数中调用虚函数（行为非预期）

   class Base {
   public:
       Base() { init(); }          // ❌ 在构造时调用虚函数
       virtual void init() { cout << "Base::init" << endl; }
   };
   class Derived : public Base {
       void init() override { cout << "Derived::init" << endl; }
   };
   Derived d;  // 输出 "Base::init"（不是 Derived::init！）
   // 构造时 vtable 尚未切换到子类，虚函数调用的是基类版本

   ✅ 不在构造函数中调用虚函数
)" << endl;
}

// ============================================
// 第五部分：函数卡片速查
// ============================================

void showFunctionCards() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第五部分：函数卡片速查" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【构造函数语法速查】

无参（默认）：  ClassName() : member(init) { }
带参：          ClassName(int a, string b) : m1(a), m2(b) { }
默认参数：      ClassName(int a = 0, string b = "") : m1(a), m2(b) { }
委托构造：      ClassName() : ClassName(0, "") { }   // C++11，调用另一个构造

【必须用初始化列表的成员类型】
  const 成员变量
  引用（&）成员变量
  没有默认构造函数的成员对象

【编译器默认生成规则】
  没有定义任何构造函数 → 编译器生成默认构造（无参，啥也不做）
  定义了任何构造函数   → 编译器不再生成默认构造
  显式要求默认构造：    ClassName() = default;   // C++11
  显式禁用：            ClassName() = delete;    // C++11
)" << endl;
}

// ============================================
// 第六部分：练习题
// ============================================

void showExercises() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第六部分：练习题（独立思考）" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
1. 实现 Date 类：
   - 成员：year, month, day（int）
   - 重载3个构造函数：
     Date()            → 初始化为 2000-01-01
     Date(int y, int m, int d) → 验证 month 在 1-12，day 在 1-31
     Date(const string& s)     → 解析 "YYYY-MM-DD" 格式字符串
   - 打印：void print() → "2026-06-01"

2. 找出以下类的所有问题：
   class Config {
       const int MAX;
       int count;
       string& label;   // 引用成员
   public:
       Config(string& s) {
           MAX = 100;         // 问题①
           count = 0;         // 问题②
           label = s;         // 问题③
       }
   };

   提示：①const不能赋值；②应用初始化列表（不是错误但低效）；③引用不能赋值，必须初始化列表

3. 思考：以下两种写法有何性能差异？
   class A {
       string name;
   public:
       A(string n) { name = n; }         // 写法1
       A(const string& n) : name(n) {}   // 写法2
       A(string&& n) : name(move(n)) {}  // 写法3
   };
)" << endl;
}

// ============================================
// main 函数
// ============================================

int main() {
    cout << string(60, '=') << endl;
    cout << " 39_constructor_basics.cpp - 构造函数基础" << endl;
    cout << string(60, '=') << endl;

    demoBasicConstructor();
    demoOverload();

    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.3：const 成员与初始化列表" << endl;
    cout << string(60, '=') << endl;
    Circle c(5.0);
    c.print();

    demoDefaultParams();
    showCommonErrors();
    showFunctionCards();
    showExercises();

    cout << "\n" << string(60, '=') << endl;
    cout << "核心要点总结" << endl;
    cout << string(60, '=') << endl;
    cout << "1. 构造函数：与类同名，无返回类型，对象创建时自动调用" << endl;
    cout << "2. 初始化列表：优先于函数体赋值，const/引用成员必须用" << endl;
    cout << "3. 初始化顺序由成员声明顺序决定，与列表书写顺序无关" << endl;
    cout << "4. 定义带参构造后，默认构造函数不再自动生成" << endl;
    cout << "5. Foo f() 是函数声明，不是对象创建（最令人烦恼的解析）" << endl;

    return 0;
}
