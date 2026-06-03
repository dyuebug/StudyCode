// ============================================
// 42_copy_constructor.cpp - 超详细注释版
// 日期：第13天
// 难度：⭐⭐ 中级
// 主题：拷贝构造函数
// ============================================

/*
【核心概念】

是什么？
- 拷贝构造函数（Copy Constructor）是一种特殊的构造函数
- 语法固定：ClassName(const ClassName& other)
- 参数必须是同类型对象的 const 引用

为什么需要？
- 当用已有对象创建新对象时，编译器需要知道如何"复制"
- 默认行为（浅拷贝）在含有指针成员时会导致崩溃（double free）
- 自定义拷贝构造函数可以实现深拷贝，确保内存独立

核心特点：
- 编译器会自动生成默认版本（执行浅拷贝）
- 参数必须是引用类型，否则会导致无限递归
- 有三种情况会自动触发调用
*/

#include <iostream>
#include <cstring>
using namespace std;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

/*
【函数卡片：拷贝构造函数】

语法：
    ClassName(const ClassName& other)

参数详解：
    const      — 保证不修改被拷贝的原对象
    ClassName& — 引用类型（必须！否则无限递归）
    other      — 被拷贝的源对象

调用时机（三种情况）：
    1. 初始化新对象：MyClass b = a;  或  MyClass b(a);
    2. 函数按值传参：void func(MyClass obj)  调用时触发
    3. 函数返回值：  return obj;  （部分情况，视编译器优化）

默认行为（编译器自动生成）：
    - 逐成员复制（memberwise copy）
    - 值类型成员：完全独立的副本（安全）
    - 指针成员：只复制地址，两个对象共享同一块内存（危险！）
*/

// ============================================
// 第二部分：基础操作实践
// ============================================

// ---- 2.1 不含指针的类：默认拷贝构造函数完全安全 ----

class SimplePoint {
public:
    int x;  // 值类型成员
    int y;  // 值类型成员

    // 普通构造函数：初始化 x 和 y
    SimplePoint(int x, int y) : x(x), y(y) {
        cout << "构造函数：Point(" << x << ", " << y << ")" << endl;
    }

    // 没有自定义拷贝构造函数
    // 编译器自动生成：SimplePoint(const SimplePoint& other)
    //   { x = other.x; y = other.y; }
    // 因为 x、y 都是 int 值类型，逐成员复制完全安全

    void show() const {
        cout << "Point(" << x << ", " << y << ")" << endl;
    }
};

void demoDefaultCopy() {
    cout << string(60, '=') << endl;
    cout << "第二部分 2.1：默认拷贝构造函数（编译器自动生成）" << endl;
    cout << string(60, '=') << endl;

    SimplePoint p1(3, 4);

    // 初始化写法1：等号写法（调用拷贝构造函数，不是赋值运算符！）
    // 原因：p2 是新对象，这是"初始化"，不是"赋值"
    SimplePoint p2 = p1;

    // 初始化写法2：括号写法（与上面完全等价）
    SimplePoint p3(p1);

    cout << "p1: "; p1.show();
    cout << "p2: "; p2.show();
    cout << "p3: "; p3.show();

    // 修改 p2 的成员，验证 p1 不受影响
    // 原因：int 是值类型，p2.x 和 p1.x 是完全独立的内存
    p2.x = 100;
    cout << "\n修改 p2.x = 100 后（p1 不受影响，因为内存独立）：" << endl;
    cout << "p1: "; p1.show();
    cout << "p2: "; p2.show();
}

// ---- 2.2 含有指针成员的类：必须自定义深拷贝 ----

class MyString {
private:
    char* data;   // 指向堆内存的指针
    int   length; // 字符串长度

public:
    // 普通构造函数
    // 参数：str - 用于初始化的 C 风格字符串
    MyString(const char* str = "") {
        length = strlen(str);           // 获取字符串长度（不含 '\0'）
        data   = new char[length + 1];  // 在堆上分配内存（+1 为 '\0' 预留空间）
        strcpy(data, str);              // 将 str 内容复制到 data 指向的内存
        cout << "普通构造：\"" << data << "\" (地址:" << (void*)data << ")" << endl;
    }

    // 自定义拷贝构造函数（深拷贝）
    // 参数：other - 被拷贝的源对象（const 引用，防止修改原对象）
    // 目的：分配新内存，使两个对象的 data 指向不同地址
    MyString(const MyString& other) {
        length = other.length;           // 复制长度
        data   = new char[length + 1];   // 分配新内存（关键！不能直接复制指针地址）
        strcpy(data, other.data);        // 把内容复制到新内存
        cout << "拷贝构造：\"" << data << "\" (地址:" << (void*)data << ")" << endl;
    }

    // 析构函数：释放堆内存
    ~MyString() {
        cout << "析构：\"" << data << "\" (地址:" << (void*)data << ")" << endl;
        delete[] data;  // 每个对象各自释放自己的内存，不会 double free
    }

    void show() const {
        cout << "MyString: \"" << data << "\", 长度:" << length
             << " (地址:" << (void*)data << ")" << endl;
    }

    // 修改首字符，用于验证两个对象的内存是否独立
    void setFirst(char c) {
        if (length > 0) data[0] = c;
    }
};

void demoCustomCopy() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.2：自定义拷贝构造函数（深拷贝）" << endl;
    cout << string(60, '=') << endl;

    MyString s1("Hello");
    MyString s2 = s1;  // 调用自定义拷贝构造函数

    cout << "\n拷贝后（注意两个对象的内存地址不同）：" << endl;
    s1.show();
    s2.show();

    // 修改 s2 的内容，验证 s1 不受影响
    // 原因：深拷贝后 s1.data 和 s2.data 指向不同内存
    s2.setFirst('J');
    cout << "\n修改 s2 首字符为 'J' 后（s1 不受影响）：" << endl;
    s1.show();
    s2.show();
}

// ---- 2.3 拷贝构造函数的三种调用时机 ----

// 时机2：函数参数按值传递
// 调用 printString(s) 时，会将 s 拷贝一份给形参，触发拷贝构造函数
void printString(MyString s) {
    // 参数 s 是 MyString 的副本（深拷贝）
    cout << "  函数内部："; s.show();
}  // 函数结束，形参 s 被析构（释放副本的内存）

// 时机3：函数返回值
MyString makeString() {
    MyString temp("World");
    return temp;
    // 编译器可能优化（RVO：Return Value Optimization）
    // 优化后直接在调用方的内存中构造，省略一次拷贝
}

void demoCopyTiming() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.3：三种调用时机" << endl;
    cout << string(60, '=') << endl;

    cout << "\n[时机1] 用已有对象初始化新对象：" << endl;
    MyString a("Apple");
    MyString b = a;  // 触发拷贝构造

    cout << "\n[时机2] 函数参数按值传递（注意：进入函数触发拷贝构造，离开函数触发析构）：" << endl;
    printString(a);

    cout << "\n[时机3] 函数返回值（视编译器RVO优化而定）：" << endl;
    MyString c = makeString();
    c.show();
}

// ============================================
// 🔍 第三部分：深入理解（可选，不影响后续学习）
// ============================================

/*
【深入理解：为什么参数必须是引用？】

假设写成：
    MyString(MyString other)  // ❌ 按值传参

问题分析：
    1. 调用 MyString b = a; 时，需要拷贝 a 给参数 other
    2. 拷贝 a 给 other 时，又需要调用拷贝构造函数
    3. 调用拷贝构造函数时，又需要拷贝参数……
    4. 无限递归 → 栈溢出崩溃

结论：拷贝构造函数的参数必须是引用（& 或 &&）

【深入理解：浅拷贝崩溃的原理】

假设默认拷贝构造函数执行：
    data   = other.data;  // 只复制指针地址（两个指针指向同一块内存！）
    length = other.length;

问题：
    s1.data ──┐
               └──→ [H][e][l][l][o][\0]  （同一块堆内存）
    s2.data ──┘

    s2 析构：delete[] s2.data  → 内存被释放
    s1 析构：delete[] s1.data  → 再次释放同一块内存 → double free → 崩溃！
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonErrors() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第四部分：常见错误和陷阱" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
❌ 错误1：拷贝构造函数参数不用引用（无限递归）
   // 错误写法：
   MyString(MyString other) { ... }

   // 触发过程：
   // MyString b = a → 调用拷贝构造 → 需要复制参数 other
   //                → 再次调用拷贝构造 → 需要复制参数 other → ...无限递归

   ✅ 正确写法：
   MyString(const MyString& other) { ... }
   // 使用引用，直接绑定原对象，不触发额外拷贝

   预防：拷贝构造函数的参数永远写成 const 引用

------------------------------------------------------------

❌ 错误2：含指针成员时不自定义拷贝构造函数（double free）
   // 问题场景：
   class BadString {
       char* data;
   public:
       BadString(const char* s) { data = new char[strlen(s)+1]; strcpy(data, s); }
       ~BadString() { delete[] data; }  // 有析构，说明管理内存
       // 没有自定义拷贝构造函数！→ 编译器生成浅拷贝 → data 地址共享
   };
   BadString s1("Hi");
   BadString s2 = s1;  // s2.data 和 s1.data 指向同一块内存
   // 析构时：s2 先 delete，s1 再 delete 同一地址 → 崩溃！

   ✅ 正确：含指针成员时必须自定义深拷贝构造函数，分配新内存

   记忆技巧：有析构释放内存 → 就需要深拷贝构造函数（三法则）

------------------------------------------------------------

❌ 错误3：分不清"初始化"和"赋值"（误认为都是赋值运算符）
   MyString a("Hello");
   MyString b = a;  // ← 这里是"初始化"，调用拷贝构造函数
   b = a;           // ← 这里是"赋值"，调用赋值运算符（operator=）

   // 区别：b 是新对象时用拷贝构造函数，b 已存在时用赋值运算符

   ✅ 判断方法：
   // 看 b 是否已经存在（被构造过）
   // - 不存在 → 初始化 → 拷贝构造函数
   // - 已存在 → 赋值   → 赋值运算符

------------------------------------------------------------

❌ 错误4：拷贝构造函数内忘记分配新内存
   MyString(const MyString& other) {
       length = other.length;
       data   = other.data;  // ❌ 只复制了地址！
       strcpy(data, other.data);  // 没意义，两个 data 指向同一块
   }

   ✅ 正确：必须先 new，再 strcpy
   MyString(const MyString& other) {
       length = other.length;
       data   = new char[length + 1];  // ✅ 分配新内存
       strcpy(data, other.data);       // ✅ 复制内容到新内存
   }

------------------------------------------------------------

❌ 错误5：误以为按值传参不会触发拷贝构造
   void func(MyString s) { ... }  // 参数是值类型

   MyString a("Test");
   func(a);  // ← 这里会触发拷贝构造函数！
   // 传入时：拷贝构造 a → s
   // 函数结束：析构 s

   ✅ 如果不需要修改参数，用 const 引用避免不必要的拷贝：
   void func(const MyString& s) { ... }  // 不触发拷贝构造，性能更好

------------------------------------------------------------

❌ 错误6：拷贝构造函数遗漏某些成员
   class Rect {
       double width, height;
       string color;  // 忘记复制！
   public:
       Rect(const Rect& other) {
           width  = other.width;
           height = other.height;
           // ❌ 忘记复制 color，导致 color 使用默认值
       }
   };

   ✅ 使用初始化列表，更不容易遗漏：
   Rect(const Rect& other)
       : width(other.width), height(other.height), color(other.color) {}

------------------------------------------------------------

❌ 错误7：const 对象无法被拷贝（参数没有加 const）
   MyString(MyString& other) { ... }  // ❌ 参数没有 const

   const MyString s("Hi");
   MyString t = s;  // 错误！const 对象无法绑定到非 const 引用

   ✅ 始终在参数前加 const：
   MyString(const MyString& other) { ... }  // ✅ 可以拷贝 const 对象
)";
}

// ============================================
// 第五部分：函数卡片速查
// ============================================

void showFunctionCards() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第五部分：函数卡片速查" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【拷贝构造函数】

语法：
    ClassName(const ClassName& other)

参数：
    - other (const ClassName&): 被拷贝的源对象，const 引用类型
      * const：防止拷贝构造函数内修改源对象
      * &：引用类型，防止无限递归

返回值：
    - 无返回值（构造函数不写返回类型）

调用时机：
    1. MyClass b = a;       → 初始化新对象
    2. void f(MyClass obj)  → 函数按值传参
    3. return obj;          → 函数返回值（视RVO优化）

示例：
    // 基本实现（值类型成员，默认也够用）
    Point(const Point& other) : x(other.x), y(other.y) {}

    // 深拷贝实现（含指针成员，必须手动分配新内存）
    MyString(const MyString& other) {
        length = other.length;
        data   = new char[length + 1];  // 分配新内存
        strcpy(data, other.data);       // 复制内容
    }

────────────────────────────────────────────────────

【三法则 (Rule of Three)】

如果一个类需要以下任意一个，通常三个都需要：
    1. 析构函数         ~ClassName()
    2. 拷贝构造函数     ClassName(const ClassName& other)
    3. 赋值运算符       ClassName& operator=(const ClassName& other)

触发条件：类管理了资源（堆内存、文件、网络连接等）

────────────────────────────────────────────────────

【深拷贝 vs 浅拷贝】

浅拷贝（编译器默认行为）：
    data = other.data;   // 只复制指针地址
    危险！两个对象共享内存，析构时 double free

深拷贝（自定义拷贝构造函数）：
    data = new char[length + 1];  // 分配新内存
    strcpy(data, other.data);     // 复制内容
    安全！两个对象内存独立
)";
}

// ============================================
// 第六部分：练习题
// ============================================

void showExercises() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第六部分：练习题（独立思考，不要看答案）" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
1. 判断题：分析以下代码，哪些行调用了拷贝构造函数？

   MyString a("Hello");
   MyString b = a;          // ← ①
   MyString c(b);           // ← ②
   b = a;                   // ← ③
   void show(MyString s);
   show(a);                 // ← ④

   提示：区分"初始化"和"赋值"，区分"构造新对象"和"给已有对象赋值"

------------------------------------------------------------

2. 实现练习：
   定义一个 DynamicArray 类，包含：
   - int* data（指向堆内存的指针）
   - int size（数组大小）
   - 构造函数：接受 size 参数，new 分配内存
   - 析构函数：delete[] data
   - 深拷贝构造函数（核心！）

   要求：在 main 中验证：
   DynamicArray a(5);
   DynamicArray b = a;
   // 修改 b 的元素后，a 的元素不受影响

   提示：深拷贝构造函数需要先 new 分配新内存，再逐元素复制

------------------------------------------------------------

3. 思考题：
   class Wrapper {
       int* p;
   public:
       Wrapper(int val) { p = new int(val); }
       ~Wrapper() { delete p; }
       // 没有拷贝构造函数
   };

   Wrapper a(42);
   Wrapper b = a;   // 这行代码执行后会发生什么？
   // 尝试分析：a.p 和 b.p 各指向哪里？
   //           程序结束时会发生什么问题？

   提示：画出内存图（堆和栈），追踪指针的指向

------------------------------------------------------------

4. 进阶练习：
   在 MyString 类的拷贝构造函数内，加入打印语句
   验证以下三种情况是否都会触发拷贝构造函数：

   a) MyString s2 = s1;
   b) void print(MyString s); 调用 print(s1);
   c) MyString make() { MyString t("tmp"); return t; }
      MyString s3 = make();

   提示：c) 中可能因为 RVO（返回值优化）而不触发
)";
}

// ============================================
// main 函数
// ============================================

int main() {
    cout << string(60, '=') << endl;
    cout << "      42_copy_constructor.cpp - 拷贝构造函数" << endl;
    cout << string(60, '=') << endl;

    demoDefaultCopy();
    demoCustomCopy();
    demoCopyTiming();
    showCommonErrors();
    showFunctionCards();
    showExercises();

    cout << "\n" << string(60, '=') << endl;
    cout << "核心要点总结" << endl;
    cout << string(60, '=') << endl;
    cout << "1. 拷贝构造函数语法：ClassName(const ClassName& other)" << endl;
    cout << "2. 参数必须是引用，否则无限递归崩溃" << endl;
    cout << "3. 含指针成员 → 必须自定义深拷贝（分配新内存 + 复制内容）" << endl;
    cout << "4. 三种触发时机：初始化 / 按值传参 / 返回值" << endl;
    cout << "5. 有析构 + 有指针 → 三法则：析构/拷贝构造/赋值运算符都要写" << endl;

    return 0;
}
