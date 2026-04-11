// 52_friend_operator_overload.cpp
// 友元函数与输入输出运算符重载

#include <iostream>
#include <string>
using namespace std;

// ============================================================
// 第一部分：为什么需要 friend
// ============================================================
// 有些运算符不适合写成成员函数，比如 << 和 >>
// 因为它们左边通常是 cout / cin，而不是我们的类对象
// 这时就常用非成员函数 + friend 的方式来重载

class Student {
private:
    string name;
    int score;

public:
    Student(const string& name = "未知", int score = 0)
        : name(name), score(score) {}

    void show() const {
        cout << "学生姓名：" << name << "，分数：" << score << endl;
    }

    // 友元输出运算符重载
    friend ostream& operator<<(ostream& out, const Student& s);

    // 友元输入运算符重载
    friend istream& operator>>(istream& in, Student& s);
};

// ============================================================
// 第二部分：重载 << 输出运算符
// ============================================================

ostream& operator<<(ostream& out, const Student& s) {
    out << "[Student] 姓名=" << s.name << "，分数=" << s.score;
    return out;
}

// ============================================================
// 第三部分：重载 >> 输入运算符
// ============================================================

istream& operator>>(istream& in, Student& s) {
    in >> s.name >> s.score;

    // 简单校验：如果分数非法，就修正到合理区间
    if (s.score < 0) {
        s.score = 0;
    }
    if (s.score > 100) {
        s.score = 100;
    }

    return in;
}

// ============================================================
// 第四部分：friend 的边界
// ============================================================

void explainFriendBoundary() {
    cout << "1. friend 可以访问类的 private 成员，但不意味着应该滥用" << endl;
    cout << "2. 只有在确实需要访问内部数据时，才考虑 friend" << endl;
    cout << "3. << 和 >> 常常用 friend，因为左操作数是 ostream/istream" << endl;
    cout << "4. 如果只需要读取数据，有时提供 getter 也可以代替 friend" << endl;
}

// ============================================================
// 主函数
// ============================================================

int main() {
    cout << "========================================" << endl;
    cout << "     友元函数与输入输出重载学习" << endl;
    cout << "========================================" << endl;

    cout << "\n=== 第一部分：输出运算符重载 ===" << endl;
    Student s1("Alice", 95);
    cout << s1 << endl;

    cout << "\n=== 第二部分：输入运算符重载 ===" << endl;
    Student s2;
    cout << "请依次输入学生姓名和分数，例如：Tom 88" << endl;
    cin >> s2;
    cout << "你输入后的对象内容是：" << s2 << endl;

    cout << "\n=== 第三部分：普通成员函数输出对比 ===" << endl;
    s2.show();
    cout << "结论：show() 可以输出对象，但 cout << s2 更自然，像内置类型一样" << endl;

    cout << "\n=== 第四部分：friend 的边界 ===" << endl;
    explainFriendBoundary();

    cout << "\n=== 核心总结 ===" << endl;
    cout << "1. << 和 >> 通常写成非成员函数" << endl;
    cout << "2. 为了访问 private 成员，这些非成员函数常写成 friend" << endl;
    cout << "3. operator<< 应返回 ostream&，这样才能连续输出" << endl;
    cout << "4. operator>> 应返回 istream&，这样才能连续输入" << endl;
    cout << "5. friend 要谨慎使用，遵循最小授权原则" << endl;

    return 0;
}
