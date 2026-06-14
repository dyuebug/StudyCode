// ============================================
// 01_hello_world.cpp - C++ 入门练习
// 日期：2026-06-14（第1天）
// 内容：命名空间、引用、函数重载
// ============================================

#include <iostream>

// ============================================
// 练习1：命名空间的使用
// ============================================

// 推荐方式：using 声明机制（需要什么声明什么）
using std::cout;
using std::endl;

// 定义自己的命名空间
namespace my_space {
    int number = 100;

    void display() {
        cout << "my_space::display() 被调用" << endl;
        cout << "my_space::number = " << number << endl;
    }
}

// ============================================
// 练习2：引用的基本使用
// ============================================

void test_reference() {
    cout << "\n=== 练习2：引用的基本使用 ===" << endl;

    int num = 10;
    int& ref = num;  // ref 是 num 的别名

    cout << "num = " << num << endl;
    cout << "ref = " << ref << endl;

    // 修改引用，原变量也会改变
    ref = 20;
    cout << "修改 ref 后，num = " << num << endl;

    // 引用和原变量的地址相同
    cout << "&num = " << &num << endl;
    cout << "&ref = " << &ref << endl;
}

// ============================================
// 练习3：引用作为函数参数
// ============================================

// 值传递（不会交换）
void swap_by_value(int x, int y) {
    int temp = x;
    x = y;
    y = temp;
    // 注意：这里只交换了形参，不影响实参
}

// 引用传递（会交换）⭐⭐⭐⭐⭐
void swap_by_reference(int& x, int& y) {
    int temp = x;
    x = y;
    y = temp;
    // 引用传递，直接修改实参
}

void test_swap() {
    cout << "\n=== 练习3：值传递 vs 引用传递 ===" << endl;

    int a = 10, b = 20;
    cout << "交换前：a = " << a << ", b = " << b << endl;

    // 值传递（不会交换）
    swap_by_value(a, b);
    cout << "值传递后：a = " << a << ", b = " << b << endl;

    // 引用传递（会交换）
    swap_by_reference(a, b);
    cout << "引用传递后：a = " << a << ", b = " << b << endl;
}

// ============================================
// 练习4：函数重载
// ============================================

// 重载1：交换两个 int
void swap(int& a, int& b) {
    cout << "调用 swap(int&, int&)" << endl;
    int temp = a;
    a = b;
    b = temp;
}

// 重载2：交换两个 double
void swap(double& a, double& b) {
    cout << "调用 swap(double&, double&)" << endl;
    double temp = a;
    a = b;
    b = temp;
}

// 重载3：打印不同类型
void print(int x) {
    cout << "int: " << x << endl;
}

void print(double x) {
    cout << "double: " << x << endl;
}

void print(const char* str) {
    cout << "string: " << str << endl;
}

void test_overload() {
    cout << "\n=== 练习4：函数重载 ===" << endl;

    // 测试重载的 swap 函数
    int x = 10, y = 20;
    cout << "交换前：x = " << x << ", y = " << y << endl;
    swap(x, y);  // 调用 swap(int&, int&)
    cout << "交换后：x = " << x << ", y = " << y << endl;

    double m = 1.5, n = 2.5;
    cout << "交换前：m = " << m << ", n = " << n << endl;
    swap(m, n);  // 调用 swap(double&, double&)
    cout << "交换后：m = " << m << ", n = " << n << endl;

    // 测试重载的 print 函数
    cout << "\n测试 print 函数重载：" << endl;
    print(100);           // 调用 print(int)
    print(3.14);          // 调用 print(double)
    print("Hello C++");   // 调用 print(const char*)
}

// ============================================
// 主函数
// ============================================

int main() {
    // Hello World
    cout << "=== Hello, C++! ===" << endl;
    cout << "这是我的第一个 C++ 程序！" << endl;

    // 测试命名空间
    cout << "\n=== 练习1：命名空间 ===" << endl;
    my_space::display();

    // 测试引用
    test_reference();

    // 测试值传递 vs 引用传递
    test_swap();

    // 测试函数重载
    test_overload();

    cout << "\n=== 程序结束 ===" << endl;
    return 0;
}

/*
编译运行：
g++ -o 01_hello_world 01_hello_world.cpp -std=c++17
./01_hello_world

预期输出：
=== Hello, C++! ===
这是我的第一个 C++ 程序！

=== 练习1：命名空间 ===
my_space::display() 被调用
my_space::number = 100

=== 练习2：引用的基本使用 ===
num = 10
ref = 10
修改 ref 后，num = 20
&num = 0x7ffc8a2b3c1c
&ref = 0x7ffc8a2b3c1c

=== 练习3：值传递 vs 引用传递 ===
交换前：a = 10, b = 20
值传递后：a = 10, b = 20
引用传递后：a = 20, b = 10

=== 练习4：函数重载 ===
交换前：x = 10, y = 20
调用 swap(int&, int&)
交换后：x = 20, y = 10
交换前：m = 1.5, n = 2.5
调用 swap(double&, double&)
交换后：m = 2.5, n = 1.5

测试 print 函数重载：
int: 100
double: 3.14
string: Hello C++

=== 程序结束 ===
*/
