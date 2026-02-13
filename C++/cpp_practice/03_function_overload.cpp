// ============================================
// 函数重载（Function Overloading）- 超详细注释版
// 日期：2026-02-13（第1天）
// 难度：⭐⭐ 中级
// 参考资料：CppFiles/CppBase/1.C++与C.md（函数重载部分）
// ============================================

/*
【核心概念】函数重载（Function Overloading）

是什么？
- 在同一作用域内，可以有一组具有相同函数名、不同参数列表的函数
- 这组函数被称为重载函数

为什么需要？
- 功能相似的函数可以使用相同的名字
- 减少函数名的数量，提高代码可读性
- 调用时根据参数自动选择合适的函数

核心特点：
- 函数名相同
- 参数列表不同（数量、类型、顺序）
- 返回类型可以不同（但不能仅靠返回类型区分）
- C 语言不支持，C++ 支持
*/

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

#include <iostream>
using std::cout;
using std::endl;

// --------------------------------------------
// 1.1 为什么需要函数重载
// --------------------------------------------

/*
【C 语言的问题】

在 C 语言中，如果要实现交换不同类型变量的功能，需要定义不同名字的函数：

void swap_int(int *a, int *b);
void swap_float(float *a, float *b);
void swap_double(double *a, double *b);

问题：
- 函数名不同，不好记忆
- 功能相似，但名字不统一
- 增加了学习和使用的负担

【C++ 的解决方案】

C++ 支持函数重载，可以使用相同的函数名：

void swap(int &a, int &b);
void swap(float &a, float &b);
void swap(double &a, double &b);

优点：
- 函数名统一，容易记忆
- 调用时根据参数类型自动选择
- 提高代码可读性
*/

// --------------------------------------------
// 1.2 函数重载的定义
// --------------------------------------------

/*
【函数重载的条件】

在同一作用域内，函数名相同，但参数列表不同，即可构成重载。

参数列表不同包括：
1. 参数数量不同
2. 参数类型不同
3. 参数顺序不同

注意：
- 仅返回类型不同，不能构成重载
- 参数名不同，不能构成重载
*/

// 示例1：参数数量不同
void print() {
    cout << "No parameters" << endl;
}

void print(int x) {
    cout << "One parameter: " << x << endl;
}

void print(int x, int y) {
    cout << "Two parameters: " << x << ", " << y << endl;
}

// 示例2：参数类型不同
void display(int x) {
    cout << "int: " << x << endl;
}

void display(double x) {
    cout << "double: " << x << endl;
}

void display(const char* str) {
    cout << "string: " << str << endl;
}

// 示例3：参数顺序不同
void func(int x, double y) {
    cout << "int, double: " << x << ", " << y << endl;
}

void func(double x, int y) {
    cout << "double, int: " << x << ", " << y << endl;
}

void test_overload_basic() {
    cout << "\n=== 函数重载的基本概念 ===" << endl;

    // 根据参数数量选择函数
    print();
    print(10);
    print(10, 20);

    // 根据参数类型选择函数
    display(10);
    display(3.14);
    display("Hello");

    // 根据参数顺序选择函数
    func(10, 3.14);
    func(3.14, 10);
}

// --------------------------------------------
// 1.3 函数重载的实现原理 ⭐⭐⭐⭐
// --------------------------------------------

/*
【名字改编（Name Mangling）】

是什么？
- 编译器会根据函数名和参数类型生成唯一的符号名
- 这个过程称为名字改编（Name Mangling）

为什么需要？
- 链接器需要唯一的符号名来区分不同的函数
- 相同函数名的重载函数需要不同的符号名

如何实现？
- 编译器将函数名、参数类型、参数数量等信息编码到符号名中
- 不同的编译器有不同的编码规则

示例：
void func(int x);           // 可能被编译为 _Z4funci
void func(double x);        // 可能被编译为 _Z4funcd
void func(int x, int y);    // 可能被编译为 _Z4funcii

查看符号名的方法：
1. 编译：g++ -c file.cpp
2. 查看：nm file.o

注意：
- C 语言没有名字改编机制，不支持函数重载
- C++ 的名字改编机制使得函数重载成为可能
*/

// 这些函数会被编译器改编为不同的符号名
void add(int a, int b) {
    cout << "int + int = " << (a + b) << endl;
}

void add(double a, double b) {
    cout << "double + double = " << (a + b) << endl;
}

void add(int a, double b) {
    cout << "int + double = " << (a + b) << endl;
}

void test_name_mangling() {
    cout << "\n=== 名字改编机制 ===" << endl;

    // 编译器根据参数类型选择对应的函数
    add(10, 20);        // 调用 add(int, int)
    add(3.14, 2.71);    // 调用 add(double, double)
    add(10, 3.14);      // 调用 add(int, double)
}

// --------------------------------------------
// 1.4 函数重载的匹配规则 ⭐⭐⭐⭐
// --------------------------------------------

/*
【函数重载的匹配规则】

当调用重载函数时，编译器按以下顺序查找匹配的函数：

1. 精确匹配：参数类型完全相同
2. 类型提升：char/short → int，float → double
3. 标准转换：int → double，double → int 等
4. 用户定义的转换：类的转换函数
5. 省略号匹配：可变参数函数

如果找到多个匹配的函数，编译器会报错（二义性）。

示例：
void func(int x);
void func(double x);

func(10);      // 精确匹配 func(int)
func(3.14);    // 精确匹配 func(double)
func('A');     // 类型提升 char → int，匹配 func(int)
func(10.0f);   // 类型提升 float → double，匹配 func(double)
*/

void process(int x) {
    cout << "process(int): " << x << endl;
}

void process(double x) {
    cout << "process(double): " << x << endl;
}

void test_overload_matching() {
    cout << "\n=== 函数重载的匹配规则 ===" << endl;

    // 精确匹配
    process(10);        // 匹配 process(int)
    process(3.14);      // 匹配 process(double)

    // 类型提升
    char ch = 'A';
    process(ch);        // char → int，匹配 process(int)

    float f = 3.14f;
    process(f);         // float → double，匹配 process(double)

    // 标准转换
    short s = 10;
    process(s);         // short → int，匹配 process(int)
}

// --------------------------------------------
// 1.5 函数重载与默认参数 ⭐⭐⭐⭐
// --------------------------------------------

/*
【函数重载与默认参数的冲突】

问题：
- 函数重载和默认参数都可以实现"一个函数名，多种调用方式"
- 但两者结合使用可能导致二义性

示例：
void func(int x);
void func(int x, int y = 0);

func(10);  // 错误：二义性，可以匹配两个函数

建议：
- 谨慎使用函数重载和默认参数的组合
- 优先使用函数重载（更清晰）
- 或者使用默认参数代替简单的重载
*/

// 示例：函数重载与默认参数的冲突
void calculate(int x) {
    cout << "calculate(int): " << x << endl;
}

// 这个函数与上面的函数在调用 calculate(10) 时产生二义性
// void calculate(int x, int y = 0) {
//     cout << "calculate(int, int): " << x << ", " << y << endl;
// }

// 正确的做法：只使用函数重载
void calculate(int x, int y) {
    cout << "calculate(int, int): " << x << ", " << y << endl;
}

void test_overload_with_default() {
    cout << "\n=== 函数重载与默认参数 ===" << endl;

    calculate(10);      // 匹配 calculate(int)
    calculate(10, 20);  // 匹配 calculate(int, int)
}

// ============================================
// 🔍 深入理解：函数重载的底层机制（选学）
// ============================================

/*
==============================================
🔍 深入理解：函数重载的底层机制（选学）
==============================================

如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【名字改编的规则】

不同编译器有不同的名字改编规则，以 GCC 为例：

函数：void func(int x, double y);
符号名：_Z4funcid

解析：
- _Z：前缀，表示 C++ 符号
- 4：函数名长度
- func：函数名
- i：int 类型
- d：double 类型

常见类型编码：
- i：int
- d：double
- f：float
- c：char
- l：long
- v：void
- P：指针（Pointer）
- R：引用（Reference）

示例：
void func(int* p);          // _Z4funcPi
void func(int& r);          // _Z4funcRi
void func(int x, int y);    // _Z4funcii

【为什么 C 语言不支持函数重载？】

C 语言的符号名：
- 函数名就是符号名
- 没有名字改编机制

示例：
void func(int x);    // 符号名：func
void func(double x); // 符号名：func（冲突！）

因此 C 语言不支持函数重载。

【extern "C" 的作用】

extern "C" 告诉编译器按 C 语言的方式编译：
- 不进行名字改编
- 函数名就是符号名

用途：
- C++ 代码调用 C 语言库
- C 语言代码调用 C++ 函数

示例：
extern "C" {
    void c_func(int x);  // 按 C 语言方式编译
}

【函数重载的性能】

编译时开销：
- 编译器需要进行名字改编
- 需要进行函数匹配
- 编译时间稍长

运行时开销：
- 没有额外开销
- 函数调用与普通函数相同
- 编译器在编译时已经确定了调用哪个函数

结论：
- 函数重载不影响运行时性能
- 只是编译时的语法糖

【函数重载与模板】

函数重载的局限：
- 需要为每种类型定义一个函数
- 代码重复，维护困难

模板的优势：
- 一次定义，适用于多种类型
- 编译器自动生成对应类型的函数

示例：
// 函数重载
void swap(int& a, int& b);
void swap(double& a, double& b);
void swap(char& a, char& b);

// 函数模板（后续学习）
template <typename T>
void swap(T& a, T& b);

==============================================
*/

// ============================================
// 第二部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

/*
【常见错误1：仅返回类型不同，不能构成重载】

❌ 错误代码：
int func(int x) {
    return x * 2;
}

double func(int x) {  // 错误：不能仅靠返回类型区分
    return x * 2.0;
}

结果：编译错误

原因：
- 函数重载根据参数列表区分
- 返回类型不参与重载决策
- 调用时无法根据返回类型选择函数

✅ 正确代码：
int func_int(int x) {
    return x * 2;
}

double func_double(int x) {
    return x * 2.0;
}

记忆技巧：
- 函数重载看参数，不看返回类型
- 参数列表必须不同


【常见错误2：参数名不同，不能构成重载】

❌ 错误代码：
void func(int x) {
    cout << x << endl;
}

void func(int y) {  // 错误：参数名不同不能构成重载
    cout << y << endl;
}

结果：编译错误，重复定义

原因：
- 参数名不参与重载决策
- 只有参数类型、数量、顺序参与

✅ 正确代码：
void func(int x) {
    cout << x << endl;
}

void func(double x) {  // 参数类型不同，可以重载
    cout << x << endl;
}

记忆技巧：
- 参数名只是标识符，不影响函数签名
- 参数类型才是关键


【常见错误3：函数重载与默认参数的二义性】

❌ 错误代码：
void func(int x) {
    cout << "func(int)" << endl;
}

void func(int x, int y = 0) {
    cout << "func(int, int)" << endl;
}

void test() {
    func(10);  // 错误：二义性
}

结果：编译错误，调用有二义性

原因：
- func(10) 可以匹配 func(int)
- func(10) 也可以匹配 func(int, int)（使用默认参数）
- 编译器无法决定调用哪个

✅ 正确代码（方式1）：
void func(int x) {
    cout << "func(int)" << endl;
}

void func(int x, int y) {  // 不使用默认参数
    cout << "func(int, int)" << endl;
}

✅ 正确代码（方式2）：
void func(int x, int y = 0) {  // 只定义一个函数
    cout << "func(int, int)" << endl;
}

记忆技巧：
- 谨慎使用函数重载和默认参数的组合
- 避免产生二义性


【常见错误4：const 引用与非 const 引用的重载】

❌ 容易混淆：
void func(int& x) {
    cout << "func(int&)" << endl;
}

void func(const int& x) {
    cout << "func(const int&)" << endl;
}

void test() {
    int a = 10;
    func(a);  // 调用哪个？
}

结果：调用 func(int&)

原因：
- 非 const 变量优先匹配非 const 引用
- 如果没有非 const 引用版本，才匹配 const 引用

规则：
int a = 10;
func(a);     // 优先匹配 func(int&)
const int b = 20;
func(b);     // 只能匹配 func(const int&)
func(100);   // 只能匹配 func(const int&)（临时对象）

记忆技巧：
- 非 const 变量优先匹配非 const 引用
- const 变量和临时对象只能匹配 const 引用


【常见错误5：类型转换导致的二义性】

❌ 错误代码：
void func(int x) {
    cout << "func(int)" << endl;
}

void func(double x) {
    cout << "func(double)" << endl;
}

void test() {
    func(3.14f);  // float 可以转换为 int 或 double
}

结果：
- 不会产生二义性
- float 会提升为 double，匹配 func(double)

但如果是这样：
void func(long x) {
    cout << "func(long)" << endl;
}

void func(double x) {
    cout << "func(double)" << endl;
}

void test() {
    func(10);  // int 可以转换为 long 或 double，产生二义性
}

结果：编译错误，二义性

✅ 正确做法：
void test() {
    func(10L);    // 明确指定 long
    func(10.0);   // 明确指定 double
}

记忆技巧：
- 避免需要类型转换的重载
- 调用时明确指定类型
*/

// ============================================
// 第三部分：实战示例
// ============================================

// 示例1：实现不同类型的 max 函数
int max(int a, int b) {
    return a > b ? a : b;
}

double max(double a, double b) {
    return a > b ? a : b;
}

int max(int a, int b, int c) {
    int temp = a > b ? a : b;
    return temp > c ? temp : c;
}

// 示例2：实现不同类型的 print 函数
void print(int x) {
    cout << "Integer: " << x << endl;
}

void print(double x) {
    cout << "Double: " << x << endl;
}

void print(const char* str) {
    cout << "String: " << str << endl;
}

void print(int arr[], int size) {
    cout << "Array: ";
    for (int i = 0; i < size; ++i) {
        cout << arr[i] << " ";
    }
    cout << endl;
}

// 示例3：实现不同类型的 swap 函数
void swap(int& a, int& b) {
    int temp = a;
    a = b;
    b = temp;
}

void swap(double& a, double& b) {
    double temp = a;
    a = b;
    b = temp;
}

void swap(char& a, char& b) {
    char temp = a;
    a = b;
    b = temp;
}

void test_practical_examples() {
    cout << "\n=== 实战示例 ===" << endl;

    // 示例1：max 函数
    cout << "max(10, 20) = " << max(10, 20) << endl;
    cout << "max(3.14, 2.71) = " << max(3.14, 2.71) << endl;
    cout << "max(10, 20, 30) = " << max(10, 20, 30) << endl;

    // 示例2：print 函数
    print(100);
    print(3.14);
    print("Hello, World!");
    int arr[] = {1, 2, 3, 4, 5};
    print(arr, 5);

    // 示例3：swap 函数
    int x = 10, y = 20;
    cout << "\n交换前：x = " << x << ", y = " << y << endl;
    swap(x, y);
    cout << "交换后：x = " << x << ", y = " << y << endl;

    double a = 3.14, b = 2.71;
    cout << "\n交换前：a = " << a << ", b = " << b << endl;
    swap(a, b);
    cout << "交换后：a = " << a << ", b = " << b << endl;
}

// ============================================
// 第四部分：练习题
// ============================================

/*
【练习1】实现 add 函数的重载
要求：实现以下重载函数
  - int add(int a, int b)
  - double add(double a, double b)
  - int add(int a, int b, int c)
提示：参数数量和类型不同
考察点：函数重载的基本概念

【练习2】实现 print 函数的重载
要求：实现以下重载函数
  - void print(int x)
  - void print(double x)
  - void print(const char* str)
  - void print(bool b)（输出 true 或 false）
提示：参数类型不同
考察点：函数重载的参数类型

【练习3】实现 compare 函数的重载
要求：实现以下重载函数
  - bool compare(int a, int b)（返回 a > b）
  - bool compare(double a, double b)（返回 a > b）
  - bool compare(const char* s1, const char* s2)（比较字符串长度）
提示：使用 strlen 函数比较字符串长度
考察点：函数重载的实际应用

【练习4】理解函数重载的匹配规则
要求：定义以下函数
  - void func(int x)
  - void func(double x)
  - 调用 func('A')、func(10)、func(3.14)、func(10.0f)
  - 观察匹配的函数
提示：注意类型提升和标准转换
考察点：函数重载的匹配规则

【练习5】避免函数重载的二义性
要求：定义以下函数
  - void func(int x)
  - void func(int x, int y = 0)
  - 尝试调用 func(10)
  - 观察编译错误
  - 修改代码避免二义性
提示：去掉默认参数或去掉一个重载函数
考察点：函数重载与默认参数的冲突
*/

// ============================================
// 主函数
// ============================================

int main() {
    cout << "=== 函数重载（Function Overloading）学习 ===" << endl;

    // 测试基本概念
    test_overload_basic();

    // 测试名字改编机制
    test_name_mangling();

    // 测试匹配规则
    test_overload_matching();

    // 测试与默认参数的关系
    test_overload_with_default();

    // 实战示例
    test_practical_examples();

    return 0;
}

// ============================================
// 学习总结
// ============================================

/*
【今日学习要点】

1. 函数重载的概念
   - 同一作用域内，函数名相同，参数列表不同
   - 减少函数名数量，提高代码可读性

2. 函数重载的条件
   - 参数数量不同
   - 参数类型不同
   - 参数顺序不同
   - 仅返回类型不同不能构成重载

3. 函数重载的实现原理
   - 名字改编（Name Mangling）
   - 编译器根据函数名和参数生成唯一符号名
   - C 语言没有名字改编，不支持函数重载

4. 函数重载的匹配规则
   - 精确匹配 > 类型提升 > 标准转换
   - 避免二义性

5. 函数重载与默认参数
   - 谨慎使用组合
   - 避免产生二义性

【下一步学习】

- 学习 const 关键字
- 学习 new/delete 表达式
- 学习 inline 函数

【学习建议】

- 函数重载是 C++ 的重要特性，提高代码可读性
- 重点理解函数重载的条件和匹配规则
- 多动手实践，理解名字改编机制
- 避免函数重载与默认参数的冲突
*/
