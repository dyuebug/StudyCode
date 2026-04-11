// 50_polymorphism_exercise.cpp
// 多态实战练习：设计一个简单的图形管理系统

#include <iostream>
#include <memory>
#include <string>
#include <vector>
using namespace std;

// ============================================================
// 第一部分：题目背景
// ============================================================
// 我们要做一个“小型图形系统”：
// 1. 所有图形都有名字、绘制功能、面积计算功能
// 2. 不同图形的实现不同，但外部调用方式统一
// 3. 这正是多态最适合解决的问题

// ============================================================
// 第二部分：抽象基类 + 派生类
// ============================================================

class Shape {
public:
    virtual string getName() const = 0;
    virtual void draw() const = 0;
    virtual double area() const = 0;
    virtual ~Shape() {}
};

class Circle : public Shape {
private:
    double radius;

public:
    Circle(double r) : radius(r) {}

    string getName() const override {
        return "圆形";
    }

    void draw() const override {
        cout << "绘制圆形，半径：" << radius << endl;
    }

    double area() const override {
        return 3.14159 * radius * radius;
    }
};

class Rectangle : public Shape {
private:
    double width;
    double height;

public:
    Rectangle(double w, double h) : width(w), height(h) {}

    string getName() const override {
        return "矩形";
    }

    void draw() const override {
        cout << "绘制矩形，长=" << width << "，宽=" << height << endl;
    }

    double area() const override {
        return width * height;
    }
};

class Triangle : public Shape {
private:
    double bottom;
    double height;

public:
    Triangle(double b, double h) : bottom(b), height(h) {}

    string getName() const override {
        return "三角形";
    }

    void draw() const override {
        cout << "绘制三角形，底=" << bottom << "，高=" << height << endl;
    }

    double area() const override {
        return 0.5 * bottom * height;
    }
};

// ============================================================
// 第三部分：统一处理流程
// ============================================================

void renderAll(const vector<unique_ptr<Shape>>& shapes) {
    cout << "\n--- 开始统一绘制所有图形 ---" << endl;
    for (const auto& shape : shapes) {
        cout << "图形名称：" << shape->getName() << endl;
        shape->draw();
        cout << "面积：" << shape->area() << endl;
        cout << endl;
    }
}

double totalArea(const vector<unique_ptr<Shape>>& shapes) {
    double sum = 0.0;
    for (const auto& shape : shapes) {
        sum += shape->area();
    }
    return sum;
}

void printLargestShape(const vector<unique_ptr<Shape>>& shapes) {
    if (shapes.empty()) {
        cout << "没有图形可比较" << endl;
        return;
    }

    const Shape* maxShape = shapes[0].get();
    for (const auto& shape : shapes) {
        if (shape->area() > maxShape->area()) {
            maxShape = shape.get();
        }
    }

    cout << "面积最大的图形是：" << maxShape->getName()
         << "，面积：" << maxShape->area() << endl;
}

// ============================================================
// 第四部分：对象切片说明
// ============================================================

class BadShape {
public:
    virtual void draw() const {
        cout << "[BadShape] 通用图形" << endl;
    }

    virtual ~BadShape() {}
};

class BadCircle : public BadShape {
public:
    void draw() const override {
        cout << "[BadCircle] 真正想调用的是这个版本" << endl;
    }
};

void drawByReference(const BadShape& shape) {
    shape.draw();
}

void explainSlicing() {
    cout << "1. 如果把子类对象按值赋给父类对象，会丢失子类那一部分数据，这叫对象切片" << endl;
    cout << "2. 所以多态场景通常用父类指针或父类引用，不直接按值传递对象" << endl;
    cout << "3. 本文件使用 unique_ptr<Shape>，既保留多态，又能自动管理内存" << endl;
}

// ============================================================
// 第五部分：练习题（不给答案）
// ============================================================

void printExercises() {
    cout << "\n=== 练习题 ===" << endl;
    cout << "1. 新增一个 Square 类，继承 Shape，并实现 getName/draw/area" << endl;
    cout << "2. 给 Shape 增加一个 virtual void scale(double factor) = 0; 接口" << endl;
    cout << "3. 写一个函数，统计面积大于 20 的图形个数" << endl;
    cout << "4. 尝试把图形系统改成员工薪资系统，体会多态思想如何迁移" << endl;
    cout << "5. 思考：为什么这里使用 unique_ptr，而不是直接写 vector<Shape>？" << endl;
}

// ============================================================
// 主函数
// ============================================================

int main() {
    cout << "========================================" << endl;
    cout << "       多态实战：图形管理系统" << endl;
    cout << "========================================" << endl;

    vector<unique_ptr<Shape>> shapes;
    shapes.push_back(make_unique<Circle>(3.0));
    shapes.push_back(make_unique<Rectangle>(4.0, 5.0));
    shapes.push_back(make_unique<Triangle>(6.0, 2.0));

    cout << "\n=== 第一部分：统一接口，不同实现 ===" << endl;
    renderAll(shapes);

    cout << "=== 第二部分：汇总统计 ===" << endl;
    cout << "所有图形总面积：" << totalArea(shapes) << endl;
    printLargestShape(shapes);

    cout << "\n=== 第三部分：父类引用触发多态 ===" << endl;
    BadCircle circle;
    drawByReference(circle);
    cout << "结论：虽然函数参数是 BadShape&，但运行时仍然调用 BadCircle::draw" << endl;

    cout << "\n=== 第四部分：对象切片说明 ===" << endl;
    explainSlicing();

    printExercises();

    cout << "\n=== 核心总结 ===" << endl;
    cout << "1. 多态让我们可以用统一接口处理不同类型对象" << endl;
    cout << "2. 抽象基类负责定义规则，派生类负责具体实现" << endl;
    cout << "3. 父类指针/引用是多态最常见的使用方式" << endl;
    cout << "4. 使用 unique_ptr 可以避免手动 delete，减少内存管理错误" << endl;
    cout << "5. 对象切片会破坏多态，所以不要把子类对象随便按值存进父类对象里" << endl;

    return 0;
}
