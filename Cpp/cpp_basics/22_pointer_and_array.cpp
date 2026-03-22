/*
 * ============================================================================
 * 文件名：22_pointer_and_array.cpp
 * 主题：指针与数组详解
 * 学习目标：
 *   1. 理解数组名就是指针
 *   2. 掌握用指针遍历数组
 *   3. 学会指针运算（p++、p+i）
 *   4. 理解数组参数传递的本质
 *   5. 掌握指针与多维数组的关系
 *
 * 编译命令：g++ -std=c++11 -o 22_pointer_and_array 22_pointer_and_array.cpp
 * 运行命令：./22_pointer_and_array (Linux/Mac) 或 22_pointer_and_array.exe (Windows)
 * ============================================================================
 */

#include <iostream>
using namespace std;

// 函数声明
void printArray(int* arr, int size);
int sumArray(int* arr, int size);

int main() {
    cout << "=== 指针与数组详解 ===" << endl << endl;

    // ========================================
    // 1. 数组名就是指针
    // ========================================
    cout << "【1. 数组名就是指针】" << endl;

    int arr[5] = {10, 20, 30, 40, 50};

    cout << "数组 arr：";
    for (int i = 0; i < 5; i++) {
        cout << arr[i] << " ";
    }
    cout << endl;

    cout << "arr（数组名）的值：" << arr << endl;
    cout << "&arr[0]（首元素地址）：" << &arr[0] << endl;
    cout << "数组名 arr 就是指向首元素的指针！" << endl;
    cout << endl;

    // 用指针访问数组元素
    int* p = arr;  // 等价于 int* p = &arr[0];
    cout << "int* p = arr;" << endl;
    cout << "p[0] = " << p[0] << endl;
    cout << "p[1] = " << p[1] << endl;
    cout << "*p = " << *p << " (等价于 arr[0])" << endl;
    cout << "*(p+1) = " << *(p+1) << " (等价于 arr[1])" << endl;
    cout << endl;

    // ========================================
    // 2. 数组访问的两种方式
    // ========================================
    cout << "【2. 数组访问的两种方式】" << endl;

    int nums[4] = {1, 2, 3, 4};

    cout << "方式1：下标访问" << endl;
    for (int i = 0; i < 4; i++) {
        cout << "nums[" << i << "] = " << nums[i] << endl;
    }
    cout << endl;

    cout << "方式2：指针访问" << endl;
    for (int i = 0; i < 4; i++) {
        cout << "*(nums + " << i << ") = " << *(nums + i) << endl;
    }
    cout << endl;

    cout << "两种方式完全等价：" << endl;
    cout << "  arr[i] 等价于 *(arr + i)" << endl;
    cout << "  &arr[i] 等价于 (arr + i)" << endl;
    cout << endl;

    // ========================================
    // 3. 指针遍历数组
    // ========================================
    cout << "【3. 指针遍历数组】" << endl;

    int data[5] = {5, 10, 15, 20, 25};
    int* ptr = data;

    cout << "方式1：指针递增遍历" << endl;
    for (int i = 0; i < 5; i++) {
        cout << "*ptr = " << *ptr << ", 地址：" << ptr << endl;
        ptr++;  // 指针移动到下一个元素
    }
    cout << endl;

    // 重置指针
    ptr = data;

    cout << "方式2：指针加偏移量遍历" << endl;
    for (int i = 0; i < 5; i++) {
        cout << "*(ptr + " << i << ") = " << *(ptr + i) << endl;
    }
    cout << endl;

    // ========================================
    // 4. 指针运算
    // ========================================
    cout << "【4. 指针运算】" << endl;

    int values[5] = {100, 200, 300, 400, 500};
    int* p1 = values;
    int* p2 = values + 2;

    cout << "p1 指向 values[0]，值：" << *p1 << "，地址：" << p1 << endl;
    cout << "p2 指向 values[2]，值：" << *p2 << "，地址：" << p2 << endl;
    cout << endl;

    cout << "指针运算：" << endl;
    cout << "p1 + 1 的值：" << *(p1 + 1) << " (values[1])" << endl;
    cout << "p1 + 3 的值：" << *(p1 + 3) << " (values[3])" << endl;
    cout << "p2 - 1 的值：" << *(p2 - 1) << " (values[1])" << endl;
    cout << endl;

    cout << "指针相减（计算元素间距）：" << endl;
    cout << "p2 - p1 = " << (p2 - p1) << " (相隔2个元素)" << endl;
    cout << endl;

    cout << "指针递增/递减：" << endl;
    int* p3 = values;
    cout << "初始：*p3 = " << *p3 << endl;
    p3++;
    cout << "p3++ 后：*p3 = " << *p3 << endl;
    p3 += 2;
    cout << "p3 += 2 后：*p3 = " << *p3 << endl;
    cout << endl;

    // ========================================
    // 5. 指针运算的本质
    // ========================================
    cout << "【5. 指针运算的本质】" << endl;

    int arr2[3] = {10, 20, 30};
    int* pa = arr2;

    cout << "pa 的地址：" << pa << endl;
    cout << "pa + 1 的地址：" << (pa + 1) << endl;
    cout << "pa + 2 的地址：" << (pa + 2) << endl;
    cout << endl;

    cout << "关键理解：" << endl;
    cout << "  指针 + 1 不是地址 + 1 字节" << endl;
    cout << "  而是地址 + sizeof(类型) 字节" << endl;
    cout << "  int* 指针 + 1 = 地址 + " << sizeof(int) << " 字节" << endl;
    cout << endl;

    // ========================================
    // 6. 数组参数传递
    // ========================================
    cout << "【6. 数组参数传递】" << endl;

    int test[5] = {1, 2, 3, 4, 5};

    cout << "调用 printArray(test, 5)：" << endl;
    printArray(test, 5);
    cout << endl;

    cout << "数组求和：" << sumArray(test, 5) << endl;
    cout << endl;

    cout << "关键理解：" << endl;
    cout << "  数组作为参数传递时，实际传递的是指针" << endl;
    cout << "  void func(int arr[]) 等价于 void func(int* arr)" << endl;
    cout << "  必须额外传递数组长度！" << endl;
    cout << endl;

    // ========================================
    // 7. 指针与字符串
    // ========================================
    cout << "【7. 指针与字符串】" << endl;

    char str[] = "Hello";
    char* pstr = str;

    cout << "字符串：" << str << endl;
    cout << "用指针遍历字符串：" << endl;
    while (*pstr != '\0') {
        cout << *pstr << " ";
        pstr++;
    }
    cout << endl << endl;

    // 字符串字面量
    const char* message = "World";  // 指向字符串字面量
    cout << "字符串字面量：" << message << endl;
    cout << "第一个字符：" << *message << endl;
    cout << "第二个字符：" << *(message + 1) << endl;
    cout << endl;

    // ========================================
    // 8. 指针与二维数组
    // ========================================
    cout << "【8. 指针与二维数组】" << endl;

    int matrix[2][3] = {
        {1, 2, 3},
        {4, 5, 6}
    };

    cout << "二维数组：" << endl;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 3; j++) {
            cout << matrix[i][j] << " ";
        }
        cout << endl;
    }
    cout << endl;

    cout << "用指针访问二维数组：" << endl;
    int* pm = &matrix[0][0];  // 指向首元素
    for (int i = 0; i < 6; i++) {
        cout << "*(pm + " << i << ") = " << *(pm + i) << endl;
    }
    cout << endl;

    cout << "二维数组在内存中是连续存储的" << endl;
    cout << "可以用一维指针遍历" << endl;
    cout << endl;

    // ========================================
    // 9. const 与指针
    // ========================================
    cout << "【9. const 与指针】" << endl;

    int x = 10, y = 20;

    // 指向常量的指针（不能通过指针修改值）
    const int* p_const = &x;
    cout << "const int* p_const = &x;" << endl;
    cout << "*p_const = " << *p_const << endl;
    // *p_const = 30;  // 错误！不能修改
    p_const = &y;  // 可以改变指向
    cout << "可以改变指向：p_const = &y, *p_const = " << *p_const << endl;
    cout << endl;

    // 常量指针（不能改变指向）
    int* const const_p = &x;
    cout << "int* const const_p = &x;" << endl;
    *const_p = 30;  // 可以修改值
    cout << "可以修改值：*const_p = 30, x = " << x << endl;
    // const_p = &y;  // 错误！不能改变指向
    cout << endl;

    // 指向常量的常量指针（都不能改）
    const int* const both_const = &x;
    cout << "const int* const both_const = &x;" << endl;
    cout << "既不能修改值，也不能改变指向" << endl;
    cout << endl;

    // ========================================
    // 10. 常见错误
    // ========================================
    cout << "【10. 常见错误】" << endl;
    cout << "❌ 错误1：数组越界" << endl;
    cout << "   int arr[5];" << endl;
    cout << "   int* p = arr + 10;  // 超出数组范围！" << endl;
    cout << endl;

    cout << "❌ 错误2：忘记传递数组长度" << endl;
    cout << "   void func(int* arr) {" << endl;
    cout << "       // sizeof(arr) 只是指针大小，不是数组大小！" << endl;
    cout << "   }" << endl;
    cout << endl;

    cout << "❌ 错误3：返回局部数组的指针" << endl;
    cout << "   int* func() {" << endl;
    cout << "       int arr[5];  // 局部数组" << endl;
    cout << "       return arr;  // 危险！函数结束后数组被销毁" << endl;
    cout << "   }" << endl;
    cout << endl;

    // ========================================
    // 11. 总结
    // ========================================
    cout << "【11. 总结】" << endl;
    cout << "✅ 数组名是指向首元素的指针" << endl;
    cout << "✅ arr[i] 等价于 *(arr + i)" << endl;
    cout << "✅ 指针可以用来遍历数组" << endl;
    cout << "✅ 指针运算按元素大小移动，不是按字节" << endl;
    cout << "✅ 数组参数传递实际是指针传递" << endl;
    cout << "✅ 必须额外传递数组长度" << endl;
    cout << "✅ 注意数组越界和野指针问题" << endl;

    return 0;
}

// 函数定义：打印数组
void printArray(int* arr, int size) {
    cout << "数组元素：";
    for (int i = 0; i < size; i++) {
        cout << arr[i] << " ";
    }
    cout << endl;
}

// 函数定义：数组求和
int sumArray(int* arr, int size) {
    int sum = 0;
    for (int i = 0; i < size; i++) {
        sum += *(arr + i);  // 用指针方式访问
    }
    return sum;
}
