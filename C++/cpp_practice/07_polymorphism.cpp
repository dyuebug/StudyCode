// ============================================
// 多态与虚函数机制 - 超详细注释版
// 日期：2026-06-20（第7天）
// 难度：⭐⭐⭐⭐ 高级
// 参考资料：CppFiles/CppBase/8.多态.md
// ============================================

/*
【核心概念】多态（Polymorphism）

是什么？
- 多态是面向对象编程的三大特性之一（封装、继承、多态）
- 多态的字面意思是"多种形态"
- 同一接口，可以有不同的实现方式

为什么需要？
- 提高代码的灵活性和可扩展性
- 实现接口和实现分离
- 便于维护和扩展（开闭原则：对扩展开放，对修改关闭）

核心特点：
- 编译时多态：函数重载、运算符重载、模板
- 运行时多态：虚函数、继承、基类指针/引用
- 动态绑定：运行时决定调用哪个函数
*/

#include <iostream>
#include <string>
#include <vector>
using namespace std;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

// --------------------------------------------
// 1.1 多态的分类
// --------------------------------------------

/*
【多态的两种形式】

1. 编译时多态（静态多态）
   - 函数重载：同名函数，不同参数
   - 运算符重载：重定义运算符行为
   - 模板：类型参数化
   - 特点：编译期确定调用哪个函数（静态绑定）

2. 运行时多态（动态多态）
   - 虚函数 + 继承 + 基类指针/引用
   - 特点：运行期确定调用哪个函数（动态绑定）
   - 本节重点：运行时多态
*/

// --------------------------------------------
// 1.2 虚函数（Virtual Function）⭐⭐⭐⭐⭐
// --------------------------------------------

/*
【虚函数的定义】

语法：在基类中用 virtual 关键字声明函数
virtual 返回类型 函数名(参数列表);

作用：
- 允许派生类重写（override）该函数
- 实现运行时多态（动态绑定）

重写规则：
- 函数名必须相同
- 参数列表必须相同（类型、个数、顺序）
- 返回类型必须相同（协变返回类型除外）
- 访问权限可以不同（但不推荐）

C++11 新特性：
- override：明确标记重写虚函数（编译器会检查）
- final：禁止派生类重写虚函数
*/

// --------------------------------------------
// 1.3 虚函数表（Virtual Table, vtable）⭐⭐⭐⭐⭐
// --------------------------------------------

/*
【虚函数表原理】（面试高频考点！）

1. 虚函数指针（vptr）
   - 每个含有虚函数的对象都有一个 vptr
   - vptr 存储在对象的起始位置（通常）
   - vptr 指向该对象所属类的 vtable

2. 虚函数表（vtable）
   - 每个含有虚函数的类都有一个 vtable
   - vtable 存储该类所有虚函数的地址
   - vtable 在编译期生成，存储在只读数据段

3. 动态绑定过程
   - 通过基类指针/引用调用虚函数
   - 通过对象的 vptr 找到 vtable
   - 在 vtable 中找到对应虚函数的地址
   - 调用该地址的函数

对象内存布局：
┌─────────────┐
│   vptr      │ ← 虚函数指针（指向vtable）
├─────────────┤
│  数据成员    │
└─────────────┘

虚函数表（vtable）：
┌─────────────────┐
│ Base::func1     │ ← 虚函数地址
├─────────────────┤
│ Base::func2     │
├─────────────────┤
│ Base::~Base     │
└─────────────────┘

为什么需要虚函数表？
- 实现动态绑定（运行时确定调用哪个函数）
- 相同类型的对象共享同一个 vtable（节省内存）
- 支持多态机制
*/

// --------------------------------------------
// 1.4 纯虚函数和抽象类 ⭐⭐⭐⭐⭐
// --------------------------------------------

/*
【纯虚函数】

语法：在虚函数声明后加 = 0
virtual 返回类型 函数名(参数列表) = 0;

特点：
- 没有函数体（不需要实现）
- 必须由派生类实现
- 用于定义接口

【抽象类】

定义：包含纯虚函数的类称为抽象类

特点：
- 不能实例化（不能创建对象）
- 可以有构造函数和析构函数
- 可以有普通成员函数和数据成员
- 派生类必须实现所有纯虚函数，否则也是抽象类

作用：
- 定义接口（类似Java的interface）
- 强制派生类实现特定功能
- 实现接口和实现分离
*/

// --------------------------------------------
// 1.5 虚析构函数 ⭐⭐⭐⭐⭐（超级重要！）
// --------------------------------------------

/*
【虚析构函数的重要性】

规则：基类的析构函数必须是虚函数！

为什么？
- 防止内存泄漏
- 确保派生类的析构函数被正确调用

示例：
class Base {
public:
    ~Base() { }  // ❌ 非虚析构函数
};

class Derived : public Base {
public:
    ~Derived() { delete[] data; }  // 释放资源
private:
    int* data;
};

Base* p = new Derived();
delete p;  // 问题：只调用~Base()，不调用~Derived()
           // 结果：内存泄漏！data没有被释放

正确做法：
class Base {
public:
    virtual ~Base() { }  // ✅ 虚析构函数
};

delete p;  // 正确：先调用~Derived()，再调用~Base()

记忆技巧：
- 只要类会被继承，析构函数就应该是虚函数
- 如果类中有虚函数，析构函数也应该是虚函数
*/

// ============================================
// 第二部分：基础操作实践
// ============================================

// --------------------------------------------
// 2.1 基础虚函数示例：图形类
// --------------------------------------------

// 基类：图形
class Shape {
public:
    // 虚函数：绘制图形
    virtual void draw() const {
        cout << "绘制图形" << endl;
    }

    // 虚函数：计算面积
    virtual double area() const {
        return 0.0;
    }

    // 虚析构函数（重要！）
    virtual ~Shape() {
        cout << "Shape析构函数" << endl;
    }
};

// 派生类：圆形
class Circle : public Shape {
public:
    Circle(double r) : _radius(r) {
        cout << "Circle构造函数" << endl;
    }

    // 重写虚函数：绘制圆形
    // 使用 override 关键字（C++11），编译器会检查是否正确重写
    void draw() const override {
        cout << "绘制圆形，半径 = " << _radius << endl;
    }

    // 重写虚函数：计算圆形面积
    double area() const override {
        return 3.14159 * _radius * _radius;
    }

    ~Circle() {
        cout << "Circle析构函数" << endl;
    }

private:
    double _radius;
};

// 派生类：矩形
class Rectangle : public Shape {
public:
    Rectangle(double w, double h) : _width(w), _height(h) {
        cout << "Rectangle构造函数" << endl;
    }

    void draw() const override {
        cout << "绘制矩形，宽 = " << _width << ", 高 = " << _height << endl;
    }

    double area() const override {
        return _width * _height;
    }

    ~Rectangle() {
        cout << "Rectangle析构函数" << endl;
    }

private:
    double _width;
    double _height;
};

// --------------------------------------------
// 2.2 动态绑定 vs 静态绑定演示
// --------------------------------------------

// 演示函数：静态绑定（通过对象调用）
void testStaticBinding() {
    cout << "\n【静态绑定：通过对象调用】" << endl;

    Circle c(5.0);
    c.draw();  // 静态绑定：编译期确定调用 Circle::draw()

    Shape s;
    s.draw();  // 静态绑定：编译期确定调用 Shape::draw()
}

// 演示函数：动态绑定（通过基类指针调用）
void testDynamicBinding() {
    cout << "\n【动态绑定：通过基类指针调用】" << endl;

    // 基类指针指向派生类对象
    Shape* p1 = new Circle(5.0);
    Shape* p2 = new Rectangle(4.0, 6.0);

    // 动态绑定：运行期通过 vptr 找到 vtable，调用派生类的函数
    p1->draw();  // 调用 Circle::draw()
    p2->draw();  // 调用 Rectangle::draw()

    cout << "圆形面积：" << p1->area() << endl;
    cout << "矩形面积：" << p2->area() << endl;

    // 释放内存（会调用虚析构函数）
    delete p1;  // 先调用 Circle::~Circle()，再调用 Shape::~Shape()
    delete p2;  // 先调用 Rectangle::~Rectangle()，再调用 Shape::~Shape()
}

// 演示函数：通过基类引用调用
void printShapeInfo(const Shape& s) {
    cout << "\n【通过基类引用调用】" << endl;
    s.draw();  // 动态绑定
    cout << "面积：" << s.area() << endl;
}

// --------------------------------------------
// 2.3 虚函数表原理演示
// --------------------------------------------

// 演示函数：观察对象大小（验证 vptr 的存在）
void testVtableSize() {
    cout << "\n【虚函数表原理：对象大小】" << endl;

    // 没有虚函数的类
    class NoVirtual {
        int data;
    };

    // 有虚函数的类
    class HasVirtual {
        int data;
    public:
        virtual void func() {}
    };

    cout << "没有虚函数的类，对象大小：" << sizeof(NoVirtual) << " 字节" << endl;
    cout << "有虚函数的类，对象大小：" << sizeof(HasVirtual) << " 字节" << endl;

    // 分析：
    // NoVirtual: 只有 int data (4字节)
    // HasVirtual: int data (4字节) + vptr (8字节，64位系统) = 12字节（可能对齐到16）

    cout << "\nShape 对象大小：" << sizeof(Shape) << " 字节（包含 vptr）" << endl;
    cout << "Circle 对象大小：" << sizeof(Circle) << " 字节（包含 vptr + _radius）" << endl;
    cout << "Rectangle 对象大小：" << sizeof(Rectangle) << " 字节（包含 vptr + _width + _height）" << endl;
}

// --------------------------------------------
// 2.4 纯虚函数和抽象类
// --------------------------------------------

// 抽象类：动物接口
class Animal {
public:
    // 纯虚函数：叫声
    virtual void makeSound() const = 0;

    // 纯虚函数：移动方式
    virtual void move() const = 0;

    // 普通成员函数
    void sleep() const {
        cout << "动物在睡觉" << endl;
    }

    // 虚析构函数
    virtual ~Animal() {
        cout << "Animal析构函数" << endl;
    }
};

// 派生类：狗
class Dog : public Animal {
public:
    // 实现纯虚函数
    void makeSound() const override {
        cout << "汪汪汪！" << endl;
    }

    void move() const override {
        cout << "狗在跑" << endl;
    }

    ~Dog() {
        cout << "Dog析构函数" << endl;
    }
};

// 派生类：猫
class Cat : public Animal {
public:
    void makeSound() const override {
        cout << "喵喵喵！" << endl;
    }

    void move() const override {
        cout << "猫在跳" << endl;
    }

    ~Cat() {
        cout << "Cat析构函数" << endl;
    }
};

// 演示函数：使用抽象类接口
void testAnimal(const Animal& a) {
    a.makeSound();  // 动态绑定
    a.move();       // 动态绑定
    a.sleep();      // 普通成员函数
}

// --------------------------------------------
// 2.5 虚析构函数的重要性演示 ⭐⭐⭐⭐⭐
// --------------------------------------------

// 错误示例：没有虚析构函数
class BaseWithoutVirtualDestructor {
public:
    BaseWithoutVirtualDestructor() {
        cout << "BaseWithoutVirtualDestructor构造" << endl;
    }

    // ❌ 非虚析构函数
    ~BaseWithoutVirtualDestructor() {
        cout << "BaseWithoutVirtualDestructor析构" << endl;
    }
};

class DerivedWithResource : public BaseWithoutVirtualDestructor {
public:
    DerivedWithResource() {
        data = new int[100];  // 分配资源
        cout << "DerivedWithResource构造，分配了资源" << endl;
    }

    ~DerivedWithResource() {
        delete[] data;  // 释放资源
        cout << "DerivedWithResource析构，释放了资源" << endl;
    }

private:
    int* data;
};

// 正确示例：有虚析构函数
class BaseWithVirtualDestructor {
public:
    BaseWithVirtualDestructor() {
        cout << "BaseWithVirtualDestructor构造" << endl;
    }

    // ✅ 虚析构函数
    virtual ~BaseWithVirtualDestructor() {
        cout << "BaseWithVirtualDestructor析构" << endl;
    }
};

class DerivedWithResourceCorrect : public BaseWithVirtualDestructor {
public:
    DerivedWithResourceCorrect() {
        data = new int[100];
        cout << "DerivedWithResourceCorrect构造，分配了资源" << endl;
    }

    ~DerivedWithResourceCorrect() {
        delete[] data;
        cout << "DerivedWithResourceCorrect析构，释放了资源" << endl;
    }

private:
    int* data;
};

// 演示函数：虚析构函数的重要性
void testVirtualDestructor() {
    cout << "\n【错误示例：没有虚析构函数】" << endl;
    {
        BaseWithoutVirtualDestructor* p = new DerivedWithResource();
        delete p;  // ❌ 问题：只调用基类析构，派生类的资源没有释放（内存泄漏）
    }

    cout << "\n【正确示例：有虚析构函数】" << endl;
    {
        BaseWithVirtualDestructor* p = new DerivedWithResourceCorrect();
        delete p;  // ✅ 正确：先调用派生类析构，再调用基类析构
    }
}

// ============================================
// 第三部分：深入理解（可选）
// ============================================

/*
==============================================
🔍 深入理解：虚函数表的内存布局（选学）
==============================================

单继承的情况：
class Base {
    virtual void func1();
    virtual void func2();
};

class Derived : public Base {
    void func1() override;  // 重写
    virtual void func3();   // 新增虚函数
};

Base 对象内存布局：
┌─────────────┐
│   vptr      │ → Base::vtable
├─────────────┤
│ 数据成员    │
└─────────────┘

Base::vtable:
┌─────────────┐
│ Base::func1 │
├─────────────┤
│ Base::func2 │
└─────────────┘

Derived 对象内存布局：
┌─────────────┐
│   vptr      │ → Derived::vtable
├─────────────┤
│ Base数据    │
├─────────────┤
│ Derived数据 │
└─────────────┘

Derived::vtable:
┌──────────────────┐
│ Derived::func1   │ ← 重写的函数
├──────────────────┤
│ Base::func2      │ ← 继承的函数
├──────────────────┤
│ Derived::func3   │ ← 新增的函数
└──────────────────┘

动态绑定的过程：
Base* p = new Derived();
p->func1();

1. p 指向 Derived 对象
2. 通过 p 的 vptr 找到 Derived::vtable
3. 在 vtable 中找到 func1 的地址（Derived::func1）
4. 调用 Derived::func1()

为什么通过对象调用不是动态绑定？
Derived d;
d.func1();  // 静态绑定

- 编译期就知道 d 的类型是 Derived
- 直接调用 Derived::func1()，不需要通过 vtable
- 效率更高

==============================================
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

// --------------------------------------------
// 4.1 错误1：忘记虚析构函数
// --------------------------------------------

/*
❌ 错误代码：
class Base {
public:
    ~Base() { }  // 非虚析构函数
};

class Derived : public Base {
public:
    ~Derived() { delete[] data; }
private:
    int* data;
};

Base* p = new Derived();
delete p;  // ❌ 问题：只调用~Base()，不调用~Derived()

结果：内存泄漏！派生类的资源没有释放

✅ 正确代码：
class Base {
public:
    virtual ~Base() { }  // 虚析构函数
};

delete p;  // ✅ 正确：先调用~Derived()，再调用~Base()

记忆技巧：
- 只要类会被继承，析构函数就应该是虚函数
- 如果类中有虚函数，析构函数也应该是虚函数
- 这是C++最重要的规则之一！
*/

// --------------------------------------------
// 4.2 错误2：虚函数重写时签名不一致
// --------------------------------------------

/*
❌ 错误代码：
class Base {
public:
    virtual void func(int x) { }
};

class Derived : public Base {
public:
    void func(double x) { }  // ❌ 参数类型不同，不是重写！
};

Base* p = new Derived();
p->func(10);  // 调用的是 Base::func(int)，不是 Derived::func(double)

结果：没有实现多态，调用了基类的函数

✅ 正确代码：
class Derived : public Base {
public:
    void func(int x) override {  // ✅ 使用 override 关键字
        // 编译器会检查是否正确重写
    }
};

记忆技巧：
- 重写虚函数时，函数签名必须完全一致
- 使用 override 关键字，让编译器帮你检查
- 参数类型、个数、const 修饰都要一致
*/

// --------------------------------------------
// 4.3 错误3：通过对象调用虚函数（没有动态绑定）
// --------------------------------------------

/*
❌ 错误理解：
"只要是虚函数，就会有动态绑定"

class Base {
public:
    virtual void func() { cout << "Base" << endl; }
};

class Derived : public Base {
public:
    void func() override { cout << "Derived" << endl; }
};

Derived d;
d.func();  // 输出：Derived

有人认为：这是动态绑定

✅ 正确理解：
这是静态绑定，不是动态绑定！

原因：
- 编译期就知道 d 的类型是 Derived
- 直接调用 Derived::func()，不需要通过 vtable
- 虽然是虚函数，但没有通过基类指针/引用调用

动态绑定的条件：
1. 必须是虚函数
2. 必须通过基类指针或引用调用
3. 两个条件缺一不可

记忆技巧：
- 对象调用：静态绑定（编译期确定）
- 指针/引用调用：动态绑定（运行期确定）
*/

// --------------------------------------------
// 4.4 错误4：构造函数中调用虚函数（静态绑定）
// --------------------------------------------

/*
❌ 错误代码：
class Base {
public:
    Base() {
        init();  // 在构造函数中调用虚函数
    }

    virtual void init() {
        cout << "Base::init()" << endl;
    }
};

class Derived : public Base {
public:
    Derived() {
        // Base() 已经调用完了
    }

    void init() override {
        cout << "Derived::init()" << endl;
    }
};

Derived d;  // 输出：Base::init()（不是 Derived::init()）

结果：构造函数中调用虚函数，是静态绑定，不会调用派生类的版本

原因：
- 创建 Derived 对象时，先调用 Base()
- 在 Base() 中，派生类部分还没有构造完成
- 此时对象的类型是 Base，不是 Derived
- 所以调用 Base::init()，而不是 Derived::init()

✅ 正确做法：
不要在构造函数中调用虚函数，或者明确知道会调用哪个版本

记忆技巧：
- 构造函数中调用虚函数：静态绑定
- 对象完全构造完成后：动态绑定
- 同理，析构函数中调用虚函数也是静态绑定
*/

// --------------------------------------------
// 4.5 错误5：切片问题（对象切割）
// --------------------------------------------

/*
❌ 错误代码：
class Base {
public:
    virtual void func() { cout << "Base" << endl; }
};

class Derived : public Base {
public:
    void func() override { cout << "Derived" << endl; }
    int extraData;
};

Derived d;
Base b = d;  // ❌ 对象切片：派生类对象赋值给基类对象
b.func();    // 输出：Base（不是 Derived）

结果：派生类部分被"切掉"了，只保留基类部分

原因：
- Base b = d; 调用 Base 的拷贝构造函数
- 只拷贝基类部分，派生类部分丢失
- b 的类型是 Base，不是 Derived
- b.func() 调用 Base::func()

✅ 正确代码：
使用指针或引用，避免对象切片

Base* p = &d;   // ✅ 指针
p->func();      // 输出：Derived

Base& r = d;    // ✅ 引用
r.func();       // 输出：Derived

记忆技巧：
- 对象赋值：切片（只保留基类部分）
- 指针/引用：多态（保留完整对象）
*/

// --------------------------------------------
// 4.6 错误6：纯虚函数没有实现导致链接错误
// --------------------------------------------

/*
❌ 错误代码：
class Base {
public:
    virtual void func() = 0;  // 纯虚函数
};

class Derived : public Base {
    // ❌ 忘记实现 func()
};

Derived d;  // ❌ 编译错误：Derived 仍然是抽象类

结果：派生类必须实现所有纯虚函数，否则也是抽象类

✅ 正确代码：
class Derived : public Base {
public:
    void func() override {  // ✅ 实现纯虚函数
        // ...
    }
};

记忆技巧：
- 纯虚函数 = 0：必须由派生类实现
- 忘记实现：派生类也是抽象类，不能实例化
*/

// ============================================
// 第五部分：关键知识点速查
// ============================================

/*
【知识点速查卡片】

────────────────────────────────────
【1. 虚函数语法】⭐⭐⭐⭐⭐

class Base {
public:
    virtual void func();        // 虚函数声明
    virtual ~Base();            // 虚析构函数
};

class Derived : public Base {
public:
    void func() override;       // 重写（C++11推荐）
};

────────────────────────────────────
【2. 纯虚函数和抽象类】⭐⭐⭐⭐⭐

class Abstract {
public:
    virtual void func() = 0;    // 纯虚函数
    virtual ~Abstract() {}      // 虚析构函数
};

// Abstract a;  // ❌ 错误：抽象类不能实例化

────────────────────────────────────
【3. 动态绑定条件】⭐⭐⭐⭐⭐

必须同时满足：
1. 必须是虚函数
2. 必须通过基类指针或引用调用

Base* p = new Derived();
p->func();  // ✅ 动态绑定

Derived d;
d.func();   // ❌ 静态绑定（虽然是虚函数）

────────────────────────────────────
【4. 虚函数表原理】⭐⭐⭐⭐⭐

每个对象包含：
- vptr（虚函数指针）→ 指向类的 vtable

每个类包含：
- vtable（虚函数表）→ 存储虚函数地址

动态绑定过程：
对象 → vptr → vtable → 虚函数地址 → 调用

────────────────────────────────────
【5. 虚析构函数规则】⭐⭐⭐⭐⭐

基类析构函数必须是虚函数！

class Base {
public:
    virtual ~Base() {}  // ✅ 必须
};

原因：防止内存泄漏

────────────────────────────────────
【6. 虚函数重写规则】

必须完全一致：
- 函数名相同
- 参数列表相同
- 返回类型相同
- const 修饰相同

建议使用 override 关键字

────────────────────────────────────
*/

// ============================================
// 第六部分：练习题
// ============================================

/*
1. 虚函数表理解题

   给定以下代码：
   class A {
   public:
       virtual void f1() {}
       virtual void f2() {}
   };

   class B : public A {
   public:
       void f1() override {}
       virtual void f3() {}
   };

   问题：
   (1) A 对象的内存布局是什么？
   (2) B 对象的内存布局是什么？
   (3) B::vtable 中有哪些函数？
   (4) sizeof(A) 和 sizeof(B) 分别是多少？

   提示：画出内存布局图
   考察点：虚函数表、对象内存布局

2. 动态绑定判断题

   给定以下代码：
   class Base {
   public:
       virtual void func() { cout << "Base" << endl; }
   };

   class Derived : public Base {
   public:
       void func() override { cout << "Derived" << endl; }
   };

   问题：下列调用输出什么？
   (1) Derived d; d.func();
   (2) Base* p = new Derived(); p->func();
   (3) Base b = Derived(); b.func();
   (4) Base& r = Derived(); r.func();

   提示：区分静态绑定和动态绑定
   考察点：动态绑定条件、对象切片

3. 虚析构函数实践

   要求：设计一个图形类层次
   - 基类：Shape（虚析构函数）
   - 派生类：Circle（动态分配内存）
   - 派生类：Rectangle（动态分配内存）

   问题：
   (1) 如果 Shape 的析构函数不是虚函数，会有什么问题？
   (2) 如何验证虚析构函数的正确性？
   (3) 编写代码演示内存泄漏和正确释放

   提示：在析构函数中输出日志
   考察点：虚析构函数、内存管理

4. 抽象类设计

   要求：设计一个支付系统
   - 抽象类：Payment（纯虚函数 pay）
   - 派生类：CreditCardPayment
   - 派生类：AlipayPayment
   - 派生类：WeChatPayment

   问题：
   (1) Payment 类应该有哪些纯虚函数？
   (2) 如何使用多态处理不同的支付方式？
   (3) 如果忘记实现纯虚函数会怎样？

   提示：使用基类指针存储不同支付方式
   考察点：抽象类、接口设计、多态

5. 构造函数中的虚函数

   给定以下代码：
   class Base {
   public:
       Base() { init(); }
       virtual void init() { cout << "Base::init()" << endl; }
   };

   class Derived : public Base {
   public:
       Derived() { init(); }
       void init() override { cout << "Derived::init()" << endl; }
   };

   问题：
   (1) Derived d; 会输出什么？
   (2) 为什么 Base() 中调用的是 Base::init()？
   (3) 如何才能在构造时调用 Derived::init()？

   提示：构造函数中调用虚函数是静态绑定
   考察点：构造函数、虚函数、动态绑定时机
*/

// ============================================
// 主函数：测试所有示例
// ============================================

int main() {
    cout << "========================================" << endl;
    cout << "   C++ 多态与虚函数机制 - 实践演示" << endl;
    cout << "========================================" << endl;

    // --------------------------------------------
    // 测试1：静态绑定 vs 动态绑定
    // --------------------------------------------
    cout << "\n【测试1：静态绑定】" << endl;
    testStaticBinding();

    cout << "\n【测试1：动态绑定】" << endl;
    testDynamicBinding();

    // --------------------------------------------
    // 测试2：通过基类引用调用
    // --------------------------------------------
    Circle c(5.0);
    Rectangle r(4.0, 6.0);

    printShapeInfo(c);  // 动态绑定
    printShapeInfo(r);  // 动态绑定

    // --------------------------------------------
    // 测试3：虚函数表原理（对象大小）
    // --------------------------------------------
    testVtableSize();

    // --------------------------------------------
    // 测试4：纯虚函数和抽象类
    // --------------------------------------------
    cout << "\n【测试4：纯虚函数和抽象类】" << endl;

    // Animal a;  // ❌ 错误：抽象类不能实例化

    Dog dog;
    Cat cat;

    cout << "\n狗：" << endl;
    testAnimal(dog);

    cout << "\n猫：" << endl;
    testAnimal(cat);

    // 使用多态存储不同的动物
    cout << "\n【多态数组】" << endl;
    vector<Animal*> animals;
    animals.push_back(new Dog());
    animals.push_back(new Cat());

    for (Animal* a : animals) {
        a->makeSound();
        a->move();
    }

    // 释放内存
    for (Animal* a : animals) {
        delete a;  // 虚析构函数确保正确释放
    }

    // --------------------------------------------
    // 测试5：虚析构函数的重要性
    // --------------------------------------------
    testVirtualDestructor();

    cout << "\n========================================" << endl;
    cout << "   所有测试完成！" << endl;
    cout << "========================================" << endl;

    return 0;
}

/*
【编译和运行】

编译命令：
g++ -o 07_polymorphism 07_polymorphism.cpp -std=c++11

运行命令：
./07_polymorphism

预期输出：
- 静态绑定和动态绑定的对比
- 对象大小的差异（有无 vptr）
- 虚函数的动态绑定效果
- 抽象类和纯虚函数的使用
- 虚析构函数的重要性演示
*/

