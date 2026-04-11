// 55_static_members.cpp
// static 成员：静态成员变量、静态成员函数与类共享数据

#include <iostream>
#include <string>
using namespace std;

// ============================================================
// 第一部分：static 成员变量的基本概念
// ============================================================
// 普通成员变量属于对象，每个对象各有一份
// static 成员变量属于整个类，所有对象共享同一份数据

class Student {
private:
    string name;
    int id;
    static int totalStudents;  // 全类共享：记录创建过多少个学生对象

public:
    Student(const string& name) : name(name) {
        totalStudents++;
        id = totalStudents;
    }

    void show() const {
        cout << "学生编号=" << id << "，姓名=" << name << endl;
    }

    static int getTotalStudents() {
        return totalStudents;
    }
};

// static 成员变量必须在类外定义一次
// 因为它不属于某个对象，而是属于整个类
int Student::totalStudents = 0;

void demoStaticVariable() {
    Student s1("Alice");
    Student s2("Bob");
    Student s3("Cindy");

    s1.show();
    s2.show();
    s3.show();

    cout << "当前已创建学生对象总数：" << Student::getTotalStudents() << endl;
}

// ============================================================
// 第二部分：共享数据的直观理解
// ============================================================

class Counter {
private:
    static int count;

public:
    Counter() {
        count++;
    }

    static int getCount() {
        return count;
    }
};

int Counter::count = 0;

void demoSharedData() {
    cout << "创建对象前，count = " << Counter::getCount() << endl;

    Counter c1;
    cout << "创建 c1 后，count = " << Counter::getCount() << endl;

    Counter c2;
    cout << "创建 c2 后，count = " << Counter::getCount() << endl;

    Counter c3;
    cout << "创建 c3 后，count = " << Counter::getCount() << endl;

    cout << "结论：count 不属于某个对象，而是整个 Counter 类共享" << endl;
}

// ============================================================
// 第三部分：static 成员函数的作用与限制
// ============================================================
// static 成员函数属于类本身，不属于某个对象
// 所以它没有 this 指针，只能直接访问 static 成员

class ToolBox {
private:
    int value;
    static int toolVersion;

public:
    ToolBox(int value) : value(value) {}

    static void showVersion() {
        cout << "工具版本号：" << toolVersion << endl;
        cout << "说明：static 成员函数可以直接访问 static 成员变量" << endl;
        // cout << value << endl; // 错误示例：不能直接访问普通成员变量
    }

    void showValue() const {
        cout << "对象自己的 value = " << value << endl;
    }
};

int ToolBox::toolVersion = 101;

void demoStaticFunction() {
    ToolBox box(88);

    ToolBox::showVersion();
    box.showValue();

    cout << "结论：static 成员函数没有 this，因此不能直接访问普通成员" << endl;
}

// ============================================================
// 第四部分：访问方式对比
// ============================================================
// 推荐用 类名::成员 的方式访问 static 成员
// 虽然对象也能访问，但容易让人误以为它属于对象

class Company {
private:
    string employeeName;
    static string companyName;

public:
    Company(const string& employeeName) : employeeName(employeeName) {}

    void showEmployee() const {
        cout << "员工姓名：" << employeeName << endl;
    }

    static void showCompanyName() {
        cout << "公司名称：" << companyName << endl;
    }
};

string Company::companyName = "ClaudeCode Tech";

void compareAccessWays() {
    Company employee("David");

    employee.showEmployee();

    cout << "推荐方式：" << endl;
    Company::showCompanyName();

    cout << "也能通过对象调用，但不推荐：" << endl;
    employee.showCompanyName();

    cout << "原因：companyName 属于 Company 类，不属于某个 employee 对象" << endl;
}

// ============================================================
// 第五部分：学习提示
// ============================================================

void printHints() {
    cout << "1. static 成员变量属于类本身，所有对象共享同一份数据" << endl;
    cout << "2. static 成员变量通常需要在类外单独定义一次" << endl;
    cout << "3. static 成员函数没有 this 指针" << endl;
    cout << "4. 所以 static 成员函数只能直接访问 static 成员" << endl;
    cout << "5. 访问 static 成员时，优先写成 类名::成员，更清晰" << endl;
}

// ============================================================
// 主函数
// ============================================================

int main() {
    cout << "========================================" << endl;
    cout << "       static 成员学习" << endl;
    cout << "========================================" << endl;

    cout << "\n=== 第一部分：static 成员变量 ===" << endl;
    demoStaticVariable();

    cout << "\n=== 第二部分：共享数据理解 ===" << endl;
    demoSharedData();

    cout << "\n=== 第三部分：static 成员函数 ===" << endl;
    demoStaticFunction();

    cout << "\n=== 第四部分：访问方式对比 ===" << endl;
    compareAccessWays();

    cout << "\n=== 第五部分：学习提示 ===" << endl;
    printHints();

    cout << "\n=== 核心总结 ===" << endl;
    cout << "1. static 成员变量属于类，所有对象共享" << endl;
    cout << "2. static 很适合表示计数器、全局配置、共享状态" << endl;
    cout << "3. static 成员函数属于类本身，没有 this 指针" << endl;
    cout << "4. 因为没有 this，所以 static 成员函数不能直接访问普通成员" << endl;
    cout << "5. 理解 static 的关键是分清：哪些数据属于对象，哪些数据属于整个类" << endl;

    return 0;
}
