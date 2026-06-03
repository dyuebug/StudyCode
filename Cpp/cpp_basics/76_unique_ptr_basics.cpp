// ============================================
// 76_unique_ptr_basics.cpp - 超详细注释版
// 日期：2026-05-30（第30天）
// 难度：⭐⭐ 中级
// 主题：unique_ptr — 独占所有权的智能指针
// ============================================

/*
【核心概念】

是什么？
- unique_ptr：独占所有权的智能指针，同一时刻只有一个 unique_ptr 拥有某个资源
- 自动在析构时 delete 所管理的对象，无需手动 delete

为什么需要？
- 裸指针（int* p = new int）需要手动 delete，忘记就内存泄漏
- unique_ptr 让内存管理自动化，离开作用域时自动释放
- 所有权语义清晰：谁拥有 unique_ptr，谁负责资源

核心特点：
- 不可拷贝（= delete 的拷贝构造/赋值），只能移动（std::move）
- 零开销：没有额外的引用计数，与裸指针性能相同
- 用 make_unique<T>(args) 创建（推荐，C++14）
*/

#include <iostream>
#include <vector>
#include <string>
#include <memory>
using namespace std;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

/*
【函数卡片：make_unique】

语法：auto p = make_unique<T>(构造参数...);
作用：在堆上创建 T 对象，返回管理它的 unique_ptr<T>
返回：unique_ptr<T>

为什么用 make_unique 而不是 new？
  1. 异常安全：new T 分配后若后续代码抛异常，裸指针无人清理
  2. 更简洁：不用重复写类型名
  3. 推荐：C++14 起标准做法

------------------------------------------------------------

【函数卡片：unique_ptr 的成员函数】

get()      — 返回原始裸指针（不转移所有权）
release()  — 放弃所有权，返回裸指针（调用方负责 delete！）
reset()    — 释放当前对象，可选地指向新对象
reset(ptr) — 释放旧对象，接管新的裸指针
swap(other) — 与另一个 unique_ptr 交换所有权

运算符：
  ->   — 访问成员（同裸指针）
  *    — 解引用（同裸指针）
  bool — 隐式转换：nullptr 为 false，有值为 true

------------------------------------------------------------

【所有权转移：std::move】

unique_ptr 不能拷贝，只能移动：
  auto p1 = make_unique<int>(42);
  auto p2 = p1;           // ❌ 编译错误：不可拷贝
  auto p3 = move(p1);     // ✅ 移动后 p1 变为 nullptr，p3 拥有资源
*/

// ============================================
// 第二部分：基础操作实践
// ============================================

// 演示用资源类：观察构造/析构时机
class Resource {
    string name;
public:
    Resource(const string& n) : name(n) {
        cout << "  [构造] " << name << endl;
    }
    ~Resource() {
        cout << "  [析构] " << name << endl;
    }
    void use() const { cout << "  [使用] " << name << endl; }
    const string& getName() const { return name; }
};

// ---- 2.1 创建与自动释放 ----

void demoCreateAndAutoFree() {
    cout << string(60, '=') << endl;
    cout << "第二部分 2.1：创建与自动释放" << endl;
    cout << string(60, '=') << endl;

    cout << "\n--- 进入作用域 ---" << endl;
    {
        // make_unique：在堆上创建 Resource，由 p 独占
        // 等价于：unique_ptr<Resource> p(new Resource("Alpha"))
        auto p = make_unique<Resource>("Alpha");

        p->use();  // 用 -> 访问成员，与裸指针相同

        cout << "  p 是否为空：" << (p ? "否" : "是") << endl;
        cout << "  原始指针地址：" << p.get() << endl;
    }  // p 离开作用域 → 析构函数自动调用 delete，Resource 被释放
    cout << "--- 离开作用域，Alpha 已自动释放 ---\n" << endl;

    // 对比：裸指针（危险）
    cout << "--- 裸指针对比（危险写法）---" << endl;
    {
        Resource* raw = new Resource("Danger");
        raw->use();
        // 如果这里抛异常，delete 永远不会执行 → 内存泄漏！
        delete raw;  // 必须手动 delete，容易忘记
    }
    cout << "--- 裸指针需要手动 delete ---\n" << endl;
}

// ---- 2.2 所有权转移（只能移动，不能拷贝）----

void demoOwnershipTransfer() {
    cout << string(60, '=') << endl;
    cout << "第二部分 2.2：所有权转移（移动语义）" << endl;
    cout << string(60, '=') << endl;

    auto p1 = make_unique<Resource>("Beta");
    cout << "p1 拥有 Beta，地址：" << p1.get() << endl;

    // 转移所有权：p1 → p2
    // std::move 将 p1 转为右值，触发 unique_ptr 的移动构造
    auto p2 = move(p1);

    cout << "移动后：" << endl;
    cout << "  p1 是否为空：" << (p1 ? "否（有资源）" : "是（已移走）") << endl;
    cout << "  p2 是否为空：" << (p2 ? "否（拥有资源）" : "是") << endl;
    cout << "  p2 地址：" << p2.get() << "（与之前 p1 地址相同）" << endl;

    // 用移动赋值转移（p2 → p3）
    unique_ptr<Resource> p3;
    p3 = move(p2);
    cout << "p2 赋值给 p3 后：" << endl;
    cout << "  p2 是否为空：" << (p2 ? "否" : "是") << endl;
    cout << "  p3 拥有："; p3->use();
}

// ---- 2.3 常用成员函数 ----

void demoMemberFunctions() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.3：常用成员函数" << endl;
    cout << string(60, '=') << endl;

    auto p = make_unique<Resource>("Gamma");

    // get()：获取裸指针，不转移所有权
    Resource* raw = p.get();
    cout << "get() 得到裸指针：" << raw << "，p 仍然有效：" << (p ? "是" : "否") << endl;

    // reset()：释放当前对象，变为 nullptr
    cout << "\n--- reset() 释放 ---" << endl;
    p.reset();  // 析构 Gamma
    cout << "reset 后 p 为空：" << (p ? "否" : "是") << endl;

    // reset(ptr)：释放旧对象，接管新指针
    cout << "\n--- reset(new) 接管新对象 ---" << endl;
    p.reset(new Resource("Delta"));  // 接管 Delta
    p->use();

    // release()：放弃所有权，返回裸指针（必须手动 delete！）
    cout << "\n--- release() 放弃所有权 ---" << endl;
    Resource* released = p.release();  // p 变为 nullptr，released 是裸指针
    cout << "release 后 p 为空：" << (p ? "否" : "是") << endl;
    released->use();
    delete released;  // release() 后必须手动 delete，否则泄漏
    cout << "手动 delete released" << endl;

    // swap()：交换两个 unique_ptr 的所有权
    cout << "\n--- swap() ---" << endl;
    auto p1 = make_unique<Resource>("Epsilon");
    auto p2 = make_unique<Resource>("Zeta");
    cout << "交换前：p1=" << p1->getName() << "，p2=" << p2->getName() << endl;
    p1.swap(p2);
    cout << "交换后：p1=" << p1->getName() << "，p2=" << p2->getName() << endl;
}

// ---- 2.4 函数参数与返回值 ----

// 工厂函数：返回 unique_ptr，转移所有权给调用方
unique_ptr<Resource> makeResource(const string& name) {
    return make_unique<Resource>(name);  // 隐式移动（NRVO）
}

// 接受 unique_ptr by value：接管所有权，函数结束时自动释放
void consumeResource(unique_ptr<Resource> res) {
    cout << "  consumeResource 持有：";
    res->use();
}  // res 析构，资源释放

// 接受 const 引用：借用，不转移所有权（推荐用于只读访问）
void inspectResource(const unique_ptr<Resource>& res) {
    if (res) cout << "  inspectResource：" << res->getName() << endl;
}

void demoFunctions() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.4：函数参数与返回值" << endl;
    cout << string(60, '=') << endl;

    cout << "\n--- 工厂函数返回 unique_ptr ---" << endl;
    auto p = makeResource("Eta");
    p->use();

    cout << "\n--- 借用（const 引用，不转移所有权）---" << endl;
    inspectResource(p);
    cout << "  调用后 p 仍有效：" << (p ? "是" : "否") << endl;

    cout << "\n--- 转移给函数（by value）---" << endl;
    consumeResource(move(p));  // 必须 move，因为 p 是左值
    cout << "  调用后 p 为空：" << (p ? "否" : "是") << endl;
}

// ---- 2.5 在容器中使用 unique_ptr ----

void demoInContainer() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.5：在容器中使用 unique_ptr" << endl;
    cout << string(60, '=') << endl;

    vector<unique_ptr<Resource>> pool;

    // push_back：必须 move（unique_ptr 不可拷贝）
    pool.push_back(make_unique<Resource>("W1"));
    pool.push_back(make_unique<Resource>("W2"));
    pool.push_back(make_unique<Resource>("W3"));

    cout << "容器中有 " << pool.size() << " 个资源" << endl;

    // 遍历：用 const auto& 借用，不移动
    for (const auto& r : pool) {
        r->use();
    }

    cout << "\n--- 容器析构，所有资源自动释放 ---" << endl;
    // pool 析构时，每个 unique_ptr 析构，每个 Resource 被 delete
}

// ============================================
// 第三部分：深入理解（可选）
// ============================================

/*
🔍 深入理解：unique_ptr 的内存布局

裸指针：
    int* p = new int(42);
    // p 本身：8字节（64位指针）
    // 堆上：4字节 int

unique_ptr<int>：
    unique_ptr<int> p = make_unique<int>(42);
    // p 本身：8字节（与裸指针完全相同！零开销抽象）
    // 堆上：4字节 int

unique_ptr 没有额外的引用计数（shared_ptr 有），
这是它"零开销"的原因：析构时直接 delete，无需计数更新。

🔍 深入理解：为什么 unique_ptr 不可拷贝？

如果允许拷贝：
    auto p1 = make_unique<int>(42);
    auto p2 = p1;  // 假设允许
    // 两个 unique_ptr 都指向同一块内存
    // p1 和 p2 各自析构时，都会 delete 同一指针 → 双重释放 → 崩溃！

所以 unique_ptr 的拷贝构造和拷贝赋值被声明为 = delete：
    unique_ptr(const unique_ptr&) = delete;
    unique_ptr& operator=(const unique_ptr&) = delete;
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonErrors() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第四部分：常见错误和陷阱" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
❌ 错误1：尝试拷贝 unique_ptr（编译错误）

   auto p1 = make_unique<int>(42);
   auto p2 = p1;         // ❌ 编译错误：unique_ptr 不可拷贝
   auto p2 = move(p1);   // ✅ 正确：移动所有权

   记忆：unique = 唯一，同一时刻只有一个拥有者

------------------------------------------------------------

❌ 错误2：move 后继续使用旧指针（悬空使用）

   auto p1 = make_unique<Resource>("X");
   auto p2 = move(p1);
   p1->use();   // ❌ p1 已经是 nullptr，解引用 nullptr → 崩溃

   ✅ 规则：move 后，原指针变为 nullptr，不能再使用
   检查：if (p1) p1->use();   // 安全

------------------------------------------------------------

❌ 错误3：release() 后忘记手动 delete（内存泄漏）

   auto p = make_unique<Resource>("Y");
   Resource* raw = p.release();
   // ❌ 忘记 delete raw → 内存泄漏！
   // release() 让 unique_ptr 放弃所有权，但不删除对象

   ✅ 正确：release() 后必须手动管理
   Resource* raw = p.release();
   // ... 使用 raw ...
   delete raw;   // 必须！

   建议：尽量避免使用 release()，通常没有必要

------------------------------------------------------------

❌ 错误4：get() 得到的裸指针存活超过 unique_ptr

   Resource* raw;
   {
       auto p = make_unique<Resource>("Z");
       raw = p.get();   // 获取裸指针
   }  // p 析构，Resource 被删除
   raw->use();   // ❌ raw 是悬空指针 → 未定义行为

   ✅ 规则：get() 得到的裸指针生命周期不能超过 unique_ptr

------------------------------------------------------------

❌ 错误5：向容器 push_back 时忘记 move

   vector<unique_ptr<Resource>> vec;
   auto p = make_unique<Resource>("W");
   vec.push_back(p);        // ❌ 编译错误：不可拷贝
   vec.push_back(move(p));  // ✅ 正确：移动进容器

   简化写法（直接临时对象，自动移动）：
   vec.push_back(make_unique<Resource>("W"));  // ✅ 临时对象自动移动

------------------------------------------------------------

❌ 错误6：用 new 初始化 unique_ptr 后又 delete（双重释放）

   Resource* raw = new Resource("X");
   unique_ptr<Resource> p(raw);
   delete raw;   // ❌ p 析构时会再次 delete raw → 双重释放 → 崩溃

   ✅ 规则：一旦裸指针交给 unique_ptr 管理，就不要再手动 delete
   unique_ptr<Resource> p(new Resource("X"));  // 交给 p 管理
   // 不要再 delete

   ✅ 最好用 make_unique 完全避免裸指针：
   auto p = make_unique<Resource>("X");

------------------------------------------------------------

❌ 错误7：在函数参数中用 unique_ptr 值传递但不 move

   void process(unique_ptr<Resource> r) { ... }
   auto p = make_unique<Resource>("A");
   process(p);        // ❌ 编译错误：不可拷贝
   process(move(p));  // ✅ 明确转移所有权
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
【创建】
    auto p = make_unique<T>(args...);   // 推荐：C++14
    unique_ptr<T> p(new T(args...));    // 可用，但不推荐

【成员函数】
    p.get()          → 裸指针（不转移所有权）
    p.release()      → 裸指针（放弃所有权，需手动 delete）
    p.reset()        → 释放对象，p 变 nullptr
    p.reset(ptr)     → 释放旧对象，接管新裸指针
    p.swap(q)        → 与 q 交换所有权

【运算符】
    p->member        → 访问成员
    *p               → 解引用
    if (p)           → 检查是否为空（false = nullptr）

【所有权转移】
    auto q = move(p)      → p 变 nullptr，q 接管
    func(move(p))         → 转移给函数
    return p;             → 函数返回时隐式移动（NRVO）

【与函数交互的最佳模式】
    工厂：unique_ptr<T> make() { return make_unique<T>(); }
    借用：void inspect(const unique_ptr<T>& p) { p->use(); }
    转移：void consume(unique_ptr<T> p) { p->use(); }  // 调用方 move(p)
    原始借用：void use(T* p) { p->do(); }  // 最灵活，调用方传 p.get()
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
1. 实现练习：Node 链表节点

   class Node {
       int value;
       unique_ptr<Node> next;   // 用 unique_ptr 管理下一个节点
   public:
       Node(int v) : value(v) {}
       // TODO：实现 append(int v) — 在末尾添加节点
       // TODO：实现 print() — 打印所有节点值
   };

   验证：
   auto head = make_unique<Node>(1);
   head->append(2);
   head->append(3);
   head->print();  // 1 2 3
   // 析构 head 时，通过 unique_ptr 链式析构所有节点（无需手动）

------------------------------------------------------------

2. 实现练习：工厂模式

   class Shape { public: virtual void draw() = 0; virtual ~Shape() = default; };
   class Circle : public Shape { ... };
   class Square : public Shape { ... };

   // TODO：实现工厂函数
   unique_ptr<Shape> createShape(const string& type) {
       // 根据 type 返回 Circle 或 Square
   }

   验证：
   auto s1 = createShape("circle");
   auto s2 = createShape("square");
   s1->draw(); s2->draw();

------------------------------------------------------------

3. 排错练习：找出以下代码的所有错误

   void badCode() {
       Resource* raw = new Resource("A");
       unique_ptr<Resource> p(raw);
       p->use();
       Resource* raw2 = p.release();
       // ...（忘记操作）
       auto p2 = p;          // 错误①
       delete p.get();       // 错误②
       delete raw;           // 错误③
   }

   分析：①不可拷贝；②get不转移所有权再delete会双重释放；③release后忘delete raw2
)" << endl;
}

// ============================================
// main 函数
// ============================================

int main() {
    cout << string(60, '=') << endl;
    cout << " 76_unique_ptr_basics.cpp - unique_ptr 独占所有权" << endl;
    cout << string(60, '=') << endl;

    demoCreateAndAutoFree();
    demoOwnershipTransfer();
    demoMemberFunctions();
    demoFunctions();
    demoInContainer();
    showCommonErrors();
    showFunctionCards();
    showExercises();

    cout << "\n" << string(60, '=') << endl;
    cout << "核心要点总结" << endl;
    cout << string(60, '=') << endl;
    cout << "1. make_unique<T>(args)：创建 unique_ptr，推荐写法" << endl;
    cout << "2. 不可拷贝，只能 move：所有权唯一，防止双重释放" << endl;
    cout << "3. 离开作用域自动 delete：不需要手动内存管理" << endl;
    cout << "4. get()借用，release()放弃，reset()替换" << endl;
    cout << "5. 零开销：内存大小与裸指针相同" << endl;

    return 0;
}
