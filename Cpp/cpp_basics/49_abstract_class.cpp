// 49_abstract_class.cpp
// 纯虚函数与抽象类：用接口强制子类实现功能

#include <iostream>
#include <string>
using namespace std;

// ============================================================
// 第一部分：什么是纯虚函数和抽象类
// ============================================================

class Graphic {
public:
    // 纯虚函数：只有声明，没有默认实现
    // = 0 的含义不是“等于0”，而是告诉编译器：这个函数必须由子类实现
    virtual void draw() const = 0;
    virtual double area() const = 0;

    // 多态基类析构函数仍然要写成 virtual
    virtual ~Graphic() {}
};

// 含有纯虚函数的类叫“抽象类”
// 抽象类不能直接创建对象，下面这种写法会编译报错：
// Graphic g;  // 错误：抽象类不能实例化

// ============================================================
// 第二部分：子类必须实现纯虚函数
// ============================================================

class Circle : public Graphic {
private:
    double radius;

public:
    Circle(double r) : radius(r) {}

    void draw() const override {
        cout << "[Circle::draw] 画一个圆，半径：" << radius << endl;
    }

    double area() const override {
        return 3.14159 * radius * radius;
    }
};

class Rectangle : public Graphic {
private:
    double width;
    double height;

public:
    Rectangle(double w, double h) : width(w), height(h) {}

    void draw() const override {
        cout << "[Rectangle::draw] 画一个矩形，长=" << width
             << "，宽=" << height << endl;
    }

    double area() const override {
        return width * height;
    }
};

// 如果子类没有实现所有纯虚函数，它自己也会变成抽象类
class Triangle : public Graphic {
private:
    double bottom;
    double height;

public:
    Triangle(double b, double h) : bottom(b), height(h) {}

    void draw() const override {
        cout << "[Triangle::draw] 画一个三角形，底=" << bottom
             << "，高=" << height << endl;
    }

    double area() const override {
        return 0.5 * bottom * height;
    }
};

// ============================================================
// 第三部分：抽象类的价值——统一接口
// ============================================================

void showGraphic(const Graphic& g) {
    g.draw();
    cout << "面积：" << g.area() << endl;
}

// ============================================================
// 第四部分：普通 virtual 和纯 virtual 的区别
// ============================================================

class Animal {
public:
    virtual void speak() const {
        cout << "[Animal] 动物发出声音" << endl;
    }

    virtual ~Animal() {}
};

class Bird : public Animal {
public:
    void speak() const override {
        cout << "[Bird] 啾啾！" << endl;
    }
};

class AbstractAnimal {
public:
    virtual void speak() const = 0;
    virtual ~AbstractAnimal() {}
};

class Dog : public AbstractAnimal {
public:
    void speak() const override {
        cout << "[Dog] 汪汪！" << endl;
    }
};

// ============================================================
// 主函数
// ============================================================

int main() {
    cout << "========================================" << endl;
    cout << "       纯虚函数与抽象类学习" << endl;
    cout << "========================================" << endl;

    cout << "\n=== 第一部分：抽象类基础 ===" << endl;
    cout << "Graphic 是抽象类，因为它含有纯虚函数 draw() 和 area()" << endl;
    cout << "抽象类的作用：规定接口，强制子类实现" << endl;

    cout << "\n=== 第二部分：子类实现纯虚函数 ===" << endl;
    Circle circle(3.0);
    Rectangle rect(4.0, 5.0);
    Triangle tri(6.0, 2.0);
    circle.draw();
    rect.draw();
    tri.draw();

    cout << "\n=== 第三部分：统一接口调用 ===" << endl;
    showGraphic(circle);
    cout << endl;
    showGraphic(rect);
    cout << endl;
    showGraphic(tri);
    cout << "结论：调用方不需要知道具体是圆形、矩形还是三角形，只关心它是不是 Graphic" << endl;

    cout << "\n=== 第四部分：普通 virtual 与纯 virtual 对比 ===" << endl;
    Bird bird;
    Dog dog;
    Animal* a1 = &bird;
    AbstractAnimal* a2 = &dog;
    a1->speak();
    a2->speak();
    cout << "普通 virtual：父类可以提供默认实现" << endl;
    cout << "纯 virtual：父类只规定接口，子类必须实现" << endl;

    cout << "\n=== 核心总结 ===" << endl;
    cout << "1. 纯虚函数写法：virtual 返回类型 函数名() = 0;" << endl;
    cout << "2. 含纯虚函数的类叫抽象类，不能直接实例化" << endl;
    cout << "3. 抽象类的价值是定义统一接口，强制子类实现" << endl;
    cout << "4. 如果子类没实现所有纯虚函数，它自己也会成为抽象类" << endl;
    cout << "5. 抽象类通常用来表达“能做什么”，而不是“具体怎么做”" << endl;

    return 0;
}
