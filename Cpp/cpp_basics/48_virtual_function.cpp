// ============================================
// 48_virtual_function.cpp - 超详细注释版
// 日期：第15天
// 难度：⭐⭐ 中级
// 主题：虚函数与动态绑定
// ============================================

/*
【核心概念】

是什么？
- 虚函数（virtual function）：用 virtual 关键字声明的成员函数
- 动态绑定（dynamic binding）：运行时根据对象实际类型决定调用哪个函数版本
- 多态（polymorphism）：同一接口（父类指针/引用），不同类型的对象表现出不同行为

为什么需要？
- 没有 virtual：父类指针调用哪个函数，编译期就固定了（看指针类型）
- 有 virtual：父类指针调用哪个函数，运行期才决定（看对象实际类型）
- 这让"统一接口，多种实现"成为可能

核心特点：
- virtual 只需要在父类声明，子类自动继承虚性
- 动态绑定通过虚函数表（vtable）实现
- 多态基类的析构函数必须是 virtual
*/

#include <iostream>
#include <string>
#include <vector>
using namespace std;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

/*
【函数卡片：virtual 虚函数声明】

位置：父类成员函数声明前
语法：virtual 返回类型 函数名(参数) [const];

示例：
    virtual void draw() const;          // 有默认实现的虚函数
    virtual double area() const = 0;    // 纯虚函数（无默认实现）

规则：
    - virtual 只在父类（声明处）写，子类 override 时不需要重复写 virtual
    - 但子类中 virtual 仍可写（可选），override 强烈推荐写

【静态绑定 vs 动态绑定】

静态绑定（非 virtual）：
    编译期决定调用哪个函数（根据指针/引用的声明类型）
    Shape* p = &circle;
    p->describe();  // 编译期确定：调用 Shape::describe

动态绑定（virtual）：
    运行期决定调用哪个函数（根据对象的实际类型）
    Shape* p = &circle;
    p->draw();  // 运行期通过 vtable 查找：调用 Circle::draw
*/

// ============================================
// 第二部分：基础操作实践
// ============================================

// ---- 2.1 有无 virtual 的直接对比 ----

// 不使用 virtual 的版本
class AnimalNoVirtual {
public:
    // 普通函数：静态绑定，父类指针调用父类版本
    void speak() const {
        cout << "[AnimalNoVirtual::speak] 动物发出声音" << endl;
    }
};

class DogNoVirtual : public AnimalNoVirtual {
public:
    // 隐藏父类 speak()，不是重写
    void speak() const {
        cout << "[DogNoVirtual::speak] 汪汪！" << endl;
    }
};

// 使用 virtual 的版本
class AnimalVirtual {
public:
    // virtual 函数：动态绑定，父类指针调用子类版本
    virtual void speak() const {
        cout << "[AnimalVirtual::speak] 动物发出声音" << endl;
    }
    // 多态基类析构函数必须是 virtual
    virtual ~AnimalVirtual() {}
};

class DogVirtual : public AnimalVirtual {
public:
    // 重写父类 virtual speak()
    void speak() const override {
        cout << "[DogVirtual::speak] 汪汪！" << endl;
    }
};

void demoVirtualVsNonVirtual() {
    cout << string(60, '=') << endl;
    cout << "第二部分 2.1：有无 virtual 的直接对比" << endl;
    cout << string(60, '=') << endl;

    DogNoVirtual dog1;
    DogVirtual   dog2;

    cout << "\n--- 无 virtual（静态绑定）---" << endl;
    // 通过子类对象直接调用：看对象类型 → 调用 DogNoVirtual::speak
    dog1.speak();
    // 通过父类指针调用：看指针类型（AnimalNoVirtual*）→ 调用 AnimalNoVirtual::speak
    AnimalNoVirtual* p1 = &dog1;
    p1->speak();  // ← 父类版本！不是子类版本
    cout << "  ↑ 无 virtual：父类指针调用父类版本（指针类型决定）" << endl;

    cout << "\n--- 有 virtual（动态绑定）---" << endl;
    dog2.speak();
    AnimalVirtual* p2 = &dog2;
    p2->speak();  // ← 子类版本！多态生效
    cout << "  ↑ 有 virtual：父类指针调用子类版本（对象类型决定）" << endl;
}

// ---- 2.2 虚函数实现多态 ----

class Shape {
public:
    string name;

    Shape(const string& n) : name(n) {}

    // virtual 函数：子类必须提供自己的绘制逻辑
    virtual void draw() const {
        cout << "[Shape::draw] 画一个图形：" << name << endl;
    }

    // virtual 函数：子类必须提供自己的面积计算
    virtual double area() const {
        return 0.0;
    }

    // virtual 析构函数：保证通过父类指针 delete 时能调用子类析构
    virtual ~Shape() {}
};

class Circle : public Shape {
public:
    double radius;

    // 参数：r - 半径
    Circle(double r) : Shape("圆形"), radius(r) {}

    void draw() const override {
        cout << "[Circle::draw] 画圆，半径=" << radius << endl;
    }

    // 圆面积 = π × r²
    double area() const override {
        return 3.14159 * radius * radius;
    }
};

class Rectangle : public Shape {
public:
    double width, height;

    Rectangle(double w, double h) : Shape("矩形"), width(w), height(h) {}

    void draw() const override {
        cout << "[Rectangle::draw] 画矩形，" << width << "×" << height << endl;
    }

    double area() const override {
        return width * height;
    }
};

class Triangle : public Shape {
public:
    double base, height;

    Triangle(double b, double h) : Shape("三角形"), base(b), height(h) {}

    void draw() const override {
        cout << "[Triangle::draw] 画三角形，底=" << base << " 高=" << height << endl;
    }

    // 三角形面积 = 底 × 高 / 2
    double area() const override {
        return 0.5 * base * height;
    }
};

// 统一接口：参数是 const Shape&，可接受任何 Shape 的子类
// 通过 virtual 函数，自动调用正确的子类版本
void printShapeInfo(const Shape& shape) {
    shape.draw();                                     // 动态绑定
    cout << "  面积：" << shape.area() << endl;       // 动态绑定
}

void demoPolymorphism() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.2：虚函数实现多态" << endl;
    cout << string(60, '=') << endl;

    Circle    c(5.0);
    Rectangle r(4.0, 3.0);
    Triangle  t(6.0, 4.0);

    cout << "\n--- 统一接口处理不同类型（多态）---" << endl;
    printShapeInfo(c);
    printShapeInfo(r);
    printShapeInfo(t);

    cout << "\n--- 用父类指针数组管理多种类型（多态的典型用法）---" << endl;
    // 数组里存 Shape*，实际指向 Circle 和 Rectangle 等对象
    Shape* shapes[] = { &c, &r, &t };
    double totalArea = 0;
    for (const Shape* s : shapes) {
        s->draw();  // 自动调用各自子类的 draw()
        totalArea += s->area();
    }
    cout << "所有图形总面积：" << totalArea << endl;
}

// ---- 2.3 virtual 析构函数的必要性 ----

class ResourceBase {
public:
    ResourceBase() {
        cout << "ResourceBase 构造" << endl;
    }

    // 如果没有 virtual 析构函数，通过父类指针 delete 子类对象
    // 只会调用 ~ResourceBase()，~ResourceDerived() 不会被调用
    virtual ~ResourceBase() {
        cout << "ResourceBase 析构" << endl;
    }
};

class ResourceDerived : public ResourceBase {
private:
    string* data;  // 需要在析构函数中释放的资源

public:
    ResourceDerived() : data(new string("子类独有的堆内存资源")) {
        cout << "ResourceDerived 构造，资源：" << *data << endl;
    }

    ~ResourceDerived() override {
        cout << "ResourceDerived 析构，释放资源：" << *data << endl;
        delete data;  // 如果析构函数不被调用，data 泄漏！
    }
};

void demoVirtualDestructor() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.3：virtual 析构函数" << endl;
    cout << string(60, '=') << endl;

    cout << "\n--- 通过父类指针管理子类对象（正确做法）---" << endl;
    ResourceBase* pb = new ResourceDerived();
    pb->show_virtual_dtor_importance_placeholder();  // 假调用，实际展示析构链
    cout << "通过父类指针 delete：（因为析构是 virtual，子类析构也会被调用）" << endl;
    delete pb;  // virtual 析构 → 先 ~ResourceDerived()，再 ~ResourceBase()
    cout << "（两个析构函数都被调用，资源正确释放）" << endl;
}

// ---- 2.4 虚函数表直觉理解 ----

void explainVTable() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.4：虚函数表（vtable）直觉理解" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【虚函数表（vtable）工作原理】

1. 每个有 virtual 函数的类，编译器会为它建一张"函数指针表"（vtable）
2. 每个该类的对象都有一个隐藏指针（vptr）指向这张表
3. 通过父类指针调用 virtual 函数时：
   a. 通过指针找到对象
   b. 读取对象的 vptr
   c. 在 vtable 中查找对应的函数指针
   d. 调用该函数

示意图：
    Circle 对象：
    ┌──────────────┐
    │ vptr ────────┼──→ Circle 的 vtable
    │ name(string) │       ┌────────────────┐
    │ radius(double│       │ draw  → Circle::draw   │
    └──────────────┘       │ area  → Circle::area   │
                           │ ~Shape→ Circle::~Circle│
                           └────────────────┘

    当执行 p->draw()（p 是 Shape*，指向 Circle）：
    → 通过 p 找到 Circle 对象
    → 读 vptr → 找 Circle 的 vtable
    → 查到 draw 对应 Circle::draw
    → 调用 Circle::draw ← 多态！
)";
}

// ============================================
// 🔍 第三部分：深入理解（可选）
// ============================================

/*
【深入理解：virtual 函数的性能开销】

调用非 virtual 函数：
    编译期确定地址，直接跳转（call instruction）
    开销：几乎为零

调用 virtual 函数：
    1. 通过对象找 vptr（一次内存读取）
    2. 通过 vptr 找 vtable（一次内存读取）
    3. 在 vtable 中查找函数指针（一次内存读取）
    4. 调用函数
    额外开销：约 2-3 次间接内存访问，通常可忽略不计

什么时候需要担心？
    - 每秒调用数亿次的热路径（数值计算、物理引擎）
    - 嵌入式系统内存极度受限时
    - 其他情况：不需要担心，优先考虑设计清晰

【深入理解：为什么 virtual 函数不能是 static？】

static 成员函数没有 this 指针，无法确定对象类型。
virtual 的本质是"根据 this 指向的对象查 vtable"。
没有 this → 无法查 vtable → 无法动态绑定 → virtual 无意义。
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonErrors() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第四部分：常见错误和陷阱" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
❌ 错误1：多态基类析构函数没有 virtual（资源泄漏）

   class Base { ~Base() {} };           // ← 无 virtual
   class Derived : public Base {
       int* data;
   public:
       Derived() { data = new int(42); }
       ~Derived() { delete data; }       // ← 这个析构不会被调用！
   };
   Base* p = new Derived();
   delete p;  // 只调用 ~Base()，~Derived() 未调用 → data 泄漏！

   ✅ 规则：只要类可能被继承并通过父类指针 delete，析构就要是 virtual

------------------------------------------------------------

❌ 错误2：在构造函数中调用 virtual 函数，期望多态

   class Base {
   public:
       Base() { draw(); }              // 构造时调用 virtual 函数
       virtual void draw() { cout << "Base::draw"; }
   };
   class Derived : public Base {
   public:
       void draw() override { cout << "Derived::draw"; }
   };

   Derived d;  // 输出 "Base::draw"，不是 "Derived::draw"！
   // 原因：构造 Base 时，Derived 部分还未初始化
   //       vptr 还未指向 Derived 的 vtable，调用的仍是 Base::draw

   ✅ 规则：不要在构造/析构函数中调用 virtual 函数（行为不符合预期）

------------------------------------------------------------

❌ 错误3：按值传递导致对象切片（slicing）

   void print(Shape shape) { shape.draw(); }  // ← 参数是值类型！

   Circle c(5.0);
   print(c);  // ← 将 Circle 复制给 Shape 参数（切片！）
              // Circle 部分被丢弃，draw() 调用 Shape::draw，多态失效！

   ✅ 正确：多态必须通过指针或引用
   void print(const Shape& shape) { shape.draw(); }   // 引用
   void print(const Shape* shape) { shape->draw(); }  // 指针

------------------------------------------------------------

❌ 错误4：误以为非 virtual 函数也能多态

   class Animal {
   public:
       void move() { cout << "Animal moves"; }  // 非 virtual
   };
   class Fish : public Animal {
   public:
       void move() { cout << "Fish swims"; }    // 隐藏，非重写
   };

   Animal* p = new Fish();
   p->move();  // 输出 "Animal moves"，不是 "Fish swims"！
   // 非 virtual 函数：静态绑定，根据声明类型 Animal* 决定

   ✅ 要实现多态：父类加 virtual

------------------------------------------------------------

❌ 错误5：忘记 override 导致签名不匹配（悄悄失效）

   class Shape {
   public:
       virtual void draw(int x, int y) const {}  // 需要两个参数
   };
   class Circle : public Shape {
   public:
       void draw() const {}  // ← 少了参数！忘写 override
       // 编译通过，但这是新函数，不是重写！
       // Shape* p = &c; p->draw(0,0) 调用的是 Shape::draw，不是 Circle 这个
   };

   ✅ 始终加 override：编译器立即报错，防止这类错误

------------------------------------------------------------

❌ 错误6：纯虚函数的子类没有实现所有纯虚函数

   class IShape {
   public:
       virtual void draw() const = 0;
       virtual double area() const = 0;
   };
   class HalfImpl : public IShape {
   public:
       void draw() const override { cout << "draw"; }
       // 忘记实现 area()！
   };
   HalfImpl h;  // ❌ 编译错误：HalfImpl 仍是抽象类（area 未实现）

   ✅ 继承抽象类时，必须实现所有纯虚函数
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
【virtual 虚函数】

声明位置：父类
语法：virtual 返回类型 函数名(参数) [const] { 实现 }
      virtual 返回类型 函数名(参数) [const] = 0;  // 纯虚

作用：
    - 开启动态绑定（运行期根据对象实际类型调用正确版本）
    - 父类指针/引用调用该函数时，能到达子类实现

────────────────────────────────────────────────────

【多态成立的三个条件】

1. 继承关系：子类继承自父类
2. 虚函数重写：子类重写父类的 virtual 函数（用 override）
3. 通过指针或引用调用：必须是父类指针（Shape*）或引用（Shape&），
   不能按值传递（会切片）

缺少任一条件，多态不成立。

────────────────────────────────────────────────────

【virtual 析构函数规则】

当一个类：
    - 有 virtual 成员函数，或
    - 被设计为基类供继承
则析构函数必须声明为 virtual：
    virtual ~ClassName() {}

原因：
    Base* p = new Derived();
    delete p;  // virtual析构 → 先~Derived()再~Base()（正确）
               // 非virtual析构 → 只~Base()（子类资源泄漏！）

────────────────────────────────────────────────────

【动态绑定的触发条件】

必须满足：
    ✅ 函数是 virtual
    ✅ 通过指针（T*）或引用（T&）调用
    ✅ 指针/引用的实际对象是子类

不满足时（静态绑定）：
    ❌ 函数不是 virtual
    ❌ 通过对象值（T obj）调用（切片）
    ❌ 通过对象直接调用（obj.func()，此时已知确切类型）
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
1. 预测题：以下每行输出什么？

   class A {
   public:
       virtual void f() { cout << "A::f "; }
       void g() { cout << "A::g "; }
       virtual ~A() {}
   };
   class B : public A {
   public:
       void f() override { cout << "B::f "; }
       void g() { cout << "B::g "; }
   };
   B b;
   A* p = &b;
   p->f();   // ①
   p->g();   // ②
   b.f();    // ③
   b.g();    // ④

   A a2 = b;  // ⑤ 对象切片
   a2.f();    // ⑥ 输出什么？

   提示：⑥ 是对象值，不是指针，思考切片的影响

------------------------------------------------------------

2. 设计题：

   设计一个动物园管理程序：
   - 抽象基类 Animal：virtual string sound() const = 0
   - 子类 Dog、Cat、Bird：各自实现 sound()
   - 函数 makeAllSpeak(vector<Animal*> animals)：
     遍历调用每个动物的 sound()

   要求：
   - 确保 Animal 析构函数是 virtual
   - 用 new 创建对象，用完后 delete（观察析构顺序）

------------------------------------------------------------

3. 排错练习：

   class Logger {
   public:
       virtual void log(string msg) { cout << "[LOG]" << msg; }
       ~Logger() {}   // 注意这里
   };
   class FileLogger : public Logger {
       ofstream* file;
   public:
       FileLogger() { file = new ofstream("log.txt"); }
       void log(string msg) override { *file << msg; }
       ~FileLogger() { file->close(); delete file; }
   };
   Logger* logger = new FileLogger();
   // ... 使用 logger
   delete logger;  // 有什么问题？

   提示：思考 ~Logger() 没有 virtual 时 delete logger 的行为

------------------------------------------------------------

4. 思考题：
   以下代码中，Circle c(5.0) 对象占用多少字节的内存？
   （假设 double 是 8 字节，string 对象 32 字节，指针 8 字节）

   class Shape {
       string name;         // 32 字节
       virtual void draw(); // 需要 vptr？
       virtual ~Shape();
   };
   class Circle : public Shape {
       double radius;       // 8 字节
   };

   提示：有 virtual 函数的类，对象内部有一个隐藏的 vptr 指针
)";
}

// ============================================
// main 函数
// ============================================

int main() {
    cout << string(60, '=') << endl;
    cout << "      48_virtual_function.cpp - 虚函数与动态绑定" << endl;
    cout << string(60, '=') << endl;

    demoVirtualVsNonVirtual();
    demoPolymorphism();

    // 2.3 virtual 析构函数演示
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.3：virtual 析构函数" << endl;
    cout << string(60, '=') << endl;
    {
        ResourceBase* pb = new ResourceDerived();
        cout << "通过父类指针 delete（virtual析构，两个析构函数都被调用）：" << endl;
        delete pb;
    }

    explainVTable();
    showCommonErrors();
    showFunctionCards();
    showExercises();

    cout << "\n" << string(60, '=') << endl;
    cout << "核心要点总结" << endl;
    cout << string(60, '=') << endl;
    cout << "1. virtual 开启动态绑定：父类指针调用子类版本（多态）" << endl;
    cout << "2. 多态三条件：继承 + virtual 重写 + 指针/引用调用" << endl;
    cout << "3. 多态基类析构函数必须是 virtual，否则子类资源泄漏" << endl;
    cout << "4. 按值传递会切片，多态必须用指针或引用" << endl;
    cout << "5. 不要在构造/析构函数中调用 virtual 函数" << endl;

    return 0;
}
