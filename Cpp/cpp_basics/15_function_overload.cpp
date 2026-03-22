// ============================================
// 函数重载 - 超详细注释版
// 日期：2026-03-10（第4天）
// 难度：⭐⭐ 中级
// ============================================

/*
【核心概念】函数重载（Function Overloading）

是什么？
- 允许多个函数使用相同的名字，但参数不同

为什么需要？
- 同一操作可能需要处理不同类型的数据
- 让函数名更简洁、更符合直觉
- 例如：print(int)、print(double)、print(string) 都叫 print

核心特点：
- 函数名相同
- 参数列表不同（个数、类型或顺序）
- 编译器根据参数自动选择合适的函数
*/

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

#include <iostream>
#include <string>
using namespace std;

// --------------------------------------------
// 1.1 什么是函数重载？
// --------------------------------------------

/*
【函数重载】

是什么？
- 在同一作用域内，定义多个同名函数
- 这些函数的参数列表必须不同

为什么需要？
- 避免为相似功能起不同的名字
- 例如：printInt()、printDouble()、printString() 太繁琐
- 统一用 print() 更简洁

编译器如何区分？
- 根据调用时传入的参数类型和个数
- 这个过程叫"函数匹配"或"重载解析"
*/

// --------------------------------------------
// 1.2 函数重载的三种方式
// --------------------------------------------

/*
【重载方式1：参数个数不同】

示例：
void func(int a);           // 1个参数
void func(int a, int b);    // 2个参数
void func(int a, int b, int c);  // 3个参数

编译器根据参数个数选择调用哪个函数
*/

/*
【重载方式2：参数类型不同】

示例：
void print(int x);          // 接收整数
void print(double x);       // 接收浮点数
void print(string x);       // 接收字符串

编译器根据参数类型选择调用哪个函数
*/

/*
【重载方式3：参数顺序不同】

示例：
void func(int a, double b);    // 第1个int，第2个double
void func(double a, int b);    // 第1个double，第2个int

编译器根据参数顺序选择调用哪个函数
*/

// ============================================
// 第二部分：基础操作实践
// ============================================

void demonstratePart2() {
    cout << string(60, '=') << endl;
    cout << "第二部分：基础操作实践" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 2.1 示例1：参数个数不同的重载
    // --------------------------------------------

    cout << "\n2.1 示例1：参数个数不同的重载" << endl;
    cout << string(60, '-') << endl;

    // 定义三个同名函数，参数个数不同
    // 函数1：计算一个数的平方
    auto square1 = [](int a) -> int {
        return a * a;
    };

    // 函数2：计算两个数的和
    auto sum2 = [](int a, int b) -> int {
        return a + b;
    };

    // 函数3：计算三个数的和
    auto sum3 = [](int a, int b, int c) -> int {
        return a + b + c;
    };

    cout << "square1(5) = " << square1(5) << endl;
    cout << "sum2(3, 4) = " << sum2(3, 4) << endl;
    cout << "sum3(1, 2, 3) = " << sum3(1, 2, 3) << endl;

    // --------------------------------------------
    // 2.2 示例2：参数类型不同的重载
    // --------------------------------------------

    cout << "\n2.2 示例2：参数类型不同的重载" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【说明】
下面定义了3个print函数，名字相同但参数类型不同：
- print(int)：打印整数
- print(double)：打印浮点数
- print(string)：打印字符串
)" << endl;
}

// 定义重载函数：打印整数
void print(int x) {
    cout << "整数：" << x << endl;
}

// 定义重载函数：打印浮点数
void print(double x) {
    cout << "浮点数：" << x << endl;
}

// 定义重载函数：打印字符串
void print(string x) {
    cout << "字符串：" << x << endl;
}

void demonstratePart2_2() {
    // 调用不同版本的print函数
    print(10);           // 调用 print(int)
    print(3.14);         // 调用 print(double)
    print("Hello");      // 调用 print(string)
}

// --------------------------------------------
// 2.3 示例3：参数顺序不同的重载
// --------------------------------------------

// 定义重载函数：第1个int，第2个double
void display(int a, double b) {
    cout << "版本1：int=" << a << ", double=" << b << endl;
}

// 定义重载函数：第1个double，第2个int
void display(double a, int b) {
    cout << "版本2：double=" << a << ", int=" << b << endl;
}

void demonstratePart2_3() {
    cout << "\n2.3 示例3：参数顺序不同的重载" << endl;
    cout << string(60, '-') << endl;

    display(10, 3.14);    // 调用版本1：display(int, double)
    display(3.14, 10);    // 调用版本2：display(double, int)
}

// ============================================
// 第三部分：深入理解（可选）
// ============================================

/*
==============================================
🔍 深入理解：函数重载的原理（选学）
==============================================

如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【编译器如何实现函数重载？】

1. 名字修饰（Name Mangling）
   - 编译器会根据参数类型修改函数名
   - 例如：
     void print(int) → _Z5printi
     void print(double) → _Z5printd
     void print(string) → _Z5printSs
   - 这样在链接时就不会冲突

2. 函数匹配过程
   - 第1步：找到所有同名函数
   - 第2步：根据参数类型筛选候选函数
   - 第3步：选择最佳匹配
   - 第4步：如果有多个同样好的匹配，报错"二义性"

3. 为什么返回值不能用于重载？
   - 因为调用时可能不使用返回值
   - 例如：func(); 编译器无法判断调用哪个版本
   - 所以C++规定：仅返回值不同不能重载

【实际应用场景】

1. 标准库中的重载
   - cout << 可以输出各种类型（int、double、string等）
   - 这就是运算符重载（后续会学）

2. 构造函数重载
   - 类可以有多个构造函数
   - 例如：string s1; string s2("hello"); string s3(s2);

3. 数学函数重载
   - abs(int)、abs(double)、abs(long)
   - max(int, int)、max(double, double)

==============================================
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void demonstratePart4() {
    cout << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 4.1 错误1：仅返回值不同不能重载
    // --------------------------------------------

    cout << "\n4.1 错误1：仅返回值不同不能重载" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
int getValue() { return 10; }
double getValue() { return 3.14; }  // 编译错误！

原因：
- 函数名相同，参数列表也相同
- 仅返回值不同不能构成重载
- 编译器无法根据返回值选择函数

✅ 正确做法：
int getIntValue() { return 10; }
double getDoubleValue() { return 3.14; }

或者：
int getValue(int dummy) { return 10; }
double getValue(double dummy) { return 3.14; }

记忆技巧：
- 重载看参数，不看返回值
- 调用时编译器只看传入的参数
)" << endl;

    // --------------------------------------------
    // 4.2 错误2：参数名不同不能重载
    // --------------------------------------------

    cout << "\n4.2 错误2：参数名不同不能重载" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
void func(int a) { }
void func(int b) { }  // 编译错误！

原因：
- 参数名不同，但类型相同
- 编译器只看参数类型，不看参数名
- 这两个函数签名完全相同

✅ 正确做法：
void func(int a) { }
void func(double b) { }  // 类型不同，可以重载

记忆技巧：
- 参数名只是标识符，不影响函数签名
- 重载看类型、个数、顺序
)" << endl;

    // --------------------------------------------
    // 4.3 错误3：const修饰不能用于重载（非指针/引用）
    // --------------------------------------------

    cout << "\n4.3 错误3：const修饰不能用于重载（非指针/引用）" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
void func(int x) { }
void func(const int x) { }  // 编译错误！

原因：
- 对于值传递，const不影响函数签名
- 因为函数内部得到的是副本
- 调用者不关心函数内部是否修改副本

✅ 正确做法（指针/引用可以）：
void func(int* x) { }
void func(const int* x) { }  // 可以重载

void func(int& x) { }
void func(const int& x) { }  // 可以重载

记忆技巧：
- 值传递：const不算重载
- 指针/引用：const算重载
)" << endl;

    // --------------------------------------------
    // 4.4 错误4：二义性调用
    // --------------------------------------------

    cout << "\n4.4 错误4：二义性调用" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
void func(int a, double b = 1.0) { }
void func(int a) { }

func(10);  // 编译错误：二义性！

原因：
- func(10) 可以匹配 func(int, double) [使用默认参数]
- func(10) 也可以匹配 func(int)
- 编译器无法决定调用哪个

✅ 正确做法：
避免重载函数和默认参数混用
或者确保调用时没有二义性

记忆技巧：
- 重载+默认参数 = 容易产生二义性
- 设计时要小心
)" << endl;

    // --------------------------------------------
    // 4.5 错误5：类型提升导致的意外匹配
    // --------------------------------------------

    cout << "\n4.5 错误5：类型提升导致的意外匹配" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
⚠️ 注意代码：
void func(int x) { cout << "int版本" << endl; }
void func(double x) { cout << "double版本" << endl; }

func('A');  // 调用哪个？

结果：
- 调用 func(int)
- 因为 char 可以自动提升为 int

原因：
- C++有类型提升规则
- char → int → long → float → double
- 编译器选择"最近"的匹配

记忆技巧：
- 注意隐式类型转换
- 必要时使用显式类型转换
)" << endl;
}

// ============================================
// 第五部分：函数卡片速查
// ============================================

void demonstratePart5() {
    cout << string(60, '=') << endl;
    cout << "函数卡片速查" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【函数重载规则】

✅ 可以构成重载：
1. 参数个数不同
   void func(int a);
   void func(int a, int b);

2. 参数类型不同
   void func(int a);
   void func(double a);

3. 参数顺序不同
   void func(int a, double b);
   void func(double a, int b);

4. 指针/引用的const修饰
   void func(int* p);
   void func(const int* p);

❌ 不能构成重载：
1. 仅返回值不同
   int func();
   double func();  // 错误！

2. 仅参数名不同
   void func(int a);
   void func(int b);  // 错误！

3. 值传递的const修饰
   void func(int x);
   void func(const int x);  // 错误！

────────────────────────────────────

【编译器匹配规则】

1. 精确匹配（最优先）
2. 类型提升匹配（char→int, float→double）
3. 标准转换匹配（int→double）
4. 用户定义转换匹配
5. 省略号匹配（...）

如果有多个同等级匹配，报错"二义性"
)" << endl;
}

// ============================================
// 第六部分：练习题
// ============================================

void demonstratePart6() {
    cout << string(60, '=') << endl;
    cout << "练习题" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
1. 定义重载函数 max()
   要求：
   - max(int, int) 返回两个整数的最大值
   - max(double, double) 返回两个浮点数的最大值
   - max(int, int, int) 返回三个整数的最大值
   提示：使用条件运算符 a > b ? a : b

2. 定义重载函数 area()
   要求：
   - area(double r) 计算圆的面积（πr²）
   - area(double w, double h) 计算矩形面积（w×h）
   - area(double a, double b, double c) 计算三角形面积（海伦公式）
   提示：π可以用 3.14159

3. 判断下列哪些可以构成重载？
   a) void f(int x); 和 void f(int y);
   b) int f(int x); 和 double f(int x);
   c) void f(int x); 和 void f(double x);
   d) void f(int x); 和 void f(const int x);
   e) void f(int* x); 和 void f(const int* x);

4. 分析下列代码的输出
   void func(int x) { cout << "A"; }
   void func(double x) { cout << "B"; }
   void func(int x, int y) { cout << "C"; }

   func(10);        // 输出？
   func(3.14);      // 输出？
   func(10, 20);    // 输出？
   func('A');       // 输出？（注意类型提升）

5. 设计一个print函数族
   要求：
   - 能打印int、double、string、bool类型
   - bool类型打印"true"或"false"而不是1或0
   - 所有函数都叫print
)" << endl;
}

// ============================================
// 主函数
// ============================================

int main() {
    cout << "============================================" << endl;
    cout << "函数重载 - 学习笔记" << endl;
    cout << "============================================" << endl;

    demonstratePart2();
    demonstratePart2_2();
    demonstratePart2_3();
    demonstratePart4();
    demonstratePart5();
    demonstratePart6();

    cout << "\n============================================" << endl;
    cout << "学习完成！" << endl;
    cout << "============================================" << endl;

    return 0;
}

/*
【编译和运行】
编译：g++ -o 15_function_overload 15_function_overload.cpp
运行：./15_function_overload (Linux/Mac) 或 15_function_overload.exe (Windows)

【C++版本要求】
C++11 及以上
*/
