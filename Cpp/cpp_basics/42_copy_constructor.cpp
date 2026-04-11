// 42_copy_constructor.cpp
// 主题：拷贝构造函数
// 学习目标：理解拷贝构造函数的概念、调用时机和实现方式

#include <iostream>
#include <cstring>
using namespace std;

// ============================================================
// 第一部分：什么是拷贝构造函数
// ============================================================
/*
    拷贝构造函数（Copy Constructor）：
    - 用已有对象初始化新对象时自动调用
    - 参数是同类型对象的常量引用：ClassName(const ClassName& other)
    - 如果不定义，编译器自动生成（执行浅拷贝）

    调用时机（三种情况）：
    1. 用已有对象初始化新对象：MyClass b = a;
    2. 函数参数按值传递：void func(MyClass obj)
    3. 函数返回值（部分情况）：return obj;
*/

// ============================================================
// 第二部分：默认拷贝构造函数（浅拷贝）
// ============================================================

class SimplePoint {
public:
    int x, y;

    // 普通构造函数
    SimplePoint(int x, int y) : x(x), y(y) {
        cout << "构造函数：Point(" << x << ", " << y << ")" << endl;
    }

    // 没有定义拷贝构造函数 → 编译器自动生成（逐成员拷贝）

    void show() const {
        cout << "Point(" << x << ", " << y << ")" << endl;
    }
};

void demoDefaultCopy() {
    cout << "\n=== 默认拷贝构造函数（编译器自动生成）===" << endl;

    SimplePoint p1(3, 4);
    SimplePoint p2 = p1;  // 调用拷贝构造函数
    SimplePoint p3(p1);   // 同上，另一种写法

    p1.show();
    p2.show();
    p3.show();

    // 修改 p2 不影响 p1（因为 int 是值类型）
    p2.x = 100;
    cout << "修改 p2.x = 100 后：" << endl;
    cout << "p1: "; p1.show();
    cout << "p2: "; p2.show();
}

// ============================================================
// 第三部分：自定义拷贝构造函数
// ============================================================

class MyString {
private:
    char* data;    // 指向堆内存
    int   length;

public:
    // 普通构造函数
    MyString(const char* str = "") {
        length = strlen(str);
        data   = new char[length + 1];
        strcpy(data, str);
        cout << "构造：\"" << data << "\"" << endl;
    }

    // 拷贝构造函数（深拷贝）
    MyString(const MyString& other) {
        length = other.length;
        data   = new char[length + 1];  // 分配新内存
        strcpy(data, other.data);       // 复制内容
        cout << "拷贝构造：\"" << data << "\"" << endl;
    }

    // 析构函数
    ~MyString() {
        cout << "析构：\"" << data << "\"" << endl;
        delete[] data;
    }

    void show() const {
        cout << "MyString: \"" << data << "\", 长度: " << length << endl;
    }

    // 修改内容（用于演示独立性）
    void setFirst(char c) {
        if (length > 0) data[0] = c;
    }
};

void demoCustomCopy() {
    cout << "\n=== 自定义拷贝构造函数（深拷贝）===" << endl;

    MyString s1("Hello");
    MyString s2 = s1;  // 调用拷贝构造函数

    cout << "拷贝后：" << endl;
    s1.show();
    s2.show();

    // 修改 s2 不影响 s1（深拷贝，内存独立）
    s2.setFirst('J');
    cout << "修改 s2 首字符为 'J' 后：" << endl;
    s1.show();
    s2.show();
}

// ============================================================
// 第四部分：拷贝构造函数的三种调用时机
// ============================================================

// 时机2：函数参数按值传递
void printString(MyString s) {  // 传入时调用拷贝构造
    cout << "函数内："; s.show();
}  // 离开函数时调用析构

// 时机3：函数返回值
MyString makeString() {
    MyString temp("World");
    return temp;  // 可能触发拷贝构造（取决于编译器优化RVO）
}

void demoCopyTiming() {
    cout << "\n=== 拷贝构造函数的三种调用时机 ===" << endl;

    cout << "\n[时机1] 用已有对象初始化新对象：" << endl;
    MyString a("Apple");
    MyString b = a;

    cout << "\n[时机2] 函数参数按值传递：" << endl;
    printString(a);

    cout << "\n[时机3] 函数返回值：" << endl;
    MyString c = makeString();
    c.show();
}

// ============================================================
// 第五部分：const 引用参数的重要性
// ============================================================
/*
    拷贝构造函数必须用引用参数：
    ✅ MyString(const MyString& other)   // 正确
    ❌ MyString(MyString other)          // 错误：会无限递归调用拷贝构造函数！

    原因：按值传参时，会调用拷贝构造函数，
          而拷贝构造函数的参数又需要按值传参，无限循环。

    加 const 的原因：
    - 被拷贝的原对象不应被修改
    - 允许拷贝 const 对象和临时对象
*/

// ============================================================
// main 函数
// ============================================================

int main() {
    cout << "========================================" << endl;
    cout << "      拷贝构造函数学习" << endl;
    cout << "========================================" << endl;

    demoDefaultCopy();
    demoCustomCopy();
    demoCopyTiming();

    cout << "\n=== 关键要点总结 ===" << endl;
    cout << "1. 拷贝构造函数：ClassName(const ClassName& other)" << endl;
    cout << "2. 编译器默认生成浅拷贝（逐成员复制）" << endl;
    cout << "3. 含有指针成员时必须自定义深拷贝" << endl;
    cout << "4. 参数必须是引用，否则无限递归" << endl;
    cout << "5. 调用时机：初始化/按值传参/返回值" << endl;

    return 0;
}
