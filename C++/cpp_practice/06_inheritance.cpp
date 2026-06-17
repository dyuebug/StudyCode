// ============================================
// 继承机制 - 超详细注释版
// 日期：2026-06-19（第6天）
// 难度：⭐⭐⭐ 中高级
// 参考资料：CppFiles/CppBase/7.继承.md
// ============================================

/*
【核心概念】继承（Inheritance）

是什么？
- 继承是面向对象编程的三大特性之一（封装、继承、多态）
- 通过继承，可以用原有类型定义新类型，新类型包含原有类型的成员
- 原有类型称为"基类"或"父类"，新类型称为"派生类"或"子类"

为什么需要？
- 代码复用：避免重复编写相同的代码
- 建立类型层次：表达"is-a"关系（学生是一个人，老师是一个人）
- 实现多态：为动态绑定提供基础

核心特点：
- 派生类继承基类的成员（数据成员和成员函数）
- 派生类可以添加新成员
- 派生类可以隐藏基类成员
- 三种继承方式：public、protected、private
*/

#include <iostream>
#include <string>
using namespace std;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

// --------------------------------------------
// 1.1 继承的基本语法
// --------------------------------------------

/*
【继承语法】

class 基类 {
    // 基类成员
};

class 派生类 : 继承方式 基类 {
    // 派生类成员
};

三种继承方式：
- public：公有继承（最常用）
- protected：保护继承
- private：私有继承
*/

// --------------------------------------------
// 1.2 三种继承方式的访问权限 ⭐⭐⭐⭐⭐
// --------------------------------------------

/*
【访问权限规则】（面试高频考点！）

记忆技巧：继承方式和基类成员访问权限做交集
private < protected < public（权限递增）

核心规则：
1. 私有成员在派生类内部和类外都无法访问
2. 非私有成员在派生类内部都可以访问
3. 只有公有继承的公有成员可以在派生类对象外访问

访问权限表格（重点记忆）：

基类成员    | public继承  | protected继承 | private继承
----------- | ----------- | ------------- | -----------
public      | public      | protected     | private
protected   | protected   | protected     | private
private     | 不可访问    | 不可访问      | 不可访问

派生类内部访问：
- 基类的private成员：不可访问（任何继承方式）
- 基类的非private成员：都可以访问（任何继承方式）

派生类对象在类外访问：
- 只有public继承的public成员可以访问
- 其他情况都不可访问
*/

// --------------------------------------------
// 1.3 派生类对象的构造和析构顺序 ⭐⭐⭐⭐⭐
// --------------------------------------------

/*
【构造和析构顺序】

重要结论：
创建派生类对象，一定会先调用派生类的构造函数，
在此过程中会先去调用基类的构造函数。

构造顺序：基类构造 → 对象成员构造 → 派生类构造
析构顺序：派生类析构 → 对象成员析构 → 基类析构（完全相反）

派生类初始化列表：
Derived(int x, int y)
: Base(x)        // 显式调用基类构造函数
, _member(y)     // 初始化对象成员
{
    // 派生类构造函数体
}

注意事项：
1. 如果不显式调用基类构造函数，默认调用基类的无参构造
2. 如果基类没有无参构造，必须在派生类初始化列表中显式调用基类构造
3. 构造顺序由继承关系决定，不由初始化列表顺序决定
*/

// --------------------------------------------
// 1.4 菱形继承和虚继承 ⭐⭐⭐⭐⭐
// --------------------------------------------

/*
【菱形继承问题】

继承结构：
       A (顶层基类)
      / \
     B   C (中间层)
      \ /
       D (底层派生类)

问题1：数据冗余
- D 中包含两份 A 的数据成员（通过 B 继承一份，通过 C 继承一份）

问题2：二义性
- 访问 A 的成员时，不知道是通过 B 还是通过 C
- 必须使用作用域解析符：d.B::member 或 d.C::member

【虚继承解决方案】

语法：
class B : virtual public A { };  // 虚继承
class C : virtual public A { };  // 虚继承
class D : public B, public C { };

效果：
- D 中只包含一份 A 的数据成员
- 无需作用域解析符，直接访问：d.member

原理：
- B 和 C 不直接包含 A 的数据成员
- B 和 C 包含虚基类指针（指向 A 的位置）
- D 负责初始化虚基类 A

虚基类的构造顺序：
虚基类构造 → 非虚基类构造 → 对象成员构造 → 派生类构造
*/

// ============================================
// 第二部分：基础操作实践
// ============================================

// --------------------------------------------
// 2.1 基础继承示例：Person → Student
// --------------------------------------------

// 基类：Person（人）
class Person {
public:
    // 构造函数：初始化姓名和年龄
    // 参数1：name - string类型，表示姓名
    // 参数2：age - int类型，表示年龄
    Person(const string& name, int age)
    : _name(name)
    , _age(age)
    {
        cout << "Person构造函数：" << _name << endl;
    }

    // 析构函数
    ~Person() {
        cout << "Person析构函数：" << _name << endl;
    }

    // 公有成员函数：显示基本信息
    void display() const {
        cout << "姓名：" << _name << ", 年龄：" << _age << endl;
    }

protected:
    // 保护成员：派生类可以访问
    string _name;
    int _age;

private:
    // 私有成员：派生类不可访问
    string _idCard;  // 身份证号
};

// 派生类：Student（学生） - 公有继承
class Student : public Person {
public:
    // 构造函数：初始化基类和派生类成员
    // 参数1：name - 姓名（传递给基类）
    // 参数2：age - 年龄（传递给基类）
    // 参数3：studentId - 学号（派生类新增）
    // 参数4：score - 成绩（派生类新增）
    Student(const string& name, int age, const string& studentId, double score)
    : Person(name, age)    // 显式调用基类构造函数
    , _studentId(studentId)
    , _score(score)
    {
        cout << "Student构造函数：" << _studentId << endl;
    }

    // 析构函数
    ~Student() {
        cout << "Student析构函数：" << _studentId << endl;
    }

    // 派生类新增成员函数
    void showInfo() const {
        // 访问基类的protected成员
        cout << "学生信息 - 姓名：" << _name
             << ", 年龄：" << _age
             << ", 学号：" << _studentId
             << ", 成绩：" << _score << endl;

        // 注意：不能访问基类的private成员_idCard
        // cout << _idCard;  // ❌ 编译错误
    }

private:
    string _studentId;  // 学号
    double _score;      // 成绩
};

// --------------------------------------------
// 2.2 三种继承方式对比
// --------------------------------------------

// 基类
class Base {
public:
    int publicMember;
protected:
    int protectedMember;
private:
    int privateMember;
};

// 公有继承
class PublicDerived : public Base {
public:
    void test() {
        publicMember = 1;       // ✅ 可以访问
        protectedMember = 2;    // ✅ 可以访问
        // privateMember = 3;   // ❌ 不可访问（基类私有成员）
    }
};

// 保护继承
class ProtectedDerived : protected Base {
public:
    void test() {
        publicMember = 1;       // ✅ 可以访问
        protectedMember = 2;    // ✅ 可以访问
        // privateMember = 3;   // ❌ 不可访问（基类私有成员）
    }
};

// 私有继承
class PrivateDerived : private Base {
public:
    void test() {
        publicMember = 1;       // ✅ 可以访问
        protectedMember = 2;    // ✅ 可以访问
        // privateMember = 3;   // ❌ 不可访问（基类私有成员）
    }
};

// --------------------------------------------
// 2.3 构造和析构顺序演示 ⭐⭐⭐⭐⭐
// --------------------------------------------

// 基类：带构造和析构日志
class BaseClass {
public:
    BaseClass() {
        cout << "1. BaseClass 构造函数" << endl;
    }

    ~BaseClass() {
        cout << "4. BaseClass 析构函数" << endl;
    }
};

// 对象成员类
class Member {
public:
    Member() {
        cout << "2. Member 构造函数" << endl;
    }

    ~Member() {
        cout << "3. Member 析构函数" << endl;
    }
};

// 派生类：包含对象成员
class DerivedClass : public BaseClass {
public:
    DerivedClass()
    : BaseClass()    // 先调用基类构造
    , _member()      // 再调用对象成员构造
    {
        cout << "3. DerivedClass 构造函数" << endl;
    }

    ~DerivedClass() {
        cout << "2. DerivedClass 析构函数" << endl;
    }

private:
    Member _member;  // 对象成员
};

// --------------------------------------------
// 2.4 多重继承示例
// --------------------------------------------

// 接口1：会飞的
class Flyable {
public:
    void fly() {
        cout << "我会飞！" << endl;
    }
};

// 接口2：会游泳的
class Swimmable {
public:
    void swim() {
        cout << "我会游泳！" << endl;
    }
};

// 多重继承：鸭子既会飞又会游泳
// 语法：class 派生类 : public 基类1, public 基类2
class Duck : public Flyable, public Swimmable {
public:
    void quack() {
        cout << "嘎嘎嘎！" << endl;
    }
};

// --------------------------------------------
// 2.5 菱形继承和虚继承 ⭐⭐⭐⭐⭐
// --------------------------------------------

// 顶层基类：动物
class Animal {
public:
    Animal(const string& name = "未命名")
    : _name(name)
    {
        cout << "Animal 构造：" << _name << endl;
    }

    void eat() {
        cout << _name << " 在吃东西" << endl;
    }

protected:
    string _name;
};

// 问题演示：非虚继承（会产生数据冗余）
class Bird : public Animal {
public:
    Bird(const string& name) : Animal(name) {
        cout << "Bird 构造" << endl;
    }
};

class Beast : public Animal {
public:
    Beast(const string& name) : Animal(name) {
        cout << "Beast 构造" << endl;
    }
};

// 蝙蝠：既是鸟又是兽（菱形继承问题）
class Bat : public Bird, public Beast {
public:
    Bat(const string& name)
    : Bird(name)
    , Beast(name)
    {
        cout << "Bat 构造" << endl;
    }

    // 访问_name会有二义性
    void show() {
        // cout << _name;  // ❌ 编译错误：二义性
        cout << "通过Bird访问：" << Bird::_name << endl;
        cout << "通过Beast访问：" << Beast::_name << endl;
    }
};

// 解决方案：虚继承
class VirtualBird : virtual public Animal {
public:
    VirtualBird(const string& name) : Animal(name) {
        cout << "VirtualBird 构造" << endl;
    }
};

class VirtualBeast : virtual public Animal {
public:
    VirtualBeast(const string& name) : Animal(name) {
        cout << "VirtualBeast 构造" << endl;
    }
};

// 虚继承的蝙蝠：只有一份Animal数据
class VirtualBat : public VirtualBird, public VirtualBeast {
public:
    // 重要：虚基类由最底层派生类负责初始化
    VirtualBat(const string& name)
    : Animal(name)           // 直接初始化虚基类
    , VirtualBird(name)      // 这里的调用会被忽略
    , VirtualBeast(name)     // 这里的调用会被忽略
    {
        cout << "VirtualBat 构造" << endl;
    }

    void show() {
        // 无需作用域解析符，直接访问
        cout << "虚继承访问：" << _name << endl;
    }
};

// ============================================
// 第三部分：深入理解（可选）
// ============================================

/*
==============================================
🔍 深入理解：虚继承的内存布局（选学）
==============================================

普通菱形继承的内存布局：
Bat对象包含：
- Bird部分（包含Animal的_name）
- Beast部分（包含Animal的_name）
- Bat自己的成员

问题：两份Animal数据，内存浪费

虚继承的内存布局：
VirtualBat对象包含：
- VirtualBird部分（包含虚基类指针，指向Animal）
- VirtualBeast部分（包含虚基类指针，指向Animal）
- Animal部分（只有一份）
- VirtualBat自己的成员

原理：
- VirtualBird和VirtualBeast不直接包含Animal数据
- 它们包含虚基类指针（vbptr），指向共享的Animal部分
- 增加了一层间接性，但解决了数据冗余和二义性

==============================================
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

// --------------------------------------------
// 4.1 错误1：忘记调用基类构造函数
// --------------------------------------------

/*
❌ 错误代码：
class Base {
public:
    Base(int x) { _x = x; }  // 基类有有参构造，没有无参构造
private:
    int _x;
};

class Derived : public Base {
public:
    Derived(int y) : _y(y) { }  // ❌ 错误：没有调用基类构造
private:
    int _y;
};

结果：编译错误 - no matching function for call to 'Base::Base()'

原因：
- 派生类构造时，如果不显式调用基类构造，默认调用基类无参构造
- 但基类没有无参构造函数

✅ 正确代码：
class Derived : public Base {
public:
    Derived(int x, int y)
    : Base(x)    // 显式调用基类有参构造
    , _y(y)
    { }
private:
    int _y;
};

记忆技巧：
- 基类有有参构造，派生类必须显式调用
- 在派生类初始化列表中调用基类构造函数
*/

// --------------------------------------------
// 4.2 错误2：访问基类私有成员
// --------------------------------------------

/*
❌ 错误代码：
class Base {
private:
    int _privateData;
};

class Derived : public Base {
public:
    void test() {
        _privateData = 10;  // ❌ 编译错误：不能访问基类私有成员
    }
};

结果：编译错误 - '_privateData' is a private member of 'Base'

原因：
- 基类的私有成员，派生类不可访问（无论什么继承方式）
- 封装性原则：私有成员只能在定义它的类内部访问

✅ 正确代码：
class Base {
protected:  // 改为protected
    int _protectedData;
public:
    void setPrivateData(int x) { _privateData = x; }  // 提供公有接口
private:
    int _privateData;
};

class Derived : public Base {
public:
    void test() {
        _protectedData = 10;  // ✅ 可以访问protected成员
        setPrivateData(20);   // ✅ 通过公有接口访问private成员
    }
};

记忆技巧：
- private：只有自己能访问
- protected：自己和子类能访问
- public：所有人都能访问
*/

// --------------------------------------------
// 4.3 错误3：混淆继承方式的访问权限
// --------------------------------------------

/*
❌ 错误理解：
"protected继承后，基类的public成员变成protected，
 所以派生类对象在类外不能访问"

这是对的！但很多人会误以为"派生类内部也不能访问"。

class Base {
public:
    int publicData;
};

class Derived : protected Base {  // protected继承
public:
    void test() {
        publicData = 10;  // ✅ 派生类内部可以访问
    }
};

void func() {
    Derived d;
    d.publicData = 20;  // ❌ 类外不能访问（publicData变成protected了）
}

结果：编译错误 - 'publicData' is a protected member of 'Base'

✅ 正确理解：
继承方式影响的是"在类外通过派生类对象访问"的权限，
不影响"在派生类内部访问"的权限。

记忆技巧：
- 派生类内部：只要不是private，都能访问
- 派生类对象在类外：看继承方式和访问权限的交集
*/

// --------------------------------------------
// 4.4 错误4：菱形继承忘记使用虚继承
// --------------------------------------------

/*
❌ 错误代码：
class Animal {
public:
    int age;
};

class Bird : public Animal { };  // 非虚继承
class Beast : public Animal { }; // 非虚继承

class Bat : public Bird, public Beast {
public:
    void setAge(int a) {
        age = a;  // ❌ 编译错误：二义性（不知道是Bird::age还是Beast::age）
    }
};

结果：编译错误 - member 'age' found in multiple base classes

原因：
- Bat包含两份Animal数据（通过Bird一份，通过Beast一份）
- 访问age时产生二义性

✅ 正确代码：
class Bird : virtual public Animal { };  // 虚继承
class Beast : virtual public Animal { }; // 虚继承

class Bat : public Bird, public Beast {
public:
    void setAge(int a) {
        age = a;  // ✅ 正确：只有一份Animal数据
    }
};

记忆技巧：
- 菱形继承必须使用虚继承
- virtual关键字写在中间层（Bird和Beast）
- 虚继承解决数据冗余和二义性
*/

// --------------------------------------------
// 4.5 错误5：构造顺序和初始化列表顺序混淆
// --------------------------------------------

/*
❌ 错误理解：
"初始化列表的顺序决定构造顺序"

class Base { };
class Member { };

class Derived : public Base {
public:
    Derived()
    : _member()   // 写在前面
    , Base()      // 写在后面
    { }
private:
    Member _member;
};

有人认为：先构造_member，再构造Base

✅ 正确理解：
构造顺序由继承关系和成员声明顺序决定，不由初始化列表顺序决定

实际构造顺序：
1. Base构造（基类总是先构造）
2. _member构造（按成员声明顺序）
3. Derived构造

记忆技巧：
- 基类 → 对象成员（按声明顺序）→ 派生类
- 初始化列表的顺序不影响构造顺序
- 但建议初始化列表顺序和实际构造顺序一致，便于阅读
*/

// --------------------------------------------
// 4.6 错误6：虚继承时忘记在最底层初始化虚基类
// --------------------------------------------

/*
❌ 错误代码：
class Animal {
public:
    Animal(const string& name) : _name(name) { }
private:
    string _name;
};

class Bird : virtual public Animal {
public:
    Bird(const string& name) : Animal(name) { }
};

class Beast : virtual public Animal {
public:
    Beast(const string& name) : Animal(name) { }
};

class Bat : public Bird, public Beast {
public:
    Bat(const string& name)
    : Bird(name)    // ❌ 错误：虚基类不会被初始化
    , Beast(name)   // ❌ 错误：虚基类不会被初始化
    { }
};

结果：编译错误 - no matching constructor for initialization of 'Animal'

原因：
- 虚基类由最底层派生类负责初始化
- Bird和Beast的初始化调用会被忽略

✅ 正确代码：
class Bat : public Bird, public Beast {
public:
    Bat(const string& name)
    : Animal(name)   // ✅ 正确：直接初始化虚基类
    , Bird(name)     // 这里的调用会被忽略
    , Beast(name)    // 这里的调用会被忽略
    { }
};

记忆技巧：
- 虚基类由最底层派生类直接初始化
- 中间层的虚基类初始化会被忽略（但仍需写上，避免编译错误）
*/

// ============================================
// 第五部分：关键知识点速查
// ============================================

/*
【知识点速查卡片】

────────────────────────────────────
【1. 继承语法】

class Derived : 继承方式 Base {
    // 派生类成员
};

三种继承方式：
- public：公有继承（最常用，is-a关系）
- protected：保护继承
- private：私有继承

────────────────────────────────────
【2. 访问权限表格】⭐⭐⭐⭐⭐

基类成员    | public继承  | protected继承 | private继承
----------- | ----------- | ------------- | -----------
public      | public      | protected     | private
protected   | protected   | protected     | private
private     | 不可访问    | 不可访问      | 不可访问

记忆技巧：继承方式和基类成员访问权限做交集

────────────────────────────────────
【3. 构造和析构顺序】⭐⭐⭐⭐⭐

构造：基类 → 对象成员 → 派生类
析构：派生类 → 对象成员 → 基类（完全相反）

虚继承时：
构造：虚基类 → 非虚基类 → 对象成员 → 派生类

────────────────────────────────────
【4. 多重继承语法】

class C : public A, public B {
    // 同时继承A和B
};

────────────────────────────────────
【5. 虚继承语法】⭐⭐⭐⭐⭐

class B : virtual public A { };
class C : virtual public A { };
class D : public B, public C { };

作用：解决菱形继承的数据冗余和二义性

────────────────────────────────────
【6. 派生类初始化列表】

Derived(int x, int y)
: Base(x)        // 调用基类构造
, _member(y)     // 初始化对象成员
{
    // 派生类构造函数体
}

────────────────────────────────────
*/

// ============================================
// 第六部分：练习题
// ============================================

/*
1. 访问权限判断题

   给定以下代码：
   class Base {
   public:
       int a;
   protected:
       int b;
   private:
       int c;
   };

   class Derived : protected Base {
   public:
       void test();
   };

   问题：
   (1) 在Derived::test()中，能访问a、b、c吗？
   (2) 在main函数中，通过Derived对象能访问a、b、c吗？
   (3) 如果改为public继承，答案有何不同？

   提示：回顾访问权限表格
   考察点：继承方式和访问权限

2. 构造顺序判断题

   给定以下代码：
   class A { public: A() { cout << "A"; } };
   class B { public: B() { cout << "B"; } };
   class C : public A {
   public:
       C() : _b(), A() { cout << "C"; }
   private:
       B _b;
   };

   问题：创建C对象时，输出是什么？

   提示：构造顺序不由初始化列表顺序决定
   考察点：构造顺序

3. 菱形继承问题

   要求：设计一个菱形继承结构
   - 顶层：Person（姓名、年龄）
   - 中层：Student（学号）、Teacher（工号）
   - 底层：TA（教学助理，既是学生又是老师）

   问题：
   (1) 如果不使用虚继承，会有什么问题？
   (2) 如何使用虚继承解决问题？
   (3) TA的构造函数应该如何写？

   提示：TA对象应该只包含一份Person数据
   考察点：菱形继承、虚继承

4. 多重继承实践

   要求：设计一个智能手机类
   - 接口1：Phone（打电话）
   - 接口2：Camera（拍照）
   - 接口3：MusicPlayer（播放音乐）
   - 类：SmartPhone（继承所有接口）

   问题：
   (1) 如何定义这些类？
   (2) SmartPhone如何继承多个接口？
   (3) 如果Phone和Camera都有display()方法，如何解决冲突？

   提示：使用作用域解析符
   考察点：多重继承、成员名冲突

5. 构造函数调用

   给定以下代码：
   class Base {
   public:
       Base(int x) { _x = x; }
   private:
       int _x;
   };

   class Derived : public Base {
   public:
       Derived(int x, int y);  // 如何实现？
   private:
       int _y;
   };

   问题：实现Derived的构造函数，要求：
   (1) 正确初始化基类
   (2) 正确初始化_y
   (3) 使用初始化列表

   提示：基类没有无参构造，必须显式调用
   考察点：派生类构造函数、初始化列表
*/

// ============================================
// 主函数：测试所有示例
// ============================================

int main() {
    cout << "========================================" << endl;
    cout << "   C++ 继承机制 - 实践演示" << endl;
    cout << "========================================" << endl;

    // --------------------------------------------
    // 测试1：基础继承
    // --------------------------------------------
    cout << "\n【测试1：基础继承 - Student】" << endl;
    Student s1("张三", 20, "2024001", 95.5);
    s1.showInfo();
    s1.display();  // 调用基类方法

    cout << "\n访问权限测试：" << endl;
    PublicDerived pd;
    pd.publicMember = 1;  // ✅ public继承，可以在类外访问public成员
    // pd.protectedMember = 2;  // ❌ protected成员不能在类外访问

    ProtectedDerived prd;
    // prd.publicMember = 1;  // ❌ protected继承，public成员变成protected

    PrivateDerived pvd;
    // pvd.publicMember = 1;  // ❌ private继承，public成员变成private

    // --------------------------------------------
    // 测试2：构造和析构顺序
    // --------------------------------------------
    cout << "\n【测试2：构造和析构顺序】" << endl;
    cout << "创建DerivedClass对象：" << endl;
    {
        DerivedClass dc;
    }
    cout << "DerivedClass对象销毁完成" << endl;

    // --------------------------------------------
    // 测试3：多重继承
    // --------------------------------------------
    cout << "\n【测试3：多重继承 - Duck】" << endl;
    Duck duck;
    duck.fly();    // 来自Flyable
    duck.swim();   // 来自Swimmable
    duck.quack();  // Duck自己的方法

    // --------------------------------------------
    // 测试4：菱形继承问题演示
    // --------------------------------------------
    cout << "\n【测试4：菱形继承问题】" << endl;
    cout << "非虚继承的Bat：" << endl;
    Bat bat("蝙蝠");
    bat.show();  // 有两份Animal数据

    cout << "\n虚继承的VirtualBat：" << endl;
    VirtualBat vbat("虚继承蝙蝠");
    vbat.show();  // 只有一份Animal数据

    cout << "\n========================================" << endl;
    cout << "   所有测试完成！" << endl;
    cout << "========================================" << endl;

    return 0;
}

/*
【编译和运行】

编译命令：
g++ -o 06_inheritance 06_inheritance.cpp -std=c++11

运行命令：
./06_inheritance

预期输出：
- Student对象的构造和析构顺序
- 访问权限的验证
- 构造析构顺序的演示
- 多重继承的功能演示
- 菱形继承问题和虚继承解决方案的对比
*/

