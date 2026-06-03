// ============================================
// 46_inheritance_constructor.cpp - 超详细注释版
// 日期：第14天
// 难度：⭐⭐ 中级
// 主题：继承中的构造函数与析构函数
// ============================================

/*
【核心概念】

是什么？
- 继承时，父类和子类各有自己的构造函数和析构函数
- 子类构造函数必须负责调用父类构造函数来初始化继承的成员

为什么需要了解顺序？
- 构造/析构顺序固定，违背这个顺序会导致未初始化访问或资源泄漏
- 调试时看日志需要知道顺序

核心特点：
- 构造顺序：父类 → 成员对象（声明顺序）→ 子类
- 析构顺序：子类 → 成员对象（逆序）→ 父类（与构造完全相反）
- 子类通过初始化列表调用父类构造函数
*/

#include <iostream>
#include <string>
using namespace std;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

/*
【函数卡片：初始化列表调用父类构造函数】

语法：
    子类名(参数列表) : 父类名(父类参数), 成员变量(初始值) { 函数体 }

参数：
    - 父类名(父类参数): 显式调用父类的某个构造函数
    - 成员变量(初始值): 初始化子类自己的成员

规则：
    - 若省略父类调用，编译器自动调用父类无参构造函数（必须存在）
    - 初始化列表中的初始化顺序由类中声明顺序决定（不是列表顺序）
    - 构造函数体执行时，所有成员已经过初始化列表初始化

【构造/析构完整顺序】

构造顺序（以 Car : public Vehicle 为例，Car 有成员 Engine、Wheel）：
    1. Vehicle 构造函数（最顶层父类优先）
    2. Engine 构造函数（成员，按声明顺序）
    3. Wheel 构造函数（成员，按声明顺序）
    4. Car 构造函数体

析构顺序（完全相反）：
    1. Car 析构函数体
    2. Wheel 析构（成员，逆序）
    3. Engine 析构（成员，逆序）
    4. Vehicle 析构函数
*/

// ============================================
// 第二部分：基础操作实践
// ============================================

// ---- 2.1 基本构造/析构顺序 ----

class Base {
public:
    string name;  // 记录对象名，方便追踪顺序

    // 父类构造函数
    // 参数：n - 对象名称
    Base(const string& n) : name(n) {
        cout << "[构造] Base(" << name << ")" << endl;
    }

    // 父类析构函数
    ~Base() {
        cout << "[析构] ~Base(" << name << ")" << endl;
    }
};

class Derived : public Base {
public:
    int value;  // 子类自己的成员

    // 子类构造函数：必须在初始化列表中调用父类构造函数
    // 参数：n - 传给父类的名称，v - 子类自己的值
    Derived(const string& n, int v)
        : Base(n),    // ← 调用父类构造函数（必须！否则 name 未初始化）
          value(v)    // ← 初始化子类成员
    {
        // 此时 Base 已构造完毕，name 和 value 都已可用
        cout << "[构造] Derived(" << name << ", " << value << ")" << endl;
    }

    ~Derived() {
        // 先执行 Derived 析构体，再自动调用 Base 析构
        cout << "[析构] ~Derived(" << name << ", " << value << ")" << endl;
    }
};

void demoBasicOrder() {
    cout << string(60, '=') << endl;
    cout << "第二部分 2.1：基本构造/析构顺序" << endl;
    cout << string(60, '=') << endl;

    cout << "\n--- 创建 Derived 对象（注意：父类先构造）---" << endl;
    {
        Derived d("测试对象", 42);
        cout << "--- 对象正在使用中 ---" << endl;
    }  // 离开作用域 → 先析构 Derived，再析构 Base

    cout << "\n结论：构造顺序 Base→Derived，析构顺序 Derived→Base（相反）" << endl;
}

// ---- 2.2 多级继承的构造/析构顺序 ----

class A {
public:
    A()  { cout << "[构造] A（最顶层）" << endl; }
    ~A() { cout << "[析构] ~A（最顶层）" << endl; }
};

class B : public A {
public:
    B()  { cout << "[构造] B（中间层）" << endl; }
    ~B() { cout << "[析构] ~B（中间层）" << endl; }
};

class C : public B {
public:
    C()  { cout << "[构造] C（最底层）" << endl; }
    ~C() { cout << "[析构] ~C（最底层）" << endl; }
};

void demoMultiLevel() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.2：多级继承顺序（A ← B ← C）" << endl;
    cout << string(60, '=') << endl;

    cout << "\n--- 创建 C 对象（三级继承）---" << endl;
    {
        C c;
        cout << "--- C 对象使用中 ---" << endl;
    }
    cout << "\n规律：构造从最顶层开始，析构从最底层开始" << endl;
}

// ---- 2.3 调用父类指定构造函数 ----

class Person {
protected:
    string name;  // protected：子类内部可直接访问
    int    age;

public:
    // 无参构造函数（默认）
    Person() : name("未知"), age(0) {
        cout << "[构造] Person()（无参）" << endl;
    }

    // 带参构造函数
    // 参数：n - 姓名，a - 年龄
    Person(const string& n, int a) : name(n), age(a) {
        cout << "[构造] Person(" << name << ", " << age << ")" << endl;
    }

    ~Person() {
        cout << "[析构] ~Person(" << name << ")" << endl;
    }

    virtual void info() const {
        cout << "姓名：" << name << "，年龄：" << age << endl;
    }
};

class Student : public Person {
protected:
    string school;
    double gpa;

public:
    // 调用父类带参构造函数（传入姓名和年龄）
    // 参数：n - 姓名，a - 年龄，s - 学校，g - GPA
    Student(const string& n, int a, const string& s, double g)
        : Person(n, a),  // ← 调用 Person(string, int)
          school(s),
          gpa(g)
    {
        cout << "[构造] Student(" << name << ", " << school << ")" << endl;
    }

    // 调用父类无参构造函数（不写则自动调用）
    Student(const string& s, double g)
        : Person(),     // ← 显式调用无参版本（也可以省略不写）
          school(s), gpa(g)
    {
        cout << "[构造] Student(school=" << school << "，姓名默认）" << endl;
    }

    ~Student() {
        cout << "[析构] ~Student(" << name << ")" << endl;
    }

    void info() const override {
        cout << "学生：" << name << "，年龄：" << age
             << "，学校：" << school << "，GPA：" << gpa << endl;
    }
};

// 三级继承：GradStudent → Student → Person
class GradStudent : public Student {
private:
    string major;  // 专业

public:
    // 调用 Student 的构造函数，Student 再调用 Person 的
    // 参数：n,a - 传给 Person；s,g - 传给 Student；m - GradStudent 自己的
    GradStudent(const string& n, int a, const string& s,
                double g, const string& m)
        : Student(n, a, s, g),  // ← 调用 Student(string, int, string, double)
          major(m)
    {
        cout << "[构造] GradStudent(" << name << ", 专业:" << major << ")" << endl;
    }

    ~GradStudent() {
        cout << "[析构] ~GradStudent(" << name << ")" << endl;
    }

    void info() const override {
        cout << "研究生：" << name << "，年龄：" << age
             << "，学校：" << school << "，专业：" << major
             << "，GPA：" << gpa << endl;
    }
};

void demoConstructorChain() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.3：构造函数调用链" << endl;
    cout << string(60, '=') << endl;

    cout << "\n--- 创建 Student（Person←Student 两级）---" << endl;
    {
        Student stu("张三", 20, "清华大学", 3.8);
        stu.info();
    }

    cout << "\n--- 创建 GradStudent（Person←Student←GradStudent 三级）---" << endl;
    {
        GradStudent gs("李四", 25, "北京大学", 3.9, "计算机科学");
        gs.info();
        cout << "--- 离开作用域，逆序析构 ---" << endl;
    }
}

// ---- 2.4 含成员对象的完整构造顺序 ----

class Engine {
public:
    Engine()  { cout << "[构造] Engine（成员对象1）" << endl; }
    ~Engine() { cout << "[析构] ~Engine（成员对象1）" << endl; }
};

class Wheel {
public:
    Wheel()  { cout << "[构造] Wheel（成员对象2）" << endl; }
    ~Wheel() { cout << "[析构] ~Wheel（成员对象2）" << endl; }
};

class Vehicle {
public:
    Vehicle()  { cout << "[构造] Vehicle（父类）" << endl; }
    ~Vehicle() { cout << "[析构] ~Vehicle（父类）" << endl; }
};

// Car 继承 Vehicle，并包含 Engine 和 Wheel 成员对象
class Car : public Vehicle {
private:
    Engine engine;  // 成员对象1（声明在前，先构造）
    Wheel  wheel;   // 成员对象2（声明在后，后构造）

public:
    Car()  { cout << "[构造] Car（自身体）" << endl; }
    ~Car() { cout << "[析构] ~Car（自身体）" << endl; }
    // 析构后自动逆序析构成员，再析构父类
};

void demoMemberOrder() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.4：含成员对象的完整构造顺序" << endl;
    cout << string(60, '=') << endl;

    cout << "\n完整顺序：父类Vehicle → 成员Engine → 成员Wheel → Car自身体" << endl;
    cout << "析构顺序：Car自身体 → Wheel → Engine → Vehicle（完全相反）" << endl;
    {
        Car car;
        cout << "--- Car 使用中 ---" << endl;
    }
}

// ============================================
// 🔍 第三部分：深入理解（可选）
// ============================================

/*
【深入理解：为什么析构顺序和构造顺序相反？】

以 Car 为例：
    构造时：先构造 Vehicle，再构造 Engine（可能依赖 Vehicle）
            最后构造 Car（可能依赖 Engine 和 Vehicle）
    析构时：先析构 Car（可能持有对 Engine 的引用）
            再析构 Engine（此时 Car 已析构，不再引用 Engine）
            最后析构 Vehicle

如果反过来（先析构 Vehicle）：
    Car 析构时可能还在用 Vehicle 的数据 → 未定义行为！

这个顺序确保了：析构时，"依赖者"先于"被依赖者"析构，
资源在有引用时不被释放，所有引用者析构后再释放资源。

【深入理解：初始化列表的执行顺序】

初始化列表中的初始化顺序，由成员在类中的声明顺序决定，
而不是列表中写的顺序！

class Foo {
    int a;
    int b;
public:
    Foo() : b(1), a(b) {}  // ← 虽然列表里 b 在前，但实际先初始化 a！
    // 因为 a 声明在 b 之前，a 先初始化时 b 还未初始化 → a = 垃圾值！
};

正确写法：Foo() : a(0), b(1) {}  或  调整声明顺序
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonErrors() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第四部分：常见错误和陷阱" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
❌ 错误1：子类构造函数中直接给父类 private 成员赋值

   class Person {
   private:
       string name;  // private
   };
   class Student : public Person {
   public:
       Student(string n) {
           name = n;   // ❌ 编译错误！private 子类不可访问
       }
   };

   ✅ 正确：通过初始化列表调用父类构造函数
   Student(string n) : Person(n) { }   // 父类构造函数负责初始化 name

------------------------------------------------------------

❌ 错误2：父类没有无参构造函数，子类却不调用父类构造函数

   class Animal {
   public:
       Animal(string name) { ... }  // 只有带参构造，没有无参构造
   };
   class Dog : public Animal {
   public:
       Dog() { ... }  // ❌ 编译错误！编译器找不到 Animal() 来自动调用
   };

   ✅ 正确：显式调用父类带参构造函数
   Dog(string name) : Animal(name) { }  // ✅

------------------------------------------------------------

❌ 错误3：误以为初始化列表的顺序决定初始化顺序

   class Rect {
       int width;   // 声明顺序1
       int height;  // 声明顺序2
   public:
       Rect() : height(10), width(height) { }  // ❌ 危险！
       // 虽然列表里 height 在前，但实际按声明顺序：
       // 1. 先初始化 width = height（此时 height 未初始化！垃圾值）
       // 2. 再初始化 height = 10
   };

   ✅ 正确：初始化列表顺序应与声明顺序一致
   Rect() : width(0), height(10) { }  // 独立初始化，不相互依赖

------------------------------------------------------------

❌ 错误4：在子类构造函数体内试图初始化 const 或引用成员

   class Foo {
       const int id;    // const 必须在初始化列表中初始化
       int& ref;        // 引用也必须在初始化列表中初始化
   public:
       Foo(int i, int& r) {
           id  = i;   // ❌ 编译错误：const 成员不能在函数体赋值
           ref = r;   // ❌ 编译错误：引用不能重新绑定
       }
   };

   ✅ 正确：const 和引用成员必须用初始化列表
   Foo(int i, int& r) : id(i), ref(r) { }  // ✅

------------------------------------------------------------

❌ 错误5：误以为子类对象只有子类析构函数被调用

   class Base { public: ~Base() { cout << "Base析构"; } };
   class Derived : public Base {
   public: ~Derived() { cout << "Derived析构"; } };

   Derived d;
   // 错误理解：以为只调用 ~Derived()
   // 实际：先调用 ~Derived()，再自动调用 ~Base()

   ✅ 正确理解：析构函数会自动链式调用，从最底层到最顶层

------------------------------------------------------------

❌ 错误6：多级继承中跳层调用构造函数

   class C : public B {
   public:
       C() : A() { }   // ❌ 不能直接调用 A（祖父类）的构造函数
                       // 只能调用直接父类 B 的构造函数
   };

   ✅ 正确：每层只负责调用直接父类的构造函数
   C() : B() { }    // C 调用 B，B 的初始化列表再调用 A

------------------------------------------------------------

❌ 错误7：在构造函数体内调用虚函数期望多态行为

   class Base {
   public:
       Base() { init(); }       // ← 在构造函数中调用 virtual 函数
       virtual void init() { cout << "Base::init" << endl; }
   };
   class Derived : public Base {
   public:
       void init() override { cout << "Derived::init" << endl; }
   };

   Derived d;  // 输出 "Base::init"，不是 "Derived::init"！
   // 原因：构造 Base 时，Derived 部分还未初始化，
   //       虚函数表还未指向 Derived，调用的仍是 Base 版本

   ✅ 建议：避免在构造/析构函数中调用虚函数
)";
}

// ============================================
// 第五部分：函数卡片速查
// ============================================

void showFunctionCards() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第五部分：函数卡片速查" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【调用父类构造函数语法】

语法：子类名(参数) : 父类名(父类参数), 成员(值) { 体 }

示例：
    // 调用父类带参构造
    Student(string n, int a, string s)
        : Person(n, a),   // 传 n 和 a 给 Person
          school(s) { }

    // 调用父类无参构造（等价于省略不写）
    Student(string s) : Person(), school(s) { }

────────────────────────────────────────────────────

【构造/析构顺序速查表】

完整顺序（以 Car : Vehicle，含 Engine/Wheel 成员为例）：

构造：Vehicle → Engine → Wheel → Car体
析构：Car体 → Wheel → Engine → Vehicle

规律：
    构造：从最顶层父类开始，逐级向下，成员按声明顺序
    析构：从自身开始，逐级向上，成员按声明逆序

────────────────────────────────────────────────────

【必须用初始化列表的情况】

1. const 成员：const int id; → 必须 : id(val)
2. 引用成员：int& ref;       → 必须 : ref(r)
3. 没有默认构造的父类       → 必须 : 父类(参数)
4. 没有默认构造的成员对象   → 必须 : 成员(参数)
)";
}

// ============================================
// 第六部分：练习题
// ============================================

void showExercises() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第六部分：练习题（独立思考）" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
1. 预测题：以下代码会输出什么？（不要运行，手动推断）

   class X { public: X(){ cout<<"X"; } ~X(){ cout<<"~X"; } };
   class Y : public X {
       X m;  // 成员对象
   public:
       Y(){ cout<<"Y"; }
       ~Y(){ cout<<"~Y"; }
   };
   int main() { Y y; }

   提示：Y 继承 X，Y 还有一个 X 类型的成员

------------------------------------------------------------

2. 排错练习：找出以下代码的所有问题

   class Animal {
       string name;
   public:
       Animal(string n) : name(n) {}
       string getName() const { return name; }
   };
   class Dog : public Animal {
       string breed;
   public:
       Dog(string n, string b) {
           name = n;      // 问题1
           breed = b;
       }
       // 忘记写析构函数（问题2？还是不需要？）
   };

   提示：分析每个问题的原因

------------------------------------------------------------

3. 实现练习：

   设计一个 Shape → Rectangle → Square 三级继承体系

   Shape：含 color(string)，构造时打印 "Shape(color) 构造"
   Rectangle：含 width, height(double)，构造时打印 "Rectangle(w,h) 构造"
   Square：只有 side(double)，构造时打印 "Square(side) 构造"

   要求：
   - 每个类都有打印自己信息的 info() 方法
   - 构造时正确调用父类构造函数
   - 在 main 中创建 Square 对象，观察构造/析构顺序

------------------------------------------------------------

4. 思考题：
   为什么 C++ 规定构造顺序是"父类优先"，而不是"子类优先"？
   如果子类先构造会有什么问题？
   提示：思考子类构造函数体内可能会用到父类的成员变量
)";
}

// ============================================
// main 函数
// ============================================

int main() {
    cout << string(60, '=') << endl;
    cout << " 46_inheritance_constructor.cpp - 继承中的构造与析构" << endl;
    cout << string(60, '=') << endl;

    demoBasicOrder();
    demoMultiLevel();
    demoConstructorChain();
    demoMemberOrder();
    showCommonErrors();
    showFunctionCards();
    showExercises();

    cout << "\n" << string(60, '=') << endl;
    cout << "核心要点总结" << endl;
    cout << string(60, '=') << endl;
    cout << "1. 构造：父类 → 成员（声明顺序）→ 子类；析构完全相反" << endl;
    cout << "2. 子类必须在初始化列表中调用父类构造函数" << endl;
    cout << "3. const 和引用成员只能在初始化列表中初始化" << endl;
    cout << "4. 初始化列表中顺序无关，实际按类中声明顺序执行" << endl;
    cout << "5. 避免在构造/析构函数中调用虚函数（多态不生效）" << endl;

    return 0;
}
