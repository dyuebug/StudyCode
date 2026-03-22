// ============================================
// 析构函数基础 - 超详细注释版
// 日期：2026-03-18（第12天）
// 难度：⭐⭐ 中级
// ============================================

/*
【核心概念】析构函数（Destructor）

是什么？
- 析构函数是类的特殊成员函数，用于清理对象占用的资源
- 对象销毁时自动调用，无需手动调用

为什么需要？
- 释放动态分配的内存，防止内存泄漏
- 关闭打开的文件
- 释放其他资源（网络连接、数据库连接等）

核心特点：
- 函数名是类名前加~符号
- 没有返回类型（连void都不写）
- 没有参数，不能重载
- 对象销毁时自动调用
*/

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

#include <iostream>
#include <string>
using namespace std;

int main() {
    cout << string(60, '=') << endl;
    cout << "第一部分：核心概念详解 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 1.1 概念1：什么是析构函数
    // --------------------------------------------

    cout << "\n1.1 什么是析构函数？" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【析构函数（Destructor）】

是什么？
- 析构函数是类的特殊成员函数
- 用于在对象销毁时清理资源

特点：
1. 函数名是类名前加~符号（例如：~ClassName）
2. 没有返回类型（连void都不写）
3. 没有参数，不能重载（一个类只能有一个析构函数）
4. 对象销毁时自动调用
5. 如果没有定义，编译器会自动生成一个默认析构函数

语法：
class ClassName {
public:
    ~ClassName() {  // 析构函数
        // 清理资源的代码
    }
};

调用时机：
- 对象生命周期结束时自动调用
- 局部对象：函数结束时
- 动态对象：delete时
- 全局对象：程序结束时
)" << endl;

    // --------------------------------------------
    // 1.2 概念2：对象的生命周期
    // --------------------------------------------

    cout << "\n1.2 对象的生命周期" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【对象的生命周期】

完整流程：
1. 创建对象 → 调用构造函数
2. 使用对象 → 调用成员函数
3. 销毁对象 → 调用析构函数

不同类型对象的生命周期：

1. 局部对象（栈对象）
   - 创建：进入作用域时
   - 销毁：离开作用域时
   - 例如：函数内定义的对象

2. 动态对象（堆对象）
   - 创建：new时
   - 销毁：delete时
   - 必须手动delete，否则内存泄漏

3. 全局对象
   - 创建：程序开始时
   - 销毁：程序结束时

4. 静态对象
   - 创建：第一次使用时
   - 销毁：程序结束时
)" << endl;

    // --------------------------------------------
    // 1.3 概念3：资源管理
    // --------------------------------------------

    cout << "\n1.3 资源管理" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【资源管理（RAII）】

RAII = Resource Acquisition Is Initialization
资源获取即初始化

核心思想：
- 构造函数中获取资源
- 析构函数中释放资源
- 利用对象生命周期自动管理资源

常见资源：
1. 内存：new/delete
2. 文件：打开/关闭
3. 网络连接：建立/断开
4. 数据库连接：打开/关闭
5. 互斥锁：加锁/解锁

优势：
- 自动管理，不会忘记释放
- 异常安全（即使发生异常，析构函数也会被调用）
- 代码更简洁

示例：
class FileHandler {
public:
    FileHandler(string filename) {
        // 构造函数：打开文件
    }
    ~FileHandler() {
        // 析构函数：关闭文件
    }
};
)" << endl;

    return 0;
}

// ============================================
// 第二部分：基础操作实践
// ============================================

#include <iostream>
#include <string>
using namespace std;

// --------------------------------------------
// 2.1 示例1：简单的析构函数
// --------------------------------------------

// 定义一个简单的类，演示析构函数的调用
class SimpleClass {
private:
    string name;

public:
    // 构造函数
    SimpleClass(string n) : name(n) {
        cout << "构造函数：创建对象 " << name << endl;
    }

    // 析构函数
    ~SimpleClass() {
        cout << "析构函数：销毁对象 " << name << endl;
    }
};

// --------------------------------------------
// 2.2 示例2：动态内存管理
// --------------------------------------------

// 定义一个动态数组类，演示资源管理
class DynamicArray {
private:
    int* data;      // 指向动态数组的指针
    int size;       // 数组大小

public:
    // 构造函数：分配内存
    DynamicArray(int s) : size(s) {
        data = new int[size];  // 动态分配内存
        cout << "构造函数：分配 " << size << " 个整数的内存" << endl;

        // 初始化数组为0
        for (int i = 0; i < size; i++) {
            data[i] = 0;
        }
    }

    // 析构函数：释放内存
    ~DynamicArray() {
        cout << "析构函数：释放内存" << endl;
        delete[] data;  // 释放动态分配的内存
        data = nullptr;  // 防止悬空指针
    }

    // 设置数组元素
    void set(int index, int value) {
        if (index >= 0 && index < size) {
            data[index] = value;
        }
    }

    // 获取数组元素
    int get(int index) {
        if (index >= 0 && index < size) {
            return data[index];
        }
        return -1;
    }

    // 显示数组
    void display() {
        cout << "数组内容：";
        for (int i = 0; i < size; i++) {
            cout << data[i] << " ";
        }
        cout << endl;
    }
};

// --------------------------------------------
// 2.3 示例3：对象生命周期演示
// --------------------------------------------

// 定义一个类，演示不同作用域的对象生命周期
class LifeCycleDemo {
private:
    string name;
    int id;

public:
    // 构造函数
    LifeCycleDemo(string n, int i) : name(n), id(i) {
        cout << "[" << id << "] 构造：" << name << endl;
    }

    // 析构函数
    ~LifeCycleDemo() {
        cout << "[" << id << "] 析构：" << name << endl;
    }

    void display() {
        cout << "[" << id << "] 使用：" << name << endl;
    }
};

// 全局对象
LifeCycleDemo globalObj("全局对象", 1);

// 测试函数
void testFunction() {
    cout << "\n进入testFunction函数" << endl;

    // 局部对象
    LifeCycleDemo localObj("局部对象", 2);
    localObj.display();

    cout << "离开testFunction函数" << endl;
    // 函数结束时，localObj自动销毁，调用析构函数
}

