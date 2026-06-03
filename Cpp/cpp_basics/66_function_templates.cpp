// ============================================
// 函数模板基础 - 超详细注释版
// 日期：2026-05-07（第21天）
// 难度：⭐⭐ 中级
// ============================================

/*
【核心概念】函数模板

是什么？
- 函数模板是一种代码生成工具，用一个模板定义可以生成多个不同类型的函数

为什么需要？
- 避免为每种类型重复编写相同逻辑的函数
- 提供类型安全的泛型编程
- STL 的算法（sort、find 等）都基于函数模板实现

核心特点：
- 编译期机制：编译时根据使用的类型生成具体代码
- 类型安全：编译器会检查类型是否支持模板中的操作
- 零运行时开销：模板实例化后就是普通函数，没有额外开销
*/

#include <iostream>
#include <string>
using namespace std;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

void part1_concepts() {
    cout << string(60, '=') << endl;
    cout << "第一部分：核心概念详解" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【为什么需要模板？】

问题：为每种类型写重复的函数
int max_int(int a, int b) { return a > b ? a : b; }
double max_double(double a, double b) { return a > b ? a : b; }

解决：用模板一次编写，多种类型复用
template<typename T>
T max(T a, T b) { return a > b ? a : b; }

【函数模板语法】

template<typename T>        // 模板声明
返回类型 函数名(参数列表) {  // 函数定义
    // 函数体
}

【模板参数推导】

编译器自动推导：
int x = max(10, 20);        // T = int
double y = max(3.14, 2.71); // T = double

显式指定：
int z = max<double>(10, 3.14); // T = double

【模板实例化】

编译期生成代码：
max(10, 20)     → 生成 int max(int, int)
max(3.14, 2.71) → 生成 double max(double, double)
)" << endl;
}

// ============================================
// 第二部分：基础操作实践
// ============================================

// 函数模板：返回两个值中的较大值
template<typename T>
T myMax(T a, T b) {
    return a > b ? a : b;
}

// 函数模板：交换两个值
template<typename T>
void mySwap(T& a, T& b) {
    T temp = a;
    a = b;
    b = temp;
}

// 函数模板：打印数组
template<typename T>
void printArray(const T* arr, int size) {
    cout << "[";
    for (int i = 0; i < size; i++) {
        cout << arr[i];
        if (i < size - 1) cout << ", ";
    }
    cout << "]" << endl;
}

// 函数模板：查找元素
template<typename T>
int findElement(const T* arr, int size, const T& target) {
    for (int i = 0; i < size; i++) {
        if (arr[i] == target) {
            return i;
        }
    }
    return -1;
}

// 函数模板：多个模板参数
template<typename T1, typename T2>
void printPair(T1 first, T2 second) {
    cout << "(" << first << ", " << second << ")" << endl;
}

void part2_practice() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分：基础操作实践" << endl;
    cout << string(60, '=') << endl;

    // 示例1：myMax
    cout << "\n示例1：myMax" << endl;
    cout << "max(10, 20) = " << myMax(10, 20) << endl;
    cout << "max(3.14, 2.71) = " << myMax(3.14, 2.71) << endl;
    cout << "max(\"hello\", \"world\") = " << myMax(string("hello"), string("world")) << endl;

    // 示例2：mySwap
    cout << "\n示例2：mySwap" << endl;
    int x = 10, y = 20;
    cout << "交换前：x = " << x << ", y = " << y << endl;
    mySwap(x, y);
    cout << "交换后：x = " << x << ", y = " << y << endl;

    // 示例3：printArray
    cout << "\n示例3：printArray" << endl;
    int intArr[] = {1, 2, 3, 4, 5};
    cout << "Int Array: ";
    printArray(intArr, 5);

    string strArr[] = {"apple", "banana", "cherry"};
    cout << "String Array: ";
    printArray(strArr, 3);

    // 示例4：findElement
    cout << "\n示例4：findElement" << endl;
    int index = findElement(intArr, 5, 3);
    cout << "Find 3 in intArr: index = " << index << endl;

    // 示例5：printPair
    cout << "\n示例5：printPair（多个模板参数）" << endl;
    printPair(10, 3.14);
    printPair(string("name"), string("Alice"));
    printPair('A', 65);
}

// ============================================
// 第三部分：深入理解（选学）
// ============================================

void part3_advanced() {
    cout << "\n" << string(60, '=') << endl;
    cout << "🔍 深入理解：模板实例化（选学）" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【模板编译过程】

1. 模板定义检查：检查语法
2. 模板实例化：使用时生成具体代码
3. 代码编译：编译生成的代码

【为什么模板定义要放在头文件？】

因为编译器需要看到完整定义才能实例化！
如果定义在 .cpp 中，其他文件看不到，无法实例化。

【模板代码膨胀】

每种类型生成一份代码：
- max<int> 生成一份
- max<double> 生成一份
- max<string> 生成一份

影响：可执行文件变大，编译时间变长
优化：编译器会优化掉未使用的实例化
)" << endl;
}

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void part4_mistakes() {
    cout << "\n" << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
❌ 错误1：参数类型不一致，编译器无法推导 T

   template<typename T>
   T myMax(T a, T b) { return a > b ? a : b; }

   myMax(10, 3.14);      // ❌ 编译错误：10 是 int，3.14 是 double，T 无法唯一确定

   ✅ 正确：显式指定模板参数，或统一参数类型
   myMax<double>(10, 3.14);  // 显式指定 T = double，10 自动转换
   myMax(10.0, 3.14);        // 两个都是 double，自动推导 T = double

   预防：如果参数类型可能混用，用多模板参数或显式指定

------------------------------------------------------------

❌ 错误2：模板类型不支持函数体内的操作

   struct Point { int x, y; };
   Point p = myMax(Point{1,2}, Point{3,4});  // ❌ 链接错误：Point 不支持 >

   // 编译器实例化 myMax<Point> 时，发现 Point 没有 operator>，报错

   ✅ 正确：确保类型支持模板内所有操作
   struct Point {
       int x, y;
       bool operator>(const Point& o) const { return x > o.x; }
   };
   Point p = myMax(Point{1,2}, Point{3,4});  // ✅ 有 operator> 了

------------------------------------------------------------

❌ 错误3：模板定义放在 .cpp 文件（链接错误）

   // utils.h
   template<typename T>
   T add(T a, T b);         // 只有声明

   // utils.cpp
   template<typename T>
   T add(T a, T b) { return a + b; }   // 定义在 .cpp

   // main.cpp
   int x = add(1, 2);       // ❌ 链接错误：找不到 add<int> 的实现

   原因：编译 main.cpp 时看不到 utils.cpp 的定义，无法实例化

   ✅ 正确：模板定义必须放在头文件（或直接在使用处可见）
   // utils.h
   template<typename T>
   T add(T a, T b) { return a + b; }   // 声明和定义都在头文件

------------------------------------------------------------

❌ 错误4：模板函数与普通函数重名，推导歧义

   void print(int x) { cout << "int: " << x; }   // 普通函数
   template<typename T>
   void print(T x)   { cout << "T: " << x; }      // 模板函数

   print(10);    // ✅ 优先调用普通函数（精确匹配 > 模板实例化）
   print(10.0);  // ✅ 调用模板（无精确匹配的普通函数）
   print<int>(10); // ✅ 显式指定，强制调用模板版本

   注意：理解重载解析顺序，避免意外调用错误版本

------------------------------------------------------------

❌ 错误5：在模板中使用 sizeof 期望得到元素数量

   template<typename T>
   void process(T arr) {
       int size = sizeof(arr) / sizeof(arr[0]);  // ❌ 数组退化为指针后，sizeof 是指针大小！
   }

   int nums[] = {1, 2, 3, 4, 5};
   process(nums);  // arr 是 int*，sizeof(arr)=8（指针大小），不是 20

   ✅ 正确：通过模板参数传递数组大小，或用 std::array/vector
   template<typename T, int N>
   void process(T (&arr)[N]) {  // 引用传递保留数组信息
       int size = N;            // N 就是数组元素个数
   }

------------------------------------------------------------

❌ 错误6：模板参数写了 class 却以为只能接受类类型

   template<class T>    // 误以为 T 只能是 class/struct
   T myMax(T a, T b) { return a > b ? a : b; }

   myMax(10, 20);       // ✅ 实际上完全可以用 int！

   说明：template<class T> 和 template<typename T> 完全等价
   class 在模板参数中只是历史遗留写法，不限制 T 只能是类类型
   推荐用 typename，语义更准确

------------------------------------------------------------

❌ 错误7：多参数模板的返回类型无法自动推导

   template<typename T1, typename T2>
   ??? add(T1 a, T2 b) { return a + b; }   // 返回类型是什么？

   add(1, 2.5);   // T1=int, T2=double，结果是 double，但编译器不知道

   ✅ C++11：用 decltype 尾置返回类型
   template<typename T1, typename T2>
   auto add(T1 a, T2 b) -> decltype(a + b) { return a + b; }

   ✅ C++14：直接 auto
   template<typename T1, typename T2>
   auto add(T1 a, T2 b) { return a + b; }  // 编译器从 return 推导
)" << endl;
}

// ============================================
// 第五部分：函数卡片速查
// ============================================

void part5_reference() {
    cout << "\n" << string(60, '=') << endl;
    cout << "函数卡片速查" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【函数模板基本语法】

template<typename T>
返回类型 函数名(参数列表) {
    函数体
}

示例：
template<typename T>
T max(T a, T b) { return a > b ? a : b; }

────────────────────────────────────

【多个模板参数】

template<typename T1, typename T2>
void print(T1 a, T2 b) {
    cout << a << ", " << b << endl;
}

────────────────────────────────────

【显式指定模板参数】

max<double>(10, 3.14);  // 显式指定 T = double

────────────────────────────────────

【返回类型推导】

template<typename T1, typename T2>
auto add(T1 a, T2 b) -> decltype(a + b) {
    return a + b;
}
)" << endl;
}

// ============================================
// 练习题
// ============================================

void exercises() {
    cout << "\n" << string(60, '=') << endl;
    cout << "练习题" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
1. 实现函数模板 min
   要求：返回两个值中的较小值
   提示：类似 max，但用 < 运算符

2. 实现函数模板 sum
   要求：计算数组所有元素的和
   提示：需要累加变量，初始值为 0

3. 实现函数模板 reverse
   要求：反转数组
   提示：交换首尾元素，向中间靠拢

4. 实现函数模板 compare
   要求：比较两个值，返回 -1（小于）、0（等于）、1（大于）
   提示：使用 < 和 == 运算符

5. 实现函数模板 clamp
   要求：将值限制在 [min, max] 范围内
   提示：如果小于 min 返回 min，大于 max 返回 max

6. 实现函数模板 contains
   要求：判断数组是否包含指定元素
   提示：遍历数组，比较每个元素

7. 实现函数模板 copy
   要求：复制数组
   提示：遍历源数组，复制到目标数组
)" << endl;
}

// ============================================
// 主函数
// ============================================

int main() {
    cout << "函数模板基础学习" << endl;
    cout << string(60, '=') << endl;

    part1_concepts();
    part2_practice();
    part3_advanced();
    part4_mistakes();
    part5_reference();
    exercises();

    cout << "\n" << string(60, '=') << endl;
    cout << "学习完成！" << endl;
    cout << string(60, '=') << endl;

    return 0;
}
