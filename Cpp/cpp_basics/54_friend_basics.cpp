// ============================================
// 54_friend_basics.cpp - 超详细注释版
// 日期：第17天
// 难度：⭐⭐ 中级
// 主题：friend 友元详解 - 函数友元与类友元
// ============================================

/*
【核心概念】

是什么？
- friend（友元）：允许外部函数或另一个类访问本类的 private/protected 成员
- 打破了封装的访问限制，是一种有选择地开放权限的机制

为什么需要？
- 运算符重载（operator<<、operator>>）的左操作数不是本类，必须用友元
- 两个紧密协作的类需要互相访问私有成员（如迭代器访问容器内部）
- 工厂函数需要访问类的私有构造函数

核心特点：
- 友元声明在类内（关键字 friend），但友元函数本身是类外的普通函数
- 友元关系不可传递（A友元B，B友元C ≠ A友元C）
- 友元关系不可继承
- friend 声明的位置（public/private/protected）不影响访问权限
*/

#include <iostream>
#include <string>
using namespace std;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

/*
【函数卡片：friend 函数声明】

语法（在类内声明）：
    friend 返回类型 函数名(参数列表);
    friend class 类名;

规则：
    1. friend 声明写在类体内，可以在 public/private/protected 任意位置
    2. 友元函数本身不是类的成员函数，不能用 obj.func() 调用
    3. 友元函数体写在类外，不加 ClassName:: 前缀
    4. 友元类：A 内声明 friend class B → B 的所有成员函数都能访问 A 的私有成员

示例：
    class Box {
        double width;                           // private
    public:
        friend double getWidth(const Box& b);   // 友元函数声明
        friend class BoxPrinter;                // 友元类声明
    };

    double getWidth(const Box& b) {             // 友元函数定义（类外，无 Box::）
        return b.width;                         // 可以访问 private 成员 width
    }
*/

// ============================================
// 第二部分：友元函数基础示例
// ============================================

// ---- 2.1 友元函数：访问私有成员 ----

class BankAccount {
private:
    string owner;    // 账户持有人
    double balance;  // 余额（private，不对外直接暴露）

public:
    BankAccount(const string& owner, double balance)
        : owner(owner), balance(balance) {}

    // 普通成员函数：显示账户信息（不显示余额，保护隐私）
    void showInfo() const {
        cout << "账户持有人：" << owner << endl;
    }

    // 友元函数声明：允许 audit() 函数访问私有成员
    // 场景：审计函数需要直接访问 balance，但不应该是成员函数（它是独立的审计系统）
    friend void audit(const BankAccount& acc);

    // 友元函数声明：允许 transfer() 访问两个账户的私有成员
    // 参数有两个账户，无法做成成员函数（只有一个 this）
    friend void transfer(BankAccount& from, BankAccount& to, double amount);
};

// 友元函数定义（类外，无 BankAccount:: 前缀）
// 可以访问 acc.owner 和 acc.balance（private 成员）
void audit(const BankAccount& acc) {
    // 直接访问 private 成员 balance，普通外部函数无法做到
    cout << "[审计] " << acc.owner << " 的余额：" << acc.balance << endl;
}

// 友元函数：在两个账户间转账
// 需要同时访问两个账户的 private 成员
void transfer(BankAccount& from, BankAccount& to, double amount) {
    if (from.balance < amount) {
        cout << "转账失败：余额不足（余额=" << from.balance
             << "，转账=" << amount << "）" << endl;
        return;
    }
    from.balance -= amount;  // 访问 from 的私有 balance
    to.balance   += amount;  // 访问 to 的私有 balance
    cout << "转账成功：" << from.owner << " → " << to.owner
         << " 金额：" << amount << endl;
}

void demoBankAccount() {
    cout << string(60, '=') << endl;
    cout << "第二部分 2.1：友元函数访问私有成员" << endl;
    cout << string(60, '=') << endl;

    BankAccount alice("Alice", 1000.0);
    BankAccount bob("Bob", 500.0);

    cout << "\n--- 普通成员函数（不显示余额）---" << endl;
    alice.showInfo();

    cout << "\n--- 友元函数 audit()（可访问私有余额）---" << endl;
    audit(alice);
    audit(bob);

    cout << "\n--- 友元函数 transfer()（访问两个账户私有成员）---" << endl;
    transfer(alice, bob, 300.0);
    audit(alice);   // 转账后检查
    audit(bob);

    cout << "\n--- 余额不足的情况 ---" << endl;
    transfer(bob, alice, 1000.0);  // bob 只有 800
}

// ---- 2.2 友元运算符：operator<< 和 operator>> ----

class Temperature {
private:
    double celsius;  // 以摄氏度存储

public:
    explicit Temperature(double c = 0.0) : celsius(c) {}

    double toCelsius()    const { return celsius; }
    double toFahrenheit() const { return celsius * 9.0 / 5.0 + 32; }
    double toKelvin()     const { return celsius + 273.15; }

    // 友元 operator<<：输出格式 "25.0°C"
    // 必须是友元：左操作数是 ostream，不是 Temperature
    friend ostream& operator<<(ostream& out, const Temperature& t);

    // 友元 operator>>：从流中读取温度值
    friend istream& operator>>(istream& in, Temperature& t);

    // 友元加法：两个温度相加（物理上有意义时）
    friend Temperature operator+(const Temperature& a, const Temperature& b);
};

ostream& operator<<(ostream& out, const Temperature& t) {
    out << t.celsius << "°C"
        << " (" << t.toFahrenheit() << "°F"
        << " / " << t.toKelvin() << "K)";
    return out;  // 必须返回 out，支持链式 cout << t1 << t2
}

istream& operator>>(istream& in, Temperature& t) {
    in >> t.celsius;   // 直接读取私有成员
    return in;         // 返回 in 引用，支持链式 cin >> t1 >> t2
}

Temperature operator+(const Temperature& a, const Temperature& b) {
    return Temperature(a.celsius + b.celsius);  // 访问两者私有 celsius
}

void demoTemperature() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.2：友元运算符 << 和 >> " << endl;
    cout << string(60, '=') << endl;

    Temperature t1(25.0);
    Temperature t2(100.0);

    cout << "\n--- 友元 operator<< 输出 ---" << endl;
    cout << "t1 = " << t1 << endl;
    cout << "t2 = " << t2 << endl;

    cout << "\n--- 链式输出（依赖 operator<< 返回 ostream&）---" << endl;
    cout << "t1 = " << t1 << "\nt2 = " << t2 << endl;

    cout << "\n--- 友元 operator+ ---" << endl;
    Temperature t3 = t1 + t2;
    cout << "t1 + t2 = " << t3 << endl;
}

// ============================================
// 第三部分：友元类
// ============================================

// ---- 3.1 友元类：迭代器访问容器私有数据 ----

// 前向声明：LinkedList 使用 Iterator，Iterator 使用 LinkedList
class LinkedList;

class Iterator {
private:
    // 指向链表节点的指针（需要访问 LinkedList 的私有结构）
    struct Node* current;

public:
    Iterator(struct Node* node) : current(node) {}

    int value() const;       // 获取当前节点值
    void next();             // 移动到下一个节点
    bool hasNext() const;    // 是否还有下一个

    friend class LinkedList;  // LinkedList 可以访问 Iterator 的私有成员
};

// 简化版链表（展示友元类概念）
class LinkedList {
private:
    // 节点结构（private，外部不能直接使用）
    struct Node {
        int value;
        Node* next;
        Node(int v) : value(v), next(nullptr) {}
    };

    Node* head;  // 链表头节点（private）
    int   size;  // 链表大小（private）

public:
    LinkedList() : head(nullptr), size(0) {}

    // 析构：释放所有节点
    ~LinkedList() {
        Node* cur = head;
        while (cur) {
            Node* tmp = cur->next;
            delete cur;
            cur = tmp;
        }
    }

    // 在链表头部插入节点
    void push_front(int val) {
        Node* node = new Node(val);
        node->next = head;
        head = node;
        ++size;
    }

    int getSize() const { return size; }

    // 返回迭代器（从头节点开始）
    Iterator begin() const {
        return Iterator(head);  // 将私有的 head 传给 Iterator
    }

    // 声明 Iterator 为友元类
    // 这样 Iterator 的成员函数也可以访问 LinkedList::Node（虽然这里 Node 已可访问）
    friend class Iterator;
};

// Iterator 成员函数访问 LinkedList::Node 的私有成员
int  Iterator::value()   const { return current->value; }
void Iterator::next()          { current = current->next; }
bool Iterator::hasNext() const { return current != nullptr; }

void demoFriendClass() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第三部分：友元类示例" << endl;
    cout << string(60, '=') << endl;

    LinkedList list;
    list.push_front(30);
    list.push_front(20);
    list.push_front(10);

    cout << "链表大小：" << list.getSize() << endl;
    cout << "链表元素：";

    // 用迭代器遍历链表（迭代器访问了链表的私有 Node 结构）
    for (Iterator it = list.begin(); it.hasNext(); it.next()) {
        cout << it.value() << " ";
    }
    cout << endl;
}

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonErrors() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第四部分：常见错误和陷阱" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
❌ 错误1：把友元函数定义加上类名前缀

   class Box {
       friend double getWidth(const Box& b);
   };

   ✗ 错误：
   double Box::getWidth(const Box& b) { return b.width; }
   // 编译错误！友元函数不是成员函数，不能加 Box::

   ✅ 正确：友元函数定义不加类名前缀
   double getWidth(const Box& b) { return b.width; }

------------------------------------------------------------

❌ 错误2：以为 friend 声明位置影响访问权限

   class Box {
   private:
       friend double getWidth(const Box& b);  // 在 private 区域声明友元
   };

   // 误以为"在 private 里声明，外部无法访问此函数"
   // 实际上：friend 声明无论放在哪个区域，都授予相同的完全访问权限
   // 访问控制（public/private）不适用于 friend 声明本身

   ✅ 习惯：friend 声明通常放在 public 区域，清楚表明"开放给外部"

------------------------------------------------------------

❌ 错误3：以为友元关系可传递

   class A {
       friend class B;  // B 是 A 的友元
   };
   class B {
       friend class C;  // C 是 B 的友元
   };

   class C {
       void func(A& a) {
           // ❌ C 不是 A 的友元！a.privateData 无法访问
           // 友元关系不传递：A友B，B友C ≠ A友C
       }
   };

   ✅ 如果 C 需要访问 A 的私有成员，A 必须显式声明 friend class C

------------------------------------------------------------

❌ 错误4：以为友元关系可继承

   class Base {
       friend class Helper;
   };
   class Derived : public Base {};

   class Helper {
       void func(Derived& d) {
           // ❌ Helper 是 Base 的友元，不是 Derived 的友元
           // d.derivedPrivate 无法访问（derivedPrivate 是 Derived 的私有成员）
       }
   };

   ✅ 友元关系不继承，必须在 Derived 中重新声明 friend class Helper

------------------------------------------------------------

❌ 错误5：滥用友元破坏封装

   ✗ 错误做法：为每个需要访问私有数据的函数都声明友元
   class Student {
       int score;
   public:
       friend void print(const Student& s);    // 友元1
       friend void compare(const Student& a, const Student& b);  // 友元2
       friend void save(const Student& s);     // 友元3
       friend void load(Student& s);           // 友元4
       // ... 友元越来越多，封装完全破坏
   };

   ✅ 正确：评估是否真的需要友元
   - 如果外部只需要读取数据 → 提供 getter 即可，不需要友元
   - 如果确实需要直接访问多个私有成员 → 考虑友元
   - 典型合理友元场景：operator<<、operator>>、互相紧密协作的类对

------------------------------------------------------------

❌ 错误6：友元函数与成员函数混淆（调用方式）

   class Box {
       double width;
   public:
       void memberFunc() { ... }          // 成员函数：用 obj.memberFunc()
       friend void friendFunc(Box& b);    // 友元函数：用 friendFunc(obj)
   };

   Box b;
   b.memberFunc();     // ✅ 成员函数调用方式
   b.friendFunc(b);    // ❌ 编译错误！友元函数不是成员函数
   friendFunc(b);      // ✅ 友元函数调用方式（像普通函数一样）
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
【友元函数声明模板】

在类内声明（位置不影响权限）：
    class MyClass {
        friend 返回类型 函数名(参数);       // 友元普通函数
        friend class 其他类名;              // 友元类
        friend 返回类型 operator<<(ostream&, const MyClass&);  // 友元运算符
    };

友元函数定义（类外，无 MyClass:: 前缀）：
    返回类型 函数名(参数) {
        // 可以直接访问 MyClass 的 private/protected 成员
    }

────────────────────────────────────────────────────

【友元的合理使用场景】

✅ 推荐使用友元：
  1. operator<< / operator>>（左操作数是 ostream/istream）
  2. 对称二元运算符（operator+(A, B) 需访问两者私有成员）
  3. 紧密协作的类对（容器 + 迭代器）

❌ 避免滥用友元：
  - 只是为了读取单个成员 → 用 getter()
  - 大量函数都声明为友元 → 考虑重新设计封装

────────────────────────────────────────────────────

【友元关系的三不原则】
  不传递：A友B，B友C ≠ A友C
  不继承：A友B，Derived继承B ≠ A友Derived
  不对称：A声明B为友元，B不自动成为A的友元（除非B也声明A）
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
1. 实现练习：为 Point 类添加以下友元

   class Point {
       double x, y;
   public:
       Point(double x, double y);
       // TODO:
       // a) friend ostream& operator<<(ostream&, const Point&)
       //    输出格式：(3.14, 2.71)
       // b) friend istream& operator>>(istream&, Point&)
       //    从输入读取两个 double
       // c) friend double distance(const Point& a, const Point& b)
       //    计算两点距离：sqrt((x2-x1)² + (y2-y1)²)
   };

   验证：
   Point p1(0,0), p2(3,4);
   cout << distance(p1, p2) << endl;  // 5

------------------------------------------------------------

2. 友元类练习：实现 Rectangle + RectanglePrinter 友元类

   class Rectangle {
       double width, height;
       string color;
   public:
       Rectangle(double w, double h, string c);
       double area() const;
       friend class RectanglePrinter;   // 声明友元类
   };

   class RectanglePrinter {
   public:
       // 以 ASCII 艺术输出矩形（用 * 字符画边框）
       // 需要访问 Rectangle 的私有 width 和 height
       void printAscii(const Rectangle& r);
       // 输出详细信息（包括私有 color）
       void printDetails(const Rectangle& r);
   };

   提示：printAscii 用双重循环，判断是否是边框格子

------------------------------------------------------------

3. 思考题：以下情况用友元还是 getter？

   a) 有一个 Person 类，外部需要读取姓名（name）显示在界面上
   b) 有 Matrix 和 Vector 类，需要实现 mat * vec 矩阵向量乘法
   c) 有一个 Config 类，外部需要读取配置项做验证
   d) 有 Tree 和 TreeIterator 类，迭代器需要访问树的内部节点结构

   判断原则：访问是否只读？是否需要多个私有成员？是否紧密耦合？
)";
}

// ============================================
// main 函数
// ============================================

int main() {
    cout << string(60, '=') << endl;
    cout << " 54_friend_basics.cpp - friend 友元详解" << endl;
    cout << string(60, '=') << endl;

    demoBankAccount();
    demoTemperature();
    demoFriendClass();
    showCommonErrors();
    showFunctionCards();
    showExercises();

    cout << "\n" << string(60, '=') << endl;
    cout << "核心要点总结" << endl;
    cout << string(60, '=') << endl;
    cout << "1. friend 声明在类内，定义在类外，调用像普通函数" << endl;
    cout << "2. operator<< 必须用友元：左操作数是 ostream，不是本类" << endl;
    cout << "3. 友元关系：不传递、不继承、不对称" << endl;
    cout << "4. friend 位置（public/private）不影响访问权限" << endl;
    cout << "5. 避免滥用友元，只在真正需要时使用" << endl;

    return 0;
}
