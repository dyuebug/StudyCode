// ============================================
// 40_destructor_basics.cpp - 超详细注释版
// 日期：2026-03-18（第12天）
// 难度：⭐⭐ 中级
// 主题：析构函数基础 — RAII、生命周期、资源释放
// ============================================

/*
【核心概念】

是什么？
- 析构函数：对象销毁时自动调用的特殊成员函数，负责释放资源
- 函数名是类名前加 ~，无返回类型，无参数，不能重载

为什么需要？
- 自动释放动态内存（防止内存泄漏）
- 关闭文件、释放网络连接等资源
- RAII 的实现核心：构造时获取资源，析构时释放

核心特点：
- 栈对象：离开作用域时自动析构
- 堆对象：delete 时析构（必须手动）
- 析构顺序与构造顺序相反（栈：后进先出）
*/

#include <iostream>
#include <string>
using namespace std;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

/*
【函数卡片：析构函数语法】

声明：~ClassName();
定义：ClassName::~ClassName() { /* 释放资源 */ }

规则：
  - 无参数，不能重载（一个类只有一个析构函数）
  - 没有返回类型（连 void 也不写）
  - 如果没有定义，编译器生成空析构函数（不做任何事）
  - 管理堆内存时必须手动定义（否则内存泄漏）

------------------------------------------------------------

【对象生命周期】

栈对象：
  { Foo f; }    // 进入 {} 时构造，离开 {} 时析构

堆对象：
  Foo* p = new Foo();  // new 时构造
  delete p;            // delete 时析构（必须手动！）

析构顺序（栈：后进先出）：
  Foo a, b, c;   // 构造：a → b → c
                 // 析构：c → b → a
*/

// ============================================
// 第二部分：基础操作实践
// ============================================

// ---- 2.1 观察析构函数的调用时机 ----

class Observer {
    string name;
public:
    Observer(const string& n) : name(n) {
        cout << "  [构造] " << name << endl;
    }
    ~Observer() {
        cout << "  [析构] " << name << endl;
    }
    void use() { cout << "  [使用] " << name << endl; }
};

void demoLifetime() {
    cout << string(60, '=') << endl;
    cout << "第二部分 2.1：析构函数调用时机" << endl;
    cout << string(60, '=') << endl;

    cout << "\n--- 栈对象：离开 {} 自动析构 ---" << endl;
    {
        Observer a("A");
        Observer b("B");
        Observer c("C");
        cout << "  三个对象已创建" << endl;
    }  // a、b、c 按 c→b→a 顺序析构（后进先出）

    cout << "\n--- 堆对象：必须 delete 才析构 ---" << endl;
    {
        Observer* p = new Observer("Heap");
        p->use();
        delete p;   // 必须手动 delete！否则内存泄漏
        cout << "  手动 delete 完成" << endl;
    }

    cout << "\n--- 作用域嵌套 ---" << endl;
    {
        Observer outer("Outer");
        {
            Observer inner("Inner");
            inner.use();
        }  // Inner 先析构
        outer.use();
    }  // Outer 后析构
}

// ---- 2.2 RAII：动态数组自动管理内存 ----

class DynArray {
    int*   data;
    size_t size;
public:
    // 构造：分配内存
    DynArray(size_t n) : size(n) {
        data = new int[n]();  // () 零初始化
        cout << "  [构造] DynArray(" << n << ")" << endl;
    }
    // 析构：释放内存（RAII 核心）
    ~DynArray() {
        delete[] data;
        cout << "  [析构] DynArray(" << size << ") — 内存已释放" << endl;
    }

    int&  operator[](size_t i) { return data[i]; }
    size_t getSize() const { return size; }

    void print() const {
        cout << "  [";
        for (size_t i = 0; i < size; i++) {
            cout << data[i];
            if (i + 1 < size) cout << ", ";
        }
        cout << "]" << endl;
    }
};

void demoRAII() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.2：RAII 动态内存管理" << endl;
    cout << string(60, '=') << endl;

    {
        DynArray arr(5);
        for (size_t i = 0; i < arr.getSize(); i++) arr[i] = (int)(i * 10);
        arr.print();
        cout << "  arr 离开作用域..." << endl;
    }  // arr 析构，自动释放内存（无需手动 delete）
    cout << "  内存已由析构函数自动释放" << endl;
}

// ---- 2.3 RAII：文件句柄模拟 ----

class FileHandle {
    string   filename;
    bool     isOpen;
public:
    FileHandle(const string& fn) : filename(fn), isOpen(true) {
        cout << "  [打开文件] " << filename << endl;
    }
    ~FileHandle() {
        if (isOpen) {
            isOpen = false;
            cout << "  [关闭文件] " << filename << "（析构自动关闭）" << endl;
        }
    }

    void write(const string& data) {
        if (isOpen)
            cout << "  [写入] " << filename << ": " << data << endl;
    }
    void close() {
        isOpen = false;
        cout << "  [手动关闭] " << filename << endl;
    }
};

void demoFileRAII() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.3：RAII 文件句柄" << endl;
    cout << string(60, '=') << endl;

    cout << "正常情况：" << endl;
    {
        FileHandle f("log.txt");
        f.write("第一行");
        f.write("第二行");
    }  // f 析构，自动关闭文件

    cout << "\n即使发生异常，RAII 也能正确关闭（智能指针的基础原理）" << endl;
}

// ---- 2.4 全局对象与局部对象的析构时序 ----

class Timed {
    string label;
public:
    Timed(const string& l) : label(l) {
        cout << "  [构造] " << label << endl;
    }
    ~Timed() {
        cout << "  [析构] " << label << endl;
    }
};

// 全局对象：程序启动时构造，程序结束时析构（最后析构）
Timed globalObj("全局对象");

void demoOrderOfDestruction() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.4：析构顺序" << endl;
    cout << string(60, '=') << endl;

    cout << "\n--- 函数内局部对象（析构比全局早）---" << endl;
    Timed local1("local1");
    Timed local2("local2");
    cout << "  函数即将返回，local2、local1 将依次析构" << endl;
    // 返回时：local2 先析构，local1 后析构，globalObj 最后（程序结束）
}

// ============================================
// 第三部分：深入理解（可选）
// ============================================

/*
🔍 深入理解：虚析构函数

class Base {
public:
    ~Base() { cout << "Base 析构" << endl; }  // ❌ 非虚
};
class Derived : public Base {
public:
    ~Derived() { cout << "Derived 析构" << endl; }
};

Base* p = new Derived();
delete p;  // ❌ 只调用 Base::~Base()，Derived 析构未调用 → 资源泄漏！

✅ 有继承关系时，基类析构函数声明为 virtual：
class Base {
public:
    virtual ~Base() { ... }  // ✅ 虚析构，delete p 调用 Derived::~Derived() 再调用 Base::~Base()
};

规则：如果类可能被继承并通过基类指针删除，必须声明虚析构函数。
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonErrors() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第四部分：常见错误和陷阱" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
❌ 错误1：堆对象忘记 delete（内存泄漏）

   void func() {
       Foo* p = new Foo();
       p->doSomething();
       // ❌ 忘记 delete p → 内存泄漏！
   }

   ✅ 要么手动 delete，要么用智能指针（unique_ptr）：
   unique_ptr<Foo> p = make_unique<Foo>();  // 自动析构

------------------------------------------------------------

❌ 错误2：delete 和 delete[] 混用

   int* arr = new int[10];
   delete arr;    // ❌ 应该用 delete[]，只析构第一个元素

   int* single = new int(5);
   delete[] single;   // ❌ 应该用 delete，行为未定义

   ✅ new → delete，new[] → delete[]（严格对应）

------------------------------------------------------------

❌ 错误3：析构函数抛出异常（导致 terminate）

   class Foo {
   public:
       ~Foo() {
           throw runtime_error("析构失败");  // ❌ 析构函数中抛异常
       }
   };
   // 如果析构时已有异常传播，再抛第二个异常 → terminate()

   ✅ 析构函数应标记 noexcept（C++11 起析构默认 noexcept）：
   ~Foo() noexcept { /* 绝不抛异常，用 try-catch 吞掉 */ }

------------------------------------------------------------

❌ 错误4：有继承但基类析构函数不是虚函数（子类资源泄漏）

   class Base { public: ~Base() {} };    // ❌ 非虚
   class Derived : public Base {
       int* data;
   public:
       Derived() { data = new int[100]; }
       ~Derived() { delete[] data; }    // 不会被调用！
   };
   Base* p = new Derived();
   delete p;  // 只调用 Base::~Base()，Derived::~Derived() 未调用 → data 泄漏

   ✅ 基类析构函数声明为 virtual：
   class Base { public: virtual ~Base() {} };

------------------------------------------------------------

❌ 错误5：双重释放（double delete）

   Foo* p = new Foo();
   delete p;
   delete p;  // ❌ 双重释放 → 未定义行为，通常崩溃

   ✅ delete 后立即置 nullptr：
   delete p;
   p = nullptr;   // 之后再 delete nullptr 是安全的空操作

------------------------------------------------------------

❌ 错误6：在析构函数中调用纯虚函数

   class Base {
   public:
       virtual void cleanup() = 0;
       ~Base() { cleanup(); }  // ❌ 析构时基类的虚表指针已切换回 Base，纯虚函数 = 未定义行为
   };

   ✅ 析构函数中只调用非虚函数，或本类的具体实现
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
【析构函数语法】
    ~ClassName() { /* 释放资源 */ }
    virtual ~ClassName() {}    // 基类（有继承时必须声明虚析构）
    ~ClassName() noexcept {}   // 不抛异常（C++11 起默认 noexcept）

【RAII 模式】
    class Resource {
        T* ptr;
    public:
        Resource() { ptr = new T(); }     // 构造：获取
        ~Resource() { delete ptr; }       // 析构：释放
    };

【析构时序规则】
  栈对象：后构造的先析构（LIFO）
  堆对象：delete 时析构（手动）
  全局/静态：程序结束时析构（比局部晚）
  成员对象：先析构成员，再析构宿主（与构造相反）

【new/delete 对应关系】
  new T    → delete p
  new T[]  → delete[] p
  make_unique<T>() → 自动调用 delete（推荐）
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
1. 实现 Matrix 类（2D 动态矩阵）：
   - 构造：Matrix(int rows, int cols) — 分配 rows×cols 的二维数组
   - 析构：正确释放二维数组（先释放每行，再释放行指针数组）
   - 验证析构被调用：在析构函数中打印 "Matrix 析构"

2. 观察析构顺序：
   写一个程序，创建以下对象并观察析构顺序：
   a) 全局对象 Global
   b) main() 中的局部对象 Local1、Local2
   c) 一个 new 出来的堆对象（记得 delete）
   d) 一个嵌套 {} 内的对象 Inner
   预测并验证析构顺序

3. 排错：以下代码有哪些问题？
   class Buffer {
       int* data;
   public:
       Buffer(int n) { data = new int[n]; }
       ~Buffer() { delete data; }             // 问题①
   };
   Buffer* b1 = new Buffer(10);
   Buffer* b2 = b1;
   delete b1;
   delete b2;                                  // 问题②
   // 提示：①应 delete[]；②双重释放
)" << endl;
}

// ============================================
// main 函数
// ============================================

int main() {
    cout << string(60, '=') << endl;
    cout << " 40_destructor_basics.cpp - 析构函数基础" << endl;
    cout << string(60, '=') << endl;

    demoLifetime();
    demoRAII();
    demoFileRAII();
    demoOrderOfDestruction();
    showCommonErrors();
    showFunctionCards();
    showExercises();

    cout << "\n" << string(60, '=') << endl;
    cout << "核心要点总结" << endl;
    cout << string(60, '=') << endl;
    cout << "1. 析构函数：~类名()，无返回类型，无参数，对象销毁时自动调用" << endl;
    cout << "2. RAII：构造获取资源，析构释放资源，自动且安全" << endl;
    cout << "3. new/delete 严格对应，new[] 对应 delete[]" << endl;
    cout << "4. 有继承且通过基类指针 delete → 基类析构函数必须是 virtual" << endl;
    cout << "5. 析构函数不能抛异常（noexcept）" << endl;
    cout << "\n【注意】全局对象在 main() 之后析构，程序结束时可见" << endl;

    return 0;
}
