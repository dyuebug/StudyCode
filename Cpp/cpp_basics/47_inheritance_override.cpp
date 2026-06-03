// ============================================
// 47_inheritance_override.cpp - 超详细注释版
// 日期：第14天
// 难度：⭐⭐ 中级
// 主题：方法重写（override）与函数隐藏
// ============================================

/*
【核心概念】

是什么？
- 函数隐藏（Hiding）：子类定义同名函数，在子类作用域内"遮蔽"父类版本
- 函数重写（Override）：子类重新实现父类的 virtual 函数，通过父类指针/引用可调用子类版本
- override 关键字：让编译器验证确实在重写 virtual 函数，防止拼写错误

为什么需要？
- 不加 virtual 的同名函数只是隐藏，通过父类指针仍调用父类版本（常见陷阱）
- virtual + override 才能实现真正的多态

核心特点：
- virtual 函数：运行时动态绑定（动态多态）
- 非 virtual 函数：编译期静态绑定（根据指针类型决定）
- override 关键字：可选但强烈推荐，防止签名错误
- final 关键字：禁止子类继续重写
*/

#include <iostream>
#include <string>
using namespace std;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

/*
【隐藏 vs 重写 对比表】

              函数隐藏（Hiding）     函数重写（Override）
父类函数声明   普通函数（无virtual）   virtual 函数
子类函数       同名函数               同名函数 + override
通过子类对象   调用子类版本           调用子类版本
通过父类指针   调用父类版本！！       调用子类版本（多态！）
用途           很少有意使用           实现多态的核心机制

【函数卡片：virtual / override / final】

virtual（父类中写）：
    virtual 返回类型 函数名(参数) { 默认实现 }
    virtual 返回类型 函数名(参数) = 0;   // 纯虚函数：无默认实现

override（子类中写，可选但推荐）：
    返回类型 函数名(参数) override { 子类实现 }
    作用：编译器验证父类中有完全匹配的 virtual 函数，否则报错

final（子类中写，可选）：
    返回类型 函数名(参数) override final { 实现 }
    作用：禁止这个函数被更深层的子类再次重写
*/

// ============================================
// 第二部分：基础操作实践
// ============================================

// ---- 2.1 隐藏 vs 重写的对比 ----

class Shape {
public:
    string color;

    Shape(const string& c = "白色") : color(c) {}

    // 普通函数（无 virtual）：子类同名函数只是"隐藏"
    void describe() const {
        cout << "[Shape::describe] 这是一个" << color << "的图形" << endl;
    }

    // virtual 函数：子类可以"重写"，通过父类指针能调用子类版本
    // virtual 告诉编译器：这个函数需要动态绑定
    virtual void draw() const {
        cout << "[Shape::draw] 画一个默认图形" << endl;
    }

    // virtual 函数：计算面积
    virtual double area() const {
        return 0.0;
    }

    // 多态基类析构函数必须是 virtual（原因见第四部分 错误3）
    virtual ~Shape() {}
};

class Circle : public Shape {
public:
    double radius;

    Circle(double r, const string& c = "红色")
        : Shape(c), radius(r) {}

    // 隐藏父类 describe()（没有 virtual，不是真正重写）
    // 效果：通过 Circle 对象调用时走这里
    //       通过 Shape* 调用时仍然走 Shape::describe！
    void describe() const {
        cout << "[Circle::describe] 这是一个" << color
             << "的圆，半径：" << radius << endl;
    }

    // 重写父类 virtual draw()
    // override：让编译器检查父类确实有 virtual void draw() const
    void draw() const override {
        cout << "[Circle::draw] 画圆，半径：" << radius << endl;
    }

    // 重写父类 virtual area()
    double area() const override {
        return 3.14159 * radius * radius;
    }
};

class Rectangle : public Shape {
public:
    double width, height;

    Rectangle(double w, double h, const string& c = "蓝色")
        : Shape(c), width(w), height(h) {}

    void describe() const {
        cout << "[Rectangle::describe] " << color
             << "矩形，" << width << "×" << height << endl;
    }

    void draw() const override {
        cout << "[Rectangle::draw] 画矩形，" << width << "×" << height << endl;
    }

    double area() const override {
        return width * height;
    }
};

void demoHideVsOverride() {
    cout << string(60, '=') << endl;
    cout << "第二部分 2.1：隐藏 vs 重写" << endl;
    cout << string(60, '=') << endl;

    Circle    circle(5.0, "红色");
    Rectangle rect(4.0, 3.0, "蓝色");

    cout << "\n--- 通过对象直接调用（两种都调用子类版本）---" << endl;
    circle.describe();  // Circle::describe（隐藏效果，对象调用看对象类型）
    circle.draw();      // Circle::draw（重写效果，对象调用同样看对象类型）

    cout << "\n--- 通过父类指针调用（关键区别！）---" << endl;
    Shape* ps = &circle;

    // 非 virtual 函数：静态绑定，根据指针类型（Shape*）决定 → 调用父类版本
    ps->describe();  // ← 调用 Shape::describe！不是 Circle::describe！
    cout << "  ↑ describe() 没有 virtual，父类指针调用父类版本（陷阱！）" << endl;

    // virtual 函数：动态绑定，根据实际对象类型（Circle）决定 → 调用子类版本
    ps->draw();      // ← 调用 Circle::draw（多态！）
    cout << "  ↑ draw() 有 virtual，父类指针调用子类版本（多态！）" << endl;
}

// ---- 2.2 override 关键字的保护作用 ----

class Animal {
public:
    virtual void speak() const {
        cout << "[Animal] 动物发出声音" << endl;
    }
    virtual ~Animal() {}
};

class Dog : public Animal {
public:
    // ✅ 正确重写：签名完全匹配父类的 virtual void speak() const
    void speak() const override {
        cout << "[Dog] 汪汪！" << endl;
    }
};

class Cat : public Animal {
public:
    void speak() const override {
        cout << "[Cat] 喵喵！" << endl;
    }
};

// 展示 override 防错的价值（以注释形式）
// class BadDog : public Animal {
// public:
//     void Speak() const override {}   // ❌ 编译错误：Speak 大写，父类没有
//     void speak(int) const override {}// ❌ 编译错误：多了一个 int 参数，父类没有
// };
// 如果没有 override，以上两种情况编译通过，但实际上是新函数而非重写，多态失效！

void demoOverride() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.2：override 关键字的保护" << endl;
    cout << string(60, '=') << endl;

    Animal* animals[] = { new Dog(), new Cat() };
    for (auto* a : animals) {
        a->speak();  // 多态调用：根据实际类型决定
        delete a;
    }
    cout << "（通过 Animal* 调用 virtual speak()，自动选择子类版本）" << endl;
}

// ---- 2.3 final 关键字 ----

class Base {
public:
    virtual void func() {
        cout << "[Base::func]" << endl;
    }
    virtual ~Base() {}
};

class Mid : public Base {
public:
    // override final：重写父类函数，同时禁止自己被子类再次重写
    void func() override final {
        cout << "[Mid::func] 这是最终版本，子类不能再重写" << endl;
    }
};

// 如果有 Deep : Mid 并尝试重写 func()，编译器报错：
// class Deep : public Mid {
//     void func() override {}  // ❌ 编译错误：func 在 Mid 中是 final
// };

void demoFinal() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.3：final 关键字" << endl;
    cout << string(60, '=') << endl;

    Mid m;
    m.func();         // 直接调用

    Base* pb = &m;
    pb->func();       // 多态调用（调用 Mid::func，因为是 virtual）
    cout << "（final 只禁止子类继续重写，不影响多态调用）" << endl;
}

// ---- 2.4 多态的实际应用：统一接口处理不同类型 ----

// 通过父类指针/引用调用 virtual 函数，实现统一接口
void printShapeInfo(const Shape& shape) {
    // 参数类型是 const Shape&，但可以传入 Circle 或 Rectangle
    // 因为 draw() 和 area() 是 virtual，运行时调用实际类型的版本
    shape.draw();
    cout << "  面积：" << shape.area() << endl;
    shape.describe();  // 非 virtual，始终调用 Shape::describe
}

void demoPolymorphism() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.4：多态实际应用" << endl;
    cout << string(60, '=') << endl;

    Circle    c(3.0, "绿色");
    Rectangle r(5.0, 2.0, "黄色");

    cout << "\n--- 以 Shape& 接收 Circle ---" << endl;
    printShapeInfo(c);

    cout << "\n--- 以 Shape& 接收 Rectangle ---" << endl;
    printShapeInfo(r);

    cout << "\n（draw/area 是 virtual → 调用各自子类版本）" << endl;
    cout << "（describe 非 virtual → 始终调用 Shape 版本）" << endl;
}

// ============================================
// 🔍 第三部分：深入理解（可选）
// ============================================

/*
【深入理解：静态绑定 vs 动态绑定】

静态绑定（非 virtual）：
    编译阶段，根据指针/引用的声明类型决定调用哪个函数
    Shape* p = &circle;
    p->describe();  // 编译器看到 Shape*，直接编译为调用 Shape::describe

动态绑定（virtual）：
    运行阶段，通过虚函数表（vtable）查找实际类型的函数
    Shape* p = &circle;
    p->draw();  // 运行时查找 p 指向对象的 vtable → 找到 Circle::draw

代价：动态绑定需要一次指针间接访问（vtable 查找），
      比静态绑定略慢，但通常可以忽略不计。

【深入理解：vtable 工作原理】

    Circle 对象内存布局（有 virtual 函数时）：
    ┌─────────────────────────────┐
    │ vptr → Circle 的 vtable     │  ← 每个有virtual函数的对象都有这个指针
    │ color (string)              │  ← 继承自 Shape 的成员
    │ radius (double)             │  ← Circle 自己的成员
    └─────────────────────────────┘

    Circle 的 vtable：
    ┌─────────────────────┐
    │ 0: Circle::draw     │
    │ 1: Circle::area     │
    │ 2: Circle::~Circle  │
    └─────────────────────┘

    调用 p->draw() 时（p 是 Shape*，指向 Circle）：
    1. 通过 p 找到 Circle 对象
    2. 读取 vptr，找到 Circle 的 vtable
    3. 查表找到 Circle::draw
    4. 调用 Circle::draw
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonErrors() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第四部分：常见错误和陷阱" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
❌ 错误1：以为"隐藏"可以实现多态（最常见陷阱！）

   class Animal {
   public:
       void sound() { cout << "Animal sound"; }  // 无 virtual
   };
   class Dog : public Animal {
   public:
       void sound() { cout << "Woof"; }  // 隐藏，不是重写
   };

   Dog dog;
   Animal* p = &dog;
   p->sound();  // ← 输出 "Animal sound"，不是 "Woof"！！
   // 原因：sound() 没有 virtual，静态绑定，根据 Animal* 决定

   ✅ 正确：要实现多态，父类必须用 virtual
   virtual void sound() { ... }   // 父类加 virtual
   void sound() override { ... }  // 子类加 override

------------------------------------------------------------

❌ 错误2：不加 override 导致函数签名错误但编译通过

   class Animal {
   public:
       virtual void speak() const { ... }
   };
   class Dog : public Animal {
   public:
       void speak() { ... }  // ← 没有 const，签名不同，没有 override
       // 编译通过！但这不是重写，是一个新的同名函数
       // Animal* p = &dog; p->speak() 调用的是 Animal::speak const！
   };

   ✅ 正确：加上 override，编译器立即报错
   void speak() override { ... }  // ❌ 编译错误：签名不匹配
   void speak() const override { ... }  // ✅ 正确签名

------------------------------------------------------------

❌ 错误3：多态基类析构函数没有 virtual（资源泄漏）

   class Animal {
   public:
       ~Animal() { cout << "Animal析构"; }  // ← 无 virtual
   };
   class Dog : public Animal {
       string* name;
   public:
       Dog() { name = new string("旺财"); }
       ~Dog() { delete name; cout << "Dog析构"; }
   };

   Animal* p = new Dog();
   delete p;  // ← 只调用 ~Animal()，Dog::~Dog() 没有被调用！
              // name 指向的内存永远泄漏！

   ✅ 正确：多态基类析构函数必须是 virtual
   virtual ~Animal() { cout << "Animal析构"; }

------------------------------------------------------------

❌ 错误4：const 不一致导致 override 失败

   class Base { public: virtual void func() const { } };
   class Derived : public Base {
   public:
       void func() override { }  // ← 少了 const！签名不同，编译错误
   };

   ✅ 正确：子类和父类的函数签名（参数类型、const）必须完全一致
   void func() const override { }  // ← 加上 const

------------------------------------------------------------

❌ 错误5：返回类型不一致（非协变情况）

   class Animal { public: virtual Animal* clone() { return new Animal(); } };
   class Dog : public Animal {
   public:
       // 允许：协变返回类型（Dog* 是 Animal* 的子类型）
       Dog* clone() override { return new Dog(); }  // ✅ 合法

       // 不允许：非协变的不同类型
       // int* clone() override { ... }  // ❌ 编译错误
   };

------------------------------------------------------------

❌ 错误6：用 final 类后继续继承

   class Shape final { ... };  // final 类：禁止继承
   class Circle : public Shape { ... };  // ❌ 编译错误！Shape 是 final

   // final 可以用于整个类（不能再被继承）
   // 也可以用于单个方法（不能再被子类重写）
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
【virtual 关键字】

位置：父类函数声明前
语法：virtual 返回类型 函数名(参数列表) [const] { 实现 }
作用：启用动态绑定，允许子类重写，通过父类指针/引用调用子类版本

示例：
    virtual void draw() const { cout << "Shape"; }    // 有默认实现
    virtual double area() const = 0;                   // 纯虚函数，无实现

────────────────────────────────────────────────────

【override 关键字】

位置：子类函数参数列表之后（或 const 之后）
语法：返回类型 函数名(参数列表) [const] override { 实现 }
作用：让编译器验证父类有完全匹配的 virtual 函数（防拼写错误）

示例：
    void draw() const override { cout << "Circle"; }
    double area() const override { return 3.14 * r * r; }

────────────────────────────────────────────────────

【final 关键字】

用于函数：void func() override final { }  → 禁止子类继续重写
用于类：  class Shape final { };           → 禁止类被继承

────────────────────────────────────────────────────

【隐藏 vs 重写 速查】

父类：void f()     子类：void f()     → 隐藏（无多态）
父类：virtual f()  子类：f() override → 重写（有多态）

判断：通过 Shape* p = &circle; p->f(); 看结果
    如果调用父类版本 → 隐藏
    如果调用子类版本 → 重写（多态）
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
1. 预测题：以下代码输出什么？

   class A {
   public:
       void f() { cout << "A::f "; }
       virtual void g() { cout << "A::g "; }
   };
   class B : public A {
   public:
       void f() { cout << "B::f "; }
       void g() override { cout << "B::g "; }
   };
   int main() {
       B b;
       A* p = &b;
       p->f();   // ①
       p->g();   // ②
       b.f();    // ③
       b.g();    // ④
   }

   提示：区分静态绑定（非virtual）和动态绑定（virtual）

------------------------------------------------------------

2. 排错练习：以下代码有哪些问题？

   class Printer {
   public:
       virtual void print(string text) { cout << text; }
       ~Printer() {}   // 问题1？
   };
   class ConsolePrinter : public Printer {
   public:
       void Print(string text) override { cout << "[Console]" << text; }  // 问题2？
       void print(string text) { cout << "[Console]" << text; }  // 问题3？
   };

   提示：
   - 问题1：析构函数与多态的关系
   - 问题2：override 失败时编译器会报错吗？
   - 问题3：没有 override 的潜在问题

------------------------------------------------------------

3. 实现练习：

   创建一个简单的图形计算程序：
   - 基类 Shape：virtual double area() const = 0（纯虚）
   - 子类 Circle(radius)：计算圆面积
   - 子类 Rect(w, h)：计算矩形面积
   - 子类 Triangle(base, height)：计算三角形面积
   - 函数 totalArea(Shape* shapes[], int n)：
     统计所有图形的总面积

   验证：多态调用能正确计算各种图形

------------------------------------------------------------

4. 思考题：
   如果父类析构函数不是 virtual，会导致什么问题？
   请设计一个最简单的代码示例，演示资源泄漏的发生。
   提示：父类指针 delete 子类对象时观察析构调用情况
)";
}

// ============================================
// main 函数
// ============================================

int main() {
    cout << string(60, '=') << endl;
    cout << " 47_inheritance_override.cpp - 方法重写与override" << endl;
    cout << string(60, '=') << endl;

    demoHideVsOverride();
    demoOverride();
    demoFinal();
    demoPolymorphism();
    showCommonErrors();
    showFunctionCards();
    showExercises();

    cout << "\n" << string(60, '=') << endl;
    cout << "核心要点总结" << endl;
    cout << string(60, '=') << endl;
    cout << "1. virtual → 动态绑定，父类指针调用子类版本（多态）" << endl;
    cout << "2. 无 virtual → 静态绑定，父类指针始终调用父类版本（隐藏）" << endl;
    cout << "3. override → 让编译器验证签名正确，强烈推荐使用" << endl;
    cout << "4. final → 禁止子类继续重写某函数或继承某类" << endl;
    cout << "5. 多态基类析构函数必须是 virtual，否则子类资源泄漏" << endl;

    return 0;
}
