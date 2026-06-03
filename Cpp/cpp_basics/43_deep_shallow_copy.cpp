// ============================================
// 43_deep_shallow_copy.cpp - 超详细注释版
// 日期：第13天
// 难度：⭐⭐ 中级
// 主题：深拷贝与浅拷贝
// ============================================

/*
【核心概念】

是什么？
- 浅拷贝（Shallow Copy）：逐字节复制，指针成员只复制地址
- 深拷贝（Deep Copy）：为指针成员分配新内存，复制指向的内容

为什么需要深拷贝？
- 浅拷贝时两个对象的指针指向同一块堆内存
- 一个对象析构释放内存后，另一个对象析构会再次释放 → double free → 程序崩溃
- 深拷贝让每个对象拥有自己独立的内存，互不干扰

核心特点：
- 编译器默认生成浅拷贝（仅逐成员赋值）
- 含有裸指针成员 → 必须自定义深拷贝
- 使用 std::string、std::vector 等标准库类型可自动获得深拷贝
*/

#include <iostream>
#include <cstring>
#include <string>
#include <vector>
using namespace std;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

/*
【浅拷贝的内存图】

    s1.data ──────────────────┐
                              ↓
                        [H][e][l][l][o][\0]  ← 堆上同一块内存
                              ↑
    s2.data ──────────────────┘

    问题：s2 析构时 delete[] s2.data → 内存释放
          s1 析构时 delete[] s1.data → 再次释放同一地址 → double free！

【深拷贝的内存图】

    s1.data ──→ [H][e][l][l][o][\0]  ← 堆上独立内存1
    s2.data ──→ [H][e][l][l][o][\0]  ← 堆上独立内存2

    安全：每个对象析构时各自释放自己的内存，互不影响
*/

// ============================================
// 第二部分：基础操作实践
// ============================================

// ---- 2.1 浅拷贝问题演示（不直接触发崩溃，用地址对比说明危险） ----

class ShallowString {
public:
    char* data;    // 公开，方便演示
    int   length;

    // 构造函数：在堆上分配内存存储字符串
    ShallowString(const char* str) {
        length = strlen(str);              // strlen 不含 '\0'
        data   = new char[length + 1];     // +1 为 '\0' 预留
        strcpy(data, str);                 // 复制字符串内容
        cout << "ShallowString 构造 \"" << data
             << "\" addr=" << (void*)data << endl;
    }

    // ⚠️ 没有自定义拷贝构造函数
    // 编译器生成的版本相当于：
    //   data   = other.data;    ← 只复制指针地址！
    //   length = other.length;

    ~ShallowString() {
        if (data) {
            cout << "ShallowString 析构 \"" << data
                 << "\" addr=" << (void*)data << endl;
            delete[] data;
            data = nullptr;
        }
    }
};

void demoShallowProblem() {
    cout << string(60, '=') << endl;
    cout << "第二部分 2.1：浅拷贝的危险（地址对比演示）" << endl;
    cout << string(60, '=') << endl;

    // 演示浅拷贝时两个指针指向同一地址
    // 注意：直接运行完整浅拷贝会 double free 崩溃
    // 这里通过手动操作来展示地址相同的现象
    ShallowString s1("Hello");

    // 模拟编译器的浅拷贝行为：
    ShallowString s2("World");  // 先构造一个对象
    delete[] s2.data;           // 释放 s2 自己的内存
    s2.data   = s1.data;        // ← 直接复制指针地址（浅拷贝的本质）
    s2.length = s1.length;

    cout << "\n浅拷贝后的地址对比：" << endl;
    cout << "s1.data 地址: " << (void*)s1.data << endl;
    cout << "s2.data 地址: " << (void*)s2.data << endl;
    cout << "地址相同: " << (s1.data == s2.data ? "是（危险！double free隐患）" : "否") << endl;

    // 防止 double free：将 s2.data 置 nullptr，让析构时跳过释放
    s2.data = nullptr;
    cout << "（已将 s2.data 设为 nullptr 避免演示时崩溃）" << endl;
}

// ---- 2.2 深拷贝的正确实现 ----

class DeepString {
private:
    char* data;    // 指向堆内存的指针
    int   length;  // 字符串长度

public:
    // 构造函数
    // 参数：str - C 风格字符串，用于初始化
    DeepString(const char* str = "") {
        length = strlen(str);
        data   = new char[length + 1];
        strcpy(data, str);
        cout << "DeepString 构造 \"" << data
             << "\" addr=" << (void*)data << endl;
    }

    // 深拷贝构造函数
    // 参数：other (const DeepString&) - 被拷贝的源对象
    // 核心：new 分配新内存，再复制内容，使两个对象内存完全独立
    DeepString(const DeepString& other) {
        length = other.length;           // 复制长度值
        data   = new char[length + 1];   // ← 关键：分配全新的内存，不复制地址
        strcpy(data, other.data);        // 将内容复制到新内存
        cout << "深拷贝构造 \"" << data
             << "\" addr=" << (void*)data
             << " (源addr=" << (void*)other.data << ")" << endl;
    }

    // 析构函数：释放堆内存
    ~DeepString() {
        cout << "DeepString 析构 \"" << data
             << "\" addr=" << (void*)data << endl;
        delete[] data;
    }

    void show(const string& label = "") const {
        if (!label.empty()) cout << label << ": ";
        cout << "\"" << data << "\" addr=" << (void*)data << endl;
    }

    // 修改首字符，用于验证内存独立性
    void setFirst(char c) {
        if (length > 0) data[0] = c;
    }
};

void demoDeepCopy() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.2：深拷贝的正确实现" << endl;
    cout << string(60, '=') << endl;

    DeepString s1("Hello");
    DeepString s2 = s1;  // 调用深拷贝构造函数

    cout << "\n深拷贝后（两个对象地址不同）：" << endl;
    s1.show("s1");
    s2.show("s2");

    // 修改 s2 不影响 s1，证明内存独立
    s2.setFirst('J');
    cout << "\n修改 s2 首字符为 'J' 后（s1 不受影响）：" << endl;
    s1.show("s1");
    s2.show("s2");
    cout << "（两个对象各自析构，各自释放自己的内存，不会 double free）" << endl;
}

// ---- 2.3 何时需要深拷贝 ----

// 情况A：只有值类型成员 → 不需要深拷贝，浅拷贝安全
class Rectangle {
public:
    double width;   // double 是值类型，拷贝时生成独立副本
    double height;  // double 是值类型

    Rectangle(double w, double h) : width(w), height(h) {}
    double area() const { return width * height; }
};

// 情况B：使用标准库容器成员 → 不需要深拷贝，标准库已内置深拷贝
class StudentRecord {
public:
    string name;           // std::string 内部有自己的深拷贝
    vector<int> scores;    // std::vector 内部有自己的深拷贝

    StudentRecord(const string& n) : name(n) {}
    void addScore(int s) { scores.push_back(s); }
    void show() const {
        cout << name << " 分数：";
        for (int s : scores) cout << s << " ";
        cout << endl;
    }
};

// 情况C：含有裸指针成员 → 必须自定义深拷贝（见 DeepString）
class RawPtrExample {
    int* data;  // 裸指针 → 触发三法则 → 必须自定义深拷贝构造函数
    int  size;
public:
    RawPtrExample(int n) : size(n) {
        data = new int[size]();  // () 表示零初始化
    }
    // ← 如果这里不定义拷贝构造函数，使用默认浅拷贝会产生 double free
    RawPtrExample(const RawPtrExample& other) : size(other.size) {
        data = new int[size];                          // 分配新内存
        for (int i = 0; i < size; ++i)
            data[i] = other.data[i];                   // 逐元素复制
        cout << "RawPtrExample 深拷贝，size=" << size << endl;
    }
    ~RawPtrExample() { delete[] data; }
    void set(int idx, int val) { if (idx < size) data[idx] = val; }
    void show(const string& lbl) const {
        cout << lbl << ": [";
        for (int i = 0; i < size; ++i) {
            cout << data[i];
            if (i < size-1) cout << ",";
        }
        cout << "]" << endl;
    }
};

void demoWhenNeedDeep() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.3：何时需要深拷贝" << endl;
    cout << string(60, '=') << endl;

    cout << "\n[情况A] 只有值类型成员 → 浅拷贝安全：" << endl;
    Rectangle r1(3.0, 4.0);
    Rectangle r2 = r1;  // 浅拷贝完全安全，r2.width 和 r1.width 是独立的 double
    r2.width = 10.0;
    cout << "r1 面积: " << r1.area() << "（r2 修改后不受影响）" << endl;
    cout << "r2 面积: " << r2.area() << endl;

    cout << "\n[情况B] 标准库成员已内置深拷贝：" << endl;
    StudentRecord stu1("Alice");
    stu1.addScore(90); stu1.addScore(85);
    StudentRecord stu2 = stu1;   // std::string 和 std::vector 自动深拷贝
    stu2.name = "Bob";
    stu2.scores[0] = 70;
    stu1.show();  // Alice 的数据不受影响
    stu2.show();

    cout << "\n[情况C] 裸指针成员 → 必须自定义深拷贝：" << endl;
    RawPtrExample a(3);
    a.set(0, 10); a.set(1, 20); a.set(2, 30);
    RawPtrExample b = a;  // 触发自定义深拷贝构造函数
    b.set(0, 999);         // 修改 b 的第一个元素
    a.show("a（b修改后）"); // a 不受影响
    b.show("b（修改后）");
}

// ============================================
// 🔍 第三部分：深入理解（可选）
// ============================================

/*
【深入理解：std::string 内部如何实现深拷贝？】

std::string 内部大致维护：
    char*  _data;   // 指向堆内存
    size_t _size;   // 字符串长度
    size_t _cap;    // 容量

它的拷贝构造函数大致：
    string(const string& other) {
        _size = other._size;
        _cap  = other._cap;
        _data = new char[_cap + 1];     // 分配新内存
        memcpy(_data, other._data, _size + 1);  // 复制内容
    }

这就是为什么使用 std::string 时不需要自己写深拷贝。
推论：使用标准库容器替代裸指针，可以避免手写深拷贝的大量工作。

【深入理解：memcpy vs strcpy】

strcpy(dst, src)：
    - 复制字符串，直到遇到 '\0' 为止
    - 适用于 C 风格字符串

memcpy(dst, src, n)：
    - 复制 n 个字节，不关心内容
    - 适用于任意二进制数据（包括含 '\0' 的数据）
    - 通常比 strcpy 稍快（底层使用 SIMD 优化）

建议：复制 C 风格字符串用 strcpy，复制任意内存用 memcpy。
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonErrors() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第四部分：常见错误和陷阱" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
❌ 错误1：含裸指针时不写拷贝构造函数（double free崩溃）

   class MyBuf {
       int* data;
   public:
       MyBuf(int n) { data = new int[n]; }
       ~MyBuf() { delete[] data; }
       // ← 没有拷贝构造函数！编译器自动生成浅拷贝
   };
   MyBuf a(5);
   MyBuf b = a;  // b.data == a.data（同一块内存）
   // 析构时：b 先 delete，a 再 delete 同一地址 → 崩溃！

   ✅ 正确：必须自定义深拷贝构造函数
   MyBuf(const MyBuf& other) {
       data = new int[size];       // 分配新内存
       memcpy(data, other.data, size * sizeof(int));  // 复制内容
   }

   记忆：有 delete 的类 → 一定要写拷贝构造函数

------------------------------------------------------------

❌ 错误2：深拷贝时忘记同步所有指针成员

   class TwoPtr {
       int* p1;
       int* p2;  // ← 第二个指针！
   public:
       TwoPtr(const TwoPtr& other) {
           p1 = new int(*other.p1);  // ✅ 深拷贝了 p1
           // p2 没有处理！                ← 浅拷贝 p2，仍然共享内存
       }
   };

   ✅ 正确：所有裸指针成员都要深拷贝
   TwoPtr(const TwoPtr& other) {
       p1 = new int(*other.p1);  // ✅
       p2 = new int(*other.p2);  // ✅
   }

------------------------------------------------------------

❌ 错误3：深拷贝时错误地使用 memcpy 复制含指针的结构

   struct Node { int val; Node* next; };
   class LinkedList {
       Node* head;
   public:
       // 错误的深拷贝：
       LinkedList(const LinkedList& other) {
           head = new Node();
           memcpy(head, other.head, sizeof(Node));
           // ← 虽然 head 是新指针，但 head->next 仍指向原链表的节点！
       }
   };

   ✅ 正确：递归/循环复制每个节点
   LinkedList(const LinkedList& other) {
       // 需要遍历链表，为每个节点 new 一个新节点
   }

   记忆：memcpy 只复制字节，不理解指针语义，不能用于含指针的深拷贝

------------------------------------------------------------

❌ 错误4：误以为 = 赋值一定调用赋值运算符

   MyString a("Hi");
   MyString b = a;   // ← ❌ 以为这是赋值运算符（operator=）

   ✅ 分析：
   // b 是新创建的对象（b 之前不存在）
   // 所以 MyString b = a; 是"用 a 初始化 b" → 拷贝构造函数！
   // 只有 b 已经存在时，b = a; 才是赋值运算符

------------------------------------------------------------

❌ 错误5：用 delete 而不是 delete[] 释放数组内存

   data = new char[length + 1];   // 分配数组
   delete data;     // ❌ 应该用 delete[]，否则行为未定义（内存泄漏/崩溃）

   ✅ 正确：
   delete[] data;   // new[] 分配的内存必须用 delete[] 释放

   记忆：new ↔ delete，new[] ↔ delete[]（配对使用）

------------------------------------------------------------

❌ 错误6：深拷贝后修改拷贝对象影响了原对象

   class Arr {
       int* data;
       int  size;
   public:
       Arr(const Arr& other) {
           size = other.size;
           data = other.data;  // ❌ 浅拷贝！
       }
   };

   Arr a = {...};
   Arr b = a;      // 浅拷贝，b.data == a.data
   b.data[0] = 99; // 修改 b，a.data[0] 也变成 99！

   ✅ 正确：分配新内存
   data = new int[size];
   for (int i = 0; i < size; i++) data[i] = other.data[i];
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
【strlen()】

头文件：<cstring>
语法：size_t strlen(const char* str)
参数：
    - str (const char*): C 风格字符串，以 '\0' 结尾
返回值：字符串长度（不含 '\0'）
示例：
    strlen("Hello") → 5
    strlen("")      → 0

────────────────────────────────────────────────────

【strcpy()】

头文件：<cstring>
语法：char* strcpy(char* dst, const char* src)
参数：
    - dst (char*): 目标缓冲区（必须已分配足够内存）
    - src (const char*): 源字符串
返回值：dst 指针（通常忽略）
注意：dst 必须有足够空间，否则缓冲区溢出
示例：
    char buf[10];
    strcpy(buf, "Hello");  // buf = "Hello"

────────────────────────────────────────────────────

【深拷贝构造函数标准模式】

ClassName(const ClassName& other) {
    // 步骤1：复制所有值类型成员
    length = other.length;

    // 步骤2：为指针成员分配新内存
    data = new char[length + 1];   // 或 new int[size] 等

    // 步骤3：复制内容到新内存
    strcpy(data, other.data);      // 字符串
    // 或：memcpy(data, other.data, size * sizeof(int));  // 整数数组
    // 或：for (int i=0; i<size; i++) data[i] = other.data[i];
}

────────────────────────────────────────────────────

【判断是否需要自定义深拷贝】

含有以下成员 → 需要深拷贝：
    ✅ int* / char* / double* 等裸指针
    ✅ 文件句柄（FILE*、fstream）
    ✅ 网络连接、数据库连接

只含以下成员 → 不需要（已自动深拷贝）：
    ✅ int、double、char 等基本类型
    ✅ std::string
    ✅ std::vector、std::list、std::map 等标准容器
    ✅ std::shared_ptr、std::unique_ptr 等智能指针
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
1. 判断题：以下哪个类需要自定义深拷贝构造函数？

   class A { int x, y; };
   class B { std::string name; int age; };
   class C { char* buf; int size; ~C(){ delete[] buf; } };
   class D { std::vector<int> data; };
   class E { int* arr; int len; };

   提示：看是否含有裸指针（且由该类负责释放）

------------------------------------------------------------

2. 实现练习：完善下面的 Image 类，使其支持安全的深拷贝

   class Image {
       unsigned char* pixels;  // 像素数据（堆上）
       int width, height;
   public:
       Image(int w, int h) : width(w), height(h) {
           pixels = new unsigned char[w * h]();
       }
       ~Image() { delete[] pixels; }

       // TODO：实现深拷贝构造函数
       // Image(const Image& other) { ... }

       void setPixel(int x, int y, unsigned char val) {
           pixels[y * width + x] = val;
       }
       unsigned char getPixel(int x, int y) const {
           return pixels[y * width + x];
       }
   };

   验证：
   Image img1(10, 10);
   img1.setPixel(0, 0, 255);
   Image img2 = img1;        // 深拷贝
   img2.setPixel(0, 0, 0);   // 修改 img2
   // img1.getPixel(0,0) 应该仍然是 255

------------------------------------------------------------

3. 排错练习：以下代码有什么问题？如何修复？

   class Matrix {
       double* data;
       int rows, cols;
   public:
       Matrix(int r, int c) : rows(r), cols(c) {
           data = new double[rows * cols]();
       }
       ~Matrix() { delete[] data; }

       // 拷贝构造函数：
       Matrix(const Matrix& other) : rows(other.rows), cols(other.cols) {
           data = other.data;   // ← 这行有问题吗？
       }
   };

   提示：追踪 data 指针的地址，分析析构时会发生什么

------------------------------------------------------------

4. 思考题：
   为什么使用 std::vector<int> 而不是 int* + size 可以避免很多问题？
   std::vector 在拷贝时做了什么操作？
   这说明了什么设计原则？（提示：RAII）
)";
}

// ============================================
// main 函数
// ============================================

int main() {
    cout << string(60, '=') << endl;
    cout << "      43_deep_shallow_copy.cpp - 深拷贝与浅拷贝" << endl;
    cout << string(60, '=') << endl;

    demoShallowProblem();
    demoDeepCopy();
    demoWhenNeedDeep();
    showCommonErrors();
    showFunctionCards();
    showExercises();

    cout << "\n" << string(60, '=') << endl;
    cout << "核心要点总结" << endl;
    cout << string(60, '=') << endl;
    cout << "1. 浅拷贝：复制指针地址 → 共享内存 → double free 崩溃" << endl;
    cout << "2. 深拷贝：new 新内存 + 复制内容 → 内存独立 → 安全" << endl;
    cout << "3. 含裸指针成员 → 必须自定义深拷贝构造函数" << endl;
    cout << "4. 使用 std::string/vector 等标准库类型 → 自动深拷贝" << endl;
    cout << "5. 三法则：有析构 → 也要有拷贝构造 + 赋值运算符" << endl;

    return 0;
}
