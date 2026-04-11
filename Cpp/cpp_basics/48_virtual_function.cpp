// 48_virtual_function.cpp
// 虚函数与动态绑定：理解多态为什么会发生

#include <iostream>
#include <string>
using namespace std;

// ============================================================
// 第一部分：没有 virtual 和有 virtual 的区别
// ============================================================

class AnimalWithoutVirtual {
public:
    void speak() const {
        cout << "[AnimalWithoutVirtual] 动物发出声音" << endl;
    }
};

class DogWithoutVirtual : public AnimalWithoutVirtual {
public:
    void speak() const {
        cout << "[DogWithoutVirtual] 汪汪！" << endl;
    }
};

class AnimalWithVirtual {
public:
    virtual void speak() const {
        cout << "[AnimalWithVirtual] 动物发出声音" << endl;
    }

    virtual ~AnimalWithVirtual() {}
};

class DogWithVirtual : public AnimalWithVirtual {
public:
    void speak() const override {
        cout << "[DogWithVirtual] 汪汪！" << endl;
    }
};

// ============================================================
// 第二部分：动态绑定的核心条件
// ============================================================

class Shape {
public:
    string name;

    Shape(const string& n) : name(n) {}

    virtual void draw() const {
        cout << "[Shape::draw] 画一个通用图形：" << name << endl;
    }

    virtual double area() const {
        return 0.0;
    }

    virtual ~Shape() {}
};

class Circle : public Shape {
public:
    double radius;

    Circle(double r) : Shape("圆形"), radius(r) {}

    void draw() const override {
        cout << "[Circle::draw] 画圆，半径：" << radius << endl;
    }

    double area() const override {
        return 3.14159 * radius * radius;
    }
};

class Rectangle : public Shape {
public:
    double width;
    double height;

    Rectangle(double w, double h) : Shape("矩形"), width(w), height(h) {}

    void draw() const override {
        cout << "[Rectangle::draw] 画矩形，长=" << width
             << "，宽=" << height << endl;
    }

    double area() const override {
        return width * height;
    }
};

void printShapeInfo(const Shape& shape) {
    // 这里的参数类型是父类引用，但实际绑定到子类对象
    // 因为 draw() 和 area() 是 virtual，所以运行时会调用子类版本
    shape.draw();
    cout << "面积：" << shape.area() << endl;
}

// ============================================================
// 第三部分：为什么多态基类析构函数必须是 virtual
// ============================================================

class Base {
public:
    Base() {
        cout << "Base 构造" << endl;
    }

    virtual void show() const {
        cout << "[Base::show] 基类对象" << endl;
    }

    virtual ~Base() {
        cout << "Base 析构" << endl;
    }
};

class Derived : public Base {
private:
    string* data;

public:
    Derived() : data(new string("子类独有的堆内存资源")) {
        cout << "Derived 构造，申请资源：" << *data << endl;
    }

    void show() const override {
        cout << "[Derived::show] 子类对象，资源内容：" << *data << endl;
    }

    ~Derived() override {
        cout << "Derived 析构，释放资源：" << *data << endl;
        delete data;
    }
};

// ============================================================
// 第四部分：虚函数表的直觉理解
// ============================================================

void explainVTable() {
    cout << "1. 有 virtual 的类，编译器通常会为它准备一张虚函数表（vtable）" << endl;
    cout << "2. 对象内部通常会保存一个指针（vptr），指向这张表" << endl;
    cout << "3. 通过父类指针/引用调用 virtual 函数时，程序会在运行时查表" << endl;
    cout << "4. 所以同样是 Shape&，绑定 Circle 还是 Rectangle，结果可以不同" << endl;
    cout << "5. 这就是多态背后的直觉：同一接口，运行时决定具体实现" << endl;
}

// ============================================================
// 主函数
// ============================================================

int main() {
    cout << "========================================" << endl;
    cout << "      虚函数与动态绑定学习" << endl;
    cout << "========================================" << endl;

    cout << "\n=== 第一部分：没有 virtual 和有 virtual 的区别 ===" << endl;
    DogWithoutVirtual dog1;
    AnimalWithoutVirtual* p1 = &dog1;
    cout << "通过父类指针调用非 virtual 函数：" << endl;
    p1->speak();
    dog1.speak();
    cout << "结论：没有 virtual 时，父类指针调用的是父类版本（静态绑定）" << endl;

    DogWithVirtual dog2;
    AnimalWithVirtual* p2 = &dog2;
    cout << "\n通过父类指针调用 virtual 函数：" << endl;
    p2->speak();
    dog2.speak();
    cout << "结论：有 virtual 时，父类指针调用的是子类版本（动态绑定）" << endl;

    cout << "\n=== 第二部分：父类引用触发多态 ===" << endl;
    Circle circle(5.0);
    Rectangle rect(4.0, 3.0);
    printShapeInfo(circle);
    cout << endl;
    printShapeInfo(rect);

    cout << "\n=== 第三部分：虚析构函数演示 ===" << endl;
    Base* pb = new Derived();
    pb->show();
    cout << "现在通过父类指针 delete 子类对象：" << endl;
    delete pb;
    cout << "结论：因为 Base 的析构函数是 virtual，所以会先调用 Derived 析构，再调用 Base 析构" << endl;

    cout << "\n=== 第四部分：vtable / vptr 直觉理解 ===" << endl;
    explainVTable();

    cout << "\n=== 核心总结 ===" << endl;
    cout << "1. virtual 让父类指针/引用可以在运行时调用子类实现" << endl;
    cout << "2. 没有 virtual 时，调用结果由指针/引用类型决定" << endl;
    cout << "3. 动态绑定是多态成立的关键机制" << endl;
    cout << "4. 多态基类析构函数必须是 virtual，否则 delete 时可能资源泄漏" << endl;
    cout << "5. 多态的本质：同一接口，运行时决定具体行为" << endl;

    return 0;
}
