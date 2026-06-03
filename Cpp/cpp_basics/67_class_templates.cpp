// ============================================
// 类模板基础 - 超详细注释版
// 日期：2026-05-07（第21天）
// 难度：⭐⭐⭐ 中高级
// ============================================

/*
【核心概念】类模板

是什么？
- 类模板是生成类的"模具"，用一个模板定义可以生成多个不同类型的类

为什么需要？
- 避免为每种类型重复编写相同逻辑的类
- STL 的所有容器（vector、map、set 等）都是类模板
- 提供类型安全的泛型数据结构

核心特点：
- 编译期实例化：编译时根据使用的类型生成具体类
- 类型安全：编译器检查类型是否支持模板中的操作
- 代码复用：一个模板可以用于多种类型
*/

#include <iostream>
#include <string>
using namespace std;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

void part1_concepts() {
    cout << string(60, '=') << endl;
    cout << "第一部分：核心概念详解" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【为什么需要类模板？】

问题：为每种类型写重复的类
class IntStack { int data[100]; ... };
class DoubleStack { double data[100]; ... };
class StringStack { string data[100]; ... };

解决：用类模板一次编写，多种类型复用
template<typename T>
class Stack { T data[100]; ... };

使用：
Stack<int> intStack;
Stack<double> doubleStack;
Stack<string> stringStack;

【类模板语法】

template<typename T>
class 类名 {
    T member;
public:
    void func(T param);
};

【类模板实例化】

必须显式指定类型：
Box<int> intBox(10);     // T = int
Box<string> strBox("hi"); // T = string

【成员函数实现】

类内定义：
template<typename T>
class Box {
    T value;
public:
    void setValue(T v) { value = v; }  // 类内定义
};

类外定义：
template<typename T>
void Box<T>::setValue(T v) {  // 注意 Box<T>::
    value = v;
}
)" << endl;
}

// ============================================
// 第二部分：基础操作实践
// ============================================

// 示例1：简单的类模板（Box）
template<typename T>
class Box {
private:
    T value;

public:
    Box(T v) : value(v) {}

    void setValue(T v) { value = v; }
    T getValue() const { return value; }

    void print() const {
        cout << "Box contains: " << value << endl;
    }
};

// 示例2：类模板实现栈（Stack）
template<typename T>
class Stack {
private:
    T data[100];
    int top;

public:
    Stack() : top(0) {}

    void push(T value) {
        if (top < 100) {
            data[top++] = value;
        }
    }

    T pop() {
        if (top > 0) {
            return data[--top];
        }
        return T();
    }

    bool isEmpty() const { return top == 0; }
    int size() const { return top; }
};

// 示例3：类模板实现键值对（Pair）
template<typename T1, typename T2>
class Pair {
private:
    T1 first;
    T2 second;

public:
    Pair(T1 f, T2 s) : first(f), second(s) {}

    T1 getFirst() const { return first; }
    T2 getSecond() const { return second; }

    void print() const {
        cout << "(" << first << ", " << second << ")" << endl;
    }
};

// 示例4：类模板的默认参数
template<typename T = int>
class Counter {
private:
    T count;

public:
    Counter(T initial = 0) : count(initial) {}

    void increment() { count++; }
    T getCount() const { return count; }
};

void part2_practice() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分：基础操作实践" << endl;
    cout << string(60, '=') << endl;

    // 示例1：Box
    cout << "\n示例1：Box" << endl;
    Box<int> intBox(42);
    intBox.print();

    Box<string> strBox("Hello");
    strBox.print();

    // 示例2：Stack
    cout << "\n示例2：Stack" << endl;
    Stack<int> intStack;
    intStack.push(10);
    intStack.push(20);
    intStack.push(30);

    cout << "Stack size: " << intStack.size() << endl;
    cout << "Pop: " << intStack.pop() << endl;
    cout << "Pop: " << intStack.pop() << endl;

    // 示例3：Pair
    cout << "\n示例3：Pair（多个模板参数）" << endl;
    Pair<string, int> person("Alice", 25);
    cout << "Person: ";
    person.print();

    Pair<int, double> point(10, 3.14);
    cout << "Point: ";
    point.print();

    // 示例4：Counter（默认参数）
    cout << "\n示例4：Counter（默认参数）" << endl;
    Counter<> counter1;  // 使用默认类型 int
    counter1.increment();
    counter1.increment();
    cout << "Counter1: " << counter1.getCount() << endl;

    Counter<double> counter2(3.5);
    counter2.increment();
    cout << "Counter2: " << counter2.getCount() << endl;
}

// ============================================
// 第三部分：深入理解（选学）
// ============================================

void part3_advanced() {
    cout << "\n" << string(60, '=') << endl;
    cout << "🔍 深入理解：类模板特化（选学）" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【什么是模板特化？】

为特定类型提供专门的实现。

【全特化示例】

// 通用模板
template<typename T>
class Printer {
public:
    void print(T value) { cout << value << endl; }
};

// 为 bool 类型特化
template<>
class Printer<bool> {
public:
    void print(bool value) {
        cout << (value ? "true" : "false") << endl;
    }
};

【偏特化示例】

// 为指针类型偏特化
template<typename T>
class Container<T*> {
    T* ptr;
public:
    T& operator*() { return *ptr; }
};

【何时使用特化？】

1. 某些类型需要特殊处理（如 bool、指针）
2. 优化特定类型的性能
3. 为特定类型提供额外功能

【STL 中的特化】

vector<bool> 就是一个特化版本，使用位压缩节省空间。
)" << endl;
}

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void part4_mistakes() {
    cout << "\n" << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【错误1：忘记指定模板参数】

❌ 错误：
template<typename T>
class Box { T value; };

Box box(10);  // 编译错误！

✅ 正确：
Box<int> box(10);  // 必须显式指定 T = int

────────────────────────────────────

【错误2：类外定义成员函数时忘记模板声明】

❌ 错误：
template<typename T>
class Box {
    void setValue(T v);
};

void Box::setValue(T v) { ... }  // 错误！

✅ 正确：
template<typename T>
void Box<T>::setValue(T v) { ... }  // 正确！

────────────────────────────────────

【错误3：模板类的静态成员】

说明：每个模板实例都有自己的静态成员

template<typename T>
class Counter {
    static int count;
};

template<typename T>
int Counter<T>::count = 0;  // 必须在类外定义

Counter<int> 和 Counter<double> 有各自独立的 count

────────────────────────────────────

【错误4：模板参数依赖的类型名】

❌ 错误：
template<typename T>
class Container {
    void func() {
        T::iterator it;  // 错误！
    }
};

✅ 正确：
template<typename T>
class Container {
    void func() {
        typename T::iterator it;  // 用 typename 声明这是类型
    }
};

────────────────────────────────────

【错误5：模板类的继承】

说明：继承模板类时要注意

template<typename T>
class Base { protected: T value; };

template<typename T>
class Derived : public Base<T> {
    void func() {
        this->value = 10;  // 必须用 this-> 或 Base<T>::
        // value = 10;     // 错误！找不到 value
    }
};
)" << endl;
}

// ============================================
// 第五部分：类模板速查
// ============================================

void part5_reference() {
    cout << "\n" << string(60, '=') << endl;
    cout << "类模板速查" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【类模板基本语法】

template<typename T>
class 类名 {
    T member;
public:
    void func(T param);
};

使用：类名<类型> 对象名;

────────────────────────────────────

【类外定义成员函数】

template<typename T>
返回类型 类名<T>::函数名(参数) {
    函数体
}

────────────────────────────────────

【多个模板参数】

template<typename T1, typename T2>
class Pair {
    T1 first;
    T2 second;
};

────────────────────────────────────

【默认模板参数】

template<typename T = int>
class Counter { T count; };

Counter<> c1;      // 使用默认类型 int
Counter<double> c2; // 显式指定 double
)" << endl;
}

// ============================================
// 练习题
// ============================================

void exercises() {
    cout << "\n" << string(60, '=') << endl;
    cout << "练习题" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
1. 实现类模板 Array
   要求：固定大小的数组，支持 [] 访问、size() 方法
   提示：用数组存储元素，记录大小

2. 实现类模板 Queue
   要求：队列数据结构，支持 enqueue、dequeue、front
   提示：用数组实现，需要 front 和 rear 两个索引

3. 实现类模板 SmartPointer
   要求：简单的智能指针，构造时分配内存，析构时释放
   提示：用 new 分配，delete 释放，重载 * 和 -> 运算符

4. 实现类模板 Optional
   要求：可选值容器，可能有值也可能没有
   提示：用 bool 标记是否有值，提供 hasValue()、getValue() 方法

5. 实现类模板 LinkedList
   要求：单链表，支持 push_front、pop_front、size
   提示：定义节点结构，用指针连接

6. 实现类模板 BinaryTree
   要求：二叉树，支持插入、查找、遍历
   提示：定义节点结构，递归实现操作

7. 实现类模板 Matrix
   要求：二维矩阵，支持加法、乘法
   提示：用二维数组存储，重载 + 和 * 运算符
)" << endl;
}

// ============================================
// 主函数
// ============================================

int main() {
    cout << "类模板基础学习" << endl;
    cout << string(60, '=') << endl;

    part1_concepts();
    part2_practice();
    part3_advanced();
    part4_mistakes();
    part5_reference();
    exercises();

    cout << "\n" << string(60, '=') << endl;
    cout << "学习完成！" << endl;
    cout << string(60, '=') << endl;

    return 0;
}
