// 51_operator_overloading_basics.cpp
// 运算符重载基础：让自定义类型像内置类型一样使用

#include <iostream>
using namespace std;

// ============================================================
// 第一部分：运算符重载的基本规则
// ============================================================
// 1. 只能重载 C++ 已有的运算符，不能发明新符号
// 2. 不能改变运算符的优先级和结合性
// 3. 不能改变运算符需要的操作数个数
// 4. 重载的目的：让自定义类型的行为更自然、更直观

// ============================================================
// 第二部分：成员函数方式重载常见运算符
// ============================================================

class Point {
private:
    int x;
    int y;

public:
    Point(int x = 0, int y = 0) : x(x), y(y) {}

    void print() const {
        cout << "(" << x << ", " << y << ")";
    }

    // 重载 + 运算符
    // 作用：让两个 Point 可以直接相加
    Point operator+(const Point& other) const {
        return Point(x + other.x, y + other.y);
    }

    // 重载 == 运算符
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }

    // 前置 ++
    // 先自增，再返回当前对象本身
    Point& operator++() {
        ++x;
        ++y;
        return *this;
    }

    // 后置 ++
    // int 参数只是占位，用来和前置 ++ 区分
    // 先保存旧值，再自增，最后返回旧值
    Point operator++(int) {
        Point temp = *this;
        x++;
        y++;
        return temp;
    }
};

// ============================================================
// 第三部分：演示前置 ++ 和后置 ++ 的区别
// ============================================================

void demoIncrement() {
    Point p(1, 2);

    cout << "原始对象 p = ";
    p.print();
    cout << endl;

    Point pre = ++p;
    cout << "执行 pre = ++p 后：" << endl;
    cout << "p = ";
    p.print();
    cout << "，pre = ";
    pre.print();
    cout << endl;

    Point post = p++;
    cout << "执行 post = p++ 后：" << endl;
    cout << "p = ";
    p.print();
    cout << "，post = ";
    post.print();
    cout << endl;
}

// ============================================================
// 第四部分：易错点说明
// ============================================================

void explainCommonMistakes() {
    cout << "1. operator+ 通常返回新对象，不修改原对象" << endl;
    cout << "2. operator== 一般写成 const 成员函数，表示比较时不修改对象" << endl;
    cout << "3. 前置 ++ 返回引用，后置 ++ 返回旧值副本" << endl;
    cout << "4. 运算符重载不是炫技工具，只应重载语义自然的操作" << endl;
    cout << "5. 本文件不展开 operator=，因为赋值运算符在第44课已单独讲解" << endl;
}

// ============================================================
// 主函数
// ============================================================

int main() {
    cout << "========================================" << endl;
    cout << "         运算符重载基础学习" << endl;
    cout << "========================================" << endl;

    cout << "\n=== 第一部分：Point 对象相加 ===" << endl;
    Point p1(2, 3);
    Point p2(4, 5);
    Point p3 = p1 + p2;
    cout << "p1 = ";
    p1.print();
    cout << endl;
    cout << "p2 = ";
    p2.print();
    cout << endl;
    cout << "p3 = p1 + p2 = ";
    p3.print();
    cout << endl;

    cout << "\n=== 第二部分：Point 对象比较 ===" << endl;
    Point a(1, 1);
    Point b(1, 1);
    Point c(2, 2);
    cout << "a == b 的结果：" << (a == b ? "true" : "false") << endl;
    cout << "a == c 的结果：" << (a == c ? "true" : "false") << endl;

    cout << "\n=== 第三部分：前置 ++ 和后置 ++ ===" << endl;
    demoIncrement();

    cout << "\n=== 第四部分：易错点说明 ===" << endl;
    explainCommonMistakes();

    cout << "\n=== 核心总结 ===" << endl;
    cout << "1. 运算符重载让自定义类型的使用方式更自然" << endl;
    cout << "2. 成员函数重载适合和对象自身强相关的操作" << endl;
    cout << "3. operator+ 常返回新对象，operator== 常返回 bool" << endl;
    cout << "4. 前置 ++ 和后置 ++ 的实现方式与返回值不同" << endl;
    cout << "5. 重载要讲语义合理性，不要为了复杂而复杂" << endl;

    return 0;
}
