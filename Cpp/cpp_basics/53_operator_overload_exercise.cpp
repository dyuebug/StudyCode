// 53_operator_overload_exercise.cpp
// 运算符重载综合练习：设计一个 Fraction 分数类

#include <iostream>
using namespace std;

// ============================================================
// 第一部分：题目背景
// ============================================================
// 我们希望 Fraction（分数）类像内置数值类型一样使用：
// 1. 两个分数可以相加
// 2. 两个分数可以比较是否相等
// 3. 可以直接用 cout 输出分数对象
// 这就是运算符重载的典型应用场景

// ============================================================
// 第二部分：Fraction 类实现
// ============================================================

class Fraction {
private:
    int numerator;     // 分子
    int denominator;   // 分母

public:
    Fraction(int numerator = 0, int denominator = 1)
        : numerator(numerator), denominator(denominator == 0 ? 1 : denominator) {}

    // 重载 +
    Fraction operator+(const Fraction& other) const {
        return Fraction(
            numerator * other.denominator + other.numerator * denominator,
            denominator * other.denominator
        );
    }

    // 重载 ==
    bool operator==(const Fraction& other) const {
        return numerator * other.denominator == other.numerator * denominator;
    }

    // 友元输出运算符
    friend ostream& operator<<(ostream& out, const Fraction& f);
};

ostream& operator<<(ostream& out, const Fraction& f) {
    out << f.numerator << "/" << f.denominator;
    return out;
}

// ============================================================
// 第三部分：综合测试
// ============================================================

void runTests() {
    Fraction f1(1, 2);
    Fraction f2(1, 3);
    Fraction f3 = f1 + f2;

    cout << "f1 = " << f1 << endl;
    cout << "f2 = " << f2 << endl;
    cout << "f3 = f1 + f2 = " << f3 << endl;

    Fraction f4(2, 4);
    cout << "f1 == f4 的结果：" << (f1 == f4 ? "true" : "false") << endl;
    cout << "解释：虽然 1/2 和 2/4 写法不同，但它们的值相等" << endl;
}

// ============================================================
// 第四部分：扩展练习题（不给答案）
// ============================================================

void printExercises() {
    cout << "\n=== 练习题 ===" << endl;
    cout << "1. 为 Fraction 重载 - 运算符" << endl;
    cout << "2. 为 Fraction 重载 * 和 / 运算符" << endl;
    cout << "3. 增加一个约分函数，让 5/6 + 1/6 输出为 1/1 或 1" << endl;
    cout << "4. 重载 != 运算符，并思考能否复用 == 的结果" << endl;
    cout << "5. 思考：分母为 0 时，除了自动改成 1，还有没有更合理的处理策略？" << endl;
}

// ============================================================
// 第五部分：学习提示
// ============================================================

void printHints() {
    cout << "1. 重载运算符时，先想清楚这个操作对类来说是否自然" << endl;
    cout << "2. + 一般返回新对象，不修改原对象" << endl;
    cout << "3. == 一般返回 bool，用来表示两个对象是否等价" << endl;
    cout << "4. << 常常写成 friend，因为左边是 ostream" << endl;
}

// ============================================================
// 主函数
// ============================================================

int main() {
    cout << "========================================" << endl;
    cout << "      运算符重载综合练习" << endl;
    cout << "========================================" << endl;

    cout << "\n=== 第一部分：Fraction 综合测试 ===" << endl;
    runTests();

    cout << "\n=== 第二部分：学习提示 ===" << endl;
    printHints();

    printExercises();

    cout << "\n=== 核心总结 ===" << endl;
    cout << "1. 运算符重载可以让自定义类型更像内置类型" << endl;
    cout << "2. Fraction 很适合练习 +、==、<< 等运算符重载" << endl;
    cout << "3. 友元函数常用于流输出/输入运算符" << endl;
    cout << "4. 综合练习的重点不只是语法，而是思考设计是否合理" << endl;
    cout << "5. 下一步可以继续扩展更多运算符，让类更完整" << endl;

    return 0;
}
