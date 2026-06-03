// ============================================
// 79_move_semantics_deep.cpp - 超详细注释版
// 日期：2026-05-31（第31天）
// 难度：⭐⭐⭐ 高级
// 主题：深入移动语义 — 右值引用、五法则、noexcept
// ============================================

/*
【核心概念】

是什么？
- 移动语义（Move Semantics）：将资源的所有权从一个对象"转移"到另一个，
  而不是复制资源内容，避免不必要的深拷贝
- 右值引用（T&&）：C++11 引入，专门用于绑定右值（临时对象、std::move 的结果）

为什么需要？
- 深拷贝代价高：复制一个含 100MB 数据的 vector，需要分配内存、逐字节复制
- 移动代价极低：只需转移内部指针，O(1) 操作
- 临时对象大量产生：函数返回值、容器扩容，都是移动的理想场景

核心特点：
- std::move：将左值强制转换为右值引用（本身不移动任何东西）
- 移动构造/赋值：接受 T&& 参数，转移而非复制
- 五法则（Rule of Five）：管理资源的类应同时定义5个特殊成员函数
- noexcept：STL 只有在移动操作不抛异常时才使用移动（否则退化为拷贝）
*/

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <utility>
using namespace std;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

/*
【函数卡片：std::move】

语法：std::move(x)
作用：将 x 强制转为右值引用 T&&，允许移动构造/赋值
返回：T&&（右值引用，不分配内存、不复制数据）
注意：move 本身不移动任何东西！真正的移动发生在被调用的构造/赋值函数中

等价于：static_cast<T&&>(x)

使用场景：
  1. 明确不再需要某个对象时：string s2 = move(s1);
  2. 将左值传给接受右值引用的函数：vec.push_back(move(obj));
  3. 在移动构造/赋值内部清空源对象

------------------------------------------------------------

【概念：左值 vs 右值】

左值（lvalue）：有名字、有持久地址的表达式
  int x = 5;    // x 是左值
  string s;     // s 是左值

右值（rvalue）：临时的、无名的表达式
  5             // 字面量是右值
  x + y         // 表达式结果是右值
  string("hi")  // 临时对象是右值

左值引用：T&   — 只能绑定左值
右值引用：T&&  — 只能绑定右值（或 move 后的左值）

------------------------------------------------------------

【五法则（Rule of Five）】

如果类需要自定义以下任意一个，通常应该全部定义：
  1. 析构函数
  2. 拷贝构造函数
  3. 拷贝赋值运算符
  4. 移动构造函数（C++11）
  5. 移动赋值运算符（C++11）

原因：手动管理资源（指针、文件句柄等）时，默认行为（浅拷贝）不安全
*/

// ============================================
// 第二部分：基础操作实践
// ============================================

// ---- 2.1 完整实现五法则的 Buffer 类 ----

class Buffer {
private:
    char*  data;  // 堆上的字节数组
    size_t size;  // 字节数

public:
    // 1. 构造函数：分配堆内存
    Buffer(size_t s, char fill = 0) : size(s) {
        data = new char[size];
        memset(data, fill, size);
        cout << "  [构造] Buffer(" << size << " bytes)" << endl;
    }

    // 2. 析构函数：释放堆内存
    ~Buffer() {
        delete[] data;
        cout << "  [析构] Buffer(" << size << " bytes)" << endl;
    }

    // 3. 拷贝构造函数（深拷贝）
    // 参数：const Buffer& — 绑定左值（不修改源对象）
    Buffer(const Buffer& other) : size(other.size) {
        data = new char[size];
        memcpy(data, other.data, size);  // 深拷贝：复制所有字节
        cout << "  [拷贝构造] Buffer(" << size << " bytes) — 深拷贝" << endl;
    }

    // 4. 拷贝赋值运算符（深拷贝）
    Buffer& operator=(const Buffer& other) {
        if (this == &other) return *this;  // 自赋值检查
        delete[] data;                      // 释放旧内存
        size = other.size;
        data = new char[size];
        memcpy(data, other.data, size);     // 深拷贝
        cout << "  [拷贝赋值] Buffer(" << size << " bytes) — 深拷贝" << endl;
        return *this;
    }

    // 5. 移动构造函数（转移所有权）
    // 参数：Buffer&& — 右值引用，绑定临时对象或 std::move 的结果
    // noexcept：保证不抛异常，STL 才会在 vector 扩容时使用移动而非拷贝
    Buffer(Buffer&& other) noexcept
        : data(other.data),   // 直接接管 other 的指针
          size(other.size) {
        other.data = nullptr; // 关键：将源对象的指针置空，防止双重释放
        other.size = 0;
        cout << "  [移动构造] Buffer(" << size << " bytes) — 零拷贝" << endl;
    }

    // 6. 移动赋值运算符（转移所有权）
    Buffer& operator=(Buffer&& other) noexcept {
        if (this == &other) return *this;
        delete[] data;           // 释放当前持有的内存
        data = other.data;       // 接管 other 的指针
        size = other.size;
        other.data = nullptr;    // 源对象置空
        other.size = 0;
        cout << "  [移动赋值] Buffer(" << size << " bytes) — 零拷贝" << endl;
        return *this;
    }

    size_t getSize()  const { return size; }
    bool   isValid()  const { return data != nullptr; }
    char*  getData()  const { return data; }
};

void demoCopyVsMove() {
    cout << string(60, '=') << endl;
    cout << "第二部分 2.1：拷贝 vs 移动" << endl;
    cout << string(60, '=') << endl;

    cout << "\n--- 拷贝构造（深拷贝）---" << endl;
    Buffer b1(1024, 'A');
    Buffer b2 = b1;  // 调用拷贝构造：复制 1024 字节
    cout << "  b1 有效：" << b1.isValid() << "，b2 有效：" << b2.isValid() << endl;

    cout << "\n--- 移动构造（零拷贝）---" << endl;
    Buffer b3 = move(b1);  // 调用移动构造：只转移指针，O(1)
    cout << "  b1 有效（移走后）：" << b1.isValid()
         << "，b3 有效：" << b3.isValid() << endl;

    cout << "\n--- 临时对象自动移动 ---" << endl;
    Buffer b4 = Buffer(512, 'B');  // Buffer(512) 是临时对象（右值），自动移动
    cout << "  b4 大小：" << b4.getSize() << endl;
}

// ---- 2.2 std::move 的本质 ----

void demoStdMove() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.2：std::move 的本质" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
std::move 只是类型转换（static_cast<T&&>），本身不移动任何东西。
真正的移动发生在被调用的移动构造/移动赋值函数中。

等价代码：
  Buffer b2 = move(b1);
  等价于：
  Buffer b2 = static_cast<Buffer&&>(b1);

规则：
  调用 move 后，b1 进入"有效但未指定"状态：
  - 可以安全析构（析构函数会检查 data != nullptr）
  - 可以重新赋值
  - 不应该再读取其内容
)" << endl;

    Buffer src(256, 'X');
    cout << "move 前 src 有效：" << src.isValid() << endl;

    Buffer dst = move(src);  // src 的指针转移给 dst
    cout << "move 后 src 有效：" << src.isValid()
         << "（nullptr，已移走）" << endl;
    cout << "dst 有效：" << dst.isValid() << endl;

    // 重新赋值 src（合法）
    src = Buffer(128, 'Y');
    cout << "重新赋值后 src 有效：" << src.isValid() << endl;
}

// ---- 2.3 noexcept 与 vector 扩容 ----

void demoNoexcept() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.3：noexcept 与 vector 扩容" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
STL 规则：vector 扩容时，只有移动构造标记了 noexcept，才会用移动；
          否则退化为拷贝（保证强异常安全性）。

本例 Buffer 的移动构造标记了 noexcept：
  Buffer(Buffer&& other) noexcept { ... }
所以 vector 扩容时会使用移动构造。
)" << endl;

    vector<Buffer> vec;
    vec.reserve(1);  // 初始容量 1

    cout << "\n向 vector 添加元素（可能触发扩容）：" << endl;
    vec.push_back(Buffer(64, '1'));   // 临时对象 → 移动
    vec.push_back(Buffer(128, '2'));  // 超出容量 → 扩容，旧元素被移动到新内存
    vec.push_back(Buffer(256, '3'));  // 再次扩容
    cout << "vector 大小：" << vec.size() << endl;
}

// ---- 2.4 移动赋值 ----

void demoMoveAssign() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.4：移动赋值" << endl;
    cout << string(60, '=') << endl;

    Buffer b1(512, 'P');
    Buffer b2(256, 'Q');

    cout << "\n--- 拷贝赋值 ---" << endl;
    b2 = b1;  // 拷贝赋值：释放 b2 旧内存，深拷贝 b1

    cout << "\n--- 移动赋值 ---" << endl;
    Buffer b3(128, 'R');
    b3 = move(b1);  // 移动赋值：释放 b3 旧内存，接管 b1 的指针

    cout << "b1 有效（移走后）：" << b1.isValid() << endl;
    cout << "b3 大小：" << b3.getSize() << endl;
}

// ============================================
// 第三部分：深入理解（可选）
// ============================================

/*
🔍 深入理解：移动后对象的状态

C++ 标准要求：移动后的对象处于"有效但未指定"（valid but unspecified）状态：
  - 有效：析构函数可以安全调用，不会崩溃
  - 未指定：值不确定，不能依赖其内容

为什么要把 other.data 置为 nullptr？
  Buffer(Buffer&& other) noexcept {
      data = other.data;
      other.data = nullptr;  // 关键！
  }

  如果不置 nullptr：
  - other 析构时也会 delete data（此时 this->data 和 other.data 指向同一块内存）
  - 双重释放（double free）→ 崩溃

  置为 nullptr 后：
  - other.data == nullptr
  - other 析构时 delete nullptr → 安全（delete nullptr 是合法的空操作）

🔍 深入理解：为什么移动比拷贝快？

  拷贝：分配新内存 + 复制 N 字节 = O(N)
  移动：复制一个指针 + 一个整数 = O(1)

  对于 string、vector、Buffer 等持有堆内存的对象：
  100MB 数据：拷贝需要 ~100ms，移动需要 ~几纳秒
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonErrors() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第四部分：常见错误和陷阱" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
❌ 错误1：移动后继续使用源对象的值

   Buffer b1(1024);
   Buffer b2 = move(b1);
   b1.getData()[0] = 'A';   // ❌ b1.data 是 nullptr，解引用崩溃！

   ✅ 规则：move 后不再使用源对象的内容，只能析构或重新赋值
   if (b1.isValid()) b1.getData()[0] = 'A';   // 安全（但 b1 已是 nullptr，不进入）

------------------------------------------------------------

❌ 错误2：移动构造忘记清空源对象的指针（双重释放）

   Buffer(Buffer&& other) noexcept {
       data = other.data;
       size = other.size;
       // ❌ 忘记：other.data = nullptr;
   }
   // other 析构时：delete other.data（与 data 是同一地址）
   // this 析构时：delete data（同一地址再次 delete）
   // 双重释放 → 崩溃或内存损坏

   ✅ 移动构造/赋值后必须清空源对象：
   other.data = nullptr;
   other.size = 0;

------------------------------------------------------------

❌ 错误3：移动构造没有标记 noexcept

   Buffer(Buffer&& other) { ... }   // ❌ 没有 noexcept

   // vector 扩容时，不确定移动会不会抛异常
   // STL 保守地使用拷贝而非移动（强异常安全性要求）
   // 结果：性能下降，大对象扩容时有大量拷贝

   ✅ 移动构造/移动赋值标记 noexcept：
   Buffer(Buffer&& other) noexcept { ... }   // STL 会优先使用移动

------------------------------------------------------------

❌ 错误4：对 const 对象调用 move（move 失效）

   const Buffer cb(1024);
   Buffer b2 = move(cb);   // move 返回 const Buffer&&
   // 但移动构造需要 Buffer&&（非 const）
   // 没有匹配的移动构造 → 退化为拷贝构造（const Buffer& 可以绑定 const Buffer&&）

   ✅ 规则：只对非 const 对象 move，对 const 对象 move 会退化为拷贝

------------------------------------------------------------

❌ 错误5：在移动赋值中忘记自赋值检查

   Buffer& operator=(Buffer&& other) noexcept {
       delete[] data;       // ❌ 如果 this == &other，释放了自己的内存！
       data = other.data;   // data 已被 delete，other.data 是野指针
       other.data = nullptr;
       return *this;
   }

   Buffer b(1024);
   b = move(b);   // 自移动赋值，触发上述 bug

   ✅ 先检查自赋值：
   if (this == &other) return *this;

------------------------------------------------------------

❌ 错误6：五法则不完整（有析构但没有移动构造）

   class MyClass {
       int* data;
   public:
       MyClass(int n) { data = new int[n]; }
       ~MyClass() { delete[] data; }
       // ❌ 没有移动构造/赋值
       // 添加到 vector 时：只能拷贝（编译器生成的默认拷贝是浅拷贝！）
       // 浅拷贝 + double delete → 崩溃
   };

   ✅ 有析构函数 → 必须同时实现五法则全部5个：
   析构 + 拷贝构造 + 拷贝赋值 + 移动构造 + 移动赋值

------------------------------------------------------------

❌ 错误7：return 时显式 move 反而阻止 RVO

   Buffer make() {
       Buffer b(1024);
       return move(b);   // ❌ 阻止了编译器的 NRVO 优化，反而多一次移动
   }

   ✅ 直接 return，让编译器决定（优先 NRVO，其次隐式移动）：
   Buffer make() {
       Buffer b(1024);
       return b;   // 编译器自动 NRVO 或移动，不需要显式 move
   }
)" << endl;
}

// ============================================
// 第五部分：函数卡片速查
// ============================================

void showFunctionCards() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第五部分：函数卡片速查" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【五法则标准模板】

class MyClass {
    T* ptr;   // 管理的资源
public:
    MyClass(size_t n) : ptr(new T[n]) {}           // 构造
    ~MyClass() { delete[] ptr; }                    // 析构

    // 拷贝（深拷贝）
    MyClass(const MyClass& o) : ptr(new T[...]) { memcpy(...); }
    MyClass& operator=(const MyClass& o) {
        if (this == &o) return *this;
        delete[] ptr; ptr = new T[...]; memcpy(...); return *this;
    }

    // 移动（转移所有权）
    MyClass(MyClass&& o) noexcept : ptr(o.ptr) { o.ptr = nullptr; }
    MyClass& operator=(MyClass&& o) noexcept {
        if (this == &o) return *this;
        delete[] ptr; ptr = o.ptr; o.ptr = nullptr; return *this;
    }
};

────────────────────────────────────────────────────

【std::move 使用场景速查】

向容器添加左值：  vec.push_back(move(obj));
转移函数参数：    process(move(data));
转移 unique_ptr：auto q = move(p);
不用 move 的场景：return 局部变量（让 NRVO 工作）

【判断什么时候用 move】

✅ 用 move：之后不再需要源对象的值
❌ 不用 move：之后还要用源对象 / 源对象是 const / return 局部变量
)" << endl;
}

// ============================================
// 第六部分：练习题
// ============================================

void showExercises() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第六部分：练习题（独立思考）" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
1. 实现练习：为 StringArray 类实现五法则

   class StringArray {
       string* arr;    // 堆上字符串数组
       size_t  count;  // 元素数量
   public:
       StringArray(size_t n);      // 分配 n 个 string
       ~StringArray();             // TODO
       StringArray(const StringArray&);         // TODO：深拷贝
       StringArray& operator=(const StringArray&); // TODO
       StringArray(StringArray&&) noexcept;     // TODO：转移
       StringArray& operator=(StringArray&&) noexcept; // TODO
   };

   验证：
   StringArray a(3);
   StringArray b = a;            // 拷贝：b 有独立副本
   StringArray c = move(a);      // 移动：a.arr 变 nullptr，c 接管
   cout << (a.arr == nullptr);   // true

------------------------------------------------------------

2. 性能测试：测量拷贝 vs 移动的时间差

   用 chrono::high_resolution_clock 测量：
   - 创建包含 1000 个 Buffer(1MB) 的 vector（push_back拷贝版 vs push_back移动版）
   - 对比时间差，体验移动语义的性能优势

   提示：
   auto start = chrono::high_resolution_clock::now();
   // ... 操作 ...
   auto us = chrono::duration_cast<chrono::microseconds>(
       chrono::high_resolution_clock::now() - start).count();

------------------------------------------------------------

3. 排错题：找出以下代码的所有问题

   class Resource {
       int* data;
       int  size;
   public:
       Resource(int n) : size(n) { data = new int[n]; }
       ~Resource() { delete data; }                // 问题①
       Resource(Resource&& o) {                    // 问题②
           data = o.data; size = o.size;           // 问题③
       }
       Resource& operator=(Resource&& o) noexcept {
           data = o.data; size = o.size;            // 问题④
           o.data = nullptr;
           return *this;
       }
   };

   提示：
   ① delete 应为 delete[]
   ② 移动构造没有 noexcept
   ③ 没有清空 o.data → 双重释放
   ④ 没有先 delete[] data 和自赋值检查
)" << endl;
}

// ============================================
// main 函数
// ============================================

int main() {
    cout << string(60, '=') << endl;
    cout << " 79_move_semantics_deep.cpp - 深入移动语义" << endl;
    cout << string(60, '=') << endl;

    demoCopyVsMove();
    demoStdMove();
    demoNoexcept();
    demoMoveAssign();
    showCommonErrors();
    showFunctionCards();
    showExercises();

    cout << "\n" << string(60, '=') << endl;
    cout << "核心要点总结" << endl;
    cout << string(60, '=') << endl;
    cout << "1. std::move 只是类型转换，真正移动发生在移动构造/赋值中" << endl;
    cout << "2. 移动后必须清空源指针（other.ptr = nullptr），防止双重释放" << endl;
    cout << "3. 移动构造/赋值标记 noexcept，STL 才会使用移动" << endl;
    cout << "4. 有析构函数 → 实现五法则全部5个" << endl;
    cout << "5. return 局部变量不要显式 move，让 NRVO 工作" << endl;

    return 0;
}
