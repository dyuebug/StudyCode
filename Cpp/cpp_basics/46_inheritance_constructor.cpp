// 46_inheritance_constructor.cpp
// 继承中的构造函数与析构函数：顺序、初始化列表、多级继承

#include <iostream>
#include <string>
using namespace std;

// ============================================================
// 第一部分：构造/析构顺序演示
// ============================================================

class Base {
public:
    string name;

    Base(const string& n) : name(n) {
        cout << "[构造] Base(" << name << ")" << endl;
    }

    ~Base() {
        cout << "[析构] ~Base(" << name << ")" << endl;
    }
};

class Derived : public Base {
public:
    int value;

    // 必须通过初始化列表调用父类构造函数
    Derived(const string& n, int v) : Base(n), value(v) {
        cout << "[构造] Derived(" << name << ", " << value << ")" << endl;
    }

    ~Derived() {
        cout << "[析构] ~Derived(" << name << ", " << value << ")" << endl;
    }
};

// ============================================================
// 第二部分：多级继承的顺序
// ============================================================

class A {
public:
    A()  { cout << "[构造] A" << endl; }
    ~A() { cout << "[析构] ~A" << endl; }
};

class B : public A {
public:
    B()  { cout << "[构造] B" << endl; }
    ~B() { cout << "[析构] ~B" << endl; }
};

class C : public B {
public:
    C()  { cout << "[构造] C" << endl; }
    ~C() { cout << "[析构] ~C" << endl; }
};

// ============================================================
// 第三部分：子类构造函数调用父类指定构造函数
// ============================================================

class Person {
protected:
    string name;
    int age;

public:
    // 默认构造函数
    Person() : name("未知"), age(0) {
        cout << "[构造] Person()" << endl;
    }

    // 带参构造函数
    Person(const string& n, int a) : name(n), age(a) {
        cout << "[构造] Person(" << name << ", " << age << ")" << endl;
    }

    ~Person() {
        cout << "[析构] ~Person(" << name << ")" << endl;
    }

    void info() const {
        cout << "姓名：" << name << "，年龄：" << age << endl;
    }
};

class Student : public Person {
private:
protected:
    string school;
    double gpa;

public:
    // 调用父类带参构造函数
    Student(const string& n, int a, const string& s, double g)
        : Person(n, a), school(s), gpa(g) {
        cout << "[构造] Student(" << name << ", " << school << ")" << endl;
    }

    // 调用父类默认构造函数（省略即可，或显式写 Person()）
    Student(const string& s, double g)
        : school(s), gpa(g) {
        cout << "[构造] Student(school=" << school << ")" << endl;
    }

    ~Student() {
        cout << "[析构] ~Student(" << name << ")" << endl;
    }

    void info() const {
        cout << "学生：" << name << "，年龄：" << age
             << "，学校：" << school << "，GPA：" << gpa << endl;
    }
};

class GradStudent : public Student {
private:
    string major;

public:
    GradStudent(const string& n, int a, const string& s,
                double g, const string& m)
        : Student(n, a, s, g), major(m) {
        cout << "[构造] GradStudent(" << name << ", " << major << ")" << endl;
    }

    ~GradStudent() {
        cout << "[析构] ~GradStudent(" << name << ")" << endl;
    }

    void info() const {
        cout << "研究生：" << name << "，年龄：" << age
             << "，学校：" << school << "，专业：" << major << endl;
    }
};

// ============================================================
// 第四部分：成员对象的构造顺序
// ============================================================

class Engine {
public:
    Engine()  { cout << "[构造] Engine" << endl; }
    ~Engine() { cout << "[析构] ~Engine" << endl; }
};

class Wheel {
public:
    Wheel()  { cout << "[构造] Wheel" << endl; }
    ~Wheel() { cout << "[析构] ~Wheel" << endl; }
};

class Vehicle {
public:
    Vehicle() { cout << "[构造] Vehicle" << endl; }
    ~Vehicle() { cout << "[析构] ~Vehicle" << endl; }
};

class Car : public Vehicle {   // 先构造父类 Vehicle
private:
    Engine engine;  // 再按声明顺序构造成员
    Wheel wheel;

public:
    Car() {
        cout << "[构造] Car" << endl;
    }
    ~Car() {
        cout << "[析构] ~Car" << endl;
        // 析构顺序：Car体 → wheel → engine → Vehicle
    }
};

// ============================================================
// 主函数
// ============================================================

int main() {
    cout << "========================================" << endl;
    cout << "    继承中的构造函数与析构函数" << endl;
    cout << "========================================" << endl;

    // --- 基本构造/析构顺序 ---
    cout << "\n=== 基本顺序：父类先构造，子类先析构 ===" << endl;
    {
        Derived d("测试", 42);
        cout << "--- 对象使用中 ---" << endl;
    }  // 离开作用域，自动析构

    // --- 多级继承 ---
    cout << "\n=== 多级继承顺序（A←B←C）===" << endl;
    {
        C c;
        cout << "--- C 对象使用中 ---" << endl;
    }

    // --- 调用父类指定构造函数 ---
    cout << "\n=== 调用父类指定构造函数 ===" << endl;
    {
        cout << "\n-- 创建 Student（调用 Person 带参构造）--" << endl;
        Student stu("张三", 20, "清华大学", 3.8);
        stu.info();

        cout << "\n-- 创建 GradStudent（三级构造链）--" << endl;
        GradStudent gs("李四", 25, "北京大学", 3.9, "计算机科学");
        gs.info();

        cout << "\n--- 离开作用域，按序析构 ---" << endl;
    }

    // --- 成员对象构造顺序 ---
    cout << "\n=== 含成员对象的构造顺序 ===" << endl;
    cout << "顺序：父类 → 成员（按声明顺序）→ 自身" << endl;
    cout << "析构：自身 → 成员（逆序）→ 父类" << endl;
    {
        Car car;
        cout << "--- Car 使用中 ---" << endl;
    }

    cout << "\n=== 核心总结 ===" << endl;
    cout << "构造顺序：基类 → 成员对象（声明顺序）→ 派生类" << endl;
    cout << "析构顺序：派生类 → 成员对象（逆序）→ 基类" << endl;
    cout << "子类必须通过初始化列表调用父类构造函数" << endl;
    cout << "若不显式调用，编译器自动调用父类默认构造函数" << endl;

    return 0;
}
