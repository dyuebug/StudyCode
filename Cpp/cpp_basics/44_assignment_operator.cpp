// 44_assignment_operator.cpp
// 主题：赋值运算符重载与三法则
// 学习目标：掌握赋值运算符重载、自赋值检查、三法则

#include <iostream>
#include <cstring>
using namespace std;

// ============================================================
// 第一部分：赋值运算符 vs 拷贝构造函数
// ============================================================
/*
    关键区分：
    MyString a = b;   // 拷贝构造函数（a 是新对象，用 b 初始化）
    MyString a;       // 默认构造函数
    a = b;            // 赋值运算符（a 已存在，用 b 赋值）

    赋值运算符特点：
    - 左侧对象已存在（需要先释放旧资源）
    - 需要检查自赋值（a = a）
    - 返回 *this 以支持链式赋值（a = b = c）
*/

// ============================================================
// 第二部分：默认赋值运算符的问题（浅拷贝）
// ============================================================

// 演示用，不实际触发 double free
void demoAssignProblem() {
    cout << "\n=== 赋值运算符 vs 拷贝构造函数 ===" << endl;
    cout << "MyString a = b; → 拷贝构造函数（a 是新对象）" << endl;
    cout << "a = b;          → 赋值运算符（a 已存在）" << endl;
    cout << "默认赋值运算符：浅拷贝 → 同样有 double free 风险" << endl;
}

// ============================================================
// 第三部分：正确实现赋值运算符
// ============================================================

class MyString {
private:
    char* data;
    int   length;

public:
    // 构造函数
    MyString(const char* str = "") {
        length = strlen(str);
        data   = new char[length + 1];
        strcpy(data, str);
        cout << "构造：\"" << data << "\"" << endl;
    }

    // 拷贝构造函数（深拷贝）
    MyString(const MyString& other) {
        length = other.length;
        data   = new char[length + 1];
        strcpy(data, other.data);
        cout << "拷贝构造：\"" << data << "\"" << endl;
    }

    // 赋值运算符重载（深拷贝）
    MyString& operator=(const MyString& other) {
        cout << "赋值运算符：\"" << data << "\" = \"" << other.data << "\"" << endl;

        // 步骤1：自赋值检查（必须！）
        if (this == &other) {
            cout << "  自赋值，跳过" << endl;
            return *this;
        }

        // 步骤2：释放旧资源
        delete[] data;

        // 步骤3：分配新内存并复制
        length = other.length;
        data   = new char[length + 1];
        strcpy(data, other.data);

        // 步骤4：返回 *this（支持链式赋值）
        return *this;
    }

    // 析构函数
    ~MyString() {
        cout << "析构：\"" << data << "\"" << endl;
        delete[] data;
    }

    void show(const string& label = "") const {
        if (!label.empty()) cout << label << ": ";
        cout << "\"" << data << "\"" << endl;
    }

    void setFirst(char c) {
        if (length > 0) data[0] = c;
    }
};

void demoAssignOperator() {
    cout << "\n=== 赋值运算符重载 ===" << endl;

    MyString s1("Hello");
    MyString s2("World");

    cout << "\n--- 普通赋值 s2 = s1 ---" << endl;
    s2 = s1;  // 调用赋值运算符
    s1.show("s1");
    s2.show("s2");

    // 修改 s2 不影响 s1（深拷贝）
    s2.setFirst('J');
    cout << "修改 s2 首字符后：" << endl;
    s1.show("s1");
    s2.show("s2");
}

void demoSelfAssign() {
    cout << "\n=== 自赋值检查 ===" << endl;
    MyString s("Test");
    s = s;  // 自赋值，必须安全处理
    s.show("自赋值后");
}

void demoChainAssign() {
    cout << "\n=== 链式赋值 a = b = c ===" << endl;
    MyString a("AAA");
    MyString b("BBB");
    MyString c("CCC");

    a = b = c;  // 先执行 b = c，再执行 a = (b=c的结果)
    a.show("a");
    b.show("b");
    c.show("c");
}

// ============================================================
// 第四部分：三法则（Rule of Three）
// ============================================================
/*
    如果一个类需要自定义以下任意一个，通常三个都需要自定义：

    1. 析构函数         ~ClassName()
    2. 拷贝构造函数     ClassName(const ClassName& other)
    3. 赋值运算符       ClassName& operator=(const ClassName& other)

    原因：这三个函数都涉及资源管理（内存、文件、连接等）
    如果需要自定义析构函数释放资源，说明类管理了资源，
    那么拷贝和赋值也需要正确处理这些资源。

    C++11 扩展为五法则（Rule of Five）：
    额外加上移动构造函数和移动赋值运算符（进阶内容）
*/

// 遵守三法则的完整示例
class Buffer {
private:
    int* data;
    int  size;

public:
    // 1. 构造函数
    Buffer(int sz, int fillVal = 0) : size(sz) {
        data = new int[size];
        for (int i = 0; i < size; ++i) data[i] = fillVal;
        cout << "Buffer 构造，大小: " << size << endl;
    }

    // 2. 析构函数（管理资源 → 触发三法则）
    ~Buffer() {
        cout << "Buffer 析构，大小: " << size << endl;
        delete[] data;
    }

    // 3. 拷贝构造函数
    Buffer(const Buffer& other) : size(other.size) {
        data = new int[size];
        for (int i = 0; i < size; ++i) data[i] = other.data[i];
        cout << "Buffer 拷贝构造，大小: " << size << endl;
    }

    // 4. 赋值运算符
    Buffer& operator=(const Buffer& other) {
        if (this == &other) return *this;  // 自赋值检查

        delete[] data;          // 释放旧资源
        size = other.size;
        data = new int[size];
        for (int i = 0; i < size; ++i) data[i] = other.data[i];
        cout << "Buffer 赋值，大小: " << size << endl;
        return *this;
    }

    void fill(int val) {
        for (int i = 0; i < size; ++i) data[i] = val;
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

void demoRuleOfThree() {
    cout << "\n=== 三法则（Rule of Three）完整示例 ===" << endl;

    Buffer b1(3, 1);
    Buffer b2(b1);       // 拷贝构造
    Buffer b3(4, 0);
    b3 = b1;             // 赋值运算符

    b1.show("b1");
    b2.show("b2");
    b3.show("b3");

    b2.fill(99);         // 修改 b2
    b1.show("b1（b2修改后，不受影响）");
}

// ============================================================
// main 函数
// ============================================================

int main() {
    cout << "========================================" << endl;
    cout << "      赋值运算符重载与三法则" << endl;
    cout << "========================================" << endl;

    demoAssignProblem();
    demoAssignOperator();
    demoSelfAssign();
    demoChainAssign();
    demoRuleOfThree();

    cout << "\n=== 核心总结 ===" << endl;
    cout << "1. 赋值运算符：MyString& operator=(const MyString& other)" << endl;
    cout << "2. 四个步骤：自赋值检查 → 释放旧资源 → 深拷贝 → return *this" << endl;
    cout << "3. 三法则：析构/拷贝构造/赋值运算符，定义一个通常需三个" << endl;
    cout << "4. 链式赋值：返回 *this 引用实现" << endl;

    return 0;
}
