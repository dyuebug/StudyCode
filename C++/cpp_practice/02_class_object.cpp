// ============================================
// 02_class_object.cpp - 类与对象练习
// 日期：2026-06-15（第2天）
// 内容：类的定义、构造函数、析构函数、封装
// ============================================

#include <iostream>
#include <cstring>

using std::cout;
using std::endl;
using std::string;

// ============================================
// 练习1：简单的 Point 类（访问控制）
// ============================================

class Point {
private:
    // 私有数据成员（只能在类内访问）
    int _x;
    int _y;

public:
    // 公有成员函数（类外可以访问）

    // 设置坐标
    void setPoint(int x, int y) {
        _x = x;
        _y = y;
    }

    // 获取 x 坐标
    int getX() {
        return _x;
    }

    // 获取 y 坐标
    int getY() {
        return _y;
    }

    // 打印坐标
    void print() {
        cout << "(" << _x << ", " << _y << ")" << endl;
    }
};

void test_point() {
    cout << "\n=== 练习1：Point 类（访问控制）===" << endl;

    Point pt;
    // pt._x = 10;  // ❌ 错误！_x 是私有的，类外不能访问

    // ✅ 正确：通过公有函数访问私有数据
    pt.setPoint(10, 20);
    cout << "x = " << pt.getX() << ", y = " << pt.getY() << endl;
    pt.print();
}

// ============================================
// 练习2：Student 类（构造函数）
// ============================================

class Student {
private:
    string _name;
    int _age;
    string _id;

public:
    // 默认构造函数
    Student() {
        cout << "调用默认构造函数 Student()" << endl;
        _name = "未知";
        _age = 0;
        _id = "000000";
    }

    // 带参数的构造函数
    Student(string name, int age, string id) {
        cout << "调用构造函数 Student(string, int, string)" << endl;
        _name = name;
        _age = age;
        _id = id;
    }

    // 析构函数
    ~Student() {
        cout << "调用析构函数 ~Student()：" << _name << endl;
    }

    // 打印学生信息
    void print() {
        cout << "姓名：" << _name
             << ", 年龄：" << _age
             << ", 学号：" << _id << endl;
    }

    // 设置方法
    void setName(string name) { _name = name; }
    void setAge(int age) { _age = age; }
    void setId(string id) { _id = id; }

    // 获取方法
    string getName() { return _name; }
    int getAge() { return _age; }
    string getId() { return _id; }
};

void test_student() {
    cout << "\n=== 练习2：Student 类（构造函数）===" << endl;

    // 使用默认构造函数
    Student s1;
    s1.print();

    // 使用带参数的构造函数
    Student s2("张三", 20, "202401");
    s2.print();

    // 修改学生信息
    s1.setName("李四");
    s1.setAge(21);
    s1.setId("202402");
    cout << "\n修改后的 s1：" << endl;
    s1.print();

    cout << "\n离开作用域，析构函数将被调用..." << endl;
}

// ============================================
// 练习3：Rectangle 类（初始化列表）
// ============================================

class Rectangle {
private:
    int _width;
    int _height;

public:
    // 使用初始化列表（推荐）⭐⭐⭐⭐⭐
    Rectangle(int width = 0, int height = 0)
    : _width(width)    // 直接初始化，不是赋值
    , _height(height)
    {
        cout << "创建矩形：宽=" << _width << ", 高=" << _height << endl;
    }

    ~Rectangle() {
        cout << "销毁矩形：宽=" << _width << ", 高=" << _height << endl;
    }

    // 计算面积
    int area() {
        return _width * _height;
    }

    // 计算周长
    int perimeter() {
        return 2 * (_width + _height);
    }

    void print() {
        cout << "矩形：宽=" << _width
             << ", 高=" << _height
             << ", 面积=" << area()
             << ", 周长=" << perimeter() << endl;
    }
};

void test_rectangle() {
    cout << "\n=== 练习3：Rectangle 类（初始化列表）===" << endl;

    Rectangle r1;           // 默认参数：0, 0
    Rectangle r2(10);       // 默认参数：10, 0
    Rectangle r3(10, 20);   // 完整参数

    r1.print();
    r2.print();
    r3.print();

    cout << "\n离开作用域，析构函数将被调用..." << endl;
}

// ============================================
// 练习4：析构函数的调用时机
// ============================================

class Demo {
private:
    string _name;

public:
    Demo(string name) : _name(name) {
        cout << "创建对象：" << _name << endl;
    }

    ~Demo() {
        cout << "销毁对象：" << _name << endl;
    }
};

// 全局对象
Demo global_obj("全局对象");

void test_destructor_timing() {
    cout << "\n=== 练习4：析构函数的调用时机 ===" << endl;

    // 局部对象
    Demo local_obj("局部对象");

    // 静态对象
    static Demo static_obj("静态对象");

    // 堆对象
    Demo* heap_obj = new Demo("堆对象");

    cout << "\n手动 delete 堆对象..." << endl;
    delete heap_obj;  // 堆对象：delete 时析构

    cout << "\n即将离开函数作用域..." << endl;
    // 局部对象：离开作用域时析构
    // 静态对象：程序结束时析构
    // 全局对象：程序结束时析构
}

// ============================================
// 练习5：封装的价值（对比）
// ============================================

// 不好的设计：数据成员 public
class BadStudent {
public:
    string name;
    int age;
};

// 好的设计：数据成员 private，提供接口
class GoodStudent {
private:
    string _name;
    int _age;

public:
    GoodStudent(string name, int age) : _name(name), _age(age) {}

    // 控制 age 的设置（防止非法值）
    void setAge(int age) {
        if (age < 0 || age > 150) {
            cout << "❌ 年龄无效！" << endl;
            return;
        }
        _age = age;
    }

    int getAge() { return _age; }

    void print() {
        cout << "姓名：" << _name << ", 年龄：" << _age << endl;
    }
};

void test_encapsulation() {
    cout << "\n=== 练习5：封装的价值 ===" << endl;

    // 不好的设计
    cout << "\n❌ 不好的设计（public 数据）：" << endl;
    BadStudent bs;
    bs.name = "张三";
    bs.age = -10;  // 可以设置非法值！
    cout << "姓名：" << bs.name << ", 年龄：" << bs.age << endl;

    // 好的设计
    cout << "\n✅ 好的设计（private + 接口）：" << endl;
    GoodStudent gs("李四", 20);
    gs.print();

    cout << "\n尝试设置非法年龄..." << endl;
    gs.setAge(-10);  // 会被拒绝！
    gs.print();

    cout << "\n设置合法年龄..." << endl;
    gs.setAge(21);
    gs.print();
}

// ============================================
// 练习6：构造函数重载
// ============================================

class Book {
private:
    string _title;
    string _author;
    double _price;

public:
    // 重载1：默认构造函数
    Book() : _title("未知"), _author("未知"), _price(0.0) {
        cout << "调用 Book()" << endl;
    }

    // 重载2：只提供书名
    Book(string title) : _title(title), _author("未知"), _price(0.0) {
        cout << "调用 Book(string)" << endl;
    }

    // 重载3：提供书名和作者
    Book(string title, string author)
    : _title(title), _author(author), _price(0.0) {
        cout << "调用 Book(string, string)" << endl;
    }

    // 重载4：提供所有信息
    Book(string title, string author, double price)
    : _title(title), _author(author), _price(price) {
        cout << "调用 Book(string, string, double)" << endl;
    }

    void print() {
        cout << "《" << _title << "》 - " << _author
             << " - ¥" << _price << endl;
    }
};

void test_constructor_overload() {
    cout << "\n=== 练习6：构造函数重载 ===" << endl;

    Book b1;
    Book b2("C++ Primer");
    Book b3("Effective C++", "Scott Meyers");
    Book b4("STL源码剖析", "侯捷", 99.0);

    cout << "\n书籍信息：" << endl;
    b1.print();
    b2.print();
    b3.print();
    b4.print();
}

// ============================================
// 主函数
// ============================================

int main() {
    cout << "========================================" << endl;
    cout << "     类与对象综合练习" << endl;
    cout << "========================================" << endl;

    // 练习1：访问控制
    test_point();

    // 练习2：构造函数和析构函数
    test_student();

    // 练习3：初始化列表
    test_rectangle();

    // 练习4：析构函数调用时机
    test_destructor_timing();

    // 练习5：封装的价值
    test_encapsulation();

    // 练习6：构造函数重载
    test_constructor_overload();

    cout << "\n========================================" << endl;
    cout << "程序即将结束，全局对象和静态对象将被销毁..." << endl;
    cout << "========================================" << endl;

    return 0;
}

/*
预期输出要点：

1. 私有成员不能直接访问，需要通过公有接口
2. 构造函数在对象创建时自动调用
3. 析构函数在对象销毁时自动调用
4. 初始化列表是推荐的初始化方式
5. 封装可以控制数据访问，防止非法值
6. 构造函数可以重载，提供多种创建对象的方式

析构顺序：
- 局部对象：离开作用域立即析构
- 堆对象：delete 时析构
- 静态对象和全局对象：程序结束时析构（后创建的先析构）
*/
