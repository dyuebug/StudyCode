// 45_inheritance_basics.cpp
// 继承基础：语法、访问控制、is-a关系

#include <iostream>
#include <string>
using namespace std;

// ============================================================
// 第一部分：继承的基本语法
// ============================================================

// 基类（父类）
class Animal {
public:
    string name;
    int age;

    Animal(const string& n, int a) : name(n), age(a) {
        cout << "Animal 构造：" << name << endl;
    }

    void eat() {
        cout << name << " 正在吃东西" << endl;
    }

    void breathe() {
        cout << name << " 正在呼吸" << endl;
    }

    void info() {
        cout << "动物：" << name << "，年龄：" << age << endl;
    }
};

// 派生类（子类）：public 继承
// Dog is-a Animal（狗是动物）
class Dog : public Animal {
public:
    string breed;  // 品种（子类自己的成员）

    Dog(const string& n, int a, const string& b)
        : Animal(n, a), breed(b) {  // 调用父类构造函数
        cout << "Dog 构造：" << name << endl;
    }

    // 子类自己的方法
    void bark() {
        cout << name << " 汪汪叫！" << endl;
    }

    void info() {  // 重新定义父类方法（隐藏）
        cout << "狗：" << name << "，年龄：" << age
             << "，品种：" << breed << endl;
    }
};

class Cat : public Animal {
public:
    bool isIndoor;  // 是否室内猫

    Cat(const string& n, int a, bool indoor)
        : Animal(n, a), isIndoor(indoor) {
        cout << "Cat 构造：" << name << endl;
    }

    void meow() {
        cout << name << " 喵喵叫！" << endl;
    }

    void info() {
        cout << "猫：" << name << "，年龄：" << age
             << "，" << (isIndoor ? "室内" : "室外") << "猫" << endl;
    }
};

// ============================================================
// 第二部分：public / protected / private 继承
// ============================================================

class Base {
public:
    int pub = 1;       // 任何地方都可访问
protected:
    int prot = 2;      // 类内 + 子类可访问，类外不可访问
private:
    int priv = 3;      // 只有类内可访问，子类也不可访问

public:
    void showAll() {
        cout << "Base: pub=" << pub
             << " prot=" << prot
             << " priv=" << priv << endl;
    }
};

// public 继承：父类 public→public, protected→protected, private→不可访问
class PublicDerived : public Base {
public:
    void show() {
        cout << "PublicDerived 访问：pub=" << pub
             << " prot=" << prot << endl;
        // priv 不可访问（编译错误）
    }
};

// protected 继承：父类 public→protected, protected→protected
class ProtectedDerived : protected Base {
public:
    void show() {
        cout << "ProtectedDerived 访问：pub=" << pub
             << " prot=" << prot << endl;
    }
};

// private 继承：父类所有成员→private
class PrivateDerived : private Base {
public:
    void show() {
        cout << "PrivateDerived 访问：pub=" << pub
             << " prot=" << prot << endl;
    }
};

// ============================================================
// 第三部分：protected 成员的作用
// ============================================================

class Shape {
protected:
    double width;   // 子类可以直接访问
    double height;

public:
    Shape(double w, double h) : width(w), height(h) {}

    virtual double area() {
        return 0;
    }
};

class Rectangle : public Shape {
public:
    Rectangle(double w, double h) : Shape(w, h) {}

    double area() override {
        // 可以直接访问父类 protected 成员
        return width * height;
    }

    void info() {
        cout << "矩形：" << width << " x " << height
             << "，面积：" << area() << endl;
    }
};

class Triangle : public Shape {
public:
    Triangle(double w, double h) : Shape(w, h) {}

    double area() override {
        return 0.5 * width * height;
    }

    void info() {
        cout << "三角形：底" << width << " 高" << height
             << "，面积：" << area() << endl;
    }
};

// ============================================================
// 主函数：演示
// ============================================================

int main() {
    cout << "========================================" << endl;
    cout << "         继承基础学习" << endl;
    cout << "========================================" << endl;

    // --- 基本继承 ---
    cout << "\n=== 基本继承：Dog 和 Cat 继承自 Animal ===" << endl;
    Dog dog("旺财", 3, "金毛");
    Cat cat("咪咪", 2, true);

    cout << "\n--- 继承父类的方法 ---" << endl;
    dog.eat();      // 继承自 Animal
    dog.breathe();  // 继承自 Animal
    dog.bark();     // Dog 自己的方法
    dog.info();     // Dog 重定义的方法

    cat.eat();      // 继承自 Animal
    cat.meow();     // Cat 自己的方法
    cat.info();     // Cat 重定义的方法

    // --- 访问继承成员 ---
    cout << "\n--- 直接访问继承的 public 成员 ---" << endl;
    cout << "dog.name = " << dog.name << endl;  // 继承自 Animal
    cout << "dog.age = " << dog.age << endl;
    cout << "dog.breed = " << dog.breed << endl; // Dog 自己的

    // --- public/protected/private 继承 ---
    cout << "\n=== 三种继承方式对比 ===" << endl;

    PublicDerived pd;
    pd.show();
    cout << "类外访问 pd.pub = " << pd.pub << endl;  // public 继承后仍是 public
    // pd.prot 类外不可访问

    ProtectedDerived protd;
    protd.show();
    // protd.pub 类外不可访问（变成了 protected）

    PrivateDerived privd;
    privd.show();
    // privd.pub 类外不可访问（变成了 private）

    // --- protected 成员 ---
    cout << "\n=== protected 成员：子类可访问，类外不可访问 ===" << endl;
    Rectangle rect(5.0, 3.0);
    Triangle tri(4.0, 6.0);
    rect.info();
    tri.info();
    // rect.width 类外不可访问（protected）

    // --- is-a 关系 ---
    cout << "\n=== is-a 关系：子类对象可赋给父类指针 ===" << endl;
    Animal* p1 = &dog;  // Dog is-a Animal
    Animal* p2 = &cat;  // Cat is-a Animal
    p1->eat();
    p2->eat();
    p1->info();  // 注意：调用的是 Animal::info()，不是 Dog::info()

    cout << "\n=== 继承基础要点总结 ===" << endl;
    cout << "1. 继承语法：class Dog : public Animal" << endl;
    cout << "2. is-a 关系：子类对象可当父类用" << endl;
    cout << "3. public 继承：最常用，保持访问权限" << endl;
    cout << "4. protected：子类可访问，类外不可访问" << endl;
    cout << "5. private：子类完全不可访问" << endl;

    return 0;
}
