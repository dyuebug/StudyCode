// 43_deep_shallow_copy.cpp
// 主题：深拷贝与浅拷贝
// 学习目标：理解浅拷贝的危险性，掌握深拷贝的正确实现

#include <iostream>
#include <cstring>
using namespace std;

// ============================================================
// 第一部分：浅拷贝的问题 —— double free 崩溃
// ============================================================
/*
    浅拷贝（Shallow Copy）：
    - 逐成员复制，指针成员只复制地址（两个对象共享同一块堆内存）
    - 问题：一个对象析构时释放了内存，另一个对象析构时再次释放 → double free → 崩溃
*/

class ShallowString {
public:
    char* data;
    int   length;

    ShallowString(const char* str) {
        length = strlen(str);
        data   = new char[length + 1];
        strcpy(data, str);
        cout << "构造(" << (void*)data << ")：\"" << data << "\"" << endl;
    }

    // 没有自定义拷贝构造函数 → 编译器生成浅拷贝
    // ShallowString(const ShallowString& other) {
    //     data   = other.data;    // 只复制指针地址！
    //     length = other.length;
    // }

    ~ShallowString() {
        cout << "析构(" << (void*)data << ")：\"" << data << "\"" << endl;
        delete[] data;  // 危险：可能 double free！
    }
};

void demoShallowProblem() {
    cout << "\n=== 浅拷贝的问题（已注释析构，防止程序崩溃）===" << endl;
    cout << "原理演示：两个指针指向同一块内存" << endl;

    // 注意：直接运行会导致 double free 崩溃
    // 这里只演示指针地址相同的问题
    ShallowString* s1 = new ShallowString("Hello");

    // 模拟浅拷贝效果
    ShallowString* s2 = new ShallowString("Hello");
    delete[] s2->data;
    s2->data = s1->data;  // 两个对象共享同一块内存！

    cout << "s1->data 地址: " << (void*)s1->data << endl;
    cout << "s2->data 地址: " << (void*)s2->data << endl;
    cout << "地址相同: " << (s1->data == s2->data ? "是（危险！）" : "否") << endl;

    // 手动清理，避免 double free
    s2->data = nullptr;  // 防止析构时 double free
    delete s1;
    delete s2;
}

// ============================================================
// 第二部分：深拷贝的正确实现
// ============================================================

class DeepString {
private:
    char* data;
    int   length;

public:
    // 构造函数
    DeepString(const char* str = "") {
        length = strlen(str);
        data   = new char[length + 1];
        strcpy(data, str);
        cout << "构造(" << (void*)data << ")：\"" << data << "\"" << endl;
    }

    // 深拷贝构造函数
    DeepString(const DeepString& other) {
        length = other.length;
        data   = new char[length + 1];  // 分配新内存（关键！）
        strcpy(data, other.data);       // 复制内容
        cout << "深拷贝(" << (void*)data << ")：\"" << data << "\"" << endl;
    }

    // 析构函数
    ~DeepString() {
        cout << "析构(" << (void*)data << ")：\"" << data << "\"" << endl;
        delete[] data;
    }

    void show() const {
        cout << "DeepString(" << (void*)data << "): \"" << data << "\"" << endl;
    }

    void setFirst(char c) {
        if (length > 0) data[0] = c;
    }
};

void demoDeepCopy() {
    cout << "\n=== 深拷贝（内存完全独立）===" << endl;

    DeepString s1("Hello");
    DeepString s2 = s1;  // 调用深拷贝构造函数

    cout << "拷贝后地址对比：" << endl;
    s1.show();
    s2.show();

    // 修改 s2 不影响 s1
    s2.setFirst('J');
    cout << "修改 s2 首字符后：" << endl;
    s1.show();
    s2.show();
    // 两个对象各自释放自己的内存，无 double free
}

// ============================================================
// 第三部分：何时需要深拷贝
// ============================================================
/*
    需要深拷贝的情况：
    ✅ 类中含有裸指针（指向堆内存）
    ✅ 类中管理文件句柄、网络连接等外部资源
    ✅ 类的析构函数会释放资源

    不需要深拷贝的情况：
    ✅ 类中只有值类型成员（int、double、char 等）
    ✅ 类中只有标准库容器（string、vector 等已自带深拷贝）
    ✅ 类中只有智能指针（shared_ptr、unique_ptr）
*/

// 不需要深拷贝的示例（全是值类型）
class Rectangle {
public:
    double width, height;  // 值类型，浅拷贝安全

    Rectangle(double w, double h) : width(w), height(h) {}

    double area() const { return width * height; }
};

// 使用 std::string 自动深拷贝
class Person {
public:
    string name;  // std::string 内部已实现深拷贝
    int    age;

    Person(const string& n, int a) : name(n), age(a) {}

    void show() const {
        cout << "Person: " << name << ", " << age << endl;
    }
};

void demoWhenNeedDeep() {
    cout << "\n=== 何时需要深拷贝 ===" << endl;

    cout << "\n[不需要] 值类型成员：" << endl;
    Rectangle r1(3.0, 4.0);
    Rectangle r2 = r1;  // 浅拷贝完全安全
    r2.width = 10.0;
    cout << "r1 面积: " << r1.area() << "（未受影响）" << endl;
    cout << "r2 面积: " << r2.area() << endl;

    cout << "\n[不需要] std::string 已内置深拷贝：" << endl;
    Person p1("Alice", 25);
    Person p2 = p1;
    p2.name = "Bob";
    p1.show();  // 不受影响
    p2.show();

    cout << "\n[需要] 裸指针成员 → 必须自定义深拷贝（见 DeepString）" << endl;
}

// ============================================================
// 第四部分：深拷贝综合案例 —— 动态数组
// ============================================================

class DynamicArray {
private:
    int* data;
    int  size;

public:
    DynamicArray(int sz) : size(sz) {
        data = new int[size];
        for (int i = 0; i < size; ++i) data[i] = i * 10;
        cout << "构造 DynamicArray[" << size << "]" << endl;
    }

    // 深拷贝构造函数
    DynamicArray(const DynamicArray& other) : size(other.size) {
        data = new int[size];
        for (int i = 0; i < size; ++i) data[i] = other.data[i];
        cout << "深拷贝 DynamicArray[" << size << "]" << endl;
    }

    ~DynamicArray() {
        cout << "析构 DynamicArray[" << size << "]" << endl;
        delete[] data;
    }

    void set(int idx, int val) {
        if (idx >= 0 && idx < size) data[idx] = val;
    }

    void show(const string& label) const {
        cout << label << ": [";
        for (int i = 0; i < size; ++i) {
            cout << data[i];
            if (i < size - 1) cout << ", ";
        }
        cout << "]" << endl;
    }
};

void demoDynamicArray() {
    cout << "\n=== 深拷贝综合案例：动态数组 ===" << endl;

    DynamicArray a1(4);
    DynamicArray a2 = a1;  // 深拷贝

    a1.show("a1（拷贝前）");
    a2.show("a2（拷贝后）");

    a2.set(0, 999);  // 修改 a2
    a1.show("a1（a2修改后）");  // a1 不受影响
    a2.show("a2（修改后）");
}

// ============================================================
// main 函数
// ============================================================

int main() {
    cout << "========================================" << endl;
    cout << "      深拷贝与浅拷贝" << endl;
    cout << "========================================" << endl;

    demoShallowProblem();
    demoDeepCopy();
    demoWhenNeedDeep();
    demoDynamicArray();

    cout << "\n=== 核心总结 ===" << endl;
    cout << "浅拷贝：复制指针地址 → 共享内存 → double free 崩溃" << endl;
    cout << "深拷贝：分配新内存 + 复制内容 → 内存独立 → 安全" << endl;
    cout << "规则：类有裸指针成员 → 必须自定义深拷贝构造函数" << endl;

    return 0;
}
