/*
 * ============================================================================
 * 文件名：21_pointer_basics.cpp
 * 主题：指针基础详解
 * 学习目标：
 *   1. 理解指针的概念（存储地址的变量）
 *   2. 掌握指针的声明和初始化
 *   3. 学会使用&（取地址）和*（解引用）运算符
 *   4. 理解指针的基本操作
 *   5. 认识空指针和野指针的危险性
 *
 * 编译命令：g++ -std=c++11 -o 21_pointer_basics 21_pointer_basics.cpp
 * 运行命令：./21_pointer_basics (Linux/Mac) 或 21_pointer_basics.exe (Windows)
 * ============================================================================
 */

#include <iostream>
using namespace std;

int main() {
    cout << "=== 指针基础详解 ===" << endl << endl;

    // ========================================
    // 1. 什么是指针？
    // ========================================
    cout << "【1. 什么是指针？】" << endl;
    cout << "指针是一个变量，它存储的是另一个变量的内存地址" << endl;
    cout << "就像门牌号：指针存储的是\"地址\"，而不是\"房子本身\"" << endl;
    cout << endl;

    int num = 42;
    cout << "变量 num 的值：" << num << endl;
    cout << "变量 num 的地址：" << &num << endl;
    cout << "地址就是内存中的位置编号" << endl;
    cout << endl;

    // ========================================
    // 2. 指针的声明和初始化
    // ========================================
    cout << "【2. 指针的声明和初始化】" << endl;

    // 声明指针：类型* 指针名
    int* ptr;  // ptr 是一个指向 int 类型的指针
    cout << "声明指针：int* ptr;" << endl;

    // 初始化指针：用 & 取变量的地址
    ptr = &num;  // ptr 现在存储 num 的地址
    cout << "初始化指针：ptr = &num;" << endl;
    cout << "ptr 的值（存储的地址）：" << ptr << endl;
    cout << "ptr 指向的值（num的值）：" << *ptr << endl;
    cout << endl;

    // 声明时直接初始化
    int value = 100;
    int* p = &value;
    cout << "声明时初始化：int* p = &value;" << endl;
    cout << "p 指向的值：" << *p << endl;
    cout << endl;

    // ========================================
    // 3. & 和 * 运算符
    // ========================================
    cout << "【3. & 和 * 运算符】" << endl;

    int x = 10;
    int* px = &x;

    cout << "变量 x = " << x << endl;
    cout << "&x（x的地址）= " << &x << endl;
    cout << "px（存储x的地址）= " << px << endl;
    cout << "*px（px指向的值）= " << *px << endl;
    cout << endl;

    cout << "关键理解：" << endl;
    cout << "  & 运算符：取地址（address-of）" << endl;
    cout << "  * 运算符：解引用（dereference），获取指针指向的值" << endl;
    cout << endl;

    // ========================================
    // 4. 通过指针修改变量
    // ========================================
    cout << "【4. 通过指针修改变量】" << endl;

    int a = 5;
    int* pa = &a;

    cout << "修改前：a = " << a << endl;
    *pa = 20;  // 通过指针修改 a 的值
    cout << "执行 *pa = 20 后：a = " << a << endl;
    cout << "指针可以间接修改变量的值！" << endl;
    cout << endl;

    // ========================================
    // 5. 指针的大小
    // ========================================
    cout << "【5. 指针的大小】" << endl;

    int* p1;
    double* p2;
    char* p3;

    cout << "int* 指针的大小：" << sizeof(p1) << " 字节" << endl;
    cout << "double* 指针的大小：" << sizeof(p2) << " 字节" << endl;
    cout << "char* 指针的大小：" << sizeof(p3) << " 字节" << endl;
    cout << "所有指针的大小相同（取决于系统：32位=4字节，64位=8字节）" << endl;
    cout << "因为指针存储的都是地址，地址的长度是固定的" << endl;
    cout << endl;

    // ========================================
    // 6. 多个指针指向同一个变量
    // ========================================
    cout << "【6. 多个指针指向同一个变量】" << endl;

    int target = 99;
    int* ptr1 = &target;
    int* ptr2 = &target;

    cout << "target = " << target << endl;
    cout << "ptr1 指向的值：" << *ptr1 << endl;
    cout << "ptr2 指向的值：" << *ptr2 << endl;

    *ptr1 = 88;
    cout << "通过 ptr1 修改为 88 后：" << endl;
    cout << "target = " << target << endl;
    cout << "ptr2 指向的值：" << *ptr2 << endl;
    cout << "多个指针可以指向同一个变量" << endl;
    cout << endl;

    // ========================================
    // 7. 空指针（nullptr）
    // ========================================
    cout << "【7. 空指针（nullptr）】" << endl;

    int* null_ptr = nullptr;  // C++11 推荐使用 nullptr
    cout << "空指针：int* null_ptr = nullptr;" << endl;
    cout << "null_ptr 的值：" << null_ptr << endl;
    cout << "空指针不指向任何有效地址" << endl;
    cout << endl;

    // 使用前检查
    if (null_ptr != nullptr) {
        cout << "指针有效，可以使用" << endl;
    } else {
        cout << "指针为空，不能解引用！" << endl;
    }
    cout << endl;

    // ========================================
    // 8. 野指针（危险！）
    // ========================================
    cout << "【8. 野指针（危险！）】" << endl;

    int* wild_ptr;  // 未初始化的指针
    cout << "未初始化的指针：int* wild_ptr;" << endl;
    cout << "wild_ptr 的值（随机地址）：" << wild_ptr << endl;
    cout << "⚠️ 危险：野指针指向未知地址，解引用会导致程序崩溃！" << endl;
    cout << "⚠️ 绝对不要使用未初始化的指针！" << endl;
    cout << endl;

    // 正确做法：初始化为 nullptr
    int* safe_ptr = nullptr;
    cout << "安全做法：int* safe_ptr = nullptr;" << endl;
    cout << "使用前检查是否为空" << endl;
    cout << endl;

    // ========================================
    // 9. 指针的常见错误
    // ========================================
    cout << "【9. 指针的常见错误】" << endl;
    cout << "❌ 错误1：解引用空指针" << endl;
    cout << "   int* p = nullptr;" << endl;
    cout << "   *p = 10;  // 崩溃！" << endl;
    cout << endl;

    cout << "❌ 错误2：使用野指针" << endl;
    cout << "   int* p;  // 未初始化" << endl;
    cout << "   *p = 10;  // 崩溃！" << endl;
    cout << endl;

    cout << "❌ 错误3：指针类型不匹配" << endl;
    cout << "   int x = 10;" << endl;
    cout << "   double* p = &x;  // 编译错误！" << endl;
    cout << endl;

    cout << "✅ 正确做法：" << endl;
    cout << "   1. 声明时初始化为 nullptr 或有效地址" << endl;
    cout << "   2. 使用前检查是否为空" << endl;
    cout << "   3. 确保指针类型匹配" << endl;
    cout << endl;

    // ========================================
    // 10. 实战案例：交换两个变量
    // ========================================
    cout << "【10. 实战案例：交换两个变量】" << endl;

    int m = 10, n = 20;
    cout << "交换前：m = " << m << ", n = " << n << endl;

    // 使用指针交换
    int* pm = &m;
    int* pn = &n;
    int temp = *pm;
    *pm = *pn;
    *pn = temp;

    cout << "交换后：m = " << m << ", n = " << n << endl;
    cout << "通过指针可以间接操作变量" << endl;
    cout << endl;

    // ========================================
    // 11. 指针的三要素
    // ========================================
    cout << "【11. 指针的三要素】" << endl;
    cout << "1. 指针的类型：决定了指针指向什么类型的数据" << endl;
    cout << "2. 指针的值：存储的地址" << endl;
    cout << "3. 指针指向的值：地址处存储的数据" << endl;
    cout << endl;

    int data = 666;
    int* pdata = &data;
    cout << "示例：int data = 666; int* pdata = &data;" << endl;
    cout << "1. 指针类型：int*（指向int的指针）" << endl;
    cout << "2. 指针的值：" << pdata << "（data的地址）" << endl;
    cout << "3. 指针指向的值：" << *pdata << "（data的值）" << endl;
    cout << endl;

    // ========================================
    // 12. 总结
    // ========================================
    cout << "【12. 总结】" << endl;
    cout << "✓ 指针存储的是地址，不是值" << endl;
    cout << "✓ & 运算符获取地址，* 运算符获取指针指向的值" << endl;
    cout << "✓ 指针必须初始化后才能使用" << endl;
    cout << "✓ nullptr 表示空指针" << endl;
    cout << "✓ 野指针非常危险，必须避免" << endl;
    cout << "✓ 指针是C++最强大也最危险的特性" << endl;
    cout << endl;

    return 0;
}
