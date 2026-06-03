// ============================================
// 51_operator_overloading_basics.cpp - 超详细注释版
// 日期：第16天
// 难度：⭐⭐ 中级
// 主题：运算符重载基础
// ============================================

/*
【核心概念】

是什么？
- 运算符重载（Operator Overloading）：赋予自定义类型已有运算符新的含义
- 让 Point + Point、 Vector == Vector 这样的写法合法且有意义

为什么需要？
- 内置类型（int、double）天然支持 +、-、==
- 自定义类型若不重载，只能调用 add()、equals() 等方法，不够直观
- 重载后代码更自然：v1 + v2 比 v1.add(v2) 更符合数学直觉

核心特点：
- 只能重载 C++ 已有的运算符，不能发明新符号
- 不能改变优先级和结合性
- 成员函数重载：左操作数是 this 对象
- 友元/非成员函数重载：适合 << >> 等左操作数不是本类的情况
*/

#include <iostream>
using namespace std;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

/*
【函数卡片：成员函数运算符重载】

语法：
    返回类型 operator运算符(参数) [const]

示例：
    Point operator+(const Point& other) const   // 二元运算符
    bool  operator==(const Point& other) const  // 比较运算符
    Point& operator++()                          // 前置自增（无参）
    Point  operator++(int)                       // 后置自增（占位 int 参数）

规则：
    - 二元运算符成员函数：只需一个参数（右操作数），左操作数是 *this
    - operator+ 通常返回新对象（const 成员函数，不修改自身）
    - operator== 通常返回 bool，加 const 表示不修改对象
    - 前置 ++ 返回 *this 引用；后置 ++ 返回旧值副本

【可以重载的运算符（部分）】

算术：  + - * / %
比较：  == != < > <= >=
赋值：  = += -= *= /=
自增：  ++ --（前置/后置）
流输出：<< >>（通常用 friend 非成员函数）
下标：  [] （返回引用，支持读写）
函数：  () （让对象像函数一样调用）

【不能重载的运算符】

.    ::    ?:    sizeof    typeid
*/

// ============================================
// 第二部分：基础操作实践
// ============================================

// ---- 2.1 基本运算符重载（Point 类）----

class Point {
private:
    int x;   // x 坐标
    int y;   // y 坐标

public:
    // 构造函数：初始化坐标
    // 参数：x, y - 坐标值，默认为 0
    Point(int x = 0, int y = 0) : x(x), y(y) {}

    // 打印坐标（方便查看结果）
    void print() const {
        cout << "(" << x << ", " << y << ")";
    }

    // ---- 加法运算符重载 ----
    // 语法：Point operator+(const Point& other) const
    // 参数：other - 右操作数（另一个 Point）
    // 返回：新 Point 对象（不修改原对象，所以加 const）
    // 使用：p1 + p2 等价于 p1.operator+(p2)
    Point operator+(const Point& other) const {
        // 创建新对象：x 和 y 分别相加
        return Point(x + other.x, y + other.y);
    }

    // ---- 减法运算符重载 ----
    // 参数：other - 右操作数
    // 返回：新 Point（坐标相减）
    Point operator-(const Point& other) const {
        return Point(x - other.x, y - other.y);
    }

    // ---- 相等比较运算符重载 ----
    // 参数：other - 右操作数
    // 返回：bool（两点坐标完全相同返回 true）
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }

    // ---- 不等比较运算符重载 ----
    // 复用 operator==（避免重复逻辑）
    bool operator!=(const Point& other) const {
        return !(*this == other);   // 用已实现的 == 取反
    }

    // ---- 复合赋值运算符 += ----
    // 参数：other - 右操作数
    // 返回：*this 引用（支持链式：p1 += p2 += p3）
    // 注意：修改了自身，所以不加 const
    Point& operator+=(const Point& other) {
        x += other.x;
        y += other.y;
        return *this;   // 返回自身引用
    }

    // ---- 前置自增 ++ ----
    // 参数：无（区别于后置 ++ 的 int 占位参数）
    // 返回：*this 引用（先自增，返回自增后的值）
    Point& operator++() {
        ++x;
        ++y;
        return *this;
    }

    // ---- 后置自增 ++ ----
    // 参数：int（占位符，仅用于与前置 ++ 区分，不使用）
    // 返回：旧值副本（先保存旧值，再自增，返回旧值）
    Point operator++(int) {
        Point old = *this;  // 保存自增前的旧值
        ++x;                // 自增
        ++y;
        return old;         // 返回旧值
    }

    // getter（供外部只读访问坐标）
    int getX() const { return x; }
    int getY() const { return y; }
};

void demoBasicOperators() {
    cout << string(60, '=') << endl;
    cout << "第二部分 2.1：基本运算符重载（Point 类）" << endl;
    cout << string(60, '=') << endl;

    Point p1(2, 3);
    Point p2(4, 5);

    cout << "\n--- 加法：p1 + p2 ---" << endl;
    Point p3 = p1 + p2;  // 调用 operator+(p2)，p1 是 this
    cout << "p1 = "; p1.print(); cout << endl;
    cout << "p2 = "; p2.print(); cout << endl;
    cout << "p3 = p1 + p2 = "; p3.print(); cout << endl;

    cout << "\n--- 减法：p2 - p1 ---" << endl;
    Point p4 = p2 - p1;
    cout << "p4 = p2 - p1 = "; p4.print(); cout << endl;

    cout << "\n--- 比较：== 和 != ---" << endl;
    Point a(1, 1), b(1, 1), c(2, 2);
    cout << "a == b: " << (a == b ? "true" : "false") << endl;
    cout << "a != c: " << (a != c ? "true" : "false") << endl;

    cout << "\n--- 复合赋值：+= ---" << endl;
    Point q(1, 1);
    cout << "q 初始值: "; q.print(); cout << endl;
    q += Point(3, 4);  // 等价于 q.operator+=(Point(3, 4))
    cout << "q += (3,4) 后: "; q.print(); cout << endl;
}

// ---- 2.2 前置 ++ 和后置 ++ 的区别 ----

void demoIncrementOperators() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.2：前置 ++ 和后置 ++ 的区别" << endl;
    cout << string(60, '=') << endl;

    Point p(1, 2);
    cout << "初始 p = "; p.print(); cout << endl;

    // 前置 ++：先自增，再返回（返回的是自增后的 p）
    cout << "\n--- 前置 ++p ---" << endl;
    Point pre = ++p;   // 等价于 p.operator++()，无 int 参数
    cout << "执行 pre = ++p 后：" << endl;
    cout << "  p   = "; p.print();   cout << "（已自增）" << endl;
    cout << "  pre = "; pre.print(); cout << "（与 p 相同，是自增后的值）" << endl;

    // 后置 ++：先保存旧值，再自增，返回旧值
    cout << "\n--- 后置 p++ ---" << endl;
    Point post = p++;  // 等价于 p.operator++(0)，有 int 参数
    cout << "执行 post = p++ 后：" << endl;
    cout << "  p    = "; p.print();    cout << "（已自增）" << endl;
    cout << "  post = "; post.print(); cout << "（旧值，自增前的）" << endl;

    cout << "\n结论：" << endl;
    cout << "  前置 ++：返回自增后的值（引用）" << endl;
    cout << "  后置 ++：返回自增前的旧值（副本），效率略低" << endl;
    cout << "  性能建议：不需要旧值时，优先用前置 ++" << endl;
}

// ============================================
// 🔍 第三部分：深入理解（可选）
// ============================================

/*
【深入理解：为什么 operator+ 返回值，operator+= 返回引用？】

operator+（加法）：
    语义：a + b 应该产生新值，不修改 a 或 b
    所以：返回新对象（按值返回）
    Point operator+(const Point& o) const { return Point(x+o.x, y+o.y); }

operator+=（复合赋值）：
    语义：a += b 就是 a = a + b，修改了 a
    支持链式：(a += b) += c → 需要 a+=b 返回 a 的引用
    所以：返回 *this 的引用
    Point& operator+=(const Point& o) { x+=o.x; ...; return *this; }

【深入理解：成员函数 vs 友元函数选择】

何时用成员函数重载：
    左操作数是本类对象：p1 + p2、p1 == p2、p1++

何时用友元/非成员函数重载：
    左操作数不是本类：cout << p（左边是 ostream，不是 Point）
    需要支持交换律：2 * p（如果 * 是成员函数，只能写 p * 2）

例：
    // 成员函数：p * 2（p 是 this）
    Point operator*(int n) const { return Point(x*n, y*n); }
    // 但 2 * p 无法调用成员函数（2 不是 Point 对象）

    // 友元非成员函数：支持两种顺序
    friend Point operator*(int n, const Point& p) { return p * n; }
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonErrors() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第四部分：常见错误和陷阱" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
❌ 错误1：operator+ 修改了 this（违背加法语义）

   Point operator+(const Point& other) {  // ← 没有 const
       x += other.x;   // ❌ 修改了 this！a + b 不应该改变 a
       y += other.y;
       return *this;
   }

   ✅ 正确：operator+ 创建新对象，不修改原对象
   Point operator+(const Point& other) const {   // ← 加 const
       return Point(x + other.x, y + other.y);   // ← 新对象
   }

------------------------------------------------------------

❌ 错误2：前置 ++ 和后置 ++ 返回值弄反

   // ❌ 错误：前置 ++ 返回旧值副本
   Point operator++() {
       Point old = *this;
       ++x; ++y;
       return old;  // 前置 ++ 应该返回自增后的引用！
   }

   ✅ 前置 ++ 返回引用（自增后的值）：
   Point& operator++() { ++x; ++y; return *this; }

   // ❌ 错误：后置 ++ 返回引用
   Point& operator++(int) {
       ++x; ++y;
       return *this;  // 后置 ++ 应该返回旧值副本！
   }

   ✅ 后置 ++ 返回旧值副本：
   Point operator++(int) {
       Point old = *this;  // 保存旧值
       ++x; ++y;
       return old;          // 返回旧值副本
   }

   记忆：前置 = 先改后返（引用），后置 = 先存后改再返旧值（副本）

------------------------------------------------------------

❌ 错误3：operator== 没有 const 修饰

   bool operator==(const Point& other) {  // ← 没有 const
       return x == other.x && y == other.y;
   }
   // 问题：const Point 对象无法调用非 const 的 ==
   const Point p(1,1);
   p == Point(1,1);   // ❌ 编译错误（p 是 const，不能调用非 const 方法）

   ✅ 正确：operator== 加 const（比较不需要修改对象）
   bool operator==(const Point& other) const { ... }

------------------------------------------------------------

❌ 错误4：operator!= 重复实现而不复用 ==

   bool operator!=(const Point& other) const {
       return x != other.x || y != other.y;  // ← 手写逻辑，与 == 逻辑重复
   }

   ✅ 正确：复用 operator== 取反（DRY 原则）
   bool operator!=(const Point& other) const {
       return !(*this == other);   // 直接复用 ==，逻辑一致
   }

------------------------------------------------------------

❌ 错误5：重载了语义不合理的运算符

   // 分数类重载 << 为"左移"（语义混乱）
   Fraction operator<<(int n) { numerator *= n; return *this; }

   // 使用时：f << 2 是什么意思？乘以2？左移？非常混乱
   Fraction f(1, 2);
   f << 2;   // 语义不清晰，违背最少惊讶原则

   ✅ 规则：只有运算符的语义与操作自然匹配时才重载
   // 分数类应该重载 +（分数相加）、==（分数相等比较），
   // 而不是 << 这种不合逻辑的操作

------------------------------------------------------------

❌ 错误6：operator+= 没有返回引用（链式调用失效）

   Point operator+=(const Point& other) {  // ← 返回值，不是引用
       x += other.x;
       y += other.y;
       return *this;  // 返回副本，不是 *this 本身
   }
   // a += b += c 时，b += c 返回 b 的副本
   // a += (副本) 的结果不影响原来的 b

   ✅ 正确：返回 *this 的引用
   Point& operator+=(const Point& other) {
       x += other.x; y += other.y;
       return *this;   // ← 引用，链式调用有效
   }
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
【常见运算符重载模板】

二元算术（+、-、*、/）：
    Type operator+(const Type& rhs) const {
        return Type(lhs_data + rhs.data);   // 返回新对象
    }

比较（==、!=、<、>）：
    bool operator==(const Type& rhs) const {
        return this->data == rhs.data;
    }
    bool operator!=(const Type& rhs) const {
        return !(*this == rhs);             // 复用 ==
    }

复合赋值（+=、-=、*=）：
    Type& operator+=(const Type& rhs) {
        data += rhs.data;
        return *this;                        // 返回引用
    }

前置自增：
    Type& operator++() { ++data; return *this; }

后置自增：
    Type operator++(int) {
        Type old = *this;
        ++(*this);                           // 复用前置 ++
        return old;
    }

流输出（非成员，friend）：
    friend ostream& operator<<(ostream& out, const Type& obj) {
        out << obj.data;
        return out;                          // 返回 ostream&（支持链式）
    }

────────────────────────────────────────────────────

【成员函数 vs 友元非成员函数选择】

用成员函数：左操作数是本类对象（p1 + p2，p1 == p2）
用友元非成员：左操作数不是本类（cout << p，2 * p）
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
1. 实现练习：为 Point 类添加以下运算符

   a) operator*（int n）：Point 乘以标量 n，返回新 Point（x*n, y*n）
   b) operator<（距离原点比较）：计算两点到原点的距离，返回 bool
   c) operator-=（复合减法）：类似 +=

   验证：
   Point p(2, 3);
   Point q = p * 3;    // (6, 9)
   Point r(1, 2);
   bool isSmaller = r < p;  // r 到原点距离 < p 到原点距离

------------------------------------------------------------

2. 设计题：为 Vector2D（二维向量）类实现完整的运算符

   class Vector2D {
       double x, y;
   public:
       Vector2D(double x, double y);
       // TODO：实现以下运算符：
       // + - 加减（向量加减）
       // * double（标量乘法：v * 2.0）
       // == != 比较
       // << 输出（friend）
       // 前置 ++ 和后置 ++（x 和 y 各加1）
   };

   提示：dot product（点积）= x1*x2 + y1*y2，可以作为额外挑战

------------------------------------------------------------

3. 排错练习：找出以下运算符实现的所有问题

   class Money {
       int cents;
   public:
       Money(int c) : cents(c) {}

       Money operator+(const Money& other) {   // 问题①
           cents += other.cents;               // 问题②
           return *this;                       // 问题③
       }
       Money operator++(int) {                 // 问题④
           return Money(++cents);
       }
   };

   提示：①没有const，②修改了this，③应返回新对象，④后置++语义错误

------------------------------------------------------------

4. 思考题：
   以下运算符，哪些适合重载？哪些不适合？
   a) 复数类的 + 运算符
   b) 字符串类的 + 运算符（拼接）
   c) 颜色类的 * 运算符
   d) 学生类的 > 运算符（比较成绩）
   e) 汽车类的 + 运算符

   判断原则：运算符的含义是否"自然"、"直觉"？
)";
}

// ============================================
// main 函数
// ============================================

int main() {
    cout << string(60, '=') << endl;
    cout << " 51_operator_overloading_basics.cpp - 运算符重载基础" << endl;
    cout << string(60, '=') << endl;

    demoBasicOperators();
    demoIncrementOperators();
    showCommonErrors();
    showFunctionCards();
    showExercises();

    cout << "\n" << string(60, '=') << endl;
    cout << "核心要点总结" << endl;
    cout << string(60, '=') << endl;
    cout << "1. operator+：const 成员函数，返回新对象（不修改 this）" << endl;
    cout << "2. operator+=：非 const，返回 *this 引用（支持链式）" << endl;
    cout << "3. operator==：加 const，返回 bool" << endl;
    cout << "4. 前置 ++ 返回引用；后置 ++ 返回旧值副本" << endl;
    cout << "5. 只重载语义自然的运算符，不为了复杂而复杂" << endl;

    return 0;
}
