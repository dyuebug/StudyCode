/*
 * ============================================================================
 * 文件名：25_const_keyword.cpp
 * 主题：const关键字详解
 * 学习目标：
 *   1. 理解const的作用（保护数据不被修改）
 *   2. 掌握const变量的声明和使用
 *   3. 理解const指针的四种组合
 *   4. 掌握const引用的使用
 *   5. 学会在函数中使用const
 *
 * 编译命令：g++ -std=c++11 -o 25_const_keyword 25_const_keyword.cpp
 * 运行命令：./25_const_keyword (Linux/Mac) 或 25_const_keyword.exe (Windows)
 * ============================================================================
 */

#include <iostream>
#include <string>
using namespace std;

// 函数声明
void printValue(const int& x);
void modifyArray(int* arr, int size);
void readArray(const int* arr, int size);
int getMax(const int* arr, int size);

int main() {
    cout << "=== const关键字详解 ===" << endl << endl;

    // ========================================
    // 1. 什么是const？
    // ========================================
    cout << "【1. 什么是const？】" << endl;
    cout << "const = constant（常量）" << endl;
    cout << "作用：声明变量为只读，防止被意外修改" << endl;
    cout << "好处：提高代码安全性，表达设计意图" << endl;
    cout << endl;

    // ========================================
    // 2. const变量
    // ========================================
    cout << "【2. const变量】" << endl;

    const int MAX_SIZE = 100;
    cout << "const int MAX_SIZE = 100;" << endl;
    cout << "MAX_SIZE = " << MAX_SIZE << endl;
    // MAX_SIZE = 200;  // 编译错误！const变量不能修改
    cout << "const变量必须在声明时初始化" << endl;
    cout << "初始化后不能修改" << endl;
    cout << endl;

    const double PI = 3.14159;
    cout << "const double PI = 3.14159;" << endl;
    cout << "PI = " << PI << endl;
    cout << "用const定义常量，比#define更安全（有类型检查）" << endl;
    cout << endl;

    // ========================================
    // 3. const指针（四种组合）
    // ========================================
    cout << "【3. const指针（四种组合）】" << endl;
    cout << endl;

    int x = 10, y = 20;

    // 组合1：普通指针（可以修改指针，可以修改指向的值）
    cout << "组合1：普通指针" << endl;
    int* p1 = &x;
    cout << "int* p1 = &x;" << endl;
    cout << "*p1 = " << *p1 << endl;
    *p1 = 15;  // 可以修改指向的值
    cout << "修改 *p1 = 15 后：x = " << x << endl;
    p1 = &y;   // 可以修改指针指向
    cout << "修改 p1 = &y 后：*p1 = " << *p1 << endl;
    cout << endl;

    // 组合2：指向常量的指针（可以修改指针，不能修改指向的值）
    cout << "组合2：指向常量的指针（const在*左边）" << endl;
    const int* p2 = &x;
    cout << "const int* p2 = &x;" << endl;
    cout << "*p2 = " << *p2 << endl;
    // *p2 = 25;  // 编译错误！不能通过p2修改x
    p2 = &y;      // 可以修改指针指向
    cout << "可以修改指针：p2 = &y 后：*p2 = " << *p2 << endl;
    cout << "不能修改值：*p2 = 25 会编译错误" << endl;
    cout << "记忆：const修饰*p2，所以*p2不能改" << endl;
    cout << endl;

    // 组合3：常量指针（不能修改指针，可以修改指向的值）
    cout << "组合3：常量指针（const在*右边）" << endl;
    int* const p3 = &x;
    cout << "int* const p3 = &x;" << endl;
    cout << "*p3 = " << *p3 << endl;
    *p3 = 35;     // 可以修改指向的值
    cout << "可以修改值：*p3 = 35 后：x = " << x << endl;
    // p3 = &y;   // 编译错误！不能修改指针指向
    cout << "不能修改指针：p3 = &y 会编译错误" << endl;
    cout << "记忆：const修饰p3，所以p3不能改" << endl;
    cout << endl;

    // 组合4：指向常量的常量指针（不能修改指针，不能修改指向的值）
    cout << "组合4：指向常量的常量指针（const在两边）" << endl;
    const int* const p4 = &x;
    cout << "const int* const p4 = &x;" << endl;
    cout << "*p4 = " << *p4 << endl;
    // *p4 = 45;  // 编译错误！不能修改指向的值
    // p4 = &y;   // 编译错误！不能修改指针指向
    cout << "不能修改值：*p4 = 45 会编译错误" << endl;
    cout << "不能修改指针：p4 = &y 会编译错误" << endl;
    cout << "记忆：两个const，什么都不能改" << endl;
    cout << endl;

    // ========================================
    // 4. const指针速记表
    // ========================================
    cout << "【4. const指针速记表】" << endl;
    cout << "┌────────────────────────┬──────────┬──────────┐" << endl;
    cout << "│      声明方式          │ 能改指针 │ 能改值   │" << endl;
    cout << "├────────────────────────┼──────────┼──────────┤" << endl;
    cout << "│ int* p                 │    ✓     │    ✓     │" << endl;
    cout << "│ const int* p           │    ✓     │    ✗     │" << endl;
    cout << "│ int* const p           │    ✗     │    ✓     │" << endl;
    cout << "│ const int* const p     │    ✗     │    ✗     │" << endl;
    cout << "└────────────────────────┴──────────┴──────────┘" << endl;
    cout << endl;

    cout << "记忆技巧：" << endl;
    cout << "  const在*左边 → 指向的值不能改" << endl;
    cout << "  const在*右边 → 指针本身不能改" << endl;
    cout << "  两边都有const → 什么都不能改" << endl;
    cout << endl;

    // ========================================
    // 5. const引用
    // ========================================
    cout << "【5. const引用】" << endl;

    int num = 100;
    const int& ref = num;

    cout << "int num = 100;" << endl;
    cout << "const int& ref = num;" << endl;
    cout << "ref = " << ref << endl;
    // ref = 200;  // 编译错误！const引用不能修改
    num = 200;     // 可以通过原变量修改
    cout << "通过原变量修改：num = 200 后：ref = " << ref << endl;
    cout << endl;

    cout << "const引用的作用：" << endl;
    cout << "  ✓ 防止通过引用意外修改原变量" << endl;
    cout << "  ✓ 可以绑定到临时对象和字面量" << endl;
    cout << "  ✓ 常用于函数参数（高效且安全）" << endl;
    cout << endl;

    // const引用可以绑定到字面量
    const int& literal_ref = 42;
    cout << "const int& literal_ref = 42; → " << literal_ref << endl;
    cout << "普通引用不能绑定到字面量，const引用可以" << endl;
    cout << endl;

    // ========================================
    // 6. const与函数参数
    // ========================================
    cout << "【6. const与函数参数】" << endl;

    int value = 50;
    cout << "调用 printValue(value)：" << endl;
    printValue(value);
    cout << "const引用参数：高效（不拷贝）且安全（不修改）" << endl;
    cout << endl;

    int arr[5] = {1, 2, 3, 4, 5};
    cout << "数组：";
    for (int i = 0; i < 5; i++) cout << arr[i] << " ";
    cout << endl;

    cout << "调用 readArray(arr, 5)：" << endl;
    readArray(arr, 5);
    cout << "const指针参数：承诺不修改数组" << endl;
    cout << endl;

    // ========================================
    // 7. const的实际应用
    // ========================================
    cout << "【7. const的实际应用】" << endl;

    int scores[6] = {85, 92, 78, 95, 88, 90};
    cout << "成绩数组：";
    for (int i = 0; i < 6; i++) cout << scores[i] << " ";
    cout << endl;

    int max_score = getMax(scores, 6);
    cout << "最高分：" << max_score << endl;
    cout << "getMax函数使用const参数，保证不会修改数组" << endl;
    cout << endl;

    // ========================================
    // 8. const与字符串
    // ========================================
    cout << "【8. const与字符串】" << endl;

    // 字符串字面量是const
    const char* str1 = "Hello";
    cout << "const char* str1 = \"Hello\";" << endl;
    cout << "str1 = " << str1 << endl;
    // str1[0] = 'h';  // 编译错误！字符串字面量是只读的
    cout << "字符串字面量是const，不能修改" << endl;
    cout << endl;

    // C++ string类
    const string str2 = "World";
    cout << "const string str2 = \"World\";" << endl;
    cout << "str2 = " << str2 << endl;
    // str2 = "Changed";  // 编译错误！const string不能修改
    cout << "const string对象不能修改" << endl;
    cout << endl;

    // ========================================
    // 9. const的最佳实践
    // ========================================
    cout << "【9. const的最佳实践】" << endl;

    cout << "✓ 使用const的场景：" << endl;
    cout << "  1. 定义常量（替代#define）" << endl;
    cout << "  2. 函数参数（不需要修改的参数）" << endl;
    cout << "  3. 函数返回值（返回不应被修改的数据）" << endl;
    cout << "  4. 成员函数（不修改对象状态）" << endl;
    cout << endl;

    cout << "✓ const的好处：" << endl;
    cout << "  1. 提高代码安全性（防止意外修改）" << endl;
    cout << "  2. 表达设计意图（这个值不应该改）" << endl;
    cout << "  3. 编译器优化（const变量可能被优化）" << endl;
    cout << "  4. 接口更清晰（一眼看出哪些会被修改）" << endl;
    cout << endl;

    // ========================================
    // 10. const常见错误
    // ========================================
    cout << "【10. const常见错误】" << endl;

    cout << "❌ 错误1：const变量未初始化" << endl;
    cout << "   // const int x;  // 编译错误！" << endl;
    cout << "   const int x = 10;  // 正确" << endl;
    cout << endl;

    cout << "❌ 错误2：试图修改const变量" << endl;
    cout << "   const int y = 20;" << endl;
    cout << "   // y = 30;  // 编译错误！" << endl;
    cout << endl;

    cout << "❌ 错误3：const指针类型不匹配" << endl;
    cout << "   int z = 40;" << endl;
    cout << "   // const int* p = &z;  // 正确" << endl;
    cout << "   // int* p = &z;  // 如果z是const，这是错误的" << endl;
    cout << endl;

    cout << "❌ 错误4：返回局部变量的const引用" << endl;
    cout << "   // const int& func() {" << endl;
    cout << "   //     int local = 10;" << endl;
    cout << "   //     return local;  // 危险！悬空引用" << endl;
    cout << "   // }" << endl;
    cout << endl;

    // ========================================
    // 11. const总结
    // ========================================
    cout << "【11. const总结】" << endl;

    cout << "const是C++中重要的安全机制：" << endl;
    cout << "  • const变量：声明常量" << endl;
    cout << "  • const指针：保护指针或指向的值" << endl;
    cout << "  • const引用：高效且安全的参数传递" << endl;
    cout << "  • const函数：承诺不修改数据" << endl;
    cout << endl;

    cout << "记住：能用const就用const，让编译器帮你保证安全！" << endl;
    cout << endl;

    return 0;
}

// ========================================
// 函数实现
// ========================================

// const引用参数：不拷贝，不修改
void printValue(const int& x) {
    cout << "  值：" << x << endl;
    // x = 100;  // 编译错误！const引用不能修改
}

// 普通指针参数：可以修改数组
void modifyArray(int* arr, int size) {
    for (int i = 0; i < size; i++) {
        arr[i] *= 2;
    }
}

// const指针参数：不能修改数组
void readArray(const int* arr, int size) {
    cout << "  数组元素：";
    for (int i = 0; i < size; i++) {
        cout << arr[i] << " ";
        // arr[i] = 0;  // 编译错误！const指针不能修改
    }
    cout << endl;
}

// const参数：保证不修改数组
int getMax(const int* arr, int size) {
    int max = arr[0];
    for (int i = 1; i < size; i++) {
        if (arr[i] > max) {
            max = arr[i];
        }
    }
    return max;
}
