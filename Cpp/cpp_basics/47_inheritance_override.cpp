// 47_inheritance_override.cpp
// 方法重写（override）、隐藏（hiding）、virtual 关键字初步

#include <iostream>
#include <string>
using namespace std;

// ============================================================
// 第一部分：隐藏 vs 重写
// ============================================================

class Shape {
public:
    string color;

    Shape(const string& c = "白色") : color(c) {}

    // 普通函数（非virtual）— 子类同名函数会「隐藏」它
    void describe() const {
        cout << "[Shape] 这是一个" << color << "的图形" << endl;
    }

    // virtual 函数 — 子类同名函数是真正的「重写」
    virtual void draw() const {
        cout << "[Shape::draw] 画一个图形" << endl;
    }

    virtual double area() const {
        return 0.0;
    }

    virtual ~Shape() {}  // 多态基类析构函数必须是 virtual
};

class Circle : public Shape {
public:
    double radius;

    Circle(double r, const string& c = "红色")
        : Shape(c), radius(r) {}

    // 隐藏父类 describe()（非virtual，不是多态）
    void describe() const {
        cout << "[Circle] 这是一个" << color << "的圆，半径：" << radius << endl;
    }

    // override 重写父类 virtual draw()
    void draw() const override {
        cout << "[Circle::draw] 画圆，半径：" << radius << endl;
    }

    // override 重写父类 virtual area()
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
        cout << "[Rectangle] 这是一个" << color
             << "的矩形，" << width << "x" << height << endl;
    }

    void draw() const override {
        cout << "[Rectangle::draw] 画矩形，" << width << "x" << height << endl;
    }

    double area() const override {
        return width * height;
    }
};

// ============================================================
// 第二部分：override 关键字的保护作用
// ============================================================

class Animal {
public:
    virtual void speak() const {
        cout << "[Animal] 动物发出声音" << endl;
    }
    virtual ~Animal() {}
};

class Dog : public Animal {
public:
    // 正确重写：编译器验证父类有同名 virtual 函数
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

// 如果写错函数签名，override 会让编译器报错：
// void speak(int x) const override {}  // 错误：父类没有这个签名

// ============================================================
// 第三部分：通过父类指针调用 — 多态的核心
// ============================================================

void printShapeInfo(const Shape* s) {
    // 调用 virtual 函数：运行时根据实际类型决定调用哪个版本
    s->draw();
    cout << "  面积：" << s->area() << endl;

    // 调用非 virtual 函数：编译时根据指针类型决定（始终调用 Shape 版本）
    s->describe();  // 始终输出 Shape::describe
}

// ============================================================
// 第四部分：final 关键字（禁止继续重写/继承）
// ============================================================

class Base {
public:
    virtual void func() {
        cout << "[Base::func]" << endl;
    }
    virtual ~Base() {}
};

class Mid : public Base {
public:
    // final：禁止子类继续重写 func
    void func() override final {
        cout << "[Mid::func] final，不可再重写" << endl;
    }
};

// class Bad : public Mid {
//     void func() override {}  // 错误：Mid::func 是 final
// };

// ============================================================
// 主函数
// ============================================================

int main() {
    cout << "========================================" << endl;
    cout << "    方法重写、隐藏与 override 关键字" << endl;
    cout << "========================================" << endl;

    Circle    c(5.0, "红色");
    Rectangle r(4.0, 3.0, "蓝色");

    // --- 隐藏 vs 重写 ---
    cout << "\n=== 直接通过对象调用（静态绑定）===" << endl;
    c.describe();   // Circle::describe（隐藏）
    c.draw();       // Circle::draw（重写）
    r.describe();   // Rectangle::describe（隐藏）
    r.draw();       // Rectangle::draw（重写）

    // --- 通过父类指针调用 ---
    cout << "\n=== 通过父类指针调用（动态绑定）===" << endl;
    Shape* shapes[] = { &c, &r };
    for (auto* s : shapes) {
        printShapeInfo(s);
        cout << endl;
    }

    // --- 隐藏的陷阱 ---
    cout << "=== 隐藏的陷阱：父类指针调用非virtual函数 ===" << endl;
    Shape* ps = &c;
    ps->describe();  // 调用 Shape::describe，不是 Circle::describe！
    c.describe();    // 调用 Circle::describe
    cout << "结论：非virtual函数通过父类指针调用，始终走父类版本" << endl;

    // --- override 保护 ---
    cout << "\n=== override 保护：多态调用动物 ===" << endl;
    Animal* animals[] = { new Dog(), new Cat() };
    for (auto* a : animals) {
        a->speak();
        delete a;
    }

    // --- final ---
    cout << "\n=== final 关键字 ===" << endl;
    Mid m;
    m.func();
    Base* pb = &m;
    pb->func();  // 多态调用 Mid::func

    cout << "\n=== 核心总结 ===" << endl;
    cout << "1. virtual  → 子类重写，运行时动态绑定（多态）" << endl;
    cout << "2. 无virtual → 子类隐藏，编译时静态绑定（非多态）" << endl;
    cout << "3. override  → 编译器验证确实重写了父类 virtual 函数" << endl;
    cout << "4. final     → 禁止子类继续重写" << endl;
    cout << "5. 多态基类析构函数必须是 virtual，否则资源泄漏" << endl;

    return 0;
}
