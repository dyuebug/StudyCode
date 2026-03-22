// ============================================
// 构造函数与析构函数练习 - 超详细注释版
// 日期：2026-03-18（第12天）
// 难度：⭐⭐ 中级
// ============================================

/*
【核心概念】构造函数与析构函数的实战应用

是什么？
- 通过实际案例学习如何使用构造函数和析构函数管理资源
- 培养RAII（资源获取即初始化）思维

为什么需要？
- 理论结合实践，加深理解
- 学习资源管理的最佳实践
- 防止内存泄漏和资源泄漏

核心特点：
- 构造函数获取资源
- 析构函数释放资源
- 利用对象生命周期自动管理
*/

// ============================================
// 第一部分：动态字符串类
// ============================================

#include <iostream>
#include <cstring>
using namespace std;

// 定义一个动态字符串类
// 功能：管理动态分配的字符串内存
class MyString {
private:
    char* data;     // 指向字符串数据的指针
    int length;     // 字符串长度

public:
    // 构造函数1：默认构造函数
    // 功能：创建空字符串
    MyString() {
        length = 0;
        data = new char[1];  // 分配1个字节
        data[0] = '\0';      // 空字符串
        cout << "构造函数1：创建空字符串" << endl;
    }

    // 构造函数2：从C风格字符串创建
    // 参数：str - C风格字符串
    MyString(const char* str) {
        if (str == nullptr) {
            length = 0;
            data = new char[1];
            data[0] = '\0';
        } else {
            length = strlen(str);
            data = new char[length + 1];  // +1是为了存储'\0'
            strcpy(data, str);
        }
        cout << "构造函数2：创建字符串 \"" << data << "\"" << endl;
    }

    // 析构函数：释放内存
    ~MyString() {
        cout << "析构函数：释放字符串 \"" << data << "\" 的内存" << endl;
        delete[] data;
        data = nullptr;
    }

    // 获取字符串长度
    int getLength() const {
        return length;
    }

    // 获取C风格字符串
    const char* c_str() const {
        return data;
    }

    // 显示字符串信息
    void display() const {
        cout << "字符串：\"" << data << "\", 长度：" << length << endl;
    }

    // 追加字符串
    void append(const char* str) {
        if (str == nullptr) return;

        int newLength = length + strlen(str);
        char* newData = new char[newLength + 1];

        // 复制原字符串
        strcpy(newData, data);
        // 追加新字符串
        strcat(newData, str);

        // 释放旧内存
        delete[] data;

        // 更新成员变量
        data = newData;
        length = newLength;
    }
};

int main() {
    cout << string(60, '=') << endl;
    cout << "第一部分：动态字符串类" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 1.1 创建字符串对象
    // --------------------------------------------

    cout << "\n1.1 创建字符串对象" << endl;
    cout << string(60, '-') << endl;

    {
        // 使用默认构造函数
        MyString str1;
        str1.display();

        // 使用带参数的构造函数
        MyString str2("Hello");
        str2.display();

        MyString str3("World");
        str3.display();

        // 离开作用域：自动调用析构函数，释放内存
    }
    cout << "\n所有字符串对象已销毁" << endl;

    // --------------------------------------------
    // 1.2 字符串操作
    // --------------------------------------------

    cout << "\n1.2 字符串操作" << endl;
    cout << string(60, '-') << endl;

    {
        MyString str("Hello");
        str.display();

        cout << "\n追加字符串：" << endl;
        str.append(" World");
        str.display();

        str.append("!");
        str.display();
    }
    cout << "\n字符串对象已销毁" << endl;

    return 0;
}

// ============================================
// 第二部分：智能数组类
// ============================================

#include <iostream>
using namespace std;

// 定义一个智能数组类
// 功能：自动管理动态数组内存，提供边界检查
class SmartArray {
private:
    int* data;      // 指向数组数据的指针
    int capacity;   // 数组容量
    int size;       // 当前元素个数

public:
    // 构造函数：创建指定容量的数组
    // 参数：cap - 数组容量
    SmartArray(int cap = 10) : capacity(cap), size(0) {
        data = new int[capacity];
        cout << "构造函数：创建容量为 " << capacity << " 的数组" << endl;

        // 初始化为0
        for (int i = 0; i < capacity; i++) {
            data[i] = 0;
        }
    }

    // 析构函数：释放内存
    ~SmartArray() {
        cout << "析构函数：释放数组内存" << endl;
        delete[] data;
        data = nullptr;
    }

    // 添加元素
    // 参数：value - 要添加的值
    // 返回：bool - 是否添加成功
    bool push(int value) {
        if (size >= capacity) {
            cout << "错误：数组已满" << endl;
            return false;
        }

        data[size] = value;
        size++;
        return true;
    }

    // 获取元素
    // 参数：index - 索引
    // 返回：元素值（如果索引无效返回-1）
    int get(int index) const {
        if (index < 0 || index >= size) {
            cout << "错误：索引越界" << endl;
            return -1;
        }
        return data[index];
    }

    // 设置元素
    // 参数：index - 索引，value - 值
    // 返回：bool - 是否设置成功
    bool set(int index, int value) {
        if (index < 0 || index >= size) {
            cout << "错误：索引越界" << endl;
            return false;
        }
        data[index] = value;
        return true;
    }

    // 获取当前元素个数
    int getSize() const {
        return size;
    }

    // 获取容量
    int getCapacity() const {
        return capacity;
    }

    // 显示数组
    void display() const {
        cout << "数组内容（" << size << "/" << capacity << "）：";
        for (int i = 0; i < size; i++) {
            cout << data[i] << " ";
        }
        cout << endl;
    }
};

