// ============================================
// 函数参数 - 超详细注释版
// 日期：2026-03-09（第3天）
// 难度：⭐⭐ 中级
// ============================================

/*
【核心概念】函数参数是什么？

是什么？
- 参数是传递给函数的输入数据
- 让函数可以处理不同的数据

为什么需要？
- 让函数更灵活（同一个函数处理不同数据）
- 避免写很多相似的函数
- 提高代码复用性

核心特点？
- 形参：函数定义时的参数（形式上的参数）
- 实参：函数调用时传入的参数（实际的参数）
- 参数传递：实参的值复制给形参
*/

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

#include <iostream>
#include <string>
using namespace std;

void conceptExplanation() {
    cout << string(60, '=') << endl;
    cout << "第一部分：核心概念详解 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 1.1 什么是形参和实参？
    // --------------------------------------------

    cout << "\n1.1 什么是形参和实参？" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【形参 vs 实参】

想象你在点外卖：
- 商家定义菜单："宫保鸡丁（辣度）"
  → 辣度是形参，表示这道菜需要一个辣度参数

- 你点餐时说："宫保鸡丁（微辣）"
  → 微辣是实参，是你实际传入的值

【C++中的形参和实参】

// 函数定义：形参
void printNumber(int num) {  // num是形参
    cout << num << endl;
}

// 函数调用：实参
int main() {
    printNumber(10);  // 10是实参

    int x = 20;
    printNumber(x);   // x是实参

    return 0;
}

【关键理解】

1. 形参（Parameter）：
   - 在函数定义时声明
   - 是一个占位符，代表"将来会传入的值"
   - 必须指定类型和名字

2. 实参（Argument）：
   - 在函数调用时提供
   - 是实际传入的值
   - 可以是常量、变量、表达式

3. 参数传递：
   - 调用函数时，实参的值会复制给形参
   - 形参和实参是两个独立的变量
   - 修改形参不会影响实参（值传递）
)" << endl;

    // --------------------------------------------
    // 1.2 参数传递的过程
    // --------------------------------------------

    cout << "\n1.2 参数传递的过程" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【参数传递的步骤】

void add(int a, int b) {
    cout << a + b << endl;
}

int main() {
    int x = 10;
    int y = 20;
    add(x, y);  // 调用函数
    return 0;
}

执行过程：

1. 准备调用 add(x, y)
   - x的值是10
   - y的值是20

2. 创建形参
   - 创建形参a，复制x的值 → a = 10
   - 创建形参b，复制y的值 → b = 20

3. 执行函数体
   - 计算 a + b = 10 + 20 = 30
   - 打印30

4. 函数结束
   - 销毁形参a和b
   - 返回main函数

【重要：值传递】

void changeValue(int num) {
    num = 100;  // 修改形参
    cout << "函数内：" << num << endl;  // 输出100
}

int main() {
    int x = 10;
    changeValue(x);
    cout << "函数外：" << x << endl;  // 输出10（没有改变）
    return 0;
}

结论：
- 修改形参不会影响实参
- 因为形参是实参的副本
- 这叫做"值传递"（Pass by Value）
)" << endl;

    // --------------------------------------------
    // 1.3 多个参数的处理
    // --------------------------------------------

    cout << "\n1.3 多个参数的处理" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【多参数函数】

// 定义：3个参数
void printInfo(string name, int age, double height) {
    cout << "姓名：" << name << endl;
    cout << "年龄：" << age << endl;
    cout << "身高：" << height << endl;
}

// 调用：按顺序传入3个实参
int main() {
    printInfo("张三", 20, 1.75);
    return 0;
}

【参数匹配规则】

1. 数量必须匹配
   ❌ printInfo("张三", 20);  // 错误：缺少参数
   ❌ printInfo("张三", 20, 1.75, 100);  // 错误：参数过多

2. 类型必须匹配（或可以自动转换）
   ✅ printInfo("张三", 20, 1.75);  // 正确
   ✅ printInfo("张三", 20, 175);   // 正确：int自动转double
   ❌ printInfo(20, "张三", 1.75);  // 错误：类型顺序不对

3. 顺序必须匹配
   - 第1个实参对应第1个形参
   - 第2个实参对应第2个形参
   - 以此类推
)" << endl;
}

// ============================================
// 第二部分：基础操作实践
// ============================================

// --------------------------------------------
// 2.1 单个参数的函数
// --------------------------------------------

// 函数定义：打印问候语
// 参数：string name - 要问候的人的名字
void greet(string name) {
    cout << "你好，" << name << "！" << endl;
}

// --------------------------------------------
// 2.2 多个参数的函数
// --------------------------------------------

// 函数定义：计算矩形面积
// 参数：double length - 长度
//       double width - 宽度
void printRectangleArea(double length, double width) {
    // 计算面积
    double area = length * width;

    // 打印结果
    cout << "长：" << length << "，宽：" << width << endl;
    cout << "面积：" << area << endl;
}

// --------------------------------------------
// 2.3 不同类型的参数
// --------------------------------------------

// 函数定义：打印学生信息
// 参数：string name - 姓名（字符串类型）
//       int age - 年龄（整数类型）
//       double score - 成绩（小数类型）
//       bool isPassed - 是否及格（布尔类型）
void printStudentInfo(string name, int age, double score, bool isPassed) {
    cout << "学生信息：" << endl;
    cout << "  姓名：" << name << endl;
    cout << "  年龄：" << age << endl;
    cout << "  成绩：" << score << endl;
    cout << "  是否及格：" << (isPassed ? "是" : "否") << endl;
}

// --------------------------------------------
// 2.4 参数是表达式
// --------------------------------------------

// 函数定义：打印两个数的和
// 参数：int a - 第一个数
//       int b - 第二个数
void printSum(int a, int b) {
    cout << a << " + " << b << " = " << (a + b) << endl;
}

// --------------------------------------------
// 2.5 演示值传递
// --------------------------------------------

// 函数定义：尝试修改参数
// 参数：int num - 要修改的数
void tryToChange(int num) {
    cout << "函数内修改前：" << num << endl;

    // 修改形参
    num = 999;

    cout << "函数内修改后：" << num << endl;
}

void testFunctions() {
    cout << string(60, '=') << endl;
    cout << "第二部分：基础操作实践" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 测试1：单个参数
    // --------------------------------------------

    cout << "\n2.1 单个参数的函数" << endl;
    cout << string(60, '-') << endl;

    // 传入字符串常量
    greet("小明");

    // 传入字符串变量
    string name = "小红";
    greet(name);

    cout << "\n说明：" << endl;
    cout << "- 可以传入常量（\"小明\"）" << endl;
    cout << "- 也可以传入变量（name）" << endl;

    // --------------------------------------------
    // 测试2：多个参数
    // --------------------------------------------

    cout << "\n2.2 多个参数的函数" << endl;
    cout << string(60, '-') << endl;

    // 传入两个参数
    printRectangleArea(5.0, 3.0);

    cout << endl;

    // 使用变量传参
    double len = 10.5;
    double wid = 4.2;
    printRectangleArea(len, wid);

    cout << "\n说明：" << endl;
    cout << "- 多个参数用逗号分隔" << endl;
    cout << "- 参数顺序要和函数定义一致" << endl;

    // --------------------------------------------
    // 测试3：不同类型的参数
    // --------------------------------------------

    cout << "\n2.3 不同类型的参数" << endl;
    cout << string(60, '-') << endl;

    // 传入4个不同类型的参数
    printStudentInfo("张三", 20, 85.5, true);

    cout << endl;

    printStudentInfo("李四", 19, 58.0, false);

    cout << "\n说明：" << endl;
    cout << "- 参数可以是不同类型" << endl;
    cout << "- 每个参数都要按类型传入正确的值" << endl;

    // --------------------------------------------
    // 测试4：参数是表达式
    // --------------------------------------------

    cout << "\n2.4 参数是表达式" << endl;
    cout << string(60, '-') << endl;

    // 传入常量
    printSum(10, 20);

    // 传入变量
    int x = 5;
    int y = 8;
    printSum(x, y);

    // 传入表达式
    printSum(x + 1, y * 2);

    // 传入函数调用结果（需要先定义返回int的函数）
    // printSum(getNumber(), getNumber());

    cout << "\n说明：" << endl;
    cout << "- 实参可以是常量、变量、表达式" << endl;
    cout << "- 表达式会先计算，再传入函数" << endl;
    cout << "- x + 1 = 6，y * 2 = 16" << endl;

    // --------------------------------------------
    // 测试5：值传递演示
    // --------------------------------------------

    cout << "\n2.5 值传递演示" << endl;
    cout << string(60, '-') << endl;

    int num = 100;
    cout << "调用前：num = " << num << endl;

    // 调用函数
    tryToChange(num);

    cout << "调用后：num = " << num << endl;

    cout << "\n说明：" << endl;
    cout << "- 函数内修改了形参，但实参没有改变" << endl;
    cout << "- 这是因为形参是实参的副本" << endl;
    cout << "- 修改副本不会影响原始数据" << endl;
}

/*
==============================================
🔍 深入理解：参数传递机制（选学）
==============================================

如果你想知道"参数传递的底层原理"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【值传递的内存模型】

void func(int a) {
    a = 100;
}

int main() {
    int x = 10;
    func(x);
    return 0;
}

内存变化：

1. main函数中：
   [x: 10]  ← x在main的栈帧中

2. 调用func(x)时：
   [x: 10]  ← main的栈帧
   [a: 10]  ← func的栈帧，复制x的值

3. 执行a = 100：
   [x: 10]  ← main的栈帧（没变）
   [a: 100] ← func的栈帧（改变了）

4. func返回后：
   [x: 10]  ← main的栈帧
   （func的栈帧被销毁）

结论：
- 形参a和实参x是两个独立的内存空间
- 修改a不会影响x

【为什么要用值传递？】

优点：
1. 安全：函数不会意外修改外部数据
2. 简单：不需要考虑副作用
3. 清晰：函数的行为更可预测

缺点：
1. 效率：复制大数据会很慢
2. 限制：无法通过参数返回多个值

解决方案（后续会学）：
- 引用传递（&）：可以修改实参
- 指针传递（*）：可以修改实参
- const引用：高效且安全

【参数传递的类型转换】

void printNumber(double num) {
    cout << num << endl;
}

int main() {
    int x = 10;
    printNumber(x);  // int自动转换为double
    return 0;
}

自动类型转换规则：
- int → double：安全，会自动转换
- double → int：不安全，会截断小数部分
- char → int：安全，转换为ASCII码
- string → int：不能自动转换，会报错

==============================================
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void commonMistakes() {
    cout << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
// --------------------------------------------
// 4.1 错误1：参数数量不匹配
// --------------------------------------------

❌ 错误代码：

void printInfo(string name, int age) {
    cout << name << ", " << age << endl;
}

int main() {
    printInfo("张三");  // 错误：缺少参数
    return 0;
}

结果：编译错误 - too few arguments to function

原因：函数定义了2个参数，但只传入了1个

✅ 正确代码：

void printInfo(string name, int age) {
    cout << name << ", " << age << endl;
}

int main() {
    printInfo("张三", 20);  // 正确：传入2个参数
    return 0;
}

记忆技巧：
- 调用函数时，参数数量必须和定义时完全一致
- 编译器会检查参数数量，不匹配会报错

// --------------------------------------------
// 4.2 错误2：参数类型不匹配
// --------------------------------------------

❌ 错误代码：

void printAge(int age) {
    cout << "年龄：" << age << endl;
}

int main() {
    printAge("20");  // 错误：传入字符串，期望整数
    return 0;
}

结果：编译错误 - invalid conversion from 'const char*' to 'int'

原因：函数期望int类型，但传入了string类型

✅ 正确代码：

void printAge(int age) {
    cout << "年龄：" << age << endl;
}

int main() {
    printAge(20);  // 正确：传入整数
    return 0;
}

记忆技巧：
- 参数类型必须匹配（或可以自动转换）
- string不能自动转换为int

// --------------------------------------------
// 4.3 错误3：参数顺序错误
// --------------------------------------------

❌ 错误代码：

void printInfo(string name, int age) {
    cout << name << ", " << age << endl;
}

int main() {
    printInfo(20, "张三");  // 错误：顺序反了
    return 0;
}

结果：编译错误 - 类型不匹配

原因：第1个参数期望string，但传入了int

✅ 正确代码：

void printInfo(string name, int age) {
    cout << name << ", " << age << endl;
}

int main() {
    printInfo("张三", 20);  // 正确：顺序正确
    return 0;
}

记忆技巧：
- 参数顺序必须和函数定义一致
- 第1个实参对应第1个形参，第2个对应第2个

// --------------------------------------------
// 4.4 错误4：期望修改实参但用了值传递
// --------------------------------------------

❌ 错误代码：

void doubleValue(int num) {
    num = num * 2;  // 只修改了形参
}

int main() {
    int x = 10;
    doubleValue(x);
    cout << x << endl;  // 输出10，没有改变
    return 0;
}

结果：x的值没有改变

原因：值传递只修改形参，不影响实参

✅ 正确代码（使用返回值）：

int doubleValue(int num) {
    return num * 2;  // 返回结果
}

int main() {
    int x = 10;
    x = doubleValue(x);  // 用返回值更新x
    cout << x << endl;  // 输出20
    return 0;
}

记忆技巧：
- 值传递不能修改实参
- 如果需要修改，使用返回值或引用（后续会学）

// --------------------------------------------
// 4.5 错误5：忘记传递必需的参数
// --------------------------------------------

❌ 错误代码：

void calculateArea(double length, double width) {
    cout << length * width << endl;
}

int main() {
    calculateArea();  // 错误：没有传参
    return 0;
}

结果：编译错误 - too few arguments

原因：函数需要2个参数，但没有传入

✅ 正确代码：

void calculateArea(double length, double width) {
    cout << length * width << endl;
}

int main() {
    calculateArea(5.0, 3.0);  // 正确：传入2个参数
    return 0;
}

记忆技巧：
- 所有参数都是必需的（除非有默认值，后续会学）
- 调用时必须提供所有参数

// --------------------------------------------
// 4.6 错误6：参数名和变量名混淆
// --------------------------------------------

❌ 容易混淆的代码：

void printValue(int value) {
    cout << value << endl;
}

int main() {
    int value = 10;
    printValue(value);  // 这里的value是实参
    return 0;
}

说明：
- main中的value和函数中的value是两个不同的变量
- 虽然名字相同，但在不同的作用域
- 函数中的value是形参，main中的value是局部变量

✅ 更清晰的写法：

void printValue(int num) {  // 形参用不同的名字
    cout << num << endl;
}

int main() {
    int value = 10;  // 实参变量
    printValue(value);  // 传入value的值
    return 0;
}

记忆技巧：
- 形参和实参可以同名，但它们是不同的变量
- 为了清晰，建议使用不同的名字

// --------------------------------------------
// 4.7 错误7：传入表达式时的计算顺序
// --------------------------------------------

⚠️ 需要注意的代码：

void printTwo(int a, int b) {
    cout << a << ", " << b << endl;
}

int main() {
    int x = 1;
    printTwo(x++, x++);  // 输出可能是 1, 2 或 2, 1
    return 0;
}

问题：
- C++没有规定参数的计算顺序
- x++的执行顺序不确定
- 结果可能因编译器而异

✅ 正确代码：

void printTwo(int a, int b) {
    cout << a << ", " << b << endl;
}

int main() {
    int x = 1;
    int a = x++;  // 先计算第一个参数
    int b = x++;  // 再计算第二个参数
    printTwo(a, b);  // 输出 1, 2（确定的顺序）
    return 0;
}

记忆技巧：
- 避免在参数中使用++、--等会改变变量的操作
- 如果需要，先计算好再传入

)" << endl;
}

// ============================================
// 第五部分：函数卡片速查
// ============================================

void functionCards() {
    cout << string(60, '=') << endl;
    cout << "第五部分：函数卡片速查" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【参数定义语法】

返回类型 函数名(参数类型1 参数名1, 参数类型2 参数名2, ...) {
    // 函数体
}

────────────────────────────────────

【单参数函数】

void greet(string name) {
    cout << "Hello, " << name << endl;
}

调用：greet("Alice");

────────────────────────────────────

【多参数函数】

void printInfo(string name, int age, double height) {
    cout << name << ", " << age << ", " << height << endl;
}

调用：printInfo("Bob", 20, 1.75);

────────────────────────────────────

【值传递特点】

- 实参的值复制给形参
- 修改形参不影响实参
- 形参和实参是独立的变量

示例：
void change(int num) {
    num = 100;  // 只改形参
}

int x = 10;
change(x);
// x仍然是10

────────────────────────────────────

【参数匹配规则】

1. 数量必须匹配
2. 类型必须匹配（或可自动转换）
3. 顺序必须匹配

)" << endl;
}

// ============================================
// 第六部分：练习题
// ============================================

void exercises() {
    cout << string(60, '=') << endl;
    cout << "练习题" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
1. 编写一个函数 printSquare(int num)
   要求：打印num的平方
   提示：计算 num * num 并打印

2. 编写一个函数 printMax(int a, int b)
   要求：打印两个数中的较大值
   提示：使用if判断，打印较大的那个

3. 编写一个函数 printTriangleArea(double base, double height)
   要求：打印三角形面积（面积 = 底 × 高 ÷ 2）
   提示：计算 base * height / 2.0

4. 编写一个函数 printGrade(string name, double score)
   要求：打印学生姓名和成绩等级
   提示：
   - score >= 90：优秀
   - score >= 80：良好
   - score >= 60：及格
   - score < 60：不及格

5. 编写一个函数 printRange(int start, int end)
   要求：打印从start到end的所有整数
   提示：使用for循环，从start到end

)" << endl;
}

// ============================================
// 主程序：运行所有示例
// ============================================

int main() {
    // 运行所有测试
    conceptExplanation();
    cout << "\n";
    testFunctions();
    cout << "\n";
    commonMistakes();
    cout << "\n";
    functionCards();
    cout << "\n";
    exercises();

    return 0;
}

// 编译命令：g++ -o 13_function_parameters 13_function_parameters.cpp
// 运行命令：./13_function_parameters 或 13_function_parameters.exe
