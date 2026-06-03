// ============================================
// 80_perfect_forwarding.cpp - 超详细注释版
// 日期：2026-05-31（第31天）
// 难度：⭐⭐⭐ 高级
// 主题：完美转发 — std::forward、万能引用、引用折叠
// ============================================

/*
【核心概念】

是什么？
- 完美转发（Perfect Forwarding）：将参数"原样"传递给另一个函数，
  保持参数的值类别（左值保持左值，右值保持右值）
- 万能引用（Universal Reference）：模板参数中的 T&&，
  既能绑定左值，也能绑定右值

为什么需要？
- 包装函数问题：func(T arg) 接受参数后，arg 本身是左值，传给下一层时丢失了右值属性
- 不完美转发：int&& → 传入内部函数时变成 int&，无法触发移动语义
- 完美转发：保持参数的值类别，让内部函数能正确选择拷贝或移动

核心特点：
- std::forward<T>(arg)：根据 T 的类型决定是否转换为右值引用
- 万能引用：template<typename T> void f(T&&)（必须是模板推导！）
- 引用折叠规则：有左值引用则结果为左值引用，否则为右值引用
*/

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <utility>
using namespace std;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

/*
【函数卡片：std::forward】

语法：std::forward<T>(arg)
作用：有条件地将参数转换为右值引用
      - T 推导为 T&（左值）→ 返回 T&（保持左值）
      - T 推导为 T（右值）→ 返回 T&&（保持右值）
返回：T& 或 T&&（取决于 T）
用途：在模板函数中将参数完美转发给下一层函数

等价逻辑（简化）：
  如果 T = int&   → forward 返回 int&（左值）
  如果 T = int    → forward 返回 int&&（右值）

------------------------------------------------------------

【万能引用（Universal Reference）vs 右值引用】

万能引用：template<typename T> void f(T&& param)
  - 在模板类型推导时：T 可以推导为 int& 或 int
  - 传左值：T = int&，param 类型 = int&
  - 传右值：T = int，param 类型 = int&&

右值引用（不是万能引用）：
  void f(int&& param)       // 只接受右值
  Widget&& w = move(x);     // 只接受右值
  auto&& x = expr;          // 是万能引用（auto 也参与推导）

------------------------------------------------------------

【引用折叠规则（Reference Collapsing）】

T& &   → T&    左值 + 左值 = 左值
T& &&  → T&    左值 + 右值 = 左值
T&& &  → T&    右值 + 左值 = 左值
T&& && → T&&   右值 + 右值 = 右值

记忆：有任何左值引用参与，结果就是左值引用
*/

// ============================================
// 第二部分：基础操作实践
// ============================================

// ---- 2.1 不完美转发 vs 完美转发 ----

// 目标函数（3个重载，用于检测传入的是左值还是右值）
void process(string& s) {
    cout << "  → process(string&) 左值版：" << s << endl;
}
void process(const string& s) {
    cout << "  → process(const string&) const左值版：" << s << endl;
}
void process(string&& s) {
    cout << "  → process(string&&) 右值版：" << s << endl;
}

// 不完美转发：T 按值传入，arg 总是左值
template<typename T>
void badWrapper(T arg) {
    cout << "  [badWrapper] ";
    process(arg);   // arg 是具名变量（左值），即使传入右值，这里也是左值版
}

// 完美转发：T&& 万能引用 + std::forward 保持值类别
template<typename T>
void goodWrapper(T&& arg) {
    cout << "  [goodWrapper] ";
    process(forward<T>(arg));  // forward 保持 arg 原来的左值/右值属性
}

void demoForwarding() {
    cout << string(60, '=') << endl;
    cout << "第二部分 2.1：不完美 vs 完美转发" << endl;
    cout << string(60, '=') << endl;

    string s = "hello";

    cout << "\n--- 传入左值 s ---" << endl;
    badWrapper(s);                // arg 是左值 → process(string&)
    goodWrapper(s);               // T=string&，forward 保持左值 → process(string&)

    cout << "\n--- 传入右值（临时对象）---" << endl;
    badWrapper(string("world"));  // ❌ 临时对象复制给 arg，arg 是左值 → process(string&)
    goodWrapper(string("world")); // ✅ T=string，forward 转为右值 → process(string&&)

    cout << "\n--- 传入 move(s) ---" << endl;
    badWrapper(move(s));          // ❌ 同上，arg 是左值
    goodWrapper(move(s));         // ✅ T=string，forward 转为右值 → process(string&&)
}

// ---- 2.2 引用折叠推导过程 ----

template<typename T>
void showTypeDeduction(T&& param) {
    // 根据 T 是否含有 & 来判断参数是左值还是右值
    cout << "  sizeof(param) = " << sizeof(param) << endl;
}

void demoRefCollapsing() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.2：引用折叠推导过程" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
传入左值时的推导过程：
  int x = 5;
  showTypeDeduction(x);
  → T 推导为 int&
  → T&& = int& && （引用折叠）→ int&
  → param 类型：int&（左值引用）

传入右值时的推导过程：
  showTypeDeduction(42);
  → T 推导为 int
  → T&& = int&&
  → param 类型：int&&（右值引用）

std::forward<T>(param) 的效果：
  T = int&  → forward 返回 int&   （左值不变）
  T = int   → forward 返回 int&&  （右值不变）
)" << endl;

    int x = 5;
    showTypeDeduction(x);   // T = int&
    showTypeDeduction(42);  // T = int
}

// ---- 2.3 工厂函数：完美转发构造参数 ----

class Widget {
    string name;
    int    value;
public:
    // 两个构造：一个接受左值（拷贝name），一个接受右值（移动name）
    Widget(const string& n, int v) : name(n), value(v) {
        cout << "  [Widget 拷贝构造] name=" << name << endl;
    }
    Widget(string&& n, int v) : name(move(n)), value(v) {
        cout << "  [Widget 移动构造] name=" << name << endl;
    }
    void show() const { cout << "  Widget(" << name << ", " << value << ")" << endl; }
};

// 完美转发的工厂函数：将参数原样传给构造函数
template<typename T, typename... Args>
unique_ptr<T> makeObject(Args&&... args) {
    // forward<Args>(args)... 展开参数包，每个参数都完美转发
    return make_unique<T>(forward<Args>(args)...);
}

void demoFactory() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.3：工厂函数完美转发" << endl;
    cout << string(60, '=') << endl;

    string name = "Alpha";

    cout << "\n--- 传左值（应调用拷贝构造）---" << endl;
    auto w1 = makeObject<Widget>(name, 1);   // name 是左值 → 拷贝构造
    w1->show();

    cout << "\n--- 传右值（应调用移动构造）---" << endl;
    auto w2 = makeObject<Widget>(string("Beta"), 2);  // 临时对象是右值 → 移动构造
    w2->show();

    cout << "\n--- 传 move(name)（应调用移动构造）---" << endl;
    auto w3 = makeObject<Widget>(move(name), 3);  // 显式 move → 移动构造
    w3->show();
    cout << "  name 移走后：\"" << name << "\"（已移走，空串）" << endl;
}

// ---- 2.4 包装器函数 ----

// 计时包装器：在调用前后打印日志，完美转发参数
template<typename Func, typename... Args>
auto withLogging(const string& funcName, Func&& func, Args&&... args) {
    cout << "  [LOG 开始] " << funcName << endl;
    // forward<Func>(func)：保持 func 的左/右值属性（可能是函数对象）
    // forward<Args>(args)...：展开并完美转发所有参数
    auto result = forward<Func>(func)(forward<Args>(args)...);
    cout << "  [LOG 结束] " << funcName << endl;
    return result;
}

int add(int a, int b) { return a + b; }
string concat(string a, string b) { return a + b; }

void demoWrapper() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.4：日志包装器" << endl;
    cout << string(60, '=') << endl;

    int sum = withLogging("add", add, 10, 20);
    cout << "  sum = " << sum << endl;

    string s = withLogging("concat", concat, string("Hello"), string(" World"));
    cout << "  s = " << s << endl;
}

// ============================================
// 第三部分：深入理解（可选）
// ============================================

/*
🔍 深入理解：std::forward 的实现原理

template<typename T>
T&& forward(remove_reference_t<T>& arg) noexcept {
    return static_cast<T&&>(arg);
}

- 如果 T = int&：返回 static_cast<int&&&> = static_cast<int&>（引用折叠）→ 左值引用
- 如果 T = int：返回 static_cast<int&&> → 右值引用

为什么需要 remove_reference_t<T>？
  remove_reference_t<int&>  = int   （去掉引用）
  remove_reference_t<int>   = int
  这样参数 arg 始终是 int&（引用），避免值传递再拷贝

🔍 深入理解：万能引用的判断条件

满足以下所有条件才是万能引用：
1. 形式必须是 T&&（不能是 const T&&、vector<T>&&）
2. T 必须是需要推导的模板参数
3. 没有 cv 限定符（const/volatile）

例：
  template<typename T>
  void f(T&& p);         // ✅ 万能引用

  template<typename T>
  void g(const T&& p);   // ❌ 不是万能引用（有 const）

  template<typename T>
  void h(vector<T>&& p); // ❌ 不是万能引用（T 被包在 vector 里，不直接推导）
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonErrors() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第四部分：常见错误和陷阱" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
❌ 错误1：用 std::move 代替 std::forward（破坏左值语义）

   template<typename T>
   void wrapper(T&& arg) {
       process(move(arg));   // ❌ 始终当作右值处理！
   }
   string s = "hello";
   wrapper(s);   // s 是左值，但被 move 后变右值，process 消耗了 s 的内容
   // s 现在是空字符串！

   ✅ 正确：用 forward 保持值类别
   process(forward<T>(arg));

------------------------------------------------------------

❌ 错误2：非模板函数中误以为 T&& 是万能引用

   void process(string&& s) {   // ← 这是右值引用，不是万能引用
       doWork(forward<string>(s));  // 无意义：T 是具体类型，不是推导的
   }
   string s = "hello";
   process(s);   // ❌ 编译错误：process 只接受右值

   ✅ 万能引用必须是模板参数推导：
   template<typename T>
   void process(T&& s) { doWork(forward<T>(s)); }  // 才是万能引用

------------------------------------------------------------

❌ 错误3：转发后继续使用已移走的参数

   template<typename T>
   void forwardAndUse(T&& arg) {
       container.push_back(forward<T>(arg));  // 转发（可能 move）
       cout << arg;   // ❌ 如果 arg 是右值，已被移走，arg 内容未定义
   }

   ✅ 转发是最后一个操作，不在转发后使用参数：
   cout << arg;                               // 先用
   container.push_back(forward<T>(arg));      // 最后转发

------------------------------------------------------------

❌ 错误4：可变参数展开时 forward 写法错误

   template<typename... Args>
   void wrapper(Args&&... args) {
       func(forward<Args>(args)...);   // ✅ 正确展开

       func(forward<args>...);         // ❌ 编译错误：forward 参数写错
       func(std::move(args)...);       // ❌ 全部强制右值，破坏左值语义
   }

   记忆：可变参数包用 forward<Args>(args)... 整体展开

------------------------------------------------------------

❌ 错误5：大括号初始化列表不能完美转发

   template<typename T>
   void make(T&& arg) { ... }

   make({1, 2, 3});   // ❌ 编译错误：{1,2,3} 无法推导类型 T

   ✅ 先构造具名对象，再转发：
   vector<int> v = {1, 2, 3};
   make(move(v));    // ✅

------------------------------------------------------------

❌ 错误6：在万能引用和 const 引用之间混淆

   template<typename T>
   void f(const T& arg) { ... }   // const 引用重载

   template<typename T>
   void f(T&& arg) { ... }        // 万能引用重载

   int x = 5;
   f(x);   // 调用万能引用版（T=int&），而不是 const 引用版！
   // 原因：T=int& 比 const int& 更精确匹配

   ✅ 注意：万能引用比 const& 更贪婪，几乎匹配任何类型
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
【std::forward】
语法：std::forward<T>(arg)
效果：T=T& → 返回左值引用；T=T → 返回右值引用
用途：在模板中转发参数，保持值类别

【万能引用模板模式】
template<typename T>
void wrapper(T&& arg) {
    func(std::forward<T>(arg));   // 完美转发
}

【可变参数完美转发】
template<typename... Args>
void wrapper(Args&&... args) {
    func(std::forward<Args>(args)...);   // 展开并完美转发
}

【工厂函数模板】
template<typename T, typename... Args>
unique_ptr<T> make(Args&&... args) {
    return make_unique<T>(std::forward<Args>(args)...);
}

【forward vs move 对比】
std::move(x)     — 无条件转为右值（不管 T 是什么）
std::forward<T>(x) — 有条件：T=T& 保持左值，T=T 转为右值

【判断是否万能引用】
✅ template<typename T> void f(T&&)        — 万能引用
✅ auto&& x = expr                          — 万能引用
❌ void f(int&&)                            — 右值引用（非万能）
❌ template<typename T> void f(const T&&)  — 右值引用（有const）
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
1. 实现练习：完善以下包装器

   template<typename Func, typename... Args>
   auto retry(int maxTimes, Func&& func, Args&&... args) {
       // TODO：尝试调用 func(args...) 最多 maxTimes 次
       // 如果调用抛出异常，捕获后继续重试
       // 最后一次失败则重新抛出异常
       // 注意：使用 forward 完美转发参数
   }

   验证：
   int callCount = 0;
   auto fragileFunc = [&](int x) {
       if (++callCount < 3) throw runtime_error("fail");
       return x * 2;
   };
   int result = retry(5, fragileFunc, 21);  // 第3次成功，result=42

------------------------------------------------------------

2. 排错练习：以下代码有哪些完美转发相关的问题？

   template<typename T>
   class Container {
       vector<T> data;
   public:
       void push(T&& elem) {              // 问题①
           data.push_back(move(elem));    // 问题②
       }

       template<typename U>
       void pushForward(U&& elem) {
           data.push_back(forward<T>(elem));  // 问题③
       }
   };

   Container<string> c;
   string s = "hello";
   c.push(s);              // 问题④（运行时）
   c.pushForward(s);       // 问题⑤（运行时）

   分析：
   ① T&& 不是万能引用（T 不是推导的），只接受右值
   ② move 没问题，但①导致左值无法传入
   ③ forward<T> 应改为 forward<U>
   ④ s 是左值，push 只接受右值 → 编译错误
   ⑤ forward<T>(elem) 中 T=string（类成员参数），而非推导的U → 错误

------------------------------------------------------------

3. 思考题：以下两段代码行为是否相同？

   代码A：
   template<typename T>
   void f(T&& arg) { g(forward<T>(arg)); }

   代码B：
   template<typename T>
   void f(T&& arg) { g(move(arg)); }

   分别考虑：传入左值时、传入右值时的行为差异
   提示：forward 有条件，move 无条件
)" << endl;
}

// ============================================
// main 函数
// ============================================

int main() {
    cout << string(60, '=') << endl;
    cout << " 80_perfect_forwarding.cpp - 完美转发" << endl;
    cout << string(60, '=') << endl;

    demoForwarding();
    demoRefCollapsing();
    demoFactory();
    demoWrapper();
    showCommonErrors();
    showFunctionCards();
    showExercises();

    cout << "\n" << string(60, '=') << endl;
    cout << "核心要点总结" << endl;
    cout << string(60, '=') << endl;
    cout << "1. 完美转发：用 T&&（万能引用）+ forward<T> 保持值类别" << endl;
    cout << "2. forward vs move：forward 有条件，move 无条件" << endl;
    cout << "3. 引用折叠：有左值引用则结果为左值引用" << endl;
    cout << "4. 可变参数包：forward<Args>(args)... 整体展开" << endl;
    cout << "5. 转发后不再使用参数（可能已被移走）" << endl;

    return 0;
}
