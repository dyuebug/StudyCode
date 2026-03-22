/*
 * ============================================================================
 * 文件名：24_reference_basics.cpp
 * 主题：引用基础详解
 * 学习目标：
 *   1. 理解引用的概念（变量的别名）
 *   2. 掌握引用的声明和初始化
 *   3. 理解引用与指针的区别
 *   4. 学会使用引用作为函数参数
 *   5. 掌握引用作为函数返回值的使用
 *
 * 编译命令：g++ -std=c++11 -o 24_reference_basics 24_reference_basics.cpp
 * 运行命令：./24_reference_basics (Linux/Mac) 或 24_reference_basics.exe (Windows)
 * ============================================================================
 */

#include <iostream>
using namespace std;

// 函数声明
void swapByValue(int a, int b);
void swapByPointer(int* a, int* b);
void swapByReference(int& a, int& b);
void printArray(int arr[], int size);
void modifyByReference(int& x);
int& getElement(int arr[], int index);

int main() {
    cout << "=== 引用基础详解 ===" << endl << endl;

    // ========================================
    // 1. 什么是引用？
    // ========================================
    cout << "【1. 什么是引用？】" << endl;
    cout << "引用是变量的别名（alias），就像一个人有多个名字" << endl;
    cout << "引用和原变量共享同一块内存" << endl;
    cout << endl;

    int num = 42;
    int& ref = num;  // ref 是 num 的引用（别名）

    cout << "原变量 num = " << num << endl;
    cout << "引用 ref = " << ref << endl;
    cout << "num 的地址：" << &num << endl;
    cout << "ref 的地址：" << &ref << endl;
    cout << "地址相同！引用和原变量是同一个东西" << endl;
    cout << endl;

    // 修改引用就是修改原变量
    ref = 100;
    cout << "修改 ref = 100 后：" << endl;
    cout << "num = " << num << endl;
    cout << "ref = " << ref << endl;
    cout << "修改引用就是修改原变量！" << endl;
    cout << endl;

    // ========================================
    // 2. 引用的声明和初始化
    // ========================================
    cout << "【2. 引用的声明和初始化】" << endl;

    int x = 10;
    int& rx = x;  // 正确：声明时必须初始化

    cout << "int& rx = x; → rx = " << rx << endl;
    cout << endl;

    cout << "引用的规则：" << endl;
    cout << "  1. 声明时必须初始化" << endl;
    cout << "  2. 初始化后不能改变引用对象" << endl;
    cout << "  3. 引用不是对象，没有自己的地址" << endl;
    cout << endl;

    // 引用初始化后不能改变
    int y = 20;
    rx = y;  // 这不是让 rx 引用 y，而是把 y 的值赋给 x
    cout << "rx = y 后：" << endl;
    cout << "x = " << x << " (x 的值变成了 20)" << endl;
    cout << "y = " << y << endl;
    cout << "rx 仍然引用 x，只是把 y 的值赋给了 x" << endl;
    cout << endl;

    // ========================================
    // 3. 引用 vs 指针
    // ========================================
    cout << "【3. 引用 vs 指针】" << endl;

    int value = 50;
    int* ptr = &value;  // 指针
    int& rvalue = value;  // 引用

    cout << "原变量：value = " << value << endl;
    cout << endl;

    cout << "指针访问：" << endl;
    cout << "  int* ptr = &value;" << endl;
    cout << "  *ptr = " << *ptr << " (需要解引用)" << endl;
    cout << "  ptr = " << ptr << " (指针本身的值是地址)" << endl;
    cout << endl;

    cout << "引用访问：" << endl;
    cout << "  int& rvalue = value;" << endl;
    cout << "  rvalue = " << rvalue << " (直接使用，像普通变量)" << endl;
    cout << "  &rvalue = " << &rvalue << " (取地址得到原变量地址)" << endl;
    cout << endl;

    cout << "关键区别：" << endl;
    cout << "┌─────────────┬──────────────┬──────────────┐" << endl;
    cout << "│   特性      │    指针      │    引用      │" << endl;
    cout << "├─────────────┼──────────────┼──────────────┤" << endl;
    cout << "│ 本质        │ 独立变量     │ 别名         │" << endl;
    cout << "│ 初始化      │ 可以不初始化 │ 必须初始化   │" << endl;
    cout << "│ 可否为空    │ 可以为nullptr│ 不能为空     │" << endl;
    cout << "│ 可否改变    │ 可以重新指向 │ 不能改变引用 │" << endl;
    cout << "│ 使用方式    │ 需要*解引用  │ 直接使用     │" << endl;
    cout << "│ 占用内存    │ 占用内存     │ 不占用内存   │" << endl;
    cout << "└─────────────┴──────────────┴──────────────┘" << endl;
    cout << endl;

    // ========================================
    // 4. 引用作为函数参数
    // ========================================
    cout << "【4. 引用作为函数参数】" << endl;

    int a = 10, b = 20;

    cout << "交换前：a = " << a << ", b = " << b << endl;
    cout << endl;

    // 方式1：值传递（无效）
    cout << "方式1：值传递" << endl;
    swapByValue(a, b);
    cout << "交换后：a = " << a << ", b = " << b << " (失败！)" << endl;
    cout << "原因：传递的是副本，不影响原变量" << endl;
    cout << endl;

    // 方式2：指针传递（有效但麻烦）
    cout << "方式2：指针传递" << endl;
    swapByPointer(&a, &b);
    cout << "交换后：a = " << a << ", b = " << b << " (成功！)" << endl;
    cout << "原因：传递地址，可以修改原变量" << endl;
    cout << "缺点：需要传地址、需要解引用" << endl;
    cout << endl;

    // 方式3：引用传递（有效且简洁）
    cout << "方式3：引用传递" << endl;
    swapByReference(a, b);
    cout << "交换后：a = " << a << ", b = " << b << " (成功！)" << endl;
    cout << "原因：引用就是别名，直接操作原变量" << endl;
    cout << "优点：语法简洁，像操作普通变量" << endl;
    cout << endl;

    // ========================================
    // 5. 引用传参的优势
    // ========================================
    cout << "【5. 引用传参的优势】" << endl;

    int arr[5] = {1, 2, 3, 4, 5};

    cout << "数组元素：";
    printArray(arr, 5);
    cout << endl;

    cout << "引用传参的好处：" << endl;
    cout << "  ✓ 避免拷贝大对象（提高效率）" << endl;
    cout << "  ✓ 可以修改原变量" << endl;
    cout << "  ✓ 语法简洁，不需要指针操作" << endl;
    cout << "  ✓ 更安全（不会为空）" << endl;
    cout << endl;

    // ========================================
    // 6. 通过引用修改变量
    // ========================================
    cout << "【6. 通过引用修改变量】" << endl;

    int n = 5;
    cout << "修改前：n = " << n << endl;

    modifyByReference(n);

    cout << "修改后：n = " << n << endl;
    cout << "函数通过引用参数修改了原变量" << endl;
    cout << endl;

    // ========================================
    // 7. 引用作为函数返回值
    // ========================================
    cout << "【7. 引用作为函数返回值】" << endl;

    int data[3] = {10, 20, 30};
    cout << "数组：";
    printArray(data, 3);

    // 通过引用返回值修改数组元素
    getElement(data, 1) = 99;

    cout << "修改 data[1] 后：";
    printArray(data, 3);
    cout << "函数返回引用，可以作为左值被赋值" << endl;
    cout << endl;

    // ========================================
    // 8. 引用的注意事项
    // ========================================
    cout << "【8. 引用的注意事项】" << endl;

    cout << "⚠️ 注意事项：" << endl;
    cout << "  1. 不要返回局部变量的引用（悬空引用）" << endl;
    cout << "  2. 引用必须初始化" << endl;
    cout << "  3. 引用不能为空" << endl;
    cout << "  4. 引用不能改变引用对象" << endl;
    cout << endl;

    cout << "❌ 错误示例：" << endl;
    cout << "  int& func() {" << endl;
    cout << "      int local = 10;" << endl;
    cout << "      return local;  // 危险！返回局部变量的引用" << endl;
    cout << "  }" << endl;
    cout << "  // local 在函数结束后被销毁，引用指向无效内存" << endl;
    cout << endl;

    // ========================================
    // 9. 何时使用引用？
    // ========================================
    cout << "【9. 何时使用引用？】" << endl;

    cout << "使用引用的场景：" << endl;
    cout << "  ✓ 函数参数传递大对象（避免拷贝）" << endl;
    cout << "  ✓ 需要在函数中修改外部变量" << endl;
    cout << "  ✓ 运算符重载（后续学习）" << endl;
    cout << "  ✓ 范围for循环（for (auto& item : container)）" << endl;
    cout << endl;

    cout << "使用指针的场景：" << endl;
    cout << "  ✓ 需要重新指向不同对象" << endl;
    cout << "  ✓ 可能为空的情况" << endl;
    cout << "  ✓ 动态内存分配" << endl;
    cout << "  ✓ 数组和指针运算" << endl;
    cout << endl;

    // ========================================
    // 10. 引用的本质
    // ========================================
    cout << "【10. 引用的本质】" << endl;

    cout << "引用的本质：" << endl;
    cout << "  - 编译器层面：引用通常被实现为指针" << endl;
    cout << "  - 语法层面：引用是变量的别名" << endl;
    cout << "  - 使用层面：引用更安全、更简洁" << endl;
    cout << endl;

    cout << "引用 = 受限的指针 + 更好的语法" << endl;
    cout << endl;

    return 0;
}

// ========================================
// 函数实现
// ========================================

// 值传递：无法交换原变量
void swapByValue(int a, int b) {
    int temp = a;
    a = b;
    b = temp;
    // a 和 b 是副本，不影响原变量
}

// 指针传递：可以交换原变量
void swapByPointer(int* a, int* b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

// 引用传递：可以交换原变量，语法更简洁
void swapByReference(int& a, int& b) {
    int temp = a;
    a = b;
    b = temp;
    // a 和 b 是原变量的引用，直接修改原变量
}

// 打印数组
void printArray(int arr[], int size) {
    for (int i = 0; i < size; i++) {
        cout << arr[i] << " ";
    }
    cout << endl;
}

// 通过引用修改变量
void modifyByReference(int& x) {
    x = x * 2;
}

// 返回数组元素的引用
int& getElement(int arr[], int index) {
    return arr[index];
}
