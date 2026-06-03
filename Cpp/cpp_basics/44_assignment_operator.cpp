// ============================================
// 44_assignment_operator.cpp - 超详细注释版
// 日期：第13天
// 难度：⭐⭐ 中级
// 主题：赋值运算符重载与三法则
// ============================================

/*
【核心概念】

是什么？
- 赋值运算符（operator=）：将已存在对象的值复制给另一个已存在对象
- 与拷贝构造函数的区别：拷贝构造是"用已有对象创建新对象"，赋值是"给已有对象重新赋值"

为什么需要自定义？
- 默认赋值运算符执行浅拷贝，含指针成员时会导致 double free
- 赋值时左侧对象已有内容（旧资源需要先释放）
- 需要处理自赋值（a = a）的边界情况

核心特点：
- 返回 *this 引用，支持链式赋值（a = b = c）
- 必须检查自赋值（this == &other），否则先释放自己再读自己 → 崩溃
- 三法则：析构/拷贝构造/赋值运算符，定义一个通常三个都要定义
*/

#include <iostream>
#include <cstring>
using namespace std;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

/*
【函数卡片：赋值运算符】

语法：
    ClassName& operator=(const ClassName& other)

参数：
    - other (const ClassName&): 赋值来源，const 引用，不修改源对象

返回值：
    - ClassName&（引用）：返回 *this，支持链式赋值（a = b = c）

四个必须步骤（顺序不能变！）：
    1. 自赋值检查：if (this == &other) return *this;
       原因：若是自赋值，后续会先 delete 自己的内存，再读取已释放的内存
    2. 释放旧资源：delete[] data; 或 delete data;
       原因：左侧对象已有内容，赋值前必须释放，否则内存泄漏
    3. 深拷贝新内容：new + 复制数据
    4. 返回 *this：return *this;

触发时机（区别于拷贝构造函数）：
    MyString b = a;   // ← 拷贝构造（b 是新对象，此时 b 不存在）
    b = a;            // ← 赋值运算符（b 已存在，重新赋值）
*/

// ============================================
// 第二部分：基础操作实践
// ============================================

// ---- 2.1 拷贝构造 vs 赋值运算符：区别演示 ----

void showDifference() {
    cout << string(60, '=') << endl;
    cout << "第二部分 2.1：拷贝构造 vs 赋值运算符" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
关键区分规则：看左侧对象是否已经存在

    MyString a("Hello");  // 普通构造（a 是新对象）

    MyString b = a;       // ← 拷贝构造！（b 是新对象，此时 b 不存在）
    MyString b(a);        // ← 拷贝构造！（等价写法）

    MyString c("World");  // 普通构造（c 已存在）
    c = a;                // ← 赋值运算符！（c 已存在，只是重新赋值）

    // 链式赋值（从右往左执行）：
    a = b = c;            // 先执行 b = c（赋值），再执行 a = (结果)（赋值）
)";
}

// ---- 2.2 完整的 MyString 类（含赋值运算符） ----

class MyString {
private:
    char* data;   // 指向堆上字符串内容
    int   length; // 字符串长度（不含 '\0'）

public:
    // 普通构造函数
    // 参数：str (const char*) - 初始字符串内容
    MyString(const char* str = "") {
        length = strlen(str);
        data   = new char[length + 1];
        strcpy(data, str);
        cout << "构造：\"" << data << "\"" << endl;
    }

    // 拷贝构造函数（深拷贝）
    // 参数：other (const MyString&) - 被拷贝的源对象
    MyString(const MyString& other) {
        length = other.length;
        data   = new char[length + 1];  // 分配新内存
        strcpy(data, other.data);       // 复制内容
        cout << "拷贝构造：\"" << data << "\"" << endl;
    }

    // 赋值运算符（深拷贝）
    // 参数：other (const MyString&) - 赋值来源
    // 返回：MyString& 引用（支持链式赋值）
    MyString& operator=(const MyString& other) {
        cout << "赋值：\"" << data << "\" = \"" << other.data << "\"" << endl;

        // 步骤1：自赋值检查（必须第一步！）
        // 原因：若跳过，步骤2会 delete 掉 data，
        //       步骤3却要读 other.data（此时已是悬空指针）→ 未定义行为
        if (this == &other) {
            cout << "  检测到自赋值，直接返回" << endl;
            return *this;
        }

        // 步骤2：释放旧资源（必须！否则内存泄漏）
        // 原因：*this 是已有对象，data 已指向一块堆内存，
        //       若直接覆盖 data 指针，旧内存永远无法释放
        delete[] data;

        // 步骤3：深拷贝（分配新内存 + 复制内容）
        length = other.length;
        data   = new char[length + 1];
        strcpy(data, other.data);

        // 步骤4：返回 *this（必须！支持 a = b = c 链式赋值）
        // 原因：a = b = c 等价于 a = (b = c)
        //       b = c 必须返回 b 的引用，才能继续执行 a = (b的结果)
        return *this;
    }

    // 析构函数
    ~MyString() {
        cout << "析构：\"" << data << "\"" << endl;
        delete[] data;
    }

    void show(const string& label = "") const {
        if (!label.empty()) cout << label << ": ";
        cout << "\"" << data << "\"" << endl;
    }

    void setFirst(char c) {
        if (length > 0) data[0] = c;
    }

    const char* c_str() const { return data; }
};

void demoAssignOperator() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.2：赋值运算符演示" << endl;
    cout << string(60, '=') << endl;

    MyString s1("Hello");
    MyString s2("World");

    cout << "\n--- 普通赋值：s2 = s1 ---" << endl;
    s2 = s1;  // 调用赋值运算符（s2 已存在）
    s1.show("s1 赋值后");
    s2.show("s2 赋值后");

    // 验证深拷贝：修改 s2 不影响 s1
    s2.setFirst('J');
    cout << "修改 s2 首字符为 'J'：" << endl;
    s1.show("s1（不受影响）");
    s2.show("s2（已修改）");
}

void demoSelfAssign() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.3：自赋值检查" << endl;
    cout << string(60, '=') << endl;

    MyString s("Test");
    s = s;  // 自赋值：应该安全，内容不变
    s.show("自赋值后（内容不变）");
}

void demoChainAssign() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.4：链式赋值" << endl;
    cout << string(60, '=') << endl;

    MyString a("AAA");
    MyString b("BBB");
    MyString c("CCC");

    cout << "执行 a = b = c（从右往左）：" << endl;
    // 等价于：a = (b = c)
    // 先执行 b = c（赋值运算符），返回 b 的引用
    // 再执行 a = b（赋值运算符）
    a = b = c;
    a.show("a");
    b.show("b");
    c.show("c");
}

// ---- 2.5 三法则（Rule of Three）完整示例 ----

class Buffer {
private:
    int* data;   // 指向堆上整数数组
    int  size;   // 数组大小

public:
    // 法则1：构造函数（不算三法则，但必须有）
    Buffer(int sz, int fillVal = 0) : size(sz) {
        data = new int[size];                          // 分配 size 个 int 的内存
        for (int i = 0; i < size; ++i) data[i] = fillVal;  // 初始化
        cout << "Buffer 构造，大小=" << size << endl;
    }

    // 法则1 of 3：析构函数（管理资源 → 触发三法则）
    ~Buffer() {
        cout << "Buffer 析构，大小=" << size << endl;
        delete[] data;  // 释放堆上数组
    }

    // 法则2 of 3：拷贝构造函数（深拷贝）
    Buffer(const Buffer& other) : size(other.size) {
        data = new int[size];                               // 分配新内存
        for (int i = 0; i < size; ++i) data[i] = other.data[i];  // 逐元素复制
        cout << "Buffer 拷贝构造，大小=" << size << endl;
    }

    // 法则3 of 3：赋值运算符（深拷贝 + 四步骤）
    Buffer& operator=(const Buffer& other) {
        if (this == &other) return *this;  // 步骤1：自赋值检查

        delete[] data;          // 步骤2：释放旧内存
        size = other.size;      // 步骤3：深拷贝
        data = new int[size];
        for (int i = 0; i < size; ++i) data[i] = other.data[i];

        cout << "Buffer 赋值，大小=" << size << endl;
        return *this;           // 步骤4：返回 *this
    }

    void fill(int val) {
        for (int i = 0; i < size; ++i) data[i] = val;
    }

    void show(const string& label) const {
        cout << label << ": [";
        for (int i = 0; i < size; ++i) {
            cout << data[i];
            if (i < size - 1) cout << ", ";
        }
        cout << "]" << endl;
    }
};

void demoRuleOfThree() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.5：三法则（Rule of Three）完整示例" << endl;
    cout << string(60, '=') << endl;

    Buffer b1(3, 1);      // 构造：[1, 1, 1]
    Buffer b2(b1);         // 拷贝构造：b2 = [1, 1, 1]（独立内存）
    Buffer b3(4, 0);       // 构造：[0, 0, 0, 0]
    b3 = b1;               // 赋值运算符：b3 = [1, 1, 1]

    b1.show("b1");
    b2.show("b2");
    b3.show("b3");

    // 修改 b2，验证 b1 不受影响
    b2.fill(99);
    cout << "b2.fill(99) 后：" << endl;
    b1.show("b1（不受影响）");
    b2.show("b2（已修改）");
}

// ============================================
// 🔍 第三部分：深入理解（可选）
// ============================================

/*
【深入理解：为什么返回引用（&）而不是值？】

情况1：返回值（MyString operator=）
    a = b = c
    → b = c 返回 b 的副本（拷贝构造）
    → a = (b的副本) 再拷贝一次
    效率低下，且 C++ 规范期望赋值运算符返回引用

情况2：返回引用（MyString& operator=）← 正确
    a = b = c
    → b = c 返回 b 本身的引用
    → a = b（引用） 直接操作 b，不产生额外拷贝
    符合语义，效率高

【深入理解：C++11 的五法则（Rule of Five）】

C++11 引入移动语义后，三法则扩展为五法则：
    1. 析构函数
    2. 拷贝构造函数
    3. 拷贝赋值运算符（operator=）
    4. 移动构造函数（ClassName(ClassName&& other) noexcept）
    5. 移动赋值运算符（ClassName& operator=(ClassName&& other) noexcept）

移动语义：将资源"转移"而不是"复制"，对大型数据效率更高
（详见后续章节）

【深入理解：自赋值检查的两种方式】

方式1：地址比较（传统方式）
    if (this == &other) return *this;

方式2：copy-and-swap（现代方式，更安全）
    MyString& operator=(MyString other) {  // 参数为值类型，触发拷贝构造
        swap(data,   other.data);          // 交换内容（other 析构时释放旧内容）
        swap(length, other.length);
        return *this;
    }
    // 优点：自动处理自赋值，异常安全（分配失败不影响原对象）
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonErrors() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第四部分：常见错误和陷阱" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
❌ 错误1：忘记自赋值检查（崩溃风险）

   MyString& operator=(const MyString& other) {
       delete[] data;           // ← 先释放自己的内存
       length = other.length;
       data = new char[length + 1];
       strcpy(data, other.data); // ← 若 other == *this，other.data 已被释放！
       return *this;
   }

   触发场景：
   MyString s("Hello");
   s = s;   // 自赋值：this == &other
   // delete[] data 释放了内存，strcpy 读已释放内存 → 未定义行为/崩溃

   ✅ 正确：第一行必须是自赋值检查
   if (this == &other) return *this;

------------------------------------------------------------

❌ 错误2：忘记释放旧资源（内存泄漏）

   MyString& operator=(const MyString& other) {
       // ← 没有 delete[] data！
       length = other.length;
       data = new char[length + 1];  // 覆盖了旧指针，旧内存永远丢失
       strcpy(data, other.data);
       return *this;
   }

   问题：每次赋值都泄漏一块内存，长时间运行程序内存耗尽

   ✅ 正确：自赋值检查之后，立即释放旧资源
   delete[] data;

------------------------------------------------------------

❌ 错误3：返回值而非引用（链式赋值失效）

   MyString operator=(const MyString& other) { ... }  // ← 返回值类型

   a = b = c;
   // b = c 返回 b 的副本，然后 a = (副本)，最终 a 赋值的是副本
   // 虽然功能上勉强正确，但每次赋值都多一次拷贝，且不符合 C++ 规范

   ✅ 正确：返回引用
   MyString& operator=(const MyString& other) { ...; return *this; }

------------------------------------------------------------

❌ 错误4：混淆拷贝构造和赋值运算符的调用时机

   MyString a("Hi");
   MyString b = a;    // ← ❌ 以为是赋值运算符
   // 实际：b 是新对象，这是"初始化" → 调用拷贝构造函数！

   MyString c("Bye");
   c = a;             // ← 这才是赋值运算符（c 已存在）

   ✅ 判断规则：
   // 新对象出现在声明时 → 拷贝构造函数
   // 已有对象用 = 赋新值 → 赋值运算符

------------------------------------------------------------

❌ 错误5：赋值运算符没有写，依赖编译器默认行为（含指针时危险）

   class Buf {
       int* data;
       int  size;
   public:
       Buf(int n) : size(n) { data = new int[n]; }
       ~Buf() { delete[] data; }
       Buf(const Buf& o) : size(o.size) { /* 深拷贝 */ }
       // ← 写了拷贝构造，却没写赋值运算符！
       // 编译器自动生成的赋值运算符仍然是浅拷贝！
   };
   Buf a(5), b(3);
   b = a;   // 调用编译器默认的浅拷贝赋值 → b.data = a.data（共享内存！）

   ✅ 三法则：定义了析构 → 拷贝构造和赋值运算符都要定义

------------------------------------------------------------

❌ 错误6：赋值运算符 size 与 data 不同步

   Buffer& operator=(const Buffer& other) {
       if (this == &other) return *this;
       delete[] data;
       data = new int[size];         // ← 用的是旧的 size！
       size = other.size;            // ← size 更新太晚了
       for (int i = 0; i < other.size; ++i) data[i] = other.data[i];
       return *this;
   }

   ✅ 正确：先更新 size，再分配内存
   size = other.size;         // 先更新 size
   data = new int[size];      // 再用新 size 分配内存
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
【赋值运算符完整模板】

ClassName& operator=(const ClassName& other) {
    // 步骤1：自赋值检查（必须第一步！）
    if (this == &other) return *this;

    // 步骤2：释放旧资源（防止内存泄漏）
    delete[] data;   // 或 delete data; 根据类型

    // 步骤3：深拷贝新内容
    length = other.length;
    data   = new char[length + 1];
    strcpy(data, other.data);

    // 步骤4：返回 *this（支持链式赋值）
    return *this;
}

────────────────────────────────────────────────────

【三法则速查表】

触发条件：类管理了资源（裸指针、文件句柄等）

需要定义的三个函数：
    ① ~ClassName()                              析构函数
    ② ClassName(const ClassName& other)         拷贝构造函数
    ③ ClassName& operator=(const ClassName& o)  赋值运算符

内存操作对比：
    拷贝构造：创建新对象，直接分配内存 + 复制
    赋值运算：覆盖已有对象，先释放旧内存，再分配 + 复制

────────────────────────────────────────────────────

【拷贝构造 vs 赋值运算符对比】

              拷贝构造函数              赋值运算符
触发时机    对象不存在，初始化时       对象已存在，被赋新值
声明语法    ClassName(const ClassName&)  ClassName& operator=(const ClassName&)
旧资源释放  不需要（对象刚创建）       需要（对象已有内容）
自赋值检查  不需要                     必须（this == &other）
返回值      无                         *this 引用
)";
}

// ============================================
// 第六部分：练习题
// ============================================

void showExercises() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第六部分：练习题（独立思考）" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
1. 判断题：以下每行代码调用的是拷贝构造函数还是赋值运算符？

   MyString a("Hello");     // ← ①
   MyString b = a;          // ← ②
   MyString c("World");     // ← ③
   c = a;                   // ← ④
   MyString d(b);           // ← ⑤
   a = a;                   // ← ⑥（特殊情况！）

   提示：看左侧对象在执行该行时是否已经存在

------------------------------------------------------------

2. 排错练习：找出以下赋值运算符的所有问题并修复

   class MyArr {
       int* data;
       int  size;
   public:
       MyArr(int n) : size(n) { data = new int[n](); }
       ~MyArr() { delete[] data; }

       MyArr& operator=(const MyArr& other) {
           // 没有自赋值检查
           length = other.size;
           data   = new int[size];   // size 还是旧值
           for (int i = 0; i < size; ++i)
               data[i] = other.data[i];
           return *this;
       }
   };

   提示：列出至少3个问题

------------------------------------------------------------

3. 实现练习：为以下 Stack 类实现完整的三法则

   class Stack {
       int* data;   // 动态数组
       int  top;    // 栈顶索引
       int  cap;    // 容量
   public:
       Stack(int capacity = 10) : cap(capacity), top(-1) {
           data = new int[cap];
       }
       // TODO：析构函数
       // TODO：拷贝构造函数
       // TODO：赋值运算符
       void push(int val) { if (top < cap-1) data[++top] = val; }
       int  peek() const  { return data[top]; }
       bool empty() const { return top == -1; }
   };

   验证：
   Stack s1; s1.push(1); s1.push(2);
   Stack s2 = s1;     // 拷贝构造
   Stack s3(5);
   s3 = s1;           // 赋值运算符
   // 修改 s1 后，s2 和 s3 不受影响

------------------------------------------------------------

4. 思考题：
   如果只定义了拷贝构造函数但没有定义赋值运算符，会发生什么？
   以下代码哪行会出问题？

   class MyStr {
       char* data;
   public:
       MyStr(const char* s) { data = new char[strlen(s)+1]; strcpy(data, s); }
       MyStr(const MyStr& o) { data = new char[strlen(o.data)+1]; strcpy(data, o.data); }
       ~MyStr() { delete[] data; }
       // 没有 operator=
   };
   MyStr a("Hi"), b("Bye");
   b = a;   // ← 这里会怎样？

   提示：编译器在没有 operator= 时会自动生成什么？
)";
}

// ============================================
// main 函数
// ============================================

int main() {
    cout << string(60, '=') << endl;
    cout << "   44_assignment_operator.cpp - 赋值运算符与三法则" << endl;
    cout << string(60, '=') << endl;

    showDifference();
    demoAssignOperator();
    demoSelfAssign();
    demoChainAssign();
    demoRuleOfThree();
    showCommonErrors();
    showFunctionCards();
    showExercises();

    cout << "\n" << string(60, '=') << endl;
    cout << "核心要点总结" << endl;
    cout << string(60, '=') << endl;
    cout << "1. 赋值运算符四步：自赋值检查→释放旧资源→深拷贝→return *this" << endl;
    cout << "2. 自赋值检查不可省略：if (this == &other) return *this;" << endl;
    cout << "3. 返回 *this 引用，支持 a = b = c 链式赋值" << endl;
    cout << "4. 三法则：析构/拷贝构造/赋值运算符，有一个就要有全部" << endl;
    cout << "5. b = a 时 b 已存在 → 赋值运算符；MyString b = a → 拷贝构造" << endl;

    return 0;
}
