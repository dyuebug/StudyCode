// ============================================
// 77_shared_ptr_basics.cpp
// 日期：2026-05-30（第30天）
// 主题：shared_ptr — 共享所有权的智能指针
// 难度：⭐⭐ 中级
// 编译：g++ -std=c++17 -o 77_shared_ptr_basics 77_shared_ptr_basics.cpp
// ============================================

/*
【核心概念】shared_ptr

是什么？
- shared_ptr 是 C++ 标准库提供的共享所有权智能指针
- 多个 shared_ptr 可以同时拥有同一个对象
- 内部维护一个"引用计数"，记录有多少个 shared_ptr 指向同一对象
- 当最后一个 shared_ptr 销毁时，对象才会被自动释放

为什么需要？
- 解决多个对象需要共享同一资源时的内存管理问题
- 避免手动 delete，消除内存泄漏和悬空指针风险
- 支持拷贝和赋值（unique_ptr 不支持拷贝）

核心特点：
- 引用计数自动管理：拷贝 +1，销毁 -1，归零时释放对象
- 线程安全：引用计数的增减是原子操作
- 控制块：独立的内存块，存储引用计数和弱引用计数
- 支持自定义删除器
*/

#include <iostream>
#include <memory>
#include <vector>
#include <string>

using namespace std;

// ============================================
// 辅助类：用于观察构造/析构时机
// ============================================
class Resource {
public:
    string name;  // 资源名称，便于识别

    // 构造函数：打印创建信息
    explicit Resource(const string& n) : name(n) {
        cout << "  [构造] Resource: " << name << endl;
    }

    // 析构函数：打印销毁信息
    ~Resource() {
        cout << "  [析构] Resource: " << name << endl;
    }

    void show() const {
        cout << "  Resource::show() -> name = " << name << endl;
    }
};

// ============================================
// 第一部分：核心概念详解 — 函数卡片 ⭐⭐⭐⭐⭐
// ============================================

/*
============================================================
【函数卡片1：make_shared<T>(args...)】

作用：创建一个 shared_ptr，同时构造 T 类型对象（推荐方式）
头文件：<memory>

语法：
    auto sp = make_shared<T>(构造参数...);

参数：
    - T        ：要创建的对象类型
    - args...  ：传递给 T 构造函数的参数

返回值：shared_ptr<T>

优势（对比 shared_ptr<T>(new T)）：
    1. 只分配一次内存（对象 + 控制块合并分配）
    2. 异常安全（new 和构造函数分离时可能泄漏）
    3. 代码更简洁

示例：
    auto sp = make_shared<int>(42);
    auto sp2 = make_shared<Resource>("res1");
============================================================

【函数卡片2：use_count()】

作用：返回当前有多少个 shared_ptr 共享同一对象
语法：
    long count = sp.use_count();

返回值：long，引用计数值
注意：
    - 仅用于调试，不要在生产逻辑中依赖此值
    - 多线程下返回值可能已过时

示例：
    auto sp1 = make_shared<int>(10);
    auto sp2 = sp1;          // 拷贝，引用计数 +1
    cout << sp1.use_count(); // 输出 2
============================================================

【函数卡片3：reset()】

作用：放弃当前所有权（引用计数 -1），可选地接管新对象
语法：
    sp.reset();              // 放弃所有权，sp 变为 nullptr
    sp.reset(new T(args));   // 放弃旧对象，接管新对象

示例：
    auto sp = make_shared<int>(5);
    sp.reset();              // 引用计数降为 0，对象被销毁（若无其他共享者）
    sp.reset(new int(99));   // sp 现在指向新的 int(99)
============================================================

【函数卡片4：get()】

作用：获取底层裸指针（不转移所有权）
语法：
    T* raw = sp.get();

返回值：T*，底层裸指针
警告：
    - 绝对不要对 get() 返回的指针调用 delete！
    - 仅用于需要裸指针的 C 风格 API 调用

示例：
    auto sp = make_shared<int>(7);
    int* raw = sp.get();     // raw 指向同一个 int
    // delete raw;           // ❌ 绝对禁止！
============================================================

【函数卡片5：unique()（C++17 已废弃，C++20 移除）】

作用：判断引用计数是否为 1（即当前是唯一所有者）
替代方案：use_count() == 1

语法（C++14 及以前）：
    bool alone = sp.unique();  // 等价于 sp.use_count() == 1

注意：C++17 起废弃，建议直接用 use_count() == 1
============================================================
*/

// ============================================
// 第二部分：代码实践 ⭐⭐⭐⭐
// ============================================

// --------------------------------------------
// 2.1 创建与引用计数变化
// --------------------------------------------
void demo_ref_count() {
    cout << string(60, '=') << endl;
    cout << "2.1 创建与引用计数变化" << endl;
    cout << string(60, '-') << endl;

    // make_shared 创建对象，引用计数 = 1
    auto sp1 = make_shared<Resource>("Alpha");
    cout << "创建 sp1，use_count = " << sp1.use_count() << endl;  // 1

    {
        // 拷贝构造：引用计数 +1，变为 2
        auto sp2 = sp1;
        cout << "拷贝给 sp2，use_count = " << sp1.use_count() << endl;  // 2

        {
            // 再次拷贝：引用计数 +1，变为 3
            auto sp3 = sp1;
            cout << "拷贝给 sp3，use_count = " << sp1.use_count() << endl;  // 3
            // sp3 离开作用域，引用计数 -1，变为 2
        }
        cout << "sp3 销毁后，use_count = " << sp1.use_count() << endl;  // 2
        // sp2 离开作用域，引用计数 -1，变为 1
    }
    cout << "sp2 销毁后，use_count = " << sp1.use_count() << endl;  // 1

    // reset：主动放弃所有权，引用计数 -1，归零 → 析构
    cout << "调用 sp1.reset()..." << endl;
    sp1.reset();  // 引用计数降为 0，Resource 被析构

    cout << "sp1 reset 后，use_count = " << sp1.use_count() << endl;   // 0
    cout << "sp1 是否为 nullptr: " << (sp1 == nullptr ? "是" : "否") << endl;
}

// --------------------------------------------
// 2.2 shared_ptr 可以拷贝（与 unique_ptr 的对比）
// --------------------------------------------
void demo_copy_vs_unique() {
    cout << string(60, '=') << endl;
    cout << "2.2 shared_ptr 可以拷贝（与 unique_ptr 对比）" << endl;
    cout << string(60, '-') << endl;

    // unique_ptr：独占所有权，不可拷贝，只能移动
    auto up = make_unique<Resource>("UniqueRes");
    // auto up2 = up;            // ❌ 编译错误：unique_ptr 不可拷贝
    auto up2 = move(up);         // ✅ 只能移动转让所有权
    cout << "unique_ptr 移动后，原指针为: "
         << (up == nullptr ? "nullptr" : "有效") << endl;

    cout << endl;

    // shared_ptr：共享所有权，可以自由拷贝
    auto sp1 = make_shared<Resource>("SharedRes");
    auto sp2 = sp1;              // ✅ 拷贝，两者共享同一对象
    auto sp3 = sp1;              // ✅ 再拷贝，三者共享

    cout << "sp1 use_count = " << sp1.use_count() << endl;  // 3
    cout << "sp2 use_count = " << sp2.use_count() << endl;  // 3（同一对象）
    cout << "sp1 和 sp2 指向同一对象: "
         << (sp1.get() == sp2.get() ? "是" : "否") << endl;

    // 通过任何一个指针修改对象，其他指针也能看到变化
    sp2->name = "SharedRes_Modified";
    cout << "通过 sp2 修改后，sp1->name = " << sp1->name << endl;
}

// --------------------------------------------
// 2.3 在容器中共享同一对象
// --------------------------------------------
void demo_shared_in_container() {
    cout << string(60, '=') << endl;
    cout << "2.3 在容器中共享同一对象" << endl;
    cout << string(60, '-') << endl;

    // 创建一个 shared_ptr，将同一对象存入 vector 多个位置
    auto res = make_shared<Resource>("SharedInVector");

    // vector 存储 shared_ptr（每个元素共享同一对象）
    vector<shared_ptr<Resource>> vec;
    vec.push_back(res);   // 拷贝给 vec[0]，use_count = 2
    vec.push_back(res);   // 拷贝给 vec[1]，use_count = 3

    cout << "存入 vector 后，use_count = " << res.use_count() << endl;  // 3

    // 通过 vector 中的元素访问对象
    vec[0]->show();
    vec[1]->show();

    // 清空 vector，容器内的 shared_ptr 销毁，use_count -2
    vec.clear();
    cout << "vector 清空后，use_count = " << res.use_count() << endl;  // 1

    // 原始的 res 仍然有效
    res->show();
}

// --------------------------------------------
// 2.4 从 unique_ptr 转换到 shared_ptr
// --------------------------------------------
void demo_unique_to_shared() {
    cout << string(60, '=') << endl;
    cout << "2.4 从 unique_ptr 转换到 shared_ptr" << endl;
    cout << string(60, '-') << endl;

    // 先用 unique_ptr 管理资源（独占）
    auto up = make_unique<Resource>("ConvertMe");
    cout << "unique_ptr 阶段，资源名: " << up->name << endl;

    // 将 unique_ptr 移动转换为 shared_ptr（unique_ptr 失去所有权）
    // 语法：shared_ptr<T> sp = std::move(unique_ptr);
    shared_ptr<Resource> sp = move(up);  // up 变为 nullptr，sp 接管

    cout << "转换后 unique_ptr 为: "
         << (up == nullptr ? "nullptr" : "有效") << endl;
    cout << "转换后 shared_ptr use_count = " << sp.use_count() << endl;  // 1

    // 现在可以自由拷贝 shared_ptr
    auto sp2 = sp;
    cout << "拷贝后 use_count = " << sp.use_count() << endl;  // 2

    // 注意：反向（shared_ptr → unique_ptr）不支持，共享所有权不能变独占
    // auto up2 = move(sp);  // ❌ 编译错误
}

// ============================================
// 第三部分：深入理解（可选）🔍
// ============================================

/*
==============================================
🔍 深入理解：引用计数内存布局与控制块（选学）
==============================================

如果你想知道 shared_ptr 内部如何实现，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【控制块（Control Block）】

shared_ptr 内部维护两个指针：
1. 指向托管对象的指针（T*）
2. 指向控制块的指针

控制块包含：
┌─────────────────────────────┐
│  强引用计数（shared count）  │ ← shared_ptr 的个数
│  弱引用计数（weak count）    │ ← weak_ptr 的个数 + 1
│  删除器（Deleter）           │ ← 默认是 delete，可自定义
│  分配器（Allocator）         │
└─────────────────────────────┘

【make_shared vs shared_ptr(new T)】

方式1：make_shared<T>(args)
  - 一次内存分配，对象和控制块在连续内存中
  - 内存布局：[控制块 | 对象数据]（连续）
  - 优点：性能更好（减少内存碎片，缓存友好）
  - 缺点：对象内存在弱引用计数归零前不会释放

方式2：shared_ptr<T>(new T(args))
  - 两次内存分配：一次 new T，一次分配控制块
  - 内存布局：[控制块]  [对象数据]（分离）
  - 缺点：性能稍差
  - 优点：对象内存在强引用计数归零时立即释放

【引用计数的线程安全】

引用计数的增减使用原子操作（std::atomic），因此：
- 多个线程同时拷贝/销毁 shared_ptr 是安全的
- 但通过 shared_ptr 访问对象本身不是线程安全的
- 即：管理（引用计数）安全，访问（对象数据）需自己加锁

==============================================
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================
void demo_common_mistakes() {
    cout << string(60, '=') << endl;
    cout << "第四部分：常见错误和陷阱" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
------------------------------------------------------------
错误1：循环引用导致内存泄漏
------------------------------------------------------------
❌ 错误代码：
  struct Node {
      shared_ptr<Node> next;
      shared_ptr<Node> prev;
      ~Node() { cout << "析构" << endl; }
  };
  {
      auto a = make_shared<Node>();
      auto b = make_shared<Node>();
      a->next = b;  // a 持有 b
      b->prev = a;  // b 又持有 a → 循环！
  }
  // 离开作用域后，a 和 b 引用计数都是 1，永远不释放！

原因：a 和 b 互相持有对方，引用计数永远不能归零。

✅ 正确做法：
  将其中一个方向改为 weak_ptr<Node> prev;
  （详见 78_weak_ptr_basics.cpp）

预防措施：父持有子用 shared_ptr，子回指父用 weak_ptr

------------------------------------------------------------
错误2：用同一裸指针创建两个独立的 shared_ptr（双重释放）
------------------------------------------------------------
❌ 错误代码：
  int* raw = new int(42);
  shared_ptr<int> sp1(raw);  // 创建控制块1，引用计数=1
  shared_ptr<int> sp2(raw);  // 创建控制块2，引用计数=1（独立！）
  // sp1 销毁 → delete raw
  // sp2 销毁 → 再次 delete raw → double free！崩溃！

原因：两个 shared_ptr 各有独立控制块，都认为自己是唯一所有者。

✅ 正确做法：
  auto sp1 = make_shared<int>(42);   // 用 make_shared
  auto sp2 = sp1;                    // 只拷贝已有 shared_ptr

预防措施：始终用 make_shared，杜绝裸指针传入 shared_ptr 构造函数

------------------------------------------------------------
错误3：delete get() 返回的裸指针
------------------------------------------------------------
❌ 错误代码：
  auto sp = make_shared<int>(10);
  int* raw = sp.get();
  delete raw;   // ❌ 手动释放了 shared_ptr 管理的内存！
  // sp 离开作用域时再次 delete → double free！

原因：get() 只是"借用"裸指针，所有权仍归 shared_ptr。

✅ 正确做法：
  // 不要 delete get() 的结果
  void c_api(int* p) { /* 只读取，不释放 */ }
  c_api(sp.get());  // ✅ 传给 C API 只读即可

预防措施：记住 get() 是"观察"，不是"取走所有权"

------------------------------------------------------------
错误4：忘记检查 nullptr 直接解引用
------------------------------------------------------------
❌ 错误代码：
  shared_ptr<Resource> sp;  // 默认构造，sp == nullptr
  sp->show();               // ❌ 解引用空指针，未定义行为（崩溃）

原因：默认构造的 shared_ptr 不指向任何对象。

✅ 正确做法：
  if (sp) {              // 检查非空（隐式转换为 bool）
      sp->show();
  }

预防措施：使用前始终检查有效性；尽量用 make_shared 初始化

------------------------------------------------------------
错误5：忽略 make_shared 的性能优势
------------------------------------------------------------
❌ 次优代码：
  shared_ptr<Resource> sp(new Resource("test"));
  // 问题：两次内存分配（new Resource + 分配控制块）

✅ 推荐代码：
  auto sp = make_shared<Resource>("test");
  // 优点：一次内存分配，异常安全，代码更简洁

预防措施：默认使用 make_shared；
          仅在需要自定义删除器时才用 shared_ptr(ptr, deleter)

------------------------------------------------------------
错误6：使用已废弃的 unique()
------------------------------------------------------------
❌ 废弃写法（C++17 起废弃，C++20 移除）：
  auto sp = make_shared<int>(5);
  if (sp.unique()) {   // C++17 警告，C++20 编译错误
      cout << "唯一所有者" << endl;
  }

原因：unique() 是 use_count()==1 的语法糖，
      多线程场景下 unique() 返回 true 后，
      另一线程可能立即拷贝，导致判断已失效。

✅ 正确做法：
  if (sp.use_count() == 1) {
      cout << "唯一所有者（当前时刻）" << endl;
  }

预防措施：开启编译器废弃警告（-Wdeprecated-declarations）
)" << endl;
}

// ============================================
// 第五部分：函数卡片速查
// ============================================
void show_function_reference() {
    cout << string(60, '=') << endl;
    cout << "第五部分：函数卡片速查" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【make_shared<T>(args)】
  作用：创建 shared_ptr（推荐方式）
  返回：shared_ptr<T>
  示例：auto sp = make_shared<int>(42);

────────────────────────────────────────
【sp.use_count()】
  作用：返回引用计数
  返回：long
  注意：多线程下仅供调试
  示例：cout << sp.use_count();

────────────────────────────────────────
【sp.reset()】
  作用：放弃所有权（引用计数-1），sp 变 nullptr
  变体：sp.reset(new T)  // 放弃旧对象，接管新对象
  示例：sp.reset();

────────────────────────────────────────
【sp.get()】
  作用：获取裸指针（不转移所有权）
  返回：T*
  警告：禁止 delete 返回值！
  示例：T* raw = sp.get();

────────────────────────────────────────
【if (sp)】
  作用：检查 shared_ptr 是否非空（布尔转换）
  示例：if (sp) { sp->method(); }

────────────────────────────────────────
【shared_ptr<T> sp = move(up)】
  作用：从 unique_ptr 转换为 shared_ptr
  注意：unique_ptr 变为 nullptr
  示例：auto up = make_unique<int>(1);
        shared_ptr<int> sp = move(up);

────────────────────────────────────────
【sp.use_count() == 1】（替代废弃的 unique()）
  作用：判断是否为唯一所有者
  示例：if (sp.use_count() == 1) { ... }
)" << endl;
}

// ============================================
// 第六部分：练习题（不提供答案）⭐⭐⭐
// ============================================
void show_exercises() {
    cout << string(60, '=') << endl;
    cout << "第六部分：练习题" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【练习1】引用计数追踪

编写程序，创建 shared_ptr<string> 指向字符串 "Hello"。
按以下步骤操作，每步后输出 use_count()：
  1. 创建 sp1
  2. 拷贝给 sp2
  3. 将 sp2 赋值给 sp3（用赋值运算符）
  4. 调用 sp2.reset()
  5. sp3 离开作用域
  6. sp1.reset()，对象是否已析构？

提示：
  - 创建带析构打印的辅助类会更直观
  - 思考 reset() 和作用域结束哪个先触发析构

────────────────────────────────────────
【练习2】shared_ptr 的函数参数传递方式比较

设计函数 void process(...)，在内部打印 use_count。
分别以以下三种方式声明并调用，观察 use_count 变化：
  1. void process(shared_ptr<Resource> sp)       // 值传递
  2. void process(shared_ptr<Resource>&& sp)     // 移动传递
  3. void process(const shared_ptr<Resource>& sp) // const 引用传递

思考：哪种性能最好？分别适用于什么场景？

提示：
  - 值传递会拷贝（use_count+1），引用传递不会
  - 移动传递后，调用处的原 sp 变为 nullptr

────────────────────────────────────────
【练习3】简单对象池（工厂函数返回 shared_ptr）

实现要求：
  1. 定义类 Connection，含 id（int）和 connected（bool）
  2. 编写工厂函数 shared_ptr<Connection> createConnection(int id)
  3. 在 main 中创建3个连接，存入 vector<shared_ptr<Connection>>
  4. 让 vec[0] 和 vec[2] 指向同一个 Connection 对象（共享）
  5. 遍历 vector，打印每个连接的 id 和 use_count
  6. 调用 vec.clear()，验证共享对象何时析构（通过析构打印确认）

提示：
  - 共享同一对象：vec.push_back(existing_sp)（拷贝已有 shared_ptr）
  - 思考：如果对象被两个元素共享，clear() 后它何时析构？
)" << endl;
}

// ============================================
// main 函数
// ============================================
int main() {
    cout << string(60, '=') << endl;
    cout << "77_shared_ptr_basics.cpp" << endl;
    cout << "主题：shared_ptr — 共享所有权的智能指针" << endl;
    cout << "日期：2026-05-30（第30天）" << endl;
    cout << string(60, '=') << endl;
    cout << endl;

    demo_ref_count();
    cout << endl;

    demo_copy_vs_unique();
    cout << endl;

    demo_shared_in_container();
    cout << endl;

    demo_unique_to_shared();
    cout << endl;

    demo_common_mistakes();
    cout << endl;

    show_function_reference();
    cout << endl;

    show_exercises();

    cout << string(60, '=') << endl;
    cout << "学习完成！建议继续学习：78_weak_ptr_basics.cpp" << endl;
    cout << string(60, '=') << endl;

    return 0;
}
