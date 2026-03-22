/*
 * ============================================================================
 * 文件名：26_reference_const_exercise.cpp
 * 主题：引用与const综合练习
 * 学习目标：
 *   1. 用引用优化函数参数传递
 *   2. 用const保护数据安全
 *   3. 掌握引用和const的组合使用
 *   4. 理解何时使用引用、何时使用const
 *   5. 实战案例：高效安全的函数设计
 *
 * 编译命令：g++ -std=c++11 -o 26_reference_const_exercise 26_reference_const_exercise.cpp
 * 运行命令：./26_reference_const_exercise (Linux/Mac) 或 26_reference_const_exercise.exe (Windows)
 * ============================================================================
 */

#include <iostream>
#include <string>
using namespace std;

// 函数声明
void printStudent(const string& name, const int& age, const double& score);
void updateScore(double& score, double delta);
int findMax(const int* arr, int size);
void swap(int& a, int& b);
string concatenate(const string& s1, const string& s2);
void processArray(const int* input, int* output, int size);

int main() {
    cout << "=== 引用与const综合练习 ===" << endl << endl;

    // ========================================
    // 练习1：用const引用传递大对象
    // ========================================
    cout << "【练习1：用const引用传递大对象】" << endl;

    string name = "Alice";
    int age = 20;
    double score = 95.5;

    cout << "学生信息：" << endl;
    printStudent(name, age, score);
    cout << endl;

    cout << "为什么用const引用？" << endl;
    cout << "  ✓ 避免拷贝（string可能很大）" << endl;
    cout << "  ✓ 保证不修改原数据" << endl;
    cout << "  ✓ 语法简洁" << endl;
    cout << endl;

    // ========================================
    // 练习2：用引用修改数据
    // ========================================
    cout << "【练习2：用引用修改数据】" << endl;

    double myScore = 85.0;
    cout << "原始分数：" << myScore << endl;

    updateScore(myScore, 5.0);
    cout << "加分后：" << myScore << endl;

    updateScore(myScore, -3.0);
    cout << "扣分后：" << myScore << endl;
    cout << endl;

    cout << "为什么用引用？" << endl;
    cout << "  ✓ 需要修改原变量" << endl;
    cout << "  ✓ 比指针语法更简洁" << endl;
    cout << endl;

    // ========================================
    // 练习3：const指针保护数组
    // ========================================
    cout << "【练习3：const指针保护数组】" << endl;

    int numbers[6] = {45, 23, 78, 12, 89, 56};
    cout << "数组：";
    for (int i = 0; i < 6; i++) cout << numbers[i] << " ";
    cout << endl;

    int max = findMax(numbers, 6);
    cout << "最大值：" << max << endl;
    cout << endl;

    cout << "为什么用const指针？" << endl;
    cout << "  ✓ 函数只读取数组，不修改" << endl;
    cout << "  ✓ const承诺不修改，更安全" << endl;
    cout << "  ✓ 调用者可以放心传递数据" << endl;
    cout << endl;

    // ========================================
    // 练习4：引用交换变量
    // ========================================
    cout << "【练习4：引用交换变量】" << endl;

    int x = 10, y = 20;
    cout << "交换前：x = " << x << ", y = " << y << endl;

    swap(x, y);

    cout << "交换后：x = " << x << ", y = " << y << endl;
    cout << endl;

    cout << "为什么用引用？" << endl;
    cout << "  ✓ 需要修改两个变量" << endl;
    cout << "  ✓ 比指针更简洁（不需要&和*）" << endl;
    cout << endl;

    // ========================================
    // 练习5：const引用返回值
    // ========================================
    cout << "【练习5：const引用返回值】" << endl;

    string s1 = "Hello";
    string s2 = "World";

    string result = concatenate(s1, s2);
    cout << "拼接结果：" << result << endl;
    cout << endl;

    cout << "为什么参数用const引用？" << endl;
    cout << "  ✓ 避免拷贝大字符串" << endl;
    cout << "  ✓ 保证不修改原字符串" << endl;
    cout << endl;

    // ========================================
    // 练习6：const和非const参数组合
    // ========================================
    cout << "【练习6：const和非const参数组合】" << endl;

    int input[5] = {1, 2, 3, 4, 5};
    int output[5];

    cout << "输入数组：";
    for (int i = 0; i < 5; i++) cout << input[i] << " ";
    cout << endl;

    processArray(input, output, 5);

    cout << "输出数组：";
    for (int i = 0; i < 5; i++) cout << output[i] << " ";
    cout << endl;
    cout << endl;

    cout << "参数设计：" << endl;
    cout << "  const int* input  → 只读，不修改" << endl;
    cout << "  int* output       → 可写，用于输出结果" << endl;
    cout << "  int size          → 只读，传值即可" << endl;
    cout << endl;

    // ========================================
    // 练习7：参数传递方式对比
    // ========================================
    cout << "【练习7：参数传递方式对比】" << endl;
    cout << endl;

    cout << "┌──────────────────┬────────┬────────┬────────┐" << endl;
    cout << "│   传递方式       │ 拷贝？ │ 可修改 │ 使用场景│" << endl;
    cout << "├──────────────────┼────────┼────────┼────────┤" << endl;
    cout << "│ void f(int x)    │   是   │   否   │ 小对象 │" << endl;
    cout << "│ void f(int& x)   │   否   │   是   │ 需修改 │" << endl;
    cout << "│ void f(const int&│   否   │   否   │ 大对象 │" << endl;
    cout << "│ void f(int* x)   │   否   │   是   │ 可为空 │" << endl;
    cout << "│ void f(const int*│   否   │   否   │ 数组   │" << endl;
    cout << "└──────────────────┴────────┴────────┴────────┘" << endl;
    cout << endl;

    // ========================================
    // 练习8：实战案例 - 学生成绩管理
    // ========================================
    cout << "【练习8：实战案例 - 学生成绩管理】" << endl;

    struct Student {
        string name;
        int age;
        double score;
    };

    Student students[3] = {
        {"Alice", 20, 95.5},
        {"Bob", 21, 88.0},
        {"Charlie", 19, 92.5}
    };

    // 函数：打印学生信息（const引用，不修改）
    auto printStudentInfo = [](const Student& s) {
        cout << "姓名：" << s.name
             << ", 年龄：" << s.age
             << ", 成绩：" << s.score << endl;
    };

    // 函数：更新学生成绩（引用，需要修改）
    auto updateStudentScore = [](Student& s, double newScore) {
        s.score = newScore;
    };

    cout << "学生列表：" << endl;
    for (int i = 0; i < 3; i++) {
        printStudentInfo(students[i]);
    }
    cout << endl;

    cout << "更新Bob的成绩为90.0：" << endl;
    updateStudentScore(students[1], 90.0);
    printStudentInfo(students[1]);
    cout << endl;

    // ========================================
    // 练习9：const的最佳实践
    // ========================================
    cout << "【练习9：const的最佳实践】" << endl;
    cout << endl;

    cout << "✓ 规则1：默认使用const" << endl;
    cout << "  如果不需要修改，就加const" << endl;
    cout << endl;

    cout << "✓ 规则2：大对象用const引用传递" << endl;
    cout << "  避免拷贝，提高效率" << endl;
    cout << endl;

    cout << "✓ 规则3：需要修改时用引用" << endl;
    cout << "  比指针更安全、更简洁" << endl;
    cout << endl;

    cout << "✓ 规则4：指针可为空时才用指针" << endl;
    cout << "  否则优先用引用" << endl;
    cout << endl;

    cout << "✓ 规则5：const承诺不修改" << endl;
    cout << "  增强代码可读性和安全性" << endl;
    cout << endl;

    // ========================================
    // 练习10：常见错误
    // ========================================
    cout << "【练习10：常见错误】" << endl;
    cout << endl;

    cout << "❌ 错误1：返回局部变量的引用" << endl;
    cout << "   int& bad() {" << endl;
    cout << "       int x = 10;" << endl;
    cout << "       return x;  // 危险！x是局部变量" << endl;
    cout << "   }" << endl;
    cout << endl;

    cout << "❌ 错误2：const引用绑定到临时对象后修改" << endl;
    cout << "   const int& ref = 42;" << endl;
    cout << "   // ref = 50;  // 编译错误！" << endl;
    cout << endl;

    cout << "❌ 错误3：忘记const导致意外修改" << endl;
    cout << "   void print(string& s) {  // 应该用const&" << endl;
    cout << "       s = \"modified\";  // 意外修改！" << endl;
    cout << "   }" << endl;
    cout << endl;

    cout << "✅ 正确做法：" << endl;
    cout << "   1. 不返回局部变量的引用" << endl;
    cout << "   2. 只读参数用const引用" << endl;
    cout << "   3. 需要修改时才用非const引用" << endl;
    cout << endl;

    // ========================================
    // 总结
    // ========================================
    cout << "【总结】" << endl;
    cout << endl;

    cout << "引用的优势：" << endl;
    cout << "  ✓ 语法简洁，像普通变量" << endl;
    cout << "  ✓ 避免拷贝，提高效率" << endl;
    cout << "  ✓ 不会为空，更安全" << endl;
    cout << endl;

    cout << "const的价值：" << endl;
    cout << "  ✓ 保护数据不被意外修改" << endl;
    cout << "  ✓ 表达设计意图（只读）" << endl;
    cout << "  ✓ 编译器帮助检查错误" << endl;
    cout << endl;

    cout << "组合使用：" << endl;
    cout << "  const引用 = 高效 + 安全" << endl;
    cout << "  是C++函数参数的最佳实践！" << endl;
    cout << endl;

    return 0;
}

// ========================================
// 函数实现
// ========================================

// 用const引用传递大对象
void printStudent(const string& name, const int& age, const double& score) {
    cout << "姓名：" << name << ", 年龄：" << age << ", 成绩：" << score << endl;
}

// 用引用修改数据
void updateScore(double& score, double delta) {
    score += delta;
}

// 用const指针保护数组
int findMax(const int* arr, int size) {
    int max = arr[0];
    for (int i = 1; i < size; i++) {
        if (arr[i] > max) {
            max = arr[i];
        }
    }
    return max;
}

// 用引用交换变量
void swap(int& a, int& b) {
    int temp = a;
    a = b;
    b = temp;
}

// const引用参数
string concatenate(const string& s1, const string& s2) {
    return s1 + " " + s2;
}

// const和非const参数组合
void processArray(const int* input, int* output, int size) {
    for (int i = 0; i < size; i++) {
        output[i] = input[i] * 2;  // 读取input，写入output
    }
}
